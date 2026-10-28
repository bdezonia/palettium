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

## Palettium v0.3: 8-bit BitBlt and Boolean raster operations

`RG_Device.ops.blit(dst, src, dx, dy, sx, sy, width, height, rop)`
copies a rectangle from `src` to `dst`. Coordinates are signed; clipping
preserves source/destination alignment. Source and destination may be the
same device, including banked VRAM. For overlapping copies the software
implementation reads pixels in a safe traversal order.

All 16 two-input bitwise Boolean ROPs are supported. The ROP is a
four-bit truth table whose bit index is `(source_bit << 1) | dest_bit`:
`RG_ROP_COPY=0xC`, `RG_ROP_XOR=0x6`, `RG_ROP_AND=0x8`,
`RG_ROP_OR=0xE`, `RG_ROP_CLEAR=0x0`, `RG_ROP_SET=0xF`.
These are **Palettium internal ROP codes**, not Win16 GDI ROP encodings
or Win32 ROP3 constants; the OS adapters must translate explicitly.

Run `make test` to exercise all earlier tests plus 346 deterministic BitBlt
cases against an independent snapshot-based reference. Included tests cover
all 16 ROPs, overlaps, negative-coordinate clipping, cross-device transfers,
and 64 KiB simulated bank boundaries.

### Current limitations / next steps

- Pixel-by-pixel VRAM callbacks are a correctness baseline, **not** an
  efficient banked copy engine. Introduce run/span access before optimizing.
- The simulator exposes one read/write bank at a time; real cards may have
  separate read and write windows and different bank granularities.
- The software line routine expects bounded coordinates; no general line
  clipper is implemented yet.
- 16-bit compilation and Windows GDI entry points are **not yet tested**.
- No Windows driver binaries are included at this stage.

## v0.4 — Win16 integration preparation

`windows/win31/` includes the Open Watcom Win16 DLL ABI probe, not yet a
Windows display driver. It prepares the toolchain without misrepresenting the
complex Windows 3.1 DDK contract. Run `make test` for the extra Win16 memory
geometry guard. The actual toolchain probe is built with
`windows/win31/build-win16.sh` once `wcc` and `wlink` are installed.
