#include "rgfx.h"
#include <limits.h>

unsigned rg_pixel_bits(RG_PixelFormat f)
{
    switch (f) {
    case RG_FMT_INDEX1: return 1;
    case RG_FMT_INDEX4: return 4;
    case RG_FMT_INDEX8: return 8;
    case RG_FMT_RGB555: case RG_FMT_RGB565: return 16;
    case RG_FMT_RGB888: return 24;
    case RG_FMT_XRGB8888: case RG_FMT_ARGB8888: return 32;
    default: return 0;
    }
}

static rg_pixel pixel_mask(unsigned bits)
{
    if (bits == 32) return 0xFFFFFFFFUL;
    return (1UL << bits) - 1UL;
}

rg_offset rg_min_stride(RG_PixelFormat f, unsigned width)
{
    unsigned bits = rg_pixel_bits(f);
    if (!bits || !width || (rg_offset)width > (ULONG_MAX - 7UL) / bits)
        return 0;
    return ((rg_offset)width * bits + 7UL) / 8UL;
}

static int setup(RG_Device *d, unsigned w, unsigned h, rg_offset stride,
                 rg_offset capacity, RG_PixelFormat f)
{
    rg_offset row = rg_min_stride(f, w);
    if (!d || !row || !h || stride < row || !capacity ||
        (rg_offset)(h - 1) > (ULONG_MAX - row) / stride ||
        ((rg_offset)(h - 1) * stride + row) > capacity)
        return 0;
    d->surface.width = w; d->surface.height = h;
    d->surface.stride = stride; d->surface.capacity = capacity;
    d->surface.format = f;
    d->card_data = 0;
    d->ops.pixel = rg_software_pixel; d->ops.line = rg_software_line;
    d->ops.fill = rg_software_fill; d->ops.blit = rg_software_blit;
    return 1;
}

int rg_init(RG_Device *d, rg_u8 *pixels, unsigned w, unsigned h,
            rg_offset stride, rg_offset capacity, RG_PixelFormat f)
{
    if (!pixels || !setup(d, w, h, stride, capacity, f)) return 0;
    d->surface.pixels = pixels;
    d->vram.read_byte = 0; d->vram.write_byte = 0;
    d->vram_context = 0;
    return 1;
}

int rg_init_vram(RG_Device *d, unsigned w, unsigned h, rg_offset stride,
                 rg_offset capacity, RG_PixelFormat f, RG_VramOps v, void *ctx)
{
    if (!v.read_byte || !v.write_byte ||
        !setup(d, w, h, stride, capacity, f)) return 0;
    d->surface.pixels = 0; d->vram = v; d->vram_context = ctx;
    return 1;
}

rg_u8 rg_read_byte(RG_Device *d, rg_offset off)
{
    if (!d || off >= d->surface.capacity) return 0;
    if (d->surface.pixels) return d->surface.pixels[off];
    if (d->vram.read_byte) return d->vram.read_byte(d->vram_context, off);
    return 0;
}
void rg_write_byte(RG_Device *d, rg_offset off, rg_u8 v)
{
    if (!d || off >= d->surface.capacity) return;
    if (d->surface.pixels) d->surface.pixels[off] = v;
    else if (d->vram.write_byte) d->vram.write_byte(d->vram_context, off, v);
}

static int valid_xy(RG_Device *d, int x, int y)
{
    return d && x >= 0 && y >= 0 &&
           (unsigned)x < d->surface.width && (unsigned)y < d->surface.height;
}
int rg_get_pixel(RG_Device *d, int x, int y, rg_pixel *v)
{
    unsigned bits, count, i, shift;
    rg_offset off;
    rg_pixel value = 0;
    if (!v || !valid_xy(d, x, y)) return 0;
    bits = rg_pixel_bits(d->surface.format);
    off = (rg_offset)(unsigned)y * d->surface.stride;
    if (bits < 8) {
        off += ((rg_offset)(unsigned)x * bits) / 8UL;
        shift = 8U - bits - (unsigned)(((rg_offset)(unsigned)x * bits) & 7UL);
        value = (rg_read_byte(d, off) >> shift) & pixel_mask(bits);
    } else {
        count = bits / 8;
        off += (rg_offset)(unsigned)x * count;
        for (i = 0; i < count; ++i)
            value |= (rg_pixel)rg_read_byte(d, off + i) << (8U * i);
    }
    *v = value;
    return 1;
}
int rg_put_pixel(RG_Device *d, int x, int y, rg_pixel value)
{
    unsigned bits, count, i, shift;
    rg_offset off;
    rg_u8 mask, old;
    if (!valid_xy(d, x, y)) return 0;
    bits = rg_pixel_bits(d->surface.format);
    value &= pixel_mask(bits);
    off = (rg_offset)(unsigned)y * d->surface.stride;
    if (bits < 8) {
        off += ((rg_offset)(unsigned)x * bits) / 8UL;
        shift = 8U - bits - (unsigned)(((rg_offset)(unsigned)x * bits) & 7UL);
        mask = (rg_u8)(pixel_mask(bits) << shift);
        old = rg_read_byte(d, off);
        rg_write_byte(d, off, (rg_u8)((old & (rg_u8)~mask) |
                       ((rg_u8)(value << shift) & mask)));
    } else {
        count = bits / 8;
        off += (rg_offset)(unsigned)x * count;
        for (i = 0; i < count; ++i)
            rg_write_byte(d, off + i, (rg_u8)(value >> (8U * i)));
    }
    return 1;
}

