# Palettium v0019 — test outputs and indexed formats

## Test output policy

All Linux test executables created by the top-level `Makefile` and the ten
`Makefile.*` regression builds now live in `tests/exes/`. That directory is
created by Make as needed. The raster test image is written to
`tests/exes/raster-demo.ppm` rather than the repository root.

Existing source, ET4000AX, DOS, and Win16 build artifacts are not moved by
this patch; the requested change covers **test executables** and test data.

Add this to the repository's `.gitignore` (the patch does not overwrite your
existing `.gitignore`):

    /tests/exes/

If older root-level test executables are present, delete them once by hand
(or with `rm -f` after confirming they aren't modified source). If they were
tracked in Git, ignoring their names will not untrack them; use `git rm
--cached <path>` and commit the change. Generated `raster-demo.ppm` can also
be deleted from the root. Future builds will not recreate those root files.

For all Linux tests, run `make test`, then for each historical separate
Makefile invoke `make -f Makefile.NAME test`. Each `clean` removes only its
own generated binary under `tests/exes/`.

## Pixel formats

The **single** unified renderer now supports ten raw-pixel formats:
INDEX1, INDEX2, INDEX4, INDEX8, INDEX16, RGB555, RGB565, RGB888,
XRGB8888, and ARGB8888.

INDEX2 stores four palette indices per byte, high-order bits first. INDEX16
stores a 16-bit *index*, little-endian in the canonical in-memory layout,
and is **not** RGB565. It does not imply the hardware has 65,536 programmable
palette entries, nor that Palettium currently implements such a palette.
Hardware backends negotiate the formats they actually support.

No legacy 8-bit-only renderer was reintroduced. ET4000AX remains INDEX8-only
for its current native graphics mode. Existing software pixel, line, fill,
and BitBlt operations use the same format-aware engine.

## Validation

Linux: top-level seven tests + ten historical Makefile test suites pass.
Cross-compilation and emulator execution are not rerun in this patch;
they should be verified on the user's installed Open Watcom and 86Box setup.
