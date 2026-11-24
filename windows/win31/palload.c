/* Palettium v0043: REAL Windows 3.1 NE display driver loader probe.
 * NOT a display driver and NOT an installer. Uses an existing VGA desktop.
 * Explicit opt-in per step. Never requests Enable(style=0), hardware I/O,
 * video modes, or cursor painting. Requires C:\PALTEST\PALSTAGE.DRV.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "pal_gdiinfo.h"

typedef WORD (FAR PASCAL *PAL_EnableFn)(LPVOID,WORD,LPSTR,LPSTR,LPVOID);
typedef WORD (FAR PASCAL *PAL_InquireFn)(LPVOID);

/* Data remains in DGROUP: the exported Win16 procedures receive FAR data
 * pointers, never a near pointer to an automatic (SS) scratch buffer. */
static unsigned char pal_gdi_guard[PAL_GDIINFO_BYTES + 16u];
static unsigned char pal_cursor_guard[12];
static PAL_GDIINFO pal_received;
static const char *const pal_required_exports[] = {
    "BITBLT", "COLORINFO", "DISABLE", "ENABLE", "OUTPUT", "PIXEL",
    "REALIZEOBJECT", "STRBLT", "SCANLR", "EXTTEXTOUT",
    "SETPALETTE", "GETPALETTE", "SETPALTRANS", "GETPALTRANS",
    "UPDATECOLORS", "INQUIRE", "SETCURSOR", "MOVECURSOR", "CHECKCURSOR"
};

static void pal_report(const char *message)
{
    MessageBox((HWND)0, message, "PALLOAD v0043", MB_OK);
}
static int pal_confirm(const char *message)
{
    return MessageBox((HWND)0, message, "PALLOAD v0043",
                      MB_YESNO | MB_ICONQUESTION) == IDYES;
}
static int pal_guard_ok(const unsigned char *area,unsigned total,
                         unsigned begin,unsigned written)
{
    unsigned i;
    for (i=0;i<total;++i)
        if ((i<begin || i>=begin+written) && area[i]!=0xA5u) return 0;
    return 1;
}

