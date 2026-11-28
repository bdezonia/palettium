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
| PALXHOOK.DRV | Enhanced-mode INT 2Fh observer and experimental recovery | One full-screen DOS round trip and clean shutdown with PALWATCH /repaintonce validated |
| PALXRPT.DRV | Driver-managed USER repaint | Native full-screen and repeated full-screen/windowed switching and clean shutdown validated |
| PALWIN31.DRV | Combined Standard/Enhanced activation build | Standard and Enhanced desktop startup validated; application performance under test |

The tested full-screen/windowed transitions and repaint work on the OEM
reference configuration. Arbitrary mid-draw switching, broader DOS programs,
active video ownership and independent VDD/grabber support remain unvalidated.
Assisted save/restore is not enabled.

## Build from the repository root

Configure Open Watcom (including `wcl`, `wlink`, `wasm`, `wrc`) and run:

```sh
make -f makefiles/Makefile test-all
sh windows/win31/build-palddi.sh
sh windows/win31/build-paltrial.sh
sh windows/win31/build-palx386.sh
sh windows/win31/build-palxhook.sh
sh windows/win31/build-palwatch.sh # optional live diagnostics
sh windows/win31/build-palxrpt.sh # validated Enhanced baseline
sh windows/win31/build-palwin31.sh # combined experiment
```

Build only the variant needed for a given experiment. Products go in
`tests/exes/`. The activation builds default to tracing; PALTRIAL alone supports
`PAL_TRIAL_TRACE=0` for an untraced build.
For PALWIN31 only, set `[Palettium] RenderTrace=0` in SYSTEM.INI to
suppress detailed drawing diagnostics while retaining startup, shutdown,
notification changes and bounded failures. Default is 1 (verbose). See
[performance isolation](../../docs/v0087-calc-trace.md) and
[shutdown drawing/cursor counts](../../docs/v0088-cursor-profile.md).
Keep tracing enabled for diagnostic sessions. Native driver link audits
check expected exports, module name and KERNEL-only imports. PALXHOOK,
PALXRPT and PALWIN31 also require the resident-segment audit.

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

Start `WIN /B` on the current VM (confirm Enhanced mode). For the startup-only fallback use PALX386.DRV instead;
it does not install the hook. Do not enable assisted save/restore.
The observer receives background/foreground notifications. Bank/cursor
resynchronization plus `PALWATCH.EXE /repaintonce` restored the desktop and
allowed normal shutdown in a VM test. The earlier visible-monitor hang is
not isolated. See [repaint and shutdown experiment](../../docs/v0082-repaint-shutdown.md).
For the validated PALXRPT native repaint configuration, follow
[the v0083 instructions](../../docs/v0083-native-repaint.md); no PALWATCH
process is needed. Retain PALXRPT and PALTRIAL as the validated single-mode
baselines. For PALWIN31, see [combined build](../../docs/v0085-unified-win31.md).
On a hang or hook-removal conflict, recover the logs before restoring the VM
snapshot. Never attempt live unloading of the observer. Save `C:\PALBOOT.TXT`
and `C:\PALWATCH.TXT` before the next launch overwrites them; also save
BOOTLOG.TXT if relevant.

## Further reading

- [Enhanced-mode VDD research](../../docs/v0075-enhanced-mode.md)
- [Enhanced startup design](../../docs/v0076-enhanced-startup.md)
- [Notification observer design and limitations](../../docs/v0078-notification-observer.md)
- [Patch notes](../../patch-notes/)
- [Historical README archive](../../docs/history/win31-readme-through-v0078.md)

Keep this guide current rather than appending milestone history.

### Application compatibility

The memory DDB adapter currently supports single-segment 1/8bpp bitmaps
only; large Paint canvases and other huge bitmaps are not validated.
Calculator and Paint application coverage is still under test. If Standard
startup reports `GATE BLOCKED` and `KEY=0000`, check `TrialEnable=4401` in
the guest SYSTEM.INI before debugging framebuffer initialization.
