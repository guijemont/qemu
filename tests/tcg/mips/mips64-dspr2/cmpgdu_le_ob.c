#include "io.h"

int main(void)
{
    long long rd, rs, rt, result, dspreg, dspresult;

    rs = 0x123456789ABCDEF0;
    rt = 0x123456789ABCDEFF;
    dspresult = 0xFF;
    result = 0xFF;

    __asm("cmpgdu.le.ob %0, %2, %3\n\t"
          "rddsp %1"
          : "=r"(rd), "=r"(dspreg)
          : "r"(rs), "r"(rt)
         );

    dspreg = ((dspreg >> 24) & 0xFF);

    if ((rd != result) || (dspreg != dspresult)) {
        printf("cmpgdu.le.ob error\n");
        return -1;
    }

    return 0;
}
