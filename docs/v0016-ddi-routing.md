# Palettium v0016: Win3.1 screen DDI routing

This is the first stateful connection between the Win3.x PDEVICE prototype
and the existing screen BitBlt renderer. It is deliberately **not** a
Windows-installable display driver.

The OS adapter will pass a 32-bit token representing a Win16 far pointer
(segment:offset) to its own `PDEVICE`. An incoming screen pointer is
accepted only if its token matches. It is never cast to a host pointer.
No arbitrary Win16 far pointer is dereferenced here. A GDI memory `BITMAP`
is **not** supported and is rejected without reading the object.

`pal_screen_ddi_attach` is called ONLY after hardware activation and
`rg_init_vram` have succeeded. `pal_screen_ddi_detach` invalidates the
routing on shutdown. Actual GDI capability advertisement remains disabled.

## Next integration steps

1. Implement Win16 platform services (A000 aperture and I/O ports) and
   initialize ET4000 via the existing hardware backend.
2. Translate the Win16 `PDEVICE FAR*` into a segment:offset token in the
   OS-specific DDI wrapper. Keep all Win16 pointer syntax there.
3. Build a production export map (`BitBlt @1`, `Disable @4`, `Enable @5`)
   and verify it with `wdump`; these are not yet provided by this patch.
4. Add GDI palette, text, cursor, screen-switching and memory BITMAP paths
   before installing a driver.

The portable core and ET4000 backend were not changed.
