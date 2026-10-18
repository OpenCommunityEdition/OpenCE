#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include "float.h"

void real_math_reset_precision(void);

#if !defined(__i386__)
#error This probe verifies the Intel x87 control word.
#endif

static unsigned short x87_control_word(void)
{
    unsigned short word;
    __asm__ __volatile__("fnstcw %0" : "=m"(word));
    return word;
}

int main(void)
{
    unsigned short before, after;
    unsigned int logical;

    if (_control87(_RC_UP | _PC_24, _MCW_RC | _MCW_PC) == 0) {
        fprintf(stderr, "could not set test rounding/precision state\n");
        return 1;
    }
    before = x87_control_word();
    if ((before & 0x0f00) != 0x0800 || fegetround() != FE_UPWARD) {
        fprintf(stderr, "test precondition not reflected in x87/fenv: %04x\n", before);
        return 1;
    }

    real_math_reset_precision();
    after = x87_control_word();
    logical = _control87(0, 0);
    if ((after & 0x0f00) != 0x0200 || fegetround() != FE_TONEAREST ||
        (after & 0x003f) != 0x003f ||
        (logical & (_MCW_RC | _MCW_PC)) != (_RC_NEAR | _PC_53)) {
        fprintf(stderr, "reset mismatch: x87 %04x, fenv %d, control %08x\n",
            after, fegetround(), logical);
        return 1;
    }
    printf("x87/fenv reset passed: before=%04x after=%04x logical=%08x\n",
        before, after, logical);
    return 0;
}
