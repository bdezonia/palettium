/* Palettium Win16 GDI BitBlt boundary, v0020.
 * Compiled as a Win16 object only. NOT AN INSTALLABLE DISPLAY DRIVER.
 * No ET4000-specific code. An eventual Enable() must establish the host.
 */
#define BitBlt PalWinApiBitBlt
#include <windows.h>
#undef BitBlt
#include <dos.h>
#include "pal_ddl_blit.h"

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
