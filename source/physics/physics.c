/*
PHYSICS.C

symbols in this file:
0013FF60 0080:
	_pin_fraction (0000)
0013FFE0 00b0:
	_physics_get_features_in_sphere (0000)
00140090 0070:
	_code_00140090 (0000)
00140100 0110:
	_render_debug_physics (0000)
00140210 0100:
	_physics_instance_new (0000)
00140310 00a0:
	_physics_test_point (0000)
001403B0 0120:
	_physics_test_vector (0000)
001404D0 0150:
	_code_001404d0 (0000)
00140620 0100:
	_code_00140620 (0000)
00140720 0b80:
	_physics_compute_new (0000)
001412A0 0470:
	_code_001412a0 (0000)
00141710 0490:
	_code_00141710 (0000)
00141BA0 0180:
	_code_00141ba0 (0000)
00141D20 01b0:
	_code_00141d20 (0000)
00141ED0 07c0:
	_physics_update_new (0000)
00142690 1430:
	_code_00142690 (0000)
00143AC0 0260:
	_physics_update (0000)
0028BC20 008d:
	??_C@_0IN@ODNFDOFI@mass_point?9?$DOground_material_type@ (0000)
0028BCB0 0021:
	??_C@_0CB@OHGIJBLL@c?3?2halo?2SOURCE?2physics?2physics?4c@ (0000)
0028BCD4 0004:
	__real@45ea6000 (0000)
0028BCD8 0057:
	??_C@_0FH@DDOACPI@unit_definition?9?$DOunit?4blip_type?$DO@ (0000)
0028BD30 0015:
	??_C@_0BF@GMDJIBPD@model_instance_valid?$AA@ (0000)
0028BD48 0010:
	??_C@_0BA@LNKGLGKJ@rotated_forward?$AA@ (0000)
0028BD58 000b:
	??_C@_0L@GBBDLPMF@rotated_up?$AA@ (0000)
0028BD64 000f:
	??_C@_0P@OGJNHPDG@up?$CB?$DNrotated_up?$AA@ (0000)
0028BD74 0019:
	??_C@_0BJ@GEGCJHGH@forward?$CB?$DNrotated_forward?$AA@ (0000)
0028BD90 0004:
	__real@364c6ce0 (0000)
0028BD94 0004:
	__real@34a5b36e (0000)
0028BD98 0004:
	__real@3b33abb1 (0000)
0028BDA0 0008:
	__real@3f80000000000000 (0000)
0028BDA8 0012:
	??_C@_0BC@MDPNAJDL@?$CGangular_velocity?$AA@ (0000)
0028BDBC 0016:
	??_C@_0BG@NPNNHCFI@?$CGangular_acceleration?$AA@ (0000)
0028BDD4 0011:
	??_C@_0BB@IHCLBKLO@?$CGlinear_velocity?$AA@ (0000)
0028BDE8 0015:
	??_C@_0BF@LNOJMOGI@?$CGlinear_acceleration?$AA@ (0000)
0028BE00 001d:
	??_C@_0BN@FOPAIIBD@instance?9?$DOphysics?9?$DOmass?$DO0?40f?$AA@ (0000)
0028BE20 0018:
	??_C@_0BI@HOJEHDKE@?$CGobject?9?$DOobject?4forward?$AA@ (0000)
0028BE38 0013:
	??_C@_0BD@JHPMDDBM@?$CGobject?9?$DOobject?4up?$AA@ (0000)
0028BE4C 001c:
	??_C@_0BM@EKBNDJOG@?$CGtranslational_acceleration?$AA@ (0000)
0028BE68 000d:
	??_C@_0N@EMPCNLII@magic_torque?$AA@ (0000)
0028BE78 000c:
	??_C@_0M@LNGNIFCJ@magic_force?$AA@ (0000)
0030CEB4 002c:
	_global_gravity (0000)
	_global_water_density (0004)
	_global_air_density (0008)
	_global_physics_collision_depth (000c)
	_depths_of_hell (0010)
0045E01A 0001:
	_debug_physics_disable_penetration_freeze (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "physics.h"

#include "collision_features.h"
#include "collision_models.h"
#include "collisions.h"
#include "effects/material_effect_definitions.h"
#include "game/game_globals.h"
#include "objects/damage.h"
#include "objects/object_definitions.h"
#include "objects/object_types.h"
#include "objects/objects.h"
#include "collision_model_definitions.h"
#include "physics_definitions.h"
#include "render/render_debug.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "units/bipeds.h"
#include "units/vehicles.h"

/* ---------- constants */

enum
{
	_collision_surface_two_sided_bit = 0,
	_collision_surface_invisible_bit,
	_collision_surface_climbable_bit,
	_collision_surface_breakable_bit,
};

enum
{
	_friction_type_point = 0,
	_friction_type_forward,
	_friction_type_left,
	_friction_type_up,
	NUMBER_OF_FRICTION_TYPES
};

enum
{
	_point_at_rest_bit = 0,
	_point_on_ground_bit,
	_point_on_volatile_surface_bit,
	_point_in_water_bit,
	_point_antigraving_bit,
	NUMBER_OF_MASS_POINT_FLAGS
};

enum
{
	_powered_mass_point_ground_friction_bit = 0,
	_powered_mass_point_water_friction_bit,
	_powered_mass_point_air_friction_bit,
	_powered_mass_point_water_lift_bit,
	_powered_mass_point_air_lift_bit,
	_powered_mass_point_thrust_bit,
	_powered_mass_point_antigrav_bit,
	NUMBER_OF_POWERED_MASS_POINT_FLAGS
};

/* ---------- macros */

/* ---------- structures */

struct powered_mass_point_definition
{
	char name[32];
	unsigned long flags;
	real antigrav_strength;
	real antigrav_offset;
	real antigrav_height;
	real antigrav_damp_fraction;
	real antigrav_normal_k1;
	real antigrav_normal_k0;
	real unused[17];
};

#include "friction_datum.h"

#include "powered_mass_point_datum.h"

#include "mass_point_datum.h"

#include "units/vehicle_datum.h"

typedef char powered_mass_point_definition_size_assert[
	sizeof(struct powered_mass_point_definition) == 0x80 ? 1 : -1];
typedef char powered_mass_point_datum_size_assert[
	sizeof(struct powered_mass_point_datum) == 0x60 ? 1 : -1];
typedef char mass_point_datum_size_assert[
	sizeof(struct mass_point_datum) == 0x130 ? 1 : -1];

/* ---------- prototypes */

static short get_material_type(
	long object_index,
	short material_index);
static void compute_ground_plane(
	long object_index,
	struct mass_point_datum *mass_point,
	struct mass_point_definition const *mass_point_definition);
static void friction_evaluate(
	short friction_type,
	real parallel_scale,
	real perpendicular_scale,
	struct friction_datum *friction,
	real_vector3d const *forward,
	real_vector3d const *up);
boolean physics_compute_biped_collision(
	struct collision_model_instance *instance,
	long biped_index);
static boolean physics_compute_vehicle_collision(
	struct physics_instance const *instance0,
	struct physics_instance const *instance1);
static void physics_compute_unit_collisions(
	long vehicle_index);
void physics_update_new(
	struct physics_instance const *instance,
	struct powered_mass_point_datum const *powered_mass_points,
	struct mass_point_datum const *mass_points,
	real_vector3d const *total_force,
	real_vector3d const *total_torque);
static void physics_update_old(
	long object_index,
	struct powered_mass_point_datum *powered_mass_points,
	struct mass_point_datum *mass_points,
	real_vector3d const *magic_force,
	real_vector3d const *magic_torque);
