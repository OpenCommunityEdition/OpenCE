/*
GAME_MODES.C

OpenCE's gametypes: SWAT, Infection and Gun Game, modes of Slayer, and
Hardcore, rules over any game engine's. Which a gametype plays is in its PC
options (game_engine.h's struct game_variant_options, from mode on), which a
saved gametype keeps (playlist_profile.c), the network game carries to every
machine (network_game_manager.h) and the game plays by
(game_variant_options_get).

- SWAT: no shields, and a shot to the head of any weapon kills (damage.c).
  Its built-in gametype starts everyone with a pistol and an assault rifle,
  with no weapons on the map, no grenades and no motion sensor.
- Infection (infection.c): on teams. Humans are red, zombies blue; the host
  chooses the first zombies after a countdown, a human killed joins them, and
  the zombies win when no human is left, the humans when the time runs out.
- Gun Game (gun_game.c): free for all. Each kill gives the killer the next
  weapon of a list; a kill with the last wins.
- Hardcore: no motion sensor, no aim assist, and a kill restores the
  killer's health and shields.

The built-in gametypes (game_modes_gametype) are written with the Xbox's
default ones, after them, and the Select Gametype screen lists them in a bank
of their own (menu_functions.c's OPENCE).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_engine_playlist.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game_modes.h"
#include "gun_game.h"
#include "infection.h"

#include <string.h>

/* game_engine.c's values of the variant's (its enums goal_radar,
game_engine_weapons and game_engine_vehicles) */
enum
{
	GOAL_RADAR_NAV_POINT = 1,
	WEAPON_SET_NO_GRENADES = 10,
	VEHICLE_SET_NONE = 1
};

/* Infection's options when a gametype has none (game_modes_options_sanitize) */
enum
{
	INFECTION_DEFAULT_INITIAL_ZOMBIES = 1,
	INFECTION_DEFAULT_ZOMBIE_SPEED = 150,
	INFECTION_DEFAULT_SKULL_DAMAGE = 200,
	INFECTION_DEFAULT_RESPAWN_TIME = 5,
	INFECTION_DEFAULT_COUNTDOWN = 10
};

enum
{
	_gametype_swat = 0,
	_gametype_infection,
	_gametype_gun_game,
	_gametype_hardcore_slayer,
	_gametype_hardcore_ctf,
	_gametype_hardcore_oddball,
	_gametype_hardcore_king,
	NUMBER_OF_GAMETYPES
};

static struct
{
	/* debug.network_test's */
	char const *name;
	/* the gametype lists' */
	wchar_t const *display_name;
	/* the variant's own (11 characters at most: the lobby's and the server
	browser's) */
	wchar_t const *variant_name;
} const game_modes_gametypes[NUMBER_OF_GAMETYPES] =
{
	{ "swat", L"SWAT", L"SWAT" },
	{ "infection", L"Infection", L"Infection" },
	{ "gun_game", L"Gun Game", L"Gun Game" },
	{ "hardcore", L"Hardcore Slayer", L"Hardcore" },
	{ "hardcore_ctf", L"Hardcore CTF", L"HC CTF" },
	{ "hardcore_oddball", L"Hardcore Oddball", L"HC Oddball" },
	{ "hardcore_king", L"Hardcore King", L"HC King" },
};

/* the kill being recorded is a melee's (game_modes_kill_by_melee) */
static boolean game_modes_melee_kill;

/* ---- the built-in gametypes */

short game_modes_gametype_count(
	void)
{
	return NUMBER_OF_GAMETYPES;
}

wchar_t const *game_modes_gametype_name(
	short index)
{
	return VALID_INDEX(index, NUMBER_OF_GAMETYPES) ? game_modes_gametypes[index].display_name : L"";
}

/* a variant without a motion sensor: no players on it, and a goal (a flag,
the ball, the hill) shown by its nav point instead */
static void game_modes_no_motion_sensor(
	struct game_variant *variant,
	struct game_variant_options *options)
{
	SET_FLAG(variant->universal_variant.flags, _game_variant_draw_object_in_motion_sensor_bit, FALSE);
	variant->universal_variant.goal_radar = GOAL_RADAR_NAV_POINT;
	options->radar_players = _radar_players_none;
}

