#include <stddef.h>
#include "pal_objects.h"

int pal_object_layout_ok(void)
{
    return sizeof(PAL_LogicalPen) == 10u &&
           offsetof(PAL_LogicalPen, width) == 2u &&
           offsetof(PAL_LogicalPen, color) == 6u &&
           sizeof(PAL_LogicalBrush) == 12u &&
           offsetof(PAL_LogicalBrush, color) == 2u &&
           offsetof(PAL_LogicalBrush, hatch) == 6u &&
           offsetof(PAL_LogicalBrush, background) == 8u &&
           sizeof(PAL_Rect16) == 8u &&
           sizeof(RG_Pen) == 8u && sizeof(RG_Brush) == 14u;
}

int pal_object_background(const RG_Palette *palette, PAL_U32 specified,
                          unsigned *physical)
{
    unsigned index;
    if (!palette || !physical || !palette->initialized) return 0;
    if ((specified & 0xFFFF0000UL) == 0xFF000000UL) {
        index = (unsigned)(specified & 0xFFFFUL);
        if (index >= 256u) return 0;
        index = rg_palette_translate(palette, index);
    } else {
        if (specified >= 256UL) return 0;
        index = (unsigned)specified;
    }
    if (index >= 256u) return 0;
    *physical = index;
    return 1;
}
