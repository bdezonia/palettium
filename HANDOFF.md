# Palettium v0030 handoff — Win16 native hardware bridge (2026-11-23 UTC)

## Maintainer-confirmed baseline v0029

The user applied `pal29.zip` over v0028; all portable targets passed:
`Makefile.et4kmodes`, `.et4k`, `.win31link`, `.screen17`, `.win31ddi`,
`.bitmap`. Open Watcom linked the 13-export NE `PALSTAGE.DRV` without
warnings. `sh dos/build-et4k.sh` built ET4KTEST.EXE. **Importantly, the
user then actually ran the DOS smoke test in 86Box at 256, 512 and 1024
KiB VRAM and reported the graphics appeared correct in all three.** This
is direct hardware-model evidence, not evidence of Win16 driver startup.

## v0030 change

1. `windows/win31/pal_et4k_win16.[ch]`: Win16 `ET4K_Port` provider.
   Uses KERNEL's `__A000H` exported *protected-mode* aperture selector
   (obtained by GetModuleHandle/GetProcAddress) instead of the invalid
   DOS assumption that `0xA000` is a Win16 selector. Provides `inp`/`outp`
   3CDh/DAC callbacks, A000 banked byte access, and INT 10h BIOS modes
   2Eh/2Fh with a saved previous mode, AH=0Fh readback, and rollback.
   No destructive VRAM probing or guessed capacity. **Compiled in the NE
   staging module, but no GDI DDI entry point calls this adapter.**
2. `windows/win31/pal_et4k_session.[ch]`: OS-independent Win3.x
   session/capability coordinator; uses existing `ET4K_Port`, `et4k` backend,
   `PAL_ScreenHost` and PDEVICE candidate. Read-only GDIINFO mode selection
   follows verified 256K→640×400×256 and 512K/1M→640×480×256; ROM veto
   can choose 640×400. Starts the already-tested banked screen host and
   restores on explicit mode attempts. No new DDI exports or duplicate
   raster/ET4000 implementations.
3. Fix in `et4k.c`: a preflight refusal (unsupported format/memory/BIOS
   query) calls shutdown without restoring the prior video mode. An actual
   mode-set attempt, including a failed/partial BIOS attempt, restores on
   shutdown. Adds `mode_attempted` state and regression coverage.
4. `tests/test_win31_native.c`, `Makefile.win31native`, and updated old
   rollback assertions test these paths for all three sizes, BIOS refusal,
   missing/invalid RAM info, bank crossing, repeated start/stop and
   geometry. No binaries shipped. Docs/README updated.

## Build, test and installation

Apply only to the user-validated v0029 tree:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal30.zip
make -f Makefile.win31native test
make -f Makefile.et4kmodes test
make -f Makefile.et4k test
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make -f Makefile.bitmap test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Host C89 suite and clean v0029 overlay are checked in the assistant's
container. **Open Watcom v0030 native Win16 callback compilation and linking
must be validated by the user.** Any unresolved Win16 symbols or compiler
issues should be fixed before attempting runtime calls. 13 exports stay
unchanged. Fixed-width v0030 label; source-only incremental ZIP built from
original changed files, with full-tree SHA256 and UTC ZIP timestamp.

## CRITICAL: STILL NOT INSTALLABLE

`Enable(style=0)` MUST continue to return failure. `Enable(style=1)`
continues to expose the old development-only 640×480 GDIINFO candidate,
not dynamically negotiated geometry. `pal_win16_bind_host` is not called
by the DDI. The active screen host still owns its own PDEVICE rather than
receiving GDI's allocated PDEVICE address. There is no Win16 capacity
configuration, safe BIOS/selector runtime validation, VDD switching,
text/cursor, large bitmap support or mandatory DDI capability audit.
Do NOT add this driver to SYSTEM.INI or run native mode switching from a
normal Windows session. Win16 Windows 3.1 standard and 386-enhanced modes
need distinct VDD lifecycle treatment, including VDDsetaddresses in
enhanced mode for packed 8-bit modes.

Next: validate the Open Watcom build, then a **recoverable diagnostic
inside Windows 3.1** that safely verifies the A000 selector and BIOS
readback without installing or mode-switching the desktop display driver.
After that, integrate *actual GDI PDEVICE ownership*, mode configuration
and VDD notifications, then text/cursor and style=0 only when operational.
Keep the same C89 shared renderer, hardware-dispatch and ET4000AX bank
backend for Win3.1, WfW3.11, Win95 and Win98.

References: Microsoft Win3.1 Guide to Programming on protected-mode
KERNEL selectors; Win3.1 DDK display initialization and VDD interface;
OS/2 Museum Win16 Retro Development (VDDsetaddresses caveat).
