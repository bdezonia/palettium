#ifndef PAL_BITMAP_H
#define PAL_BITMAP_H
/* Windows 3.x display DDI BITMAP header, not Win32 BITMAP.
 * Historical DDK: first WORD 0 means system-memory bitmap;
 * nonzero means caller supplied the driver's private PDEVICE.
 * Do not dereference bmBits here: it is a segmented far pointer.
 */
#include "pal_pdevice.h"

#pragma pack(push, 1)
typedef struct PAL_BITMAP16 {
    PAL_S16 bmType;
    PAL_S16 bmWidth;
    PAL_S16 bmHeight;
    PAL_S16 bmWidthBytes;
    unsigned char bmPlanes;
    unsigned char bmBitsPixel;
    PAL_U32 bmBits;            /* Win16 FAR address, not a host pointer */
    PAL_U32 bmWidthPlanes;
    PAL_U32 bmlpPDevice;       /* Win16 FAR address, not a host pointer */
    PAL_S16 bmSegmentIndex;
    PAL_S16 bmScanSegment;
    PAL_S16 bmFillBytes;
    PAL_S16 reserved1;
    PAL_S16 reserved2;
} PAL_BITMAP16;
#pragma pack(pop)

#define PAL_BITMAP16_BYTES 32u
#define PAL_TARGET_INVALID 0
#define PAL_TARGET_MEMORY  1
#define PAL_TARGET_SCREEN  2

/* Caller supplies the first word, not an untrusted near pointer cast.
 * Supports single-segment 1bpp monochrome and 8bpp indexed bitmaps.
 * No pixel access, no mode switching, and no GDI exports occur here. */
int pal_ddi_bitmap_layout_ok(void);
int pal_ddi_memory_bitmap_ok(const PAL_BITMAP16 *bmp);
int pal_ddi_target_kind(PAL_U16 first_word,
                        const PAL_BITMAP16 *bmp,
                        const PAL_PDEVICE *pd);

#endif
