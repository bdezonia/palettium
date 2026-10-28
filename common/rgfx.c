#include "rgfx.h"

int rg_init(RG_Device *dev, rg_u8 *pixels,
            unsigned width, unsigned height, unsigned stride)
{
    if (!dev || !pixels || !width || !height || stride < width)
        return 0;
    dev->surface.pixels = pixels;
    dev->surface.width = width;
    dev->surface.height = height;
    dev->surface.stride = stride;
    dev->card_data = 0;
    dev->ops.pixel = rg_software_pixel;
    dev->ops.line = rg_software_line;
    dev->ops.fill = rg_software_fill;
    return 1;
}

void rg_software_pixel(RG_Device *dev, int x, int y, rg_u8 color)
{
    if (x < 0 || y < 0 ||
        (unsigned)x >= dev->surface.width ||
        (unsigned)y >= dev->surface.height)
        return;
    dev->surface.pixels[(unsigned)y * dev->surface.stride + (unsigned)x] = color;
}

/* Bresenham. Deliberately independent of dev->ops.pixel to prevent
   accidental acceleration/fallback recursion. Caller should use bounded
   coordinates; high-range integer overflow protection comes later. */
void rg_software_line(RG_Device *dev, int x0, int y0,
                      int x1, int y1, rg_u8 color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int e2;
    for (;;) {
        rg_software_pixel(dev, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void rg_software_fill(RG_Device *dev, int x, int y,
                      int width, int height, rg_u8 color)
{
    int xx, yy;
    long left, top, right, bottom;
    if (width <= 0 || height <= 0) return;
    left = x; top = y;
    right = left + width; bottom = top + height;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > (long)dev->surface.width) right = (long)dev->surface.width;
    if (bottom > (long)dev->surface.height) bottom = (long)dev->surface.height;
    if (left >= right || top >= bottom) return;
    for (yy = (int)top; yy < (int)bottom; ++yy)
        for (xx = (int)left; xx < (int)right; ++xx)
            dev->surface.pixels[(unsigned)yy * dev->surface.stride + (unsigned)xx] = color;
}