static void rotate_vectors3d_by_angular_velocity(
	real_vector3d const *forward,
	real_vector3d const *up,
	real_vector3d const *angular_velocity,
	real_vector3d *rotated_forward,
	real_vector3d *rotated_up);

/* ---------- globals */

real global_gravity = 0.0035651792f;
real global_water_density = 1.0f;
real global_air_density = 0.0011f;
real global_physics_collision_depth = 0.2f;
real_plane3d depths_of_hell =
{
	{ 0.0f, 0.0f, 1.0f },
	-256.0f
};

/* ---------- public code */

real pin_fraction(
	real value,
	real begin,
	real end)
{
	if (begin < end)
	{
		if (value <= begin)
			return 0.0f;
		else if (value >= end)
			return 1.0f;
		else
			return (value - begin) / (end - begin);
	}
	else
	{
		if (value <= end)
			return 1.0f;
		else if (value >= begin)
			return 0.0f;
		else
			return (begin - value) / (begin - end);
	}
}

boolean physics_get_features_in_sphere(
	struct physics_instance const *instance,
	real_point3d const *center,
	real radius,
	real height,
	real width,
	struct collision_feature_list *features)
{
	short mass_point_index;
	real scaled_radius;

	for (mass_point_index = 0;
		mass_point_index < instance->physics->mass_points.count;
		mass_point_index++)
	{
		struct mass_point_definition const *mass_point = TAG_BLOCK_GET_ELEMENT(
			&instance->physics->mass_points,
			mass_point_index,
			struct mass_point_definition);
		real_point3d point;

		matrix4x3_transform_point(&instance->world_matrix, &mass_point->position, &point);

		scaled_radius = mass_point->radius * instance->world_matrix.scale;

		collision_features_from_point(
			&point,
			height,
			scaled_radius + width,
			instance->object_index,
			NONE,
			0,
			NONE,
			NONE,
			features);
	}

	return features->count[_collision_feature_sphere] ||
		features->count[_collision_feature_cylinder] ||
		features->count[_collision_feature_prism];
}

static short get_material_type(
	long object_index,
	short material_index)
{
	if (material_index != NONE)
	{
		if (object_index != NONE)
		{
			struct object_datum *object = object_get(object_index);
			struct object_definition *definition = object_definition_get(object->definition_index);
			struct collision_model *collision_model = collision_model_definition_get(
				definition->object.collision_model.index);

			return TAG_BLOCK_GET_ELEMENT(
				&collision_model->resistance.materials,
				material_index,
				struct damage_resistance_material)->material_type;
		}
		else
		{
			return TAG_BLOCK_GET_ELEMENT(
				&global_structure_bsp_get()->collision_materials,
				material_index,
				struct structure_collision_material)->runtime_physics_material_type;
		}
	}

	return NONE;
}

void render_debug_physics(
	struct physics_instance *instance)
{
	struct object_datum *object = object_get(instance->object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	real_point3d center_of_mass;
	short mass_point_index;

	matrix4x3_transform_point(
		&instance->world_matrix,
		&instance->physics->center_of_mass,
		&center_of_mass);

	render_debug_vectors(
		TRUE,
		&center_of_mass,
		&instance->world_matrix.forward,
		&instance->world_matrix.up,
		definition->object.bounding_radius);

	for (mass_point_index = 0;
		mass_point_index < instance->physics->mass_points.count;
		mass_point_index++)
	{
		struct mass_point_definition const *mass_point = TAG_BLOCK_GET_ELEMENT(
			&instance->physics->mass_points,
			mass_point_index,
			struct mass_point_definition);
		real_point3d position;
		real_vector3d forward;
		real_vector3d up;

		matrix4x3_transform_point(
			&instance->world_matrix,
			&mass_point->position,
			&position);
		matrix4x3_transform_normal(
			&instance->world_matrix,
			&mass_point->forward,
			&forward);
		matrix4x3_transform_normal(
			&instance->world_matrix,
			&mass_point->up,
			&up);

		render_debug_sphere(
			TRUE,
			&position,
			mass_point->radius,
			global_real_argb_white);
		render_debug_vectors(
			TRUE,
			&position,
			&forward,
			&up,
			mass_point->radius * 0.5f);
	}
}

boolean physics_instance_new(
	struct physics_instance *instance,
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	real_point3d center_of_mass;

	if (definition->object.physics.index != NONE)
	{
		instance->object_index = object_index;
		instance->physics = physics_definition_get(definition->object.physics.index);

		instance->world_matrix.scale = 1.0f;
		object_get_origin(object_index, &instance->world_matrix.position);
		object_get_orientation(object_index, &instance->world_matrix.forward, &instance->world_matrix.up);
		cross_product3d(&instance->world_matrix.up, &instance->world_matrix.forward, &instance->world_matrix.left);

		set_real_point3d(&center_of_mass,
			-instance->physics->center_of_mass.x,
			-instance->physics->center_of_mass.y,
			-instance->physics->center_of_mass.z);
		matrix4x3_transform_point(&instance->world_matrix, &center_of_mass, &center_of_mass);
		instance->world_matrix.position = center_of_mass;

		return TRUE;
	}

	return FALSE;
}

boolean physics_test_point(
	struct physics_instance const *instance,
	real_point3d const *point)
{
	real_point3d local_point;
	struct physics_definition const *physics;
	short mass_point_index;

	matrix4x3_inverse_transform_point(&instance->world_matrix, point, &local_point);
	physics = instance->physics;

	for (mass_point_index = 0;
		mass_point_index < physics->mass_points.count;
		mass_point_index++)
	{
		struct mass_point_definition const *mass_point = TAG_BLOCK_GET_ELEMENT(
			&physics->mass_points,
			mass_point_index,
			struct mass_point_definition);

		if (point_in_sphere(&local_point, &mass_point->position, mass_point->radius))
			return TRUE;

		physics = instance->physics;
	}

	return FALSE;
}

boolean physics_test_vector(
	struct physics_instance const *instance,
	real_point3d const *point,
	real_vector3d const *vector,
	struct physics_test_vector_result *result)
{
	boolean hit = FALSE;
	real_point3d local_point;
	real_vector3d local_vector;
	real_vector3d normal;
	short mass_point_index;

	result->t = REAL_MAX;

	matrix4x3_inverse_transform_point(&instance->world_matrix, point, &local_point);
	matrix4x3_inverse_transform_vector(&instance->world_matrix, vector, &local_vector);

	for (mass_point_index = 0;
		mass_point_index < instance->physics->mass_points.count;
		mass_point_index++)
	{
		struct mass_point_definition const *mass_point = TAG_BLOCK_GET_ELEMENT(
			&instance->physics->mass_points,
			mass_point_index,
			struct mass_point_definition);
		real t;

		if (sphere_test_vector3d(
				&mass_point->position,
				mass_point->radius,
				&local_point,
				&local_vector,
				&t,
				&normal) &&
			result->t > t)
		{
			real_point3d intersection;

			result->t = t;
			// Preserve the January inline schedule without emitting a point_from_line3d COMDAT.
			intersection.x = local_vector.i*t + local_point.x;
			intersection.y = local_vector.j*t + local_point.y;
			intersection.z = local_vector.k*t + local_point.z;
			plane3d_from_point_and_normal(
				&result->plane,
				&intersection,
				&normal);
			hit = TRUE;
		}
	}

	if (hit)
		matrix4x3_transform_plane(&instance->world_matrix, &result->plane, &result->plane);

