# Palettium — checkpoint v0025 handoff

Date: 2026-11-23 UTC. Base: validated v0024 (apply `pal25.zip` over
`pal24.zip` / working v0024 repository). See `PATCH-v0025.txt` and root
`README.md` for changed files and quick usage. Use fixed-width tags
`v0025`, `v0026`, ...; preserve existing tags and stable production source
names. Source-only incremental ZIP with verified UTC timestamps and test
binaries under `tests/exes/`.

## Objective and architecture

Reusable C89 graphics-driver building framework for **Win3.1, WfW3.11,
Win95, Win98**. Goal is a small hardware backend for detection/mode/banking/
palette combined with common raster fallbacks and thin OS-specific display
DDI adapters. **Get an actual Win3.1 ET4000AX display driver running first**;
allow performance overrides later. Do not fork code per OS or make a one-off
ET4000 implementation. The WfW/Win95/Win98 adapters are placeholders.

Key components: `include/rgfx.h` and `common/rgfx.c` (10 format raw raster
engine and RG_Ops overrides), `include/rg_hardware.h` / `common/rg_hardware.c`
(hardware abstraction), `hardware/tseng/et4000ax/et4k.[ch]` (register 0x3CD
read/write banks, DAC, 640x480x8 mode via platform callback), `common/
rg_gdi_*.c` (GDI ROP/Blt), `windows/win31/` (real Win16 ABI staging).
No Win16 far pointer or BIOS call belongs in the common/card backend.

## v0025 new work

- `include/rg_palette.h`, `common/rg_palette.c`: shared INDEX8 palette
  state and 20 static Windows RGB colors in slots 0-9 and 246-255; 236
  dynamic colors; GDI logical-to-physical and deterministic inverse maps,
  exact static-vs-dynamic ColorInfo conversions, rectangle reindexing via
  existing RG_Ops. Uses long arithmetic for squared RGB differences to
  avoid **16-bit int overflow** in Win16 builds. No new OS-specific
  duplicated renderer.
- Extended existing `PAL_ScreenHost` to hold this palette and program 20
  reserved DAC colors **after** successful hardware/PDEVICE mode setup.
  Requires palette callback for Win3.x INDEX8 mode but leaves the generic
  RG_Hardware palette callback optional for other OS adapters. Dynamic
  palette entry writes pass through the existing hardware palette hook.
- `windows/win31/palddi.c` now supplies 12 DDI exports:
  BitBlt @1, ColorInfo @2, Disable @4, Enable @5, Output @8, Pixel @9,
  ScanLR @12, SetPalette @22, GetPalette @23, SetPalTrans @24,
  GetPalTrans @25, UpdateColors @26. Palette DDIs with no PDEVICE
  argument are inactive until the host is running. ColorInfo/ScanLR check
  the Win16 far PDEVICE identity; real Win16 Far/near data stays in adapter.
- `ScanLR`: physical INDEX8 color test, scan left/right, equal/unequal,
  -1 for not found, 0x8000 at Win16 boundary for invalid input. `UpdateColors`
  honors a 256-word phys-to-phys map and uses hardware/RG_Ops pixel path.
- Updated `README.md` and Win31 README to reflect actual current work.
- Fixed the two tautological comparisons from 16-bit Open Watcom warnings
  with `UINT_MAX` preprocessor guards while preserving host overflow checks.

## Test status

- v0024 user Open Watcom validation: linked NE `PALSTAGE.DRV`; `wdump` showed
  BITBLT @1, DISABLE @4, ENABLE @5, OUTPUT @8, PIXEL @9. v0024 host
  Pixel all 16 ROP2, invalid token/bounds, override, screen17 ET4000 PASS.
- v0025 host C89 (Ubuntu/gcc in assistant environment): `make clean &&
  make test`, Makefiles .et4k/.gdi/.gdiblt/.gdiinfo/.pdevice/.bitmap/
  .win31blit/.ddi16/.screen17/.win31ddi/.enable22/.win31link all PASS.
  v0025 new unit covers static/dynamic RGB, ColorInfo and indexed mapping,
  duplicate inverse mapping, validation-before-write for UpdateColors.
  v0025 ET4000AX host test confirms 20 DAC reserved entries, DAC 6-bit
  values, palette write, ScanLR and UpdateColors crossing 64K banks.
- **v0025 Win16 Open Watcom link is not tested here**: tools are not
  installed in assistant environment. User should run commands below and
  send full output. Export checker updated for 12 ordinals, not real
  loader execution proof. The GDIINFO layout is a 110-byte candidate, not
  validated operational capability metrics.

## NOT INSTALLABLE — safe boundary remains

`PALSTAGE.DRV` still deliberately fails `Enable(style=0)`; `Enable(style=1)`
produces a preliminary GDIINFO, with no RC_PALETTE claim. The entrypoints
can operate only when a host is already hardware-bound; there is **no
production Win16 ET4000 mode init / PDEVICE installation**. RealizeObject
pens/brushes, memory bitmap support, GDI text, cursor, other required DDI,
and enhanced-mode/standard-mode screen switching are not yet provided. Do
not tell user to edit SYSTEM.INI, install PALSTAGE.DRV, or remove the
Enable(style=0) safety gate prematurely.

## Most important next development task — v0026+

1. Audit actual Windows 3.1 DDK `RealizeObject`/brush/pen physical layouts
   and the GDI simulation contract for Output/BitBlt. Implement functional
   minimal physical object realization and the bitmap/text fallback paths,
   tested with strict Win16 ABI, rather than stubs that claim success.
2. Once mandatory services are ready, integrate platform-specific ET4000AX
   mode initialization, banked A0000h far aperture I/O, PDEVICE binding,
   screen enable/disable, and a safe VGA fallback for 86Box startup.
3. Audit Enhanced Mode VDD/grabber behavior; validate Windows 3.1 startup
   on disposable emulator image before broad install recommendations.
4. Keep 4-OS reuse in view. The palette/shared renderer are OS-neutral;
   Win3.x far pointers and GDI ordinals stay in the Win31 adapter.

## User commands

Use Downloads path explicitly (previous v0024 instructions forgot it):

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal25.zip
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Optional full test: `make clean && make test`. Expect 12 DDI exports.
No generated `.DRV`, `.OBJ` or host executable in incremental ZIP. User's
Open Watcom is installed on their Linux machine, not available here.

## Historical milestones

v0023: linked PALSTAGE.DRV staging exports @1/@4/@5. v0024: @8 Output
GDI simulation fallback and @9 Pixel (all 16 ROP2), confirmed on user
machine. v0025: ColorInfo, ScanLR, static palette and 5 palette operations.
Source README through v0023 was stale; rewritten here. Keep historical
`PATCH-*` source notes, avoid reintroducing retired numbered palddi files.

Historical reference: Windows 3.0/3.1 DDK Display Palette section, ScanLR
and ColorInfo reference, and Windows 3.0 DDK adaptation guide via PCjs.
No additional GDIINFO palette capability flags until installable.
