#include "pal_bitmap_device.h"

int pal_ddi_bitmap_device(RG_Device *device, const PAL_BITMAP16 *bitmap,
                          RG_VramOps bytes, void *context)
{
    RG_PixelFormat fmt;
    if (!device || !pal_ddi_memory_bitmap_ok(bitmap) ||
        !bytes.read_byte || !bytes.write_byte) return 0;
    fmt = bitmap->bmBitsPixel == 1 ? RG_FMT_INDEX1 : RG_FMT_INDEX8;
    return rg_init_vram(device, (unsigned)bitmap->bmWidth,
                        (unsigned)bitmap->bmHeight,
                        (rg_offset)(unsigned)bitmap->bmWidthBytes,
                        (rg_offset)bitmap->bmWidthPlanes, fmt, bytes, context);
}
