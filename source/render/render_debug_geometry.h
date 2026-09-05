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
void render_debug_quadrilateral(
	boolean immediate,
	real_point3d const *point0,
	real_point3d const *point1,
	real_point3d const *point2,
	real_point3d const *point3,
	real_argb_color const *color);
void render_debug_box2d_outline(
	boolean immediate,
	real_rectangle2d const *bounds,
	real_argb_color const *color);
void render_debug_polygon(
	real_point3d const *points,
	short point_count,
	real_argb_color const *color);
void render_debug_polygon_edges(
	real_point3d const *points,
	short point_count,
	real_argb_color const *color);

#endif // __RENDER_DEBUG_GEOMETRY_H
