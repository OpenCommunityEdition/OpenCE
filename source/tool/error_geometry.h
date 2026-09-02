/*
ERROR_GEOMETRY.H

header included in hcex build.
*/

#ifndef __ERROR_GEOMETRY_H
#define __ERROR_GEOMETRY_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"

/* ---------- prototypes/ERROR_GEOMETRY.C */

void error_geometry_triangle(
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2,
	real_argb_color const *color);

#endif // __ERROR_GEOMETRY_H
