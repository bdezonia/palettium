#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "i86.h"
int WinMain(HINSTANCE,HINSTANCE,LPSTR,int);
static char report[1024];
static unsigned pos,flags_calls,profile_calls,query_calls,closed,opened,consent;
static unsigned short ret_es,ret_di;
static DWORD flags;
static int allow_query,fail_write;
DWORD GetWinFlags(void) { ++flags_calls; return flags; }
int MessageBox(HWND w,const char *s,const char *title,unsigned options)
{
    (void)w; (void)title;
    if (!(options & MB_YESNO)) return IDNO;
    if (strstr(s,"Create/replace")) return consent ? IDYES : IDNO;
    assert(strstr(s,"1684h"));
    return allow_query ? IDYES : IDNO;
}
int GetPrivateProfileString(const char *section,const char *key,const char *fallback,
                            char *buf,int size,const char *filename)
{
    const char *value;
    (void)fallback;
    ++profile_calls;
    assert(!strcmp(filename,"SYSTEM.INI"));
    if (!strcmp(section,"boot")) {
        if (!strcmp(key,"display.drv")) value="SVGA256.DRV";
        else if (!strcmp(key,"286grabber")) value="VGACOLOR.2GR";
        else { assert(!strcmp(key,"386grabber")); value="VGADIB.3GR"; }
    } else { assert(!strcmp(section,"386Enh")&&!strcmp(key,"display")); value="VDDSVGA.386"; }
    assert(strlen(value)<(size_t)size);
    strcpy(buf,value); return (int)strlen(value);
}
HFILE _lcreat(const char *name,int attrs)
{
    assert(!strcmp(name,"C:\\PALENH.TXT") && attrs==0 && !opened);
    opened=1; pos=0; report[0]=0; return 1;
}
HFILE _lopen(const char *name,int mode)
{
    assert(!strcmp(name,"C:\\PALENH.TXT") && mode==OF_WRITE && !opened && closed==1);
    opened=1; pos=0; return 1;
}
long _llseek(HFILE f,long off,int origin)
{
    assert(f==1 && opened && !off && origin==2);
    pos=(unsigned)strlen(report); return (long)pos;
}
unsigned _lwrite(HFILE f,const char *buf,unsigned count)
{
    assert(f==1 && opened && pos+count<sizeof(report));
    if (fail_write) return 0;
    memcpy(report+pos,buf,count); pos+=count; report[pos]=0; return count;
}
int _lclose(HFILE f) { assert(f==1 && opened); opened=0; ++closed; return 0; }
void segread(struct SREGS *s) { s->es=0x1234; s->ds=0x4321; s->ss=0x4321; s->cs=0x2345; }
int int86x(int number,const union REGS *in,union REGS *out,struct SREGS *s)
{
    ++query_calls;
    assert(number==0x2F && in->w.ax==0x1684 && in->w.bx==0xA &&
           in->w.di==0 && s->es==0 && s->ds==0x4321);
    assert(closed==1 && !opened && strstr(report,"ABOUT_TO_CALL_INT2F"));
    out->w.di=ret_di; s->es=ret_es; return 0;
}
static void reset(DWORD f,unsigned yes,unsigned query,unsigned es,unsigned di)
{
    flags=f; consent=yes; allow_query=(int)query; ret_es=(unsigned short)es;
    ret_di=(unsigned short)di; pos=flags_calls=profile_calls=query_calls=closed=opened=0;
    fail_write=0; report[0]=0;
}
static void run(DWORD f,unsigned yes,unsigned query,unsigned es,unsigned di,
                unsigned expected_queries,const char *expected)
{
    reset(f,yes,query,es,di);
    assert(WinMain(0,0,(char *)"",0)==0 && !opened);
    assert(flags_calls==yes && profile_calls==4u*yes && query_calls==expected_queries);
    assert(closed==yes+expected_queries);
    assert(strstr(report,expected));
    if (yes) assert(strstr(report,"[386Enh] display=VDDSVGA.386") &&
                    strstr(report,"[boot] 386grabber=VGADIB.3GR"));
}
int main(void)
{
    run(0xC29UL,0,1,1,0,0,"");
    run(0x419UL,1,1,1,0,0,"SKIPPED_NOT_ENHANCED_PROTECTED");
    run(0x31UL,1,1,1,0,0,"SKIPPED_NOT_ENHANCED_PROTECTED");
    run(0x20UL,1,1,1,0,0,"SKIPPED_NOT_ENHANCED_PROTECTED");
    run(0xC29UL,1,0,1,0,0,"SKIPPED_BY_OPERATOR");
    run(0xC29UL,1,1,0,0,1,"VDD_API_STATUS=NO_PROTECTED_MODE_ENTRY\r\n");
    run(0xC29UL,1,1,0,0x1234,1,"NO_PROTECTED_MODE_ENTRY_ZERO_SELECTOR");
    run(0xC29UL,1,1,0x47,0,1,"ADDRESS_RETURNED_NOT_VALIDATED");
    reset(0xC29UL,1,1,0x47,0x10); fail_write=1;
    assert(WinMain(0,0,(char *)"",0)==2 && !opened && !query_calls);
    puts("PASS: PALENH configuration, explicit opt-in, protected Enhanced gate, saved checkpoint, raw API return; no API invocation");
    return 0;
}
