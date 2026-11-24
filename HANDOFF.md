# Palettium v0046 — eliminate early USER dependency from trial display driver

**Observed failure (maintainer, Windows 3.1 Standard mode):** v0044 and v0045
`WIN /S` displayed the splash screen, then reported `Error loading
PALTRIAL.DRV` and returned to DOS. The supplied `BOOTLOG.TXT` showed:

```
LoadStart = PALTRIAL.DRV
LoadStart = USER.DLL
LoadFail = USER.DLL Failure code is 02
LoadFail = PALTRIAL.DRV Failure code is 02
```

The supplied **actual v0045 PALTRIAL.DRV** (34,526 bytes) is an MZ/NE Win16
DLL. Its NE module-reference table contains exactly `KERNEL`, `USER`, and
its segment relocation records contain a call to **USER ordinal 1**, the
Win16 `MessageBox` function. The generated linker file does not explicitly
request USER and the driver C source does not call MessageBox. The call is
inside Open Watcom's 16-bit general interrupt dispatcher, introduced by the
`int86(0x10,...)` calls in `windows/win31/pal_et4k_win16.c`. This explains
why the same driver can load from PALLOAD **after** Windows has initialized
USER but can fail when used as the display driver **before** USER has loaded.

## v0046 targeted change

- Replace the two `int86` calls **only in the Windows 3.1 native hardware
  adapter** with a dedicated `#pragma aux` 16-bit `INT 10h` wrapper. It takes
  AX, preserves DS and ES, makes one BIOS video interrupt, and returns AX.
  Existing saved-mode, readback and rollback logic is unchanged. A host
  fallback remains for non-Watcom source inspection; it cannot be linked
  into a Win16 build. The shared ET4000AX hardware abstraction is untouched.
- NEW `tools/check_win31_imports.py` parses the **actual NE module-reference
  table**, not just exports or strings, and refuses all dependencies except
  KERNEL during early display bootstrap. The Win16 build script runs this
  automatically on PALSTAGE.DRV and PALTRIAL.DRV; if USER still appears the
  build is a **FAIL**, not a false PASS. Synthetic NE tests cover bad imports
  and malformed images; host tests do NOT validate Watcom assembler.
- **26 GDI exports unchanged.** `PALSTAGE.DRV` still unconditionally refuses
  `Enable(style=0)`; separate PALTRIAL still requires actual Windows 3.1
  Standard mode, configured 256KiB ET4000AX, and TrialEnable=4401.
- This patch corrects a **known loader dependency defect**. It does not yet
  demonstrate successful GDI initialization or prove that calling BIOS
  INT 10h in the Windows Standard-mode context will work. Expect additional
  issues if the system gets farther into startup.

