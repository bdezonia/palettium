#ifndef PAL_ET4K_H
#define PAL_ET4K_H

#include "rg_hardware.h"

/* ET4000AX packed INDEX8 modes supported by the current renderer.
 * A BIOS mode number is a known Tseng convention, not a universal
 * guarantee that a particular board's firmware implements the mode.
 * The platform callback must actually establish the mode.
 */
#define ET4K_BIOS_640X400X8 0x2Fu
#define ET4K_BIOS_640X480X8 0x2Eu

/* 0x3CD: low nibble write bank, high nibble read bank; 64 KiB windows.
 * No OS-specific port I/O, BIOS calls, far pointers or VRAM probes here.
 */
typedef rg_u8 (*ET4K_InFn)(void *cookie, unsigned port);
typedef void (*ET4K_OutFn)(void *cookie, unsigned port, rg_u8 value);
typedef rg_u8 (*ET4K_MemReadFn)(void *cookie, unsigned offset);
typedef void (*ET4K_MemWriteFn)(void *cookie, unsigned offset, rg_u8 value);
typedef int (*ET4K_ModeFn)(void *cookie, unsigned width, unsigned height);
typedef void (*ET4K_RestoreFn)(void *cookie);
/* Optional, read-only VRAM-size discovery supplied by the OS/BIOS adapter.
 * Return zero for unknown. Must not write VRAM or change video state.
 * If omitted, et4k_setup's explicit capacity is required. */
typedef unsigned long (*ET4K_CapacityFn)(void *cookie);
/* Optional non-destructive firmware/board-specific availability query.
 * If absent, mode listing reports VRAM-compatible CANDIDATES only;
 * set_mode is still authoritative and may refuse a mode. */
typedef int (*ET4K_ModeQueryFn)(void *cookie, unsigned bios_mode);

typedef struct ET4K_Port {
    void *cookie;
    ET4K_InFn in_port;
    ET4K_OutFn out_port;
    ET4K_MemReadFn mem_read;
    ET4K_MemWriteFn mem_write;
    ET4K_ModeFn set_mode;
    ET4K_RestoreFn restore;
    ET4K_CapacityFn query_capacity;
    ET4K_ModeQueryFn query_mode;
    /* Optional software aperture fill. count is 1..65535, and the
     * entire range is in the currently selected write bank. Zero if absent.
     * All callers must initialize this field (or zero-initialize ET4K_Port). */
    void (*mem_fill)(void *cookie, unsigned offset, unsigned count, rg_u8 value);
} ET4K_Port;

typedef struct ET4K_State {
    ET4K_Port io;
    unsigned long capacity;
    unsigned read_bank;
    unsigned write_bank;
    unsigned bank_valid;
    unsigned mode_attempted; /* shutdown restores ONLY after a mode attempt */
    unsigned active;
} ET4K_State;

typedef struct ET4K_ModeInfo {
    unsigned width;
    unsigned height;
    RG_PixelFormat format;
    unsigned bios_mode;
    unsigned long minimum_vram;
} ET4K_ModeInfo;

/* v0036 diagnostic ONLY: snapshot CRTC 37h (Video System Configuration 2).
 * Reads 3CCh and the active color/mono CRTC index/data ports. Selecting
 * register 37h temporarily WRITES the CRTC INDEX port only; its previous
 * index is restored before returning. Does not write CRTC DATA, touch
 * bank 3CDh, probe VRAM, set a BIOS mode, or unlock protected registers.
 * An inaccessible/locked extended register may return misleading data.
 * Call only where raw port access is explicitly permitted (DOS), NEVER
 * from Win3.x protected mode without a VDD contract. This is NOT a
 * chipset-identity, VRAM-capacity, or mode-eligibility probe.
 */
typedef struct ET4K_Crtc37 {
    unsigned crtc_index_port;
    rg_u8 misc_output;
    rg_u8 previous_index;
    rg_u8 raw_value;
    unsigned bus_width_bits;     /* 8/16/32 memory bus from low bits */
    unsigned chip_depth_k;       /* nominal 64K / 256K memory organization */
    unsigned long capacity_hint; /* hypothetical bytes, NEVER verified */
} ET4K_Crtc37;
int et4k_crtc37_snapshot(const ET4K_Port *io, ET4K_Crtc37 *result);
/* Decode register 37h as an ET4000-family memory organization hint.
 * This does NOT check physical chips, VRAM aliases, chipset model or BIOS.
 */
void et4k_crtc37_decode(rg_u8 raw, ET4K_Crtc37 *result);

/* capacity = known bytes (256K/512K/1M) or zero to use query_capacity.
 * In particular, 256K is no longer rejected at initialization.
 * If capacity is unknown or invalid, no mode will be set.
 */
void et4k_setup(ET4K_State *state, ET4K_Port io, unsigned long capacity);
void et4k_make_hardware(ET4K_State *state, RG_Hardware *hardware);
/* Optional read-only board-capacity probe, callable before mode selection.
 * Reject unknown sizes; never change registers or framebuffer memory.
 * Zero means "not known" and must not be treated as 512K or 1M. */
unsigned long et4k_probe_capacity(ET4K_State *state);
/* Pure catalog check; no hardware access. Returns 0 on unsupported format,
 * dimensions, or insufficient capacity. The output can be NULL. */
int et4k_mode_info(unsigned width, unsigned height, RG_PixelFormat format,
                    ET4K_ModeInfo *info);
/* Read-only candidate checks. Optional query callbacks may run; these do
 * not attempt a video mode. Use AFTER initialize or with known capacity. */
int et4k_mode_eligible(const ET4K_State *state,
                        const ET4K_ModeInfo *info);
/* Prefer 640x480x256, then 640x400x256 on a 256K board.
 * 640x480x16 planar is NOT supported by this packed-pixel backend.
 * Returns candidate geometry only: caller still must use rg_hw_open.
 */
int et4k_preferred_mode(ET4K_State *state, RG_Mode *mode);

#endif
