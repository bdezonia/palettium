#ifndef PAL_OBJECTS_H
#define PAL_OBJECTS_H
/* DDK Win3.x GDI logical objects, NOT the SDK LOGBRUSH layout.
 * The display DDI's logical brush has a fourth lbBkColor DWORD and
 * must be exactly 12 bytes. Host/Win16 layout checked by tests.
 */
#include "pal_pdevice.h"
#include "rg_objects.h"
#pragma pack(push, 1)
typedef struct PAL_LogicalPen {
    PAL_U16 style;
    PAL_POINT16 width;
    PAL_U32 color;
} PAL_LogicalPen;
typedef struct PAL_LogicalBrush {
    PAL_U16 style;
    PAL_U32 color;
    PAL_U16 hatch;
    PAL_U32 background;
} PAL_LogicalBrush;
typedef struct PAL_Rect16 {
    PAL_S16 left, top, right, bottom;
} PAL_Rect16;
#pragma pack(pop)

int pal_object_layout_ok(void);
/* DDK hatch background may be a literal physical index (0..255) or
 * 0xFF00iiii, a GDI logical palette index requiring translation. */
int pal_object_background(const RG_Palette *palette, PAL_U32 specified,
                          unsigned *physical);
#endif
