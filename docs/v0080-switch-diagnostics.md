# v0080: first DOS-switch failure and durable diagnostics

## Observations

The v0078 passive hook desktop-only run had successful installation, chained
API discovery and removal. The subsequent DOS return experiment
with PALXHOOK showed a nearly black desktop with recognizable window frames,
titles, a selected MS-DOS Prompt label and cursor. The user could not shut down
normally. PALBOOT(20261127-232635).TXT contains 4586 nonempty/log lines, ending
with a normal cursor snapshot (18 accepted shapes, zero rejects, zero mask
mismatches). The initial hook install and discovery probe both pass. There is
no Disable entry, hook removal or final counter dump. Absence of this dump is
NOT evidence of zero switch notifications. The visual result is consistent
with lost video-memory contents, changed palette or bank state, missing repaint
or a combination. It does not identify which is responsible. Whether Windows
continued scheduling ordinary tasks after the switch is also unverified.

## Diagnostic changes

- Keep the interrupt handler, chaining, activation keys and graphics operations
  unchanged. No 4000h request, no additional hardware I/O, no USER/GDI driver
  imports. The new named export PALHOOKSTATUS @501 (PALXHOOK only) is a FAR
  PASCAL one-WORD read of resident observer data. It loads its own DGROUP,
  returns 805Ah for query FFFFh while installed, and returns FFFFh when not
  installed or index outside 0..5 (other than signature). Actual 16-bit counter
  values may wrap, including FFFFh. The signature is not a security boundary.
- Existing ordinary DDI trace entry and draw-end paths persist snapshots on
  counter change or every 256 checkpoints, independent of trace budget. Only
  after verified hook installation; also before removal. Includes cached bank
  state (not actual register reads). Never log from the ISR or cursor callbacks.
- PALWATCH.EXE resolves the private export from the already loaded DISPLAY
  module; no LoadLibrary or hardware calls. It checks Enhanced mode and the
  configured driver, then samples in normal window timer message context.
  Every report line is closed before proceeding. The six individual reads
  are not a simultaneous snapshot; a switch during collection may split a row.
  Timer scheduling can pause while Windows is backgrounded or frozen; a missing
  heartbeat is not proof that notifications were absent. No guarantee of host
  disk flush across abrupt VM reset; copy report before rolling back snapshot.

## Controlled test

Apply v0080 after v0079. Build PALXHOOK and PALWATCH, bind the same local VGA
resources, verify imports/exports/resident segments. Keep the previous driver
outside SYSTEM for recovery. Use disposable Enhanced-mode reference VDD VM,
512 KiB VRAM and existing 4402/4403 activation keys. Start WIN /B (default
Enhanced mode on user's VM; confirm GetWinFlags/About). Launch PALWATCH.EXE
from Program Manager File/Run, leave it open, confirm C:\PALWATCH.TXT has at
least one sample. Launch MS-DOS Prompt as in previous test; if full-screen,
run DIR, EXIT. Allow Windows a few seconds to process timer messages if still
responsive. If safe, move/cover a window to force a repaint and note whether
colors/contents recover. Do not rely on normal shutdown. If hung or severe
corruption, reset disposable VM as necessary, copy both C:\PALWATCH.TXT and
C:\PALBOOT.TXT before snapshot rollback. Keep the image if different.

Interpretation: nonzero BG/FG or SAVE/RESTORE identifies a received notification;
zero values only count if a post-return sample is present. If no post-return
sample, task scheduling or observer persistence remains unresolved. A successful
repaint would favor loss of saved framebuffer contents (not definitive); failed
repaint motivates bank and mode/palette investigation. Next implementation should
handle ownership and invalidation through the OS bridge and hardware backend;
consider donor VDD-specific registration and supported assisted restore contract
only with correct repaint, synchronization and cursor handling. Do not enable
assisted save/restore blindly.

## Validation boundaries

Host instruction model verifies original ISR behavior and the private export's
index, signature, DS/register restoration and callee stack cleanup. Mock task
checks heartbeat persistence and failure handling. Native Watcom assembly,
linker export name, far-call ABI and real Windows task scheduling are not
validated here. Existing Standard-mode and PALX386 driver paths remain untouched.
