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
#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "ai/actions.h"
#include "ai/actor_definitions.h"
#include "ai/actors.h"
#include "ai/props.h"
#include "units/units.h"
#include "units/vehicle_definitions.h"
#include "units/vehicles.h"
#undef arccosine
#undef normalize3d

/* ---------- constants */

enum
{
	NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS = 8,
};

/* ---------- macros */

#define _full_circle (2.f*_pi)

/* ---------- structures */

/* The shared vehicle tag layout remains opaque in the public header. Actor
 * movement reads only these two January-authenticated AI pathing fields, so
 * keep that layout fragment local to this translation unit. */
struct vehicle_definition
{
	byte __unknown0[0x388];
	real ai_avoidance_distance;
	real ai_pathfinding_radius;
};

typedef char actor_moving_vehicle_avoidance_distance_offset_assert[
	offsetof(struct vehicle_definition, ai_avoidance_distance) == 0x388 ? 1 : -1];
typedef char actor_moving_vehicle_pathfinding_radius_offset_assert[
	offsetof(struct vehicle_definition, ai_pathfinding_radius) == 0x38C ? 1 : -1];

/* ---------- prototypes */

/* The owner declaration in actions.h is macro-renamed while importing the
 * January inline set; restore the external name after that schedule ends. */
real normalize3d(
	real_vector3d *vector);

/* ---------- globals */

real const avoidance_ray_angles[NUMBER_OF_VECTOR_AVOIDANCE_DIRECTIONS] =
{
	0.f,
	_pi/4,
	_pi/2,
	3*_pi/4,
	_pi,
	3.9269909f,
	_pi + _pi/2,
	5.4977875f,
};

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

		result = actor_path_refresh(actor_index, TRUE, FALSE);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, FALSE);
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

		result = actor_path_refresh(actor_index, TRUE, FALSE);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, FALSE);
	}

	return result;
}

boolean actor_move_to_firing_position(
	long actor_index,
	short firing_position_index,
	boolean temporary)
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

		result = actor_path_refresh(actor_index, TRUE, temporary);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, temporary);
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

		result = actor_path_refresh(actor_index, TRUE, FALSE);
	}
	else if (actor->meta.timeslice && !actor->control.path.refreshed_this_tick)
	{
		result = actor_path_refresh(actor_index, FALSE, FALSE);
	}

	return result;
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

	return actor_path_refresh(actor_index, TRUE, FALSE);
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
			FALSE);
	}

	if (actor->control.path.destination_orders.destination_type != _destination_halt)
	{
		actor->orders.move.destination.destination_type = _destination_halt;
		actor->control.path.destination_orders = actor->orders.move.destination;
	}

	return actor_path_refresh(actor_index, TRUE, FALSE);
}
