/* v0036: test the exact ET4000AX CRTC 37h I/O snapshot path.
 * This mock rejects any CRTC DATA write, 3CDh access, mode-setting or VRAM
 * activity. A single CRTC index selection and restore are permitted.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "et4k.h"
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"FAIL CRTC37 line %d: %s\n",__LINE__,#x);exit(1); } \
} while (0)

typedef struct Mock {
    unsigned misc;
    unsigned base;
    rg_u8 index;
    rg_u8 raw37;
    unsigned reads;
    unsigned index_writes;
    unsigned errors;
} Mock;
static rg_u8 in_port(void *ctx,unsigned port)
{
    Mock *m=(Mock *)ctx;
    ++m->reads;
    if (port==0x3ccu) return (rg_u8)m->misc;
    if (port==m->base) return m->index;
    if (port==m->base+1u && m->index==0x37u) return m->raw37;
    ++m->errors;
    return 0;
}
static void out_port(void *ctx,unsigned port,rg_u8 v)
{
    Mock *m=(Mock *)ctx;
    if (port!=m->base || m->index_writes>=2u) {
        ++m->errors;
        return;
    }
    if (m->index_writes==0u && v!=0x37u) ++m->errors;
    if (m->index_writes==1u && v!=0x13u) ++m->errors;
    m->index=v;
    ++m->index_writes;
}
static void check_snapshot(unsigned misc,rg_u8 raw,
                           unsigned port,unsigned bus,unsigned depth,
                           unsigned long bytes)
{
    ET4K_Port io;
    ET4K_Crtc37 result;
    Mock m;
    memset(&m,0,sizeof m);
    memset(&io,0,sizeof io);
    memset(&result,0xaa,sizeof result);
    m.misc=misc; m.base=port; m.index=0x13u; m.raw37=raw;
    io.cookie=&m;io.in_port=in_port;io.out_port=out_port;
    CHECK(et4k_crtc37_snapshot(&io,&result));
    CHECK(result.misc_output==misc);
    CHECK(result.crtc_index_port==port);
    CHECK(result.previous_index==0x13u);
    CHECK(result.raw_value==raw);
    CHECK(result.bus_width_bits==bus);
    CHECK(result.chip_depth_k==depth);
    CHECK(result.capacity_hint==bytes);
    CHECK(m.index==0x13u && m.index_writes==2u && m.reads==3u);
    CHECK(m.errors==0);
}
int main(void)
{
    ET4K_Port io;
    ET4K_Crtc37 result,old;
    rg_u8 raws[8]={0x00,0x01,0x02,0x03,0x08,0x09,0x0a,0x0b};
    unsigned long capacities[8]={65536UL,65536UL,131072UL,262144UL,
                                 262144UL,262144UL,524288UL,1048576UL};
    unsigned i;
    for(i=0;i<8u;++i) {
        unsigned width=(i%4u<2u)?8u:((i%4u==2u)?16u:32u);
        unsigned depth=(i<4u)?64u:256u;
        check_snapshot(0x01u,raws[i],0x3d4u,width,depth,capacities[i]);
        check_snapshot(0x00u,raws[i],0x3b4u,width,depth,capacities[i]);
    }
    check_snapshot(0x01u,0x8bu,0x3d4u,32u,256u,1048576UL);
    check_snapshot(0x00u,0xf2u,0x3b4u,16u,64u,131072UL);
    memset(&io,0,sizeof io);
    memset(&result,0x5a,sizeof result);
    old=result;
    CHECK(!et4k_crtc37_snapshot(0,&result));
    CHECK(!et4k_crtc37_snapshot(&io,&result));
    CHECK(memcmp(&result,&old,sizeof result)==0);
    io.in_port=in_port;
    CHECK(!et4k_crtc37_snapshot(&io,&result));
    CHECK(!et4k_crtc37_snapshot(&io,0));
    printf("PASS: ET4000AX CRTC37 index restore, color/mono ports, memory hints; no VRAM/bank/BIOS writes\n");
    return 0;
}
