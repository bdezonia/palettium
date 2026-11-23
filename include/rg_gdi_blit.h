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
/* DDK mono/color conversion for GDI memory BITMAPs: a mono 1 bit is
 * background (white), a 0 bit is text/foreground (black). Color->mono
 * writes 1 only for source pixels matching background. Only pattern-
 * independent ROP3 are currently accepted; refusal performs no writes.
 * Same-format transfers keep the existing hardware blit override. */
int rg_gdi_blit_color(RG_Device *dst, RG_Device *src,
                      int dx, int dy, int sx, int sy,
                      int width, int height, unsigned long win_rop,
                      unsigned text_color, unsigned background_color);
/* Map a physical 8bpp palette index into a monochrome output bit.
 * The DDK's white/background bit is 1. */
unsigned rg_gdi_mono_color(unsigned physical, unsigned background_color);
#endif
