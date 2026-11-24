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
    /* Windows 3.1 DDK: supply coherent standard screen mapping metrics.
     * 96 logical pixels per inch is the conventional VGA font scale.
     * Integer rounded physical dimensions are descriptive only; these
     * do NOT imply a verified monitor size or readiness to install.
     * Mapping pairs express exactly one inch at 96 logical pixels.
     * Positive logical Y / negative viewport Y follows Win3.x DDK.
     * Keep every coordinate within signed 16-bit range. */
    info->dpLogPixelsX = 96;
    info->dpLogPixelsY = 96;
    info->dpHorzSize = (PAL_S16)(((unsigned long)width*254UL+480UL)/960UL);
    info->dpVertSize = (PAL_S16)(((unsigned long)height*254UL+480UL)/960UL);
    info->dpAspectX = 100;
    info->dpAspectY = 100;
    info->dpAspectXY = 141;
    info->dpStyleLen = 282;
    info->dpMLoWin.x = info->dpMLoWin.y = 254;
    info->dpMHiWin.x = info->dpMHiWin.y = 2540;
    info->dpELoWin.x = info->dpELoWin.y = 1000;
    info->dpEHiWin.x = info->dpEHiWin.y = 10000;
    info->dpTwpWin.x = info->dpTwpWin.y = 1440;
    info->dpMLoVpt.x = info->dpMHiVpt.x = 96;
    info->dpELoVpt.x = info->dpEHiVpt.x = 96;
    info->dpTwpVpt.x = 96;
    info->dpMLoVpt.y = info->dpMHiVpt.y = -96;
    info->dpELoVpt.y = info->dpEHiVpt.y = -96;
    info->dpTwpVpt.y = -96;
    /* Physical dimensions and mapping metrics need DDK/sample calibration.
     * Keep capability bits, palette support and drawing claims at zero.
     * NOT valid as an operational display-driver GDIINFO yet. */
    return 1;
}
