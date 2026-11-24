#include "pal_et4k_session.h"
#include "pal_boottrace.h"
#include "pal_enable.h"
#include "pal_vdd.h"
#include <string.h>

int pal_et4k_candidate(PAL_GDIINFO *info, ET4K_Port io,
                        unsigned long installed_bytes)
{
    ET4K_State temporary;
    RG_Mode mode;
    if (!info) return 0;
    et4k_setup(&temporary,io,installed_bytes);
    if (!et4k_preferred_mode(&temporary,&mode)) return 0;
    return pal_enable_capability(info,mode.width,mode.height);
}

static int pal_et4k_open_prepared(PAL_ET4KSession *session,
                                   PAL_U32 token,
                                   unsigned width, unsigned height)
{
    pal_boottrace("SESSION OPEN PREPARED");
    et4k_make_hardware(&session->card,&session->hardware);
    return pal_screen_host_start(&session->host,&session->hardware,
                                  width,height,token);
}

int pal_et4k_session_start(PAL_ET4KSession *session, ET4K_Port io,
                           unsigned long installed_bytes,
                           PAL_U32 pdevice_token)
{
    RG_Mode mode;
    if (!session || !pdevice_token || session->host.running) return 0;
    et4k_setup(&session->card,io,installed_bytes);
    /* Zero capacity is permitted only with a separately authorized,
     * read-only platform capacity callback. A CRTC hint is NOT one. */
    if (!et4k_preferred_mode(&session->card,&mode)) return 0;
    return pal_et4k_open_prepared(session,pdevice_token,
                                   mode.width,mode.height);
}

int pal_et4k_session_start_mode(PAL_ET4KSession *session, ET4K_Port io,
                                unsigned long installed_bytes,
                                PAL_U32 pdevice_token,
                                unsigned width, unsigned height)
{
    ET4K_ModeInfo info;
    pal_boottrace("SESSION START MODE ENTER");
    if (!session || !pdevice_token || session->host.running ||
        !et4k_mode_info(width,height,RG_FMT_INDEX8,&info)) return 0;
    et4k_setup(&session->card,io,installed_bytes);
    if (!et4k_probe_capacity(&session->card)) {
        pal_boottrace("SESSION CAPACITY FAIL");
        return 0;
    }
    if (!et4k_mode_eligible(&session->card,&info)) {
        pal_boottrace("SESSION ELIGIBILITY FAIL");
        return 0;
    }
    pal_boottrace("SESSION ELIGIBILITY OK");
    return pal_et4k_open_prepared(session,pdevice_token,width,height);
}

void pal_et4k_session_stop(PAL_ET4KSession *session)
{
    if (session) pal_screen_host_stop(&session->host);
}

/* Build a plan from an explicit known/selected capacity, not a guessed ROM
 * capacity. The caller can provide CRTC37 as a cross-check, but a mismatch
 * blocks planning rather than silently increasing supported VRAM. In 386
 * Enhanced mode, the stock VGA VDD cannot track a >256KiB framebuffer;
 * the lower-memory 640x400 candidate is still UNVERIFIED for VDD safety. */
