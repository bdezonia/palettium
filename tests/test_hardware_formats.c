#include <stdio.h>
#include <string.h>
#include "rg_hardware.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); return 1; } } while(0)
typedef struct DeviceMock { rg_u8 memory[128]; int starts,stops; } DeviceMock;
static int start(void *p) { ((DeviceMock *)p)->starts++; return 1; }
static int set_mode(void *p, RG_Mode *m)
{
    (void)p;
    if (m->format != RG_FMT_RGB565 || m->width != 8 || m->height != 4) return 0;
    m->stride = 20; m->capacity = 80;
    return 1;
}
static rg_u8 read_byte(void *p, rg_offset o) { return ((DeviceMock *)p)->memory[(unsigned)o]; }
static void write_byte(void *p, rg_offset o, rg_u8 v) { ((DeviceMock *)p)->memory[(unsigned)o]=v; }
static void stop(void *p) { ((DeviceMock *)p)->stops++; }
int main(void)
{
    RG_Device dev;
    RG_Hardware hw;
    RG_HwBinding b;
    RG_Mode mode;
    DeviceMock fixture;
    rg_pixel pixel;
    memset(&fixture,0,sizeof fixture); memset(&hw,0,sizeof hw); memset(&b,0,sizeof b);
    hw.context=&fixture; hw.initialize=start; hw.set_mode=set_mode;
    hw.read_byte=read_byte; hw.write_byte=write_byte; hw.shutdown=stop;
    mode.width=8; mode.height=4; mode.format=RG_FMT_RGB565;
    mode.stride=0; mode.capacity=0;
    CHECK(rg_hw_open(&b,&dev,&hw,&mode));
    CHECK(dev.surface.format==RG_FMT_RGB565);
    CHECK(dev.surface.stride==20);
    dev.ops.pixel(&dev,1,1,0xABCDUL);
    CHECK(rg_get_pixel(&dev,1,1,&pixel) && pixel==0xABCDUL);
    rg_hw_close(&b);
    CHECK(fixture.starts==1 && fixture.stops==1);
    /* ET4000 remains INDEX8-only, not a generic true-color implementation. */
    puts("PASS: hardware mode negotiates RGB565; ET4000 remains INDEX8-only by contract");
    return 0;
}
