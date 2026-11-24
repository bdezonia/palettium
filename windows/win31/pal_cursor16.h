#ifndef PAL_CURSOR16_H
#define PAL_CURSOR16_H
/* Windows 3.x CURSORSHAPE marshalling and cooperative software-cursor
 * state. Portable C89, independent of Win16 selector and port I/O.
 * MoveCursor queues coordinates: CheckCursor consumes them only when the
 * DDI is not drawing. Native interrupt-time synchronization is NOT YET
 * independently validated; Enable(style=0) remains blocked.
 */
#include "rg_cursor.h"
#define PAL_CURSOR16_HEADER 12u
#define PAL_CURSOR16_MAX_WIRE (PAL_CURSOR16_HEADER + 256u)
typedef struct PAL_Cursor16 {
    RG_Cursor image;
    RG_Device *screen;
    volatile unsigned writing;
    volatile unsigned pending;
    volatile short pending_x, pending_y;
    int x, y;
    unsigned draw_depth, wants_shape;
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
