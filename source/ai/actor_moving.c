/*
ACTOR_MOVING.C

symbols in this file:
00018940 0030:
	_actor_move_keep_moving_past_destination (0000)
00018970 0040:
	_actor_move_animation_busy (0000)
000189B0 0030:
	_actor_path_clear (0000)
000189E0 0020:
	_actor_path_has_path (0000)
00018A00 0040:
	_actor_path_at_destination (0000)
00018A40 0040:
	_actor_path_get_destination_firing_position_index (0000)
00018A80 00c0:
	_actor_path_input_new (0000)
00018B40 0010:
	_arccosine (0000)
00018B50 0040:
	_midpoint3d (0000)
00018B90 0090:
	_actor_test_destination (0000)
00018C20 01d0:
	_actor_get_stopping_distances (0000)
00018DF0 0080:
	_actor_move_animation_impulse (0000)
00018E70 0090:
	_actor_move_force_stop (0000)
00018F00 0250:
	_actor_move_try_evasion_vector (0000)
00019150 01a0:
	_actor_move_try_evasion_direction (0000)
000192F0 0100:
	_actor_aim_jump (0000)
000193F0 0240:
	_code_000193f0 (0000)
00019630 02f0:
	_code_00019630 (0000)
00019920 00f0:
	_code_00019920 (0000)
00019A10 0090:
	_actor_move_transform_avoidance_vector (0000)
00019AA0 0140:
	_actor_move_get_avoidance_direction (0000)
00019BE0 0150:
	_actor_move_initialize (0000)
00019D30 0110:
	_actor_path_3d_available (0000)
00019E40 0280:
	_code_00019e40 (0000)
0001A0C0 02d0:
	_code_0001a0c0 (0000)
0001A390 1030:
	_code_0001a390 (0000)
0001B3C0 05a0:
	_actor_path_refresh (0000)
0001B960 03d0:
	_actor_destination_update (0000)
0001BD30 0130:
	_actor_move_to_point (0000)
0001BE60 00b0:
	_actor_move_to_move_position (0000)
0001BF10 00b0:
	_actor_move_to_firing_position (0000)
0001BFC0 00f0:
	_actor_move_to_prop (0000)
0001C0B0 0ac0:
	_code_0001c0b0 (0000)
0001CB70 0c40:
	_actor_move_update (0000)
0001D7B0 0090:
	_actor_move_halt (0000)
0001D840 0080:
	_actor_move_halt_at_firing_position (0000)
00245318 01e8:
	_sense_ray_length (0000)
	_sense_ray_offset (0004)
	_sense_ray_divergence (0008)
	_sense_ray_offsets (000c)
	_sense_ray_divergences (0030)
	_sense_ray_angles (0054)
	_avoidance_ray_length (0078)
	_avoidance_ray_angles (007c)
	_avoidance_ray_offsets (009c)
	_avoidance_ray_divergence (00a4)
	_sense_ray_avoidance_weights (00b0)
	_avoid_ray_avoidance_weights (01d0)
	_avoid_ray_clear_bias_time (01d8)
	_avoid_ray_adjacent_fractions (01dc)
	_avoid_ray_fully_obstructed_t (01e4)
00245500 0004:
	__real@3daaaaab (0000)
00245504 0004:
	__real@bf000000 (0000)
00245508 0019:
	??_C@_0BJ@MLAKEMMB@evasion_vector?5?$CG?$CG?5result?$AA@ (0000)
00245524 0021:
	??_C@_0CB@NNMAEIGB@c?3?2halo?2SOURCE?2ai?2actor_moving?4c@ (0000)
00245548 0038:
	??_C@_0DI@MLEEHIMI@alignment_vector?5?$CG?$CG?5evade_direct@ (0000)
00245580 000c:
	??_C@_0M@LILBOJLP@collision_t?$AA@ (0000)
0024558C 001c:
	??_C@_0BM@PBPJGODL@ray_origin?5?$CG?$CG?5ray_direction?$AA@ (0000)
002455A8 0020:
	??_C@_0CA@DELIOAJE@avoidance_data?5?$CG?$CG?5avoidance_ray?$AA@ (0000)
002455C8 002a:
	??_C@_0CK@ELKMJFEA@?$CIangle?5?$DO?$DN?50?40f?$CJ?5?$CG?$CG?5?$CIangle?5?$DM?5_ful@ (0000)
002455F4 0004:
	__real@40c90fdb (0000)
002455F8 0054:
	??_C@_0FE@HKFOPCHI@warning?3?5actor_move_get_avoidanc@ (0000)
0024564C 0016:
	??_C@_0BG@BFFHAOOA@desired_facing_vector?$AA@ (0000)
00245664 0014:
	??_C@_0BE@NFPBALFN@facing_direction?9?$DOk?$AA@ (0000)
00245678 0025:
	??_C@_0CF@KDINJLFK@?$CFs?0?5?$CFs?3?5assert_valid_realcmp?$CI?$CFf?0@ (0000)
002456A0 0016:
	??_C@_0BG@DNKOIPNK@movement_direction?9?$DOk?$AA@ (0000)
002456B8 0005:
	??_C@_04MPNNEBNI@0?40f?$AA@ (0000)
002456C0 0023:
	??_C@_0CD@NOAIDOJA@?$CIreal_vector2d?5?$CK?$CJ?5facing_directi@ (0000)
002456E4 0025:
	??_C@_0CF@IPKJEDPE@?$CIreal_vector2d?5?$CK?$CJ?5movement_direc@ (0000)
0024570C 0011:
	??_C@_0BB@CEGHPIDG@facing_direction?$AA@ (0000)
00245720 0013:
	??_C@_0BD@FPDMFHB@movement_direction?$AA@ (0000)
00245734 0004:
	__real@3f860a92 (0000)
00245738 0004:
	__real@3f44ec4f (0000)
0024573C 0004:
	__real@3fa66666 (0000)
00245740 0004:
	__real@3b23d70b (0000)
00245744 0004:
	__real@be4ccccd (0000)
00245748 0004:
	__real@40200001 (0000)
00245750 0080:
	??_C@_0IA@DPLAEGLA@?$CImovement_direction_approximatio@ (0000)
002457D0 0066:
	??_C@_0GG@CIIOBGBO@?$CIbest_avoidance_direction?5?$DO?$DN?50?$CJ?5@ (0000)
00245838 0004:
	__real@ff7fffff (0000)
0024583C 0004:
	__real@41480000 (0000)
00245840 0004:
	__real@3ca3d70a (0000)
00245844 0004:
	__real@42960000 (0000)
00245848 0027:
	??_C@_0CH@FHDECA@avoidance_rotation?5?$CG?$CG?5emergency_@ (0000)
00245870 004a:
	??_C@_0EK@EPCHDGAN@actor?9?$DOcontrol?4path?4destination_@ (0000)
002458BC 0004:
	__real@3c23d70b (0000)
002458C0 002e:
	??_C@_0CO@CIMGKAKP@pathfinding?5is?5attempting?5to?5wal@ (0000)
002458F0 0004:
	__real@49742400 (0000)
002458F4 002a:
	??_C@_0CK@FMFBKFJI@?$CFs?3?5fell?5off?5end?5of?5unfinished?5p@ (0000)
00245920 000b:
	??_C@_0L@DMJIGJL@final_step?$AA@ (0000)
0024592C 0004:
	__real@3cb851ec (0000)
00245930 0004:
	__real@3d800000 (0000)
00245934 000c:
	??_C@_0M@BLMAJMLK@destination?$AA@ (0000)
00245940 0058:
	??_C@_0FI@KHOOINPJ@?$CIfacing_direction?5?$DN?$DN?5_actor_faci@ (0000)
00245998 0021:
	??_C@_0CB@CFOCDAMI@adjust?5angle?5?$CF?44f?5?9?$DO?5?$CF?44f?5?$CI?$CF?44f?$CJ@ (0000)
002459BC 001f:
	??_C@_0BP@KEBEJPIO@steer?5?$CF?44f?5?9?5oversteer?5to?5?$CF?44f?$AA@ (0000)
002459DC 0029:
	??_C@_0CJ@GAOIDHOG@steer?5?$CF?44f?5?$DM?5?$CF?44f?5?9?5clear?5overst@ (0000)
00245A08 000b:
	??_C@_0L@KHLHAIBH@steer?5?$CF?44f?$AA@ (0000)
00245A14 0020:
	??_C@_0CA@KJFKJOKH@steer?5?$CF?44f?5?$CIset?5oversteer?5?$CF?44f?$CJ?$AA@ (0000)
00245A34 0004:
	__real@bf800000 (0000)
00245A38 0039:
	??_C@_0DJ@NBFPGAMC@?$CImaximum_throttle?5?$DO?$DN?50?40f?$CJ?5?$CG?$CG?5?$CIm@ (0000)
00245A74 0004:
	__real@3f733333 (0000)
00245A78 000f:
	??_C@_0P@JJNKCIHG@?$CGfacing_vector?$AA@ (0000)
00245A88 000f:
	??_C@_0P@NHABDOCD@?$CB?$CCunreachable?$CC?$AA@ (0000)
00245A98 0004:
	__real@3a83126f (0000)
00245A9C 001c:
	??_C@_0BM@IMMKLGND@?$CGactor?9?$DOinput?4facing_vector?$AA@ (0000)
*/

