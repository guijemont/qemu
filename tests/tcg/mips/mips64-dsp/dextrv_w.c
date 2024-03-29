#include "io.h"

int main(void)
{
    long long rt, rs;
    long long achi, acli;
    long long res;

    achi = 0x87654321;
    acli = 0x12345678;
    rs = 0x8;

    res = 0x123456;

    __asm
        ("mthi %1, $ac1\n\t"
         "mtlo %2, $ac1\n\t"
         "dextrv.w %0, $ac1, %3\n\t"
         : "=r"(rt)
         : "r"(achi), "r"(acli), "r"(rs)
        );
    if (rt != res) {
        printf("dextrv.w error\n");
        return -1;
    }

    return 0;
}
