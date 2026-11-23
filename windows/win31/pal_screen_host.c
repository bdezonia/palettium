#include <string.h>
#include "pal_screen_host.h"

int pal_screen_host_start(PAL_ScreenHost *host, RG_Hardware *hardware,
                          unsigned width, unsigned height,
                          PAL_U32 pdevice_token)
{
    RG_Mode mode;
    if (!host || !hardware || host->running || !pdevice_token ||
        !width || !height || width > 32767u || height > 32767u)
        return 0;
    /* Caller zero-initializes host before first use. Avoid overwriting a
     * live binding on failed/repeated starts. */
    memset(&host->pdevice, 0, sizeof(host->pdevice));
    memset(&host->device, 0, sizeof(host->device));
    memset(&host->ddi, 0, sizeof(host->ddi));
    memset(&host->binding, 0, sizeof(host->binding));
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
    if (!host || !host->running) return 0;
    return rg_hw_palette(&host->binding, index, red, green, blue);
}
