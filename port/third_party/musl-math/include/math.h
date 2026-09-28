/*
MATH.H

The <math.h> the musl sources in ../src see, instead of the system's: only
what they use, with the same types on every port (port/include/halo_math.h
says why). Written for this port, not musl's.
*/

#ifndef __HALO_MUSL_MATH_MATH_H
#define __HALO_MUSL_MATH_MATH_H

/* sin, pow and the rest are defined as the halo_ functions */
#include "../../../include/halo_math.h"

/* no excess precision anywhere (x87, FLT_EVAL_METHOD 2, is not used) */
typedef double double_t;
typedef float float_t;

/* musl chooses an FMA instruction where the target has one; one code path
for every port */
#undef __FP_FAST_FMA
#undef __FP_FAST_FMAF

#define INFINITY __builtin_inff()
#define NAN __builtin_nanf("")
#define HUGE_VAL __builtin_huge_val()

#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define signbit(x) __builtin_signbit(x)

/* exact in IEEE arithmetic, so any implementation gives the same result */
#define fabs(x) __builtin_fabs(x)
#define sqrt(x) __builtin_sqrt(x)
#define floor(x) __builtin_floor(x)
#define scalbn(x, n) __builtin_scalbn(x, n)

#endif