## Build and verify on maintainer's Open Watcom machine

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal46.zip
make clean && make test
sh windows/win31/build-palddi.sh
sh windows/win31/build-paltrial.sh
python3 tools/check_win31_imports.py tests/exes/PALTRIAL.DRV
wdump -x tests/exes/PALTRIAL.DRV
```

**Key success criterion:** the import audit prints `NE imported modules:
KERNEL` (or equivalent KERNEL-only), and neither driver build fails with a
USER import warning. If Watcom rejects the pragma assembly, report that
compiler diagnostic; don't bypass the audit or deploy a driver that fails.
No Win3.1 runtime boot has been tested with this variant yet.

## Recovery-only 86Box followup

On the **same disposable 86Box snapshot**, replace only PALTRIAL.DRV in
`C:\WINDOWS\SYSTEM`; preserve the VGA SYSTEM.INI backup and `[Palettium]`
settings. Boot from DOS with `WIN /B /S` so a fresh BOOTLOG.TXT is recorded.
If it fails, recover VGA from DOS. NEVER rename, copy, or fabricate USER.DLL
as a workaround for its early load failure. See v0044/v0045 instructions for
trial authorization and DOS recovery.

---

# Palettium v0045 — complete missing Win3.1 GDI bootstrap ordinals

**Observed failure:** User's v0044 build compiled and linked cleanly with 19
exports, but a disposable Windows 3.1 86Box `WIN /S` boot showed the splash
screen, then exited to DOS with `Error loading PALTRIAL.DRV`. No evidence
was supplied that the ET4000AX mode changed; the message is ambiguous.

## v0045 diagnostic finding

Our previous export checker validated **only the 19 exports we implemented**.
Historical Windows 3.1 DDK contracts require additional entry points. Missing
ones were Control @3, EnumDFonts @6, EnumObj @7, GetCharWidth @15,
DeviceBitmap @16, SetAttribute @18. FastBorder @17 is included for broad
compatibility (the 3.0 DDK calls it optional, a 1992 reference calls it
required). The full ordinal range @1..18 is now covered except the
printer-only DeviceMode @13. Existing @22..26 and @101..104 stay unchanged.

## New entry behavior

- Control implements QUERYESCSUPPORT=8 only, and reports only that escape as
  supported. All unsupported escapes refuse without side effects.
- EnumDFonts and EnumObj return the documented no-device-fonts/objects result
  (1) without touching callback thunks. This avoids inventing the Win16
  LOGFONT/LOGPEN callback ABI but means actual catalogs remain unimplemented.
- GetCharWidth uses the verified standard raster FONTINFO decoder and returns
  glyph widths. It prevalidates all values and far-pointer segment bounds.
- DeviceBitmap is the mandatory no-device-bitmap stub (0).
- SetAttribute is the mandatory reserved stub (0).
- FastBorder is exported but refuses drawing, returning nonzero error. This
  avoids falsely claiming that it drew anything; a functional implementation
  will be needed if Windows calls it during normal rendering.

The Win16 build scripts and export validators now expect **26 DDI ordinals**.
Default PALSTAGE Enable(style=0) remains closed; PALTRIAL still requires
Standard mode, exact 256KiB, and TrialEnable=4401. No mode-switch,
framebuffer, cursor-IRQ, or VDD code has been changed. THIS IS NOT YET A
CONFIRMED FIX for the boot failure: loader error, failed initialization and
missing exports can result in similar outward symptoms.

## Build/test on workstation

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal45.zip
make clean && make test
make -f Makefile.win31text test
make -f Makefile.win31trial test
sh windows/win31/build-palddi.sh
sh windows/win31/build-paltrial.sh
wdump -x tests/exes/PALTRIAL.DRV
```

Expect 26 exports and no Watcom warnings. Build is host-tested, but **Open
Watcom v0045 linking and Windows boot remain untested** until user runs them.

## Recovery-first 86Box followup

1. Keep the working VGA SYSTEM.INI backup and the disposable VM snapshot.
2. Verify the actual 256KiB ET4000AX setting and `[Palettium]` keys.
3. Copy *new* `PALTRIAL.DRV` to the Windows SYSTEM directory, overwriting old.
4. With trial boot configuration and DOS recoverability, try `WIN /S` once.
5. If it fails, try `WIN /B /S` to generate `C:\WINDOWS\BOOTLOG.TXT`
   (or restore VGA first and run `WIN /B` to confirm logging on that image).
   Preserve last LoadStart/LoadFail and related lines; this may help
   distinguish NE loading from later GDI initialization. Do not infer from a
   missing bootlog entry that a specific DDI succeeded.
6. Restore the original SYSTEM.INI from DOS after the observation.

## Remaining blockers

Win3.1 native startup, GDI ordinal resolution, BIOS mode switching and
actual drawing in a Windows desktop have not been runtime validated. Host
checks are not Win16 ABI tests. Native Win16 cursor reentrancy and some fonts
remain unverified. Keep WfW3.11/Win95/Win98 shared architecture intact.

---

# Palettium v0044 — first separately gated Win3.1 Standard-mode display boot

**Baseline:** Maintainer compiled v0043 successfully without warnings and
reported that PALLOAD's Windows 3.1 tests all appeared to run fine. The
individual dialog texts were not captured. GDI-owned style=0, mode changes,
mouse IRQ and text during live GDI startup remain UNTESTED.

