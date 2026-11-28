#include <assert.h>
#include <stdio.h>
#include "pal_repaint.h"
int main(void)
{
    PAL_Repaint s={0,0,0,0,0,0,0,0,0};
    assert(!pal_repaint_claim(&s,2,1,1));
    pal_repaint_permission(&s,1);
    pal_repaint_arm(&s,0);
    assert(!pal_repaint_claim(&s,1,1,1));
    assert(!pal_repaint_claim(&s,2,0,1));
    assert(!pal_repaint_claim(&s,2,1,1) && s.deferred==1);
    pal_repaint_permission(&s,0x100); /* low byte flag */
    assert(!pal_repaint_claim(&s,2,1,0) && s.deferred==2);
    assert(pal_repaint_claim(&s,2,1,1));
    assert(s.busy && s.last==1 && s.calls==1);
    assert(!pal_repaint_claim(&s,2,1,1));
    assert(!pal_repaint_claim(&s,2,2,1));
    pal_repaint_finish(&s);
    assert(!s.busy && s.completed==1);
    assert(!pal_repaint_claim(&s,2,1,1));
    assert(pal_repaint_claim(&s,2,2,1));
    pal_repaint_finish(&s);
    s.last=65535u;
    assert(pal_repaint_claim(&s,2,0,1));
    pal_repaint_finish(&s);
    s.armed=0;
    assert(!pal_repaint_claim(&s,2,1,1));
    assert(s.completed==3 && s.calls==3 && s.disables==1 && s.enables==1);
    puts("PASS: native repaint policy, defer, recovery gate, reentry, wrap and disarm");
    return 0;
}
