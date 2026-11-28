/* Host-only execution of the actual combined driver's profile gate.
 * Linker garbage collection discards unrelated Win16 DDI functions.
 * This does not emulate a native driver, selector or interrupt. */
#include <stdio.h>
#include <string.h>
#include "../windows/win31/palddi.c"
static DWORD flags;
static unsigned vram,trial,startup,hook,repaint;
static int render_trace=-1;
static const char *vdd,*grabber;
static unsigned enhanced_reads;
volatile unsigned pal_boottrace_busy,pal_boottrace_suspended;
DWORD GetWinFlags(void) { return flags; }
int GetPrivateProfileInt(const char *section,const char *key,int fallback,const char *filename)
{
    (void)section; (void)filename;
    if (!strcmp(key,"VRAMKiB")) return vram;
    if (!strcmp(key,"TrialEnable")) return trial;
    if (!strcmp(key,"RenderTrace")) return render_trace<0?fallback:render_trace;
    ++enhanced_reads;
    if (!strcmp(key,"EnhancedStartup")) return startup;
    if (!strcmp(key,"EnhancedHook")) return hook;
    if (!strcmp(key,"EnhancedRepaint")) return repaint;
    return fallback;
}
int GetPrivateProfileString(const char *section,const char *key,const char *fallback,char *buffer,int size,const char *filename)
{
    const char *source=!strcmp(section,"386Enh") ? vdd : grabber;
    unsigned n=(unsigned)strlen(source);
    (void)key; (void)fallback; (void)filename;
    ++enhanced_reads;
    if (n>=(unsigned)size) n=(unsigned)size-1u;
    memcpy(buffer,source,n); buffer[n]=0;
    return (int)n;
}
void pal_boottrace(const char *event) { (void)event; }
void pal_boottrace_hex(const char *event,unsigned value) { (void)event; (void)value; }
/* Route-level stubs: observe calls, not hardware or ABI correctness. */
static unsigned queries,installs,removes,resolves,ports,starts,stops,last_auth;
static int plan_assessment;
static unsigned selector=0x3b;
static unsigned hook_status=1;
static unsigned fg;
static void fake_repaint(void) { }
HMODULE GetModuleHandle(const char *name) { (void)name; ++resolves; return 1; }
FARPROC GetProcAddress(HMODULE module,const char *name)
{ (void)module; (void)name; return fake_repaint; }
void *paldiag_test_mk_fp(unsigned seg,unsigned off)
{ static char ordinal; (void)seg; (void)off; return &ordinal; }
unsigned pal_win16_vdd_selector(void) { ++queries; return selector; }
unsigned PAL_HOOK_CALL pal_hook_install(void) { ++installs; return hook_status; }
unsigned PAL_HOOK_CALL pal_hook_remove(void) { ++removes; return 1; }
unsigned PAL_HOOK_CALL pal_hook_read(unsigned index)
{ return index==PAL_HOOK_QUERY ? 1u : index==PAL_HOOK_FG ? fg : 0u; }
int pal_win16_et4k_profile_plan(PAL_ET4KPlan *p)
{ memset(p,0,sizeof(*p)); p->vdd_assessment=plan_assessment;
  p->configured_bytes=(unsigned long)vram*1024UL; p->mode.height=vram==256?400:480; return 1; }
int pal_win16_et4k_port(PAL_Win16ET4K *a,ET4K_Port *p)
{ (void)a; memset(p,0,sizeof(*p)); ++ports; return 1; }
PAL_U32 pal_ddi_far_token(PAL_U16 seg,PAL_U16 off)
{ (void)seg; (void)off; return 1; }
int pal_screen_host_gdi_snapshot(PAL_ScreenHost *h,PAL_U32 token,PAL_PDEVICE *p)
{ (void)h; (void)token; memset(p,0,sizeof(*p)); return 1; }
void pal_cursor16_bind(PAL_Cursor16 *c,RG_Device *d) { c->screen=d; }
void pal_cursor16_unbind(PAL_Cursor16 *c) { c->screen=0; }
int pal_et4k_session_start_published(PAL_ET4KSession *s,ET4K_Port p,
    const PAL_ET4KPlan *plan,PAL_U32 token,PAL_ET4KPublish publish,
    void *ctx,int auth)
{ (void)p; (void)plan; ++starts; last_auth=auth;
  s->host.ddi.screen_token=token; return publish(ctx,&s->host.pdevice); }
