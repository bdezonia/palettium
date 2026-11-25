/* Palettium Win16 GDI entry-point staging implementation.
 * Compiled as a Win16 object only. NOT AN INSTALLABLE DISPLAY DRIVER.
 * Shared drawing/DDI marshaling is card-agnostic; native mode changes are
 * delegated to the existing ET4000AX session and Win16 platform bridge.
 * Enable(style=0) is blocked until mandatory display services are ready.
 */
/* SDK names sometimes overlap Win16 display-DDI exports. */
#define Control PalWinApiControl
#define EnumDFonts PalWinApiEnumDFonts
#define EnumObj PalWinApiEnumObj
#define GetCharWidth PalWinApiGetCharWidth
#define DeviceBitmap PalWinApiDeviceBitmap
#define FastBorder PalWinApiFastBorder
#define SetAttribute PalWinApiSetAttribute
#define Inquire PalWinApiInquire
#define SetCursor PalWinApiSetCursor
#define MoveCursor PalWinApiMoveCursor
#define CheckCursor PalWinApiCheckCursor
#define ExtTextOut PalWinApiExtTextOut
#define StrBlt PalWinApiStrBlt
#define RealizeObject PalWinApiRealizeObject
#define BitBlt PalWinApiBitBlt
#define ColorInfo PalWinApiColorInfo
#define ScanLR PalWinApiScanLR
#define SetPalette PalWinApiSetPalette
#define GetPalette PalWinApiGetPalette
#define SetPalTrans PalWinApiSetPalTrans
#define GetPalTrans PalWinApiGetPalTrans
#define UpdateColors PalWinApiUpdateColors
#include <windows.h>
#undef Control
#undef EnumDFonts
#undef EnumObj
#undef GetCharWidth
#undef DeviceBitmap
#undef FastBorder
#undef SetAttribute
#undef Inquire
#undef SetCursor
#undef MoveCursor
#undef CheckCursor
#undef ExtTextOut
#undef StrBlt
#undef RealizeObject
#undef BitBlt
#undef ColorInfo
#undef ScanLR
#undef SetPalette
#undef GetPalette
#undef SetPalTrans
#undef GetPalTrans
#undef UpdateColors
#include <dos.h>
#include "pal_ddl_blit.h"
#include "pal_enable.h"
#include "pal_et4k_win16.h"
#include "pal_vdd.h"
#include "pal_objects.h"
#include "pal_bitmap_device.h"
#include "rg_gdi_blit.h"
#include "rg_gdi_text.h"
#include "pal_font16.h"
#include "pal_cursor16.h"
#include "pal_trial_gate.h"
#include "pal_boottrace.h"

static PAL_ScreenHost *pal_active_host = 0;
#if defined(PAL_WIN31_BOOTTRACE)
static unsigned pal_ddi_trace_budget = 256u;
static unsigned pal_color_trace_budget = 24u;
static int pal_ddi_trace_enter(const char *name)
{
    if (!pal_ddi_trace_budget) return 0;
    --pal_ddi_trace_budget;
    pal_boottrace(name);
    return 1;
}
#endif

/* Persistent storage: GDI owns only the published 20-byte PDEVICE. */
static PAL_ET4KSession pal_native_session;
static PAL_Win16ET4K pal_native_adapter;
/* Driver-owned, persistent: never allocate a 4 KiB saved-under block
 * on the small Win16 stack. Used only after GDI publishes a screen host. */
static PAL_Cursor16 pal_win16_cursor;

static void pal_cursor_draw_begin(void)
{
    if (pal_active_host && pal_win16_cursor.screen ==
            &pal_active_host->device)
        pal_cursor16_begin_draw(&pal_win16_cursor);
}
static void pal_cursor_draw_end(void)
{
    if (pal_win16_cursor.draw_depth)
        pal_cursor16_end_draw(&pal_win16_cursor);
}


/* GDI-owned PDEVICE handoff, used by the dormant transactional startup.
 * GDI allocates a far PDEVICE according to GDIINFO.dpDEVICEsize. The
 * framebuffer session remains in persistent driver storage; only the
 * 20-byte wire PDEVICE is copied into GDI's buffer. Never cast the GDI
 * FAR address to a near PAL_PDEVICE pointer, and never bind an unverified
 * host or a token not identical to the destination address.
 *
 * The transactional Win16 startup uses this ONLY after BIOS, framebuffer
 * and palette startup complete. No video-mode change happens here. */
int FAR PASCAL pal_win16_publish_host(PAL_ScreenHost *host,
                                      LPVOID gdi_pdevice)
{
    PAL_PDEVICE image;
    PAL_U32 token;
    const unsigned char *source;
    unsigned char FAR *target;
    unsigned i;
    if (pal_active_host || !gdi_pdevice ||
        (PAL_U32)(PAL_U16)FP_OFF(gdi_pdevice) +
        (PAL_U32)sizeof(image) > 65536UL) return 0;
    token=pal_ddi_far_token((PAL_U16)FP_SEG(gdi_pdevice),
                            (PAL_U16)FP_OFF(gdi_pdevice));
    if (!pal_screen_host_gdi_snapshot(host,token,&image)) {
        pal_boottrace("PUBLISH SNAPSHOT FAIL");
        return 0;
    }
    source=(const unsigned char *)&image;
    target=(unsigned char FAR *)gdi_pdevice;
    for (i=0; i<sizeof(image); ++i) target[i]=source[i];
    pal_cursor16_bind(&pal_win16_cursor,&host->device);
    pal_active_host=host;
    pal_boottrace_hex("PDEVICE TYPE", (unsigned)target[0] | ((unsigned)target[1]<<8));
    pal_boottrace_hex("PDEVICE SIZE", sizeof(image));
    pal_boottrace("PUBLISH OK");
    return 1;
}

/* Complete hardware setup -> validated PDEVICE copy -> DDI binding as
 * one transaction. Publication is the sole exposure of the live host.
 * The portable coordinator rolls back hardware if this copy fails.
 * This path is gated OFF in PALSTAGE. PALTRIAL may opt in under Standard
 * mode alone; Windows real-world GDI, font and IRQ behavior is unproven. */
typedef struct PAL_Win16Commit {
    LPVOID gdi_far_destination;
} PAL_Win16Commit;

static int pal_win16_publish_transaction(void *context,
                                         const PAL_PDEVICE *image)
{
    PAL_Win16Commit *commit=(PAL_Win16Commit *)context;
    (void)image; /* publisher re-checks host and token before FAR writes */
    return commit && pal_win16_publish_host(&pal_native_session.host,
                                             commit->gdi_far_destination);
}

static int pal_win16_activate(LPVOID destination)
{
    PAL_ET4KPlan plan;
    ET4K_Port port;
    PAL_Win16Commit commit;
    PAL_U32 token;
    if (!destination || pal_active_host ||
        !pal_win16_et4k_profile_plan(&plan)) {
        pal_boottrace("ACT PLAN FAIL");
        return 0;
    }
    pal_boottrace("ACT PLAN OK");
    /* Platform-adapter lookup is deliberately after the pure planning
     * preflight; no A000 selector or I/O is touched during style=1. */
    if (plan.vdd_assessment != PAL_VDD_NOT_ENHANCED ||
        (PAL_U32)(PAL_U16)FP_OFF(destination) +
        (PAL_U32)sizeof(PAL_PDEVICE) > 65536UL) {
        pal_boottrace("ACT PRECHECK FAIL");
        return 0;
    }
    if (!pal_win16_et4k_port(&pal_native_adapter,&port)) {
        pal_boottrace("ACT PORT FAIL");
        return 0;
    }
    pal_boottrace("ACT PORT OK");
    token=pal_ddi_far_token((PAL_U16)FP_SEG(destination),
                            (PAL_U16)FP_OFF(destination));
    /* context points to a typed stack object that HOLDS the FAR pointer.
     * Never cast GDI's FAR LPVOID directly to the callback's untyped void*. */
    commit.gdi_far_destination=destination;
    {
        int succeeded=pal_et4k_session_start_published(&pal_native_session,
                       port,&plan,token,pal_win16_publish_transaction,
                       &commit,1);
        pal_boottrace(succeeded ? "ACT SESSION OK" : "ACT SESSION FAIL");
        return succeeded;
    }
}

/* DDK Win3.x cursor DDI. Inquire is called before Enable, so it does
 * not require a screen host. The shape wire header is 6 signed WORDs,
 * followed by height*4 AND bytes and height*4 XOR bytes. Preflight
 * segment bounds and copy to near storage before touching old cursor. */
WORD FAR PASCAL Inquire(LPVOID information)
{
    WORD FAR *out=(WORD FAR *)information;
    pal_boottrace("INQUIRE ENTER");
    if (!information || (PAL_U32)(PAL_U16)FP_OFF(information)+4UL>65536UL)
        return 0;
    out[0]=1u; /* IBM PC Microsoft Mouse X mickeys-per-pixel */
    out[1]=2u; /* Y */
    pal_boottrace("INQUIRE OK");
    return 4u;
}
void FAR PASCAL SetCursor(LPVOID shape)
{
    unsigned char header[PAL_CURSOR16_HEADER];
    unsigned char raw[PAL_CURSOR16_MAX_WIRE];
    const unsigned char FAR *far_bytes=(const unsigned char FAR *)shape;
    unsigned available,need,i;
    PAL_U32 remaining;
    if (!shape) { (void)pal_cursor16_set(&pal_win16_cursor,0,0); return; }
    remaining=65536UL-(PAL_U32)(PAL_U16)FP_OFF(shape);
    if (remaining<(PAL_U32)PAL_CURSOR16_HEADER) return;
    available=(unsigned)(remaining>(PAL_U32)PAL_CURSOR16_MAX_WIRE ?
                (PAL_U32)PAL_CURSOR16_MAX_WIRE : remaining);
    for (i=0;i<PAL_CURSOR16_HEADER;++i) header[i]=far_bytes[i];
    need=pal_cursor16_wire_size(header,available);
    if (!need) return;
    for (i=0;i<need;++i) raw[i]=far_bytes[i];
    (void)pal_cursor16_set(&pal_win16_cursor,raw,need);
}
/* No device access at mouse-interrupt time. CheckCursor is called at
 * timer interrupt time; on a released drawing transaction it may apply
 * queued movement. This is a staging implementation: native nested-mouse
 * critical-region handling remains unverified, so Enable stays gated. */
