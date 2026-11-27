/* The production evaluator with controlled table samples, including the
 * reverse slide that exposed a negative alpha in Deathtrap's wet materials. */
#include <math.h>
#include "harness.h"
#include "enums.inc"

#define VALID_INDEX(i, n) ((unsigned)(i) < (unsigned)(n))
#define PIN(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define fast_ftol(x) ((long)(x))
#define function_type_error(kind, type) ((void)0)
static boolean function_tables_initialized = TRUE;
static byte samples[NUMBER_OF_PERIODIC_FUNCTIONS][PERIODIC_FUNCTION_TABLE_SIZE];
static byte *periodic_function_tables[NUMBER_OF_PERIODIC_FUNCTIONS];
#include "under_test.inc"

static void near(real actual, real expected)
{
	CHECK(fabs(actual - expected) < 0.000001f, "got %.9g, expected %.9g", actual, expected);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short type;
	long i;
	real value, old;
	for (type = 0; type < NUMBER_OF_PERIODIC_FUNCTIONS; type++)
	{
		periodic_function_tables[type] = samples[type];
		for (i = 0; i < PERIODIC_FUNCTION_TABLE_SIZE; i++)
			samples[type][i] = (byte)(i % 256);
	}
	CASE("negative")
	{
		/* -0.25 table samples is three quarters through the last interval. */
		samples[_periodic_function_cosine][1023] = 204;
		samples[_periodic_function_cosine][0] = 51;
		near(periodic_function_evaluate(_periodic_function_cosine, -0.25f / 25.6f), 0.35f);
	}
	CASE("slide-wrap")
	{
		for (type = _periodic_function_slide; type <= _periodic_function_slide_variable_period; type++)
		{
			samples[type][1023] = 204;
			samples[type][0] = 51;
			near(periodic_function_evaluate(type, -0.25f / 25.6f), 0.1f);
			near(periodic_function_evaluate(type, -0.75f / 25.6f), 0.9f);
		}
	}
	CASE("integer")
	{
		near(periodic_function_evaluate(_periodic_function_slide, -1.0f / 25.6f), 1.0f);
		near(periodic_function_evaluate(_periodic_function_slide, -40.0f), 0.0f);
		near(periodic_function_evaluate(_periodic_function_slide, -0.0f), 0.0f);
	}
	CASE("positive")
	{
		for (type = 0; type < NUMBER_OF_PERIODIC_FUNCTIONS; type++)
			for (i = 0; i < 20000; i++)
			{
				value = periodic_function_evaluate(type, i / 128.0f);
				old = original_evaluate(type, i / 128.0f);
				CHECK(memcmp(&value, &old, sizeof(value)) == 0, "positive time changed: %d %ld", type, i);
			}
	}
	CASE("range")
	{
		for (type = 0; type < NUMBER_OF_PERIODIC_FUNCTIONS; type++)
			for (i = -20000; i <= 20000; i++)
			{
				value = periodic_function_evaluate(type, i / 128.0f);
				CHECK(value >= 0.0f && value <= 1.0f, "outside range: %d %ld %.9g", type, i, value);
			}
	}
	CASE("uninitialized")
	{
		function_tables_initialized = FALSE;
		near(periodic_function_evaluate(_periodic_function_slide, -0.1f), 0.0f);
		near(periodic_function_evaluate(_periodic_function_one, -0.1f), 1.0f);
	}
	return 0;
}
