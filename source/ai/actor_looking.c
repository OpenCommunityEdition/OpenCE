/*
ACTOR_LOOKING.C

symbols in this file:
00015E80 0070:
	_actor_look_secondary_stop (0000)
00015EF0 0130:
	_actor_look_compute_prop_interest (0000)
00016020 0050:
	_actor_look_get_looking_definition (0000)
00016070 0370:
	_actor_look_secondary (0000)
000163E0 0080:
	_actor_look_valid_aim_vector (0000)
00016460 00f0:
	_actor_look_valid_look_vector (0000)
00016550 00b0:
	_actor_looking_test_validity (0000)
00016600 0260:
	_actor_look_idle_find_prop (0000)
00016860 0160:
	_actor_look_idle_timer (0000)
000169C0 0260:
	_actor_look_find_random_vector (0000)
00016C20 0050:
	_valid_real_normal2d (0000)
00016C70 0660:
	_actor_look_decode_direction (0000)
000172D0 0210:
	_actor_look_idle_new_major_direction (0000)
000174E0 0170:
	_actor_look_idle_new_minor_direction (0000)
00017650 1270:
	_actor_look_update (0000)
000188C0 0080:
	_actor_look_affect_movement (0000)
00244C74 0070:
	_global_secondary_look_priorities (0000)
	_global_secondary_look_times (0038)
00244CE4 000e:
	??_C@_0O@FNONJPCB@?$CFs?3?5look?9stop?$AA@ (0000)
00244CF4 0004:
	__real@3fe66666 (0000)
00244CF8 0012:
	??_C@_0BC@CBOPKOHG@?$CFs?3?5look?5?$CFs?5?$CFs?5?$CFd?$AA@ (0000)
00244D0C 0009:
	??_C@_08IILCFMOF@scripted?$AA@ (0000)
00244D18 0007:
	??_C@_06NCEIAKB@danger?$AA@ (0000)
00244D20 0007:
	??_C@_06GJOIPFFF@damage?$AA@ (0000)
00244D28 000c:
	??_C@_0M@KFECPMLC@combat?9prop?$AA@ (0000)
00244D34 000f:
	??_C@_0P@HANCNCFH@comm?9direction?$AA@ (0000)
00244D44 000a:
	??_C@_09OLJCJFBL@comm?9prop?$AA@ (0000)
00244D50 000e:
	??_C@_0O@BOCADBDG@shooting?9prop?$AA@ (0000)
00244D60 0009:
	??_C@_08NDPBJKMG@new?9prop?$AA@ (0000)
00244D6C 0007:
	??_C@_06LDFNIMBB@impact?$AA@ (0000)
00244D74 000c:
	??_C@_0M@JOHOFPJB@moving?9prop?$AA@ (0000)
00244D80 000c:
	??_C@_0M@PINLHHEI@environment?$AA@ (0000)
00244D8C 0010:
	??_C@_0BA@MGHLNDAM@override?9facing?$AA@ (0000)
00244D9C 0009:
	??_C@_08PNBPAFID@override?$AA@ (0000)
00244DA8 000d:
	??_C@_0N@OPBBNFGJ@stop?9and?9aim?$AA@ (0000)
00244DB8 000d:
	??_C@_0N@FFMCKHMJ@turn?9and?9aim?$AA@ (0000)
00244DC8 0004:
	??_C@_03MKHEFFMA@aim?$AA@ (0000)
00244DCC 0009:
	??_C@_08IECGFEAB@idle?9aim?$AA@ (0000)
00244DD8 000a:
	??_C@_09CPPFOLKN@idle?9look?$AA@ (0000)
00244DE4 0008:
	??_C@_07DLHCIBDH@default?$AA@ (0000)
00244DEC 0037:
	??_C@_0DH@BKMNBGNP@?$CItype?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CItype?5?$DM?5NUMBER_OF@ (0000)
00244E24 0022:
	??_C@_0CC@OBFKKCBI@c?3?2halo?2SOURCE?2ai?2actor_looking?4@ (0000)
00244E48 0004:
	__real@3ada740e (0000)
00244E50 0049:
	??_C@_0EJ@PDHFHBNF@?$CFs?3?5logic?5error?5in?5actor?5looking@ (0000)
00244E9C 0008:
	??_C@_07FJPMPBHO@looking?$AA@ (0000)
00244EA4 0007:
	??_C@_06CPNACJHN@aiming?$AA@ (0000)
00244EAC 0007:
	??_C@_06LJBECLPP@facing?$AA@ (0000)
00244EB4 000e:
	??_C@_0O@LDGGHEKC@result_vector?$AA@ (0000)
00244EC4 0023:
	??_C@_0CD@JLCDLMPD@denormalized?5?$CFf?3?5?$DMerror?$DO?5?$CItype?5?$CF@ (0000)
00244EE8 0021:
	??_C@_0CB@ILCNAOOH@denormalized?5?$CFf?3?5object?5?$CI0x?$CF08X?$CJ@ (0000)
00244F0C 001f:
	??_C@_0BP@JMCBPAFA@denormalized?5?$CFf?3?5danger?5?$CInone?$CJ?$AA@ (0000)
00244F2C 0023:
	??_C@_0CD@BJDFLJJM@denormalized?5?$CFf?3?5danger?5?$CI?$CFf?5?$CFf?5?$CF@ (0000)
00244F50 0023:
	??_C@_0CD@MPCIMEDM@denormalized?5?$CFf?3?5vector?5?$CI?$CFf?5?$CFf?5?$CF@ (0000)
00244F74 0022:
	??_C@_0CC@KKBEICMF@denormalized?5?$CFf?3?5point?5?$CI?$CFf?5?$CFf?5?$CFf@ (0000)
00244F98 003a:
	??_C@_0DK@KHHBJFIN@denormalized?5?$CFf?3?5prop?50x?$CF08X?$CFs?5?$CI@ (0000)
00244FD4 000b:
	??_C@_0L@CGKLOAKK@?5?$CIinvalid?$CJ?$AA@ (0000)
00244FE0 001f:
	??_C@_0BP@IAJLBFML@denormalized?5?$CFf?3?5target?5?$CInone?$CJ?$AA@ (0000)
00245000 0036:
	??_C@_0DG@LJDBLOA@denormalized?5?$CFf?3?5target?5?$CIprop?50x@ (0000)
00245038 0042:
	??_C@_0EC@ONEINDFA@denormalized?5?$CFf?3?5target?5type?5?$CFd?5@ (0000)
0024507C 003c:
	??_C@_0DM@KFLCBPJA@denormalized?5?$CFf?3?5target?5type?5?$CFd?5@ (0000)
002450B8 0032:
	??_C@_0DC@MBKOGPLI@denormalized?5?$CFf?3?5?$CFsmoving?5?$CIp?$CFf?5?$CF@ (0000)
002450EC 0005:
	??_C@_04LKOGMCLF@not?5?$AA@ (0000)
002450F4 000a:
	??_C@_09HPKLLIJF@direction?$AA@ (0000)
00245100 000e:
	??_C@_0O@DNCJFFDC@specification?$AA@ (0000)
00245110 001e:
	??_C@_0BO@ICLEICLJ@?$CGactor?9?$DOoutput?4looking_vector?$AA@ (0000)
00245130 001d:
	??_C@_0BN@MLJELDAF@?$CGactor?9?$DOoutput?4aiming_vector?$AA@ (0000)
00245150 001d:
	??_C@_0BN@BNGLBJLG@?$CGactor?9?$DOoutput?4facing_vector?$AA@ (0000)
00245170 0004:
	__real@3f666666 (0000)
00245174 0027:
	??_C@_0CH@MHPDGGDP@?$CFs?3?5assert_valid_real_normal2d?$CI?$CF@ (0000)
0024519C 002f:
	??_C@_0CP@OAIIPIIE@?$CIreal_vector2d?5?$CK?$CJ?5?$CGactor?9?$DOoutput@ (0000)
002451CC 0013:
	??_C@_0BD@CADKODCD@?$CGidle_minor_vector?$AA@ (0000)
002451E0 0024:
	??_C@_0CE@LCIPMIDM@actor?9?$DOcontrol?4idle_minor_timer?5@ (0000)
00245204 0013:
	??_C@_0BD@JGOPOGFP@?$CGidle_major_vector?$AA@ (0000)
00245218 004b:
	??_C@_0EL@IKADNPKK@?$CBactor?9?$DOcontrol?4idle_major_activ@ (0000)
00245264 0016:
	??_C@_0BG@GOAPBADG@?$CFs?3?5look?5timer?5expire?$AA@ (0000)
0024527C 0012:
	??_C@_0BC@OJIPDKMD@?$CGsecondary_vector?$AA@ (0000)
00245290 0010:
	??_C@_0BA@GFAAOIFJ@?$CGprimary_vector?$AA@ (0000)
002452A0 0027:
	??_C@_0CH@JLLNHPPI@?$CGactor?9?$DOcontrol?4desired_looking_@ (0000)
002452C8 0026:
	??_C@_0CG@KOFPIDJD@?$CGactor?9?$DOcontrol?4desired_aiming_v@ (0000)
002452F0 0026:
	??_C@_0CG@HIKACJCA@?$CGactor?9?$DOcontrol?4desired_facing_v@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "ai/actor_looking.h"

#include "actors.h"
#include "actor_definitions.h"
#include "ai_debug.h"
#include "ai_profile.h"
#include "cseries/errors.h"
#include "items/weapon_definitions.h"
#include "main/console.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "props.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_idle_look_none = 0,
	_idle_look_noncombat,
	_idle_look_guarding,
	_idle_look_searching,
	_idle_look_combat,
	NUMBER_OF_IDLE_LOOK_TYPES,
};

enum
{
	_idle_timer_facing = 0,
	_idle_timer_aiming,
	_idle_timer_looking,
};

/* ---------- macros */