	return hit;
}

static void compute_ground_plane(
	long object_index,
	struct mass_point_datum *mass_point,
	struct mass_point_definition const *mass_point_definition)
{
	struct collision_feature_list features;
	struct collision_plane collision;

	mass_point->ground_plane = depths_of_hell;
	mass_point->ground_material_type = NONE;
	mass_point->ground_depth = mass_point_definition->radius -
		plane3d_distance_to_point(&mass_point->ground_plane, &mass_point->position);

	if (collision_get_features_in_sphere(
			_collision_test_for_bipeds_dead_flags,
			&mass_point->position,
			mass_point_definition->radius,
			0.0f,
			mass_point_definition->radius,
			object_index,
			&features) &&
		collision_features_test_point(&features, &mass_point->position, &collision))
	{
		mass_point->ground_plane = collision.plane;
		mass_point->ground_depth = collision.t;
		mass_point->ground_material_type = get_material_type(
			collision.object_index,
			collision.material_index);

		SET_FLAG(
			mass_point->flags,
			_point_on_volatile_surface_bit,
			TEST_FLAG(collision.flags, _collision_surface_breakable_bit) ||
				(collision.object_index != NONE &&
					!TEST_FLAG(_object_mask_scenery, object_get_type(collision.object_index))));

		if (collision.object_index != NONE)
			object_deplete_shield(collision.object_index);
	}

	match_assert(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		338,
		mass_point->ground_material_type==NONE || (mass_point->ground_material_type>=0 && mass_point->ground_material_type<NUMBER_OF_MATERIAL_TYPES));

