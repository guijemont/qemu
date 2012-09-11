#include "io.h"

int main(void)
{
    long long rs, rt, dsp;
    long long ach = 5, acl = 5;
    long long resulth, resultl, resultdsp;

    rs      = 0xBC0123AD;
    rt      = 0x01643721;
    resulth = 0x7FFFFFFF;
    resultl = 0xFFFFFFFFFFFFFFFF;
    resultdsp = 0x01;
    __asm
        ("mthi  %0, $ac1\n\t"
         "mtlo  %1, $ac1\n\t"
         "dpsq_sa.l.w $ac1, %3, %4\n\t"
         "mfhi  %0, $ac1\n\t"
         "mflo  %1, $ac1\n\t"
         "rddsp %2\n\t"
         : "+r"(ach), "+r"(acl), "=r"(dsp)
         : "r"(rs), "r"(rt)
        );
    dsp = (dsp >> 17) & 0x01;
    if ((dsp != resultdsp) || (ach != resulth) || (acl != resultl)) {
        printf("dpsq_sa.l.w wrong\n");

        return -1;
    }

    return 0;
}
