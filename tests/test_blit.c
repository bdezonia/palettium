#include <stdio.h>
#include <string.h>
#include "rgfx.h"
#include "sim_bank.h"
#define W 640
#define H 480
#define N (W*H)
static rg_u8 actual[N], expected[N], snapshot[N], other[N];
static unsigned long seed = 0x1234UL;
static unsigned rnd(void) { seed = seed * 1664525UL + 1013904223UL; return (unsigned)(seed >> 16); }
static rg_u8 reference_rop(rg_u8 s, rg_u8 d, unsigned rop)
{
    unsigned bit, out = 0;
    for (bit = 0; bit < 8; ++bit)
        if ((rop >> ((((s >> bit) & 1U) << 1) | ((d >> bit) & 1U))) & 1U)
            out |= 1U << bit;
    return (rg_u8)out;
}
static void reference(int dx, int dy, int sx, int sy, int w, int h, unsigned rop, int separate)
{
    long x, y, tx, ty, ux, uy;
    const rg_u8 *source = separate ? other : snapshot;
    for (y=0; y<h; ++y) for (x=0; x<w; ++x) {
        tx = (long)dx+x; ty = (long)dy+y;
        ux = (long)sx+x; uy = (long)sy+y;
        if (tx>=0 && tx<W && ty>=0 && ty<H &&
            ux>=0 && ux<W && uy>=0 && uy<H) {
            unsigned long dest = (unsigned long)ty*W+(unsigned long)tx;
            unsigned long src = (unsigned long)uy*W+(unsigned long)ux;
            expected[dest] = reference_rop(source[src], expected[dest], rop);
        }
    }
}
static int run_case(RG_Device *a, RG_Device *source, int dx, int dy,
                    int sx, int sy, int w, int h, unsigned rop, int separate)
{
    static unsigned long count = 0;
    memcpy(snapshot, expected, N);
    reference(dx,dy,sx,sy,w,h,rop,separate);
    a->ops.blit(a,source,dx,dy,sx,sy,w,h,rop);
    ++count;
    if (memcmp(actual,expected,N)) {
        unsigned long i;
        for (i=0;i<N;++i) if (actual[i]!=expected[i]) break;
        fprintf(stderr,"FAIL case %lu rop=%u separate=%d dx=%d dy=%d sx=%d sy=%d w=%d h=%d offset=%lu got=%u expected=%u\n",
            count,rop,separate,dx,dy,sx,sy,w,h,i,(unsigned)actual[i],(unsigned)expected[i]);
        return 0;
    }
    return 1;
}
/* Padded rows, all storage combinations and observable I/O counts. */
#define PW 19
#define PH 11
#define PS 24
#define PN (PS*PH)
typedef struct CountedBytes {
    rg_u8 data[PN];
    unsigned reads, writes;
} CountedBytes;
static rg_u8 counted_read(void *ctx, rg_offset off)
{
    CountedBytes *b=(CountedBytes *)ctx;
    ++b->reads;
    return b->data[off];
}
static void counted_write(void *ctx, rg_offset off, rg_u8 value)
{
    CountedBytes *b=(CountedBytes *)ctx;
    ++b->writes;
    b->data[off]=value;
}
static int row_tests(void)
{
    RG_Device d,s;
    RG_VramOps io;
    CountedBytes db,sb;
    rg_u8 want[PN], before[PN];
    unsigned mode,rop,i,x,y,same;
    io.read_byte=counted_read; io.write_byte=counted_write;
    for (mode=0;mode<4;++mode) for (rop=0;rop<16;++rop) {
        for (i=0;i<PN;++i) {
            db.data[i]=(rg_u8)(i*29u+7u);
            sb.data[i]=(rg_u8)(i*17u+11u);
        }
        memcpy(want,db.data,PN);
        db.reads=db.writes=sb.reads=sb.writes=0;
        if (mode&1u) {
            if (!rg_init_vram(&d,PW,PH,PS,PN,RG_FMT_INDEX8,io,&db)) return 0;
        } else if (!rg_init(&d,db.data,PW,PH,PS,PN,RG_FMT_INDEX8)) return 0;
        if (mode&2u) {
            if (!rg_init_vram(&s,PW,PH,PS,PN,RG_FMT_INDEX8,io,&sb)) return 0;
        } else if (!rg_init(&s,sb.data,PW,PH,PS,PN,RG_FMT_INDEX8)) return 0;
        for (y=0;y<5;++y) for (x=0;x<12;++x)
            want[(y+3)*PS+x+4]=reference_rop(sb.data[(y+1)*PS+x+2],want[(y+3)*PS+x+4],rop);
        rg_software_blit(&d,&s,4,3,2,1,12,5,rop);
        if (memcmp(want,db.data,PN)) return 0;
        if ((mode&1u) && db.writes!=(rop==10u?0u:60u)) return 0;
        if ((mode&1u) && db.reads!=((rop!=10u && (rop&5u)!=((rop>>1)&5u))?60u:0u)) return 0;
        if ((mode&2u) && sb.reads!=(((rop&3u)!=(rop>>2))?60u:0u)) return 0;
        if (sb.writes) return 0;
    }
    /* Flat memmove as well as callback rows: horizontal and vertical
     * overlapping copies in both directions, with padding untouched. */
    for (mode=0;mode<2;++mode) for (same=0;same<4;++same) {
        unsigned dx=same==0?5u:1u, sx=same==1?5u:1u;
        unsigned dy=same==2?4u:1u, sy=same==3?4u:1u;
        for (i=0;i<PN;++i) db.data[i]=(rg_u8)(i*31u+9u);
        memcpy(before,db.data,PN); memcpy(want,before,PN);
        if (mode) {
            if (!rg_init_vram(&d,PW,PH,PS,PN,RG_FMT_INDEX8,io,&db)) return 0;
        } else if (!rg_init(&d,db.data,PW,PH,PS,PN,RG_FMT_INDEX8)) return 0;
        for (y=0;y<6;++y) for (x=0;x<12;++x)
            want[(dy+y)*PS+dx+x]=before[(sy+y)*PS+sx+x];
        rg_software_blit(&d,&d,(int)dx,(int)dy,(int)sx,(int)sy,12,6,RG_ROP_COPY);
        if (memcmp(want,db.data,PN)) return 0;
    }
    puts("PASS: 72 padded INDEX8 row cases, flat/callback combinations, overlap, exact I/O counts");
    return 1;
}
int main(void)
{
    RG_Device dst, src;
    PAL_BankSim bank;
    RG_VramOps access;
    unsigned long i;
    unsigned rop, k;
    for (i=0;i<N;++i) { actual[i]=(rg_u8)rnd(); other[i]=(rg_u8)rnd(); }
    memcpy(expected, actual, N);
    pal_bank_init(&bank,actual,N);
    access.read_byte=pal_bank_read; access.write_byte=pal_bank_write;
    if (!rg_init_vram(&dst, W, H, W, (rg_offset)W * H, RG_FMT_INDEX8, access, &bank) || !rg_init(&src, other, W, H, W, (rg_offset)W * H, RG_FMT_INDEX8)) return 1;
    for (rop=0;rop<16;++rop) {
        if (!run_case(&dst,&dst,0,103,0,101,640,80,rop,0)) return 2;
        if (!run_case(&dst,&dst,20,100,0,100,600,65,rop,0)) return 3;
        if (!run_case(&dst,&dst,0,100,20,100,600,65,rop,0)) return 4;
        if (!run_case(&dst,&dst,-20,-10,0,0,150,200,rop,0)) return 5;
        if (!run_case(&dst,&dst,0,0,0,0,640,480,rop,0)) return 6;
        if (!run_case(&dst,&src,100,102,0,0,500,140,rop,1)) return 7;
    }
    for (k=0;k<250;++k) {
        int dx=(int)(rnd()%760)-60, dy=(int)(rnd()%590)-55;
        int sx=(int)(rnd()%760)-60, sy=(int)(rnd()%590)-55;
        int w=(int)(rnd()%175)+1, h=(int)(rnd()%125)+1;
        int separate=(int)(k%3 == 0);
        if (!run_case(&dst,separate ? &src : &dst,dx,dy,sx,sy,w,h,k%16,separate)) return 8;
    }
    if (bank.switch_count == 0) return 9;
    if (!row_tests()) return 10;
    printf("PASS: 346 BitBlt cases (all 16 ROPs, overlap, clipping, cross-bank, cross-device); bank switches=%lu\n",bank.switch_count);
    return 0;
}
