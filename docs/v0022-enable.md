# Palettium v0022 — Win3.1 two-stage Enable integration

The *real* Windows 3.x display-driver `Enable` entry point now handles
**style 1**, returning a 110-byte candidate GDIINFO block. Style 0 still
returns zero, so **do not install this display driver**.

The capability block deliberately advertises no drawing or palette capability
bits. It is only a layout/entry-point integration exercise, not a complete
GDIINFO suitable for a functioning Windows desktop.

`pal_enable.c` is independent of the ET4000 hardware backend. The Win16
wrapper copies a near stack structure to the caller's far address bytewise.
This avoids mistakenly treating a segment:offset destination as a near pointer.

The staging Win16 build now compiles the consolidated DDI entry point and
its capability helper together as object files. The full driver still needs
verified exports, linkage, the full DDI and screen-switching integration.

## Run

    make -f Makefile.enable22 test
    sh windows/win31/build-palddi.sh

Keep the VM's existing VGA driver installed. Never replace SYSTEM.DRV or
modify SYSTEM.INI to use this incomplete code.

## Sources

- Windows 3.0 DDK Adaptation Guide, section 2.3 (two Enable stages):
  https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/
- Windows 3.1 DDK GDIINFO reference:
  https://dos-help.soulsphere.org/ddag31qh.hlp/dispgdiinfo.html

## Future work

1. Verify and complete all advertised GDI capabilities and mandatory entry points.
2. Link a full NE driver and audit every required export ordinal.
3. Add Win16 hardware services and screen switching; connect existing RG_Hardware.
4. Try the driver only on a disposable VM snapshot, with rollback.
