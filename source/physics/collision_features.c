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
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "render/render_debug.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void collision_features_from_point(
	const real_point3d *point,
	real depth,
	real radius,
	long source_index,
	long object_index,
	byte flags,
	byte breakable_surface_index,
	short material_index,
	struct collision_feature_list *features);
void collision_features_from_polygon(
	short point_count,
	const real_point3d *points,
	const real_plane3d *plane,
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

void collision_features_from_point(
	const real_point3d *point,
	real depth,
	real radius,
	long source_index,
	long object_index,
	byte flags,
	byte breakable_surface_index,
	short material_index,
	struct collision_feature_list *features)
{
	short sphere_index = features->count[_collision_feature_sphere];

	if (sphere_index < MAXIMUM_COLLISION_FEATURES_PER_TEST)
	{
		struct collision_feature_sphere *sphere = &features->spheres[sphere_index];
		features->count[_collision_feature_sphere]++;
		sphere->common.source_index = source_index;
		sphere->common.object_index = object_index;
		sphere->common.flags = flags;
		sphere->common.material_index = material_index;
		sphere->common.breakable_surface_index = breakable_surface_index;
		sphere->center = *point;
		sphere->radius = radius;
	}

	if (depth > 0.0f)
	{
		real lower_z = point->z - depth;
		sphere_index = features->count[_collision_feature_sphere];
		if (sphere_index < MAXIMUM_COLLISION_FEATURES_PER_TEST)
		{
			struct collision_feature_sphere *sphere = &features->spheres[sphere_index];
			features->count[_collision_feature_sphere]++;
			sphere->common.source_index = source_index;
			sphere->common.object_index = object_index;
			sphere->common.flags = flags;
			sphere->common.breakable_surface_index = breakable_surface_index;
			sphere->common.material_index = material_index;
			set_real_point3d(&sphere->center, point->x, point->y, lower_z);
			sphere->radius = radius;
		}

		{
			short cylinder_index = features->count[_collision_feature_cylinder];
			if (cylinder_index < MAXIMUM_COLLISION_FEATURES_PER_TEST)
			{
				struct collision_feature_cylinder *cylinder = &features->cylinders[cylinder_index];
				features->count[_collision_feature_cylinder]++;
				cylinder->common.source_index = source_index;
				cylinder->common.object_index = object_index;
				cylinder->common.flags = flags;
				cylinder->common.breakable_surface_index = breakable_surface_index;
				cylinder->common.material_index = material_index;
				set_real_point3d(&cylinder->point, point->x, point->y, lower_z);
				set_real_vector3d(&cylinder->height, 0.0f, 0.0f, depth);
				cylinder->radius = radius;
			}
		}
	}

	return;
}

void collision_features_from_vertex(
	const struct collision_bsp *collision_bsp,
	long vertex_index,
	const real_matrix4x3 *transform,
	real depth,
	real radius,
	long source_index,
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
	long object_index;
	const real_point3d *point;
	real_point3d transformed_point;
	if (source_index != NONE)
		object_index = NONE;
	else
		object_index = edge->surface_indices[0];

	if (transform)
		point = matrix4x3_transform_point(transform, &vertex->point, &transformed_point);
	else
		point = &vertex->point;

	collision_features_from_point(
		point,
		depth,
		radius,
		source_index,
		object_index,
		surface->flags,
		surface->breakable_surface_index,
		surface->material_index,
		features);

	return;
}

void collision_features_from_surface(
	const struct collision_bsp *collision_bsp,
	long surface_index,
	const real_matrix4x3 *transform,
	real radius,
	long source_index,
	long object_index,
	struct collision_feature_list *features)
{
	const struct collision_surface *surface = TAG_BLOCK_GET_ELEMENT(
		&collision_bsp->surfaces,
		surface_index,
		struct collision_surface);
	real_point3d points[MAXIMUM_VERTICES_PER_COLLISION_SURFACE];
	real_plane3d plane;
	short point_count = collision_surface_polygon(
		collision_bsp,
		surface_index,
		points);
	long feature_surface_index;

	bsp3d_get_plane_from_designator(
		(const struct bsp3d *)collision_bsp,
		surface->plane_designator,
		&plane);

	if (transform)
	{
		short remaining_point_count = point_count;
		if (remaining_point_count > 0)
		{
			real_point3d *point = points;

			while (remaining_point_count-- > 0)
			{
				matrix4x3_transform_point(transform, point, point);
				point++;
			}
		}
		matrix4x3_transform_plane(transform, &plane, &plane);
	}

	if (object_index != NONE)
		feature_surface_index = NONE;
	else
		feature_surface_index = surface_index;

	collision_features_from_polygon(
		point_count,
		points,
		&plane,
		radius,
		source_index,
		object_index,
		feature_surface_index,
		surface->flags,
		surface->breakable_surface_index,
		surface->material_index,
		features);

	return;
}

/* ---------- private code */
