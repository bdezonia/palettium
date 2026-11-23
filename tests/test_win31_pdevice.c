#include <stdio.h>
#include <string.h>
#include "pal_pdevice.h"
#define CHECK(x) do { if (!(x)) { \
  fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    PAL_PDEVICE pd, bad;
    PAL_GDIINFO info;
    CHECK(pal_pd_layout_ok());
    CHECK(pal_win31_layout_ok());
    CHECK(!pal_pd_prepare((PAL_PDEVICE *)0,640,480,640));
    CHECK(!pal_pd_prepare(&pd,0,480,640));
    CHECK(!pal_pd_prepare(&pd,640,480,639));
    CHECK(!pal_pd_prepare(&pd,640,480,0));
    CHECK(!pal_pd_prepare(&pd,640,480,65536u));
    CHECK(pal_pd_prepare(&pd,640,480,640));
    CHECK(pal_pd_valid(&pd));
    CHECK(pd.state==PAL_PD_READY);
    CHECK(!pal_pd_stop((PAL_PDEVICE *)0));
    CHECK(pal_pd_capability_candidate(&info,&pd));
    CHECK(info.dpDEVICEsize==sizeof(pd));
    CHECK(info.dpHorzRes==640 && info.dpVertRes==480);
    CHECK(info.dpBitsPixel==8 && info.dpPlanes==1);
    CHECK(pal_pd_activate(&pd));
    CHECK(pd.state==PAL_PD_ACTIVE);
    CHECK(!pal_pd_activate(&pd));
    CHECK(pal_pd_stop(&pd));
    CHECK(pd.state==PAL_PD_READY);
    bad=pd;
    bad.magic=0;
    CHECK(!pal_pd_valid(&bad));
    CHECK(!pal_pd_activate(&bad));
    memset(&bad,0,sizeof(bad));
    CHECK(!pal_pd_capability_candidate(&info,&bad));
    CHECK(pal_pd_prepare(&pd,800,600,832));
    CHECK(pal_pd_capability_candidate(&info,&pd));
    CHECK(info.dpDEVICEsize==20 && info.dpHorzRes==800);
    puts("PASS: Win31 private PDEVICE layout, validation, lifecycle and GDIINFO size");
    return 0;
}
