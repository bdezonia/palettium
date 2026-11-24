#include <limits.h>
#include "rg_gdi_text.h"

static int text_clip(RG_Device *dev, const RG_Rect *user, RG_Rect *c)
{
    if (!dev || !c || !dev->ops.pixel || !dev->surface.width ||
        !dev->surface.height || dev->surface.width > 32767u ||
        dev->surface.height > 32767u ||
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
int rg_gdi_text_fill(RG_Device *dev, RG_Rect rect,
                     const RG_Rect *user, unsigned color)
{
    RG_Rect c;
    int xx,yy;
    if (!text_clip(dev,user,&c) || !text_color_valid(dev,color) ||
        rect.left>rect.right || rect.top>rect.bottom) return 0;
    if (c.left<rect.left) c.left=rect.left;
    if (c.top<rect.top) c.top=rect.top;
    if (c.right>rect.right) c.right=rect.right;
    if (c.bottom>rect.bottom) c.bottom=rect.bottom;
    for (yy=c.top;yy<c.bottom;++yy)
        for (xx=c.left;xx<c.right;++xx)
            dev->ops.pixel(dev,xx,yy,(rg_pixel)color);
    return 1;
}
int rg_gdi_text_glyph(RG_Device *dev, int x, int y,
                      const RG_Rect *user, const RG_Glyph *glyph,
                      RG_GlyphReadFn read_byte, void *context,
                      unsigned color)
{
    RG_Rect c;
    long gx,gy;
    unsigned col,row,mask;
    rg_u8 bits;
    rg_offset rel;
    if (!text_clip(dev,user,&c) || !glyph || !read_byte ||
        !text_color_valid(dev,color) || !glyph->width || !glyph->height ||
        glyph->width>128u || glyph->height>128u) return 0;
    /* The caller has already checked the callback's address range.
     * Guard the local offset math too (32-bit long on Win16). */
    if (glyph->offset > ULONG_MAX -
        (rg_offset)(((glyph->width+7u)/8u)*glyph->height)) return 0;
    for (col=0;col<glyph->width;++col) {
        gx=(long)x+(long)col;
        if (gx<(long)c.left || gx>=(long)c.right) continue;
        for (row=0;row<glyph->height;++row) {
            gy=(long)y+(long)row;
            if (gy<(long)c.top || gy>=(long)c.bottom) continue;
            rel=glyph->offset+(rg_offset)(col/8u)*glyph->height+row;
            bits=read_byte(context,rel);
            mask=0x80u>>(col&7u);
            if (bits & mask) dev->ops.pixel(dev,(int)gx,(int)gy,
                                             (rg_pixel)color);
        }
    }
    return 1;
}
