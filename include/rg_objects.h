#ifndef RG_OBJECTS_H
#define RG_OBJECTS_H
/* Driver-independent INDEX8 physical pen/brush objects.
 * All fields are WORD-sized: GDI may pass these verbatim to the Win16 DDI.
 * OS-specific logical object layouts and far pointers stay in the adapter.
 */
#include "rg_palette.h"

#define RG_PEN_SIGNATURE 0x504Eu
#define RG_BRUSH_SIGNATURE 0x4252u
#define RG_PEN_SOLID 0u
#define RG_PEN_NULL 5u
#define RG_BRUSH_SOLID 0u
#define RG_BRUSH_HOLLOW 1u
#define RG_BRUSH_HATCHED 2u
#define RG_BRUSH_PATTERN 3u

typedef struct RG_Pen {
    unsigned short signature;
    unsigned short style;
    unsigned short color;
    unsigned short width;
} RG_Pen;

typedef struct RG_Brush {
    unsigned short signature;
    unsigned short style;
    unsigned short foreground;
    unsigned short background;
    unsigned short hatch;
    unsigned short phase_x;
    unsigned short phase_y;
} RG_Brush;

/* Logical colors: Win3.x RGB COLORREF or 0xFF00iiii index. Unsupported
 * styled/wide pens are reduced to nominal solid, 1px GDI pens; null
 * pens stay null. Device fonts and device bitmaps are not implemented. */
int rg_pen_realize(const RG_Palette *palette, unsigned style, int width,
                   rg_pixel logical_color, RG_Pen *out);
int rg_brush_realize(const RG_Palette *palette, unsigned style,
                     rg_pixel logical_foreground, unsigned physical_background,
                     unsigned hatch, unsigned phase_x, unsigned phase_y,
                     RG_Brush *out);
int rg_pen_valid(const RG_Pen *pen);
int rg_brush_valid(const RG_Brush *brush);
/* Return whether a hatch pixel is foreground, using absolute device coords.
 * All six Win3.x standard hatches repeat every 8 pixels. */
int rg_brush_hatch_foreground(const RG_Brush *brush, int x, int y);
#endif
