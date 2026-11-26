/* PALENH v0075: independent Win16 application, not a display driver.
 * Capture display/grabber/VDD configuration and look up the protected-mode
 * VDD API address. Never call the returned entry point, change video mode,
 * access the aperture, touch ports, or authorize Palettium activation.
 * The operator should use a recoverable VM running its known-good donor. */
#include <windows.h>
#include <i86.h>
#include <stdio.h>
#include <string.h>

#define REPORT_PATH "C:\\PALENH.TXT"
#define TITLE "Palettium PALENH v0075"

static int put(HFILE f, const char *s)
{
    unsigned n=(unsigned)strlen(s);
    return _lwrite(f,s,n)==n;
}
static int profile(HFILE f,const char *section,const char *key)
{
    char value[128],line[200];
    unsigned n;
    value[0]=0;
    n=(unsigned)GetPrivateProfileString(section,key,"",value,
                                         sizeof(value),"SYSTEM.INI");
    value[sizeof(value)-1u]=0;
    sprintf(line,"[%s] %s=%s%s\r\n",section,key,
            value[0] ? value : "(missing/empty)",
            n>=sizeof(value)-1u ? " [TRUNCATED]" : "");
    return put(f,line);
}
static int close_report(HFILE f,int ok)
{
    if (_lclose(f)<0) ok=0;
    if (!ok) MessageBox(0,"Incomplete report; disk write/close failed.",TITLE,MB_OK);
    return ok;
}
static const char *entry_status(unsigned es,unsigned di)
{
    if (es) return "ADDRESS_RETURNED_NOT_VALIDATED";
    if (di) return "NO_PROTECTED_MODE_ENTRY_ZERO_SELECTOR";
    return "NO_PROTECTED_MODE_ENTRY";
}
int PASCAL WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR cmd,int show)
{
    DWORD flags;
    HFILE f;
    char line[200];
    int eligible,query,ok;
    union REGS in,out;
    struct SREGS seg;
    (void)instance; (void)previous; (void)cmd; (void)show;
    if (MessageBox(0,"Use a disposable VM with its working display driver.\n"
                  "Create/replace C:\\PALENH.TXT with configuration and mode flags?\n"
                  "No video hardware will be accessed.",TITLE,
                  MB_YESNO|MB_ICONQUESTION)!=IDYES) return 0;
    flags=GetWinFlags();
    /* Require protected mode + Enhanced mode, not contradictory Standard. */
    eligible=(flags & 0x0031UL)==0x0021UL;
    f=_lcreat(REPORT_PATH,0);
    if (f<0) { MessageBox(0,"Cannot create " REPORT_PATH,TITLE,MB_OK); return 1; }
    sprintf(line,"PALENH v0075\r\nGetWinFlags=%08lX\r\n",(unsigned long)flags);
    ok=put(f,line) && profile(f,"boot","display.drv") &&
        profile(f,"boot","286grabber") && profile(f,"boot","386grabber") &&
        profile(f,"386Enh","display") &&
        put(f,"INI names are configuration only, not loaded VDD identity.\r\n");
    if (!ok) { (void)close_report(f,0); return 2; }
    query=eligible && MessageBox(0,"Query INT 2Fh AX=1684h BX=000Ah for the VDD API address?\n"
                          "The returned address will NOT be called or dereferenced.\n"
                          "Use the VM snapshot if the interrupt faults.",TITLE,
                          MB_YESNO|MB_ICONQUESTION)==IDYES;
    ok=put(f,!eligible ? "API_QUERY=SKIPPED_NOT_ENHANCED_PROTECTED\r\n" :
               !query ? "API_QUERY=SKIPPED_BY_OPERATOR\r\n" :
                        "API_QUERY=ABOUT_TO_CALL_INT2F_1684_000A\r\n");
    /* Persist the pre-interrupt checkpoint, even if the interrupt faults. */
    if (!close_report(f,ok)) return 2;
    if (query) {
        memset(&in,0,sizeof(in)); memset(&out,0,sizeof(out));
        segread(&seg);
        in.w.ax=0x1684u; in.w.bx=0x000Au; in.w.di=0;
        seg.es=0;
        (void)int86x(0x2F,&in,&out,&seg);
        /* Zero ES is never callable; nonzero ES is not proof of safety. */
        f=_lopen(REPORT_PATH,OF_WRITE);
        if (f<0) { MessageBox(0,"Cannot append to " REPORT_PATH,TITLE,MB_OK); return 3; }
        ok=_llseek(f,0L,2)>=0L;
        sprintf(line,"VDD_API_ES_DI=%04X:%04X\r\nVDD_API_STATUS=%s\r\n",
                (unsigned)seg.es,(unsigned)out.w.di,
                entry_status(seg.es,out.w.di));
        ok=ok && put(f,line) && put(f,"Returned address not invoked. No activation permission.\r\n");
        if (!close_report(f,ok)) return 3;
    }
    MessageBox(0,"Report saved to " REPORT_PATH ".\nNo video mode, register or VRAM changes were requested.",TITLE,MB_OK);
    return 0;
}
