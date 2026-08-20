/*
PLAYER_RUMBLE.C

symbols in this file:
000A8EA0 0020:
	_rumble_initialize (0000)
000A8EC0 0010:
	_rumble_dispose (0000)
000A8ED0 0020:
	_rumble_initialize_for_new_map (0000)
000A8EF0 0020:
	_rumble_player_set_scripted_values (0000)
000A8F10 0020:
	_rumble_player_set_scale (0000)
000A8F30 0170:
	_rumble_player_impulse (0000)
000A90A0 0030:
	_rumble_player_clear (0000)
000A90D0 0040:
	_rumble_clear_all_now (0000)
000A9110 0030:
	_rumble_player_continuous (0000)
000A9140 0060:
	_rumble_dispose_from_old_map (0000)
000A91A0 01a0:
	_code_000a91a0 (0000)
000A9340 0130:
	_rumble_update (0000)
0025CE94 0007:
	??_C@_06OLEBFMCD@rumble?$AA@ (0000)
0025CE9C 0012:
	??_C@_0BC@NEAPFALG@rumble_definition?$AA@ (0000)
0025CEB0 0024:
	??_C@_0CE@KKOMCMLO@c?3?2halo?2SOURCE?2game?2player_rumbl@ (0000)
00453404 0004:
	_bss_00453404 (0000)
*/

/* ---------- headers */

#include "game/player_rumble.h"

/* ---------- constants */

enum
{
	MAXIMUM_LOCAL_PLAYERS = 4,
	MAXIMUM_RUMBLE_IMPULSES = 8,
	NUMBER_OF_RUMBLE_MOTORS = 2
};

/* ---------- macros */

/* ---------- structures */

struct rumble_motor
{
	real scale;
	real duration;
	short transition_function;
	short pad;
	real unused[2];
};

struct rumble_definition
{
	struct rumble_motor motors[NUMBER_OF_RUMBLE_MOTORS];
	real scale_floor;
	byte unused[16];
};

struct rumble_player
{
	struct rumble_definition impulses[MAXIMUM_RUMBLE_IMPULSES];
	real impulse_time[MAXIMUM_RUMBLE_IMPULSES];
	real continuous_left;
	real continuous_right;
};

struct rumble_globals
{
	struct rumble_player players[MAXIMUM_LOCAL_PLAYERS];
	real scripted_left_motor;
	real scripted_right_motor;
	real scripted_scale;
};

/* ---------- prototypes */

void *game_state_malloc(
	const char *name,
	const char *type,
	long size);
boolean input_has_gamepad(
	short gamepad_index);
void input_set_gamepad_rumbler_state(
	short gamepad_index,
	word left_motor,
	word right_motor);
void rumble_initialize(
	void);
void rumble_initialize_for_new_map(
	void);
void rumble_dispose_from_old_map(
	void);
void rumble_player_set_scale(
	real scale);
void rumble_player_impulse(
	short local_player_index,
	struct rumble_definition *rumble_definition,
	real scale,
	real duration_scale);
void rumble_player_continuous(
	short local_player_index,
	real left_motor,
	real right_motor);

/* ---------- globals */

static struct rumble_globals *bss_00453404;

/* ---------- public code */

void rumble_initialize(
	void)
{
	bss_00453404 = (struct rumble_globals *)game_state_malloc(
		"rumble",
		NULL,
		sizeof(*bss_00453404));

	return;
}

void rumble_dispose(
	void)
{
	return;
}

void rumble_initialize_for_new_map(
	void)
{
	csmemset(bss_00453404, 0, sizeof(*bss_00453404));

	return;
}

void rumble_player_set_scripted_values(
	real left_motor,
	real right_motor)
{
	bss_00453404->scripted_left_motor = left_motor;
	bss_00453404->scripted_right_motor = right_motor;

	return;
}

void rumble_player_set_scale(
	real scale)
{
	bss_00453404->scripted_scale = scale;

	return;
}

void rumble_player_impulse(
	short local_player_index,
	struct rumble_definition *rumble_definition,
	real scale,
	real duration_scale)
{
	struct rumble_player *player = &bss_00453404->players[local_player_index];
	struct rumble_definition *impulse = player->impulses;
	real longest = player->impulse_time[0];
	real motor_scale;
	long impulse_index;

	match_assert(
		"c:\\halo\\SOURCE\\game\\player_rumble.c",
		0xa4,
		rumble_definition);

	if (longest < player->impulse_time[1])
	{
		impulse = &player->impulses[1];
		longest = player->impulse_time[1];
	}
	if (longest < player->impulse_time[2])
	{
		impulse = &player->impulses[2];
		longest = player->impulse_time[2];
	}
	if (longest < player->impulse_time[3])
	{
		impulse = &player->impulses[3];
		longest = player->impulse_time[3];
	}
	if (longest < player->impulse_time[4])
	{
		impulse = &player->impulses[4];
		longest = player->impulse_time[4];
	}
	if (longest < player->impulse_time[5])
	{
		impulse = &player->impulses[5];
		longest = player->impulse_time[5];
	}
	if (longest < player->impulse_time[6])
	{
		impulse = &player->impulses[6];
		longest = player->impulse_time[6];
	}
	if (longest < player->impulse_time[7])
		impulse = &player->impulses[7];

	*impulse = *rumble_definition;

	motor_scale = (1.0f - rumble_definition->scale_floor) * scale +
		rumble_definition->scale_floor;

	impulse->motors[0].scale *= motor_scale;
	impulse->motors[1].scale *= motor_scale;
	impulse->motors[0].duration *= duration_scale;
	impulse->motors[1].duration *= duration_scale;

	impulse_index = impulse - player->impulses;
	player->impulse_time[impulse_index] = 0.0f;

	return;
}

void rumble_player_clear(
	short local_player_index)
{
	csmemset(
		&bss_00453404->players[local_player_index],
		0,
		sizeof(struct rumble_player));

	return;
}

void rumble_clear_all_now(
	void)
{
	long gamepad_index;

	csmemset(bss_00453404, 0, sizeof(*bss_00453404));

	for (gamepad_index = 0;
		gamepad_index < MAXIMUM_LOCAL_PLAYERS;
		gamepad_index++)
	{
		if (input_has_gamepad((short)gamepad_index))
		{
			input_set_gamepad_rumbler_state(
				(short)gamepad_index,
				0,
				0);
		}
	}

	return;
}

void rumble_player_continuous(
	short local_player_index,
	real left_motor,
	real right_motor)
{
	struct rumble_player *player = &bss_00453404->players[local_player_index];

	player->continuous_left = left_motor;
	player->continuous_right = right_motor;

	return;
}

void rumble_dispose_from_old_map(
	void)
{
	short gamepad_index;
	long index;

	for (gamepad_index = 0;
		gamepad_index < MAXIMUM_LOCAL_PLAYERS;
		gamepad_index++)
	{
		input_set_gamepad_rumbler_state(gamepad_index, 0, 0);
	}

	csmemset(bss_00453404, 0, sizeof(*bss_00453404));

	for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
	{
		if (input_has_gamepad((short)index))
		{
			input_set_gamepad_rumbler_state((short)index, 0, 0);
		}
	}

	return;
}

/* ---------- private code */
