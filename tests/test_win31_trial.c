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
    CHECK(pal_win31_trial_permitted(0x0011UL,512u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0C29UL,512u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,512u,0u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,512u,4402u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,1024u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,0u,4401u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,256u,0u));
    CHECK(!pal_win31_trial_permitted(0x0011UL,256u,4402u));
    for (i=0;i<64u;++i) {
        f=(unsigned long)i;
        CHECK(pal_win31_trial_permitted(f,256u,4401u)==
              ((f&0x0031UL)==0x0011UL));
        CHECK(pal_win31_trial_permitted(f,512u,4401u)==
              ((f&0x0031UL)==0x0011UL));
    }
    CHECK(pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,"VDDTLI4.386","V7VGA.3GR"));
    CHECK(pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,"vddtli4.386","v7vga.3gr"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,"*vddvga","V7VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,"VDDTLI4.386","VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,0,"V7VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,512u,4401u,"VDDTLI4.386","V7VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,256u,4402u,"VDDTLI4.386","V7VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,1024u,4402u,"VDDTLI4.386","V7VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,"VDDTLI4.386 ","V7VGA.3GR"));
    CHECK(!pal_win31_enh_startup_permitted(0x0C29UL,512u,4402u,"VDDTLI4.386",""));
    for (i=0;i<64u;++i) {
        CHECK(pal_win31_enh_startup_permitted(i,512u,4402u,"VDDTLI4.386","V7VGA.3GR")==((i&0x31u)==0x21u));
    }
    puts("PASS: v0076 Enhanced experimental gate");
    puts("PASS: v0053 trial gate only Standard protected mode, 256/512KiB, key 4401");
    return 0;
}
