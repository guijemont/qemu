#include "io.h"

int main(void)
{
    long long rd, rt, rs, dsp;
    long long result, resultdsp;

    rt        = 0x8765432112345678;
    rs = 0x8;
    result    = 0x6543210034567800;
    resultdsp = 1;

    __asm
        ("shllv.pw %0, %2, %3\n\t"
         "rddsp %1\n\t"
         : "=r"(rd), "=r"(dsp)
         : "r"(rt), "r"(rs)
        );

    dsp = (dsp >> 22) & 0x01;
    if ((dsp != resultdsp) || (rd  != result)) {
        printf("shllv.pw wrong\n");
        return -1;
    }

    return 0;
}
