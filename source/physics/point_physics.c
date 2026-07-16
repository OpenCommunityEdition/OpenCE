/*
POINT_PHYSICS.C

symbols in this file:
001440B0 0030:
	_point_physics_initialize_for_new_map (0000)
001440E0 0010:
	_point_physics_dispose_from_old_map (0000)
001440F0 0020:
	_point_physics_definition_get_mass (0000)
00144110 0160:
	_point_physics_definition_interpolate (0000)
00144270 0030:
	_code_00144270 (0000)
001442A0 0570:
	_point_physics_update (0000)
0028BE84 0004:
	__real@47e7aaac (0000)
0028BE88 0011:
	??_C@_0BB@FCDBFPJL@t?$DO?$DN0?4f?5?$CG?$CG?5t?$DM?$DN1?4f?$AA@ (0000)
0028BE9C 0009:
	??_C@_08BIGLLGLE@physics2?$AA@ (0000)
0028BEA8 0009:
	??_C@_08DDEGOFHH@physics1?$AA@ (0000)
0028BEB4 0027:
	??_C@_0CH@PCKPBNOM@c?3?2halo?2SOURCE?2physics?2point_phy@ (0000)
0028BEDC 0004:
	__real@3ba3d70a (0000)
0028BEE0 000c:
	??_C@_0M@FKGDOMDE@radius?$DO?$DN0?4f?$AA@ (0000)
0028BEF0 0041:
	??_C@_0EB@EKLOAHE@?$CBtranslational_force?5?$HM?$HM?5valid_re@ (0000)
0028BF34 0017:
	??_C@_0BH@HLBAAAIL@translational_velocity?$AA@ (0000)
0030CEE0 000c:
	_global_point_physics_reference (0000)
0045E01C 0008:
	_bss_0045e01c (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"

#define valid_real_point3d valid_real_point3d_inline
#define valid_real_vector3d valid_real_vector3d_inline
#include "physics/point_physics.h"
#undef valid_real_vector3d
#undef valid_real_point3d

#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "render/render_debug.h"
#include "scenario/wind.h"

/* ---------- constants */

#define POINT_PHYSICS_DENSITY_SCALE 118613.344f

enum
{
	_point_physics_flamethrower_collision_bit,
	_point_physics_collides_with_structures_bit,
	_point_physics_collides_with_water_surface_bit,
	_point_physics_uses_simple_wind_bit,
	_point_physics_uses_damped_wind_bit,
	_point_physics_no_gravity_bit,
};

enum
{
	_point_physics_result_air_bit,
	_point_physics_result_water_bit,
	_point_physics_result_water_collision_bit,
	_point_physics_result_structure_collision_bit,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

boolean valid_real_point3d(
	real_point3d const *point);
boolean valid_real_vector3d(
	real_vector3d const *vector);

static void code_00144270(
	struct point_physics_definition const *definition,
	real_point3d const *position,
	real radius);

/* ---------- globals */

extern real global_air_density;
extern real global_gravity;
extern real global_water_density;
extern boolean debug_point_physics;

struct point_physics_globals point_physics_globals;

struct tag_reference_definition global_point_physics_reference =
{
	0,
	POINT_PHYSICS_TAG,
	NULL,
};

/* ---------- public code */

void point_physics_initialize_for_new_map(
	void)
{
	point_physics_globals.air_density= global_air_density * POINT_PHYSICS_DENSITY_SCALE;
	point_physics_globals.water_density= global_water_density * POINT_PHYSICS_DENSITY_SCALE;

	return;
}

void point_physics_dispose_from_old_map(
	void)
{
	return;
}

real point_physics_definition_get_mass(
	struct point_physics_definition const *definition,
	real scale)
{
	return scale * definition->density * scale * scale;
}

struct point_physics_definition *point_physics_definition_interpolate(
	struct point_physics_definition const *physics1,
	struct point_physics_definition const *physics2,
	real t,
	struct point_physics_definition *result)
{
	real one_minus_t = 1.f - t;

