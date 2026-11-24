#include <stdio.h>
#include "pal_trial_gate.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL at line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    unsigned long f;
    unsigned i;
    CHECK(pal_win31_trial_permitted(0x0011UL,256u,4401u));
    CHECK(pal_win31_trial_permitted(0x0C19UL,256u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0C29UL,256u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0031UL,256u,4401u));
    CHECK(!pal_win31_trial_permitted(0UL,256u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0010UL,256u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,512u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,1024u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,0u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,256u,0u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,256u,4402u));
    for (i=0;i<64u;++i) {
        f=(unsigned long)i;
        CHECK(pal_win31_trial_permitted(f,256u,4401u)==
              ((f&0x0031UL)==0x0011UL));
    }
    puts("PASS: v0044 trial gate only Standard protected mode, 256KiB, key 4401");
    return 0;
}
