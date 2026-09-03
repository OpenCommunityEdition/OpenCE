/*
RENDER_DEBUG_GEOMETRY.H

Narrow geometry interface owned by RENDER_DEBUG.C.
*/

#ifndef __RENDER_DEBUG_GEOMETRY_H
#define __RENDER_DEBUG_GEOMETRY_H
#pragma once

/* ---------- prototypes/RENDER_DEBUG.C */

void render_debug_triangle(
	boolean immediate,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2,
	real_argb_color const *color);

#endif // __RENDER_DEBUG_GEOMETRY_H
