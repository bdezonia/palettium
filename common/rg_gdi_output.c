#include "rg_gdi_output.h"
#include "rg_gdi_rop.h"

/* Only individual pixels are dispatched here: existing hardware pixel
 * overrides are honored automatically. No duplicate ET4000 renderer. */
static void draw_pixel(RG_Device *dev, int x, int y,
                       unsigned physical, unsigned rop2)
{
    rg_pixel old;
    if (rop2 == 13u) {
        dev->ops.pixel(dev, x, y, (rg_pixel)physical);
    } else {
        /* Dimensions and surface validity checked before this loop. */
        if (rg_get_pixel(dev, x, y, &old))
            dev->ops.pixel(dev, x, y,
                (rg_pixel)rg_gdi_rop2_byte(rop2, (rg_u8)physical,
                                           (rg_u8)old));
    }
}

int rg_gdi_output_rectangle(RG_Device *dev, RG_Rect r,
                            const RG_Rect *user_clip, const RG_Pen *pen,
                            const RG_Brush *brush, unsigned rop2,
                            unsigned bg_mode)
{
    RG_Rect clip;
    int x, y, border;
    unsigned color;
    if (!dev || dev->surface.format != RG_FMT_INDEX8 ||
        !dev->ops.pixel || !dev->surface.width || !dev->surface.height ||
        dev->surface.width > 32767u || dev->surface.height > 32767u ||
        rop2 < 1u || rop2 > 16u ||
        (pen && !rg_pen_valid(pen)) ||
        (brush && !rg_brush_valid(brush)) ||
        bg_mode < 1u || bg_mode > 2u ||
        r.left > r.right || r.top > r.bottom) return 0;
    clip.left = 0; clip.top = 0;
    clip.right = (int)dev->surface.width;
    clip.bottom = (int)dev->surface.height;
    if (user_clip) {
        if (user_clip->left > user_clip->right ||
            user_clip->top > user_clip->bottom) return 0;
        if (clip.left < user_clip->left) clip.left = user_clip->left;
        if (clip.top < user_clip->top) clip.top = user_clip->top;
        if (clip.right > user_clip->right) clip.right = user_clip->right;
        if (clip.bottom > user_clip->bottom) clip.bottom = user_clip->bottom;
    }
    if (clip.left < r.left) clip.left = r.left;
    if (clip.top < r.top) clip.top = r.top;
    if (clip.right > r.right) clip.right = r.right;
    if (clip.bottom > r.bottom) clip.bottom = r.bottom;
    if (clip.left >= clip.right || clip.top >= clip.bottom) return 1;
    /* Interior first, border second per the Windows 3.x DDK contract. */
    if (brush && brush->style != RG_BRUSH_HOLLOW) {
        for (y=clip.top; y<clip.bottom; ++y) {
            for (x=clip.left; x<clip.right; ++x) {
                /* A border pixel must see the ROP exactly once. Filling
                 * it first would corrupt XOR/invert (and styled) output. */
                border = pen && pen->style != RG_PEN_NULL &&
                         (x == r.left || x == r.right-1 ||
                          y == r.top || y == r.bottom-1);
                if (border) continue;
                color = brush->foreground;
                if (brush->style == RG_BRUSH_HATCHED &&
                    !rg_brush_hatch_foreground(brush, x, y)) {
                    if (bg_mode == 1u) continue; /* TRANSPARENT */
                    color = brush->background;
                }
                draw_pixel(dev, x, y, color, rop2);
            }
        }
    }
    if (pen && pen->style != RG_PEN_NULL) {
        for (y=clip.top; y<clip.bottom; ++y) {
            for (x=clip.left; x<clip.right; ++x) {
                border = (x == r.left || x == r.right-1 ||
                          y == r.top || y == r.bottom-1);
                if (border) draw_pixel(dev, x, y, pen->color, rop2);
            }
        }
    }
    return 1;
}
