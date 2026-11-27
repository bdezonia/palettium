#ifndef PAL_TRIAL_GATE_H
#define PAL_TRIAL_GATE_H
/* Pure C89 policy for the separate Win3.1 Standard-mode ET4000AX trial.
 * Deliberately excludes enhanced mode, unknown hardware and auto-enable. */
int pal_win31_trial_permitted(unsigned long flags, unsigned vram_kib,
                              unsigned confirmation_key);
/* Experimental startup only; exact known reference configuration, not
 * certification of loaded VDD identity or screen switching. */
int pal_win31_enh_startup_permitted(unsigned long flags, unsigned vram_kib,
    unsigned key, const char *vdd, const char *grabber);
#endif
