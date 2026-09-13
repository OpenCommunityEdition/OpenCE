/*
COLLISIONS.C

symbols in this file:
0013D2D0 0030:
	_collision_model_get_material_type (0000)
0013D300 0060:
	_collision_test_sphere (0000)
0013D360 0120:
	_object_test_point (0000)
0013D480 00b0:
	_collision_test_point (0000)
0013D530 0290:
	_object_test_vector (0000)
0013D7C0 06d0:
	_collision_test_vector (0000)
0013DE90 0190:
	_collision_test_vector_exit (0000)
0013E020 0170:
	_collision_test_pill (0000)
0013E190 00d0:
	_collision_test_pill_new (0000)
0013E260 0220:
	_object_get_features_in_sphere (0000)
0013E480 0200:
	_collision_get_features_in_sphere (0000)
0013E680 0040:
	_clip_position_to_plane (0000)
0013E6C0 0040:
	_clip_velocity_to_plane (0000)
0013E700 0080:
	_clip_position_to_line (0000)
0013E780 0050:
	_clip_velocity_to_line (0000)
0013E7D0 00a0:
	_collision_fix_pill_nudge_collision (0000)
0013E870 02a0:
	_collision_fix_pill (0000)
0013EB10 1290:
	_collision_move_point (0000)
0013FDA0 0180:
	_collision_move_pill (0000)
0013FF20 0040:
	_collision_move_sphere (0000)
0028BAB8 0008:
	__real@3fa0000000000000 (0000)
0028BAC0 0008:
	__real@3f30000000000000 (0000)
0028BAC8 0004:
	__real@3d000000 (0000)
0028BACC 0004:
	__real@3f441b7d (0000)
0028BAD0 0024:
	??_C@_0CE@BAGBGBHP@c?3?2halo?2SOURCE?2physics?2collision@ (0000)
0028BAF4 000d:
	??_C@_0N@LANGNGIH@new_velocity?$AA@ (0000)
0028BB04 000c:
	??_C@_0M@HGHPBCFN@?$CGclip_plane?$AA@ (0000)
0028BB10 000c:
	??_C@_0M@KHLCGDHC@?$CGclip_point?$AA@ (0000)
0028BB1C 0012:
	??_C@_0BC@GFMKCLPO@?$CGclip_line_vector?$AA@ (0000)
0028BB30 0011:
	??_C@_0BB@IPCKNOKM@?$CGclip_line_point?$AA@ (0000)
0028BB44 000d:
	??_C@_0N@HNOBANOD@clip_count?$DM3?$AA@ (0000)
0028BB54 002f:
	??_C@_0CP@BCIJNMNG@?$CFs?3?5assert_valid_real_plane3d?$CI?$CFf@ (0000)
0028BB84 0012:
	??_C@_0BC@KMBJNJFM@?$CGcollision?9?$DOplane?$AA@ (0000)
0028BB98 000a:
	??_C@_09MFIJNMKH@?$CGvelocity?$AA@ (0000)
0028BBA4 000a:
	??_C@_09KHKMDFEK@?$CGposition?$AA@ (0000)
0028BBB0 0012:
	??_C@_0BC@JCEJOAFB@?$CGclipped_velocity?$AA@ (0000)
0028BBC4 0012:
	??_C@_0BC@PAGMAJLM@?$CGclipped_position?$AA@ (0000)
0028BBD8 0028:
	??_C@_0CI@OLDCDOIO@collision_count?$DMmaximum_collisio@ (0000)
0028BC00 000d:
	??_C@_0N@MHPNJHFJ@old_velocity?$AA@ (0000)
0028BC10 000d:
	??_C@_0N@KFNIHOLE@old_position?$AA@ (0000)
0030CDE8 00cc:
	_data_0030cde8 (0000)
0045E000 001a:
	_bss_0045e000 (0000)
	_debug_collision_skip_objects (0018)
	_debug_collision_skip_vectors (0019)
*/

/* ---------- headers */

#include "cseries.h"
#include "collisions.h"