void FAR PASCAL MoveCursor(WORD x,WORD y)
{
    pal_cursor16_post_move(&pal_win16_cursor,(short)x,(short)y);
}
void FAR PASCAL CheckCursor(void)
{
    pal_cursor16_check(&pal_win16_cursor);
}

/* ONLY the separately named PALTRIAL.DRV build may request a first GDI
 * boot in a disposable 86Box guest. Default PALSTAGE.DRV always refuses.
 * Require BOTH the explicit trial key and actual Windows Standard mode,
 * plus explicit 256 or 512 KiB configured VRAM; no enhanced-mode VDD paths permitted.
 * This is an experimental boot gate, NOT a claim of driver correctness. */
static int pal_win16_install_ready(void)
{
#ifdef PAL_WIN31_TRIAL_MODE
    unsigned long flags=(unsigned long)GetWinFlags();
    unsigned vram=(unsigned)GetPrivateProfileInt("Palettium","VRAMKiB",0,
                                                "SYSTEM.INI");
    unsigned key=(unsigned)GetPrivateProfileInt("Palettium","TrialEnable",0,
                                               "SYSTEM.INI");
    int allowed=pal_win31_trial_permitted(flags,vram,key);
#ifdef PAL_WIN31_BOOTTRACE
    char msg[]="GATE FLAGS=00000000 VRAM=0000 KEY=0000";
    static const char hex[]="0123456789ABCDEF";
    unsigned i;
    for (i=0;i<8u;++i)
        msg[18u-i]=hex[(unsigned)(flags >> (4u*i)) & 15u];
    for (i=0;i<4u;++i) {
        msg[28u-i]=hex[(vram >> (4u*i)) & 15u];
        msg[37u-i]=hex[(key >> (4u*i)) & 15u];
    }
    pal_boottrace(msg);
#endif
    pal_boottrace(allowed ? "GATE OK" : "GATE BLOCKED");
    return allowed;
#else
    return 0;
#endif
}

/* GDI memory bitmap operands are segmented FAR BITMAP16 headers. Decode
 * them only after checking the first WORD and the 64 KiB header boundary.
 * The bits themselves are accessed with offset-preserving callbacks, never
 * by casting a Win16 far pointer to a flat/near RG_Surface.pixels. */
typedef struct PAL_Win16Bits {
    PAL_U32 address;
} PAL_Win16Bits;

static rg_u8 pal_win16_bits_read(void *ctx, rg_offset offset)
{
    PAL_Win16Bits *bits = (PAL_Win16Bits *)ctx;
    const rg_u8 FAR *p=(const rg_u8 FAR *)MK_FP(
        (unsigned)(bits->address >> 16),
        (unsigned)((bits->address & 65535UL) + offset));
    return *p;
}

static void pal_win16_bits_write(void *ctx, rg_offset offset, rg_u8 value)
{
    PAL_Win16Bits *bits = (PAL_Win16Bits *)ctx;
    rg_u8 FAR *p=(rg_u8 FAR *)MK_FP((unsigned)(bits->address >> 16),
        (unsigned)((bits->address & 65535UL) + offset));
    *p=value;
}

#if defined(PAL_WIN31_BOOTTRACE)
static unsigned pal_bitmap_fail_budget=8u;
static unsigned pal_mode_fail_budget=8u;
static unsigned pal_text_fail_budget=12u;
static DWORD pal_text_fail(unsigned reason)
{
    if (pal_text_fail_budget) {
        --pal_text_fail_budget;
        pal_boottrace_hex("TEXT FAIL REASON",reason);
    }
    return 0x80000000UL;
}
#else
#define pal_text_fail(reason) (0x80000000UL)
#endif

static int pal_win16_device(LPVOID operand, RG_Device *storage,
                             PAL_Win16Bits *bits, RG_Device **result)
{
    PAL_U32 token;
    PAL_BITMAP16 bitmap;
    const unsigned char FAR *from;
    unsigned char *to;
    RG_VramOps io;
    unsigned i;
    if (!pal_active_host || !pal_active_host->running || !operand ||
        !pal_screen_host_output_ready(pal_active_host,
                     pal_active_host->ddi.screen_token)) return 0;
    token=pal_ddi_far_token((PAL_U16)FP_SEG(operand),
                            (PAL_U16)FP_OFF(operand));
    if (token == pal_active_host->ddi.screen_token) {
        *result=&pal_active_host->device;
        return 1;
    }
    if ((PAL_U32)(PAL_U16)FP_OFF(operand) +
        (PAL_U32)sizeof(bitmap)>65536UL ||
        *(const WORD FAR *)operand != 0u) return 0;
    from=(const unsigned char FAR *)operand;
    to=(unsigned char *)&bitmap;
    for (i=0; i<sizeof(bitmap); ++i) to[i]=from[i];
    if (!pal_ddi_memory_bitmap_ok(&bitmap)) {
#if defined(PAL_WIN31_BOOTTRACE)
        if (pal_bitmap_fail_budget) {
            --pal_bitmap_fail_budget;
            pal_boottrace_hex("BITMAP REJECT WIDTH",(unsigned)bitmap.bmWidth);
            pal_boottrace_hex("BITMAP HEIGHT",(unsigned)bitmap.bmHeight);
            pal_boottrace_hex("BITMAP STRIDE",(unsigned)bitmap.bmWidthBytes);
            pal_boottrace_hex("BITMAP PLANES",(unsigned)bitmap.bmPlanes);
            pal_boottrace_hex("BITMAP BPP",(unsigned)bitmap.bmBitsPixel);
            pal_boottrace_hex("BITMAP BITS OFF",(unsigned)bitmap.bmBits);
            pal_boottrace_hex("BITMAP BITS SEL",(unsigned)(bitmap.bmBits>>16));
            pal_boottrace_hex("BITMAP PLANE BYTES LO",(unsigned)bitmap.bmWidthPlanes);
            pal_boottrace_hex("BITMAP PLANE BYTES HI",(unsigned)(bitmap.bmWidthPlanes>>16));
            pal_boottrace_hex("BITMAP SEG INDEX",(unsigned)bitmap.bmSegmentIndex);
            pal_boottrace_hex("BITMAP SCAN SEG",(unsigned)bitmap.bmScanSegment);
            pal_boottrace_hex("BITMAP FILL BYTES",(unsigned)bitmap.bmFillBytes);
        }
#endif
        return 0;
    }
    bits->address=bitmap.bmBits;
    io.read_byte=pal_win16_bits_read;
    io.write_byte=pal_win16_bits_write;
    if (!pal_ddi_bitmap_device(storage,&bitmap,io,bits)) return 0;
    *result=storage;
    return 1;
}

/* The Win3.x DRAWMODE begins with Rop2, bkMode, bkColor (physical
 * DWORD) and TextColor (physical DWORD). For mixed-depth bitmap blits
 * we need both physical indices. Return failure for absent/invalid data;
 * never guess colors when a conversion would change the result. */
static int pal_win16_mode_colors(LPVOID mode, unsigned *text,
                                 unsigned *background)
{
    const WORD FAR *w=(const WORD FAR *)mode;
    PAL_U32 bg, fg;
    if (!mode || !text || !background ||
        (PAL_U32)(PAL_U16)FP_OFF(mode)+12UL>65536UL) return 0;
    bg=(PAL_U32)w[2] | ((PAL_U32)w[3] << 16);
    fg=(PAL_U32)w[4] | ((PAL_U32)w[5] << 16);
    if (bg > 255UL || fg > 255UL) {
#if defined(PAL_WIN31_BOOTTRACE)
        if (pal_mode_fail_budget) {
            --pal_mode_fail_budget;
            pal_boottrace_hex("MODE REJECT BG LO",(unsigned)bg);
            pal_boottrace_hex("MODE REJECT BG HI",(unsigned)(bg>>16));
            pal_boottrace_hex("MODE REJECT FG LO",(unsigned)fg);
            pal_boottrace_hex("MODE REJECT FG HI",(unsigned)(fg>>16));
        }
#endif
        return 0;
    }
    *background=(unsigned)bg;
    *text=(unsigned)fg;
    return 1;
}


/* Standard Win3.x raster-font text. Glyphs remain in GDI's own FAR font
 * segment; the shared text renderer consumes only verified column-major
 * bytes via a small callback. We intentionally do not advertise RC_BIGFONT,
 * TrueType sparse glyphs, rotation, or font-synthesis capabilities. */
typedef struct PAL_Win16FontBytes {
    PAL_U16 selector;
    PAL_U16 base;
} PAL_Win16FontBytes;

