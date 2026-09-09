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

struct friction_datum
{
	real_vector3d friction;
	real_vector3d parallel;
	real_vector3d perpendicular;
};

struct powered_mass_point_datum
{
	real ground_friction_velocity;
	real water_friction_velocity;
	real air_friction_velocity;
	real water_lift_ratio;
	real air_lift_ratio;
	real thrust_fraction;
	real antigrav_fraction;
	real_quaternion rotation;
	real_matrix4x3 rotation_matrix;
};

struct mass_point_datum
{
	unsigned long flags;
	real_point3d position;
	real_vector3d forward;
	real_vector3d left;
	real_vector3d up;
	struct location location;
	real_vector3d radius;
	real_vector3d velocity;
	real_vector3d velocity_relative_to_ground;
	real_plane3d ground_plane;
	short ground_material_type;
	short pad0;
	real ground_depth;
	short water_material_type;
	short pad1;
	real water_depth;
	real normal_force_magnitude;
	real_vector3d normal_force;
	struct friction_datum ground_friction;
	real water_pressure_magnitude;
	real_vector3d water_pressure;
	struct friction_datum water_friction;
	struct friction_datum air_friction;
	real_vector3d powered_force;
	real_vector3d force;
	real_vector3d torque;
};

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

/* ---------- private code */
