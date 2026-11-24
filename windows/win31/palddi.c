/* Palettium Win16 GDI entry-point staging implementation.
 * Compiled as a Win16 object only. NOT AN INSTALLABLE DISPLAY DRIVER.
 * No ET4000-specific code. Enable deliberately fails until ready.
 */
/* SDK names sometimes overlap Win16 display-DDI exports. */
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
#include "pal_objects.h"
#include "pal_bitmap_device.h"
#include "rg_gdi_blit.h"

static PAL_ScreenHost *pal_active_host = 0;

/* Used only by a future verified Enable/Disable implementation, not exported.
 * Never hand out a bound host until video mode and PDEVICE setup succeed.
 */
void FAR PASCAL pal_win16_bind_host(PAL_ScreenHost *host)
{
    pal_active_host = host;
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
    if (!pal_ddi_memory_bitmap_ok(&bitmap)) return 0;
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
    if (bg > 255UL || fg > 255UL) return 0;
    *background=(unsigned)bg;
    *text=(unsigned)fg;
    return 1;
}

/* DDK: BitBlt(dest,dx,dy,src,sx,sy,w,h,rop,brush,drawmode).
 * Same-screen keeps the existing hardware blit path. All 1/8bpp
 * memory combinations use shared software/hardware RG_Device operations.
 * Pattern-dependent ROP3 still refuses without writing. */