static rg_u8 pal_win16_font_read(void *context, rg_offset offset)
{
    PAL_Win16FontBytes *font=(PAL_Win16FontBytes *)context;
    const rg_u8 FAR *p=(const rg_u8 FAR *)MK_FP(font->selector,
                          (unsigned)((PAL_U32)font->base+(PAL_U32)offset));
    return *p;
}

/* Glyph offsets are segment-relative, unlike FONTINFO field offsets. */
static rg_u8 pal_win16_glyph_read(void *context, rg_offset offset)
{
    PAL_Win16FontBytes *font=(PAL_Win16FontBytes *)context;
    const rg_u8 FAR *p=(const rg_u8 FAR *)MK_FP(font->selector,
                                               (unsigned)offset);
    return *p;
}

static int pal_win16_rect(LPVOID source, RG_Rect *rect)
{
    const short FAR *p;
    if (!source || !rect ||
        (PAL_U32)(PAL_U16)FP_OFF(source)+8UL>65536UL) return 0;
    p=(const short FAR *)source;
    rect->left=p[0]; rect->top=p[1];
    rect->right=p[2]; rect->bottom=p[3];
    return rect->left<=rect->right && rect->top<=rect->bottom;
}

/* Return x-extent in low word and y-extent in high word. Error is the
 * documented 80000000h sentinel. A width > 32767 clamps to 32767. */
static DWORD pal_win16_text(LPVOID dest, short x, short y,
                            LPVOID clip_rect, LPSTR text, short count,
                            LPVOID font_address, LPVOID drawmode,
                            LPVOID text_transform, LPVOID widths,
                            LPVOID opaque_rect, WORD options)
{
    RG_Device mem, *target;
    PAL_Win16Bits bits;
    PAL_Win16FontBytes fb;
    PAL_Font16 font;
    RG_Glyph glyph;
    RG_Rect clip, opaque, text_box;
    RG_Rect *clip_use=0;
    const short FAR *mode=(const short FAR *)drawmode;
    const short FAR *dx=(const short FAR *)widths;
    const unsigned char FAR *str=(const unsigned char FAR *)text;
    unsigned n,i, fg,bg, bgmode, code;
    long cursor, next, minimum, maximum, advance;
    long space_adjust, min_end, max_end;
    int pass;
    (void)text_transform; /* string precision, no arbitrary rotation */
    if ((options & (WORD)~6u) != 0u) return pal_text_fail(101u);
    if (!pal_active_host || !dest) return pal_text_fail(102u);
    if (!pal_win16_device(dest,&mem,&bits,&target)) return pal_text_fail(103u);
    if (!pal_win16_mode_colors(drawmode,&fg,&bg)) return pal_text_fail(104u);
    if ((PAL_U32)(PAL_U16)FP_OFF(drawmode)+24UL>65536UL) return pal_text_fail(105u);
    bgmode=(unsigned)mode[1];
    if (bgmode!=1u && bgmode!=2u) return pal_text_fail(1u);
    if (target->surface.format==RG_FMT_INDEX1) {
        fg=rg_gdi_mono_color(fg,bg);
        bg=1u; /* GDI monochrome: set bit is background */
    }
    if (clip_rect) {
        if (!pal_win16_rect(clip_rect,&clip)) return pal_text_fail(2u);
        clip_use=&clip;
    }
    if (options & 6u) {
        if (!opaque_rect || !pal_win16_rect(opaque_rect,&opaque))
            return pal_text_fail(3u);
        if (options & 4u) {
            if (!clip_use) { clip=opaque; clip_use=&clip; }
            else {
                if (clip.left<opaque.left) clip.left=opaque.left;
                if (clip.top<opaque.top) clip.top=opaque.top;
                if (clip.right>opaque.right) clip.right=opaque.right;
                if (clip.bottom>opaque.bottom) clip.bottom=opaque.bottom;
            }
        }
    }
    if (count==0) {
        if (!(options & 2u)) return pal_text_fail(4u);
        return rg_gdi_text_fill(target,opaque,clip_use,bg) ?
               1UL : pal_text_fail(5u);
    }
    if (count == (-32767-1)) return pal_text_fail(6u);
    n=(unsigned)(count<0 ? -count : count);
    if (!text || !font_address || !n ||
        (PAL_U32)(PAL_U16)FP_OFF(text)+(PAL_U32)n>65536UL ||
        (widths && (PAL_U32)(PAL_U16)FP_OFF(widths)+
                      (PAL_U32)n*2UL>65536UL))
        return pal_text_fail(7u);
    fb.selector=(PAL_U16)FP_SEG(font_address);
    fb.base=(PAL_U16)FP_OFF(font_address);
    if (!pal_font16_open(&font,pal_win16_font_read,&fb,
                          65536UL-(PAL_U32)fb.base))
        return pal_text_fail(8u);
#if defined(PAL_WIN31_BOOTTRACE)
    {
        static unsigned font_base_budget=8u;
        if (font_base_budget) {
            --font_base_budget;
            pal_boottrace_hex("FONT SEGMENT BASE OFFSET",fb.base);
            pal_boottrace_hex("FONT FIRST INPUT CHAR",(unsigned)str[0]);
            pal_boottrace_hex("FONT FIRST CHAR",font.first);
        }
    }
#endif
    /* Two passes: check EVERY glyph/advance before any pixel changes.
     * This also makes width-only calls fully read-only. */
    cursor=(long)x; minimum=cursor; maximum=cursor;
    for (pass=0;pass<2;++pass) {
        cursor=(long)x; minimum=cursor; maximum=cursor;
        for (i=0;i<n;++i) {
            code=(unsigned)str[i];
            if (!pal_font16_glyph(&font,pal_win16_font_read,&fb,code,&glyph)) {
#if defined(PAL_WIN31_BOOTTRACE)
                if (pal_text_fail_budget) {
                    rg_offset entry;
                    unsigned actual;
                    actual=(code<font.first || code>font.last) ?
                           font.default_char : code;
                    entry=PAL_FONT16_CHARTABLE_OFFSET+
                          (rg_offset)(actual-font.first)*4UL;
                    pal_boottrace_hex("FONT BASE",fb.base);
                    pal_boottrace_hex("FONT FIRST",font.first);
                    pal_boottrace_hex("FONT LAST",font.last);
                    pal_boottrace_hex("FONT MAX WIDTH",font.max_width);
                    pal_boottrace_hex("FONT FIXED WIDTH",font.fixed_width);
                    pal_boottrace_hex("FONT HEIGHT",font.height);
                    pal_boottrace_hex("FONT CHAR",actual);
                    if (entry+4UL<=font.segment_bytes) {
                        pal_boottrace_hex("FONT ENTRY WIDTH",
                            (unsigned)pal_win16_font_read(&fb,entry) |
                            ((unsigned)pal_win16_font_read(&fb,entry+1UL)<<8));
                        pal_boottrace_hex("FONT ENTRY OFFSET",
                            (unsigned)pal_win16_font_read(&fb,entry+2UL) |
                            ((unsigned)pal_win16_font_read(&fb,entry+3UL)<<8));
                    }
                }
#endif
                return pal_text_fail(9u);
            }
            if (cursor< -32768L || cursor>32767L) return pal_text_fail(10u);
            min_end=cursor;
            max_end=cursor+(long)glyph.width;
            if (min_end<minimum) minimum=min_end;
            if (max_end>maximum) maximum=max_end;
            advance=widths ? (long)dx[i] : (long)glyph.width;
            advance+=(long)mode[11]; /* CharExtra at DRAWMODE +22 */
            if (code==font.break_char) advance+=(long)mode[7];
            next=cursor+advance;
            if (next < -32768L || next>32767L) return pal_text_fail(11u);
            if (next<minimum) minimum=next;
            if (next>maximum) maximum=next;
            if (pass==1 && count>0 &&
                !rg_gdi_text_glyph(target,(int)cursor,y,clip_use,&glyph,
                                   pal_win16_glyph_read,&fb,fg))
                return pal_text_fail(12u);
            cursor=next;
        }
        if (pass==0) {
            if ((long)y+(long)font.height>32767L ||
                minimum < -32768L || maximum>32767L) return pal_text_fail(13u);
            if (count<0) break; /* measure, do not draw */
            text_box.left=(int)minimum;
            text_box.top=y;
            text_box.right=(int)maximum;
            text_box.bottom=(int)((long)y+(long)font.height);
            /* Opaque rectangle is independent of DRAWMODE background mode.
             * In normal opaque mode, fill the string's full bounding box. */
            if (options & 2u) {
                if (!rg_gdi_text_fill(target,opaque,clip_use,bg))
                    return pal_text_fail(14u);
            } else if (bgmode==2u) {
                if (!rg_gdi_text_fill(target,text_box,clip_use,bg))
                    return pal_text_fail(15u);
            }
        }
    }
    space_adjust=maximum-minimum;
    if (space_adjust<0L) space_adjust=0L;
    if (space_adjust>32767L) space_adjust=32767L;
    return ((DWORD)(WORD)font.height<<16) | (DWORD)(WORD)space_adjust;
}

#if defined(PAL_WIN31_BOOTTRACE)
static DWORD FAR PASCAL pal_trace_impl_ExtTextOut(LPVOID dest, short x, short y,
                            LPVOID clip_rect, LPSTR text, short count,
                            LPVOID font, LPVOID drawmode,
                            LPVOID text_transform, LPVOID widths,
                            LPVOID opaque_rect, WORD options)
#else
DWORD FAR PASCAL ExtTextOut(LPVOID dest, short x, short y,
                            LPVOID clip_rect, LPSTR text, short count,
                            LPVOID font, LPVOID drawmode,
                            LPVOID text_transform, LPVOID widths,
                            LPVOID opaque_rect, WORD options)
