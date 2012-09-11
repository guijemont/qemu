#include "io.h"

int main(void)
{
    long long rd, rs, rt;
    long long result;

    rs     = 0xFFFFFFFF;
    rt     = 0x10101010;
    result = 0x100F100F;
    __asm
        ("addq_s.ph   %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addq_s.ph wrong\n");

        return -1;
    }

    rs     = 0x3712847D;
    rt     = 0x0031AF2D;
    result = 0x37438000;
    __asm
        ("addq_s.ph   %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addq_s.ph wrong\n");

        return -1;
    }

    return 0;
}
