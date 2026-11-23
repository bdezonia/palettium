/* v0029: ET4000AX VRAM-aware packed INDEX8 mode selection.
 * No BIOS, Windows or physical IO required; each callback is mocked.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "et4k.h"
#include "pal_screen_host.h"

#define KB256 262144UL
#define KB512 524288UL
#define MB1 1048576UL
#define TOKEN 0x12340020UL
#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr,"FAIL modes:%d: %s\n",__LINE__,#c); exit(1); } } while (0)

typedef struct Mock {
    rg_u8 *vram;
    unsigned long installed;
    rg_u8 bank;
    unsigned bad, modes, restores, capacity_queries, mode_queries;
    unsigned accepted_400, accepted_480;
    unsigned last_width, last_height;
} Mock;

static rg_u8 in_port(void *ctx, unsigned port)
{
    Mock *m=(Mock *)ctx;
    if (port==0x3CD) return m->bank;
    ++m->bad;
    return 0;
}
static void out_port(void *ctx, unsigned port, rg_u8 v)
{
    Mock *m=(Mock *)ctx;
    if (port==0x3CD) m->bank=v;
    else if (port != 0x3C8 && port != 0x3C9) ++m->bad;
}
static rg_u8 mem_read(void *ctx, unsigned off)
{
    Mock *m=(Mock *)ctx;
    unsigned long addr=(unsigned long)(m->bank>>4)*65536UL+off;
    if (addr >= m->installed) { ++m->bad; return 0; }
    return m->vram[addr];
}
static void mem_write(void *ctx, unsigned off, rg_u8 v)
{
    Mock *m=(Mock *)ctx;
    unsigned long addr=(unsigned long)(m->bank&15)*65536UL+off;
    if (addr >= m->installed) { ++m->bad; return; }
    m->vram[addr]=v;
}
static int set_mode(void *ctx, unsigned width, unsigned height)
{
    Mock *m=(Mock *)ctx;
    ++m->modes;
    m->last_width=width; m->last_height=height;
    return width==640u && ((height==400u && m->accepted_400) ||
                           (height==480u && m->accepted_480));
}
static void restore(void *ctx) { ++((Mock *)ctx)->restores; }
static unsigned long query_capacity(void *ctx)
{
    Mock *m=(Mock *)ctx;
    ++m->capacity_queries;
    return m->installed;
}
static int query_mode(void *ctx, unsigned bios)
{
    Mock *m=(Mock *)ctx;
    ++m->mode_queries;
    return (bios==ET4K_BIOS_640X400X8 && m->accepted_400) ||
           (bios==ET4K_BIOS_640X480X8 && m->accepted_480);
}
static void prepare(Mock *m, ET4K_Port *io)
{
    memset(io,0,sizeof(*io));
    io->cookie=m;
    io->in_port=in_port; io->out_port=out_port;
    io->mem_read=mem_read; io->mem_write=mem_write;
    io->set_mode=set_mode; io->restore=restore;
}
static void open_close(Mock *m, ET4K_State *state, ET4K_Port *io,
                       unsigned long known, unsigned h)
{
    RG_Hardware hw;
    RG_Device dev;
    RG_HwBinding bind;
    RG_Mode mode;
    unsigned old_modes=m->modes;
    et4k_setup(state,*io,known);
    et4k_make_hardware(state,&hw);
    memset(&bind,0,sizeof(bind));
    CHECK(et4k_preferred_mode(state,&mode));
    CHECK(mode.width==640u && mode.height==h &&
          mode.format==RG_FMT_INDEX8 && mode.stride==0);
    CHECK(rg_hw_open(&bind,&dev,&hw,&mode));
    CHECK(m->modes==old_modes+1 && m->last_height==h);
    CHECK(mode.capacity==known && mode.stride==640UL);
    CHECK(dev.surface.width==640u && dev.surface.height==h);
    rg_write_byte(&dev,65535UL,17u);
    rg_write_byte(&dev,65536UL,18u);
    CHECK(rg_read_byte(&dev,65535UL)==17u);
    CHECK(rg_read_byte(&dev,65536UL)==18u);
    rg_write_byte(&dev,(rg_offset)h*640UL-1UL,87u);
    CHECK(rg_read_byte(&dev,(rg_offset)h*640UL-1UL)==87u);
    CHECK(m->bad==0);
    rg_hw_close(&bind);
}
int main(void)
{
    Mock *m=(Mock *)calloc(1,sizeof(*m));
    ET4K_Port io;
    ET4K_State state;
    ET4K_ModeInfo info;
    RG_Mode mode;
    RG_Hardware hw;
    RG_HwBinding binding;
    RG_Device dev;
    PAL_ScreenHost host;
    PAL_GDIINFO candidate;
    unsigned old_modes, old_restores;
    CHECK(m!=0);
    m->vram=(rg_u8 *)calloc(MB1,1);
    CHECK(m->vram!=0);
    m->installed=KB256;
    m->accepted_400=1; m->accepted_480=1;
    prepare(m,&io);
    CHECK(et4k_mode_info(640u,400u,RG_FMT_INDEX8,&info) &&
          info.bios_mode==ET4K_BIOS_640X400X8 &&
          info.minimum_vram==256000UL);
    CHECK(et4k_mode_info(640u,480u,RG_FMT_INDEX8,&info) &&
          info.bios_mode==ET4K_BIOS_640X480X8 &&
          info.minimum_vram==307200UL);
    CHECK(!et4k_mode_info(640u,480u,RG_FMT_INDEX4,&info));
    CHECK(!et4k_mode_info(800u,600u,RG_FMT_INDEX8,&info));
    CHECK(!et4k_mode_info(640u,350u,RG_FMT_INDEX8,&info));
    et4k_setup(&state,io,KB256);
    CHECK(et4k_preferred_mode(&state,&mode) && mode.height==400u);
    CHECK(!et4k_mode_info(640u,480u,RG_FMT_INDEX4,0));
    CHECK(m->modes==0 && m->bad==0); /* read-only catalog */
    CHECK(et4k_mode_info(640u,480u,RG_FMT_INDEX8,&info));
    CHECK(!et4k_mode_eligible(&state,&info));
    CHECK(et4k_mode_info(640u,400u,RG_FMT_INDEX8,&info));
    CHECK(et4k_mode_eligible(&state,&info));
    open_close(m,&state,&io,KB256,400u);

    /* A 256K Win31 screen host can use the SAME pixel/ROP/DAC layer,
     * with proper PDEVICE and GDIINFO geometry and 4x64K bank limit. */
    et4k_setup(&state,io,KB256);
    et4k_make_hardware(&state,&hw);
    memset(&host,0,sizeof(host));
    CHECK(pal_screen_host_start(&host,&hw,640u,400u,TOKEN));
    CHECK(host.pdevice.width==640u && host.pdevice.height==400u);
    CHECK(host.device.surface.capacity==KB256);
    CHECK(pal_pd_capability_candidate(&candidate,&host.pdevice));
    CHECK(candidate.dpHorzRes==640 && candidate.dpVertRes==400 &&
          candidate.dpBitsPixel==8 && candidate.dpPlanes==1);
    CHECK(!pal_screen_host_start(&host,&hw,640u,480u,TOKEN));
    { PAL_U32 result = 0;
      CHECK(pal_screen_host_pixel(&host,TOKEN,639,399,99,13,&result)); }
    CHECK(rg_read_byte(&host.device,255999UL)==99u);
    CHECK(m->bad==0);
    pal_screen_host_stop(&host);

    /* Preflight must neither attempt BIOS mode setting nor restore a
     * mode which was never changed. This matters in live Win16. */
    et4k_setup(&state,io,KB256);
    et4k_make_hardware(&state,&hw);
    memset(&binding,0,sizeof(binding));
    mode.width=640u; mode.height=480u;
    mode.format=RG_FMT_INDEX8; mode.stride=0; mode.capacity=0;
    old_modes=m->modes; old_restores=m->restores;
    CHECK(!rg_hw_open(&binding,&dev,&hw,&mode));
    CHECK(m->modes==old_modes && m->restores==old_restores);

    m->installed=KB512;
    open_close(m,&state,&io,KB512,480u);
    m->installed=MB1;
    open_close(m,&state,&io,MB1,480u);

    /* Actual board support, not memory alone: firmware can refuse 2Eh. */
    m->installed=KB512;
    m->accepted_480=0;
    io.query_mode=query_mode;
    et4k_setup(&state,io,KB512);
    CHECK(et4k_preferred_mode(&state,&mode) && mode.height==400u);
    old_modes=m->modes;
    mode.height=480u;
    et4k_make_hardware(&state,&hw);
    memset(&binding,0,sizeof(binding));
    CHECK(!rg_hw_open(&binding,&dev,&hw,&mode));
    CHECK(m->modes==old_modes);
    CHECK(m->mode_queries>=2);
    m->accepted_480=1;

    /* Unknown capacity is never guessed. Optional callback must return a
     * recognized, nonzero size. No destructive alias probing. */
    io.query_mode=0;
    et4k_setup(&state,io,0);
    et4k_make_hardware(&state,&hw);
    memset(&binding,0,sizeof(binding));
    CHECK(!et4k_preferred_mode(&state,&mode));
    CHECK(!rg_hw_open(&binding,&dev,&hw,&mode));
    io.query_capacity=query_capacity;
    et4k_setup(&state,io,0);
    et4k_make_hardware(&state,&hw);
    CHECK(et4k_preferred_mode(&state,&mode) && mode.height==480u);
    mode.width=640u; mode.height=480u;
    mode.format=RG_FMT_INDEX8; mode.stride=0; mode.capacity=0;
    CHECK(rg_hw_open(&binding,&dev,&hw,&mode));
    CHECK(state.capacity==KB512 && m->capacity_queries==1);
    rg_hw_close(&binding);
    m->installed=0;
    et4k_setup(&state,io,0);
    et4k_make_hardware(&state,&hw);
    CHECK(!et4k_preferred_mode(&state,&mode));
    memset(&binding,0,sizeof(binding));
    old_modes=m->modes;
    CHECK(!rg_hw_open(&binding,&dev,&hw,&mode));
    CHECK(m->modes==old_modes && !binding.active);
    m->installed=131072UL; /* unsupported VRAM size cannot be inferred */
    et4k_setup(&state,io,0);
    CHECK(!rg_hw_open(&binding,&dev,&hw,&mode));
    CHECK(m->modes==old_modes && m->bad==0);

    /* Firmware refusal still shuts down, and a missing mode-query hook
     * never means "mode supported" -- set_mode must accept it. */
    m->installed=KB512; m->accepted_480=0;
    io.query_capacity=0;
    et4k_setup(&state,io,KB512);
    et4k_make_hardware(&state,&hw);
    old_modes=m->modes; old_restores=m->restores;
    CHECK(!rg_hw_open(&binding,&dev,&hw,&mode));
    CHECK(m->modes==old_modes+1 && m->restores==old_restores+1);
    free(m->vram);
    free(m);
    puts("PASS: ET4000AX 256K/512K/1M modes, BIOS gates, Win31 host, no unsafe VRAM probing");
    return 0;
}
