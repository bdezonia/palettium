#ifndef PAL_SWITCH_H
#define PAL_SWITCH_H
#include "et4k.h"
#include "pal_cursor16.h"
/* Called only with the cursor draw guard held, from outside the multiplex
 * ISR. A new foreground epoch invalidates software assumptions, not VRAM.
 * Do not write the old saved-under pixels into a possibly restored screen.
 * No port, BIOS, USER, DOS or framebuffer access here. */
static int pal_switch_resume(volatile unsigned *last, unsigned foreground,
                             ET4K_State *card, PAL_Cursor16 *cursor)
{
    if (*last==foreground || !card->active || !cursor->screen ||
        cursor->composing || cursor->draw_depth!=1u) return 0;
    card->bank_valid=0;
    cursor->image.visible=0;
    cursor->image.device=0;
    *last=foreground;
    return 1;
}
#endif