int pal_et4k_plan(PAL_ET4KPlan *out, unsigned long configured_bytes,
                  unsigned long observed_hint, unsigned long windows_flags,
                  const char *vdd_display)
{
    ET4K_ModeInfo info;
    unsigned height;
    int stock_enhanced;
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    out->configured_bytes=configured_bytes;
    out->observed_hint=observed_hint;
    out->status=PAL_ET4K_PLAN_NO_CAPACITY;
    out->vdd_assessment=PAL_VDD_UNVERIFIED;
    if (configured_bytes != 262144UL && configured_bytes != 524288UL &&
        configured_bytes != 1048576UL) return 0;
    if (observed_hint && observed_hint != configured_bytes) {
        out->status=PAL_ET4K_PLAN_HINT_CONFLICT;
        return 0;
    }
    stock_enhanced=(windows_flags & 0x0020UL) != 0UL &&
                    pal_vdd_kind(vdd_display)==PAL_VDD_STOCK_VGA31;
    /* Prefer 480 lines for a sufficiently large board, except under the
     * stock Win3.1 enhanced-mode VDD. Framebuffer size, not installed RAM,
     * governs this documented >256KiB incompatibility. */
    height=(configured_bytes >= 307200UL && !stock_enhanced) ? 480u : 400u;
    if (!et4k_mode_info(640u,height,RG_FMT_INDEX8,&info) ||
        configured_bytes < info.minimum_vram) {
        out->status=PAL_ET4K_PLAN_NO_MODE;
        return 0;
    }
    out->mode.width=info.width;
    out->mode.height=info.height;
    out->mode.format=info.format;
    out->mode.stride=0;
    out->mode.capacity=0;
    out->bios_mode=info.bios_mode;
    out->framebuffer_bytes=info.minimum_vram;
    out->vdd_assessment=pal_vdd_preflight(windows_flags,vdd_display,
                                          info.minimum_vram);
    out->status=stock_enhanced && configured_bytes >= 307200UL
              ? PAL_ET4K_PLAN_STOCK_VDD_LIMIT : PAL_ET4K_PLAN_OK;
    /* Even an otherwise-valid mode is not proof that VDD, BIOS, PDEVICE,
     * text or cursor can operate. Never turn a candidate into permission. */
    out->can_enable=0u;
    return 1;
}

int pal_et4k_plan_capability(PAL_GDIINFO *info, const PAL_ET4KPlan *plan)
{
    if (!plan || !info ||
        (plan->status != PAL_ET4K_PLAN_OK &&
         plan->status != PAL_ET4K_PLAN_STOCK_VDD_LIMIT)) return 0;
    return pal_enable_capability(info,plan->mode.width,plan->mode.height);
}

/* Hardware and GDI publication are one transaction. Do not expose a
 * drawable screen between mode setting and successful PDEVICE publication.
 * Production Win16 code must additionally guarantee FAR-buffer validity,
 * atomic commit, a stable session address, and a VDD-approved environment.
 * The current Win16 driver does not authorize this operation. */
int pal_et4k_session_start_published(PAL_ET4KSession *session,
                                     ET4K_Port io,
                                     const PAL_ET4KPlan *plan,
                                     PAL_U32 pdevice_token,
                                     PAL_ET4KPublish publish,
                                     void *context,
                                     int platform_authorized)
{
    ET4K_ModeInfo geometry;
    PAL_PDEVICE image;
    pal_boottrace("SESSION TRANSACTION ENTER");
    if (!platform_authorized || !session || !plan || !pdevice_token ||
        !publish || session->host.running ||
        plan->status != PAL_ET4K_PLAN_OK ||
        plan->vdd_assessment != PAL_VDD_NOT_ENHANCED ||
        plan->configured_bytes == 0UL ||
        (plan->observed_hint &&
         plan->configured_bytes != plan->observed_hint) ||
        !et4k_mode_info(plan->mode.width,plan->mode.height,
                        plan->mode.format,&geometry) ||
        geometry.bios_mode != plan->bios_mode ||
        geometry.minimum_vram != plan->framebuffer_bytes ||
        plan->configured_bytes < geometry.minimum_vram) {
        pal_boottrace("SESSION PREFLIGHT FAIL");
        return 0;
    }
    pal_boottrace("SESSION PREFLIGHT OK");
    if (!pal_et4k_session_start_mode(session,io,plan->configured_bytes,
                                     pdevice_token,plan->mode.width,
                                     plan->mode.height)) return 0;
    pal_boottrace("SESSION HOST OK");
    if (!pal_screen_host_gdi_snapshot(&session->host,pdevice_token,&image)) {
        pal_boottrace("SESSION SNAPSHOT FAIL");
        pal_et4k_session_stop(session);
        return 0;
    }
    pal_boottrace("SESSION SNAPSHOT OK");
    if (!publish(context,&image)) {
        pal_boottrace("SESSION PUBLISH FAIL");
        pal_et4k_session_stop(session);
        return 0;
    }
    return 1;
}
