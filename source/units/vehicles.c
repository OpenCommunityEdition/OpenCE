/*
VEHICLES.C

symbols in this file:
001A4E30 0010:
	_vehicles_initialize (0000)
001A4E40 0010:
	_vehicles_initialize_for_new_map (0000)
001A4E50 0010:
	_vehicles_dispose_from_old_map (0000)
001A4E60 0010:
	_vehicles_dispose (0000)
001A4E70 0030:
	_vehicle_place (0000)
001A4EA0 0010:
	_vehicle_delete (0000)
001A4EB0 0030:
	_vehicle_causes_collision_damage (0000)
001A4EE0 0010:
	_vehicle_build_update (0000)
001A4EF0 0010:
	_vehicle_parse_update (0000)
001A4F00 0050:
	_vehicle_hover (0000)
001A4F50 0020:
	_code_001a4f50 (0000)
001A4F70 0030:
	_vehicle_is_flipped (0000)
001A4FA0 00a0:
	_code_001a4fa0 (0000)
001A5040 0020:
	_set_real_quaternion (0000)
001A5060 00b0:
	_vehicle_reset (0000)
001A5110 0070:
	_vehicle_new (0000)
001A5180 0400:
	_vehicle_preprocess_node_orientations (0000)
001A5580 0100:
	_vehicle_accelerate (0000)
001A5680 0060:
	_vehicle_render_debug (0000)
001A56E0 0130:
	_vehicle_find_pathfinding_surface_index (0000)
001A5810 00d0:
	_code_001a5810 (0000)
001A58E0 0150:
	_code_001a58e0 (0000)
001A5A30 0110:
	_code_001a5a30 (0000)
001A5B40 0310:
	_code_001a5b40 (0000)
001A5E50 0440:
	_code_001a5e50 (0000)
001A6290 0300:
	_code_001a6290 (0000)
001A6590 0180:
	_code_001a6590 (0000)
001A6710 0200:
	_code_001a6710 (0000)
001A6910 0290:
	_code_001a6910 (0000)
001A6BA0 0220:
	_code_001a6ba0 (0000)
001A6DC0 0320:
	_code_001a6dc0 (0000)
001A70E0 01d0:
	_code_001a70e0 (0000)
001A72B0 0520:
	_vehicle_export_function_values (0000)
001A77D0 0180:
	_vehicle_moving_near_any_player (0000)
001A7950 0170:
	_vehicle_stuck (0000)
001A7AC0 03a0:
	_code_001a7ac0 (0000)
001A7E60 09a0:
	_code_001a7e60 (0000)
001A8800 0070:
	_code_001a8800 (0000)
001A8870 0910:
	_vehicle_update (0000)
002A6200 000f:
	??_C@_0P@GMMIJBCG@vehicle_update?$AA@ (0000)
002A6210 0004:
	__real@3c64c389 (0000)
002A6214 0004:
	__real@bc64c389 (0000)
002A6218 0008:
	__real@3f9c987120000000 (0000)
002A6220 0004:
	__real@3e088889 (0000)
002A6224 0004:
	__real@3f866666 (0000)
002A6228 0004:
	__real@3f55c28f (0000)
002A622C 000a:
	??_C@_09IPNEIKLN@reflected?$AA@ (0000)
002A6238 000e:
	??_C@_0O@FEOFEIPO@jet?5thrusters?$AA@ (0000)
002A6248 0010:
	??_C@_0BA@JGHLOGHL@hover?5thrusters?$AA@ (0000)
002A6258 0009:
	??_C@_08FLPDFBMI@midpoint?$AA@ (0000)
002A6264 0004:
	__real@4235d174 (0000)
002A6268 0004:
	__real@3fd55556 (0000)
002A626C 0004:
	__real@4091745d (0000)
002A6270 0004:
	__real@3f969697 (0000)
002A6274 0004:
	__real@bd4ccccd (0000)
002A6278 0004:
	__real@3b03126f (0000)
002A627C 0004:
	__real@3b83126f (0000)
002A6280 0004:
	__real@bbbea2f1 (0000)
002A6284 0004:
	__real@3b7e2e97 (0000)
002A6288 0004:
	__real@3acb5879 (0000)
002A628C 0004:
	__real@3b64c389 (0000)
002A6290 0004:
	__real@bb64c389 (0000)
002A6298 0008:
	__real@3f7c987120000000 (0000)
002A62A0 0006:
	??_C@_05KCCEOPLE@?$HOblur?$AA@ (0000)
002A62A8 0020:
	??_C@_0CA@NBAFPFDO@c?3?2halo?2SOURCE?2units?2vehicles?4c?$AA@ (0000)
002A62C8 0008:
	__real@3fd3333333333333 (0000)
002A62D0 0008:
	__real@bfd3333333333333 (0000)
00316228 05f8:
	_data_00316228 (0000)
*/

/* ---------- headers */

#define set_real_quaternion vehicles_set_real_quaternion_inline
#include "cseries/cseries.h"
#include "units/vehicles.h"
#include "units/vehicle_definitions.h"
#include "scenario/scenario_definitions.h"
#undef set_real_quaternion

