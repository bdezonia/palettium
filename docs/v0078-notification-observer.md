# v0078 — passive resident Enhanced-mode notification bridge

Baseline: the reorganized v0077 tree and the maintainer's successful v0076
PALX386 desktop launch (About reported 386 Enhanced mode). This does not
validate switching or hardware-state preservation. All nine v0077 manifest
entries matched before edits. Preserve PALTRIAL and PALX386 as independent
fallbacks, the reusable C89 raster/operation overrides, all four OS targets,
future acceleration and independent installation/resources goals.

## Contract and design

Windows 3.x DDK describes hooking INT 2F using DOS 35h/25h and saving the
prior vector. With assisted save/restore disabled, the VDD normally saves
and restores memory and registers across background/foreground changes;
4001h/4002h may still occur. 4005h/4006h are register save/restore requests
associated with assisted operation. This is documented behavior, not proof
that VDDTLI4 implements it correctly for this driver. See:
- https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/ section 2.3.1
- https://dos-help.soulsphere.org/ddag31qh.hlp/winint2fabout.html
- https://open-watcom.github.io/open-watcom-v2-wikidocs/lguide.html (SEGMENT)

`pal_hook.asm` is an original, standalone 16-bit observer, not copied from
reference binaries. Fixed/preloaded code and data are requested only in
PALXHOOK. A conservative NE audit rejects any moveable, non-preloaded,
discardable segment or read-only data. It must pass after native linking
and again after resource binding. This is an attribute check, not proof of
actual runtime residency, relocation correctness or a safe unload lifecycle.

The ISR only updates six resident 16-bit words: background, foreground,
register save, register restore, last focus (0 unknown, 1 background,
2 foreground), and 1684h query count. Counters wrap modulo 65536. No C,
USER, GDI, KERNEL, DOS, BIOS or hardware access from inside the handler.
No notification is consumed or acknowledged; it forwards all AX values.
The handler loads its own DGROUP, preserves live flags and registers, and
uses two reserved stack words and RETF to transfer to the previous handler
with the original interrupt frame intact. The prior handler owns IRET and
all return semantics. Last focus is an observation, NOT authorization to
access the framebuffer. The observer does not gate drawing or fix bank,
palette or cursor state. No 4000h, 4007h, private VDD API invocation or
synthetic screen-switch notification is issued.

Installation is after successful PDEVICE publication, not during style=1
or failed startup. DOS get/set operations run outside the ISR; installation
and restoration are verified with get-vector. Status 0 = no installation,
1 = verified installed/removed (remove also succeeds if not installed),
2 = conflict/uncertain. If an intervening handler is present at shutdown,
we refuse to overwrite its chain. A conflict may leave our code referenced;
no safe live unloading is promised. Do not hot-swap/unload the driver, use
network/TSR experiments or continue after a conflict: reboot/restore the VM.
No race-free arbitrary third-party vector management is claimed. If Enable
succeeds but installation fails, the driver remains in its startup-only
mode rather than failing GDI startup with a potentially installed hook.
The experiment performs another real 1684h VDD lookup and logs whether
its returned selector matches the startup selector and the observer saw
at least one query. It does not compare DI or prove all VDD services work.
Only normal Enable/Disable log these observations; no file I/O from ISR.

## Build from repository root

```
unzip -o ~/Downloads/pal78.zip
sha256sum -c checksums/CHECKSUMS-v0078.sha256
make -f makefiles/Makefile clean
make -f makefiles/Makefile test-all
sh windows/win31/build-palxhook.sh
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --extract tests/exes/PALBOOT.res
wrc -q -bt=windows tests/exes/PALBOOT.res tests/exes/PALXHOOK.DRV
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --verify tests/exes/PALXHOOK.DRV
python3 tools/check_win31_imports.py tests/exes/PALXHOOK.DRV
python3 tools/check_win31_resident.py tests/exes/PALXHOOK.DRV
```

Native link also performs existing module-name/import/export audits.
No proprietary resources or reference binaries are included. If assembler,
linker or residency check fails, do not install; return the exact output.
Native Watcom and VM validation are pending. The new host test executes
an instruction-level MODEL of actual assembly source, including mock DOS
vector operations, 65536 AX values, register/frame preservation, counters,
install/restore failure, duplicate and intervening-hook cases. It does not
assemble or execute real machine code, prove native calling conventions,
segment relocation, actual interrupt stack behavior or OS reentrancy.
Host C89 syntax and synthetic NE audit fixtures pass. Main and all 23
standalone regression targets pass.

## Phase A: desktop and chain validation (required first)

Take a disposable snapshot of the known working PALX386 512 KiB ET4000AX
VM. Copy the newly built, resource-bound PALXHOOK.DRV to SYSTEM. Preserve
all settings except `[boot] display.drv=PALXHOOK.DRV` and add
`[Palettium] EnhancedHook=4403` to the existing section. Keep
`EnhancedStartup=4402`, `VRAMKiB=512`, VDDTLI4.386 and V7VGA.3GR as before.
Boot `WIN /B /3`. Inspect desktop, cursor, menus; confirm Enhanced mode.
No DOS applications or intentional VM switching in Phase A. Exit normally.
From DOS copy C:\PALBOOT.TXT elsewhere BEFORE the next Windows launch
(which overwrites it). Expected: HOOK INSTALL STATUS=0001, HOOK CHAIN
PROBE OK, nonzero query count, HOOK REMOVE STATUS=0001. Zero switch counters
are normal. If hang, unexpected selector, conflict or failed status, stop,
reboot/restore snapshot and return trace + BOOTLOG.TXT if available.

## Phase B: first controlled switch observation (only after Phase A passes)

We prefer to review Phase A first. The next experiment will be a single
full-screen DOS switch and return under unassisted VDD control, using a
fresh disposable snapshot and saving traces, not repeated switching or
windowed DOS sessions yet. The observer is not a screen-ownership guard;
there is still risk of display corruption or a hang. Do not enable 4000h.
A future active ownership protocol must quiesce screen drawing and cursor
safely, handle reentrancy, invalidate bank state at correct boundaries,
restore palette/register state as required, and defer repaint to a legal
USER context without static USER imports. This is v0079+ work; v0078
intentionally establishes native interrupt-chain mechanics first.
