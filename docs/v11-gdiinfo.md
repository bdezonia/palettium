# Palettium v0.11: Win3.1 GDIINFO ABI

This patch adds an isolated GDIINFO wire-layout candidate and an independent test.
It does NOT turn PALGDI.DRV into an installable display driver. The v0.10
`Enable()` still returns failure, deliberately.

## What is checked

- 16-bit WORD, 32-bit LONG, 16-bit POINT and 110-byte GDIINFO layout.
- Key field offsets: resolution, physical-device size, raster bits, palette.
- Only sample geometry (640 x 480, 8bpp) is produced by the candidate builder.
- No drawing/brush/text/palette capabilities advertised; incomplete physical
  metrics and PDEVICE mean this record MUST NOT be handed to GDI yet.

The original DDK materials provide the field ordering and 3.1 capability
extensions. Sources to compare against:

- https://dos-help.soulsphere.org/ddag31qh.hlp/dispgdiinfo.html
- https://dos-help.soulsphere.org/ddag31qh.hlp/disppalette.html
- https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/
- https://yksofthome.blogspot.com/2011/ (GDIINFO field example)

## Usage

    make -f Makefile.gdiinfo test
    sh windows/win31/build-palcap.sh
    wdump -x windows/win31/PALCAP.DLL

The DLL exports PALETTIUMCAPLAYOUT: when called, it returns 110 on success,
0 on failure. Export presence alone does not prove the function executes.

## Before installation

Must implement and validate a real PDEVICE; set exact GDIINFO mapping/metric
values; implement proper drawing, palette/color translation, cursor support,
DDI exports/calling conventions, and DOS/Windows screen switching. Retain a
known-good VM disk image. Never install PALGDI.DRV or PALCAP.DLL as display.

No changes to common/rgfx.c or ET4000AX backends are introduced.
