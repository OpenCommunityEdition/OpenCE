/*
OBSERVER.C

symbols in this file:
00078970 0040:
	_code_00078970 (0000)
000789B0 0010:
	_observer_initialize (0000)
000789C0 0130:
	_code_000789c0 (0000)
00078AF0 0050:
	_observer_initialize_for_new_map (0000)
00078B40 0010:
	_observer_dispose_from_old_map (0000)
00078B50 0090:
	_observer_get_camera (0000)
00078BE0 0080:
	_observer_command_has_finished (0000)
00078C60 0070:
	_observer_reconnect_to_structure_bsp (0000)
00078CD0 01d0:
	_code_00078cd0 (0000)
00078EA0 0200:
	_code_00078ea0 (0000)
000790A0 0050:
	_observer_obsolete_position (0000)
000790F0 0090:
	_observer_up_from_forward (0000)
00079180 0080:
	_code_00079180 (0000)
00079200 00e0:
	_code_00079200 (0000)
000792E0 0040:
	_code_000792e0 (0000)
00079320 03b0:
	_observer_set_camera (0000)
000796D0 0410:
	_code_000796d0 (0000)
00079AE0 05a0:
	_code_00079ae0 (0000)
0007A080 0620:
	_code_0007a080 (0000)
0007A6A0 0120:
	_code_0007a6a0 (0000)
0007A7C0 02f0:
	_code_0007a7c0 (0000)
0007AAB0 0070:
	_code_0007aab0 (0000)
0007AB20 0840:
	_code_0007ab20 (0000)
0007B360 0050:
	_code_0007b360 (0000)
0007B3B0 00a0:
	_code_0007b3b0 (0000)
0007B450 0130:
	_observer_update (0000)
00256F3C 001c:
	_rdata_00256f3c (0000)
00256F58 0021:
	??_C@_0CB@MBFPPGME@c?3?2halo?2SOURCE?2camera?2observer?4c@ (0000)
00256F80 0084:
	??_C@_0IE@MHMDJBI@observer?9?$DOresult?4location?4cluste@ (0000)
00257004 0028:
	??_C@_0CI@NBOHAIIP@observer?9?$DOvelocities?4n?$FLparameter@ (0000)
0025702C 0009:
	??_C@_08LOHDAPLH@forward1?$AA@ (0000)
00257038 0004:
	??_C@_03BAPGBPKP@up1?$AA@ (0000)
00257040 0042:
	??_C@_0EC@LAMMDMMJ@?$CFs?0?5?$CFs?3?5assert_valid_real_vector@ (0000)
00257084 0009:
	??_C@_08KHGIDOPG@forward0?$AA@ (0000)
00257090 0004:
	??_C@_03JONCOOO@up0?$AA@ (0000)
00257098 0008:
	__real@3fb99999a0000000 (0000)
002570A0 002d:
	??_C@_0CN@JLJEHJNH@valid_field_of_view?$CIobserver?9?$DOfi@ (0000)
002570D0 0035:
	??_C@_0DF@POOMEEJD@valid_world_real_point3d?$CI?$CGobserv@ (0000)
00257108 0025:
	??_C@_0CF@HBCBGNBI@valid_focus_distance?$CIfocus_dista@ (0000)
00257130 0043:
	??_C@_0ED@DKCKLPPD@valid_world_real_point3d?$CI?$CIreal_p@ (0000)
00257174 0013:
	??_C@_0BD@EHDGCMIA@?$CGobserver?9?$DOforward?$AA@ (0000)
00257188 000e:
	??_C@_0O@NJKILIMK@?$CGobserver?9?$DOup?$AA@ (0000)
00257198 002a:
	??_C@_0CK@OGOGINPI@valid_world_real_point3d?$CI?$CGfocus_@ (0000)
002571C4 001d:
	??_C@_0BN@FGDILBJM@?$CBobserver?9?$DOupdated_for_frame?$AA@ (0000)
002571E8 0062:
	??_C@_0GC@DNGKAJPP@observer?9?$DOheader_signature?$DN?$DNOBSE@ (0000)
002DCC6C 0016:
	_data_002dcc6c (0000)
0031D4B8 0a74:
	_bss_0031d4b8 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "observer.h"

#include "game/players.h"
#include "networking/network_connection.h"
#include "cache/predicted_resources.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"

/* ---------- constants */

enum
{
	OBSERVER_SIGNATURE = 'rad!',

	NUMBER_OF_OBSERVER_VELOCITIES = 11
};

#define DEFAULT_OBSERVER_FIELD_OF_VIEW DEGREES_TO_RADIANS(50.f)
#define OBSERVER_SINE_REGION_ANGLE 0.174f

/* ---------- macros */

/* ---------- structures */

struct observer_derivative
{
	real n[NUMBER_OF_OBSERVER_VELOCITIES];
	real_vector3d rotation_offset;
};

