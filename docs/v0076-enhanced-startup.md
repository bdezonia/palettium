# v0076: first explicitly gated Enhanced-mode startup experiment

Apply after v0075. v0074 user report: text subjectively ~30% faster. Splash
is now present on cold boot and restart in Standard mode. Cause of original
splash corruption remains unknown. v0075 reference report shows flags 0C29,
VGA448.DRV, VGACOLRX.GR2, V7VGA.3GR, VDDTLI4.386, VDD API 003B:0378.
The report confirms startup and discovery, not successful DOS switching.

## Local binary evidence (no proprietary bytes distributed)

SHA-256 of maintainer-provided reference files:

```
745c3c3258b3fe125bad0105baaf5bf2c80a81a772fca595f235ba7ae5572680  VGA448.DRV
eb3d54c2c53ae8a98771dc8e32c1261e6e211dd8d90f7fa35abde85bb495e3db  VDDTLI4.386
a01ce84f46b31719c6f677f2973e4d24a838e0f1a55239e8acb61823cf920757  V7VGA.3GR
```

VGA448.DRV is NE, display description 640x480 256 colors; VDDTLI4 is LE
VxD with VDDVGA strings; V7VGA is NE. Reference driver segment 1 begins
at file 0xBA0, segment 2 at 0x6280. Disassembly (16-bit, offsets relative
to segments) identifies the following:

- 2:015A-0172 obtains INT 2F vector with DOS 35h, saves previous vector,
  installs handler 1:413A via DOS 25h. 2:019F-01A7 restores previous vector.
- 1:413A-4148 distinguishes 4001h and 4002h, jumps to prior vector for
  other functions. 1:4149-4193 uses driver data and callback tables;
  returns via IRET with saved carry clear on accepted handling, set on
  a refusal path. Do not copy proprietary implementation. Register
  preservation, chaining, code/data residency and carry semantics matter.
- 2:030C-0319 requests BIOS mode 2Eh, verifies with AH=0Fh. This matches
  Palettium's existing ET4000 mode. 2:02BC-02CA tests masked WinFlags for
  protected Enhanced and issues 4000h after setup. 2:02E3-0306 restores
  BIOS mode 3 and issues 4007h under equivalent flags.
- 1:410E-4139 and 1:4194-41B1 implement deferred repaint logic, including
  an indirect callback. Exact identity of every reference callback has
  not been resolved. Raw searches found no literal 1684h lookup in this
  DRV; V7VGA.3GR has one at file 0x1CBF. This does not prove the absence
  of indirectly invoked private VDD calls. VDD dispatcher not fully decoded.

Historical DDK describes the 4000h service as disabling some trapping and
changing memory preservation; the VM may be required to repaint on return.
4001h/4002h are background/foreground notifications, 4005h/4006h save/restore
registers, 4007h reverses assisted save. See
https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/ section 2.3.1 and
https://dos-help.soulsphere.org/ddag31qh.hlp/winint2fabout.html .

## Why the first experiment deliberately does NOT issue 4000h

We isolate basic Enhanced startup (loader, GDI, protected aperture, BIOS,
VDD-mediated banking and palette) before implementing assisted switching.
Under the documented default unassisted contract the VDD is responsible
for display preservation. We have NOT established that the selected OEM
VDD fulfills that contract for Palettium. An API selector is not proof of
compatibility. No callback hook, private VDD call or assisted save request
is issued by v0076. This is a limited experiment, not production support.
Do not launch DOS sessions, switch VMs, or intentionally test background
operation with PALX386 yet. Even desktop startup can hang or corrupt display;
use a disposable snapshot. No claim of ownership safety or bank-cache
coherence when the VDD or other VM changes registers. The restriction is
an operator test protocol, not enforced by the driver.

## Build and tests

```
sha256sum -c CHECKSUMS-v0076.sha256
make clean && make test
make -f Makefile.win31native test
sh windows/win31/build-palx386.sh
```

Separate PALX386.DRV uses same 26 DDI exports and KERNEL-only import audit.
PALTRIAL remains Standard-only key 4401 and PALSTAGE still refuses style 0.
The native lookup is an isolated Watcom pragma aux INT 2Fh wrapper, ES:DI
zeroed before lookup, DS/ES/BP preserved, only selector returned. It is not
called in other builds. Host syntax and model tests pass; actual Watcom
compile, NE audit and native runtime are pending. Host fallback returns zero.

Use the same local VGA.DRV resource source that worked with PALTRIAL:

```
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --extract tests/exes/PALBOOT.res
wrc -q -bt=windows tests/exes/PALBOOT.res tests/exes/PALX386.DRV
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --verify tests/exes/PALX386.DRV
python3 tools/check_win31_imports.py tests/exes/PALX386.DRV
```

No reference binaries are packaged. Before changing the installed OEM
configuration, test the reference driver's DOS window/Alt+Enter switching
if not done, and report outcome separately. Save a VM snapshot and copy
SYSTEM.INI to a backup from DOS. Copy PALX386.DRV to C:\WINDOWS\SYSTEM.
Change ONLY display.drv in [boot] to PALX386.DRV; leave the OEM grabbers
and VDD untouched. Add/update [Palettium] VRAMKiB=512 and EnhancedStartup=4402.
Do not duplicate sections or keys. The trial is for the 512 KiB ET4000AX
baseline; do not lie about installed VRAM. Keep existing TrialEnable for
Standard driver if present. Boot WIN /B /3. If desktop starts, inspect
normal window drawing, cursor, menus, colors, then exit normally. Do not
launch DOS applications or switch VMs while using PALX386. On hang, reset
and restore SYSTEM.INI or snapshot. Return C:\PALBOOT.TXT and BOOTLOG.TXT
(if present), whether desktop was visible and whether shutdown worked.

New trace markers: ENH EXPERIMENT GATE, ENH API QUERY ENTER, ENH API SELECTOR,
ENH STARTUP ONLY NO ASSISTED SAVE. Gate key is logged in hex (1132).
Actual Enhanced flags must match mask 0031h == 0021h; exact case-insensitive
VDD/grabber strings, explicit 512 KiB, new key are required. A plan is not
permission. Transaction requires distinct authorization and rejects stock
VDD / incompatible geometry, unwinds failed publication/BIOS as before.

## Next milestone

Based on outcome, implement a native resident INT 2F notification bridge
with chaining and register preservation, deferred repaint and cursor/bank
ownership coordination. Investigate VDD-specific behavior before enabling
4000h. Do not add direct USER imports. Preserve Standard, shared C89 raster,
operations overrides, memory bitmap rendering and all v0073 handoff goals.
