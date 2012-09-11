#include"io.h"

int main(void)
{
    long long rs, rt, dsp;
    long long ach = 5, acl = 5;
    long long resulth, resultl, resultdsp;

    rs     = 0x00FF00FF;
    rt     = 0x00010002;
    resulth = 0x00;
    resultl = 0x7fffffff;
    resultdsp = 0x01;
    __asm
        ("mthi  %0, $ac1\n\t"
         "mtlo  %1, $ac1\n\t"
         "dpaqx_sa.w.ph $ac1, %3, %4\n\t"
         "mfhi  %0, $ac1\n\t"
         "mflo  %1, $ac1\n\t"
         "rddsp %2\n\t"
         : "+r"(ach), "+r"(acl), "=r"(dsp)
         : "r"(rs), "r"(rt)
        );

    dsp = (dsp >> 17) & 0x01;
    if (dsp != resultdsp) {
        printf("dpaqx_sa.w.ph error\n");
        return -1;
    }

    if (ach != resulth) {
        printf("dpaqx_sa.w.ph error\n");
        return -1;
    }

    if (acl != resultl) {
        printf("dpaqx_sa.w.ph error\n");
        return -1;
    }

    return 0;
}
