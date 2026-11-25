#ifndef PAL_GDIINFO_H
#define PAL_GDIINFO_H
/* Win3.x GDIINFO wire layout. Keep separate from Win32 winddi.h GDIINFO.
 * All fields are 16-bit words except two 32-bit spot sizes.
 * POINT fields use two 16-bit signed coordinates.
 * Compilers must pass the layout test before use in a GDI adapter.
 */
typedef signed short PAL_S16;
typedef unsigned short PAL_U16;
/* 16-bit Watcom uses 32-bit long; LP64 Linux uses 32-bit int. */
#if defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8
typedef signed int PAL_S32;
#else
typedef signed long PAL_S32;
#endif
#pragma pack(push, 1)
typedef struct PAL_POINT16 { PAL_S16 x, y; } PAL_POINT16;
typedef struct PAL_GDIINFO {
    PAL_S16 dpVersion, dpTechnology, dpHorzSize, dpVertSize;
    PAL_S16 dpHorzRes, dpVertRes, dpBitsPixel, dpPlanes;
    PAL_S16 dpNumBrushes, dpNumPens, futureuse, dpNumFonts;
    PAL_S16 dpNumColors;
    PAL_U16 dpDEVICEsize;
    PAL_U16 dpCurves, dpLines, dpPolygonals, dpText, dpClip, dpRaster;
    PAL_S16 dpAspectX, dpAspectY, dpAspectXY, dpStyleLen;
    PAL_POINT16 dpMLoWin, dpMLoVpt, dpMHiWin, dpMHiVpt;
    PAL_POINT16 dpELoWin, dpELoVpt, dpEHiWin, dpEHiVpt;
    PAL_POINT16 dpTwpWin, dpTwpVpt;
    PAL_S16 dpLogPixelsX, dpLogPixelsY, dpDCManage, dpCaps1;
    PAL_S32 dpSpotSizeX, dpSpotSizeY;
    PAL_S16 dpPalColors, dpPalReserved, dpPalResolution;
} PAL_GDIINFO;
#pragma pack(pop)

#define PAL_GDIINFO_BYTES 110u
#define PAL_WIN31_GDI_VERSION 0x030Au

/* Capability record for a staging 8bpp INDEX8 driver. The module does not
 * become installable solely because GDIINFO is coherent; live startup is
 * separately gated and may fail for unsupported bitmaps, fonts or hardware.
 * Do not set RC_DEVBITS/RC_STRETCHBLT until the matching DDIs exist. */
#define PAL_RC_BITBLT        0x0001u
#define PAL_RC_GDI20_OUTPUT  0x0010u
#define PAL_RC_DI_BITMAP     0x0080u
#define PAL_RC_PALETTE       0x0100u
#define PAL_TC_OP_CHARACTER  0x0001u
#define PAL_TC_RA_ABLE       0x2000u
#define PAL_CP_RECTANGLE     0x0001u
#define PAL_LC_POLYLINE     0x0002u
#define PAL_PC_RECTANGLE    0x0002u

int pal_win31_make_candidate(PAL_GDIINFO *info,
                             unsigned width, unsigned height,
                             unsigned pdevice_bytes);
/* Fails unless the compiler's structure layout matches documented offsets. */
int pal_win31_layout_ok(void);
#endif
