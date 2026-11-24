## v0044: opt-in Standard-mode boot experiment (86Box clone only)

A separate `PALTRIAL.DRV` can be built with `sh windows/win31/build-paltrial.sh`.
Its `Enable(style=0)` is gated on Win3.1 Standard mode, 256KiB ET4000AX and
explicit `[Palettium] TrialEnable=4401`; default PALSTAGE.DRV always refuses.
GDIINFO now describes the minimal implemented drawing, font and palette
capabilities instead of advertising all zero. PALTRIAL is an **untested
experimental driver**, not a stable installer. See HANDOFF.md for precise
DOS backup, `WIN /S` and recovery steps. Do not run PALTRIAL in your ordinary
Windows configuration.

# Windows 3.1 staging display driver (v0042)


## v0043: Real driver loader exercise (non-installing)

`build-palload.sh` builds `tests/exes/PALLOAD.EXE`, a Win16 GUI diagnostic
that loads the real `PALSTAGE.DRV` from `C:\PALTEST\PALSTAGE.DRV` using
Windows `LoadLibrary`. It verifies all 19 export names and optionally calls
`Inquire @101` and `Enable(style=1)` for the GDIINFO record. No driver is
installed; existing VGA desktop remains active, and no hardware is touched.
A four-step MessageBox transcript isolates loader versus ABI failures.
Copy `PALLOAD.EXE` and `PALSTAGE.DRV` to `C:\PALTEST` in snapshotted 86Box;
run PALLOAD from Windows File Manager (NOT DOS). The `Enable(style=1)`
query requires an explicit correct `[Palettium] VRAMKiB=...` setting in
`SYSTEM.INI` to return nonzero; missing config is an expected refusal.
Do not change `[boot] display.drv`, or treat this as approval to install.


**v0042:** Open Watcom's eight W124 warnings are addressed at their
sources. Mouse IRQ publication and timer-side consumption now use paired
Win3.x DDK critical sections in the Win16 build, while the cursor compositor
rejects nested timer execution. Host tests simulate a mouse IRQ during
banked-VRAM cursor painting, but native timing and assembly need Windows
verification. `Enable(style=0)` remains disabled; no installation yet.

**v0041:** The existing `PALSTAGE.DRV` exports four new cursor DDIs:
`Inquire @101`, `SetCursor @102`, `MoveCursor @103`, `CheckCursor @104`.
Win16 `CURSORSHAPE` is decoded using its documented 12-byte header and
32×32 two-mask format. Mouse-position updates are deferred; the rendering
and text DDIs now exclude the software cursor around framebuffer access.
Host tests passed. v0042 adds a guarded native mailbox, but runtime
interrupt behavior still needs verification in Windows 3.1.
**19 NE exports are expected. Enable(style=0) still fails; do not install.**

The maintainer validated v0040 `StrBlt @11` and `ExtTextOut @14`
under Open Watcom, including the 15-export linked Win16 driver. The shared
cursor renderer is retained; no hardware-specific duplicate was introduced.

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
huge/multiplane memory bitmaps, patterned brushes, complete text support, cursor DDIs and window
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

## v0039: Transactional native session, still gated

`pal_et4k_session_start_published` joins the existing ET4000AX session,
GDI-owned PDEVICE snapshot, and an atomic Win16 FAR-memory publication
callback. The host is not exposed until mode setting, palette programming,
and GDI publication all succeed. Any failure after mode setting closes the
hardware binding and invokes the existing BIOS restore callback. Host tests
exercise 256/512/1024 KiB, rejected VDDs and missing authorization.

The Win16 `Enable(style=0)` branch is now wired to this code *but still
unconditionally refuses to activate it*: `pal_win16_install_ready()`
returns zero until mandatory text and cursor services and their ABI are
implemented and validated. The portable transaction also rejects enhanced
mode irrespective of configured VDD filename. `Disable` will stop the native
hardware session only when the caller supplies the matching GDI FAR PDEVICE.
Do not install PALSTAGE.DRV or select it in SYSTEM.INI yet. The next
engineering milestone is a real text renderer (`ExtTextOut`, `StrBlt`) and
cursor cooperation, not additional VRAM probing.