#include "breakable_surfaces.h"
#include "bsp3d.h"
#include "collision_bsp.h"
#include "collision_features.h"
#include "collision_model_definitions.h"
#include "collision_models.h"
#include "collision_usage.h"
#include "physics.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "units/bipeds.h"

/* ---------- constants */

/* ---------- macros */

/* Preserve January's in-TU scalar expansion without selecting the external
 * point_from_line3d COMDAT for this object. */
#define COLLISION_POINT_FROM_LINE3D(point, vector, distance, result) \
	do \
	{ \
		real_point3d *line_result = (result); \
		real line_distance = (distance); \
		real_vector3d const *line_vector = (vector); \
		real_point3d const *line_point = (point); \
		line_result->x = line_vector->i * line_distance + line_point->x; \
		line_result->y = line_vector->j * line_distance + line_point->y; \
		line_result->z = line_vector->k * line_distance + line_point->z; \
	} while (0)

/* ---------- structures */

struct collision_usage_times
{
	LARGE_INTEGER features;
	LARGE_INTEGER vector_objects;
	LARGE_INTEGER vector_structure;
};

/* ---------- prototypes */

boolean collision_bsp_test_pill_new(
	struct collision_bsp const *bsp,
	short breakable_surface_count,
	byte const *breakable_surface_flags,
	real_point3d const *point,
	real_vector3d const *vector,
	real radius,
	real *t,
	real_vector3d *normal);
boolean collision_bsp_test_pill(
	struct collision_bsp const *bsp,
	real_point3d const *point,
	real_vector3d const *vector,
	real radius,
	real maximum_t,
	struct collision_bsp_test_pill_result *result);
static boolean object_test_point(
	long object_index,
	unsigned long flags,
	real_point3d const *point,
	long ignore_object_index);
boolean collision_test_vector_exit(
	struct collision_result const *previous_collision,
	real_point3d const *point,
	real_vector3d const *vector,
	struct collision_result *exit_collision);
static void object_get_features_in_sphere(
	unsigned long flags,
	long object_index,
	real_point3d const *center,
	real radius,
	real height,
	real width,
	long ignore_object_index,
	struct collision_feature_list *features);
static void collision_fix_pill_nudge_collision(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_plane *collision);
/* ---------- globals */

static real_vector3d collision_fix_pill_offsets[17] =
{
	{ { -1.0f, 0.0f, 0.0f } },
	{ { 1.0f, 0.0f, 0.0f } },
	{ { 0.0f, -1.0f, 0.0f } },
	{ { 0.0f, 1.0f, 0.0f } },
	{ { -0.70710677f, -0.70710677f, 0.0f } },
	{ { 0.70710677f, 0.70710677f, 0.0f } },
	{ { 0.70710677f, -0.70710677f, 0.0f } },
	{ { -0.70710677f, 0.70710677f, 0.0f } },
	{ { 0.0f, 0.0f, 1.0f } },
	{ { -0.70710677f, 0.0f, 0.70710677f } },
	{ { 0.70710677f, 0.0f, 0.70710677f } },
	{ { 0.0f, -0.70710677f, 0.70710677f } },
	{ { 0.0f, 0.70710677f, 0.70710677f } },
	{ { -0.57735026f, -0.57735026f, 0.57735026f } },
	{ { 0.57735026f, 0.57735026f, 0.57735026f } },
	{ { 0.57735026f, -0.57735026f, 0.57735026f } },
	{ { -0.57735026f, 0.57735026f, 0.57735026f } },
};

static struct collision_usage_times collision_usage_times;
boolean debug_collision_skip_objects;
boolean debug_collision_skip_vectors;

/* ---------- public code */

long collision_model_get_material_type(
	struct collision_model const *model,
	short material_index)
{
	long material_type;

	if (material_index != NONE)
	{
		material_type = TAG_BLOCK_GET_ELEMENT(
			&model->resistance.materials,
			material_index,
			struct damage_resistance_material)->material_type;
	}
	else
	{
		material_type = NONE;
	}

	return material_type;
}

