/* Palettium Win3.1 hardware environment diagnostic, v0035.
 * Ordinary Win16 GUI application (NOT a display driver).
 * STEP 0 appears before kernel probing. Each query requires opt-in.
 * Experimental STEP 5 reads exactly ONE BYTE from __A000H:0000 after
 * explicit confirmation and an immediate pre-read checkpoint. It can
 * still fault in a misconfigured VM; never run on a nonrecoverable system.
 * Optional STEP 6 reads SYSTEM.INI [386Enh] display= as an advisory.
 * No selector-base lookups, mode changes, port I/O, VRAM writes or banking.
 */
#include <windows.h>
#include <dos.h>
#include <stdio.h>
#include "pal_vdd.h"

static void report(const char *message)
{
    MessageBox((HWND)0, message, "Palettium PALDIAG v0035", MB_OK);
}

static int proceed(const char *message)
{
    return MessageBox((HWND)0, message, "Palettium PALDIAG v0035",
                      MB_YESNO | MB_ICONQUESTION) == IDYES;
}

int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command)
{
    DWORD flags;
    HMODULE kernel;
    FARPROC aperture;
    unsigned long raw;
    unsigned low_word, high_word;
    char message[380];
    char vdd_name[80];
    unsigned vdd_length;
    int kind, verdict;
    union REGS regs;
    const volatile unsigned char FAR *video_byte;
    unsigned value;

    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show_command;

    /* If Windows faults before this dialog, no A000 selector query ran.
     * Investigate the Win16 EXE loader/entry point/runtime instead. */
    report("STEP 0: PALDIAG v0035 entered WinMain.\n\n"
           "No KERNEL selector query or BIOS call has occurred.\n"
           "Each potentially troublesome query has its own checkpoint.");

    if (!proceed("NEXT: GetWinFlags() only.\n\n"
                 "Choose No to exit without any KERNEL query.")) return 0;
    flags=(DWORD)GetWinFlags();
    sprintf(message, "STEP 1: GetWinFlags returned %08lX.\n\n"
            "No video memory, BIOS, or port access occurred.",
            (unsigned long)flags);
    report(message);

    if (!proceed("NEXT: GetModuleHandle(\"KERNEL\") only.\n\n"
                 "Choose No to stop here.")) return 0;
    kernel=GetModuleHandle("KERNEL");
    sprintf(message, "STEP 2: GetModuleHandle returned %04X.\n\n"
            "No video memory, BIOS, or port access occurred.",
            (unsigned)kernel);
    report(message);
    if (!kernel) return 2;

    if (!proceed("NEXT: GetProcAddress(KERNEL, \"__A000H\") only.\n\n"
                 "This is an EXPORTED SELECTOR, not a callable function.\n"
                 "We will NOT dereference it or call GetSelectorBase.\n"
                 "Choose No to stop here.")) return 0;
    aperture=GetProcAddress(kernel, "__A000H");
    if (!aperture) {
        report("STEP 3: __A000H export was not found.\n"
               "No hardware access was attempted.");
        return 3;
    }
    /* A far address has a selector/offset word pair.  In Win3.x KERNEL's
     * exported __A000H pseudo-entry, the offset (LOW WORD) is documented
     * to carry the selector.  Report both words, DO NOT validate by calling
     * GetSelectorBase on an unverified selector (v0032 fault suspect). */
    raw=(unsigned long)aperture;
    low_word=(unsigned)(raw & 65535UL);
    high_word=(unsigned)((raw >> 16) & 65535UL);
    sprintf(message,
            "STEP 3: __A000H export returned.\n\n"
            "Raw high:low = %04X:%04X\n"
            "Low word (documented selector): %04X\n"
            "GetWinFlags: %08lX\n\n"
            "No GetSelectorBase call. No A000h access.\n"
            "Record this dialog before continuing.",
            high_word,low_word,low_word,(unsigned long)flags);
    report(message);

    /* An optional BIOS readback is independent of the later VRAM test.
     * Declining STEP 4 must NOT automatically authorize or prevent STEP 5. */
    if (proceed("OPTIONAL STEP 4: Call BIOS INT 10h, AH=0Fh?\n\n"
                "This only queries the current video mode.\n"
                "Windows may virtualize the answer.\n"
                "Choose No to skip this operation.")) {
        regs.x.ax=0x0F00u;
        int86(0x10,&regs,&regs);
        sprintf(message,
                "STEP 4: INT 10h AH=0Fh returned.\n\n"
                "AL mode = %02X, AH columns = %02X, BH page = %02X\n\n"
                "No video mode was changed.",
                (unsigned)regs.h.al,(unsigned)regs.h.ah,(unsigned)regs.h.bh);
        report(message);
    }

    /* A000h access in 386 Enhanced mode may be intercepted by a VDD.
     * The exported selector address alone does NOT establish physical
     * identity, read safety, or permission for SVGA bank switching.
     * Even this single read can cause a GP fault: explicit opt-in only. */
    if (proceed("EXPERIMENTAL STEP 5: Read ONE BYTE from A000h?\n\n"
                 "Requires a recoverable 86Box snapshot. A fault is possible.\n"
                 "No write, port I/O, mode or bank switch occurs.\n"
                 "Choose No to SKIP the VRAM read and go to STEP 6.")) {

    if (!(flags & 1UL) || low_word == 0u || low_word == 65535u) {
        report("STEP 5: A000h read was REFUSED.\n"
               "Protected-mode flag or selector preflight failed.");
        return 5;
    }
    sprintf(message,
            "STEP 5A: ABOUT TO READ A000h:0000h.\n\n"
            "Selector = %04X; one volatile byte load follows\n"
            "after this dialog closes.\n"
            "If Windows faults, report STEP 5A as last dialog.\n"
            "No VRAM write, mode change or bank change.",low_word);
    report(message);
    video_byte=(const volatile unsigned char FAR *)MK_FP(low_word,0u);
    value=(unsigned)*video_byte;
    sprintf(message,
            "STEP 5B: A000h:0000h byte read returned %02X.\n\n"
            "This verifies an accessible mapping in this run,\n"
            "NOT physical VRAM identity or VDD compatibility.\n"
            "No framebuffer write or other hardware change.",value);
    report(message);
    }

    /* STEP 6 reads only SYSTEM.INI; no video hardware interaction.
     * This is a CONFIGURATION hint, not an inspection of the loaded VDD.
     * Allow it regardless of whether STEP 4 or STEP 5 was skipped. */
    if (proceed("OPTIONAL STEP 6: Read SYSTEM.INI [386Enh] display=?\n\n"
                "This is a read-only configuration query.\n"
                "It does NOT verify which VDD is loaded or compatible.\n"
                "No video memory, BIOS, or register access.")) {
        vdd_name[0]=0;
        vdd_length=(unsigned)GetPrivateProfileString(
            "386Enh","display","",vdd_name,
            (int)sizeof(vdd_name),"SYSTEM.INI");
        vdd_name[sizeof(vdd_name)-1u]=0;
        /* A full buffer may be truncated; never classify as validated. */
        if (vdd_length>=sizeof(vdd_name)-1u) {
            report("STEP 6: SYSTEM.INI display= is too long to report.\n"
                   "Treat configured VDD as UNKNOWN, not validated.");
        } else {
            kind=pal_vdd_kind(vdd_name);
            verdict=pal_vdd_preflight((unsigned long)flags,vdd_name,
                                       307200UL /* 640x480x8 */);
            sprintf(message,
                    "STEP 6: SYSTEM.INI [386Enh] display=\n"
                    "%s\n"
                    "Classification: %s\n\n"
                    "640x480x8 (307200 bytes): %s\n"
                    "640x400x8 (256000 bytes): UNVERIFIED\n\n"
                    "Configured only; loaded VDD NOT verified.",
                    vdd_name[0] ? vdd_name : "(missing)",
                    pal_vdd_kind_name(kind),
                    verdict==PAL_VDD_KNOWN_INCOMPATIBLE ?
                    "KNOWN INCOMPATIBLE" :
                    verdict==PAL_VDD_NOT_ENHANCED ?
                    "no enhanced-mode VDD" : "UNVERIFIED");
            report(message);
        }
    }
    return 0;
}