#define ACTOR_LOOKING_DEBUG_PRINTING_ENABLED() (ai_debug.print_secondary_looking)

/* ---------- structures */

typedef char actor_looking_ai_debug_printing_offset_must_be_0x9D[
	offsetof(struct ai_debug_state, print_secondary_looking) == 0x9D ? 1 : -1];
typedef char actor_looking_actor_vehicle_index_offset_must_be_0x158[
	offsetof(struct actor_datum, input.vehicle_index) == 0x158 ? 1 : -1];
typedef char actor_looking_secondary_look_type_offset_must_be_0x544[
	offsetof(struct actor_datum, control.secondary_look_type) == 0x544 ? 1 : -1];
typedef char actor_looking_secondary_look_priority_offset_must_be_0x546[
	offsetof(struct actor_datum, control.secondary_look_priority) == 0x546 ? 1 : -1];
typedef char actor_looking_secondary_look_timer_offset_must_be_0x548[
	offsetof(struct actor_datum, control.secondary_look_timer) == 0x548 ? 1 : -1];
typedef char actor_looking_prop_unit_index_offset_must_be_0x18[
	offsetof(struct prop_datum, unit_index) == 0x18 ? 1 : -1];
typedef char actor_looking_prop_state_offset_must_be_0x24[
	offsetof(struct prop_datum, state) == 0x24 ? 1 : -1];
