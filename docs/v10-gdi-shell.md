# Palettium v0.10: first Win3.1 GDI driver ABI shell

## Status: NOT INSTALLABLE

`PALGDI.DRV` is a Win16 NE module with the real exported names `Enable`
and `Disable`. The `Enable` implementation returns 0 in *every* case.
Therefore it can never claim that a video mode or a complete capability
structure has been initialized. `Disable` is a no-op. The shell is a
build/export validation artifact, not a functional Windows display driver.

**Do not** copy `PALGDI.DRV` into the Windows system directory, change
`SYSTEM.INI`, replace `VGA.DRV`, or install it through Windows Setup.
Keep your working VGA installation untouched.

## Build on Linux using your installed Open Watcom v2

```sh
sh windows/win31/build-palgdi.sh
file windows/win31/PALGDI.DRV
wdump -x windows/win31/PALGDI.DRV
```

Expected: NE Windows module, named `ENABLE` and `DISABLE` exports;
script verifies both. This build has *not* been executed with Open Watcom
in the author's environment; your machine is the toolchain validation.

## Source and design rationale

The original Windows DDK documents
`Enable(lpDestDev, wStyle, lpDestDevType, lpOutputFile, lpData)`.
`wStyle & 1` distinguishes GDIINFO and PDEVICE paths. `Disable(lpDestDev)`
receives a PDEVICE pointer. Both are 16-bit Windows exports.

Authoritative references:
- https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/
- https://dos-help.soulsphere.org/ddag31qh.hlp/dispinit.html
- https://dos-help.soulsphere.org/ddag31qh.hlp/dispgdiinfo.html

## Why this is incremental

Existing Palettium core, ET4000AX backend, and Windows runtime tests are
unchanged. This validates the beginning of the OS-specific DDI boundary
without introducing any chipset code into `windows/win31`.

## What remains before it may be installed

1. Validate the *Windows 3.1* DDK `GDIINFO` and `PDEVICE` layouts, alignment
   and return sizes from an original DDK sample or compatible source.
2. Implement both `Enable` modes (capabilities and physical device), and
   `Disable` hardware state restoration.
3. Implement required GDI drawing, text, bitmap, brush, palette, cursor and
   switching hooks, or explicitly compatible software fallbacks.
4. Integrate framebuffer callbacks and ET4000AX mode initialization through
   Palettium's hardware interface.
5. Audit Windows 3.x grabber/VDD interaction, especially 386 Enhanced Mode.
6. Test on an isolated snapshot of the 86Box VM, retain original VGA backup.

Once a complete initialization path exists, a separate non-system test harness
should invoke it with correctly sized structures and guard bytes before
any attempt to boot Windows using the driver.
