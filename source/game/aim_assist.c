/*
AIM_ASSIST.C

symbols in this file:
00093C00 0050:
	_compute_attenuation (0000)
00093C50 0030:
	_compute_composite_attenuation (0000)
00093C80 00f0:
	_unit_get_aim_assist_parameters (0000)
00093D70 00a0:
	_compare_targets (0000)
00093E10 0010:
	_reciprocal_square_root (0000)
00093E20 0060:
	_limit3d (0000)
00093E80 0020:
	_set_real_euler_angles2d (0000)
00093EA0 00f0:
	_aim_assist_clear_line_of_sight (0000)
00093F90 01a0:
	_object_compute_autoaim_target (0000)
00094130 01a0:
	_aim_assist_compute_target (0000)
000942D0 0110:
	_autoaim_compute_target (0000)
000943E0 0190:
	_find_aim_assist_targets_recursive (0000)
00094570 0130:
	_find_aim_assist_targets (0000)
000946A0 0100:
	_aim_assist (0000)
000947A0 0340:
	_player_aim_projectile (0000)
00094AE0 0170:
	_local_player_aim_assist (0000)
0025AC3C 0021:
	??_C@_0CB@CDDNCJJI@c?3?2halo?2SOURCE?2game?2aim_assist?4c@ (0000)
0025AC60 0004:
	__real@43000000 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "game/aim_assist.h"

#include "camera/director.h"
#include "camera/observer.h"
#include "game/game.h"
#include "game/game_allegiance.h"
#include "game/game_globals.h"
#include "game/player_control.h"
#include "game/players.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "objects/objects.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"
#include "structures/structures.h"
#include "tag_files/tag_groups.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "units/unit_definitions.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	MAXIMUM_AIM_ASSIST_TARGETS= 64,
	MAXIMUM_AIM_ASSIST_CLUSTERS= 512,
	MAXIMUM_AIM_ASSIST_OBJECTS= 2048
};

/* ---------- macros */

/* ---------- structures */

struct aim_assist_target
{
	long object_index;
	real_point3d position;
	real_vector3d vector;
	real_vector3d direction;
	real distance;
	real angle;
	real autoaim_level;
	real magnetism_level;
};

/* ---------- prototypes */

static real compute_attenuation(
	real variable,
	real maximum);
static real compute_composite_attenuation(
	real distance,
	real maximum_distance,
	real angle,
	real maximum_angle);
static boolean unit_get_aim_assist_parameters(
	long unit_index,
	short zoom_level,
	struct aim_assist_parameters *parameters);
static int compare_targets(
	void const *target0,
	void const *target1);
static void object_compute_autoaim_target(
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	real_point3d *target);
boolean aim_assist_compute_target(
	struct aim_assist_parameters const *parameters,
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	struct aim_assist_target *target);
static short find_aim_assist_targets_recursive(
	struct aim_assist_parameters const *parameters,
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	real distance,
	real angle_sine,
	real angle_cosine,
	long ignore_object_index,
	short ignore_team_index,
	short maximum_target_count,
	struct aim_assist_target *targets);
static short find_aim_assist_targets(
	struct aim_assist_parameters const *parameters,
	short cluster_index,
	real_point3d const *position,
	real_vector3d const *direction,
	long ignore_object_index,
	short ignore_team_index,
	short maximum_target_count,
	struct aim_assist_target *targets);
boolean aim_assist(
	struct aim_assist_parameters const *parameters,
	real_point3d const *position,
	real_vector3d const *direction,
	long ignore_object_index,
	short ignore_team_index,
	struct aim_assist_target *target);

/* ---------- globals */

/* ---------- code (definitions in January object order) */

static real compute_attenuation(
	real variable,
	real maximum)
{
	real half_maximum= maximum*0.5f;

	if (variable>=maximum)
	{
		return 0.f;
	}

	if (variable<=half_maximum)
	{
		return 1.f;
	}

	return (maximum - variable) / (maximum - half_maximum);
}

static real compute_composite_attenuation(
	real distance,
	real maximum_distance,
	real angle,
	real maximum_angle)
{
	return compute_attenuation(distance, maximum_distance) * compute_attenuation(angle, maximum_angle);
}

