/* v0040: Win3.x 2.x raster font decoding and shared column-major text.
 * This tests the actual layout decoder; Win16 FAR pointers are in palddi.c. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_font16.h"

static rg_u8 font_bytes[256];
static rg_u8 surface[32u*24u];
static rg_u8 video[256u*1024u];
static unsigned writes;
static rg_u8 fr(void *ctx,rg_offset offset)
{
    rg_u8 *bytes=(rg_u8 *)ctx;
    return bytes[offset];
}
static void pixel_count(RG_Device *dev,int x,int y,rg_pixel value)
{
    ++writes;
    rg_software_pixel(dev,x,y,value);
}
static void word_at(rg_u8 *b,unsigned off,unsigned value)
{
    b[off]=(rg_u8)value;
    b[off+1]=(rg_u8)(value>>8);
}
static void make_font(void)
{
    static const rg_u8 a[8]={0x18,0x24,0x42,0x7E,0x42,0x42,0x42,0};
    static const rg_u8 b[8]={0x7C,0x42,0x42,0x7C,0x42,0x42,0x7C,0};
    unsigned j;
    memset(font_bytes,0,sizeof(font_bytes));
    word_at(font_bytes,20,8); /* dfPixWidth */
    word_at(font_bytes,22,8); /* dfPixHeight */
    word_at(font_bytes,27,8); /* dfMaxWidth */
    font_bytes[29]='A'; font_bytes[30]='B';
    font_bytes[31]=0; /* default first char */
    font_bytes[32]=(rg_u8)('B'-'A');
    word_at(font_bytes,52,8); word_at(font_bytes,54,64);
    word_at(font_bytes,56,8); word_at(font_bytes,58,72);
    word_at(font_bytes,60,8); word_at(font_bytes,62,80); /* sentinel */
    for (j=0;j<8;++j) {font_bytes[64+j]=a[j];font_bytes[72+j]=b[j];}
}
static rg_u8 vr(void *ctx,rg_offset off) { (void)ctx; return video[off]; }
static void vw(void *ctx,rg_offset off,rg_u8 val)
{ (void)ctx; video[off]=val; }
int main(void)
{
    RG_Device dev,banked,mono;
    RG_VramOps io;
    RG_Glyph glyph;
    PAL_Font16 font;
    RG_Rect clip,rect;
    rg_u8 onebit[8];
    unsigned i;
    make_font();
    assert(pal_font16_open(&font,fr,font_bytes,128));
    assert(font.height==8 && font.max_width==8 && font.first=='A' &&
           font.last=='B' && font.default_char=='A');
    assert(pal_font16_glyph(&font,fr,font_bytes,'A',&glyph));
    assert(glyph.width==8 && glyph.height==8 && glyph.offset==64);
    assert(pal_font16_glyph(&font,fr,font_bytes,'Z',&glyph));
    assert(glyph.offset==64); /* missing character => default */
    assert(!pal_font16_open(&font,fr,font_bytes,56));
    font_bytes[0]=1; /* vector is forbidden */
    assert(!pal_font16_open(&font,fr,font_bytes,128));
    font_bytes[0]=0;
    assert(pal_font16_open(&font,fr,font_bytes,128));
    word_at(font_bytes,54,122); /* bounds crossing */
    assert(!pal_font16_glyph(&font,fr,font_bytes,'A',&glyph));
    word_at(font_bytes,54,64);
    assert(pal_font16_glyph(&font,fr,font_bytes,'A',&glyph));

    memset(surface,7,sizeof(surface));
    assert(rg_init(&dev,surface,32,24,32,32UL*24UL,RG_FMT_INDEX8));
    dev.ops.pixel=pixel_count;
    clip.left=0;clip.top=0;clip.right=32;clip.bottom=24;
    writes=0;
    assert(rg_gdi_text_glyph(&dev,4,3,&clip,&glyph,fr,font_bytes,12));
    assert(surface[3u*32u+4u+3u]==12); /* top row 0x18 only at cols 3,4 */
    assert(surface[3u*32u+4u+4u]==12);
    assert(surface[6u*32u+4u+1u]==12); /* middle full line */
    assert(surface[5u*32u+4u]==7);
    assert(writes==18u); /* bits set in A */
    clip.left=7;clip.top=5;clip.right=10;clip.bottom=7;
    writes=0;
    assert(rg_gdi_text_glyph(&dev,4,3,&clip,&glyph,fr,font_bytes,23));
    assert(writes>0u && writes<18u);
    rect.left=4;rect.top=3;rect.right=12;rect.bottom=11;
    writes=0;
    assert(rg_gdi_text_fill(&dev,rect,&clip,15));
    assert(writes==6u);
    assert(surface[5u*32u+7u]==15);
    assert(!rg_gdi_text_glyph(&dev,4,3,&clip,&glyph,0,font_bytes,5));
    assert(!rg_gdi_text_fill(&dev,rect,&clip,300));

    memset(onebit,0,sizeof(onebit));
    assert(rg_init(&mono,onebit,8,8,1,8,RG_FMT_INDEX1));
    rect.left=0;rect.top=0;rect.right=8;rect.bottom=8;
    assert(rg_gdi_text_fill(&mono,rect,0,1));
    assert(rg_gdi_text_glyph(&mono,0,0,0,&glyph,fr,font_bytes,0));
    assert(onebit[0]==0xE7); /* zero glyph pixel only overwrites set bits */
    /* Win3.x text foreground can equal 0 on monochrome: glyph writes zero. */
    assert(onebit[3]==0x81);
    assert(!rg_gdi_text_glyph(&mono,0,0,0,&glyph,fr,font_bytes,2));

    /* Exact 64 KiB bank boundary on a 640x400 8bpp surface. The common
     * glyph renderer relies on the same callback-based byte routing as
     * ET4000AX banked VRAM; it has no window-specific copies. */
    memset(video,0,sizeof(video));
    io.read_byte=vr; io.write_byte=vw;
    assert(rg_init_vram(&banked,640,400,640,256u*1024UL,
                        RG_FMT_INDEX8,io,0));
    assert(rg_gdi_text_glyph(&banked,252,102,0,&glyph,fr,font_bytes,88));
    assert(video[102UL*640UL+255UL]==88);
    assert(video[102UL*640UL+256UL]==88);
    assert(video[102UL*640UL+259UL]==0);
    for(i=0;i<8u;++i) assert(video[102UL*640UL+i]==0);
    puts("PASS: Win3.x standard FONTINFO, column-major glyphs, clipping, mono and banked screen");
    return 0;
}
