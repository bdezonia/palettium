#include <stddef.h>
#include "pal_bitmap.h"

int pal_ddi_bitmap_layout_ok(void)
{
    return sizeof(PAL_BITMAP16) == PAL_BITMAP16_BYTES &&
           offsetof(PAL_BITMAP16, bmType) == 0u &&
           offsetof(PAL_BITMAP16, bmWidthBytes) == 6u &&
           offsetof(PAL_BITMAP16, bmPlanes) == 8u &&
           offsetof(PAL_BITMAP16, bmBitsPixel) == 9u &&
           offsetof(PAL_BITMAP16, bmBits) == 10u &&
           offsetof(PAL_BITMAP16, bmWidthPlanes) == 14u &&
           offsetof(PAL_BITMAP16, bmlpPDevice) == 18u &&
           offsetof(PAL_BITMAP16, bmScanSegment) == 24u &&
           offsetof(PAL_BITMAP16, reserved2) == 30u;
}

int pal_ddi_memory_bitmap_ok(const PAL_BITMAP16 *bmp)
{
    unsigned long total, row;
    if (!bmp || !pal_ddi_bitmap_layout_ok()) return 0;
    if (bmp->bmType != 0 || bmp->bmWidth <= 0 || bmp->bmHeight <= 0 ||
        bmp->bmWidthBytes <= 0 || bmp->bmPlanes != 1 ||
        (bmp->bmBitsPixel != 1 && bmp->bmBitsPixel != 8) ||
        bmp->bmBits == 0UL) return 0;
    row = ((unsigned long)(unsigned)bmp->bmWidth *
           (unsigned long)bmp->bmBitsPixel + 7UL) / 8UL;
    if ((unsigned long)(unsigned)bmp->bmWidthBytes < row) return 0;
    total = (unsigned long)(unsigned)bmp->bmHeight *
            (unsigned long)(unsigned)bmp->bmWidthBytes;
    /* First version: single-segment 8-bit memory bitmaps only.
     * Don't pretend a >64 KB DDB is directly addressable in Win16.
     * Windows-specific segmented access will be added separately. */
    if (total > 65535UL || bmp->bmWidthPlanes != total ||
        (bmp->bmBits & 65535UL) + total > 65536UL) return 0;
    if (bmp->bmSegmentIndex != 0 || bmp->bmScanSegment != 0 ||
        bmp->bmFillBytes != 0) return 0;
    return 1;
}

int pal_ddi_target_kind(PAL_U16 first_word,
                        const PAL_BITMAP16 *bmp,
                        const PAL_PDEVICE *pd)
{
    if (first_word == 0u)
        return pal_ddi_memory_bitmap_ok(bmp) ? PAL_TARGET_MEMORY :
                                                PAL_TARGET_INVALID;
    if (!pd || pd->type != first_word || !pal_pd_valid(pd))
        return PAL_TARGET_INVALID;
    return PAL_TARGET_SCREEN;
}
