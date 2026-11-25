/* Host tests compile the ACTUAL Win16 text entry points with strictly
 * synthetic selector APIs. The compiler's FAR ABI is NOT emulated here;
 * actual Win16 validation still needs Open Watcom and the Windows VM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rgfx.h"
#include "pal_gdiinfo.h"
#include "pal_screen_ddi.h"
#include "pal_screen_host.h"
#include "pal_bitmap_device.h"
#include "rg_gdi_text.h"
#include "rg_gdi_blit.h"
#include "pal_font16.h"
#include "driver_syntax.h"

static unsigned char font_area[256];
static unsigned char bitmap_source[512], bitmap_dest[512];
static PAL_BITMAP16 source_header, dest_header;
void *paldiag_test_mk_fp(unsigned selector,unsigned offset)
{
    if (selector==0x30u || selector==0x40u) {
        assert(offset<sizeof(bitmap_source));
        return (selector==0x30u?bitmap_source:bitmap_dest)+offset;
    }
    assert(offset < sizeof(font_area));
    return font_area+offset;
}
PAL_U32 pal_ddi_far_token(PAL_U16 seg, PAL_U16 off)
{ return ((PAL_U32)seg<<16)|off; }
int pal_screen_host_output_ready(PAL_ScreenHost *host,PAL_U32 token)
{ return host && host->running && token==host->ddi.screen_token; }
int pal_screen_host_pixel(PAL_ScreenHost *h,PAL_U32 token,
                          int x,int y,PAL_U32 color,unsigned rop2,
                          PAL_U32 *result)
{
    rg_pixel p=0;
    if (!h || !h->running || token!=h->ddi.screen_token || !result ||
        !rg_get_pixel(&h->device,x,y,&p)) return 0;
    if (rop2) {
        if (rop2!=13u || !rg_put_pixel(&h->device,x,y,color)) return 0;
        p=color;
    }
    *result=(PAL_U32)p;
    return 1;
}
int pal_screen_host_bitblt(PAL_ScreenHost *h,PAL_U32 dt,int dx,int dy,
    PAL_U32 st,int sx,int sy,int width,int height,unsigned long rop)
{
    if (!h || !h->running || dt!=h->ddi.screen_token || st!=dt) return 0;
    return rg_gdi_blit_rop3(&h->device,&h->device,dx,dy,sx,sy,width,height,rop);
}

/* Simulate an offset only for a selected host pointer. This tests
 * actual guard branches, not protected-mode selector access. */
static const void *offset_pointer;
static unsigned simulated_offset;
static unsigned test_fp_off(const void *p)
{ return p == offset_pointer ? simulated_offset : 0u; }
#undef FP_OFF
#define FP_OFF(pointer) test_fp_off(pointer)
#undef FP_SEG
#define FP_SEG(pointer) ((pointer)==&source_header?0x10u:(pointer)==&dest_header?0x20u:0u)

#if defined(PAL_WIN31_BOOTTRACE)
static unsigned trace_pixels, trace_nonzero, trace_hash;
static unsigned source_pixels, source_nonzero, source_hash;
static int trace_source;
void pal_boottrace(const char *event) {
    if (!strcmp(event,"BMP SOURCE")) trace_source=1;
    if (!strcmp(event,"BMP DEST AFTER")) trace_source=0;
}
void pal_boottrace_hex(const char *event,unsigned value)
{
    if (!strcmp(event,"BMP SAMPLE PIXELS")) {
        if (trace_source) source_pixels=value; else trace_pixels=value;
    } else if (!strcmp(event,"BMP NONZERO")) {
        if (trace_source) source_nonzero=value; else trace_nonzero=value;
    } else if (!strcmp(event,"BMP HASH")) {
        if (trace_source) source_hash=value; else trace_hash=value;
    }
}
#endif

/* Include the real Win16 implementation to exercise exported wrappers.
 * Unused Win16 DDI functions are dropped via --gc-sections at link time. */
