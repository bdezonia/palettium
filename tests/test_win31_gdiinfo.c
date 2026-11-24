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
    CHECK(g.dpRaster==(PAL_RC_BITBLT | PAL_RC_GDI20_OUTPUT | PAL_RC_PALETTE));
    CHECK(g.dpNumBrushes==20 && g.dpNumPens==20 && g.dpNumColors==20);
    CHECK(g.dpNumFonts==0 && g.dpDCManage==4);
    CHECK(g.dpPalColors==256 && g.dpPalReserved==20 && g.dpPalResolution==18);
    CHECK(g.dpLines==PAL_LC_POLYLINE && g.dpPolygonals==PAL_PC_RECTANGLE);
    CHECK(g.dpText==0x2001u && g.dpClip==PAL_CP_RECTANGLE);
    CHECK(!(g.dpRaster&0x8000u)); /* no RC_DEVBITS */
    CHECK(g.dpLogPixelsX==96 && g.dpLogPixelsY==96);
    CHECK(g.dpHorzSize==169 && g.dpVertSize==127);
    CHECK(g.dpAspectX==100 && g.dpAspectY==100 &&
          g.dpAspectXY==141 && g.dpStyleLen==282);
    CHECK(g.dpMLoWin.x==254 && g.dpMLoWin.y==254 &&
          g.dpMLoVpt.x==96 && g.dpMLoVpt.y==-96);
    CHECK(g.dpMHiWin.x==2540 && g.dpMHiVpt.x==96 &&
          g.dpMHiVpt.y==-96);
    CHECK(g.dpELoWin.x==1000 && g.dpELoVpt.x==96 &&
          g.dpELoVpt.y==-96);
    CHECK(g.dpEHiWin.x==10000 && g.dpEHiVpt.x==96 &&
          g.dpEHiVpt.y==-96);
    CHECK(g.dpTwpWin.x==1440 && g.dpTwpVpt.x==96 &&
          g.dpTwpVpt.y==-96);
    CHECK(pal_win31_make_candidate(&g,640,400,20));
    CHECK(g.dpHorzSize==169 && g.dpVertSize==106 &&
          g.dpLogPixelsX==96 && g.dpDEVICEsize==20);
    CHECK(pal_win31_make_candidate(&g,32767u,32767u,20));
    CHECK(g.dpHorzSize>0 && g.dpVertSize>0);
    printf("PASS: Win31 GDIINFO 110-byte ABI, offsets, mapping metrics and candidate builder (not installable)\n");
    return 0;
}
