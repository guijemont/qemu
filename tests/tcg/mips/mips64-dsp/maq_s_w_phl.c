#include "io.h"

int main(void)
{
    long long rt, rs;
    long long achi, acli;
    long long acho, aclo;
    long long resulth, resultl;

    achi = 0x05;
    acli = 0xB4CB;
    rs  = 0xFF060000;
    rt  = 0xCB000000;
    resulth = 0x04;
    resultl = 0xFFFFFFFF947438CB;

    __asm
        ("mthi %2, $ac1\n\t"
         "mtlo %3, $ac1\n\t"
         "maq_s.w.phl $ac1, %4, %5\n\t"
         "mfhi %0, $ac1\n\t"
         "mflo %1, $ac1\n\t"
         : "=r"(acho), "=r"(aclo)
         : "r"(achi), "r"(acli), "r"(rs), "r"(rt)
        );
    if ((resulth != acho) || (resultl != aclo)) {
        printf("maq_s.w.phl wrong\n");

        return -1;
    }

    return 0;
}
