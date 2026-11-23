/*
 * Palettium v0.8: Windows 3.1 runtime test for PALCORE.DLL.
 *
 * This is a normal Win16 application, NOT a display driver.
 * Keep PALTEST.EXE and PALCORE.DLL in the same directory.
 */
#include <windows.h>

typedef WORD (FAR PASCAL *PalCoreTestFn)(void);

static char hex_digit(unsigned value)
{
    value &= 15U;
    if (value < 10U)
        return (char)('0' + value);
    return (char)('A' + value - 10U);
}

static void show_failures(WORD failures)
{
    char message[] = "Palettium core self-test FAILED: 0x0000";
    unsigned value = (unsigned)failures;
    unsigned i;
    /* Four hexadecimal positions at end of fixed message. */
    for (i = 0; i < 4; ++i) {
        message[sizeof(message) - 2U - i] = hex_digit(value);
        value >>= 4;
    }
    MessageBox((HWND)0, message, "PALTEST", MB_OK | MB_ICONSTOP);
}

int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command)
{
    HINSTANCE module;
    FARPROC address;
    PalCoreTestFn self_test;
    WORD result;

    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show_command;

    module = LoadLibrary("PALCORE.DLL");
    /* Win16 LoadLibrary returns an error code <= 32 on failure. */
    if ((UINT)module <= 32U) {
        MessageBox((HWND)0,
                   "Cannot load PALCORE.DLL. Put it beside PALTEST.EXE.",
                   "PALTEST", MB_OK | MB_ICONSTOP);
        return 1;
    }

    address = GetProcAddress(module, "PALETTIUMCORESELFTEST");
    if (address == (FARPROC)0) {
        MessageBox((HWND)0, "PALCORE.DLL has no self-test export.",
                   "PALTEST", MB_OK | MB_ICONSTOP);
        FreeLibrary(module);
        return 2;
    }

    self_test = (PalCoreTestFn)address;
    result = (*self_test)();

    if (result == 0) {
        MessageBox((HWND)0,
                   "PASS: Palettium core runs under Windows 3.1.\n"
                   "Pixel, line, fill, and BitBlt checks passed.",
                   "PALTEST", MB_OK | MB_ICONINFORMATION);
    } else {
        show_failures(result);
    }

    FreeLibrary(module);
    return result == 0 ? 0 : 3;
}
