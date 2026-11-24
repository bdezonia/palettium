/* Win16 loader entry for the Palettium GDI staging module.
 * This file contains no card-specific code and no graphics assumptions.
 * PALSTAGE.DRV is NOT an installable display driver.
 */
#include <windows.h>
#include "pal_boottrace.h"

BOOL FAR PASCAL LibMain(HINSTANCE instance, WORD data_segment,
                        WORD heap_size, LPSTR command_line)
{
    (void)instance;
    (void)data_segment;
    (void)heap_size;
    (void)command_line;
    pal_boottrace("LIBMAIN OK");
    return TRUE;
}