#include "../windows/win31/palddi.c"

static unsigned char pixels[32u*24u];
static PAL_ScreenHost host;
static void ww(unsigned n,unsigned x)
{ font_area[n]=(unsigned char)x; font_area[n+1]=(unsigned char)(x>>8); }
static void make_font(void)
{
    static const unsigned char a[8]={0x18,0x24,0x42,0x7E,0x42,0x42,0x42,0};
    static const unsigned char b[8]={0x7C,0x42,0x42,0x7C,0x42,0x42,0x7C,0};
    unsigned i;
    memset(font_area,0,sizeof(font_area));
    ww(20,8); ww(22,8); ww(27,8);
    font_area[29]='A';font_area[30]='B';font_area[31]=0;
    ww(52,8);ww(54,64);ww(56,8);ww(58,72);ww(60,8);ww(62,80);
    for(i=0;i<8;++i) {font_area[64+i]=a[i];font_area[72+i]=b[i];}
}
int main(void)
{
    unsigned char cursor_shape[76];
    WORD mouse_info[2];
    RG_Brush solid;
    short mode[12];
    short clip[4],opaque[4],widths[2];
    DWORD ext;
    WORD glyph_widths[3]={1000u,1000u,1000u};
    WORD query_escape=8u;
    WORD palette_table[256];
    unsigned i;
    unsigned char str[2]={'A','B'};
    make_font();
    assert(pal_far_range(0u,65536UL));
    assert(!pal_far_range(0u,65537UL));
    assert(pal_far_range(65535u,1UL));
    assert(!pal_far_range(65535u,2UL));
    assert(!pal_far_range(1u,0xffffffffUL));
    /* Exercise v0045 DDK-required exports using the real Win16 source. */
    assert(Control(0,8u,&query_escape,0)==1u);
    query_escape=9u;
    assert(Control(0,8u,&query_escape,0)==0u);
    assert(Control(0,9u,&query_escape,0)==0u);
    assert(EnumDFonts(0,0,0,0)==1u);
    assert(EnumObj(0,1u,0,0)==1u);
    assert(DeviceBitmap(0,0u,0,0)==0u);
    assert(SetAttribute(0,0u,0u,0u)==0u);
    assert(FastBorder(0,0u,0u,0UL,0,0,0,0)==0UL);
    assert(GetCharWidth(0,glyph_widths,'A','C',font_area,0,0)==1u);
    assert(glyph_widths[0]==8u && glyph_widths[1]==8u && glyph_widths[2]==8u);
    glyph_widths[0]=1000u;
    assert(GetCharWidth(0,glyph_widths,'C','A',font_area,0,0)==0u);
    assert(glyph_widths[0]==1000u);
    assert(GetCharWidth(0,glyph_widths,'A','A',0,0,0)==0u);
    memset(&host,0,sizeof(host));
    assert(rg_init(&host.device,pixels,32,24,32,32UL*24UL,RG_FMT_INDEX8));
    host.running=1;
    host.ddi.screen_token=0UL;
    pal_active_host=&host;
    rg_palette_init(&host.palette);
    for (i=0;i<256u;++i) palette_table[i]=0xffffu;
    offset_pointer=palette_table;
    simulated_offset=65025u;
    GetPalTrans(palette_table);
    for (i=0;i<256u;++i) assert(palette_table[i]==0xffffu);
    simulated_offset=65024u; /* exactly fits 512 bytes */
    GetPalTrans(palette_table);
    for (i=0;i<256u;++i) assert(palette_table[i]==i);
    offset_pointer=0;
    memset(mode,0,sizeof(mode));
    mode[1]=1; /* transparent */
    mode[2]=2; /* bg physical index */
    mode[4]=9; /* foreground */
    clip[0]=0;clip[1]=0;clip[2]=32;clip[3]=24;
    opaque[0]=0;opaque[1]=0;opaque[2]=16;opaque[3]=8;
    memset(pixels,2,sizeof(pixels));
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0,0,0,0);
    assert(ext==0x00080010UL);
    assert(pixels[0u*32u+3u]==9);
    assert(pixels[0u*32u+4u]==9);
    assert(pixels[0u*32u+0u]==2);
    ext=StrBlt((LPVOID)&host.pdevice,0,10,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0);
    assert(ext==0x00080010UL);
    assert(pixels[10u*32u+3u]==9);
    for(i=0;i<32u*24u;++i) pixels[i]=5;
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,-2,
         (LPVOID)font_area,(LPVOID)mode,0,0,0,0);
    assert(ext==0x00080010UL);
    for(i=0;i<32u*24u;++i) assert(pixels[i]==5); /* measure only */
    widths[0]=4;widths[1]=6;
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,-2,
         (LPVOID)font_area,(LPVOID)mode,0,widths,0,0);
    assert(ext==0x0008000CUL); /* overlapping glyphs reach x=12 */
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,0,
         0,(LPVOID)mode,0,0,opaque,2);
    assert(ext==1UL && pixels[0]==2 && pixels[8u*32u]==5);
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0,0,opaque,6);
    assert(ext==0x00080010UL);
    assert(pixels[3]==9 && pixels[0]==2);
    /* Exercise the ACTUAL cursor exports against the ACTUAL text wrapper:
     * mouse movement cannot write video memory until CheckCursor runs,
     * and drawing behind cursor must update saved-under correctly. */
    memset(cursor_shape,0,sizeof(cursor_shape));
    cursor_shape[4]=8;cursor_shape[6]=8;cursor_shape[8]=4;
    for (i=0;i<32u;++i) cursor_shape[12u+i]=0xffu;
    cursor_shape[12]=0x7fu; /* top-left cursor pixel AND=0 */
    assert(Inquire(mouse_info)==4u);
    assert(mouse_info[0]==1u && mouse_info[1]==2u);
    memset(pixels,5,sizeof(pixels));
    pal_cursor16_bind(&pal_win16_cursor,&host.device);
    cursor_shape[10]=1;cursor_shape[11]=1;
    SetCursor(cursor_shape);
    assert(pixels[0]==0u); /* shape is displayed immediately at current position */