#endif
{
    DWORD result;
    if (count>0 || (options&2u)) pal_cursor_draw_begin();
    result=pal_win16_text(dest,x,y,clip_rect,text,count,font,drawmode,
                          text_transform,widths,opaque_rect,options);
    if (count>0 || (options&2u)) pal_cursor_draw_end();
    return result;
}
#if defined(PAL_WIN31_BOOTTRACE)
DWORD FAR PASCAL ExtTextOut(LPVOID dest, short x, short y,
                            LPVOID clip_rect, LPSTR text, short count,
                            LPVOID font, LPVOID drawmode,
                            LPVOID text_transform, LPVOID widths,
                            LPVOID opaque_rect, WORD options)
{
    DWORD result;
    int traced = pal_ddi_trace_enter("DDI ExtTextOut ENTER");
    if (traced) {
        pal_boottrace_hex("TEXT COUNT",(unsigned)count);
        pal_boottrace_hex("TEXT OPTIONS",(unsigned)options);
    }
    result = pal_trace_impl_ExtTextOut(dest, x, y, clip_rect, text, count, font, drawmode, text_transform, widths, opaque_rect, options);
    if (traced) pal_boottrace_hex("DDI ExtTextOut RETURN LO", (unsigned)result);
    if (traced) pal_boottrace_hex("DDI ExtTextOut RETURN HI", (unsigned)(result >> 16));
    return result;
}
#endif

/* Legacy DDI @11 is mandatory for pre-GDI20 clients; share exactly
 * the same checked implementation, without extended opaque options. */
#if defined(PAL_WIN31_BOOTTRACE)
static DWORD FAR PASCAL pal_trace_impl_StrBlt(LPVOID dest, short x, short y,
                         LPVOID clip_rect, LPSTR text, short count,
                         LPVOID font, LPVOID drawmode, LPVOID text_transform)
#else
DWORD FAR PASCAL StrBlt(LPVOID dest, short x, short y,
                         LPVOID clip_rect, LPSTR text, short count,
                         LPVOID font, LPVOID drawmode, LPVOID text_transform)
#endif
{
    DWORD result;
    if (count>0) pal_cursor_draw_begin();
    result=pal_win16_text(dest,x,y,clip_rect,text,count,font,drawmode,
                          text_transform,0,0,0);
    if (count>0) pal_cursor_draw_end();
    return result;
}
#if defined(PAL_WIN31_BOOTTRACE)
DWORD FAR PASCAL StrBlt(LPVOID dest, short x, short y,
                         LPVOID clip_rect, LPSTR text, short count,
                         LPVOID font, LPVOID drawmode, LPVOID text_transform)
{
    DWORD result;
    int traced = pal_ddi_trace_enter("DDI StrBlt ENTER");
    result = pal_trace_impl_StrBlt(dest, x, y, clip_rect, text, count, font, drawmode, text_transform);
    if (traced) pal_boottrace_hex("DDI StrBlt RETURN LO", (unsigned)result);
    if (traced) pal_boottrace_hex("DDI StrBlt RETURN HI", (unsigned)(result >> 16));
    return result;
}
#endif

/* DDK: BitBlt(dest,dx,dy,src,sx,sy,w,h,rop,brush,drawmode).
 * Same-screen keeps the existing hardware blit path. All 1/8bpp
 * memory combinations use shared software/hardware RG_Device operations.
 * Pattern-dependent ROP3 still refuses without writing. */
static BOOL pal_win16_bitblt_impl(LPVOID dest, short dx, short dy,
                       LPVOID src, short sx, short sy,
                       short width, short height, DWORD rop,
                       LPVOID brush, LPVOID drawmode)
{
    PAL_U32 dt, st;
    PAL_Win16Bits dbits, sbits;
    RG_Device dmem, smem, *d, *s;
    unsigned text=0, background=0;
    (void)brush;
    if (!pal_active_host || !dest || width <= 0 || height <= 0)
        return FALSE;
    /* Source-independent fills must not require a source bitmap. */
    if (((rop >> 16) & 255UL) == 0xF0UL ||
        ((rop >> 16) & 255UL) == 0x00UL ||
        ((rop >> 16) & 255UL) == 0xFFUL) {
        RG_Brush b;
        unsigned i, value;
        const unsigned char FAR *from;
        unsigned char *to;
        unsigned code=(unsigned)((rop >> 16) & 255UL);
        if (!pal_win16_device(dest,&dmem,&dbits,&d) || !d->ops.fill)
            return FALSE;
        if (code == 0xF0u) {
            if (!brush || (PAL_U32)(PAL_U16)FP_OFF(brush)+sizeof(b)>65536UL)
                return FALSE;
            from=(const unsigned char FAR *)brush;
            to=(unsigned char *)&b;
            for (i=0;i<sizeof(b);++i) to[i]=from[i];
            if (!rg_brush_valid(&b) || b.style != RG_BRUSH_SOLID) return FALSE;
            value=b.foreground;
            if (d->surface.format == RG_FMT_INDEX1) {
                if (!pal_win16_mode_colors(drawmode,&text,&background)) return FALSE;
                value=rg_gdi_mono_color(value,background);
            }
        } else value=code == 0 ? 0u : (d->surface.format == RG_FMT_INDEX1 ? 1u : 255u);
        d->ops.fill(d,dx,dy,width,height,value);
        return TRUE;
    }
    if (!src) return FALSE;
    dt=pal_ddi_far_token((PAL_U16)FP_SEG(dest),(PAL_U16)FP_OFF(dest));
    st=pal_ddi_far_token((PAL_U16)FP_SEG(src),(PAL_U16)FP_OFF(src));
    if (dt == pal_active_host->ddi.screen_token && dt == st)
        return pal_screen_host_bitblt(pal_active_host,dt,dx,dy,st,sx,sy,
                                      width,height,(unsigned long)rop)
                   ? TRUE : FALSE;
    if (!pal_win16_device(dest,&dmem,&dbits,&d) ||
        !pal_win16_device(src,&smem,&sbits,&s)) return FALSE;
    if (dt == st) s=d; /* preserve overlap direction for in-place blits */
    else if (d != &pal_active_host->device &&
             s != &pal_active_host->device &&
             dbits.address == sbits.address) {
        /* Two distinct BITMAP headers may alias the same backing memory.
         * Treat truly identical surfaces as one device for overlap-safe
         * copies; refuse conflicting views of the same bytes. */
        if (d->surface.width != s->surface.width ||
            d->surface.height != s->surface.height ||
            d->surface.stride != s->surface.stride ||
            d->surface.format != s->surface.format) return FALSE;
        s=d;
    }
    if (d->surface.format != s->surface.format &&
        !pal_win16_mode_colors(drawmode,&text,&background)) return FALSE;
    return rg_gdi_blit_color(d,s,dx,dy,sx,sy,width,height,
                              (unsigned long)rop,text,background)
               ? TRUE : FALSE;
}

#if defined(PAL_WIN31_BOOTTRACE)
static BOOL FAR PASCAL pal_trace_impl_BitBlt(LPVOID dest, short dx, short dy,
                       LPVOID src, short sx, short sy,
                       short width, short height, DWORD rop,
                       LPVOID brush, LPVOID drawmode)
#else
BOOL FAR PASCAL BitBlt(LPVOID dest, short dx, short dy,
                       LPVOID src, short sx, short sy,
                       short width, short height, DWORD rop,
                       LPVOID brush, LPVOID drawmode)
#endif
{
    BOOL result;
    pal_cursor_draw_begin(); /* exclude from BOTH screen read and write */
    result=pal_win16_bitblt_impl(dest,dx,dy,src,sx,sy,width,height,
                                 rop,brush,drawmode);
    pal_cursor_draw_end();
    return result;
}
#if defined(PAL_WIN31_BOOTTRACE)
BOOL FAR PASCAL BitBlt(LPVOID dest, short dx, short dy,
                       LPVOID src, short sx, short sy,
                       short width, short height, DWORD rop,
                       LPVOID brush, LPVOID drawmode)
{
    BOOL result;
    int traced = pal_ddi_trace_enter("DDI BitBlt ENTER");
    if (traced) {
        pal_boottrace_hex("BLT ROP3", (unsigned)((rop >> 16) & 255UL));
        pal_boottrace_hex("BLT SRC PRESENT", (unsigned)(src != 0));
        pal_boottrace_hex("BLT BRUSH PRESENT", (unsigned)(brush != 0));
        pal_boottrace_hex("BLT WIDTH", (unsigned)width);
        pal_boottrace_hex("BLT HEIGHT", (unsigned)height);
        pal_boottrace_hex("BLT DEST SCREEN", (unsigned)(pal_active_host && dest &&
           pal_ddi_far_token((PAL_U16)FP_SEG(dest),(PAL_U16)FP_OFF(dest)) == pal_active_host->ddi.screen_token));
    }
    result = pal_trace_impl_BitBlt(dest, dx, dy, src, sx, sy, width, height, rop, brush, drawmode);
    if (traced) pal_boottrace_hex("DDI BitBlt RETURN LO", (unsigned)result);
    return result;
}
#endif

/* Pixel @9: DRAWMODE starts with a 16-bit Rop2. A null DRAWMODE
 * requests a read, for which the color argument is ignored.
 * The physical color for INDEX8 is the already-translated palette index.
 * All other devices must be refused before touching VRAM.
 */
