# Palettium

Palettium is a reusable C89 graphics-driver framework for legacy Windows:
Windows 3.1, Windows for Workgroups 3.11, Windows 95 and Windows 98.
The aim is to implement a small hardware backend once and reuse shared
software rendering across OS adapters, with optional hardware acceleration.

## Architecture

- `RG_Device` represents a drawable surface; `RG_Ops` provides software
  pixel, line, fill and blit defaults with per-operation hardware overrides.
- `RG_VramOps` supplies logical byte access to linear or banked storage.
  Framebuffer offsets are at least 32 bits; OS adapters handle segmented
  memory, platform I/O and native DDI conventions.
- `RG_Hardware` manages hardware initialization, mode selection, palette,
  VRAM access, optional acceleration and shutdown.
- Shared GDI helpers provide clipping, raster operations, pens/brushes,
  text, palette translation and cursor composition. Chipset-specific mode,
  banking and DAC logic live outside the renderer.

See [hardware binding](docs/hardware-api.md) and the
[ET4000AX backend](docs/et4000ax.md) for engineering background.

## Current capabilities

| Area | Status |
| --- | --- |
| Shared raster | Indexed 1/2/4/8/16-bit, RGB555/565/888 and XRGB/ARGB8888 raw-pixel formats |
| Win3.x drawing | 1/8bpp memory bitmaps, screen drawing, text, palette, DIB operations and software cursor; partial DDI coverage |
| ET4000AX | Native packed 640x400x256 and 640x480x256, banked VRAM and DAC; no VESA dependency |
| Windows 3.1 Standard mode | Experimental desktop runtime validated in 86Box |
| Windows 3.1 Enhanced mode | Experimental desktop startup, passive INT 2Fh hook installation, chaining and removal runtime validated; DOS switching unvalidated |
| WfW 3.11, Windows 95/98 | Planned adapters, not working drivers yet |

The ET4000AX DOS backend has been tested at 256, 512 and 1024 KiB.
The Win3.1 experiments have narrower activation restrictions; see the
[Windows 3.1 README](windows/win31/README.md). These are development
builds, not general-purpose installable drivers. Use disposable VM snapshots.

## Repository layout

```text
common/        Shared C89 graphics implementation
include/       Shared interfaces
hardware/      Chipset and simulation backends
windows/       OS adapters and native build scripts
dos/           DOS hardware diagnostics
makefiles/     Host build and test recipes
tests/         Regression tests; generated host/Win16 binaries in tests/exes/
tools/         Build, verification and packaging utilities
docs/          Engineering documentation
patch-notes/   Release-specific changes and validation history
checksums/     Patch SHA-256 manifests
```

## Build and test

Run host builds from the repository root (C compiler, make and Python 3):

```sh
make -f makefiles/Makefile clean
make -f makefiles/Makefile test-all
```

`test` runs the main suite; `test-all` also discovers standalone
`makefiles/Makefile.*` test targets. For a single suite, for example:

```sh
make -f makefiles/Makefile.win31native test
```

Native DOS and Win16 builds require Open Watcom; see the
[Windows 3.1 build and runtime guide](windows/win31/README.md) and
[ET4000AX notes](docs/et4000ax.md). Host tests do not replace native
link audits or actual VM testing.

## Roadmap

1. Complete Windows 3.1 Enhanced-mode support: screen switching, video
   ownership, bank/palette/cursor state and repaint; preserve Standard mode.
2. Finish driver coverage, hardening, independent resources and installation.
3. Reuse the framework for WfW 3.11, Windows 95 and Windows 98.
4. Extend chipset and display-format support and demonstrate hardware
   acceleration (ET4000/W32 is an intended early target).

## Development records

Apply ZIP patches from the repository root and verify their corresponding
`checksums/CHECKSUMS-vNNNN.sha256` using `sha256sum -c`. Historical
manifests describe the original release layout, not the current tree.
See [patch notes](patch-notes/), [engineering notes](docs/) and
[layout migration](docs/v0077-layout.md). The former cumulative README
logs are preserved as [historical reference](docs/history/), not current
build or installation guidance. New milestone history belongs in patch
notes; update this README only when its current descriptions change.

[License](LICENSE)
