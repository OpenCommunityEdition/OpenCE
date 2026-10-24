#include "harness.h"

#include <math.h>
#include <stdint.h>
#include "under_test.inc"

static uint64_t bits(double value)
{
	uint64_t result;

	memcpy(&result, &value, sizeof(result));
	return result;
}

/* halo_fmod against the C library's fmod, to the bit (a zero's sign and a
NaN's too) */
static void compare(double x, double y)
{
	double expected = fmod(x, y), result = halo_fmod(x, y);

	if (isnan(expected))
		CHECK(isnan(result), "fmod(%.17g, %g): %.17g, not NaN", x, y, result);
	else
		CHECK(bits(result) == bits(expected), "fmod(%.17g, %g): %.17g, not %.17g", x, y, result, expected);
}

static const double edges[] = {
	0.0, -0.0, 1.0, -1.0, 3.0, -3.0, 0.5, -0.5, 0.25, -0.75, 1e-310, -1e-310, 1e-30, -1e-30,
	0.99999999999999989, -0.99999999999999989, 1.0000000000000002, 25.6, -25.6,
	8388607.5, 8388608.0, 16777215.0, 16777217.0, 4503599627370495.5, -4503599627370495.5,
	2147483647.0, 2147483647.75, -2147483647.75, -2147483648.0, -2147483648.5, 2147483648.0,
	2147483648.25, 3e9, -3e9, 1e300, -1e300, 1.7976931348623157e308
};

int main(int argc, char **argv)
{
	const char *case_name = argv[1];
	static const double divisors[] = {2.0, 0.5, -1.0, 3.0, 6.2831853071795862, 0.0};
	uint32_t seed = 12345;
	int i, d;

	CASE("fraction")
	{
		for (i = 0; i < (int)(sizeof(edges) / sizeof(edges[0])); i++)
			compare(edges[i], 1.0);
		compare(HUGE_VAL, 1.0);
		compare(-HUGE_VAL, 1.0);
		compare(NAN, 1.0);
		/* the periodic functions' times (a float scaled by 25.6), and
		values of every size */
		for (i = 0; i < 200000; i++)
		{
			float time;

			seed = seed * 1664525u + 1013904223u;
			time = (float)(seed >> 8) / 256.0f * ((seed & 1) ? 1.0f : -1.0f);
			compare((double)(time * 25.6f), 1.0);
			compare(ldexp((double)seed / 4294967296.0, (int)(seed % 80) - 20) * ((seed & 2) ? 1.0 : -1.0), 1.0);
		}
	}
	CASE("other")
	{
		for (d = 0; d < (int)(sizeof(divisors) / sizeof(divisors[0])); d++)
			for (i = 0; i < (int)(sizeof(edges) / sizeof(edges[0])); i++)
				compare(edges[i], divisors[d]);
	}
	return 0;
}
