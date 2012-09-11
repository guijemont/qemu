#include "io.h"

int main(void)
{
  long long rs, rt, dspreg, dspresult;

  rs = 0x123456789ABCDEF0;
  rt = 0x123456789ABCDEFF;
  dspresult = 0x02;

  __asm
      ("cmp.eq.pw %1, %2\n\t"
       "rddsp %0\n\t"
       : "=r"(dspreg)
       : "r"(rs), "r"(rt)
      );

  dspreg = ((dspreg >> 24) & 0x03);

  if (dspreg != dspresult) {
    printf("cmp.eq.pw error\n");

    return -1;
  }

  return 0;
}
