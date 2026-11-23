#ifndef PAL_ENABLE_H
#define PAL_ENABLE_H
/* Hardware-independent capability-stage helper for the Win3.x adapter.
 * The resulting record is a development candidate, NOT a safe operational
 * screen driver capability declaration. Style 0 remains disabled.
 */
#include "pal_gdiinfo.h"
int pal_enable_capability(PAL_GDIINFO *out, unsigned width,
                          unsigned height);
#endif
