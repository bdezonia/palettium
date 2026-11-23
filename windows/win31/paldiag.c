/* Palettium Win3.1 protected-mode diagnostic, v0033 crash isolation.
 * Ordinary Win16 GUI application (not a display driver).
 * STEP 0 appears before ANY KERNEL probing; all subsequent calls require
 * separate opt-in.  No selector-base lookups, A000 dereferences, mode sets,
 * port I/O, palette writes, or other hardware changes are made.
 */
#include <windows.h>
#include <dos.h>
#include <stdio.h>

static void report(const char *message)
{
    MessageBox((HWND)0, message, "Palettium PALDIAG v0033", MB_OK);
}

static int proceed(const char *message)
{
    return MessageBox((HWND)0, message, "Palettium PALDIAG v0033",
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
    char message[330];
    union REGS regs;

    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show_command;

    /* If Windows faults before this dialog, no A000 selector query ran.
     * Investigate the Win16 EXE loader/entry point/runtime instead. */
    report("STEP 0: PALDIAG v0033 entered WinMain.\n\n"
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

    if (!proceed("OPTIONAL STEP 4: Call BIOS INT 10h, AH=0Fh?\n\n"
                 "This only queries the current video mode.\n"
                 "Windows may virtualize the answer.\n"
                 "Choose No for the safest diagnostic run.")) return 0;
    regs.x.ax=0x0F00u;
    int86(0x10,&regs,&regs);
    sprintf(message,
            "STEP 4: INT 10h AH=0Fh returned.\n\n"
            "AL mode = %02X, AH columns = %02X, BH page = %02X\n\n"
            "No video mode was changed.",
            (unsigned)regs.h.al,(unsigned)regs.h.ah,(unsigned)regs.h.bh);
    report(message);
    return 0;
}
