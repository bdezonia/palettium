# Palettium — v0026 handoff

Date: 2026-11-23 UTC. Incremental **v0026**, apply `pal26.zip` over a
**validated v0025** working tree. Stable production filenames, fixed-width
milestones and tests under `tests/exes/` are preserved. The source ZIP has
correct UTC timestamp extras and America/Chicago DOS timestamps.

## Objective and architecture

Build one reusable C89 graphics driver framework targeting **Windows 3.1,
WfW 3.11, Windows 95 and Windows 98**. First priority: genuinely running
Windows 3.1 ET4000AX display driver. Shared common/ renderer + RG_Ops
hardware overrides + card backend (`et4k.c`) + OS-specific DDI adapter.
Only the Win31 adapter knows the segmented/far pointer/GDI ABI; no OS code
or duplicated pixel drawing in the ET4000 backend. Windows 95/98 adapters
remain placeholders. `PALSTAGE.DRV` is **NOT INSTALLABLE**.

## User-confirmed validated baseline v0025

User ran all Win31 Pixel/palette/far-token/ET4000 banked VRAM tests under
Linux and built the 12-export Win16 PALSTAGE.DRV using Open Watcom.
`wdump` confirmed BITBLT @1, COLORINFO @2, DISABLE @4, ENABLE @5,
OUTPUT @8, PIXEL @9, SCANLR @12, and SET/GETPALETTE @22/@23,
SET/GETPALTRANS @24/@25, UPDATECOLORS @26. Previous W124 compiler warnings
were absent. `Enable(style=0)` still deliberately rejects initialization.

## New in v0026

- **RealizeObject @10**: DDK object protocol (size query, creation, resource-
  free deletion) for `OBJ_PEN` and `OBJ_BRUSH`, Win3.x 10-byte LOGPEN and
  12-byte DDK LOGBRUSH with 32-bit hatch background. The adapter copies
  Win16 far-memory fields; core `rg_objects` normalizes to GDI-owned physical
  8-byte pens and 14-byte brushes. Pens are nominal solid 1px or null; brushes
  solid/hollow/hatched (six patterns; 8px phase), physical palette mapping.
  Unsupported `OBJ_FONT` and `OBJ_PBITMAP` return 0 (RC_DEVBITS unclaimed).
  Pattern brushes are currently unsupported; never claim realization.
- **Output @8 OS_RECTANGLE (style=6)**: actual INDEX8 rendering, half-open
  bounds, clipping, transparent/opaque hatched background, pen border and
  brush interior, all 16 ROP2 operations. Existing RG_Ops pixel overrides
  used; drawmode/objects marshalled at Win16 boundary. Other Output styles
  still return -1 for GDI simulation. Border pixels get ROP exactly once.
- Pixel ROP2 shares the single common implementation in `rg_gdi_rop.c`.
- Screen host checks PDEVICE identity/lifecycle before output writes.
- Host C89 unit tests cover exact DDK layout, physical objects, hatches,
  translation and ROP2. ET4000AX host integration verifies rectangle
  drawing across 64KiB bank boundary and invalid-token refusal.
- New exports count **13**, with REALIZEOBJECT @10. Build, ordinal check,
  README and changelog updated. No numbered duplicate production sources.

## Validation and open tests

Assistant host regression: `make -f Makefile.win31link test`, `.screen17`,
`.win31ddi` and all project Makefile test targets PASS on Linux/gcc C89
`-Wall -Wextra -Werror`. Open Watcom `wcc`/`wlink` **not installed in this
workspace**, so **v0026 Win16 compilation and linking are unverified**;
the user must run them on the maintainer's machine. The v0025 Win16 binary
link was user-confirmed, not v0026.

## NOT INSTALLABLE — DO NOT REMOVE SAFETY GATE

`Enable(style=0)` returns failure. `Enable(style=1)` emits only preliminary
110-byte GDIINFO without drawing/palette capability claims. No production
Win16 ET4000AX BIOS mode set, port/aperture callbacks, genuine GDI-owned
PDEVICE lifecycle, cursor, bitmap/memory DC or text drawing. GDI-required
OS_SCANLINES / OS_POLYLINE need implementing even though rectangles now draw.
Pattern brushes (GDI common) need memory bitmap support. Do not change
Windows SYSTEM.INI or install PALSTAGE.DRV yet.

## Most important next step: v0027+

Implement GDI-required `Output(OS_SCANLINES)` and `Output(OS_POLYLINE)`
against shared raster with correct pen/brush/memory DC contracts. Investigate
minimal memory bitmap and text fallback (`ExtTextOut` / `StrBlt`) prior to
real `Enable(style=0)`. In parallel design actual Win16 ET4000AX port I/O,
banked far A000 aperture and native mode init with safe restoration. Test in a
recoverable 86Box image, including standard/enhanced-mode VDD/grabber concerns.
Keep Win95/Win98 adapter reuse; no one-off ET4000AX graphics duplication.

## Maintainer commands

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal26.zip
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Optional broader suite: `make clean && make test`; other Makefiles also have
`test` targets. Expect 13 NE exports including `REALIZEOBJECT @10`.
Check `PATCH-v0026.txt`, `README.md`. ZIP contains only changed source files,
not binaries, and checksum manifest covers all repository source files.
