# Palettium v0014 — GDI BitBlt bridge

This is a **portable drawing increment**, not an installable Win3.x display driver.

## What works

`rg_gdi_blit_rop3()` accepts a Windows ROP DWORD, translates its truth-table
byte to a Palettium ROP2 when the pattern operand is irrelevant, and calls
`dst->ops.blit`. Therefore accelerated backends can override BitBlt without
changing the common bridge. The original rasterizer provides the default.

The implementation performs no Win16 far-pointer dereferences. It does not
claim to implement memory DDB access, patterned brushes, monochrome conversion,
or DDI physical-device selection. Unsupported ROP3s fail without writing.

## Important Win3.x DDI correction

The historical DDK specifies **BitBlt export ordinal 1**. The experimental
PALGDI.DRV built in v0010 assigned ordinal 1 to Enable. Therefore that binary
cannot be treated as a correct GDI export map. Do not install it.

The next adapter stage must:

1. Establish exact name/ordinal exports for the full driver interface.
2. Decode 16-bit far pointers for screen PDEVICE and memory BITMAP safely.
3. Provide hardware-bound RG_Device instances for the screen and segmented
   access to memory DDBs.
4. Map brush and DRAWMODE semantics, including color conversion.
5. Keep Enable() failing until the required GDI entry points and screen-switching
   behavior are ready.

A real GDI `BitBlt` has this parameter order:

    BitBlt(lpDestDev, destX, destY, lpSrcDev, srcX, srcY,
           width, height, rop3, lpBrush, lpDrawMode)

See Windows 3.1 DDK `BitBlt`: https://dos-help.soulsphere.org/ddag31qh.hlp/BitBlt.html

## Test

    make -f Makefile.gdiblt test

This test covers 16 pattern-independent ROP3 conversions, overlapping same-device
copies, clipped copies, an intercepted hardware override, and failure without
pixel writes on unsupported cases.

## Integration contract

`RG_Device` remains OS-neutral. The OS adapter owns pointer/bitmap decoding,
while `RG_Device.ops.blit` selects software or hardware acceleration. An ET4000AX
backend continues to provide only VRAM access; an ET4000/W32 backend may override
`.blit` when an operation can be accelerated.
