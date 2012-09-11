#include "io.h"

int main(void)
{
    long long rt, ach, acl, dsp;
    long long result;

    ach = 0x05;
    acl = 0xB4CB;
    result = 0xFFFFFFFFA0001699;
    __asm
        ("mthi %2, $ac1\n\t"
         "mtlo %3, $ac1\n\t"
         "extr_r.w %0, $ac1, 0x03\n\t"
         "rddsp %1\n\t"
         : "=r"(rt), "=r"(dsp)
         : "r"(ach), "r"(acl)
        );
    dsp = (dsp >> 23) & 0x01;
    if ((dsp != 1) || (result != rt)) {
        printf("extr_r.w wrong\n");

        return -1;
    }

    return 0;
}