	match_assert("c:\\halo\\SOURCE\\physics\\point_physics.c", 332, physics1);
	match_assert("c:\\halo\\SOURCE\\physics\\point_physics.c", 333, physics2);
	match_assert("c:\\halo\\SOURCE\\physics\\point_physics.c", 334, t>=0.f && t<=1.f);
	match_assert("c:\\halo\\SOURCE\\physics\\point_physics.c", 335, result);

	result->flags = physics1->flags;
	result->unknown20 = t * physics2->unknown20 + one_minus_t * physics1->unknown20;
	result->water_gravity_scale = t * physics2->water_gravity_scale + one_minus_t * physics1->water_gravity_scale;
	result->air_gravity_scale = t * physics2->air_gravity_scale + one_minus_t * physics1->air_gravity_scale;
	result->density = t * physics2->density + one_minus_t * physics1->density;
	result->air_friction = t * physics2->air_friction + one_minus_t * physics1->air_friction;
	result->water_friction = t * physics2->water_friction + one_minus_t * physics1->water_friction;
	result->surface_friction = t * physics2->surface_friction + one_minus_t * physics1->surface_friction;
	result->elasticity = t * physics2->elasticity + one_minus_t * physics1->elasticity;

	return result;
}

static void code_00144270(
	struct point_physics_definition const *definition,
	real_point3d const *position,
	real radius)
{
	render_debug_point(
		TRUE,
		position,
		radius,
		TEST_FLAG(definition->flags, _point_physics_collides_with_structures_bit)
			? global_real_argb_red
			: global_real_argb_green);

	return;
}

unsigned long point_physics_update(
	unsigned long flags,
	struct point_physics_definition const *definition,
	struct location *location,
	long weather_palette_index,
	real_point3d *position,
	real_vector3d *translational_velocity,
	real_vector3d const *translational_force,
	real_vector3d *collision_normal,
	short *material_type,
	real radius,
	real delta_time)
{
	unsigned long result_flags = 0;
	unsigned long current_flags;
	unsigned long collision_flags;
	boolean in_water;
	short collision_count;
	real radius_squared;
	real radius_cubed;
	real mass;
	real force_scale;
	real friction;
	real gravity_scale;
	real gravity_acceleration;
	real friction_fraction;
	real collision_radius;
	real_vector3d current;
	real_vector3d movement;
	real_vector3d parallel;
	real_vector3d perpendicular;
	struct collision_result collision;

	match_assert_valid_real_point3d(
		"c:\\halo\\SOURCE\\physics\\point_physics.c",
		185,
		position);
	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\point_physics.c",
		186,
		translational_velocity);
	match_assert(
		"c:\\halo\\SOURCE\\physics\\point_physics.c",
		187,
		!translational_force || valid_real_vector3d(translational_force));
	match_assert(
		"c:\\halo\\SOURCE\\physics\\point_physics.c",
		188,
		radius>=0.f);

