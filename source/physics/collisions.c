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

#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D
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
#include "game/game_globals.h"
#include "math/real_math_planes.h"
#include "scenario/fog_definitions.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "units/bipeds.h"

#undef REAL_MATH_EXTERNAL_POINT_FROM_LINE3D

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
static boolean object_test_vector(
	long object_index,
	unsigned long flags,
	unsigned long bsp_flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision);
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
boolean debug_collision_skip_objects = FALSE;
boolean debug_collision_skip_vectors = FALSE;

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

boolean collision_test_vector(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision)
{
	boolean hit = FALSE;
	boolean test_environment = TEST_FLAG(flags, _collision_test_structure_bit) ||
		TEST_FLAG(flags, _collision_test_media_bit) ||
		TEST_FLAG(flags, _collision_test_objects_bit);

	if (debug_collision_skip_vectors)
	{
		test_environment = FALSE;
	}

	collision->type = NONE;
	collision->start_location.leaf_index = NONE;
	collision->start_location.cluster_index = NONE;
	collision->location.leaf_index = NONE;
	collision->location.cluster_index = NONE;
	collision->t = 1.0f;

	if (test_environment)
	{
		struct structure_bsp *structure_bsp = global_structure_bsp_get();
		boolean test_objects = TEST_FLAG(flags, _collision_test_objects_bit);
		unsigned long bsp_flags;
		struct collision_bsp_test_vector_result bsp_result;

		if (debug_collision_skip_objects)
		{
			test_objects = FALSE;
		}

		if (!(flags & (FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit))))
		{
			flags |= FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit);
		}

		bsp_flags = 0;
		SET_FLAG(bsp_flags, _collision_test_front_facing_surfaces_bit, TEST_FLAG(flags, _collision_test_front_facing_surfaces_bit));
		SET_FLAG(bsp_flags, _collision_test_back_facing_surfaces_bit, TEST_FLAG(flags, _collision_test_back_facing_surfaces_bit));
		SET_FLAG(bsp_flags, _collision_test_ignore_two_sided_surfaces_bit, TEST_FLAG(flags, _collision_test_ignore_two_sided_surfaces_bit));
		SET_FLAG(bsp_flags, _collision_test_ignore_invisible_surfaces_bit, TEST_FLAG(flags, _collision_test_ignore_invisible_surfaces_bit));
		SET_FLAG(bsp_flags, _collision_test_ignore_breakable_surfaces_bit, TEST_FLAG(flags, _collision_test_ignore_breakable_surfaces_bit));

		collision_log_usage(_collision_function_vector_structure);
		collision_log_start_time(&collision_usage_times.vector_structure);
		if (collision_bsp_test_vector(
			bsp_flags,
			global_collision_bsp_get(),
			MAXIMUM_BREAKABLE_SURFACES_PER_MAP,
			breakable_surface_flags_get(),
			point,
			vector,
			REAL_MAX,
			&bsp_result) &&
			TEST_FLAG(flags, _collision_test_structure_bit))
		{
			long material_type;

			collision->t = bsp_result.t;
			collision->type = _collision_result_structure;
			collision->plane = *bsp_result.plane;
			if (bsp_result.plane_designator & LONG_MIN)
			{
				plane3d_negate(&collision->plane, &collision->plane);
			}

			if (bsp_result.material_index != NONE)
			{
				material_type = TAG_BLOCK_GET_ELEMENT(
					&structure_bsp->collision_materials,
					bsp_result.material_index,
					struct structure_collision_material)->runtime_physics_material_type;
			}
			else
			{
				material_type = NONE;
			}

			collision->material_type = (short)material_type;
			collision->surface_index = bsp_result.surface_index;
			collision->plane_designator = bsp_result.plane_designator;
			collision->flags = bsp_result.flags;
			collision->breakable_surface_index = bsp_result.breakable_surface_index;
			collision->material_index = bsp_result.material_index;
			hit = TRUE;
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
		collision_log_end_time(
			_collision_function_vector_structure,
			collision_usage_times.vector_structure.QuadPart);

		if (TEST_FLAG(flags, _collision_test_media_bit) &&
			collision->location.cluster_index != NONE)
		{
			short fog_reference = TAG_BLOCK_GET_ELEMENT(
				&structure_bsp->clusters,
				collision->location.cluster_index,
				struct structure_cluster)->fog_reference;

			if (fog_reference != NONE && TEST_FLAG((word)fog_reference, 15))
			{
				struct structure_fog_plane const *fog_plane = TAG_BLOCK_GET_ELEMENT(
					&structure_bsp->fog_planes,
					fog_reference & SHORT_MAX,
					struct structure_fog_plane);

				if (fog_plane->runtime_material_type != NONE)
				{
					struct structure_fog_region const *fog_region = TAG_BLOCK_GET_ELEMENT(
						&structure_bsp->fog_regions,
						fog_plane->region_index,
						struct structure_fog_region);
					struct structure_fog_palette_entry const *fog_palette_entry = TAG_BLOCK_GET_ELEMENT(
						&structure_bsp->fog_palette,
						fog_region->fog_palette_index,
						struct structure_fog_palette_entry);
					struct fog_definition const *fog = fog_definition_get(fog_palette_entry->fog.index);
					real_plane3d plane;
					real point_distance;
					real vector_dot;

					plane = fog_plane->plane;
					plane.d -= fog->plane_distance;
					point_distance = plane3d_distance_to_point(&plane, point);
					vector_dot = dot_product3d(&plane.n, vector);

					if ((point_distance > 0.0f) != (vector_dot > 0.0f) &&
						fabs(point_distance) < fabs(vector_dot) &&
						fabs(vector_dot) >= _real_epsilon)
					{
						real t = -(point_distance / vector_dot);

						if (t < collision->t)
						{
							boolean negate = point_distance < 0.0f;
							long material_type;

							collision->t = t;
							collision->plane = plane;
							collision->type = _collision_result_media;
							if (negate)
							{
								plane3d_negate(&collision->plane, &collision->plane);
								material_type = _material_water;
							}
							else
							{
								material_type = fog_plane->runtime_material_type;
							}
							collision->material_type = (short)material_type;
							hit = TRUE;
						}
					}
				}
			}
		}

		if (test_objects && bsp_result.leaf_count > 0)
		{
			long leaf_index;

			collision_log_usage(_collision_function_vector_objects);
			collision_log_start_time(&collision_usage_times.vector_objects);
			if (!(flags & _collision_test_objects_all_types_flags))
			{
				flags |= _collision_test_objects_all_types_flags;
			}

			structure_cluster_marker_begin();
			object_marker_begin();
			for (leaf_index = 0; leaf_index < bsp_result.leaf_count; leaf_index++)
			{
				long leaf = bsp_result.leaf_indices[leaf_index];
				long cluster_index;

				if (leaf == NONE)
				{
					cluster_index = NONE;
				}
				else
				{
					cluster_index = TAG_BLOCK_GET_ELEMENT(
						&global_structure_bsp_get()->leaves,
						leaf & LONG_MAX,
						struct structure_leaf)->cluster_index;
				}

				if (structure_cluster_mark(cluster_index))
				{
					long reference_index;
					long object_index;

					for (object_index = cluster_get_first_collideable_object(&reference_index, cluster_index);
						object_index != NONE;
						object_index = cluster_get_next_collideable_object(&reference_index))
					{
						if (object_mark_function(object_index) &&
							object_test_vector(
								object_index,
								flags,
								bsp_flags,
								point,
								vector,
								ignore_object_index,
								collision))
						{
							hit = TRUE;
						}
					}
				}
			}
			object_marker_end();
			structure_cluster_marker_end();
			collision_log_end_time(
				_collision_function_vector_objects,
				collision_usage_times.vector_objects.QuadPart);
		}

		if (!hit)
		{
			collision->t = 1.0f;
		}

		COLLISION_POINT_FROM_LINE3D(point, vector, collision->t, &collision->point);
		if (TEST_FLAG(flags, _collision_test_try_to_keep_location_valid_bit) &&
			hit &&
			collision->location.leaf_index != NONE &&
			scenario_leaf_index_from_point(&collision->point) != collision->location.leaf_index)
		{
			point_from_line3d(&collision->point, &collision->plane.n, 0.00024414062f, &collision->point);
			scenario_location_from_point(&collision->location, &collision->point);
			if (collision->location.leaf_index == NONE)
			{
				real vector_dot = dot_product3d(vector, &collision->plane.n);
				real step = (vector_dot != 0.0f) ? 0.00024414062f / fabs(vector_dot) : 0.03125f;

				do
				{
					collision->t = MAX(collision->t - step, 0.0f);
					COLLISION_POINT_FROM_LINE3D(point, vector, collision->t, &collision->point);
					scenario_location_from_point(&collision->location, &collision->point);
					if (collision->t <= 0.0f)
					{
						break;
					}
				}
				while (collision->location.leaf_index == NONE);
			}
		}
	}
	else
	{
		collision->t = 1.0f;
		add_vectors3d(
			(real_vector3d const *)point,
			vector,
			(real_vector3d *)&collision->point);
		scenario_location_from_point(&collision->location, &collision->point);
	}

	return hit;
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

		set_real_point3d(
			&reverse_origin,
			vector->i + point->x,
			(vector->j) + point->y,
			vector->k + point->z);
		negate_vector3d(vector, &reverse_vector);

		{
			struct collision_model_instance instance;

			if (collision_model_instance_new(&instance, previous_collision->object_index))
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
					if (result.bsp_result.plane_designator & LONG_MIN)
					{
						plane3d_negate(&exit_collision->plane, &exit_collision->plane);
					}

					exit_collision->material_type = collision_model_get_material_type(
						instance.model,
						result.bsp_result.material_index);
					exit_collision->object_index = previous_collision->object_index;
					exit_collision->region_index = result.region_index;
					exit_collision->node_index = result.node_index;
					exit_collision->bsp_index = result.bsp_index;
					exit_collision->surface_index = result.bsp_result.surface_index;
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
		COLLISION_POINT_FROM_LINE3D(point, vector, exit_collision->t, &exit_collision->point);
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
			short leaf_reference_index;

			if (!(flags & _collision_test_objects_all_types_flags))
			{
				flags |= _collision_test_objects_all_types_flags;
			}

			structure_cluster_marker_begin();
			object_marker_begin();
			for (leaf_reference_index = 0;
				leaf_reference_index < result.leaf_count;
				leaf_reference_index++)
			{
				long leaf_index = result.leaf_indices[leaf_reference_index];
				struct structure_leaf const *leaf = TAG_BLOCK_GET_ELEMENT(
					&structure_bsp->leaves,
					leaf_index & LONG_MAX,
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

	match_assert(
		"c:\\halo\\SOURCE\\physics\\collisions.c",
		0x4F8,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] =
		_collision_user_bipeds;

	{
		real_point3d center;

		set_real_point3d(
			&center,
			old_position->x,
			old_position->y,
			height * 0.5f + old_position->z);
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
		struct collision_plane collision;
		real_point3d fallback;
		boolean have_fallback;
		short offset_index;

		have_fallback = FALSE;
		for (offset_index = 0;
			offset_index < NUMBEROF(collision_fix_pill_offsets);
			offset_index++)
		{
			real_vector3d const *offset = &collision_fix_pill_offsets[offset_index];
			real_point3d candidate;

			candidate.x = offset->i * distance + old_position->x;
			candidate.y = offset->j * distance + old_position->y;
			candidate.z = offset->k * distance + old_position->z;

			if (!collision_features_test_point(&features, &candidate, &collision) &&
				!collision_test_point(flags, &candidate, ignore_object_index))
			{
				real_vector3d ground_vector;

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
			real_vector3d ground_vector;

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

static void clip_position_to_plane(
	real_point3d const *position,
	real_plane3d const *plane,
	real_point3d *clipped_position)
{
	COLLISION_POINT_FROM_LINE3D(
		position,
		&plane->n,
		-plane3d_distance_to_point(plane, position),
		clipped_position);

	return;
}

static void clip_velocity_to_plane(
	real_vector3d const *velocity,
	real_plane3d const *plane,
	real_vector3d *clipped_velocity)
{
	real distance = -dot_product3d(velocity, &plane->n);

	clipped_velocity->i = plane->n.i * distance + velocity->i;
	clipped_velocity->j = plane->n.j * distance + velocity->j;
	clipped_velocity->k = plane->n.k * distance + velocity->k;

	return;
}

static void clip_position_to_line(
	real_point3d const *position,
	real_point3d const *point,
	real_vector3d const *vector,
	real_point3d *clipped_position)
{
	real_vector3d offset;

	vector_from_points3d(point, position, &offset);
	COLLISION_POINT_FROM_LINE3D(
		point,
		vector,
		dot_product3d(&offset, vector) / magnitude_squared3d(vector),
		clipped_position);

	return;
}

static void clip_velocity_to_line(
	real_vector3d const *velocity,
	real_point3d const *point,
	real_vector3d const *vector,
	real_vector3d *clipped_velocity)
{
	scale_vector3d(
		vector,
		dot_product3d(velocity, vector) / magnitude_squared3d(vector),
		clipped_velocity);

	return;
}

short collision_move_point(
	real_point3d const *old_position,
	real_vector3d const *old_velocity,
	struct collision_feature_list const *features,
	real_point3d *new_position,
	real_vector3d *new_velocity,
	short maximum_collision_count,
	struct collision_plane *collisions)
{
	short collision_count = 0;
	real_vector3d velocity = *old_velocity;
	real_point3d clipped_position = *old_position;
	real_vector3d clipped_velocity = *old_velocity;
	short clip_count = 0;
	short clip_indices[3];
	real_plane3d clip_plane;
	real_point3d clip_line_point;
	real_vector3d clip_line_vector;
	real_point3d clip_point;

	match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3AD, old_position);
	match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3AE, old_velocity);
	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3AF, features->count[_collision_feature_sphere]<=MAXIMUM_COLLISION_FEATURES_PER_TEST);
	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3B0, features->count[_collision_feature_cylinder]<=MAXIMUM_COLLISION_FEATURES_PER_TEST);
	match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3B1, features->count[_collision_feature_prism]<=MAXIMUM_COLLISION_FEATURES_PER_TEST);

	while (!(fabs(clipped_velocity.i) < _real_epsilon &&
		fabs(clipped_velocity.j) < _real_epsilon &&
		fabs(clipped_velocity.k) < _real_epsilon))
	{
		struct collision_plane *collision = &collisions[collision_count];

		match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3BF, collision_count<maximum_collision_count);
		match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3C0, &clipped_position);
		match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3C1, &clipped_velocity);

		if (collision_features_test_vector(features, &clipped_position, &clipped_velocity, collision))
		{
			real_point3d position;
			short new_clip_indices[3];
			short new_clip_count = 0;

			collision_count++;
			position = collision->point;
			scale_vector3d(&velocity, 1.0f - collision->t, &velocity);

			match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3CC, &position);
			match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3CD, &velocity);
			match_vassert(
				"c:\\halo\\SOURCE\\physics\\collisions.c",
				0x3CE,
				valid_real_normal3d(&collision->plane.n) && valid_real(collision->plane.d),
				csprintf(
					temporary,
					"%s: assert_valid_real_plane3d(%f, %f, %f / %f)",
					"&collision->plane",
					collision->plane.n.i,
					collision->plane.n.j,
					collision->plane.n.k,
					collision->plane.d));
			match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3D1, clip_count<3);

			new_clip_indices[new_clip_count++] = collision_count - 1;
			clip_plane = collisions[new_clip_indices[0]].plane;
			clip_velocity_to_plane(&velocity, &clip_plane, &clipped_velocity);
			clip_position_to_plane(&position, &clip_plane, &clipped_position);

			if (clip_count > 0)
			{
				if (dot_product3d(&clipped_velocity, &collisions[clip_indices[0]].plane.n) < -_real_epsilon &&
					line_from_planes3d(
						&collisions[new_clip_indices[0]].plane,
						&collisions[clip_indices[0]].plane,
						&clip_line_point,
						&clip_line_vector))
				{
					match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3E1, &clip_line_point);
					match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3E2, &clip_line_vector);

					new_clip_indices[new_clip_count++] = clip_indices[0];
					clip_velocity_to_line(&velocity, &clip_line_point, &clip_line_vector, &clipped_velocity);
					clip_position_to_line(&position, &clip_line_point, &clip_line_vector, &clipped_position);

					if (clip_count > 1 &&
						dot_product3d(&clipped_velocity, &collisions[clip_indices[1]].plane.n) < -_real_epsilon &&
						point_from_planes3d(
							&collisions[new_clip_indices[0]].plane,
							&collisions[new_clip_indices[1]].plane,
							&collisions[clip_indices[1]].plane,
							&clip_point))
					{
						match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3EE, &clip_point);

						new_clip_indices[new_clip_count++] = clip_indices[1];
						clipped_velocity.i = 0.0f;
						clipped_velocity.j = 0.0f;
						clipped_velocity.k = 0.0f;
						clipped_position = clip_point;
					}
				}
				else if (clip_count > 1 &&
					dot_product3d(&clipped_velocity, &collisions[clip_indices[1]].plane.n) < -_real_epsilon &&
					line_from_planes3d(
						&collisions[new_clip_indices[0]].plane,
						&collisions[clip_indices[1]].plane,
						&clip_line_point,
						&clip_line_vector))
				{
					match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3FE, &clip_line_point);
					match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x3FF, &clip_line_vector);

					new_clip_indices[new_clip_count++] = clip_indices[1];
					clip_velocity_to_line(&velocity, &clip_line_point, &clip_line_vector, &clipped_velocity);
					clip_position_to_line(&position, &clip_line_point, &clip_line_vector, &clipped_position);
				}
			}

			match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x40A, &clipped_position);
			match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x40B, &clipped_velocity);

			clip_count = new_clip_count;
			csmemcpy(clip_indices, new_clip_indices, clip_count * sizeof(short));
		}
		else
		{
			clipped_position = collision->point;
			match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x418, &clipped_position);
			break;
		}

		if (collision_count >= maximum_collision_count)
		{
			break;
		}
	}

	*new_position = clipped_position;
	switch (clip_count)
	{
	case 0:
		*new_velocity = *old_velocity;
		break;

	case 1:
		match_vassert(
			"c:\\halo\\SOURCE\\physics\\collisions.c",
			0x428,
			valid_real_plane3d(&clip_plane),
			csprintf(
				temporary,
				"%s: assert_valid_real_plane3d(%f, %f, %f / %f)",
				"&clip_plane",
				clip_plane.n.i,
				clip_plane.n.j,
				clip_plane.n.k,
				clip_plane.d));
		clip_velocity_to_plane(old_velocity, &clip_plane, new_velocity);
		break;

	case 2:
		match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x42D, &clip_line_point);
		match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x42E, &clip_line_vector);
		clip_velocity_to_line(old_velocity, &clip_line_point, &clip_line_vector, new_velocity);
		break;

	case 3:
		match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x433, &clip_point);
		new_velocity->i = 0.0f;
		new_velocity->j = 0.0f;
		new_velocity->k = 0.0f;
		break;

	default:
		match_assert("c:\\halo\\SOURCE\\physics\\collisions.c", 0x438, !"unreachable");
	}

	match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x43B, new_position);
	match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x43C, new_velocity);

	if (clip_count > 1 && collision_count < maximum_collision_count)
	{
		struct collision_plane *collision = &collisions[collision_count++];
		struct collision_plane const *last_collision = &collisions[clip_indices[clip_count - 1]];
		real minimum_k = 0.0f;
		short steepest_clip_index = NONE;
		short clip_index;

		collision->t = last_collision->t;
		collision->point = last_collision->point;
		collision->object_index = NONE;
		collision->surface_index = NONE;
		collision->flags = 0;
		collision->breakable_surface_index = 0;
		collision->material_index = NONE;

		for (clip_index = 0; clip_index < clip_count; clip_index++)
		{
			if (collisions[clip_indices[clip_index]].plane.n.k < minimum_k)
			{
				minimum_k = collisions[clip_indices[clip_index]].plane.n.k;
				steepest_clip_index = clip_index;
			}
		}

		if (clip_count == 2)
		{
			if (steepest_clip_index != NONE)
			{
				struct collision_plane const *steepest = &collisions[clip_indices[steepest_clip_index]];

				if (steepest_clip_index == 0)
				{
					cross_product3d(&clip_line_vector, &steepest->plane.n, &collision->plane.n);
				}
				else
				{
					cross_product3d(&steepest->plane.n, &clip_line_vector, &collision->plane.n);
				}
			}
			else
			{
				COLLISION_POINT_FROM_LINE3D(
					(real_point3d const *)global_up3d,
					&clip_line_vector,
					-(clip_line_vector.k / magnitude_squared3d(&clip_line_vector)),
					(real_point3d *)&collision->plane.n);
			}

			if (normalize3d(&collision->plane.n) != 0.0f)
			{
				collision->plane.d = dot_product3d((real_vector3d *)&clip_line_point, &collision->plane.n);
			}
			else
			{
				collision_count--;
			}
		}
		else
		{
			if (steepest_clip_index != NONE)
			{
				struct collision_plane const *steepest = &collisions[clip_indices[steepest_clip_index]];

				COLLISION_POINT_FROM_LINE3D(
					(real_point3d const *)global_up3d,
					&steepest->plane.n,
					-steepest->plane.n.k,
					(real_point3d *)&collision->plane.n);
				if (normalize3d(&collision->plane.n) != 0.0f)
				{
					collision->plane.d = dot_product3d((real_vector3d *)&clip_point, &collision->plane.n);
				}
				else
				{
					collision_count--;
				}
			}
			else
			{
				collision->plane.n = *global_up3d;
				collision->plane.d = dot_product3d((real_vector3d *)&clip_point, &collision->plane.n);
			}
		}
	}

	match_assert_valid_real_point3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x497, new_position);
	match_assert_valid_real_vector3d("c:\\halo\\SOURCE\\physics\\collisions.c", 0x498, new_velocity);

	return collision_count;
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

