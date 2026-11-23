#include <string.h>
#include "pal_screen_ddi.h"

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