static boolean unit_get_aim_assist_parameters(
	long unit_index,
	short zoom_level,
	struct aim_assist_parameters *parameters)
{
	if (unit_index!=NONE)
	{
		struct unit_datum *unit= unit_get(unit_index);
		long weapon_index= unit_inventory_get_weapon(unit_index, unit->unit.current_weapon_index);

		if (weapon_index!=NONE)
		{
			struct weapon_datum *weapon= weapon_get(weapon_index);
			struct weapon_definition *weapon_definition= weapon_definition_get(weapon->definition_index);

			if (zoom_level!=NONE || !TEST_FLAG(weapon_definition->weapon.flags, _weapon_aim_assists_only_when_zoomed_bit))
			{
				real magnification= weapon_get_zoom_magnification(weapon_index, zoom_level);
				real inverse_magnification= 1.f/magnification;

				parameters->autoaim_angle= weapon_definition->weapon.aim_assist_parameters.autoaim_angle*inverse_magnification;
				parameters->autoaim_distance= weapon_definition->weapon.aim_assist_parameters.autoaim_distance*magnification;
				parameters->magnetism_angle= weapon_definition->weapon.aim_assist_parameters.magnetism_angle*inverse_magnification;
				parameters->magnetism_distance= weapon_definition->weapon.aim_assist_parameters.magnetism_distance*magnification;
				parameters->deviation_angle= MAX(weapon_definition->weapon.aim_assist_parameters.deviation_angle,
					weapon_definition->weapon.aim_assist_parameters.autoaim_angle)*inverse_magnification;

				return TRUE;
			}
		}
	}

	return FALSE;
}

static int compare_targets(
	void const *target0,
	void const *target1)
{
	struct aim_assist_target const *first= target0;
	struct aim_assist_target const *second= target1;

	if (first->autoaim_level>second->autoaim_level) return -1;
	if (first->autoaim_level<second->autoaim_level) return 1;

	if (first->magnetism_level>second->magnetism_level) return -1;
	if (first->magnetism_level<second->magnetism_level) return 1;

	if (first->distance<second->distance) return -1;
	if (first->distance>second->distance) return 1;

	if (first->angle<second->angle) return -1;
	if (first->angle>second->angle) return 1;

	return DATUM_INDEX_TO_ABSOLUTE_INDEX(first->object_index) - DATUM_INDEX_TO_ABSOLUTE_INDEX(second->object_index);
}

