#include <string.h>
#include "pal_cursor16.h"

static int signed_word(const rg_u8 *p)
{
    unsigned value=(unsigned)p[0] | ((unsigned)p[1]<<8);
    return value >= 32768u ? (int)((long)value-65536L) : (int)value;
}
/* Windows 3.x DDK WINDEFS.INC's EnterCrit is PUSHF/CLI. LeaveCrit
 * (POPFF) uses the IRET trampoline below rather than POPF because of the
 * 80286 interrupt-mask restoration problem. These bytes implement:
 *     JMP $+3 ; IRET ; PUSH CS ; CALL <IRET>
 * The final CALL displacement is -5 relative to the next IP.
 * Never replace these with plain CLI/STI. Use only on 16-bit Open Watcom;
 * NOT an approved 32-bit Windows 95/98 implementation.
 *
 * Pal_Move16 is exactly three consecutive 16-bit words. Both pointers
 * address near, persistent DGROUP storage (not stack locals). The assembly
 * does not call BIOS, allocate memory, or touch the framebuffer.
 */
#if defined(__WATCOMC__) && !defined(__386__)
typedef char PAL_Move16_must_be_six_bytes[(sizeof(PAL_Move16)==6u)?1:-1];
static void pal_irq_publish(PAL_Move16 *move, short x, short y);
#pragma aux pal_irq_publish = \
    "pushf" \
    "cli" \
    "mov [si+2],ax" \
    "mov [si+4],dx" \
    "mov word ptr [si],1" \
    0xeb 0x01 0xcf 0x0e 0xe8 0xfb 0xff \
    parm [si] [ax] [dx] modify [];
static void pal_irq_consume(PAL_Move16 *move, PAL_Move16 *snapshot);
#pragma aux pal_irq_consume = \
    "pushf" \
    "cli" \
    "mov bx,[si]" \
    "mov [di],bx" \
    "mov bx,[si+2]" \
    "mov [di+2],bx" \
    "mov bx,[si+4]" \
    "mov [di+4],bx" \
    "mov word ptr [si],0" \
    0xeb 0x01 0xcf 0x0e 0xe8 0xfb 0xff \
    parm [si] [di] modify [bx];
#else
/* Host simulator: actual atomicity is provided only by the Win16 DDK
 * critical sequence above; tests cannot establish interrupt safety. */
static void pal_irq_publish(PAL_Move16 *move, short x, short y)
{
    move->x=x; move->y=y; move->ready=1;
}
static void pal_irq_consume(PAL_Move16 *move, PAL_Move16 *snapshot)
{
    snapshot->ready=move->ready;
    snapshot->x=move->x;
    snapshot->y=move->y;
    move->ready=0;
}
#endif

unsigned pal_cursor16_wire_size(const rg_u8 *p, unsigned available)
{
    int width,height,color;
    unsigned total;
    if (!p || available < PAL_CURSOR16_HEADER) return 0;
    width=signed_word(p+4); height=signed_word(p+6);
    color=signed_word(p+10);
    if (width<1 || width>32 || height<1 || height>32 ||
        signed_word(p+8)!=4 || (color!=0 && color!=0x0101))
        return 0;
    /* Old DDK specifies zero. Also accept explicit one-plane/one-bit
     * metadata (0101h), with exactly the same bounded monochrome layout.
     * Native Windows shape metadata is still to be measured. */
    total=PAL_CURSOR16_HEADER + (unsigned)height*8u;
    return total <= available ? total : 0;
}
void pal_cursor16_unbind(PAL_Cursor16 *s)
{
    if (!s) return;
    ++s->draw_depth; /* prevent timer DDI while detaching framebuffer */
    rg_cursor_hide(&s->image);
    s->screen=0;
    s->incoming.ready=0;
    s->snapshot.ready=0;
    s->draw_depth=0;
    s->composing=0;
}
void pal_cursor16_bind(PAL_Cursor16 *s,RG_Device *screen)
{
    if (!s) return;
    pal_cursor16_unbind(s);
    ++s->draw_depth;
    s->screen=screen;
    s->incoming.ready=1;
    --s->draw_depth;
}
int pal_cursor16_set(PAL_Cursor16 *s, const rg_u8 *p,unsigned available)
{
    rg_u8 and_bits[128],xor_bits[128];
    unsigned size,w,h,bytes,y;
    if (!s) return 0;
    if (!p) {
        ++s->draw_depth; /* keep timer CheckCursor out of state changes */
        rg_cursor_hide(&s->image);
        s->wants_shape=0;
        s->image.has_shape=0;
        --s->draw_depth;
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
    ++s->draw_depth;
    if (!rg_cursor_shape(&s->image,and_bits,xor_bits,w,h,
                           signed_word(p),signed_word(p+2))) {
        --s->draw_depth;
        return 0;
    }
    s->wants_shape=1;
    --s->draw_depth;
    /* Defer hardware drawing until the next CheckCursor / end-draw. */
    return 1;
}
/* Mouse IRQ: one tiny interrupt-masked publication. No VRAM, BIOS or
 * port I/O, no C-only busy check, and no discarded nested MoveCursor.
 * A later nested IRQ publishes after the older IRQ resumes; Windows may
 * defer delivery, but a half-written coordinate pair is never exposed. */
void pal_cursor16_post_move(PAL_Cursor16 *s,short x,short y)
{
    if (!s) return;
    pal_irq_publish(&s->incoming,x,y);
}
/* Timer/idle compositor. Claim the guard before touching VRAM, so nested
 * timer interrupts and calls from end_draw cannot reenter the compositor.
 * The queue take atomically reads X/Y/ready and clears ready in the same
 * critical interval: a newer mouse IRQ cannot be erased by our take. */
void pal_cursor16_check(PAL_Cursor16 *s)
{
    if (!s || s->draw_depth || s->composing || !s->screen) return;
    s->composing=1;
    if (s->draw_depth || !s->screen) { s->composing=0; return; }
    pal_irq_consume(&s->incoming,&s->snapshot);
    if (s->snapshot.ready) {
        s->x=s->snapshot.x;
        s->y=s->snapshot.y;
        s->snapshot.ready=0;
    }
    if (!s->wants_shape) rg_cursor_hide(&s->image);
    else if (!s->image.visible || s->image.x!=s->x ||
             s->image.y!=s->y)
        (void)rg_cursor_show(&s->image,s->screen,s->x,s->y);
    s->composing=0;
}
void pal_cursor16_begin_draw(PAL_Cursor16 *s)
{
    if (!s) return;
    ++s->draw_depth;
    rg_cursor_hide(&s->image); /* also exclude nested drawing after a hold */
}
/* Hold timer composition while a drawing transaction is active, but do
 * not hide unless a screen region actually intersects the cursor. */
void pal_cursor16_hold(PAL_Cursor16 *s)
{
    if (s) ++s->draw_depth;
}
void pal_cursor16_exclude_rect(PAL_Cursor16 *s,int x,int y,int w,int h)
{
    long left,top,right,bottom;
    if (!s || !s->draw_depth || !s->image.visible || w<=0 || h<=0) return;
    left=(long)s->image.x-s->image.hot_x;
    top=(long)s->image.y-s->image.hot_y;
    right=left+s->image.width;
    bottom=top+s->image.height;
    if ((long)x<right && (long)x+w>left &&
        (long)y<bottom && (long)y+h>top) rg_cursor_hide(&s->image);
}
void pal_cursor16_end_draw(PAL_Cursor16 *s)
{
    if (!s || !s->draw_depth) return;
    --s->draw_depth;
    if (!s->draw_depth) pal_cursor16_check(s);
}
