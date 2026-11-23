#include "rg_objects.h"

static int rg_object_color(const RG_Palette *palette, rg_pixel logical,
                           unsigned short *physical)
{
    rg_pixel effective, index;
    if (!rg_palette_color_info(palette, logical, 1, &effective, &index) ||
        index >= 256UL) return 0;
    *physical = (unsigned short)index;
    return 1;
}

int rg_pen_realize(const RG_Palette *palette, unsigned style, int width,
                   rg_pixel logical_color, RG_Pen *out)
{
    RG_Pen candidate;
    if (!palette || !out || style > 5u) return 0;
    candidate.signature = RG_PEN_SIGNATURE;
    candidate.style = style == RG_PEN_NULL ? RG_PEN_NULL : RG_PEN_SOLID;
    candidate.width = 1u;
    candidate.color = 0u;
    (void)width; /* GDI synthesizes wide/styled lines using a nominal pen. */
    if (candidate.style != RG_PEN_NULL &&
        !rg_object_color(palette, logical_color, &candidate.color)) return 0;
    *out = candidate;
    return 1;
}

int rg_brush_realize(const RG_Palette *palette, unsigned style,
                     rg_pixel logical_foreground, unsigned physical_background,
                     unsigned hatch, unsigned phase_x, unsigned phase_y,
                     RG_Brush *out)
{
    RG_Brush candidate;
    if (!palette || !out || style > RG_BRUSH_HATCHED ||
        (style == RG_BRUSH_HATCHED && (hatch > 5u ||
                                         physical_background >= 256u))) return 0;
    candidate.signature = RG_BRUSH_SIGNATURE;
    candidate.style = (unsigned short)style;
    candidate.foreground = 0u;
    candidate.background = 0u;
    candidate.hatch = 0u;
    candidate.phase_x = (unsigned short)(phase_x & 7u);
    candidate.phase_y = (unsigned short)(phase_y & 7u);
    if (style != RG_BRUSH_HOLLOW &&
        !rg_object_color(palette, logical_foreground, &candidate.foreground)) return 0;
    if (style == RG_BRUSH_HATCHED) {
        candidate.background = (unsigned short)physical_background;
        candidate.hatch = (unsigned short)hatch;
    }
    *out = candidate;
    return 1;
}

int rg_pen_valid(const RG_Pen *pen)
{
    return pen && pen->signature == RG_PEN_SIGNATURE &&
           (pen->style == RG_PEN_SOLID || pen->style == RG_PEN_NULL) &&
           pen->width == 1u && pen->color < 256u;
}

int rg_brush_valid(const RG_Brush *brush)
{
    return brush && brush->signature == RG_BRUSH_SIGNATURE &&
           brush->style <= RG_BRUSH_HATCHED &&
           brush->foreground < 256u && brush->background < 256u &&
           brush->hatch <= 5u && brush->phase_x < 8u &&
           brush->phase_y < 8u;
}

int rg_brush_hatch_foreground(const RG_Brush *brush, int x, int y)
{
    unsigned xx, yy;
    if (!rg_brush_valid(brush) || brush->style != RG_BRUSH_HATCHED ||
        x < 0 || y < 0) return 0;
    xx = ((unsigned)x + brush->phase_x) & 7u;
    yy = ((unsigned)y + brush->phase_y) & 7u;
    switch (brush->hatch) {
    case 0: return yy == 0u;                 /* horizontal */
    case 1: return xx == 0u;                 /* vertical */
    case 2: return ((xx + yy) & 7u) == 0u; /* forward diagonal */
    case 3: return xx == yy;                 /* backward diagonal */
    case 4: return yy == 0u || xx == 0u;  /* cross */
    default: return ((xx + yy) & 7u) == 0u || xx == yy;
    }
}