struct observer
{
	long header_signature;
	struct observer_command *pending_command;
	struct observer_command last_command;
	boolean updated_for_frame;
	boolean first_command;
	byte pad072[2];
	struct observer_result result;
	union
	{
		struct
		{
			real_point3d focus_position;
			real_vector3d focus_offset;
			real focus_distance;
			real field_of_view;
			real_vector3d forward;
			real_vector3d up;
		};
		real parameters[14];
	};
	struct observer_derivative velocities;
	struct observer_derivative accelerations;
	real a[NUMBER_OF_OBSERVER_VELOCITIES];
	real b[NUMBER_OF_OBSERVER_VELOCITIES];
	real c[NUMBER_OF_OBSERVER_VELOCITIES];
	real d[NUMBER_OF_OBSERVER_VELOCITIES];
	real e[NUMBER_OF_OBSERVER_VELOCITIES];
	real f[NUMBER_OF_OBSERVER_VELOCITIES];
	struct observer_derivative displacements;
	long trailer_signature;
};

struct observer_globals
{
	real dtime;
	struct observer local_players[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
};

typedef char observer_last_command_offset_assert[
	offsetof(struct observer, last_command) == 0x8 ? 1 : -1];
typedef char observer_result_offset_assert[
	offsetof(struct observer, result) == 0x74 ? 1 : -1];
typedef char observer_velocities_offset_assert[
	offsetof(struct observer, velocities) == 0xE8 ? 1 : -1];
typedef char observer_accelerations_offset_assert[
	offsetof(struct observer, accelerations) == 0x120 ? 1 : -1];
typedef char observer_displacements_offset_assert[
	offsetof(struct observer, displacements) == 0x260 ? 1 : -1];
typedef char observer_size_assert[
	sizeof(struct observer) == 0x29C ? 1 : -1];
typedef char observer_globals_size_assert[
	sizeof(struct observer_globals) == 0xA74 ? 1 : -1];

/* ---------- prototypes */

static struct observer *observer_get(
	short local_player_index);
static void observer_clear(
	struct observer *observer);
static void observer_update_accelerations(
	short local_player_index);
static void observer_update_velocities(
	short local_player_index);
static void observer_apply_rotational_displacement(
	real_vector3d const *rotational_displacement,
	real_vector3d *forward,
	real_vector3d *up);
static boolean observer_collision_test_with_t(
	real_point3d const *point0,
	real_point3d const *point1,
	real *t,
	boolean ignore_media);
static void observer_update_command(
	short local_player_index);
static void observer_update_polynomial(
	short local_player_index);
static void observer_update_positions(
	short local_player_index);
static void observer_rotational_displacement(
	real_vector3d const *forward0,
	real_vector3d const *up0,
	real_vector3d const *forward1,
	real_vector3d const *up1,
	real_vector3d *rotation);
static void observer_check_penetration(
	real_point3d *focus_position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real *distance,
	real safe_distance);
static void observer_find_displacement(
	real const position0[14],
	real const position1[14],
	struct observer_derivative *displacement);
static void observer_postcheck(
	short local_player_index);
static void observer_update_displacements(
	short local_player_index);
static void observer_pass_time(
	short local_player_index);

/* ---------- globals */

static real const observer_maximum_accelerations[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS] =
{
	1500.f,
	1500.f,
	100000.f,
	100000.f,
	100000.f
};

static short observer_parameter_real_counts[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS] =
{
	3,
	3,
	1,
	1,
	6
};

static short observer_parameter_derivative_real_counts[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS] =
{
	3,
	3,
	1,
	1,
	3
};

static struct observer_globals observer_globals;

/* ---------- private code */

static struct observer *observer_get(
	short local_player_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x72,
		local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);

	return &observer_globals.local_players[local_player_index];
}

/* ---------- public code */

void observer_initialize(
	void)
{
	return;
}

static void observer_clear(
	struct observer *observer)
{
	observer->forward = *global_forward3d;
	observer->up = *global_up3d;
	observer->field_of_view = DEFAULT_OBSERVER_FIELD_OF_VIEW;

	observer->result.position = *global_origin3d;
	observer->result.location.cluster_index = NONE;
	observer->result.location.leaf_index = NONE;
	observer->result.velocity = *global_zero_vector3d;
	observer->result.forward = *global_forward3d;
	observer->result.up = *global_up3d;
	observer->result.field_of_view = DEFAULT_OBSERVER_FIELD_OF_VIEW;

	csmemset(&observer->last_command, 0, sizeof(observer->last_command));
	observer->last_command.forward = observer->forward;
	observer->last_command.up = observer->up;
	observer->last_command.field_of_view = observer->field_of_view;

	observer->trailer_signature = OBSERVER_SIGNATURE;
	observer->header_signature = OBSERVER_SIGNATURE;

	observer->updated_for_frame = TRUE;
	observer->first_command = FALSE;

	return;
}

void observer_initialize_for_new_map(
	void)
{
	short local_player_index;

	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		observer_clear(observer_get(local_player_index));
	}

	return;
}

void observer_dispose_from_old_map(
	void)
{
	return;
}

struct observer_result const *observer_get_camera(
	short local_player_index)
{
	struct observer_result const *result = NULL;

	if (local_player_index != NONE)
	{
		struct observer *observer = observer_get(local_player_index);

		result = &observer->result;

		match_assert(
			"c:\\halo\\SOURCE\\camera\\observer.c",
			0x12D,
			observer->result.location.cluster_index>=NONE && observer->result.location.cluster_index<global_structure_bsp_get()->clusters.count);
	}

	return result;
}

