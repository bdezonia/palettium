#include "pal_trial_gate.h"
int pal_win31_trial_permitted(unsigned long flags, unsigned vram_kib,
                              unsigned confirmation_key)
{
    /* WF_PMODE=1, WF_STANDARD=0x10, WF_ENHANCED=0x20; all others ignored. */
    return (flags & 0x0031UL) == 0x0011UL &&
           (vram_kib == 256u || vram_kib == 512u) && confirmation_key == 4401u;
}

static int same_ascii(const char *a, const char *b)
{
    unsigned ca,cb;
    if (!a || !b) return 0;
    do {
        ca=(unsigned char)*a++; cb=(unsigned char)*b++;
        if (ca>='a' && ca<='z') ca-='a'-'A';
        if (cb>='a' && cb<='z') cb-='a'-'A';
        if (ca!=cb) return 0;
    } while (ca);
    return 1;
}
int pal_win31_enh_startup_permitted(unsigned long flags, unsigned vram_kib,
    unsigned key, const char *vdd, const char *grabber)
{
    return (flags & 0x0031UL)==0x0021UL && vram_kib==512u && key==4402u &&
        same_ascii(vdd,"VDDTLI4.386") && same_ascii(grabber,"V7VGA.3GR");
}
