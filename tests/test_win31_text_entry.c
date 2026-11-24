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
int pal_ddi_memory_bitmap_ok(const PAL_BITMAP16 *bitmap)
{ (void)bitmap; return 0; }
int pal_ddi_bitmap_device(RG_Device *device,const PAL_BITMAP16 *bitmap,
                          RG_VramOps ops,void *context)
{ (void)device;(void)bitmap;(void)ops;(void)context;return 0; }

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
    ww(51,8);ww(53,63);ww(55,8);ww(57,71);ww(59,8);ww(61,79);
    for(i=0;i<8;++i) {font_area[63+i]=a[i];font_area[71+i]=b[i];}
}
int main(void)
{
    short mode[12];
    short clip[4],opaque[4],widths[2];
    DWORD ext;
    unsigned i;
    unsigned char str[2]={'A','B'};
    make_font();
    memset(&host,0,sizeof(host));
    assert(rg_init(&host.device,pixels,32,24,32,32UL*24UL,RG_FMT_INDEX8));
    host.running=1;
    host.ddi.screen_token=0UL;
    pal_active_host=&host;
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
    /* No destination/session: fail closed, never touch memory. */
    pal_active_host=0;
    ext=ExtTextOut((LPVOID)&host.pdevice,0,0,clip,(LPSTR)str,2,
         (LPVOID)font_area,(LPVOID)mode,0,0,opaque,0);
    assert(ext==0x80000000UL);
    puts("PASS: actual Win16 StrBlt/ExtTextOut wrappers, measure, opaque, clip, invalid host");
    return 0;
}