#if defined(PAL_WIN31_BOOTTRACE)
    assert(pal_cursor_mask_mismatches==0u);
    assert(pal_cursor_raw_counts[0]==1u && pal_cursor_raw_counts[1]==0u);
    assert(pal_cursor_raw_counts[2]==63u && pal_cursor_raw_counts[3]==0u);
    for (i=0;i<4u;++i)
        assert(pal_cursor_raw_counts[i]==pal_cursor_decoded_counts[i]);
#endif
    offset_pointer=cursor_shape;
    simulated_offset=65461u; /* only 75 bytes remain: reject without changing shape */
    SetCursor(cursor_shape);
    assert(pixels[0]==0u && pal_win16_cursor.wants_shape);
    simulated_offset=65460u; /* 76 bytes: exact fit */
    SetCursor(cursor_shape);
    assert(pixels[0]==0u);
    offset_pointer=0;
    MoveCursor(3u,0u);
    assert(pixels[3]==5u); /* deferred, no mouse IRQ hardware write */
    CheckCursor();
    assert(pixels[3]==0u);
    assert(Pixel((LPVOID)&host.pdevice,3,0,0UL,0)==5UL);
    assert(pixels[3]==0u); /* read sees unobscured screen */
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0,0,0,0);
    assert(ext==0x00080010UL && pixels[3]==0u);
    assert(Pixel((LPVOID)&host.pdevice,3,0,7UL,(LPVOID)(mode))==
           0x80000000UL); /* unsupported ROP2 must not write */
    mode[0]=13; /* R2_COPYPEN */
    assert(Pixel((LPVOID)&host.pdevice,3,0,7UL,(LPVOID)(mode))==7UL);
    assert(pixels[3]==0u);
    SetCursor(0);
    assert(pixels[3]==7u); /* new underlying Pixel wins after restore */
    pal_cursor16_unbind(&pal_win16_cursor);
    /* Exercise native bitmap marshaling with two distinct selectors,
     * nonzero bit offsets, real bitmap validators and actual BitBlt.
     * This does NOT emulate Watcom FAR pointers or protected-mode ABI. */
    memset(&source_header,0,sizeof(source_header));
    source_header.bmWidth=32; source_header.bmHeight=64;
    source_header.bmWidthBytes=4; source_header.bmPlanes=1;
    source_header.bmBitsPixel=1; source_header.bmWidthPlanes=256;
    source_header.bmBits=0x00300010UL;
    dest_header=source_header;
    dest_header.bmBits=0x0040000cUL;
    memset(bitmap_source,0,sizeof(bitmap_source));
    memset(bitmap_dest,0xA5,sizeof(bitmap_dest));
    memset(bitmap_source+16,0xff,128);
    bitmap_source[16]=0x7f; /* black pixel at (0,0), rest transparent */
    bitmap_source[16+128]=0x40; /* invert the second pixel */
    assert(BitBlt(&dest_header,0,0,&source_header,0,0,32,64,
                  0x00CC0020UL,0,0));
    assert(!memcmp(bitmap_source+16,bitmap_dest+12,256));
    assert(bitmap_dest[11]==0xA5 && bitmap_dest[268]==0xA5);
