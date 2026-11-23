#include "rg_hardware.h"

int rg_hw_open(RG_HwBinding *binding, RG_Device *device,
               RG_Hardware *hardware, RG_Mode *mode)
{
    RG_VramOps vram;
    if (!binding || !device || !hardware || !mode || binding->active)
        return 0;
    if (!hardware->initialize || !hardware->set_mode ||
        !hardware->read_byte || !hardware->write_byte ||
        !mode->width || !mode->height || !rg_pixel_bits(mode->format))
        return 0;
    if (!hardware->initialize(hardware->context)) return 0;
    if (!hardware->set_mode(hardware->context, mode)) {
        if (hardware->shutdown) hardware->shutdown(hardware->context);
        return 0;
    }
    if (!mode->width || !mode->height || !rg_pixel_bits(mode->format)) {
        if (hardware->shutdown) hardware->shutdown(hardware->context);
        return 0;
    }
    vram.read_byte = hardware->read_byte;
    vram.write_byte = hardware->write_byte;
    if (!rg_init_vram(device, mode->width, mode->height,
                      mode->stride, mode->capacity, mode->format,
                      vram, hardware->context)) {
        if (hardware->shutdown) hardware->shutdown(hardware->context);
        return 0;
    }
    device->card_data = hardware->context;
    binding->device = device;
    binding->hardware = hardware;
    binding->active = 1;
    if (hardware->install_accel)
        hardware->install_accel(hardware->context, device);
    return 1;
}

int rg_hw_palette(RG_HwBinding *binding, unsigned index,
                  rg_u8 red, rg_u8 green, rg_u8 blue)
{
    if (!binding || !binding->active || index >= 256 ||
        !binding->hardware->set_palette) return 0;
    binding->hardware->set_palette(binding->hardware->context,
                                   index, red, green, blue);
    return 1;
}

void rg_hw_close(RG_HwBinding *binding)
{
    if (!binding || !binding->active) return;
    if (binding->hardware->shutdown)
        binding->hardware->shutdown(binding->hardware->context);
    /* Guard against accidental reuse after closing hardware. */
    binding->device->surface.pixels = 0;
    binding->device->vram.read_byte = 0;
    binding->device->vram.write_byte = 0;
    binding->device->vram_context = 0;
    binding->device->card_data = 0;
    binding->active = 0;
    binding->device = 0;
    binding->hardware = 0;
}
