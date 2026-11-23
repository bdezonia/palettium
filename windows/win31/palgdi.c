/* Palettium Windows 3.1 GDI ABI shell, v0.10.
 *
 * IMPORTANT: NOT AN INSTALLABLE DISPLAY DRIVER.
 * Deliberately refuses Enable until correct DDK GDIINFO/PDEVICE layouts,
 * drawing entry points, video-switching and cursor paths are implemented.
 * This module contains NO ET4000-specific code.
 */
#include <windows.h>

/* Microsoft Win3.x DDK: Enable(destination, style, device, output, data).
 * Style bit 0 selects GDIINFO (1) vs physical device (0).
 * We must never claim success until both paths are fully implemented.
 * LPVOID is used to leave both structures opaque in this shell.
 */
WORD FAR PASCAL __export Enable(LPVOID destination,
                               WORD style,
                               LPSTR device_type,
                               LPSTR output_file,
                               LPVOID data)
{
    (void)destination;
    (void)style;
    (void)device_type;
    (void)output_file;
    (void)data;
    return 0; /* fail closed: no mode switch, no GDIINFO fabrication */
}

/* DDK Disable receives a pointer to the physical device. */
void FAR PASCAL __export Disable(LPVOID physical_device)
{
    (void)physical_device;
}

/* Ordinary Win16 DLL startup, not a video-mode initialization path. */
BOOL FAR PASCAL LibMain(HINSTANCE instance, WORD ds, WORD heap, LPSTR cmd)
{
    (void)instance;
    (void)ds;
    (void)heap;
    (void)cmd;
    return TRUE;
}
