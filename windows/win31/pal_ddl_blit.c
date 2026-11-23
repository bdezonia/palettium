#include "pal_ddl_blit.h"

PAL_U32 pal_ddi_far_token(PAL_U16 segment, PAL_U16 offset)
{
    return ((PAL_U32)segment << 16) | (PAL_U32)offset;
}

int pal_ddi_bitblt_words(PAL_ScreenHost *host,
                         PAL_U16 dst_segment, PAL_U16 dst_offset,
                         int dx, int dy,
                         PAL_U16 src_segment, PAL_U16 src_offset,
                         int sx, int sy, int width, int height,
                         unsigned long rop)
{
    PAL_U32 dst = pal_ddi_far_token(dst_segment, dst_offset);
    PAL_U32 src = pal_ddi_far_token(src_segment, src_offset);
    if (!host || !host->running || !dst || !src || width <= 0 || height <= 0)
        return 0;
    return pal_screen_host_bitblt(host, dst, dx, dy,
                                  src, sx, sy, width, height, rop);
}
