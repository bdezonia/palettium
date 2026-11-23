/* Host regression for the ACTUAL Win16 PALDIAG source, including the
 * opt-in one-byte A000 read.  No real video memory or ports are accessed. */
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "dos.h"
#include "pal_vdd.h"

int WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR line,int command);

static int allowed_steps; /* cumulative YES to STEP 1..3 */
static int allow_bios, allow_byte, allow_vdd;
static const char *configured_vdd;
static int have_kernel, have_aperture;
static DWORD fake_flags;
static unsigned fake_selector;
static volatile unsigned char fake_video_byte;
static int initial_seen, stage_seen[7], armed_seen, refusal_seen;
static int kernel_calls, aperture_calls, flags_calls, bios_calls, map_calls;
static int vdd_calls;
static int requested_steps, bad_message;

HMODULE GetModuleHandle(const char *name)
{
    ++kernel_calls;
    if (strcmp(name,"KERNEL") != 0 || !have_kernel) return 0;
    return 1;
}
FARPROC GetProcAddress(HMODULE module,const char *name)
{
    ++aperture_calls;
    if (module != 1 || strcmp(name,"__A000H") != 0 || !have_aperture)
        return (FARPROC)0;
    return (FARPROC)(unsigned long)(0xABCD0000UL | (unsigned long)fake_selector);
}
DWORD GetWinFlags(void) { ++flags_calls; return fake_flags; }
void *paldiag_test_mk_fp(unsigned selector,unsigned offset)
{
    ++map_calls;
    if (selector != fake_selector || offset != 0u) ++bad_message;
    return (void *)&fake_video_byte;
}
int GetPrivateProfileString(const char *section,const char *key,
                            const char *fallback,char *buffer,int size,
                            const char *filename)
{
    size_t length;
    const char *value=configured_vdd ? configured_vdd : fallback;
    ++vdd_calls;
    if (strcmp(section,"386Enh") || strcmp(key,"display") ||
        strcmp(filename,"SYSTEM.INI") || size<2) ++bad_message;
    length=strlen(value);
    if (length>=(size_t)size) length=(size_t)size-1u;
    memcpy(buffer,value,length);
    buffer[length]=0;
    return (int)length;
}
int MessageBox(HWND window,const char *message,const char *title,unsigned flags)
{
    (void)window;
    if (!strstr(title,"v0035")) ++bad_message;
    if (strstr(message,"STEP 0:")) ++initial_seen;
    if (strstr(message,"STEP 1:")) ++stage_seen[1];
    if (strstr(message,"STEP 2:")) ++stage_seen[2];
    if (strstr(message,"STEP 3:")) {
        ++stage_seen[3];
        if (have_aperture && fake_selector == 0x0047u &&
            !strstr(message,"ABCD:0047")) ++bad_message;
    }
    if (strstr(message,"STEP 4: INT 10h")) {
        ++stage_seen[4];
        if (!strstr(message,"AL mode = 2F")) ++bad_message;
    }
    if (strstr(message,"STEP 5A:")) {
        ++armed_seen;
        if (map_calls) ++bad_message;
    }
    if (strstr(message,"STEP 5B:")) {
        ++stage_seen[5];
        if (!strstr(message,"returned 7B") || map_calls != 1) ++bad_message;
    }
    if (strstr(message,"STEP 5: A000h read was REFUSED")) ++refusal_seen;
    if (strstr(message,"STEP 6: SYSTEM.INI")) {
        ++stage_seen[6];
        if (configured_vdd && strcmp(configured_vdd,"*vddvga")==0 &&
            !strstr(message,"KNOWN INCOMPATIBLE")) ++bad_message;
    }
    if (flags & MB_YESNO) {
        if (strstr(message,"OPTIONAL STEP 4")) return allow_bios ? IDYES : IDNO;
        if (strstr(message,"EXPERIMENTAL STEP 5")) return allow_byte ? IDYES : IDNO;
        if (strstr(message,"OPTIONAL STEP 6")) return allow_vdd ? IDYES : IDNO;
        ++requested_steps;
        return requested_steps <= allowed_steps ? IDYES : IDNO;
    }
    return IDNO;
}
int int86(int interrupt_number,union REGS *in,union REGS *out)
{
    if (interrupt_number != 0x10 || in->x.ax != 0x0F00u) ++bad_message;
    ++bios_calls;
    out->h.al=0x2F;
    out->h.ah=80;
    out->h.bh=0;
    return 0;
}
static void reset(int step,int bios,int read_byte)
{
    allowed_steps=step;
    allow_bios=bios;
    allow_byte=read_byte;
    requested_steps=0;
    initial_seen=kernel_calls=aperture_calls=flags_calls=bios_calls=map_calls=vdd_calls=0;
    allow_vdd=0;
    configured_vdd="*vddvga";
    armed_seen=refusal_seen=bad_message=0;
    memset(stage_seen,0,sizeof(stage_seen));
    have_kernel=have_aperture=1;
    fake_flags=0x00000C29UL;  /* actual v0033 Windows 3.1 report */
    fake_selector=0x0047u;
    fake_video_byte=0x7Bu;
}
static int run(int step,int bios,int read_byte,int expected)
{
    int rc, reached_bios, reached_byte;
    reset(step,bios,read_byte);
    rc=WinMain(0,0,(char *)"",0);
    reached_bios=(step>=3 && bios);
    reached_byte=(step>=3 && read_byte);
    if (rc != expected || initial_seen != 1 || bad_message ||
        flags_calls != (step>=1) || kernel_calls != (step>=2) ||
        aperture_calls != (step>=3) ||
        stage_seen[1] != (step>=1) || stage_seen[2] != (step>=2) ||
        stage_seen[3] != (step>=3) ||
        bios_calls != reached_bios || stage_seen[4] != reached_bios ||
        map_calls != reached_byte || stage_seen[5] != reached_byte ||
        armed_seen != reached_byte || refusal_seen != 0 ||
        vdd_calls != 0 || stage_seen[6] != 0) {
        fprintf(stderr,"FAIL step=%d bios=%d byte=%d rc=%d bad=%d map=%d armed=%d\n",
                step,bios,read_byte,rc,bad_message,map_calls,armed_seen);
        return 0;
    }
    return 1;
}
int main(void)
{
    if (!run(0,0,0,0) || !run(1,0,0,0) || !run(2,0,0,0) ||
        !run(3,0,0,0) || !run(3,1,0,0) ||
        !run(3,0,1,0) || !run(3,1,1,0)) return 1;

    reset(3,1,1);
    have_kernel=0;
    if (WinMain(0,0,(char *)"",0) != 2 || aperture_calls || bios_calls ||
        map_calls) return 2;
    reset(3,1,1);
    have_aperture=0;
    if (WinMain(0,0,(char *)"",0) != 3 || bios_calls || map_calls) return 3;
    reset(3,0,1);
    fake_flags=0x20UL; /* No protected mode: refuse the byte read. */
    if (WinMain(0,0,(char *)"",0) != 5 || !refusal_seen ||
        armed_seen || map_calls) return 4;
    reset(3,0,1);
    fake_selector=0u; /* Non-null GetProcAddress, but invalid low selector. */
    if (WinMain(0,0,(char *)"",0) != 5 || !refusal_seen ||
        armed_seen || map_calls) return 5;
    /* The new SYSTEM.INI inspection must operate even when the optional
     * video-memory read is declined, and must not touch the aperture. */
    reset(3,0,0);
    allow_vdd=1;
    if (WinMain(0,0,(char *)"",0)!=0 || !stage_seen[6] ||
        vdd_calls!=1 || map_calls || bios_calls || bad_message) return 6;
    reset(3,1,1);
    allow_vdd=1;
    configured_vdd="VDDVGA30.386";
    if (WinMain(0,0,(char *)"",0)!=0 || stage_seen[6]!=1 ||
        vdd_calls!=1 || map_calls!=1 || bios_calls!=1 || bad_message)
        return 7;
    reset(3,0,0);
    allow_vdd=1;
    configured_vdd="";
    if (WinMain(0,0,(char *)"",0)!=0 || !stage_seen[6] ||
        vdd_calls!=1 || bad_message) return 8;
    reset(3,0,0);
    allow_vdd=1;
    configured_vdd="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                   "abcdefghijklmnopqrstuvwxyzabcde";
    if (WinMain(0,0,(char *)"",0)!=0 || !stage_seen[6] ||
        vdd_calls!=1 || bad_message) return 9;
    if (pal_vdd_kind("  *VDDVGA  ")!=PAL_VDD_STOCK_VGA31 ||
        pal_vdd_kind("C:\\WINDOWS\\SYSTEM\\vDdVgA30.386")!=PAL_VDD_VGA30 ||
        pal_vdd_kind("VDDTSENG.386")!=PAL_VDD_OTHER ||
        pal_vdd_kind("")!=PAL_VDD_UNKNOWN ||
        pal_vdd_preflight(0xC29UL,"*vddvga",307200UL)!=PAL_VDD_KNOWN_INCOMPATIBLE ||
        pal_vdd_preflight(0xC29UL,"*vddvga",256000UL)!=PAL_VDD_UNVERIFIED ||
        pal_vdd_preflight(0xC09UL,"*vddvga",307200UL)!=PAL_VDD_NOT_ENHANCED ||
        pal_vdd_preflight(0xC29UL,"VDDVGA30.386",307200UL)!=PAL_VDD_UNVERIFIED)
        return 10;
    puts("PASS: PALDIAG v0035 read-only VDD configuration + conservative 256K/512K guidance, previous API/VRAM opt-ins");
    return 0;
}