/* ---------- headers */

#define arccosine arccosine_inline
#define normalize3d normalize3d_inline
#define scale_vector3d scale_vector3d_inline
#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D
#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "ai/actions.h"
#include "ai/actor_definitions.h"
#include "ai/actor_types.h"
#include "ai/actors.h"
#include "ai/ai_communication.h"
#include "ai/ai_debug.h"
#include "ai/path_structure_bsp.h"
#include "ai/props.h"
#include "game/game.h"
#include "physics/collision_bsp.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "units/biped_definitions.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "units/vehicle_datum.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"
#undef REAL_MATH_EXTERNAL_POINT_FROM_LINE3D
#undef arccosine
#undef normalize3d
#undef scale_vector3d

/* ---------- constants */

enum
{
	NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS = 8,
};

enum
{
	_actor_evade_left = 0,
	_actor_evade_right,
	_actor_evade_forward,
	_actor_evade_back,
	_actor_evade_random_side,
};

enum
{
	_actor_mode_asleep = 1,
	_actor_mode_alert,
	_actor_mode_combat,
};

/* ---------- macros */

#define _full_circle (2.f*_pi)

/* ---------- structures */

/* The shared vehicle tag layout remains opaque in the public header. Actor
 * movement reads this January-authenticated speed and AI-driving fragment, so
 * keep that layout fragment local to this translation unit. */
struct vehicle_definition
{
	byte __unknown0[0x2F8];
	real maximum_forward_speed;
	real maximum_reverse_speed;
	real speed_acceleration;
	real speed_deceleration;
	byte __unknown308[0x78];
	real ai_sideslip_distance;
	byte __unknown384[4];
	real ai_avoidance_distance;
	real ai_pathfinding_radius;
	byte __unknown390[8];
	real ai_oversteer_angle_lower_bound;
	real ai_oversteer_angle_upper_bound;
	real ai_steering_max_angle;
	real ai_steering_max_throttle;
};

struct vector_avoidance_ray
{
	real length;
	real_vector3d offset;
	real_vector3d divergence;
};

typedef char actor_moving_vehicle_maximum_forward_speed_offset_assert[
	offsetof(struct vehicle_definition, maximum_forward_speed) == 0x2F8 ? 1 : -1];
typedef char actor_moving_vehicle_speed_acceleration_offset_assert[
	offsetof(struct vehicle_definition, speed_acceleration) == 0x300 ? 1 : -1];
typedef char actor_moving_vehicle_sideslip_distance_offset_assert[
	offsetof(struct vehicle_definition, ai_sideslip_distance) == 0x380 ? 1 : -1];
typedef char actor_moving_vehicle_avoidance_distance_offset_assert[
	offsetof(struct vehicle_definition, ai_avoidance_distance) == 0x388 ? 1 : -1];
typedef char actor_moving_vehicle_pathfinding_radius_offset_assert[
	offsetof(struct vehicle_definition, ai_pathfinding_radius) == 0x38C ? 1 : -1];
typedef char actor_moving_vehicle_oversteer_lower_bound_offset_assert[
	offsetof(struct vehicle_definition, ai_oversteer_angle_lower_bound) == 0x398 ? 1 : -1];
typedef char actor_moving_vehicle_oversteer_upper_bound_offset_assert[
	offsetof(struct vehicle_definition, ai_oversteer_angle_upper_bound) == 0x39C ? 1 : -1];
typedef char actor_moving_vehicle_steering_max_angle_offset_assert[
	offsetof(struct vehicle_definition, ai_steering_max_angle) == 0x3A0 ? 1 : -1];
typedef char actor_moving_vehicle_steering_max_throttle_offset_assert[
	offsetof(struct vehicle_definition, ai_steering_max_throttle) == 0x3A4 ? 1 : -1];

/* ---------- prototypes */

/* The owner declaration in actions.h is macro-renamed while importing the
 * January inline set; restore the external name after that schedule ends. */
real normalize3d(
	real_vector3d *vector);

real_vector3d *scale_vector3d(
	real_vector3d const *vector,
	real scale,
	real_vector3d *result);

void actor_move_vector_avoidance(
	long actor_index,
	real_vector3d const *movement_direction,
	real_vector3d *avoidance_rotation,
	real *emergency_amount);

void actor_move_calculate_movement(
	long actor_index,
	boolean move_in_3d,
	short override_facing,
	boolean free_movement,
	real free_movement_distance_squared,
	boolean allow_all_moving_turns,
	real steering_maximum_angle,
	real oversteer_minimum_angle,
	real oversteer_maximum_angle,
	real rotation_emergency_amount,
	real maximum_throttle,
	real_vector3d const *desired_movement_vector,
	real_vector3d *desired_facing_vector,
	short *desired_facing_direction,
	real_vector3d *desired_throttle,
	boolean *movement_thwarted,
	boolean *movement_complete);

/* ---------- globals */

