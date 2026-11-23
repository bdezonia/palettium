# Palettium v0032 handoff — safe Win3.1 selector/BIOS diagnostic (2026-11-23 UTC)

## Verified v0031 baseline

User applied pal31.zip over v0030; `make clean && make test`, 7 shared
raster/hardware checks plus the build-maintenance test, every requested
auxiliary Makefile target and the Open Watcom Windows 3.1 `PALSTAGE.DRV`
compile+link **PASSED** on the user workstation. No warnings reported.
`wdump -x tests/exes/PALSTAGE.DRV` showed all 13 expected DDI exports.
The v0030 ET4K include/link errors are fixed, and `make clean` now removes
all generated output. User also successfully ran ET4KTEST in 86Box in DOS
at 256/512/1024 KiB VRAM (visual correctness at all three). None of this
yet validates a live Windows 3.1 display-driver lifecycle.

## v0032 deliverable

- NEW `windows/win31/paldiag.c`: standalone Win16 GUI diagnostic.
  Queries `GetModuleHandle("KERNEL")`, `GetProcAddress("__A000H")`,
  and optionally `GetSelectorBase`/`GetWinFlags`, displaying both high and
  low words of the returned export address and the available bases. The
  native adapter in `pal_et4k_win16.c` assumes the LOW word is the selector;
  **do not adopt that assumption for actual video access until confirmed**.
  No A000h dereference, I/O port writes, driver load, palette update or
  video-mode set is performed.
- After the initial report, a second message box asks permission for a
  read-only `INT 10h AH=0Fh` current-mode query (reports AL/AH/BH). The
  default safe path is pressing **No**. In enhanced mode Windows can
  virtualize the result; do not interpret it as physical BIOS validation.
- NEW stable script `windows/win31/build-paldiag.sh` generates
  **`tests/exes/PALDIAG.EXE`** with Open Watcom's `wcl` Win16 GUI target.
  No new driver DDI exports, no changes to PALSTAGE.DRV or its link recipe.
- NEW host test `tests/test_win31_diag.c` compiles the actual paldiag.c
  against 2 lightweight mock Win16 API headers. Covers no-BIOS default,
  explicit opt-in, missing KERNEL/export/function and reported selector
  bases. `Makefile.win31diag` runs it; root `make test` includes it.
  `tests/test_build_maintenance.py` checks the new diagnostic binaries
  are removed by root `make clean` (sources remain). README and Win31
  README are current.

## Build and run

Apply v0032 patch over v0031; first build host tests and the regular
Win16 staging driver to ensure nothing regressed:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal32.zip
make clean && make test
make -f Makefile.win31diag test
sh windows/win31/build-palddi.sh
sh windows/win31/build-paldiag.sh
wdump -x tests/exes/PALSTAGE.DRV
```

**User Open Watcom Win16 compilation/link of PALDIAG.EXE NOT YET TESTED.**
Copy the resulting `tests/exes/PALDIAG.EXE` into a disposable **Windows
3.1** VM. Run it as an ordinary Win16 program. Capture full first dialog,
plus optional BIOS readback. Start in a recoverable VM under the existing
display driver; do not replace that driver. Test Windows standard mode
and enhanced mode separately only if both can be configured safely.

## Important limitations and next steps

**PALSTAGE.DRV NOT INSTALLABLE**: `Enable(style=0)` still intentionally
returns failure; no GDI-owned PDEVICE, active Win16 host binding, verified
VRAM capacity, 386 enhanced-mode VDD handoff, text/cursor, or huge-bitmap
capability. PALDIAG reports selector mapping metadata but **does not prove
A000h VRAM reads/writes are safe**. No port I/O, mode programming or VDD
coordination is exercised. After actual Win3.1 PALDIAG results, correct
the Win16 selector assumption if necessary, then implement a safe
non-installed driver startup test and real GDI ownership. Preserve C89
shared renderer, ET4000AX backend, four OS adapters and fixed-width tags.
