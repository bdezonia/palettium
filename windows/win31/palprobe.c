/*
 * Palettium Win16 toolchain smoke-test DLL.
 *
 * Verifies basic ABI assumptions for the shared graphics library.
 * This is NOT a GDI display driver.
 */

#include <windows.h>
#include "rgfx.h"

/*
 * Return a bitmask describing the compiler's ABI properties.
 *
 * Bit 0: 8-bit byte
 * Bit 1: VRAM offsets are at least 32 bits
 * Bit 2: 16-bit int
 * Bit 3: Graphics operations table exists
 *
 * Exported through the module definition file.
 */
WORD FAR PASCAL PalettiumAbiProbe(void)
{
    WORD bits = 0;

    if (sizeof(rg_u8) == 1)
        bits |= 0x0001;

    if (sizeof(rg_offset) >= 4)
        bits |= 0x0002;

    if (sizeof(int) == 2)
        bits |= 0x0004;

    if (sizeof(RG_Ops) > 0)
        bits |= 0x0008;

    return bits;
}

/*
 * Windows DLL initialization.
 */
BOOL FAR PASCAL LibMain(
    HINSTANCE hInstance,
    WORD wDataSegment,
    WORD wHeapSize,
    LPSTR lpszCmdLine)
{
    (void)hInstance;
    (void)wDataSegment;
    (void)wHeapSize;
    (void)lpszCmdLine;

    return TRUE;
}
```
