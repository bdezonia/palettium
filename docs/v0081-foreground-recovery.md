# Foreground recovery experiment

## Observed v0080 runtime (2026-11-27)

PALWATCH samples 7 and 8 announce background and foreground respectively.
The counters settle at BG=1, FG=1, SAVE=0, RESTORE=0, FOCUS=2, QUERY=10
(hex A). PALBOOT lines 5046–5085 corroborate the events. Bank numbers
are cached software values, not hardware reads. Both logs persist after
the desktop becomes corrupt. BOOTLOG records successful initial startup.
Moving the Main window exposes white regions of Program Manager; new
painting works at least partially, while unpainted regions remain black.
The framebuffer contents, palette, register state and repaint damage are
not individually established. No normal shutdown recorded.

The Windows DDK describes 4002h as a foreground notification and expects
full restoration by the VDD in unassisted mode. See
https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/ (section 2.3,
4000h–4007h). The reference configuration evidently does not give a
correct Palettium desktop after the switch. Do not assume that cached
bank numbers represent the actual register after that transition.

## Implementation

Only PALXHOOK changes functional behavior. At the next idle cursor/DDI
boundary after a foreground epoch, invalidate the cached read/write
bank pair (existing ET4000 selector will rewrite both on next access)
and discard cursor visibility/device association without writing stale
saved-under pixels. Preserve the cursor shape, mailbox and coordinates.
This uses the existing cursor draw-depth guard; it does not guarantee
that an arbitrary mid-drawing switch is serialized. No hardware I/O,
USER call, BIOS call or file access in the interrupt handler. No new
assisted-save request, no mode reset or palette reprogramming. Background
ownership is still delegated to the VDD in unassisted mode. The live trace
records HOOK RECOVERED FG alongside the observer's FG counter. A pending
recovery may be applied by the cursor path before that trace is written.

PALWATCH now accepts optional /repaint. The first sample establishes the
baseline; a changed foreground counter with announced foreground queues
RedrawWindow(GetDesktopWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
RDW_FRAME | RDW_ALLCHILDREN) from its ordinary timer message, not the
ISR or driver. See Windows 3.1 SDK RedrawWindow and historical MFC help
https://dos-help.soulsphere.org/mfc.hlp/RedrawWindow.html. This is a test
of USER repaint, not a final driver-managed repaint mechanism. A success
return means queued, not that the screen actually looks correct. It
retains the v0080 private export signature, so it can also be used with
v0080 if an independent repaint-only control is needed.

## Test

Build PALXHOOK and PALWATCH, bind resources, and keep the same SYSTEM.INI.
Use a disposable snapshot. First run PALWATCH.EXE without arguments,
then switch to DOS and back; observe desktop and drag a window. Save
logs. If still corrupt, repeat in a clean snapshot using
PALWATCH.EXE /repaint, leaving the monitor running before DOS launch.
Recover logs before restarting/overwriting them. Compare images, live
foreground/recovered epochs, and REPAINT REQUEST/QUEUED/FAILED markers.
Do not attempt assisted save or live unload. If still corrupt, inspect
actual hardware state and VDD registration; do not blindly reset the mode.

## Validation limits

Host C89 tests exercise the guarded transition, no repeated recovery,
16-bit counter wrap, mock external ET4000 bank register change, and
opt-in repaint/failure logging. Existing full suite passes. Open Watcom
and 86Box not available here; native compilation, timing, full-screen and
windowed switching and visually correct repaint remain unvalidated.
