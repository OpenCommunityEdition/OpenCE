/*
COLLISION_FEATURES.C

symbols in this file:
0013A590 0020:
	_collision_features_new (0000)
0013A5B0 0020:
	_render_debug_collision_sphere (0000)
0013A5D0 0030:
	_render_debug_collision_cylinder (0000)
0013A600 0130:
	_collision_features_from_point (0000)
0013A730 0340:
	_collision_features_from_line (0000)
0013AA70 01b0:
	_collision_features_from_polygon (0000)
0013AC20 00a0:
	_collision_features_from_vertex (0000)
0013ACC0 01b0:
	_collision_features_from_edge (0000)
0013AE70 00d0:
	_collision_features_from_surface (0000)
0013AF40 01a0:
	_collision_bsp_get_features_in_sphere (0000)
0013B0E0 00d0:
	_collision_sphere_test_point (0000)
0013B1B0 0180:
	_collision_cylinder_test_point (0000)
0013B330 0130:
	_collision_prism_test_point (0000)
0013B460 01a0:
	_collision_features_test_point (0000)
0013B600 0180:
	_collision_sphere_test_vector (0000)
0013B780 02f0:
	_collision_cylinder_test_vector (0000)
0013BA70 0290:
	_collision_prism_test_vector (0000)
0013BD00 0220:
	_collision_features_test_vector (0000)
0013BF20 00e0:
	_render_debug_collision_prism (0000)
0013C000 0130:
	_render_debug_collision_features (0000)
0028B3D8 005c:
	??_C@_0FM@KALIBNPG@global_projection3d_mappings?$FLpri@ (0000)
0028B434 0030:
	??_C@_0DA@JKDEENK@point_count?$DM?$DNMAXIMUM_POINTS_PER_@ (0000)
0028B464 002c:
	??_C@_0CM@EILEGMIO@c?3?2halo?2SOURCE?2physics?2collision@ (0000)
0028B490 007f:
	??_C@_0HP@FNCENBBI@features?9?$DOcount?$FL_collision_featu@ (0000)
0028B510 0085:
	??_C@_0IF@ELKHLMNC@features?9?$DOcount?$FL_collision_featu@ (0000)
0028B598 0081:
	??_C@_0IB@CFLACBAG@features?9?$DOcount?$FL_collision_featu@ (0000)
0028B61C 0037:
	??_C@_0DH@KHNBPIGP@prism?9?$DOpoint_count?$DM?$DNMAXIMUM_POIN@ (0000)
0028B658 004f:
	??_C@_0EP@MHGDBNFA@features?9?$DOcount?$FL_collision_featu@ (0000)
0028B6A8 0052:
	??_C@_0FC@BDBIHIKB@features?9?$DOcount?$FL_collision_featu@ (0000)
0028B700 0050:
	??_C@_0FA@PGLAPNNA@features?9?$DOcount?$FL_collision_featu@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "collision_features.h"

#include "math/real_math.h"
#include "physics/collision_bsp_definitions.h"
#include "render/render_debug.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void collision_features_from_point(
	const real_point3d *point,
	real radius,
	long source_index,
	long object_index,
	long surface_index,
	byte flags,
	byte breakable_surface_index,
	short material_index,
	struct collision_feature_list *features);

/* ---------- globals */

/* ---------- public code */

void collision_features_new(
	struct collision_feature_list *features)
{
	csmemset(features->count, 0, sizeof(features->count));

	return;
}

void render_debug_collision_sphere(
	const struct collision_feature_sphere *sphere,
	const real_argb_color *color)
{
	render_debug_sphere(TRUE, &sphere->center, sphere->radius, color);

	return;
}

void render_debug_collision_cylinder(
	const struct collision_feature_cylinder *cylinder,
	const real_argb_color *color)
{
	render_debug_cylinder(
		TRUE,
		&cylinder->point,
		&cylinder->height,
		cylinder->radius,
		color);

	return;
}

void collision_features_from_vertex(
	const struct collision_bsp *collision_bsp,
	long vertex_index,
	const real_matrix4x3 *transform,
	real radius,
	long source_index,
	long object_index,
	struct collision_feature_list *features)
{
	const struct collision_vertex *vertex = TAG_BLOCK_GET_ELEMENT(
		&collision_bsp->vertices,
		vertex_index,
		struct collision_vertex);
	const struct collision_edge *edge = TAG_BLOCK_GET_ELEMENT(
		&collision_bsp->edges,
		vertex->first_edge_index,
		struct collision_edge);
	const struct collision_surface *surface = TAG_BLOCK_GET_ELEMENT(
		&collision_bsp->surfaces,
		edge->surface_indices[0],
		struct collision_surface);
	long surface_index;
	const real_point3d *point;
	real_point3d transformed_point;
	if (object_index != NONE)
		surface_index = NONE;
	else
		surface_index = edge->surface_indices[0];

	if (transform)
		point = matrix4x3_transform_point(transform, &vertex->point, &transformed_point);
	else
		point = &vertex->point;

	collision_features_from_point(
		point,
		radius,
		source_index,
		object_index,
		surface_index,
		surface->flags,
		surface->breakable_surface_index,
		surface->material_index,
		features);

	return;
}

/* ---------- private code */
