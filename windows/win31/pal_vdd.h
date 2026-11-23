#ifndef PAL_VDD_H
#define PAL_VDD_H
/* Read-only Win3.x enhanced-mode VDD configuration assessment.
 * The caller supplies SYSTEM.INI [386Enh] display=; a configuration
 * value is NOT proof of which VxD was actually loaded or its behavior.
 * This Win3.x-specific policy does not belong in the shared renderer. */
#define PAL_VDD_UNKNOWN 0
#define PAL_VDD_STOCK_VGA31 1
#define PAL_VDD_VGA30 2
#define PAL_VDD_OTHER 3
#define PAL_VDD_UNVERIFIED 0
#define PAL_VDD_KNOWN_INCOMPATIBLE 1
#define PAL_VDD_NOT_ENHANCED 2

int pal_vdd_kind(const char *configured_display);
int pal_vdd_preflight(unsigned long windows_flags,
                      const char *configured_display,
                      unsigned long framebuffer_bytes);
const char *pal_vdd_kind_name(int kind);
#endif
