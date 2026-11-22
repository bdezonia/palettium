# Palettium hardware binding, v0.5

This is an incremental extension of the existing `RG_Device`, `RG_Ops`, and
`RG_VramOps` API. No existing rasterizer entry points or test sources changed.

`RG_Hardware` manages the lifecycle of *one* 8-bit indexed card and mode:

* `initialize`, `set_mode` (required): establish hardware and an actual stride;
* `read_byte`, `write_byte` (required): access logical framebuffer offsets,
  hiding any 64 KB bank switching inside the backend;
* `set_palette` (optional): accept 8-bit RGB components; backend handles DAC
  quantization if needed;
* `install_accel` (optional): replace selected `device->ops` function pointers,
  leaving the remaining software defaults untouched;
* `shutdown` (optional): clean up, including failed mode setup.

All backend storage is caller-owned. Zero-initialize `RG_HwBinding` once before
first use. Do not copy, reset, or reuse a live binding until `rg_hw_close`.
The backend must arrange for valid VRAM to remain accessible throughout a binding.
`RG_Hardware` is an interface contract, **not an ET4000 driver**. An ET4000AX
implementation can use all-software drawing; W32 acceleration can be added
later by overriding a single operation at a time. The same hardware module
will be called by four separate Windows OS adapters.

## Build and test

From the repository root: `make test`. `hardware-test` checks a simulated
banked surface, palette callback, opt-in override, fallback, double-open
rejection, and cleanup after failed mode selection.

## Boundaries and known work

The library neither probes ISA ports nor implements a Windows GDI ABI. Those
belong to OS-service and chipset-specific modules. For 16-bit Windows, keep
framebuffer offsets at least 32 bits and use callback VRAM access (not near
pointer arithmetic over a 307200-byte screen).

Initial implementation uses byte-at-a-time access, not bulk bank-aware transfer.
It is designed for correctness; performance can be improved without changing
`RG_Ops`. A separate low-level service layer may be needed for OS-specific
port I/O and mapping. No device is claimed supported yet.