static boolean object_test_vector(
	long object_index,
	unsigned long flags,
	unsigned long bsp_flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision)
{
	boolean hit = FALSE;

	do
	{
		struct object_datum const *object = object_get(object_index);

		if (object_index != ignore_object_index &&
			!TEST_FLAG(object->object.flags, _object_invisible_bit) &&
			TEST_FLAG(flags, object->object.type + _collision_test_objects_first_type_bit) &&
			fast_vector_intersects_sphere(
				point,
				vector,
				&object->object.bounding_sphere_center,
				(object->object.bounding_sphere_radius)))
		{
			if (TEST_FLAG(_object_mask_vehicle, object->object.type) &&
				TEST_FLAG(flags, _collision_test_use_vehicle_physics_bit))
			{
				struct physics_instance instance;
				struct physics_test_vector_result result;

				if (physics_instance_new(&instance, object_index) &&
					physics_test_vector(&instance, point, vector, &result) &&
					collision->t > result.t)
				{
					collision->type = _collision_result_object;
					collision->t = result.t;
					collision->plane = result.plane;
					collision->material_type = NONE;
					collision->object_index = object_index;
					collision->region_index = NONE;
					collision->node_index = NONE;
					collision->bsp_index = NONE;
					collision->surface_index = NONE;
					collision->plane_designator = NONE;
					collision->flags = 0;
					collision->breakable_surface_index = 0;
					collision->material_index = NONE;
					hit = TRUE;
				}
			}
			else
			{
				struct collision_model_instance instance;
				struct collision_model_test_vector_result result;

				if (collision_model_instance_new(&instance, object_index) &&
					collision_model_test_vector(&instance, bsp_flags, point, vector, &result) &&
					collision->t > result.bsp_result.t)
				{
					collision->type = _collision_result_object;
					collision->t = result.bsp_result.t;
					matrix4x3_transform_plane(
						&instance.matrices[result.node_index],
						result.bsp_result.plane,
						&collision->plane);
					if (result.bsp_result.plane_designator & LONG_MIN)
					{
						plane3d_negate(&collision->plane, &collision->plane);
					}

					collision->material_type = (short)collision_model_get_material_type(
						instance.model,
						result.bsp_result.material_index);
					collision->object_index = object_index;
					collision->region_index = result.region_index;
					collision->node_index = result.node_index;
					collision->bsp_index = result.bsp_index;
					collision->surface_index = result.bsp_result.surface_index;
					collision->plane_designator = result.bsp_result.plane_designator;
					collision->flags = result.bsp_result.flags;
					collision->breakable_surface_index = result.bsp_result.breakable_surface_index;
					collision->material_index = result.bsp_result.material_index;
					hit = TRUE;
				}
			}

			if (object->object.first_child_object_index != NONE &&
				object_test_vector(
					object->object.first_child_object_index,
					flags,
					bsp_flags,
					point,
					vector,
					ignore_object_index,
					collision))
			{
				hit = TRUE;
			}
		}

		object_index = object->object.next_object_index;
	}
	while (object_index != NONE);

	return hit;
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
	if (collision->t > 0.0f)
	{
		real_point3d *contact = &collision->point;

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
