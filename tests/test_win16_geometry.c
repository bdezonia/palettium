/* Guard 16-bit video-memory assumptions independent of compiler. */
#include "rgfx.h"
#include <stdio.h>
#include <limits.h>
static rg_u8 read_byte(void *ctx, rg_offset off)
{ (void)ctx; (void)off; return 0; }
static void write_byte(void *ctx, rg_offset off, rg_u8 v)
{ (void)ctx; (void)off; (void)v; }
int main(void)
{
    RG_Device dev;
    RG_VramOps ops;
    if (sizeof(rg_offset) < 4) return 1;
    if ((rg_offset)640UL * 480UL != 307200UL) return 2;
    /* Reject a framebuffer whose offset would overflow rg_offset. */
    ops.read_byte = 0;
    ops.write_byte = 0;
    if (rg_init_vram(&dev, 640, 480, 640, 307200UL, RG_FMT_INDEX8, ops, 0)) return 3;
    ops.read_byte = read_byte;
    ops.write_byte = write_byte;
    /* No allocated buffer needed: use a non-dereferencing VRAM fixture. */
    if (rg_init_vram(&dev, (unsigned)INT_MAX+1u, 1,
                     (rg_offset)INT_MAX+1UL, (rg_offset)INT_MAX+1UL,
                     RG_FMT_INDEX8, ops, 0)) return 4;
    if (rg_init_vram(&dev, 1, (unsigned)INT_MAX+1u, 1,
                     (rg_offset)INT_MAX+1UL, RG_FMT_INDEX8, ops, 0)) return 5;
    if (rg_init_vram(&dev, 1, 2, ULONG_MAX, ULONG_MAX,
                     RG_FMT_INDEX8, ops, 0)) return 6;
    if (!rg_init_vram(&dev, 1, 2, 65536UL, 65537UL,
                      RG_FMT_INDEX8, ops, 0)) return 7;
    if (rg_init_vram(&dev, 1, 2, 65536UL, 65536UL,
                     RG_FMT_INDEX8, ops, 0)) return 8;
    puts("PASS: Win16 integration guard: 32-bit offsets, callback validation");
    return 0;
}
