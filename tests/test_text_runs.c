/* Independent pixel reference, operation counts and exact font reads. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "rg_gdi_text.h"
#define W 137u
#define H 132u
#define MAXSTRIDE 143u
#define FONTBASE 13u
static rg_u8 actual[MAXSTRIDE*H], expected[MAXSTRIDE*H];
static rg_u8 font[FONTBASE+2048u], reads[FONTBASE+2048u];
static unsigned long nread,npixel,nfill,npaint;
static unsigned cases;
static rg_u8 read_font(void *ctx, rg_offset off)
{
    (void)ctx;
    assert(off>=FONTBASE && off<sizeof(font));
    assert(reads[off]==0); /* no byte is fetched twice */
    reads[off]=1; ++nread;
    return font[off];
}
static void count_pixel(RG_Device *d,int x,int y,rg_pixel p)
{
    assert(x>=0 && y>=0 && (unsigned)x<W && (unsigned)y<H);
    ++npixel; rg_software_pixel(d,x,y,p);
}
static void count_fill(RG_Device *d,int x,int y,int w,int h,rg_pixel p)
{
    assert(x>=0 && y>=0 && w>0 && h>0 &&
           (unsigned)(x+w)<=W && (unsigned)(y+h)<=H);
    ++nfill; npaint+=(unsigned long)w*(unsigned long)h;
    rg_software_fill(d,x,y,w,h,p);
}
static void reset(void)
{
    nread=npixel=nfill=npaint=0;
    memset(reads,0,sizeof(reads));
    memset(actual,0xA5,sizeof(actual));
    memset(expected,0xA5,sizeof(expected));
}
static void one(RG_PixelFormat format,unsigned width,unsigned height,
                int x,int y,const RG_Rect *clip,unsigned pattern,unsigned mode)
{
    RG_Device d,ref;
    RG_Glyph g;
    unsigned row,col,color,stride,bit,was_set;
    unsigned long want_reads=0,want_pixels=0,want_runs=0;
    rg_offset off;
    long gx,gy;
    rg_u8 want[FONTBASE+2048u];
    reset(); memset(want,0,sizeof(want));
    stride=format==RG_FMT_INDEX1 ? 21u : MAXSTRIDE;
    color=format==RG_FMT_INDEX1 ? pattern%2u : 93u;
    assert(rg_init(&d,actual,W,H,stride,(rg_offset)stride*H,format));
    assert(rg_init(&ref,expected,W,H,stride,(rg_offset)stride*H,format));
    if (mode==1u) d.ops.pixel=count_pixel; /* default fill must not bypass */
    if (mode==2u) { d.ops.pixel=count_pixel; d.ops.fill=count_fill; }
    if (mode==3u) { d.ops.pixel=count_pixel; d.ops.fill=0; }
    if (mode==4u) { d.ops.pixel=0; d.ops.fill=count_fill; }
    g.offset=FONTBASE; g.width=width; g.height=height;
    for (off=FONTBASE;off<sizeof(font);++off)
        font[off]=pattern==0u ? 0xFF : pattern==1u ? 0 :
            pattern==2u ? 0xAA : (rg_u8)((off*37UL+off/7UL)^0xD3u);
    /* Reference deliberately uses original column-byte addressing per pixel;
     * count the unique visible source bytes separately from set pixels. */
    for (row=0;row<height;++row) {
        was_set=0;
        for (col=0;col<width;++col) {
            gx=(long)x+col; gy=(long)y+row;
            if (gx<0 || gy<0 || gx>=W || gy>=H ||
                (clip && (gx<clip->left || gx>=clip->right ||
                          gy<clip->top || gy>=clip->bottom))) {
                was_set=0; continue;
            }
            off=FONTBASE+(rg_offset)(col/8u)*height+row;
            if (!want[off]) { want[off]=1; ++want_reads; }
            bit=font[off] & (0x80u>>(col%8u));
            if (bit) {
                ++want_pixels; if (!was_set) ++want_runs;
                assert(rg_put_pixel(&ref,(int)gx,(int)gy,color));
            }
            was_set=bit;
        }
    }
    assert(rg_gdi_text_glyph(&d,x,y,clip,&g,read_font,0,color));
    assert(memcmp(actual,expected,sizeof(actual))==0);
    assert(memcmp(reads,want,sizeof(want))==0 && nread==want_reads);
    if (mode==1u || mode==3u) assert(npixel==want_pixels && nfill==0);
    if (mode==2u || mode==4u)
        assert(npixel==0 && nfill==want_runs && npaint==want_pixels);
    ++cases;
}
static void rectangles(void)
{
    RG_Device d;
    RG_Rect r,c;
    reset();
    assert(rg_init(&d,actual,W,H,MAXSTRIDE,sizeof(actual),RG_FMT_INDEX8));
    d.ops.pixel=count_pixel; d.ops.fill=count_fill;
    r.left=-5; r.top=3; r.right=20; r.bottom=20;
    c.left=2; c.top=6; c.right=11; c.bottom=16;
    assert(rg_gdi_text_fill(&d,r,&c,17));
    assert(nfill==1 && npixel==0 && npaint==90);
    c.left=c.right;
    assert(rg_gdi_text_fill(&d,r,&c,17));
    assert(nfill==1);
    c.left=21; c.right=24;
    assert(rg_gdi_text_fill(&d,r,&c,17));
    assert(nfill==1);
    c.left=2; c.right=11; d.ops.fill=0;
    assert(rg_gdi_text_fill(&d,r,&c,17));
    assert(npixel==90);
    d.ops.pixel=0;
    assert(!rg_gdi_text_fill(&d,r,&c,17));
}
int main(void)
{
    static const unsigned widths[]={1,7,8,9,17,128};
    RG_Rect clip,empty;
    RG_Device d;
    RG_Glyph g;
    unsigned f,w,p,m;
    clip.left=5; clip.top=4; clip.right=109; clip.bottom=90;
    empty.left=empty.right=5; empty.top=0; empty.bottom=H;
    for (f=0;f<2u;++f) for (w=0;w<sizeof(widths)/sizeof(widths[0]);++w)
        for (p=0;p<4u;++p) for (m=0;m<5u;++m) {
            RG_PixelFormat format=f ? RG_FMT_INDEX1 : RG_FMT_INDEX8;
            one(format,widths[w],128,3,2,0,p,m);
            one(format,widths[w],13,-3,-2,&clip,p,m);
            one(format,widths[w],13,4,2,&clip,p,m);
            one(format,widths[w],13,133,128,0,p,m);
            one(format,widths[w],13,0,0,&empty,p,m);
        }
    one(RG_FMT_INDEX8,128,128,-32768,32767,0,0,2);
    one(RG_FMT_INDEX1,128,128,32767,-32768,0,0,2);
    one(RG_FMT_INDEX8,128,128,0,0,0,0,2);
    assert(nread==2048UL && nfill==128UL && npaint==16384UL);
    printf("Dense 128x128: glyph reads 16384 -> %lu; drawing calls 16384 -> %lu\n",nread,nfill);
    rectangles();
    g.width=9; g.height=2; g.offset=ULONG_MAX-1UL;
    assert(rg_init(&d,actual,W,H,MAXSTRIDE,sizeof(actual),RG_FMT_INDEX8));
    assert(!rg_gdi_text_glyph(&d,0,0,0,&g,read_font,0,1));
    printf("PASS: %u independent text cases, exact reads, runs, padding, clipping and overrides\n",cases);
    return 0;
}
