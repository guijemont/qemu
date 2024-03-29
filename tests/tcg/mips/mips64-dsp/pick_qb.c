#include "io.h"

int main(void)
{
    long long rd, rs, rt, dsp;
    long long result;

    rs = 0x12345678;
    rt = 0x87654321;
    dsp = 0x0A000000;
    result = 0x12655621;

    __asm
        ("wrdsp %3, 0x10\n\t"
         "pick.qb %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt), "r"(dsp)
        );
    if (rd != result) {
        printf("pick.qb wrong\n");

        return -1;
    }

    return 0;
}
