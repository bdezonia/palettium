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

/* Staged, non-operational Win3.x mode plan. The caller supplies an explicit
 * VRAM configuration (not a CRTC-derived capacity). CRTC37 is optional
 * cross-check evidence only. Neither a VDD filename nor this plan authorizes
 * a protected-mode mode set, port write, or Enable(style=0).
 * This planner performs NO I/O and never calls the BIOS query callback. */
#define PAL_ET4K_PLAN_OK 1u
#define PAL_ET4K_PLAN_NO_CAPACITY 2u
#define PAL_ET4K_PLAN_HINT_CONFLICT 3u
#define PAL_ET4K_PLAN_NO_MODE 4u
#define PAL_ET4K_PLAN_STOCK_VDD_LIMIT 5u

typedef struct PAL_ET4KPlan {
    RG_Mode mode;
    unsigned bios_mode;
    unsigned long configured_bytes;
    unsigned long observed_hint;
    unsigned long framebuffer_bytes;
    unsigned status;
    int vdd_assessment;
    /* Always zero until an independently validated VDD/GDI lifecycle is
     * implemented. Not a permit for et4k_session_start(). */
    unsigned can_enable;
} PAL_ET4KPlan;

int pal_et4k_plan(PAL_ET4KPlan *out, unsigned long configured_bytes,
                  unsigned long observed_hint, unsigned long windows_flags,
                  const char *vdd_display);
int pal_et4k_plan_capability(PAL_GDIINFO *info,
                             const PAL_ET4KPlan *plan);
/* Explicit geometry for a future GDI-owned PDEVICE, currently host-only.
 * This does not bypass Windows VDD checks and MUST NOT be used directly
 * by Win16 Enable(style=0). */
int pal_et4k_session_start_mode(PAL_ET4KSession *session, ET4K_Port io,
                                unsigned long configured_bytes,
                                PAL_U32 pdevice_token,
                                unsigned width, unsigned height);

/* Transactional GDI publication. The platform validates the REAL GDI FAR
 * buffer and makes an atomic copy of the PDEVICE bytes in commit(). This
 * never treats a Win16 FAR address as a C near pointer. The callback MUST
 * leave its destination unchanged on failure. The session is rolled back
 * after failed mode setup, snapshot or commit, including video-mode restore.
 *
 * PLATFORM AUTHORIZATION IS NOT CONFERRED BY A PLAN. The caller must obtain
 * it separately. The explicit experimental authorization admits ONLY
 * an unverified 512 KiB 640x480 Enhanced startup plan; the platform must
 * enforce the exact VDD/grabber configuration and startup-only restriction. */
/* Distinct authorization values: ordinary Standard callers must never
 * accidentally admit an Enhanced plan. STARTUP is not a switching permit. */
#define PAL_ET4K_AUTH_STANDARD 1
#define PAL_ET4K_AUTH_ENH_STARTUP 2
typedef int (*PAL_ET4KPublish)(void *context, const PAL_PDEVICE *image);
int pal_et4k_session_start_published(PAL_ET4KSession *session,
                                     ET4K_Port io,
                                     const PAL_ET4KPlan *plan,
                                     PAL_U32 pdevice_token,
                                     PAL_ET4KPublish publish,
                                     void *context,
                                     int platform_authorized);

#endif /* PAL_ET4K_SESSION_H */
