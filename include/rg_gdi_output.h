#ifndef RG_GDI_OUTPUT_H
#define RG_GDI_OUTPUT_H
/* Shared screen rectangle renderer for Win3.x Output(OS_RECTANGLE).
 * Geometry is half-open [left,right)x[top,bottom); clipping is also
 * half-open. NULL pen/brush means no border/fill. ROP2 is 1..16.
 * The public entry refuses invalid objects/coordinates without writes.
 */
#include "rg_objects.h"

typedef struct RG_Rect {
    int left, top, right, bottom;
} RG_Rect;

int rg_gdi_output_rectangle(RG_Device *device, RG_Rect rect,
                            const RG_Rect *clip, const RG_Pen *pen,
                            const RG_Brush *brush, unsigned rop2,
                            unsigned background_mode);
#endif
