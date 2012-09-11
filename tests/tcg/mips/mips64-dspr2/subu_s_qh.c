#include "io.h"

int main(void)
{
    long long rd, rs, rt, dspreg, result, dspresult;
    rs = 0x123456789ABCDEF0;
    rt = 0x123456789ABCDEF1;
    result = 0x0000000000000000;
    dspresult = 0x01;

    __asm("subu_s.qh %0, %2, %3\n\t"
          "rddsp %1\n\t"
          : "=r"(rd), "=r"(dspreg)
          : "r"(rs), "r"(rt)
         );

    dspreg = ((dspreg >> 20) & 0x01);
    if ((rd != result) || (dspreg != dspresult)) {
        printf("subu_s.qh error\n\t");
        return -1;
    }

    return 0;
}
