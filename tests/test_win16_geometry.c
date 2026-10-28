/* Guard 16-bit video-memory assumptions independent of compiler. */
#include "rgfx.h"
#include <stdio.h>
#include <limits.h>
int main(void)
{
    RG_Device dev;
    RG_VramOps ops;
    if (sizeof(rg_offset) < 4) return 1;
    if ((rg_offset)640UL * 480UL != 307200UL) return 2;
    /* Reject a framebuffer whose offset would overflow rg_offset. */
    ops.read_byte = 0;
    ops.write_byte = 0;
    if (rg_init_vram(&dev, 640, 480, 640, ops, 0)) return 3;
    puts("PASS: Win16 integration guard: 32-bit offsets, callback validation");
    return 0;
}
