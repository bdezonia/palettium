# Palettium v0041 handoff — staged Win16 mouse cursor DDIs

## Baseline: v0040 user validated

Maintainer built v0040 on Ubuntu/Open Watcom (November 24, 2026).
`make clean && make test`; text, cursor, Win31 link, native session and
ET4000AX screen tests all PASSED. `sh windows/win31/build-palddi.sh`
linked `tests/exes/PALSTAGE.DRV` with all 15 expected exports including
`StrBlt @11` and `ExtTextOut @14`, verified using wdump. No reported
compiler/linker warnings. This confirms compilation, not Windows startup.

Earlier 86Box runtime data remains valid: ET4KTEST DOS output at
256/512/1024 KiB, CRTC37=0Dh/0Eh/0Fh on V8.01X/V8.06X/V1.21X
(9/9), PALDIAG protected mode A000h:0000h read and BIOS mode readback.
The v0035 VDD report was stock `*vddvga` in 386 Enhanced mode. None
proves Windows mode transitions or the installable driver lifecycle.

## v0041 changes

1. New Win16 display DDI exports in existing `palddi.c`: `Inquire @101`
   (4-byte CURSORINFO, X=1/Y=2), `SetCursor @102`, `MoveCursor @103`,
   and `CheckCursor @104`. Export count **19** with stable production
   filename `PALSTAGE.DRV`.
2. `windows/win31/pal_cursor16.[ch]` decodes GDI CURSORSHAPE's six
   little-endian signed WORDs followed by row-stride-four AND/XOR masks,
   in that order. Hotspots may be negative. Masks must be 1..32 pixels
   wide/tall, widthbytes=4, color=0 and fit within a single Win16 segment.
   Bad shapes do not replace a working cursor. The OS-only FAR copy
   remains in `palddi.c`; shared C89 cursor raster is reused.
3. `MoveCursor` queues coordinates without BIOS, VRAM, bank or port I/O.
   `CheckCursor` applies the latest completed coordinates only if no DDI
   drawing transaction is active. Cursor exclusion brackets BitBlt,
   Pixel read/write, ScanLR, Output, StrBlt, ExtTextOut and UpdateColors.
   Cursor save-under is restored before screen access and re-composited
   afterward. The shared software cursor now supports negative hotspots.
4. New host tests validate wire ABI, malformed shapes, ordinary/banked
   save-under, deferred movements, cursor/text overlap, and pixel-read /
   pixel-write exclusion. Full C89 regressions pass in assistant runtime;
   **Open Watcom v0041 compilation is not yet user validated**.

## Critical safety/functional limit — not installable

`Enable(style=0)` still returns failure. This is intentional and MUST NOT
be changed merely because all 19 NE exports link. Win16 mouse interrupt
reentrancy is not fully solved: coordinate publication has a C-only busy
flag but no validated atomic EnterCrit/LeaveCrit equivalent; nested
MoveCursor calls may lose a newer position. Likewise CheckCursor must be
protected against timer-interrupt reentry and hardware bank manipulation.
Current cursor code is a working host-tested staging path, **not an
interrupt-safe production DDI**. Do not install PALSTAGE.DRV in SYSTEM.INI.
Other remaining blockers: 386 Enhanced VDD, native display-mode handoff,
standard-mode authorization, large fonts, additional Windows driver resource
requirements and a real 86Box desktop installation audit.

## Build and validate

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal41.zip
make clean && make test
make -f Makefile.win31cursor test
make -f Makefile.win31text test
make -f Makefile.cursor test
make -f Makefile.win31native test
make -f Makefile.win31link test
make -f Makefile.screen17 test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Expected 19 exports; new ones are INQUIRE @101, SETCURSOR @102,
MOVECURSOR @103, CHECKCURSOR @104. Report compiler errors/warnings if any.
No need to run this DRV under Windows until the startup gate is cleared.

## Next: v0042

Prioritize an actually verified Win16 interrupt-safe critical primitive
and non-reentrant cursor compositor, check DDK mouse callback and exclusion
semantics in a recoverable 86Box VM, then another strict install-readiness
audit. Do not let additional diagnostics displace the first desktop goal.

## Packaging

Fixed-width sequential milestone names. Fresh source-only incremental ZIP
built from original modified files, full-source SHA256 manifest, ZIP 0x5455
UTC mtime and America/Chicago DOS header. Stable filenames, C89 shared
framework for Windows 3.1/WfW 3.11/95/98, outputs under tests/exes and
comprehensive `make clean`.
