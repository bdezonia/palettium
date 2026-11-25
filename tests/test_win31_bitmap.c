#include <stdio.h>
#include <string.h>
#include "pal_bitmap.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; \
} } while (0)

int main(void)
{
    PAL_BITMAP16 b;
    PAL_PDEVICE pd;
    CHECK(pal_ddi_bitmap_layout_ok());
    memset(&b, 0, sizeof(b));
    b.bmWidth = 32;
    b.bmHeight = 16;
    b.bmWidthBytes = 32;
    b.bmPlanes = 1;
    b.bmBitsPixel = 8;
    b.bmBits = 0x12340000UL; /* symbolic Win16 segmented address */
    b.bmWidthPlanes = 512;
    CHECK(pal_ddi_memory_bitmap_ok(&b));
    CHECK(pal_ddi_target_kind(0, &b, 0) == PAL_TARGET_MEMORY);
    b.bmType = 1;
    CHECK(!pal_ddi_memory_bitmap_ok(&b));
    b.bmType = 0;
    b.bmBitsPixel = 4;
    CHECK(!pal_ddi_memory_bitmap_ok(&b));
    b.bmBitsPixel = 8;
    b.bmWidthPlanes = 511;
    CHECK(!pal_ddi_memory_bitmap_ok(&b));
    b.bmWidthPlanes = 512;
    b.bmHeight = 3000;
    b.bmWidthPlanes = 96000;
    CHECK(!pal_ddi_memory_bitmap_ok(&b));
    b.bmHeight = 16;
    b.bmWidthPlanes = 512;
    /* Actual GDI startup bitmap headers from v0059. */
    b.bmWidth = 36; b.bmHeight = 18; b.bmWidthBytes = 36;
    b.bmBits = 0x062f0020UL; b.bmWidthPlanes = 648;
    b.bmScanSegment = 0x071c; b.bmFillBytes = 16;
    CHECK(pal_ddi_memory_bitmap_ok(&b));
    b.bmWidth = 13; b.bmHeight = 14; b.bmWidthBytes = 2;
    b.bmBitsPixel = 1; b.bmWidthPlanes = 28;
    b.bmScanSegment = 0x8000; b.bmFillBytes = 0;
    CHECK(pal_ddi_memory_bitmap_ok(&b));
    b.bmSegmentIndex = 8;
    CHECK(!pal_ddi_memory_bitmap_ok(&b));
    b.bmSegmentIndex = 0; b.bmBits = 0x062ffff0UL;
    CHECK(!pal_ddi_memory_bitmap_ok(&b));
    CHECK(pal_pd_prepare(&pd, 640, 480, 640));
    CHECK(pal_ddi_target_kind(PAL_PD_DISPLAY_TYPE, 0, &pd) == PAL_TARGET_SCREEN);
    CHECK(pal_ddi_target_kind(1, 0, &pd) == PAL_TARGET_INVALID);
    CHECK(pal_ddi_target_kind(2, 0, &pd) == PAL_TARGET_INVALID);
    CHECK(pal_ddi_target_kind(0, 0, &pd) == PAL_TARGET_INVALID);
    pd.magic = 0;
    CHECK(pal_ddi_target_kind(PAL_PD_DISPLAY_TYPE, 0, &pd) == PAL_TARGET_INVALID);
    puts("PASS: Win31 32-byte BITMAP ABI, memory/screen discriminator, safe rejection");
    return 0;
}