void game_modes_gametype(
	short index,
	struct game_variant *variant,
	struct game_variant_options *options)
{
	struct game_variant temporary;

	switch (index)
	{
	case _gametype_hardcore_ctf:
		*variant = *build_game_variant_ctf(&temporary);
		break;
	case _gametype_hardcore_oddball:
		*variant = *build_game_variant_team_oddball(&temporary);
		break;
	case _gametype_hardcore_king:
		*variant = *build_game_variant_team_king(&temporary);
		break;
	case _gametype_gun_game:
		*variant = *build_game_variant_slayer(&temporary);
		break;
	default:
		*variant = *build_game_variant_team_slayer(&temporary);
		break;
	}
	game_variant_options_default(variant, options);

	switch (index)
	{
	case _gametype_swat:
		options->mode = _game_mode_swat;
		SET_FLAG(variant->universal_variant.flags, _game_variant_no_shields_bit, TRUE);
		variant->universal_variant.weapon_set = WEAPON_SET_NO_GRENADES;
		variant->universal_variant.vehicle_set = VEHICLE_SET_NONE;
		variant->universal_variant.respawn_time = 150;
		game_modes_no_motion_sensor(variant, options);
		options->loadout = _loadout_custom;
		options->primary_weapon = _loadout_weapon_pistol;
		options->secondary_weapon = _loadout_weapon_assault_rifle;
		options->no_map_weapons = TRUE;
		break;
	case _gametype_infection:
		options->mode = _game_mode_infection;
		variant->universal_variant.vehicle_set = VEHICLE_SET_NONE;
		variant->universal_variant.respawn_time = INFECTION_DEFAULT_RESPAWN_TIME * TICKS_PER_SECOND;
		variant->universal_variant.suicide_penalty = 0;
		options->time_limit = 5;
		options->friendly_fire = _friendly_fire_off;
		break;
	case _gametype_gun_game:
		options->mode = _game_mode_gun_game;
		variant->universal_variant.weapon_set = WEAPON_SET_NO_GRENADES;
		variant->universal_variant.vehicle_set = VEHICLE_SET_NONE;
		variant->universal_variant.respawn_time = 90;
		variant->universal_variant.suicide_penalty = 0;
		options->no_map_weapons = TRUE;
		break;
	default:
		options->hardcore = TRUE;
		options->friendly_fire = _friendly_fire_on;
		game_modes_no_motion_sensor(variant, options);
		break;
	}
	/* (each team's vehicles the variant's, as game_variant_options_default
	gives them) */
	options->vehicle_set[0] = options->vehicle_set[1] = (byte)variant->universal_variant.vehicle_set;
	game_modes_options_sanitize(options);
	csmemset(variant->human_readable_game_description, 0, sizeof(variant->human_readable_game_description));
	if (VALID_INDEX(index, NUMBER_OF_GAMETYPES))
	{
		ustrncpy(variant->human_readable_game_description, game_modes_gametypes[index].variant_name,
			NUMBEROF(variant->human_readable_game_description) - 1);
	}
}

boolean game_modes_gametype_by_name(
	char const *name,
	struct game_variant *variant,
	struct game_variant_options *options)
{
	short index;

	for (index = 0; index < NUMBER_OF_GAMETYPES; index++)
	{
		if (!strcmp(name, game_modes_gametypes[index].name))
		{
			game_modes_gametype(index, variant, options);
			return TRUE;
		}
	}
	return FALSE;
}

/* ---- a gametype's options */

void game_modes_options_default(
	struct game_variant_options *options)
{
	options->mode = _game_mode_none;
	options->hardcore = FALSE;
	options->infection_initial_zombies = INFECTION_DEFAULT_INITIAL_ZOMBIES;
	options->infection_zombie_speed = INFECTION_DEFAULT_ZOMBIE_SPEED;
	options->infection_skull_damage = INFECTION_DEFAULT_SKULL_DAMAGE;
	options->infection_respawn_time = INFECTION_DEFAULT_RESPAWN_TIME;
	options->infection_countdown = INFECTION_DEFAULT_COUNTDOWN;
	options->infection_zombie_shields = TRUE;
	options->gun_game_order = _gun_game_order_strongest_first;
	options->gun_game_melee_demotes = TRUE;
	csmemset(options->reserved, 0, sizeof(options->reserved));
}