rg_pixel rg_rgb_pack(RG_PixelFormat f, rg_u8 r, rg_u8 g, rg_u8 b, rg_u8 a)
{
    switch (f) {
    case RG_FMT_RGB555: return ((rg_pixel)(r >> 3) << 10) |
                               ((rg_pixel)(g >> 3) << 5) | (b >> 3);
    case RG_FMT_RGB565: return ((rg_pixel)(r >> 3) << 11) |
                               ((rg_pixel)(g >> 2) << 5) | (b >> 3);
    case RG_FMT_RGB888: case RG_FMT_XRGB8888:
        return ((rg_pixel)r << 16) | ((rg_pixel)g << 8) | b;
    case RG_FMT_ARGB8888:
        return ((rg_pixel)a << 24) | ((rg_pixel)r << 16) |
               ((rg_pixel)g << 8) | b;
    default: return 0;
    }
}

void rg_software_pixel(RG_Device *d, int x, int y, rg_pixel p)
{
    (void)rg_put_pixel(d, x, y, p);
}
void rg_software_line(RG_Device *d, int x0, int y0, int x1, int y1, rg_pixel p)
{
    long x = x0, y = y0, tx = x1, ty = y1;
    long dx = tx > x ? tx - x : x - tx;
    long dy = ty > y ? ty - y : y - ty;
    long sx = x < tx ? 1 : -1, sy = y < ty ? 1 : -1;
    long err = dx - dy, e2;
    rg_offset i, steps = (rg_offset)(dx > dy ? dx : dy) + 1UL;
    if (!d || steps > 65536UL) return;
    for (i = 0; i < steps; ++i) {
        rg_software_pixel(d, (int)x, (int)y, p);
        if (x == tx && y == ty) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}
void rg_software_fill(RG_Device *d, int x, int y, int w, int h, rg_pixel p)
{
    long l = x, t = y, r = l + (long)w, b = t + (long)h, xx, yy;
    if (!d || w <= 0 || h <= 0) return;
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (r > (long)d->surface.width) r = d->surface.width;
    if (b > (long)d->surface.height) b = d->surface.height;
    for (yy = t; yy < b; ++yy)
        for (xx = l; xx < r; ++xx) rg_software_pixel(d, (int)xx, (int)yy, p);
}
static rg_pixel rop_apply(rg_pixel s, rg_pixel d, unsigned rop, rg_pixel mask)
{
    rg_pixel v = 0;
    if (rop & 1U) v |= ~s & ~d;
    if (rop & 2U) v |= ~s & d;
    if (rop & 4U) v |= s & ~d;
    if (rop & 8U) v |= s & d;
    return v & mask;
}
void rg_software_blit(RG_Device *dst, RG_Device *src, int dx, int dy,
                      int sx, int sy, int w, int h, unsigned rop)
{
    long l = 0, t = 0, r = w, b = h, q, yy, xx;
    long y0, y1, ys, x0, x1, xs;
    rg_pixel sv, dv, mask;
    int reverse_y, reverse_x;
    if (!dst || !src || w <= 0 || h <= 0 || rop > 15U ||
        dst->surface.format != src->surface.format) return;
    mask = pixel_mask(rg_pixel_bits(dst->surface.format));
    q = -(long)dx; if (l < q) l = q;
    q = -(long)sx; if (l < q) l = q;
    q = (long)dst->surface.width - dx; if (r > q) r = q;
    q = (long)src->surface.width - sx; if (r > q) r = q;
    q = -(long)dy; if (t < q) t = q;
    q = -(long)sy; if (t < q) t = q;
    q = (long)dst->surface.height - dy; if (b > q) b = q;
    q = (long)src->surface.height - sy; if (b > q) b = q;
    if (l >= r || t >= b) return;
    reverse_y = (dst == src && dy > sy);
    reverse_x = (dst == src && dy == sy && dx > sx);
    y0 = reverse_y ? b - 1 : t; y1 = reverse_y ? t - 1 : b;
    ys = reverse_y ? -1 : 1;
    x0 = reverse_x ? r - 1 : l; x1 = reverse_x ? l - 1 : r;
    xs = reverse_x ? -1 : 1;
    for (yy = y0; yy != y1; yy += ys)
        for (xx = x0; xx != x1; xx += xs) {
            if (!rg_get_pixel(src, sx + (int)xx, sy + (int)yy, &sv)) continue;
            if (rop == RG_ROP_COPY) dv = sv;
            else {
                if (!rg_get_pixel(dst, dx + (int)xx, dy + (int)yy, &dv)) continue;
                dv = rop_apply(sv, dv, rop, mask);
            }
            rg_put_pixel(dst, dx + (int)xx, dy + (int)yy, dv);
        }
}
