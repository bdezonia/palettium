/* Temporary boot-phase recorder for the separately authorized PALTRIAL.
 * Uses only KERNEL _l* file APIs: no USER/GDI imports, no video writes.
 * The record C:\PALBOOT.TXT can be read from DOS after failed WIN /B /S.
 * Failure to write is nonfatal; a missing file does not prove a phase was
 * skipped. Every marker is opened/written/closed to survive aborts.
 * Not suitable as a production early-boot logging architecture.
 */
#include "pal_boottrace.h"
#if defined(PAL_WIN31_BOOTTRACE)
#include <windows.h>
static int pal_trace_started;
void pal_boottrace(const char *event)
{
    HFILE file;
    unsigned length=0;
    if (!event) return;
    if (!pal_trace_started) {
        pal_trace_started=1;
        file=_lcreat("C:\\PALBOOT.TXT",0);
    } else {
        file=_lopen("C:\\PALBOOT.TXT",OF_WRITE);
        if (file>=0) (void)_llseek(file,0L,2);
    }
    if (file<0) return;
    while (length<80u && event[length]) ++length;
    if (length) (void)_lwrite(file,event,length);
    (void)_lwrite(file,"\r\n",2u);
    (void)_lclose(file);
}
#endif
