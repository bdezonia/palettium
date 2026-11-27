# v0079 — observer runtime validation and next experiment

The maintainer's PALBOOT(20261127-231700).TXT (6743 lines) establishes
successful v0078 Enhanced-mode initialization and normal Disable path.
Flags 00000C29, VRAM 0200h (512 KiB), activation key 1132h (4402), VDD
selector 003B, BIOS request/actual 002Eh. Hook install status 0001,
chain probe OK, query count 0001 at installation. At shutdown:

```
HOOK REMOVE STATUS=0001
HOOK BACKGROUND=0000
HOOK FOREGROUND=0000
HOOK SAVE REGS=0000
HOOK RESTORE REGS=0000
HOOK LAST FOCUS=0000
HOOK QUERY COUNT=0004
```

BIOS restore request 0003 and return are logged. Cursor rejects and mask
mismatches are zero. The trace alone does not prove visual correctness or
all interrupt return semantics. It does establish successful installation,
observed queries and verified removal on this configuration. No deliberate
DOS switching was requested in the first test; zero notification counts
are consistent with that protocol, not proof notifications are unsupported.

Two TEXT FAIL REASON=000C markers (lines 6040 and 6067) remain open:
`palddi.c` returns that reason when `rg_gdi_text_glyph` rejects a positive
count render. The trace does not identify which glyph validation condition
failed. No visual text failure has been reported with this trace. Preserve
as a diagnostic issue; do not change shared text code on this evidence alone.

## Next experiment: Phase B (same v0078 executable)

Use the 512 KiB OEM VDD/grabber configuration that passed Phase A. If not
already done, validate full-screen DOS switching under the reference driver
first. Start from a disposable VM snapshot; use the traced PALXHOOK.DRV
with the same INI settings as Phase A. No rebuild or new activation key is
needed. Do not enable assisted save/restore. The observer is not an active
screen-ownership guard; display corruption or a hang remains possible.

1. Boot WIN /B /3; confirm desktop and cursor are intact.
2. Launch the MS-DOS Prompt from Program Manager. If already full-screen,
   leave it there; if windowed, press Alt+Enter once to make it full-screen.
   If the launch itself fails or corrupts the display, stop the experiment.
3. At the DOS prompt run DIR (no graphics program). Return to Windows with
   EXIT. This is a single full-screen session, not a repeated stress test.
4. Inspect the desktop, cursor, colors, menu/text rendering and lower screen
   area; report visual corruption or unexpected behavior. Exit Windows
   normally if possible. Save PALBOOT.TXT before the next launch.
5. On hang, serious corruption, or removal conflict, restore the snapshot.
   Save available trace before doing so if practical; do not continue using
   a conflicted hook. Report the exact point of failure.

If it passes, inspect notification counts and test more switching only in a
subsequent controlled experiment. Zero counts during a real switch may mean
this VDD does not send them without assisted mode, or that our observation
path is incomplete. A future ownership protocol must be based on actual
behavior, not assumed counts. Do not automatically issue 4000h in response.

## Documentation policy

README files describe the current framework and instructions. Release
history belongs in patch-notes; engineering details in docs. Both prior
Markdown READMEs are preserved byte-for-byte in docs/history/ as historical
snapshots. Their relative links and old build commands may be obsolete.
No runtime implementation or build recipes change in v0079.
