/* Win16 exported ABI probe. This is not a display driver. */
#include <windows.h>
#include "pal_bitmap.h"
WORD FAR PASCAL __export PalettiumBitmapSelfTest(void)
{
    PAL_BITMAP16 b;
    PAL_PDEVICE pd;
    if (!pal_ddi_bitmap_layout_ok()) return 1;
    if (!pal_pd_prepare(&pd, 640, 480, 640)) return 2;
    if (pal_ddi_target_kind(1, 0, &pd) != PAL_TARGET_SCREEN) return 3;
    b.bmType=0;
    b.bmWidth=16;
    b.bmHeight=16;
    b.bmWidthBytes=16;
    b.bmPlanes=1;
    b.bmBitsPixel=8;
    b.bmBits=0x12340000UL;
    b.bmWidthPlanes=256UL;
    b.bmlpPDevice=0UL;
    b.bmSegmentIndex=0;
    b.bmScanSegment=0;
    b.bmFillBytes=0;
    b.reserved1=0;
    b.reserved2=0;
    if (pal_ddi_target_kind(0, &b, 0) != PAL_TARGET_MEMORY) return 4;
    return 0;
}
BOOL FAR PASCAL LibMain(HINSTANCE instance, WORD ds, WORD heap, LPSTR cmd)
{
    (void)instance; (void)ds; (void)heap; (void)cmd;
    return TRUE;
}
