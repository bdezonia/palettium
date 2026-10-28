#ifndef PAL_SIM_BANK_H
#define PAL_SIM_BANK_H
#include "rgfx.h"

/* Host-only test device. Models a banked aperture, not a real SVGA chipset.
   V1: a single 64-KiB read/write window, 64-KiB bank granularity. */
#define PAL_BANK_SIZE 65536UL
typedef struct PAL_BankSim {
    rg_u8 *backing;
    rg_offset size;
    rg_offset current_bank;
    unsigned long switch_count;
} PAL_BankSim;

void pal_bank_init(PAL_BankSim *sim, rg_u8 *backing, rg_offset size);
rg_u8 pal_bank_read(void *context, rg_offset offset);
void pal_bank_write(void *context, rg_offset offset, rg_u8 value);
#endif
