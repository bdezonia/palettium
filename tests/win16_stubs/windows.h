/* Host-only minimal Win16 API stubs for PALDIAG executable regression. */
#ifndef PALDIAG_TEST_WINDOWS_H
#define PALDIAG_TEST_WINDOWS_H
#define FAR
#define PASCAL
#define MB_OK 0x0000
#define MB_ICONSTOP 0x0010
#define MB_ICONINFORMATION 0x0040
#define MB_ICONQUESTION 0x0020
#define MB_YESNO 0x0004
#define IDYES 6
#define IDNO 7
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int HMODULE;
typedef int HINSTANCE;
typedef int HWND;
typedef char *LPSTR;
typedef void (*FARPROC)(void);
HMODULE GetModuleHandle(const char *name);
FARPROC GetProcAddress(HMODULE module,const char *name);
DWORD GetWinFlags(void);
int MessageBox(HWND window,const char *message,const char *title,unsigned flags);
#endif
