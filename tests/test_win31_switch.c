#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "pal_switch.h"
int main(void)
{
    ET4K_State card;
    PAL_Cursor16 cursor;
    RG_Device device;
    unsigned last=0;
    memset(&card,0,sizeof(card)); memset(&cursor,0,sizeof(cursor));
    card.active=1; card.bank_valid=1; card.read_bank=3; card.write_bank=4;
    cursor.screen=&device; cursor.image.device=&device; cursor.image.visible=1;
    cursor.wants_shape=1; cursor.image.has_shape=1;
    assert(!pal_switch_resume(&last,1,&card,&cursor)); /* guard not held */
    cursor.draw_depth=1; cursor.composing=1;
    assert(!pal_switch_resume(&last,1,&card,&cursor));
    cursor.composing=0;
    assert(!pal_switch_resume(&last,0,&card,&cursor));
    assert(pal_switch_resume(&last,1,&card,&cursor));
    assert(last==1&&!card.bank_valid&&!cursor.image.visible&&!cursor.image.device);
    assert(card.read_bank==3&&card.write_bank==4&&cursor.screen==&device);
    assert(cursor.image.has_shape&&cursor.wants_shape&&cursor.draw_depth==1);
    card.bank_valid=1;
    assert(!pal_switch_resume(&last,1,&card,&cursor)&&card.bank_valid);
    last=65535u;
    assert(pal_switch_resume(&last,0,&card,&cursor)&&last==0&&!card.bank_valid);
    puts("PASS: guarded foreground epoch, bank invalidation, cursor discard, repeat and wrap");
    return 0;
}
