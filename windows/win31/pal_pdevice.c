#include <stddef.h>
#include <string.h>
#include "pal_pdevice.h"

int pal_pd_layout_ok(void)
{
    return sizeof(PAL_PDEVICE) == 20u && sizeof(PAL_U32) == 4u &&
           offsetof(PAL_PDEVICE, width) == 6u &&
           offsetof(PAL_PDEVICE, pitch) == 10u &&
           offsetof(PAL_PDEVICE, magic) == 16u;
}

int pal_pd_valid(const PAL_PDEVICE *pd)
{
    if (!pd || !pal_pd_layout_ok()) return 0;
    if (pd->type != 1u || pd->bytes != sizeof(*pd) ||
        pd->magic != PAL_PD_MAGIC || pd->bpp != 8u ||
        pd->reserved != 0u ||
        pd->width == 0u || pd->height == 0u ||
        pd->pitch < pd->width) return 0;
    return pd->state == PAL_PD_READY || pd->state == PAL_PD_ACTIVE;
}

int pal_pd_prepare(PAL_PDEVICE *pd, unsigned width, unsigned height,
                   unsigned pitch)
{
    if (!pd || !pal_pd_layout_ok() || !width || !height ||
        width > 32767u || height > 32767u ||
        pitch < width || pitch > 65535u) return 0;
    /* The 8-bit framebuffer spans more than 64K; no near VRAM pointer. */
    memset(pd, 0, sizeof(*pd));
    pd->type = 1u;
    pd->bytes = (PAL_U16)sizeof(*pd);
    pd->state = PAL_PD_READY;
    pd->width = (PAL_U16)width;
    pd->height = (PAL_U16)height;
    pd->pitch = (PAL_U16)pitch;
    pd->bpp = 8u;
    pd->magic = PAL_PD_MAGIC;
    return 1;
}

int pal_pd_activate(PAL_PDEVICE *pd)
{
    if (!pal_pd_valid(pd) || pd->state != PAL_PD_READY) return 0;
    /* State marker only: the real adapter must initialize hardware first. */
    pd->state = PAL_PD_ACTIVE;
    return 1;
}

int pal_pd_stop(PAL_PDEVICE *pd)
{
    if (!pal_pd_valid(pd)) return 0;
    pd->state = PAL_PD_READY;
    return 1;
}

int pal_pd_capability_candidate(PAL_GDIINFO *info, const PAL_PDEVICE *pd)
{
    if (!pal_pd_valid(pd)) return 0;
    return pal_win31_make_candidate(info, pd->width, pd->height,
                                    (unsigned)sizeof(*pd));
}
