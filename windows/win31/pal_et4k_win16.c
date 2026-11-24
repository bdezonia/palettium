/* Native Windows 3.1 platform adapter for the existing ET4000AX backend.
 * IMPORTANT: unlike real-mode DOS, MK_FP(0xA000, offset) is invalid here.
 * KERNEL's exported __A000H symbol is a protected-mode selector. Windows
 * 3.1 Guide to Programming: global selectors for memory-mapped hardware.
 * Do not call this adapter from Enable until the VDD and GDI lifecycle
 * are implemented. In particular, raw BIOS INT 10h is NOT sufficient to
 * coordinate DOS-session switching in 386 Enhanced mode.
 */
#include <windows.h>
#include <dos.h>
#include <conio.h>
#include "pal_et4k_win16.h"

static rg_u8 win16_in(void *ctx, unsigned port)
{
    (void)ctx;
    return (rg_u8)inp(port);
}
static void win16_out(void *ctx, unsigned port, rg_u8 value)
{
    (void)ctx;
    outp(port,value);
}
static rg_u8 win16_read(void *ctx, unsigned offset)
{
    const volatile rg_u8 FAR *p;
    PAL_Win16ET4K *w=(PAL_Win16ET4K *)ctx;
    p=(const volatile rg_u8 FAR *)MK_FP(w->aperture_selector,offset);
    return *p;
}
static void win16_write(void *ctx, unsigned offset, rg_u8 value)
{
    volatile rg_u8 FAR *p;
    PAL_Win16ET4K *w=(PAL_Win16ET4K *)ctx;
    p=(volatile rg_u8 FAR *)MK_FP(w->aperture_selector,offset);
    *p=value;
}
static unsigned win16_bios_current_mode(void)
{
    union REGS r;
    r.h.ah=0x0Fu;
    int86(0x10,&r,&r);
    return (unsigned)r.h.al;
}
static void win16_bios_set(unsigned mode)
{
    union REGS r;
    r.h.ah=0;
    r.h.al=(unsigned char)mode;
    int86(0x10,&r,&r);
}
static int win16_set_mode(void *ctx, unsigned width, unsigned height)
{
    PAL_Win16ET4K *w=(PAL_Win16ET4K *)ctx;
    unsigned wanted;
    if (!w || !w->aperture_selector || width != 640u ||
        (height != 400u && height != 480u)) return 0;
    wanted=height==400u ? ET4K_BIOS_640X400X8 : ET4K_BIOS_640X480X8;
    /* Save the previous BIOS video mode BEFORE any attempt. Neither RAM
     * size nor ROM support is inferred from a returned BIOS mode number. */
    if (!w->previous_valid) {
        w->saved_bios_mode=win16_bios_current_mode();
        if (w->saved_bios_mode > 0x7Fu) return 0;
        w->previous_valid=1;
    }
    w->bios_mode_attempted=1;
    win16_bios_set(wanted);
    /* INT 10h AH=00h has no standardized success return. If AH=0Fh
     * does not report our requested mode, fail closed; shutdown restores.
     * This verification has NOT yet been tested inside Windows 3.1. */
    return win16_bios_current_mode() == wanted;
}
static void win16_restore(void *ctx)
{
    PAL_Win16ET4K *w=(PAL_Win16ET4K *)ctx;
    if (!w || !w->bios_mode_attempted || !w->previous_valid) return;
    win16_bios_set(w->saved_bios_mode);
    w->previous_valid=0;
    w->bios_mode_attempted=0;
}

int pal_win16_et4k_port(PAL_Win16ET4K *w, ET4K_Port *out)
{
    HMODULE kernel;
    FARPROC entry;
    unsigned selector;
    if (!w || !out) return 0;
    /* KERNEL.174 (__A000H) exports a selector as the LOW WORD of an
     * address, not a function to call. Do not dereference 'entry'. */
    kernel=GetModuleHandle("KERNEL");
    if (!kernel) return 0;
    entry=GetProcAddress(kernel,"__A000H");
    if (!entry) return 0;
    selector=(unsigned)((unsigned long)entry & 65535UL);
    if (!selector) return 0;
    w->aperture_selector=selector;
    w->saved_bios_mode=0;
    w->previous_valid=0;
    w->bios_mode_attempted=0;
    out->cookie=w;
    out->in_port=win16_in;
    out->out_port=win16_out;
    out->mem_read=win16_read;
    out->mem_write=win16_write;
    out->set_mode=win16_set_mode;
    out->restore=win16_restore;
    out->query_capacity=0; /* Never guess VRAM from register bits. */
    out->query_mode=0; /* BIOS support is confirmed only by a mode set. */
    return 1;
}


int pal_win16_et4k_crtc37_advisory(const ET4K_Port *io,
                                    ET4K_Crtc37 *result,
                                    int ports_authorized)
{
    /* The Windows 3.1 stock VGA VDD traps graphics operations in enhanced
     * mode. A DOS test cannot establish safety of raw Win16 port accesses.
     * Even in Standard mode, callers must authorize port access explicitly. */
    if (!ports_authorized || (GetWinFlags() & 0x0020u) != 0u) return 0;
    return et4k_crtc37_snapshot(io,result);
}

int pal_win16_et4k_profile_plan(PAL_ET4KPlan *plan)
{
    unsigned kib;
    char display[96];
    if (!plan) return 0;
    /* Windows 3.1 DDK supports reading display configuration using the
     * GetPrivateProfile* APIs. Profile values are operator-supplied, NOT
     * proof of installed VRAM or actual VDD compatibility. This path
     * never opens the hardware or invokes the BIOS. */
    kib=(unsigned)GetPrivateProfileInt("Palettium","VRAMKiB",0,"SYSTEM.INI");
    if (kib!=256u && kib!=512u && kib!=1024u) return 0;
    display[0]=0;
    (void)GetPrivateProfileString("386Enh","display","",display,
                                  sizeof(display),"SYSTEM.INI");
    return pal_et4k_plan(plan,(unsigned long)kib*1024UL,0UL,
                          (unsigned long)GetWinFlags(),display);
}
