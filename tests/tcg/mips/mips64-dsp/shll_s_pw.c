#include "io.h"

int main(void)
{
    long long rd, rt, dsp;
    long long result, resultdsp;

    rt        = 0x8765432112345678;
    result    = 0x800000007fffffff;
    resultdsp = 1;

    __asm
        ("shll_s.pw %0, %2, 0x8\n\t"
         "rddsp %1\n\t"
         : "=r"(rd), "=r"(dsp)
         : "r"(rt)
        );

    dsp = (dsp >> 22) & 0x01;
    if ((dsp != resultdsp) || (rd  != result)) {
        printf("shll_s.pw wrong\n");
        return -1;
    }

    return 0;
}
