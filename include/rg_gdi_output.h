#ifndef RG_GDI_OUTPUT_H
#define RG_GDI_OUTPUT_H
/* OS-independent INDEX8 GDI drawing primitives. Win16 DDI POINT arrays
 * are decoded only in the OS adapter. Coordinates and clipping are half-open.
 * A line segment draws its first point, but not its final endpoint (DDK).
 * All new primitive entry points refuse invalid arguments before writing.
 */
#include "rg_objects.h"

typedef struct RG_Rect {
    int left, top, right, bottom;
} RG_Rect;

int rg_gdi_output_rectangle(RG_Device *device, RG_Rect rect,
                            const RG_Rect *clip, const RG_Pen *pen,
                            const RG_Brush *brush, unsigned rop2,
                            unsigned background_mode);
/* Draw one interval [left,right) at y, with brush taking precedence.
 * Without a brush use the physical pen. No object => successful no-op.
 */
int rg_gdi_output_scanline(RG_Device *device, int y, int left, int right,
                           const RG_Rect *clip, const RG_Pen *pen,
                           const RG_Brush *brush, unsigned rop2,
                           unsigned background_mode);
/* One solid GDI polyline segment, from (x0,y0) through but excluding
 * (x1,y1). Adjacent segments therefore never double-write joints.
 * 32-bit arithmetic protects Win16's potentially 16-bit int.
 */
int rg_gdi_output_segment(RG_Device *device, int x0, int y0,
                          int x1, int y1, const RG_Rect *clip,
                          const RG_Pen *pen, unsigned rop2);
#endif