boolean observer_command_has_finished(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	short parameter_index;

	if (observer->last_command.timer != 0.f)
	{
		return FALSE;
	}

	for (parameter_index = 0;
		parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
		parameter_index++)
	{
		if (observer->last_command.parameter_timers[parameter_index] != 0.f)
		{
			return FALSE;
		}
	}

	return TRUE;
}

void observer_reconnect_to_structure_bsp(
	void)
{
	short local_player_index;

	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		if (local_player_get_player_index(local_player_index) != NONE)
		{
			struct observer *observer = observer_get(local_player_index);

			scenario_location_from_point(
				&observer->result.location,
				&observer->result.position);
		}
	}

	return;
}

void observer_obsolete_position(
	short local_player_index)
{
	observer_clear(observer_get(local_player_index));

	return;
}

void observer_up_from_forward(
	real_vector3d const *forward,
	real_vector3d *up)
{
	real_vector3d right;

	right.i = forward->j;
	right.j = -forward->i;
	right.k = 0.f;

	if (normalize3d(&right) == 0.f)
	{
		right.i = 1.f;
		right.j = right.k = 0.f;
	}

	cross_product3d(&right, forward, up);

	return;
}

void observer_set_camera(
	short local_player_index,
	struct observer_command *command)
{
	struct observer *observer = observer_get(local_player_index);

	match_vassert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0xE9,
		command &&
		(!TEST_FLAG(command->flags, _observer_command_valid_bit) ||
		(valid_real_vector3d_axes2(&command->forward, &command->up) &&
			valid_real(command->focus_position.x) && command->focus_position.x>=-5000.f && command->focus_position.x<=5000.f &&
			valid_real(command->focus_position.y) && command->focus_position.y>=-5000.f && command->focus_position.y<=5000.f &&
			valid_real(command->focus_position.z) && command->focus_position.z>=-5000.f && command->focus_position.z<=5000.f &&
			valid_real(command->focus_offset.i) && command->focus_offset.i>=-5000.f && command->focus_offset.i<=5000.f &&
			valid_real(command->focus_offset.j) && command->focus_offset.j>=-5000.f && command->focus_offset.j<=5000.f &&
			valid_real(command->focus_offset.k) && command->focus_offset.k>=-5000.f && command->focus_offset.k<=5000.f &&
			valid_real_vector3d(&command->focus_velocity) &&
			valid_real(command->focus_distance) && command->focus_distance>=0.f && command->focus_distance<=5000.f &&
			valid_real(command->field_of_view) && command->field_of_view>=0.001f && command->field_of_view<=_pi / 2.f &&
			valid_real(command->timer) && command->timer>=0.f && command->timer<=3600.f)),
		csprintf(
			temporary,
			"Invalid camera command.\nF: (%f, %f, %f) U: (%f, %f, %f)\nP: (%f, %f, %f) O: (%f, %f, %f)\nD: %f V: (%f, %f, %f), FOV: %f, T: %f, FL: %ld",
			command->forward.i,
			command->forward.j,
			command->forward.k,
			command->up.i,
			command->up.j,
			command->up.k,
			command->focus_position.x,
			command->focus_position.y,
			command->focus_position.z,
			command->focus_offset.i,
			command->focus_offset.j,
			command->focus_offset.k,
			command->focus_distance,
			command->focus_velocity.i,
			command->focus_velocity.j,
			command->focus_velocity.k,
			command->field_of_view,
			command->timer,
			command->flags));

	observer->pending_command = command;
	observer->updated_for_frame = FALSE;

	if (!observer->first_command)
	{
		observer->first_command = TRUE;
		observer->pending_command->timer = 0.f;
		SET_FLAG(observer->pending_command->flags, _observer_command_force_time_bit, TRUE);
		csmemset(
			observer->pending_command->parameter_timers,
			0,
			sizeof(observer->pending_command->parameter_timers));
	}

	return;
}

static void observer_update_accelerations(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	real *acceleration = observer->accelerations.n;
	real *a = observer->a;
	real *b = observer->b;
	real *c = observer->c;
	real *d = observer->d;
	real *timer = observer->last_command.parameter_timers;
	short parameter_index;

	for (parameter_index = 0;
		parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
		parameter_index++)
	{
		real remaining_time = *timer - observer_globals.dtime;
		short value_count = observer_parameter_derivative_real_counts[parameter_index];
		short value_index;

		if (!(remaining_time > 0.f))
		{
			csmemset(acceleration, 0, sizeof(real)*value_count);
		}
		else
		{
			for (value_index = 0; value_index < value_count; value_index++)
			{
				real remaining_time_squared = remaining_time*remaining_time;
				real value =
					20.f*a[value_index]*remaining_time_squared*remaining_time +
					12.f*b[value_index]*remaining_time_squared +
					6.f*c[value_index]*remaining_time +
					2.f*d[value_index];

				acceleration[value_index] = value;
				if (value > observer_maximum_accelerations[parameter_index] ||
					value < -observer_maximum_accelerations[parameter_index])
				{
					short other_parameter_index;

					for (other_parameter_index = 0;
						other_parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
						other_parameter_index++)
					{
						if (other_parameter_index != parameter_index &&
							observer->last_command.parameter_timers[other_parameter_index] == *timer)
						{
							observer->last_command.parameter_timers[other_parameter_index] = 0.f;
						}
					}
					*timer = 0.f;
				}
			}
		}

		acceleration += value_count;
		a += value_count;
		b += value_count;
		c += value_count;
		d += value_count;
		timer++;
	}

	return;
}

