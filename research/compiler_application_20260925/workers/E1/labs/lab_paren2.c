/* E1 lab L2 (never landing): does 0x267 depend on the multiplier / operand kind / accumulation context? */
#include "cseries.h"
#include "math/real_math.h"

real lab_const_mul(
	real a)
{
	return (a + 1.0f) * 0.25f;
}

real lab_random_const_mul(
	void)
{
	return (real_random() + 1.0f) * 0.25f;
}

real lab_accumulate(
	real a,
	real b)
{
	return (a + 1.0f) * 0.25f + b;
}

real lab_mul_first(
	real a)
{
	return 0.25f * (a + 1.0f);
}
