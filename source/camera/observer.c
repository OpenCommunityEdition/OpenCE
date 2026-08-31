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
#include "scenario/scenario.h"
#include "structures/structure_bsp_definitions.h"

/* ---------- constants */

enum
{
	OBSERVER_SIGNATURE = 'rad!',

	NUMBER_OF_OBSERVER_VELOCITIES = 11
};

#define DEFAULT_OBSERVER_FIELD_OF_VIEW DEGREES_TO_RADIANS(50.f)

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
	real_point3d focus_position;
	real_vector3d focus_offset;
	real focus_distance;
	real field_of_view;
	real_vector3d forward;
	real_vector3d up;
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

/* ---------- globals */

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