typedef char actor_looking_prop_enemy_offset_must_be_0x60[
	offsetof(struct prop_datum, enemy) == 0x60 ? 1 : -1];
typedef char actor_looking_prop_dead_ticks_offset_must_be_0x76[
	offsetof(struct prop_datum, dead_ticks) == 0x76 ? 1 : -1];
typedef char actor_looking_prop_vehicle_index_offset_must_be_0x110[
	offsetof(struct prop_datum, vehicle_index) == 0x110 ? 1 : -1];
typedef char actor_looking_prop_quantized_distance_offset_must_be_0x121[
	offsetof(struct prop_datum, quantized_distance) == 0x121 ? 1 : -1];
typedef char actor_looking_prop_quantized_speed_offset_must_be_0x123[
	offsetof(struct prop_datum, quantized_speed) == 0x123 ? 1 : -1];
typedef char actor_looking_prop_dead_offset_must_be_0x127[
	offsetof(struct prop_datum, dead) == 0x127 ? 1 : -1];
typedef char actor_looking_prop_shooting_offset_must_be_0x12F[
	offsetof(struct prop_datum, shooting) == 0x12F ? 1 : -1];

/* ---------- prototypes */

static boolean actor_look_decode_direction(
	long actor_index,
	struct direction_specification *specification,
	real_vector3d *direction);
static struct actor_idle_looking *actor_look_get_looking_definition(
	long actor_index);
static boolean actor_look_valid_aim_vector(
	real yaw_deviation,
	real_vector3d const *facing_vector,
	real_vector3d const *attempted_aiming_vector);