	return;
}

static void friction_evaluate(
	short friction_type,
	real parallel_scale,
	real perpendicular_scale,
	struct friction_datum *friction,
	real_vector3d const *forward,
	real_vector3d const *up)
{
	real_vector3d left;

	if (friction_type == _friction_type_point)
	{
		friction->parallel = friction->friction;
		set_real_vector3d(&friction->perpendicular, 0.0f, 0.0f, 0.0f);
		return;
	}

	switch (friction_type)
	{
	case _friction_type_forward:
		component_vectors_from_normal3d(
			&friction->friction,
			forward,
			&friction->parallel,
			&friction->perpendicular);
		break;

	case _friction_type_left:
		cross_product3d(up, forward, &left);
		component_vectors_from_normal3d(
			&friction->friction,
			&left,
			&friction->parallel,
			&friction->perpendicular);
		break;

	case _friction_type_up:
		component_vectors_from_normal3d(
			&friction->friction,
			up,
			&friction->parallel,
			&friction->perpendicular);
		break;

	default:
		match_vassert("c:\\halo\\SOURCE\\physics\\physics.c", 383, FALSE, NULL);
		break;
	}

	scale_vector3d(&friction->parallel, parallel_scale, &friction->parallel);
	scale_vector3d(&friction->perpendicular, perpendicular_scale, &friction->perpendicular);
	add_vectors3d(&friction->parallel, &friction->perpendicular, &friction->friction);

	return;
}

void physics_compute_new(
	struct physics_instance const *instance,
	struct powered_mass_point_datum const *powered_mass_points,
	struct mass_point_datum *mass_points,
	real_vector3d *total_force,
	real_vector3d *total_torque)
{
	struct object_datum *object = object_get(instance->object_index);
	struct physics_definition const *physics = instance->physics;
	real gravity = physics->gravity_scale*global_gravity;
	short mass_point_index;

	set_real_vector3d(total_force, 0.0f, 0.0f, -physics->mass*gravity);
	set_real_vector3d(total_torque, 0.0f, 0.0f, 0.0f);

	memset(mass_points, 0, sizeof(struct mass_point_datum)*physics->mass_points.count);

	for (mass_point_index = 0;
		mass_point_index < physics->mass_points.count;
		mass_point_index++)
	{
		struct mass_point_definition const *mass_point_definition = TAG_BLOCK_GET_ELEMENT(
			&physics->mass_points,
			mass_point_index,
			struct mass_point_definition);
		struct mass_point_datum *mass_point = mass_points + mass_point_index;
		struct powered_mass_point_definition const *powered_mass_point_definition = NULL;
		struct powered_mass_point_datum const *powered_mass_point = NULL;
		real_vector3d powered_velocity;

		if (mass_point_definition->powered_mass_point_index != NONE && powered_mass_points)
		{
			powered_mass_point_definition = TAG_BLOCK_GET_ELEMENT(
				&physics->powered_mass_points,
				mass_point_definition->powered_mass_point_index,
				struct powered_mass_point_definition);
			powered_mass_point = powered_mass_points + mass_point_definition->powered_mass_point_index;
		}

		mass_point->flags = 0;
		matrix4x3_transform_point(
			&instance->world_matrix,
			&mass_point_definition->position,
			&mass_point->position);
		if (powered_mass_point)
		{
			real_matrix4x3 powered_matrix;

			matrix4x3_multiply(&instance->world_matrix, &powered_mass_point->rotation_matrix, &powered_matrix);
			matrix4x3_transform_normal(&powered_matrix, &mass_point_definition->forward, &mass_point->forward);
			matrix4x3_transform_normal(&powered_matrix, &mass_point_definition->up, &mass_point->up);
		}
		else
		{
			matrix4x3_transform_normal(&instance->world_matrix, &mass_point_definition->forward, &mass_point->forward);
			matrix4x3_transform_normal(&instance->world_matrix, &mass_point_definition->up, &mass_point->up);
		}
		scenario_location_from_point(&mass_point->location, &mass_point->position);

		vector_from_points3d(&object->object.position, &mass_point->position, &mass_point->radius);
		cross_product3d(&object->object.angular_velocity, &mass_point->radius, &mass_point->velocity);
		add_vectors3d(&object->object.translational_velocity, &mass_point->velocity, &mass_point->velocity);

		compute_ground_plane(instance->object_index, mass_point, mass_point_definition);
		mass_point->water_depth = scenario_location_water_depth(&mass_point->location, &mass_point->position);

		if (mass_point->ground_depth > 0.0f)
		{
			struct material_definition const *material = scenario_material_definition_get(
				mass_point->ground_material_type);
			real ground_friction = physics->ground_friction;
			real ground_normal_k1 = physics->ground_normal_k1;
			real ground_normal_k0 = physics->ground_normal_k0;
			real ground_depth = physics->ground_depth;
			real ground_damp_fraction = physics->ground_damp_fraction;
			real normal_velocity;
			real ground_scale;

			if (material->physics_ground_friction_scale > 0.0f && physics->mass <= 7500.0f)
				ground_friction *= material->physics_ground_friction_scale;
			if (material->physics_ground_friction_normal_k1_scale > 0.0f)
				ground_normal_k1 *= material->physics_ground_friction_normal_k1_scale;
			if (material->physics_ground_friction_normal_k0_scale > 0.0f)
				ground_normal_k0 *= material->physics_ground_friction_normal_k0_scale;
			if (material->physics_ground_depth_scale > 0.0f)
				ground_depth *= material->physics_ground_depth_scale;
			if (material->physics_ground_damp_fraction_scale > 0.0f)
				ground_damp_fraction *= material->physics_ground_damp_fraction_scale;

			normal_velocity = dot_product3d(&mass_point->velocity, &mass_point->ground_plane.n);
			mass_point->normal_force_magnitude = physics->mass*
				(global_gravity/ground_depth*mass_point->ground_depth - normal_velocity*ground_damp_fraction);
			scale_vector3d(&mass_point->ground_plane.n, mass_point->normal_force_magnitude, &mass_point->normal_force);

			scale_vector3d(&mass_point->ground_plane.n, -normal_velocity, &mass_point->velocity_relative_to_ground);
			add_vectors3d(&mass_point->velocity_relative_to_ground, &mass_point->velocity, &mass_point->velocity_relative_to_ground);

			ground_scale = -mass_point_definition->mass*ground_friction;
			scale_vector3d(&mass_point->velocity_relative_to_ground, ground_scale, &mass_point->ground_friction.friction);

			if (powered_mass_point_definition &&
				TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_ground_friction_bit) &&
				powered_mass_point->ground_friction_velocity != 0.0f)
			{
				real fraction = pin_fraction(mass_point->ground_plane.n.k, ground_normal_k0, ground_normal_k1);
				real alignment = PIN(dot_product3d(&mass_point->up, &mass_point->ground_plane.n), 0.0f, 1.0f);
				real weight = alignment*alignment*fraction*fraction*ground_scale;
				real_vector3d projected_velocity;

				scale_vector3d(&mass_point->forward, -powered_mass_point->ground_friction_velocity, &powered_velocity);
				scale_vector3d(
					&mass_point->ground_plane.n,
					-dot_product3d(&powered_velocity, &mass_point->ground_plane.n),
					&projected_velocity);
				add_vectors3d(&projected_velocity, &powered_velocity, &projected_velocity);
				add_vectors3d(&mass_point->velocity_relative_to_ground, &projected_velocity, &mass_point->velocity_relative_to_ground);
				scale_vector3d(&projected_velocity, weight, &projected_velocity);
				add_vectors3d(&mass_point->ground_friction.friction, &projected_velocity, &mass_point->ground_friction.friction);
			}

			friction_evaluate(
				mass_point_definition->friction_type,
				mass_point_definition->friction_parallel_scale,
				mass_point_definition->friction_perpendicular_scale,
				&mass_point->ground_friction,
				&mass_point->forward,
				&mass_point->up);
		}

		if (mass_point->water_depth > 0.0f)
		{
			real depth_fraction = mass_point->water_depth >= physics->water_depth ?
				1.0f : mass_point->water_depth/physics->water_depth;
			real water_scale = -mass_point_definition->mass*physics->water_friction;

			if (mass_point_definition->density > 0.0f && physics->water_depth > 0.0f)
			{
				mass_point->water_pressure_magnitude = mass_point_definition->mass/mass_point_definition->density*
					physics->water_density*depth_fraction*gravity;
				set_real_vector3d(&mass_point->water_pressure, 0.0f, 0.0f, mass_point->water_pressure_magnitude);
			}

			if (powered_mass_point_definition &&
				TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_water_friction_bit) &&
				powered_mass_point->water_friction_velocity != 0.0f)
			{
				scale_vector3d(&mass_point->forward, -powered_mass_point->water_friction_velocity, &powered_velocity);
				add_vectors3d(&powered_velocity, &mass_point->velocity, &powered_velocity);
				scale_vector3d(&powered_velocity, water_scale, &mass_point->water_friction.friction);
			}
			else
			{
				scale_vector3d(&mass_point->velocity, water_scale, &mass_point->water_friction.friction);
			}

			friction_evaluate(
				mass_point_definition->friction_type,
				mass_point_definition->friction_parallel_scale,
				mass_point_definition->friction_perpendicular_scale,
				&mass_point->water_friction,
				&mass_point->forward,
				&mass_point->up);

			if (powered_mass_point_definition &&
				TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_water_lift_bit) &&
				powered_mass_point->water_lift_ratio != 0.0f)
			{
				real lift = ABS(dot_product3d(&mass_point->forward, &mass_point->velocity))*
					powered_mass_point->water_lift_ratio*physics->mass*depth_fraction;
				real_vector3d lift_force;

				scale_vector3d(&mass_point->up, lift, &lift_force);
				add_vectors3d(&mass_point->powered_force, &lift_force, &mass_point->powered_force);
			}
		}

		{
			real air_scale = -mass_point_definition->mass*physics->air_friction;

			if (powered_mass_point_definition &&
				TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_air_friction_bit) &&
				powered_mass_point->air_friction_velocity != 0.0f)
			{
				scale_vector3d(&mass_point->forward, -powered_mass_point->air_friction_velocity, &powered_velocity);
				add_vectors3d(&powered_velocity, &mass_point->velocity, &powered_velocity);
				scale_vector3d(&powered_velocity, air_scale, &mass_point->air_friction.friction);
			}
			else
			{
				scale_vector3d(&mass_point->velocity, air_scale, &mass_point->air_friction.friction);
			}

			friction_evaluate(
				mass_point_definition->friction_type,
				mass_point_definition->friction_parallel_scale,
				mass_point_definition->friction_perpendicular_scale,
				&mass_point->air_friction,
				&mass_point->forward,
				&mass_point->up);

			if (powered_mass_point_definition &&
				TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_air_lift_bit) &&
				powered_mass_point->air_lift_ratio != 0.0f)
			{
				real lift = ABS(dot_product3d(&mass_point->forward, &mass_point->velocity))*
					physics->mass*powered_mass_point->air_lift_ratio;
				real_vector3d lift_force;

				scale_vector3d(&mass_point->up, lift, &lift_force);
				add_vectors3d(&mass_point->powered_force, &lift_force, &mass_point->powered_force);
			}
		}

		SET_FLAG(mass_point->flags, _point_at_rest_bit, magnitude_squared3d(&mass_point->velocity) < 0.0011111111f);
		SET_FLAG(mass_point->flags, _point_on_ground_bit, mass_point->ground_depth > 0.0f);
		SET_FLAG(mass_point->flags, _point_in_water_bit, mass_point->water_depth > 0.0f);

		if (powered_mass_point_definition)
		{
			if (TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_thrust_bit))
			{
				real_vector3d thrust;

				scale_vector3d(&mass_point->forward, powered_mass_point->thrust_fraction*physics->mass, &thrust);
				add_vectors3d(&mass_point->powered_force, &thrust, &mass_point->powered_force);
			}

			if (TEST_FLAG(powered_mass_point_definition->flags, _powered_mass_point_antigrav_bit))
			{
				real probe_length = mass_point_definition->radius + powered_mass_point_definition->antigrav_height;
				real_point3d probe_point = mass_point->position;
				real_vector3d probe_vector;
				struct collision_result collision;

				scale_vector3d(global_down3d, probe_length, &probe_vector);

				if (collision_test_vector(
						_collision_test_for_bipeds_dead_flags,
						&probe_point,
						&probe_vector,
						instance->object_index,
						&collision))
				{
					real height = probe_length*collision.t - mass_point_definition->radius;
					real alignment = pin_fraction(
						mass_point->up.k,
						powered_mass_point_definition->antigrav_normal_k0,
						powered_mass_point_definition->antigrav_normal_k1);
					real ground_effect = height <= 0.0f ?
						1.0f : 1.0f - height/powered_mass_point_definition->antigrav_height;
					real magnitude = (ground_effect*ground_effect*global_gravity -
						dot_product3d(&collision.plane.n, &mass_point->velocity)*
							powered_mass_point_definition->antigrav_damp_fraction)*
						powered_mass_point->antigrav_fraction*
						powered_mass_point_definition->antigrav_strength*
						physics->mass*alignment;
					real_vector3d antigrav_force;

					scale_vector3d(&collision.plane.n, magnitude, &antigrav_force);
					add_vectors3d(&mass_point->powered_force, &antigrav_force, &mass_point->powered_force);
					SET_FLAG(mass_point->flags, _point_antigraving_bit, TRUE);
				}
			}
		}

		add_vectors3d(&mass_point->normal_force, &mass_point->ground_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->water_pressure, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->water_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->air_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->powered_force, &mass_point->force);
		cross_product3d(&mass_point->radius, &mass_point->force, &mass_point->torque);

		add_vectors3d(total_force, &mass_point->force, total_force);
		add_vectors3d(total_torque, &mass_point->torque, total_torque);
	}

	return;
}