static void observer_update_velocities(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	real inverse_dtime = (real)(1.0 / observer_globals.dtime);
	real *displacement = observer->displacements.n;
	real *velocity = observer->velocities.n;
	real *a = observer->a;
	real *b = observer->b;
	real *c = observer->c;
	real *d = observer->d;
	real *e = observer->e;
	real *timer = observer->last_command.parameter_timers;
	byte *parameter_flags = observer->last_command.parameter_flags;
	short parameter_index;

	for (parameter_index = 0;
		parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
		parameter_index++)
	{
		real remaining_time = *timer - observer_globals.dtime;
		short value_count = observer_parameter_derivative_real_counts[parameter_index];
		short value_index;

		if (!(remaining_time > 0.f))
		{
			if (TEST_FLAG(observer->last_command.flags, _observer_command_valid_bit))
			{
				if (TEST_FLAG(*parameter_flags, _observer_time_force_bit) ||
					TEST_FLAG(observer->last_command.flags, _observer_command_force_time_bit))
				{
					csmemset(velocity, 0, sizeof(real)*value_count);
				}
				else
				{
					for (value_index = 0; value_index < value_count; value_index++)
					{
						velocity[value_index] = -displacement[value_index]*inverse_dtime;
					}
				}
			}
		}
		else
		{
			for (value_index = 0; value_index < value_count; value_index++)
			{
				real remaining_time_squared = remaining_time*remaining_time;

				velocity[value_index] =
					5.f*a[value_index]*remaining_time_squared*remaining_time_squared +
					4.f*b[value_index]*remaining_time_squared*remaining_time +
					3.f*c[value_index]*remaining_time_squared +
					2.f*d[value_index]*remaining_time +
					e[value_index];
			}
		}

		displacement += value_count;
		velocity += value_count;
		a += value_count;
		b += value_count;
		c += value_count;
		d += value_count;
		e += value_count;
		timer++;
		parameter_flags++;
	}

	return;
}

static boolean observer_collision_test_with_t(
	real_point3d const *point0,
	real_point3d const *point1,
	real *t,
	boolean ignore_media)
{
	unsigned long flags =
		FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_media_bit) |
		FLAG(_collision_test_objects_bit) |
		FLAG(_collision_test_objects_scenery_bit);
	real_vector3d vector;
	struct collision_result collision;
	boolean result;

	SET_FLAG(flags, _collision_test_media_bit, !ignore_media);

	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x4B4,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_observer;

	vector_from_points3d(point0, point1, &vector);
	result = collision_test_vector(flags, point0, &vector, NONE, &collision);
	if (result)
	{
		*t = collision.t;
	}

	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x4BA,
		global_current_collision_user_depth > 1);
	global_current_collision_user_depth--;

	return result;
}

static void observer_update_command(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	struct observer_command *command = observer->pending_command;
	short parameter_index;

	match_vassert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x172,
		command &&
		(!TEST_FLAG(command->flags, _observer_command_valid_bit) ||
		(valid_real_vector3d_axes2(&command->forward, &command->up) &&
			valid_real(command->focus_position.x) && command->focus_position.x>=-5000.f && command->focus_position.x<=5000.f &&
			valid_real(command->focus_position.y) && command->focus_position.y>=-5000.f && command->focus_position.y<=5000.f &&
			valid_real(command->focus_position.z) && command->focus_position.z>=-5000.f && command->focus_position.z<=5000.f &&
			valid_real(command->focus_offset.i) && command->focus_offset.i>=-5000.f && command->focus_offset.i<=5000.f &&
			valid_real(command->focus_offset.j) && command->focus_offset.j>=-5000.f && command->focus_offset.j<=5000.f &&
			valid_real(command->focus_offset.k) && command->focus_offset.k>=-5000.f && command->focus_offset.k<=5000.f &&
			valid_real_vector3d(&command->focus_velocity) &&
			valid_real(command->focus_distance) && command->focus_distance>=0.f && command->focus_distance<=5000.f &&
			valid_real(command->field_of_view) && command->field_of_view>=0.001f && command->field_of_view<=_pi / 2.f &&
			valid_real(command->timer) && command->timer>=0.f && command->timer<=3600.f)),
		csprintf(
			temporary,
			"Invalid camera command.\nF: (%f, %f, %f) U: (%f, %f, %f)\nP: (%f, %f, %f) O: (%f, %f, %f)\nD: %f V: (%f, %f, %f), FOV: %f, T: %f, FL: %ld",
			command->forward.i,
			command->forward.j,
			command->forward.k,
			command->up.i,
			command->up.j,
			command->up.k,
			command->focus_position.x,
			command->focus_position.y,
			command->focus_position.z,
			command->focus_offset.i,
			command->focus_offset.j,
			command->focus_offset.k,
			command->focus_distance,
			command->focus_velocity.i,
			command->focus_velocity.j,
			command->focus_velocity.k,
			command->field_of_view,
			command->timer,
			command->flags));

	if (TEST_FLAG(command->flags, _observer_command_valid_bit))
	{
		for (parameter_index = 0;
			parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
			parameter_index++)
		{
			real last_timer = observer->last_command.parameter_timers[parameter_index];

			if (TEST_FLAG(command->parameter_flags[parameter_index], _observer_time_valid_bit))
			{
				if (!TEST_FLAG(command->parameter_flags[parameter_index], _observer_time_force_bit) &&
					command->parameter_timers[parameter_index] < last_timer)
				{
					command->parameter_timers[parameter_index] = MIN(last_timer, 2.f);
				}
			}
			else if (command->timer >= last_timer ||
				TEST_FLAG(command->flags, _observer_command_force_time_bit))
			{
				command->parameter_timers[parameter_index] = command->timer;
			}
			else
			{
				command->parameter_timers[parameter_index] = MIN(last_timer, 2.f);
			}
		}

		observer->last_command = *command;
	}

	return;
}

