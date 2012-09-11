#include "io.h"

int main(void)
{
    long long rd, rs, rt, result;

    rs = 0x123456789ABCDEF0;
    rt = 0x123456789ABCDEFF;
    result = 0x01;

    __asm
        ("cmpgu.lt.ob %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );

    if (rd != result) {
        printf("cmpgu.lt.ob error\n");

        return -1;
    }

   return 0;
}
