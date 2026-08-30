/* Control for the differential C1 profile: identical headers and identical
   locals to m6only.c, but WITHOUT the cross-product expression.  Sampling
   both and subtracting cancels header parsing, which otherwise dominates. */

#include "cseries.h"
#include "math/real_math.h"

extern void micro_fill(real_vector3d *v);

void micro_m6(
	real_vector3d *result)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&a);
	micro_fill(&b);
	result->i = a.i;
	return;
}
