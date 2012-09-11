#include "io.h"

int main(void)
{
    long long rd, rs, rt;
    long long result;

    rs     = 0x00FF00FF;
    rt     = 0x00010001;
    result = 0x00000000;
    __asm
        ("addu.qb %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addu.qb wrong\n");

        return -1;
    }

    rs     = 0xFFFF1111;
    rt     = 0x00020001;
    result = 0xFFFFFFFFFF011112;
    __asm
        ("addu.qb %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addu.qb wrong\n");

        return -1;
    }

    return 0;
}
