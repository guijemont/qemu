#include"io.h"

int main(void)
{
    long long rd, rs, rt, dsp;
    long long result, resultdsp;

    rs = 0x03FB1234;
    rt = 0x0BCC4321;
    result = 0x7fff7FFF;
    resultdsp = 1;

    __asm
        ("mul_s.ph %0, %2, %3\n\t"
         "rddsp %1\n\t"
         : "=r"(rd), "=r"(dsp)
         : "r"(rs), "r"(rt)
        );
    dsp = (dsp >> 21) & 0x01;
    if (rd != result || dsp != resultdsp) {
        printf("mul_s.ph error\n");
        return -1;
    }

    return 0;
}
