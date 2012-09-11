#include "io.h"

int main(void)
{
    long long rd, rs, rt;
    long long result;

    rs     = 0x10FF01FF;
    rt     = 0x10010001;
    result = 0x21000200;
    __asm
        ("addwc %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addwc wrong\n");

        return -1;
    }

    rs     = 0xFFFF1111;
    rt     = 0x00020001;
    result = 0x00011112;
    __asm
        ("addwc %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (rd != result) {
        printf("addwc wrong\n");

        return -1;
    }

    return 0;
}
