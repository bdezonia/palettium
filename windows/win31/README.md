# Windows 3.1 staging display driver (v0028)

`palddi.c` is the stable Win16 DDI entry-point file. `build-palddi.sh` builds
the current `PALSTAGE.DRV` NE module with Open Watcom from the repository root:

```sh
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

The module exports BitBlt @1, ColorInfo @2, Disable @4, Enable @5,
Output @8, Pixel @9, **RealizeObject @10**, ScanLR @12, and palette
operations @22 through @26. `Output` now renders rectangles (6) **and** the two fundamental GDI
synthesis primitives: `OS_SCANLINES` (4) and `OS_POLYLINE` (18). Both use
shared clipping and ROP2 through the existing bank-aware pixel dispatch.
`OS_BEGINNSCAN` (80) / `OS_ENDNSCAN` (81) are no-op bracketing hints.
Other shapes request GDI simulation (-1). In v0028, rectangles,
scanlines and polylines can also target a validated GDI memory BITMAP:
packed mono 1bpp or INDEX8, single-segment only. BitBlt now includes
single-segment memory↔screen and memory↔memory transfers, with DDK
mono/color conversion using the DRAWMODE physical foreground/background
indices. Supported ROPs remain pattern-independent. Pixel and ScanLR
are still screen-only; memory BITMAPs larger than 64 KiB are rejected.
See `README.md` in the project root for test commands and architecture.

**This driver is NOT INSTALLABLE.** `Enable(style=0)` is deliberately disabled;
`Enable(style=1)` returns a candidate GDIINFO with zero advertised drawing or
palette capability bits. The palette and output machinery is only reachable in an *already running*
screen host. The Win16 ET4000AX mode initialization, active GDI PDEVICE,
huge/multiplane memory bitmaps, patterned brushes, text, cursor and window
switch paths remain unfinished. v0027 passed Open Watcom; v0028 Win16 build
awaits maintainer testing. Never select PALSTAGE.DRV
in SYSTEM.INI.

Win16 segmented-memory caveat: the screen framebuffer is larger than 64 KiB;
access it only through the hardware backend's banked byte callbacks, not a
near pointer. `PAL_ScreenHost` must be held in persistent storage, not a small
Win16 stack. If a driver backend is implemented, its I/O and BIOS mode set
belong in platform adapters, not `common/` or the generic ET4000AX backend.

The older `palprobe`, `palcore`, `paltest` and other probe files are retained
for reference. Do not build them into PALSTAGE.DRV or clone the stable DDI
implementation into new numbered production files.
