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
typedef unsigned short UINT;
typedef unsigned long DWORD;
typedef int HMODULE;
typedef int HINSTANCE;
typedef int HWND;
typedef int HFILE;
#define OF_WRITE 1u
HFILE _lcreat(const char *filename,int attrs);
HFILE _lopen(const char *filename,int flags);
long _llseek(HFILE file,long offset,int origin);
unsigned _lwrite(HFILE file,const char *source,unsigned count);
int _lclose(HFILE file);
typedef char *LPSTR;
typedef void *LPVOID;
typedef void (*FARPROC)(void);
HINSTANCE LoadLibrary(const char *path);
void FreeLibrary(HINSTANCE module);
HMODULE GetModuleHandle(const char *name);
FARPROC GetProcAddress(HMODULE module,const char *name);
DWORD GetWinFlags(void);
int GetPrivateProfileInt(const char *section,const char *key,
                         int fallback,const char *filename);
int GetPrivateProfileString(const char *section,const char *key,
                            const char *fallback,char *buffer,int size,
                            const char *filename);
int MessageBox(HWND window,const char *message,const char *title,unsigned flags);
#endif
