# Palettium v0039 handoff — transactional native/GDI startup (2026-11-23)

## Maintainer-verified baseline

v0038 was applied over v0037 and fully verified by the maintainer on
Ubuntu/Open Watcom on November 23, 2026. `make clean && make test`,
`Makefile.win31native test`, `Makefile.win31link test`,
`Makefile.screen17 test`, and `sh windows/win31/build-palddi.sh` passed;
`wdump -x tests/exes/PALSTAGE.DRV` confirmed the existing 13 NE exports,
with no compiler/linker warnings reported. The v0038 GDI-owned 20-byte
PDEVICE snapshot, 110-byte GDIINFO metrics, and banked screen lifecycle
all passed host tests. User-validated DOS ET4KTEST INFO CRTC37 reads were
0Dh/0Eh/0Fh for 256/512/1024 KiB across V8.01X, V8.06X, V1.21X
(9/9 in 86Box). Win16 PALDIAG A000h read worked; `[386Enh] display=`
was configured as `*vddvga`, whose SVGA compatibility is not established.

## v0039 changes

1. **Transactional mode+GDI commit:** `pal_et4k_session_start_published()`
   lives in the existing `pal_et4k_session.[ch]`, using the same ET4000AX
   hardware backend and screen host as before. It rejects absent platform
   authorization, malformed plan, CRTC-hint conflict, mismatched mode
   metadata, unknown capacity, null callback/token and ALL 386 Enhanced
   configurations **before any I/O**. The caller supplies a validated,
   atomic GDI commit callback; it may commit only after hardware startup
   and exact GDI PDEVICE token snapshot succeed. Failed publication shuts
   down the binding and restores the earlier mode exactly once.
2. **Win16 transaction wired, not activated:** `palddi.c` now keeps the
   native session and Win16 aperture/BIOS context in persistent storage;
   `pal_win16_activate()` prepares the profile plan, checks Standard-mode
   eligibility and GDI FAR-pointer segment bounds, then calls the same
   transactional coordinator with the checked v0038 FAR PDEVICE publisher.
   A typed C context struct carries the FAR destination intact; no invalid
   FAR-to-untyped `void*` cast. `Disable` checks far-pointer identity and
   closes the native session, allowing its BIOS-restore callback to run.
   **`pal_win16_install_ready()` returns zero**, so `Enable(style=0)` still
   fails with NO mode/port/VRAM writes. No new DDI exports or duplicate
   rendering implementations have been added.
3. Tests for 256, 512, 1024 KiB successful publication; missing permit;
   unsafe enhanced mode; mode-metadata mismatch; GDI commit rejection;
   repeated start/stop; BIOS set failure and rollback. Maintenance test
   asserts mandatory install gate and native-session release remain present.

## Important newly confirmed blocker

The Windows 3.1 DDK says `ExtTextOut` must provide functional text output
and `StrBlt` must be exported for compatibility; the driver also has cursor
responsibilities. None is presently implemented. Therefore **do not enable
style=0 or install PALSTAGE.DRV**. Even Standard mode needs these DDIs;
386 Enhanced mode additionally requires a validated VDD switching contract.
The configured VDD name alone is not permission to change the video mode.

## Verification and user test

Assistant host: `make clean && make test`, all auxiliary Makefile test
recipes, clean v0038 -> v0039 overlay, complete source SHA256 checksums,
ZIP integrity and UTC 0x5455/America-Chicago DOS timestamps: PASS.
**The Win16/Open Watcom v0039 build and any Windows runtime test are NOT
performed yet.** On the maintainer's Linux machine:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal39.zip
make clean && make test
make -f Makefile.win31native test
make -f Makefile.win31link test
make -f Makefile.screen17 test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

The `wdump` output must still contain 13 ordinals. Do not replace the
working Windows display driver; no 86Box installation trial is requested.

## Next: operational DDI completion, not diagnostics

Focus v0040 on correct Win3.x text/font service and raster glyph rendering
(reusing common bitmap/BitBlt), then cursor; verify DDK ABI before adding
exports. Once mandatory DDI services exist, validate Standard-mode hardware
start first on a snapshot-restorable 86Box VM, and separately develop a
VDD-compatible enhanced-mode lifecycle. The global goal remains one shared
C89 raster library + card backends + thin Windows 3.1, WfW 3.11, Windows 95,
Windows 98 adapters. Keep fixed-width tags, stable production filenames,
source-only incremental ZIP patches, `tests/exes/` outputs, full `make clean`
and UTC-correct ZIP timestamps. Preserve all previous tags/checkpoints.
