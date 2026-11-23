/* Regression for Win3.x single-segment PBITMAP: 1/8bpp, ROP, GDI colors. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_bitmap_device.h"
#include "rg_gdi_blit.h"
#include "rg_gdi_rop.h"
#include "rg_gdi_output.h"

#define W 32
#define H 12
typedef struct Bytes {
    rg_u8 v[W*H + 2];
    rg_offset limit;
    unsigned bad, read_count, write_count;
} Bytes;
static rg_u8 read_mem(void *p, rg_offset off)
{
    Bytes *b=(Bytes *)p;
    if (off>=b->limit) { ++b->bad; return 0; }
    ++b->read_count;
    return b->v[off];
}
static void write_mem(void *p, rg_offset off, rg_u8 value)
{
    Bytes *b=(Bytes *)p;
    if (off>=b->limit) { ++b->bad; return; }
    ++b->write_count;
    b->v[off]=value;
}
static PAL_BITMAP16 bitmap(unsigned bpp, unsigned stride,
                            unsigned long at)
{
    PAL_BITMAP16 b;
    memset(&b,0,sizeof(b));
    b.bmWidth=W; b.bmHeight=H; b.bmWidthBytes=(PAL_S16)stride;
    b.bmPlanes=1; b.bmBitsPixel=(unsigned char)bpp;
    b.bmBits=at; b.bmWidthPlanes=stride*H;
    return b;
}
#define GET(d,x,y) get((d),(x),(y))
static unsigned get(RG_Device *d, int x, int y)
{
    rg_pixel v=99;
    assert(rg_get_pixel(d,x,y,&v));
    return (unsigned)v;
}
static void test_validation(void)
{
    PAL_BITMAP16 b=bitmap(1,4,0xA0001000UL);
    assert(pal_ddi_bitmap_layout_ok());
    assert(pal_ddi_memory_bitmap_ok(&b));
    b.bmWidthBytes=3; assert(!pal_ddi_memory_bitmap_ok(&b));
    b=bitmap(8,32,0xA000FF00UL);
    assert(!pal_ddi_memory_bitmap_ok(&b)); /* crosses segment */
    b=bitmap(8,32,0xA0000000UL);
    assert(pal_ddi_memory_bitmap_ok(&b));
    b.bmWidthPlanes--; assert(!pal_ddi_memory_bitmap_ok(&b));
    b=bitmap(1,4,0xA0001000UL);
    b.bmSegmentIndex=8; assert(!pal_ddi_memory_bitmap_ok(&b));
    b=bitmap(1,4,0xA0001000UL);
    b.bmBitsPixel=4; assert(!pal_ddi_memory_bitmap_ok(&b));
    b=bitmap(1,4,0xA0001000UL);
    b.bmHeight=32767; b.bmWidthPlanes=131068UL;
    assert(!pal_ddi_memory_bitmap_ok(&b));
}
static void test_output_and_blit(void)
{
    Bytes m, c;
    PAL_BITMAP16 bm=bitmap(1,4,0xA0001000UL);
    PAL_BITMAP16 bc=bitmap(8,32,0xB0000000UL);
    RG_VramOps io;
    RG_Device mono, color;
    RG_Brush brush;
    RG_Pen pen;
    RG_Rect r, clip;
    unsigned i, writes;
    io.read_byte=read_mem; io.write_byte=write_mem;
    memset(&m,0,sizeof(m)); memset(&c,0,sizeof(c));
    m.limit=48; c.limit=W*H;
    m.v[48]=0xA5; c.v[W*H]=0x5A;
    assert(pal_ddi_bitmap_device(&mono,&bm,io,&m));
    assert(pal_ddi_bitmap_device(&color,&bc,io,&c));
    assert(mono.surface.format==RG_FMT_INDEX1);
    assert(color.surface.format==RG_FMT_INDEX8);
    assert(rg_gdi_mono_color(7,7)==1 && rg_gdi_mono_color(4,7)==0);
    /* 1bpp packed bits are MSB-first; physical bits are already converted
     * by the Win16 adapter before calling the common renderer. */
    for (i=0;i<48u;++i) m.v[i]=0xFF;
    memset(&brush,0,sizeof(brush));
    brush.signature=RG_BRUSH_SIGNATURE; brush.style=RG_BRUSH_SOLID;
    brush.foreground=0; brush.background=1;
    memset(&pen,0,sizeof(pen));
    pen.signature=RG_PEN_SIGNATURE; pen.style=RG_PEN_NULL; pen.width=1;
    r.left=3;r.top=2;r.right=20;r.bottom=7;
    clip.left=5;clip.top=3;clip.right=18;clip.bottom=8;
    assert(rg_gdi_output_rectangle(&mono,r,&clip,&pen,&brush,13,2));
    assert(GET(&mono,4,3)==1 && GET(&mono,5,3)==0 && GET(&mono,17,6)==0);
    assert(GET(&mono,18,6)==1 && GET(&mono,6,7)==1);
    pen.style=RG_PEN_SOLID; pen.color=1;
    assert(rg_gdi_output_scanline(&mono,8,6,14,0,&pen,0,13,2));
    assert(GET(&mono,6,8)==1);
    pen.color=0;
    assert(rg_gdi_output_segment(&mono,8,9,12,9,0,&pen,13));
    assert(GET(&mono,8,9)==0 && GET(&mono,11,9)==0 && GET(&mono,12,9)==1);
    /* ROP2 must honor MONO bit width, including inverse/XOR cases. */
    for (i=1; i<=16u; ++i) {
        unsigned result;
        unsigned expect;
        assert(rg_put_pixel(&mono,1,10,1));
        pen.color=0;
        assert(rg_gdi_output_scanline(&mono,10,1,2,0,&pen,0,i,2));
        result=GET(&mono,1,10);
        expect=rg_gdi_rop2_byte(i,0,1)&1u;
        assert(result==expect);
    }
    /* DDK mono 1 = background (7), 0 = text color (4). */
    assert(rg_gdi_blit_color(&color,&mono,0,0,0,0,W,H,
                             0x00CC0020UL,4,7));
    assert(GET(&color,4,3)==7 && GET(&color,5,3)==4);
    assert(GET(&color,11,9)==4 && GET(&color,12,9)==7);
    /* Color->mono maps all colors except matching background to zero. */
    memset(m.v,0,48);
    assert(rg_gdi_blit_color(&mono,&color,0,0,0,0,W,H,
                             0x00CC0020UL,4,7));
    assert(GET(&mono,4,3)==1 && GET(&mono,5,3)==0);
    assert(GET(&mono,11,9)==0 && GET(&mono,12,9)==1);
    /* ROP3 which reads pattern must refuse WITHOUT modifying data. */
    writes=m.write_count;
    assert(!rg_gdi_blit_color(&mono,&color,0,0,0,0,16,3,
                              0x00F00021UL,4,7));
    assert(m.write_count==writes);
    assert(!rg_gdi_blit_color(&mono,&color,0,0,0,0,16,3,
                              0x00CC0020UL,4,256));
    assert(m.write_count==writes);
    /* Copies are clipped consistently against both surfaces. */
    assert(rg_gdi_blit_color(&mono,&color,-3,-3,0,0,W,H,
                             0x00CC0020UL,4,7));
    assert(!m.bad && !c.bad && m.v[48]==0xA5 && c.v[W*H]==0x5A);
    /* Same-memory overlap: use a single device, preserve source rows. */
    for (i=0;i<W;++i) assert(rg_put_pixel(&mono,(int)i,0,(i&1u)));
    for (i=0;i<W;++i) assert(rg_put_pixel(&mono,(int)i,1,0));
    assert(rg_gdi_blit_color(&mono,&mono,0,1,0,0,W,1,
                             0x00CC0020UL,4,7));
    for (i=0;i<W;++i) assert(GET(&mono,(int)i,1)==(i&1u));
    assert(!m.bad && !c.bad);
}
int main(void)
{
    test_validation(); test_output_and_blit();
    puts("PASS: Win31 1/8bpp PBITMAP segment bounds, packed pixels, Output, mono/color BitBlt, overlap");
    return 0;
}
