# Palettium v0.2 — vintage Windows graphics-driver framework

Shared software drawing primitives and a hardware-access interface for eventual
Windows 3.1, Windows for Workgroups 3.11, Windows 95 and Windows 98 drivers.

## Build and test (Linux)

```sh
make test
```

Expected results:

```
PASS: raster demo created raster-demo.ppm; mock override called 3 times
PASS: banked 640x480 agrees with linear framebuffer; bank switches=11
```

The switch count is specific to this simulated workload. `raster-demo.ppm` is
an indexed-color visualization with a demo-only color mapping.

## Architecture

`include/rgfx.h` declares an 8-bit indexed framebuffer graphics API.
`rg_init()` supports host-addressable linear memory (the v0.1 API).
`rg_init_vram()` supports a framebuffer whose pixels are read/written through
byte callbacks; offsets use `unsigned long` so they are 32-bit even on 16-bit C.
The common raster routines use `rg_write_byte()` rather than indexing VRAM.

`hardware/sim_bank/` is a **test double**, not a Tseng/VESA implementation:
it models a 64-KiB CPU aperture with 64-KiB bank granularity. It gives us a
repeatable bank-switching test independent of any emulator or physical card.

`RG_Device.ops` keeps the per-operation override design: replace just those
operations the card accelerates; call explicit `rg_software_*` functions for
fallback. Backend initialization and OS adapters remain future work.

## Limitations / next work

- **Not yet an installable Windows driver**; `windows/` contains placeholders.
- The Linux simulator does not model hardware I/O ports, separate read/write
  banks, overlapping bank windows, or hardware timing.
- Software line drawing still expects reasonably bounded coordinates; implement
  robust pre-clipping before passing untrusted GDI coordinates to it.
- Per-byte callback access is correct but not fast. A banked span/window
  interface should come before performance work on large rectangles and BitBlts.
- Some 16-bit compilers have data-pointer and far-memory limitations. The direct
  `pixels` path is for framebuffers addressable by the chosen memory model;
  a 16-bit Windows backend should normally use callbacks (and far access where
  necessary). No Open Watcom legacy-target compile has been claimed yet.
- No BitBlt, palette programming, VESA mode setting, or chipset support yet.

## Repository notes

The archive intentionally excludes compiled executables and generated PPMs.
Use `make clean` to remove local build outputs.
