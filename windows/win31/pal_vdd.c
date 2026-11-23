/* Windows 3.x VDD configuration classification; portable C89.
 * Microsoft KB80901 documents that the Windows 3.1 standard VGA VDD
 * cannot support SVGA drivers using >256 KiB. OEM/legacy settings need
 * independent validation; we NEVER return "compatible" from an INI name.
 */
#include <ctype.h>
#include <string.h>
#include "pal_vdd.h"

static int equals_nocase(const char *a, const char *b)
{
    unsigned char ca,cb;
    while (*a && *b) {
        ca=(unsigned char)*a++;
        cb=(unsigned char)*b++;
        if (tolower(ca)!=tolower(cb)) return 0;
    }
    return *a==0 && *b==0;
}

int pal_vdd_kind(const char *configured_display)
{
    const char *begin,*end,*part;
    char value[96];
    unsigned len,i;
    if (!configured_display) return PAL_VDD_UNKNOWN;
    begin=configured_display;
    while (*begin==' ' || *begin=='\t') ++begin;
    end=begin+strlen(begin);
    while (end>begin && (end[-1]==' ' || end[-1]=='\t')) --end;
    if (end==begin || (unsigned long)(end-begin)>=sizeof(value))
        return PAL_VDD_UNKNOWN;
    /* SYSTEM.INI permits filename paths for an OEM VDD; only the exact
     * filename in the final component has classification significance. */
    part=begin;
    for (i=0u;i<(unsigned)(end-begin);++i)
        if (begin[i]=='\\' || begin[i]=='/') part=begin+i+1;
    len=(unsigned)(end-part);
    if (!len || len>=sizeof(value)) return PAL_VDD_UNKNOWN;
    memcpy(value,part,len);
    value[len]=0;
    if (equals_nocase(value,"*vddvga") ||
        equals_nocase(value,"vddvga.386")) return PAL_VDD_STOCK_VGA31;
    if (equals_nocase(value,"vddvga30.386")) return PAL_VDD_VGA30;
    return PAL_VDD_OTHER;
}

int pal_vdd_preflight(unsigned long windows_flags,
                      const char *configured_display,
                      unsigned long framebuffer_bytes)
{
    /* WF_ENHANCED = 0x0020; protected by Windows' video virtualization.
     * In Standard mode, do not infer any VDD configuration requirements. */
    if ((windows_flags & 0x0020UL)==0UL) return PAL_VDD_NOT_ENHANCED;
    if (pal_vdd_kind(configured_display)==PAL_VDD_STOCK_VGA31 &&
        framebuffer_bytes>262144UL) return PAL_VDD_KNOWN_INCOMPATIBLE;
    return PAL_VDD_UNVERIFIED;
}

const char *pal_vdd_kind_name(int kind)
{
    switch (kind) {
    case PAL_VDD_STOCK_VGA31: return "stock Windows 3.1 VGA VDD";
    case PAL_VDD_VGA30: return "Windows 3.0-compatible VGA VDD";
    case PAL_VDD_OTHER: return "other/custom (not validated)";
    default: return "missing/unknown";
    }
}
