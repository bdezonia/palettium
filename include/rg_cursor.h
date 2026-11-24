#ifndef RG_CURSOR_H
#define RG_CURSOR_H
/* Shared, device-independent SOFTWARE cursor compositor for up to 32x32.
 * AND mask applies first, XOR mask second; masks use MSB-first rows.
 * Saved-under is restored when hidden. The operating-system adapter MUST
 * exclude the cursor around all drawing to its visible rectangle, and
 * serialize mouse interrupt-time access before exposing cursor DDIs.
 */
#include "rgfx.h"
#define RG_CURSOR_MAX_WIDTH 32u
#define RG_CURSOR_MAX_HEIGHT 32u
#define RG_CURSOR_MASK_BYTES 128u

typedef struct RG_Cursor {
    RG_Device *device;
    unsigned char and_bits[RG_CURSOR_MASK_BYTES];
    unsigned char xor_bits[RG_CURSOR_MASK_BYTES];
    rg_pixel saved[RG_CURSOR_MAX_WIDTH*RG_CURSOR_MAX_HEIGHT];
    unsigned width,height,has_shape,visible;
    int hot_x,hot_y,x,y;
} RG_Cursor;

void rg_cursor_init(RG_Cursor *);
int rg_cursor_shape(RG_Cursor *, const rg_u8 *,const rg_u8 *,
                    unsigned width,unsigned height,int hot_x,int hot_y);
int rg_cursor_show(RG_Cursor *,RG_Device *,int x,int y);
int rg_cursor_move(RG_Cursor *,int x,int y);
void rg_cursor_hide(RG_Cursor *);
#endif
