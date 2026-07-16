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
#include "collision_bsp.h"
#include "collision_model_definitions.h"
#include "scenario/scenario.h"

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

/* ---------- globals */

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

boolean collision_move_sphere(
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

/* ---------- private code */
