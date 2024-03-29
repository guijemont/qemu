#include "io.h"

int main(void)
{
    long long rt, dsp;
    long long achi, acli;
    long long res, resdsp;

    achi = 0x87654321;
    acli = 0x12345678;

    res = 0x8000000000000000;
    resdsp = 0x1;

    __asm
        ("mthi %2, $ac1\n\t"
         "mtlo %3, $ac1\n\t"
         "dextr_rs.l %0, $ac1, 0x8\n\t"
         "rddsp %1\n\t"
         : "=r"(rt), "=r"(dsp)
         : "r"(achi), "r"(acli)
        );
    dsp = (dsp >> 23) & 0x1;

    if ((dsp != resdsp) || (rt != res)) {
        printf("dextr_rs.l error\n");
        return -1;
    }

    return 0;
}