int PASCAL WinMain(HINSTANCE instance,HINSTANCE previous,
                   LPSTR command_line,int show_command)
{
    HINSTANCE module;
    FARPROC address;
    PAL_EnableFn enable;
    PAL_InquireFn inquire;
    unsigned i,missing=0,problem=0;
    WORD result;
    unsigned configured_kib;
    char message[380];
    (void)instance; (void)previous; (void)command_line; (void)show_command;

    pal_report("STEP 0: PALLOAD entered WinMain.\n\n"
               "Existing VGA desktop remains active.\n"
               "No Palettium driver was loaded or called.\n"
               "All subsequent steps require Yes.");
    if (!pal_confirm("STEP 1: Load C:\\PALTEST\\PALSTAGE.DRV?\n\n"
                     "This calls Win16 LoadLibrary on the REAL NE module.\n"
                     "No display-mode or video-memory operation is requested.\n"
                     "Use a disposable 86Box snapshot. No exits safely."))
        return 0;
    module=LoadLibrary("C:\\PALTEST\\PALSTAGE.DRV");
    if ((UINT)module<=32u) {
        sprintf(message,"STEP 1 FAILED: LoadLibrary returned %u.\n\n"
                        "Check C:\\PALTEST\\PALSTAGE.DRV and DLL imports.\n"
                        "Windows display configuration was NOT modified.",
                        (unsigned)(UINT)module);
        pal_report(message);
        return 1;
    }
    sprintf(message,"STEP 1 PASS: PALSTAGE.DRV loaded by Windows.\n\n"
                    "Module handle = %04X. No DDI called yet.",
                    (unsigned)(UINT)module);
    pal_report(message);

    if (!pal_confirm("STEP 2: Resolve the 19 expected DDI exports?\n\n"
                     "Uses GetProcAddress only; no entry point is called."))
        goto finish;
    for (i=0;i<sizeof(pal_required_exports)/sizeof(pal_required_exports[0]);++i) {
        address=GetProcAddress(module,pal_required_exports[i]);
        if (!address) {
            ++missing;
            sprintf(message,"STEP 2: Missing export %s.",
                            pal_required_exports[i]);
            pal_report(message);
        }
    }
    sprintf(message,"STEP 2: %u of 19 named DDI exports resolved.\n\n"
                    "No drawing or hardware method was called.",19u-missing);
    pal_report(message);
    if (missing) { problem=2; goto finish; }

    if (!pal_confirm("STEP 3: Call cursor Inquire @101?\n\n"
                     "Writes only four bytes into PALLOAD's own RAM.\n"
                     "Does not create or move a cursor."))
        goto finish;
    address=GetProcAddress(module,"INQUIRE");
    if (!address) { problem=3; goto finish; }
    inquire=(PAL_InquireFn)address;
    memset(pal_cursor_guard,0xA5,sizeof(pal_cursor_guard));
    result=(*inquire)((LPVOID)&pal_cursor_guard[4]);
    if (result!=4u || pal_cursor_guard[4]!=1u ||
        pal_cursor_guard[5]!=0u || pal_cursor_guard[6]!=2u ||
        pal_cursor_guard[7]!=0u ||
        !pal_guard_ok(pal_cursor_guard,sizeof(pal_cursor_guard),4u,4u)) {
        sprintf(message,"STEP 3 FAILED: Inquire returned %u or corrupted RAM.",
                        (unsigned)result);
        pal_report(message); problem=3; goto finish;
    }
    pal_report("STEP 3 PASS: Inquire returned four bytes (X=1, Y=2).\n"
               "Both guard regions were preserved.");

    if (!pal_confirm("STEP 4: Call Enable(style=1) for GDIINFO ONLY?\n\n"
                     "No physical initialization. No Enable(style=0).\n"
                     "Requires SYSTEM.INI [Palettium] VRAMKiB=256/512/1024\n"
                     "for a nonzero result. Driver stays UNINSTALLED."))
        goto finish;
    configured_kib=(unsigned)GetPrivateProfileInt(
                       "Palettium","VRAMKiB",0,"SYSTEM.INI");
    address=GetProcAddress(module,"ENABLE");
    if (!address) { problem=4; goto finish; }
    enable=(PAL_EnableFn)address;
    memset(pal_gdi_guard,0xA5,sizeof(pal_gdi_guard));
    result=(*enable)((LPVOID)&pal_gdi_guard[8],1u,
                      (LPSTR)0,(LPSTR)0,(LPVOID)0);
    if (!pal_guard_ok(pal_gdi_guard,sizeof(pal_gdi_guard),8u,
                      result==PAL_GDIINFO_BYTES ? PAL_GDIINFO_BYTES : 0u)) {
        pal_report("STEP 4 FAILED: GDIINFO buffer guard changed unexpectedly.");
        problem=4; goto finish;
    }
    if (result==0u && configured_kib==0u) {
        pal_report("STEP 4 EXPECTED REFUSAL: VRAMKiB is not configured.\n\n"
                   "To query a mode candidate, set [Palettium] VRAMKiB\n"
                   "to actual 86Box card VRAM and rerun PALLOAD.\n"
                   "No framebuffer or system display driver was changed.");
        goto finish;
    }
    if (result!=PAL_GDIINFO_BYTES) {
        sprintf(message,"STEP 4 REFUSED: Enable(style=1) returned %u.\n"
                        "Configured VRAMKiB = %u. No mode change occurred.",
                        (unsigned)result,configured_kib);
        pal_report(message); problem=4; goto finish;
    }
    memcpy(&pal_received,&pal_gdi_guard[8],PAL_GDIINFO_BYTES);
    if (sizeof(pal_received)!=PAL_GDIINFO_BYTES ||
        pal_received.dpVersion!=PAL_WIN31_GDI_VERSION ||
        pal_received.dpTechnology!=1 ||
        pal_received.dpHorzRes!=640 ||
        (pal_received.dpVertRes!=400 && pal_received.dpVertRes!=480) ||
        pal_received.dpBitsPixel!=8 || pal_received.dpPlanes!=1 ||
        pal_received.dpDEVICEsize!=20u ||
        pal_received.dpLogPixelsX!=96 || pal_received.dpLogPixelsY!=96) {
        pal_report("STEP 4 FAILED: GDIINFO layout or geometry invalid.");
        problem=4; goto finish;
    }
    sprintf(message,"STEP 4 PASS: Real driver Enable(style=1).\n\n"
                    "GDIINFO bytes: %u\nResolution: %d x %d x 8\n"
                    "PDEVICE bytes: %u\nConfigured VRAMKiB: %u\n\n"
                    "No hardware was initialized or changed.",
                    (unsigned)result,(int)pal_received.dpHorzRes,
                    (int)pal_received.dpVertRes,
                    (unsigned)pal_received.dpDEVICEsize,configured_kib);
    pal_report(message);
finish:
    FreeLibrary(module);
    pal_report(problem ? "FINISH: Probe encountered a failure.\n"
                         "PALSTAGE.DRV unloaded; system display unchanged."
                       : "FINISH: PALSTAGE.DRV unloaded.\n"
                         "No Windows display-driver installation occurred.");
    return (int)problem;
}
