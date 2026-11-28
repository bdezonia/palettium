/* v0082 task-context observer; optional deferred USER repaint.
 * The driver export reads one resident WORD without file/Windows calls. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define REPORT "C:\\PALWATCH.TXT"
#define TITLE "PALWATCH v0082"
#define TIMER_ID 1u
#define SIGNATURE 0x805au

typedef WORD (FAR PASCAL *ReadHook)(WORD);
static ReadHook read_hook;
static int failed;
static unsigned sequence;
static int repaint_enabled, have_sample, one_shot;
static int stopping, timer_running, repaint_queued;
static unsigned settle_ticks;
static WORD last_fg;
static int append(const char *line)
{
    HFILE f=_lopen(REPORT,OF_WRITE);
    unsigned n=(unsigned)strlen(line);
    int ok;
    if (f<0) return 0;
    ok=_llseek(f,0L,2)>=0L && _lwrite(f,line,n)==n;
    if (_lclose(f)<0) ok=0;
    return ok;
}
static int sample(void)
{
    char line[160];
    WORD v[6];
    unsigned i;
    if (read_hook(0xffffu)!=SIGNATURE) {
        (void)append("HOOK SIGNATURE LOST\r\n");
        return 0;
    }
    for (i=0;i<6u;++i) v[i]=read_hook((WORD)i);
    /* Keep heartbeat even when no events; timestamps identify gaps. */
    sprintf(line,"N=%04X TICK=%08lX BG=%04X FG=%04X SAVE=%04X RESTORE=%04X FOCUS=%04X QUERY=%04X\r\n",
            sequence++,(unsigned long)GetTickCount(),(unsigned)v[0],(unsigned)v[1],
            (unsigned)v[2],(unsigned)v[3],(unsigned)v[4],(unsigned)v[5]);
    if (!append(line)) return 0;
    /* No repaint on initial sample, background, or unchanged epoch.
     * Request invalidation in task context, not in the interrupt or DDI.
     * USER schedules painting via its normal message processing. */
    if (repaint_enabled && have_sample && v[1]!=last_fg && v[4]==2u) {
        if (!append("REPAINT REQUEST\r\n")) return 0;
        if (!RedrawWindow(GetDesktopWindow(),0,0,
             RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN)) {
            if (!append("REPAINT FAILED\r\n") || one_shot) return 0;
        } else {
            if (!append("REPAINT QUEUED\r\n")) return 0;
            repaint_queued=1;
            settle_ticks=0;
        }
    }
    /* Do not consume a foreground epoch if the independently read focus
     * still announces background. Retry at the next timer sample. */
    if (!have_sample || v[4]!=1u) last_fg=v[1];
    have_sample=1;
    return 1;
}
/* Disarm before destroying a window or allowing session teardown.
 * A queued WM_TIMER can still arrive after KillTimer: stopping prevents
 * all further driver-export calls, even on the error path. */
