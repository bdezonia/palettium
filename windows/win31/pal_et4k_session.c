#include "pal_et4k_session.h"
#include "pal_enable.h"

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

int pal_et4k_session_start(PAL_ET4KSession *session, ET4K_Port io,
                           unsigned long installed_bytes,
                           PAL_U32 pdevice_token)
{
    RG_Mode mode;
    if (!session || !pdevice_token || session->host.running) return 0;
    et4k_setup(&session->card,io,installed_bytes);
    /* Zero capacity is permitted only when the platform offers a strictly
     * read-only query; this call never performs an alias/write probe. */
    if (!et4k_preferred_mode(&session->card,&mode)) return 0;
    et4k_make_hardware(&session->card,&session->hardware);
    return pal_screen_host_start(&session->host,&session->hardware,
                                  mode.width,mode.height,pdevice_token);
}

void pal_et4k_session_stop(PAL_ET4KSession *session)
{
    if (session) pal_screen_host_stop(&session->host);
}