boolean collision_test_sphere(
	real_point3d const *center,
	real radius,
	long ignore_object_index)
{
	boolean result;
	struct collision_bsp_test_sphere_result sphere_result;

	if (bsp3d_test_point(global_bsp3d_get(), 0, center) != NONE)
	{
		result = collision_bsp_test_sphere(
			global_collision_bsp_get(),
			MAXIMUM_BREAKABLE_SURFACES_PER_MAP,
			breakable_surface_flags_get(),
			center,
			radius,
			&sphere_result);
		if (!result)
		{
			return result;
		}
	}

	return TRUE;
}

boolean collision_test_point(
	unsigned long flags,
	real_point3d const *point,
	long ignore_object_index)
{
	unsigned long collision_flags = flags;
	long leaf_index;
	long reference_index;
	boolean test_objects;

	if (!(collision_flags & _collision_test_environment_flags))
	{
		goto no_collision;
	}

	leaf_index = bsp3d_test_point(global_bsp3d_get(), 0, point);
	test_objects = TEST_FLAG(collision_flags, _collision_test_objects_bit);
	if (debug_collision_skip_objects)
	{
		test_objects = FALSE;
	}

	if (leaf_index == NONE)
	{
		goto collision;
	}

	if (!test_objects)
	{
		goto no_collision;
	}

	{
		short cluster_index = TAG_BLOCK_GET_ELEMENT(
			&global_structure_bsp_get()->leaves,
			leaf_index & LONG_MAX,
			struct structure_leaf)->cluster_index;
		long object_index;

		for (object_index = cluster_get_first_collideable_object(&reference_index, cluster_index);
			object_index != NONE;
			object_index = cluster_get_next_collideable_object(&reference_index))
		{
			if (object_test_point(object_index, collision_flags, point, ignore_object_index))
			{
				goto collision;
			}
		}
	}

no_collision:
	return FALSE;

collision:
	return TRUE;
}

boolean collision_test_pill(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	real radius,
	long ignore_object_index,
	struct collision_result *collision)
{
	boolean hit = FALSE;
	struct collision_bsp_test_pill_result bsp_result;

	collision->type = NONE;
	collision->t = REAL_MAX;

	if (collision_bsp_test_pill(
		global_collision_bsp_get(),
		point,
		vector,
		radius,
		REAL_MAX,
		&bsp_result))
	{
		collision->t = bsp_result.t;
		if (flags & FLAG(_collision_test_structure_bit))
		{
			collision->plane = bsp_result.plane;
			collision->flags = 0;
			collision->breakable_surface_index = 0;
			collision->type = 2;
			collision->material_type = bsp_result.material_index;
			collision->surface_index = bsp_result.surface_index;
			collision->plane_designator = NONE;
			collision->material_index = bsp_result.material_index;
			hit = TRUE;
		}
	}

	if (bsp_result.leaf_count > 0)
	{
		long leaf_index = bsp_result.leaf_indices[0];
		long cluster_index;

		collision->start_location.leaf_index = leaf_index;
		if (leaf_index == NONE)
		{
			cluster_index = NONE;
		}
		else
		{
			cluster_index = TAG_BLOCK_GET_ELEMENT(
				&global_structure_bsp_get()->leaves,
				leaf_index & LONG_MAX,
				struct structure_leaf)->cluster_index;
		}
		collision->start_location.cluster_index = cluster_index;

		leaf_index = bsp_result.leaf_indices[bsp_result.leaf_count - 1];
		collision->location.leaf_index = leaf_index;
		if (leaf_index == NONE)
		{
			cluster_index = NONE;
		}
		else
		{
			cluster_index = TAG_BLOCK_GET_ELEMENT(
				&global_structure_bsp_get()->leaves,
				leaf_index & LONG_MAX,
				struct structure_leaf)->cluster_index;
		}
		collision->location.cluster_index = cluster_index;
	}

	if (!hit)
	{
		collision->t = 1.0f;
	}

	COLLISION_POINT_FROM_LINE3D(point, vector, collision->t, &collision->point);
	scenario_location_from_point(&collision->location, &collision->point);

