/* v0026 host C89 regression: exact Win3.x logical object ABI, physical
 * palette colors, nominal pens, brushes, hatch phase, clipped ROP2 output. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_objects.h"
#include "rg_gdi_output.h"
#include "rg_gdi_rop.h"

#define W 24u
#define H 18u
static rg_u8 pixels[W*H];
static unsigned writes;
static void counted_pixel(RG_Device *dev, int x, int y, rg_pixel c)
{
    ++writes;
    rg_software_pixel(dev,x,y,c);
}
static rg_u8 at(unsigned x, unsigned y) { return pixels[y*W+x]; }

int main(void)
{
    RG_Palette palette;
    RG_Pen pen, nullpen, broken;
    RG_Brush solid, hollow, hatch, badbrush;
    RG_Device dev;
    RG_Rect r, clip;
    unsigned short table[256];
    unsigned i, rop, bg;
    assert(pal_object_layout_ok());
    rg_palette_init(&palette);
    for (i=0; i<256u; ++i) table[i] = (unsigned short)i;
    table[20]=111;
    assert(rg_palette_set_translation(&palette,table));
    assert(pal_object_background(&palette,42UL,&bg) && bg==42u);
    assert(pal_object_background(&palette,0xFF000014UL,&bg) && bg==111u);
    assert(!pal_object_background(&palette,0xFF000100UL,&bg));
    assert(!pal_object_background(&palette,256UL,&bg));
    assert(rg_pen_realize(&palette,4,99,0xFF000014UL,&pen));
    assert(rg_pen_valid(&pen) && pen.style==RG_PEN_SOLID &&
           pen.width==1u && pen.color==111u);
    assert(rg_pen_realize(&palette,RG_PEN_NULL,0,0xFFFFFFFFUL,&nullpen));
    assert(rg_pen_valid(&nullpen) && nullpen.style==RG_PEN_NULL);
    assert(!rg_pen_realize(&palette,6,1,0,&broken));
    assert(!rg_pen_realize(&palette,0,1,0xFF000100UL,&broken));
    assert(rg_brush_realize(&palette,RG_BRUSH_SOLID,0xFF000014UL,
                             0,0,0,0,&solid));
    assert(rg_brush_valid(&solid) && solid.foreground==111u);
    assert(rg_brush_realize(&palette,RG_BRUSH_HOLLOW,0xFFFFFFFFUL,
                             0,0,0,0,&hollow));
    assert(rg_brush_valid(&hollow) && hollow.style==RG_BRUSH_HOLLOW);
    assert(rg_brush_realize(&palette,RG_BRUSH_HATCHED,0xFF000014UL,
                             42,4,1,2,&hatch));
    assert(rg_brush_valid(&hatch) && hatch.foreground==111u &&
           hatch.background==42u && hatch.hatch==4u);
    assert(rg_brush_hatch_foreground(&hatch,7,6)); /* (7+1)%8==0 */
    assert(rg_brush_hatch_foreground(&hatch,0,6)); /* (6+2)%8==0 */
    assert(!rg_brush_hatch_foreground(&hatch,0,0));
    /* Independently exercise each of the six standard Win3.x hatches. */
    for (i=0; i<6u; ++i) {
        assert(rg_brush_realize(&palette,RG_BRUSH_HATCHED,
                0xFF000014UL,42,i,0,0,&hatch));
        assert(rg_brush_hatch_foreground(&hatch,0,0));
        if (i==0u) {
            assert(rg_brush_hatch_foreground(&hatch,4,0));
            assert(!rg_brush_hatch_foreground(&hatch,0,4));
        } else if (i==1u) {
            assert(rg_brush_hatch_foreground(&hatch,0,4));
            assert(!rg_brush_hatch_foreground(&hatch,4,0));
        } else if (i==2u) {
            assert(rg_brush_hatch_foreground(&hatch,1,7));
            assert(!rg_brush_hatch_foreground(&hatch,1,1));
        } else if (i==3u) {
            assert(rg_brush_hatch_foreground(&hatch,3,3));
            assert(!rg_brush_hatch_foreground(&hatch,1,7));
        } else if (i==4u) {
            assert(rg_brush_hatch_foreground(&hatch,4,0));
            assert(rg_brush_hatch_foreground(&hatch,0,4));
        } else {
            assert(rg_brush_hatch_foreground(&hatch,1,7));
            assert(rg_brush_hatch_foreground(&hatch,3,3));
        }
    }
    assert(rg_brush_realize(&palette,RG_BRUSH_HATCHED,
                             0xFF000014UL,42,4,1,2,&hatch));
    assert(!rg_brush_realize(&palette,RG_BRUSH_PATTERN,0,0,0,0,0,&badbrush));
    assert(!rg_brush_realize(&palette,RG_BRUSH_HATCHED,0,0,6,0,0,&badbrush));
    assert(!rg_brush_realize(&palette,RG_BRUSH_HATCHED,0,256,0,0,0,&badbrush));
    assert(!rg_brush_realize(&palette,RG_BRUSH_SOLID,0xFF000100UL,
                             0,0,0,0,&badbrush));
    assert(rg_init(&dev,pixels,W,H,W,sizeof(pixels),RG_FMT_INDEX8));
    dev.ops.pixel=counted_pixel;
    r.left=4; r.top=4; r.right=12; r.bottom=10;
    clip.left=7; clip.top=2; clip.right=20; clip.bottom=8;
    memset(pixels,3,sizeof(pixels)); writes=0;
    assert(rg_gdi_output_rectangle(&dev,r,&clip,&pen,&solid,13,2));
    assert(writes>0);
    assert(at(6,6)==3 && at(7,6)==111 && at(9,6)==111);
    assert(at(9,8)==3 && at(11,7)==111 && at(12,6)==3);
    /* Null/hollow draw nothing while still reporting success. */
    memset(pixels,3,sizeof(pixels)); writes=0;
    assert(rg_gdi_output_rectangle(&dev,r,0,&nullpen,&hollow,13,2));
    assert(writes==0 && at(4,4)==3);
    /* Six hatches: opaque gaps take physical background; transparent skips. */
    memset(pixels,3,sizeof(pixels)); writes=0;
    r.left=0; r.top=0; r.right=16; r.bottom=16;
    assert(rg_gdi_output_rectangle(&dev,r,0,0,&hatch,13,2));
    assert(at(7,6)==111 && at(2,2)==42);
    memset(pixels,3,sizeof(pixels)); writes=0;
    assert(rg_gdi_output_rectangle(&dev,r,0,0,&hatch,13,1));
    assert(at(7,6)==111 && at(2,2)==3);
    /* Border is not filled twice: XOR pen must not XOR brush first. */
    r.left=4; r.top=4; r.right=8; r.bottom=8;
    memset(pixels,0,sizeof(pixels)); writes=0;
    assert(rg_gdi_output_rectangle(&dev,r,0,&pen,&solid,7,2));
    assert(at(4,4)==111u && at(5,5)==111u);
    assert(writes==16u); /* 12 border + 4 interior; exactly one each */
    /* ROP2 results match shared Pixel exactly for all operations. */
    r.left=0; r.top=0; r.right=1; r.bottom=1;
    for (rop=1; rop<=16; ++rop) {
        pixels[0]=0xA5;
        assert(rg_gdi_output_rectangle(&dev,r,0,0,&solid,rop,2));
        assert(pixels[0]==rg_gdi_rop2_byte(rop,111u,0xA5));
    }
    writes=0; broken=pen; broken.signature=0;
    assert(!rg_gdi_output_rectangle(&dev,r,0,&broken,&solid,13,2));
    assert(writes==0);
    assert(!rg_gdi_output_rectangle(&dev,r,0,&pen,&solid,0,2));
    assert(!rg_gdi_output_rectangle(&dev,r,0,&pen,&solid,13,3));
    r.left=10; r.right=9;
    assert(!rg_gdi_output_rectangle(&dev,r,0,&pen,&solid,13,2));
    assert(writes==0);
    puts("PASS: Win3.x object wire layout, pens/brushes, 6 hatches, clipped ROP2 rectangle");
    return 0;
}
