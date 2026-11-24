# Palettium

A **C89 graphics-driver framework for legacy Windows**, designed to keep
pixel rendering and video-card backends reusable across four operating-system
adapters: **Windows 3.1, Windows for Workgroups 3.11, Windows 95, and
Windows 98**.

Current development milestone: **v0038** (incremental tags are fixed-width).
The immediate objective is the **first working Windows 3.1 ET4000AX display
driver**. Palettium is *not* yet an installable driver package.

## What works today

- Shared software pixel/line/fill/BitBlt operations, with optional
  per-operation `RG_Ops` hardware overrides. Ten framebuffer formats are
  supported, from indexed 1/2/4/8/16 through RGB555/565/888 and 32-bit
  XRGB/ARGB storage. Framebuffer offsets are 32-bit or larger.
- Byte-callback framebuffers with bank-switching, host simulation, and an
  **ET4000AX backend** (`hardware/tseng/et4000ax/`) supporting native
  **640×480×8** (requires at least 512 KiB) and **640×400×8** (fits
  256 KiB) packed modes, independent 64 KiB read/write banks and a 6-bit
  DAC. Mode selection considers verified VRAM capacity and optionally BIOS
  support; actual port/aperture I/O comes from platform callbacks. Planar
  VGA 640×480×16 is not yet rendered by this backend; no VESA requirement.
- The ET4000AX DOS demo was **run by the maintainer in 86Box with 256, 512
  and 1024 KiB VRAM**, and the graphics appeared correct in all three
  configurations (November 23, 2026). Earlier PALCORE/PALTEST Win16 graphics tests also passed
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
  bank crossing. The maintainer **validated the v0027 Open Watcom Win16
  build and 13 exports on November 23, 2026**.
- **v0028 (maintainer-validated Win16, November 23, 2026):**
  1bpp and 8bpp GDI-owned memory BITMAPs with checked 32-byte Win3.x
  headers and single-segment byte callbacks. `Output` now supports
  rectangle/scanline/polyline drawing to memory BITMAPs as well as screens.
  `BitBlt` supports screen↔memory and memory↔memory with same-format ROPs
  and historical monochrome↔8-bit conversion (1=background, 0=text).
  The common raster and existing ET4000AX hardware path are reused,
  including bank crossing; patterned ROP3 still refuses. Huge segmented
  BITMAPs and memory-bitmap Pixel/ScanLR are **not** implemented.
- **v0029 (maintainer-validated, November 23, 2026):** ET4000AX mode catalog and
  memory-capacity gates. 256 KiB cards can use the 640×400×256 packed mode
  (Tseng BIOS 2Fh) without pretending 640×480×256 fits. For 512 KiB or
  1 MiB cards, 640×480×256 (Tseng BIOS 2Eh) is preferred; a firmware
  refusal can trigger a 640×400 candidate fallback. Known capacity can be
  supplied by the OS adapter, or a strictly read-only platform query can
  discover it. **The actual firmware mode-set callback must succeed**;
  capacity by itself does not guarantee that an individual card's BIOS
  supports either mode. The Win3.1 PDEVICE/GDIINFO host path now passes
  640×400 geometry tests. The existing DOS smoke test now requires an
  explicit `ET4KTEST 256|512|1024` argument matching a disposable 86Box
  VM's VRAM setting and uses BIOS 2Fh or 2Eh as appropriate. The Win16
  staging binary does **not** yet negotiate modes or initialize the native
  ET4000AX hardware.

- **v0030 (host-tested; Win16 compilation failed at the missing ET4K header):** a protected-mode
  Windows 3.1 ET4000AX platform adapter uses KERNEL's `__A000H` mapped
  selector rather than a real-mode `A000h` pointer, along with native byte
  port I/O and an INT 10h mode-set/readback callback. A platform-neutral
  Win3.x ET4000AX session coordinator chooses GDIINFO geometry from
  **verified** VRAM, starts/stops the existing screen host, and rolls back
  failed firmware attempts. Pure preflight refusals no longer reset the
  user's video mode. **The native adapter is only compiled/linked, not
  activated by the DDI**: VDD coordination, GDI-owned PDEVICE binding,
  cursor/text and mode configuration are still needed before installation.

