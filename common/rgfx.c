#include "rgfx.h"
#include <limits.h>

static int rg_geometry_ok(unsigned width, unsigned height, unsigned stride)
{
    if (!width || !height || stride < width) return 0;
    /* Last accessible byte offset must fit in rg_offset on the target. */
    if ((rg_offset)(height - 1) >
        (ULONG_MAX - (rg_offset)(width - 1)) / (rg_offset)stride)
        return 0;
    return 1;
}

static void rg_set_defaults(RG_Device *dev, unsigned width,
                            unsigned height, unsigned stride)
{
    dev->surface.width = width;
    dev->surface.height = height;
    dev->surface.stride = stride;
    dev->card_data = 0;
    dev->ops.pixel = rg_software_pixel;
    dev->ops.line = rg_software_line;
    dev->ops.fill = rg_software_fill;
    dev->ops.blit = rg_software_blit;
}

int rg_init(RG_Device *dev, rg_u8 *pixels,
            unsigned width, unsigned height, unsigned stride)
{
    if (!dev || !pixels || !rg_geometry_ok(width, height, stride)) return 0;
    rg_set_defaults(dev, width, height, stride);
    dev->surface.pixels = pixels;
    dev->vram.read_byte = 0;
    dev->vram.write_byte = 0;
    dev->vram_context = 0;
    return 1;
}

int rg_init_vram(RG_Device *dev, unsigned width, unsigned height,
                 unsigned stride, RG_VramOps ops, void *context)
{
    if (!dev || !ops.read_byte || !ops.write_byte ||
        !rg_geometry_ok(width, height, stride)) return 0;
    rg_set_defaults(dev, width, height, stride);
    dev->surface.pixels = 0;
    dev->vram = ops;
    dev->vram_context = context;
    return 1;
}

rg_u8 rg_read_byte(RG_Device *dev, rg_offset offset)
{
    if (dev->surface.pixels) return dev->surface.pixels[offset];
    return dev->vram.read_byte(dev->vram_context, offset);
}

void rg_write_byte(RG_Device *dev, rg_offset offset, rg_u8 value)
{
    if (dev->surface.pixels) dev->surface.pixels[offset] = value;
    else dev->vram.write_byte(dev->vram_context, offset, value);
}

void rg_software_pixel(RG_Device *dev, int x, int y, rg_u8 color)
{
    if (x < 0 || y < 0 || (unsigned)x >= dev->surface.width ||
        (unsigned)y >= dev->surface.height) return;
    rg_write_byte(dev, (rg_offset)(unsigned)y * dev->surface.stride +
                  (unsigned)x, color);
}

/* Bounded coordinates only; clip extreme lines before passing here. */
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
    long left, top, right, bottom;
    rg_offset row, pos;
    unsigned xx;
    if (width <= 0 || height <= 0) return;
    left = x; top = y;
    /* Safe for 16-bit signed int coordinates and 32-bit long. */
    right = left + (long)width;
    bottom = top + (long)height;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > (long)dev->surface.width) right = (long)dev->surface.width;
    if (bottom > (long)dev->surface.height) bottom = (long)dev->surface.height;
    if (left >= right || top >= bottom) return;
    for (row = (rg_offset)top; row < (rg_offset)bottom; ++row) {
        pos = row * dev->surface.stride + (rg_offset)left;
        for (xx = 0; xx < (unsigned)(right - left); ++xx)
            rg_write_byte(dev, pos + xx, color);
    }
}

/* Bitwise 2-input Boolean ROP: 4-bit truth table indexed by (S,D).
   Operates on all eight bitplanes independently. */
static rg_u8 rg_rop_apply(rg_u8 source, rg_u8 dest, unsigned rop)
{
    unsigned s = source;
    unsigned d = dest;
    unsigned v = 0;
    rop &= 15U;
    if (rop & 1U) v |= (~s & ~d);
    if (rop & 2U) v |= (~s & d);
    if (rop & 4U) v |= (s & ~d);
    if (rop & 8U) v |= (s & d);
    return (rg_u8)v;
}

void rg_software_blit(RG_Device *dst, RG_Device *src,
                      int dx, int dy, int sx, int sy,
                      int width, int height, unsigned rop)
{
    long left, top, right, bottom, q;
    long yy, xx, ystart, yend, ystep, xstart, xend, xstep;
    rg_offset so, doff;
    rg_u8 sv, dv;
    int backward_y, backward_x;
    if (!dst || !src || width <= 0 || height <= 0) return;
    /* Work in relative rectangle coordinates. Clip BOTH endpoints while
       preserving the source-to-destination pixel mapping. */
    left = 0; top = 0; right = width; bottom = height;
    q = -(long)sx; if (left < q) left = q;
    q = -(long)dx; if (left < q) left = q;
    q = (long)src->surface.width - sx; if (right > q) right = q;
    q = (long)dst->surface.width - dx; if (right > q) right = q;
    q = -(long)sy; if (top < q) top = q;
    q = -(long)dy; if (top < q) top = q;
    q = (long)src->surface.height - sy; if (bottom > q) bottom = q;
    q = (long)dst->surface.height - dy; if (bottom > q) bottom = q;
    if (left >= right || top >= bottom) return;

    /* Memmove-style traversal preserves source values for same-device
       overlapping copies, also when reading/writing banked video RAM. */
    backward_y = (src == dst && dy > sy);
    backward_x = (src == dst && dy == sy && dx > sx);
    ystart = backward_y ? bottom - 1 : top;
    yend = backward_y ? top - 1 : bottom;
    ystep = backward_y ? -1 : 1;
    xstart = backward_x ? right - 1 : left;
    xend = backward_x ? left - 1 : right;
    xstep = backward_x ? -1 : 1;
    for (yy = ystart; yy != yend; yy += ystep) {
        for (xx = xstart; xx != xend; xx += xstep) {
            so = (rg_offset)(sy + yy) * src->surface.stride +
                 (rg_offset)(sx + xx);
            doff = (rg_offset)(dy + yy) * dst->surface.stride +
                   (rg_offset)(dx + xx);
            sv = rg_read_byte(src, so);
            if (rop == RG_ROP_COPY) {
                rg_write_byte(dst, doff, sv);
            } else {
                dv = rg_read_byte(dst, doff);
                rg_write_byte(dst, doff, rg_rop_apply(sv, dv, rop));
            }
        }
    }
}