real const sense_ray_length = 1.f;
real const sense_ray_offset = 0.7f;
real const sense_ray_divergence = 0.052359879f;
real const sense_ray_offsets[9] =
{
	0.f,
	0.5f,
	0.5f,
	0.5f,
	0.5f,
	1.f,
	1.f,
	1.f,
	1.f,
};
real const sense_ray_divergences[9] =
{
	0.f,
	0.3f,
	0.3f,
	0.3f,
	0.3f,
	1.f,
	1.f,
	1.f,
	1.f,
};
real const sense_ray_angles[9] =
{
	0.f,
	0.f,
	1.5707964f,
	3.1415927f,
	4.712389f,
	0.f,
	1.5707964f,
	3.1415927f,
	4.712389f,
};
real const avoidance_ray_length = 0.7f;
real const avoidance_ray_angles[NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS] =
{
	0.f,
	0.78539819f,
	1.5707964f,
	2.3561945f,
	3.1415927f,
	3.9269909f,
	4.712389f,
	5.4977875f,
};
real const avoidance_ray_offsets[2] =
{
	0.7f,
	1.f,
};
real const avoidance_ray_divergence[2] =
{
	0.5235988f,
	0.95993108f,
};
real const sense_ray_avoidance_weights[9][NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS] =
{
	{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
	{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
	{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
	{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
	{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f },
	{ -0.5f, -0.5f, 0.f, 0.7f, 1.f, 0.7f, 0.f, -0.5f },
	{ 0.f, -0.5f, -0.5f, -0.5f, 0.f, 0.7f, 1.f, 0.7f },
	{ 1.f, 0.7f, 0.f, -0.5f, -0.5f, -0.5f, 0.f, 0.7f },
	{ 0.f, 0.7f, 1.f, 0.7f, 0.f, -0.5f, -0.5f, -0.5f },
};
real const avoid_ray_avoidance_weights[2] =
{
	0.8f,
	1.2f,
};
long const avoid_ray_clear_bias_time = 75;
real const avoid_ray_adjacent_fractions[2] =
{
	0.8f,
	0.5f,
};
real const avoid_ray_fully_obstructed_t = 0.5f;

extern struct vector_avoidance_ray avoidance_rays[NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS][2];
extern real_vector3d avoidance_directions[NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS];
extern struct vector_avoidance_ray sense_rays[9];

/* ---------- public code */

void actor_move_keep_moving_past_destination(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->orders.move.destination.keep_moving = TRUE;
	actor->control.path.destination_orders.keep_moving = TRUE;

	return;
}

boolean actor_move_animation_busy(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->orders.move.animation.impulse == NONE)
	{
		long unit_index = actor->meta.unit_index;
		if (unit_index == NONE || !unit_is_busy(unit_index))
			return FALSE;
	}

	return TRUE;
}

void actor_path_clear(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	actor->control.path.path.valid = FALSE;
	actor->control.path.at_destination = TRUE;
	actor->control.path.destination_original_distance = 0.0f;

	return;
}

boolean actor_path_has_path(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	return actor->control.path.path.valid;
}

boolean actor_path_at_destination(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	return !actor->control.path.path.valid || actor->control.path.at_destination;
}

short actor_path_get_destination_firing_position_index(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	short firing_position_index = NONE;

	if (actor->control.path.destination_orders.destination_type == _destination_firing_position)
		firing_position_index = actor->control.path.destination_orders.firing_position_index;

	return firing_position_index;
}

void actor_path_input_new(
	long actor_index,
	struct path_input *input)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition =
		actor_definition_get(actor->meta.definition_index);
	real pathfinding_radius = definition->moving.pathfinding_radius;
	long ignore_source_object_index = actor->meta.unit_index;

	if (actor->input.vehicle_driver_type > _actor_vehicle_driver_none)
	{
		struct unit_datum *vehicle = vehicle_get(actor->input.vehicle_index);
		struct vehicle_definition *vehicle_definition =
			vehicle_specific_definition_get(vehicle->definition_index);

		ignore_source_object_index = actor->input.vehicle_index;
		if (vehicle_definition->ai_pathfinding_radius > 0.f)
			pathfinding_radius = vehicle_definition->ai_pathfinding_radius;
	}

	actor_find_pathfinding_location(actor_index);
	path_input_new(
		input,
		pathfinding_radius,
		actor->emotions.ignorant_of_broken_surfaces,
		ignore_source_object_index);
	path_input_set_start(
		input,
		&actor->input.pathfinding_point,
		actor->input.pathfinding_surface_index);

	return;
}

boolean actor_move_animation_impulse(
	long actor_index,
	short impulse,
	real_vector2d const *alignment_vector)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	actor_set_dormant(actor_index, FALSE);

	if (!actor_move_animation_busy(actor_index))
	{
		actor->orders.move.animation.impulse = impulse;
		actor->orders.move.animation.alignment_vector = *alignment_vector;
		result = TRUE;
	}

	return result;
}

boolean actor_move_force_stop(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	if (!actor_move_animation_busy(actor_index) &&
		!actor_action_deny_transition(actor_index))
	{
		actor->control.moving = FALSE;
		actor->output.throttle = *global_zero_vector3d;
		actor_unit_control_stop_animation_impulse(actor_index);
		result = TRUE;
	}

	return result;
}

boolean actor_aim_jump(
	long actor_index,
	long unit_index,
	boolean leap,
	real jump_magnitude,
	real_vector3d *jump_velocity)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->input.vehicle_index == NONE)
	{
		if (actor->meta.swarm)
		{
			actor_type_swarm_aim_jump(
				actor_index,
				unit_index,
				jump_magnitude,
				jump_velocity);
		}
		else if (actor->control.jumping_targeted)
		{
			real magnitude;

			if (actor->state.action == _actor_action_charge &&
				actor->state.action_data.charge.goal == _charge_goal_melee_leaping)
			{
				leap = TRUE;
			}

			jump_velocity->i = actor->control.jump_alignment_vector.i *
				actor->control.jump_target_horizontal_vel;
			jump_velocity->j = actor->control.jump_alignment_vector.j *
				actor->control.jump_target_horizontal_vel;
			jump_velocity->k = actor->control.jump_target_vertical_vel;
			magnitude = magnitude3d(jump_velocity);

			if (!leap && magnitude > jump_magnitude)
			{
				scale_vector3d(jump_velocity, jump_magnitude / magnitude, jump_velocity);
			}
		}
	}

	actor->control.jumping_targeted = FALSE;
	return TRUE;
}

void actor_move_transform_avoidance_vector(
	struct vector_avoidance_data *avoidance_data,
	real_vector3d const *avoidance_vector,
	real_vector3d *direction_vector)
{
	real component;

	*direction_vector = *global_zero_vector3d;

	component = avoidance_vector->i;
	direction_vector->i += component * avoidance_data->forward.i;
	direction_vector->j += component * avoidance_data->forward.j;
	direction_vector->k += component * avoidance_data->forward.k;

	component = avoidance_vector->j;
	direction_vector->i += component * avoidance_data->left.i;
	direction_vector->j += component * avoidance_data->left.j;
	direction_vector->k += component * avoidance_data->left.k;

	component = avoidance_vector->k;
	direction_vector->i += component * avoidance_data->up.i;
	direction_vector->j += component * avoidance_data->up.j;
	direction_vector->k += component * avoidance_data->up.k;

	return;
}

