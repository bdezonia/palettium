#ifndef PAL_ET4K_WIN16_H
#define PAL_ET4K_WIN16_H
/* Win16-only ET4000AX platform services; not an OS-independent backend.
 * This adapter is compiled into PALSTAGE.DRV for *link validation*, but
 * no DDI entry point activates it. An installable driver additionally
 * needs GDI PDEVICE ownership, text/cursor and VDD coordination.
 */
#include "et4k.h"
#include "pal_et4k_session.h"

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
/* Read ONLY the user's explicit Palettium VRAMKiB setting and the
 * configured [386Enh] VDD. Does not read hardware or authorize Enable.
 * If no valid explicit capacity is set, returns zero (fail closed). */
int pal_win16_et4k_profile_plan(PAL_ET4KPlan *plan);
/* Lookup only: no far call to returned API and no assisted-save request. */
unsigned pal_win16_vdd_selector(void);
/* CRTC37 diagnostic wiring is intentionally NOT a query_capacity callback.
 * A controller must explicitly permit raw ports, and 386 Enhanced mode is
 * always refused here until a real VDD port-access contract exists. */
int pal_win16_et4k_crtc37_advisory(const ET4K_Port *io,
                                    ET4K_Crtc37 *result,
                                    int ports_authorized);
#endif
