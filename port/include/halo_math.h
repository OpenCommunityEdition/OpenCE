/*
HALO_MATH.H

The same maths functions on every native port (included by the game's
<math.h>: port/linux/include/math.h, port/windows/include/crt/math.h).

In a system link game every machine simulates the players from the same
inputs, and what the host does not correct (projectiles, devices, the
objects it does not send, port/linux/NETCODE.md) only stays the same on
every machine if their floating point results agree to the last bit. The
C libraries' sin, pow and the rest are
not the same bit for bit (glibc on Linux, the UCRT on Windows, musl on
Android), and the compiler folds calls with constant arguments using the
build machine's own. So the game calls halo_ versions instead, built on
every port from the same source (musl's, in port/third_party/musl-math),
and never known to the compiler as builtins.

Every port also compiles with -ffp-contract=off: fusing a multiply and an
add into one FMA instruction (which -march=native and ARM64 do by default)
rounds once instead of twice.

The macros are function-like, so they rename calls (and musl's definitions)
but not variables or fields of the same names. The functions that are
exact in IEEE arithmetic (sqrt, fabs, floor, ceil, fmod) need nothing.
*/

#ifndef __HALO_MATH_H
#define __HALO_MATH_H

double halo_sin(double x);
double halo_cos(double x);
double halo_tan(double x);
double halo_asin(double x);
double halo_acos(double x);
double halo_atan(double x);
double halo_atan2(double y, double x);
double halo_exp(double x);
double halo_log(double x);
double halo_log2(double x);
double halo_log10(double x);
double halo_pow(double x, double y);

#define sin(x) halo_sin(x)
#define cos(x) halo_cos(x)
#define tan(x) halo_tan(x)
#define asin(x) halo_asin(x)
#define acos(x) halo_acos(x)
#define atan(x) halo_atan(x)
#define atan2(y, x) halo_atan2(y, x)
#define exp(x) halo_exp(x)
#define log(x) halo_log(x)
#define log2(x) halo_log2(x)
#define log10(x) halo_log10(x)
#define pow(x, y) halo_pow(x, y)

#endif
