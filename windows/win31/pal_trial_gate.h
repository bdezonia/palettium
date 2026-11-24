#ifndef PAL_TRIAL_GATE_H
#define PAL_TRIAL_GATE_H
/* Pure C89 policy for the separate Win3.1 Standard-mode ET4000AX trial.
 * Deliberately excludes enhanced mode, unknown hardware and auto-enable. */
int pal_win31_trial_permitted(unsigned long flags, unsigned vram_kib,
                              unsigned confirmation_key);
#endif
