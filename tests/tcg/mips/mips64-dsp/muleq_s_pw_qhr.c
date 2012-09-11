#include "io.h"

int main(void)
{
    long long rd, rs, rt, result;
    rd = 0;
    rs = 0x1234567845BCFFFF;
    rt = 0x8765432198529AD2;
    result = 0x52fbec7035a2ca5c;

    __asm
        ("muleq_s.pw.qhr %0, %1, %2\n\t"
         : "=r"(rd)
         : "r"(rs), "r"(rt)
        );

    if (result != rd) {
        printf("muleq_s.pw.qhr error\n");

        return -1;
    }

    return 0;
}