/* a byte option within its range, else its default */
static byte game_modes_option(
	byte value,
	byte minimum,
	byte maximum,
	byte default_value)
{
	return value >= minimum && value <= maximum ? value : default_value;
}

void game_modes_options_sanitize(
	struct game_variant_options *options)
{
	if (options->mode >= NUMBER_OF_GAME_MODES)
		options->mode = _game_mode_none;
	options->hardcore = options->hardcore != FALSE;
	options->infection_initial_zombies = game_modes_option(options->infection_initial_zombies, 1, 16,
		INFECTION_DEFAULT_INITIAL_ZOMBIES);
	options->infection_zombie_speed = game_modes_option(options->infection_zombie_speed, 100, 200,
		INFECTION_DEFAULT_ZOMBIE_SPEED);
	options->infection_skull_damage = game_modes_option(options->infection_skull_damage, 100, 250,
		INFECTION_DEFAULT_SKULL_DAMAGE);
	options->infection_respawn_time = game_modes_option(options->infection_respawn_time, 1, 30,
		INFECTION_DEFAULT_RESPAWN_TIME);
	options->infection_countdown = game_modes_option(options->infection_countdown, 0, 30,
		INFECTION_DEFAULT_COUNTDOWN);
	options->infection_zombie_shields = options->infection_zombie_shields != FALSE;
	if (options->gun_game_order >= NUMBER_OF_GUN_GAME_ORDERS)
		options->gun_game_order = _gun_game_order_strongest_first;
	options->gun_game_melee_demotes = options->gun_game_melee_demotes != FALSE;
	options->force_team_spawns = options->force_team_spawns != FALSE;
	csmemset(options->reserved, 0, sizeof(options->reserved));
}

short game_modes_variant_mode(
	struct game_variant const *variant,
	struct game_variant_options const *options)
{
	if (!variant || !options || variant->game_engine_index != game_engine_slayer)
		return _game_mode_none;
	switch (options->mode)
	{
	case _game_mode_swat:
		return _game_mode_swat;
	case _game_mode_infection:
		return variant->universal_variant.teams ? _game_mode_infection : _game_mode_none;
	case _game_mode_gun_game:
		return !variant->universal_variant.teams ? _game_mode_gun_game : _game_mode_none;
	}
	return _game_mode_none;
}

byte game_modes_kind(
	struct game_variant const *variant,
	struct game_variant_options const *options)
{
	return (byte)(game_modes_variant_mode(variant, options) |
		(options && options->hardcore ? GAME_MODES_KIND_HARDCORE : 0));
}

wchar_t const *game_modes_kind_name(
	long game_engine_index,
	byte kind,
	boolean abbreviated)
{
	/* (each kind's name, and its name with Hardcore's, in full and short) */
	static wchar_t const *const mode_names[NUMBER_OF_GAME_MODES][2][2] =
	{
		{ { NULL, NULL }, { NULL, NULL } },
		{ { L"SWAT", L"SWAT" }, { L"HARDCORE SWAT", L"HC SWAT" } },
		{ { L"INFECTION", L"INFECTION" }, { L"HARDCORE INFECTION", L"HC INFECTION" } },
		{ { L"GUN GAME", L"GUN GAME" }, { L"HARDCORE GUN GAME", L"HC GUN GAME" } },
	};
	static wchar_t const *const hardcore_names[][2] =
	{
		{ NULL, NULL },
		{ L"HARDCORE CTF", L"HC CTF" },
		{ L"HARDCORE SLAYER", L"HC SLAYER" },
		{ L"HARDCORE ODDBALL", L"HC ODDBALL" },
		{ L"HARDCORE KING", L"HC KING" },
		{ L"HARDCORE RACE", L"HC RACE" },
	};
	boolean hardcore = (kind & GAME_MODES_KIND_HARDCORE) != 0;
	short mode = (short)(kind & ~GAME_MODES_KIND_HARDCORE);

	/* (another machine's: a mode of Slayer only, and one this build knows) */
	if (game_engine_index == game_engine_slayer && mode > _game_mode_none && mode < NUMBER_OF_GAME_MODES)
		return mode_names[mode][hardcore][abbreviated != FALSE];
	if (!hardcore || game_engine_index <= game_engine_none || game_engine_index >= (long)NUMBEROF(hardcore_names))
		return NULL;
	return hardcore_names[game_engine_index][abbreviated != FALSE];
}

