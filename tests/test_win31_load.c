/* Tests the actual PALLOAD Win16 GUI code with simulated Windows APIs
 * and a stand-in for PALSTAGE's ABI. No graphics hardware touched. */
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "pal_gdiinfo.h"

int WinMain(HINSTANCE,HINSTANCE,LPSTR,int);

static int stop_after, answers, load_calls, free_calls, lookups;
static int enable_calls, inquire_calls, ini_calls, bad_calls, bad_guard;
static int load_error, export_missing, inquire_bad, enable_bad;
static int corrupt_inquire_guard;
static int configured_kib, reported_pass;

static WORD FAR PASCAL stub_enable(void *dest,WORD style,LPSTR type,
                                   LPSTR output,void *data)
{
    PAL_GDIINFO info;
    (void)type; (void)output; (void)data;
    ++enable_calls;
    if (style!=1u) { ++bad_calls; return 0; }
    if (configured_kib==0 || enable_bad) return 0;
    if (!pal_win31_make_candidate(&info,640u,
              configured_kib==256 ? 400u : 480u,20u)) return 0;
    memcpy(dest,&info,PAL_GDIINFO_BYTES);
    if (bad_guard) ((unsigned char *)dest)[PAL_GDIINFO_BYTES]=0;
    return PAL_GDIINFO_BYTES;
}
static WORD FAR PASCAL stub_inquire(void *buf)
{
    unsigned char *p=(unsigned char *)buf;
    ++inquire_calls;
    p[0]=1; p[1]=0; p[2]=2; p[3]=0;
    if (inquire_bad) p[0]=0;
    if (corrupt_inquire_guard) p[4]=0;
    return 4;
}
static void stub_unused(void) {}

HINSTANCE LoadLibrary(const char *path)
{
    ++load_calls;
    if (strcmp(path,"C:\\PALTEST\\PALSTAGE.DRV")!=0) ++bad_calls;
    return load_error ? 2 : 400;
}
void FreeLibrary(HINSTANCE module)
{
    ++free_calls;
    if (module!=400) ++bad_calls;
}
FARPROC GetProcAddress(HMODULE module,const char *name)
{
    ++lookups;
    if (module!=400) ++bad_calls;
    if (export_missing && strcmp(name,"CHECKCURSOR")==0) return (FARPROC)0;
    if (strcmp(name,"ENABLE")==0) return (FARPROC)stub_enable;
    if (strcmp(name,"INQUIRE")==0) return (FARPROC)stub_inquire;
    return (FARPROC)stub_unused;
}
int GetPrivateProfileInt(const char *section,const char *key,
                         int fallback,const char *filename)
{
    ++ini_calls;
    if (strcmp(section,"Palettium") || strcmp(key,"VRAMKiB") ||
        strcmp(filename,"SYSTEM.INI") || fallback) ++bad_calls;
    return configured_kib;
}
int MessageBox(HWND window,const char *message,const char *title,
               unsigned flags)
{
    (void)window;
    if (strcmp(title,"PALLOAD v0043")) ++bad_calls;
    if (strstr(message,"STEP 4 PASS")) ++reported_pass;
    if (flags & MB_YESNO) return (++answers<=stop_after) ? IDYES : IDNO;
    return 0;
}
static void reset(void)
{
    stop_after=4; answers=load_calls=free_calls=lookups=enable_calls=0;
    inquire_calls=ini_calls=bad_calls=reported_pass=0;
    load_error=export_missing=inquire_bad=enable_bad=bad_guard=0;
    corrupt_inquire_guard=0;
    configured_kib=256;
}
static int run(int allowed,int expected,int expected_load,int expected_free,
               int expected_inquire,int expected_enable,int expected_pass)
{
    int ret,expected_lookups=0;
    stop_after=allowed;
    if (allowed>=2 && !load_error) {
        expected_lookups=19;
        if (allowed>=3 && !export_missing) ++expected_lookups;
        if (allowed>=4 && !export_missing && !inquire_bad &&
            !corrupt_inquire_guard) ++expected_lookups;
    }
    ret=WinMain(0,0,(LPSTR)"",0);
    if (lookups!=expected_lookups || ret!=expected || load_calls!=expected_load ||
        free_calls!=expected_free || inquire_calls!=expected_inquire ||
        enable_calls!=expected_enable || bad_calls ||
        reported_pass!=expected_pass ||
        (enable_calls!=0 && ini_calls!=1)) {
        fprintf(stderr,"PALLOAD FAIL: ret %d want %d load/free %d/%d inquire/enable %d/%d bad %d pass %d\n",
                ret,expected,load_calls,free_calls,inquire_calls,enable_calls,
                bad_calls,reported_pass);
        return 0;
    }
    return 1;
}
int main(void)
{
    reset(); if (!run(0,0,0,0,0,0,0)) return 1;
    reset(); load_error=1;
    if (!run(1,1,1,0,0,0,0)) return 2;
    reset(); if (!run(1,0,1,1,0,0,0)) return 3;
    reset(); export_missing=1;
    if (!run(2,2,1,1,0,0,0)) return 4;
    reset(); if (!run(2,0,1,1,0,0,0)) return 5;
    reset(); inquire_bad=1;
    if (!run(3,3,1,1,1,0,0)) return 6;
    reset(); if (!run(3,0,1,1,1,0,0)) return 7;
    reset(); corrupt_inquire_guard=1;
    if (!run(3,3,1,1,1,0,0)) return 14;
    reset(); configured_kib=0;
    if (!run(4,0,1,1,1,1,0)) return 8;
    reset(); if (!run(4,0,1,1,1,1,1)) return 9;
    reset(); configured_kib=1024;
    if (!run(4,0,1,1,1,1,1)) return 10;
    reset(); configured_kib=512;
    if (!run(4,0,1,1,1,1,1)) return 11;
    reset(); enable_bad=1;
    if (!run(4,4,1,1,1,1,0)) return 12;
    reset(); bad_guard=1;
    if (!run(4,4,1,1,1,1,0)) return 13;
    puts("PASS: actual Win16 PALLOAD opt-in loader, 19 exports, cursor Inquire, GDIINFO, guard/failure paths; never style=0");
    return 0;
}