## v0044 changes

- Corrected GDIINFO's previously all-zero capabilities for the existing
  software DDI: reported BitBlt, Win2-compatible text output, palette
  services, restricted basic polyline/rectangle, clipping, 20 static system
  colors, 256 palette entries, 18-bit DAC and dpDCManage=4. Still **not**
  claiming device-dependent bitmaps, DIB support, scaling, >64K fonts,
  or arbitrary paths. These values follow the Win3.x DDK, not Win32 GDIINFO.
- Normal `PALSTAGE.DRV` continues to *unconditionally refuse* Enable(style=0).
- Added a portable C89 Standard-mode/VRAM/key authorization predicate with
  exhaustive low-bit and negative tests.
- Added **separate** PALTRIAL.DRV variant: exactly the same 19 DDIs and shared
  engine, except Enable(style=0) is permitted **only if** WinFlags includes
  WF_PMODE | WF_STANDARD and *not* WF_ENHANCED, and SYSTEM.INI
  `[Palettium] VRAMKiB=256` and `TrialEnable=4401` are both set. The
  existing transaction then chooses ET4000AX BIOS 640x400x8 (2Fh), verifies
  readback, starts banked framebuffer, initializes static palette, copies a
  20-byte GDI-owned PDEVICE, and binds DDIs; Disable restores original mode.
  This is an **unverified boot experiment**, not production-ready.
- Diagnostic PALLOAD.EXE is unaffected: it still loads only PALSTAGE.DRV
  and never calls style=0.

## Build

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal44.zip
make clean && make test
make -f Makefile.gdiinfo test
make -f Makefile.win31trial test
sh windows/win31/build-palddi.sh
sh windows/win31/build-paltrial.sh
sh windows/win31/build-palload.sh
wdump -x tests/exes/PALTRIAL.DRV
```

## Controlled first boot — disposable 86Box clone ONLY

Do NOT test on a real machine or your only bootable VM. First ensure the
86Box snapshot/clone and its DOS boot are recoverable. While the existing
VGA desktop is still working, copy `PALTRIAL.DRV` into `C:\WINDOWS\SYSTEM`.
Exit Windows fully to DOS. Before edits:

```
cd \WINDOWS
copy SYSTEM.INI SYSVGA.BAK
```

Edit SYSTEM.INI using a DOS text editor (not an active Windows session):

```
[boot]
display.drv=PALTRIAL.DRV

[Palettium]
VRAMKiB=256
TrialEnable=4401
```

**Change only the `display.drv` value in an existing [boot] section.**
Keep the rest of SYSTEM.INI and existing [386Enh] settings unchanged. The
86Box ET4000AX device must be configured to **256 KiB** (not merely an
INI guess). From DOS start `WIN /S` to force Windows 3.1 **Standard mode**.
Never `WIN /3` and never boot this trial in enhanced mode.

A black screen, partial desktop, fault, missing fonts, palette errors or
freeze all count as useful FAILURES, not a reason to keep running. Observe
whether the BIOS mode changes to 640x400x256 and whether any Windows desktop
pixels appear. The native mouse ISR and larger fonts are not fully proven.

## DOS recovery (even if Windows fails to start)

Power off/revert your 86Box snapshot or boot DOS *without starting Windows*.
At DOS prompt:

```
cd \WINDOWS
copy SYSVGA.BAK SYSTEM.INI
```

If Windows was installed elsewhere, use that directory. The backup is the
whole original SYSTEM.INI; this reverses the display selection and all trial
keys. If even DOS cannot boot, revert the 86Box disk snapshot. Never rely on
mouse/keyboard controls inside a potentially failed Windows session.

**Boundary:** Test validates an experimental boot attempt, not a working
or correct driver; palette translations, large fonts, VDD/standard-mode
switching semantics, Win16 IRQ timing and restore/resume need real VM evidence.

---

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
