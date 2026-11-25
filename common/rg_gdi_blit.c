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
        dst->surface.stride < rg_min_stride(dst->surface.format, dst->surface.width) ||
        src->surface.stride < rg_min_stride(src->surface.format, src->surface.width) ||
        dst->surface.format != src->surface.format) return 0;
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

unsigned rg_gdi_mono_color(unsigned physical, unsigned background_color)
{
    return physical == background_color ? 1u : 0u;
}

/* Preserve same-format hardware acceleration; cross-format conversions
 * use the shared surface pixel operations and hardware pixel overrides.
 * Coordinates are clipped jointly so invalid extent never reads/writes
 * outside either bitmap. No allocation or separate rendering engine. */
int rg_gdi_blit_color(RG_Device *dst, RG_Device *src,
                      int dx, int dy, int sx, int sy,
                      int width, int height, unsigned long win_rop,
                      unsigned text_color, unsigned background_color)
{
    int truth;
    long l=0, t=0, r=width, b=height, q, x, y;
    rg_pixel from, old;
    unsigned converted;
    RG_PixelFormat df, sf;
    if (!dst || !src || width <= 0 || height <= 0 ||
        !dst->ops.pixel || !dst->ops.blit ||
        !dst->surface.width || !dst->surface.height ||
        !src->surface.width || !src->surface.height ||
        (!dst->surface.pixels && (!dst->vram.read_byte || !dst->vram.write_byte)) ||
        (!src->surface.pixels && (!src->vram.read_byte || !src->vram.write_byte)) ||
        text_color > 255u || background_color > 255u) return 0;
    truth = rg_gdi_rop3_to_rop2(rg_gdi_rop3_code(win_rop));
    if (truth < 0) return 0;
    df=dst->surface.format; sf=src->surface.format;
    if ((df != RG_FMT_INDEX1 && df != RG_FMT_INDEX8) ||
        (sf != RG_FMT_INDEX1 && sf != RG_FMT_INDEX8)) return 0;
    if (df == sf) return rg_gdi_blit_rop3(dst, src, dx, dy, sx, sy,
                                           width, height, win_rop);
    q=-(long)dx; if (l<q) l=q;
    q=-(long)sx; if (l<q) l=q;
    q=(long)dst->surface.width-(long)dx; if (r>q) r=q;
    q=(long)src->surface.width-(long)sx; if (r>q) r=q;
    q=-(long)dy; if (t<q) t=q;
    q=-(long)sy; if (t<q) t=q;
    q=(long)dst->surface.height-(long)dy; if (b>q) b=q;
    q=(long)src->surface.height-(long)sy; if (b>q) b=q;
    if (l>=r || t>=b) return 1;
    for (y=t; y<b; ++y) for (x=l; x<r; ++x) {
        if (!rg_get_pixel(src, sx+(int)x, sy+(int)y, &from)) return 0;
        if (sf == RG_FMT_INDEX1)
            converted=from ? background_color : text_color;
        else
            converted=rg_gdi_mono_color((unsigned)from,background_color);
        if (truth == (int)RG_ROP_COPY) old=converted;
        else {
            if (!rg_get_pixel(dst, dx+(int)x, dy+(int)y, &old)) return 0;
            old=(rg_pixel)rg_gdi_rop2_byte((unsigned)truth+1u,
                                           (rg_u8)converted,(rg_u8)old);
        }
        dst->ops.pixel(dst, dx+(int)x, dy+(int)y,old);
    }
    return 1;
}

/* Source-independent ROP3, using a solid pattern when needed.  Preserve
 * fill overrides for destination-independent operations.  All geometry is
 * clipped before access; the caller owns cursor exclusion. */
int rg_gdi_blit_solid(RG_Device *dst, int dx, int dy, int width, int height,
                       unsigned long win_rop, unsigned pattern)
{
    unsigned code=rg_gdi_rop3_code(win_rop), mask, lo, hi;
    long l=dx, t=dy, r=l+(long)width, b=t+(long)height, x,y;
    rg_pixel old;
    if (!dst || width<=0 || height<=0 || !dst->ops.fill || !dst->ops.pixel ||
        !dst->surface.width || !dst->surface.height ||
        (!dst->surface.pixels && (!dst->vram.read_byte || !dst->vram.write_byte)) ||
        (code & 0x33u)!=((code>>2)&0x33u)) return 0;
    if (dst->surface.format==RG_FMT_INDEX1) mask=1u;
    else if (dst->surface.format==RG_FMT_INDEX8) mask=255u;
    else return 0;
    if (pattern>mask) return 0;
    lo=rg_gdi_rop3_byte(code,(rg_u8)pattern,0,0)&mask;
    hi=rg_gdi_rop3_byte(code,(rg_u8)pattern,0,(rg_u8)mask)&mask;
    if (lo==hi) {
        dst->ops.fill(dst,dx,dy,width,height,lo);
        return 1;
    }
    if (lo==0u && hi==mask) return 1; /* destination unchanged */
    if (l<0) l=0;
    if (t<0) t=0;
    if (r>(long)dst->surface.width) r=dst->surface.width;
    if (b>(long)dst->surface.height) b=dst->surface.height;
    if (l>=r || t>=b) return 1;
    for (y=t;y<b;++y) for (x=l;x<r;++x) {
        if (!rg_get_pixel(dst,(int)x,(int)y,&old)) return 0;
        dst->ops.pixel(dst,(int)x,(int)y,(old & hi) | (~old & lo));
    }
    return 1;
}