	if (delta_time != 0.f)
	{
		mass = definition->density;
		radius_squared = radius * radius;
		radius_cubed = radius_squared * radius;
		current_flags = TEST_FLAG(definition->flags, _point_physics_uses_simple_wind_bit);
		SET_FLAG(
			current_flags,
			1,
			TEST_FLAG(definition->flags, _point_physics_uses_damped_wind_bit));

		if (TEST_FLAG(flags, 0))
		{
			in_water = TEST_FLAG(flags, 1);
			scenario_get_current_from_weather_palette(
				position,
				&current,
				current_flags,
				(short)weather_palette_index);
		}
		else
		{
			in_water = scenario_get_current(
				location,
				position,
				&current,
				current_flags);
		}

		if (in_water)
		{
			result_flags = FLAG(_point_physics_result_water_bit);
			mass += point_physics_globals.water_density;
			gravity_scale = definition->water_gravity_scale;
			friction = radius_squared * definition->water_friction;
		}
		else
		{
			result_flags = FLAG(_point_physics_result_air_bit);
			mass += point_physics_globals.air_density;
			gravity_scale = definition->air_gravity_scale;
			friction = radius_squared * definition->air_friction;
		}

		mass = radius_cubed * mass;
		force_scale = delta_time / mass;

		if (TEST_FLAG(definition->flags, _point_physics_no_gravity_bit))
			gravity_scale = 0.f;

		if (translational_force && mass != 0.f)
		{
			translational_velocity->i += translational_force->i * force_scale;
			translational_velocity->j += translational_force->j * force_scale;
			translational_velocity->k += translational_force->k * force_scale;
		}

		gravity_acceleration = global_gravity * (real)TICKS_PER_SECOND;
		gravity_acceleration *= (real)TICKS_PER_SECOND;
		translational_velocity->k += gravity_acceleration * gravity_scale * delta_time;

		if (mass == 0.f)
		{
			if (friction == 0.f)
				friction_fraction = 0.f;
			else
				friction_fraction = 1.f;
		}
		else
		{
			friction_fraction = PIN(force_scale * friction, 0.f, 1.f);
		}

		translational_velocity->i +=
			(current.i - translational_velocity->i) * friction_fraction;
		translational_velocity->j +=
			(current.j - translational_velocity->j) * friction_fraction;
		translational_velocity->k +=
			(current.k - translational_velocity->k) * friction_fraction;

		if (TEST_FLAG(definition->flags, _point_physics_collides_with_water_surface_bit) &&
			!TEST_FLAG(flags, 2))
		{
			collision_flags = FLAG(_collision_test_front_facing_surfaces_bit) |
				FLAG(_collision_test_media_bit);
		}
		else
		{
			collision_flags = FLAG(_collision_test_front_facing_surfaces_bit);
		}
		if (TEST_FLAG(definition->flags, _point_physics_collides_with_structures_bit) &&
			!TEST_FLAG(flags, 2))
		{
			collision_flags |= FLAG(_collision_test_structure_bit);
		}
		else
		{
			collision_flags &= ~FLAG(_collision_test_structure_bit);
		}

		match_assert(
			"c:\\halo\\SOURCE\\physics\\point_physics.c",
			269,
			global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
		global_current_collision_users[global_current_collision_user_depth++] = 13;

		collision_count = 0;
		while (delta_time != 0.f && collision_count < 3)
		{
			movement.i = translational_velocity->i * delta_time;
			movement.j = translational_velocity->j * delta_time;
			movement.k = translational_velocity->k * delta_time;

			if (collision_test_vector(
				collision_flags,
				position,
				&movement,
				NONE,
				&collision))
			{
				collision_radius = MIN(radius, 0.005f);
				if (collision.type == 0)
					result_flags |= FLAG(_point_physics_result_structure_collision_bit);
				else if (collision.type == 2)
					result_flags |= FLAG(_point_physics_result_water_collision_bit);

				if (collision_normal)
					*collision_normal = collision.plane.n;
				if (material_type)
					*material_type = (short)collision.material_type;

				component_vectors_from_normal3d(
					translational_velocity,
					&collision.plane.n,
					&parallel,
					&perpendicular);

				translational_velocity->i =
					(1.f - definition->surface_friction) * perpendicular.i -
					definition->elasticity * parallel.i;
				translational_velocity->j =
					(1.f - definition->surface_friction) * perpendicular.j -
					definition->elasticity * parallel.j;
				translational_velocity->k =
					(1.f - definition->surface_friction) * perpendicular.k -
					definition->elasticity * parallel.k;

				if (collision.location.leaf_index != NONE)
					*location = collision.location;

				position->x = collision.point.x + collision.plane.n.i * collision_radius;
				position->y = collision.point.y + collision.plane.n.j * collision_radius;
				position->z = collision.point.z + collision.plane.n.k * collision_radius;
				delta_time -= collision.t * delta_time;
				collision_count++;
			}
			else
			{
				if (collision.location.leaf_index != NONE)
					*location = collision.location;
				*position = collision.point;
				break;
			}
		}

		match_assert(
			"c:\\halo\\SOURCE\\physics\\point_physics.c",
			312,
			global_current_collision_user_depth > 1);
		--global_current_collision_user_depth;
	}

	if (debug_point_physics)
		code_00144270(definition, position, radius);

	return result_flags;
}

/* ---------- private code */
