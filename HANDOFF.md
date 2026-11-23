# Palettium v0033 handoff — Win3.1 diagnostic GP-fault isolation (2026-11-23 UTC)

## Baseline and observed fault

v0031 fully validated on user's Ubuntu/Open Watcom; Win16 `PALSTAGE.DRV`
compiled and linked with 13 existing exports. User ran the DOS ET4KTEST in
86Box at 256/512/1024 KiB and saw correct screens. v0032 patch compiled
and linked both the staged driver and Win16 GUI `PALDIAG.EXE`; its host test
also passed. But the FIRST launch of PALDIAG under Windows 3.1 generated
an **Application Error / General Protection Fault in KRNL386.EXE at
0001:2272**, before any diagnostic dialog was displayed (user screenshot,
November 23). It is NOT established which call faulted. Do not equate a
successful host test with a working Windows Win16 runtime.

## v0033 source changes

- `windows/win31/paldiag.c`: retains the SAME `PALDIAG.EXE` filename and
  existing Open Watcom build script, but now displays a first STEP 0 dialog
  before ANY KERNEL query. It asks separately before `GetWinFlags` (STEP 1),
  `GetModuleHandle("KERNEL")` (STEP 2), and
  `GetProcAddress(kernel, "__A000H")` (STEP 3). After success it prints the
  low and high words of the pseudo-export. This isolates any fresh GP fault
  to a much narrower stage and lets the user decline probing altogether.
- REMOVED all `GetSelectorBase` lookups. A selector passing `(value&7)==7`
  need not be a valid descriptor; consulting an invalid descriptor is an
  unsafe diagnostic. Windows documents the LOW WORD returned by
  GetProcAddress for __A000H as the selector; we report, not dereference it.
- Optional INT 10h AH=0Fh readback still requires explicit Yes at STEP 4.
  No driver install, mode set, port I/O, VRAM reads/writes or VDD changes.
- `tests/test_win31_diag.c`: mock actual Win16 source; verifies STEP 0
  appears without any kernel calls; each gate runs only its intended query;
  selector-base is never called; BIOS stays opt-in; missing KERNEL/export
  fail cleanly. README and Win3.1 README updated.

## Usage

Apply v0033 over the user-validated v0032 tree:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal33.zip
make -f Makefile.win31diag test
sh windows/win31/build-paldiag.sh
```

Copy `tests/exes/PALDIAG.EXE` into a **recoverable Windows 3.1 86Box VM**.
The user's current working Windows display driver remains installed.
Run PALDIAG and capture the **STEP 0** dialog. Choose Yes one step at a
 time only if Windows remains stable. Stop after STEP 3 and record the raw
high:low address; **choose No for optional BIOS readback on first run**.
If a crash happens, report the exact last dialog and the action immediately
preceding the fault. A fault before STEP 0 points toward EXE entry or
loader/runtime, not toward any probed KERNEL API.

## Status / limitations

Host regression passes, but Win16/Open Watcom build and Windows runtime
for v0033 require user test. `PALSTAGE.DRV` is unchanged and NOT INSTALLABLE
(`Enable(style=0)` deliberately fails); selector hardware bridge dormant;
GDI-owned PDEVICE, text/cursor, capacity configuration, VDD handshake and
real Windows mode initialization still missing. Keep shared four-OS C89
architecture, stable production filenames, fixed-width milestone tags,
short incremental ZIP patches with correct UTC timestamps, executable
outputs under tests/exes, and `make clean` comprehensive.