static DWORD pal_win16_pixel_impl(LPVOID dest, short x, short y,
                       DWORD physical_color, LPVOID drawmode)
{
    PAL_U32 result;
    PAL_U32 token;
    unsigned rop2 = 0;
    if (!pal_active_host || !dest) return 0x80000000UL;
    token = pal_ddi_far_token((PAL_U16)FP_SEG(dest), (PAL_U16)FP_OFF(dest));
    if (drawmode) {
        rop2 = *(const WORD FAR *)drawmode;
        if (rop2 < 1u || rop2 > 16u) return 0x80000000UL;
    }
    if (!pal_screen_host_pixel(pal_active_host, token, x, y,
                               (PAL_U32)physical_color, rop2, &result))
        return 0x80000000UL;
    return (DWORD)result;
}

#if defined(PAL_WIN31_BOOTTRACE)
static DWORD FAR PASCAL pal_trace_impl_Pixel(LPVOID dest, short x, short y,
                       DWORD physical_color, LPVOID drawmode)
#else
DWORD FAR PASCAL Pixel(LPVOID dest, short x, short y,
                       DWORD physical_color, LPVOID drawmode)
#endif
{
    DWORD result;
    pal_cursor_draw_begin(); /* reads must omit composited mouse bits */
    result=pal_win16_pixel_impl(dest,x,y,physical_color,drawmode);
    pal_cursor_draw_end();
    return result;
}
#if defined(PAL_WIN31_BOOTTRACE)
DWORD FAR PASCAL Pixel(LPVOID dest, short x, short y,
                       DWORD physical_color, LPVOID drawmode)
{
    DWORD result;
    int traced = pal_ddi_trace_enter("DDI Pixel ENTER");
    result = pal_trace_impl_Pixel(dest, x, y, physical_color, drawmode);
    if (traced) pal_boottrace_hex("DDI Pixel RETURN LO", (unsigned)result);
    if (traced) pal_boottrace_hex("DDI Pixel RETURN HI", (unsigned)(result >> 16));
    return result;
}
#endif

/* RealizeObject @10: Win3.x DDK, GDI-owned physical storage. Pens and
 * solid/hollow/hatched brushes are private, WORD-aligned objects shared
 * with the common renderer. The DDK LOGBRUSH has a physical background
 * DWORD at byte 8 (unlike the SDK structure). No indirect allocations:
 * deletions are no-ops. Fonts remain GDI-realized; device bitmaps are off.
 */
/* Object realization needs a device format, not accessible pixel storage.
 * A size query can precede allocation of the bitmap's bits. Keep the
 * existing physical pen/brush representation for both screen and memory;
 * Output already converts physical indices for monochrome destinations. */
static int pal_win16_object_target(LPVOID dest)
{
    const PAL_BITMAP16 FAR *bitmap;
    PAL_U32 token;
    if (!pal_active_host || !pal_active_host->running || !dest) return 0;
    token=pal_ddi_far_token((PAL_U16)FP_SEG(dest), (PAL_U16)FP_OFF(dest));
    if (token == pal_active_host->ddi.screen_token) return 1;
    if ((PAL_U32)(PAL_U16)FP_OFF(dest)+10UL > 65536UL) return 0;
    bitmap=(const PAL_BITMAP16 FAR *)dest;
    return bitmap->bmType == 0 && bitmap->bmPlanes == 1 &&
           (bitmap->bmBitsPixel == 1 || bitmap->bmBitsPixel == 8);
}
#if defined(PAL_WIN31_BOOTTRACE)
static void pal_trace_object_target(LPVOID dest)
{
    const PAL_BITMAP16 FAR *bitmap;
    PAL_U32 token;
    pal_boottrace_hex("DEST PRESENT", (unsigned)(dest != 0));
    if (!dest || !pal_active_host) return;
    token=pal_ddi_far_token((PAL_U16)FP_SEG(dest), (PAL_U16)FP_OFF(dest));
    pal_boottrace_hex("DEST SCREEN", (unsigned)(token == pal_active_host->ddi.screen_token));
    if ((PAL_U32)(PAL_U16)FP_OFF(dest)+2UL > 65536UL) return;
    bitmap=(const PAL_BITMAP16 FAR *)dest;
    pal_boottrace_hex("DEST TYPE", (unsigned)bitmap->bmType);
    if (bitmap->bmType == 0 && (PAL_U32)(PAL_U16)FP_OFF(dest)+10UL <= 65536UL) {
        pal_boottrace_hex("DEST PLANES", bitmap->bmPlanes);
        pal_boottrace_hex("DEST BPP", bitmap->bmBitsPixel);
    }
}
#endif

#if defined(PAL_WIN31_BOOTTRACE)
static DWORD FAR PASCAL pal_trace_impl_RealizeObject(LPVOID dest, short style, LPVOID input,
                               LPVOID output, LPVOID text_xform)
#else
DWORD FAR PASCAL RealizeObject(LPVOID dest, short style, LPVOID input,
                               LPVOID output, LPVOID text_xform)
#endif
{
    const PAL_LogicalPen FAR *in_pen;
    const PAL_LogicalBrush FAR *in_brush;
    const PAL_POINT16 FAR *phase;
    RG_Pen phys_pen;
    RG_Brush phys_brush;
    const unsigned char *source;
    unsigned char FAR *target;
    unsigned i, size;
    if (!pal_object_layout_ok() || !pal_win16_object_target(dest)) return 0;
    if (style == -1 || style == -2) return 1; /* no owned resources */
    if (!input) return 0;
    source = 0;
    size = 0;
    if (style == 1) { /* OBJ_PEN */
        in_pen = (const PAL_LogicalPen FAR *)input;
        if (!rg_pen_realize(&pal_active_host->palette, in_pen->style,
                            in_pen->width.x, (rg_pixel)in_pen->color,
                            &phys_pen)) return 0;
        source = (const unsigned char *)&phys_pen;
        size = sizeof(phys_pen);
    } else if (style == 2) { /* OBJ_BRUSH */
        unsigned x_phase=0, y_phase=0, background=0;
        in_brush = (const PAL_LogicalBrush FAR *)input;
        if (text_xform) {
            phase = (const PAL_POINT16 FAR *)text_xform;
            x_phase = (unsigned)phase->x & 7u;
            y_phase = (unsigned)phase->y & 7u;
        }
        if (in_brush->style == RG_BRUSH_HATCHED &&
            !pal_object_background(&pal_active_host->palette,
                                    in_brush->background, &background)) return 0;
        if (!rg_brush_realize(&pal_active_host->palette, in_brush->style,
                              (rg_pixel)in_brush->color, background,
                              in_brush->hatch, x_phase, y_phase,
                              &phys_brush)) return 0;
        source = (const unsigned char *)&phys_brush;
        size = sizeof(phys_brush);
    } else return 0; /* no fonts, device bitmaps or unknown styles */
    if (output) {
        target = (unsigned char FAR *)output;
        for (i=0; i<size; ++i) target[i]=source[i];
    }
    return (DWORD)size;
}
#if defined(PAL_WIN31_BOOTTRACE)
DWORD FAR PASCAL RealizeObject(LPVOID dest, short style, LPVOID input,
                               LPVOID output, LPVOID text_xform)
{
    DWORD result;
    int traced = pal_ddi_trace_enter("DDI RealizeObject ENTER");
    if (traced) pal_trace_object_target(dest);
    if (traced) pal_boottrace_hex("  style", (unsigned)style);
    if (traced) pal_boottrace_hex("  (input != 0)", (unsigned)(input != 0));
    if (traced) pal_boottrace_hex("  (output != 0)", (unsigned)(output != 0));
    if (traced && style == 2 && input) {
        const PAL_LogicalBrush FAR *b = (const PAL_LogicalBrush FAR *)input;
        pal_boottrace_hex("BRUSH STYLE", b->style);
        pal_boottrace_hex("BRUSH COLOR LO", (unsigned)b->color);
        pal_boottrace_hex("BRUSH COLOR HI", (unsigned)(b->color >> 16));
        pal_boottrace_hex("BRUSH HATCH", b->hatch);
        pal_boottrace_hex("BRUSH BACKGROUND LO", (unsigned)b->background);
        pal_boottrace_hex("BRUSH BACKGROUND HI", (unsigned)(b->background >> 16));
    }
    result = pal_trace_impl_RealizeObject(dest, style, input, output, text_xform);
    if (traced) pal_boottrace_hex("DDI RealizeObject RETURN LO", (unsigned)result);
    if (traced) pal_boottrace_hex("DDI RealizeObject RETURN HI", (unsigned)(result >> 16));
    return result;
}
#endif

/* Output @8: GDI's required OS_SCANLINES (4) and OS_POLYLINE (18),
 * the existing OS_RECTANGLE (6), plus harmless BEGIN/END scan hints.
 * Other shapes return -1 for GDI synthesis. Screen identity must be valid
 * before marshalling or touching the video memory. Scanline POINT[0].y
 * gives the common Y; POINT[1..count-1] each hold an (left,right) X pair.
 * Each polyline segment excludes its final endpoint, preventing double XOR
 * at joins. Bounds/ROP/physical objects are checked before drawing.
 */
