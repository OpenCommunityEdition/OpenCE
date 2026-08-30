/* Micro-probes for the commutative FP multiply operand-role choice.

   Each function computes products of members of two stack-local vectors.
   The question in every case: for `X*Y` with both operands in memory, which
   one does VC7 emit as the `fld` and which as the `fmul` memory operand?  */

#include "cseries.h"
#include "math/real_math.h"

extern void micro_fill(real_vector3d *v);

/* m1: a defined first, then b; two products, second uses a.k */
void micro_m1(real *out)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&a);
	micro_fill(&b);
	out[0] = a.i * b.j;
	out[1] = a.k * b.i;
	return;
}

/* m2: b defined first, then a */
void micro_m2(real *out)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&b);
	micro_fill(&a);
	out[0] = a.i * b.j;
	out[1] = a.k * b.i;
	return;
}

/* m3: the a.k product alone */
void micro_m3(real *out)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&a);
	micro_fill(&b);
	out[0] = a.k * b.i;
	return;
}

/* m4: the a.k product first */
void micro_m4(real *out)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&a);
	micro_fill(&b);
	out[0] = a.k * b.i;
	out[1] = a.i * b.j;
	return;
}

/* m5: a.i product alone */
void micro_m5(real *out)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&a);
	micro_fill(&b);
	out[0] = a.i * b.j;
	return;
}

/* m6: the full cross-product shape from two locals */
void micro_m6(real_vector3d *result)
{
	real_vector3d a;
	real_vector3d b;

	micro_fill(&a);
	micro_fill(&b);
	cross_product3d(&a, &b, result);
	return;
}
