#include <string.h>
#include "pal_cursor16.h"

static int signed_word(const rg_u8 *p)
{
    unsigned value=(unsigned)p[0] | ((unsigned)p[1]<<8);
    return value >= 32768u ? (int)((long)value-65536L) : (int)value;
}
unsigned pal_cursor16_wire_size(const rg_u8 *p, unsigned available)
{
    int width,height,hotx,hoty;
    unsigned total;
    if (!p || available < PAL_CURSOR16_HEADER) return 0;
    hotx=signed_word(p); hoty=signed_word(p+2);
    width=signed_word(p+4); height=signed_word(p+6);
    if (width<1 || width>32 || height<1 || height>32 ||
        signed_word(p+8)!=4 || signed_word(p+10)!=0 ||
        hotx < -32768 || hotx>32767 || hoty < -32768 || hoty>32767)
        return 0;
    total=PAL_CURSOR16_HEADER + (unsigned)height*8u;
    return total <= available ? total : 0;
}
void pal_cursor16_unbind(PAL_Cursor16 *s)
{
    if (!s) return;
    rg_cursor_hide(&s->image);
    s->screen=0;
    s->draw_depth=0;
}
void pal_cursor16_bind(PAL_Cursor16 *s,RG_Device *screen)
{
    if (!s) return;
    pal_cursor16_unbind(s);
    s->screen=screen;
    s->pending=1;
}
int pal_cursor16_set(PAL_Cursor16 *s, const rg_u8 *p,unsigned available)
{
    rg_u8 and_bits[128],xor_bits[128];
    unsigned size,w,h,bytes,y;
    if (!s) return 0;
    if (!p) {
        rg_cursor_hide(&s->image);
        s->wants_shape=0;
        s->image.has_shape=0;
        return 1;
    }
    size=pal_cursor16_wire_size(p,available);
    if (!size) return 0; /* MUST leave old cursor intact on malformed shape */
    w=(unsigned)signed_word(p+4);h=(unsigned)signed_word(p+6);
    bytes=(w+7u)/8u;
    memset(and_bits,0xff,sizeof(and_bits));
    memset(xor_bits,0,sizeof(xor_bits));
    for (y=0;y<h;++y) {
        memcpy(and_bits+y*bytes,p+PAL_CURSOR16_HEADER+y*4u,bytes);
        memcpy(xor_bits+y*bytes,p+PAL_CURSOR16_HEADER+h*4u+y*4u,bytes);
    }
    if (!rg_cursor_shape(&s->image,and_bits,xor_bits,w,h,
                           signed_word(p),signed_word(p+2))) return 0;
    s->wants_shape=1;
    s->pending=1;
    /* Defer hardware drawing until the next CheckCursor / end-draw. */
    return 1;
}
/* Called from MoveCursor at mouse-interrupt time. Never access the screen,
 * BIOS, bank registers or hardware ports here. The busy flag avoids reading
 * partly published coordinates in CheckCursor. A nested MoveCursor can lose
 * a newer position; this is why Win16 interrupt authorization stays OFF. */
void pal_cursor16_post_move(PAL_Cursor16 *s,short x,short y)
{
    if (!s || s->writing) return;
    s->writing=1;
    s->pending=0;
    s->pending_x=x;
    s->pending_y=y;
    s->pending=1;
    s->writing=0;
}
void pal_cursor16_check(PAL_Cursor16 *s)
{
    int x,y;
    if (!s || s->draw_depth || s->writing || !s->screen) return;
    if (s->pending) {
        x=s->pending_x; y=s->pending_y;
        s->pending=0;
        s->x=x;s->y=y;
    }
    if (!s->wants_shape) { rg_cursor_hide(&s->image); return; }
    if (s->image.visible && s->image.x==s->x && s->image.y==s->y)
        return;
    (void)rg_cursor_show(&s->image,s->screen,s->x,s->y);
}
void pal_cursor16_begin_draw(PAL_Cursor16 *s)
{
    if (!s) return;
    ++s->draw_depth;
    if (s->draw_depth==1u) rg_cursor_hide(&s->image);
}
void pal_cursor16_end_draw(PAL_Cursor16 *s)
{
    if (!s || !s->draw_depth) return;
    --s->draw_depth;
    if (!s->draw_depth) pal_cursor16_check(s);
}
