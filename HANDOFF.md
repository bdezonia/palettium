# Palettium — checkpoint v0024 handoff

Updated for v0024 (Nov 23, 2026 UTC). Base: user-uploaded `pal23cp(1).zip` checkpoint v0023; apply `pal24.zip` over its `palettium/` root.

## Read this first
Palettium is a reusable **C89 vintage graphics-driver building framework** for **Windows 3.1, WfW 3.11, Windows 95, and Windows 98**. Objective: write a small card-specific detection/mode/banking/palette backend, link it with common software rasterization and thin OS adapters, produce four baseline display-driver packages; then override individual drawing primitives with acceleration. Do not turn this into a one-off ET4000 driver or spend months on probes.

Version tags: fixed-width `v0012`, `v0013`, ... next `v0025`. Do not alter previously created Git tags without user's instruction. Deliver small incremental ZIP patches, timestamp them with actual UTC and extended ZIP UTC timestamps, test `unzip` extraction in America/Chicago. Linux test executables belong in `tests/exes/` (gitignored).

## v0024 delta (new)
- The stable Win16 `palddi.c` now exports `Output @8` (proper GDI simulation return `0xFFFF`) and `Pixel @9` (DDK 32-bit return, error `0x80000000`). `BitBlt @1`, `Disable @4`, `Enable @5` remain. `Output` intentionally delegates all styles to GDI; `Pixel` supports physical INDEX8 only, reads and ROP2 writes, and refuses inactive/foreign PDEVICE, invalid coordinates, color, or ROP2. The existing RG_Ops pixel override is honored. The DRAWMODE Rop2 is Win16's first WORD.
- Host tests: `make -f Makefile.win31link test` validates Pixel all 16 ROP2 operations, error behavior, and export checker; `make -f Makefile.screen17 test` additionally tests ET4000-backed Pixel through banked VRAM. All v0023 host suites were rerun in v0024 and passed. **The Open Watcom Win16 build still needs testing on the user's workstation.**
- `tools/pack_utc.py` now encodes DOS ZIP time as America/Chicago wall time and uses 0x5455 extended UTC exact mtime; tests must check extraction timestamps against ZIP UTC field.
- New: `PATCH-v0024.txt` (details). There are no new production source filenames, driver hardware changes, or OS-specific rewrites. Windows 3.1 is **still not installable**; `Enable(style=0)` returns failure. No system.ini changes.

## Validated v0023 baseline
- Linux: `make test` passed all seven default targets from this uploaded snapshot. Additional 13 Makefile test targets also passed (`Makefile.et4k`, `.gdi`, `.gdiblt`, `.gdiinfo`, `.pdevice`, `.bitmap`, `.win31blit`, `.ddi16`, `.screen17`, `.win31ddi`, `.enable22`, `.far20`, `.win31link`); 20 total test targets, not 20 separate subsystems.
- Prior user validation: Open Watcom builds 16-bit DOS ET4000 test, on 86Box with emulated ET4000AX the full-screen 640x480x256 demo drew graphics, VRAM readback PASS, returned to DOS. `PALCORE.DLL` and `PALTEST.EXE` executed graphics self-test under Windows 3.1: PASS.
- v0023 user output: `sh windows/win31/build-palddi.sh` compiles 13 source files and links `tests/exes/PALSTAGE.DRV`; `wdump -x` verifies `BITBLT @1`, `DISABLE @4`, `ENABLE @5` (record in `tests/exes/PALSTAGE.exports.txt` from uploaded snapshot). The correct build script is the final **fix4** version in this snapshot. Earlier scripts duplicated linker `SYSTEM` directives: DO NOT reinstate.
- **`PALSTAGE.DRV` IS NOT INSTALLABLE.** `Enable(style=0)` returns failure and other mandatory DDIs, palette/cursor/text operations and Windows 386 Enhanced Mode display switching are missing. Do not tell user to replace SYSTEM.INI or install it.