static WORD pal_win16_output_impl(LPVOID dest, WORD style, WORD count,
                        LPVOID points, LPVOID pen, LPVOID brush,
                        LPVOID drawmode, LPVOID cliprect)
{
    const PAL_POINT16 FAR *pt;
    const PAL_Rect16 FAR *cr;
    const WORD FAR *draw;
    const unsigned char FAR *source;
    unsigned char *dst_bytes;
    RG_Pen near_pen;
    RG_Brush near_brush;
    RG_Rect bounds, clipped;
    PAL_Win16Bits mbits;
    RG_Device mdevice, *target;
    const RG_Rect *cptr=0;
    unsigned i, rop2, bgmode;
    if (!pal_active_host || !pal_active_host->running || !dest)
        return (WORD)-1;
    if (!pal_win16_device(dest,&mdevice,&mbits,&target))
        return (WORD)-1;
    /* Win3.x bracketing hints have no drawn pixels and can be ignored. */
    if (style == 80u || style == 81u) return 1u;
    if ((style != 4u && style != 6u && style != 18u) ||
        count < 2u || count > 16384u || !points || !drawmode ||
        !pal_object_layout_ok()) return (WORD)-1;
    if (style == 6u && count != 2u) return (WORD)-1;
    /* Win16 far pointers have a 16-bit offset. Do not index a POINT
     * sequence across a 64 KiB segment wrap. */
    if ((PAL_U32)(PAL_U16)FP_OFF(points) +
        (PAL_U32)count * (PAL_U32)sizeof(PAL_POINT16) > 65536UL)
        return (WORD)-1;
    pt = (const PAL_POINT16 FAR *)points;
    draw = (const WORD FAR *)drawmode;
    rop2 = draw[0]; bgmode = draw[1];
    if (rop2 < 1u || rop2 > 16u || bgmode < 1u || bgmode > 2u)
        return (WORD)-1;
    if (cliprect) {
        cr = (const PAL_Rect16 FAR *)cliprect;
        clipped.left = cr->left; clipped.top = cr->top;
        clipped.right = cr->right; clipped.bottom = cr->bottom;
        cptr = &clipped;
    }
    if (pen) {
        source = (const unsigned char FAR *)pen;
        dst_bytes = (unsigned char *)&near_pen;
        for (i=0; i<sizeof(near_pen); ++i) dst_bytes[i]=source[i];
        if (!rg_pen_valid(&near_pen)) return (WORD)-1;
    }
    if (brush) {
        source = (const unsigned char FAR *)brush;
        dst_bytes = (unsigned char *)&near_brush;
        for (i=0; i<sizeof(near_brush); ++i) dst_bytes[i]=source[i];
        if (!rg_brush_valid(&near_brush)) return (WORD)-1;
    }
    if (target->surface.format == RG_FMT_INDEX1) {
        unsigned text=0, background=0;
        if (!pal_win16_mode_colors(drawmode,&text,&background))
            return (WORD)-1;
        /* Win3.x monochrome 1=background, 0=other (text). */
        if (pen)
            near_pen.color=(unsigned short)rg_gdi_mono_color(
                                      near_pen.color,background);
        if (brush) {
            near_brush.foreground=(unsigned short)rg_gdi_mono_color(
                                      near_brush.foreground,background);
            near_brush.background=(unsigned short)rg_gdi_mono_color(
                                      near_brush.background,background);
        }
    }
    if (style == 4u) {
        /* Preflight every span before the first write: reject malformed
         * pairs without leaving a half-rendered scanline list. */
        for (i=1u; i<(unsigned)count; ++i)
            if (pt[i].x > pt[i].y) return (WORD)-1;
        for (i=1u; i<(unsigned)count; ++i)
            if (!rg_gdi_output_scanline(target,
                    pt[0].y, pt[i].x, pt[i].y, cptr,
                    pen ? &near_pen : 0, brush ? &near_brush : 0,
                    rop2, bgmode)) return (WORD)-1;
        return 1u;
    }
    if (style == 18u) {
        for (i=1u; i<(unsigned)count; ++i)
            if (!rg_gdi_output_segment(target,
                    pt[i-1u].x, pt[i-1u].y, pt[i].x, pt[i].y,
                    cptr, pen ? &near_pen : 0, rop2)) return (WORD)-1;
        return 1u;
    }
    bounds.left = pt[0].x; bounds.top = pt[0].y;
    bounds.right = pt[1].x; bounds.bottom = pt[1].y;
    return rg_gdi_output_rectangle(target, bounds, cptr,
             pen ? &near_pen : 0, brush ? &near_brush : 0,
             rop2, bgmode) ? 1u : (WORD)-1;
}

#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_Output(LPVOID dest, WORD style, WORD count,
                        LPVOID points, LPVOID pen, LPVOID brush,
                        LPVOID drawmode, LPVOID cliprect)
#else
WORD FAR PASCAL Output(LPVOID dest, WORD style, WORD count,
                        LPVOID points, LPVOID pen, LPVOID brush,
                        LPVOID drawmode, LPVOID cliprect)
#endif
{
    WORD result;
    if (style==80u || style==81u) return
        pal_win16_output_impl(dest,style,count,points,pen,brush,drawmode,
                               cliprect);
    pal_cursor_draw_begin();
    result=pal_win16_output_impl(dest,style,count,points,pen,brush,
                                  drawmode,cliprect);
    pal_cursor_draw_end();
    return result;
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL Output(LPVOID dest, WORD style, WORD count,
                        LPVOID points, LPVOID pen, LPVOID brush,
                        LPVOID drawmode, LPVOID cliprect)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI Output ENTER");
    if (traced) pal_boottrace_hex("  style", (unsigned)style);
    result = pal_trace_impl_Output(dest, style, count, points, pen, brush, drawmode, cliprect);
    if (traced) pal_boottrace_hex("DDI Output RETURN LO", (unsigned)result);
    return result;
}
#endif

/* DDK GDI bootstrap and compatibility DDIs. These were missing in v0044
 * although our export validator reported all 19 *known* exports. GDI may
 * resolve the required ordinals before it ever calls Enable(style=1).
 * Keep this ABI complete independently of whether hardware starts.
 * Deliberately unsupported features return documented failures, never
 * pretend an operation drew anything or dereference an unknown PDEVICE.
 */
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_Control(LPVOID device, WORD escape, LPVOID input,
                        LPVOID output)
#else
WORD FAR PASCAL Control(LPVOID device, WORD escape, LPVOID input,
                        LPVOID output)
#endif
{
    const WORD FAR *which=(const WORD FAR *)input;
    (void)device; (void)output;
    /* DDK QUERYESCSUPPORT (8); only this escape is implemented. */
    if (escape!=8u || !input ||
        (PAL_U32)(PAL_U16)FP_OFF(input)+2UL>65536UL) return 0;
    return *which==8u ? 1u : 0u;
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL Control(LPVOID device, WORD escape, LPVOID input,
                        LPVOID output)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI Control ENTER");
    if (traced) pal_boottrace_hex("  escape", (unsigned)escape);
    result = pal_trace_impl_Control(device, escape, input, output);
    if (traced) pal_boottrace_hex("DDI Control RETURN LO", (unsigned)result);
    return result;
}
#endif
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_EnumDFonts(LPVOID device, LPVOID face,
                           FARPROC callback, LPVOID client)
#else
WORD FAR PASCAL EnumDFonts(LPVOID device, LPVOID face,
                           FARPROC callback, LPVOID client)
#endif
{
    (void)device; (void)face; (void)callback; (void)client;
    /* No DEVICE fonts; GDI owns its raster and TrueType fonts. */
    return 1u; /* documented no-device-fonts enumeration result */
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL EnumDFonts(LPVOID device, LPVOID face,
                           FARPROC callback, LPVOID client)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI EnumDFonts ENTER");
    result = pal_trace_impl_EnumDFonts(device, face, callback, client);
    if (traced) pal_boottrace_hex("DDI EnumDFonts RETURN LO", (unsigned)result);
    return result;
}
#endif
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_EnumObj(LPVOID device, WORD style, FARPROC callback,
                        LPVOID client)
#else
WORD FAR PASCAL EnumObj(LPVOID device, WORD style, FARPROC callback,
                        LPVOID client)
#endif
{
    (void)device; (void)style; (void)callback; (void)client;
    /* No enumerated DEVICE pen/brush catalog yet; GDI stock objects
     * remain available through RealizeObject. Do not call a foreign thunk
     * with a guessed LOGPEN/LOGBRUSH wire layout. */
    return 1u; /* documented no-device-objects enumeration result */
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL EnumObj(LPVOID device, WORD style, FARPROC callback,
                        LPVOID client)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI EnumObj ENTER");
    result = pal_trace_impl_EnumObj(device, style, callback, client);
    if (traced) pal_boottrace_hex("DDI EnumObj RETURN LO", (unsigned)result);
    return result;
}
#endif
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_GetCharWidth(LPVOID device, LPVOID buffer,
                             WORD first, WORD last, LPVOID font_address,
                             LPVOID drawmode, LPVOID text_transform)
#else
WORD FAR PASCAL GetCharWidth(LPVOID device, LPVOID buffer,
                             WORD first, WORD last, LPVOID font_address,
                             LPVOID drawmode, LPVOID text_transform)
