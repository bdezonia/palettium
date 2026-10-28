# Windows 3.1 integration — first step

This target produces a tiny **Win16 DLL toolchain smoke test** named
`PALPROBE.DLL`. **It is not yet a GDI display driver** and should not be
installed in Windows' display-driver configuration.

From Linux with Open Watcom's 16-bit tools installed and configured:

```sh
cd windows/win31
./build-win16.sh
```

The result should be an NE-format Win16 DLL exporting `PalettiumAbiProbe`.
You can inspect it with `wdis` or another NE-aware executable inspector.
This probe asserts that `unsigned long` can hold VRAM offsets and that the
compiler uses 16-bit `int`. It does not talk to real video hardware.

**Next integration step:** obtain a legally usable Windows 3.1 DDK display
sample and the matching DDK headers/structures, then provide accurate GDI
entry points and capabilities, a proper mode-init path, and packaging.
The DDK sample is important: the display driver contract goes well beyond
creating a DLL or exporting `BitBlt`.

### Win16 memory model warning

The `rg_init()` linear buffer pointer is *not* suitable for a 307,200-byte
640x480x8 framebuffer in a Win16 near segment. Use `rg_init_vram()` with
callbacks whose internal implementation performs correct bank switching and
16-bit segment:offset access. Likewise, generic C function-pointer tables
must eventually be compiled with explicit, ABI-consistent calling conventions
and far/near qualifiers on the Win16 side before invoking them across segments.

The current C core source is **not yet certified with a 16-bit compiler**.
This probe intentionally compiles just the public header into a Win16 DLL;
full cross-compilation and execution tests are still necessary.
