# Palettium v0.8 — Windows 3.1 runtime smoke test

This increment introduces an **application**, `PALTEST.EXE`, that loads the
v0.7 `PALCORE.DLL` and calls its exported `PALETTIUMCORESELFTEST` procedure.
It does not install or replace a display driver.

## Build on Linux

Use your established Open Watcom v2 shell environment (`WATCOM`, `PATH`,
`INCLUDE`). With v0.7 already installed and `PALCORE.DLL` built:

```sh
sh windows/win31/build-paltest.sh
file windows/win31/PALTEST.EXE
```

If the DLL is absent, run `sh windows/win31/build-palcore.sh` first.

## Transfer to the 86Box Windows 3.1 machine

**Shut down Windows before switching transfer images.** To create a floppy
image using mtools on Linux:

```sh
mkdir -p emulator
mformat -C -f 1440 -i emulator/paltest.img ::
mcopy -i emulator/paltest.img windows/win31/PALCORE.DLL ::
mcopy -i emulator/paltest.img windows/win31/PALTEST.EXE ::
mdir -i emulator/paltest.img ::
```

Mount `paltest.img` in 86Box as floppy A:. In MS-DOS, before starting Windows:

```dos
MD C:\PALTEST
COPY A:\PALCORE.DLL C:\PALTEST
COPY A:\PALTEST.EXE C:\PALTEST
WIN
```

In Windows Program Manager select **File -> Run** and enter
`C:\PALTEST\PALTEST.EXE`.

Success: a message box reads `PASS: Palettium core runs under Windows 3.1.`
Nonzero failure masks are shown in hexadecimal. If the DLL fails to load or
its export cannot be found, the program reports those conditions separately.

## Scope and next steps

A PASS confirms only that this specific 64x48 software raster self-test works
under Win16. It does not demonstrate physical VRAM access, ET4000 register
I/O from Windows, GDI integration, or a working display driver. Continue
with Windows 3.1 DDK-defined display-driver integration after this test.

Files in this patch are strictly additive. Existing sources and makefiles
are unchanged.
