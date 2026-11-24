#include "pal_font16.h"

static unsigned font_word(RG_GlyphReadFn rd,void *ctx,rg_offset off)
{
    return (unsigned)rd(ctx,off) | ((unsigned)rd(ctx,off+1UL)<<8);
}
int pal_font16_open(PAL_Font16 *out, RG_GlyphReadFn rd,
                    void *ctx, rg_offset cap)
{
    unsigned type, height, width, maximum, first, last, fallback;
    rg_offset entries;
    if (!out || !rd || cap<PAL_FONT16_HEADER_BYTES) return 0;
    type=font_word(rd,ctx,0);
    width=font_word(rd,ctx,20);
    height=font_word(rd,ctx,22);
    maximum=font_word(rd,ctx,27);
    first=rd(ctx,29); last=rd(ctx,30); fallback=rd(ctx,31);
    /* Only GDI standard raster fonts: vector or external-addressed
     * glyphs, 3.x big-font formats and unusual oversize fonts need a
     * separate decoder. Never invent a fallback glyph's location. */
    if ((type & 0x0005u)!=0u || first>last || !height || height>128u ||
        !maximum || maximum>128u || width>128u ||
        (width && width>maximum) ||
        fallback>last-first) return 0;
    entries=(rg_offset)(last-first+2u)*4UL;
    if (entries>cap-PAL_FONT16_CHARTABLE_OFFSET) return 0;
    out->first=first; out->last=last;
    out->default_char=first+fallback;
    out->break_char=first+(unsigned)rd(ctx,32);
    out->height=height; out->max_width=maximum;
    out->fixed_width=width; out->segment_bytes=cap;
    return 1;
}
int pal_font16_glyph(const PAL_Font16 *font,RG_GlyphReadFn rd,void *ctx,
                     unsigned code, RG_Glyph *out)
{
    rg_offset pos,off,bytes;
    unsigned width;
    if (!font || !rd || !out || !font->height ||
        !font->segment_bytes) return 0;
    if (code<font->first || code>font->last) code=font->default_char;
    pos=PAL_FONT16_CHARTABLE_OFFSET+(rg_offset)(code-font->first)*4UL;
    if (pos+4UL>font->segment_bytes) return 0;
    width=font_word(rd,ctx,pos);
    off=(rg_offset)font_word(rd,ctx,pos+2UL);
    if (!width || width>font->max_width ||
        (font->fixed_width && width!=font->fixed_width)) return 0;
    bytes=(rg_offset)((width+7u)/8u)*(rg_offset)font->height;
    if (off< PAL_FONT16_CHARTABLE_OFFSET+
          (rg_offset)(font->last-font->first+2u)*4UL ||
        off>font->segment_bytes || bytes>font->segment_bytes-off) return 0;
    out->width=width; out->height=font->height; out->offset=off;
    return 1;
}
