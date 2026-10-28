/* Palettium Win16 toolchain smoke-test DLL.
 * This is NOT a GDI display driver. Do not install it as SYSTEM.DRV.
 * It deliberately requires no DDK headers.
 */
#include "rgfx.h"

/* The symbol is exported via PALPROBE.LNK. No Windows header required. */
unsigned short __far __pascal PalettiumAbiProbe(void)
{
    /* Bit 0: 8-bit byte; bit 1: 32-bit VRAM offset;
       bit 2: 16-bit int; bit 3: operation table exists. */
    unsigned short bits = 0;
    if (sizeof(rg_u8) == 1) bits |= 1;
    if (sizeof(rg_offset) >= 4) bits |= 2;
    if (sizeof(int) == 2) bits |= 4;
    if (sizeof(RG_Ops) > 0) bits |= 8;
    return bits;
}
