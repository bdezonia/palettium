#ifndef PAL_ET4K_SESSION_H
#define PAL_ET4K_SESSION_H
/* Win3.x session coordinator. No Win16 segment, BIOS or port access.
 * Platform supplies ET4K_Port. Not connected to GDI Enable(style=0).
 * Use separate persistent storage for the session and DDI PDEVICE.
 */
#include "et4k.h"
#include "pal_screen_host.h"

typedef struct PAL_ET4KSession {
    ET4K_State card;
    RG_Hardware hardware;
    PAL_ScreenHost host;
} PAL_ET4KSession;

/* GDI style-1 read-only preflight for a VERIFIED installed capacity.
 * Never changes video mode and does not guess the amount of RAM. */
int pal_et4k_candidate(PAL_GDIINFO *info, ET4K_Port io,
                        unsigned long installed_bytes);
/* A nonzero token must name the actual PDEVICE far pointer in the future
 * GDI integration; tests use a synthetic stable far-token. Does not
 * install a DDI host binding or enable PALSTAGE.DRV in Windows. */
int pal_et4k_session_start(PAL_ET4KSession *session, ET4K_Port io,
                           unsigned long installed_bytes,
                           PAL_U32 pdevice_token);
void pal_et4k_session_stop(PAL_ET4KSession *session);
#endif
