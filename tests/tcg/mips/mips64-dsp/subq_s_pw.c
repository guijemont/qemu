#include "io.h"

int main(void)
{
    long long rd, rs, rt, result, dspreg, dspresult;
    rt = 0x9FFFFFFD9FFFFFFD;
    rs = 0x4000000080000000;
    result = 0x7fffffffe0000003;
    dspresult = 0x1;

    __asm
        ("subq_s.pw %0, %2, %3\n\t"
         "rddsp %1\n\t"
         : "=r"(rd), "=r"(dspreg)
         : "r"(rs), "r"(rt)
        );
    dspreg = (dspreg >> 20) & 0x1;
    if ((rd != result) || (dspreg != dspresult)) {
        printf("subq_s.pw error1\n");

        return -1;
    }

    rt = 0x123456789ABCDEF1;
    rs = 0x123456789ABCDEF2;
    result =  0x0000000000000001;
    /* This time we do not set dspctrl, but it setted in pre-action. */
    dspresult = 0x1;

    __asm
        ("subq_s.pw %0, %2, %3\n\t"
         "rddsp %1\n\t"
         : "=r"(rd), "=r"(dspreg)
         : "r"(rs), "r"(rt)
        );
    dspreg = (dspreg >> 20) & 0x1;
    if ((rd != result) || (dspreg != dspresult)) {
        printf("subq_s.pw error2\n");

        return -1;
    }

    return 0;
}

