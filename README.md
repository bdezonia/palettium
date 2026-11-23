# Palettium

A **C89 graphics-driver framework for legacy Windows**, designed to keep
pixel rendering and video-card backends reusable across four operating-system
adapters: **Windows 3.1, Windows for Workgroups 3.11, Windows 95, and
Windows 98**.

Current development milestone: **v0027** (incremental tags are fixed-width).
The immediate objective is the **first working Windows 3.1 ET4000AX display
driver**. Palettium is *not* yet an installable driver package.

## What works today

- Shared software pixel/line/fill/BitBlt operations, with optional
  per-operation `RG_Ops` hardware overrides. Ten framebuffer formats are
  supported, from indexed 1/2/4/8/16 through RGB555/565/888 and 32-bit
  XRGB/ARGB storage. Framebuffer offsets are 32-bit or larger.
- Byte-callback framebuffers with bank-switching, host simulation, and an
  **ET4000AX backend** (`hardware/tseng/et4000ax/`) supporting native
  **640×480×8** mode, independent 64 KiB read/write banks and a 6-bit DAC.
  Video-mode selection and actual port/aperture I/O come from the platform
  callbacks; no VESA requirement is baked into the hardware backend.
- The ET4000AX DOS demo was previously run on **86Box** (display and VRAM
  readback passed). Earlier PALCORE/PALTEST Win16 graphics tests also passed
  in Windows 3.1. These are *not* proof of a working display driver.
- A linked Open Watcom NE staging module `PALSTAGE.DRV` with Windows 3.x DDI
  exports. v0024 exports for BitBlt, Enable, Disable, Output and Pixel passed
  an Open Watcom build and `wdump` verification on the maintainer's machine.
- **New in v0025:** common INDEX8 system-palette state and logical/physical
  translation, 20 reserved Windows colors, `ColorInfo`, `ScanLR`, the five
  palette operations (Set/GetPalette, Set/GetPalTrans, UpdateColors), and
  ET4000AX-backed palette and scan/remap tests. The Win16 build and all
  12 exports **passed** on the maintainer's Open Watcom installation
  (November 23, 2026).
- **New in v0026:** mandatory `RealizeObject @10` with nominal physical
  pens and solid, hollow and hatched brushes (all six hatch styles),
  a genuinely rendered `Output(OS_RECTANGLE)` with clipping and ROP2,
  and ET4000AX bank-crossing rectangle tests. Object realization and
  shape rendering reuse shared C89 code rather than duplicating the
  Windows adapter or the ET4000AX implementation. Pattern brushes are
  not yet realized. **v0026 Win16 build and all 13 exports passed** on the
  maintainer's Open Watcom system (November 23, 2026).
- **New in v0027:** the two DDK-required GDI synthesis primitives now have
  screen-backed implementations: `Output(OS_SCANLINES=4)` (multiple
  right-exclusive horizontal intervals, brush/pen, hatches and ROP2) and
  `Output(OS_POLYLINE=18)` (connected 1px segments with exclusive final
  endpoints, clipping and ROP2). Both reuse `common/rg_gdi_output.c` and
  the same hardware pixel overrides and clipping path as rectangles.
  BEGINNSCAN/ENDNSCAN hints are safe no-ops. Host tests cover geometry,
  every ROP2, XOR joins, extreme signed coordinates and ET4000AX VRAM
  bank crossing. v0027 Win16 linking **requires maintainer validation**.

## Architecture

| Layer | Location | Responsibility |
| --- | --- | --- |
| Shared raster | `include/rgfx.h`, `common/rgfx.c` | Software fallbacks, raw pixels, callbacks, overrides |
| GDI helpers | `common/rg_gdi_*.c`, `common/rg_win31_screen.c` | ROP and screen blit translation |
| Physical objects/output | `include/rg_objects.h`, `common/rg_objects.c`, `common/rg_gdi_output.c` | Pen/brush realization, hatch tests, shared clipped rectangles, scanlines and polylines |
| Color management | `include/rg_palette.h`, `common/rg_palette.c` | INDEX8 RGB table, static colors, translation, remap |
| Hardware contract | `include/rg_hardware.h`, `common/rg_hardware.c` | Mode, lifecycle, palette and optional acceleration |
| Hardware backend | `hardware/tseng/et4000ax/` | ET4000AX bank/DAC operations, no Windows ABI |
| Win3.x adapter | `windows/win31/` | DDI ABI, GDIINFO, PDEVICE, far pointers, screen host |
| Additional OS adapters | `windows/wfw311/`, `windows/win95/`, `windows/win98/` | Planned, currently placeholders |

The intention is for a **small card-specific backend** to combine with shared
software drawing and *four thin OS adapters*, rather than duplicating the
renderer for each chipset or Windows release. The Windows 95/98 interfaces and
packaging still require substantial development; Windows 3.1 and WfW 3.11
cannot be assumed binary-identical without testing.

## Build and test (Linux)

For the shared C89 renderer and existing regression suite:

```sh
make clean && make test
```

For the current Windows 3.1 and ET4000AX integration tests:

```sh
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make -f Makefile.enable22 test
make -f Makefile.et4k test
```

Tests compile with a host C89 compiler; generated executables and artifacts go
under **`tests/exes/`** (ignored by Git). The Windows-target-specific ABI is
also checked with host layout tests; these tests **do not simulate a complete
Windows 3.1 GDI startup**.

For a Windows 3.1 NE staging binary, configure Open Watcom's Win16 tools
(`wcc`, `wlink`, `wdump`) and run from the repository root:

```sh
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

This builds `tests/exes/PALSTAGE.DRV`, plus an export report. v0027 expects
**13 exports**, including `REALIZEOBJECT @10`, `COLORINFO @2`,
`SCANLR @12`, and palette DDIs `@22..26`. The script checks ordinals.
The 13-export **v0026** build passed on the maintainer's Open Watcom
system; the **v0027 Win16 link is not yet validated** there.

When applying an incremental patch downloaded through your browser:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal27.zip
```

The archive contains **repository-relative paths** (no enclosing directory).
Apply it over a **validated v0026** tree. Source ZIPs carry exact UTC timestamps in the
extended ZIP field and the local DOS ZIP header timestamp for America/Chicago.

## Important: NOT INSTALLABLE

**Do not copy `PALSTAGE.DRV` into Windows' display-driver configuration or
change `SYSTEM.INI` to select it.** The `Enable(style=0)` screen initialization
path deliberately fails. v0027 now draws the two required scanline/polyline
styles **to the screen**, but it does **not** yet support drawing to GDI-owned
monochrome or color memory bitmaps, patterned brushes, text, cursor management,
a complete GDI simulation contract, or proven Win16 mode setting and screen
switching. `Enable(style=1)` returns only a *candidate* GDIINFO; it intentionally does **not** advertise RC_PALETTE
until the rest of the palette-capable display DDI is integrated and validated.

### Next engineering priorities

Implement GDI memory-bitmap output and minimum cursor/text fallback contracts;
connect Win16 ET4000AX mode set, VRAM aperture and lifecycle to a real
GDI-owned PDEVICE; then implement the remaining required services and test
startup in a recoverable 86Box Windows 3.1 environment (standard and 386
Enhanced Mode). Keep port access OS-specific and share code with WfW/9x
where their driver contracts allow.

Check `HANDOFF.md` and `PATCH-v0027.txt` for precise checkpoint status and
remaining test gaps. Earlier `docs/` and patch notes document the milestones.
