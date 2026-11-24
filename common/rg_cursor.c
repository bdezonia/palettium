#include <string.h>
#include "rg_cursor.h"

static int cursor_device_ok(RG_Device *d)
{
    return d && d->ops.pixel && d->surface.width && d->surface.height &&
      (d->surface.format==RG_FMT_INDEX1 ||
       d->surface.format==RG_FMT_INDEX8) &&
      d->surface.width<=32767u && d->surface.height<=32767u;
}
static unsigned bit_at(const rg_u8 *bits,unsigned stride,unsigned x,unsigned y)
{
    return ((unsigned)bits[(rg_offset)y*stride+x/8u] >> (7u-(x&7u)))&1u;
}
void rg_cursor_init(RG_Cursor *c)
{
    if (c) memset(c,0,sizeof(*c));
}
void rg_cursor_hide(RG_Cursor *c)
{
    unsigned xx,yy;
    long xpos,ypos;
    RG_Device *d;
    if (!c || !c->visible || !c->device) return;
    d=c->device;
    for (yy=0;yy<c->height;++yy) for (xx=0;xx<c->width;++xx) {
        xpos=(long)c->x-(long)c->hot_x+(long)xx;
        ypos=(long)c->y-(long)c->hot_y+(long)yy;
        if (xpos>=0 && xpos<(long)d->surface.width &&
            ypos>=0 && ypos<(long)d->surface.height)
            d->ops.pixel(d,(int)xpos,(int)ypos,
                       c->saved[yy*RG_CURSOR_MAX_WIDTH+xx]);
    }
    c->visible=0;
    c->device=0;
}
int rg_cursor_shape(RG_Cursor *c,const rg_u8 *and_mask,
                    const rg_u8 *xor_mask,unsigned width,unsigned height,
                    int hot_x,int hot_y)
{
    unsigned y,bytes=(width+7u)/8u;
    if (!c || !and_mask || !xor_mask || !width || !height ||
        width>RG_CURSOR_MAX_WIDTH || height>RG_CURSOR_MAX_HEIGHT ||
        hot_x<0 || hot_y<0 || hot_x>=(int)width || hot_y>=(int)height)
        return 0;
    if (c->visible) rg_cursor_hide(c);
    memset(c->and_bits,0xFF,sizeof(c->and_bits));
    memset(c->xor_bits,0,sizeof(c->xor_bits));
    for (y=0;y<height;++y) {
        memcpy(c->and_bits+y*4u,and_mask+y*bytes,bytes);
        memcpy(c->xor_bits+y*4u,xor_mask+y*bytes,bytes);
    }
    c->width=width;c->height=height;c->hot_x=hot_x;c->hot_y=hot_y;
    c->has_shape=1;
    return 1;
}
int rg_cursor_show(RG_Cursor *c,RG_Device *d,int x,int y)
{
    unsigned xx,yy,bits_width;
    long xpos,ypos;
    rg_pixel old,mask,value;
    if (!c || !c->has_shape || !cursor_device_ok(d)) return 0;
    if (c->visible) rg_cursor_hide(c);
    c->device=d;c->x=x;c->y=y;
    bits_width=rg_pixel_bits(d->surface.format);
    mask=(1UL<<bits_width)-1UL;
    /* Read saved pixels before drawing ANY cursor pixels, so subsequent
     * overlapping moves/hides cannot read altered background values. */
    for (yy=0;yy<c->height;++yy) for (xx=0;xx<c->width;++xx) {
        xpos=(long)x-(long)c->hot_x+(long)xx;
        ypos=(long)y-(long)c->hot_y+(long)yy;
        old=0;
        if (xpos>=0 && xpos<(long)d->surface.width &&
            ypos>=0 && ypos<(long)d->surface.height &&
            !rg_get_pixel(d,(int)xpos,(int)ypos,&old)) {
            c->device=0;
            return 0;
        }
        c->saved[yy*RG_CURSOR_MAX_WIDTH+xx]=old;
    }
    c->visible=1;
    for (yy=0;yy<c->height;++yy) for (xx=0;xx<c->width;++xx) {
        xpos=(long)x-(long)c->hot_x+(long)xx;
        ypos=(long)y-(long)c->hot_y+(long)yy;
        if (xpos<0 || xpos>=(long)d->surface.width ||
            ypos<0 || ypos>=(long)d->surface.height) continue;
        value=c->saved[yy*RG_CURSOR_MAX_WIDTH+xx];
        value=(value & (bit_at(c->and_bits,4u,xx,yy)?mask:0UL)) ^
                   (bit_at(c->xor_bits,4u,xx,yy)?mask:0UL);
        d->ops.pixel(d,(int)xpos,(int)ypos,value);
    }
    return 1;
}
int rg_cursor_move(RG_Cursor *c,int x,int y)
{
    RG_Device *d;
    if (!c) return 0;
    if (!c->visible) { c->x=x;c->y=y; return c->has_shape ? 1 : 0; }
    d=c->device;
    return rg_cursor_show(c,d,x,y);
}