void pal_et4k_session_stop(PAL_ET4KSession *s) { (void)s; ++stops; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    unsigned f;
    PAL_PDEVICE dest;
    vram=512; trial=4401; startup=4402; hook=4403; repaint=4404;
    vdd="VDDTLI4.386"; grabber="V7VGA.3GR";
    for (f=0; f<64; ++f) {
        flags=f; enhanced_reads=0;
        CHECK(pal_win16_install_ready()==((f&0x31u)==0x11u || (f&0x31u)==0x21u));
        CHECK(PAL_ENH_ACTIVE==((f&0x31u)==0x21u));
        CHECK((enhanced_reads!=0)==PAL_ENH_ACTIVE);
    }
    flags=0x0c19; vram=256; startup=hook=repaint=0;
    vdd="*vddvga"; grabber="VGA.3GR"; enhanced_reads=0;
    CHECK(pal_win16_install_ready() && !PAL_ENH_ACTIVE && !enhanced_reads);
    trial=0; CHECK(!pal_win16_install_ready());
    trial=4401; vram=512; CHECK(pal_win16_install_ready());
    vram=1024; CHECK(!pal_win16_install_ready());
    flags=0x0c29; vram=512; trial=0;
    vdd="VDDTLI4.386"; grabber="V7VGA.3GR";
    CHECK(!pal_win16_install_ready());
    startup=4402; CHECK(!pal_win16_install_ready());
    hook=4403; CHECK(!pal_win16_install_ready());
    repaint=4404; CHECK(pal_win16_install_ready() && PAL_ENH_ACTIVE);
    pal_active_host=&pal_native_session.host; flags=0x0c19;
    CHECK(!pal_win16_install_ready() && PAL_ENH_ACTIVE);
    pal_active_host=0; flags=0x0c29;
    vram=256; CHECK(!pal_win16_install_ready());
    vram=1024; CHECK(!pal_win16_install_ready());
    vram=512; vdd="*vddvga"; CHECK(!pal_win16_install_ready());
    vdd="VDDTLI4.386"; grabber="VGA.3GR"; CHECK(!pal_win16_install_ready());
    grabber="V7VGA.3GR"; CHECK(pal_win16_install_ready());
    puts("PASS: actual unified activation gate, mode isolation and all 64 low flag combinations");
    /* Standard does not depend on VDD discovery, resolve USER or install
     * a hook, even when the Enhanced profile keys are also present. */
    flags=0x0c19; vram=256; trial=4401; selector=0;
    CHECK(pal_win16_install_ready());
    plan_assessment=PAL_VDD_UNVERIFIED;
    CHECK(!pal_win16_activate(&dest) && !ports && !starts);
    plan_assessment=PAL_VDD_NOT_ENHANCED;
    CHECK(pal_win16_activate(&dest) && last_auth==PAL_ET4K_AUTH_STANDARD);
    CHECK(ports==1 && starts==1 && !queries && !installs && !resolves);
    CHECK(!pal_repaint.armed && !pal_hook_recovery_enabled);
    Disable(&dest);
    CHECK(stops==1 && !removes && !pal_active_host);
    flags=0x0c29; vram=512;
    CHECK(pal_win16_install_ready());
    CHECK(!pal_win16_activate(&dest) && queries==0 && ports==1);
    plan_assessment=PAL_VDD_UNVERIFIED;
    CHECK(!pal_win16_activate(&dest) && queries==1 && ports==1);
    selector=0x3b;
    CHECK(pal_win16_activate(&dest) && last_auth==PAL_ET4K_AUTH_ENH_STARTUP);
    CHECK(ports==2 && starts==2 && queries==3 && installs==1 && resolves==1);
    CHECK(pal_repaint.armed && pal_hook_recovery_enabled);
    Disable(&dest);
    CHECK(stops==2 && removes==1 && !pal_active_host && !pal_repaint.armed);
    /* No stale hook/repaint state from Enhanced on a fresh Standard route. */
    flags=0x0c19; plan_assessment=PAL_VDD_NOT_ENHANCED;
    CHECK(pal_win16_install_ready() && pal_win16_activate(&dest));
    CHECK(queries==3 && installs==1 && resolves==1 && !pal_hook_recovery_enabled);
    Disable(&dest); CHECK(removes==1 && stops==3);
    render_trace=0;
    CHECK(pal_win16_install_ready() && !pal_render_trace);
    CHECK(pal_win16_activate(&dest));
    Disable(&dest);
    render_trace=1;
    CHECK(pal_win16_install_ready() && pal_render_trace);
    puts("PASS: quiet rendering profile preserves unified activation and shutdown");
    puts("PASS: actual unified activate/Disable dispatch with mocked platform services");
    return 0;
}
