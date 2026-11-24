# Palettium v0037 handoff — Win3.x capability planning (2026-11-23)

## Validated baseline

- User-validated v0036 `make clean && make test`, `Makefile.et4k test`,
  `Makefile.et4kmodes test`, Open Watcom `PALSTAGE.DRV` link (13 exports),
  and DOS `build/dos/ET4KTEST.EXE` build, all PASS without compiler warnings.
- User ran ET4KTEST INFO in plain 86Box DOS with BIOS V8.01X (04/07/93),
  V8.06X (04/15/92), V1.21X (02/12/91), each at 256/512/1024 KiB:
  **9/9 PASS**. Register 37h consistently 0Dh/0Eh/0Fh; hints 256/512/1024
  KiB (8/16/32-bit memory bus, 256K chip depth). No physical card checked.
- Earlier reboot resolved: the user had transferred extensionless Linux ELF
  `tests/exes/et4k-test` and renamed it as a DOS EXE. **Never do this**.
  Transfer exactly `build/dos/ET4KTEST.EXE` instead; it works.
- User-verified Win3.1 diagnostics: PALDIAG v0033 queried KERNEL selector
  `FFFF:1027`, GetWinFlags=0C29h (386 Enhanced), and BIOS mode 12h; v0034
  read A000:0000h byte 00h without GPF; v0035 reported SYSTEM.INI
  `[386Enh] display=*vddvga`. Only configuration, not loaded VDD verified.
- Microsoft's Windows 3.1 stock VGA VDD does not work with SVGA desktop
  framebuffers using >256KiB. 640x400x8 is below that boundary, but
  compatibility with stock VDD is **unverified**.

## v0037 changes

- Source `windows/win31/pal_et4k_session.[ch]`: pure C89 `PAL_ET4KPlan`
  takes **explicit configured VRAM** and optional observed CRTC37 hint,
  rejects absence/invalid size or mismatch; selects 640x400x8 for 256KiB
  and 640x480x8 at >=512KiB except under enhanced-mode configured stock
  VDD, where it limits framebuffer candidate to <=256KiB (640x400x8).
  Never I/O, never claims VDD compatibility, `can_enable=0` invariant.
  Also provides explicit-mode session start using existing host/backend,
  without duplicate raster code; legacy preferred-mode start preserved.
- Source `windows/win31/pal_et4k_win16.[ch]`: read-only Win16 INI-profile
  adapter for `[Palettium] VRAMKiB` and `[386Enh] display`, GetWinFlags;
  optional explicit CRTC37 diagnostic refuses raw port access in 386
  Enhanced mode. No automatic CRTC capacity acceptance.
- Production `palddi.c`: style-1 `Enable` calls profile + planner to return
  a conservative GDIINFO candidate, rather than blindly claiming 640x480.
  Missing/invalid configured capacity fails closed; `style=0` **still fails**.
  Checks far-pointer segment-boundary before writing GDIINFO. Does not call
  BIOS, port I/O, VRAM, or video mode setting. 13 DDI ordinals unchanged.
- Win16 build script now includes existing `pal_vdd.c` (shared Win3.x
  policy, not an ET4000 card module). Host native Makefile includes it.
- Extended host regression tests in `tests/test_win31_native.c` validate
  256/512/1024, stock-VDD constrained 640x400, VDDVGA30 candidate,
  mismatched hint refusal, no hardware access during plan, explicit-mode
  host startup and preflight refusal. Existing tests remain intact.

## Tests and run instructions

Assistant host tests: `make clean && make test` plus all 15 auxiliary
Makefile `test` targets passed (Linux C89). Open Watcom v0037 Win16 build
and actual Windows runtime **NOT YET TESTED**. This patch requires test
on user's configured Open Watcom toolchain:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal37.zip
make clean && make test
make -f Makefile.win31native test
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.et4k test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

**Do not install the DRV, alter SYSTEM.INI, or attempt a Windows video
mode switch yet.** Style-1 read-only capability returns 0 if user has not
specified an explicit VRAM amount. It is only a staging path, not a
hardware probe or a completed GDIINFO. This is intentional.

## Next critical work

The bottleneck remains GDI startup and protected-mode integration, not
just more demos: GDI-owned persistent PDEVICE allocation, correct full
GDIINFO capabilities/metrics, cursor and text DDIs, 386 Enhanced VDD/
grabber service coordination (e.g. VDDsetaddresses), and reversible Win16
BIOS/mode lifecycle. Keep `Enable(style=0)` blocked until these work.
For all four targets (Win3.1, WfW3.11, Win95, Win98), keep a common C89
renderer with hardware-override hooks, per-card backend and small OS
adapters; do not fork raster code. Keep fixed-width v#### tags, stable
production filenames, source-only small ZIPs, executables under tests/exes,
and correct UTC 0x5455 ZIP metadata plus Chicago DOS header time.
