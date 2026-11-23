#ifndef PAL_SCREEN_DDI_H
#define PAL_SCREEN_DDI_H
/* OS-private bridge. A Win16 far pointer is represented by its 32-bit
 * segment:offset token, NOT by a host C pointer or a VRAM offset. */
#include "pal_pdevice.h"
#include "rg_win31_screen.h"

typedef struct PAL_ScreenDDI {
    PAL_PDEVICE *pdevice;       /* pointer owned by OS adapter */
    RG_Win31Screen screen;      /* binds an already initialized RG_Device */
    PAL_U32 screen_token;       /* segment:offset identity of PDEVICE */
    int installed;
} PAL_ScreenDDI;

/* Only call after actual video-mode setup and VRAM initialization succeed.
 * The token must be the far address of the SAME PDEVICE supplied by GDI.
 * Never use zero, which would accidentally match a null DDI operand. */
int pal_screen_ddi_attach(PAL_ScreenDDI *bridge, PAL_PDEVICE *pd,
                          RG_Device *device, PAL_U32 far_token);
void pal_screen_ddi_detach(PAL_ScreenDDI *bridge);

/* Screen-to-screen only. Memory BITMAPs are deliberately not supported.
 * Tokens come from OS adapter; no untrusted DDI pointer is dereferenced. */
int pal_screen_ddi_bitblt(PAL_ScreenDDI *bridge,
                          PAL_U32 dst_token, int dx, int dy,
                          PAL_U32 src_token, int sx, int sy,
                          int width, int height, unsigned long rop);
#endif
