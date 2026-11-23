#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pal_screen_host.h"
#include "et4k.h"

#define CAP 524288UL
#define TOKEN 0x1A2B0040UL

typedef struct Mock {
    rg_u8 bytes[CAP];
    rg_u8 bank;
    unsigned changes, modes, restores, bad, palette;
} Mock;

static rg_u8 port_in(void *p, unsigned port)
{
    Mock *m=(Mock *)p;
    if (port!=0x3CD) { ++m->bad; return 0; }
    return m->bank;
}
static void port_out(void *p, unsigned port, rg_u8 val)
{
    Mock *m=(Mock *)p;
    if (port==0x3CD) { m->bank=val; ++m->changes; }
    else if (port==0x3C8 || port==0x3C9) ++m->palette;
    else ++m->bad;
}
static rg_u8 read_window(void *p, unsigned off)
{
    Mock *m=(Mock *)p;
    unsigned long at=(unsigned long)(m->bank>>4)*65536UL+off;
    if (at>=CAP) { ++m->bad; return 0; }
    return m->bytes[at];
}
static void write_window(void *p, unsigned off, rg_u8 val)
{
    Mock *m=(Mock *)p;
    unsigned long at=(unsigned long)(m->bank&15)*65536UL+off;
    if (at>=CAP) { ++m->bad; return; }
    m->bytes[at]=val;
}
static int mode_set(void *p, unsigned w, unsigned h)
{
    Mock *m=(Mock *)p;
    ++m->modes;
    return w==640 && h==480;
}
static void mode_restore(void *p) { ++((Mock *)p)->restores; }

int main(void)
{
    Mock *m=(Mock *)calloc(1,sizeof(*m));
    ET4K_Port io;
    ET4K_State et;
    RG_Hardware hw;
    PAL_ScreenHost host;
    rg_offset a=65535UL, b=65536UL;
    assert(m);
    memset(&io,0,sizeof(io));
    memset(&host,0,sizeof(host));
    io.cookie=m; io.in_port=port_in; io.out_port=port_out;
    io.mem_read=read_window; io.mem_write=write_window;
    io.set_mode=mode_set; io.restore=mode_restore;
    et4k_setup(&et,io,CAP);
    et4k_make_hardware(&et,&hw);
    assert(!pal_screen_host_start(&host,&hw,640,480,0));
    assert(!pal_screen_host_start(&host,&hw,800,600,TOKEN));
    assert(m->restores==1);
    assert(pal_screen_host_start(&host,&hw,640,480,TOKEN));
    assert(!pal_screen_host_start(&host,&hw,640,480,TOKEN));
    assert(host.pdevice.state==PAL_PD_ACTIVE);
    rg_write_byte(&host.device,a,0x12);
    rg_write_byte(&host.device,b,0x34);
    assert(rg_read_byte(&host.device,a)==0x12);
    assert(rg_read_byte(&host.device,b)==0x34);
    assert((m->bank>>4)==1);
    assert((m->bank&15)==1);
    rg_software_fill(&host.device,5,5,40,15,0x71);
    assert(pal_screen_host_bitblt(&host,TOKEN,30,35,TOKEN,5,5,
                                  40,15,0x00CC0020UL));
    assert(rg_read_byte(&host.device,35UL*640UL+30)==0x71);
    assert(!pal_screen_host_bitblt(&host,TOKEN+1,0,0,TOKEN,0,0,
                                   10,10,0x00CC0020UL));
    assert(pal_screen_host_palette(&host,9,255,128,0));
    assert(m->palette==4);
    assert(m->changes>3 && m->bad==0);
    pal_screen_host_stop(&host);
    assert(m->restores==2);
    assert(!pal_screen_host_bitblt(&host,TOKEN,0,0,TOKEN,0,0,
                                   5,5,0x00CC0020UL));
    assert(!pal_screen_host_palette(&host,2,0,0,0));
    pal_screen_host_stop(&host);
    assert(m->restores==2);
    assert(pal_screen_host_start(&host,&hw,640,480,TOKEN));
    pal_screen_host_stop(&host);
    assert(m->restores==3);
    free(m);
    puts("PASS: Win31 lifecycle -> ET4000 backend -> banked VRAM -> GDI BitBlt");
    return 0;
}
