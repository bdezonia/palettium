#ifndef PAL_ET4K_WIN16_H
#define PAL_ET4K_WIN16_H
/* Win16-only ET4000AX platform services; not an OS-independent backend.
 * This adapter is compiled into PALSTAGE.DRV for *link validation*, but
 * no DDI entry point activates it. An installable driver additionally
 * needs GDI PDEVICE ownership, text/cursor and VDD coordination.
 */
#include "et4k.h"

typedef struct PAL_Win16ET4K {
    unsigned aperture_selector;
    unsigned saved_bios_mode;
    unsigned previous_valid;
    unsigned bios_mode_attempted;
} PAL_Win16ET4K;

/* Resolves KERNEL's A000h protected-mode selector; no direct physical
 * segment casts and NO video-mode change. Caller must provide validated
 * capacity (256/512/1024 KiB) to et4k_setup/session_start separately. */
int pal_win16_et4k_port(PAL_Win16ET4K *context, ET4K_Port *out);
#endif
