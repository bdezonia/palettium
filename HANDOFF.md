# Palettium — v0027 handoff

Date: 2026-11-23 UTC. Incremental **v0027**, apply `pal27.zip` over a
**user-validated v0026** checkout. Production names stay stable, milestone
numbers fixed-width, generated executables stay in `tests/exes/`.
The patch is a fresh ZIP with correct Chicago-local DOS timestamps and UTC
0x5455 extras, source files only.

## Four-OS architecture and priority

One C89 reusable graphics framework for Windows 3.1, WfW 3.11, Windows 95
and Windows 98. First target is a real Windows 3.1 ET4000AX desktop.
The generic `common/` drawing operations use `RG_Device`/`RG_Ops`, the
hardware-neutral RG_Hardware interface and card backend `et4k.c`.
Win3.1 segmented-far DDI pointers stay in `windows/win31/palddi.c`;
card-specific port access and mode operations stay outside the common core.
Other three OS adapters remain planned, not implemented.

## Validated v0026 checkpoint

Maintainer's Open Watcom and Ubuntu test results 2026-11-23: Pixel/palette,
pen/brush realization, clipped rectangle output, far-token BitBlt,
ET4000AX banked VRAM and all 13 Win3.x exports PASS; wcc/wlink PASS without
reported warnings. The 13 exports are BitBlt @1, ColorInfo @2,
Disable @4, Enable @5, Output @8, Pixel @9, RealizeObject @10,
ScanLR @12, SetPalette @22, GetPalette @23, SetPalTrans @24,
GetPalTrans @25, UpdateColors @26. Root README was updated in v0025.

## v0027 changes

- **Output @8 implements GDI-mandatory screen primitives** OS_SCANLINES=4
  and OS_POLYLINE=18, while retaining OS_RECTANGLE=6. The DDK requires
  these two foundation primitives so GDI can synthesize unsupported shapes.
  No additional exports are added (still 13).
- OS_SCANLINES wire format: `POINT[0].y` is the common scan Y;
  each remaining `POINT` stores one [left,right) pair in its x/y fields.
  Preflight all pairs to reject malformed input before any writes.
  Brush if given, otherwise pen; hats with transparent/opaque background;
  all 16 binary ROP2s; clipping through the screen framebuffer.
- OS_POLYLINE draws connected nominal 1px pen segments, excludes the
  endpoint of each segment to avoid XOR double writes at joins. Bresenham
  uses `long` (32-bit on Win16) for extreme signed-16 point differences.
  No device-specific duplicate raster path. OS_BEGINNSCAN/ENDNSCAN are
  safe no-op hints for a validated live screen.
- A shared clip calculator and hardware pixel dispatch serve all three
  shapes. Host `PAL_ScreenHost` centralizes lifecycle/token validation.
- New host regression `tests/test_win31_lines.c` covers all 16 ROP2 values,
  brush/pen selection, hatches, diagonal/steep/reversed lines, clipping,
  exclusive endpoints, multi-segment XOR and coordinate extremes.
  ET4000AX integration verifies real bank crossing by both primitives.
- `README.md`, this handoff, Win31 README and patch notes updated.
  `CHECKSUMS.sha256` is full-source-tree SHA256; incremental ZIP is small.

## Validation and next maintainer test

Linux host C89 `-Wall -Wextra -Werror` test targets:

```sh
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make clean && make test
```

All tested successfully in the creation environment. **Open Watcom is not
installed in the assistant's environment**: v0027 Win16 compile/link is
UNVERIFIED until the maintainer runs:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal27.zip
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Expect 13 unchanged DDI exports, with Output @8 enhanced in behavior.

## NOT INSTALLABLE: Enable(style=0) remains gated

`PALSTAGE.DRV` is a staging NE Win16 DLL that links but **must not be
selected as the Windows display driver**. `Enable(style=0)` always fails;
`Enable(style=1)` returns only preliminary 110-byte GDIINFO with raster,
bitmap, clipping and palette capabilities unadvertised. The new lines
are screen-only and do **not** satisfy all DDK obligations for drawing to
GDI-owned 1bpp memory bitmaps. Critical remaining gaps: monochrome/color
memory BITMAP operations, fonts/text fallback, cursor services, GDI-owned
PDEVICE lifecycle, real Win16 ET4000AX BIOS/ports/A000 banking, mode restore
and 386 enhanced-mode coordination. Do not change SYSTEM.INI yet.

## Recommended v0028 direction

Implement **memory BITMAP drawing and blit contracts** so all foundational
Output operations work on both screen and memory bitmaps. Prefer a small
adapter for GDI's bitmap header and reuse `RG_Device`/`common` software
algorithms, not a second raster engine. In parallel, map the real Win16
hardware/mode lifecycle and test first boot in recoverable 86Box.
Advance actual `Enable(style=0)` only when these responsibilities are met.

References (historical DDK):
- https://dos-help.soulsphere.org/ddag31qh.hlp/Output.html
- https://dos-help.soulsphere.org/ddag31qh.hlp/dispoutput.html
- https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/
