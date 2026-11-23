#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rg_win31_screen.h"
#include "rg_gdi_rop.h"
#define WIDTH 48
#define HEIGHT 23
static rg_u8 data[WIDTH * HEIGHT];
static rg_u8 ref[WIDTH * HEIGHT];
static rg_u8 snapshot[WIDTH * HEIGHT];
static RG_Device device;
static RG_Win31Screen screen;
static unsigned intercepted;
static void blit_override(RG_Device *d, RG_Device *s, int dx,int dy,
                          int sx,int sy,int w,int h,unsigned rop)
{
    ++intercepted;
    rg_software_blit(d,s,dx,dy,sx,sy,w,h,rop);
}
static void seed(void)
{
    unsigned i;
    for (i = 0; i < sizeof(data); ++i)
        data[i] = (rg_u8)((i * 71U + 39U) & 255U);
    memcpy(ref, data, sizeof(data));
    assert(rg_init(&device, data, WIDTH, HEIGHT, WIDTH, (rg_offset)WIDTH * HEIGHT, RG_FMT_INDEX8));
    assert(rg_win31_screen_bind(&screen, &device, WIDTH, HEIGHT, WIDTH));
}
static void reference(int dx,int dy,int sx,int sy,int w,int h,unsigned rop3)
{
    int x,y;
    memcpy(snapshot, ref, sizeof(ref));
    for (y=0;y<h;++y) for (x=0;x<w;++x) {
        int a=dx+x,b=dy+y,c=sx+x,d=sy+y;
        if(a<0||b<0||c<0||d<0||a>=WIDTH||c>=WIDTH||b>=HEIGHT||d>=HEIGHT) continue;
        ref[b*WIDTH+a]=rg_gdi_rop3_byte(rop3,0,
                        snapshot[d*WIDTH+c], snapshot[b*WIDTH+a]);
    }
}
int main(void)
{
    unsigned rop;
    rg_u8 before[sizeof(data)];
    RG_Win31Screen inactive;
    memset(&inactive, 0, sizeof(inactive));
    for (rop=0;rop<16U;++rop) {
        unsigned op = rop | (rop << 4);
        seed();
        reference(8,4,3,2,32,16,op);
        assert(rg_win31_screen_bitblt(&screen,&screen,8,4,3,2,32,16,
                                     (unsigned long)op<<16));
        assert(memcmp(data,ref,sizeof(data))==0);
        seed();
        reference(-3,0,7,5,35,16,op);
        assert(rg_win31_screen_bitblt(&screen,&screen,-3,0,7,5,35,16,
                                     (unsigned long)op<<16));
        assert(memcmp(data,ref,sizeof(data))==0);
    }
    seed();
    device.ops.blit=blit_override;
    intercepted=0;
    assert(rg_win31_screen_bitblt(&screen,&screen,0,0,5,5,10,8,0x00CC0020UL));
    assert(intercepted == 1);
    memcpy(before,data,sizeof(data));
    assert(!rg_win31_screen_bitblt(&screen,&screen,0,0,0,0,8,8,0x00F00021UL));
    assert(memcmp(before,data,sizeof(data))==0);
    assert(!rg_win31_screen_bitblt(&screen,&inactive,0,0,0,0,8,8,0x00CC0020UL));
    assert(!rg_win31_screen_bind(&inactive,&device,WIDTH,HEIGHT,WIDTH+1));
    assert(memcmp(before,data,sizeof(data))==0);
    rg_win31_screen_deactivate(&screen);
    assert(!rg_win31_screen_bitblt(&screen,&screen,0,0,0,0,8,8,0x00CC0020UL));
    assert(memcmp(before,data,sizeof(data))==0);
    puts("PASS: Win31 screen BitBlt: 32 reference cases, override, inactive/invalid/unsupported rejection");
    return 0;
}
