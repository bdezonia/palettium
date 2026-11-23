#include <stdio.h>
#include <string.h>
#include "pal_gdiinfo.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    PAL_GDIINFO g;
    CHECK(pal_win31_layout_ok());
    CHECK(!pal_win31_make_candidate((PAL_GDIINFO *)0,640,480,64));
    CHECK(!pal_win31_make_candidate(&g,0,480,64));
    CHECK(!pal_win31_make_candidate(&g,640,480,0));
    CHECK(!pal_win31_make_candidate(&g,640,480,65536u));
    memset(&g, 0xA5, sizeof g);
    CHECK(pal_win31_make_candidate(&g,640,480,64));
    CHECK(g.dpVersion == 0x30A && g.dpTechnology == 1);
    CHECK(g.dpHorzRes == 640 && g.dpVertRes == 480);
    CHECK(g.dpBitsPixel == 8 && g.dpPlanes == 1);
    CHECK(g.dpDEVICEsize == 64);
    CHECK(g.dpRaster == 0 && g.dpPalColors == 0 && g.dpPalReserved == 0);
    CHECK(g.dpLines == 0 && g.dpText == 0 && g.dpClip == 0);
    printf("PASS: Win31 GDIINFO 110-byte ABI, offsets, candidate builder (not installable)\n");
    return 0;
}
