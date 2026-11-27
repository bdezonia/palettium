# Windows 3.1 adapter

This adapter connects Win16 GDI/DDI calls to the shared C89 renderer and
ET4000AX backend. See the [project README](../../README.md) for architecture
and host tests. Native drivers are experimental; use disposable 86Box VMs.

## Driver builds and current status

| Driver | Purpose | Runtime status |
| --- | --- | --- |
| PALSTAGE.DRV | Non-activating staging and ABI checks | Never install |
| PALTRIAL.DRV | Standard-mode trial | Desktop validated |
| PALX386.DRV | Enhanced-mode startup only | Desktop validated |
| PALXHOOK.DRV | Enhanced-mode passive INT 2Fh observer | Desktop, hook chain probe and removal validated |

Full-screen/windowed DOS compatibility and active video ownership are not
validated. Assisted save/restore is not enabled. Do not treat startup or
passive observation as full Enhanced-mode support.

## Build from the repository root

Configure Open Watcom (including `wcl`, `wlink`, `wasm`, `wrc`) and run:

```sh
make -f makefiles/Makefile test-all
sh windows/win31/build-palddi.sh
sh windows/win31/build-paltrial.sh
sh windows/win31/build-palx386.sh
sh windows/win31/build-palxhook.sh
```

Build only the variant needed for a given experiment. Products go in
`tests/exes/`. The activation builds default to tracing; set
`PAL_TRIAL_TRACE=0` before invoking their build script for an untraced build.
Keep tracing enabled for diagnostic sessions. Native driver link audits
check expected exports, module name and KERNEL-only imports. PALXHOOK
also requires the resident-segment audit.

## Bind local startup resources

The current trials still require locally supplied VGA.DRV resources.
For example, to prepare PALXHOOK (adjust the local VGA.DRV path):

```sh
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --extract tests/exes/PALBOOT.res
wrc -q -bt=windows tests/exes/PALBOOT.res tests/exes/PALXHOOK.DRV
python3 tools/win31_trial_resources.py ~/Downloads/vga.drv --verify tests/exes/PALXHOOK.DRV
python3 tools/check_win31_imports.py tests/exes/PALXHOOK.DRV
python3 tools/check_win31_resident.py tests/exes/PALXHOOK.DRV
```

For PALTRIAL/PALX386 use the corresponding filename and omit the observer's
resident-segment audit. Do not install if a required audit fails.
Proprietary resources and OEM VDD/grabber binaries are not distributed here.

## Standard-mode trial

Use an ET4000AX VM with 256 or 512 KiB VRAM, matching the configured value.
Back up SYSTEM.INI and save a VM snapshot. Keep the working VGA configuration
and copy the resource-bound PALTRIAL.DRV into `C:\WINDOWS\SYSTEM`. Change
only the indicated keys in the existing sections:

```ini
[boot]
display.drv=PALTRIAL.DRV

[Palettium]
VRAMKiB=512
TrialEnable=4401
```

Set VRAMKiB to 256 instead on a 256 KiB VM. Start `WIN /S`.

## Enhanced-mode observer trial

Use the validated 512 KiB ET4000AX OEM configuration (VGA448.DRV reference
installation), not the stock VGA VDD. Preserve other INI entries and take
a snapshot before changing drivers. Copy the resource-bound PALXHOOK.DRV
into SYSTEM, then set the following keys in the existing sections:

```ini
[boot]
display.drv=PALXHOOK.DRV
286grabber=VGACOLRX.GR2
386grabber=V7VGA.3GR

[386Enh]
display=VDDTLI4.386

[Palettium]
VRAMKiB=512
EnhancedStartup=4402
EnhancedHook=4403
```

Start `WIN /B /3`. For the startup-only fallback use PALX386.DRV instead;
it does not install the hook. Do not enable assisted save/restore.
For switching experiments follow the controlled protocol in
[v0079 runtime validation](../../docs/v0079-runtime-validation.md).
Restore the VM snapshot on a hang or hook-removal conflict. Never attempt
live unloading of the observer. Save `C:\PALBOOT.TXT` after exit and before
the next launch overwrites it; also save BOOTLOG.TXT if relevant.

## Further reading

- [Enhanced-mode VDD research](../../docs/v0075-enhanced-mode.md)
- [Enhanced startup design](../../docs/v0076-enhanced-startup.md)
- [Notification observer design and limitations](../../docs/v0078-notification-observer.md)
- [Patch notes](../../patch-notes/)
- [Historical README archive](../../docs/history/win31-readme-through-v0078.md)

Keep this guide current rather than appending milestone history.
