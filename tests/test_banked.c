#include <stdio.h>
#include <string.h>
#include "rgfx.h"
#include "sim_bank.h"

#define W 640
#define H 480
static rg_u8 linear[W * H];
static rg_u8 banked[W * H];

static int check(const char *what)
{
    if (memcmp(linear, banked, sizeof(linear))) {
        fprintf(stderr, "FAIL: %s: linear and banked VRAM differ\n", what);
        return 0;
    }
    return 1;
}

int main(void)
{
    RG_Device a, b;
    PAL_BankSim sim;
    RG_VramOps access;
    unsigned x;
    memset(linear, 0, sizeof(linear));
    memset(banked, 0, sizeof(banked));
    pal_bank_init(&sim, banked, sizeof(banked));
    access.read_byte = pal_bank_read;
    access.write_byte = pal_bank_write;
    if (!rg_init(&a, linear, W, H, W) ||
        !rg_init_vram(&b, W, H, W, access, &sim)) return 1;
    { RG_VramOps invalid; invalid.read_byte = 0; invalid.write_byte = 0;
      if (rg_init_vram(&b, W, H, W, invalid, &sim)) return 2; }
    if (!rg_init_vram(&b, W, H, W, access, &sim)) return 3;

    /* Cross 64-KiB boundary inside a scanline (65536 / 640 = 102 r256). */
    a.ops.fill(&a, 0, 100, W, 7, 77);
    b.ops.fill(&b, 0, 100, W, 7, 77);
    if (!check("cross-bank rectangle")) return 4;
    a.ops.line(&a, 0, 0, W - 1, H - 1, 200);
    b.ops.line(&b, 0, 0, W - 1, H - 1, 200);
    if (!check("diagonal")) return 5;
    a.ops.fill(&a, -15, 475, 670, 10, 123);
    b.ops.fill(&b, -15, 475, 670, 10, 123);
    if (!check("clipped rectangle")) return 6;
    for (x = 0; x < W; ++x) {
        a.ops.pixel(&a, (int)x, 102, (rg_u8)x);
        b.ops.pixel(&b, (int)x, 102, (rg_u8)x);
    }
    if (!check("scanline crossing bank")) return 7;
    /* Force reverse bank order, test reading through the same abstraction. */
    if (rg_read_byte(&b, W * H - 1UL) != linear[W * H - 1] ||
        rg_read_byte(&b, 65535UL) != linear[65535UL] ||
        rg_read_byte(&b, 65536UL) != linear[65536UL]) return 8;
    if (sim.switch_count < 4) return 9;
    printf("PASS: banked 640x480 agrees with linear framebuffer; bank switches=%lu\n",
           sim.switch_count);
    return 0;
}
