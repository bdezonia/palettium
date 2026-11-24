#ifndef PAL_FONT16_H
#define PAL_FONT16_H
/* Win3.x GDI standard (non-big) raster FONTINFO decoder. The in-memory
 * GDI structure starts at dfType (NOT .FNT's dfVersion/copyright header).
 * Standard 2.x FONTINFO has a 51-byte header, then 4-byte glyph entries.
 * GDI must not advertise RC_BIGFONT until 3.x layouts are supported.
 * The caller provides a verified bounded byte reader; no FAR casts here.
 */
#include "rg_gdi_text.h"
#define PAL_FONT16_HEADER_BYTES 51u
#define PAL_FONT16_CHARTABLE_OFFSET 51u

typedef struct PAL_Font16 {
    unsigned first, last, default_char, break_char;
    unsigned height, max_width, fixed_width;
    rg_offset segment_bytes; /* remaining bytes through segment boundary */
} PAL_Font16;

/* read_byte must be safe on [0, segment_bytes). Standard raster only. */
int pal_font16_open(PAL_Font16 *out, RG_GlyphReadFn read_byte,
                    void *context, rg_offset segment_bytes);
int pal_font16_glyph(const PAL_Font16 *font,
                     RG_GlyphReadFn read_byte, void *context,
                     unsigned code, RG_Glyph *out);
#endif
