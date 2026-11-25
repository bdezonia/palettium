#include "rg_hardware.h"
#include "sim_bank.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 640
#define HEIGHT 480
#define SIZE ((rg_offset)WIDTH * HEIGHT)

typedef struct Fixture {
    PAL_BankSim sim;
    rg_u8 *memory;
    int inits;
    int modes;
    int stops;
    int palettes;
    int accelerations;
    int fail_mode;
} Fixture;

static int start(void *v) { ++((Fixture *)v)->inits; return 1; }
static int mode(void *v, RG_Mode *m)
{
    Fixture *f = (Fixture *)v;
    ++f->modes;
    if (f->fail_mode || m->width != WIDTH || m->height != HEIGHT ||
        m->format != RG_FMT_INDEX8) return 0;
    m->stride = WIDTH;
    m->capacity = SIZE;
    pal_bank_init(&f->sim, f->memory, SIZE);
    return 1;
}
static void stop(void *v) { ++((Fixture *)v)->stops; }
static rg_u8 read_byte(void *v, rg_offset o)
{ return pal_bank_read(&((Fixture *)v)->sim, o); }
static void write_byte(void *v, rg_offset o, rg_u8 b)
{ pal_bank_write(&((Fixture *)v)->sim, o, b); }
static void palette(void *v, unsigned i, rg_u8 r, rg_u8 g, rg_u8 b)
{
    Fixture *f = (Fixture *)v;
    (void)i; (void)r; (void)g; (void)b;
    ++f->palettes;
}
static void fast_fill(RG_Device *d, int x, int y, int w, int h, rg_pixel c)
{
    Fixture *f = (Fixture *)d->card_data;
    ++f->accelerations;
    rg_software_fill(d, x, y, w, h, c);
}
static void install(void *v, RG_Device *d)
{ (void)v; d->ops.fill = fast_fill; }
static int check(int condition, const char *msg)
{
    if (!condition) fprintf(stderr, "FAIL: %s\n", msg);
    return condition;
}
int main(void)
{
    Fixture f;
    RG_Hardware hw;
    RG_HwBinding binding;
    RG_Device dev, linear;
    RG_Mode selected;
    rg_u8 *expected, *backing;
    int ok = 1;
    rg_pixel value = 123UL;
    memset(&f, 0, sizeof(f));
    memset(&binding, 0, sizeof(binding));
    backing = (rg_u8 *)calloc((size_t)SIZE, 1);
    expected = (rg_u8 *)calloc((size_t)SIZE, 1);
    if (!backing || !expected) { free(backing); free(expected); return 1; }
    f.memory = backing;
    memset(&hw, 0, sizeof(hw));
    hw.context = &f;
    hw.initialize = start;
    hw.set_mode = mode;
    hw.shutdown = stop;
    hw.read_byte = read_byte;
    hw.write_byte = write_byte;
    hw.set_palette = palette;
    hw.install_accel = install;
    selected.width = WIDTH;
    selected.height = HEIGHT;
    selected.format = RG_FMT_INDEX8;
    selected.stride = 0;
    ok &= check(rg_hw_open(&binding, &dev, &hw, &selected), "open");
    ok &= check(!rg_hw_open(&binding, &dev, &hw, &selected), "double open rejected");
    ok &= check(selected.stride == WIDTH, "actual stride");
    ok &= check(rg_init(&linear, expected, WIDTH, HEIGHT, WIDTH, (rg_offset)WIDTH * HEIGHT, RG_FMT_INDEX8), "linear init");
    dev.ops.fill(&dev, 2, 100, 550, 190, 0x5a);
    linear.ops.fill(&linear, 2, 100, 550, 190, 0x5a);
    dev.ops.line(&dev, 1, 1, 638, 479, 0xf1);
    linear.ops.line(&linear, 1, 1, 638, 479, 0xf1);
    dev.ops.blit(&dev, &dev, 30, 140, 2, 100, 400, 180, RG_ROP_XOR);
    linear.ops.blit(&linear, &linear, 30, 140, 2, 100, 400, 180, RG_ROP_XOR);
    ok &= check(memcmp(backing, expected, (size_t)SIZE) == 0,
                "banked and linear images differ");
    ok &= check(f.sim.switch_count > 0, "bank crossing exercised");
    ok &= check(f.accelerations == 1, "one overridden operation");
    ok &= check(rg_hw_palette(&binding, 1, 255, 128, 0), "palette");
    ok &= check(!rg_hw_palette(&binding, 256, 0, 0, 0), "palette bounds");
    ok &= check(f.palettes == 1, "palette callback count");
    rg_hw_close(&binding);
    ok &= check(f.stops == 1 && !binding.active, "close");
    ok &= check(!rg_hw_palette(&binding, 1, 0, 0, 0), "palette after close");
    ok &= check(!rg_get_pixel(&dev, 0, 0, &value) && value == 123UL,
                "closed device read fails without publishing output");
    ok &= check(!rg_put_pixel(&dev, 0, 0, 9), "closed device write fails");
    ok &= check(dev.ops.fill == rg_software_fill &&
                dev.ops.line == rg_software_line &&
                dev.ops.blit == rg_software_blit &&
                dev.ops.pixel == rg_software_pixel, "stale overrides removed");
    dev.ops.fill(&dev, 0, 0, WIDTH, HEIGHT, 9);
    dev.ops.line(&dev, 0, 0, 10, 10, 9);
    dev.ops.blit(&dev, &linear, 0, 0, 0, 0, 4, 4, RG_ROP_COPY);
    ok &= check(f.accelerations == 1 &&
                memcmp(backing, expected, (size_t)SIZE) == 0,
                "late calls are harmless");
    f.fail_mode = 1;
    ok &= check(!rg_hw_open(&binding, &dev, &hw, &selected), "failed mode rejected");
    ok &= check(f.stops == 2 && !binding.active, "failed mode shutdown");
    free(backing);
    free(expected);
    if (!ok) return 1;
    puts("PASS: hardware binding, fallback, override, palette, lifecycle, banked equality");
    return 0;
}
