#ifndef PAL_REPAINT_H
#define PAL_REPAINT_H
/* Windows-specific repaint epoch policy; no hardware or USER dependencies.
 * A request may be claimed only when foreground, permitted, recovered and
 * idle. Commit before the callback, since USER may reenter DISPLAY. */
typedef struct PAL_Repaint {
    volatile unsigned armed, blocked, busy, last, calls, completed;
    volatile unsigned disables, enables, deferred;
} PAL_Repaint;
static void pal_repaint_arm(PAL_Repaint *s, unsigned fg)
{
    s->last=fg; s->busy=0; s->calls=0; s->completed=0;
    s->deferred=0; s->armed=1;
}
static void pal_repaint_permission(PAL_Repaint *s, unsigned disable)
{
    s->blocked=(disable & 255u)!=0;
    if (s->blocked) ++s->disables;
    else ++s->enables;
}
static int pal_repaint_claim(PAL_Repaint *s, unsigned focus, unsigned fg,
                             unsigned ready)
{
    if (!s->armed || s->busy || focus!=2u || fg==s->last) return 0;
    if (s->blocked || !ready) { ++s->deferred; return 0; }
    s->busy=1; s->last=fg; ++s->calls;
    return 1;
}
static void pal_repaint_finish(PAL_Repaint *s)
{
    ++s->completed; s->busy=0;
}
#endif