static boolean physics_compute_vehicle_collision(
	struct physics_instance const *instance0,
	struct physics_instance const *instance1)
{
	boolean collision = FALSE;
	struct vehicle_datum *object0 = vehicle_datum_get(instance0->object_index);
	struct vehicle_datum *object1 = vehicle_datum_get(instance1->object_index);
	real mass_scale = square_root(instance0->physics->mass * instance1->physics->mass);
	real_vector3d force0 = { 0.0f, 0.0f, 0.0f };
	real_vector3d force1 = { 0.0f, 0.0f, 0.0f };
	real_vector3d torque0 = { 0.0f, 0.0f, 0.0f };
	real_vector3d torque1 = { 0.0f, 0.0f, 0.0f };
	short mass_point0_index;

	for (mass_point0_index = 0;
		mass_point0_index < instance0->physics->mass_points.count;
		mass_point0_index++)
	{
		struct mass_point_definition const *mass_point0 = TAG_BLOCK_GET_ELEMENT(
			&instance0->physics->mass_points,
			mass_point0_index,
			struct mass_point_definition);
		real_point3d point0;
		short mass_point1_index;

		matrix4x3_transform_point(&instance0->world_matrix, &mass_point0->position, &point0);

		for (mass_point1_index = 0;
			mass_point1_index < instance1->physics->mass_points.count;
			mass_point1_index++)
		{
			struct mass_point_definition const *mass_point1 = TAG_BLOCK_GET_ELEMENT(
				&instance1->physics->mass_points,
				mass_point1_index,
				struct mass_point_definition);
			real radius = mass_point0->radius + mass_point1->radius;
			real_point3d point1;
			real_vector3d direction;
			real distance;

			matrix4x3_transform_point(&instance1->world_matrix, &mass_point1->position, &point1);
			vector_from_points3d(&point0, &point1, &direction);
			distance = normalize3d(&direction);

			if (distance < radius && distance > 0.0f)
			{
				real penetration = (radius - distance) * 0.5f;
				real force_magnitude = (global_gravity / global_physics_collision_depth) *
					penetration * mass_scale * 2.0f;
				real_vector3d collision_force0;
				real_vector3d collision_force1;
				real_point3d collision_point;
				real_vector3d radius0;
				real_vector3d radius1;
				real_vector3d collision_torque0;
				real_vector3d collision_torque1;

				scale_vector3d(&direction, -force_magnitude, &collision_force0);
				scale_vector3d(&direction, force_magnitude, &collision_force1);
				collision_point.x = point0.x + direction.i * (mass_point0->radius - penetration);
				collision_point.y = point0.y + direction.j * (mass_point0->radius - penetration);
				collision_point.z = point0.z + direction.k * (mass_point0->radius - penetration);
				vector_from_points3d(&object0->object.position, &collision_point, &radius0);
				vector_from_points3d(&object1->object.position, &collision_point, &radius1);
				cross_product3d(&radius0, &collision_force0, &collision_torque0);
				collision = TRUE;
				cross_product3d(&radius1, &collision_force1, &collision_torque1);
				add_vectors3d(&force0, &collision_force0, &force0);
				add_vectors3d(&force1, &collision_force1, &force1);
				add_vectors3d(&torque0, &collision_torque0, &torque0);
				add_vectors3d(&torque1, &collision_torque1, &torque1);
			}
		}
	}

	if (collision)
	{
		add_vectors3d(&object0->vehicle.collision_force, &force0, &object0->vehicle.collision_force);
		add_vectors3d(&object0->vehicle.collision_torque, &torque0, &object0->vehicle.collision_torque);
		SET_FLAG(object0->object.flags, _object_at_rest_bit, FALSE);

		if (!(instance1->physics->radius > 0.0f))
		{
			add_vectors3d(&object1->vehicle.collision_force, &force1, &object1->vehicle.collision_force);
			add_vectors3d(&object1->vehicle.collision_torque, &torque1, &object1->vehicle.collision_torque);
			SET_FLAG(object1->object.flags, _object_at_rest_bit, FALSE);
		}
	}

	return collision;
}

static void physics_compute_unit_collisions(
	long vehicle_index)
{
	struct collision_model_instance collision_instance;
	struct physics_instance vehicle_physics;
	long object_indices[2048];
	boolean model_instance_valid = collision_model_instance_new(
		&collision_instance,
		vehicle_index);
	struct unit_datum *vehicle;
	short object_count;
	short object_number;

	if (!physics_instance_new(&vehicle_physics, vehicle_index))
		return;

	vehicle = vehicle_get(vehicle_index);
	object_count = objects_in_sphere(
		_object_mask_biped,
		_object_mask_vehicle + (model_instance_valid ? _object_mask_biped : 0),
		&vehicle->object.location,
		&vehicle->object.bounding_sphere_center,
		vehicle->object.bounding_sphere_radius,
		object_indices,
		NUMBEROF(object_indices));

	for (object_number = 0; object_number < object_count; object_number++)
	{
		long object_index = object_indices[object_number];
		struct object_header_datum *object_header = object_header_get(object_index);

		switch (object_header->type)
		{
		case _object_type_biped:
		{
			struct biped_datum *biped = biped_get(object_index);

			match_assert(
				"c:\\halo\\SOURCE\\physics\\physics.c",
				669,
				model_instance_valid);
			if (!TEST_FLAG(biped->object.damage_flags, _object_dead_bit))
				physics_compute_biped_collision(&collision_instance, object_index);
			break;
		}

		case _object_type_vehicle:
		{
			if (object_index != vehicle_index)
			{
				struct physics_instance other_physics;

				if (physics_instance_new(&other_physics, object_index))
				{
					struct unit_datum *other_vehicle = vehicle_get(object_index);

					if (DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index) <
						DATUM_INDEX_TO_ABSOLUTE_INDEX(vehicle_index) ||
						TEST_FLAG(other_vehicle->object.flags, _object_at_rest_bit) ||
						other_physics.physics->radius > 0.0f)
					{
						physics_compute_vehicle_collision(&vehicle_physics, &other_physics);
					}
				}
			}
			break;
		}

		default:
			break;
		}
	}

	return;
}

/* NonMatching: the owner-safe natural reconstruction is 0x14A0 bytes with 114
 * relocations versus the January target's 0x1430 bytes and 115 relocations.
 * Its final axes predicate also falls out of line after the earlier codegen
 * divergence, so this coherent candidate is parked without schedule tuning. */
