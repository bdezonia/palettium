#include "pal_trial_gate.h"
int pal_win31_trial_permitted(unsigned long flags, unsigned vram_kib,
                              unsigned confirmation_key)
{
    /* WF_PMODE=1, WF_STANDARD=0x10, WF_ENHANCED=0x20; all others ignored. */
    return (flags & 0x0031UL) == 0x0011UL &&
           (vram_kib == 256u || vram_kib == 512u) && confirmation_key == 4401u;
}
