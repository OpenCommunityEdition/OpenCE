/*
GAME_ENGINE.H

header included in hcex build.
*/

#ifndef __GAME_ENGINE_H
#define __GAME_ENGINE_H
#pragma once

/* ---------- constants */

enum
{
	_game_engine_allow_dynamic_lighting_bit = 0,
	_game_engine_allow_integrated_lights_bit,
	_game_engine_disable_infinite_grenades_bit,
};

enum
{
	_game_variant_draw_object_in_motion_sensor_bit = 0,
	_game_variant_infinite_grenades_bit = 2,
	_game_variant_no_shields_bit,
};

/* ---------- macros */

/* ---------- structures */

struct game_variant
{
	short variant_index;
	byte unused0[0x16];
	long engine_type;
	boolean has_teams;
	byte unused1[3];
	unsigned long flags;
	byte unused24[0x14];
	long maximum_lives;
	byte unused3C[0x2C];
};

typedef char verify_game_variant_size[sizeof(struct game_variant) == 0x68 ? 1 : -1];

struct game_engine
{
	char const *name;
	long type;
	void (*dispose)(void);
	boolean (*initialize_for_new_map)(void);
	void (*dispose_from_old_map)(void);
	void (*player_added)(long player_index);
	void (*game_ending)(void);
	void (*game_starting)(void);
	void (*statistics_append)(void);
	void (*handle_client_message)(void);
	void (*handle_server_message)(void);
	void (*unknown2C)(void);
	void (*post_rasterize_objects)(void);
	void (*unknown34[9])(void);
	boolean (*allow_pick_up)(
		long unit_index,
		long weapon_index);
	void (*player_damaged_player)(
		long damaging_player_index,
		long dead_player_index,
		long damage_type);
	void (*unknown60[3])(void);
	void (*prespawn_player_update)(
		long player_index);
	void (*unknown70[3])(void);
	boolean (*test_flag)(
		long flag);
	boolean (*test_trait)(
		long trait,
		long value);
	long (*did_player_win)(
		long player_index);
};

typedef char verify_game_engine_size[sizeof(struct game_engine) == 0x88 ? 1 : -1];

/* ---------- prototypes/GAME_ENGINE.C */

void game_engine_playlist_initialize(
	void);

boolean game_engine_running(
	void);

boolean game_engine_force_single_screen(
	void);

void game_engine_dispose(
	void);

void game_engine_dispose_from_old_map(
	void);

void game_engine_game_ending(
	void);

void game_engine_game_starting(
	void);

void game_engine_post_rasterize_objects(
	void);

boolean game_engine_can_score(
	void);

boolean game_engine_allow_pick_up(
	long unit_index,
	long weapon_index);

boolean game_engine_test_flag(
	long flag);

boolean game_engine_test_trait(
	long trait,
	long value);

void game_engine_prespawn_player_update(
	long player_index);

long game_engine_did_player_win(
	long player_index);

struct game_variant *game_engine_get_variant(
	void);

boolean game_engine_get_goal_in_use(
	short goal_index);

boolean game_engine_has_teams(
	void);

boolean game_engine_allow_pause(
	void);

boolean game_engine_allow_dynamic_lighting(
	long object_index);

boolean game_engine_infinite_grenades(
	long player_index);

boolean game_engine_has_shield(
	long player_index);

boolean game_engine_draw_object_in_motion_sensor(
	void);

boolean game_engine_should_end_game(
	void);

void game_engine_clear_goal_position(
	short goal_index);

long get_flag_definition_index(
	void);

long get_ball_definition_index(
	void);

void game_engine_override_map_name(
	char const *map_name);

void game_engine_override_game_variant(
	struct game_variant const *variant);

void game_engine_switch_to_postgame(
	void);

void game_engine_load_stage(
	char const *map_name);

void game_engine_end_game(
	void);

void game_engine_player_damaged_player(
	long damaging_player_index,
	long dead_player_index,
	long damage_type);

boolean game_engine_player_is_out_of_lives(
	long player_index);

boolean game_engine_hud_draw_messages(
	long player_index);

boolean game_engine_force_autopickup(
	long unit_index,
	long weapon_index);

void game_engine_play_multiplayer_sound(
	long sound_index);

long game_engine_remap_object_definition(long definition_index);

long game_engine_remap_vehicle(long vehicle_definition_index);
long game_engine_remap_equipment(long equipment_definition_index);
long game_engine_remap_weapon(long weapon_definition_index);


boolean game_engine_allow_integrated_lights(
	long object_index);

void game_engine_player_killed(
	long killing_player_index,
	long damage_type,
	long dead_player_index,
	boolean friendly_fire);

/* ---------- globals */

extern struct game_engine *game_engine;

/* ---------- public code */

#endif // __GAME_ENGINE_H
