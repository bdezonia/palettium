# Retro-GFX — milestone 0 (software library spike)

A first, **working** C89 graphics library skeleton for eventual Windows 3.1,
Windows for Workgroups 3.11, Windows 95 and Windows 98 display-driver backends.

## On Linux

```sh
make test
```

This produces `raster-demo.ppm` (view with an image viewer, e.g. ImageMagick)
and confirms that an optional line drawing override is called.

## Current scope

- 8-bit indexed-color framebuffer in ordinary host memory
- Software pixel, line, and clipped rectangle fill routines
- Per-device function pointer dispatch
- Explicit software fallback callable from accelerated backends
- Linux C89 build and smoke test

**Not yet a real Windows driver.** There is no Windows DDK glue, palette handling,
mode setting, banked framebuffers, BitBlt, GDI clipping regions or binary driver
packaging. The PPM's RGB mapping is only a preview of the 8-bit pixel indices.

## Next milestones

1. Freeze and test framebuffer/bank-switching hardware services contract.
2. Add BitBlt with overlap, clipping and raster-operation tests.
3. Establish an Open Watcom 16-bit compile-and-link smoke test.
4. Develop first Win3.1 GDI driver using historically accurate DDK interfaces.
5. Configure 86Box with an emulated SVGA card and boot Win3.1.
6. Adapt driver layers for WfW 3.11, Win95, Win98.

`windows/` contains placeholders for the four future adapters, not working builds.

IMPORTANT: The current Bresenham implementation expects short bounded
coordinates; before untrusted GDI coordinates are accepted, add a safe line
clipper to prevent overflow and slow off-screen raster traversal.
