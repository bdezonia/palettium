/* Host-side exercise of the real Win16 diagnostic with fault-isolating steps. */
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "dos.h"

int WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR line,int command);
static int step_to_allow;   /* 0..3; step 4 is optional BIOS readback */
static int allow_bios;
static int have_kernel;
static int have_aperture;
static int initial_seen, stage_seen[5], kernel_calls, aperture_calls;
static int flags_calls, bios_calls, base_calls;
static int requested_steps;

HMODULE GetModuleHandle(const char *name)
{
    ++kernel_calls;
    if (strcmp(name,"KERNEL") != 0 || !have_kernel) return 0;
    return 1;
}
FARPROC GetProcAddress(HMODULE module,const char *name)
{
    ++aperture_calls;
    if (module != 1 || strcmp(name,"__A000H") != 0 || !have_aperture)
        return (FARPROC)0;
    return (FARPROC)(unsigned long)0xABCD0047UL;
}
DWORD GetWinFlags(void) { ++flags_calls; return 0x00000020UL; }
int MessageBox(HWND window,const char *message,const char *title,unsigned flags)
{
    (void)window;
    (void)title;
    if (strstr(message,"STEP 0:")) ++initial_seen;
    if (strstr(message,"STEP 1:")) ++stage_seen[1];
    if (strstr(message,"STEP 2:")) ++stage_seen[2];
    if (strstr(message,"STEP 3:")) {
        ++stage_seen[3];
        if (have_aperture &&
            (!strstr(message,"ABCD:0047") || !strstr(message,"0047")))
            return -9;
    }
    if (strstr(message,"STEP 4: INT 10h")) {
        ++stage_seen[4];
        if (!strstr(message,"AL mode = 2F")) return -9;
    }
    if (flags & MB_YESNO) {
        if (strstr(message,"OPTIONAL STEP 4")) return allow_bios ? IDYES : IDNO;
        ++requested_steps;
        return requested_steps <= step_to_allow ? IDYES : IDNO;
    }
    return IDNO;
}
int int86(int interrupt_number,union REGS *in,union REGS *out)
{
    if (interrupt_number != 0x10 || in->x.ax != 0x0F00u) return -1;
    ++bios_calls;
    out->h.al=0x2F;
    out->h.ah=80;
    out->h.bh=0;
    return 0;
}
static void reset(int step, int bios)
{
    step_to_allow=step;
    allow_bios=bios;
    requested_steps=0;
    initial_seen=kernel_calls=aperture_calls=flags_calls=bios_calls=base_calls=0;
    memset(stage_seen,0,sizeof(stage_seen));
    have_kernel=have_aperture=1;
}
static int run(int step,int bios,int expected)
{
    int rc;
    reset(step,bios);
    rc=WinMain(0,0,(char *)"",0);
    if (rc != expected || initial_seen != 1 || base_calls != 0) { fprintf(stderr,"early step %d rc %d exp %d init %d\n", step,rc,expected,initial_seen); return 0; }
    if (flags_calls != (step>=1) || kernel_calls != (step>=2) ||
        aperture_calls != (step>=3)) { fprintf(stderr,"calls step %d flags %d kernel %d aperture %d\n",step,flags_calls,kernel_calls,aperture_calls); return 0; }
    if (stage_seen[1] != (step>=1) || stage_seen[2] != (step>=2) ||
        stage_seen[3] != (step>=3)) { fprintf(stderr,"stages step %d %d %d %d\n",step,stage_seen[1],stage_seen[2],stage_seen[3]); return 0; }
    if (bios_calls != (step>=3 && bios) ||
        stage_seen[4] != (step>=3 && bios)) {fprintf(stderr,"bios step %d %d calls %d stage %d\n",step,bios,bios_calls,stage_seen[4]); return 0;}
    return 1;
}
int main(void)
{
    if (!run(0,0,0) || !run(1,0,0) || !run(2,0,0) ||
        !run(3,0,0) || !run(3,1,0)) return 1;
    reset(3,0);
    have_kernel=0;
    if (WinMain(0,0,(char *)"",0) != 2 || aperture_calls || bios_calls) return 2;
    reset(3,0);
    have_aperture=0;
    if (WinMain(0,0,(char *)"",0) != 3 || bios_calls) return 3;
    puts("PASS: PALDIAG v0033 checkpoints isolate API calls; no selector base/dereference; BIOS opt-in");
    return 0;
}
