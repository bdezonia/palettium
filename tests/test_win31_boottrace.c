/* Host-only Win16 KERNEL trace mock, no real DOS file I/O. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "pal_boottrace.h"
static char output[1024];
static unsigned length,opens,closes,writes;
static int active;
HFILE _lcreat(const char *filename,int attrs)
{
    assert(!strcmp(filename,"C:\\PALBOOT.TXT") && attrs==0);
    assert(!active);
    ++opens; length=0; output[0]=0; active=1;
    return 7;
}
HFILE _lopen(const char *filename,int flags)
{
    assert(!strcmp(filename,"C:\\PALBOOT.TXT") && flags==OF_WRITE);
    assert(!active);
    ++opens; active=1;
    return 7;
}
long _llseek(HFILE file,long offset,int origin)
{
    assert(file==7 && active && offset==0L && origin==2);
    return (long)length;
}
unsigned _lwrite(HFILE file,const char *source,unsigned count)
{
    assert(file==7 && active && length+count<sizeof(output));
    memcpy(output+length,source,count); length+=count;
    output[length]=0; ++writes;
    return count;
}
int _lclose(HFILE file)
{
    assert(file==7 && active);active=0;++closes;return 0;
}
int main(void)
{
    pal_boottrace("LIBMAIN OK");
    pal_boottrace("ENABLE1 ENTER");
    pal_boottrace("ENABLE1 OK");
    assert(!strcmp(output,"LIBMAIN OK\r\nENABLE1 ENTER\r\nENABLE1 OK\r\n"));
    assert(opens==3 && closes==3 && writes==6 && !active);
    pal_boottrace_hex("BIOS REQUEST",0x002fu);
    assert(!strcmp(output,"LIBMAIN OK\r\nENABLE1 ENTER\r\nENABLE1 OK\r\nBIOS REQUEST=002F\r\n"));
    assert(opens==4 && closes==4 && writes==8 && !active);
    puts("PASS: Win16 trial-only KERNEL boottrace create/append/close markers");
    return 0;
}
