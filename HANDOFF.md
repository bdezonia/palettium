# Palettium — v0028 handoff

Date: 2026-11-23 UTC. Apply incremental `pal28.zip` over a **user-validated
v0027** repository. Stable production filenames, fixed-width v0028 milestone,
C89 common core, binaries under `tests/exes/`, fresh ZIP UTC 0x5455 timestamps
and America/Chicago DOS ZIP local-clock fields are preserved.

## Objective / architecture

Build one reusable four-OS graphics driver framework (Windows 3.1, WfW 3.11,
Windows 95, Windows 98), first getting a genuine Windows 3.1 ET4000AX desktop.
The shared `RG_Device`/`RG_Ops` raster operations stay OS/card-independent.
`windows/win31/` owns the segmented 16-bit display-DDI ABI. The hardware-neutral
`RG_Hardware` API and ET4000AX backend keep read/write banks and palette/mode
capabilities independent from the Windows adapter. Other OS adapters are
planned, not functional.

## Confirmed maintainer v0027 checkpoint

User's Ubuntu/Open Watcom results 2026-11-23: Win31 Pixel/palette/objects,
Output rectangle/scanlines/polyline, all 16 ROP2, ET4000AX banked VRAM,
far-token BitBlt, wcc/wlink and `wdump` with 13 exports all PASS, without
reported Win16 warnings. The exported ordinals remain:
BitBlt @1, ColorInfo @2, Disable @4, Enable @5, Output @8, Pixel @9,
RealizeObject @10, ScanLR @12, SetPalette @22, GetPalette @23,
SetPalTrans @24, GetPalTrans @25, UpdateColors @26.

## New in v0028

- `windows/win31/pal_bitmap.c` validates 32-byte DDK PBITMAP for both 1bpp
  packed monochrome and 8bpp INDEX8, requiring a single segment (total <=
  65535 bytes, bmBits segment offset + byte count <= 65536, non-huge metadata).
  No claims of compatibility for huge GDI bitmap segments or planar formats.
- `pal_bitmap_device.c` binds the checked bitmap to an ordinary RG_Device
  via byte callbacks; `palddi.c` owns Win16 `MK_FP`/segment:offset handling
  and near copies of the 32-byte FAR bitmap header. The shared code never
  dereferences a Win16 far pointer or duplicates the raster engine.
- Output @8 directs OS_RECTANGLE, OS_SCANLINES and OS_POLYLINE to physical
  screen or memory bitmap; `common/rg_gdi_output.c` handles INDEX1 as well
  as INDEX8, retaining ROP2, clipping, hatches and endpoints.
- BitBlt @1 supports same-format 1/8bpp screen/memory and memory/memory,
  plus mono↔color conversion from DRAWMODE physical text/background colors.
  Mono 1 means background, mono 0 means text; the reverse conversion maps
  color equal to background to mono 1, all other colors to mono 0. Only
  pattern-independent ROP3 are handled; unsupported operations return FALSE.
  Memory aliasing uses the SAME RG_Device for correct overlapping copies.
- New `tests/test_win31_mem.c`: header/segment validation, 1bpp packed data,
  RGB/mono conversion, Output rectangles/scanlines/polylines, rejection
  without writes and in-place overlap. ET4000AX integration checks both
  conversion directions across 64 KiB VRAM read/write banks.
- **No new exports**. Source-only small incremental ZIP; README and Win31
  README updated. No change to unrelated OS adapters.

## Validation

Linux host C89 `-Wall -Wextra -Werror` tests PASS:

```sh
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make -f Makefile.bitmap test
make clean && make test
```

An auxiliary host C89 syntax check of `palddi.c` with stub Win16 headers
passed; this does NOT validate Win16 ABI or Open Watcom. **Open Watcom v0028
build is unverified** until the maintainer runs:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal28.zip
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make -f Makefile.bitmap test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

## NOT INSTALLABLE / next actual startup work

`Enable(style=0)` intentionally fails; `PALSTAGE.DRV` must **not** be selected
in SYSTEM.INI. The v0028 memory functionality is only reachable after a
hypothetical successful screen host startup; hardware initialization and GDI
PDEVICE binding are not wired to a real Win16 display driver yet. Major
remaining contracts: >64 KiB segmented memory BITMAPs, memory BitmapBits,
Pixel/ScanLR for memory, patterned brushes, text and cursor services,
GDIINFO metrics/capability truthfulness, 16-bit ET4000AX BIOS/port/A000 mode
setup/restoration, enhanced-mode switching. Prioritize a minimal functioning
Win16 mode/host boot path and essential text/cursor without dropping four-OS
reuse. Test startup first in a recoverable 86Box image, never on a primary OS.

Historical sources: Windows 3.1 DDK BitBlt, BITMAP and display Output
(https://dos-help.soulsphere.org/ddag31qh.hlp/BitBlt.html and
https://dos-help.soulsphere.org/ddag31qh.hlp/dispoutput.html).
