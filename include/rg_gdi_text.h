#ifndef RG_GDI_TEXT_H
#define RG_GDI_TEXT_H
/* Shared C89 physical-color raster glyph operations for Win3.x and later.
 * Raster glyph bytes are column-major (one vertical byte per scanline for
 * each 8-pixel-wide column). OS adapters validate font offsets/segments.
 * No Windows headers or card-specific drawing paths are used here.
 */
#include "rg_gdi_output.h"

typedef rg_u8 (*RG_GlyphReadFn)(void *, rg_offset);
typedef struct RG_Glyph {
    unsigned width;
    unsigned height;
    rg_offset offset; /* byte offset within caller-owned font storage */
} RG_Glyph;
/* Clipping is half-open. Writes use ops.fill for rectangles and horizontal
 * runs; ops.pixel is the fallback. A custom pixel operation takes precedence
 * over the default software fill, but an explicit fill override wins.
 * Font data callbacks must remain valid for the complete glyph span.
 * GDI text always uses COPY, not DRAWMODE.Rop2. */
int rg_gdi_text_fill(RG_Device *dev, RG_Rect rect,
                     const RG_Rect *clip, unsigned color);
int rg_gdi_text_glyph(RG_Device *dev, int x, int y,
                      const RG_Rect *clip, const RG_Glyph *glyph,
                      RG_GlyphReadFn read_byte, void *read_context,
                      unsigned text_color);
#endif