wchar_t const *game_modes_variant_kind_name(
	struct game_variant const *variant,
	struct game_variant_options const *options)
{
	return variant && options ?
		game_modes_kind_name(variant->game_engine_index, game_modes_kind(variant, options), FALSE) : NULL;
}

boolean game_modes_variant_is_opence(
	struct game_variant const *variant,
	struct game_variant_options const *options)
{
	return options && (options->hardcore || game_modes_variant_mode(variant, options) != _game_mode_none);
}

/* ---- the game being played */

short game_modes_mode(
	void)
{
	if (!game_engine_running())
		return _game_mode_none;
	return game_modes_variant_mode(game_engine_get_variant(), game_variant_options_get());
}

boolean game_modes_hardcore(
	void)
{
	return game_engine_running() && game_variant_options_get()->hardcore;
}

struct game_engine *game_modes_engine(
	struct game_engine *engine)
{
	switch (game_modes_mode())
	{
	case _game_mode_swat:
		/* (SWAT is played without shields, whatever its variant's) */
		SET_FLAG(game_engine_get_variant()->universal_variant.flags, _game_variant_no_shields_bit, TRUE);
		break;
	case _game_mode_infection:
		return infection_engine_get(engine);
	case _game_mode_gun_game:
		return gun_game_engine_get(engine);
	}
	return engine;
}

void game_modes_postspawn(
	long player_index)
{
	switch (game_modes_mode())
	{
	case _game_mode_infection:
		infection_postspawn(player_index);
		break;
	case _game_mode_gun_game:
		gun_game_postspawn(player_index);
		break;
	}
}

void game_modes_player_killed(
	long killing_player_index,
	long dead_player_index,
	boolean friendly_fire)
{
	struct player_datum *killer;
	struct unit_datum *unit;

	/* Hardcore: the host restores the killer's health, and their shields
	unless an overshield has them past full */
	if (!game_modes_hardcore() || network_game_distributed_client() || friendly_fire ||
		killing_player_index == NONE || killing_player_index == dead_player_index)
	{
		return;
	}
	killer = player_try_and_get(killing_player_index);
	if (!killer || killer->unit_index == NONE)
		return;
	unit = unit_try_and_get(killer->unit_index);
	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;
	unit->object.body_vitality = 1.0f;
	if (unit->object.maximum_shield_vitality > 0.0f && unit->object.shield_vitality < 1.0f)
	{
		unit->object.shield_vitality = 1.0f;
		SET_FLAG(unit->object.damage_flags, _object_shield_depleted_bit, FALSE);
		SET_FLAG(unit->object.damage_flags, _object_shield_charging_bit, FALSE);
	}
}

void game_modes_kill_by_melee(
	boolean melee)
{
	game_modes_melee_kill = melee;
}

boolean game_modes_killed_by_melee(
	void)
{
	return game_modes_melee_kill;
}

boolean game_modes_headshot_kills(
	long object_index,
	boolean shot)
{
	struct unit_datum *unit;

	if (!shot || game_modes_mode() != _game_mode_swat)
		return FALSE;
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_biped);
	return unit && unit->unit.player_index != NONE;
}

boolean game_modes_motion_sensor_hidden(
	void)
{
	return game_modes_hardcore();
}

boolean game_modes_aim_assist_off(
	void)
{
	return game_modes_hardcore();
}
