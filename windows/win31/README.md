# Windows 3.1 staging display driver (v0035)

`palddi.c` is the stable Win16 DDI entry-point file. `build-palddi.sh` builds
the current `PALSTAGE.DRV` NE module with Open Watcom from the repository root:

```sh
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

The module exports BitBlt @1, ColorInfo @2, Disable @4, Enable @5,
Output @8, Pixel @9, **RealizeObject @10**, ScanLR @12, and palette
operations @22 through @26. `Output` now renders rectangles (6) **and** the two fundamental GDI
synthesis primitives: `OS_SCANLINES` (4) and `OS_POLYLINE` (18). Both use
shared clipping and ROP2 through the existing bank-aware pixel dispatch.
`OS_BEGINNSCAN` (80) / `OS_ENDNSCAN` (81) are no-op bracketing hints.
Other shapes request GDI simulation (-1). In v0028, rectangles,
scanlines and polylines can also target a validated GDI memory BITMAP:
packed mono 1bpp or INDEX8, single-segment only. BitBlt now includes
single-segment memory↔screen and memory↔memory transfers, with DDK
mono/color conversion using the DRAWMODE physical foreground/background
indices. Supported ROPs remain pattern-independent. Pixel and ScanLR
are still screen-only; memory BITMAPs larger than 64 KiB are rejected.
See `README.md` in the project root for test commands and architecture.

**This driver is NOT INSTALLABLE.** `Enable(style=0)` is deliberately disabled;
`Enable(style=1)` returns a candidate GDIINFO with zero advertised drawing or
palette capability bits. The palette and output machinery is only reachable in an *already running*
screen host. The Win16 ET4000AX mode initialization, active GDI PDEVICE,
huge/multiplane memory bitmaps, patterned brushes, text, cursor and window
switch paths remain unfinished. v0029 passed Open Watcom; v0030 failed while compiling `pal_et4k_session.c` because the ET4K include directory was absent. v0031 fixes this include path and adds the ET4K backend to the Win16 link list; the maintainer confirmed the v0031 Win16 build and all 13 exports. Never select PALSTAGE.DRV
in SYSTEM.INI.

Win16 segmented-memory caveat: the screen framebuffer is larger than 64 KiB;
access it only through the hardware backend's banked byte callbacks, not a
near pointer. `PAL_ScreenHost` must be held in persistent storage, not a small
Win16 stack. If a driver backend is implemented, its I/O and BIOS mode set
belong in platform adapters, not `common/` or the generic ET4000AX backend.

The older `palprobe`, `palcore`, `paltest` and other probe files are retained
for reference. Do not build them into PALSTAGE.DRV or clone the stable DDI
implementation into new numbered production files.

## Mode-selection status (v0031)

The shared ET4000AX backend accepts 640x400x256 on verified 256 KiB cards
(Tseng BIOS mode 2Fh), and 640x480x256 on cards with at least 512 KiB
(mode 2Eh). Optional read-only VRAM-capacity and BIOS-mode-query platform
callbacks support validation; actual mode changes must be acknowledged by
`set_mode`. Current Win16 `Enable(style=1)` STILL returns the old hardcoded
640x480 **candidate** and `Enable(style=0)` still fails. Native Windows
mode programming, 256K-specific GDIINFO discovery, A000 aperture mapping,
planar 16-color output and enhanced-mode VDD coordination are not done.
This new catalog is useful in host integration but does NOT make the NE
module appropriate for installation.

## Native adapter checkpoint (v0031)

`pal_et4k_win16.c` is **real Win16 port/aperture/BIOS adapter code**, not an
active DDI service. It resolves the protected-mode selector exported by
KERNEL (`__A000H`, ordinal 174), rather than illegally using `0xA000` as
a segment selector. It provides the byte I/O callbacks of `ET4K_Port` and
uses BIOS INT 10h for mode selection, with a pre-mode save and attempted
restore. The BIOS mode is read back via AH=0Fh; some ROMs may report an
unexpected value, in which case the adapter refuses and attempts rollback.
The BIOS/selector paths have not yet run inside Windows.

`pal_et4k_session.c` is a platform-independent GDI/capability preflight
and screen-host lifecycle on top of that same ET4000AX backend. It requires
a verified capacity; Win16 provides **no automatic size detector** at this
checkpoint, and no DDI entry point calls the adapter or coordinator. The
current `Enable(style=1)` still returns the original 640x480 development
candidate, NOT a negotiated geometry; style=0 still fails. A working
display driver additionally needs GDI's PDEVICE pointer bound correctly,
capacity configuration, mouse cursor, text, and 386-enhanced VDD cooperation
(including VDDsetaddresses for packed SVGA on Win3.1). **Do not install.**

## v0031 Win16 build repair and cleanup

`build-palddi.sh` now supplies `-i=hardware/tseng/et4000ax` to Open
Watcom and compiles `hardware/tseng/et4000ax/et4k.c` into the staging
module. The previously reported missing `et4k.h` caused cascading
`ET4K_Port`/`ET4K_State` errors; missing `et4k.c` would also have blocked
linking. This **passed** on the maintainer's Open Watcom
toolchain with all 13 exports. The driver still deliberately refuses `Enable(style=0)`.

From the project root, `make clean` now deletes all generated artifacts
under `tests/exes/` and `build/`, plus historical Win16 probe DLL, EXE and
OBJ files, without removing `.LNK`, `.c`, or `.h` sources.

## v0033 PALDIAG crash-isolation revision

The maintainer's v0032 `PALDIAG.EXE` build succeeded, but running it in
Windows 3.1 raised a GPF in `KRNL386.EXE` at `0001:2272` before any dialog.
That alone does **not** identify the failing API. The original diagnostic
called `GetSelectorBase` for words passing an incomplete selector-pattern
check; the Win3.1 kernel can fault on an invalid descriptor. The new
v0033 diagnostic **does not call `GetSelectorBase` at all**.

`PALDIAG.EXE` now shows STEP 0 at the beginning of `WinMain` without any
KERNEL probe. Each next probe requires a separate Yes: `GetWinFlags`,
`GetModuleHandle("KERNEL")`, then `GetProcAddress("__A000H")`.
After a successful STEP 3, record raw high:low address and the documented
low-word selector. Decline the optional BIOS query on the first run.
If any step crashes Windows, note **the last dialog seen and whether Yes
was pressed**. If STEP 0 never appears, investigate EXE loader/entry/ABI
rather than assuming that `GetSelectorBase` is responsible. Run in a
recoverable 86Box VM with the existing display driver. PALSTAGE.DRV must
not be installed.

```sh
make -f Makefile.win31diag test
sh windows/win31/build-paldiag.sh
```

## v0032 diagnostic (superseded)

The initial `PALDIAG.EXE` inspected both words of `__A000H` and tried
selector-base calls based on a bit-pattern heuristic. It compiled with
Open Watcom but caused a KRNL386 GP fault before its first report dialog
under Windows 3.1. Do not use that version; the revised v0033 program
above is intended to identify the faulting call without unsafe base queries.

## v0033 actual 86Box result and v0034 experimental read

The maintainer **ran v0033 PALDIAG in Windows 3.1 successfully**. The
screenshots show `GetModuleHandle("KERNEL") = 010F`, raw `__A000H` export
`FFFF:1027` (low-word selector candidate `1027`), `GetWinFlags=00000C29`
(protected/386 Enhanced), and optional BIOS INT 10h AH=0Fh reporting
mode `12h`, columns `50h`, page `00h`. No GP fault appeared. This does not
prove that the selector safely maps banked Tseng VRAM.

v0034 keeps the same `PALDIAG.EXE` and STEP 0–4 behavior, but now asks
**separately** whether to attempt one read-only byte access using
`MK_FP(selector, 0)`. It performs preflight (protected mode, nonzero/non-FFFF
selector), shows **STEP 5A before the actual volatile load**, then shows
**STEP 5B** with the returned byte if successful. The user can decline both
the BIOS call and the video-memory read independently. No video memory writes,
video mode changes, bank selection, or I/O port calls are present.

Use only a **recoverable 86Box snapshot**; a GP fault remains possible.
If Windows faults after STEP 5A, report that precise last dialog. A returned
byte does NOT prove that this is the physical A0000h aperture rather than a
VDD-managed view in 386 Enhanced mode. Before enabling an SVGA desktop,
implement the Windows 3.1 VDD coordination and device lifecycle; a standalone
GUI process's read is not permission for the display driver to do raw BIOS
mode switching or 3CDh bank I/O. Do not install `PALSTAGE.DRV`.

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal34.zip
make -f Makefile.win31diag test
sh windows/win31/build-paldiag.sh
```