static boolean actor_look_valid_look_vector(
	real yaw_deviation,
	real_vector2d const *cone_limits,
	real_vector3d const *aiming_vector,
	real_vector3d const *attempted_looking_vector,
	real_vector3d const *look_vector);
static long actor_look_idle_timer(
	long actor_index,
	struct actor_idle_looking *looking_definition,
	short timer_type,
	boolean interesting_direction);
static boolean actor_look_find_random_vector(
	real_point3d const *center_point,
	real_vector3d const *base_vector,
	boolean avoid_obstructions,
	real yaw_minimum,
	real yaw_maximum,
	real pitch_minimum,
	real pitch_maximum,
	real_vector3d *result_vector);

/* ---------- globals */

/* ---------- public code */

void actor_look_secondary_stop(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (actor->control.secondary_look_type > 0 &&
		ACTOR_LOOKING_DEBUG_PRINTING_ENABLED())
	{
		console_printf(
			FALSE,
			"%s: look-stop",
			ai_debug_describe_actor(
				actor_index,
				NONE,
				FALSE,
				temporary,
				NUMBEROF(temporary)));
	}

	actor->control.secondary_look_priority = _secondary_look_priority_none;
	actor->control.secondary_look_type = _secondary_look_none;
	actor->control.secondary_look_timer = 0;

	return;
}

real actor_look_compute_prop_interest(
	long actor_index,
	long prop_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct prop_datum *prop = prop_get(prop_index);
	real interest = 0.0f;
	real weight;
	short state = prop->state;

	if (state >= _prop_state_becoming_unacknowledged &&
		state <= _prop_state_acknowledged)
	{
		if (prop->dead)
		{
			interest = prop->dead_ticks < 210 ? 1.8f : 0.4f;
		}
		else if (prop->enemy)
		{
			interest = 2.0f;
		}
		else
		{
			interest = 1.0f;
		}
	}
	else if (prop->enemy &&
		state >= _prop_state_uninspected_orphan &&
		state <= _prop_state_inspected_orphan)
	{
		interest = 1.5f;
	}

	if (prop->unit_index == actor->input.vehicle_index ||
		prop->vehicle_index == actor->input.vehicle_index)
	{
		interest = 0.0f;
	}

	weight = prop->vehicle_index != NONE ? 1.5f : 1.0f;

	switch (prop->quantized_speed)
	{
	case 1:
		interest += weight * 0.5f;
		break;
	case 2:
		interest += weight;
		break;
	case 3:
		interest += weight * 2.0f;
		break;
	}

	if (prop->shooting)
	{
		interest += weight * 2.0f;
	}

	switch (prop->quantized_distance)
	{
	case 1:
		interest *= 0.6f;
		break;
	case 3:
		interest *= 0.4f;
		break;
	case 4:
		interest *= 0.2f;
		break;
	}

	return interest;
}

boolean valid_real_normal2d(
	real_vector2d const *normal)
{
	return valid_realcmp(magnitude_squared2d(normal), 1.0f);
}

void actor_look_affect_movement(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct direction_specification *movement_direction = &actor->orders.look.primary_direction;

	if (movement_direction->type == _direction_specification_movement &&
		!actor_path_has_path(actor_index))
	{
		actor->orders.look.primary_priority = _primary_priority_none;
	}

	actor->control.moving_forced_by_aiming =
		actor->orders.look.primary_priority >= _primary_priority_aiming &&
		movement_direction->type != _direction_specification_movement &&
		actor_look_decode_direction(
			actor_index,
			movement_direction,
			&actor->control.moving_forced_aim_direction);

	return;
}

/* ---------- private code */

static struct actor_idle_looking *actor_look_get_looking_definition(
	long actor_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct actor_definition *definition = actor_definition_get(actor->meta.definition_index);
	long idle_look_type = actor->orders.look.idle_look_type;

	if (idle_look_type != _idle_look_guarding)
	{
		if (idle_look_type <= _idle_look_guarding || idle_look_type > _idle_look_combat)
			return &definition->looking.idle_look_noncombat;

		return &definition->looking.idle_look_search_combat;
	}

	return &definition->looking.idle_look_guard;
}

