# Palettium v0013 — Windows 3.x BITMAP boundary

This patch adds the 32-byte **DDK display-driver BITMAP**, which is distinct
from the Win32 `BITMAP` type. Its first WORD (`bmType`) is zero for a GDI
memory bitmap, while the driver's private PDEVICE starts with a nonzero word.

The structure mirrors the historical 3.x DDK: width/height/stride are signed
16-bit integers; planes and bits-per-pixel are bytes; `bmBits`, `bmWidthPlanes`,
and `bmlpPDevice` are 32-bit; the segment fields follow. Its full layout is
checked on Linux and in a Win16 DLL probe.

The current validator deliberately supports only *single-segment*, 8-bit,
one-plane memory bitmaps with size <=65535 bytes. Multisegment BITMAPs and
far pointer dereferencing are NOT implemented. Passing a real GDI BITMAP
through the validated structure does not yet mean its pixels can be read.

An actual GDI BitBlt must also handle brush realization, palette translation,
conditional source absence, colors and screen switching. None of that is
claimed or exported here. `PALGDI.DRV` remains unsafe to install.

## Run

    make -f Makefile.bitmap test
    sh windows/win31/build-palbmp.sh

The Win16 DLL is a **probe**, never install it as a display driver.

## References

- Microsoft, Windows 3.0/3.1 Device Driver Adaptation Guide,
  chapter 12 BITMAP structure and chapter 2 PDEVICE distinction.
- https://www.pcjs.org/documents/books/mspl13/win/w3ddkadp/

## Next

Compare original DDK BITMAP and Win16 <windows.h> definitions, implement
safe segmented memory bitmap access, then integrate GDI BitBlt carefully.
