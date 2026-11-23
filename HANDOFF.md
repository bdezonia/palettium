# Palettium v0036 handoff — ET4000 ROM memory-configuration diagnostics (2026-11-23 UTC)

## Verified baseline

- The user validated v0031 `make clean`, all host tests, and the Open
  Watcom `PALSTAGE.DRV` build/link with 13 Windows 3.1 DDI exports.
- The user ran ET4KTEST under 86Box DOS with 256, 512 and 1024 KiB of
  ET4000AX VRAM; graphical output appeared correct in all three cases.
- v0033 Win16 PALDIAG ran in Windows 3.1 386 Enhanced mode:
  `GetModuleHandle("KERNEL")=010F`, `__A000H=FFFF:1027`,
  `GetWinFlags=0C29`, INT10 AH=0Fh reported mode 12h.
- v0034 Win16 PALDIAG STEP 5B successfully read A000h:0000h as `00h`,
  without GP fault; this is one selector read, not proof of real VRAM
  identity or VDD support.
- **v0035 STEP 6 confirmed in a user Windows 3.1 screenshot:**
  `SYSTEM.INI [386Enh] display=*vddvga` (configured stock VGA VDD).
  Screenshot reports 640x480x8 known incompatible with that VDD,
  640x400x8 unverified. Actual loaded VDD and behavior not confirmed.

## ROM research (user-uploaded et4kroms.zip)

Six 32KiB option ROMs (generic V8.01X/V8.06X, V1.10X/V1.21X,
TriGem V1.31X, Kasan V8.02X) have valid 55AA headers/checksums; two
512KiB Korean ROMs are fonts. ROM disassembly shows register `37h` used
for video-memory bus/chip configuration in BIOS memory checks. The V8
BIOSes exempt certain extended modes (including 2Fh) from a higher-memory
eligibility check, unlike the earlier V1 series. The BIOSes also perform
**destructive memory tests during initialization**, which must never be
copied into a live Windows driver. Static findings are in the earlier
`et4k_audit.zip` (no proprietary ROM bytes in that artifact).

## v0036 additions

- `et4k_crtc37_snapshot` / `et4k_crtc37_decode` added to existing ET4000AX
  backend. Exactly two CRTC INDEX-port writes (select 37h, restore saved
  index); read miscellaneous output 3CCh to choose 3D4h/3D5h or 3B4h/3B5h.
  No CRTC DATA writes, no bank/VRAM I/O, no BIOS or mode changes.
- Decodes register bits 0–1 as an 8/16/32-bit bus, bit 3 as nominal
  64K/256K chip depth, yielding a **candidate** memory organization in
  bytes. The register may be protected, stale, or interpreted differently
  by BIOS/boards, so this is NOT verified installed VRAM and never drives
  `et4k_setup` or mode selection automatically.
- The existing stable DOS executable `ET4KTEST.EXE` adds `INFO` option:
  read-only CRTC37 plus a C000h ROM version/date signature hint, no mode
  setting or framebuffer activity. `ET4KTEST 256|512|1024` behavior and
  explicit VRAM requirement are preserved; those paths also print INFO.
- `tests/test_et4k_crtc37.c` mocks all raw port I/O, verifies both active
  CRTC port pairs, previous-index restoration, 8 bus/depth combinations,
  failure on unavailable callbacks, and forbids CRTC DATA / bank writes.
- Same 13 DDI exports; dormant native Win16 adapter remains unchanged;
  `PALSTAGE.DRV` is STILL NOT INSTALLABLE (`Enable(style=0)` fails).

## Testing and acceptance

Host Linux C89 tests and ZIP overlay/UTC timestamp checks run by assistant;
**Open Watcom builds and DOS INFO output of v0036 have NOT been verified
by the maintainer yet**. The user has validated the v0035 Windows runtime
VDD configuration step, which is unrelated to this DOS-only patch.

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal36.zip
make clean && make test
make -f Makefile.et4k test
make -f Makefile.et4kmodes test
sh windows/win31/build-palddi.sh
sh dos/build-et4k.sh
```

In **plain DOS**, in a recoverable ET4000AX 86Box VM (never a Windows DOS
box), run `ET4KTEST INFO` at 256, 512 and 1024 KiB. Record complete
CRTC37 raw value, decoded organization and reported C000h BIOS identity;
record the actual selected ROM from 86Box settings. If convenient repeat
under V8 and V1 BIOSes. The output is a clue, **not** permission to try
unsafe modes or automatically select a VRAM size.

## Architecture and next priorities

Keep C89 renderer/common mode interface and card-specific ET4000 backend
reusable for Win3.1, WfW3.11, Win95, Win98. No redundant raster implementation.
The actual blocker for a desktop remains Win3.1 GDI startup: persistent
GDI-owned PDEVICE, text/cursor and any remaining critical DDIs, native
mode lifecycle, rollback, and VDD/grabber cooperation. `*vddvga` is
incompatible with a >256 KiB SVGA framebuffer under Windows 3.1 386
Enhanced mode (Microsoft KB 80901). 640x400x8 on 256 KiB is not thereby
proved compatible. Do NOT install PALSTAGE or switch Windows display
configuration until these are resolved. Preserve comprehensive `make clean`,
`tests/exes`, fixed-width v#### labels, small incremental source-only ZIP,
UTC 0x5455 plus America/Chicago-local DOS ZIP timestamps, and explicit
host vs Open Watcom vs 86Box validation status.
