/* v0030: Win3.x ET4000AX mode preflight, session, and rollback. The real
 * Win16 BIOS/selector provider has the same ET4K_Port contract, but is
 * never called by these portable host tests. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pal_et4k_session.h"
#include "pal_vdd.h"

#define SIZE256 262144UL
#define SIZE512 524288UL
#define SIZE1M 1048576UL
#define TOKEN 0x42100030UL
#define CHECK(c) do { if (!(c)) { \
  fprintf(stderr,"FAIL native:%d %s\n",__LINE__,#c); exit(1); } } while(0)

typedef struct Mock {
    rg_u8 *vram;
    unsigned long bytes;
    unsigned bank, attempted, restored, query_count, bad;
    unsigned allow400, allow480, force_failure;
    unsigned last_height;
} Mock;
static rg_u8 port_in(void *p, unsigned port)
{
    Mock *m=(Mock *)p;
    if (port!=0x3CDu) { ++m->bad; return 0; }
    return (rg_u8)m->bank;
}
static void port_out(void *p, unsigned port, rg_u8 value)
{
    Mock *m=(Mock *)p;
    if (port==0x3CDu) m->bank=value;
    else if (port!=0x3C8u && port!=0x3C9u) ++m->bad;
}
static rg_u8 aperture_read(void *p, unsigned offset)
{
    Mock *m=(Mock *)p;
    unsigned long off=((unsigned long)(m->bank>>4)*65536UL)+offset;
    if (off>=m->bytes) { ++m->bad; return 0; }
    return m->vram[off];
}
static void aperture_write(void *p, unsigned offset, rg_u8 v)
{
    Mock *m=(Mock *)p;
    unsigned long off=((unsigned long)(m->bank&15u)*65536UL)+offset;
    if (off>=m->bytes) { ++m->bad; return; }
    m->vram[off]=v;
}
static int mode_set(void *p, unsigned width, unsigned height)
{
    Mock *m=(Mock *)p;
    ++m->attempted;
    m->last_height=height;
    return !m->force_failure && width==640u &&
       ((height==400u && m->allow400) ||
        (height==480u && m->allow480));
}
static void mode_restore(void *p) { ++((Mock *)p)->restored; }
static int mode_query(void *p, unsigned bios)
{
    Mock *m=(Mock *)p;
    ++m->query_count;
    return (bios==ET4K_BIOS_640X400X8 && m->allow400) ||
           (bios==ET4K_BIOS_640X480X8 && m->allow480);
}
static unsigned long capacity_query(void *p)
{
    return ((Mock *)p)->bytes;
}
static ET4K_Port make_port(Mock *m)
{
    ET4K_Port io;
    memset(&io,0,sizeof(io));
    io.cookie=m;
    io.in_port=port_in; io.out_port=port_out;
    io.mem_read=aperture_read; io.mem_write=aperture_write;
    io.set_mode=mode_set; io.restore=mode_restore;
    io.query_mode=mode_query;
    return io;
}
static void exercise(Mock *m, unsigned long bytes, unsigned height)
{
    PAL_ET4KSession session;
    PAL_GDIINFO candidate;
    PAL_U32 result;
    ET4K_Port io=make_port(m);
    unsigned before=m->attempted, old_restores=m->restored;
    memset(&session,0,sizeof(session));
    CHECK(pal_et4k_candidate(&candidate,io,bytes));
    CHECK(candidate.dpHorzRes==640 && candidate.dpVertRes==(int)height);
    CHECK(candidate.dpBitsPixel==8 && candidate.dpDEVICEsize==
          sizeof(PAL_PDEVICE));
    CHECK(m->attempted==before && m->restored==old_restores);
    CHECK(pal_et4k_session_start(&session,io,bytes,TOKEN));
    CHECK(m->attempted==before+1 && m->last_height==height);
    CHECK(session.host.running && session.host.pdevice.height==height);
    CHECK(session.host.device.surface.capacity==bytes);
    CHECK(!pal_et4k_session_start(&session,io,bytes,TOKEN));
    result=0;
    CHECK(pal_screen_host_pixel(&session.host,TOKEN,639,(int)height-1,
                                 213u,13u,&result));
    CHECK(m->vram[(unsigned long)height*640UL-1UL]==213u);
    CHECK(session.card.mode_attempted);
    pal_et4k_session_stop(&session);
    CHECK(!session.host.running && !session.card.mode_attempted);
    CHECK(m->restored==old_restores+1);
    pal_et4k_session_stop(&session);
    CHECK(m->restored==old_restores+1 && m->bad==0);
}
int main(void)
{
    Mock m;
    PAL_ET4KSession session;
    PAL_GDIINFO cap;
    ET4K_Port io;
    RG_Mode mode;
    RG_HwBinding binding;
    RG_Device dev;
    unsigned before;
    unsigned before_query;
    PAL_ET4KPlan plan;
    memset(&m,0,sizeof(m));
    m.vram=(rg_u8 *)calloc(SIZE1M,1);
    CHECK(m.vram!=0);
    m.allow400=m.allow480=1;
    m.bytes=SIZE256;
    exercise(&m,SIZE256,400u);
    m.bytes=SIZE512;
    exercise(&m,SIZE512,480u);
    m.bytes=SIZE1M;
    exercise(&m,SIZE1M,480u);
    m.bytes=SIZE512;
    m.allow480=0;
    exercise(&m,SIZE512,400u); /* ROM veto avoids switching to 480 */

    io=make_port(&m);
    memset(&session,0,sizeof(session));
    before=m.attempted;
    CHECK(!pal_et4k_candidate(&cap,io,0));
    CHECK(!pal_et4k_candidate(&cap,io,131072UL));
    CHECK(!pal_et4k_session_start(&session,io,0,TOKEN));
    CHECK(!pal_et4k_session_start(&session,io,SIZE512,0));
    CHECK(m.attempted==before);
    io.query_capacity=capacity_query;
    CHECK(pal_et4k_candidate(&cap,io,0) && cap.dpVertRes==400);
    CHECK(pal_et4k_session_start(&session,io,0,TOKEN));
    CHECK(session.card.capacity==SIZE512);
    pal_et4k_session_stop(&session);
    CHECK(m.attempted==before+1);

    /* Firmware refuses a mode AFTER a mode-set attempt. Its provider may
     * have partially changed registers; restore exactly once. */
    memset(&session,0,sizeof(session));
    io=make_port(&m);
    m.allow480=1;
    m.force_failure=1;
    before=m.restored;
    CHECK(!pal_et4k_session_start(&session,io,SIZE512,TOKEN));
    CHECK(!session.host.running && !session.card.mode_attempted);
    CHECK(m.restored==before+1);
    CHECK(m.bad==0);
    m.force_failure=0;

    /* A preflight refusal never invokes mode-setting or BIOS restore. */
    memset(&binding,0,sizeof(binding));
    et4k_setup(&session.card,io,SIZE256);
    et4k_make_hardware(&session.card,&session.hardware);
    mode.width=640; mode.height=480; mode.format=RG_FMT_INDEX8;
    mode.stride=0; mode.capacity=0;
    before=m.restored;
    CHECK(!rg_hw_open(&binding,&dev,&session.hardware,&mode));
    CHECK(m.restored==before && !session.card.mode_attempted);
    CHECK(m.bad==0);
    /* v0037 startup planning is pure: no BIOS, bank, port or VRAM access.
     * An observed CRTC37 capacity can REJECT mismatched config; it must
     * never silently override the user's explicitly provided capacity. */
    m.bytes=SIZE512;
    io=make_port(&m);
    before=m.attempted;
    before_query=m.query_count;
    CHECK(pal_et4k_plan(&plan,SIZE256,SIZE256,0x0C29UL,"*vddvga"));
    CHECK(plan.mode.height==400u && plan.bios_mode==0x2Fu);
    CHECK(plan.status==PAL_ET4K_PLAN_OK && !plan.can_enable);
    CHECK(plan.vdd_assessment==PAL_VDD_UNVERIFIED);
    CHECK(pal_et4k_plan_capability(&cap,&plan) && cap.dpVertRes==400);
    CHECK(pal_et4k_plan(&plan,SIZE512,SIZE512,0x0C29UL,"*vddvga"));
    CHECK(plan.status==PAL_ET4K_PLAN_STOCK_VDD_LIMIT &&
          plan.mode.height==400u && plan.framebuffer_bytes==256000UL);
    CHECK(plan.vdd_assessment==PAL_VDD_UNVERIFIED && !plan.can_enable);
    CHECK(pal_et4k_plan(&plan,SIZE1M,SIZE1M,0x0C29UL,"*vddvga"));
    CHECK(plan.status==PAL_ET4K_PLAN_STOCK_VDD_LIMIT &&
          plan.mode.height==400u);
    CHECK(pal_et4k_plan(&plan,SIZE512,SIZE512,0x0C29UL,
                        "VDDVGA30.386"));
    CHECK(plan.status==PAL_ET4K_PLAN_OK && plan.mode.height==480u);
    CHECK(plan.vdd_assessment==PAL_VDD_UNVERIFIED);
    CHECK(pal_et4k_plan(&plan,SIZE512,SIZE512,0x0001UL,"*vddvga"));
    CHECK(plan.mode.height==480u &&
          plan.vdd_assessment==PAL_VDD_NOT_ENHANCED);
    CHECK(pal_et4k_plan(&plan,SIZE512,0UL,0x0C29UL,"custom.386"));
    CHECK(plan.mode.height==480u && !plan.can_enable);
    CHECK(!pal_et4k_plan(&plan,0UL,SIZE512,0x0C29UL,"*vddvga"));
    CHECK(plan.status==PAL_ET4K_PLAN_NO_CAPACITY);
    CHECK(!pal_et4k_plan_capability(&cap,&plan));
    CHECK(!pal_et4k_plan(&plan,SIZE256,SIZE512,0x0C29UL,"*vddvga"));
    CHECK(plan.status==PAL_ET4K_PLAN_HINT_CONFLICT);
    CHECK(!pal_et4k_plan(&plan,123UL,0,0x0C29UL,"*vddvga"));
    CHECK(plan.status==PAL_ET4K_PLAN_NO_CAPACITY);
    CHECK(m.attempted==before && m.query_count==before_query && m.bad==0);

    /* Explicit low-memory geometry goes through the already-tested host
     * path even with more RAM installed. This test grants no Win16 VDD
     * permission: it is a mock platform exercise only. */
    memset(&session,0,sizeof(session));
    m.allow400=1;
    CHECK(pal_et4k_session_start_mode(&session,io,SIZE512,TOKEN,
                                      640u,400u));
    CHECK(session.host.pdevice.height==400u && m.last_height==400u);
    pal_et4k_session_stop(&session);
    memset(&session,0,sizeof(session));
    before=m.attempted;
    CHECK(!pal_et4k_session_start_mode(&session,io,SIZE256,TOKEN,
                                       640u,480u));
    CHECK(m.attempted==before && !session.host.running);

    free(m.vram);
    puts("PASS: v0037 VRAM/VDD GDI plan, explicit-mode session, rollback and 256K/512K/1M host");
    return 0;
}
