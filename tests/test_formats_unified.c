#include "rgfx.h"
#include "sim_bank.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)

static rg_pixel mask_for(unsigned bits)
{
    if (bits == 32) return 0xffffffffUL;
    return (1UL << bits) - 1UL;
}
static rg_pixel rop_ref(rg_pixel s, rg_pixel d, unsigned r, rg_pixel mask)
{
    unsigned bit;
    rg_pixel result = 0;
    for (bit=0; bit<32; ++bit) {
        unsigned index = (unsigned)(((s>>bit)&1UL)*2UL + ((d>>bit)&1UL));
        if ((r >> index) & 1U) result |= (1UL<<bit);
    }
    return result & mask;
}
static int exercise(RG_PixelFormat fmt)
{
    const unsigned w=21, h=13;
    rg_offset stride = rg_min_stride(fmt,w) + 5UL;
    rg_offset cap = stride * h;
    rg_u8 a[2048], b[2048], snapshot[2048];
    RG_Device d, src;
    rg_pixel value, mask = mask_for(rg_pixel_bits(fmt));
    unsigned x,y,rop;
    CHECK(cap < sizeof a);
    memset(a,0,sizeof a); memset(b,0,sizeof b);
    CHECK(rg_init(&d,a,w,h,stride,cap,fmt));
    CHECK(rg_init(&src,b,w,h,stride,cap,fmt));
    CHECK(!rg_init(&src,b,w,h,rg_min_stride(fmt,w)-1,cap,fmt));
    CHECK(rg_init(&src,b,w,h,stride,cap,fmt));
    for (y=0;y<h;++y) for (x=0;x<w;++x) {
        rg_pixel p = ((rg_pixel)x*0x10203UL + (rg_pixel)y*0x3123UL) & mask;
        CHECK(rg_put_pixel(&d,(int)x,(int)y,p));
        CHECK(rg_get_pixel(&d,(int)x,(int)y,&value) && value==p);
        CHECK(rg_put_pixel(&src,(int)x,(int)y,(p^mask)&mask));
    }
    CHECK(!rg_put_pixel(&d,-1,0,1));
    CHECK(!rg_get_pixel(&d,(int)w,0,&value));
    for (rop=0; rop<16; ++rop) {
        rg_pixel expect;
        CHECK(rg_get_pixel(&d,6,5,&value));
        CHECK(rg_get_pixel(&src,2,3,&expect));
        expect = rop_ref(expect,value,rop,mask);
        d.ops.blit(&d,&src,6,5,2,3,1,1,rop);
        CHECK(rg_get_pixel(&d,6,5,&value) && value==expect);
    }
    /* Independent source snapshot for same-device overlapping COPY. */
    memcpy(snapshot,a,(size_t)cap);
    d.ops.blit(&d,&d,2,4,1,3,16,8,RG_ROP_COPY);
    for (y=0;y<8;++y) for (x=0;x<16;++x) {
        RG_Device prev;
        CHECK(rg_init(&prev,snapshot,w,h,stride,cap,fmt));
        CHECK(rg_get_pixel(&prev,(int)x+1,(int)y+3,&value));
        { rg_pixel actual; CHECK(rg_get_pixel(&d,(int)x+2,(int)y+4,&actual) && actual==value); }
    }
    d.ops.fill(&d,-2,-1,4,3,mask);
    CHECK(rg_get_pixel(&d,0,0,&value) && value==mask);
    d.ops.line(&d,0,0,20,12,0);
    CHECK(rg_get_pixel(&d,20,12,&value) && value==0);
    return 0;
}
int main(void)
{
    RG_PixelFormat f;
    rg_u8 *memory;
    PAL_BankSim sim;
    RG_VramOps ops;
    RG_Device dev;
    rg_pixel value;
    rg_offset offset = 65535UL;
    unsigned n;
    for (n=1;n<=10;++n) {
        f = (RG_PixelFormat)n;
        if (exercise(f)) return 1;
    }
    /* INDEX2: four pixels per byte, high-order pair first. */
    {
        rg_u8 packed[2] = { 0, 0 };
        RG_Device two;
        unsigned i;
        CHECK(rg_init(&two,packed,5,1,2,2,RG_FMT_INDEX2));
        for (i=0;i<5;++i) CHECK(rg_put_pixel(&two,(int)i,0,(rg_pixel)(i&3U)));
        CHECK(packed[0]==0x1b && packed[1]==0x00);
        CHECK(rg_put_pixel(&two,4,0,3));
        CHECK(packed[1]==0xc0);
        CHECK(rg_get_pixel(&two,4,0,&value) && value==3);
    }
    /* INDEX16: a 16-bit palette index, NOT an RGB565 pixel. */
    {
        rg_u8 packed[4] = { 0, 0, 0, 0 };
        RG_Device sixteen;
        CHECK(rg_init(&sixteen,packed,2,1,4,4,RG_FMT_INDEX16));
        CHECK(rg_put_pixel(&sixteen,0,0,0x1234UL));
        CHECK(rg_put_pixel(&sixteen,1,0,0xfedcUL));
        CHECK(packed[0]==0x34 && packed[1]==0x12);
        CHECK(packed[2]==0xdc && packed[3]==0xfe);
        CHECK(rg_get_pixel(&sixteen,1,0,&value) && value==0xfedcUL);
    }
    /* 24-bit pixel starting at byte 65535, spanning two 64K banks. */
    memory=(rg_u8 *)calloc(65540UL,1);
    CHECK(memory!=0);
    pal_bank_init(&sim,memory,65540UL);
    ops.read_byte=pal_bank_read; ops.write_byte=pal_bank_write;
    CHECK(rg_init_vram(&dev,21846,1,65540UL,65540UL,
                       RG_FMT_RGB888,ops,&sim));
    /* x=21845 -> offset 65535. */
    CHECK((rg_offset)21845*3UL==offset);
    CHECK(rg_put_pixel(&dev,21845,0,0x123456UL));
    CHECK(memory[65535]==0x56 && memory[65536]==0x34 && memory[65537]==0x12);
    CHECK(rg_get_pixel(&dev,21845,0,&value) && value==0x123456UL);
    CHECK(sim.switch_count>0);
    CHECK(rg_rgb_pack(RG_FMT_RGB565,255,255,255,0)==0xffffUL);
    CHECK(rg_rgb_pack(RG_FMT_RGB888,0x12,0x34,0x56,0)==0x123456UL);
    free(memory);
    puts("PASS: unified 10 formats; clipping, ROPs, overlap, RGB packing, cross-bank pixel");
    return 0;
}
