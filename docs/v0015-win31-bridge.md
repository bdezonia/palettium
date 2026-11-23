# Palettium v0015: screen BitBlt binding and Win16 DDI signature

The portable `RG_Win31Screen` binding is the first part of an OS-side
screen drawing adapter. A caller supplies an initialized `RG_Device` and
video geometry. All screen-to-screen BitBlt drawing passes through the
existing `rg_gdi_blit_rop3()` path and therefore honors `ops.blit` overrides.
The adapter contains **no Tseng register operations** and does not require
VESA. This is an intentional single-purpose stage, not a desktop driver.

The C89 Linux test checks 32 reference operations with overlapped/clipped
rectangles, override dispatch, inactive devices, invalid geometry, and
unsupported brush-dependent ROPs. We have not implemented memory DDBs,
segmented pointer decoding, palette realization, cursor, or the full DDI.

`palddi15.c` records the 11-argument Windows 3.1 `BitBlt` ABI in a
named staging entry (`PalettiumDdiBitBltStub`), alongside
`Enable` and `Disable`. It fails closed. The Win16 build is *only* a
signature/link check; it does not prove a correct driver export table.
Reference Windows 3.1 ordinals are `BitBlt @1`, `Disable @4`, `Enable @5`.
Earlier `PALGDI.DRV` used incorrect ordinals; do not install it. We must
provide explicit linker ordinals before treating any build as a driver.

## Run

    make -f Makefile.win31blit test
    sh windows/win31/build-palddi15.sh

The second command requires Open Watcom. `PALDDI15.DRV` must **never**
be installed, regardless of whether it compiles. The source/binary is
marked intentionally non-installable.

## Next engineering step

Bind a Win16 `PDEVICE` to the `RG_Win31Screen` instance only after real
ET4000 mode setup; translate GDI screen pointers and single-segment 8-bit
memory DDBs into validated RG_Device callback surfaces. Then implement
brushes, text, cursor, palette and video switching before enabling the DDI.

## Packaging hygiene

All ZIP entry timestamps are set to 2025-01-01 12:00:00, not inherited
from the container clock. The packaging test extracts into a fresh
location and checks that no extracted file has a future mtime.