void actor_move_get_avoidance_direction(
	struct vector_avoidance_data *avoidance_data,
	real direction,
	real_vector3d *direction_vector)
{
	real_vector3d avoidance_vector;
	real angle = REAL_MAX;
	short direction_index;

	if (direction < 0.f || direction >= (real)NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS)
		direction = 0.f;

	for (direction_index = 0;
		direction_index < NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS;
		direction_index++)
	{
		if ((real)direction_index + 1.f > direction)
		{
			real fraction = direction - (real)direction_index;
			real angle0 = avoidance_ray_angles[direction_index];
			real angle1 =
				direction_index == NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS - 1
					? avoidance_ray_angles[0]
					: avoidance_ray_angles[direction_index + 1];

			angle = angle0*(1.f - fraction) + angle1*fraction;
			break;
		}
	}

	if (angle == REAL_MAX)
	{
		angle = 0.f;
		error(
			_error_silent,
			"warning: actor_move_get_avoidance_vector couldn't find out-of-bounds direction %.4f",
			direction);
	}

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_moving.c",
		2743,
		(angle >= 0.0f) && (angle < _full_circle));

	avoidance_vector.i = 0.f;
	avoidance_vector.j = cosine(angle);
	avoidance_vector.k = sine(angle);
	actor_move_transform_avoidance_vector(
		avoidance_data,
		&avoidance_vector,
		direction_vector);

	return;
}

void actor_move_initialize(
	void)
{
	long sense_ray_index;

	for (sense_ray_index = 0;
		sense_ray_index < NUMBEROF(sense_rays);
		sense_ray_index++)
	{
		struct vector_avoidance_ray *ray = &sense_rays[sense_ray_index];
		real ray_sine = sine(sense_ray_angles[sense_ray_index]);
		real ray_cosine = cosine(sense_ray_angles[sense_ray_index]);
		real divergence = sense_ray_divergence *
			sense_ray_divergences[sense_ray_index];
		real divergence_sine = sine(divergence);
		real divergence_cosine = cosine(divergence);
		real offset = sense_ray_offset * sense_ray_offsets[sense_ray_index];

		ray->length = sense_ray_length;
		ray->offset.i = 0.f;
		ray->offset.j = offset * ray_cosine;
		ray->offset.k = offset * ray_sine;
		ray->divergence.i = divergence_cosine;
		ray->divergence.j = divergence_sine * ray_cosine;
		ray->divergence.k = divergence_sine * ray_sine;
	}

	{
		short avoidance_ray_index;

		for (avoidance_ray_index = 0;
			avoidance_ray_index < NUMBEROF(avoidance_ray_offsets);
			avoidance_ray_index++)
		{
			real divergence_sine = sine(
				avoidance_ray_divergence[avoidance_ray_index]);
			real divergence_cosine = cosine(
				avoidance_ray_divergence[avoidance_ray_index]);
			real offset = avoidance_ray_offsets[avoidance_ray_index];
			long direction_index;

			for (direction_index = 0;
				direction_index < NUMBEROF(avoidance_directions);
				direction_index++)
			{
				struct vector_avoidance_ray *avoidance_ray =
					&avoidance_rays[direction_index][avoidance_ray_index];

				set_real_vector3d(
					&avoidance_directions[direction_index],
					0.f,
					cosine(avoidance_ray_angles[direction_index]),
					sine(avoidance_ray_angles[direction_index]));
				avoidance_ray->length = avoidance_ray_length;
				scale_vector3d_inline(
					&avoidance_directions[direction_index],
					offset,
					&avoidance_ray->offset);
				scale_vector3d_inline(
					&avoidance_directions[direction_index],
					divergence_sine,
					&avoidance_ray->divergence);
				avoidance_ray->divergence.i = divergence_cosine;
			}
		}
	}

	return;
}

boolean actor_path_3d_available(
	long actor_index,
	real_point3d const *destination,
	real *avoidance_distance_reference)
{
	struct actor_datum *actor = actor_get(actor_index);
	real avoidance_distance = 0.f;
	boolean available = TRUE;

	if (actor->input.vehicle_driver_type == _actor_vehicle_driver_directional_flying)
	{
		struct unit_datum *vehicle = vehicle_get(actor->input.vehicle_index);
		struct vehicle_definition *vehicle_definition =
			vehicle_specific_definition_get(vehicle->definition_index);

		avoidance_distance = vehicle_definition->ai_avoidance_distance;
		if (avoidance_distance > 0.f &&
			actor->control.vector_avoidance_rotation_emergency_instantaneous > 0.9f)
		{
			real_vector3d direction;

			if (normalize3d(vector_from_points3d(
					&actor->input.position.body_position,
					destination,
					&direction)) > 0.f &&
				dot_product3d(&direction, &actor->input.facing_vector) > 0.984f)
			{
				available = FALSE;
			}
		}
	}

	if (avoidance_distance_reference)
		*avoidance_distance_reference = avoidance_distance;

	return available;
}

real arccosine(
	real x)
{
	return acos(x);
}

/* ---------- private code */

boolean actor_test_destination(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->control.path.destination_orders.destination_type == _destination_none ||
		actor->control.path.destination_orders.destination_type == _destination_halt)
	{
		actor->control.path.at_destination = TRUE;
	}
	else
	{
		real tolerance = actor_destination_tolerance(actor_index);

		if (distance_squared3d(
			&actor->input.position.body_position,
			&actor->control.path.destination.point) < tolerance*tolerance)
		{
			actor->control.path.at_destination = TRUE;
		}
	}

	return actor->control.path.at_destination;
}

void actor_get_stopping_distances(
	long actor_index,
	real *current_stopping_distance_reference,
	real *maximum_stopping_distance_reference)
{
	struct actor_datum *actor = actor_get(actor_index);
	real reference_speed = 0.083333336f;
	real current_speed = 0.0f;
	real deceleration = 0.016666668f;
	real stopping_deceleration = 0.026666667f;

	if (actor->input.vehicle_index != NONE)
	{
		long vehicle_driver_type = actor->input.vehicle_driver_type;

		if (vehicle_driver_type >= _actor_vehicle_driver_hovering_ground &&
			vehicle_driver_type <= _actor_vehicle_driver_nondirectional_ground)
		{
			struct unit_datum *vehicle = vehicle_get(actor->input.vehicle_index);
			struct vehicle_definition *definition =
				vehicle_specific_definition_get(vehicle->definition_index);

			current_speed = dot_product3d(
				&vehicle->object.translational_velocity,
				&vehicle->object.forward);
			reference_speed = definition->maximum_forward_speed;
			deceleration = definition->speed_acceleration;
			stopping_deceleration = deceleration;
		}
	}
	else if (actor->meta.unit_index != NONE)
	{
		struct biped_datum *biped = biped_try_and_get(actor->meta.unit_index);

		if (biped)
		{
			struct biped_definition *definition = biped_definition_get(biped->definition_index);

			current_speed = dot_product3d(
				&biped->object.translational_velocity,
				&biped->object.forward);
			if (TEST_FLAG(definition->biped.flags, _biped_flying_bit))
			{
				reference_speed = definition->biped.flying_velocity *
					(1.0f / TICKS_PER_SECOND);
				deceleration = definition->biped.flying_acceleration *
					(1.0f / TICKS_PER_SECOND);
				stopping_deceleration = definition->biped.flying_deceleration *
					(1.0f / TICKS_PER_SECOND);

				if (actor->control.crouching &&
					definition->biped.flying_crouch_velocity_modifier > 0.0f)
				{
					reference_speed *= definition->biped.flying_crouch_velocity_modifier;
					deceleration *= definition->biped.flying_crouch_velocity_modifier;
					stopping_deceleration *= definition->biped.flying_crouch_velocity_modifier;
				}
			}
		}
	}

	if (current_stopping_distance_reference)
	{
		*current_stopping_distance_reference = current_speed * current_speed /
			(stopping_deceleration * 2.0f);
	}

	if (maximum_stopping_distance_reference)
	{
		if (current_speed > reference_speed)
			reference_speed = current_speed;

		*maximum_stopping_distance_reference =
			(reference_speed * reference_speed - current_speed * current_speed) /
				(deceleration * 2.0f) +
			reference_speed * reference_speed / (stopping_deceleration * 2.0f);
	}

	return;
}

