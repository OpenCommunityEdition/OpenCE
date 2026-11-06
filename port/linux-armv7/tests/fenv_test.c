/*
FENV_TEST.C

The MSVC floating-point control the game calls (_controlfp, _statusfp,
_clearfp; port/linux/src/msvc_crt.c) on 32-bit ARM, where the rounding mode and
the exception flags are those of the VFP unit. Linked with msvc_crt.o and run
on armv7 hardware or under qemu-user (port/linux-armv7/README.md, "Testing the
floating-point control").
*/

#include <float.h>
#include <stdio.h>
#include <string.h>

unsigned int _controlfp(unsigned int new_value, unsigned int mask);
unsigned int _statusfp(void);
unsigned int _clearfp(void);

/* the MSVC values (port/linux/include/float.h) and the x87 status bits
_statusfp returns */
#define MCW_RC 0x300u
#define RC_NEAR 0x000u
#define RC_DOWN 0x100u
#define RC_UP 0x200u
#define RC_CHOP 0x300u
#define SW_INVALID 0x01u
#define SW_ZERODIVIDE 0x04u
#define SW_OVERFLOW 0x08u
#define SW_UNDERFLOW 0x10u
#define SW_INEXACT 0x20u

static int failures;

static unsigned int bits(float value)
{
	unsigned int result;

	memcpy(&result, &value, sizeof(result));
	return result;
}

static void check(const char *name, unsigned int got, unsigned int wanted)
{
	if (got != wanted)
	{
		printf("FAIL %s: got %08x, wanted %08x\n", name, got, wanted);
		failures++;
	}
	else
	{
		printf("ok   %s\n", name);
	}
}

/* 1/3 and -1/3 in the rounding mode */
static void rounding(const char *name, unsigned int mode, unsigned int positive, unsigned int negative)
{
	volatile float one = 1.0f, three = 3.0f, minus_one = -1.0f;
	char label[64];
	volatile float quotient;

	check("control word reads back the mode", _controlfp(mode, MCW_RC) & MCW_RC, mode);
	quotient = one / three;
	snprintf(label, sizeof(label), "%s: 1/3", name);
	check(label, bits(quotient), positive);
	quotient = minus_one / three;
	snprintf(label, sizeof(label), "%s: -1/3", name);
	check(label, bits(quotient), negative);
}

int main(void)
{
	volatile float zero = 0.0f, one = 1.0f, big = FLT_MAX, small = FLT_MIN, three = 3.0f;
	volatile float result;

	rounding("nearest", RC_NEAR, 0x3eaaaaabu, 0xbeaaaaabu);
	rounding("down", RC_DOWN, 0x3eaaaaaau, 0xbeaaaaabu);
	rounding("up", RC_UP, 0x3eaaaaabu, 0xbeaaaaaau);
	rounding("chop", RC_CHOP, 0x3eaaaaaau, 0xbeaaaaaau);
	_controlfp(RC_NEAR, MCW_RC);

	_clearfp();
	result = one / zero;
	check("1/0 sets zero divide", _statusfp() & SW_ZERODIVIDE, SW_ZERODIVIDE);
	check("_clearfp returns the flags", _clearfp() & SW_ZERODIVIDE, SW_ZERODIVIDE);
	check("and clears them", _statusfp(), 0);

	result = zero / zero;
	check("0/0 sets invalid", _statusfp() & SW_INVALID, SW_INVALID);
	_clearfp();

	result = big * 2.0f;
	check("FLT_MAX*2 sets overflow", _statusfp() & SW_OVERFLOW, SW_OVERFLOW);
	check("and inexact", _statusfp() & SW_INEXACT, SW_INEXACT);
	_clearfp();

	result = small / three;
	check("FLT_MIN/3 sets underflow", _statusfp() & SW_UNDERFLOW, SW_UNDERFLOW);
	_clearfp();

	result = one + one;
	check("an exact sum sets no flag", _statusfp(), 0);
	(void)result;

	printf("%d failure(s)\n", failures);
	return failures != 0;
}
