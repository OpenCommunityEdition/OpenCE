/*
GEOMETRY.H

header included in hcex build.
*/

#ifndef __GEOMETRY_H
#define __GEOMETRY_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct geosphere
{
	short segment_count;
	short pad2;
	real_point3d *vertices;
	short *triangle_strip_vertex_indices;
	short vertex_count;
	short triangle_count;
	short triangle_strip_count;
	short pad18;
};

/* ---------- prototypes/GEOMETRY.C */

struct geosphere *geosphere_new(
	short segment_count);
void geosphere_dispose(
	struct geosphere *sphere);

short convex_hull2d(
	short point_count,
	real_point2d const *points,
	short *hull_indices);
boolean convex_hull2d_verify(
	short vertex_count,
	real_point2d const *vertices,
	short index_count,
	short const *indices);
boolean convex_hull2d_test_point(
	short count,
	real_point2d const *points,
	real_point2d const *point,
	real epsilon);
boolean convex_hull2d_test_circle(
	short count,
	real_point2d const *points,
	real_point2d const *center,
	real radius);
boolean convex_hull2d_test_point_indexed(
	short count,
	short const *indices,
	real_point2d const *points,
	real_point2d const *point,
	real epsilon);
real convex_hull2d_area(
	short count,
	real_point2d const *vertices);
boolean convex_hull2d_test_vector(
	short count,
	real_point2d const *points,
	real_point2d const *origin,
	real_vector2d const *vector,
	real *minimum_distance,
	real *maximum_distance);
boolean convex_polygon2d_verify(
	short count,
	real *vertices);
short convex_polygon3d_clip_to_plane(
	short vertex_count,
	real_point3d const *vertices,
	real_plane3d const *plane,
	short maximum_vertex_count,
	real_point3d *clipped_vertices,
	short *vertex_indices,
	real epsilon,
	boolean keep_degenerate);
short convex_hull2d_intersect(
	short p_count,
	real_point2d const *p,
	long q_count,
	real_point2d const *q,
	short maximum_count,
	real_point2d *result,
	real epsilon);

/* ---------- globals */

extern real global_convex_hull3d_delta;
extern real global_convex_hull3d_epsilon;

/* ---------- public code */

#endif // __GEOMETRY_H