boolean actor_move_try_evasion_vector(
	long actor_index,
	real_vector2d *evasion_vector,
	real evade_distance,
	real maximum_ledge_height,
	boolean *evasion_is_ledge_reference,
	struct path_collision_result *result)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition = actor_definition_get(actor->meta.definition_index);
	boolean evasion_is_ledge = FALSE;
	boolean found = FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_moving.c",
		0x484,
		evasion_vector && result);

	if (!actor->state.flying)
	{
		real_point2d evasion_point;

		evasion_point.x = evade_distance * evasion_vector->i +
			actor->input.position.body_position.x;
		evasion_point.y = evade_distance * evasion_vector->j +
			actor->input.position.body_position.y;

		actor_find_pathfinding_location(actor_index);
		if (!structure_test_pill2d(
			global_structure_bsp_get(),
			actor->emotions.ignorant_of_broken_surfaces,
			(real_point2d const *)&actor->input.pathfinding_point,
			actor->input.pathfinding_surface_index,
			&evasion_point,
			NONE,
			definition->moving.pathfinding_radius,
			0,
			result))
		{
			real height_delta = result->point.z - actor->input.position.body_position.z;

			found = TRUE;
			if (height_delta > evade_distance * 0.5f ||
				(maximum_ledge_height == 0.0f &&
					height_delta < evade_distance * -0.5f))
			{
				found = FALSE;
			}
		}

		if (!found && maximum_ledge_height > 0.0f)
		{
			struct collision_bsp *collision_bsp = global_collision_bsp_get();
			struct collision_bsp_test_vector_result collision_result;
			real_point3d actor_midpoint;
			real_vector3d collision_vector;

			actor_midpoint.x =
				(actor->input.position.head_position.x + actor->input.position.body_position.x) * 0.5f;
			actor_midpoint.y =
				(actor->input.position.head_position.y + actor->input.position.body_position.y) * 0.5f;
			actor_midpoint.z =
				(actor->input.position.body_position.z + actor->input.position.head_position.z) * 0.5f;
			collision_vector.i = evade_distance * evasion_vector->i;
			collision_vector.j = evade_distance * evasion_vector->j;
			collision_vector.k = 0.0f;

			if (!collision_bsp_test_vector(
				FLAG(_collision_test_front_facing_surfaces_bit) |
					FLAG(_collision_test_back_facing_surfaces_bit),
				collision_bsp,
				0,
				NULL,
				&actor_midpoint,
				&collision_vector,
				REAL_MAX,
				&collision_result))
			{
				found = TRUE;
				evasion_is_ledge = TRUE;
				if (maximum_ledge_height < REAL_MAX)
				{
					real_point3d down_ray_start;
					real_vector3d down_ray_vector;

					point_from_line3d(
						&actor_midpoint,
						&collision_vector,
						1.0f,
						&down_ray_start);
					scale_vector3d(
						global_down3d,
						maximum_ledge_height,
						&down_ray_vector);
					if (!collision_bsp_test_vector(
						FLAG(_collision_test_front_facing_surfaces_bit) |
							FLAG(_collision_test_back_facing_surfaces_bit),
						collision_bsp,
						0,
						NULL,
						&down_ray_start,
						&down_ray_vector,
						REAL_MAX,
						&collision_result))
					{
						found = FALSE;
					}
				}
			}
		}
	}

	if (evasion_is_ledge_reference)
		*evasion_is_ledge_reference = evasion_is_ledge;

	return found;
}

boolean actor_move_try_evasion_direction(
	long actor_index,
	real_vector2d *alignment_vector,
	real evade_distance,
	short *evade_direction_reference,
	real maximum_ledge_height,
	boolean *evasion_is_ledge,
	struct path_collision_result *result)
{
	real_vector2d evasion_vector;
	short attempt_count;
	short attempt_index;
	short evade_direction;

	(void)actor_get(actor_index);
	attempt_count = 1;
	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_moving.c",
		0x4E4,
		alignment_vector && evade_direction_reference && result);

	evade_direction = *evade_direction_reference;
	switch (evade_direction)
	{
	case _actor_evade_left:
		evasion_vector.i = -alignment_vector->j;
		evasion_vector.j = alignment_vector->i;
		break;

	case _actor_evade_right:
		evasion_vector.i = alignment_vector->j;
		evasion_vector.j = -alignment_vector->i;
		break;

	case _actor_evade_forward:
		evasion_vector = *alignment_vector;
		break;

	case _actor_evade_back:
		evasion_vector.i = -alignment_vector->i;
		evasion_vector.j = -alignment_vector->j;
		break;

	case _actor_evade_random_side:
		if (seed_random(get_global_random_seed_address()) > 0x8000)
		{
			evade_direction = _actor_evade_left;
			evasion_vector.i = -alignment_vector->j;
			evasion_vector.j = alignment_vector->i;
			attempt_count = 2;
		}
		else
		{
			evade_direction = _actor_evade_right;
			evasion_vector.i = alignment_vector->j;
			evasion_vector.j = -alignment_vector->i;
			attempt_count = 2;
		}
		break;

	default:
		match_vassert(
			"c:\\halo\\SOURCE\\ai\\actor_moving.c",
			0x508,
			FALSE,
			NULL);
		break;
	}

	attempt_index = 0;
	if (attempt_count > 0)
	{
		do
		{
			if (actor_move_try_evasion_vector(
				actor_index,
				&evasion_vector,
				evade_distance,
				maximum_ledge_height,
				evasion_is_ledge,
				result))
			{
				*evade_direction_reference = evade_direction;
				return TRUE;
			}

			attempt_index++;
			evasion_vector.i = -evasion_vector.i;
			evade_direction ^= 1;
			evasion_vector.j = -evasion_vector.j;
		}
		while (attempt_index < attempt_count);
	}

	*evade_direction_reference = NONE;
	return FALSE;
}