#endif
{
    PAL_Win16FontBytes fb;
    PAL_Font16 font;
    RG_Glyph glyph;
    WORD FAR *out=(WORD FAR *)buffer;
    unsigned count,i;
    (void)device; (void)drawmode; (void)text_transform;
    if (!buffer || !font_address || first>last || last>255u ||
        (PAL_U32)(PAL_U16)FP_OFF(font_address)+
            (PAL_U32)PAL_FONT16_HEADER_BYTES>65536UL) return 0;
    count=(unsigned)(last-first)+1u;
    if ((PAL_U32)(PAL_U16)FP_OFF(buffer)+(PAL_U32)count*2UL>65536UL)
        return 0;
    fb.selector=(PAL_U16)FP_SEG(font_address);
    fb.base=(PAL_U16)FP_OFF(font_address);
    if (!pal_font16_open(&font,pal_win16_font_read,&fb,
                         65536UL-(PAL_U32)fb.base)) return 0;
    /* Validate full range first: no partial writes on a bad font. */
    for (i=0;i<count;++i)
        if (!pal_font16_glyph(&font,pal_win16_font_read,&fb,
                             (unsigned)first+i,&glyph)) return 0;
    for (i=0;i<count;++i) {
        (void)pal_font16_glyph(&font,pal_win16_font_read,&fb,
                              (unsigned)first+i,&glyph);
        out[i]=(WORD)glyph.width;
    }
    return 1u;
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL GetCharWidth(LPVOID device, LPVOID buffer,
                             WORD first, WORD last, LPVOID font_address,
                             LPVOID drawmode, LPVOID text_transform)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI GetCharWidth ENTER");
    result = pal_trace_impl_GetCharWidth(device, buffer, first, last, font_address, drawmode, text_transform);
    if (traced) pal_boottrace_hex("DDI GetCharWidth RETURN LO", (unsigned)result);
    return result;
}
#endif
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_DeviceBitmap(LPVOID device, WORD command,
                             LPVOID bitmap, LPVOID data)
#else
WORD FAR PASCAL DeviceBitmap(LPVOID device, WORD command,
                             LPVOID bitmap, LPVOID data)
#endif
{
    (void)device; (void)command; (void)bitmap; (void)data;
    /* DDK-required stub. Never advertise RC_DEVBITS. */
    return 0u;
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL DeviceBitmap(LPVOID device, WORD command,
                             LPVOID bitmap, LPVOID data)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI DeviceBitmap ENTER");
    result = pal_trace_impl_DeviceBitmap(device, command, bitmap, data);
    if (traced) pal_boottrace_hex("DDI DeviceBitmap RETURN LO", (unsigned)result);
    return result;
}
#endif
/* FastBorder is present for historical ordinal compatibility but not yet
 * supported by our renderer. Nonzero signals failure; no pixels touched. */
#if defined(PAL_WIN31_BOOTTRACE)
static DWORD FAR PASCAL pal_trace_impl_FastBorder(LPVOID rect, WORD hx, WORD vy, DWORD rop,
                            LPVOID device, LPVOID brush, LPVOID mode,
                            LPVOID clip)
#else
DWORD FAR PASCAL FastBorder(LPVOID rect, WORD hx, WORD vy, DWORD rop,
                            LPVOID device, LPVOID brush, LPVOID mode,
                            LPVOID clip)
#endif
{
    (void)rect; (void)hx; (void)vy; (void)rop;
    (void)device; (void)brush; (void)mode; (void)clip;
    return 1UL;
}
#if defined(PAL_WIN31_BOOTTRACE)
DWORD FAR PASCAL FastBorder(LPVOID rect, WORD hx, WORD vy, DWORD rop,
                            LPVOID device, LPVOID brush, LPVOID mode,
                            LPVOID clip)
{
    DWORD result;
    int traced = pal_ddi_trace_enter("DDI FastBorder ENTER");
    result = pal_trace_impl_FastBorder(rect, hx, vy, rop, device, brush, mode, clip);
    if (traced) pal_boottrace_hex("DDI FastBorder RETURN LO", (unsigned)result);
    if (traced) pal_boottrace_hex("DDI FastBorder RETURN HI", (unsigned)(result >> 16));
    return result;
}
#endif
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_SetAttribute(LPVOID device, WORD a, WORD b, WORD c)
#else
WORD FAR PASCAL SetAttribute(LPVOID device, WORD a, WORD b, WORD c)
#endif
{
    (void)device; (void)a; (void)b; (void)c;
    /* Reserved function required by GDI: must return zero. */
    return 0u;
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL SetAttribute(LPVOID device, WORD a, WORD b, WORD c)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI SetAttribute ENTER");
    result = pal_trace_impl_SetAttribute(device, a, b, c);
    if (traced) pal_boottrace_hex("DDI SetAttribute RETURN LO", (unsigned)result);
    return result;
}
#endif

/* Windows 3.x DDI: style 1 requests GDIINFO, style 0 requests physical
 * initialization. Production staging build ALWAYS rejects style 0; only an
 * opt-in trial build, in Standard mode, may enter the native transaction. */
WORD FAR PASCAL Enable(LPVOID destination, WORD style,
                       LPSTR device_type, LPSTR output_file, LPVOID data)
{
    PAL_GDIINFO candidate;
    PAL_ET4KPlan plan;
    unsigned char FAR *out;
    const unsigned char *in;
    unsigned i;
    (void)device_type; (void)output_file; (void)data;
    if (style == 0u) {
        int started;
        pal_boottrace("ENABLE0 ENTER");
        if (!pal_win16_install_ready()) return 0;
        started=pal_win16_activate(destination);
        pal_boottrace(started ? "ENABLE0 OK" : "ENABLE0 FAIL");
        pal_boottrace_hex("ENABLE0 RETURN", started ? 1u : 0u);
        return started ? 1u : 0u;
    }
    if (style != 1u || !destination) {
        pal_boottrace("ENABLE BAD STYLE OR NULL");
        return 0;
    }
    pal_boottrace("ENABLE1 ENTER");
    /* Never advertise 640x480 on an unknown or 256KiB card. Until we
     * have an authorized in-Windows capacity probe, require explicit
     * [Palettium] VRAMKiB in SYSTEM.INI. This is still just a candidate
     * GDIINFO; style=0 remains blocked. The planner caps stock-VDD
     * enhanced-mode candidates to a <=256KiB framebuffer. */
    if ((PAL_U32)(PAL_U16)FP_OFF(destination) +
        (PAL_U32)sizeof(candidate) > 65536UL ||
        !pal_win16_et4k_profile_plan(&plan) ||
        !pal_et4k_plan_capability(&candidate,&plan)) {
        pal_boottrace("ENABLE1 FAIL");
        return 0;
    }
    out = (unsigned char FAR *)destination;
    in = (const unsigned char *)&candidate;
    for (i = 0; i < sizeof(candidate); ++i) out[i] = in[i];
    pal_boottrace_hex("GDIINFO SIZE", sizeof(candidate));
    pal_boottrace_hex("GDIINFO PDEVICE SIZE", candidate.dpDEVICEsize);
    pal_boottrace_hex("GDIINFO RASTER", candidate.dpRaster);
    pal_boottrace_hex("GDIINFO TEXT", candidate.dpText);
    pal_boottrace_hex("GDIINFO WIDTH", candidate.dpHorzRes);
    pal_boottrace_hex("GDIINFO HEIGHT", candidate.dpVertRes);
    pal_boottrace("ENABLE1 OK");
    return (WORD)sizeof(candidate);
}

void FAR PASCAL Disable(LPVOID physical_device)
{
    PAL_U32 token;
    /* An unrelated PDEVICE may not remove a binding. The eventual
     * installed driver must also close its owning hardware session here.
     * No session is installed by PALSTAGE today. */
    if (!pal_active_host || !physical_device) return;
    token=pal_ddi_far_token((PAL_U16)FP_SEG(physical_device),
                            (PAL_U16)FP_OFF(physical_device));
    if (token != pal_active_host->ddi.screen_token) return;
    pal_boottrace("DISABLE ENTER");
    /* Detach the public binding before shutting the hardware down.
     * The static session and native selector context must remain valid
     * throughout RG_Hardware.close / BIOS restore. */
    pal_cursor16_unbind(&pal_win16_cursor);
    if (pal_active_host == &pal_native_session.host) {
        pal_active_host=0;
        pal_et4k_session_stop(&pal_native_session);
    } else {
        pal_active_host=0; /* staging test-host compatibility */
    }
}

/* ColorInfo @2: GDI passes a logical COLORREF (0x00BBGGRR) or an
 * explicit 0xFF00iiii palette index. Physical color is the framebuffer
 * index, NOT a DAC RGB value. Avoid dereferencing foreign PDEVICEs.
 */
#if defined(PAL_WIN31_BOOTTRACE)
static DWORD FAR PASCAL pal_trace_impl_ColorInfo(LPVOID device, DWORD color, LPVOID physical)
#else
DWORD FAR PASCAL ColorInfo(LPVOID device, DWORD color, LPVOID physical)
#endif
{
    PAL_U32 logical, index;
    PAL_U32 token;
    if (!pal_active_host || !device) return 0;
    token = pal_ddi_far_token((PAL_U16)FP_SEG(device),
                              (PAL_U16)FP_OFF(device));
    if (token == pal_active_host->ddi.screen_token) {
        if (!pal_screen_host_colorinfo(pal_active_host, token, (PAL_U32)color,
                                       physical != 0, &logical, &index)) return 0;
    } else {
        const PAL_BITMAP16 FAR *bmp;
        rg_pixel rgb, idx=0;
        unsigned sum;
        if (!pal_win16_object_target(device)) return 0;
        bmp=(const PAL_BITMAP16 FAR *)device;
        if (bmp->bmBitsPixel == 8) {
            if (!rg_palette_color_info(&pal_active_host->palette,(rg_pixel)color,
                                       physical != 0,&rgb,&idx)) return 0;
            logical=(PAL_U32)rgb; index=(PAL_U32)idx;
        } else {
            if (physical) {
                if ((color & 0xFF000000UL) != 0) return 0;
                sum=(unsigned)(color & 255UL)+(unsigned)((color >> 8)&255UL)+
                    (unsigned)((color >> 16)&255UL);
                index=sum>=383u ? 1u : 0u;
            } else {
                if (color > 1UL) return 0;
                index=(PAL_U32)color;
            }
            logical=index ? 0x00FFFFFFUL : 0UL;
        }
    }
    if (physical) *(DWORD FAR *)physical = (DWORD)index;
    return (DWORD)logical;
}
#if defined(PAL_WIN31_BOOTTRACE)
DWORD FAR PASCAL ColorInfo(LPVOID device, DWORD color, LPVOID physical)
{
    DWORD result;
    int traced = 0;
    if (pal_color_trace_budget) {
        --pal_color_trace_budget;
        traced = pal_ddi_trace_enter("DDI ColorInfo ENTER");
    }
    if (traced) {
        pal_trace_object_target(device);
        pal_boottrace_hex("COLOR INPUT LO", (unsigned)color);
        pal_boottrace_hex("COLOR INPUT HI", (unsigned)(color >> 16));
        pal_boottrace_hex("COLOR FORWARD", (unsigned)(physical != 0));
    }
    result = pal_trace_impl_ColorInfo(device, color, physical);
    if (traced) pal_boottrace_hex("DDI ColorInfo RETURN LO", (unsigned)result);
    if (traced) pal_boottrace_hex("DDI ColorInfo RETURN HI", (unsigned)(result >> 16));
    return result;
}
#endif

