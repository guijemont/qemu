#include "io.h"

int main(void)
{
    long long rd, result;
    rd = 0;
    result = 0x000001FF000001FF;

    __asm
        ("repl.pw %0, 0x1FF\n\t"
         : "=r"(rd)
        );

    if (result != rd) {
        printf("repl.pw error\n");

        return -1;
    }

    rd = 0;
    result = 0xFFFFFE00FFFFFE00;
    __asm
        ("repl.pw %0, 0xFFFFFFFFFFFFFE00\n\t"
         : "=r"(rd)
        );

    if (result != rd) {
        printf("repl.pw error\n");

        return -1;
    }

    return 0;
}
