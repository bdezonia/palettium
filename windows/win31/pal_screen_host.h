#ifndef PAL_SCREEN_HOST_H
#define PAL_SCREEN_HOST_H
/* OS-specific lifecycle coordinator. No ET4000 registers or BIOS calls.
 * All storage is caller-owned and must outlive an opened session.
 * This is NOT an installable Windows display driver. */
#include "pal_screen_ddi.h"
#include "rg_hardware.h"
#include "rg_palette.h"
#include "rg_gdi_output.h"

typedef struct PAL_ScreenHost {
    PAL_PDEVICE pdevice;
    RG_Device device;
    RG_HwBinding binding;
    PAL_ScreenDDI ddi;
    RG_Palette palette;
    unsigned running;
} PAL_ScreenHost;

/* Pass an OS-created far-address token for host.pdevice. In production the
 * Win16 adapter must compute it from the actual DDI far pointer. */
int pal_screen_host_start(PAL_ScreenHost *host, RG_Hardware *hardware,
                          unsigned width, unsigned height,
                          PAL_U32 pdevice_token);
void pal_screen_host_stop(PAL_ScreenHost *host);
/* Serialize the initialized private PDEVICE for the storage GDI allocated.
 * The Win16 adapter supplies the exact far-pointer token for that storage;
 * only an already-running host bound to the SAME token may publish it.
 * No hardware access and no change to the live DDI binding occurs here.
 * The adapter must separately copy these bytes to the GDI FAR buffer
 * before exposing the host to exported drawing functions. */
int pal_screen_host_gdi_snapshot(PAL_ScreenHost *host,
                                  PAL_U32 gdi_far_token,
                                  PAL_PDEVICE *image);
int pal_screen_host_bitblt(PAL_ScreenHost *host,
                           PAL_U32 dst_token, int dx, int dy,
                           PAL_U32 src_token, int sx, int sy,
                           int width, int height, unsigned long rop);
int pal_screen_host_pixel(PAL_ScreenHost *host, PAL_U32 token,
                          int x, int y, PAL_U32 physical_color,
                          unsigned rop2, PAL_U32 *result);
int pal_screen_host_palette(PAL_ScreenHost *host, unsigned index,
                            rg_u8 red, rg_u8 green, rg_u8 blue);
int pal_screen_host_colorinfo(PAL_ScreenHost *host, PAL_U32 token,
                               PAL_U32 input, int forward,
                               PAL_U32 *logical, PAL_U32 *physical);
int pal_screen_host_update_colors(PAL_ScreenHost *host,
                                  unsigned x, unsigned y,
                                  unsigned width, unsigned height,
                                  const unsigned short *table);
int pal_screen_host_scanlr(PAL_ScreenHost *host, PAL_U32 token,
                           int x, int y, PAL_U32 physical,
                           unsigned style, int *found);
/* Screen-only Output primitives. Physical objects come from RealizeObject. */
int pal_screen_host_rectangle(PAL_ScreenHost *host, PAL_U32 token,
                              RG_Rect bounds, const RG_Rect *clip,
                              const RG_Pen *pen, const RG_Brush *brush,
                              unsigned rop2, unsigned background_mode);
int pal_screen_host_scanline(PAL_ScreenHost *host, PAL_U32 token,
                              int y, int left, int right,
                              const RG_Rect *clip, const RG_Pen *pen,
                              const RG_Brush *brush, unsigned rop2,
                              unsigned background_mode);
int pal_screen_host_segment(PAL_ScreenHost *host, PAL_U32 token,
                             int x0, int y0, int x1, int y1,
                             const RG_Rect *clip, const RG_Pen *pen,
                             unsigned rop2);
/* Output's begin/end scan hints require a live screen token but no pixels. */
int pal_screen_host_output_ready(PAL_ScreenHost *host, PAL_U32 token);
#endif
