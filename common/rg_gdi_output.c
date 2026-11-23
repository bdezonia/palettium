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

static int output_clip(RG_Device *dev, const RG_Rect *user,
                       RG_Rect *out)
{
    if (!dev || !out || !dev->ops.pixel ||
        (dev->surface.format != RG_FMT_INDEX8 &&
         dev->surface.format != RG_FMT_INDEX1) ||
        !dev->surface.width || !dev->surface.height ||
        dev->surface.width > 32767u || dev->surface.height > 32767u)
        return 0;
    out->left=0; out->top=0;
    out->right=(int)dev->surface.width;
    out->bottom=(int)dev->surface.height;
    if (user) {
        if (user->left > user->right || user->top > user->bottom)
            return 0;
        if (out->left < user->left) out->left=user->left;
        if (out->top < user->top) out->top=user->top;
        if (out->right > user->right) out->right=user->right;
        if (out->bottom > user->bottom) out->bottom=user->bottom;
    }
    return 1;
}


int rg_gdi_output_rectangle(RG_Device *dev, RG_Rect r,
                            const RG_Rect *user_clip, const RG_Pen *pen,
                            const RG_Brush *brush, unsigned rop2,
                            unsigned bg_mode)
{
    RG_Rect clip;
    int x, y, border;
    unsigned color;
    if (!output_clip(dev,user_clip,&clip) ||
        rop2 < 1u || rop2 > 16u ||
        (pen && !rg_pen_valid(pen)) ||
        (brush && !rg_brush_valid(brush)) ||
        bg_mode < 1u || bg_mode > 2u ||
        r.left > r.right || r.top > r.bottom) return 0;
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

/* The required GDI synthesis primitives use shared pixel dispatch too.
 * rg_software_line has different endpoint and ROP semantics. */

int rg_gdi_output_scanline(RG_Device *dev, int y, int left, int right,
                           const RG_Rect *user_clip, const RG_Pen *pen,
                           const RG_Brush *brush, unsigned rop2,
                           unsigned bg_mode)
{
    RG_Rect clip;
    int x;
    unsigned color;
    if (!output_clip(dev,user_clip,&clip) || left > right ||
        rop2 < 1u || rop2 > 16u || bg_mode < 1u || bg_mode > 2u ||
        (pen && !rg_pen_valid(pen)) ||
        (brush && !rg_brush_valid(brush))) return 0;
    if (y < clip.top || y >= clip.bottom) return 1;
    if (left < clip.left) left=clip.left;
    if (right > clip.right) right=clip.right;
    if (left >= right) return 1;
    for (x=left; x<right; ++x) {
        if (brush) {
            if (brush->style == RG_BRUSH_HOLLOW) continue;
            color=brush->foreground;
            if (brush->style == RG_BRUSH_HATCHED &&
                !rg_brush_hatch_foreground(brush,x,y)) {
                if (bg_mode == 1u) continue;
                color=brush->background;
            }
        } else {
            if (!pen || pen->style == RG_PEN_NULL) continue;
            color=pen->color;
        }
        draw_pixel(dev,x,y,color,rop2);
    }
    return 1;
}

int rg_gdi_output_segment(RG_Device *dev, int x0, int y0,
                          int x1, int y1, const RG_Rect *user_clip,
                          const RG_Pen *pen, unsigned rop2)
{
    RG_Rect clip;
    long x=(long)x0, y=(long)y0;
    long dx=(long)x1-x, dy=(long)y1-y;
    long sx=(dx>0L)?1L:-1L, sy=(dy>0L)?1L:-1L;
    long ax=(dx<0L)?-dx:dx, ay=(dy<0L)?-dy:dy;
    long err, step, count;
    if (!output_clip(dev,user_clip,&clip) ||
        rop2 < 1u || rop2 > 16u || (pen && !rg_pen_valid(pen))) return 0;
    if (!pen || pen->style == RG_PEN_NULL || (ax==0L && ay==0L))
        return 1;
    /* Bresenham with one loop per pixel and the final endpoint excluded.
     * Longs avoid signed-16 overflow even for extreme Win16 POINT values. */
    if (ax >= ay) {
        err = ax / 2L;
        for (count=0; count<ax; ++count) {
            if (x >= (long)clip.left && x < (long)clip.right &&
                y >= (long)clip.top && y < (long)clip.bottom)
                draw_pixel(dev,(int)x,(int)y,pen->color,rop2);
            x += sx;
            err -= ay;
            if (err < 0L) { y += sy; err += ax; }
        }
    } else {
        err = ay / 2L;
        for (step=0; step<ay; ++step) {
            if (x >= (long)clip.left && x < (long)clip.right &&
                y >= (long)clip.top && y < (long)clip.bottom)
                draw_pixel(dev,(int)x,(int)y,pen->color,rop2);
            y += sy;
            err -= ax;
            if (err < 0L) { x += sx; err += ay; }
        }
    }
    return 1;
}
