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
    assert(rg_init(&da,a,W,H,W));
    assert(rg_init(&db,b,W,H,W));
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
    puts("PASS: GDI BitBlt bridge: 32 overlap/clipping cases, 16 ROP2 reductions, hardware override, safe rejection");
    return 0;
}
