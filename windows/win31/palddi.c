/* Palettium Win16 GDI entry-point staging implementation.
 * Compiled as a Win16 object only. NOT AN INSTALLABLE DISPLAY DRIVER.
 * No ET4000-specific code. Enable deliberately fails until ready.
 */
/* SDK names sometimes overlap Win16 display-DDI exports. */
#define BitBlt PalWinApiBitBlt
#define ColorInfo PalWinApiColorInfo
#define ScanLR PalWinApiScanLR
#define SetPalette PalWinApiSetPalette
#define GetPalette PalWinApiGetPalette
#define SetPalTrans PalWinApiSetPalTrans
#define GetPalTrans PalWinApiGetPalTrans
#define UpdateColors PalWinApiUpdateColors
#include <windows.h>
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

static PAL_ScreenHost *pal_active_host = 0;

/* Used only by a future verified Enable/Disable implementation, not exported.
 * Never hand out a bound host until video mode and PDEVICE setup succeed.
 */
void FAR PASCAL pal_win16_bind_host(PAL_ScreenHost *host)
{
    pal_active_host = host;
}

/* DDK: BitBlt(dest,dx,dy,src,sx,sy,w,h,rop,brush,drawmode).
 * Return zero for unsupported request. All GDI pointers remain opaque.
 * The host-side bridge rejects memory bitmap operands by identity.
 */
BOOL FAR PASCAL BitBlt(LPVOID dest, short dx, short dy,
                       LPVOID src, short sx, short sy,
                       short width, short height, DWORD rop,
                       LPVOID brush, LPVOID drawmode)
{
    (void)brush; (void)drawmode;
    if (!pal_active_host) return FALSE;
    return pal_ddi_bitblt_words(pal_active_host,
               (PAL_U16)FP_SEG(dest), (PAL_U16)FP_OFF(dest), dx, dy,
               (PAL_U16)FP_SEG(src), (PAL_U16)FP_OFF(src), sx, sy,
               width, height, (unsigned long)rop) ? TRUE : FALSE;
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

/* Output @8: returning -1 is the DDK-defined instruction for GDI to
 * simulate the unsupported figure. Returning 0 would mean hard failure.
 * It is safe to export while no drawing styles are yet accelerated.
 */
WORD FAR PASCAL Output(LPVOID dest, WORD style, WORD count,
                        LPVOID points, LPVOID pen, LPVOID brush,
                        LPVOID drawmode, LPVOID cliprect)
{
    (void)dest; (void)style; (void)count; (void)points;
    (void)pen; (void)brush; (void)drawmode; (void)cliprect;
    return (WORD)-1;
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
    unsigned char FAR *out;
    const unsigned char *in;
    unsigned i;
    (void)device_type; (void)output_file; (void)data;
    if (style != 1u || !destination) return 0;
    if (!pal_enable_capability(&candidate, 640u, 480u)) return 0;
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
