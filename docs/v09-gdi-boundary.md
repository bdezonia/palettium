# Palettium v0.9 — GDI translation groundwork

This incremental patch intentionally does **not** build a usable display driver.
It adds the first GDI-specific translation component needed by the four future
Windows adapters: ternary raster operations (pattern, source, destination).

## Why this is needed

Palettium's v0.3 two-input BitBlt engine implements a four-bit ROP (S,D).
Windows GDI's ROP3 operations generally depend on three inputs (P,S,D),
represented by an eight-bit Boolean truth table in bits 16..23 of a GDI
raster-operation code. A brush pattern cannot always be ignored.

This library is Win16-safe C89 with no dependency on <windows.h>. It supports
all 256 ROP3 truth tables and recognizes the sixteen two-input operations that do not depend
on the brush/pattern input (and can use existing optimized RG_Ops.blit paths).

Example: SRCCOPY = 0x00CC0020, PATCOPY = 0x00F00021, SRCINVERT = 0x00660046.
ROP3 codes and bitwise operations are **not** the same as Windows ROP2 pen codes.

## What is still needed before PAL.DRV can be installed

* Use the genuine Windows 3.1 DDK definitions and sample implementation for
  GDIINFO, PDEVICE, and the mandatory display entry points; don't fabricate
  their memory layouts. GDIINFO.dpVersion is 0x030A for Windows 3.1.
* Implement the two-phase Enable lifecycle, correct Win16 calling conventions,
  ordinals/exports, cursor and text drawing, bitmap surfaces, clipping,
  physical pen/brush translation, palette handling, and screen restoration.
* Verify expected behavior in Standard Mode before tackling 386 Enhanced Mode
  and its VDD/screen-switching contract.
* Keep the existing ET4000AX backend and the common rasterizer independent of
  Windows display-driver objects; prefer small reusable adapter functions.
* Do **not** install PALCORE.DLL, PALTEST.EXE, or a placeholder .DRV as a
  Windows display driver.

## How to test

    cc -std=c89 -O2 -Wall -Wextra -Werror -Iinclude \
      common/rg_gdi_rop.c tests/test_gdi_rop.c -o gdi-rop-test
    ./gdi-rop-test

The tests cover all 256 ternary functions against independent minterm
reference values, and test Windows raster operation constants.

## Historical references

* Windows 3.1 DDK display graphics functions:
  https://dos-help.soulsphere.org/ddag31qh.hlp/Graphics_Functions.html
* Windows 3.1 GDIINFO:
  https://dos-help.soulsphere.org/ddag31qh.hlp/dispgdiinfo.html
* Windows 3.1 DDK Video Seven based driver example (for understanding ABI,
  **not** for using its VESA hardware layer):
  https://github.com/PluMGMK/vbesvga.drv

Next target: a DDK-accurate display-driver adapter with real GDIINFO/PDEVICE
and safe trial boot on the backed-up Windows 3.1 installation. This is a
larger task than merely exporting Enable/Disable.
