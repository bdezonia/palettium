#ifndef PAL_ET4K_H
#define PAL_ET4K_H

#include "rg_hardware.h"

/* Native ET4000AX backend, revision 1.
 * 0x3CD: low nibble write bank, high nibble read bank; 64 KiB windows.
 * Firmware/platform sets the graphics mode. No VESA dependency.
 * Platform functions deal with physical ports and the A000 aperture.
 */
typedef rg_u8 (*ET4K_InFn)(void *cookie, unsigned port);
typedef void (*ET4K_OutFn)(void *cookie, unsigned port, rg_u8 value);
typedef rg_u8 (*ET4K_MemReadFn)(void *cookie, unsigned offset);
typedef void (*ET4K_MemWriteFn)(void *cookie, unsigned offset, rg_u8 value);
typedef int (*ET4K_ModeFn)(void *cookie, unsigned width, unsigned height);
typedef void (*ET4K_RestoreFn)(void *cookie);

typedef struct ET4K_Port {
    void *cookie;
    ET4K_InFn in_port;
    ET4K_OutFn out_port;
    ET4K_MemReadFn mem_read;
    ET4K_MemWriteFn mem_write;
    ET4K_ModeFn set_mode;
    ET4K_RestoreFn restore;
} ET4K_Port;

typedef struct ET4K_State {
    ET4K_Port io;
    unsigned long capacity;
    unsigned read_bank;
    unsigned write_bank;
    unsigned bank_valid;
    unsigned active;
} ET4K_State;

/* capacity is VRAM size in bytes (usually 512K or 1M); state lives in caller. */
void et4k_setup(ET4K_State *state, ET4K_Port io, unsigned long capacity);
void et4k_make_hardware(ET4K_State *state, RG_Hardware *hardware);

#endif