static void observer_update_polynomial(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	real *acceleration = observer->accelerations.n;
	real *displacement = observer->displacements.n;
	real *velocity = observer->velocities.n;
	real *a = observer->a;
	real *b = observer->b;
	real *c = observer->c;
	real *d = observer->d;
	real *e = observer->e;
	real *f = observer->f;
	real *timer = observer->last_command.parameter_timers;
	short parameter_index;

	match_vassert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x1F6,
		!TEST_FLAG(observer->last_command.flags, _observer_command_valid_bit) ||
		(valid_real_vector3d_axes2(&observer->last_command.forward, &observer->last_command.up) &&
			valid_real(observer->last_command.focus_position.x) && observer->last_command.focus_position.x>=-5000.f && observer->last_command.focus_position.x<=5000.f &&
			valid_real(observer->last_command.focus_position.y) && observer->last_command.focus_position.y>=-5000.f && observer->last_command.focus_position.y<=5000.f &&
			valid_real(observer->last_command.focus_position.z) && observer->last_command.focus_position.z>=-5000.f && observer->last_command.focus_position.z<=5000.f &&
			valid_real(observer->last_command.focus_offset.i) && observer->last_command.focus_offset.i>=-5000.f && observer->last_command.focus_offset.i<=5000.f &&
			valid_real(observer->last_command.focus_offset.j) && observer->last_command.focus_offset.j>=-5000.f && observer->last_command.focus_offset.j<=5000.f &&
			valid_real(observer->last_command.focus_offset.k) && observer->last_command.focus_offset.k>=-5000.f && observer->last_command.focus_offset.k<=5000.f &&
			valid_real_vector3d(&observer->last_command.focus_velocity) &&
			valid_real(observer->last_command.focus_distance) && observer->last_command.focus_distance>=0.f && observer->last_command.focus_distance<=5000.f &&
			valid_real(observer->last_command.field_of_view) && observer->last_command.field_of_view>=0.001f && observer->last_command.field_of_view<=_pi / 2.f &&
			valid_real(observer->last_command.timer) && observer->last_command.timer>=0.f && observer->last_command.timer<=3600.f),
		csprintf(
			temporary,
			"Invalid camera command.\nF: (%f, %f, %f) U: (%f, %f, %f)\nP: (%f, %f, %f) O: (%f, %f, %f)\nD: %f V: (%f, %f, %f), FOV: %f, T: %f, FL: %ld",
			observer->last_command.forward.i,
			observer->last_command.forward.j,
			observer->last_command.forward.k,
			observer->last_command.up.i,
			observer->last_command.up.j,
			observer->last_command.up.k,
			observer->last_command.focus_position.x,
			observer->last_command.focus_position.y,
			observer->last_command.focus_position.z,
			observer->last_command.focus_offset.i,
			observer->last_command.focus_offset.j,
			observer->last_command.focus_offset.k,
			observer->last_command.focus_distance,
			observer->last_command.focus_velocity.i,
			observer->last_command.focus_velocity.j,
			observer->last_command.focus_velocity.k,
			observer->last_command.field_of_view,
			observer->last_command.timer,
			observer->last_command.flags));

	for (parameter_index = 0;
		parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
		parameter_index++)
	{
		short value_count = observer_parameter_derivative_real_counts[parameter_index];

		if (TEST_FLAG(observer->last_command.flags, _observer_command_valid_bit) &&
			*timer > observer_globals.dtime)
		{
			real inverse_time = 1.f / *timer;
			real inverse_time_squared = inverse_time*inverse_time;
			real inverse_time_cubed = inverse_time_squared*inverse_time;
			real inverse_time_fourth = inverse_time_cubed*inverse_time;
			real inverse_time_fifth = inverse_time_fourth*inverse_time;
			short value_index;

			for (value_index = 0; value_index < value_count; value_index++)
			{
				a[value_index] =
					acceleration[value_index]*inverse_time_cubed*0.5f -
					(displacement[value_index]*inverse_time_fifth*6.f +
						velocity[value_index]*inverse_time_fourth*3.f);
				b[value_index] =
					displacement[value_index]*inverse_time_fourth*15.f +
					velocity[value_index]*inverse_time_cubed*7.f -
					acceleration[value_index]*inverse_time_squared;
				c[value_index] =
					acceleration[value_index]*inverse_time*0.5f -
					(displacement[value_index]*inverse_time_cubed*10.f +
						velocity[value_index]*inverse_time_squared*4.f);
				d[value_index] = 0.f;
				e[value_index] = 0.f;
				f[value_index] = displacement[value_index];

				if (parameter_index == _observer_command_parameter_focus_position)
				{
					real final_velocity = observer->last_command.focus_velocity.n[value_index]*30.f;

					a[value_index] -= 3.f*final_velocity*inverse_time_fourth;
					b[value_index] += 8.f*final_velocity*inverse_time_cubed;
					c[value_index] -= 6.f*final_velocity*inverse_time_squared;
					e[value_index] += final_velocity;
				}
			}
		}

		acceleration += value_count;
		displacement += value_count;
		velocity += value_count;
		a += value_count;
		b += value_count;
		c += value_count;
		d += value_count;
		e += value_count;
		f += value_count;
		timer++;
	}

	return;
}

