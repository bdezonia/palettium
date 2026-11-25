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
#include "pal_font16.h"
#include "driver_syntax.h"

static unsigned char font_area[256];
void *paldiag_test_mk_fp(unsigned selector,unsigned offset)
{
    (void)selector;
    assert(offset < sizeof(font_area));
    return font_area+offset;
}
PAL_U32 pal_ddi_far_token(PAL_U16 seg, PAL_U16 off)
{ (void)seg; (void)off; return 0UL; }
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
int pal_ddi_memory_bitmap_ok(const PAL_BITMAP16 *bitmap)
{ (void)bitmap; return 0; }
int pal_ddi_bitmap_device(RG_Device *device,const PAL_BITMAP16 *bitmap,
                          RG_VramOps ops,void *context)
{ (void)device;(void)bitmap;(void)ops;(void)context;return 0; }

/* Simulate an offset only for a selected host pointer. This tests
 * actual guard branches, not protected-mode selector access. */
static const void *offset_pointer;
static unsigned simulated_offset;
static unsigned test_fp_off(const void *p)
{ return p == offset_pointer ? simulated_offset : 0u; }
#undef FP_OFF
#define FP_OFF(pointer) test_fp_off(pointer)

#if defined(PAL_WIN31_BOOTTRACE)
void pal_boottrace(const char *event) { (void)event; }
void pal_boottrace_hex(const char *event,unsigned value)
{ (void)event; (void)value; }
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
    assert(FastBorder(0,0u,0u,0UL,0,0,0,0)!=0UL);
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
    /* No destination/session: fail closed, never touch memory. */
    pal_active_host=0;
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0,0,opaque,0);
    assert(ext==0x80000000UL);
    puts("PASS: actual Win16 text wrappers plus required DDK bootstrap exports, font widths, cursor");
    return 0;
}