static void stop_timer(HWND hwnd)
{
    if (stopping) return;
    stopping=1;
    (void)append("TIMER STOP ENTER\r\n");
    if (timer_running) {
        timer_running=0;
        (void)append(KillTimer(hwnd,TIMER_ID) ?
                     "TIMER STOP OK\r\n" : "TIMER STOP FAILED\r\n");
    }
}
static void close_watch(HWND hwnd)
{
    (void)append("CLOSE ENTER\r\n");
    stop_timer(hwnd);
    (void)append("DESTROY WINDOW ENTER\r\n");
    if (!DestroyWindow(hwnd)) {
        (void)append("DESTROY WINDOW FAILED\r\n");
        failed=1;
        PostQuitMessage(0);
    } else (void)append("DESTROY WINDOW RETURN\r\n");
}
LONG FAR PASCAL __export watch_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    switch (msg) {
    case WM_TIMER:
        if (wp==TIMER_ID && !stopping) {
            if (!sample()) {
                failed=1;
                stop_timer(hwnd);
                if (one_shot) close_watch(hwnd);
                else SetWindowText(hwnd,"PALWATCH: logging stopped");
            } else if (one_shot && repaint_queued && ++settle_ticks>=3u) {
                (void)append("ONE SHOT COMPLETE\r\n");
                close_watch(hwnd);
            }
        }
        return 0;
    case WM_CLOSE:
        close_watch(hwnd);
        return 0;
    case WM_QUERYENDSESSION:
        /* Query alone is not a commitment; a canceled shutdown must
         * leave sampling operational. No driver call here. */
        (void)append("QUERY END SESSION\r\n");
        return 1;
    case WM_ENDSESSION:
        if (wp) {
            stop_timer(hwnd);
            (void)append("END SESSION TRUE\r\n");
            PostQuitMessage(0);
        } else (void)append("END SESSION CANCELED\r\n");
        return 0;
    case WM_DESTROY:
        (void)append("WM_DESTROY ENTER\r\n");
        stop_timer(hwnd);
        (void)append(failed ? "STOP AFTER FAILURE\r\n" : "STOP\r\n");
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}
int PASCAL WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR cmd,int show)
{
    static const char cls[]="PalWatch82";
    WNDCLASS wc;
    HWND hwnd;
    HMODULE module;
    HFILE f;
    MSG msg;
    char driver[32];
    int rc;
    (void)show;
    one_shot=cmd && !lstrcmpi(cmd,"/repaintonce");
    repaint_enabled=one_shot || (cmd && !lstrcmpi(cmd,"/repaint"));
    stopping=timer_running=repaint_queued=0;
    settle_ticks=0;
    failed=0; sequence=0;
    have_sample=0;
    last_fg=0;
    if (previous) return 1;
    GetPrivateProfileString("boot","display.drv","",driver,sizeof(driver),"SYSTEM.INI");
    if ((GetWinFlags() & 0x0031UL)!=0x0021UL || lstrcmpi(driver,"PALXHOOK.DRV")) {
        MessageBox(0,"Requires Enhanced mode and v0080 or newer PALXHOOK.DRV.",TITLE,MB_OK);
        return 1;
    }
    module=GetModuleHandle("DISPLAY");
    read_hook=module ? (ReadHook)GetProcAddress(module,"PALHOOKSTATUS") : 0;
    if (!read_hook) {
        MessageBox(0,"PALHOOKSTATUS not found. Install v0080 or newer PALXHOOK.DRV.",TITLE,MB_OK);
        return 2;
    }
    f=_lcreat(REPORT,0);
    if (f<0) return 3;
    if (_lclose(f)<0) return 3;
    if (!append("PALWATCH v0082; individually sampled WORD counters\r\nABOUT TO READ HOOK\r\n") || !sample()) {
        MessageBox(0,"Cannot read active hook or write report.",TITLE,MB_OK);
        return 3;
    }
    if (!append(repaint_enabled ? "REPAINT MODE ON\r\n" : "REPAINT MODE OFF\r\n")) return 3;
    if (one_shot && !append("ONE SHOT HIDDEN\r\n")) return 3;
    memset(&wc,0,sizeof(wc));
    wc.lpfnWndProc=watch_proc; wc.hInstance=instance;
    wc.hCursor=LoadCursor(0,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); wc.lpszClassName=cls;
    if (!RegisterClass(&wc)) return 4;
    hwnd=CreateWindow(cls,TITLE,WS_OVERLAPPEDWINDOW,0,0,260,80,0,0,instance,0);
    if (!hwnd) return 5;
    if (!SetTimer(hwnd,TIMER_ID,1000u,0)) {
        (void)append("TIMER START FAILED\r\n");
        DestroyWindow(hwnd); return 6;
    }
    timer_running=1;
    if (!one_shot) ShowWindow(hwnd,SW_SHOWNORMAL);
    while ((rc=GetMessage(&msg,0,0,0))>0) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    stop_timer(hwnd);
    (void)append("MESSAGE LOOP EXIT\r\n");
    return rc<0 || failed ? 7 : 0;
}
