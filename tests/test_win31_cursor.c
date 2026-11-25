#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_cursor16.h"
#define W 40u
#define H 32u
static rg_u8 frame[W*H];
static rg_u8 vram[640u*400u];
static unsigned reads,writes;
static PAL_Cursor16 *irq_target;
static unsigned irq_calls,irq_inject;
static rg_u8 rd(void *ctx,rg_offset off)
{(void)ctx;++reads;return vram[off];}
static void wr(void *ctx,rg_offset off,rg_u8 value)
{
    (void)ctx;
    ++writes;
    vram[off]=value;
    if (irq_inject && irq_target) {
        /* Model a mouse update + nested timer callback in the middle of
         * cursor composition; the outer compositor must not recurse. */
        assert(irq_target->composing);
        irq_inject=0;
        ++irq_calls;
        pal_cursor16_post_move(irq_target,260,102);
        pal_cursor16_check(irq_target);
    }
}
static void word(rg_u8 *p,int value)
{unsigned w=(unsigned)value;p[0]=(rg_u8)w;p[1]=(rg_u8)(w>>8);}
static void shape(rg_u8 *p,unsigned width,unsigned height,int hotx,int hoty)
{
    unsigned i;
    memset(p,0,PAL_CURSOR16_MAX_WIRE);
    word(p,hotx);word(p+2,hoty);word(p+4,(int)width);
    word(p+6,(int)height);word(p+8,4);word(p+10,0);
    for(i=0;i<4u*height;++i) p[PAL_CURSOR16_HEADER+i]=0xff;
    /* first row: AND0/XOR0 black, AND0/XOR1 white, transparent, inverted */
    p[12]=0x3fu;
    p[12+height*4u]=0x50u;
}
int main(void)
{
    static PAL_Cursor16 cursor;
    RG_Device d,banked;
    RG_VramOps io;
    rg_u8 wire[PAL_CURSOR16_MAX_WIRE];
    unsigned i,write_before,read_before;
    memset(&cursor,0,sizeof(cursor));
    assert(rg_init(&d,frame,W,H,W,W*H,RG_FMT_INDEX8));
    memset(frame,0x55,sizeof(frame));
    shape(wire,8,8,0,0);
    assert(pal_cursor16_wire_size(wire,11)==0);
    assert(pal_cursor16_wire_size(wire,76)==76);
    assert(pal_cursor16_wire_size(wire,75)==0);
    wire[8]=3;assert(!pal_cursor16_wire_size(wire,76));wire[8]=4;
    wire[10]=1;assert(!pal_cursor16_wire_size(wire,76));
    wire[11]=1;assert(pal_cursor16_wire_size(wire,76)==76);
    wire[10]=2;assert(!pal_cursor16_wire_size(wire,76));
    wire[10]=1; /* explicit one-plane/one-bit metadata */
    assert(pal_cursor16_set(&cursor,wire,76));
    pal_cursor16_bind(&cursor,&d);
    assert(!cursor.image.visible);
    pal_cursor16_post_move(&cursor,6,5);
    assert(!cursor.image.visible); /* mouse IRQ only queues */
    pal_cursor16_check(&cursor);
    assert(cursor.image.visible);
    assert(frame[5u*W+6u]==0);
    assert(frame[5u*W+7u]==255);
    assert(frame[5u*W+8u]==0x55);
    assert(frame[5u*W+9u]==0xaa);
    pal_cursor16_begin_draw(&cursor);
    assert(!cursor.image.visible && frame[5u*W+6u]==0x55);
    pal_cursor16_post_move(&cursor,9,5);
    pal_cursor16_check(&cursor);
    assert(!cursor.image.visible);
    frame[5u*W+9u]=0x33; /* screen draw under new cursor */
    pal_cursor16_end_draw(&cursor);
    assert(cursor.image.visible && frame[5u*W+9u]==0);
    pal_cursor16_begin_draw(&cursor);
    assert(frame[5u*W+9u]==0x33); /* saved-under was updated */
    pal_cursor16_end_draw(&cursor);
    assert(pal_cursor16_set(&cursor,0,0));
    assert(!cursor.image.visible && frame[5u*W+9u]==0x33);
    for (i=0;i<W*H;++i) if(i!=5u*W+9u) assert(frame[i]==0x55);
    /* Signed hotspots outside rectangle permitted by the DDK. */
    shape(wire,8,8,-2,-3);
    assert(pal_cursor16_set(&cursor,wire,76));
    pal_cursor16_post_move(&cursor,0,0);
    pal_cursor16_check(&cursor);
    assert(cursor.image.visible);
    pal_cursor16_unbind(&cursor);
    assert(!cursor.image.visible);
    assert(cursor.screen==0);
    /* Banked framebuffer: updates are deferred while draw is active. */
    io.read_byte=rd;io.write_byte=wr;
    memset(vram,0x55,sizeof(vram));
    assert(rg_init_vram(&banked,640,400,640,640UL*400UL,
                         RG_FMT_INDEX8,io,0));
    shape(wire,8,8,0,0);
    assert(pal_cursor16_set(&cursor,wire,76));
    pal_cursor16_bind(&cursor,&banked);
    pal_cursor16_post_move(&cursor,254,102);
    assert(writes==0);
    pal_cursor16_check(&cursor);
    assert(vram[102UL*640UL+254UL]==0);
    assert(vram[102UL*640UL+255UL]==255);
    /* Only three nontransparent pixels in the 8x8 shape. */
    assert(reads==3u && writes==3u);
    write_before=writes; read_before=reads;
    pal_cursor16_hold(&cursor);
    pal_cursor16_exclude_rect(&cursor,262,102,10,8); /* touches edge only */
    pal_cursor16_exclude_rect(&cursor,0,0,32,32);
    assert(cursor.image.visible && cursor.draw_depth==1u);
    pal_cursor16_check(&cursor);
    pal_cursor16_end_draw(&cursor);
    assert(cursor.image.visible && reads==read_before && writes==write_before);
    /* Nested full draw must still hide after an outer non-hiding hold. */
    pal_cursor16_hold(&cursor);
    pal_cursor16_begin_draw(&cursor);
    assert(!cursor.image.visible && cursor.draw_depth==2u);
    assert(vram[102UL*640UL+254UL]==0x55);
    pal_cursor16_end_draw(&cursor);
    assert(!cursor.image.visible);
    pal_cursor16_end_draw(&cursor);
    assert(cursor.image.visible);
    pal_cursor16_hold(&cursor);
    pal_cursor16_exclude_rect(&cursor,253,101,2,2); /* overlaps one pixel */
    assert(!cursor.image.visible);
    assert(vram[102UL*640UL+254UL]==0x55);
    pal_cursor16_end_draw(&cursor);
    assert(cursor.image.visible);
    pal_cursor16_begin_draw(&cursor);
    write_before=writes;
    pal_cursor16_post_move(&cursor,256,102);
    assert(writes==write_before); /* no IRQ-time VRAM writes */
    vram[102UL*640UL+256UL]=0x77;
    pal_cursor16_end_draw(&cursor);
    assert(vram[102UL*640UL+256UL]==0);
    pal_cursor16_unbind(&cursor);
    assert(vram[102UL*640UL+256UL]==0x77);
    assert(reads>0 && writes>0);
    /* Reentrant cursor update cannot recursively composite or lose the
     * most recent published position; it is consumed at next idle check. */
    shape(wire,8,8,0,0);
    assert(pal_cursor16_set(&cursor,wire,76));
    pal_cursor16_bind(&cursor,&banked);
    irq_target=&cursor;
    irq_inject=1;
    pal_cursor16_post_move(&cursor,254,102);
    pal_cursor16_check(&cursor);
    assert(irq_calls==1 && cursor.image.visible && cursor.image.x==254);
    assert(cursor.incoming.ready);
    pal_cursor16_check(&cursor);
    assert(cursor.image.visible && cursor.image.x==260);
    assert(!cursor.incoming.ready);
    pal_cursor16_unbind(&cursor);
    irq_target=0;
    /* All-transparent 32x32 cursor does no video I/O, even while moving. */
    shape(wire,32,32,0,0);
    memset(wire+12,0xff,128u);
    memset(wire+140,0,128u);
    assert(pal_cursor16_set(&cursor,wire,sizeof(wire)));
    pal_cursor16_bind(&cursor,&banked);
    read_before=reads; write_before=writes;
    pal_cursor16_post_move(&cursor,640-10,400-10);
    pal_cursor16_check(&cursor);
    assert(cursor.image.visible);
    pal_cursor16_post_move(&cursor,0,0);
    pal_cursor16_check(&cursor);
    pal_cursor16_unbind(&cursor);
    assert(reads==read_before && writes==write_before);
    puts("PASS: Win3.x CURSORSHAPE, atomic-mailbox model, nested compositor guard, banked save-under");
    return 0;
}
