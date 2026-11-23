#ifndef PAL_SCREEN_HOST_H
#define PAL_SCREEN_HOST_H
/* OS-specific lifecycle coordinator. No ET4000 registers or BIOS calls.
 * All storage is caller-owned and must outlive an opened session.
 * This is NOT an installable Windows display driver. */
#include "pal_screen_ddi.h"
#include "rg_hardware.h"

typedef struct PAL_ScreenHost {
    PAL_PDEVICE pdevice;
    RG_Device device;
    RG_HwBinding binding;
    PAL_ScreenDDI ddi;
    unsigned running;
} PAL_ScreenHost;

/* Pass an OS-created far-address token for host.pdevice. In production the
 * Win16 adapter must compute it from the actual DDI far pointer. */
int pal_screen_host_start(PAL_ScreenHost *host, RG_Hardware *hardware,
                          unsigned width, unsigned height,
                          PAL_U32 pdevice_token);
void pal_screen_host_stop(PAL_ScreenHost *host);
int pal_screen_host_bitblt(PAL_ScreenHost *host,
                           PAL_U32 dst_token, int dx, int dy,
                           PAL_U32 src_token, int sx, int sy,
                           int width, int height, unsigned long rop);
int pal_screen_host_pixel(PAL_ScreenHost *host, PAL_U32 token,
                          int x, int y, PAL_U32 physical_color,
                          unsigned rop2, PAL_U32 *result);
int pal_screen_host_palette(PAL_ScreenHost *host, unsigned index,
                            rg_u8 red, rg_u8 green, rg_u8 blue);
#endif
