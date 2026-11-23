#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_screen_ddi.h"

#define W 48
#define H 24
#define TOKEN 0x3A1200A0UL
static rg_u8 pixels[W*H];
static rg_u8 snap[W*H];
static rg_u8 expected[W*H];
static unsigned override_calls;
static void intercepted(RG_Device *d, RG_Device *s, int dx, int dy,
                        int sx, int sy, int w, int h, unsigned rop)
{
    ++override_calls;
    rg_software_blit(d, s, dx, dy, sx, sy, w, h, rop);
}
static void seed(void)
{
    unsigned i;
    for (i=0; i<sizeof(pixels); ++i)
        pixels[i]=(rg_u8)((i*31u+7u)&255u);
}
int main(void)
{
    RG_Device dev;
    PAL_PDEVICE pd;
    PAL_ScreenDDI bridge;
    unsigned i;
    memset(&bridge,0,sizeof(bridge));
    seed();
    assert(rg_init(&dev,pixels,W,H,W));
    assert(pal_pd_prepare(&pd,W,H,W));
    assert(!pal_screen_ddi_attach(&bridge,&pd,&dev,TOKEN));
    assert(pal_pd_activate(&pd));
    assert(!pal_screen_ddi_attach(&bridge,&pd,&dev,0));
    assert(pal_screen_ddi_attach(&bridge,&pd,&dev,TOKEN));
    dev.ops.blit=intercepted;
    memcpy(snap,pixels,sizeof(snap));
    memcpy(expected,pixels,sizeof(expected));
    /* Reference for an overlapping SRCCOPY: reads from original snapshot. */
    for (i=0;i<10u;++i) {
        unsigned x;
        for (x=0;x<20u;++x)
            expected[(i+4u)*W+(x+7u)]=snap[(i+2u)*W+(x+3u)];
    }
    assert(pal_screen_ddi_bitblt(&bridge,TOKEN,7,4,TOKEN,3,2,
                                  20,10,0x00CC0020UL));
    assert(memcmp(pixels,expected,sizeof(pixels))==0);
    assert(override_calls==1u);
    memcpy(snap,pixels,sizeof(snap));
    assert(!pal_screen_ddi_bitblt(&bridge,TOKEN+1,0,0,TOKEN,0,0,
                                   10,10,0x00CC0020UL));
    assert(!pal_screen_ddi_bitblt(&bridge,TOKEN,0,0,TOKEN+1,0,0,
                                   10,10,0x00CC0020UL));
    assert(!pal_screen_ddi_bitblt(&bridge,0,0,0,TOKEN,0,0,
                                   10,10,0x00CC0020UL));
    assert(!pal_screen_ddi_bitblt(&bridge,TOKEN,0,0,TOKEN,0,0,
                                   10,10,0x00F00021UL));
    assert(memcmp(pixels,snap,sizeof(pixels))==0);
    assert(override_calls==1u);
    assert(pal_pd_stop(&pd));
    assert(!pal_screen_ddi_bitblt(&bridge,TOKEN,0,0,TOKEN,0,0,
                                   10,10,0x00CC0020UL));
    assert(pal_pd_activate(&pd));
    pal_screen_ddi_detach(&bridge);
    assert(!pal_screen_ddi_bitblt(&bridge,TOKEN,0,0,TOKEN,0,0,
                                   10,10,0x00CC0020UL));
    puts("PASS: Win31 DDI screen identity, overlap, HW override, rejection and lifecycle");
    return 0;
}