	return hit;
}

boolean collision_test_vector_exit(
	struct collision_result const *previous_collision,
	real_point3d const *point,
	real_vector3d const *vector,
	struct collision_result *exit_collision)
{
	boolean found = FALSE;

	exit_collision->type = NONE;
	exit_collision->t = REAL_MAX;

	if (previous_collision->type == _collision_result_object)
	{
		real_point3d reverse_origin;
		real_vector3d reverse_vector;

		reverse_origin.x = point->x + vector->i;
		reverse_origin.y = point->y + vector->j;
		reverse_origin.z = point->z + vector->k;
		reverse_vector.i = -vector->i;
		reverse_vector.j = -vector->j;
		reverse_vector.k = -vector->k;

		{
			long object_index;
			struct collision_model_instance instance;

			object_index = previous_collision->object_index;
			if (collision_model_instance_new(&instance, object_index))
			{
				struct collision_model_test_vector_result result;

				if (collision_model_test_vector(
					&instance,
					FLAG(_collision_test_front_facing_surfaces_bit),
					&reverse_origin,
					&reverse_vector,
					&result))
				{
					exit_collision->type = _collision_result_object;
					exit_collision->t = 1.0f - result.bsp_result.t;
					matrix4x3_transform_plane(
						&instance.matrices[result.node_index],
						result.bsp_result.plane,
						&exit_collision->plane);
					if (result.bsp_result.plane_designator < 0)
					{
						plane3d_negate(&exit_collision->plane, &exit_collision->plane);
					}

					exit_collision->material_type = collision_model_get_material_type(
						instance.model,
						result.bsp_result.material_index);
					exit_collision->region_index = result.region_index;
					exit_collision->node_index = result.node_index;
					exit_collision->bsp_index = result.bsp_index;
					exit_collision->surface_index = result.bsp_result.surface_index;
					exit_collision->object_index = previous_collision->object_index;
					exit_collision->plane_designator = result.bsp_result.plane_designator;
					exit_collision->flags = result.bsp_result.flags;
					exit_collision->breakable_surface_index = result.bsp_result.breakable_surface_index;
					exit_collision->material_index = result.bsp_result.material_index;
					found = TRUE;
				}
			}
		}
	}

	if (found)
	{
		exit_collision->point.x = vector->i * exit_collision->t + point->x;
		exit_collision->point.y = vector->j * exit_collision->t + point->y;
		exit_collision->point.z = vector->k * exit_collision->t + point->z;
	}

	return found;
}

boolean collision_get_features_in_sphere(
	unsigned long flags,
	real_point3d const *center,
	real radius,
	real height,
	real width,
	long ignore_object_index,
	struct collision_feature_list *features)
{
	unsigned long structure_flags;

	collision_features_new(features);
	structure_flags = flags & FLAG(_collision_test_structure_bit);
	if (structure_flags ||
		(flags & (FLAG(_collision_test_media_bit) | FLAG(_collision_test_objects_bit))))
	{
		struct structure_bsp const *structure_bsp = global_structure_bsp_get();
		struct collision_bsp const *collision_bsp = global_collision_bsp_get();
		boolean objects = TEST_FLAG(flags, _collision_test_objects_bit);
		struct collision_bsp_test_sphere_result result;

		if (debug_collision_skip_objects)
		{
			objects = FALSE;
		}

		collision_log_usage(_collision_function_vector_bounds_object);
		collision_log_start_time(&collision_usage_times.features);
		radius += 0.0625f;

		if (collision_bsp_test_sphere(
			collision_bsp,
			MAXIMUM_BREAKABLE_SURFACES_PER_MAP,
			breakable_surface_flags_get(),
			center,
			radius,
			&result) &&
			structure_flags)
		{
			collision_bsp_get_features_in_sphere(
				collision_bsp,
				&result,
				NULL,
				height,
				width,
				NONE,
				features);
		}

		if (objects && result.leaf_count > 0)
		{
			short leaf_index;

			if (!(flags & _collision_test_objects_all_types_flags))
			{
				flags |= _collision_test_objects_all_types_flags;
			}

			structure_cluster_marker_begin();
			object_marker_begin();
			for (leaf_index = 0; leaf_index < result.leaf_count; leaf_index++)
			{
				struct structure_leaf const *leaf = TAG_BLOCK_GET_ELEMENT(
					&structure_bsp->leaves,
					result.leaf_indices[leaf_index] & LONG_MAX,
					struct structure_leaf);

				if (structure_cluster_mark(leaf->cluster_index))
				{
					long reference_index;
					long object_index;

					for (object_index = cluster_get_first_collideable_object(
							&reference_index,
							leaf->cluster_index);
						object_index != NONE;
						object_index = cluster_get_next_collideable_object(&reference_index))
					{
						if (object_mark_function(object_index))
						{
							object_get_features_in_sphere(
								flags,
								object_index,
								center,
								radius,
								height,
								width,
								ignore_object_index,
								features);
						}
					}
				}
			}
			object_marker_end();
			structure_cluster_marker_end();
		}

		collision_log_end_time(
			_collision_function_vector_bounds_object,
			collision_usage_times.features.QuadPart);
	}

