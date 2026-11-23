#ifndef RG_GDI_BLIT_H
#define RG_GDI_BLIT_H
/* Portable bridge from Win3.x ROP3 to the Palettium two-input engine.
 * v0014 deliberately handles only operations independent of brush pattern.
 * Return 1 for performed, 0 for unsupported/invalid (no writes).
 * This is NOT a Windows DDI pointer decoder.
 */
#include "rgfx.h"
int rg_gdi_blit_rop3(RG_Device *dst, RG_Device *src,
                     int dx, int dy, int sx, int sy,
                     int width, int height, unsigned long win_rop);
#endif
