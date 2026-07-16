/*
PLAYERS.H

header included in hcex build.
*/

#ifndef __PLAYERS_H
#define __PLAYERS_H
#pragma once

/* ---------- headers */

#include "game/game.h"

/* ---------- constants */

enum
{
	_player_powerup_active_camouflage = 0,
	_player_powerup_full_spectrum_vision,
	NUMBER_OF_PLAYER_POWERUPS,
	MAXIMUM_LOCAL_PLAYERS = 4,
};


/* ---------- macros */

#define player_get(index)			((struct player_datum*)datum_get(player_data, index))
#define player_try_and_get(index)	((struct player_datum*)datum_try_and_get(player_data, index))

/* ---------- structures */

struct player_action
{
	unsigned long control_flags;
	real_euler_angles2d desired_facing;
	real_vector2d throttle;
	real primary_trigger;
	short desired_weapon_index;
	short desired_grenade_index;
	short desired_zoom_level;
	short pad;
};

struct network_player
{
	wchar_t name[12];
	short primary_color_index;
	short icon_index;
	char machine_index;
	char controller_index;
	char team_index;
	char player_list_index;
};

struct player_datum
{
	short identifier;
	short local_player_index;
	wchar_t name[12];
	long squad_index;
	long team_index;
	long action_object_index;
	short action_result;
	short action_seat_index;
	long respawn_timer;
	long respawn_penalty;
	long unit_index;
	long dead_unit_index;
	short cluster_index;
	boolean swapped_weapons;
	byte pad0;
	long aim_assist_unit_index;
	long aim_assist_timestamp;
	struct network_player network_player_data;
	short powerup_durations[NUMBER_OF_PLAYER_POWERUPS];
	real speed_multiplier;
	long unknown70;
	long unknown74;
	long unknown78;
	long unknown7c;
	byte unknown80[4];
	long death_time;
	long unknown88;
	struct game_statistics statistics;
	long telefrag_timeout;
	long quit_out_of_game_time;
	boolean is_blocking_teleporter;
	boolean quit_out_of_game;
	struct player_action action_input;
};

struct players_globals
{
	long unknown0;
	long local_players[MAXIMUM_LOCAL_PLAYERS];
	long dead_units[MAXIMUM_LOCAL_PLAYERS];
	short local_player_count;
	short double_speed_ticks;
	boolean all_dead;
	boolean input_disabled;
	short pending_teleport_starting_location_index;
	short respawn_failure;
	boolean respawn_failed;
	byte pad2F;
	unsigned long combined_pvs[16];
	unsigned long combined_pvs_local[16];
};

typedef char players_globals_local_player_count_offset_assert[
	offsetof(struct players_globals, local_player_count) == 0x24 ? 1 : -1];
typedef char players_globals_respawn_failure_offset_assert[
	offsetof(struct players_globals, respawn_failure) == 0x2C ? 1 : -1];
typedef char players_globals_size_assert[
	sizeof(struct players_globals) == 0xB0 ? 1 : -1];

typedef char player_datum_team_index_offset_assert[
	offsetof(struct player_datum, team_index) == 0x20 ? 1 : -1];
typedef char player_datum_statistics_offset_assert[
	offsetof(struct player_datum, statistics) == 0x8C ? 1 : -1];

/* ---------- prototypes/PLAYER_CONTROL.C */

void player_control_unzoom(long unit_index);

/* ---------- prototypes/PLAYERS.C */

long local_player_get_player_index(
	short local_player_index);

short local_player_count(
	void);

short players_get_respawn_failure(
	void);

long player_index_from_unit_index(long unit_index);

unsigned long const *players_get_combined_pvs_local(void);
unsigned long const *players_get_combined_pvs(void);

void player_control_fix_for_loaded_game_state(void);

/* ---------- prototypes/PLAYER_QUEUES_NEW.C */

void update_queues_reset_and_fill_with_lies(void);

/* ---------- globals */

extern struct data_array *player_data;
extern struct players_globals *players_globals;

/* ---------- public code */

#endif // __PLAYERS_H
