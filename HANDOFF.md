# Palettium v0043 handoff — first real Win3.1 NE driver load probe

## Maintainer-confirmed baseline: v0042 (Nov 24 2026)

The user ran the entire clean C89 test suite, Win3.1 text, cursor, native
session and ET4000AX host regressions: **all PASS**. Open Watcom built and
linked PALSTAGE.DRV, with all 19 exports present and **no warnings**;
the v0041 W124 warnings are resolved. The Win16 critical sections compiled.
The driver is still deliberately NOT INSTALLABLE.

## v0043 scope

A real Windows 3.1 runtime loader exercise without replacing the active VGA
DDI: `windows/win31/palload.c` / `build-palload.sh` build `PALLOAD.EXE`.
The GUI gives independently confirmed, numbered steps:

1. `LoadLibrary("C:\\PALTEST\\PALSTAGE.DRV")` — actual NE module load.
2. `GetProcAddress` 19 known DDI names (no invocation).
3. `Inquire @101` — query 4 cursor-info bytes, memory guard checks.
4. `Enable(style=1)` — read **only** 110-byte candidate GDIINFO with guard
   bytes; validate geometry, pixel depth, device size and mapping metrics.

The app NEVER calls Enable(style=0), `SetCursor`, any drawing or any hardware
function. It calls `FreeLibrary` on all successful-load paths. Host regression
`tests/test_win31_load.c` runs the exact WinMain source with simulated Win16
functions, including absent config, GDIINFO corruption, missing exports,
refused load and opt-out behavior. Linux host tests do NOT prove the Win16
EXE compiles or that Windows can load the DRV: user must verify in 86Box.

## User tests

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal43.zip
make clean && make test
make -f Makefile.win31load test
sh windows/win31/build-palddi.sh
sh windows/win31/build-palload.sh
wdump -x tests/exes/PALSTAGE.DRV
```

For **first VM runtime test**, snapshot 86Box. Using the currently working
Windows 3.1 VGA driver, copy `tests/exes/PALSTAGE.DRV` and `PALLOAD.EXE`
into `C:\PALTEST` in the VM. Launch `PALLOAD.EXE` from Windows File Manager.
You need not modify SYSTEM.INI for steps 1–3. STEP 4 returns zero by policy
without `[Palettium] VRAMKiB=256` (or actual configured card capacity),
which is expected, not a loader failure. To test a successful query later,
add *only* that Palettium section to a snapshotted SYSTEM.INI and rerun;
never change `[boot] display.drv` or `[386Enh] display` yet.

Report each dialog result and any Windows GPF/LoadLibrary code, particularly
the last STEP before a failure. If `LoadLibrary` fails, inspect DLL imports,
entry point and runtime relocation; do NOT enable the driver or touch VDDs
as a workaround. No mode writes, port I/O, VRAM writes or BIOS calls are
performed in the new probe.

## Installation blockers (unchanged)

- `Enable(style=0)` still deliberately returns zero.
- No Win3.1 VDD/bank-switch/screen-resume authorization.
- Mouse IRQ/timer behavior not runtime-proven for a live device.
- GDI-owned PDEVICE runtime publication, full font/bitmap and capability
  details, and safe rollback/recovery still unverified.

---

# Palettium v0042 handoff — Win16 mouse critical region and installation audit

## Validated baseline: v0041 (user result, Nov 24, 2026)

The maintainer applied v0041 and ran `make clean && make test`, Win31 cursor,
text, shared cursor, native session, Win31 link and ET4000AX screen suites:
**all PASS**. Open Watcom compiled/linked `PALSTAGE.DRV`; wdump confirmed
**19** expected exports including cursor @101..104. There were **eight
nonfatal W124** comparisons-always-false in `common/rg_cursor.c` lines
44–45 and `windows/win31/pal_cursor16.c` line 18.

## v0042 change set

1. Remove the eight redundant 16-bit signed-range checks (no warning
   suppression). On 32-bit hosts the shared cursor still validates 16-bit
   hotspot ranges, guarded by `#if INT_MAX > 32767`.
