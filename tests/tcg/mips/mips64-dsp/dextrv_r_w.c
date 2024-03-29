#include "io.h"

int main(void)
{
    long long rt, rs, dsp;
    long long achi, acli;
    long long res, resdsp;

    achi = 0x87654321;
    acli = 0x12345678;
    rs = 0x8;

    res = 0x123456;
    resdsp = 0x01;

    __asm
        ("mthi %2, $ac1\n\t"
         "mtlo %3, $ac1\n\t"
         "dextrv_r.w %0, $ac1, %4\n\t"
         "rddsp %1\n\t"
         : "=r"(rt), "=r"(dsp)
         : "r"(achi), "r"(acli), "r"(rs)
        );

    dsp = (dsp >> 23) & 0x1;

    if ((dsp != resdsp) || (rt != res)) {
        printf("dextrv_r.w error\n");
        return -1;
    }

    return 0;
}
