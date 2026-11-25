#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "et4k.h"

#define VRAM_SIZE 524288UL
#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "FAIL at %d: %s\n", __LINE__, #expr); exit(1); } } while (0)

typedef struct Mock {
    rg_u8 vram[VRAM_SIZE];
    rg_u8 reg;
    unsigned writes_to_bank;
    unsigned invalid;
    unsigned long fill_calls, byte_calls;
    unsigned mode_calls;
    unsigned restore_calls;
    unsigned palette_writes;
    rg_u8 pal[4];
} Mock;

static rg_u8 mock_in(void *ctx, unsigned port)
{
    Mock *m = (Mock *)ctx;
    if (port == 0x3CD) return m->reg;
    m->invalid++;
    return 0;
}
static void mock_out(void *ctx, unsigned port, rg_u8 value)
{
    Mock *m = (Mock *)ctx;
    if (port == 0x3CD) { m->reg = value; m->writes_to_bank++; }
    else if (port == 0x3C8 || port == 0x3C9) {
        if (m->palette_writes < 4) m->pal[m->palette_writes] = value;
        m->palette_writes++;
    } else m->invalid++;
}
static rg_u8 mock_read(void *ctx, unsigned off)
{
    Mock *m = (Mock *)ctx;
    unsigned long addr = (unsigned long)(m->reg >> 4) * 65536UL + off;
    if (addr >= VRAM_SIZE) { m->invalid++; return 0; }
    return m->vram[addr];
}
static void mock_write(void *ctx, unsigned off, rg_u8 v)
{
    Mock *m = (Mock *)ctx;
    unsigned long addr = (unsigned long)(m->reg & 15) * 65536UL + off;
    if (addr >= VRAM_SIZE) { m->invalid++; return; }
    m->byte_calls++;
    m->vram[addr] = v;
}
static void mock_fill(void *ctx, unsigned off, unsigned count, rg_u8 v)
{
    Mock *m=(Mock *)ctx;
    unsigned long addr=(unsigned long)(m->reg & 15)*65536UL+off;
    if (!count || (unsigned long)off+count>65536UL ||
        addr+count>VRAM_SIZE) { m->invalid++; return; }
    m->fill_calls++;
    memset(m->vram+addr,v,count);
}
static int mock_mode(void *ctx, unsigned w, unsigned h)
{
    Mock *m = (Mock *)ctx;
    m->mode_calls++;
    return w == 640 && h == 480;
}
static void mock_restore(void *ctx)
{
    Mock *m = (Mock *)ctx;
    m->restore_calls++;
}

int main(void)
{
    Mock *m = (Mock *)calloc(1, sizeof(*m));
    ET4K_Port io;
    ET4K_State state;
    RG_Hardware hw;
    RG_Device dev;
    RG_HwBinding binding;
    RG_Mode mode;
    unsigned n;
    unsigned long off;
    CHECK(m != 0);
    memset(&io, 0, sizeof io);
    io.cookie = m;
    io.in_port = mock_in;
    io.out_port = mock_out;
    io.mem_read = mock_read;
    io.mem_write = mock_write;
    io.mem_fill = mock_fill;
    io.set_mode = mock_mode;
    io.restore = mock_restore;
    et4k_setup(&state, io, VRAM_SIZE);
    et4k_make_hardware(&state, &hw);
    memset(&binding, 0, sizeof binding);
    mode.width = 640; mode.height = 480; mode.format = RG_FMT_INDEX8; mode.stride = 0;
    CHECK(rg_hw_open(&binding, &dev, &hw, &mode));
    CHECK(mode.stride == 640 && m->mode_calls == 1);
    CHECK(m->reg == 0);
    for (n = 0; n < 480; ++n) {
        rg_u8 val = (rg_u8)(n * 37U);
        rg_write_byte(&dev, (rg_offset)n * 640UL + 2, val);
    }
    for (n = 0; n < 480; ++n) {
        rg_u8 val = (rg_u8)(n * 37U);
        CHECK(rg_read_byte(&dev, (rg_offset)n * 640UL + 2) == val);
    }
    rg_write_byte(&dev, 65535UL, 0x11);
    rg_write_byte(&dev, 65536UL, 0x22);
    CHECK(rg_read_byte(&dev, 65535UL) == 0x11);
    CHECK(rg_read_byte(&dev, 65536UL) == 0x22);
    CHECK((m->reg >> 4) == 1);
    CHECK((m->reg & 15) == 1);
    rg_software_line(&dev, 0, 0, 639, 479, 0x5a);
    CHECK(rg_read_byte(&dev, 0) == 0x5a);
    off = 479UL * 640UL + 639;
    CHECK(rg_read_byte(&dev, off) == 0x5a);
    /* Verify every byte, including the untouched area beyond the screen.
     * Full screen crosses four 64 KiB boundaries, none on scanline edges. */
    memset(m->vram,0xA5,sizeof(m->vram));
    (void)rg_read_byte(&dev,65536UL); /* preserve an independent read bank */
    m->fill_calls=m->byte_calls=0;
    dev.ops.fill(&dev,0,0,640,480,0x37);
    CHECK(m->fill_calls==484UL && m->byte_calls==0);
    CHECK((m->reg >> 4)==1);
    for (off=0;off<VRAM_SIZE;++off)
        CHECK(m->vram[off]==(off<307200UL ? 0x37 : 0xA5));
    dev.ops.fill(&dev,-5,102,270,1,0x6C);
    for (off=0;off<VRAM_SIZE;++off)
        CHECK(m->vram[off]==(off>=65280UL && off<65545UL ? 0x6C :
                             (off<307200UL ? 0x37 : 0xA5)));
    CHECK(m->fill_calls==486UL && m->byte_calls==0);
    dev.ops.fill(&dev,640,0,1,1,0);
    CHECK(m->fill_calls==486UL);
    state.io.mem_fill=0;
    dev.ops.fill(&dev,255,102,3,1,0x19);
    CHECK(m->byte_calls==3 && m->vram[65535UL]==0x19 &&
          m->vram[65536UL]==0x19 && m->vram[65537UL]==0x19);
    CHECK(rg_hw_palette(&binding, 7, 255, 128, 0));
    CHECK(m->palette_writes == 4);
    CHECK(m->pal[0] == 7 && m->pal[1] == 63 && m->pal[2] == 32 && m->pal[3] == 0);
    CHECK(m->writes_to_bank > 6);
    CHECK(m->invalid == 0);
    rg_hw_close(&binding);
    CHECK(m->restore_calls == 1);
    dev.ops.fill(&dev,0,0,640,480,0);
    CHECK(m->byte_calls==3 && m->fill_calls==486UL);
    /* Same backend MUST reject a pixel format its silicon does not support. */
    mode.format = RG_FMT_RGB565;
    mode.capacity = 0;
    CHECK(!rg_hw_open(&binding, &dev, &hw, &mode));
    /* Unsupported format never reached BIOS mode setting. */
    CHECK(m->restore_calls == 1);
    free(m);
    puts("PASS: ET4000AX bank register, independent read/write banks, framebuffer, palette and lifecycle");
    return 0;
}