static void physics_update_old(
	long object_index,
	struct powered_mass_point_datum *powered_mass_points,
	struct mass_point_datum *mass_points,
	real_vector3d const *magic_force,
	real_vector3d const *magic_torque)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition const *object_definition = object_definition_get(object->definition_index);
	struct physics_definition const *physics = physics_definition_get(
		object_definition->object.physics.index);
	real gravity = physics->gravity_scale*global_gravity;
	real_matrix4x3 world_matrix;
	real_vector3d total_force = { 0.0f, 0.0f, -physics->mass*gravity };
	real_vector3d total_torque = { 0.0f, 0.0f, 0.0f };
	real_vector3d translational_acceleration = { 0.0f, 0.0f, 0.0f };
	real_vector3d angular_acceleration = { 0.0f, 0.0f, 0.0f };
	short stopped_mass_point_count = 0;
	short grounded_mass_point_count = 0;
	short volatile_mass_point_count = 0;
	short submerged_mass_point_count = 0;
	short mass_point_index;

	matrix4x3_from_point_and_vectors(
		&world_matrix,
		&object->object.position,
		&object->object.forward,
		&object->object.up);

	if (powered_mass_points)
	{
		short powered_mass_point_index;

		for (powered_mass_point_index = 0;
			powered_mass_point_index < physics->powered_mass_points.count;
			powered_mass_point_index++)
		{
			struct powered_mass_point_datum *powered_mass_point =
				powered_mass_points + powered_mass_point_index;

			matrix4x3_rotation_from_quaternion(
				&powered_mass_point->rotation_matrix,
				&powered_mass_point->rotation);
			matrix4x3_transpose(&powered_mass_point->rotation_matrix);
		}
	}

	memset(mass_points, 0, sizeof(struct mass_point_datum)*physics->mass_points.count);

	if (magic_force)
	{
		match_assert_valid_real_vector3d(
			"c:\\halo\\SOURCE\\physics\\physics.c",
			1255,
			magic_force);
		add_vectors3d(&total_force, magic_force, &total_force);
	}
	if (magic_torque)
	{
		match_assert_valid_real_vector3d(
			"c:\\halo\\SOURCE\\physics\\physics.c",
			1261,
			magic_torque);
		total_torque = *magic_torque;
	}

	for (mass_point_index = 0;
		mass_point_index < physics->mass_points.count;
		mass_point_index++)
	{
		struct mass_point_definition const *mass_point_definition = TAG_BLOCK_GET_ELEMENT(
			&physics->mass_points,
			mass_point_index,
			struct mass_point_definition);
		struct mass_point_datum *mass_point = mass_points + mass_point_index;
		struct powered_mass_point_definition const *powered_mass_point_definition = NULL;
		struct powered_mass_point_datum *powered_mass_point = NULL;
		real_point3d local_position;

		if (mass_point_definition->powered_mass_point_index != NONE && powered_mass_points)
		{
			powered_mass_point_definition = TAG_BLOCK_GET_ELEMENT(
				&physics->powered_mass_points,
				mass_point_definition->powered_mass_point_index,
				struct powered_mass_point_definition);
			if (powered_mass_point_definition)
			{
				powered_mass_point =
					powered_mass_points + mass_point_definition->powered_mass_point_index;
			}
		}

		mass_point->flags = 0;
		local_position.x = mass_point_definition->position.x - physics->center_of_mass.x;
		local_position.y = mass_point_definition->position.y - physics->center_of_mass.y;
		local_position.z = mass_point_definition->position.z - physics->center_of_mass.z;
		matrix4x3_transform_point(&world_matrix, &local_position, &mass_point->position);

		if (powered_mass_point)
		{
			real_matrix4x3 powered_matrix;

			matrix4x3_multiply(
				&world_matrix,
				&powered_mass_point->rotation_matrix,
				&powered_matrix);
			matrix4x3_transform_normal(
				&powered_matrix,
				&mass_point_definition->forward,
				&mass_point->forward);
			matrix4x3_transform_normal(
				&powered_matrix,
				&mass_point_definition->up,
				&mass_point->up);
		}
		else
		{
			matrix4x3_transform_normal(
				&world_matrix,
				&mass_point_definition->forward,
				&mass_point->forward);
			matrix4x3_transform_normal(
				&world_matrix,
				&mass_point_definition->up,
				&mass_point->up);
		}

		scenario_location_from_point(&mass_point->location, &mass_point->position);
		vector_from_points3d(&object->object.position, &mass_point->position, &mass_point->radius);
		cross_product3d(&object->object.angular_velocity, &mass_point->radius, &mass_point->velocity);
		add_vectors3d(
			&object->object.translational_velocity,
			&mass_point->velocity,
			&mass_point->velocity);

		compute_ground_plane(object_index, mass_point, mass_point_definition);
		mass_point->water_depth = scenario_location_water_depth(
			&mass_point->location,
			&mass_point->position);

		if (mass_point->ground_depth > 0.0f && physics->ground_depth > 0.0f)
		{
			real normal_velocity = dot_product3d(
				&mass_point->velocity,
				&mass_point->ground_plane.n);
			real ground_scale = -mass_point_definition->mass*physics->ground_friction;

			mass_point->normal_force_magnitude = physics->mass*(
				global_gravity/physics->ground_depth*mass_point->ground_depth -
				normal_velocity*physics->ground_damp_fraction);
			scale_vector3d(
				&mass_point->ground_plane.n,
				mass_point->normal_force_magnitude,
				&mass_point->normal_force);
			scale_vector3d(
				&mass_point->ground_plane.n,
				-normal_velocity,
				&mass_point->velocity_relative_to_ground);
			add_vectors3d(
				&mass_point->velocity_relative_to_ground,
				&mass_point->velocity,
				&mass_point->velocity_relative_to_ground);
			scale_vector3d(
				&mass_point->velocity_relative_to_ground,
				ground_scale,
				&mass_point->ground_friction.friction);

			if (powered_mass_point_definition &&
				TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_ground_friction_bit) &&
				powered_mass_point->ground_friction_velocity != 0.0f)
			{
				real fraction = pin_fraction(
					mass_point->ground_plane.n.k,
					physics->ground_normal_k0,
					physics->ground_normal_k1);
				real alignment = PIN(
					dot_product3d(&mass_point->up, &mass_point->ground_plane.n),
					0.0f,
					1.0f);
				real weight = alignment*alignment*fraction*fraction*ground_scale;
				real_vector3d powered_velocity;
				real_vector3d projected_velocity;

				scale_vector3d(
					&mass_point->forward,
					-powered_mass_point->ground_friction_velocity,
					&powered_velocity);
				scale_vector3d(
					&mass_point->ground_plane.n,
					-dot_product3d(&powered_velocity, &mass_point->ground_plane.n),
					&projected_velocity);
				add_vectors3d(&projected_velocity, &powered_velocity, &projected_velocity);
				add_vectors3d(
					&mass_point->velocity_relative_to_ground,
					&projected_velocity,
					&mass_point->velocity_relative_to_ground);
				scale_vector3d(&projected_velocity, weight, &projected_velocity);
				add_vectors3d(
					&mass_point->ground_friction.friction,
					&projected_velocity,
					&mass_point->ground_friction.friction);
			}

			if (mass_point->ground_material_type == _material_ice)
			{
				friction_evaluate(
					mass_point_definition->friction_type,
					mass_point_definition->friction_parallel_scale*0.125f,
					mass_point_definition->friction_perpendicular_scale*0.125f,
					&mass_point->ground_friction,
					&mass_point->forward,
					&mass_point->up);
			}
			else
			{
				friction_evaluate(
					mass_point_definition->friction_type,
					mass_point_definition->friction_parallel_scale,
					mass_point_definition->friction_perpendicular_scale,
					&mass_point->ground_friction,
					&mass_point->forward,
					&mass_point->up);
			}
		}

		if (mass_point->water_depth > 0.0f)
		{
			real depth_fraction = mass_point->water_depth >= physics->water_depth ?
				1.0f : mass_point->water_depth/physics->water_depth;
			real water_scale = -mass_point_definition->mass*physics->water_friction;

			if (mass_point_definition->density > 0.0f && physics->water_depth > 0.0f)
			{
				mass_point->water_pressure_magnitude =
					mass_point_definition->mass/mass_point_definition->density*
					physics->water_density*depth_fraction*gravity;
				set_real_vector3d(
					&mass_point->water_pressure,
					0.0f,
					0.0f,
					mass_point->water_pressure_magnitude);
			}

			if (powered_mass_point_definition &&
				TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_water_friction_bit) &&
				powered_mass_point->water_friction_velocity != 0.0f)
			{
				real_vector3d powered_velocity;

				scale_vector3d(
					&mass_point->forward,
					-powered_mass_point->water_friction_velocity,
					&powered_velocity);
				add_vectors3d(&powered_velocity, &mass_point->velocity, &powered_velocity);
				scale_vector3d(
					&powered_velocity,
					water_scale,
					&mass_point->water_friction.friction);
			}
			else
			{
				scale_vector3d(
					&mass_point->velocity,
					water_scale,
					&mass_point->water_friction.friction);
			}

			friction_evaluate(
				mass_point_definition->friction_type,
				mass_point_definition->friction_parallel_scale,
				mass_point_definition->friction_perpendicular_scale,
				&mass_point->water_friction,
				&mass_point->forward,
				&mass_point->up);

			if (powered_mass_point_definition &&
				TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_water_lift_bit) &&
				powered_mass_point->water_lift_ratio != 0.0f)
			{
				real lift = ABS(dot_product3d(&mass_point->forward, &mass_point->velocity))*
					powered_mass_point->water_lift_ratio*physics->mass*depth_fraction;
				real_vector3d lift_force;

				scale_vector3d(&mass_point->up, lift, &lift_force);
				add_vectors3d(&mass_point->powered_force, &lift_force, &mass_point->powered_force);
			}
		}

		{
			real air_scale = -mass_point_definition->mass*physics->air_friction;

			if (powered_mass_point_definition &&
				TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_air_friction_bit) &&
				powered_mass_point->air_friction_velocity != 0.0f)
			{
				real_vector3d powered_velocity;

				scale_vector3d(
					&mass_point->forward,
					-powered_mass_point->air_friction_velocity,
					&powered_velocity);
				add_vectors3d(&powered_velocity, &mass_point->velocity, &powered_velocity);
				scale_vector3d(
					&powered_velocity,
					air_scale,
					&mass_point->air_friction.friction);
			}
			else
			{
				scale_vector3d(
					&mass_point->velocity,
					air_scale,
					&mass_point->air_friction.friction);
			}

			friction_evaluate(
				mass_point_definition->friction_type,
				mass_point_definition->friction_parallel_scale,
				mass_point_definition->friction_perpendicular_scale,
				&mass_point->air_friction,
				&mass_point->forward,
				&mass_point->up);

			if (powered_mass_point_definition &&
				TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_air_lift_bit) &&
				powered_mass_point->air_lift_ratio != 0.0f)
			{
				real lift = ABS(dot_product3d(&mass_point->forward, &mass_point->velocity))*
					powered_mass_point->air_lift_ratio*physics->mass;
				real_vector3d lift_force;

				scale_vector3d(&mass_point->up, lift, &lift_force);
				add_vectors3d(&mass_point->powered_force, &lift_force, &mass_point->powered_force);
			}
		}

		SET_FLAG(
			mass_point->flags,
			_point_at_rest_bit,
			magnitude_squared3d(&mass_point->velocity) < 0.0011111111f);
		SET_FLAG(mass_point->flags, _point_on_ground_bit, mass_point->ground_depth > 0.0f);
		SET_FLAG(mass_point->flags, _point_in_water_bit, mass_point->water_depth > 0.0f);
		stopped_mass_point_count += TEST_FLAG(mass_point->flags, _point_at_rest_bit);
		grounded_mass_point_count += TEST_FLAG(mass_point->flags, _point_on_ground_bit);
		volatile_mass_point_count += TEST_FLAG(
			mass_point->flags,
			_point_on_volatile_surface_bit);
		submerged_mass_point_count += TEST_FLAG(mass_point->flags, _point_in_water_bit);

		if (powered_mass_point_definition)
		{
			if (TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_thrust_bit))
			{
				real_vector3d thrust;

				scale_vector3d(
					&mass_point->forward,
					powered_mass_point->thrust_fraction*physics->mass,
					&thrust);
				add_vectors3d(&mass_point->powered_force, &thrust, &mass_point->powered_force);
			}

			if (TEST_FLAG(
					powered_mass_point_definition->flags,
					_powered_mass_point_antigrav_bit))
			{
				real probe_length =
					mass_point_definition->radius + powered_mass_point_definition->antigrav_height;
				real_point3d probe_point = mass_point->position;
				real_vector3d probe_vector;
				struct collision_result collision;

				scale_vector3d(global_down3d, probe_length, &probe_vector);
				if (collision_test_vector(
						_collision_test_for_bipeds_dead_flags,
						&probe_point,
						&probe_vector,
						object_index,
						&collision))
				{
					real height = probe_length*collision.t - mass_point_definition->radius;
					real alignment = pin_fraction(
						mass_point->up.k,
						powered_mass_point_definition->antigrav_normal_k0,
						powered_mass_point_definition->antigrav_normal_k1);
					real ground_effect = height <= 0.0f ?
						1.0f : 1.0f - height/powered_mass_point_definition->antigrav_height;
					real magnitude = (ground_effect*ground_effect*global_gravity -
						dot_product3d(&collision.plane.n, &mass_point->velocity)*
							powered_mass_point_definition->antigrav_damp_fraction)*
						powered_mass_point->antigrav_fraction*
						powered_mass_point_definition->antigrav_strength*
						physics->mass*alignment;
					real_vector3d antigrav_force;

					scale_vector3d(&collision.plane.n, magnitude, &antigrav_force);
					add_vectors3d(
						&mass_point->powered_force,
						&antigrav_force,
						&mass_point->powered_force);
				}
			}
		}

		add_vectors3d(&mass_point->normal_force, &mass_point->ground_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->water_pressure, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->water_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->air_friction.friction, &mass_point->force);
		add_vectors3d(&mass_point->force, &mass_point->powered_force, &mass_point->force);
		cross_product3d(&mass_point->radius, &mass_point->force, &mass_point->torque);
		add_vectors3d(&total_force, &mass_point->force, &total_force);
		add_vectors3d(&total_torque, &mass_point->torque, &total_torque);
	}

	if (physics->mass != 0.0f)
		scale_vector3d(&total_force, 1.0f/physics->mass, &translational_acceleration);

	{
		real_vector3d torque_axis = total_torque;
		real torque_magnitude = normalize3d(&torque_axis);

		if (torque_magnitude != 0.0f)
		{
			real moment_of_inertia = 0.0f;

			for (mass_point_index = 0;
				mass_point_index < physics->mass_points.count;
				mass_point_index++)
			{
				struct mass_point_definition const *mass_point_definition = TAG_BLOCK_GET_ELEMENT(
					&physics->mass_points,
					mass_point_index,
					struct mass_point_definition);
				struct mass_point_datum const *mass_point = mass_points + mass_point_index;
				real projection = -dot_product3d(&mass_point->radius, &torque_axis);
				real_vector3d perpendicular_radius;

				scale_vector3d(&torque_axis, projection, &perpendicular_radius);
				add_vectors3d(
					&perpendicular_radius,
					&mass_point->radius,
					&perpendicular_radius);
				moment_of_inertia += (
					magnitude_squared3d(&perpendicular_radius) +
					mass_point_definition->radius*mass_point_definition->radius*0.4f)*
					mass_point_definition->mass*physics->moment;
			}

			if (moment_of_inertia != 0.0f)
			{
				scale_vector3d(
					&total_torque,
					1.0f/moment_of_inertia,
					&angular_acceleration);
			}
		}
	}

	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		1539,
		&translational_acceleration);
	add_vectors3d(
		&object->object.translational_velocity,
		&translational_acceleration,
		&object->object.translational_velocity);
	match_assert_valid_real_vector3d(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		1543,
		&angular_acceleration);
	add_vectors3d(
		&object->object.angular_velocity,
		&angular_acceleration,
		&object->object.angular_velocity);

	{
		real_point3d new_position;
		struct location new_location;
		real_vector3d rotation_axis = object->object.angular_velocity;
		real angular_speed;

		new_position.x = object->object.position.x + object->object.translational_velocity.i;
		new_position.y = object->object.position.y + object->object.translational_velocity.j;
		new_position.z = object->object.position.z + object->object.translational_velocity.k;
		scenario_location_from_line(
			&new_location,
			&object->object.location,
			&object->object.position,
			&new_position);
		object_translate(object_index, &new_position, &new_location);

		angular_speed = normalize3d(&rotation_axis);
		if (angular_speed != 0.0f)
		{
			real sine_value = sine(angular_speed);
			real cosine_value = cosine(angular_speed);
			real orthogonalization;

			rotate_vector_about_axis(
				&object->object.forward,
				&rotation_axis,
				sine_value,
				cosine_value);
			rotate_vector_about_axis(
				&object->object.up,
				&rotation_axis,
				sine_value,
				cosine_value);
			normalize3d(&object->object.forward);
			orthogonalization = -dot_product3d(
				&object->object.up,
				&object->object.forward);
			object->object.up.i += orthogonalization*object->object.forward.i;
			object->object.up.j += orthogonalization*object->object.forward.j;
			object->object.up.k += orthogonalization*object->object.forward.k;
			normalize3d(&object->object.up);
		}
	}

	SET_FLAG(
		object->object.flags,
		_object_at_rest_bit,
		stopped_mass_point_count == physics->mass_points.count &&
		grounded_mass_point_count >= 3 &&
		volatile_mass_point_count == 0 &&
		magnitude_squared3d(&object->object.translational_velocity) <= 0.0011111111f &&
		magnitude_squared3d(&object->object.angular_velocity) <= 0.0027415568f &&
		magnitude_squared3d(&translational_acceleration) <= 0.00000030864197f &&
		magnitude_squared3d(&angular_acceleration) <= 0.0000030461742f);
	SET_FLAG(object->object.flags, _object_on_ground_bit, grounded_mass_point_count > 0);
	SET_FLAG(object->object.flags, _object_on_media_bit, submerged_mass_point_count > 0);
	SET_FLAG(object->object.flags, _object_partially_under_media_bit, submerged_mass_point_count > 0);
	SET_FLAG(
		object->object.flags,
		_object_wholly_under_media_bit,
		submerged_mass_point_count == physics->mass_points.count);
	match_assert_valid_real_vector3d_axes2(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		1595,
		&object->object.forward,
		&object->object.up);

	return;
}

