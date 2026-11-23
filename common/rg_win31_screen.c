#include "rg_win31_screen.h"

int rg_win31_screen_bind(RG_Win31Screen *screen, RG_Device *device,
                         unsigned width, unsigned height, unsigned pitch)
{
    if (!screen || !device || !width || !height || pitch < width ||
        device->surface.width != width || device->surface.height != height ||
        device->surface.stride != pitch || !device->ops.blit ||
        (!device->surface.pixels &&
         (!device->vram.read_byte || !device->vram.write_byte))) return 0;
    screen->device = device;
    screen->width = width;
    screen->height = height;
    screen->pitch = pitch;
    screen->active = 1;
    return 1;
}

void rg_win31_screen_deactivate(RG_Win31Screen *screen)
{
    if (screen) screen->active = 0;
}

int rg_win31_screen_bitblt(RG_Win31Screen *dst, RG_Win31Screen *src,
                            int dx, int dy, int sx, int sy,
                            int width, int height, unsigned long rop)
{
    if (!dst || !src || !dst->active || !src->active ||
        !dst->device || !src->device ||
        dst->device->surface.width != dst->width ||
        dst->device->surface.height != dst->height ||
        dst->device->surface.stride != dst->pitch ||
        src->device->surface.width != src->width ||
        src->device->surface.height != src->height ||
        src->device->surface.stride != src->pitch) return 0;
    return rg_gdi_blit_rop3(dst->device, src->device,
                             dx, dy, sx, sy, width, height, rop);
}
