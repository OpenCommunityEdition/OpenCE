/*
COLLISION_FEATURES.H

header included in hcex build.
*/

#ifndef __COLLISION_FEATURES_H
#define __COLLISION_FEATURES_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"

/* ---------- constants */

enum
{
	MAXIMUM_COLLISION_FEATURES_PER_TEST = 256,
};

enum collision_feature_type
{
	_collision_feature_sphere,
	_collision_feature_cylinder,
	_collision_feature_prism,
	NUMBER_OF_COLLISION_FEATURE_TYPES,
};

/* ---------- macros */

/* ---------- structures */

struct collision_feature_common
{
	long source_index;
	long object_index;
	byte flags;
	byte breakable_surface_index;
	short material_index;
};

struct collision_feature_sphere
{
	struct collision_feature_common common;
	union real_point3d center;
	real radius;
};

struct collision_feature_cylinder
{
	struct collision_feature_common common;
	union real_point3d point;
	union real_vector3d height;
	real radius;
};

struct collision_feature_prism
{
	struct collision_feature_common common;
	byte data[0x5C];
};

struct collision_feature_list
{
	short count[NUMBER_OF_COLLISION_FEATURE_TYPES];
	word pad;
	struct collision_feature_sphere spheres[MAXIMUM_COLLISION_FEATURES_PER_TEST];
	struct collision_feature_cylinder cylinders[MAXIMUM_COLLISION_FEATURES_PER_TEST];
	struct collision_feature_prism prisms[MAXIMUM_COLLISION_FEATURES_PER_TEST];
};

typedef char collision_feature_common_size_assert[
	sizeof(struct collision_feature_common) == 0x0C ? 1 : -1];
typedef char collision_feature_sphere_size_assert[
	sizeof(struct collision_feature_sphere) == 0x1C ? 1 : -1];
typedef char collision_feature_cylinder_size_assert[
	sizeof(struct collision_feature_cylinder) == 0x28 ? 1 : -1];
typedef char collision_feature_prism_size_assert[
	sizeof(struct collision_feature_prism) == 0x68 ? 1 : -1];
typedef char collision_feature_list_spheres_offset_assert[
	offsetof(struct collision_feature_list, spheres) == 0x08 ? 1 : -1];
typedef char collision_feature_list_cylinders_offset_assert[
	offsetof(struct collision_feature_list, cylinders) == 0x1C08 ? 1 : -1];
typedef char collision_feature_list_prisms_offset_assert[
	offsetof(struct collision_feature_list, prisms) == 0x4408 ? 1 : -1];

/* ---------- prototypes/COLLISION_FEATURES.C */

void collision_features_new(
	struct collision_feature_list *features);
void render_debug_collision_sphere(
	const struct collision_feature_sphere *sphere,
	const real_argb_color *color);
void render_debug_collision_cylinder(
	const struct collision_feature_cylinder *cylinder,
	const real_argb_color *color);
void collision_features_from_vertex(
	const struct collision_bsp *collision_bsp,
	long vertex_index,
	const real_matrix4x3 *transform,
	real radius,
	long source_index,
	long object_index,
	struct collision_feature_list *features);
void collision_features_from_surface(
	const struct collision_bsp *collision_bsp,
	long surface_index,
	const real_matrix4x3 *transform,
	real radius,
	long source_index,
	long object_index,
	struct collision_feature_list *features);

/* ---------- globals */

/* ---------- public code */

#endif // __COLLISION_FEATURES_H
