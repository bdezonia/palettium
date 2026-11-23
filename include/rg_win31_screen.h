#ifndef RG_WIN31_SCREEN_H
#define RG_WIN31_SCREEN_H
/* Adapter-neutral Win3.x screen-screen BitBlt dispatch.
 * This module never owns VRAM or decodes a Win16 far pointer.
 * A real OS adapter must validate the PDEVICE and establish its binding.
 */
#include "rgfx.h"
#include "rg_gdi_blit.h"

typedef struct RG_Win31Screen {
    RG_Device *device;
    unsigned width;
    unsigned height;
    unsigned pitch;
    int active;
} RG_Win31Screen;

int rg_win31_screen_bind(RG_Win31Screen *screen, RG_Device *device,
                         unsigned width, unsigned height, unsigned pitch);
void rg_win31_screen_deactivate(RG_Win31Screen *screen);
int rg_win31_screen_bitblt(RG_Win31Screen *dst, RG_Win31Screen *src,
                            int dx, int dy, int sx, int sy,
                            int width, int height, unsigned long rop);
#endif
