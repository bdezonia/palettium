#ifndef RG_HARDWARE_H
#define RG_HARDWARE_H

#include "rgfx.h"

/* Palettium hardware contract, revision 1. C89 and Win16 friendly.
 * All offsets are logical 32-bit-or-greater VRAM byte offsets.
 * The backend handles bank switching internally in read/write_byte.
 * No operating-system types, port I/O or far pointers appear here.
 */
typedef struct RG_Mode {
    unsigned width;
    unsigned height;
    unsigned depth;     /* revision 1: only 8-bit indexed color */
    unsigned stride;    /* actual bytes per scanline, supplied by backend */
} RG_Mode;

struct RG_Hardware;
typedef struct RG_Hardware RG_Hardware;

typedef int (*RG_HwInitFn)(void *context);
typedef int (*RG_HwModeFn)(void *context, RG_Mode *mode);
typedef void (*RG_HwShutdownFn)(void *context);
typedef void (*RG_HwPaletteFn)(void *context, unsigned index,
                               rg_u8 red, rg_u8 green, rg_u8 blue);
typedef void (*RG_HwAccelFn)(void *context, RG_Device *device);

struct RG_Hardware {
    void *context;
    RG_HwInitFn initialize;       /* required: nonzero = success */
    RG_HwModeFn set_mode;         /* required: may adjust actual stride */
    RG_HwShutdownFn shutdown;    /* optional */
    RG_ReadByteFn read_byte;     /* required; bank handling stays here */
    RG_WriteByteFn write_byte;   /* required; bank handling stays here */
    RG_HwPaletteFn set_palette;  /* optional; 8-bit RGB inputs */
    RG_HwAccelFn install_accel;  /* optional; override selected RG_Ops */
};

typedef struct RG_HwBinding {
    RG_Device *device;
    RG_Hardware *hardware;
    unsigned active;
} RG_HwBinding;

/* Caller owns device, hardware, backend context and binding throughout use.
 * Set binding.active=0 before the first call; never open twice without close.
 * On failure initialized hardware is shut down, if callback is supplied.
 * Mode is input and, on success, describes the actual selected surface.
 */
int rg_hw_open(RG_HwBinding *binding, RG_Device *device,
               RG_Hardware *hardware, RG_Mode *mode);
int rg_hw_palette(RG_HwBinding *binding, unsigned index,
                  rg_u8 red, rg_u8 green, rg_u8 blue);
void rg_hw_close(RG_HwBinding *binding);

#endif