boolean actor_move_to_point(
	long actor_index,
	real_point3d const *destination,
	long surface_index,
	long ignore_target_object_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = TRUE;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_moving.c",
		951,
		destination);

	actor->firing_positions.current_position_index = NONE;
	actor_set_dormant(actor_index, FALSE);

	if (actor->control.path.destination_orders.destination_type != _destination_raw_location ||
		actor->control.path.destination_orders.raw.surface_index != surface_index ||
		distance_squared3d(destination, &actor->control.path.destination_orders.raw.point) > 0.1f*0.1f)
	{
		actor->orders.move.destination.destination_type = _destination_raw_location;
		actor->orders.move.destination.keep_moving = FALSE;
		actor->orders.move.destination.raw.point = *destination;
		actor->orders.move.destination.raw.surface_index = surface_index;
		actor->orders.move.destination.ignore_target_object_index = ignore_target_object_index;
		actor->control.path.destination_orders = actor->orders.move.destination;

		result = actor_path_refresh(actor_index, TRUE, NULL);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, NULL);
	}

	return result;
}

boolean actor_move_to_move_position(
	long actor_index,
	short move_position_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = TRUE;

	actor->firing_positions.current_position_index = NONE;
	actor_set_dormant(actor_index, FALSE);

	if (actor->control.path.destination_orders.destination_type != _destination_move_position ||
		actor->control.path.destination_orders.move_position_index != move_position_index)
	{
		actor->orders.move.destination.destination_type = _destination_move_position;
		actor->orders.move.destination.keep_moving = FALSE;
		actor->orders.move.destination.move_position_index = move_position_index;
		actor->orders.move.destination.ignore_target_object_index = NONE;
		actor->control.path.destination_orders = actor->orders.move.destination;

		result = actor_path_refresh(actor_index, TRUE, NULL);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, NULL);
	}

	return result;
}

boolean actor_move_to_firing_position(
	long actor_index,
	short firing_position_index,
	struct path_state *cached_path_state)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = TRUE;

	actor_set_dormant(actor_index, FALSE);

	if (actor->control.path.destination_orders.destination_type != _destination_firing_position ||
		actor->control.path.destination_orders.firing_position_index != firing_position_index)
	{
		actor->orders.move.destination.destination_type = _destination_firing_position;
		actor->orders.move.destination.keep_moving = FALSE;
		actor->orders.move.destination.firing_position_index = firing_position_index;
		actor->orders.move.destination.ignore_target_object_index = NONE;
		actor->firing_positions.moved_away_from_firing_position = FALSE;
		actor->control.path.destination_orders = actor->orders.move.destination;

		result = actor_path_refresh(actor_index, TRUE, cached_path_state);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, cached_path_state);
	}

	return result;
}

boolean actor_move_to_prop(
	long actor_index,
	long prop_index,
	real accept_radius)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = TRUE;

	actor->firing_positions.current_position_index = NONE;
	actor_set_dormant(actor_index, FALSE);

	if (actor->control.path.destination_orders.destination_type != _destination_prop ||
		actor->control.path.destination_orders.prop.prop_index != prop_index ||
		actor->control.path.destination_orders.prop.accept_radius != accept_radius)
	{
		struct prop_datum *prop = prop_get(prop_index);

		actor->orders.move.destination.destination_type = _destination_prop;
		actor->orders.move.destination.keep_moving = FALSE;
		actor->orders.move.destination.prop.prop_index = prop_index;
		actor->orders.move.destination.prop.accept_radius = accept_radius;
		actor->orders.move.destination.ignore_target_object_index =
			prop->vehicle_index == NONE ? prop->unit_index : prop->vehicle_index;
		actor->control.path.destination_orders = actor->orders.move.destination;

		result = actor_path_refresh(actor_index, TRUE, NULL);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, NULL);
	}

	return result;
}

