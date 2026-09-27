/* E1 lab L3 (never landing): what does VC7 13.00.9254 /O2 emit for the /Od-attested r*0.25f + 0.25f? */
#include "cseries.h"
#include "math/real_math.h"

real lab_factor_param(
	real a)
{
	return a * 0.25f + 0.25f;
}

real lab_factor_random(
	void)
{
	return real_random() * 0.25f + 0.25f;
}
