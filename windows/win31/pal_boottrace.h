#ifndef PAL_BOOTTRACE_H
#define PAL_BOOTTRACE_H
/* Isolated trial diagnostic. No tracing or file I/O in PALSTAGE. */
#if defined(PAL_WIN31_BOOTTRACE)
void pal_boottrace(const char *event);
#else
#define pal_boottrace(event) ((void)0)
#endif
#endif