/* ScanLR @12, flood-fill horizontal search. Signed -1 means not found;
 * 0x8000 means invalid device, coordinates, or unsupported bitmap.
 */
#if defined(PAL_WIN31_BOOTTRACE)
static WORD FAR PASCAL pal_trace_impl_ScanLR(LPVOID device, short x, short y,
                       DWORD physical, WORD style)
#else
WORD FAR PASCAL ScanLR(LPVOID device, short x, short y,
                       DWORD physical, WORD style)
#endif
{
    int found;
    PAL_U32 token;
    if (!pal_active_host || !device) return 0x8000u;
    token = pal_ddi_far_token((PAL_U16)FP_SEG(device),
                              (PAL_U16)FP_OFF(device));
    pal_cursor_draw_begin();
    if (!pal_screen_host_scanlr(pal_active_host, token, x, y,
                                (PAL_U32)physical, style, &found)) {
        pal_cursor_draw_end();
        return 0x8000u;
    }
    pal_cursor_draw_end();
    return (WORD)found;
}
#if defined(PAL_WIN31_BOOTTRACE)
WORD FAR PASCAL ScanLR(LPVOID device, short x, short y,
                       DWORD physical, WORD style)
{
    WORD result;
    int traced = pal_ddi_trace_enter("DDI ScanLR ENTER");
    result = pal_trace_impl_ScanLR(device, x, y, physical, style);
    if (traced) pal_boottrace_hex("DDI ScanLR RETURN LO", (unsigned)result);
    return result;
}
#endif

/* Win3.x palette DDIs have no PDEVICE argument; they are valid only
 * after the screen host has completed hardware and PDEVICE startup.
 * Keep Win16 far-memory copying here; core palette owns color policy.
 */
#if defined(PAL_WIN31_BOOTTRACE)
static void FAR PASCAL pal_trace_impl_SetPalette(short start, short count, LPVOID palette)
#else
void FAR PASCAL SetPalette(short start, short count, LPVOID palette)
#endif
{
    const unsigned char FAR *src = (const unsigned char FAR *)palette;
    PAL_ScreenHost *host = pal_active_host;
    RG_Color current;
    unsigned i, idx, off;
    if (!host || !host->running || !src || start < 0 || count < 0 ||
        (unsigned)start > 256u ||
        (unsigned)count > 256u-(unsigned)start) return;
    /* Validate the entire transaction before any hardware write. */
    for (i=0; i<(unsigned)count; ++i) {
        idx = (unsigned)start+i;
        if (!rg_palette_is_static(idx)) continue;
        off = 4u*i;
        if (!rg_palette_get_entry(&host->palette, idx, &current) ||
            current.red != src[off] ||
            current.green != src[off+1u] ||
            current.blue != src[off+2u]) return;
    }
    for (i=0; i<(unsigned)count; ++i) {
        idx = (unsigned)start+i;
        if (rg_palette_is_static(idx)) continue;
        off = 4u*i;
        (void)pal_screen_host_palette(host, idx,
                src[off], src[off+1u], src[off+2u]);
    }
}
#if defined(PAL_WIN31_BOOTTRACE)
void FAR PASCAL SetPalette(short start, short count, LPVOID palette)
{
    int traced = pal_ddi_trace_enter("DDI SetPalette ENTER");
    if (traced) pal_boottrace_hex("  start", (unsigned)start);
    if (traced) pal_boottrace_hex("  count", (unsigned)count);
    pal_trace_impl_SetPalette(start, count, palette);
    if (traced) pal_boottrace("DDI SetPalette RETURN");
}
#endif

#if defined(PAL_WIN31_BOOTTRACE)
static void FAR PASCAL pal_trace_impl_GetPalette(short start, short count, LPVOID palette)
#else
void FAR PASCAL GetPalette(short start, short count, LPVOID palette)
#endif
{
    unsigned char FAR *dst = (unsigned char FAR *)palette;
    RG_Color color;
    unsigned i, off;
    if (!pal_active_host || !pal_active_host->running || !dst ||
        start < 0 || count < 0 || (unsigned)start > 256u ||
        (unsigned)count > 256u-(unsigned)start) return;
    for (i=0; i<(unsigned)count; ++i) {
        if (!rg_palette_get_entry(&pal_active_host->palette,
                                   (unsigned)start+i, &color)) return;
        off = 4u*i;
        dst[off] = color.red;
        dst[off+1u] = color.green;
        dst[off+2u] = color.blue;
        dst[off+3u] = 0;
    }
}
#if defined(PAL_WIN31_BOOTTRACE)
void FAR PASCAL GetPalette(short start, short count, LPVOID palette)
{
    int traced = pal_ddi_trace_enter("DDI GetPalette ENTER");
    if (traced) pal_boottrace_hex("  start", (unsigned)start);
    if (traced) pal_boottrace_hex("  count", (unsigned)count);
    pal_trace_impl_GetPalette(start, count, palette);
    if (traced) pal_boottrace("DDI GetPalette RETURN");
}
#endif

#if defined(PAL_WIN31_BOOTTRACE)
static void FAR PASCAL pal_trace_impl_SetPalTrans(LPVOID translation)
#else
void FAR PASCAL SetPalTrans(LPVOID translation)
#endif
{
    const WORD FAR *src = (const WORD FAR *)translation;
    unsigned short table[256];
    unsigned i;
    if (!pal_active_host || !pal_active_host->running) return;
    if (src) for (i=0; i<256u; ++i) table[i] = src[i];
    (void)rg_palette_set_translation(&pal_active_host->palette,
                                      src ? table : 0);
}
#if defined(PAL_WIN31_BOOTTRACE)
void FAR PASCAL SetPalTrans(LPVOID translation)
{
    int traced = pal_ddi_trace_enter("DDI SetPalTrans ENTER");
    pal_trace_impl_SetPalTrans(translation);
    if (traced) pal_boottrace("DDI SetPalTrans RETURN");
}
#endif

#if defined(PAL_WIN31_BOOTTRACE)
static void FAR PASCAL pal_trace_impl_GetPalTrans(LPVOID translation)
#else
void FAR PASCAL GetPalTrans(LPVOID translation)
#endif
{
    WORD FAR *dst = (WORD FAR *)translation;
    unsigned i;
    if (!pal_active_host || !pal_active_host->running || !dst) return;
    for (i=0; i<256u; ++i)
        dst[i] = (WORD)pal_active_host->palette.logical_to_physical[i];
}
#if defined(PAL_WIN31_BOOTTRACE)
void FAR PASCAL GetPalTrans(LPVOID translation)
{
    int traced = pal_ddi_trace_enter("DDI GetPalTrans ENTER");
    pal_trace_impl_GetPalTrans(translation);
    if (traced) pal_boottrace("DDI GetPalTrans RETURN");
}
#endif

/* UpdateColors @26: x/y/extents, followed by a 256-WORD physical-index
 * translation table. Bounds and all 256 values are validated by core
 * BEFORE the first pixel is touched.
 */
#if defined(PAL_WIN31_BOOTTRACE)
static void FAR PASCAL pal_trace_impl_UpdateColors(WORD x, WORD y, WORD width, WORD height,
                             LPVOID translation)
#else
void FAR PASCAL UpdateColors(WORD x, WORD y, WORD width, WORD height,
                             LPVOID translation)
#endif
{
    const WORD FAR *src = (const WORD FAR *)translation;
    unsigned short table[256];
    unsigned i;
    if (!pal_active_host || !pal_active_host->running || !src) return;
    for (i=0; i<256u; ++i) table[i] = src[i];
    pal_cursor_draw_begin();
    (void)pal_screen_host_update_colors(pal_active_host, x, y,
                                        width, height, table);
    pal_cursor_draw_end();
}
#if defined(PAL_WIN31_BOOTTRACE)
void FAR PASCAL UpdateColors(WORD x, WORD y, WORD width, WORD height,
                             LPVOID translation)
{
    int traced = pal_ddi_trace_enter("DDI UpdateColors ENTER");
    pal_trace_impl_UpdateColors(x, y, width, height, translation);
    if (traced) pal_boottrace("DDI UpdateColors RETURN");
}
#endif
