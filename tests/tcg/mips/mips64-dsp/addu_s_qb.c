#include "io.h"

int main(void)
{
    long long rd, rs, rt;
    long long result;

    rs     = 0x10FF01FF;
    rt     = 0x10010001;
    result = 0x20FF01FF;
    __asm
        ("addu_s.qb %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addu_s.qb error 1\n");

        return -1;
    }

    rs     = 0xFFFFFFFFFFFF1111;
    rt     = 0x00020001;
    result = 0xFFFFFFFFFFFF1112;
    __asm
        ("addu_s.qb %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addu_s.qb error 2\n");

        return -1;
    }


    return 0;
}
