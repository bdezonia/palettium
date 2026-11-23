#include <string.h>
#include "pal_screen_ddi.h"
#include "rg_gdi_rop.h"

int pal_screen_ddi_attach(PAL_ScreenDDI *bridge, PAL_PDEVICE *pd,
                          RG_Device *device, PAL_U32 far_token)
{
    if (!bridge || !pd || !device || !far_token ||
        !pal_pd_valid(pd) || pd->state != PAL_PD_ACTIVE ||
        !rg_win31_screen_bind(&bridge->screen, device,
                              pd->width, pd->height, pd->pitch)) return 0;
    bridge->pdevice = pd;
    bridge->screen_token = far_token;
    bridge->installed = 1;
    return 1;
}

void pal_screen_ddi_detach(PAL_ScreenDDI *bridge)
{
    if (!bridge) return;
    rg_win31_screen_deactivate(&bridge->screen);
    bridge->installed = 0;
    bridge->screen_token = 0;
    bridge->pdevice = 0;
}

int pal_screen_ddi_bitblt(PAL_ScreenDDI *bridge,
                          PAL_U32 dst_token, int dx, int dy,
                          PAL_U32 src_token, int sx, int sy,
                          int width, int height, unsigned long rop)
{
    if (!bridge || !bridge->installed || !bridge->pdevice ||
        !bridge->screen_token || dst_token != bridge->screen_token ||
        src_token != bridge->screen_token ||
        !pal_pd_valid(bridge->pdevice) ||
        bridge->pdevice->state != PAL_PD_ACTIVE ||
        bridge->pdevice->width != bridge->screen.width ||
        bridge->pdevice->height != bridge->screen.height ||
        bridge->pdevice->pitch != bridge->screen.pitch) return 0;
    return rg_win31_screen_bitblt(&bridge->screen, &bridge->screen,
                                 dx, dy, sx, sy, width, height, rop);
}

/* ROP2 values map directly to the 16 Boolean truth tables by subtracting 1.
 * Truth table input bit index is (source_bit << 1) | dest_bit, matching
 * the shared raster engine's raw-pixel ROP convention. */
int pal_screen_ddi_pixel(PAL_ScreenDDI *bridge, PAL_U32 device_token,
                         int x, int y, PAL_U32 physical_color,
                         unsigned rop2, PAL_U32 *result)
{
    RG_Device *device;
    rg_pixel old, value;
    if (!result || !bridge || !bridge->installed || !bridge->pdevice ||
        !bridge->screen_token || device_token != bridge->screen_token ||
        !pal_pd_valid(bridge->pdevice) ||
        bridge->pdevice->state != PAL_PD_ACTIVE ||
        !bridge->screen.active || !bridge->screen.device ||
        bridge->pdevice->width != bridge->screen.width ||
        bridge->pdevice->height != bridge->screen.height ||
        bridge->pdevice->pitch != bridge->screen.pitch)
        return 0;
    device = bridge->screen.device;
    if (device->surface.format != RG_FMT_INDEX8 ||
        device->surface.width != bridge->screen.width ||
        device->surface.height != bridge->screen.height ||
        device->surface.stride != bridge->screen.pitch ||
        x < 0 || y < 0 || (unsigned)x >= bridge->screen.width ||
        (unsigned)y >= bridge->screen.height || rop2 > 16u)
        return 0;
    if (!rop2) {
        if (!rg_get_pixel(device, x, y, &old)) return 0;
        *result = (PAL_U32)old;
        return 1;
    }
    if (physical_color > 255UL || !device->ops.pixel) return 0;
    if (rop2 == 13u) value = (rg_pixel)physical_color; /* R2_COPYPEN */
    else {
        if (!rg_get_pixel(device, x, y, &old)) return 0;
        value = rg_gdi_rop2_byte(rop2, (rg_u8)physical_color,
                                 (rg_u8)old);
    }
    device->ops.pixel(device, x, y, value);
    *result = 1UL;
    return 1;
}

int pal_screen_ddi_scanlr(PAL_ScreenDDI *bridge, PAL_U32 token,
                          int x, int y, PAL_U32 physical_color,
                          unsigned style, int *found)
{
    RG_Device *dev;
    rg_pixel value;
    int at, step, match;
    if (!bridge || !bridge->installed || !bridge->pdevice ||
        !bridge->screen.active || !bridge->screen.device ||
        !bridge->screen_token || token != bridge->screen_token ||
        !pal_pd_valid(bridge->pdevice) ||
        bridge->pdevice->state != PAL_PD_ACTIVE ||
        bridge->pdevice->width != bridge->screen.width ||
        bridge->pdevice->height != bridge->screen.height ||
        bridge->pdevice->pitch != bridge->screen.pitch ||
        !found || physical_color > 255UL || style > 3u ||
        x < 0 || y < 0 || (unsigned)x >= bridge->screen.width ||
        (unsigned)y >= bridge->screen.height) return 0;
    dev = bridge->screen.device;
    if (dev->surface.format != RG_FMT_INDEX8 ||
        dev->surface.width != bridge->screen.width ||
        dev->surface.height != bridge->screen.height ||
        dev->surface.stride != bridge->screen.pitch) return 0;
    step = (style & 2u) ? -1 : 1;
    match = !!(style & 1u);
    for (at=x; at>=0 && (unsigned)at<bridge->screen.width; at+=step) {
        if (!rg_get_pixel(dev, at, y, &value)) return 0;
        if ((value == physical_color) == match) { *found=at; return 1; }
    }
    *found = -1;
    return 1;
}
