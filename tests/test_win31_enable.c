#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_enable.h"
#include "pal_pdevice.h"

int main(void)
{
    PAL_GDIINFO info;
    PAL_GDIINFO before;
    assert(pal_enable_capability(&info, 640u, 480u));
    assert(sizeof(info) == 110u);
    assert(info.dpVersion == PAL_WIN31_GDI_VERSION);
    assert(info.dpHorzRes == 640 && info.dpVertRes == 480);
    assert(info.dpBitsPixel == 8 && info.dpPlanes == 1);
    assert(info.dpDEVICEsize == sizeof(PAL_PDEVICE));
    assert(info.dpRaster==(PAL_RC_BITBLT|PAL_RC_GDI20_OUTPUT|PAL_RC_DI_BITMAP|PAL_RC_PALETTE));
    assert(info.dpPalColors == 256 && info.dpPalReserved == 20);
    before = info;
    assert(!pal_enable_capability(0, 640, 480));
    assert(!pal_enable_capability(&info, 0, 480));
    assert(!pal_enable_capability(&info, 640, 0));
    assert(!pal_enable_capability(&info, 32768u, 480u));
    assert(memcmp(&info, &before, sizeof(info)) == 0);
    puts("PASS: Win31 Enable style-1 candidate, 110-byte layout, safe rejection");
    return 0;
}