BOOL FAR PASCAL BitBlt(LPVOID dest, short dx, short dy,
                       LPVOID src, short sx, short sy,
                       short width, short height, DWORD rop,
                       LPVOID brush, LPVOID drawmode)
{
    PAL_U32 dt, st;
    PAL_Win16Bits dbits, sbits;
    RG_Device dmem, smem, *d, *s;
    unsigned text=0, background=0;
    (void)brush;
    if (!pal_active_host || !dest || !src || width <= 0 || height <= 0)
        return FALSE;
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

/* Pixel @9: DRAWMODE starts with a 16-bit Rop2. A null DRAWMODE
 * requests a read, for which the color argument is ignored.
 * The physical color for INDEX8 is the already-translated palette index.
 * All other devices must be refused before touching VRAM.
 */
DWORD FAR PASCAL Pixel(LPVOID dest, short x, short y,
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

/* RealizeObject @10: Win3.x DDK, GDI-owned physical storage. Pens and
 * solid/hollow/hatched brushes are private, WORD-aligned objects shared
 * with the common renderer. The DDK LOGBRUSH has a physical background
 * DWORD at byte 8 (unlike the SDK structure). No indirect allocations:
 * deletions are no-ops. Fonts remain GDI-realized; device bitmaps are off.
 */
DWORD FAR PASCAL RealizeObject(LPVOID dest, short style, LPVOID input,
                               LPVOID output, LPVOID text_xform)
{
    PAL_U32 token;
    const PAL_LogicalPen FAR *in_pen;
    const PAL_LogicalBrush FAR *in_brush;
    const PAL_POINT16 FAR *phase;
    RG_Pen phys_pen;
    RG_Brush phys_brush;
    const unsigned char *source;
    unsigned char FAR *target;
    unsigned i, size;
    if (!pal_active_host || !pal_active_host->running || !dest ||
        !pal_object_layout_ok()) return 0;
    token = pal_ddi_far_token((PAL_U16)FP_SEG(dest),
                              (PAL_U16)FP_OFF(dest));
    if (token != pal_active_host->ddi.screen_token) return 0;
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

/* Output @8: GDI's required OS_SCANLINES (4) and OS_POLYLINE (18),
 * the existing OS_RECTANGLE (6), plus harmless BEGIN/END scan hints.
 * Other shapes return -1 for GDI synthesis. Screen identity must be valid
 * before marshalling or touching the video memory. Scanline POINT[0].y
 * gives the common Y; POINT[1..count-1] each hold an (left,right) X pair.
 * Each polyline segment excludes its final endpoint, preventing double XOR
 * at joins. Bounds/ROP/physical objects are checked before drawing.
 */
WORD FAR PASCAL Output(LPVOID dest, WORD style, WORD count,
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

/* Windows 3.x DDI: style 1 requests GDIINFO, style 0 requests
 * physical initialization. Capability discovery is safe to exercise;
 * actual enablement remains deliberately disabled until mandatory
 * GDI services and enhanced-mode switching are implemented.
 */
WORD FAR PASCAL Enable(LPVOID destination, WORD style,
                       LPSTR device_type, LPSTR output_file, LPVOID data)
{
    PAL_GDIINFO candidate;
    PAL_ET4KPlan plan;
    unsigned char FAR *out;
    const unsigned char *in;
    unsigned i;
    (void)device_type; (void)output_file; (void)data;
    if (style != 1u || !destination) return 0;
    /* Never advertise 640x480 on an unknown or 256KiB card. Until we
     * have an authorized in-Windows capacity probe, require explicit
     * [Palettium] VRAMKiB in SYSTEM.INI. This is still just a candidate
     * GDIINFO; style=0 remains blocked. The planner caps stock-VDD
     * enhanced-mode candidates to a <=256KiB framebuffer. */
    if ((PAL_U32)(PAL_U16)FP_OFF(destination) +
        (PAL_U32)sizeof(candidate) > 65536UL ||
        !pal_win16_et4k_profile_plan(&plan) ||
        !pal_et4k_plan_capability(&candidate,&plan)) return 0;
    out = (unsigned char FAR *)destination;
    in = (const unsigned char *)&candidate;
    for (i = 0; i < sizeof(candidate); ++i) out[i] = in[i];
    return (WORD)sizeof(candidate);
}

void FAR PASCAL Disable(LPVOID physical_device)
{
    (void)physical_device;
    pal_active_host = 0;
}

/* ColorInfo @2: GDI passes a logical COLORREF (0x00BBGGRR) or an
 * explicit 0xFF00iiii palette index. Physical color is the framebuffer
 * index, NOT a DAC RGB value. Avoid dereferencing foreign PDEVICEs.
 */
DWORD FAR PASCAL ColorInfo(LPVOID device, DWORD color, LPVOID physical)
{
    PAL_U32 logical, index;
    PAL_U32 token;
    if (!pal_active_host || !device) return 0;
    token = pal_ddi_far_token((PAL_U16)FP_SEG(device),
                              (PAL_U16)FP_OFF(device));
    if (!pal_screen_host_colorinfo(pal_active_host, token, (PAL_U32)color,
                                   physical != 0, &logical, &index)) return 0;
    if (physical) *(DWORD FAR *)physical = (DWORD)index;
    return (DWORD)logical;
}

/* ScanLR @12, flood-fill horizontal search. Signed -1 means not found;
 * 0x8000 means invalid device, coordinates, or unsupported bitmap.
 */
WORD FAR PASCAL ScanLR(LPVOID device, short x, short y,
                       DWORD physical, WORD style)
{
    int found;
    PAL_U32 token;
    if (!pal_active_host || !device) return 0x8000u;
    token = pal_ddi_far_token((PAL_U16)FP_SEG(device),
                              (PAL_U16)FP_OFF(device));
    if (!pal_screen_host_scanlr(pal_active_host, token, x, y,
                                (PAL_U32)physical, style, &found))
        return 0x8000u;
    return (WORD)found;
}

/* Win3.x palette DDIs have no PDEVICE argument; they are valid only
 * after the screen host has completed hardware and PDEVICE startup.
 * Keep Win16 far-memory copying here; core palette owns color policy.
 */
void FAR PASCAL SetPalette(short start, short count, LPVOID palette)
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

void FAR PASCAL GetPalette(short start, short count, LPVOID palette)
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

void FAR PASCAL SetPalTrans(LPVOID translation)
{
    const WORD FAR *src = (const WORD FAR *)translation;
    unsigned short table[256];
    unsigned i;
    if (!pal_active_host || !pal_active_host->running) return;
    if (src) for (i=0; i<256u; ++i) table[i] = src[i];
    (void)rg_palette_set_translation(&pal_active_host->palette,
                                      src ? table : 0);
}

void FAR PASCAL GetPalTrans(LPVOID translation)
{
    WORD FAR *dst = (WORD FAR *)translation;
    unsigned i;
    if (!pal_active_host || !pal_active_host->running || !dst) return;
    for (i=0; i<256u; ++i)
        dst[i] = (WORD)pal_active_host->palette.logical_to_physical[i];
}

/* UpdateColors @26: x/y/extents, followed by a 256-WORD physical-index
 * translation table. Bounds and all 256 values are validated by core
 * BEFORE the first pixel is touched.
 */
void FAR PASCAL UpdateColors(WORD x, WORD y, WORD width, WORD height,
                             LPVOID translation)
{
    const WORD FAR *src = (const WORD FAR *)translation;
    unsigned short table[256];
    unsigned i;
    if (!pal_active_host || !pal_active_host->running || !src) return;
    for (i=0; i<256u; ++i) table[i] = src[i];
    (void)pal_screen_host_update_colors(pal_active_host, x, y,
                                        width, height, table);
}
