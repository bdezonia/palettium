/* Palettium ET4000AX DOS smoke test of BIOS modes 2Eh and 2Fh.
 * DOS-only BIOS and port access stay outside the portable backend.
 * ONLY for a recoverable ET4000AX VM configured for 256/512/1024 KiB.
 * User MUST supply the VM's actual capacity; a wrong larger value is unsafe.
 */
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
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
    if (width != 640 || (height != 480 && height != 400)) return 0;
    r.h.ah = 0;
    r.h.al = (unsigned char)(height == 480 ? ET4K_BIOS_640X480X8
                                          : ET4K_BIOS_640X400X8);
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
/* This is strictly a ROM *identification hint* for the six known
 * C000h ET4000 option ROMs analyzed at v0036, not a chipset probe.
 * It reads bytes but never writes option-ROM space.
 */
static void dos_rom_identity(void)
{
    const unsigned char __far *rom=(const unsigned char __far *)MK_FP(0xC000,0);
    char date[9];
    char version[7];
    unsigned n;
    if (rom[0]!=0x55u || rom[1]!=0xaau || rom[2]!=0x40u ||
        rom[0x98u]!='V' || (rom[0x99u]!='1' && rom[0x99u]!='8') ||
        rom[0x9au]!='.') {
        puts("C000h: no recognized ET4000 32KiB ROM signature.");
        return;
    }
    for (n=0;n<8u;++n) date[n]=(char)rom[0x8fu+n];
    date[8]='\0';
    for (n=0;n<6u;++n) version[n]=(char)rom[0x98u+n];
    version[6]='\0';
    if (date[2]!='/' || date[5]!='/') {
        puts("C000h: ET4000-like ROM signature, date format unknown.");
        return;
    }
    printf("C000h ROM identification hint: %s, BIOS date %s\n",
           version, date);
}

static void dos_crtc37_report(const ET4K_Port *io)
{
    ET4K_Crtc37 info;
    if (!et4k_crtc37_snapshot(io,&info)) {
        puts("CRTC 37h: unavailable (missing I/O callback).");
        return;
    }
    printf("CRTC 37h raw=%02Xh (index port %03Xh, previous index %02Xh)\n",
           (unsigned)info.raw_value,info.crtc_index_port,
           (unsigned)info.previous_index);
    printf("   Width=%u-bit, chip depth=%uK; organization HINT=%lu KiB\n",
           info.bus_width_bits,info.chip_depth_k,
           info.capacity_hint/1024UL);
    puts("   NOT verified VRAM! BIOS protection/board wiring may differ.");
    puts("   Never use this hint to override the explicit 86Box VRAM setting.");
}

int main(int argc, char **argv)
{
    ET4K_Port io;
    unsigned y, x;
    unsigned long installed;
    int ok = 1;
    if (argc != 2 || (strcmp(argv[1],"INFO") != 0 &&
                      strcmp(argv[1],"256") != 0 &&
                      strcmp(argv[1],"512") != 0 &&
                      strcmp(argv[1],"1024") != 0)) {
        puts("Usage: ET4KTEST INFO | 256 | 512 | 1024");
        puts("INFO: DOS-only read-only ROM/register report, no mode switch.");
        puts("Graphics: enter VRAM KiB explicitly, matching your 86Box setting.");
        puts("A falsely larger VRAM value can corrupt graphics memory.");
        return 2;
    }
    memset(&io, 0, sizeof io);
    memset(&binding, 0, sizeof binding);
    io.in_port = dos_in;
    io.out_port = dos_out;
    dos_rom_identity();
    dos_crtc37_report(&io);
    if (strcmp(argv[1],"INFO") == 0) {
        puts("INFO finished: no BIOS mode call, VRAM access or bank change.");
        return 0;
    }
    installed=(unsigned long)atoi(argv[1])*1024UL;
    io.mem_read = dos_mem_read;
    io.mem_write = dos_mem_write;
    io.set_mode = dos_mode;
    io.restore = dos_restore;
    et4k_setup(&card, io, installed);
    et4k_make_hardware(&card, &hardware);
    if (!et4k_preferred_mode(&card, &mode)) {
        puts("No compatible packed 8-bit ET4000AX mode for this VRAM size.");
        return 1;
    }
    printf("Trying %ux%ux256 (Tseng BIOS %02Xh) on %s KiB VRAM.\n",
           mode.width,mode.height,
           mode.height == 480 ? ET4K_BIOS_640X480X8 : ET4K_BIOS_640X400X8,
           argv[1]);
    if (!rg_hw_open(&binding, &device, &hardware, &mode)) {
        puts("ET4000AX setup failed. Check selected 86Box adapter.");
        return 1;
    }
    /* Gradient covers only the active framebuffer and its legal banks. */
    for (y = 0; y < mode.height; ++y)
        for (x = 0; x < mode.width; ++x)
            rg_write_byte(&device, (rg_offset)y * mode.stride + x,
                          (rg_u8)(((x >> 2) + (y >> 2)) & 255U));
    for (x = 0; x < 256; ++x)
        rg_hw_palette(&binding, x, (rg_u8)x, (rg_u8)(255U-x), (rg_u8)x);
    /* Check non-border cells to make read and write banks diverge. */
    for (y = 0; y < mode.height; y += 29) {
        rg_offset p = (rg_offset)y * mode.stride + 13UL;
        rg_u8 want = (rg_u8)(((13U >> 2) + (y >> 2)) & 255U);
        if (rg_read_byte(&device, p) != want) ok = 0;
    }
    rg_software_line(&device, 0, 0, (int)mode.width-1,
                     (int)mode.height-1, 255);
    rg_software_line(&device, 0, (int)mode.height-1,
                     (int)mode.width-1, 0, 255);
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
