/* Host simulation of the independent task: no Win16 ABI claims. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "win16_stubs/windows.h"
#define __export
#define WM_TIMER 0x113
#define WM_DESTROY 2
#define WM_CLOSE 0x10
#define WM_QUERYENDSESSION 0x11
#define WM_ENDSESSION 0x16
#define WS_OVERLAPPEDWINDOW 0
#define SW_SHOWNORMAL 1
#define IDC_ARROW 32512
#define COLOR_WINDOW 5
#define RDW_INVALIDATE 1u
#define RDW_ERASE 4u
#define RDW_FRAME 1024u
#define RDW_ALLCHILDREN 128u
static HWND GetDesktopWindow(void);
static int RedrawWindow(HWND,void *,int,unsigned);
#define lstrcmpi strcmp
typedef long LONG, LPARAM;
typedef unsigned WPARAM;
typedef int HBRUSH;
typedef LONG (*WNDPROC)(HWND,UINT,WPARAM,LPARAM);
typedef struct { WNDPROC lpfnWndProc; HINSTANCE hInstance; int hCursor; HBRUSH hbrBackground; const char *lpszClassName; } WNDCLASS;
typedef struct { unsigned message; } MSG;
static int LoadCursor(int a,int b);
static int RegisterClass(WNDCLASS *wc);
static HWND CreateWindow(const char *cls,const char *title,unsigned style,int x,int y,int w,int h,HWND parent,int menu,HINSTANCE inst,void *p);
static int SetTimer(HWND h,unsigned id,unsigned interval,void *cb);
static int KillTimer(HWND h,unsigned id);
static int SetWindowText(HWND h,const char *s);
static int DestroyWindow(HWND h);
static void PostQuitMessage(int n);
static LONG DefWindowProc(HWND h,UINT m,WPARAM w,LPARAM l);
static void ShowWindow(HWND h,int n);
static int GetMessage(MSG *m,HWND h,unsigned lo,unsigned hi);
static void TranslateMessage(MSG *m);
static void DispatchMessage(MSG *m);
static DWORD GetTickCount(void);
#include "../windows/win31/palwatch.c"
static char report[4096];
static unsigned size,step,closes,reads,repaints,kills,shows,destroys,quits;
static int scenario;
static int mode=1,active=1,missing,write_failure,timer_failure,opened,repaint_failure;
static WNDPROC proc;
static WORD fake_read(WORD i)
{
    ++reads;
    if (i==0xffffu) return active ? 0x805au : 0xffffu;
    if (i==4) return step>1u ? 2u : 0u;
    return (WORD)((i==0 || i==1) && step>1u ? 1u : 0u);
}
DWORD GetWinFlags(void) { return mode ? 0xc29ul : 0x11ul; }
int GetPrivateProfileString(const char *s,const char *k,const char *d,char *b,int n,const char *f)
{ (void)s;(void)k;(void)d;(void)n;(void)f; strcpy(b,"PALXHOOK.DRV"); return 12; }
HMODULE GetModuleHandle(const char *s) { assert(!strcmp(s,"DISPLAY")); return 1; }
FARPROC GetProcAddress(HMODULE h,const char *s)
{ union { FARPROC p; ReadHook r; } u; assert(h==1 && !strcmp(s,"PALHOOKSTATUS")); u.r=fake_read; return missing ? 0 : u.p; }
int MessageBox(HWND h,const char *m,const char *t,unsigned flags) { (void)h;(void)m;(void)t;(void)flags;return 0; }
HFILE _lcreat(const char *s,int a) { (void)a; assert(!strcmp(s,REPORT)); assert(!opened); opened=1; size=0; report[0]=0; return 1; }
HFILE _lopen(const char *s,int flags) { assert(!strcmp(s,REPORT)&&flags==OF_WRITE&&!opened);opened=1;return 1; }
long _llseek(HFILE f,long n,int origin) { assert(f==1 && n==0 && origin==2 && opened);return size; }
unsigned _lwrite(HFILE f,const char *s,unsigned n) { assert(f==1&&opened); if (write_failure) return 0;assert(size+n<sizeof(report));memcpy(report+size,s,n);size+=n;report[size]=0;return n; }
int _lclose(HFILE f) { assert(f==1&&opened);opened=0;++closes;return 0; }
static int LoadCursor(int a,int b) { (void)a;(void)b;return 1; }
static int RegisterClass(WNDCLASS *wc) { proc=wc->lpfnWndProc;return 1; }
static HWND CreateWindow(const char *c,const char *t,unsigned s,int x,int y,int w,int h,HWND p,int m,HINSTANCE i,void *z)
{ (void)c;(void)t;(void)s;(void)x;(void)y;(void)w;(void)h;(void)p;(void)m;(void)i;(void)z;return 1; }
static int SetTimer(HWND h,unsigned id,unsigned interval,void *cb) { assert(h==1&&id==1&&interval==1000&&!cb);return !timer_failure; }
static int KillTimer(HWND h,unsigned id) { (void)h;(void)id;++kills;return 1; }
static int SetWindowText(HWND h,const char *s) { (void)h;(void)s;return 1; }
static int DestroyWindow(HWND h) { unsigned before=reads; ++destroys; if (!timer_failure) assert(kills==1); proc(h,WM_DESTROY,0,0); proc(h,WM_TIMER,1,0); assert(reads==before); return 1; }
static void PostQuitMessage(int n) { (void)n;++quits; }
static LONG DefWindowProc(HWND h,UINT m,WPARAM w,LPARAM l) { (void)h;(void)m;(void)w;(void)l;return 0; }
static void ShowWindow(HWND h,int n) { (void)h;(void)n;++shows; }
static int GetMessage(MSG *m,HWND h,unsigned lo,unsigned hi)
{
    (void)h;(void)lo;(void)hi;
    if (quits) return 0;
    ++step;
    assert(step<12u);
    if (scenario==1) m->message=WM_TIMER;
    else if (scenario==2) m->message=step==1 ? WM_QUERYENDSESSION : step==2 ? WM_ENDSESSION : WM_TIMER;
    else if (scenario==3) m->message=step==1 ? WM_QUERYENDSESSION : step==2 ? WM_ENDSESSION : step==3 ? WM_TIMER : WM_CLOSE;
    else m->message=step==3 ? WM_CLOSE : WM_TIMER;
    return 1;
}
static void TranslateMessage(MSG *m) { (void)m; }
static void DispatchMessage(MSG *m)
{
    unsigned before=reads;
    LONG result=proc(1,(UINT)m->message,scenario==3 && m->message==WM_ENDSESSION ? 0 : 1,0);
    if (m->message==WM_QUERYENDSESSION) assert(result==1 && reads==before && kills==0);
    if (m->message==WM_ENDSESSION && scenario==2) {
        proc(1,WM_TIMER,1,0); assert(reads==before && kills==1);
    }
}
static DWORD GetTickCount(void) { return 1000ul*step; }
static HWND GetDesktopWindow(void) { return 2; }
static int RedrawWindow(HWND h,void *r,int region,unsigned flags)
{ assert(h==2&&!r&&!region&&flags==(RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN)); ++repaints; return !repaint_failure; }
static void reset(void) { size=step=closes=reads=sequence=repaints=kills=shows=destroys=quits=0;scenario=0;repaint_failure=0;failed=0; mode=active=1; missing=write_failure=timer_failure=opened=0;report[0]=0; }
int main(void)
{
    reset();assert(WinMain(1,0,"",0)==0);assert(!opened&&reads==21&&repaints==0&&kills==1&&destroys==1);
    assert(strstr(report,"ABOUT TO READ HOOK\r\nN=0000"));assert(strstr(report,"BG=0001 FG=0001"));assert(strstr(report,"STOP\r\n"));
    reset();assert(WinMain(1,0,"/repaint",0)==0);assert(repaints==1&&strstr(report,"REPAINT QUEUED"));
    reset();repaint_failure=1;assert(WinMain(1,0,"/repaint",0)==0);assert(repaints==1&&strstr(report,"REPAINT FAILED"));
    reset();scenario=1;assert(WinMain(1,0,"/repaintonce",0)==0);
    assert(repaints==1&&shows==0&&kills==1&&destroys==1&&step==4);
    assert(strstr(report,"ONE SHOT COMPLETE")&&strstr(report,"DESTROY WINDOW RETURN")&&strstr(report,"MESSAGE LOOP EXIT"));
    reset();scenario=1;repaint_failure=1;assert(WinMain(1,0,"/repaintonce",0)==7);
    assert(shows==0&&kills==1&&destroys==1&&strstr(report,"STOP AFTER FAILURE"));
    reset();scenario=2;assert(WinMain(1,0,"",0)==0);
    assert(kills==1&&reads==7&&strstr(report,"END SESSION TRUE"));
    reset();scenario=3;assert(WinMain(1,0,"",0)==0);
    assert(kills==1&&reads==14&&strstr(report,"END SESSION CANCELED")&&strstr(report,"STOP\r\n"));
    reset();mode=0;assert(WinMain(1,0,"",0)==1&&reads==0&&closes==0);
    reset();missing=1;assert(WinMain(1,0,"",0)==2&&reads==0&&closes==0);
    reset();active=0;assert(WinMain(1,0,"",0)==3&&reads==1&&strstr(report,"HOOK SIGNATURE LOST"));
    reset();write_failure=1;assert(WinMain(1,0,"",0)==3&&reads==0&&!opened);
    reset();timer_failure=1;assert(WinMain(1,0,"",0)==6&&strstr(report,"TIMER START FAILED"));
    puts("PASS: watcher simulation, repaint, hidden one-shot, close ordering, late timer, session end/cancel, I/O and timer failure");return 0;
}
