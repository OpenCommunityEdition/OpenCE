#ifndef HALO_MAC_NATIVE_GEOMETRY_MATH_H
#define HALO_MAC_NATIVE_GEOMETRY_MATH_H
/* Scoped to vertex compression, whose inputs are finite. Fixed nearest-even
   matches the engine's default FISTP contract without changing the process FP
   environment or importing Darwin's rounding routines. */
double mac_geometry_floor(double);
double mac_geometry_rint(double);
#define floor(x) mac_geometry_floor(x)
#define __builtin_rint(x) mac_geometry_rint(x)
#endif
