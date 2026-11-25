/* Test common INDEX8 palette rules separately from legacy Win16 FAR ABI. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "rg_palette.h"

#define W 20
#define H 7
static rg_u8 px[W * H];
static unsigned writes;
static void pixel_override(RG_Device *dev, int x, int y, rg_pixel v)
{
    ++writes;
    rg_software_pixel(dev,x,y,v);
}

int main(void)
{
    RG_Palette pal;
    RG_Color color, sample;
    RG_Device dev;
    unsigned short map[256];
    rg_pixel logical, physical, prev;
    unsigned i, j, translated;
    memset(&pal, 0, sizeof(pal));
    translated=42;
    assert(!rg_palette_translate(&pal,0,&translated) && translated==42);
    assert(!rg_palette_untranslate(&pal,0,&translated) && translated==42);
    rg_palette_init(&pal);
    for (i=0; i<256u; ++i) {
        assert(rg_palette_translate(&pal,i,&translated) && translated==i);
        assert(rg_palette_untranslate(&pal,i,&translated) && translated==i);
    }
    translated=42;
    assert(!rg_palette_translate(0,0,&translated) && translated==42);
    assert(!rg_palette_untranslate(0,0,&translated) && translated==42);
    assert(!rg_palette_translate(&pal,256u,&translated) && translated==42);
    assert(!rg_palette_untranslate(&pal,256u,&translated) && translated==42);
    assert(!rg_palette_translate(&pal,UINT_MAX,&translated) && translated==42);
    assert(!rg_palette_untranslate(&pal,UINT_MAX,&translated) && translated==42);
    assert(!rg_palette_translate(&pal,0,0));
    assert(!rg_palette_untranslate(&pal,0,0));
    assert(pal.initialized);
    assert(rg_palette_is_static(0) && rg_palette_is_static(9) &&
           rg_palette_is_static(246) && rg_palette_is_static(255));
    assert(!rg_palette_is_static(10) && !rg_palette_is_static(245));
    assert(rg_palette_get_entry(&pal,0,&color) && !color.red && !color.green && !color.blue);
    assert(rg_palette_get_entry(&pal,9,&color) && color.red==166 && color.green==202 && color.blue==240);
    assert(rg_palette_get_entry(&pal,246,&color) && color.red==255 && color.green==251 && color.blue==240);
    assert(rg_palette_get_entry(&pal,255,&color) && color.red==255 && color.green==255 && color.blue==255);
    sample.red=77; sample.green=88; sample.blue=99; sample.flags=0xFE;
    assert(!rg_palette_set_entry(&pal,0,sample));
    assert(!rg_palette_set_entry(&pal,255,sample));
    assert(!rg_palette_set_entry(&pal,256,sample));
    assert(rg_palette_set_entry(&pal,20,sample));
    assert(rg_palette_get_entry(&pal,20,&color) && color.red==77 && color.green==88 && color.blue==99 && color.flags==0);
    assert(!rg_palette_get_entry(&pal,256,&color));
    assert(!rg_palette_get_entry(&pal,20,0));
    /* ColorInfo physical->logical: reserved = RGB, dynamic = indexed. */
    assert(rg_palette_color_info(&pal,0,0,&logical,0) && logical==0);
    assert(rg_palette_color_info(&pal,255,0,&logical,0) && logical==0x00FFFFFFUL);
    assert(rg_palette_color_info(&pal,9,0,&logical,0) && logical==0x00F0CAA6UL);
    assert(rg_palette_color_info(&pal,20,0,&logical,0) && logical==0xFF000014UL);
    assert(rg_palette_color_info(&pal,0xFF000014UL,0,&logical,0) && logical==0xFF000014UL);
    assert(!rg_palette_color_info(&pal,256,0,&logical,0));
    /* RGB->index nearest of 20 reserved; test 16-bit intermediate overflow. */
    assert(rg_palette_color_info(&pal,0x00000000UL,1,&logical,&physical) && !logical && !physical);
    assert(rg_palette_color_info(&pal,0x00FFFFFFUL,1,&logical,&physical) && logical==0x00FFFFFFUL && physical==255);
    assert(rg_palette_color_info(&pal,0x00F0CAA6UL,1,&logical,&physical) && physical==9);
    assert(rg_palette_color_info(&pal,0xFF000020UL,1,&logical,&physical) && logical==0xFF000020UL && physical==32);
    prev=0x123UL;
    assert(!rg_palette_color_info(&pal,0xFF000100UL,1,&logical,&prev) && prev==0x123UL);
    assert(!rg_palette_color_info(&pal,0x01010203UL,1,&logical,&prev));
    for (i=0; i<256u; ++i) map[i]=(unsigned short)i;
    map[10]=100; map[11]=100; map[12]=200;
    assert(rg_palette_set_translation(&pal,map));
    assert(rg_palette_translate(&pal,10,&translated) && translated==100 && rg_palette_translate(&pal,11,&translated) && translated==100);
    assert(rg_palette_untranslate(&pal,100,&translated) && translated==10);
    assert(rg_palette_untranslate(&pal,200,&translated) && translated==12);
    assert(rg_palette_color_info(&pal,100,0,&logical,0) && logical==0xFF00000AUL);
    assert(rg_palette_color_info(&pal,0xFF00000BUL,1,&logical,&physical) && physical==100);
    map[12]=256;
    assert(!rg_palette_set_translation(&pal,map) && rg_palette_translate(&pal,12,&translated) && translated==200);
    map[12]=200; map[0]=1;
    assert(!rg_palette_set_translation(&pal,map) && rg_palette_translate(&pal,0,&translated) && translated==0);
    assert(rg_palette_set_translation(&pal,0));
    assert(pal.physical_to_logical[100]==100 && rg_palette_translate(&pal,12,&translated) && translated==12);
    /* Corrupt tables must not leak invalid indices or change outputs. */
    pal.logical_to_physical[20]=256u;
    pal.physical_to_logical[20]=256u;
    translated=42; logical=0x123UL; physical=0x456UL;
    assert(!rg_palette_translate(&pal,20,&translated) && translated==42);
    assert(!rg_palette_untranslate(&pal,20,&translated) && translated==42);
    assert(!rg_palette_color_info(&pal,0xFF000014UL,1,&logical,&physical));
    assert(logical==0x123UL && physical==0x456UL);
    assert(!rg_palette_color_info(&pal,20,0,&logical,0) && logical==0x123UL);
    assert(rg_palette_set_translation(&pal,0));
    /* UpdateColors must validate the whole map and rectangle before writes. */
    assert(rg_init(&dev,px,W,H,W,sizeof(px),RG_FMT_INDEX8));
    dev.ops.pixel=pixel_override;
    for (j=0; j<H; ++j) for (i=0; i<W; ++i) px[j*W+i]=(rg_u8)((i+j)%17);
    for (i=0; i<256u; ++i) map[i]=(unsigned short)i;
    map[7]=99; map[8]=100;
    assert(rg_palette_remap_rect(&dev,5,2,10,4,map));
    assert(px[2*W+5]==99 && px[2*W+6]==100);
    assert(px[2*W+4]==6); /* outside rectangle unchanged */
    assert(writes>0);
    writes=0;
    map[255]=256;
    assert(!rg_palette_remap_rect(&dev,5,2,10,4,map) && writes==0);
    map[255]=255;
    assert(!rg_palette_remap_rect(&dev,W-1,2,2,1,map) && writes==0);
    assert(!rg_palette_remap_rect(&dev,1,H,1,1,map) && writes==0);
    assert(!rg_palette_remap_rect(0,1,1,1,1,map));
    assert(rg_palette_remap_rect(&dev,W,H,0,0,map));
    puts("PASS: default Win3.x palette, ColorInfo, mapping/inverse, bounds, remap");
    return 0;
}
