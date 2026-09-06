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

void error_geometry_initialize(
	void);

void error_geometry_dispose(
	void);

void error_geometry_set_name(
	char const *name);

void error_geometry_point(
	real_point3d const *point,
	real_argb_color const *color);

void error_geometry_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real_argb_color const *color);

void error_geometry_triangle(
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2,
	real_argb_color const *color);

void error_geometry_polygon(
	short point_count,
	real_point3d const *points,
	real_argb_color const *color);

void error_geometry_rectangle3d(
	real_rectangle3d const *bounds,
	real_argb_color const *color);

void error_geometry_bounded_point(
	real_point3d const *point,
	real radius,
	real_argb_color const *color);

void error_geometry_bounded_line(
	real_point3d const *p0,
	real_point3d const *p1,
	real radius,
	real_argb_color const *color);

void error_geometry_bounded_polygon(
	short point_count,
	real_point3d const *points,
	real radius,
	real_argb_color const *color);

void error_geometry_comment(
	char const *format,
	...);

#endif // __ERROR_GEOMETRY_H
