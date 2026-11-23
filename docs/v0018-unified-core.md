# Palettium v0018 — unified pixel-format core (breaking refactor)

This **replaces** the earlier additive v0018 `RG_Pixmap` experiment.

There is now one drawing device (`RG_Device`), one surface descriptor
(`RG_Surface`), and one rasterization implementation (`common/rgfx.c`).
All software and hardware drawing goes through the same `RG_Ops` table.
There is no compatibility wrapper for the old 8-bit-only `rg_init` interface.

## Format model

`RG_PixelFormat` supports INDEX1 (MSB-first), INDEX4 (high nibble first),
INDEX8, RGB555, RGB565, RGB888 (B,G,R byte storage), XRGB8888
(B,G,R,X), and ARGB8888 (B,G,R,A). These format names describe raw
framebuffer layout, **not** blended RGB or color-space conversion.
Logical palette programming is still a separate hardware operation.

`rg_pixel` is an unsigned long (32-bit under Open Watcom Win16). All
raster operations are performed on stored pixel bits and masked to
format width. Per-row `stride` and `capacity` are **bytes**, with an
`rg_offset` type that has at least 32 bits. `rg_min_stride` computes
packed scanline byte requirements. Invalid geometry/capacity is rejected
before any framebuffer access.

Banked VRAM continues to use the existing byte callbacks (`RG_VramOps`),
so packed pixel operations and multi-byte pixels spanning a bank boundary
work without card-specific code in the rasterizer. `rg_init` with a plain
memory pointer is intended for directly addressable host buffers; Win16
screen framebuffers must use `rg_init_vram` with correct segmented-memory
callbacks. Do NOT cast a Win16 near pointer to a >64 KiB framebuffer.

## Breaking API changes

- `rg_init(device, ptr, width, height, stride, capacity, format)`
- `rg_init_vram(device, width, height, stride, capacity, format, io, context)`
- `RG_Mode.depth` is replaced by `RG_Mode.format`; `RG_Mode.capacity` is
  supplied by the selected hardware backend.
- `RG_Ops.pixel`, `.line`, `.fill` receive a `rg_pixel`, not `rg_u8`.
- `RG_Surface.stride` is an `rg_offset`; `.format` and `.capacity` are new.
- `rg_get_pixel` / `rg_put_pixel` operate on stored pixel values.

ET4000AX currently supports **only INDEX8** and 640x480, as its existing
native mode programming does. It now explicitly negotiates that format
through the shared hardware contract. Its `0x3CD` register banking logic
and DOS demonstration are otherwise unchanged. New cards may advertise
other formats without changing the core rasterizer.

The Windows 3.1 device/bitmap adapters remain staged for 640x480x8-bit
GDI initialization: those OS interfaces must eventually learn to advertise
additional modes correctly. They now compile against the unified RG_Device
API; no parallel 8-bit rasterizer exists. This milestone does not produce
an installable Windows display driver.

## Verification

On Linux run `make clean && make test` plus the historical component
makefiles (including `Makefile.et4k`, `Makefile.screen17`). The unified
format test checks eight formats, overlap, clipping, ROP truth tables,
color packing, and a 24-bit pixel straddling the 64 KiB bank boundary.
The hardware-format test opens RGB565 through an abstract backend; the
ET4000-specific test rejects RGB565 while retaining INDEX8 behavior.

On the developer's machine, **also** run:

```
sh dos/build-et4k.sh
sh windows/win31/build-palcore.sh
```

These Win16/16-bit DOS targets were not built in the authoring environment
(Open Watcom is not installed there). After compilation, run the DOS
hardware test in 86Box and the existing PALTEST/PALCORE Win3.1 runtime
probe. Only those runs can validate target execution.

## If you installed the original additive v0018

That obsolete patch added `rg_pixmap.h`, `rg_pixmap.c`, `test_pixmap.c`, and
`Makefile.pixmap`. These are **superseded**, not an alternate API.
Delete those four paths (after reviewing `git status`); keep the new
single `include/rgfx.h` / `common/rgfx.c` implementation. Never compile
`common/rg_pixmap.c` against this replacement API.
