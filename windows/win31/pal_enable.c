#include "pal_enable.h"
#include "pal_pdevice.h"

int pal_enable_capability(PAL_GDIINFO *out, unsigned width,
                          unsigned height)
{
    if (!out || !pal_pd_layout_ok()) return 0;
    /* No hardware access during GDI's capability discovery stage. */
    return pal_win31_make_candidate(out, width, height,
                                    (unsigned)sizeof(PAL_PDEVICE));
}