static void observer_apply_rotational_displacement(
	real_vector3d const *rotational_displacement,
	real_vector3d *forward,
	real_vector3d *up)
{
	real_vector3d axis = *rotational_displacement;
	real angle = normalize3d(&axis);

	if (angle != 0.f)
	{
		real sine_value = (real)sin(angle);
		real cosine_value = (real)cos(angle);

		rotate_vector_about_axis(forward, &axis, sine_value, cosine_value);
		rotate_vector_about_axis(up, &axis, sine_value, cosine_value);
	}

	return;
}

static void observer_update_positions(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	real displacement[NUMBER_OF_OBSERVER_VELOCITIES];
	real *delta = displacement;
	real *position = observer->parameters;
	real *target = observer->last_command.parameters;
	real *velocity = observer->velocities.n;
	real *a = observer->a;
	real *b = observer->b;
	real *c = observer->c;
	real *d = observer->d;
	real *e = observer->e;
	real *f = observer->f;
	real *timer = observer->last_command.parameter_timers;
	short value_index;
	short parameter_index;

	for (value_index = 0; value_index < NUMBER_OF_OBSERVER_VELOCITIES; value_index++)
	{
		match_assert_valid_real(
			"c:\\halo\\SOURCE\\camera\\observer.c",
			0x2F4,
			observer->velocities.n[value_index]);
	}
	csmemset(displacement, 0, sizeof(displacement));

	for (parameter_index = 0;
		parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
		parameter_index++)
	{
		real remaining_time = *timer - observer_globals.dtime;
		short derivative_count = observer_parameter_derivative_real_counts[parameter_index];
		short real_count = observer_parameter_real_counts[parameter_index];
		boolean snapped_to_target = FALSE;

		if (remaining_time > 0.f)
		{
			for (value_index = 0; value_index < derivative_count; value_index++)
			{
				real remaining_time_squared = remaining_time*remaining_time;

				delta[value_index] =
					a[value_index]*remaining_time_squared*remaining_time_squared*remaining_time +
					b[value_index]*remaining_time_squared*remaining_time_squared +
					c[value_index]*remaining_time_squared*remaining_time +
					d[value_index]*remaining_time_squared +
					e[value_index]*remaining_time +
					f[value_index];
			}
		}
		else if (TEST_FLAG(observer->last_command.flags, _observer_command_valid_bit))
		{
			for (value_index = 0; value_index < real_count; value_index++)
			{
				position[value_index] = target[value_index];
			}
			snapped_to_target = TRUE;
		}
		else
		{
			for (value_index = 0; value_index < derivative_count; value_index++)
			{
				delta[value_index] = -velocity[value_index]*observer_globals.dtime;
			}
		}

		if (!snapped_to_target)
		{
			if (parameter_index == _observer_command_parameter_orientation)
			{
				real_vector3d rotational_displacement = { delta[0], delta[1], delta[2] };

				observer_apply_rotational_displacement(
					&rotational_displacement,
					&observer->forward,
					&observer->up);
			}
			else
			{
				for (value_index = 0; value_index < derivative_count; value_index++)
				{
					position[value_index] += delta[value_index];
				}
			}
		}

		position += real_count;
		target += real_count;
		delta += derivative_count;
		velocity += derivative_count;
		a += derivative_count;
		b += derivative_count;
		c += derivative_count;
		d += derivative_count;
		e += derivative_count;
		f += derivative_count;
		timer++;
	}

	if (!valid_real_vector3d_axes2(&observer->forward, &observer->up))
	{
		real_vector3d left;

		cross_product3d(&observer->up, &observer->forward, &left);
		cross_product3d(&observer->forward, &left, &observer->up);
		normalize3d(&observer->forward);
		normalize3d(&observer->up);
	}

	return;
}

static void observer_rotational_displacement(
	real_vector3d const *forward0,
	real_vector3d const *up0,
	real_vector3d const *forward1,
	real_vector3d const *up1,
	real_vector3d *rotation)
{
	real_matrix4x3 matrix0;
	real_matrix4x3 matrix1;

	match_assert_valid_real_vector3d_axes2(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x382,
		forward0,
		up0);
	match_assert_valid_real_vector3d_axes2(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x383,
		forward1,
		up1);

	matrix4x3_rotation_from_vectors(&matrix0, forward0, up0);
	matrix4x3_rotation_from_vectors(&matrix1, forward1, up1);
	vector_from_matrices4x3(&matrix0, &matrix1, rotation);

	return;
}

static void observer_find_displacement(
	real const position0[14],
	real const position1[14],
	struct observer_derivative *displacement)
{
	real *displacement_value = displacement->n;
	short parameter_index = 0;