2. Replace the C-only `writing` flag in mouse-IRQ `MoveCursor` with one
   6-byte mailbox: 16-bit ready, X, Y. On the 16-bit Open Watcom target,
   `#pragma aux` emits `PUSHF/CLI` and the Windows DDK `POPFF` trampoline
   (`JMP/IRET/PUSH CS/CALL`) around both publication and consumption. Do not
   replace with raw `STI` or `POPF`. Only the platform adapter contains this
   code; the shared C89 cursor raster remains unchanged.
3. `CheckCursor` uses a non-reentrant `composing` guard. An IRQ that arrives
   during a pixel write publishes a *new* pending position for the next check,
   and cannot recursively repaint the cursor. Unbind, bind and shape changes
   suppress timer painting while modifying the cursor state.
4. Extend host tests to inject a simulated nested timer+mouse callback during
   banked cursor paint, verifying no recursive composition or dropped queue.
   Source-build guard checks both DDK-style critical sequences and the
   permanent `Enable(style=0)` refusal.

**Crucial distinction:** All portable C89 and host tests may pass without
executing the Win16 Watcom assembly. We cannot assert Win16 assembler syntax,
IRQ timing, DS/pointer behavior, or VDD integration until separately tested
on the maintainer's machine and subsequently in recoverable 86Box Windows.

## Strict installation blockers, in priority order

- **Windows runtime:** native cursor IRQ/timer behavior has not been tested
  with an active Palettium screen host; only host simulation is available.
- **Enhanced-mode VDD:** known `[386Enh] display=*vddvga` from PALDIAG. This
  setting cannot be assumed safe for any ET4000 AX mode; >256 KiB framebuffers
  exceed the stock VDD's documented support. No VDDsetaddresses handshake.
- **Mode handoff:** no permitted, verified Win16 hardware mode transition
  inside Windows, or restart/resume/fullscreen DOS save-restore lifecycle.
- **GDI/ABI:** `Enable(style=0)` remains disabled; no Windows runtime GDI
  PDEVICE test, significant FONTINFO limitations, no huge bitmap support,
  and other unverified mandatory entry-point/resource requirements.
- **Install package:** no proven driver SYSTEM.INI/OEMSETUP.INF integration
  or tested recovery image. Do not present a staged NE link as bootable.

The strongest path to a first desktop is an isolated **standard-mode**
Windows 3.1 VM configured with verified 256-KiB ET4000AX VRAM, explicit
Palettium VRAM setting and a snapshot. This still requires a separate
native Enable and cursor/runtime approval and is **not authorized by v0042**.
Keep the four-OS reusable backend and avoid copying the Windows DDI into a
second implementation.

## Next build validation (maintainer)

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal42.zip
make clean && make test
make -f Makefile.win31cursor test
make -f Makefile.win31text test
make -f Makefile.win31native test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Expected 19 NE exports (unchanged). Record any Open Watcom errors or warnings
in `pal_cursor16.c`. Do **not** install PALSTAGE.DRV in Windows yet.

## Research evidence retained

Previous PALDIAG in 86Box Win3.1 returned `__A000H` = `FFFF:1027`, one
successful read of `A000h:0000h`, BIOS `INT 10h` readback mode `12h`,
GetWinFlags `0C29h`; `[386Enh] display=*vddvga` configured. DOS ET4KTEST
INFO correctly decoded CRTC37 as 0Dh/0Eh/0Fh for 256/512/1024 KiB across
three BIOSes (9/9). None establishes a functioning Win3.1 display driver.

## Packaging rules

Use fixed-width sequential tags (v0041, v0042, ...), short incremental ZIPs
built fresh from changed files, full-source checksums, correct UTC ZIP extra
field + Chicago DOS timestamps, preserve stable production filenames,
outputs under `tests/exes` and comprehensive `make clean`.
