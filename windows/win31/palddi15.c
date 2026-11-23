/* Palettium Win3.1 DDI export/ABI staging binary (NOT installable).
 * Historical target ordinals: BitBlt 1, Disable 4, Enable 5.
 * Export names in this smoke test are deliberately NOT a valid DDI map.
 * No GDIINFO capabilities claimed; no screen adapter bound.
 * This keeps the build/ABI exercise separate from our hardware backend.
 */
#include <windows.h>

/* Deliberate no-op while far-pointer source/dest decoding and device binding
 * remain unfinished. Refusing is safer than modifying unknown screen memory.
 * Pointers are opaque here; this function DOES NOT perform BitBlt.
 */
BOOL FAR PASCAL __export PalettiumDdiBitBltStub(LPVOID dst, WORD dx, WORD dy,
                              LPVOID src, WORD sx, WORD sy,
                              WORD width, WORD height, DWORD rop,
                              LPVOID brush, LPVOID drawmode)
{
    (void)dst; (void)dx; (void)dy; (void)src;
    (void)sx; (void)sy; (void)width; (void)height;
    (void)rop; (void)brush; (void)drawmode;
    return FALSE;
}

void FAR PASCAL __export Disable(LPVOID pd)
{
    (void)pd;
}

WORD FAR PASCAL __export Enable(LPVOID destination, WORD style,
                              LPSTR devtype, LPSTR output, LPVOID data)
{
    (void)destination; (void)style; (void)devtype;
    (void)output; (void)data;
    return 0;
}

BOOL FAR PASCAL LibMain(HINSTANCE h, WORD ds, WORD heap, LPSTR cmd)
{
    (void)h; (void)ds; (void)heap; (void)cmd;
    return TRUE;
}