## v0037: VRAM- and VDD-aware capability staging

The old Win16 `Enable(style=1)` unconditionally claimed 640x480x8, even
when the card had only 256 KiB. It now reads **only** an explicitly
configured `[Palettium] VRAMKiB` value in `SYSTEM.INI` (256/512/1024).
If absent or invalid it returns zero; it does not probe CRTC37, BIOS,
VRAM or graphics registers from GDI initialization. For **future** staging
in a disposable VM, an administrator could configure, for example:

```ini
[Palettium]
VRAMKiB=256
```

**Do not change SYSTEM.INI to install this driver yet.** That sample is
configuration documentation, not an installation instruction. 86Box/DOS
can establish the actual VM VRAM setting. The Win3.x-only planner also
reads `[386Enh] display=` and `GetWinFlags`, so when Windows is running
in 386 Enhanced mode with configured `*vddvga`, it avoids advertising a
>256KiB 640x480x8 framebuffer even on 512KiB/1MiB cards. It proposes
640x400x8, which **is not yet established as VDD-compatible**. A custom
VDD filename does not grant compatibility automatically either.

The `PAL_ET4KPlan` from `pal_et4k_session.c` tracks the optional CRTC37
capacity hint as cross-check only, refuses contradictory hints, and
**always leaves `can_enable=0`**. Native `pal_et4k_win16.c` exposes a
separately authorized CRTC37 advisory function, but refuses raw port
access in 386 Enhanced mode; it is not called by GDI. The existing
`pal_et4k_session_start_mode` can exercise 640x400/640x480 in isolated
mock-host tests without duplicating rendering code, but is not connected
to `Enable(style=0)`. Actual BIOS/VDD/PDEVICE/text/cursor integration
still blocks installation.

## Mode-selection status (v0031)

The shared ET4000AX backend accepts 640x400x256 on verified 256 KiB cards
(Tseng BIOS mode 2Fh), and 640x480x256 on cards with at least 512 KiB
(mode 2Eh). Optional read-only VRAM-capacity and BIOS-mode-query platform
callbacks support validation; actual mode changes must be acknowledged by
`set_mode`. v0037 Win16 `Enable(style=1)` now selects an explicit-VRAM-dependent
candidate and `Enable(style=0)` still fails. Native Windows
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
v0037 `Enable(style=1)` requires the explicit SYSTEM.INI VRAM setting and
returns a policy-filtered development candidate, NOT a negotiated geometry; style=0 still fails. A working
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

## v0038: GDI-owned PDEVICE handoff (staging only)

The host's `PAL_PDEVICE` remains an internal near-memory shadow used by
shared C89 DDI checks. **GDI owns the actual far 20-byte PDEVICE buffer**;
it is not correct to cast its far pointer to a near C pointer. The helper
`pal_screen_host_gdi_snapshot` accepts only the exact identity token from an
initialized, running host, and makes a validated copy without hardware I/O.
`pal_win16_publish_host` checks the far buffer's 64 KiB boundary, marshals
those bytes and binds the existing DDI host only after the copy. A mismatched
`Disable` token cannot unbind it. The helper is intentionally **not called
from `Enable`** while cursor/text, GDI capability flags and enhanced-mode VDD
cooperation are unfinished. Do not install PALSTAGE.DRV.

v0038 also populates conservative standard Win3.x GDIINFO screen metrics:
96 DPI, rounded millimeter extents, 100:100:141 aspect, styled-line
length 282, and consistent one-inch metric/English/twips mapping ratios.
These are candidate-only fields; no device-bitmap, text or palette
capability bits are advertised by the placeholder record.
