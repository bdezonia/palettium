#include "rg_gdi_blit.h"
#include "rg_gdi_rop.h"

int rg_gdi_blit_rop3(RG_Device *dst, RG_Device *src,
                     int dx, int dy, int sx, int sy,
                     int width, int height, unsigned long win_rop)
{
    int rop2;
    if (!dst || !src || width <= 0 || height <= 0) return 0;
    if (!dst->surface.width || !dst->surface.height ||
        !src->surface.width || !src->surface.height ||
        dst->surface.stride < dst->surface.width ||
        src->surface.stride < src->surface.width) return 0;
    if ((!dst->surface.pixels && (!dst->vram.read_byte || !dst->vram.write_byte)) ||
        (!src->surface.pixels && (!src->vram.read_byte || !src->vram.write_byte)))
        return 0;
    rop2 = rg_gdi_rop3_to_rop2(rg_gdi_rop3_code(win_rop));
    if (rop2 < 0) return 0;
    /* Preserve card-specific hardware overrides when present. */
    if (!dst->ops.blit) return 0;
    dst->ops.blit(dst, src, dx, dy, sx, sy, width, height, (unsigned)rop2);
    return 1;
}
