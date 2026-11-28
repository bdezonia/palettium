# Experimental driver-managed repaint

## Validated baseline (v0082)

The user's 2026-11-27 session (PALBOOT(20261128-002210).TXT and
PALWATCH(1).TXT) completed the hidden `/repaintonce` experiment and a normal
Windows exit. The monitor records BG=1 FG=1, REPAINT REQUEST/QUEUED, two
more samples, ONE SHOT COMPLETE, TIMER STOP OK, WM_DESTROY ENTER, STOP,
DESTROY WINDOW RETURN and MESSAGE LOOP EXIT. The driver records a recovered
foreground epoch, invalid bank cache and subsequent valid banks; on exit it
records DISABLE ENTER, HOOK REMOVE STATUS=0001, BG=1 FG=1, no save/restore
notifications and BIOS RESTORE RETURN. Visual restoration and normal exit
are user-reported. The ordinary visible /repaint mode was not retested;
its earlier hang is not isolated. Twelve TEXT FAIL REASON=0004 markers
occur before the switch; retain this as a separate text diagnostic, not
as evidence of a failed repaint.

## Reference and interface

The supplied VGA448.DRV (SHA-256
745c3c3258b3fe125bad0105baaf5bf2c80a81a772fca595f235ba7ae5572680) was
examined using its NE segment table and 16-bit objdump disassembly.
Segment 2 offsets 0100h–011Dh look up a module and ordinal 0113h (275)
through far calls and store a far pointer. Segment 1 offsets 4110h–4139h
contain an exported-style byte flag in a WORD stack slot, checking pending
state and invoking a helper when enabled. Segment 1 offsets 4194h–41B1h
show the helper checking a saved far pointer and disable flag, setting a
pending flag or invoking the pointer with no arguments. Its INT 2Fh hook
at 413Ah compares AX with 4002h and 4001h. These findings are consistent
with the documented USER RepaintScreen procedure, not proof of identical
behavior or safe stack timing in Palettium. No reference code is shipped.

Sources: Windows 3.0 DDK Adaptation Guide, display switching discussion
(https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/); Windows 3.1
DDK Interrupt 2Fh overview
(https://dos-help.soulsphere.org/ddag31qh.hlp/winint2fabout.html).
The donor binary corroborates the older DDK's ordinal and disable/pending
mechanism. This is separate from the VDD registration and assisted-save
interfaces, neither of which we enable in this experiment.

## v0083 implementation

- New PALXRPT.DRV build, independent of existing PALXHOOK/PALX386/PALTRIAL.
  Requires all existing Enhanced gates plus EnhancedRepaint=4404.
- Resolve USER ordinal 275 at Enable (after publication, before installing
  the hook); use GetModuleHandle/GetProcAddress via KERNEL. If unavailable,
  the native callback is not armed. No static USER or GDI import intended.
- The resident hook invokes the new far C bridge only for a foreground
  notification in this build. It preserves the original interrupt frame and
  chains as before, saves the 386 general register halves and segment
  registers around the callback, and clears DF for C. The actual assembled
  code, register preservation and far ABI must be validated natively.
- Only claim an unprocessed foreground epoch if enabled by ordinal 500,
  recovery is complete, no cursor drawing/composition is in progress and
  trace I/O is idle. Claim before calling USER. The dispatch lock uses
  x86 memory XCHG in the native build. An enable notification retries a
  pending epoch; no generic DDI entry or cursor ISR invokes USER.
- Prevent Palettium file logging during the callback and nested driver
  execution. No diagnostic DOS call in the INT 2Fh hook or ordinal 500.
  Log native calls/returns/deferrals/permission counters later from ordinary
  drawing or Disable. Disarm before shutdown. Retain v0081 bank and cursor
  recovery. Shared renderer and chipset backend unchanged.

## Limitations and runtime procedure

This is a new experimental native callback, not the same call path as the
validated asynchronous PALWATCH RedrawWindow. The USER entry, its own
internal execution, stack capacity, interrupt/USER reentrancy and loader
lifetime require real Win16 validation. We do not yet establish full
screen ownership or safe arbitrary mid-draw transitions. A request deferred
because drawing or trace I/O is active may remain pending until another
foreground or an explicit ordinal-500 enable notification. No callback
attempts from arbitrary GDI contexts, timer cursor paths, or after Disable.
There is no mode reset, palette rewrite or INT 2Fh 4000h activation.

Build from repo root:

```
sha256sum -c checksums/CHECKSUMS-v0083.sha256
make -f makefiles/Makefile test-all
sh windows/win31/build-palxrpt.sh
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --extract tests/exes/PALBOOT.res
wrc -q -bt=windows tests/exes/PALBOOT.res tests/exes/PALXRPT.DRV
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --verify tests/exes/PALXRPT.DRV
python3 tools/check_win31_imports.py tests/exes/PALXRPT.DRV
python3 tools/check_win31_resident.py tests/exes/PALXRPT.DRV
```

Use disposable VM snapshot. Keep the working PALXHOOK binary installed and
copy PALXRPT.DRV beside it. Change only `[boot] display.drv=PALXRPT.DRV`
and add `[Palettium] EnhancedRepaint=4404` to the existing configuration.
Keep VRAMKiB=512, EnhancedStartup=4402, EnhancedHook=4403, OEM VDD/grabber.
Start `WIN /B`. First do a desktop-only session and exit. Confirm
NATIVE REPAINT RESOLVED, zero calls and normal hook removal. If resolution
fails, stop; do not test DOS. Next fresh session, no PALWATCH, enter
full-screen DOS, EXIT to Windows, verify desktop and exit normally. Send
PALBOOT.TXT for each run. Expected FG=1, recovered FG=1, native calls and
returns=1, normal Disable and BIOS restoration. If callback is deferred or
fails, do not assume visual restoration; recover trace after reset. Do not
repeat hard resets unnecessarily. Fall back by restoring PALXHOOK.DRV and
using PALWATCH /repaintonce (only a single round trip per monitor run).

Host validation: complete clean test-all suite; both ASM variants modeled
for all AX values, new C89 policy and syntax checks, boottrace suppression
mock. The source interpreter models low words and stack sizes, not 32-bit
register halves, CPU interrupt semantics, real USER callback or actual NE
relocations. No Watcom compiler/assembler or VM available in this environment.
