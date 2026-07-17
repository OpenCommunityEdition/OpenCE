/*
COLLISIONS.C

symbols in this file:
0013D2D0 0030:
	_collision_model_get_material_type (0000)
0013D300 0060:
	_collision_test_sphere (0000)
0013D360 0120:
	_code_0013d360 (0000)
0013D480 00b0:
	_collision_test_point (0000)
0013D530 0290:
	_code_0013d530 (0000)
0013D7C0 06d0:
	_collision_test_vector (0000)
0013DE90 0190:
	_collision_test_vector_exit (0000)
0013E020 0170:
	_collision_test_pill (0000)
0013E190 00d0:
	_collision_test_pill_new (0000)
0013E260 0220:
	_code_0013e260 (0000)
0013E480 0200:
	_collision_get_features_in_sphere (0000)
0013E680 0040:
	_code_0013e680 (0000)
0013E6C0 0040:
	_code_0013e6c0 (0000)
0013E700 0080:
	_code_0013e700 (0000)
0013E780 0050:
	_code_0013e780 (0000)
0013E7D0 00a0:
	_code_0013e7d0 (0000)
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

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

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
boolean code_0013d360(
	long object_index,
	unsigned long flags,
	real_point3d const *point,
	long ignore_object_index);
boolean collision_get_features_in_sphere(
	unsigned long flags,
	real_point3d const *center,
	real enclosing_radius,
	real height,
	real radius,
	long ignore_object_index,
	struct collision_feature_list *features);
short collision_move_point(
	real_point3d const *position,
	real_vector3d const *velocity,
	struct collision_feature_list const *features,
	real_point3d *clipped_position,
	real_vector3d *clipped_velocity,
	struct collision_result *collisions,
	short *collision_count);

/* ---------- globals */

extern boolean debug_collision_skip_objects;

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
	real radius)
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
			if (code_0013d360(object_index, collision_flags, point, ignore_object_index))
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

	point_from_line3d(point, vector, collision->t, &collision->point);
	scenario_location_from_point(&collision->location, &collision->point);

	return hit;
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

	point_from_line3d(point, vector, collision->t, &collision->point);
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
	struct collision_result *collisions,
	short *collision_count)
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
		collisions,
		collision_count);
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
	struct collision_result *collisions,
	short *collision_count)
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
			collisions,
			collision_count);
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

boolean code_0013d360(
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

				if ((FLAG(object_type) & _object_mask_vehicle) &&
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
					code_0013d360(
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