	do
	{
		*displacement_value++ = *position1++ - *position0++;
		parameter_index++;
	}
	while (parameter_index < 8);

	while (parameter_index < 14)
	{
		observer_rotational_displacement(
			(real_vector3d const *)position0,
			(real_vector3d const *)(position0 + 3),
			(real_vector3d const *)position1,
			(real_vector3d const *)(position1 + 3),
			(real_vector3d *)displacement_value);
		position0 += 6;
		position1 += 6;
		displacement_value += 3;
		parameter_index += 6;
	}

	return;
}

static void observer_check_penetration(
	real_point3d *focus_position,
	real_vector3d const *forward,
	real_vector3d const *up,
	real *distance,
	real safe_distance)
{
	struct location focus_location;
	boolean ignore_media;
	real collision_t = 1.f;
	real reach = *distance + safe_distance;
	real_point3d desired_position;
	real step = *distance*OBSERVER_SINE_REGION_ANGLE;
	real_vector3d perturbations[2];
	real first_t;
	real best_t;
	real_vector3d *best_offset = NULL;
	real best_sign = 0.f;
	short probe_index;

	scenario_location_from_point(&focus_location, focus_position);
	ignore_media = scenario_location_underwater(&focus_location, focus_position, NULL);

	desired_position.x = focus_position->x - forward->i*reach;
	desired_position.y = focus_position->y - forward->j*reach;
	desired_position.z = focus_position->z - forward->k*reach;
	observer_collision_test_with_t(
		focus_position,
		&desired_position,
		&collision_t,
		ignore_media);

	scale_vector3d(up, step, &perturbations[0]);
	cross_product3d(up, forward, &perturbations[1]);
	scale_vector3d(&perturbations[1], step, &perturbations[1]);

	first_t = collision_t;
	best_t = first_t;
	for (probe_index = 0; probe_index < 4; probe_index++)
	{
		real sign = TEST_FLAG(probe_index, 1) ? 1.f : -1.f;
		real_vector3d *offset = &perturbations[probe_index & 1];
		real_point3d probe;

		probe.x = desired_position.x + sign*offset->i;
		probe.y = desired_position.y + sign*offset->j;
		probe.z = desired_position.z + sign*offset->k;
		if (observer_collision_test_with_t(
			focus_position,
			&probe,
			&collision_t,
			ignore_media) &&
			collision_t < best_t)
		{
			best_t = collision_t;
			best_offset = offset;
			best_sign = sign;
		}
	}

	if (best_offset)
	{
		real last_miss_t = first_t;
		real last_hit_t = best_t;
		real lower = 0.f;
		real upper = best_sign;
		short iteration;

		for (iteration = 0; iteration < 10; iteration++)
		{
			real middle = (upper + lower)*0.5f;
			real_point3d probe;
			boolean hit;

			probe.x = desired_position.x + middle*best_offset->i;
			probe.y = desired_position.y + middle*best_offset->j;
			probe.z = desired_position.z + middle*best_offset->k;
			hit = observer_collision_test_with_t(
				focus_position,
				&probe,
				&collision_t,
				ignore_media);
			if (hit && fabs(collision_t - last_hit_t) < 0.1f)
			{
				upper = middle;
				last_hit_t = collision_t;
			}
			else
			{
				lower = middle;
				last_miss_t = hit ? collision_t : 1.f;
			}
		}

		if (last_miss_t < last_hit_t)
			upper = lower;
		else if (upper < 0.f)
			upper = -upper;

		*distance *= (1.f - upper)*best_t + upper*first_t;
	}
	else
	{
		*distance *= first_t;
	}

	return;
}