## v0035: VDD configuration advisory (no hardware changes)

The maintainer's v0034 PALDIAG screenshots confirm the single A000h byte
read at selector 1027h, offset 0, returned 00h without a GPF. This is
not an SVGA bank-switch/VDD endorsement.

In v0035, the same PALDIAG.EXE adds opt-in STEP 6, reporting the **configured**
`SYSTEM.INI [386Enh] display=` value. You can decline STEP 5's experimental
VRAM read and still accept STEP 6. A portable Win3.x-only `pal_vdd.c`
classifies the setting and warns that the stock Win3.1 VGA VDD cannot safely
be assumed to support a 640x480x256 (307200-byte) desktop, because Windows
3.1's standard VDD is documented to be incompatible with SVGA drivers
requiring more than 256 KiB (Microsoft KB80901). A custom or VDDVGA30 VDD
entry is merely an unverified candidate; reading SYSTEM.INI does NOT prove
which VxD was actually loaded, or that the ET4000AX register state can be
saved/restored during DOS-window switches. No configuration is written.

Before enabling the driver: establish the intended VDD, grabber and Win3.1
`VDDsetaddresses` service 0Ch contract against historical DDK sources,
then implement a verified GDI-owned PDEVICE/mode lifecycle and safe rollback.
Do not make any in-guest `SYSTEM.INI` change to install PALSTAGE.DRV.
