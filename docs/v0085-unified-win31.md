# v0085: combined Standard and Enhanced build

## Runtime evidence for v0083 (v0084 only changed licensing)

User reported a successful series of Alt+Enter switches between full-screen
and windowed DOS. PALBOOT(20261128-011539).TXT has 5,992 nonempty lines and
shows five background and five foreground notifications, recovered FG=5,
five native repaint calls and five returns, no deferrals or blocked state,
94 disable and 94 enable notifications, zero cursor rejections/mask
mismatches, successful hook removal and BIOS restoration call to mode 3.
This validates the observed session, not arbitrary VDD behavior or every DOS
program. Eight TEXT FAIL REASON=0004 entries (zero character count without
ETO_OPAQUE) and one 000C entry (glyph renderer failure) remain to investigate;
no corresponding visual problem was reported. No changes to text rendering
in this patch.

## Combined build

PALWIN31.DRV combines the existing PALTRIAL and PALXRPT code paths. It is
still opt-in, not a production driver. The GetWinFlags protected-mode
bits select the route during style-0 authorization; each route retains its
existing independent key and VRAM restrictions. Standard skips all Enhanced
profile checks, VDD API discovery, USER resolution, hook installation and
removal. Enhanced retains the OEM VDD/grabber restrictions and all three
keys. A mismatched plan is rejected before port setup; an already published
session cannot change its route. Session authorization remains distinct.
The existing single-mode build scripts remain available. The new build
uses the same resident ASM and link audits as PALXRPT, with tracing enabled.
No changes to raster, chipset, cursor, hook ASM or repaint policy. The
unarmed repaint-permission handler may record ordinal-500 permission calls
in Standard mode, but cannot dispatch USER without an armed callback.

Host testing executes the actual combined profile gate for all 64 low flag
combinations and missing keys, capacity restrictions, VDD/grabber mismatch,
and an already published session. Mock platform services exercise the actual
activation/Disable paths for Standard and Enhanced, including an incorrect
plan, missing VDD selector and switching back to a fresh Standard session.
Existing single-mode syntax checks, interrupt models and regression suites
remain in place. Native Open Watcom and Windows testing are still required.

## Build

From root:

```
make -f makefiles/Makefile test-all
sh windows/win31/build-palwin31.sh
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --extract tests/exes/PALBOOT.res
wrc -q -bt=windows tests/exes/PALBOOT.res tests/exes/PALWIN31.DRV
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --verify tests/exes/PALWIN31.DRV
python3 tools/check_win31_imports.py tests/exes/PALWIN31.DRV
python3 tools/check_win31_resident.py tests/exes/PALWIN31.DRV
```

Keep the same working 512 KiB ET4000AX VM and OEM VDD/grabbers. Save a
snapshot and retain PALTRIAL.DRV/PALXRPT.DRV. Copy resource-bound PALWIN31.DRV
into SYSTEM. In the existing INI sections set:

```
[boot]
display.drv=PALWIN31.DRV
286grabber=VGACOLRX.GR2
386grabber=V7VGA.3GR

[386Enh]
display=VDDTLI4.386

[Palettium]
VRAMKiB=512
TrialEnable=4401
EnhancedStartup=4402
EnhancedHook=4403
EnhancedRepaint=4404
```

Test Standard first with WIN /S /B, verify About Windows, basic desktop and
normal shutdown. Expect UNIFIED STANDARD ROUTE and no ENH API QUERY, NATIVE
REPAINT RESOLVED or HOOK INSTALL/REMOVE markers. Save PALBOOT.TXT as a separate
file. Then boot WIN /B in Enhanced, confirm About Windows, repeat several
Alt+Enter DOS switches, exit DOS and Windows. Expect UNIFIED ENHANCED ROUTE,
NATIVE REPAINT RESOLVED, balanced BG/FG, repaint calls/returns and clean hook
removal. Save new trace. No PALWATCH needed. If Standard fails with the OEM
configuration, compare the original validated Standard PALTRIAL setup before
attributing the failure to consolidation. No new VM mode or capacity is
claimed validated; a 256 KiB Standard trial is separate later testing.
