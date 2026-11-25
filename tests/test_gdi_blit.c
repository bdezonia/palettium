#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rg_gdi_blit.h"
#include "rg_gdi_rop.h"

#define W 39
#define H 17
static unsigned char a[W*H], b[W*H], backup[W*H];
static RG_Device da, db;
static int intercepts;
static void counted(RG_Device *dst, RG_Device *src, int dx,int dy,int sx,int sy,
                    int w,int h,unsigned rop)
{
    ++intercepts;
    rg_software_blit(dst,src,dx,dy,sx,sy,w,h,rop);
}
static void seed(void)
{
    unsigned i;
    for(i=0;i<sizeof(a);++i) { a[i]=(unsigned char)((i*53U+7U)&255U); b[i]=a[i]; }
    assert(rg_init(&da, a, W, H, W, (rg_offset)W * H, RG_FMT_INDEX8));
    assert(rg_init(&db, b, W, H, W, (rg_offset)W * H, RG_FMT_INDEX8));
}
static void reference(int dx,int dy,int sx,int sy,int ww,int hh,unsigned rop3)
{
    int x,y;
    unsigned char d,s;
    memcpy(backup,b,sizeof(b));
    for(y=0;y<hh;++y) for(x=0;x<ww;++x) {
        int tx=dx+x,ty=dy+y,ux=sx+x,uy=sy+y;
        if(tx<0||ty<0||ux<0||uy<0||tx>=W||ty>=H||ux>=W||uy>=H)continue;
        d=backup[ty*W+tx]; s=backup[uy*W+ux];
        b[ty*W+tx]=rg_gdi_rop3_byte(rop3,0,s,d);
    }
}
int main(void)
{
    unsigned rop;
    int n=0;
    for(rop=0;rop<16;++rop) {
        unsigned rop3=rop|(rop<<4);
        seed();
        reference(7,5,3,2,26,10,rop3);
        da.ops.blit=counted;
        intercepts=0;
        assert(rg_gdi_blit_rop3(&da,&da,7,5,3,2,26,10,
                                 (unsigned long)rop3<<16));
        assert(intercepts==1 && memcmp(a,b,sizeof(a))==0);
        ++n;
        seed();
        reference(-4,3,6,-2,35,19,rop3);
        assert(rg_gdi_blit_rop3(&da,&da,-4,3,6,-2,35,19,
                                 (unsigned long)rop3<<16));
        assert(memcmp(a,b,sizeof(a))==0);
        ++n;
    }
    seed();
    memcpy(backup,a,sizeof(a));
    assert(!rg_gdi_blit_rop3(&da,&da,0,0,0,0,8,8,0x00F00021UL));
    assert(memcmp(a,backup,sizeof(a))==0);
    assert(!rg_gdi_blit_rop3(&da,0,0,0,0,0,8,8,0x00CC0020UL));
    assert(!rg_gdi_blit_rop3(&da,&da,0,0,0,0,0,8,0x00CC0020UL));
    /* Exhaustively validate the sixteen source-independent truth tables
     * for indexed 1/8bpp and clipping. Inversion twice restores bytes. */
    for (rop=0;rop<256u;++rop) {
        unsigned i;
        if ((rop&0x33u)!=((rop>>2)&0x33u)) continue;
        seed();
        assert(rg_gdi_blit_solid(&da,-2,1,12,3,(unsigned long)rop<<16,0x5au));
        for(i=0;i<W*H;++i) {
            unsigned expected=b[i];
            if (i/W>=1u && i/W<4u && i%W<10u)
                expected=rg_gdi_rop3_byte(rop,0x5au,0,b[i]);
            assert(a[i]==expected);
        }
    }
    for (rop=0;rop<256u;++rop) {
        unsigned i,bit,old,expected,pattern;
        if ((rop&0x33u)!=((rop>>2)&0x33u)) continue;
        for(pattern=0;pattern<2u;++pattern) {
            seed();
            assert(rg_init(&da,a,32,8,4,32,RG_FMT_INDEX1));
            assert(rg_gdi_blit_solid(&da,3,1,9,3,(unsigned long)rop<<16,pattern));
            for(i=0;i<32u;++i) for(bit=0;bit<8u;++bit) {
                unsigned x=(i%4u)*8u+bit, y=i/4u;
                old=(b[i]>>(7u-bit))&1u;
                expected=old;
                if(x>=3u && x<12u && y>=1u && y<4u)
                    expected=rg_gdi_rop3_byte(rop,(rg_u8)pattern,0,(rg_u8)old)&1u;
                assert(((a[i]>>(7u-bit))&1u)==expected);
            }
        }
    }
    seed();
    assert(rg_init(&da,a,32,8,4,32,RG_FMT_INDEX1));
    memcpy(backup,a,32);
    assert(rg_gdi_blit_solid(&da,0,0,32,8,0x00550009UL,0));
    assert(rg_gdi_blit_solid(&da,0,0,32,8,0x00550009UL,0));
    assert(!memcmp(a,backup,32));
    assert(!rg_gdi_blit_solid(&da,0,0,32,8,0x00CC0020UL,0));
    assert(!memcmp(a,backup,32));
    puts("PASS: GDI BitBlt bridge: 32 overlap/clipping cases, 16 ROP2 reductions, hardware override, safe rejection, source-independent solid ROPs");
    return 0;
}
