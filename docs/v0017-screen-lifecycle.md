# Palettium v0017: Win31 screen lifecycle integration

This patch wires existing `RG_Hardware` device setup/teardown to Win31's
private PDEVICE and its screen-to-screen DDI bridge. It does not add ET4000
register operations to the OS adapter. The test uses the actual ET4000AX
backend with emulated `0x3CD` and a 512 KiB banked framebuffer.

The caller owns `PAL_ScreenHost`, `RG_Hardware`, and backend state for the
entire session, and must zero-initialize `PAL_ScreenHost` before first use.
The `pdevice_token` is an OS-supplied segment:offset identity, never a
VRAM address. A production Win16 shim must derive it from the actual GDI
PDEVICE far address and must not treat a host pointer as a Win16 token.

Mode setup is delegated to `RG_Hardware`; a failed mode setup calls the
backend's shutdown hook. Successful startup initializes PDEVICE, binds the
screen bridge, and enables BitBlt. Stop disables drawing before hardware
shutdown. The process is safe to exercise in host tests.

**Not an installable driver.** It lacks a complete Win3.x GDI export map,
resource/cursor/text/brush services, hardware mode protection in enhanced
mode, real Win16 far-pointer validation and video virtualization handling.
Keep the existing VGA display driver installed in the VM.

Run `make -f Makefile.screen17 test`, then your previous regression tests.