/* ---------- constants */

enum
{
	_vehicle_hovering_bit = 1,
};

/* ---------- macros */

/* ---------- structures */

struct scenario_object_permutation
{
	unsigned long change_colors[4];
	byte region_permutations[8];
	unsigned long unused[2];
};

struct scenario_unit_datum
{
	real body_vitality;
	unsigned long flags;
	unsigned long unused[2];
};

struct scenario_vehicle_datum
{
	struct scenario_object_datum object;
	struct scenario_object_permutation permutation;
	struct scenario_unit_datum unit;
	byte multiplayer_team_index;
	byte unused_byte;
	word multiplayer_spawn_flags;
	long unused[7];
};

struct vehicle_definition
{
	struct unit_definition unit;
	unsigned long flags;
};

struct _vehicle_runtime_data
{
	word flags;
	short stop_time;
	byte airborne_ticks;
	byte upending_type;
	byte upending_ticks;
	byte on_ground_ticks;
	real speed;
	real slide;
	real turn;
	real wheel;
	real left_tread;
	real right_tread;
	real hover;
	real thrust;
	byte suspension[8];
	real_point3d hover_position;
};

struct vehicle_runtime_datum
{
	struct unit_datum unit;
	struct _vehicle_runtime_data vehicle;
};

typedef char scenario_object_permutation_size_assert[
	sizeof(struct scenario_object_permutation) == 0x20 ? 1 : -1];
typedef char scenario_unit_datum_size_assert[
	sizeof(struct scenario_unit_datum) == 0x10 ? 1 : -1];
typedef char scenario_vehicle_datum_size_assert[
	sizeof(struct scenario_vehicle_datum) == 0x78 ? 1 : -1];
typedef char scenario_vehicle_permutation_offset_assert[
	offsetof(struct scenario_vehicle_datum, permutation) == 0x28 ? 1 : -1];
typedef char scenario_vehicle_unit_offset_assert[
	offsetof(struct scenario_vehicle_datum, unit) == 0x48 ? 1 : -1];
typedef char vehicle_definition_flags_offset_assert[
	offsetof(struct vehicle_definition, flags) == 0x2F0 ? 1 : -1];
typedef char vehicle_unit_datum_size_assert[
	sizeof(struct unit_datum) == 0x424 ? 1 : -1];
typedef char vehicle_runtime_flags_offset_assert[
	offsetof(struct vehicle_runtime_datum, vehicle.flags) == 0x424 ? 1 : -1];
typedef char vehicle_runtime_hover_position_offset_assert[
	offsetof(struct vehicle_runtime_datum, vehicle.hover_position) == 0x454 ? 1 : -1];

/* ---------- prototypes */

void unit_place(
	long unit_index,
	struct scenario_unit_datum *scenario_unit);

/* ---------- globals */

/* ---------- public code */

void vehicles_initialize(
	void)
{
	return;
}

void vehicles_initialize_for_new_map(
	void)
{
	return;
}

void vehicles_dispose_from_old_map(
	void)
{
	return;
}

void vehicles_dispose(
	void)
{
	return;
}

void vehicle_delete(
	long vehicle_index)
{
	return;
}

/* ---------- private code */

void vehicle_place(
	long vehicle_index,
	struct scenario_vehicle_datum *scenario_vehicle)
{
	unit_place(
		vehicle_index,
		&scenario_vehicle->unit);
	object_add_scenario_permutation(
		vehicle_index,
		&scenario_vehicle->permutation);

	return;
}

boolean vehicle_causes_collision_damage(
	long vehicle_index)
{
	struct unit_datum *vehicle;
	struct vehicle_definition *definition;

	vehicle = vehicle_get(vehicle_index);
	definition = vehicle_specific_definition_get(vehicle->definition_index);

	return TEST_FLAG(
		definition->flags,
		_vehicle_causes_collision_damage_bit);
}

boolean vehicle_is_flipped(
	long vehicle_index)
{
	struct unit_datum *vehicle;

	vehicle = vehicle_get(vehicle_index);

	return vehicle->object.up.k < 0.2f;
}

void vehicle_hover(
	long vehicle_index,
	boolean hover)
{
	if (vehicle_index != NONE)
	{
		struct vehicle_runtime_datum *vehicle;

		vehicle = vehicle_runtime_get(vehicle_index);
		if (hover)
		{
			object_get_origin(
				vehicle_index,
				&vehicle->vehicle.hover_position);
			SET_FLAG(
				vehicle->vehicle.flags,
				_vehicle_hovering_bit,
				TRUE);
		}
		else
		{
			SET_FLAG(
				vehicle->vehicle.flags,
				_vehicle_hovering_bit,
				FALSE);
		}
	}

	return;
}

boolean vehicle_build_update(
	void)
{
	return TRUE;
}

boolean vehicle_parse_update(
	void)
{
	return TRUE;
}

real_quaternion *set_real_quaternion(
	real_quaternion *quaternion,
	real i,
	real j,
	real k,
	real w)
{
	quaternion->v.i = i;
	quaternion->v.j = j;
	quaternion->v.k = k;
	quaternion->w = w;

	return quaternion;
}
