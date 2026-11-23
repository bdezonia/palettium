#ifndef PAL_DDL_BLIT_H
#define PAL_DDL_BLIT_H
/* Far address marshalling for Win16 GDI BitBlt; no pointer dereferencing.
 * This interface is OS-private and independent of any chipset.
 */
#include "pal_screen_host.h"

/* Windows far address identity: high word selector, low word offset. */
PAL_U32 pal_ddi_far_token(PAL_U16 segment, PAL_U16 offset);

/* Call only after successful hardware startup and GDI device binding.
 * This stage supports screen-to-screen only and pattern-independent ROPs.
 * Return 1 on success, 0 on failure; no writes on unsupported inputs.
 */
int pal_ddi_bitblt_words(PAL_ScreenHost *host,
                         PAL_U16 dst_segment, PAL_U16 dst_offset,
                         int dx, int dy,
                         PAL_U16 src_segment, PAL_U16 src_offset,
                         int sx, int sy, int width, int height,
                         unsigned long rop);
#endif
