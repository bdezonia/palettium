#include "et4k.h"

#define ET4K_BANK_PORT 0x3CD
#define ET4K_DAC_WRITE 0x3C8
#define ET4K_DAC_DATA  0x3C9

/* Read-only ET4000 memory-configuration *hint*. The only OUTs select
 * CRTC index 37h and restore the previous index. No CRTC data writes.
 * The BIOS/board may protect or implement this register differently;
 * never promote the decoded guess to ET4K_State.capacity automatically.
 */
void et4k_crtc37_decode(rg_u8 raw, ET4K_Crtc37 *result)
{
    unsigned bus;
    unsigned depth;
    if (!result) return;
    bus = (unsigned)(raw & 3u);
    result->raw_value = raw;
    result->bus_width_bits = (bus >= 2u) ? (bus == 2u ? 16u : 32u) : 8u;
    depth = (raw & 8u) ? 256u : 64u;
    result->chip_depth_k = depth;
    result->capacity_hint = (unsigned long)(result->bus_width_bits / 8u)
                            * (unsigned long)depth * 1024UL;
}

int et4k_crtc37_snapshot(const ET4K_Port *io, ET4K_Crtc37 *result)
{
    ET4K_Crtc37 temp;
    unsigned index_port;
    rg_u8 original;
    rg_u8 raw;
    if (!io || !result || !io->in_port || !io->out_port) return 0;
    temp.misc_output = io->in_port(io->cookie, 0x3CCu);
    index_port = (temp.misc_output & 1u) ? 0x3D4u : 0x3B4u;
    original = io->in_port(io->cookie, index_port);
    io->out_port(io->cookie, index_port, 0x37u);
    raw = io->in_port(io->cookie, index_port + 1u);
    io->out_port(io->cookie, index_port, original);
    temp.crtc_index_port = index_port;
    temp.previous_index = original;
    et4k_crtc37_decode(raw, &temp);
    *result = temp;
    return 1;
}

/* The 4-bit bank selector addresses at most 1 MiB. ET4000AX ROM modes
 * 2Eh (640x480x8) and 2Fh (640x400x8) use packed bytes, so 256K cards
 * can use the latter without inventing a planar framebuffer layout.
 * BIOS support and VRAM size are separate checks. */
static int et4k_valid_capacity(unsigned long bytes)
{
    return bytes == 262144UL || bytes == 524288UL || bytes == 1048576UL;
}

unsigned long et4k_probe_capacity(ET4K_State *s)
{
    unsigned long detected;
    if (!s) return 0;
    if (et4k_valid_capacity(s->capacity)) return s->capacity;
    /* A wrongly specified nonzero size is a configuration error. */
    if (s->capacity || !s->io.query_capacity) return 0;
    detected=s->io.query_capacity(s->io.cookie);
    if (!et4k_valid_capacity(detected)) return 0;
    s->capacity=detected;
    return detected;
}

int et4k_mode_info(unsigned width, unsigned height, RG_PixelFormat format,
                   ET4K_ModeInfo *info)
{
    ET4K_ModeInfo found;
    if (format != RG_FMT_INDEX8 || width != 640u) return 0;
    found.width = width;
    found.height = height;
    found.format = RG_FMT_INDEX8;
    if (height == 480u) {
        found.bios_mode = ET4K_BIOS_640X480X8;
        found.minimum_vram = 307200UL;
    } else if (height == 400u) {
        found.bios_mode = ET4K_BIOS_640X400X8;
        found.minimum_vram = 256000UL;
    } else return 0;
    if (info) *info = found;
    return 1;
}

int et4k_mode_eligible(const ET4K_State *s,
                       const ET4K_ModeInfo *info)
{
    ET4K_ModeInfo known;
    if (!s || !info || !et4k_valid_capacity(s->capacity) ||
        !et4k_mode_info(info->width, info->height, info->format, &known) ||
        known.bios_mode != info->bios_mode ||
        s->capacity < known.minimum_vram) return 0;
    return !s->io.query_mode ||
           s->io.query_mode(s->io.cookie, known.bios_mode);
}

