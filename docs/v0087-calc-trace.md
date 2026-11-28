# v0087: Calculator performance isolation

The maintainer confirmed Standard startup after restoring TrialEnable=4401.
Enhanced startup and Calculator work, but button layout is slow and the
cursor flashes repeatedly. Paint is deliberately outside this experiment.
The uploaded Enhanced trace PALBOOT(20261128-020552).TXT has approximately
6,900 lines: 96 bitmap transfer dumps, 55 DIB dumps, 110 text entry traces,
9 cursor snapshots and 18 hook snapshots. It ends with successful hook
removal and BIOS restoration; no background/foreground events occurred.
The trace identifies the combined driver, not a specific build revision.

The existing file recorder opens/seeks/writes/closes for each marker;
bitmap diagnostics also sample pixels. Their performance impact is unknown.
Introduce a unified-driver profile key RenderTrace, default 1. Set it to 0
for an A/B experiment using the same PALWIN31.DRV in both sessions. Quiet
mode skips detailed DDI, DIB, bitmap sampling, font-base and intermediate
cursor diagnostics and unchanged periodic hook snapshots. Startup/shutdown,
changed hook counters, bounded error messages, and final cursor counters
remain. BOOTTRACE stays compiled in, including functional recovery code.
The drawing/cursor algorithms, activation gates and other driver builds
are unchanged. This is not a proven performance or flicker fix.

Build and resource-bind as in v0086. Test Calculator only, with the same
pointer location outside its window and the same VM settings. Compare
RenderTrace=1 and RenderTrace=0 in fresh Windows sessions; save each log
separately. Observe time to complete button layout and cursor flashing.
No Paint test is requested yet. Native Watcom/runtime validation pending.

Host regression tests exercise actual quiet BitBlt and DIB wrappers,
verify identical output and untouched detailed trace budgets, and check
quiet profile selection and mocked combined activation/shutdown. The
complete clean host regression suite passes.
