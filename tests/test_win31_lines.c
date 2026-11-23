/* v0027: GDI-required scanlines/polylines. Independent geometry/ROP checks,
 * including endpoint exclusion and Win16 signed-coordinate extremes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rg_gdi_output.h"
#include "rg_gdi_rop.h"

#define W 64u
#define H 32u
static rg_u8 frame[W*H];
static unsigned writes;
static void monitored_pixel(RG_Device *dev,int x,int y,rg_pixel physical)
{
    ++writes;
    rg_software_pixel(dev,x,y,physical);
}
static unsigned at(unsigned x,unsigned y) { return frame[y*W+x]; }
static void clear_frame(void) { memset(frame,0,sizeof(frame)); writes=0; }

int main(void)
{
    RG_Device dev;
    RG_Palette pal;
    RG_Pen pen, nullpen, badpen;
    RG_Brush solid, hatch, hollow, invalid;
    RG_Rect clip;
    unsigned i;
    assert(rg_init(&dev,frame,W,H,W,sizeof(frame),RG_FMT_INDEX8));
    dev.ops.pixel=monitored_pixel;
    rg_palette_init(&pal);
    assert(rg_pen_realize(&pal,RG_PEN_SOLID,1,0xFF000055UL,&pen));
    assert(rg_pen_realize(&pal,RG_PEN_NULL,1,0,&nullpen));
    assert(rg_brush_realize(&pal,RG_BRUSH_SOLID,0xFF000066UL,
                             0,0,0,0,&solid));
    assert(rg_brush_realize(&pal,RG_BRUSH_HATCHED,0xFF000077UL,
                             0x22,0,0,0,&hatch));
    assert(rg_brush_realize(&pal,RG_BRUSH_HOLLOW,0,
                             0,0,0,0,&hollow));
    clip.left=6; clip.top=2; clip.right=13; clip.bottom=18;
    clear_frame();
    assert(rg_gdi_output_scanline(&dev,4,1,16,&clip,&pen,&solid,13,2));
    assert(writes==7u && at(5,4)==0 && at(6,4)==0x66 &&
           at(12,4)==0x66 && at(13,4)==0);
    /* One GDI scanline call can contain several [left,right) pairs.
     * Core processes the spans independently without drawing across gaps. */
    clear_frame();
    assert(rg_gdi_output_scanline(&dev,4,2,5,0,&pen,&solid,13,2));
    assert(rg_gdi_output_scanline(&dev,4,9,12,0,&pen,&solid,13,2));
    assert(writes==6u && at(2,4)==0x66 && at(4,4)==0x66 &&
           at(5,4)==0 && at(8,4)==0 && at(9,4)==0x66 &&
           at(11,4)==0x66 && at(12,4)==0);
    clear_frame();
    assert(rg_gdi_output_scanline(&dev,4,1,16,&clip,&pen,0,13,2));
    assert(at(6,4)==0x55 && at(12,4)==0x55);
    clear_frame();
    assert(rg_gdi_output_scanline(&dev,8,0,16,0,&pen,&hatch,13,1));
    assert(at(0,8)==0x77 && at(1,8)==0x77 && at(8,8)==0x77);
    assert(rg_gdi_output_scanline(&dev,9,0,16,0,&pen,&hatch,13,1));
    assert(at(0,9)==0 && at(8,9)==0);
    assert(rg_gdi_output_scanline(&dev,9,0,16,0,&pen,&hatch,13,2));
    assert(at(0,9)==0x22 && at(8,9)==0x22);
    /* A supplied hollow brush suppresses the fallback pen. */
    clear_frame();
    assert(rg_gdi_output_scanline(&dev,4,0,20,0,&pen,&hollow,13,2));
    assert(writes==0);
    assert(rg_gdi_output_scanline(&dev,-1,0,20,0,&pen,&solid,13,2));
    assert(writes==0);
    assert(rg_gdi_output_scanline(&dev,4,4,4,0,&pen,&solid,13,2));
    assert(writes==0);
    /* Scanline uses a brush or, only when no brush, a pen. */
    for (i=1; i<=16; ++i) {
        clear_frame();
        frame[4*W+3]=0xA5;
        assert(rg_gdi_output_scanline(&dev,4,3,4,0,&pen,0,i,2));
        assert(at(3,4)==rg_gdi_rop2_byte(i,0x55,0xA5));
        clear_frame();
        frame[4*W+3]=0xA5;
        assert(rg_gdi_output_segment(&dev,3,4,4,4,0,&pen,i));
        assert(at(3,4)==rg_gdi_rop2_byte(i,0x55,0xA5));
        assert(at(4,4)==0);
    }
    clear_frame();
    assert(rg_gdi_output_segment(&dev,2,4,7,4,0,&pen,7));
    assert(rg_gdi_output_segment(&dev,7,4,7,9,0,&pen,7));
    assert(writes==10u && at(2,4)==0x55 && at(6,4)==0x55 &&
           at(7,4)==0x55 && at(7,8)==0x55 && at(7,9)==0);
    clear_frame();
    assert(rg_gdi_output_segment(&dev,7,4,2,4,0,&pen,13));
    assert(writes==5u && at(7,4)==0x55 && at(3,4)==0x55 &&
           at(2,4)==0);
    clear_frame();
    assert(rg_gdi_output_segment(&dev,2,2,9,9,0,&pen,13));
    assert(writes==7u && at(2,2)==0x55 && at(8,8)==0x55 &&
           at(9,9)==0);
    clear_frame();
    assert(rg_gdi_output_segment(&dev,2,2,4,9,0,&pen,13));
    assert(writes==7u && at(2,2)==0x55 && at(4,9)==0);
    clear_frame();
    assert(rg_gdi_output_segment(&dev,-10,4,20,4,&clip,&pen,13));
    assert(writes==7u && at(6,4)==0x55 && at(12,4)==0x55 &&
           at(13,4)==0);
    /* Win16 int may be 16-bit: displacement 65535 must not overflow. */
    clear_frame();
    assert(rg_gdi_output_segment(&dev,-32768,5,32767,5,0,&pen,13));
    assert(writes==W && at(0,5)==0x55 && at(W-1,5)==0x55);
    /* No-op for null pen or zero-length segment. */
    clear_frame();
    assert(rg_gdi_output_segment(&dev,7,4,7,4,0,&pen,7));
    assert(rg_gdi_output_segment(&dev,1,1,20,20,0,&nullpen,7));
    assert(writes==0);
    badpen=pen; badpen.signature=0;
    invalid=solid; invalid.hatch=8u;
    assert(!rg_gdi_output_scanline(&dev,4,5,2,0,&pen,&solid,13,2));
    assert(!rg_gdi_output_scanline(&dev,4,2,5,0,&pen,&invalid,13,2));
    assert(!rg_gdi_output_scanline(&dev,4,2,5,0,&pen,&solid,0,2));
    assert(!rg_gdi_output_scanline(&dev,4,2,5,0,&pen,&solid,13,3));
    assert(!rg_gdi_output_segment(&dev,0,0,10,0,0,&badpen,13));
    assert(!rg_gdi_output_segment(&dev,0,0,10,0,0,&pen,17));
    clip.left=8; clip.right=7;
    assert(!rg_gdi_output_scanline(&dev,4,2,5,&clip,&pen,&solid,13,2));
    assert(!rg_gdi_output_segment(&dev,0,0,10,0,&clip,&pen,13));
    assert(writes==0);
    puts("PASS: Win3.x scanline/polyline, ROP2, clipping, hatch, joint XOR, 16-bit extremes");
    return 0;
}
