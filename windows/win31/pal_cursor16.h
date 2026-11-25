#ifndef PAL_CURSOR16_H
#define PAL_CURSOR16_H
/* Windows 3.x CURSORSHAPE marshalling and cooperative software-cursor
 * state. Portable C89, independent of Win16 selector and port I/O.
 * MoveCursor publishes coordinates into an interrupt-safe Win16 mailbox;
 * CheckCursor consumes them only while the compositor is idle. The native
 * DDK critical sequence still needs runtime validation in 86Box.
 */
#include "rg_cursor.h"
#define PAL_CURSOR16_HEADER 12u
#define PAL_CURSOR16_MAX_WIRE (PAL_CURSOR16_HEADER + 256u)
/* Three 16-bit WORDs: publication flag, X and Y. On the Win16 platform
 * the DDK EnterCrit/LeaveCrit-equivalent assembly updates/consumes the
 * complete tuple in one interrupt-masked transaction. Host builds use the
 * same wire structure but ordinary C operations. */
typedef struct PAL_Move16 {
    volatile unsigned ready;
    volatile short x, y;
} PAL_Move16;
typedef struct PAL_Cursor16 {
    RG_Cursor image;
    RG_Device *screen;
    PAL_Move16 incoming, snapshot;
    volatile unsigned composing;
    int x, y;
    volatile unsigned draw_depth;
    unsigned wants_shape;
} PAL_Cursor16;

/* Wire header is six little-endian signed WORDs, masks are two consecutive
 * arrays of height*csWidthBytes, 4 bytes per row. Negative hotspots legal. */
unsigned pal_cursor16_wire_size(const rg_u8 *header, unsigned available);
int pal_cursor16_set(PAL_Cursor16 *, const rg_u8 *, unsigned);
void pal_cursor16_bind(PAL_Cursor16 *, RG_Device *);
void pal_cursor16_unbind(PAL_Cursor16 *);
void pal_cursor16_post_move(PAL_Cursor16 *, short, short);
void pal_cursor16_check(PAL_Cursor16 *);
void pal_cursor16_begin_draw(PAL_Cursor16 *);
void pal_cursor16_end_draw(PAL_Cursor16 *);
#endif
