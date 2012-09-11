#include "io.h"

int main(void)
{
    long long rd, rs, rt, result;

    rd = 0;
    rs = 0x1234567802020204;
    rt = 0x0034432112344321;
    result = 0x006886422468FFFF;

    __asm
        ("muleu_s.qh.obr %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );

    if (rd != result) {
        printf("muleu_s.qh.obr error\n");

        return -1;
    }

    return 0;
}
