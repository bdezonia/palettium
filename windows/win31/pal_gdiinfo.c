#include <limits.h>
#include <stddef.h>
#include <string.h>
#include "pal_gdiinfo.h"

int pal_win31_layout_ok(void)
{
    return sizeof(PAL_S16) == 2 && sizeof(PAL_U16) == 2 &&
           sizeof(PAL_S32) == 4 && sizeof(PAL_POINT16) == 4 &&
           sizeof(PAL_GDIINFO) == PAL_GDIINFO_BYTES &&
           offsetof(PAL_GDIINFO, dpHorzRes) == 8 &&
           offsetof(PAL_GDIINFO, dpDEVICEsize) == 26 &&
           offsetof(PAL_GDIINFO, dpRaster) == 38 &&
           offsetof(PAL_GDIINFO, dpMLoWin) == 48 &&
           offsetof(PAL_GDIINFO, dpLogPixelsX) == 88 &&
           offsetof(PAL_GDIINFO, dpDCManage) == 92 &&
           offsetof(PAL_GDIINFO, dpCaps1) == 94 &&
           offsetof(PAL_GDIINFO, dpSpotSizeX) == 96 &&
           offsetof(PAL_GDIINFO, dpPalColors) == 104 &&
           offsetof(PAL_GDIINFO, dpPalResolution) == 108;
}

int pal_win31_make_candidate(PAL_GDIINFO *info,
                             unsigned width, unsigned height,
                             unsigned pdevice_bytes)
{
    if (!info || !pal_win31_layout_ok()) return 0;
    if (!width || !height || width > 32767u || height > 32767u ||
        !pdevice_bytes) return 0;
#if UINT_MAX > 65535U
    if (pdevice_bytes > 65535u) return 0;
#endif
    memset(info, 0, sizeof(*info));
    info->dpVersion = PAL_WIN31_GDI_VERSION;
    info->dpTechnology = 1; /* DT_RASDISPLAY */
    info->dpHorzRes = (PAL_S16)width;
    info->dpVertRes = (PAL_S16)height;
    info->dpBitsPixel = 8;
    info->dpPlanes = 1;
    info->dpDEVICEsize = (PAL_U16)pdevice_bytes;
    /* Physical dimensions and mapping metrics need DDK/sample calibration.
     * Keep capability bits, palette support and drawing claims at zero.
     * NOT valid as an operational display-driver GDIINFO yet. */
    return 1;
}
