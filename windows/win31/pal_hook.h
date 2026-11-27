#ifndef PAL_HOOK_H
#define PAL_HOOK_H
/* Only PALXHOOK links the resident assembly observer. No shared raster
 * code, C ISR or notification acknowledgement is involved. */
#define PAL_HOOK_BG 0u
#define PAL_HOOK_FG 1u
#define PAL_HOOK_SAVE 2u
#define PAL_HOOK_RESTORE 3u
#define PAL_HOOK_FOCUS 4u
#define PAL_HOOK_QUERY 5u
#define PAL_HOOK_KEY 4403u
#if defined(__WATCOMC__)
#define PAL_HOOK_CALL __far __cdecl
#else
#define PAL_HOOK_CALL
#endif
unsigned PAL_HOOK_CALL pal_hook_install(void);
unsigned PAL_HOOK_CALL pal_hook_remove(void);
unsigned PAL_HOOK_CALL pal_hook_read(unsigned index);
#endif
