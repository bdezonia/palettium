#ifndef PAL_BITMAP_DEVICE_H
#define PAL_BITMAP_DEVICE_H
/* Win3.x memory BITMAP -> generic RG_Device adapter. The Windows 16-bit
 * segment:offset is NOT a host pointer. Callers supply checked byte I/O
 * for a single 64K segment; this module never assumes a flat address. */
#include "pal_bitmap.h"
#include "rgfx.h"

int pal_ddi_bitmap_device(RG_Device *device, const PAL_BITMAP16 *bitmap,
                          RG_VramOps bytes, void *context);

#endif