#if defined(PAL_WIN31_BOOTTRACE)
    assert(source_pixels==2048u && trace_pixels==2048u);
    assert(source_nonzero==1024u && trace_nonzero==source_nonzero);
    assert(source_hash==trace_hash);
#endif
    /* Cursor mask is generated by actual bitmap transfer, not a second
     * handcrafted independent fixture. */
    memset(bitmap_dest,0,12);
    bitmap_dest[4]=32; bitmap_dest[6]=32; bitmap_dest[8]=4;
    bitmap_dest[10]=1; bitmap_dest[11]=1;
    memset(pixels,0x55,sizeof(pixels));
    pal_cursor16_bind(&pal_win16_cursor,&host.device);
    MoveCursor(0,0); CheckCursor();
    SetCursor(bitmap_dest);
    assert(pixels[0]==0 && pixels[1]==0xaa && pixels[2]==0x55);
    SetCursor(0);
    assert(pixels[0]==0x55 && pixels[1]==0x55);
    pal_cursor16_unbind(&pal_win16_cursor);
    assert(BitBlt(&dest_header,0,0,0,0,0,32,64,0x00550009UL,0,0));
    for(i=0;i<256;++i)
        assert((unsigned)bitmap_dest[12+i]+bitmap_source[16+i]==255u);
    assert(BitBlt(&dest_header,0,0,0,0,0,32,64,0x00550009UL,0,0));
    assert(!memcmp(bitmap_dest+12,bitmap_source+16,256));
    /* A typical moving-window outline: two PATINVERTs cancel exactly. */
    assert(rg_brush_realize(&host.palette,RG_BRUSH_SOLID,0x00FFFFFFUL,
                            0,0,0,0,&solid));
    memset(pixels,0x55,sizeof(pixels));
    assert(BitBlt(&host.pdevice,3,2,0,0,0,9,1,0x005A0049UL,&solid,0));
    for(i=0;i<sizeof(pixels);++i)
        assert(pixels[i]==(i>=67u && i<76u?0xaau:0x55u));
    assert(BitBlt(&host.pdevice,3,2,0,0,0,9,1,0x005A0049UL,&solid,0));
    for(i=0;i<sizeof(pixels);++i) assert(pixels[i]==0x55u);
    /* No destination/session: fail closed, never touch memory. */
    pal_active_host=0;
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0,0,opaque,0);
    assert(ext==0x80000000UL);
    puts("PASS: actual Win16 text wrappers plus required DDK bootstrap exports, font widths, cursor, memory bitmap transfers and outline inversion");
    return 0;
}
