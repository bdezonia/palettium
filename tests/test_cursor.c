#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rg_cursor.h"
#define W 40u
#define H 32u
static rg_u8 framebuffer[W*H];
static rg_u8 video[256u*1024u];
static rg_u8 vr(void *ctx,rg_offset off){(void)ctx;return video[off];}
static void vw(void *ctx,rg_offset off,rg_u8 value)
{(void)ctx;video[off]=value;}
int main(void)
{
    static RG_Cursor c; /* avoid a 4K stack frame on Win16 */
    RG_Device d,banked;
    RG_VramOps io;
    rg_u8 andmask[8],xormask[8];
    unsigned i;
    rg_cursor_init(&c);
    assert(rg_init(&d,framebuffer,W,H,W,W*H,RG_FMT_INDEX8));
    memset(framebuffer,0x55,sizeof(framebuffer));
    memset(andmask,0xFF,sizeof(andmask));
    memset(xormask,0,sizeof(xormask));
    /* Four outcomes: black, white, unchanged, inverted. */
    andmask[0]=0x30; xormask[0]=0x50;
    assert(!rg_cursor_show(&c,&d,6,5));
    assert(!rg_cursor_shape(&c,andmask,xormask,0,8,0,0));
    assert(rg_cursor_shape(&c,andmask,xormask,8,8,0,0));
    assert(rg_cursor_show(&c,&d,6,5));
    assert(framebuffer[5*W+6]==0); /* AND=0 XOR=0 */
    assert(framebuffer[5*W+7]==255); /* AND=0 XOR=1 */
    assert(framebuffer[5*W+8]==0x55); /* AND=1 XOR=0 */
    assert(framebuffer[5*W+9]==0xAA); /* AND=1 XOR=1 */
    assert(rg_cursor_move(&c,9,5));
    assert(framebuffer[5*W+6]==0x55); /* restored after moving */
    assert(framebuffer[5*W+9]==0);
    rg_cursor_hide(&c);
    for (i=0;i<W*H;++i) assert(framebuffer[i]==0x55);
    assert(rg_cursor_shape(&c,andmask,xormask,8,8,2,3));
    assert(rg_cursor_show(&c,&d,0,0)); /* negative hot-spot origin clipped */
    rg_cursor_hide(&c);
    for (i=0;i<W*H;++i) assert(framebuffer[i]==0x55);
    assert(rg_cursor_shape(&c,andmask,xormask,8,8,-1,2));
    assert(rg_cursor_show(&c,&d,0,0));
    rg_cursor_hide(&c);
    for (i=0;i<W*H;++i) assert(framebuffer[i]==0x55);

    memset(video,0x55,sizeof(video));
    io.read_byte=vr; io.write_byte=vw;
    assert(rg_init_vram(&banked,640,400,640,256u*1024UL,
                        RG_FMT_INDEX8,io,0));
    assert(rg_cursor_shape(&c,andmask,xormask,8,8,0,0));
    assert(rg_cursor_show(&c,&banked,254,102));
    assert(video[102UL*640UL+254UL]==0);
    assert(video[102UL*640UL+255UL]==255);
    assert(video[102UL*640UL+256UL]==0x55);
    assert(video[102UL*640UL+257UL]==0xAA);
    rg_cursor_hide(&c);
    for (i=250;i<265u;++i)
        assert(video[102UL*640UL+(rg_offset)i]==0x55);
    puts("PASS: shared software cursor AND/XOR, save-under, move, clip, banked restore");
    return 0;
}