static boolean actor_look_valid_aim_vector(
	real yaw_deviation,
	real_vector3d const *facing_vector,
	real_vector3d const *attempted_aiming_vector)
{
	real_vector2d aiming_vector2d;
	real_vector2d facing_vector2d;
	boolean result = FALSE;

	aiming_vector2d.i = attempted_aiming_vector->i;
	aiming_vector2d.j = attempted_aiming_vector->j;
	facing_vector2d.i = facing_vector->i;
	facing_vector2d.j = facing_vector->j;
	if (normalize2d(&aiming_vector2d) > 0.0f &&
		dot_product2d(&aiming_vector2d, &facing_vector2d) > yaw_deviation)
	{
		result = TRUE;
	}

	return result;
}

static boolean actor_look_valid_look_vector(
	real yaw_deviation,
	real_vector2d const *cone_limits,
	real_vector3d const *aiming_vector,
	real_vector3d const *attempted_looking_vector,
	real_vector3d const *look_vector)
{
	real_vector2d aiming_vector2d;
	real_vector2d attempted_looking_vector2d;
	real_vector2d looking_vector2d;
	boolean result = FALSE;

	aiming_vector2d.i = aiming_vector->i;
	aiming_vector2d.j = aiming_vector->j;
	attempted_looking_vector2d.i = attempted_looking_vector->i;
	attempted_looking_vector2d.j = attempted_looking_vector->j;
	looking_vector2d.i = look_vector->i;
	looking_vector2d.j = look_vector->j;

	if (normalize2d(&looking_vector2d) > 0.0f &&
		dot_product2d(&looking_vector2d, &aiming_vector2d) > yaw_deviation &&
		normalize2d(&attempted_looking_vector2d) > 0.0f)
	{
		short side = cross_product2d(&looking_vector2d, &attempted_looking_vector2d) > 0.0f;

		if (dot_product2d(&attempted_looking_vector2d, &looking_vector2d) > cone_limits->n[side])
			result = TRUE;
	}

	return result;
}

static long actor_look_idle_timer(
	long actor_index,
	struct actor_idle_looking *looking_definition,
	short timer_type,
	boolean interesting_direction)
{
	struct actor_datum *actor = actor_get(actor_index);
	struct weapon_definition *weapon_definition;
	real time_lower_bound;
	real time_upper_bound;
	real time;
	long ticks;

	(void)actor_definition_get(actor->meta.definition_index);
	weapon_definition = actor_get_weapon_definition(actor_index);
	switch (timer_type)
	{
	case _idle_timer_facing:
		time_lower_bound = looking_definition->idle_facing_time_lower_bound;
		time_upper_bound = looking_definition->idle_facing_time_upper_bound;
		break;
	case _idle_timer_aiming:
		time_lower_bound = looking_definition->idle_aim_time_lower_bound;
		time_upper_bound = looking_definition->idle_aim_time_upper_bound;
		break;
	case _idle_timer_looking:
		time_lower_bound = looking_definition->idle_look_time_lower_bound;
		time_upper_bound = looking_definition->idle_look_time_upper_bound;
		break;
	default:
		match_vassert(
			"c:\\halo\\SOURCE\\ai\\actor_looking.c",
			986,
			FALSE,
			NULL);
		break;
	}

	if (time_lower_bound <= 0.0f && time_upper_bound <= 0.0f)
	{
		char const *timer_names[] =
		{
			"facing",
			"aiming",
			"looking",
		};

		error(
			_error_silent,
			"%s: logic error in actor looking: %s mode should be disabled (%.1f %.1f)",
			ai_debug_describe_actor(
				actor_index,
				NONE,
				TRUE,
				temporary,
				NUMBEROF(temporary)),
			timer_names[timer_type],
			time_lower_bound,
			time_upper_bound);
		time = 0.5f;
	}
	else
	{
		time = real_seed_random_range(
			get_global_random_seed_address(),
			time_lower_bound,
			time_upper_bound);
	}

	if (weapon_definition && weapon_definition->weapon.ai_idle_look_time_modifier > 0.0f)
		time *= weapon_definition->weapon.ai_idle_look_time_modifier;
	if (interesting_direction)
		time *= 1.5f;

	ticks = fast_ftol(time * TICKS_PER_SECOND);
	if (ticks <= 1)
		ticks = 1;

	return ticks;
}

