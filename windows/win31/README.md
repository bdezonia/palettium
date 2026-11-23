# Windows 3.1 staging display driver (v0031)

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
switch paths remain unfinished. v0029 passed Open Watcom; v0030 failed while compiling `pal_et4k_session.c` because the ET4K include directory was absent. v0031 fixes this include path and adds the ET4K backend to the Win16 link list; maintainer verification of the v0031 build is pending. Never select PALSTAGE.DRV
in SYSTEM.INI.

Win16 segmented-memory caveat: the screen framebuffer is larger than 64 KiB;
access it only through the hardware backend's banked byte callbacks, not a
near pointer. `PAL_ScreenHost` must be held in persistent storage, not a small
Win16 stack. If a driver backend is implemented, its I/O and BIOS mode set
belong in platform adapters, not `common/` or the generic ET4000AX backend.

The older `palprobe`, `palcore`, `paltest` and other probe files are retained
for reference. Do not build them into PALSTAGE.DRV or clone the stable DDI
implementation into new numbered production files.

## Mode-selection status (v0031)

The shared ET4000AX backend accepts 640x400x256 on verified 256 KiB cards
(Tseng BIOS mode 2Fh), and 640x480x256 on cards with at least 512 KiB
(mode 2Eh). Optional read-only VRAM-capacity and BIOS-mode-query platform
callbacks support validation; actual mode changes must be acknowledged by
`set_mode`. Current Win16 `Enable(style=1)` STILL returns the old hardcoded
640x480 **candidate** and `Enable(style=0)` still fails. Native Windows
mode programming, 256K-specific GDIINFO discovery, A000 aperture mapping,
planar 16-color output and enhanced-mode VDD coordination are not done.
This new catalog is useful in host integration but does NOT make the NE
module appropriate for installation.

## Native adapter checkpoint (v0031)

`pal_et4k_win16.c` is **real Win16 port/aperture/BIOS adapter code**, not an
active DDI service. It resolves the protected-mode selector exported by
KERNEL (`__A000H`, ordinal 174), rather than illegally using `0xA000` as
a segment selector. It provides the byte I/O callbacks of `ET4K_Port` and
uses BIOS INT 10h for mode selection, with a pre-mode save and attempted
restore. The BIOS mode is read back via AH=0Fh; some ROMs may report an
unexpected value, in which case the adapter refuses and attempts rollback.
The BIOS/selector paths have not yet run inside Windows.

`pal_et4k_session.c` is a platform-independent GDI/capability preflight
and screen-host lifecycle on top of that same ET4000AX backend. It requires
a verified capacity; Win16 provides **no automatic size detector** at this
checkpoint, and no DDI entry point calls the adapter or coordinator. The
current `Enable(style=1)` still returns the original 640x480 development
candidate, NOT a negotiated geometry; style=0 still fails. A working
display driver additionally needs GDI's PDEVICE pointer bound correctly,
capacity configuration, mouse cursor, text, and 386-enhanced VDD cooperation
(including VDDsetaddresses for packed SVGA on Win3.1). **Do not install.**

## v0031 Win16 build repair and cleanup

`build-palddi.sh` now supplies `-i=hardware/tseng/et4000ax` to Open
Watcom and compiles `hardware/tseng/et4000ax/et4k.c` into the staging
module. The previously reported missing `et4k.h` caused cascading
`ET4K_Port`/`ET4K_State` errors; missing `et4k.c` would also have blocked
linking. This has **not** been verified with the maintainer's Open Watcom
toolchain yet. The driver still deliberately refuses `Enable(style=0)`.

From the project root, `make clean` now deletes all generated artifacts
under `tests/exes/` and `build/`, plus historical Win16 probe DLL, EXE and
OBJ files, without removing `.LNK`, `.c`, or `.h` sources.