void physics_update(
	long object_index,
	struct powered_mass_point_datum *powered_mass_points,
	struct mass_point_datum *mass_points,
	real_vector3d const *magic_force,
	real_vector3d const *magic_torque)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *object_definition = object_definition_get(object->definition_index);
	struct physics_definition *physics = physics_definition_get(
		object_definition->object.physics.index);
	struct physics_instance instance;
	real_vector3d total_force;
	real_vector3d total_torque;
	short powered_mass_point_index;

	if (physics->radius > 0.0f)
	{
		physics_update_old(
			object_index,
			powered_mass_points,
			mass_points,
			magic_force,
			magic_torque);
		return;
	}

	physics_instance_new(&instance, object_index);
	if (powered_mass_points)
	{
		for (powered_mass_point_index = 0;
			powered_mass_point_index < physics->powered_mass_points.count;
			powered_mass_point_index++)
		{
			struct powered_mass_point_datum *powered_mass_point =
				powered_mass_points + powered_mass_point_index;

			matrix4x3_rotation_from_quaternion(
				&powered_mass_point->rotation_matrix,
				&powered_mass_point->rotation);
			matrix4x3_transpose(&powered_mass_point->rotation_matrix);
		}
	}

	physics_compute_new(
		&instance,
		powered_mass_points,
		mass_points,
		&total_force,
		&total_torque);

	{
		struct vehicle_datum *vehicle = vehicle_datum_get(object_index);

		total_force.i += vehicle->vehicle.collision_force.i;
		total_force.j += vehicle->vehicle.collision_force.j;
		total_force.k += vehicle->vehicle.collision_force.k;
		total_torque.i += vehicle->vehicle.collision_torque.i;
		total_torque.j += vehicle->vehicle.collision_torque.j;
		total_torque.k += vehicle->vehicle.collision_torque.k;
		vehicle->vehicle.collision_force.i = 0.0f;
		vehicle->vehicle.collision_force.j = 0.0f;
		vehicle->vehicle.collision_force.k = 0.0f;
		vehicle->vehicle.collision_torque.i = 0.0f;
		vehicle->vehicle.collision_torque.j = 0.0f;
		vehicle->vehicle.collision_torque.k = 0.0f;
	}

	if (magic_force)
	{
		match_assert_valid_real_vector3d(
			"c:\\halo\\SOURCE\\physics\\physics.c",
			270,
			magic_force);
		total_force.i += magic_force->i;
		total_force.j += magic_force->j;
		total_force.k += magic_force->k;
	}
	if (magic_torque)
	{
		match_assert_valid_real_vector3d(
			"c:\\halo\\SOURCE\\physics\\physics.c",
			276,
			magic_torque);
		total_torque.i += magic_torque->i;
		total_torque.j += magic_torque->j;
		total_torque.k += magic_torque->k;
	}

	physics_update_new(
		&instance,
		powered_mass_points,
		mass_points,
		&total_force,
		&total_torque);
	physics_compute_unit_collisions(object_index);

	return;
}

