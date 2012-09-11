#include "io.h"

int main(void)
{
    long long rd, rs, rt, result;

    rd = 0;
    rs = 0x1234567802020202;
    rt = 0x0034432112344321;
    result = 0x03A8FFFFFFFFFFFF;

    __asm
        ("muleu_s.qh.obl %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );

    if (rd != result) {
        printf("muleu_s.qh.obl error\n");

        return -1;
    }

    return 0;
}
