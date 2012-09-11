#include "io.h"

int main(void)
{
    long long rs, dsp;
    long long achi, acli;
    long long res, rsdsp;


    rs = 0xaaaabbbbccccdddd;
    achi = 0x87654321;
    acli = 0x12345678;
    dsp = 0x22;

    res = 0x62;

    __asm
        ("mthi %1, $ac1\n\t"
         "mtlo %2, $ac1\n\t"
         "wrdsp %3\n\t"
         "dmthlip %4, $ac1\n\t"
         "rddsp %0\n\t"
         : "=r"(rsdsp)
         : "r"(achi), "r"(acli), "r"(dsp), "r"(rs)
        );
    if (rsdsp != res) {
        printf("dmthlip error\n");
        return -1;
    }

    return 0;
}