static void rotate_vectors3d_by_angular_velocity(
	real_vector3d const *forward,
	real_vector3d const *up,
	real_vector3d const *angular_velocity,
	real_vector3d *rotated_forward,
	real_vector3d *rotated_up)
{
	real_vector3d axis = *angular_velocity;
	real magnitude = normalize3d(&axis);

	match_assert("c:\\halo\\SOURCE\\physics\\physics.c", 944, forward!=rotated_forward);
	match_assert("c:\\halo\\SOURCE\\physics\\physics.c", 945, up!=rotated_up);

	if (magnitude != 0.0f)
	{
		real_matrix4x3 rotation;
		real dot;

		matrix4x3_rotation_from_axis_and_angle(
			&rotation,
			&axis,
			sine(magnitude),
			cosine(magnitude));
		matrix4x3_transform_vector(&rotation, forward, rotated_forward);
		matrix4x3_transform_vector(&rotation, up, rotated_up);
		normalize3d(rotated_forward);

		dot = -dot_product3d(rotated_up, rotated_forward);
		rotated_up->i += dot*rotated_forward->i;
		rotated_up->j += dot*rotated_forward->j;
		rotated_up->k += dot*rotated_forward->k;
		normalize3d(rotated_up);
	}
	else
	{
		*rotated_forward = *forward;
		*rotated_up = *up;
	}

	match_assert_valid_real_vector3d_axes2(
		"c:\\halo\\SOURCE\\physics\\physics.c",
		965,
		rotated_forward,
		rotated_up);

	return;
}

/* ---------- private code */