## Main source architecture
- `include/rgfx.h`, `common/rgfx.c`: single unified pixel engine; `RG_Device` has `RG_Surface` (width/height, byte stride, capacity, format), `RG_Ops` function pointer overrides, byte-level VRAM callbacks, raw 32-bit pixel type. INDEX1, INDEX2, INDEX4, INDEX8, INDEX16, RGB555, RGB565, RGB888, XRGB8888, ARGB8888. XRGB high byte unused, ARGB high byte alpha storage (no compositing claimed).
- `include/rg_hardware.h`, `common/rg_hardware.c`: backend lifecycle, mode and palette capabilities, optional acceleration.
- `hardware/tseng/et4000ax/et4k.[ch]`: reusable ET4000AX hardware backend; native register 0x3CD independent read/write bank, palette ports, currently **only 640x480 INDEX8**; BIOS-native mode set supplied by platform callback, not VESA. Hardware code must remain OS-independent. `dos/et4k_demo.c` is real 86Box test.
- `hardware/sim_bank/`: bank simulator; reusable Linux tests.
- `common/rg_gdi_rop.c`, `common/rg_gdi_blit.c`, `common/rg_win31_screen.c`: GDI ROP3 and screen blit adaptation.
- `windows/win31/`: PAL_GDIINFO (110-byte layout), private PDEVICE, screen host/DDI routing, consolidated `palddi.c`, `pal_driver_entry.c`. `Enable(style=1)` writes preliminary capability structure; style 0 disabled. `BitBlt` supports narrow screen-to-screen subset; memory bitmap/far access incomplete. `Pixel` supports physical INDEX8 read and ROP2 write through the host bridge; `Output` delegates to GDI. `Disable` currently unbinds.
- Other OS directories are placeholders. Future OS-specific adapters may share implementation but must have distinct builds.

## Concrete next work — v0025
1. Use the actual snapshot code; audit DDK ABI and export ordinals against historical sample, and inspect what an installable 256-color Win3.1 driver really requires. Do not assert that the current candidate GDIINFO is valid.
2. Complete ColorInfo with an explicitly selected/initialized fixed palette and physical-color translations; audit palette-capable (Set/GetPalette/Trans/UpdateColors) requirements before setting RC_PALETTE. Next implement mandatory RealizeObject, bitmap and text/scan/cursor operations with correct ABI and functional GDI simulation. Avoid another standalone probe DLL or numbered production .c file.
3. Implement platform-specific ET4000 mode setting/Win16 VRAM access, and actual Enable(style=0) PDEVICE creation, only when enough mandatory services are implemented. Audit VDD/grabber compatibility in standard and 386 Enhanced Mode; preserve safe standard VGA boot backup.
4. Consider bringing next OS adapter requirements into architecture reviews so Win31 choices don't hardwire card assumptions.
5. Fix two existing Open Watcom W124 warnings only after inspecting expressions; on host `pal_gdiinfo.c:28`, `pal_pdevice.c:29`, comparisons vs 65535 are tautological for 16-bit unsigned. Avoid hiding genuine errors.

## Known cleanup and technical debt
- Top-level `README.md` is stale (still describes v0.2 and INDEX8-only); update it as a separate documented change.
- User upload included compiled DLL/EXE/OBJ, staging linker output, `.err`, `tests/exes`, and a long history of PATCH-* notes. This checkpoint archive omits **generated binaries/outputs only** and leaves all historical source, scripts, and text notes intact. Do not reintroduce `palddi15.c`, `palddi20.c`, `palgdi.c` or their retired scripts.
- `windows/win31` contains older *probe source* (palprobe, palcore, paltest, palcap, etc.) and scripts: keep for reference, but production should converge on stable `palddi.c`/`build-palddi.sh`.
- `tests/exes/` is ignored; main make `clean` does not necessarily remove every supplemental target. Not a blocker.

## Commands
```
make clean && make test
make -f Makefile.et4k test
make -f Makefile.screen17 test
make -f Makefile.win31link test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```
Open Watcom is installed on user's Linux machine, **not assumed installed here**. User can report compiler/linker output.

## Packaging
Source snapshot ZIP is extracted as `palettium/`, with this file `palettium/HANDOFF.md`. The original `src.zip` remains the authoritative full upload. Clean checkpoint deliberately excludes compiled products and caches without altering original repository. UTC timestamp recorded in ZIP extended Unix mtime, ZIP DOS local time encoded for America/Chicago; verify extraction and clock skew. Source manifest with SHA256 hashes is included.
