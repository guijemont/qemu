#include "io.h"

int main(void)
{
    long long rt, rs;
    long long achi, acli;
    long long acho, aclo;
    long long resulth, resultl;

    achi = 0x05;
    acli = 0xB4CB;
    rs  = 0xFF06;
    rt  = 0xCB00;
    resulth = 0xFFFFFFFFFFFFFFFF;
    resultl = 0xFFFFFFFF947438cb;

    __asm
        ("mthi %2, $ac1\n\t"
         "mtlo %3, $ac1\n\t"
         "maq_sa.w.phr $ac1, %4, %5\n\t"
         "mfhi %0, $ac1\n\t"
         "mflo %1, $ac1\n\t"
         : "=r"(acho), "=r"(aclo)
         : "r"(achi), "r"(acli), "r"(rs), "r"(rt)
        );
    if ((resulth != acho) || (resultl != aclo)) {
        printf("maq_sa.w.phr wrong\n");

        return -1;
    }

    return 0;
}