	return features->count[_collision_feature_sphere] != 0 ||
		features->count[_collision_feature_cylinder] != 0 ||
		features->count[_collision_feature_prism] != 0;
}

boolean collision_fix_pill(
	unsigned long flags,
	real_point3d const *old_position,
	real distance,
	real height,
	real width,
	long ignore_object_index,
	real_point3d *new_position)
{
	boolean result = FALSE;
	struct collision_feature_list features;
	struct collision_plane first_collision;
	struct collision_plane collision;
	real_vector3d ground_vector;
	real_point3d candidate;
	real_point3d fallback;
	boolean have_fallback;
	short offset_index;

	match_assert(
		"c:\\halo\\SOURCE\\physics\\collisions.c",
		0x4F8,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] =
		_collision_user_bipeds;

	{
		real_point3d center;

		center.x = old_position->x;
		center.y = old_position->y;
		center.z = height * 0.5f + old_position->z;
		collision_get_features_in_sphere(
			flags,
			&center,
			height * 0.5f + distance + width,
			height,
			width,
			ignore_object_index,
			&features);
	}

	if (!collision_features_test_point(&features, old_position, &first_collision) &&
		!collision_test_point(flags, old_position, ignore_object_index))
	{
		*new_position = *old_position;
		result = TRUE;
	}
	else
	{
		have_fallback = FALSE;
		for (offset_index = 0;
			offset_index < NUMBEROF(collision_fix_pill_offsets);
			offset_index++)
		{
			real_vector3d const *offset = &collision_fix_pill_offsets[offset_index];

			candidate.x = offset->i * distance + old_position->x;
			candidate.y = offset->j * distance + old_position->y;
			candidate.z = offset->k * distance + old_position->z;

			if (!collision_features_test_point(&features, &candidate, &collision) &&
				!collision_test_point(flags, &candidate, ignore_object_index))
			{
				ground_vector.i = global_down3d->i * distance;
				ground_vector.j = global_down3d->j * distance;
				ground_vector.k = global_down3d->k * distance;
				if (collision_features_test_vector(
						&features,
						&candidate,
						&ground_vector,
						&collision) &&
					collision.plane.n.k > 0.76604444f)
				{
					collision_fix_pill_nudge_collision(
						flags,
						&candidate,
						&ground_vector,
						ignore_object_index,
						&collision);
					*new_position = collision.point;
					result = TRUE;
					break;
				}

				if (!have_fallback)
				{
					fallback = candidate;
					have_fallback = TRUE;
				}
			}
		}

		if (!result && have_fallback)
		{
			ground_vector.i = old_position->x - fallback.x;
			ground_vector.j = old_position->y - fallback.y;
			ground_vector.k = old_position->z - fallback.z;
			collision_features_test_vector(
				&features,
				&fallback,
				&ground_vector,
				&collision);
			collision_fix_pill_nudge_collision(
				flags,
				&fallback,
				&ground_vector,
				ignore_object_index,
				&collision);
			*new_position = collision.point;
			result = TRUE;
		}
	}

	match_assert(
		"c:\\halo\\SOURCE\\physics\\collisions.c",
		0x562,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return result;
}

boolean collision_test_pill_new(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	real radius,
	long ignore_object_index,
	struct collision_result *collision)
{
	boolean result = FALSE;
	real t;
	real_vector3d normal;

	collision->type = NONE;
	collision->start_location.leaf_index = NONE;
	collision->start_location.cluster_index = NONE;
	collision->location.leaf_index = NONE;
	collision->location.cluster_index = NONE;
	collision->t = 1.0f;

	if (collision_bsp_test_pill_new(
		global_collision_bsp_get(),
		0,
		NULL,
		point,
		vector,
		radius,
		&t,
		&normal))
	{
		collision->t = t;
		collision->plane.n = normal;
		collision->type = 2;
		collision->plane.d = REAL_MAX;
		collision->material_type = NONE;
		collision->surface_index = NONE;
		collision->plane_designator = NONE;
		collision->flags = 0;
		collision->breakable_surface_index = 0;
		collision->material_index = NONE;
		result = TRUE;
	}

	COLLISION_POINT_FROM_LINE3D(point, vector, collision->t, &collision->point);
	collision->plane.n.i = 0.0f;
	collision->plane.n.j = 0.0f;
	collision->plane.n.k = 0.0f;

	return result;
}

short collision_move_sphere(
	unsigned long flags,
	real_point3d const *position,
	real_vector3d const *velocity,
	real radius,
	long ignore_object_index,
	real_point3d *clipped_position,
	real_vector3d *clipped_velocity,
	short maximum_collision_count,
	struct collision_plane *collisions)
{
	return collision_move_pill(
		flags,
		position,
		velocity,
		0.0f,
		radius,
		ignore_object_index,
		clipped_position,
		clipped_velocity,
		maximum_collision_count,
		collisions);
}

short collision_move_pill(
	unsigned long flags,
	real_point3d const *position,
	real_vector3d const *velocity,
	real height,
	real radius,
	long ignore_object_index,
	real_point3d *clipped_position,
	real_vector3d *clipped_velocity,
	short maximum_collision_count,
	struct collision_plane *collisions)
{
	short result = FALSE;
	struct collision_feature_list features;
	real_point3d center;

	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 1214, global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = 7;

	center.x = position->x + velocity->i * 0.5f;
	center.y = position->y + velocity->j * 0.5f;
	center.z = height * 0.5f + (position->z + velocity->k * 0.5f);

	if (collision_get_features_in_sphere(
		flags,
		&center,
		magnitude3d(velocity) * 0.5f + height * 0.5f + radius,
		height,
		radius,
		ignore_object_index,
		&features))
	{
		result = collision_move_point(
			position,
			velocity,
			&features,
			clipped_position,
			clipped_velocity,
			maximum_collision_count,
			collisions);
	}
	else
	{
		add_vectors3d(
			(real_vector3d const *)position,
			velocity,
			(real_vector3d *)clipped_position);
		*clipped_velocity = *velocity;
	}

	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 1230, global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return result;
}

/* ---------- private code */

static boolean object_test_point(
	long object_index,
	unsigned long flags,
	real_point3d const *point,
	long ignore_object_index)
{
	do
	{
		struct object_datum const *object = object_get(object_index);

		if (object_index != ignore_object_index &&
			!(object->object.flags & FLAG(_object_invisible_bit)))
		{
			long object_type = object->object.type;

			if ((flags & FLAG(object_type + _collision_test_objects_first_type_bit)) &&
				point_in_sphere(point, &object->object.bounding_sphere_center, object->object.bounding_sphere_radius))
			{
				boolean hit;

				if (TEST_FLAG(_object_mask_vehicle, object_type) &&
					(flags & FLAG(_collision_test_use_vehicle_physics_bit)))
				{
					struct physics_instance instance;

					hit = physics_instance_new(&instance, object_index) &&
						physics_test_point(&instance, point);
				}
				else
				{
					struct collision_model_instance instance;

					hit = collision_model_instance_new(&instance, object_index) &&
						collision_model_test_point(&instance, point);
				}

				if (hit)
				{
					return TRUE;
				}

				if (object->object.first_child_object_index != NONE &&
					object_test_point(
						object->object.first_child_object_index,
						flags,
						point,
						ignore_object_index))
				{
					return TRUE;
				}
			}
		}

		object_index = object->object.next_object_index;
	}
	while (object_index != NONE);

	return FALSE;
}

static void object_get_features_in_sphere(
	unsigned long flags,
	long object_index,
	real_point3d const *center,
	real radius,
	real height,
	real width,
	long ignore_object_index,
	struct collision_feature_list *features)
{
	do
	{
		struct object_datum *object = object_get(object_index);

		if (object_index != ignore_object_index &&
			!TEST_FLAG(object->object.flags, _object_invisible_bit) &&
			!TEST_FLAG(object->object.flags, _object_no_collisions_bit) &&
			(!TEST_FLAG(object->object.damage_flags, _object_dead_bit) ||
				object->object.type != _object_type_biped))
		{
			real reach = object->object.bounding_sphere_radius;

			reach += radius;
			if (point_in_sphere(
				center,
				&object->object.bounding_sphere_center,
				reach))
			{
				if (TEST_FLAG(flags, object->object.type + _collision_test_objects_first_type_bit))
				{
					switch (object->object.type)
					{
					case _object_type_biped:
						{
							struct biped_datum *biped = (struct biped_datum *)object;

							if ((!TEST_FLAG(flags, _collision_test_skip_passthrough_bipeds_bit) ||
								!TEST_FLAG(biped->biped.flags, _biped_movement_passes_through_bipeds_bit)) &&
								(object->object.parent_object_index == NONE ||
									biped->unit.parent_seat_index == NONE))
							{
								real_point3d base;
								real pill_height;
								real pill_width;

								biped_get_physics_pill(
									object_index,
									&base,
									&pill_height,
									&pill_width);
								base.z += pill_height;
								collision_features_from_point(
									&base,
									pill_height + height,
									pill_width + width,
									object_index,
									NONE,
									0,
									(byte)NONE,
									NONE,
									features);
							}
						}
						break;

					case _object_type_vehicle:
					case _object_type_scenery:
					case _object_type_machine:
					case _object_type_control:
						if (TEST_FLAG(_object_mask_vehicle, object->object.type) &&
							TEST_FLAG(flags, _collision_test_use_vehicle_physics_bit))
						{
							struct physics_instance instance;

							if (physics_instance_new(&instance, object_index))
							{
								physics_get_features_in_sphere(
									&instance,
									center,
									radius,
									height,
									width,
									features);
							}
						}
						else
						{
							struct collision_model_instance instance;

							if (collision_model_instance_new(&instance, object_index))
							{
								collision_model_get_features_in_sphere(
									&instance,
									center,
									radius,
									height,
									width,
									features);
							}
						}
						break;
					}
				}

				if (object->object.first_child_object_index != NONE)
				{
					object_get_features_in_sphere(
						flags,
						object->object.first_child_object_index,
						center,
						radius,
						height,
						width,
						ignore_object_index,
						features);
				}
			}
		}

		object_index = object->object.next_object_index;
	}
	while (object_index != NONE);

	return;
}

static void collision_fix_pill_nudge_collision(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_plane *collision)
{
	real_point3d *contact = &collision->point;

	if (collision->t > 0.0f)
	{
		do
		{
			real t;

			if (!collision_test_point(flags, contact, ignore_object_index))
			{
				break;
			}

			t = collision->t - 0.03125f;
			collision->t = t;
			contact->x = vector->i * t + point->x;
			contact->y = vector->j * t + point->y;
			contact->z = vector->k * t + point->z;
		}
		while (collision->t > 0.0f);
	}

	if (collision->t <= 0.0f)
	{
		collision->point = *point;
	}

	return;
}
