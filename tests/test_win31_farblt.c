#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_ddl_blit.h"
#define W 32
#define H 20
#define SEG 0x4B20u
#define OFF 0x0100u
static rg_u8 pixels[W*H];
static unsigned calls;
static void wrapped(RG_Device *dst, RG_Device *src, int dx, int dy,
                    int sx, int sy, int w, int h, unsigned rop)
{
    ++calls;
    rg_software_blit(dst, src, dx, dy, sx, sy, w, h, rop);
}
int main(void)
{
    PAL_ScreenHost host;
    RG_Device device;
    unsigned i;
    rg_u8 original[W*H];
    memset(&host, 0, sizeof(host));
    for (i = 0; i < sizeof(pixels); ++i) pixels[i] = (rg_u8)(i & 255u);
    assert(rg_init(&device, pixels, W, H, W, (rg_offset)W*H, RG_FMT_INDEX8));
    assert(pal_pd_prepare(&host.pdevice,W,H,W));
    assert(pal_pd_activate(&host.pdevice));
    assert(pal_screen_ddi_attach(&host.ddi,&host.pdevice,&device,
                                 pal_ddi_far_token(SEG,OFF)));
    host.running = 1;
    device.ops.blit = wrapped;
    assert(pal_ddi_far_token(SEG,OFF)==0x4B200100UL);
    assert(pal_ddi_far_token(0,0)==0);
    memcpy(original,pixels,sizeof(pixels));
    assert(!pal_ddi_bitblt_words(&host,SEG,OFF,0,0,SEG,OFF,0,0,0,4,0xCC0020UL));
    assert(!pal_ddi_bitblt_words(&host,SEG,OFF+2,0,0,SEG,OFF,0,0,4,4,0xCC0020UL));
    assert(!pal_ddi_bitblt_words(&host,SEG,OFF,0,0,SEG+1,OFF,0,0,4,4,0xCC0020UL));
    assert(!pal_ddi_bitblt_words(&host,SEG,OFF,0,0,SEG,OFF,0,0,4,4,0xF00021UL));
    assert(!pal_ddi_bitblt_words(0,SEG,OFF,0,0,SEG,OFF,0,0,4,4,0xCC0020UL));
    assert(memcmp(original,pixels,sizeof(pixels))==0 && calls==0);
    assert(pal_ddi_bitblt_words(&host,SEG,OFF,7,6,SEG,OFF,2,3,9,5,0xCC0020UL));
    assert(calls==1);
    for (i=0; i<5; ++i) {
        unsigned x;
        for (x=0; x<9; ++x)
            assert(pixels[(6+i)*W+7+x] == original[(3+i)*W+2+x]);
    }
    assert(pal_pd_stop(&host.pdevice));
    assert(!pal_ddi_bitblt_words(&host,SEG,OFF,0,0,SEG,OFF,0,0,4,4,0xCC0020UL));
    pal_screen_ddi_detach(&host.ddi);
    puts("PASS: Win31 far-token BitBlt routing, identities, ROP refusal, lifecycle");
    return 0;
}
