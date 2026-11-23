/* Host test compiles the real Win16 PALDIAG source against mock OS calls. */
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "dos.h"

int WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR line,int command);
static int option_yes;
static int selector_shown;
static int bios_shown;
static int bios_reads;
static int base_reads;
static int missing_aperture;
static int missing_kernel;
static int missing_base;

static DWORD fake_base(WORD selector)
{
    ++base_reads;
    return selector == 0x0007u ? 0x000A0000UL : 0xFFFFFFFFUL;
}
HMODULE GetModuleHandle(const char *name)
{
    if (strcmp(name,"KERNEL") != 0 || missing_kernel) return 0;
    return 1;
}
FARPROC GetProcAddress(HMODULE module,const char *name)
{
    if (module != 1) return (FARPROC)0;
    if (strcmp(name,"__A000H") == 0)
        return missing_aperture ? (FARPROC)0 : (FARPROC)(unsigned long)0x00070008UL;
    if (strcmp(name,"GetSelectorBase") == 0)
        return missing_base ? (FARPROC)0 : (FARPROC)fake_base;
    return (FARPROC)0;
}
DWORD GetWinFlags(void) { return 0x00000020UL; }
int MessageBox(HWND window,const char *message,const char *title,unsigned flags)
{
    (void)window;
    (void)title;
    if (strstr(message,"KERNEL __A000H")) {
        ++selector_shown;
        if (!strstr(message,"0007:0008") || !strstr(message,"000A0000")) return -9;
    }
    if (strstr(message,"INT 10h AH=0Fh reported")) {
        ++bios_shown;
        if (!strstr(message,"AL = 2F")) return -9;
    }
    if (flags & MB_YESNO) return option_yes ? IDYES : IDNO;
    return IDNO;
}
int int86(int interrupt_number, union REGS *in,union REGS *out)
{
    if (interrupt_number != 0x10 || in->x.ax != 0x0F00u) return -1;
    ++bios_reads;
    out->h.al=0x2F;
    out->h.ah=80;
    out->h.bh=0;
    return 0;
}
static int run(int yes)
{
    option_yes=yes;
    selector_shown=bios_shown=bios_reads=base_reads=0;
    if (WinMain(0,0,(char *)"",0) != 0) return 0;
    if (selector_shown != 1 || base_reads != 1) return 0;
    if (bios_shown != yes || bios_reads != yes) return 0;
    return 1;
}
int main(void)
{
    if (!run(0) || !run(1)) return 1;
    missing_base=1;
    selector_shown=bios_shown=bios_reads=base_reads=0;
    if (WinMain(0,0,(char *)"",0) != 0 || base_reads != 0) return 2;
    missing_base=0;
    missing_aperture=1;
    bios_reads=0;
    if (WinMain(0,0,(char *)"",0) != 2 || bios_reads) return 3;
    missing_aperture=0;
    missing_kernel=1;
    if (WinMain(0,0,(char *)"",0) != 1 || bios_reads) return 4;
    puts("PASS: PALDIAG reports selectors read-only; BIOS query opt-in; missing exports fail safely");
    return 0;
}
