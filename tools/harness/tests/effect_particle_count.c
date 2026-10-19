/*
EFFECT_PARTICLE_COUNT.C (test)

effect_real_to_byte (effects.c): how many particles an effect event makes, from a real that may be an infinity (the
split screen reduction divides by the 0 local players there are before the players are made). The x86 turns a real
outside a long into 0x80000000, a byte of 0; PowerPC and ARM saturate it, and an infinity became 255 particles there.
test_effect_particle_count.py takes the function from the source; its negative control is the saturating conversion.
*/

#include "harness.h"
#include <math.h>

#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	volatile real zero = 0.0f;

	CASE("counts")
	{
		CHECK(effect_real_to_byte(30.0f) == 30, "30");
		CHECK(effect_real_to_byte(7.9f) == 7, "7.9 is cut");
		CHECK(effect_real_to_byte(0.0f) == 0, "0");
		CHECK(effect_real_to_byte(-1.0f) == 255, "-1 is the byte it always was");
	}

	CASE("split-screen")
	{
		/* (count - 6) / 1 player + 6, and the 0 players of the first tick */
		CHECK(effect_real_to_byte((30.0f - 6) / 2 + 6) == 18, "two players");
		CHECK(effect_real_to_byte((30.0f - 6) / zero + 6) == 0, "no players: %d", effect_real_to_byte((30.0f - 6) / zero + 6));
		CHECK(effect_real_to_byte(-(30.0f - 6) / zero + 6) == 0, "negative infinity");
	}

	CASE("not-a-number")
	{
		CHECK(effect_real_to_byte(zero / zero) == 0, "nan");
		CHECK(effect_real_to_byte(3.0e9f) == 0, "past a long");
		CHECK(effect_real_to_byte(-3.0e9f) == 0, "before a long");
	}

	return 0;
}
