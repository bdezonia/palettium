#include <string.h>
#include "pal_screen_host.h"

int pal_screen_host_start(PAL_ScreenHost *host, RG_Hardware *hardware,
                          unsigned width, unsigned height,
                          PAL_U32 pdevice_token)
{
    RG_Mode mode;
    unsigned i;
    RG_Color color;
    if (!host || !hardware || host->running || !pdevice_token ||
        !width || !height || width > 32767u || height > 32767u)
        return 0;
    /* Win3.x INDEX8 screen requires a writable DAC. The core hardware
     * interface stays optional so other OS adapters need not use palettes. */
    if (!hardware->set_palette) return 0;
    /* Caller zero-initializes host before first use. Avoid overwriting a
     * live binding on failed/repeated starts. */
    memset(&host->pdevice, 0, sizeof(host->pdevice));
    memset(&host->device, 0, sizeof(host->device));
    memset(&host->ddi, 0, sizeof(host->ddi));
    memset(&host->binding, 0, sizeof(host->binding));
    rg_palette_init(&host->palette);
    mode.width = width;
    mode.height = height;
    mode.format = RG_FMT_INDEX8;
    mode.stride = 0;
    mode.capacity = 0;
    if (!rg_hw_open(&host->binding, &host->device, hardware, &mode))
        return 0;
    if (!pal_pd_prepare(&host->pdevice, mode.width, mode.height,
                        mode.stride) ||
        !pal_pd_activate(&host->pdevice) ||
        !pal_screen_ddi_attach(&host->ddi, &host->pdevice,
                               &host->device, pdevice_token)) {
        rg_hw_close(&host->binding);
        memset(&host->ddi, 0, sizeof(host->ddi));
        memset(&host->pdevice, 0, sizeof(host->pdevice));
        return 0;
    }
    /* Initialize exactly the 20 static Windows colors after the mode set.
     * GDI owns the remaining 236 dynamic entries. */
    for (i=0; i<256u; ++i) {
        if (!rg_palette_is_static(i)) continue;
        rg_palette_get_entry(&host->palette, i, &color);
        if (!rg_hw_palette(&host->binding, i,
                           color.red, color.green, color.blue)) {
            pal_screen_ddi_detach(&host->ddi);
            pal_pd_stop(&host->pdevice);
            rg_hw_close(&host->binding);
            return 0;
        }
    }
    host->running = 1;
    return 1;
}

void pal_screen_host_stop(PAL_ScreenHost *host)
{
    if (!host || !host->running) return;
    pal_screen_ddi_detach(&host->ddi);
    pal_pd_stop(&host->pdevice);
    rg_hw_close(&host->binding);
    host->running = 0;
}

int pal_screen_host_bitblt(PAL_ScreenHost *host,
                           PAL_U32 dst_token, int dx, int dy,
                           PAL_U32 src_token, int sx, int sy,
                           int width, int height, unsigned long rop)
{
    if (!host || !host->running) return 0;
    return pal_screen_ddi_bitblt(&host->ddi, dst_token, dx, dy,
                                 src_token, sx, sy, width, height, rop);
}

int pal_screen_host_palette(PAL_ScreenHost *host, unsigned index,
                            rg_u8 red, rg_u8 green, rg_u8 blue)
{
    RG_Color color;
    if (!host || !host->running || !host->binding.hardware ||
        !host->binding.hardware->set_palette ||
        rg_palette_is_static(index) || index >= 256u) return 0;
    color.red = red; color.green = green; color.blue = blue; color.flags = 0;
    if (!rg_hw_palette(&host->binding, index, red, green, blue)) return 0;
    return rg_palette_set_entry(&host->palette, index, color);
}

int pal_screen_host_pixel(PAL_ScreenHost *host, PAL_U32 token,
                          int x, int y, PAL_U32 physical_color,
                          unsigned rop2, PAL_U32 *result)
{
    if (!host || !host->running) return 0;
    return pal_screen_ddi_pixel(&host->ddi, token, x, y,
                                physical_color, rop2, result);
}

int pal_screen_host_colorinfo(PAL_ScreenHost *host, PAL_U32 token,
                               PAL_U32 input, int forward,
                               PAL_U32 *logical, PAL_U32 *physical)
{
    rg_pixel result, index=0;
    if (!host || !host->running || token != host->ddi.screen_token ||
        !logical) return 0;
    if (!rg_palette_color_info(&host->palette, (rg_pixel)input,
                               forward, &result, &index)) return 0;
    *logical = (PAL_U32)result;
    if (forward) *physical = (PAL_U32)index;
    return 1;
}

int pal_screen_host_update_colors(PAL_ScreenHost *host,
                                  unsigned x, unsigned y,
                                  unsigned width, unsigned height,
                                  const unsigned short *table)
{
    if (!host || !host->running || !host->ddi.installed ||
        host->pdevice.state != PAL_PD_ACTIVE ||
        host->ddi.screen.device != &host->device) return 0;
    return rg_palette_remap_rect(&host->device, x, y, width, height, table);
}

int pal_screen_host_scanlr(PAL_ScreenHost *host, PAL_U32 token,
                           int x, int y, PAL_U32 physical,
                           unsigned style, int *found)
{
    if (!host || !host->running) return 0;
    return pal_screen_ddi_scanlr(&host->ddi, token, x, y,
                                 physical, style, found);
}

int pal_screen_host_rectangle(PAL_ScreenHost *host, PAL_U32 token,
                              RG_Rect bounds, const RG_Rect *clip,
                              const RG_Pen *pen, const RG_Brush *brush,
                              unsigned rop2, unsigned background_mode)
{
    if (!host || !host->running || !token || !host->ddi.installed ||
        token != host->ddi.screen_token ||
        !pal_pd_valid(&host->pdevice) ||
        host->pdevice.state != PAL_PD_ACTIVE ||
        host->ddi.pdevice != &host->pdevice ||
        !host->ddi.screen.active ||
        host->ddi.screen.device != &host->device ||
        host->pdevice.width != host->device.surface.width ||
        host->pdevice.height != host->device.surface.height ||
        host->pdevice.pitch != host->device.surface.stride) return 0;
    return rg_gdi_output_rectangle(&host->device, bounds, clip,
                                    pen, brush, rop2, background_mode);
}
