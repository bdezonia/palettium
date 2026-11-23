#ifndef PAL_PDEVICE_H
#define PAL_PDEVICE_H
/* Palettium Win3.x private PDEVICE candidate. Not an installable DDI.
 * The DDK permits driver-defined PDEVICE contents. No process pointers
 * are embedded: Win16 near/far pointer semantics belong to the OS adapter.
 */
#include "pal_gdiinfo.h"
#define PAL_PD_MAGIC 0x50414C44UL
#if defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8
typedef unsigned int PAL_U32;
#else
typedef unsigned long PAL_U32;
#endif
#define PAL_PD_OFF 0u
#define PAL_PD_READY 1u
#define PAL_PD_ACTIVE 2u
#pragma pack(push, 1)
typedef struct PAL_PDEVICE {
    PAL_U16 type;          /* nonzero: device-specific, not RAM BITMAP */
    PAL_U16 bytes;         /* sizeof(PAL_PDEVICE) */
    PAL_U16 state;         /* OFF/READY/ACTIVE */
    PAL_U16 width;
    PAL_U16 height;
    PAL_U16 pitch;         /* byte stride, 8-bit indexed only */
    PAL_U16 bpp;
    PAL_U16 reserved;
    PAL_U32 magic;     /* exact 32-bit value across host and Win16 */
} PAL_PDEVICE;
#pragma pack(pop)

/* First stage prepares private state only; no BIOS/port I/O is performed.
 * A future platform/hardware binding activates the device separately. */
int pal_pd_layout_ok(void);
int pal_pd_prepare(PAL_PDEVICE *pd, unsigned width, unsigned height,
                   unsigned pitch);
int pal_pd_activate(PAL_PDEVICE *pd);
int pal_pd_stop(PAL_PDEVICE *pd);
int pal_pd_valid(const PAL_PDEVICE *pd);
int pal_pd_capability_candidate(PAL_GDIINFO *info, const PAL_PDEVICE *pd);
#endif