void actor_move_update(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition =
		actor_definition_get(actor->meta.definition_index);
	short movement_type;
	short override_facing;
	boolean move_in_3d = FALSE;
	boolean free_movement = FALSE;
	boolean allow_all_moving_turns = FALSE;
	boolean allow_jump = FALSE;
	boolean force_stationary_facing = FALSE;
	boolean clear_firing_positions = FALSE;
	boolean crouch;
	real free_movement_distance_squared = 0.f;
	real steering_maximum_angle = 0.f;
	real oversteer_minimum_angle = 0.f;
	real oversteer_maximum_angle = 0.f;
	real rotation_emergency_amount = 0.f;
	real maximum_throttle = 1.f;

	match_assert_valid_real_normal3d(
		"c:\\halo\\SOURCE\\ai\\actor_moving.c",
		286,
		&actor->input.facing_vector);

	actor->control.desired_facing_vector = actor->input.facing_vector;
	actor->control.face_exactly = FALSE;
	actor->control.free_facing_vector = TRUE;
	actor->control.optional_facing_vector = TRUE;

	if (actor->orders.move.override_movement_direction)
	{
		actor->control.moving = TRUE;
		actor->control.free_facing_vector = FALSE;
		actor->control.moving_towards_vector =
			actor->orders.move.override_movement_direction_vector;
		actor->control.vector_avoidance_rotation = *global_zero_vector3d;
		actor->control.vector_avoidance_rotation_emergency = 0.f;
		actor->control.vector_avoidance_rotation_emergency_instantaneous = 0.f;
	}
	else if (actor->input.vehicle_driver_type ==
		_actor_vehicle_driver_directional_flying)
	{
		real_vector3d scaled_facing;
		real_vector3d avoidance_rotation;
		real_vector3d const *movement_direction;
		real emergency_amount;
		real rotation_magnitude_squared;
		real previous_magnitude_squared;
		real blend;

		if (actor->control.moving)
		{
			movement_direction = &actor->control.moving_towards_vector;
		}
		else
		{
			scale_vector3d_inline(
				&actor->input.facing_vector,
				3.f,
				&scaled_facing);
			movement_direction = &scaled_facing;
		}

		actor_move_vector_avoidance(
			actor_index,
			movement_direction,
			&avoidance_rotation,
			&emergency_amount);

		rotation_magnitude_squared = magnitude_squared3d(&avoidance_rotation);
		previous_magnitude_squared =
			magnitude_squared3d(&actor->control.vector_avoidance_rotation);
		blend = rotation_magnitude_squared > previous_magnitude_squared
			? 0.3f
			: 0.05f;

		actor->control.vector_avoidance_rotation.i =
			actor->control.vector_avoidance_rotation.i * (1.f - blend) +
			avoidance_rotation.i * blend;
		actor->control.vector_avoidance_rotation.j =
			actor->control.vector_avoidance_rotation.j * (1.f - blend) +
			avoidance_rotation.j * blend;
		actor->control.vector_avoidance_rotation.k =
			actor->control.vector_avoidance_rotation.k * (1.f - blend) +
			avoidance_rotation.k * blend;

		if (magnitude_squared3d(&actor->control.vector_avoidance_rotation) <
			_real_epsilon)
		{
			actor->control.vector_avoidance_rotation = *global_zero_vector3d;
		}

		actor->control.vector_avoidance_rotation_emergency_instantaneous =
			emergency_amount;
		actor->control.vector_avoidance_rotation_emergency =
			actor->control.vector_avoidance_rotation_emergency * (1.f - blend) +
			emergency_amount * blend;
		if (actor->control.vector_avoidance_rotation_emergency < 0.001f)
			actor->control.vector_avoidance_rotation_emergency = 0.f;

		if (actor->control.moving)
		{
			real_vector3d rotation =
				actor->control.vector_avoidance_rotation;
			real angle_squared = magnitude_squared3d(&rotation);

			if (angle_squared > _real_epsilon)
			{
				real angle = square_root(angle_squared);
				real inverse_angle = 1.f / angle;

				rotation.i *= inverse_angle;
				rotation.j *= inverse_angle;
				rotation.k *= inverse_angle;
				rotate_vector_about_axis(
					&actor->control.moving_towards_vector,
					&rotation,
					sine(angle),
					cosine(angle));
			}

			rotation_emergency_amount =
				actor->control.vector_avoidance_rotation_emergency;
		}
	}

	movement_type = actor->orders.move.override_movement_type;
	if (movement_type == NONE)
	{
		movement_type = _actor_movement_type_combat;
		if (actor->orders.move.flaming)
		{
			movement_type = _actor_movement_type_flaming;
		}
		else if (actor->orders.move.panicked)
		{
			movement_type = _actor_movement_type_panic;
		}
		else
		{
			switch (actor->state.mode)
			{
			case _actor_mode_asleep:
				movement_type = _actor_movement_type_asleep;
				break;

			case _actor_mode_alert:
				movement_type = _actor_movement_type_alert;
				break;

			case _actor_mode_combat:
				movement_type = _actor_movement_type_combat;
				break;
			}
		}
	}
	actor->output.movement_type = movement_type;

	override_facing = actor->orders.move.override_movement_facing;
	if (actor->control.path.path.valid &&
		actor->control.path.destination_original_distance >=
			definition->moving.stationary_moving_distance)
	{
		crouch = actor->orders.move.moving_crouch;
	}
	else
	{
		crouch = actor->orders.move.stationary_crouch;
	}

	if (actor->input.vehicle_driver_type > _actor_vehicle_driver_none)
	{
		struct vehicle_datum *vehicle =
			vehicle_datum_get(actor->input.vehicle_index);
		struct vehicle_definition *vehicle_definition =
			vehicle_specific_definition_get(vehicle->definition_index);

		steering_maximum_angle =
			vehicle_definition->ai_steering_max_angle;
		oversteer_minimum_angle =
			vehicle_definition->ai_oversteer_angle_lower_bound;
		oversteer_maximum_angle =
			vehicle_definition->ai_oversteer_angle_upper_bound;
		if (vehicle_definition->ai_steering_max_throttle > 0.f)
		{
			maximum_throttle =
				vehicle_definition->ai_steering_max_throttle;
		}

		switch (actor->input.vehicle_driver_type)
		{
		case _actor_vehicle_driver_hovering_ground:
			if (vehicle->vehicle.airborne_ticks)
			{
				allow_jump = TRUE;
				actor->control.moving = FALSE;
				actor->control.free_facing_vector = TRUE;
				crouch = FALSE;
				break;
			}

			if (vehicle->vehicle.hover < 0.7f &&
				vehicle->object.up.k < 0.8f)
			{
				real_vector3d escape_direction;

				allow_jump = TRUE;
				set_real_vector3d(
					&escape_direction,
					vehicle->object.up.i,
					vehicle->object.up.j,
					0.f);
				if (normalize3d(&escape_direction) > 0.f)
				{
					actor->control.moving = TRUE;
					scale_vector3d_inline(
						&escape_direction,
						3.f,
						&actor->control.moving_towards_vector);
				}
				else
				{
					actor->control.moving = FALSE;
				}
				crouch = FALSE;
				break;
			}

			/* fall through */

		case _actor_vehicle_driver_nondirectional_ground:
			free_movement = TRUE;
			free_movement_distance_squared =
				vehicle_definition->ai_sideslip_distance *
				vehicle_definition->ai_sideslip_distance;
			crouch = FALSE;
			break;

		case _actor_vehicle_driver_directional_flying:
			{
				real_vector3d stuck_direction;

				if (vehicle_stuck(
					actor->input.vehicle_index,
					&stuck_direction))
				{
					actor->control.moving = FALSE;
					actor->control.free_facing_vector = FALSE;
					actor->control.optional_facing_vector = FALSE;
					actor->control.desired_facing_vector.i =
						-stuck_direction.i;
					actor->control.desired_facing_vector.j =
						-stuck_direction.j;
					actor->control.desired_facing_vector.k =
						-stuck_direction.k;
				}
				else
				{
					override_facing = 0;
					move_in_3d = TRUE;
					allow_all_moving_turns = TRUE;
				}
				crouch = FALSE;
			}
			break;

		default:
			actor->control.moving = FALSE;
			actor->control.moving_facing_direction = 0;
			actor->control.free_facing_vector =
				actor->meta.type == _actor_mounted_weapon ||
				actor->input.vehicle_gunner;
			crouch = FALSE;
			break;
		}
	}
	else if (actor->input.vehicle_passenger)
	{
		actor->control.moving = FALSE;
		actor->control.moving_facing_direction = 0;
		actor->control.free_facing_vector =
			actor->meta.type == _actor_mounted_weapon ||
			actor->input.vehicle_gunner;
		actor->control.optional_facing_vector = FALSE;
		crouch = FALSE;
	}
	else if (actor->orders.move.animation.impulse != NONE)
	{
		actor->control.moving = FALSE;
		actor->control.free_facing_vector = FALSE;
		actor->control.optional_facing_vector = FALSE;
		crouch = FALSE;
	}
	else if (actor->output.movement_type == _actor_movement_type_asleep)
	{
		actor->control.moving = FALSE;
		actor->control.free_facing_vector = FALSE;
		actor->control.optional_facing_vector = FALSE;
		force_stationary_facing = TRUE;
		crouch = FALSE;
	}
	else if (actor->input.in_midair && !actor->state.flying)
	{
		actor->control.moving = FALSE;
		actor->control.free_facing_vector = TRUE;
		crouch = FALSE;
	}
	else if (actor->control.grenade_trying_to_throw)
	{
		real_vector3d grenade_direction;

		actor->control.moving = FALSE;
		crouch = FALSE;
		vector_from_points3d(
			&actor->input.position.body_position,
			&actor->control.grenade_current_target,
			&grenade_direction);
		if (normalize3d(&grenade_direction) > 0.f)
		{
			actor->control.desired_facing_vector = grenade_direction;
			actor->control.free_facing_vector = FALSE;
			actor->control.optional_facing_vector = FALSE;
			actor->control.face_exactly = TRUE;
		}
		else
		{
			actor->control.free_facing_vector = TRUE;
		}
	}
	else if (actor->emotions.moving_into_fire_timer <= 0)
	{
		clear_firing_positions = TRUE;
		if (actor->output.movement_type != _actor_movement_type_combat ||
			TEST_FLAG(
				definition->flags,
				crouch
					? _actor_definition_crouching_must_move_forward_bit
					: _actor_definition_standing_must_move_forward_bit))
		{
			actor->control.moving_forced_by_aiming = FALSE;
		}

		if (actor->output.movement_type == _actor_movement_type_flaming)
			force_stationary_facing = TRUE;

		if (TEST_FLAG(definition->flags, _actor_definition_flying_bit))
		{
			free_movement = TRUE;
			free_movement_distance_squared =
				definition->moving.free_flying_sidestep_distance *
				definition->moving.free_flying_sidestep_distance;
			move_in_3d = TRUE;
			if (actor->control.moving_forced_by_aiming)
				free_movement_distance_squared *= 4.f;
		}
	}
	else
	{
		actor->control.moving = FALSE;
		actor->control.free_facing_vector = TRUE;
		crouch = TEST_FLAG(
			definition->flags,
			_actor_definition_crouch_in_line_of_fire_bit);
	}

	if (actor->control.moving && !actor->control.movement_complete)
	{
		actor_move_calculate_movement(
			actor_index,
			move_in_3d,
			override_facing,
			free_movement,
			free_movement_distance_squared,
			allow_all_moving_turns,
			steering_maximum_angle,
			oversteer_minimum_angle,
			oversteer_maximum_angle,
			rotation_emergency_amount,
			maximum_throttle,
			&actor->control.moving_towards_vector,
			&actor->control.desired_facing_vector,
			&actor->control.moving_facing_direction,
			&actor->output.throttle,
			&actor->control.movement_thwarted,
			&actor->control.movement_complete);
		if (actor->control.movement_complete)
			actor->control.moving = FALSE;
	}

	if (actor->control.moving)
	{
		actor->control.optional_facing_vector = FALSE;
		actor->control.free_facing_vector = FALSE;
	}
	else if (force_stationary_facing)
	{
		actor->control.desired_facing_vector = actor->input.facing_vector;
		actor->control.moving_facing_direction = 0;
		actor->control.optional_facing_vector = FALSE;
		actor->control.free_facing_vector = FALSE;
	}
	else if (actor->control.fixed_stationary_facing)
	{
		actor->control.desired_facing_vector =
			actor->control.fixed_stationary_facing_vector;
		actor->control.moving_facing_direction = 0;
		actor->control.optional_facing_vector = TRUE;
		actor->control.free_facing_vector = FALSE;
	}

	match_assert_valid_real_normal3d(
		"c:\\halo\\SOURCE\\ai\\actor_moving.c",
		655,
		&actor->control.desired_facing_vector);

	if (clear_firing_positions && !actor->control.moving)
		actor_clear_discarded_firing_positions(actor_index, TRUE);

	if (actor->control.moving &&
		TEST_FLAG(
			definition->flags,
			_actor_definition_cannot_move_crouching_bit))
	{
		crouch = FALSE;
	}

	if (game_connection() == _game_connection_local && ai_debug.force_crouch)
		crouch = TRUE;

	actor->control.desire_stationary_facing = FALSE;
	if (crouch &&
		TEST_FLAG(
			definition->flags,
			_actor_definition_fixed_crouch_facing_bit))
	{
		actor->control.desire_stationary_facing = TRUE;
	}
	actor->control.crouching = crouch;
	actor_unit_control_crouch(actor_index, crouch);

	actor = actor_get(actor_index);
	if (!actor_move_animation_busy(actor_index) &&
		actor->input.vehicle_index == NONE &&
		!actor->input.in_midair &&
		actor->emotions.berserk &&
		!actor->emotions.played_berserk_sound)
	{
		long cause_unit_index = NONE;
		real_vector2d alignment;

		alignment.i = actor->input.facing_vector.i;
		alignment.j = actor->input.facing_vector.j;
		if (actor->target.target_prop_index != NONE)
		{
			struct prop_datum *prop =
				prop_get(actor->target.target_prop_index);

			cause_unit_index = prop->unit_index;
			alignment.i = prop->actor_to_prop.i;
			alignment.j = prop->actor_to_prop.j;
			if (normalize2d(&alignment) == 0.f)
			{
				alignment.i = actor->input.facing_vector.i;
				alignment.j = actor->input.facing_vector.j;
			}
		}

		actor_move_animation_impulse(actor_index, 0, &alignment);
		ai_communication_event(
			_ai_communication_berserk,
			actor->meta.unit_index,
			cause_unit_index,
			_comm_hostility_enemy,
			NONE,
			NONE,
			NULL);
		actor->emotions.played_berserk_sound = TRUE;
	}

	if (allow_jump)
	{
		actor_unit_control_jump(actor_index);
	}
	else if (actor->input.in_midair || actor->input.vehicle_index != NONE)
	{
		actor->control.jumping_targeted = FALSE;
	}
	else if (!actor_move_animation_busy(actor_index) &&
		actor->orders.move.jump)
	{
		boolean leaped = FALSE;

		if (actor->orders.move.jump_leap)
		{
			real_vector2d alignment;

			if (actor->orders.move.jump_targeted)
			{
				alignment = actor->orders.move.jump_alignment_vector;
			}
			else
			{
				alignment.i = actor->input.facing_vector.i;
				alignment.j = actor->input.facing_vector.j;
				if (normalize2d(&alignment) == 0.f)
					alignment = *global_forward2d;
			}

			leaped = unit_leap_begin(
				actor->meta.unit_index,
				&alignment);
		}

		if (leaped)
		{
			ai_communication_event(
				_ai_communication_leap,
				actor->meta.unit_index,
				NONE,
				NONE,
				NONE,
				NONE,
				NULL);
		}
		else
		{
			actor_unit_control_jump(actor_index);
		}

		if (actor->orders.move.jump_targeted)
		{
			actor->control.jump_target_horizontal_vel =
				actor->orders.move.jump_target_horizontal_vel;
			actor->control.jumping_targeted = TRUE;
			actor->control.jump_target_vertical_vel =
				actor->orders.move.jump_target_vertical_vel;
			actor->control.jump_alignment_vector =
				actor->orders.move.jump_alignment_vector;
		}
	}

	actor->output.animation = actor->orders.move.animation;

	return;
}

