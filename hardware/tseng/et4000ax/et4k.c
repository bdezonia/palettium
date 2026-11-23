#include "et4k.h"

#define ET4K_BANK_PORT 0x3CD
#define ET4K_DAC_WRITE 0x3C8
#define ET4K_DAC_DATA  0x3C9

/* This version deliberately supports up to sixteen 64K pages (1 MiB).
 * The port interface may be extended for larger later chipsets.
 */
static int et4k_init(void *ctx)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (!s || !s->io.in_port || !s->io.out_port ||
        !s->io.mem_read || !s->io.mem_write || !s->io.set_mode ||
        s->capacity < 307200UL || s->capacity > 1048576UL)
        return 0;
    s->bank_valid = 0;
    s->active = 1;
    return 1;
}

static int et4k_mode(void *ctx, RG_Mode *mode)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (!s->active || mode->format != RG_FMT_INDEX8 ||
        mode->width != 640 || mode->height != 480 ||
        (unsigned long)mode->width * mode->height > s->capacity)
        return 0;
    if (!s->io.set_mode(s->io.cookie, 640, 480)) return 0;
    mode->stride = 640;
    mode->capacity = s->capacity;
    /* Reset both bank selectors to zero after firmware mode setup. */
    s->read_bank = s->write_bank = 0;
    s->io.out_port(s->io.cookie, ET4K_BANK_PORT, 0);
    s->bank_valid = 1;
    return 1;
}

static void et4k_shutdown(void *ctx)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (s->active && s->io.restore) s->io.restore(s->io.cookie);
    s->active = 0;
    s->bank_valid = 0;
}

static void et4k_select(ET4K_State *s, unsigned bank, int reading)
{
    if (!s->bank_valid || (reading ? s->read_bank : s->write_bank) != bank) {
        if (reading) s->read_bank = bank;
        else s->write_bank = bank;
        s->io.out_port(s->io.cookie, ET4K_BANK_PORT,
            (rg_u8)((s->read_bank << 4) | s->write_bank));
        s->bank_valid = 1;
    }
}

static rg_u8 et4k_read(void *ctx, rg_offset offset)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (!s->active || offset >= s->capacity) return 0;
    et4k_select(s, (unsigned)(offset >> 16), 1);
    return s->io.mem_read(s->io.cookie, (unsigned)(offset & 65535UL));
}

static void et4k_write(void *ctx, rg_offset offset, rg_u8 value)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (!s->active || offset >= s->capacity) return;
    et4k_select(s, (unsigned)(offset >> 16), 0);
    s->io.mem_write(s->io.cookie, (unsigned)(offset & 65535UL), value);
}

static void et4k_palette(void *ctx, unsigned index,
                         rg_u8 red, rg_u8 green, rg_u8 blue)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (!s->active || index >= 256) return;
    s->io.out_port(s->io.cookie, ET4K_DAC_WRITE, (rg_u8)index);
    s->io.out_port(s->io.cookie, ET4K_DAC_DATA, (rg_u8)(red >> 2));
    s->io.out_port(s->io.cookie, ET4K_DAC_DATA, (rg_u8)(green >> 2));
    s->io.out_port(s->io.cookie, ET4K_DAC_DATA, (rg_u8)(blue >> 2));
}

void et4k_setup(ET4K_State *s, ET4K_Port io, unsigned long capacity)
{
    s->io = io;
    s->capacity = capacity;
    s->read_bank = 0;
    s->write_bank = 0;
    s->bank_valid = 0;
    s->active = 0;
}

void et4k_make_hardware(ET4K_State *s, RG_Hardware *hw)
{
    hw->context = s;
    hw->initialize = et4k_init;
    hw->set_mode = et4k_mode;
    hw->shutdown = et4k_shutdown;
    hw->read_byte = et4k_read;
    hw->write_byte = et4k_write;
    hw->set_palette = et4k_palette;
    hw->install_accel = 0;
}
