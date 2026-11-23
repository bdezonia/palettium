# Palettium v0012: Win31 private device-state candidate

This is **not** an installable display driver. Do not set `display.drv` to
PALGDI.DRV or PALDEV.DLL. Existing `Enable()` in PALGDI remains fail-closed.

## Scope

- A private 20-byte PDEVICE candidate with no host pointers, platform I/O,
  or framebuffer mapping embedded in its structure.
- A driver-owned prepare/activate/stop state machine (the activation flag
  does **not** initialize hardware).
- The preceding v0.11 GDIINFO candidate now obtains `dpDEVICEsize` from the actual
  private structure rather than an arbitrary constant.
- C89 host tests and a Win16 Open Watcom build/export probe.

The historical DDK leaves PDEVICE contents to the driver. Its first word
can identify a device-specific layout; actual GDI drawing and bitmap
operations must still be implemented before any initialization can succeed.
We are not asserting this private structure is a universal GDI PDEVICE.

## Commands

    make test
    make -f Makefile.gdiinfo test
    make -f Makefile.pdevice test
    sh windows/win31/build-paldev.sh

`PALDEV.DLL` requires a Win16 runtime invocation to test the exported
function; named export inspection alone does not execute it.

## Roadmap

The next useful step is to inspect a real DDK VGA/Video Seven sample, then
establish the GDI-facing screen bitmap / PDEVICE layout and implement
missing DDI exports. Physical mode changes remain backend-controlled.
