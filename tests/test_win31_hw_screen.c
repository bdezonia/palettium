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
    rg_u8 dac[256][3];
    unsigned dac_index, dac_channel;
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
    else if (port==0x3C8) {
        m->dac_index=val; m->dac_channel=0; ++m->palette;
    } else if (port==0x3C9) {
        if (m->dac_channel>=3) { ++m->bad; return; }
        m->dac[m->dac_index][m->dac_channel++]=val;
        ++m->palette;
    }
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
    PAL_U32 physical = 0, logical = 0;
    RG_Pen pen;
    RG_Brush brush;
    RG_Rect rect, clip;
    unsigned short map[256];
    int found;
    unsigned i;
    assert(m);
    memset(&io,0,sizeof(io));
    memset(&host,0,sizeof(host));
    io.cookie=m; io.in_port=port_in; io.out_port=port_out;
    io.mem_read=read_window; io.mem_write=write_window;
    io.set_mode=mode_set; io.restore=mode_restore;
    et4k_setup(&et,io,CAP);
    et4k_make_hardware(&et,&hw);
    assert(!pal_screen_host_start(&host,&hw,640,480,0));
    /* Palette is mandatory for a Win3.x 256-color screen, but remains
       optional in the hardware API for other platform adapters. */
    hw.set_palette=0;
    assert(!pal_screen_host_start(&host,&hw,640,480,TOKEN));
    et4k_make_hardware(&et,&hw);
    assert(!pal_screen_host_start(&host,&hw,800,600,TOKEN));
    assert(m->restores==1);
    assert(pal_screen_host_start(&host,&hw,640,480,TOKEN));
    assert(!pal_screen_host_start(&host,&hw,640,480,TOKEN));
    assert(host.pdevice.state==PAL_PD_ACTIVE);
    assert(m->palette==80); /* exact 20 static DAC entries * 4 port writes */
    assert(m->dac[0][0]==0 && m->dac[0][1]==0 && m->dac[0][2]==0);
    assert(m->dac[9][0]==41 && m->dac[9][1]==50 && m->dac[9][2]==60);
    assert(m->dac[246][0]==63 && m->dac[246][1]==62 && m->dac[246][2]==60);
    assert(m->dac[255][0]==63 && m->dac[255][1]==63 && m->dac[255][2]==63);
    assert(pal_screen_host_colorinfo(&host,TOKEN,0xFF00000CUL,1,&logical,&physical));
    assert(logical==0xFF00000CUL && physical==12UL);
    assert(pal_screen_host_colorinfo(&host,TOKEN,255,0,&logical,&physical));
    assert(logical==0x00FFFFFFUL);
    assert(!pal_screen_host_colorinfo(&host,TOKEN+1,255,0,&logical,&physical));
    rg_write_byte(&host.device,a,0x12);
    rg_write_byte(&host.device,b,0x34);
    assert(rg_read_byte(&host.device,a)==0x12);
    assert(rg_read_byte(&host.device,b)==0x34);
    assert((m->bank>>4)==1);
    assert((m->bank&15)==1);
    assert(pal_screen_host_pixel(&host,TOKEN,0,0,0x83UL,13,&physical));
    assert(physical==1UL);
    assert(pal_screen_host_pixel(&host,TOKEN,0,0,0,0,&physical));
    assert(physical==0x83UL);
    assert(!pal_screen_host_pixel(&host,TOKEN+1,0,0,0,0,&physical));
    rg_software_fill(&host.device,5,5,40,15,0x71);
    assert(pal_screen_host_bitblt(&host,TOKEN,30,35,TOKEN,5,5,
                                  40,15,0x00CC0020UL));
    assert(rg_read_byte(&host.device,35UL*640UL+30)==0x71);
    assert(!pal_screen_host_bitblt(&host,TOKEN+1,0,0,TOKEN,0,0,
                                   10,10,0x00CC0020UL));
    assert(!pal_screen_host_palette(&host,9,255,128,0));
    assert(pal_screen_host_palette(&host,12,255,128,0));
    assert(m->palette==84);
    assert(m->dac[12][0]==63 && m->dac[12][1]==32 && m->dac[12][2]==0);
    assert(host.palette.entries[12].red==255 && host.palette.entries[12].green==128);
    for (i=0; i<256u; ++i) map[i]=(unsigned short)i;
    /* physical color remapping and ScanLR must cross the 64K bank. */
    assert(pal_screen_host_pixel(&host,TOKEN,254,102,6,13,&physical));
    assert(pal_screen_host_pixel(&host,TOKEN,256,102,7,13,&physical));
    assert(pal_screen_host_pixel(&host,TOKEN,260,102,8,13,&physical));
    assert(pal_screen_host_scanlr(&host,TOKEN,250,102,7,1,&found) && found==256);
    assert(pal_screen_host_scanlr(&host,TOKEN,260,102,8,3,&found) && found==260);
    assert(pal_screen_host_scanlr(&host,TOKEN,256,102,7,0,&found) && found==257);
    assert(pal_screen_host_scanlr(&host,TOKEN,639,102,8,1,&found) && found==-1);
    assert(!pal_screen_host_scanlr(&host,TOKEN,640,102,8,1,&found));
    assert(!pal_screen_host_scanlr(&host,TOKEN+1,256,102,7,1,&found));
    assert(!pal_screen_host_scanlr(&host,TOKEN,256,102,7,4,&found));
    assert(!pal_screen_host_scanlr(&host,TOKEN,256,102,256,1,&found));
    map[6]=106; map[7]=107; map[8]=108;
    assert(pal_screen_host_update_colors(&host,250,102,20,1,map));
    assert(pal_screen_host_pixel(&host,TOKEN,254,102,0,0,&physical) && physical==106);
    assert(pal_screen_host_pixel(&host,TOKEN,256,102,0,0,&physical) && physical==107);
    assert(pal_screen_host_pixel(&host,TOKEN,260,102,0,0,&physical) && physical==108);
    map[255]=256;
    assert(!pal_screen_host_update_colors(&host,250,102,20,1,map));
    assert(!pal_screen_host_update_colors(&host,635,102,20,1,map));
    /* v0026: real OS_RECTANGLE path draws through ET4000AX bank switching.
     * Row 102 starts at offset 65280, so x=256 reaches the next bank. */
    assert(rg_pen_realize(&host.palette,0,1,0xFF00003FUL,&pen));
    assert(rg_brush_realize(&host.palette,RG_BRUSH_SOLID,
                             0xFF00004DUL,0,0,0,0,&brush));
    rect.left=250; rect.top=102; rect.right=270; rect.bottom=105;
    clip.left=252; clip.top=102; clip.right=268; clip.bottom=105;
    assert(!pal_screen_host_rectangle(&host,TOKEN+1,rect,&clip,
                                       &pen,&brush,13,2));
    assert(pal_screen_host_rectangle(&host,TOKEN,rect,&clip,
                                     &pen,&brush,13,2));
    assert(rg_read_byte(&host.device,102UL*640UL+252UL)==63u);
    assert(rg_read_byte(&host.device,103UL*640UL+252UL)==77u);
    assert(rg_read_byte(&host.device,103UL*640UL+256UL)==77u);
    assert(rg_read_byte(&host.device,104UL*640UL+267UL)==63u);
    assert(m->changes>3 && m->bad==0);
    pal_screen_host_stop(&host);
    assert(!pal_screen_host_rectangle(&host,TOKEN,rect,&clip,
                                       &pen,&brush,13,2));
    assert(m->restores==2);
    assert(!pal_screen_host_bitblt(&host,TOKEN,0,0,TOKEN,0,0,
                                   5,5,0x00CC0020UL));
    assert(!pal_screen_host_palette(&host,12,0,0,0));
    assert(!pal_screen_host_colorinfo(&host,TOKEN,0,0,&logical,&physical));
    assert(!pal_screen_host_pixel(&host,TOKEN,0,0,0,0,&physical));
    pal_screen_host_stop(&host);
    assert(m->restores==2);
    assert(pal_screen_host_start(&host,&hw,640,480,TOKEN));
    assert(m->palette==164);
    assert(host.palette.entries[12].red==0); /* reset on restart */
    pal_screen_host_stop(&host);
    assert(m->restores==3);
    free(m);
    puts("PASS: Win31 ET4000 banked BitBlt/Pixel/ScanLR, palette, clipped Output rectangle");
    return 0;
}
