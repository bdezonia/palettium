/* Palettium Windows 3.1 protected-mode environment diagnostic.
 * An ordinary Win16 GUI application; NEVER a display driver.
 * Default: report KERNEL A000h selector entry and bases only.
 * Optional: BIOS INT 10h AH=0Fh current-mode readback.
 * No A000h dereference, port I/O, palette change or video mode switch.
 */
#include <windows.h>
#include <dos.h>
#include <stdio.h>

typedef DWORD (FAR PASCAL *PAL_BaseFn)(WORD);

static void show_error(const char *message)
{
    MessageBox((HWND)0, message, "Palettium PALDIAG", MB_OK | MB_ICONSTOP);
}

/* KERNEL exposes selector pseudo-entries, not ordinary functions.  The
 * returned FARPROC has two WORDs.  Print both; do not assume which word
 * contains the selector until the real Windows runtime has been examined.
 * GetSelectorBase is asked only about plausible Win16 LDT selectors.
 */
static int plausible_selector(unsigned candidate)
{
    return candidate != 0u && (candidate & 7u) == 7u;
}

int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command)
{
    HMODULE kernel;
    FARPROC aperture;
    FARPROC base_entry;
    PAL_BaseFn get_base;
    unsigned low_word, high_word;
    unsigned low_checked=0u, high_checked=0u;
    unsigned long low_base=0UL, high_base=0UL;
    unsigned long raw;
    unsigned long flags;
    char message[520];
    int answer;
    union REGS regs;

    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show_command;

    kernel=GetModuleHandle("KERNEL");
    if (!kernel) {
        show_error("Cannot locate Windows KERNEL. No hardware was accessed.");
        return 1;
    }
    aperture=GetProcAddress(kernel,"__A000H");
    if (!aperture) {
        show_error("KERNEL does not export __A000H. No hardware was accessed.");
        return 2;
    }
    /* Conversion matches the existing v0030 Win16 adapter, but unlike that
     * adapter, this program does not commit to either WORD as a selector. */
    raw=(unsigned long)aperture;
    low_word=(unsigned)(raw & 65535UL);
    high_word=(unsigned)((raw >> 16) & 65535UL);
    flags=(unsigned long)GetWinFlags();

    base_entry=GetProcAddress(kernel,"GetSelectorBase");
    get_base=(PAL_BaseFn)base_entry;
    if (get_base) {
        if (plausible_selector(low_word)) {
            low_base=(unsigned long)(*get_base)((WORD)low_word);
            low_checked=1u;
        }
        if (plausible_selector(high_word)) {
            high_base=(unsigned long)(*get_base)((WORD)high_word);
            high_checked=1u;
        }
    }
    sprintf(message,
        "Read-only Windows 3.1 environment check\n\n"
        "KERNEL __A000H: %04X:%04X (high:low)\n"
        "GetSelectorBase entry: %s\n"
        "Low word  %04X: %s %08lX\n"
        "High word %04X: %s %08lX\n"
        "GetWinFlags: %08lX\n\n"
        "Expected A000h physical base: 000A0000.\n"
        "A selector base may be linear or virtualized,\n"
        "especially in 386 enhanced mode.\n"
        "NO VRAM was read or written. No mode was changed.\n\n"
        "Record these values for the Palettium maintainer.",
        high_word,low_word,base_entry ? "available" : "unavailable",
        low_word,low_checked ? "base" : "not checked",low_base,
        high_word,high_checked ? "base" : "not checked",high_base,
        flags);
    MessageBox((HWND)0,message,"Palettium PALDIAG - selector",MB_OK | MB_ICONINFORMATION);

    answer=MessageBox((HWND)0,
        "OPTIONAL: query the current BIOS video mode\n"
        "using INT 10h AH=0Fh?\n\n"
        "This requests information only; no mode set,\n"
        "A000h access, or port I/O is performed.\n"
        "Windows 386-enhanced mode may virtualize this reply.\n"
        "Choose No to skip the BIOS query.",
        "Palettium PALDIAG - BIOS query",MB_YESNO | MB_ICONQUESTION);
    if (answer != IDYES) return 0;
    regs.x.ax=0x0F00u;
    int86(0x10,&regs,&regs);
    sprintf(message,
        "INT 10h AH=0Fh reported:\n\n"
        "AL = %02X (current mode)\n"
        "AH = %02X (text columns)\n"
        "BH = %02X (active page)\n\n"
        "This is a readback only. No mode was changed.\n"
        "The result may reflect Windows video virtualization.",
        (unsigned)regs.h.al,(unsigned)regs.h.ah,(unsigned)regs.h.bh);
    MessageBox((HWND)0,message,"Palettium PALDIAG - BIOS",MB_OK | MB_ICONINFORMATION);
    return 0;
}
