#include "io.h"

int main(void)
{
    long long rd, rs, rt;
    long long result;

    rs = 0x12345678;
    rt = 0x87654321;
    result = 0x24AC0086;

    __asm
        ("precrqu_s.qb.ph %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );
    if (result != rd) {
        printf("precrqu_s.qb.ph wrong\n");

        return -1;
    }

    return 0;
}