- **v0031 (maintainer-validated build-repair checkpoint):** the Open Watcom Win16 link recipe now
  supplies the ET4000AX header path and compiles the existing ET4000AX
  backend, fixing the maintainer-reported v0030 include error and the
  subsequent unresolved-symbol risk. `make clean` is now **project-wide**:
  it removes all generated binaries under `tests/exes/`, DOS products under
  `build/`, and legacy Win16 probe outputs while preserving sources and
  checked-in link recipes. A maintenance regression checks both. **Open
  Watcom v0031 compilation and 13-export link **passed** on the maintainer's
  machine. No DDI, raster or mode
  behavior was changed, and the driver is still not installable.

- **v0032 (maintainer-tested Win16 build, Windows runtime FAILED):**
  The standalone Win16 `PALDIAG.EXE` compiled and linked under Open Watcom,
  alongside the 13-export staging display driver. The first attempt to run
  PALDIAG inside Windows 3.1 instead raised **GP fault KRNL386.EXE
  0001:2272**, before a diagnostic dialog appeared. Its previous use of
  `GetSelectorBase` on bit-pattern-selected words is not an adequate
  validity check; the precise faulting call is **not established**.

- **v0033 (maintainer-validated Windows 3.1 runtime):** The revised
  `PALDIAG.EXE` ran successfully in Windows 3.1 under 86Box. Screenshots
  show `GetModuleHandle("KERNEL") = 010F`, the exported `__A000H` address
  `FFFF:1027` (documented selector candidate `1027` in the low word),
  `GetWinFlags = 00000C29` (protected / 386 Enhanced mode), and the
  *optional* INT 10h AH=0Fh query returning video mode `12h`, 80 columns,
  page 0. No GP fault occurred and no mode change was attempted.
  The earlier v0032 fault remains unlocalized; `GetSelectorBase` was
  removed for v0033, but that does not prove it caused the fault.
- **v0034 (maintainer-validated Windows 3.1 A000h read):** The 86Box
  Windows 3.1 STEP 5A/5B screenshots confirm one protected-mode
  `A000h:0000h` byte read via selector `1027h` returned **00h**, without
  a GP fault. This does **not** prove unvirtualized VRAM mapping or port,
  bank and mode-set compatibility with Windows' VDD.

- **v0035 (VDD compatibility reconnaissance; Windows runtime PASS):**
  The existing `PALDIAG.EXE` can now optionally read the configured
  `[386Enh] display=` entry from `SYSTEM.INI`, after the existing STEP 5.
  Declining the experimental byte read does **not** block this new STEP 6.
  A new tiny Win3.x-only C89 classifier recognizes `*VDDVGA`,
  `VDDVGA30.386`, and unknown/OEM settings. It warns **KNOWN INCOMPATIBLE**
  if stock Windows 3.1 VDD is configured for 640×480×256's 307,200-byte
  framebuffer. All other VDD cases remain **UNVERIFIED**: configuration
  does not establish the VxD loaded or a safe mode switch. It makes no
  changes to `SYSTEM.INI` and adds no DDI exports. Windows 3.1's default
  VGA VDD is documented to be incompatible with SVGA display drivers
  using over 256 KiB (Microsoft KB80901). Even the 256 KiB ET4000AX
  packed-color mode requires testing with the VDD. **The maintainer's
  v0035 STEP 6 screenshot confirmed `SYSTEM.INI [386Enh] display=*vddvga`
  in 86Box**, with 640×480×8 flagged known incompatible and 640×400×8
  unverified. This is the configured VDD string, not proof of which VDD is
  actively loaded.
