/* Palettium ET4000AX 640x480x256 DOS smoke test.
 * DOS-specific BIOS and port access remains here, outside the backend.
 * This uses the Tseng native BIOS mode 2Eh, NOT VESA.
 * Run ONLY on an emulated ET4000AX with at least 512 KiB VRAM.
 */
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include "et4k.h"

static ET4K_State card;
static RG_Hardware hardware;
static RG_Device device;
static RG_HwBinding binding;
static RG_Mode mode;

static rg_u8 dos_in(void *ctx, unsigned port)
{
    (void)ctx;
    return (rg_u8)inp(port);
}
static void dos_out(void *ctx, unsigned port, rg_u8 value)
{
    (void)ctx;
    outp(port, value);
}
static rg_u8 dos_mem_read(void *ctx, unsigned offset)
{
    rg_u8 __far *p;
    (void)ctx;
    p = (rg_u8 __far *)MK_FP(0xA000, offset);
    return *p;
}
static void dos_mem_write(void *ctx, unsigned offset, rg_u8 value)
{
    rg_u8 __far *p;
    (void)ctx;
    p = (rg_u8 __far *)MK_FP(0xA000, offset);
    *p = value;
}
static int dos_mode(void *ctx, unsigned width, unsigned height)
{
    union REGS r;
    (void)ctx;
    if (width != 640 || height != 480) return 0;
    r.h.ah = 0;
    r.h.al = 0x2e; /* Tseng ET4000AX BIOS 640x480x256 */
    int86(0x10, &r, &r);
    /* No standardized status value from this non-VESA BIOS service.
     * Visible test + memory readback below provide verification. */
    return 1;
}
static void dos_restore(void *ctx)
{
    union REGS r;
    (void)ctx;
    r.h.ah = 0;
    r.h.al = 3;
    int86(0x10, &r, &r);
}
int main(void)
{
    ET4K_Port io;
    unsigned y, x;
    int ok = 1;
    memset(&io, 0, sizeof io);
    memset(&binding, 0, sizeof binding);
    io.in_port = dos_in;
    io.out_port = dos_out;
    io.mem_read = dos_mem_read;
    io.mem_write = dos_mem_write;
    io.set_mode = dos_mode;
    io.restore = dos_restore;
    et4k_setup(&card, io, 1048576UL);
    et4k_make_hardware(&card, &hardware);
    mode.width = 640;
    mode.height = 480;
    mode.format = RG_FMT_INDEX8;
    mode.stride = 0;
    if (!rg_hw_open(&binding, &device, &hardware, &mode)) {
        puts("ET4000AX setup failed. Check selected 86Box adapter.");
        return 1;
    }
    /* Distinctive per-scanline gradient; must cross all 64K pages. */
    for (y = 0; y < 480; ++y)
        for (x = 0; x < 640; ++x)
            rg_write_byte(&device, (rg_offset)y * 640UL + x,
                          (rg_u8)(((x >> 2) + (y >> 2)) & 255U));
    for (x = 0; x < 256; ++x)
        rg_hw_palette(&binding, x, (rg_u8)x, (rg_u8)(255U-x), (rg_u8)x);
    /* Check non-border cells to make read and write banks diverge. */
    for (y = 0; y < 480; y += 29) {
        rg_offset p = (rg_offset)y * 640UL + 13UL;
        rg_u8 want = (rg_u8)(((13U >> 2) + (y >> 2)) & 255U);
        if (rg_read_byte(&device, p) != want) ok = 0;
    }
    rg_software_line(&device, 0, 0, 639, 479, 255);
    rg_software_line(&device, 0, 479, 639, 0, 255);
    /* A user-visible image constitutes the primary mode verification. */
    getch();
    rg_hw_close(&binding);
    if (!ok) {
        puts("FAIL: ET4000AX VRAM readback");
        return 1;
    }
    puts("PASS: ET4000AX VRAM readback. Did graphics appear correct?");
    return 0;
}
