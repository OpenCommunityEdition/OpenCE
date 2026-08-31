/*
PROJECTILES.C

symbols in this file:
000E7460 0010:
	_projectiles_initialize (0000)
000E7470 0010:
	_projectiles_initialize_for_new_map (0000)
000E7480 0010:
	_projectiles_dispose_from_old_map (0000)
000E7490 0010:
	_projectiles_dispose (0000)
000E74A0 0030:
	_projectile_kill_tracer (0000)
000E74D0 0040:
	_projectiles_delete_all (0000)
000E7510 0010:
	_projectile_delete (0000)
000E7520 0020:
	_projectile_set_target_object_index (0000)
000E7540 0030:
	_projectile_make_tracer (0000)
000E7570 0020:
	_projectile_get_ballistic_acceleration (0000)
000E7590 0030:
	_projectile_estimate_time_to_target (0000)
000E75C0 0040:
	_dangerous_projectiles_near_player (0000)
000E7600 0030:
	_projectile_handle_deleted_object (0000)
000E7630 0020:
	_code_000e7630 (0000)
000E7650 0060:
	_code_000e7650 (0000)
000E76B0 00e0:
	_projectile_export_function_values (0000)
000E7790 0060:
	_code_000e7790 (0000)
000E77F0 0070:
	_projectile_handle_parent_destroyed (0000)
000E7860 0030:
	_random_vector_in_cone3d (0000)
000E7890 0370:
	_projectile_aim_ballistic (0000)
000E7C00 00c0:
	_projectile_aim_linear (0000)
000E7CC0 00c0:
	_projectile_aim (0000)
000E7D80 00b0:
	_code_000e7d80 (0000)
000E7E30 00e0:
	_code_000e7e30 (0000)
000E7F10 0200:
	_code_000e7f10 (0000)
000E8110 0410:
	_code_000e8110 (0000)
000E8520 01b0:
	_projectile_new (0000)
000E86D0 01f0:
	_projectile_accelerate (0000)
000E88C0 0b70:
	_code_000e88c0 (0000)
000E9430 0fe0:
	_projectile_update (0000)
00279028 000b:
	??_C@_0L@IKCANML@reflection?$AA@ (0000)
00279034 0012:
	??_C@_0BC@MIGMEKLO@negative?5incident?$AA@ (0000)
00279048 0009:
	??_C@_08NAKCOKKJ@incident?$AA@ (0000)
00279054 0012:
	??_C@_0BC@IEANEAFD@projectile_update?$AA@ (0000)
00279068 0004:
	_rdata_00279068 (0000)
0027906C 0023:
	??_C@_0CD@LFGANAAE@c?3?2halo?2SOURCE?2items?2projectiles@ (0000)
00279090 002f:
	??_C@_0CP@JLELNEII@projectile?9?$DOobject?4parent_object@ (0000)
002790C0 0012:
	??_C@_0BC@KMKCONEA@result_aim_vector?$AA@ (0000)
002790D4 0014:
	??_C@_0BE@NNDLGLKI@v_desired_sq?5?$DO?50?40f?$AA@ (0000)
002790E8 0016:
	??_C@_0BG@HPLEINHD@t_squared_max?5?$DO?$DN?50?40f?$AA@ (0000)
00279100 0014:
	??_C@_0BE@BBPDBBBA@4?40f?5?$CK?5a?5?$CK?5c?5?$DO?50?40f?$AA@ (0000)
00279114 002b:
	??_C@_0CL@KDJFFEJL@?$CGprojectile?9?$DOobject?4translationa@ (0000)
00279140 000d:
	??_C@_0N@OKFAMOHD@acceleration?$AA@ (0000)
00279150 0004:
	__real@3c088889 (0000)
00279154 0004:
	__real@3f22f983 (0000)
00279158 001e:
	??_C@_0BO@CBMNHFJH@collision?9?$DOobject_index?$CB?$DNNONE?$AA@ (0000)
00279178 001c:
	??_C@_0BM@JNPELLAN@?$CGprojectile?9?$DOobject?4forward?$AA@ (0000)
00279194 0017:
	??_C@_0BH@CAHIPIOP@?$CGprojectile?9?$DOobject?4up?$AA@ (0000)
002791AC 000e:
	??_C@_0O@DJJKDDEB@?$CGnew_position?$AA@ (0000)
002791BC 002a:
	??_C@_0CK@HFBLHMOF@projectile?5velocity?5is?5bad?5after@ (0000)
002791E8 002f:
	??_C@_0CP@HBPNPJKN@projectile?5velocity?5is?5bad?5after@ (0000)
00279218 0004:
	__real@3f7d70a4 (0000)
0027921C 002b:
	??_C@_0CL@BHNFKMBE@projectile?5velocity?5is?5bad?5after@ (0000)
00306B30 060c:
	_data_00306b30 (0000)
*/