static void observer_postcheck(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	real_point3d focus_position = observer->focus_position;
	real focus_distance = PIN(observer->focus_distance, 0.f, REAL_MAX);
	real heading_length;
	real heading_i = observer->forward.i;
	real heading_j = observer->forward.j;
	real offset_i;
	real offset_j;
	struct location location;
	real water_depth;

	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x3AF,
		valid_real_point3d(&focus_position) &&
		focus_position.x>=-5000.f && focus_position.x<=5000.f &&
		focus_position.y>=-5000.f && focus_position.y<=5000.f &&
		focus_position.z>=-5000.f && focus_position.z<=5000.f);
	match_assert_valid_real_vector3d_axes2(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x3B0,
		&observer->forward,
		&observer->up);
	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x3B1,
		valid_real_vector3d(&observer->focus_offset) &&
		observer->focus_offset.i>=-5000.f && observer->focus_offset.i<=5000.f &&
		observer->focus_offset.j>=-5000.f && observer->focus_offset.j<=5000.f &&
		observer->focus_offset.k>=-5000.f && observer->focus_offset.k<=5000.f);
	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x3B2,
		valid_real(focus_distance) && focus_distance>=0.f && focus_distance<=5000.f);

	observer->field_of_view = PIN(observer->field_of_view, 0.001f, _pi/2.f);
	focus_position.x = PIN(focus_position.x, -5000.f, 5000.f);
	focus_position.y = PIN(focus_position.y, -5000.f, 5000.f);
	focus_position.z = PIN(focus_position.z, -5000.f, 5000.f);
	focus_distance = PIN(focus_distance, 0.f, 5000.f);

	heading_length = square_root(heading_i*heading_i + heading_j*heading_j);
	if (fabs(heading_length) >= 0.0001f)
	{
		heading_i /= heading_length;
		heading_j /= heading_length;
	}

	offset_i = observer->focus_offset.i*heading_i + observer->focus_offset.j*heading_j;
	offset_j = observer->focus_offset.i*heading_j - observer->focus_offset.j*heading_i;
	focus_position.x += offset_i;
	focus_position.y += offset_j;
	focus_position.z += observer->focus_offset.k;

	if (!TEST_FLAG(observer->last_command.flags, _observer_command_ignore_obstructions_bit) &&
		focus_distance != 0.f)
	{
		observer_check_penetration(
			&focus_position,
			&observer->forward,
			&observer->up,
			&focus_distance,
			0.02f);
	}

	observer->result.position.x = focus_position.x - observer->forward.i*focus_distance;
	observer->result.position.y = focus_position.y - observer->forward.j*focus_distance;
	observer->result.position.z = focus_position.z - observer->forward.k*focus_distance;

	scenario_location_from_point(&location, &observer->result.position);
	if (location.cluster_index != NONE)
	{
		if (location.cluster_index != observer->result.location.cluster_index)
		{
			struct structure_cluster *cluster = TAG_BLOCK_GET_ELEMENT(
				&global_structure_bsp_get()->clusters,
				location.cluster_index,
				struct structure_cluster);

			predicted_resources_precache(&cluster->predicted_resources);
		}
		observer->result.location = location;
	}

	water_depth = scenario_location_water_depth(
		&observer->result.location,
		&observer->result.position);
	if (fabs(water_depth) < 0.05f)
	{
		if (water_depth > 0.f)
		{
			observer->result.position.z -= 0.05f - water_depth;
		}
		else
		{
			observer->result.position.z += water_depth + 0.05f;
		}
	}

	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x41F,
		valid_real_point3d(&observer->result.position) &&
		observer->result.position.x>=-5000.f && observer->result.position.x<=5000.f &&
		observer->result.position.y>=-5000.f && observer->result.position.y<=5000.f &&
		observer->result.position.z>=-5000.f && observer->result.position.z<=5000.f);
	match_assert_valid_real_vector3d_axes2(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x420,
		&observer->forward,
		&observer->up);
	match_assert(
		"c:\\halo\\SOURCE\\camera\\observer.c",
		0x421,
		valid_real(observer->field_of_view) &&
		observer->field_of_view>=0.001f && observer->field_of_view<=_pi/2.f);

	observer->result.position.x = PIN(observer->result.position.x, -5000.f, 5000.f);
	observer->result.position.y = PIN(observer->result.position.y, -5000.f, 5000.f);
	observer->result.position.z = PIN(observer->result.position.z, -5000.f, 5000.f);
	observer->result.velocity.i = -observer->velocities.n[0];
	observer->result.velocity.j = -observer->velocities.n[1];
	observer->result.velocity.k = -observer->velocities.n[2];
	observer->result.forward = observer->forward;
	observer->result.up = observer->up;
	observer->result.field_of_view = observer->field_of_view;

	return;
}

static void observer_update_displacements(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);

	observer_find_displacement(
		observer->parameters,
		observer->last_command.parameters,
		&observer->displacements);

	return;
}

static void observer_pass_time(
	short local_player_index)
{
	struct observer *observer = observer_get(local_player_index);
	real *timer = observer->last_command.parameter_timers;
	short parameter_index;

	if (!TEST_FLAG(observer->pending_command->flags, _observer_command_freeze_camera_bit))
	{
		observer_update_displacements(local_player_index);
		observer_update_polynomial(local_player_index);
		observer_update_accelerations(local_player_index);
		observer_update_velocities(local_player_index);
		observer_update_positions(local_player_index);

		for (parameter_index = 0;
			parameter_index < NUMBER_OF_OBSERVER_COMMAND_PARAMETERS;
			parameter_index++)
		{
			real remaining_time = *timer - observer_globals.dtime;

			*timer++ = remaining_time > 0.f ? remaining_time : 0.f;
		}
	}

	return;
}

void observer_update(
	real time_delta_sec)
{
	short local_player_index;

	observer_globals.dtime = time_delta_sec;
	for (local_player_index = 0;
		local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS;
		local_player_index++)
	{
		if (local_player_get_player_index(local_player_index) != NONE)
		{
			struct observer *observer = observer_get(local_player_index);

			match_assert(
				"c:\\halo\\SOURCE\\camera\\observer.c",
				0x108,
				observer->header_signature==OBSERVER_SIGNATURE &&
				observer->trailer_signature==OBSERVER_SIGNATURE);
			match_assert(
				"c:\\halo\\SOURCE\\camera\\observer.c",
				0x109,
				!observer->updated_for_frame);

			observer->updated_for_frame = TRUE;
			observer_update_command(local_player_index);
			if (observer_globals.dtime != 0.f)
			{
				observer_pass_time(local_player_index);
			}
			observer_postcheck(local_player_index);

			match_assert(
				"c:\\halo\\SOURCE\\camera\\observer.c",
				0x117,
				observer->header_signature==OBSERVER_SIGNATURE &&
				observer->trailer_signature==OBSERVER_SIGNATURE);
		}
	}

	return;
}