boolean actor_move_halt(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->input.vehicle_driver_type == _actor_vehicle_driver_directional_flying &&
		actor->control.moving)
	{
		return actor_move_to_point(
			actor_index,
			&actor->input.position.body_position,
			actor->input.pathfinding_surface_index,
			NONE);
	}

	actor->firing_positions.current_position_index = NONE;
	if (actor->control.path.destination_orders.destination_type != _destination_halt)
	{
		actor->orders.move.destination.destination_type = _destination_halt;
		actor->control.path.destination_orders = actor->orders.move.destination;
	}

	return actor_path_refresh(actor_index, TRUE, NULL);
}

boolean actor_move_halt_at_firing_position(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->input.vehicle_driver_type == _actor_vehicle_driver_directional_flying)
	{
		short firing_position_index = actor->firing_positions.current_position_index;

		if (firing_position_index == NONE)
			return actor_move_halt(actor_index);

		return actor_move_to_firing_position(
			actor_index,
			firing_position_index,
			NULL);
	}

	if (actor->control.path.destination_orders.destination_type != _destination_halt)
	{
		actor->orders.move.destination.destination_type = _destination_halt;
		actor->control.path.destination_orders = actor->orders.move.destination;
	}

	return actor_path_refresh(actor_index, TRUE, NULL);
}