boolean aim_assist_clear_line_of_sight(
	real_point3d const *point0,
	real_point3d const *point1,
	long ignore_object_index,
	long target_object_index)
{
	boolean clear = FALSE;
	real_vector3d vector;
	struct collision_result collision;
	long ignore_parent_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\aim_assist.c",
		350,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_aim_assist;

	ignore_parent_index = object_get_ultimate_parent(ignore_object_index);
	vector.i = point1->x - point0->x;
	vector.j = point1->y - point0->y;
	vector.k = point1->z - point0->z;

	if (!collision_test_vector(
		_collision_test_for_line_of_sight_flags,
		point0,
		&vector,
		ignore_parent_index,
		&collision))
	{
		clear = TRUE;
	}
	else if (collision.type == _collision_result_object &&
		object_get_ultimate_parent(collision.object_index) ==
		object_get_ultimate_parent(target_object_index))
	{
		clear = TRUE;
	}

	match_assert(
		"c:\\halo\\SOURCE\\game\\aim_assist.c",
		367,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return clear;
}

static void object_compute_autoaim_target(
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	real_point3d *target)
{
	real width;

	// closest point on the pill axis to the aiming line
	{
		real_point3d base;
		real_vector3d height, normal;
		real normal_magnitude_squared;

		biped_get_autoaim_pill(object_index, &base, &height, &width);
		cross_product3d(&height, direction, &normal);
		normal_magnitude_squared= magnitude_squared3d(&normal);
		if (normal_magnitude_squared>0.f)
		{
			real_vector3d offset, cross;
			real t;

			vector_from_points3d(&base, position, &offset);
			cross_product3d(&offset, direction, &cross);
			t= PIN(dot_product3d(&cross, &normal)/normal_magnitude_squared, 0.f, 1.f);
			target->x= height.i*t + base.x;
			target->y= height.j*t + base.y;
			target->z= height.k*t + base.z;
		}
		else
		{
			*target= base;
		}
	}

	// pull the target toward the aiming line by at most the pill width
	{
		real_vector3d offset, perpendicular;
		real projection;

		vector_from_points3d(position, target, &offset);
		projection= -dot_product3d(&offset, direction);
		perpendicular.i= direction->i*projection + offset.i;
		perpendicular.j= direction->j*projection + offset.j;
		perpendicular.k= direction->k*projection + offset.k;
		limit3d(&perpendicular, width);
		target->x-= perpendicular.i;
		target->y-= perpendicular.j;
		target->z-= perpendicular.k;
	}

	return;
}

boolean aim_assist_compute_target(
	struct aim_assist_parameters const *parameters,
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	struct aim_assist_target *target)
{
	real distance, cosine, angle;

	target->object_index= object_index;
	object_compute_autoaim_target(object_index, position, direction, &target->position);
	vector_from_points3d(position, &target->position, &target->vector);
	target->direction= target->vector;
	distance= normalize3d(&target->direction);
	target->distance= distance;
	cosine= dot_product3d(direction, &target->direction);
	angle= arccosine(PIN(cosine, -1.f, 1.f));
	target->angle= angle;

	if (parameters)
	{
		target->autoaim_level= compute_composite_attenuation(distance, parameters->autoaim_distance,
			angle, parameters->autoaim_angle);
		target->magnetism_level= compute_composite_attenuation(distance, parameters->magnetism_distance,
			angle, parameters->magnetism_angle);
		if (target->magnetism_level>0.f)
		{
			if (TEST_FLAG(unit_definition_get(unit_get(target->object_index)->definition_index)->unit.flags,
				_unit_is_inconsequential_bit))
			{
				target->magnetism_level*= TAG_BLOCK_GET_ELEMENT(&scenario_get_game_globals()->player_control, 0,
					struct game_globals_player_control)->magnetism_inconsequential_target_scale;
			}
		}
	}
	else
	{
		target->autoaim_level= 0.f;
		target->magnetism_level= 0.f;
	}

	return target->autoaim_level>0.f || target->magnetism_level>0.f;
}

boolean autoaim_compute_target(
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	long unit_index,
	real_point3d *target_position,
	real_vector3d *target_direction,
	real *target_distance,
	real *target_angle)
{
	boolean result= FALSE;

	object_compute_autoaim_target(object_index, position, direction, target_position);
	if (aim_assist_clear_line_of_sight(position, target_position, unit_index, object_index))
	{
		vector_from_points3d(position, target_position, target_direction);
		*target_distance= normalize3d(target_direction);
		if (*target_distance!=0.f)
		{
			*target_angle= arccosine(PIN(dot_product3d(direction, target_direction), -1.f, 1.f));
			result= TRUE;
		}
	}

	return result;
}

static short find_aim_assist_targets_recursive(
	struct aim_assist_parameters const *parameters,
	long object_index,
	real_point3d const *position,
	real_vector3d const *direction,
	real distance,
	real angle_sine,
	real angle_cosine,
	long ignore_object_index,
	short ignore_team_index,
	short maximum_target_count,
	struct aim_assist_target *targets)
{
	short target_count= 0;

	do
	{
		struct object_datum *object= object_get(object_index);

		if (TEST_FLAG(_object_mask_unit, object->object.type) &&
			!TEST_FLAG(object->object.flags, _object_invisible_bit) &&
			unit_get(object_index)->unit.active_camouflage<1.f &&
			sphere_intersects_cone3d(&object->object.bounding_sphere_center, object->object.bounding_sphere_radius,
				position, direction, distance, angle_sine, angle_cosine))
		{
			if (TEST_FLAG(_object_mask_biped, object->object.type) &&
				!TEST_FLAG(object->object.damage_flags, _object_dead_bit) &&
				object_index!=ignore_object_index &&
				game_team_is_enemy(ignore_team_index, object->object.owner_team_index) &&
				!TEST_FLAG(biped_definition_get(object->definition_index)->unit.flags, _unit_ignored_by_autoaiming_bit))
			{
				struct aim_assist_target target;

				if (aim_assist_compute_target(parameters, object_index, position, direction, &target) &&
					target_count<maximum_target_count)
				{
					targets[target_count++]= target;
				}
			}

			if (object->object.first_child_object_index!=NONE && target_count<maximum_target_count)
			{
				target_count+= find_aim_assist_targets_recursive(parameters, object->object.first_child_object_index,
					position, direction, distance, angle_sine, angle_cosine, ignore_object_index, ignore_team_index,
					maximum_target_count - target_count, &targets[target_count]);
			}
		}

		object_index= object->object.next_object_index;
	}
	while (object_index!=NONE && target_count<maximum_target_count);

	return target_count;
}

static short find_aim_assist_targets(
	struct aim_assist_parameters const *parameters,
	short cluster_index,
	real_point3d const *position,
	real_vector3d const *direction,
	long ignore_object_index,
	short ignore_team_index,
	short maximum_target_count,
	struct aim_assist_target *targets)
{
	short target_count= 0;
	real distance= MAX(parameters->autoaim_distance, parameters->magnetism_distance);
	real angle= MAX(parameters->autoaim_angle, parameters->magnetism_angle);

	if (distance>0.f && angle>0.f)
	{
		real angle_sine= sine(angle);
		real angle_cosine= cosine(angle);
		short cluster_indices[MAXIMUM_AIM_ASSIST_CLUSTERS];
		long object_indices[MAXIMUM_AIM_ASSIST_OBJECTS];
		short cluster_count= structure_clusters_in_cone(cluster_index, position, direction, distance, angle_sine, angle_cosine,
			NUMBEROF(cluster_indices), cluster_indices);
		short object_count= objects_in_clusters_by_indices(FLAG(0), cluster_count, cluster_indices,
			NUMBEROF(object_indices), object_indices); // collideable objects only
		short object_index;

		for (object_index= 0; object_index<object_count; ++object_index)
		{
			target_count+= find_aim_assist_targets_recursive(parameters, object_indices[object_index], position, direction,
				distance, angle_sine, angle_cosine, ignore_object_index, ignore_team_index,
				maximum_target_count - target_count, &targets[target_count]);
			if (target_count>=maximum_target_count)
			{
				break;
			}
		}
	}

	return target_count;
}

boolean aim_assist(
	struct aim_assist_parameters const *parameters,
	real_point3d const *position,
	real_vector3d const *direction,
	long ignore_object_index,
	short ignore_team_index,
	struct aim_assist_target *target)
{
	if (scenario_leaf_index_from_point(position)!=NONE)
	{
		short cluster_index= TAG_BLOCK_GET_ELEMENT(
			&global_structure_bsp_get()->leaves,
			scenario_leaf_index_from_point(position) & LONG_MAX,
			struct structure_leaf)->cluster_index;

		if (cluster_index!=NONE)
		{
			struct aim_assist_target targets[MAXIMUM_AIM_ASSIST_TARGETS];
			short target_count= find_aim_assist_targets(parameters, cluster_index, position, direction,
				ignore_object_index, ignore_team_index, NUMBEROF(targets), targets);

			if (target_count>0)
			{
				short target_index;

				qsort(targets, target_count, sizeof(struct aim_assist_target), compare_targets);
				for (target_index= 0; target_index<target_count; ++target_index)
				{
					if (aim_assist_clear_line_of_sight(position, &targets[target_index].position,
						ignore_object_index, targets[target_index].object_index))
					{
						*target= targets[target_index];
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

long local_player_aim_assist(
	short local_player_index,
	real *autoaim_level,
	real *magnetism_level,
	real_euler_angles2d *target_angular_position,
	real_euler_angles2d *target_angular_velocity)
{
	director_perspective perspective= director_get_perspective(local_player_index);

	*autoaim_level= 0.f;
	*magnetism_level= 0.f;
	set_real_euler_angles2d(target_angular_position, 0.f, 0.f);
	set_real_euler_angles2d(target_angular_velocity, 0.f, 0.f);

	if (perspective==_director_perspective_first_person || perspective==_director_perspective_third_person)
	{
		struct player_datum *player= player_get(local_player_get_player_index(local_player_index));
		long aiming_unit_index= unit_get_aiming_unit_index(player->unit_index);
		short zoom_level= player_control_get_zoom_level(local_player_index);
		struct aim_assist_parameters parameters;

		if (unit_get_aim_assist_parameters(aiming_unit_index, zoom_level, &parameters))
		{
			struct observer_result const *camera= observer_get_camera(local_player_index);
			struct aim_assist_target target;

			if (aim_assist(&parameters, &camera->position, &camera->forward, aiming_unit_index, player->team_index, &target))
			{
				real_vector3d aiming_velocity, target_velocity, relative_velocity;
				real horizontal_distance_squared, horizontal_distance;

				*autoaim_level= target.autoaim_level;
				*magnetism_level= target.magnetism_level;
				euler_angles2d_from_vector3d(target_angular_position, &target.vector);

				object_get_velocities(player->unit_index, &aiming_velocity, NULL);
				object_get_velocities(target.object_index, &target_velocity, NULL);

				subtract_vectors3d(&target_velocity, &aiming_velocity, &relative_velocity);
				horizontal_distance_squared= target.vector.i*target.vector.i + target.vector.j*target.vector.j;
				horizontal_distance= square_root(horizontal_distance_squared);
				target_angular_velocity->yaw= (target.vector.i*relative_velocity.j - target.vector.j*relative_velocity.i) /
					horizontal_distance_squared;
				target_angular_velocity->pitch= (relative_velocity.k*horizontal_distance -
					(target.vector.i*relative_velocity.i + target.vector.j*relative_velocity.j)/horizontal_distance*target.vector.k) /
					(target.vector.k*target.vector.k + horizontal_distance_squared);

				return target.object_index;
			}
		}
	}

	return NONE;
}
