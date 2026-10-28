#include "sim_bank.h"
#include <assert.h>

void pal_bank_init(PAL_BankSim *sim, rg_u8 *backing, rg_offset size)
{
    sim->backing = backing;
    sim->size = size;
    sim->current_bank = 0;
    sim->switch_count = 0;
}

static rg_offset pal_window_offset(PAL_BankSim *sim, rg_offset offset)
{
    rg_offset bank;
    rg_offset within;
    assert(offset < sim->size);
    bank = offset / PAL_BANK_SIZE;
    within = offset % PAL_BANK_SIZE;
    if (bank != sim->current_bank) {
        sim->current_bank = bank;
        ++sim->switch_count;
    }
    /* Resolve via selected bank and aperture offset, as an actual driver would. */
    return sim->current_bank * PAL_BANK_SIZE + within;
}

rg_u8 pal_bank_read(void *context, rg_offset offset)
{
    PAL_BankSim *sim = (PAL_BankSim *)context;
    return sim->backing[pal_window_offset(sim, offset)];
}

void pal_bank_write(void *context, rg_offset offset, rg_u8 value)
{
    PAL_BankSim *sim = (PAL_BankSim *)context;
    sim->backing[pal_window_offset(sim, offset)] = value;
}