/* ---------- headers */

#define normalize3d normalize3d_inline
#define random_vector_in_cone3d random_vector_in_cone3d_inline
#include "cseries.h"
#include "projectiles.h"
#undef normalize3d
#undef random_vector_in_cone3d

#include "projectile_definitions.h"

#include "objects/objects.h"
#include "physics/physics.h"

/* ---------- constants */

enum projectile_datum_flags
{
	_projectile_has_nonzero_angular_velocity_bit = 0,
	_projectile_tracer_bit = 1,
	_projectile_collided_once_bit = 2,
	_projectile_attached_bit = 3,
	_projectile_stopped_after_collision_bit = 4,
	_projectile_counting_down_bit = 5,
	_projectile_already_super_exploded_bit = 6,
	_projectile_will_super_explode_bit = 7,
	NUMBER_OF_PROJECTILE_DATUM_FLAGS,
};

enum projectile_definition_flags
{
	_projectile_oriented_along_velocity_bit = 0,
	_projectile_aim_ballistic_bit = 1,
	_projectile_detonation_max_time_if_attached_bit = 2,
	_projectile_super_combining_explosion_bit = 3,
	_projectile_combine_initial_velocity_with_parent_velocity_bit = 4,
	_projectile_random_detonation_time_when_attached_bit = 5,
	_projectile_minimum_unattached_detonation_time = 6,
	NUMBER_OF_PROJECTILE_DEFINITION_FLAGS,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

real normalize3d(
	real_vector3d *v);

boolean projectile_aim_ballistic(
	real base_velocity,
	real gravity_scale,
	real_point3d const *origin,
	real_point3d const *target_point,
	real *target_velocity_min,
	real *target_ballistic_fraction_min,
	real *forced_velocity,
	boolean lob,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance,
	real *result_vertical_velocity,
	real *result_horizontal_velocity);

boolean projectile_aim_linear(
	real base_velocity,
	real_point3d const *origin,
	real_point3d const *target_point,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance);

/* ---------- globals */

/* ---------- public code */

void projectiles_initialize(
	void)
{
	return;
}

void projectiles_initialize_for_new_map(
	void)
{
	return;
}

void projectiles_dispose_from_old_map(
	void)
{
	return;
}

void projectiles_dispose(
	void)
{
	return;
}

void projectile_kill_tracer(
	long projectile_index)
{
	struct projectile_datum *projectile;

	projectile = projectile_get(projectile_index);
	projectile->projectile.flags &= ~FLAG(_projectile_tracer_bit);

	return;
}

void projectiles_delete_all(
	void)
{
	struct object_iterator iterator;

	object_iterator_new(&iterator, _object_mask_projectile, 0);
	while (object_iterator_next(&iterator))
	{
		object_delete(iterator.index);
	}

	return;
}

void projectile_delete(
	long projectile_index)
{
	return;
}

void projectile_set_target_object_index(
	long projectile_index,
	long target_object_index)
{
	struct projectile_datum *projectile;

	projectile = projectile_get(projectile_index);
	projectile->projectile.target_object_index = target_object_index;

	return;
}

void projectile_make_tracer(
	long projectile_index)
{
	struct projectile_datum *projectile;

	projectile = projectile_get(projectile_index);
	projectile->projectile.flags |= FLAG(_projectile_tracer_bit);

	return;
}

void projectile_handle_deleted_object(
	long projectile_index,
	long deleted_object_index)
{
	struct projectile_datum *projectile;

	projectile = projectile_get(projectile_index);
	if (projectile->projectile.target_object_index == deleted_object_index)
		projectile->projectile.target_object_index = NONE;

	return;
}

boolean projectile_handle_parent_destroyed(
	long projectile_index)
{
	struct projectile_datum *projectile;

	projectile = projectile_get(projectile_index);
	match_assert(
		"c:\\halo\\SOURCE\\items\\projectiles.c",
		1845,
		projectile->object.parent_object_index != NONE);
	projectile->projectile.arming_time = 1.0f;
	projectile->projectile.detonation_timer = 1.0f;
	projectile->projectile.flags &= ~FLAG(_projectile_attached_bit);
	object_detach(projectile_index);

	return TRUE;
}

boolean dangerous_projectiles_near_player(
	void)
{
	struct object_iterator iterator;
	struct object_datum *projectile;

	object_iterator_new(&iterator, _object_mask_projectile, 0);
	projectile = (struct object_datum *)object_iterator_next(&iterator);
	if (projectile)
	{
		projectile_definition_get(projectile->definition_index);
		return TRUE;
	}

	return FALSE;
}

/* ---------- private code */

real projectile_get_ballistic_acceleration(
	struct projectile_definition const *definition)
{
	return -(definition->projectile.air_gravity_scale * global_gravity);
}

real projectile_estimate_time_to_target(
	struct projectile_definition const *definition,
	real target_distance)
{
	real time_to_target = 0.0f;

	if (definition->projectile.initial_velocity > 0.0f)
		time_to_target = target_distance / definition->projectile.initial_velocity;

	return time_to_target;
}

real_vector3d *random_vector_in_cone3d(
	real_vector3d const *axis,
	real inner_cone_angle,
	real outer_cone_angle,
	real_vector3d *result)
{
	return seed_random_vector_in_cone3d(
		get_global_random_seed_address(),
		axis,
		inner_cone_angle,
		outer_cone_angle,
		result);
}

boolean projectile_aim_linear(
	real base_velocity,
	real_point3d const *origin,
	real_point3d const *target_point,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance)
{
	real_vector3d aim_vector;
	real distance;
	real ticks;

	aim_vector.i = target_point->x - origin->x;
	aim_vector.j = target_point->y - origin->y;
	aim_vector.k = target_point->z - origin->z;
	distance = normalize3d(&aim_vector);

	if (base_velocity > 0.0f)
		ticks = distance / base_velocity;
	else
		ticks = 0.0f;

	match_assert(
		"c:\\halo\\SOURCE\\items\\projectiles.c",
		921,
		result_aim_vector);
	*result_aim_vector = aim_vector;

	if (result_distance)
		*result_distance = distance;
	if (result_velocity)
		*result_velocity = base_velocity;
	if (result_ticks)
		*result_ticks = ticks;

	return TRUE;
}

boolean projectile_aim(
	struct projectile_definition const *definition,
	real_point3d const *origin,
	real_point3d const *target_point,
	real const *override_velocity_max,
	real *target_velocity_min,
	real *target_ballistic_fraction_min,
	real *forced_velocity,
	boolean lob,
	real_vector3d *result_aim_vector,
	real *result_velocity,
	real *result_ticks,
	real *result_distance,
	boolean *result_linear)
{
	real base_velocity;
	boolean result;

	if (!override_velocity_max)
		base_velocity = definition->projectile.initial_velocity;
	else
		base_velocity = *override_velocity_max;

	if (TEST_FLAG(definition->projectile.flags, _projectile_aim_ballistic_bit) &&
		definition->projectile.air_gravity_scale > 0.0f)
	{
		result = projectile_aim_ballistic(
			base_velocity,
			definition->projectile.air_gravity_scale,
			origin,
			target_point,
			target_velocity_min,
			target_ballistic_fraction_min,
			forced_velocity,
			lob,
			result_aim_vector,
			result_velocity,
			result_ticks,
			result_distance,
			NULL,
			NULL);

		if (result_linear)
			*result_linear = FALSE;
	}
	else
	{
		result = projectile_aim_linear(
			base_velocity,
			origin,
			target_point,
			result_aim_vector,
			result_velocity,
			result_ticks,
			result_distance);

		if (result_linear)
			*result_linear = TRUE;
	}

	return result;
}
