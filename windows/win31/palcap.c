/* Win16 ABI-only test DLL. Not a display driver. */
#include <windows.h>
#include "pal_gdiinfo.h"
WORD FAR PASCAL __export PalettiumCapLayout(void)
{
    PAL_GDIINFO info;
    if (!pal_win31_layout_ok()) return 0;
    if (!pal_win31_make_candidate(&info, 640, 480, 64)) return 0;
    if (info.dpVersion != 0x030A || info.dpBitsPixel != 8) return 0;
    return (WORD)sizeof(info); /* expected 110 */
}
BOOL FAR PASCAL LibMain(HINSTANCE h, WORD s, WORD z, LPSTR arg)
{
    (void)h; (void)s; (void)z; (void)arg;
    return TRUE;
}
