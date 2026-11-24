# Palettium v0040 handoff — Win3.x text and cursor raster foundations

## Confirmed baseline: v0039

The maintainer validated v0039 on Ubuntu/Open Watcom on November 23,
2026. `make clean && make test`, `Makefile.win31native test`,
`Makefile.win31link test`, `Makefile.screen17 test`, and
`sh windows/win31/build-palddi.sh` all passed. `wdump -x` confirmed 13
staging NE exports. The v0039 native ET4000AX -> GDI-owned PDEVICE
transaction is implemented but still not authorized by Enable(style=0).
The known stock Windows 3.1 386-Enhanced `*vddvga` compatibility problem
persists, and a direct Windows 3.1 driver startup has NOT been tested.

Earlier real 86Box evidence: DOS ET4KTEST displays worked at 256/512/
1024 KiB. CRTC37 reported 0Dh/0Eh/0Fh across BIOS V8.01X, V8.06X,
and V1.21X (9/9). PALDIAG safely retrieved __A000H, made one byte
read and BIOS-mode readback in 386 Enhanced mode. None of these tests
constitutes permission to change video mode from the installed driver.

## New in v0040

1. Two real text DDI entry points are now compiled into the existing
   `windows/win31/palddi.c`: **StrBlt @11** and **ExtTextOut @14**.
   Both route through the same implementation. The adapter reads standard
   Windows 2.x-era GDI raster FONTINFO and glyph offsets through checked
   Win16 FAR segment windows, supports proportional widths, fallback
   characters, clipped glyph pixels, physical DRAWMODE text/background
   colors, positive drawing, negative-count extent-only and zero-count
   ETO_OPAQUE. It accepts basic supplied advance widths, CharExtra and
   BreakExtra. Unsupported formats or malformed pointers fail closed.
2. Shared `common/rg_gdi_text.c` uses no Windows/Card-specific imports:
   it draws column-major 1bpp glyph bytes through RG_Device.pixel and
   supports INDEX8 and packed INDEX1 memory devices, including banked
   screen VRAM. The Win16 font header parser (`pal_font16.c`) is separate
   from this shared renderer; no duplicate ET4000AX text engine.
3. Shared `common/rg_cursor.c` implements a 1/8bpp-index software cursor
   compositing AND/XOR masks with save-under, clipping, hotspots and
   move/hide restoration. **It is NOT yet connected to Win3.1's @101–104
   cursor DDIs**: proper CURSORSHAPE unpacking, mouse-interrupt safety,
   and cursor exclusion around every graphics DDI remain unresolved.
4. Export verifier and Win16 link source list updated to **15 exports**.
   The stable production filename stays `PALSTAGE.DRV`; test binaries
   remain in `tests/exes/`. Comprehensive `make clean` removes them.

## Strict limits (especially important)

**PALSTAGE.DRV IS STILL NOT INSTALLABLE. Enable(style=0) returns zero.**
No native mode switch can occur from GDI. Candidate GDIINFO still has
zero advertised graphics/text capability bits; do not set
RC_BIGFONT, RC_PALETTE, TC_*, or RC_GDI20_OUTPUT prematurely.

Text decoder is the STANDARD (non-big) GDI raster-font layout with a
51-byte FONTINFO base and four-byte 2.x char-table entries. It does
not support 3.x big/sparse TrueType, vector fonts, font synthesis,
rotation, underline/strikeout, full break-error justification, huge
font/bitmap segments or arbitrary glyph heights. It is only a first
real text path, NOT a complete 1992 DDK text implementation. Cursor
compositor has no OS cursor exports or interrupt-safe critical section.
Screen painting still lacks coordinated cursor exclusion, several
optional GDI services and an enhanced-mode VDD contract. Standard-mode
hardware startup is gated too. Do not install the driver in SYSTEM.INI.

The v0040 assistant tests are C89 host simulations; the real Open Watcom
Win16 ABI and 86Box Windows runtime are **NOT yet verified**. The host
wrapper test compiles actual `palddi.c` with fake FAR-pointer APIs and
ELF section garbage collection; it cannot validate real selectors.

## Build/test instructions for maintainer

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal40.zip
make clean
make test
make -f Makefile.win31text test
make -f Makefile.cursor test
make -f Makefile.win31native test
make -f Makefile.win31link test
make -f Makefile.screen17 test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

Expected additional exports: `STRBLT @11` and `EXTTEXTOUT @14`.
Do not install this DRV or switch video modes under Windows yet.

## Next: v0041

Prioritize DDK-accurate Win16 cursor `Inquire @101`, `SetCursor @102`,
`MoveCursor @103`, `CheckCursor @104` with correct CURSORSHAPE wire ABI,
interrupt-level synchronization, and exclusion around every screen draw.
Next complete standard text justification/special fonts and GDIINFO
capability reporting. Only after a real DDI readiness audit should the
first recoverable **Standard-mode** 86Box install test be authorized;
386 Enhanced requires separate validated VDD coordination. Maintain
four-OS adapters with shared C89 common code and card-specific backends.

## Packaging discipline

Fixed-width sequential milestone tags; keep existing git tags untouched;
source-only small additive ZIP patches built freshly from changed source;
full source SHA256 manifest; UTC Extended Timestamp (0x5455) and matching
America/Chicago local DOS ZIP field; stable filenames; test executables
under tests/exes; globally comprehensive `make clean`.
