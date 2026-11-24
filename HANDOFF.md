# Palettium v0042 handoff — Win16 mouse critical region and installation audit

## Validated baseline: v0041 (user result, Nov 24, 2026)

The maintainer applied v0041 and ran `make clean && make test`, Win31 cursor,
text, shared cursor, native session, Win31 link and ET4000AX screen suites:
**all PASS**. Open Watcom compiled/linked `PALSTAGE.DRV`; wdump confirmed
**19** expected exports including cursor @101..104. There were **eight
nonfatal W124** comparisons-always-false in `common/rg_cursor.c` lines
44–45 and `windows/win31/pal_cursor16.c` line 18.

## v0042 change set

1. Remove the eight redundant 16-bit signed-range checks (no warning
   suppression). On 32-bit hosts the shared cursor still validates 16-bit
   hotspot ranges, guarded by `#if INT_MAX > 32767`.
2. Replace the C-only `writing` flag in mouse-IRQ `MoveCursor` with one
   6-byte mailbox: 16-bit ready, X, Y. On the 16-bit Open Watcom target,
   `#pragma aux` emits `PUSHF/CLI` and the Windows DDK `POPFF` trampoline
   (`JMP/IRET/PUSH CS/CALL`) around both publication and consumption. Do not
   replace with raw `STI` or `POPF`. Only the platform adapter contains this
   code; the shared C89 cursor raster remains unchanged.
3. `CheckCursor` uses a non-reentrant `composing` guard. An IRQ that arrives
   during a pixel write publishes a *new* pending position for the next check,
   and cannot recursively repaint the cursor. Unbind, bind and shape changes
   suppress timer painting while modifying the cursor state.
4. Extend host tests to inject a simulated nested timer+mouse callback during
   banked cursor paint, verifying no recursive composition or dropped queue.
   Source-build guard checks both DDK-style critical sequences and the
   permanent `Enable(style=0)` refusal.

**Crucial distinction:** All portable C89 and host tests may pass without
executing the Win16 Watcom assembly. We cannot assert Win16 assembler syntax,
IRQ timing, DS/pointer behavior, or VDD integration until separately tested
on the maintainer's machine and subsequently in recoverable 86Box Windows.

## Strict installation blockers, in priority order

- **Windows runtime:** native cursor IRQ/timer behavior has not been tested
  with an active Palettium screen host; only host simulation is available.
- **Enhanced-mode VDD:** known `[386Enh] display=*vddvga` from PALDIAG. This
  setting cannot be assumed safe for any ET4000 AX mode; >256 KiB framebuffers
  exceed the stock VDD's documented support. No VDDsetaddresses handshake.
- **Mode handoff:** no permitted, verified Win16 hardware mode transition
  inside Windows, or restart/resume/fullscreen DOS save-restore lifecycle.
- **GDI/ABI:** `Enable(style=0)` remains disabled; no Windows runtime GDI
  PDEVICE test, significant FONTINFO limitations, no huge bitmap support,
  and other unverified mandatory entry-point/resource requirements.
- **Install package:** no proven driver SYSTEM.INI/OEMSETUP.INF integration
  or tested recovery image. Do not present a staged NE link as bootable.

The strongest path to a first desktop is an isolated **standard-mode**
Windows 3.1 VM configured with verified 256-KiB ET4000AX VRAM, explicit
Palettium VRAM setting and a snapshot. This still requires a separate
native Enable and cursor/runtime approval and is **not authorized by v0042**.
Keep the four-OS reusable backend and avoid copying the Windows DDI into a
second implementation.

## Next build validation (maintainer)

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal42.zip
make clean && make test
make -f Makefile.win31cursor test
make -f Makefile.win31text test
make -f Makefile.win31native test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Expected 19 NE exports (unchanged). Record any Open Watcom errors or warnings
in `pal_cursor16.c`. Do **not** install PALSTAGE.DRV in Windows yet.

## Research evidence retained

Previous PALDIAG in 86Box Win3.1 returned `__A000H` = `FFFF:1027`, one
successful read of `A000h:0000h`, BIOS `INT 10h` readback mode `12h`,
GetWinFlags `0C29h`; `[386Enh] display=*vddvga` configured. DOS ET4KTEST
INFO correctly decoded CRTC37 as 0Dh/0Eh/0Fh for 256/512/1024 KiB across
three BIOSes (9/9). None establishes a functioning Win3.1 display driver.

## Packaging rules

Use fixed-width sequential tags (v0041, v0042, ...), short incremental ZIPs
built fresh from changed files, full-source checksums, correct UTC ZIP extra
field + Chicago DOS timestamps, preserve stable production filenames,
outputs under `tests/exes` and comprehensive `make clean`.
