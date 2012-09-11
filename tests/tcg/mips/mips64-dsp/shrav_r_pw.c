#include "io.h"

int main(void)
{
    long long rd, rt, rs;
    long long res;

    rt = 0x1234567887654321;
    rs = 0x4;
    res = 0x01234568f8765432;

    __asm
        ("shrav_r.pw %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rt), "r"(rs)
        );

    if (rd != res) {
        printf("shrav_r.pw error\n");
        return -1;
    }
    return 0;
}
