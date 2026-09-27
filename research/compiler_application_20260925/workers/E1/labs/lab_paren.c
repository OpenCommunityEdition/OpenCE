/* E1 lab (never landing): what creates C2 opcode 0x267 (0-byte, scheduler unit 0) between fadd and fmul? */
#include "cseries.h"
#include "math/real_math.h"

real lab_paren_sum_times(
	real a,
	real b)
{
	return (a + 1.0f) * b;
}

real lab_local_sum_times(
	real a,
	real b)
{
	real t = a + 1.0f;

	return t * b;
}

real lab_no_paren(
	real a,
	real b)
{
	return a * b + b;
}

real lab_paren_plain(
	real a,
	real b)
{
	return (a * b) + b;
}
