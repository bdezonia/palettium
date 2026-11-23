#ifndef RG_GDI_ROP_H
#define RG_GDI_ROP_H

#include "rgfx.h"

/* Shared GDI interoperability, C89/Win16 friendly. No Windows headers.
 * Truth-table index is (pattern_bit << 2) | (source_bit << 1) | dest_bit.
 * Thus 0xCC=SRCCOPY, 0xAA=DSTCOPY, 0xF0=PATCOPY.
 */
rg_u8 rg_gdi_rop3_byte(unsigned rop3, rg_u8 pattern,
                      rg_u8 source, rg_u8 dest);

/* Extract ROP3 from a Windows DWORD raster-op code (e.g. 0x00CC0020UL).
 * Does NOT validate the non-truth-table bits of a GDI command.
 */
unsigned rg_gdi_rop3_code(unsigned long win_rop);

/* Return 4-bit source/destination truth table if the given ROP3 ignores
 * pattern (0..15). Return -1 if the result depends on pattern.
 */
int rg_gdi_rop3_to_rop2(unsigned rop3);

#endif
