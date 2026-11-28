/* Host simulation of the independent task: no Win16 ABI claims. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "win16_stubs/windows.h"
#define __export
#define WM_TIMER 0x113
#define WM_DESTROY 2
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
static unsigned size,step,closes,reads,repaints;
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
static int KillTimer(HWND h,unsigned id) { (void)h;(void)id;return 1; }
static int SetWindowText(HWND h,const char *s) { (void)h;(void)s;return 1; }
static int DestroyWindow(HWND h) { proc(h,WM_DESTROY,0,0);return 1; }
static void PostQuitMessage(int n) { (void)n; }
static LONG DefWindowProc(HWND h,UINT m,WPARAM w,LPARAM l) { (void)h;(void)m;(void)w;(void)l;return 0; }
static void ShowWindow(HWND h,int n) { (void)h;(void)n; }
static int GetMessage(MSG *m,HWND h,unsigned lo,unsigned hi) { (void)h;(void)lo;(void)hi; ++step;if(step>3) return 0;m->message=step==3 ? WM_DESTROY : WM_TIMER;return 1; }
static void TranslateMessage(MSG *m) { (void)m; }
static void DispatchMessage(MSG *m) { proc(1,(UINT)m->message,1,0); }
static DWORD GetTickCount(void) { return 1000ul*step; }
static HWND GetDesktopWindow(void) { return 2; }
static int RedrawWindow(HWND h,void *r,int region,unsigned flags)
{ assert(h==2&&!r&&!region&&flags==(RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN)); ++repaints; return !repaint_failure; }
static void reset(void) { size=step=closes=reads=sequence=repaints=0;repaint_failure=0;failed=0; mode=active=1; missing=write_failure=timer_failure=opened=0;report[0]=0; }
int main(void)
{
    reset();assert(WinMain(1,0,"",0)==0);assert(!opened&&closes==7&&reads==21&&repaints==0);
    assert(strstr(report,"ABOUT TO READ HOOK\r\nN=0000"));assert(strstr(report,"BG=0001 FG=0001"));assert(strstr(report,"STOP\r\n"));
    reset();assert(WinMain(1,0,"/repaint",0)==0);assert(repaints==1&&strstr(report,"REPAINT QUEUED"));
    reset();repaint_failure=1;assert(WinMain(1,0,"/repaint",0)==0);assert(repaints==1&&strstr(report,"REPAINT FAILED"));
    reset();mode=0;assert(WinMain(1,0,"",0)==1&&reads==0&&closes==0);
    reset();missing=1;assert(WinMain(1,0,"",0)==2&&reads==0&&closes==0);
    reset();active=0;assert(WinMain(1,0,"",0)==3&&reads==1&&strstr(report,"HOOK SIGNATURE LOST"));
    reset();write_failure=1;assert(WinMain(1,0,"",0)==3&&reads==0&&!opened);
    reset();timer_failure=1;assert(WinMain(1,0,"",0)==6&&strstr(report,"TIMER START FAILED"));
    puts("PASS: live watcher task simulation, persisted heartbeat, missing export, inactive hook, I/O and timer failure");return 0;
}
