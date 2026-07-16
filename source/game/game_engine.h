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
};

/* ---------- macros */

/* ---------- structures */

struct game_variant
{
	short variant_index;
	byte unused0[0x1A];
	boolean has_teams;
	byte unused1[3];
	unsigned long flags;
	byte unused2[0x44];
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
	void (*unknown34[21])(void);
};

typedef char verify_game_engine_size[sizeof(struct game_engine) == 0x88 ? 1 : -1];

/* ---------- prototypes/GAME_ENGINE.C */

void game_engine_playlist_initialize(
	void);

boolean game_engine_running(void);

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

boolean game_engine_infinite_grenades(long player_index);

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
