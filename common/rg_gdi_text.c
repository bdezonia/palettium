#include <limits.h>
#include "rg_gdi_text.h"

/* A specialized fill takes precedence. A default software fill must not
 * bypass a pixel-only hardware override (or a pixel-only adapter). */
static RG_FillFn text_fill_op(RG_Device *dev)
{
    if (dev->ops.fill == rg_software_fill && dev->ops.pixel &&
        dev->ops.pixel != rg_software_pixel) return 0;
    return dev->ops.fill;
}

static int text_clip(RG_Device *dev, const RG_Rect *user, RG_Rect *c)
{
    if (!dev || !c || (!dev->ops.pixel && !dev->ops.fill) ||
        !dev->surface.width || !dev->surface.height ||
        dev->surface.width > 32767u || dev->surface.height > 32767u ||
        (dev->surface.format != RG_FMT_INDEX8 &&
         dev->surface.format != RG_FMT_INDEX1)) return 0;
    c->left=0; c->top=0;
    c->right=(int)dev->surface.width;
    c->bottom=(int)dev->surface.height;
    if (user) {
        if (user->left>user->right || user->top>user->bottom) return 0;
        if (c->left<user->left) c->left=user->left;
        if (c->top<user->top) c->top=user->top;
        if (c->right>user->right) c->right=user->right;
        if (c->bottom>user->bottom) c->bottom=user->bottom;
    }
    return 1;
}
static int text_color_valid(RG_Device *dev, unsigned color)
{
    return dev->surface.format==RG_FMT_INDEX1 ? color<=1u : color<=255u;
}
static void text_rect(RG_Device *dev, RG_FillFn fill, int l, int t,
                      int r, int b, rg_pixel color)
{
    int xx,yy;
    if (l>=r || t>=b) return;
    if (fill) {
        fill(dev,l,t,r-l,b-t,color);
        return;
    }
    for (yy=t;yy<b;++yy)
        for (xx=l;xx<r;++xx) dev->ops.pixel(dev,xx,yy,color);
}
int rg_gdi_text_fill(RG_Device *dev, RG_Rect rect,
                     const RG_Rect *user, unsigned color)
{
    RG_Rect c;
    if (!text_clip(dev,user,&c) || !text_color_valid(dev,color) ||
        rect.left>rect.right || rect.top>rect.bottom) return 0;
    if (c.left<rect.left) c.left=rect.left;
    if (c.top<rect.top) c.top=rect.top;
    if (c.right>rect.right) c.right=rect.right;
    if (c.bottom>rect.bottom) c.bottom=rect.bottom;
    text_rect(dev,text_fill_op(dev),c.left,c.top,c.right,c.bottom,
              (rg_pixel)color);
    return 1;
}
int rg_gdi_text_glyph(RG_Device *dev, int x, int y,
                      const RG_Rect *user, const RG_Glyph *glyph,
                      RG_GlyphReadFn read_byte, void *context,
                      unsigned color)
{
    RG_Rect c;
    RG_FillFn fill;
    long l,t,r,b;
    unsigned col,row,first,last,byte,limit,mask,start;
    int run;
    rg_u8 bits;
    rg_offset rel;
    if (!text_clip(dev,user,&c) || !glyph || !read_byte ||
        !text_color_valid(dev,color) || !glyph->width || !glyph->height ||
        glyph->width>128u || glyph->height>128u) return 0;
    /* The caller has already checked the callback's address range.
     * Guard the local offset math too (32-bit long on Win16). */
    if (glyph->offset > ULONG_MAX -
        (rg_offset)(((glyph->width+7u)/8u)*glyph->height)) return 0;
    if (c.left>=c.right || c.top>=c.bottom || x>=c.right ||
        y>=c.bottom || (long)x<=(long)c.left-(long)glyph->width ||
        (long)y<=(long)c.top-(long)glyph->height) return 1;
    l=x; t=y; r=l+(long)glyph->width; b=t+(long)glyph->height;
    if (l<c.left) l=c.left;
    if (t<c.top) t=c.top;
    if (r>c.right) r=c.right;
    if (b>c.bottom) b=c.bottom;
    if (l>=r || t>=b) return 1;
    /* Differences are bounded by glyph size; absolute coordinates remain
     * in the clipped surface, even at Win16 signed-coordinate limits. */
    first=(unsigned)(l-(long)x); last=(unsigned)(r-(long)x);
    fill=text_fill_op(dev);
    for (row=(unsigned)(t-(long)y);row<(unsigned)(b-(long)y);++row) {
        run=0; start=0;
        col=first;
        while (col<last) {
            byte=col/8u;
            rel=glyph->offset+(rg_offset)byte*glyph->height+row;
            bits=read_byte(context,rel);
            limit=(byte+1u)*8u;
            if (limit>last) limit=last;
            mask=0x80u>>(col&7u);
            for (;col<limit;++col,mask>>=1) {
                if (bits & mask) {
                    if (!run) { start=col; run=1; }
                } else if (run) {
                    text_rect(dev,fill,(int)((long)x+start),(int)((long)y+row),
                              (int)((long)x+col),(int)((long)y+row+1L),
                              (rg_pixel)color);
                    run=0;
                }
            }
        }
        if (run)
            text_rect(dev,fill,(int)((long)x+start),(int)((long)y+row),
                      (int)((long)x+last),(int)((long)y+row+1L),
                      (rg_pixel)color);
    }
    return 1;
}
