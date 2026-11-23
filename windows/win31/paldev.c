/* Win16 structure/lifecycle probe. Not a Windows display driver. */
#include <windows.h>
#include "pal_pdevice.h"
WORD FAR PASCAL __export PalettiumPDeviceSelfTest(void)
{
    PAL_PDEVICE pd;
    PAL_GDIINFO info;
    if (!pal_pd_layout_ok() || !pal_win31_layout_ok()) return 1;
    if (!pal_pd_prepare(&pd,640,480,640)) return 2;
    if (!pal_pd_capability_candidate(&info,&pd)) return 3;
    if (info.dpDEVICEsize != sizeof(pd)) return 4;
    if (!pal_pd_activate(&pd)) return 5;
    if (!pal_pd_stop(&pd)) return 6;
    return 0;
}
BOOL FAR PASCAL LibMain(HINSTANCE instance, WORD ds, WORD heap, LPSTR cmd)
{
    (void)instance; (void)ds; (void)heap; (void)cmd;
    return TRUE;
}