static boolean actor_look_find_random_vector(
	real_point3d const *center_point,
	real_vector3d const *base_vector,
	boolean avoid_obstructions,
	real yaw_minimum,
	real yaw_maximum,
	real pitch_minimum,
	real pitch_maximum,
	real_vector3d *result_vector)
{
	real_vector3d pitch_axis;
	real_vector3d direction;
	short attempt_count = 0;
	boolean unobstructed;

	pitch_axis.i = -base_vector->j;
	pitch_axis.j = base_vector->i;
	pitch_axis.k = 0.0f;
	if (normalize3d(&pitch_axis) == 0.0f)
		pitch_axis = *global_left3d;

	do
	{
		real yaw = real_seed_random_range(
			get_global_random_seed_address(),
			yaw_minimum,
			yaw_maximum);
		real pitch = real_seed_random_range(
			get_global_random_seed_address(),
			pitch_minimum,
			pitch_maximum);

		direction = *base_vector;
		rotate_vector_about_axis(
			&direction,
			&pitch_axis,
			(real)sin(pitch),
			(real)cos(pitch));
		rotate_vector_about_axis(
			&direction,
			global_up3d,
			(real)sin(yaw),
			(real)cos(yaw));

		unobstructed = TRUE;
		if (avoid_obstructions)
		{
			real_vector3d collision_vector;
			struct collision_result collision;

			ai_profile.meters[_ai_meter_collisions].accumulator++;
			match_assert(
				"c:\\halo\\SOURCE\\ai\\actor_looking.c",
				1066,
				global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
			global_current_collision_users[global_current_collision_user_depth++] =
				_collision_user_ai_look;
			scale_vector3d(&direction, 3.0f, &collision_vector);
			unobstructed = !collision_test_vector(
				FLAG(_collision_test_front_facing_surfaces_bit) |
					FLAG(_collision_test_structure_bit),
				center_point,
				&collision_vector,
				NONE,
				&collision);
			match_assert(
				"c:\\halo\\SOURCE\\ai\\actor_looking.c",
				1070,
				global_current_collision_user_depth > 1);
			--global_current_collision_user_depth;
		}

		if (!unobstructed)
			attempt_count++;
	}
	while (!unobstructed && attempt_count < 10);

	if (!unobstructed)
		return FALSE;

	match_assert(
		"c:\\halo\\SOURCE\\ai\\actor_looking.c",
		1077,
		result_vector);
	normalize3d(&direction);
	*result_vector = direction;

	return TRUE;
}

static boolean actor_look_decode_direction(
	long actor_index,
	struct direction_specification *specification,
	real_vector3d *direction)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	match_assert("c:\\halo\\SOURCE\\ai\\actor_looking.c", 349, specification);
	match_assert("c:\\halo\\SOURCE\\ai\\actor_looking.c", 350, direction);

	switch (specification->type)
	{
	case _direction_specification_movement:
		if (actor->control.moving)
		{
			*direction = actor->control.moving_towards_vector;
			result = normalize3d(direction) > 0.0f;
		}
		break;

	case _direction_specification_prop:
		{
			struct prop_datum *prop = prop_try_and_get(specification->prop_index);

			if (prop)
			{
				vector_from_points3d(&actor->input.position.head_position, &prop->head_position, direction);
				result = normalize3d(direction) > 0.0f;
			}
		}
		break;

	case _direction_specification_target:
		if (actor->control.fire_state == _actor_fire_state_bursting)
		{
			*direction = actor->control.burst_aim_vector;
			result = TRUE;
		}
		else if (actor->control.aiming_at_fire_target)
		{
			*direction = actor->control.current_fire_target_aim_vector;
			result = TRUE;
		}
		else if (actor->target.target_prop_index != NONE)
		{
			struct prop_datum *prop = prop_get(actor->target.target_prop_index);

			vector_from_points3d(&actor->input.position.head_position, &prop->center_of_mass, direction);
			result = normalize3d(direction) > 0.0f;
		}
		break;

	case _direction_specification_point:
		vector_from_points3d(&actor->input.position.head_position, &specification->point, direction);
		result = normalize3d(direction) > 0.0f;
		break;

	case _direction_specification_vector:
		*direction = specification->vector;
		result = TRUE;
		break;

	case _direction_specification_danger:
		if (actor->danger_zone.danger_type > _actor_danger_zone_none)
		{
			vector_from_points3d(&actor->input.position.head_position, &actor->danger_zone.position, direction);
			result = normalize3d(direction) > 0.0f;
		}
		break;

	case _direction_specification_object:
		{
			struct object_datum *object = object_try_and_get(specification->object_index);

			if (object)
			{
				real_point3d look_point;

				if (TEST_FLAG(_object_mask_unit, object->object.type))
				{
					unit_get_head_position(specification->object_index, &look_point);
				}
				else
				{
					object_get_origin(specification->object_index, &look_point);
				}

				vector_from_points3d(&actor->input.position.head_position, &look_point, direction);
				result = normalize3d(direction) > 0.0f;
			}
		}
		break;

	default:
		match_vassert("c:\\halo\\SOURCE\\ai\\actor_looking.c", 449, FALSE, NULL);
		break;
	}

	if (result)
	{
		if (!valid_real_normal3d(direction))
		{
			real magnitude = magnitude3d(direction);

			switch (specification->type)
			{
			case _direction_specification_movement:
				/* BUG (preserved for exact matching): January loads the point's
				 * z field twice (actor + 0x514). A corrected diagnostic should
				 * print the y field as its second point component.
				 */
				sprintf(temporary, "denormalized %f: %smoving (p%f %f %f) (v%f %f %f)",
					magnitude, actor->control.moving ? "" : "not ",
					actor->control.moving_towards_point.x,
					actor->control.moving_towards_point.z,
					actor->control.moving_towards_point.z,
					actor->control.moving_towards_vector.i,
					actor->control.moving_towards_vector.j,
					actor->control.moving_towards_vector.k);
				break;

			case _direction_specification_prop:
				{
					struct prop_datum *prop = prop_try_and_get(specification->prop_index);

					sprintf(temporary, "denormalized %f: prop 0x%08X%s (actor 0x%08X / us 0x%08X)",
						magnitude, specification->prop_index, prop ? "" : " (invalid)",
						prop ? prop->owner_actor_index : 0, actor_index);
				}
				break;

			case _direction_specification_target:
				if (actor->control.fire_state == _actor_fire_state_bursting)
				{
					sprintf(temporary, "denormalized %f: target type %d (burst aim vector %f %f %f)",
						magnitude, actor->control.current_fire_target_type,
						actor->control.burst_aim_vector.i,
						actor->control.burst_aim_vector.j,
						actor->control.burst_aim_vector.k);
				}
				else if (actor->control.aiming_at_fire_target)
				{
					sprintf(temporary, "denormalized %f: target type %d (fire target aim vector %f %f %f)",
						magnitude, actor->control.current_fire_target_type,
						actor->control.current_fire_target_aim_vector.i,
						actor->control.current_fire_target_aim_vector.j,
						actor->control.current_fire_target_aim_vector.k);
				}
				else if (actor->target.target_prop_index != NONE)
				{
					struct prop_datum *prop = prop_get(actor->target.target_prop_index);

					sprintf(temporary, "denormalized %f: target (prop 0x%08X center %f %f %f)",
						magnitude, actor->target.target_prop_index,
						prop->center_of_mass.x, prop->center_of_mass.y, prop->center_of_mass.z);
				}
				else
				{
					sprintf(temporary, "denormalized %f: target (none)", magnitude);
				}
				break;

			case _direction_specification_point:
				sprintf(temporary, "denormalized %f: point (%f %f %f)", magnitude,
					specification->point.x, specification->point.y, specification->point.z);
				break;

			case _direction_specification_vector:
				sprintf(temporary, "denormalized %f: vector (%f %f %f)", magnitude,
					specification->vector.i, specification->vector.j, specification->vector.k);
				break;

			case _direction_specification_danger:
				if (actor->danger_zone.danger_type > _actor_danger_zone_none)
				{
					sprintf(temporary, "denormalized %f: danger (%f %f %f)", magnitude,
						actor->danger_zone.position.x, actor->danger_zone.position.y, actor->danger_zone.position.z);
				}
				else
				{
					sprintf(temporary, "denormalized %f: danger (none)", magnitude);
				}
				break;

			case _direction_specification_object:
				sprintf(temporary, "denormalized %f: object (0x%08X)", magnitude, specification->object_index);
				break;

			default:
				sprintf(temporary, "denormalized %f: <error> (type %d)", magnitude, specification->type);
				break;
			}

			display_assert(temporary, "c:\\halo\\SOURCE\\ai\\actor_looking.c", 526, FALSE);
		}

		match_assert_valid_real_normal3d("c:\\halo\\SOURCE\\ai\\actor_looking.c", 529, direction);
	}

	return result;
}
