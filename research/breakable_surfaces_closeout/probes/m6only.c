/* Single-function C2 debugger target: the cross-product operand tie alone.

   Every FP instruction this TU emits belongs to the six cross products, so
   any FP-path breakpoint hit is relevant.  Emitted (our build):
     18: fld [ebp-0x10]  (a.k)   1b: fmul [ebp-8]    (b.j)   <- TIE: January
     ...                                                        loads b.j
   Layout: a.i=-0x18 a.j=-0x14 a.k=-0x10 | b.i=-0xc b.j=-8 b.k=-4  */

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
	cross_product3d(&a, &b, result);
	return;
}