- **v0036 (host-tested; DOS runtime awaiting user validation):** static
  analysis of six user-provided 86Box ET4000 option ROMs found BIOS-specific
  CRTC register `37h` memory-organization and mode-eligibility logic. A
  strictly diagnostic snapshot helper in the existing ET4000AX backend
  reads the active color/mono CRTC index/data pair and **restores its previous
  index**. The helper decodes a *candidate* capacity from bus width and chip
  depth, but never calls a VRAM probe, changes a graphics mode, or promotes
  the hint to a verified size. `ET4KTEST INFO` under **plain DOS** prints
  the C000h video ROM version/date hint and raw register `37h` without any
  video-mode transition or framebuffer/bank access; existing
  `ET4KTEST 256|512|1024` graphics tests print the hint first and still
  obey the explicitly provided VRAM size. Reading index `37h` requires two
  index-port writes (select/restore), **not** register-data writes.
  Register accessibility, BIOS variation, and 86Box interpretations must be
  tested on the maintainer's VM before treating a capacity hint as useful.

- **v0036 runtime validation (maintainer, November 23, 2026):**
  the correct DOS `build/dos/ET4KTEST.EXE` ran `INFO` in 86Box under
  V8.01X (1993), V8.06X (1992), and V1.21X (1991) BIOSes at 256/512/1024 KiB
  each. **9/9 configurations** reported CRTC 37h `0Dh/0Eh/0Fh`, with
  capacity hints matching the configured memory. The values were observations
  from one emulator family, not proof of VRAM on physical cards. The similarly
  named extensionless `tests/exes/et4k-test` is a Linux ELF executable and
  must NEVER be transferred to DOS or renamed to `.EXE`.
- **New in v0037 (host-tested; Win16 build pending):** the Win3.x startup
  path uses a read-only, explicitly configured `[Palettium] VRAMKiB`
  (`256`, `512`, or `1024`) in SYSTEM.INI for **GDI `Enable(style=1)`**.
  Missing/invalid capacity returns zero instead of always claiming 640x480.
  The shared ET4000AX + Win3.x planner chooses 640x400x8 on a 256 KiB
  card and a 640x480x8 candidate on larger cards, **except** under the
  configured stock Windows 3.1 VGA VDD in 386 Enhanced mode, where the
  >256KiB 640x480 framebuffer is excluded and 640x400 remains **UNVERIFIED**.
  Optional CRTC37 hints can reject mismatched configuration but can never
  authorize more VRAM. A separate explicit-mode screen-host test exercises
  the 640x400 candidate on 512 KiB. No mode set, VRAM access, or port I/O
  is performed by style-1 planning. Raw Win16 CRTC37 access is refused in
  Enhanced mode unless an actual VDD coordination path is developed.
  **`Enable(style=0)` still fails; do not install PALSTAGE.DRV.**

- **v0038 (host-tested; Open Watcom validation pending):** the Win16
  adapter now has an opt-in, **dormant GDI-owned PDEVICE publication** step.
  Only a fully started screen host matching the caller's exact segment:offset
  token may produce the 20-byte physical-device image. The Win16 adapter
  checks that the GDI-allocated destination does not cross a 64 KiB segment
  boundary, copies the image as far bytes, and only then binds drawing.
  `Disable` refuses unrelated PDEVICE tokens. This does **not** initialize
  the ET4000AX in the exported `Enable(style=0)` path (still blocked).
  The 110-byte GDIINFO candidate also has coherent 96-DPI mapping metrics,
  integer physical dimensions, and default square-pixel aspects; **drawing,
  bitmap, palette and font capability flags remain zero** until startup is
  operational and those promises can be honored. Core and ET4000AX mock
  tests cover publication success/failure, stale/mismatched devices and
  standard metric transforms. The card-specific hardware/backend and shared
  C89 rendering remain unchanged.

## Architecture

| Layer | Location | Responsibility |
| --- | --- | --- |
| Shared raster | `include/rgfx.h`, `common/rgfx.c` | Software fallbacks, raw pixels, callbacks, overrides |
| GDI helpers | `common/rg_gdi_*.c`, `common/rg_win31_screen.c` | ROP and screen blit translation |
| Physical objects/output | `include/rg_objects.h`, `common/rg_objects.c`, `common/rg_gdi_output.c` | Pen/brush realization, hatch tests, shared clipped rectangles, scanlines and polylines |
| Color management | `include/rg_palette.h`, `common/rg_palette.c` | INDEX8 RGB table, static colors, translation, remap |
| Hardware contract | `include/rg_hardware.h`, `common/rg_hardware.c` | Mode, lifecycle, palette and optional acceleration |
| Hardware backend | `hardware/tseng/et4000ax/` | ET4000AX bank/DAC operations, no Windows ABI |
| Win3.x adapter | `windows/win31/` | DDI ABI, GDIINFO, PDEVICE, far pointers, memory bitmap adapter, screen host |
| Additional OS adapters | `windows/wfw311/`, `windows/win95/`, `windows/win98/` | Planned, currently placeholders |

