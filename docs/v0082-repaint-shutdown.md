# Repaint validation and post-switch hang isolation

## v0081 runtime evidence

2026-11-27: user reports PALWATCH closes and Windows exits normally before
any DOS switch. Session 1 (passive): desktop corrupt after DOS; normal exit
uncertain. Session 2 (/repaint): visible desktop fully restored, then Windows
hung when user attempted to close PALWATCH. Both logs stop before normal
shutdown. PALBOOT1 lines 5075–5085 and PALBOOT2 lines 5097–5107 show FG=1,
recovered FG=1 and invalid bank cache. PALBOOT2 lines 5108–5151 show subsequent
valid cache selections. PALWATCH2 lines 11–24 show FG=1, repaint request queued,
and continued timer sampling. Neither monitor log records STOP. No DISABLE
ENTER in either driver log. Earlier post-switch exit hang occurred without
PALWATCH. The two recovery paths together visibly work, but the individual
necessity of bank/cursor recovery is not established. No claim of working
shutdown, windowed DOS, or arbitrary repeated switches.

## v0082 experiment

Monitor-only change. Retain the v0081 PALXHOOK binary. Explicit WM_CLOSE,
WM_QUERYENDSESSION, WM_ENDSESSION and WM_DESTROY handling. Timer disarmed
before window destruction and committed shutdown; late timer messages cannot
read the driver. Canceled shutdown leaves sampling active. Log entry/return
of timer stop, destruction and message-loop exit. Diagnostics may themselves
fail if DOS/USER is already hung; absence of a marker is not proof of location.

/repaint retains the visible monitor. /repaintonce creates a hidden window,
queues the same asynchronous desktop repaint after a new foreground epoch,
then exits on the third timer sample counting the successful request sample
(two further samples). It exits on sampling/repaint failure, or session end;
if no switch occurs it remains running until closed or Windows exits. The
one-shot is experimental and not a final driver dependency. No USER call is
added to the driver or ISR; no 4000h assisted save, mode reset or palette change.

## Runtime test

Build only PALWATCH.EXE and copy it to the disposable VM. Keep existing v0081
PALXHOOK.DRV, resources, SYSTEM.INI. Run Windows using WIN /B. First test a
visible /repaint monitor with no DOS switch and close normally. Next use
/repaint after DOS and record the last close markers on hang. Independently
use /repaintonce, wait for it to exit automatically after DOS return, then try
Windows shutdown if responsive. Recover both logs before another run/reset.
A clean STOP and MESSAGE LOOP EXIT with a later shutdown hang would separate
monitor teardown from driver/VDD teardown. No close marker could mean the
system hung before the application received WM_CLOSE. Do not repeat hard
resets unnecessarily; use a disposable VM snapshot.

## Next native integration

DDK adaptation guide (https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/)
describes dynamically obtaining USER ordinal 275 (repaint) and honoring
UserRepaintDisable around USER critical sections. Investigate the actual
Win3.1 reference driver's call timing, ABI, VDD registration and lifetime
before adding a native driver repaint callback; do not add static USER imports
or call it indiscriminately from a DDI/interrupt. The monitor experiment is
not a substitute for full VDD ownership management or shutdown validation.