int et4k_preferred_mode(ET4K_State *s, RG_Mode *mode)
{
    ET4K_ModeInfo info;
    if (!mode || !et4k_probe_capacity(s)) return 0;
    if (!et4k_mode_info(640u,480u,RG_FMT_INDEX8,&info) ||
        !et4k_mode_eligible(s,&info)) {
        if (!et4k_mode_info(640u,400u,RG_FMT_INDEX8,&info) ||
            !et4k_mode_eligible(s,&info)) return 0;
    }
    mode->width=info.width;
    mode->height=info.height;
    mode->format=info.format;
    mode->stride=0;
    mode->capacity=0;
    return 1;
}
static int et4k_init(void *ctx)
{
    ET4K_State *s = (ET4K_State *)ctx;
    if (!s || !s->io.in_port || !s->io.out_port ||
        !s->io.mem_read || !s->io.mem_write || !s->io.set_mode)
        return 0;
    /* Capacity query is a non-destructive platform callback. Never infer
     * 512K from a successful BIOS request, and never alias-probe VRAM. */
    if (!et4k_probe_capacity(s)) return 0;
    s->bank_valid = 0;
    s->mode_attempted = 0;
    s->active = 1;
    return 1;
}

static int et4k_mode(void *ctx, RG_Mode *mode)
{
    ET4K_State *s = (ET4K_State *)ctx;
    ET4K_ModeInfo info;
    if (!s || !mode || !s->active ||
        !et4k_mode_info(mode->width, mode->height, mode->format, &info) ||
        !et4k_mode_eligible(s, &info)) return 0;
    /* A BIOS call can partially alter the hardware even when it reports
     * failure. Roll it back on shutdown. A VRAM/ROM preflight refusal,
     * by contrast, must NOT restore or disturb the existing display. */
    s->mode_attempted = 1;
    if (!s->io.set_mode(s->io.cookie, info.width, info.height)) return 0;
    mode->stride = info.width;
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
    if (s->active && s->mode_attempted && s->io.restore)
        s->io.restore(s->io.cookie);
    s->mode_attempted = 0;
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

/* Software fill override: bank selection once per contiguous aperture run,
 * not once per pixel. The platform may implement a tight aperture loop. */
static void et4k_fill(RG_Device *d, int x, int y, int w, int h, rg_pixel p)
{
    ET4K_State *s;
    long l=x, t=y, r=l+(long)w, b=t+(long)h, yy;
    rg_offset off, remaining, room, chunk;
    unsigned pos, count, i;
    if (!d || w<=0 || h<=0) return;
    s=(ET4K_State *)d->card_data;
    if (!s || !s->active || !d->vram.write_byte) return;
    if (d->surface.format!=RG_FMT_INDEX8) {
        rg_software_fill(d,x,y,w,h,p);
        return;
    }
    if (l<0) l=0;
    if (t<0) t=0;
    if (r>(long)d->surface.width) r=d->surface.width;
    if (b>(long)d->surface.height) b=d->surface.height;
    if (l>=r || t>=b) return;
    for (yy=t; yy<b; ++yy) {
        off=(rg_offset)yy*d->surface.stride+(rg_offset)l;
        remaining=(rg_offset)(r-l);
        if (off>=d->surface.capacity || remaining>d->surface.capacity-off ||
            off>=s->capacity || remaining>s->capacity-off) return;
        while (remaining) {
            pos=(unsigned)(off & 65535UL);
            room=65536UL-(rg_offset)pos;
            chunk=remaining<room ? remaining : room;
            if (chunk>65535UL) chunk=65535UL;
            count=(unsigned)chunk;
            et4k_select(s,(unsigned)(off>>16),0);
            if (s->io.mem_fill)
                s->io.mem_fill(s->io.cookie,pos,count,(rg_u8)p);
            else
                for (i=0;i<count;++i)
                    s->io.mem_write(s->io.cookie,pos+i,(rg_u8)p);
            off+=chunk;
            remaining-=chunk;
        }
    }
}
static void et4k_install_software(void *ctx, RG_Device *d)
{
    (void)ctx;
    if (d && d->surface.format==RG_FMT_INDEX8) d->ops.fill=et4k_fill;
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
    s->mode_attempted = 0;
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
    hw->install_accel = et4k_install_software;
}