The intention is for a **small card-specific backend** to combine with shared
software drawing and *four thin OS adapters*, rather than duplicating the
renderer for each chipset or Windows release. The Windows 95/98 interfaces and
packaging still require substantial development; Windows 3.1 and WfW 3.11
cannot be assumed binary-identical without testing.

## Build and test (Linux)

The top-level `make clean` removes **all** generated test executables,
Win16 driver objects/exports, DOS build outputs, and historical Win16 probe
binaries. It is idempotent; recompilation is required afterwards. For the
shared C89 renderer and existing regression suite:

```sh
make clean && make test
```

For the current Windows 3.1 and ET4000AX integration tests:

```sh
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make -f Makefile.bitmap test
make -f Makefile.enable22 test
make -f Makefile.et4k test
make -f Makefile.et4kmodes test
make -f Makefile.win31native test
make -f Makefile.win31diag test
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

This builds `tests/exes/PALSTAGE.DRV`, plus an export report. v0038 retains
**13 exports**, including `REALIZEOBJECT @10`, `COLORINFO @2`,
`SCANLR @12`, and palette DDIs `@22..26`. The script checks ordinals.
The 13-export **v0029** build passed on the maintainer's Open Watcom
system. The **v0030 build stopped** on a missing `et4k.h` include directory;
v0031 adds that directory and includes `et4k.c` in the Win16 link recipe.
The **v0031 Open Watcom build passed on the maintainer's machine**.
The v0032 PALDIAG build succeeded but its original Windows runtime failed; v0033 and v0034 PALDIAG then succeeded in 86Box, and the v0035 STEP 6 VDD-configuration screenshot succeeded. v0036 changes only the DOS diagnostic and shared ET4000 backend; its Open Watcom DOS build and runtime still await maintainer testing.

When applying an incremental patch downloaded through your browser:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal36.zip
```

The archive contains **repository-relative paths** (no enclosing directory).
Apply it over the **v0035** tree. Source ZIPs carry exact UTC timestamps in the
extended ZIP field and the local DOS ZIP header timestamp for America/Chicago.

## Important: NOT INSTALLABLE

**Do not copy `PALSTAGE.DRV` into Windows' display-driver configuration or
change `SYSTEM.INI` to select it.** The `Enable(style=0)` screen initialization
path deliberately fails. v0029 adds ET4000AX mode/capacity selection; v0028 added 1/8-bit memory
bitmaps to GDI `Output` and `BitBlt`, but huge/planar bitmaps, memory
`Pixel`/`ScanLR`, patterned brushes, text, cursor management, the complete
GDI contract and real Win16 mode setting/screen switching are missing. `Enable(style=1)` returns only a *candidate* GDIINFO; it intentionally does **not** advertise RC_PALETTE
until the rest of the palette-capable display DDI is integrated and validated.

### Next engineering priorities

Complete the Win16 platform-specific **read-only VRAM discovery** and **BIOS
mode-acceptance verification**, then wire native mode set, VRAM aperture,
GDIINFO geometry selection and the active PDEVICE lifecycle into an actually
operational driver. The interim v0029 backend accepts known 256K/512K/1M
capacities but does not detect chips itself. Develop planar 640×480×16
separately if a 480-line desktop on 256K boards is required. Add minimum
text/cursor and missing bitmap DDIs and test Win3.1 standard-mode startup
first, then 386 enhanced mode with the appropriate VDD and screen switching,
in a recoverable 86Box VM. Preserve shared code across the four OS targets.

Check `HANDOFF.md` and `PATCH-v0036.txt` for precise checkpoint status and
remaining test gaps. Earlier `docs/` and patch notes document the milestones.
