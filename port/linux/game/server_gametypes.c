/*
SERVER_GAMETYPES.C

A dedicated server's gametypes (server_gametypes.h; docs/dedicated_server.md):
the variant of a built-in gametype by its name (the Xbox's default gametypes'
builders, game_engine_playlist.h), of OpenCE's (game_modes.c), of a gametype
file (gametypes/<name>.toml: its base, then its rules) or of a gametype a
player's game saved (gametypes/<name>.gametype), with a rotation entry's own
rules over it. Each rule is checked to be of the variant's game engine: a
bad gametype never reaches the wire, where the clients play by it.
*/

#include "cseries.h"
#include "game/game_engine.h"
#include "game/game_engine_playlist.h"
#include "saved games/playlist_profile.h"
#include "saved games/saved_game_files.h"
#include "text/unicode.h"
#include "game_modes.h"
#include "server_gametypes.h"

#include "../src/server_mode.h" /* port: port/linux/src/server_mode.c */
#include "../src/server_toml.h" /* port: port/linux/src/server_toml.c */
#include "../src/server_json.h" /* port: port/linux/src/server_json.c */
#include "../src/posix.h" /* port: port/linux/src/posix_files.c */

#include <stdio.h>
#include <string.h>

#if HALO_SERVER_BUILD

/* the built-in gametypes, by name, and what the lobby and the server
browser call them (11 characters) */
static struct
{
	char const *name;
	struct game_variant *(*build)(struct game_variant *variant);
	wchar_t const *shown;
} const server_builtin_gametypes[] =
{
	{ "slayer", build_game_variant_slayer, L"Slayer" },
	{ "team_slayer", build_game_variant_team_slayer, L"Team Slayer" },
	{ "slayer_pro", build_game_variant_slayer_pro, L"Slayer Pro" },
	{ "elimination", build_game_variant_elimination, L"Elimination" },
	{ "phantoms", build_game_variant_phantoms, L"Phantoms" },
	{ "endurance", build_game_variant_endurance, L"Endurance" },
	{ "rockets", build_game_variant_rockets, L"Rockets" },
	{ "snipers", build_game_variant_snipers, L"Snipers" },
	{ "stalker", build_game_variant_stalker, L"Stalker" },
	{ "ctf", build_game_variant_ctf, L"CTF" },
	{ "ironctf", build_game_variant_iron_ctf, L"Iron CTF" },
	{ "ctf_pro", build_game_variant_ctf_pro, L"CTF Pro" },
	{ "invasion", build_game_variant_invasion, L"Invasion" },
	{ "oddball", build_game_variant_oddball, L"Oddball" },
	{ "team_oddball", build_game_variant_team_oddball, L"Team Ball" },
	{ "reverse_tag", build_game_variant_reverse_tag, L"Reverse Tag" },
	{ "accumulation", build_game_variant_accumulation, L"Accumulate" },
	{ "juggernaut", build_game_variant_juggernaut, L"Juggernaut" },
	{ "king", build_game_variant_king, L"King" },
	{ "team_king", build_game_variant_team_king, L"Team King" },
	{ "king_pro", build_game_variant_king_pro, L"King Pro" },
	{ "crazy_king", build_game_variant_crazy_king, L"Crazy King" },
	{ "race", build_game_variant_race, L"Race" },
	{ "team_race", build_game_variant_team_race, L"Team Race" },
	{ "rally", build_game_variant_rally, L"Rally" },
	{ "team_rally", build_game_variant_team_rally, L"Team Rally" },
};

/* Halo PC's dedicated server's names for them */
static struct
{
	char const *alias;
	char const *name;
} const server_gametype_aliases[] =
{
	{ "ffa", "slayer" },
	{ "ffa_slayer", "slayer" },
	{ "teamslayer", "team_slayer" },
	{ "tslayer", "team_slayer" },
	{ "koth", "king" },
	{ "team_koth", "team_king" },
	{ "iron_ctf", "ironctf" },
	{ "teamrace", "team_race" },
	{ "teamoddball", "team_oddball" },
};

static char const *const server_opence_gametypes[] =
{
	"swat", "infection", "gun_game", "hardcore", "hardcore_ctf", "hardcore_oddball", "hardcore_king",
};

char const *server_gametype_builtin_name(
	long index)
{
	if (index >= 0 && index < (long)NUMBEROF(server_builtin_gametypes))
		return server_builtin_gametypes[index].name;
	index -= (long)NUMBEROF(server_builtin_gametypes);
	if (index >= 0 && index < (long)NUMBEROF(server_opence_gametypes))
		return server_opence_gametypes[index];
	return NULL;
}

char const *server_gametype_engine_name(
	long game_engine_index)
{
	switch (game_engine_index)
	{
	case game_engine_ctf: return "ctf";
	case game_engine_slayer: return "slayer";
	case game_engine_oddball: return "oddball";
	case game_engine_king: return "king";
	case game_engine_race: return "race";
	default: return "none";
	}
}

/* a built-in's or OpenCE's gametype by its name (or alias): TRUE */
static boolean server_gametype_base(
	char const *name,
	struct game_variant *variant,
	struct game_variant_options *options)
{
	struct game_variant temporary;
	long index;

	for (index = 0; index < (long)NUMBEROF(server_gametype_aliases); index++)
	{
		if (!strcmp(name, server_gametype_aliases[index].alias))
			name = server_gametype_aliases[index].name;
	}
	for (index = 0; index < (long)NUMBEROF(server_builtin_gametypes); index++)
	{
		if (!strcmp(name, server_builtin_gametypes[index].name))
		{
			*variant = *server_builtin_gametypes[index].build(&temporary);
			game_variant_options_default(variant, options);
			if (!variant->human_readable_game_description[0])
			{
				ustrncpy(variant->human_readable_game_description, server_builtin_gametypes[index].shown,
					NUMBEROF(variant->human_readable_game_description) - 1);
			}
			return TRUE;
		}
	}
	return game_modes_gametype_by_name(name, variant, options);
}

static void server_gametype_flag(
	struct game_variant *variant,
	long bit,
	long value)
{
	SET_FLAG(variant->universal_variant.flags, bit, value != 0);
}

/* a rule over the variant and options (its engine already checked) */
static void server_gametype_rule(
	struct server_rule const *rule,
	struct game_variant *variant,
	struct game_variant_options *options,
	boolean *team_vehicles_given,
	boolean *vehicle_counts_given)
{
	char const *key = rule->key;
	long value = rule->value;
	struct universal_variant *universal = &variant->universal_variant;
	static char const *const vehicles[NUMBER_OF_VARIANT_VEHICLES] =
		{ "warthogs", "ghosts", "scorpions", "rocket_warthogs", "banshees", "gun_turrets" };
	long index;

	if (!strcmp(key, "score_to_win")) universal->score_to_win = value;
	else if (!strcmp(key, "lives")) universal->lives = value;
	else if (!strcmp(key, "respawn_time")) universal->respawn_time = value * TICKS_PER_SECOND;
	else if (!strcmp(key, "respawn_time_growth")) universal->respawn_time_growth = value * TICKS_PER_SECOND;
	else if (!strcmp(key, "suicide_penalty")) universal->suicide_penalty = value * TICKS_PER_SECOND;
	else if (!strcmp(key, "health")) universal->health = (real)value / 100.0f;
	else if (!strcmp(key, "teams")) universal->teams = (boolean)(value != 0);
	else if (!strcmp(key, "odd_man_out")) universal->odd_man_out = (boolean)(value != 0);
	else if (!strcmp(key, "radar")) server_gametype_flag(variant, _game_variant_draw_object_in_motion_sensor_bit, value);
	else if (!strcmp(key, "friend_indicators")) server_gametype_flag(variant, _game_variant_allow_friendly_navpoints_bit, value);
	else if (!strcmp(key, "infinite_grenades")) server_gametype_flag(variant, _game_variant_infinite_grenades_bit, value);
	else if (!strcmp(key, "no_shields")) server_gametype_flag(variant, _game_variant_no_shields_bit, value);
	else if (!strcmp(key, "invisible")) server_gametype_flag(variant, _game_variant_always_invisible_bit, value);
	else if (!strcmp(key, "generic_equipment")) server_gametype_flag(variant, _game_variant_generic_starting_equipment_bit, value);
	else if (!strcmp(key, "weapon_set")) universal->weapon_set = value;
	else if (!strcmp(key, "vehicle_set")) universal->vehicle_set = value;
	else if (!strcmp(key, "goal_radar")) universal->goal_radar = value;
	else if (!strcmp(key, "time_limit")) options->time_limit = (short)value;
	else if (!strcmp(key, "friendly_fire")) options->friendly_fire = (short)value;
	else if (!strcmp(key, "friendly_fire_penalty")) options->friendly_fire_penalty = (short)value;
	else if (!strcmp(key, "vehicle_respawn_time")) options->vehicle_respawn_time = (short)value;
	else if (!strcmp(key, "auto_team_balance")) options->auto_team_balance = (boolean)(value != 0);
	else if (!strcmp(key, "radar_players")) options->radar_players = (byte)value;
	else if (!strcmp(key, "loadout")) options->loadout = (byte)value;
	else if (!strcmp(key, "primary_weapon")) options->primary_weapon = (byte)value;
	else if (!strcmp(key, "secondary_weapon")) options->secondary_weapon = (byte)value;
	else if (!strcmp(key, "no_map_weapons")) options->no_map_weapons = (boolean)(value != 0);
	else if (!strcmp(key, "hardcore")) options->hardcore = (boolean)(value != 0);
	else if (!strcmp(key, "team_vehicles"))
	{
		byte set = value == 1 ? VARIANT_VEHICLE_SET_PC : value == 2 ? VARIANT_VEHICLE_SET_CUSTOM :
			(byte)universal->vehicle_set;

		options->vehicle_set[0] = options->vehicle_set[1] = set;
		*team_vehicles_given = TRUE;
	}
	else if (!strcmp(key, "assault")) variant->game_engine_variant.ctf.assault = (boolean)(value != 0);
	else if (!strcmp(key, "reset_on_capture")) variant->game_engine_variant.ctf.reset_on_capture = (boolean)(value != 0);
	else if (!strcmp(key, "flag_must_reset")) variant->game_engine_variant.ctf.flag_must_reset = (boolean)(value != 0);
	else if (!strcmp(key, "flag_at_home_to_score")) variant->game_engine_variant.ctf.flag_at_home_to_score = (boolean)(value != 0);
	else if (!strcmp(key, "single_flag_time")) variant->game_engine_variant.ctf.single_flag_time = value * TICKS_PER_SECOND;
	else if (!strcmp(key, "no_death_bonus")) variant->game_engine_variant.slayer.no_death_bonus = (boolean)(value != 0);
	else if (!strcmp(key, "no_kill_penalty")) variant->game_engine_variant.slayer.no_kill_penalty = (boolean)(value != 0);
	else if (!strcmp(key, "kill_in_order")) variant->game_engine_variant.slayer.kill_in_order = (boolean)(value != 0);
	else if (!strcmp(key, "moving_hill")) variant->game_engine_variant.king.moving_hill = (boolean)(value != 0);
	else if (!strcmp(key, "random_start")) variant->game_engine_variant.oddball.random_start = (boolean)(value != 0);
	else if (!strcmp(key, "ball_spawn_delay")) variant->game_engine_variant.oddball.ball_spawn_delay = (boolean)(value != 0);
	else if (!strcmp(key, "speed_with_ball")) variant->game_engine_variant.oddball.speed_with_ball = value;
	else if (!strcmp(key, "trait_with_ball")) variant->game_engine_variant.oddball.trait_with_ball = value;
	else if (!strcmp(key, "trait_without_ball")) variant->game_engine_variant.oddball.trait_without_ball = value;
	else if (!strcmp(key, "ball_type")) variant->game_engine_variant.oddball.oddball_ball_type = value;
	else if (!strcmp(key, "ball_spawn_count")) variant->game_engine_variant.oddball.ball_spawn_count = value;
	else if (!strcmp(key, "race_type")) variant->game_engine_variant.race.race_type = value;
	else if (!strcmp(key, "team_scoring")) variant->game_engine_variant.race.team_scoring = value;
	else
	{
		for (index = 0; index < NUMBER_OF_VARIANT_VEHICLES; index++)
		{
			if (!strcmp(key, vehicles[index]))
			{
				options->vehicle_counts[0][index] = options->vehicle_counts[1][index] = (byte)value;
				*vehicle_counts_given = TRUE;
			}
		}
	}
}

/* the rules over the variant, each of its engine: FALSE and why if one is
another's */
static boolean server_gametype_apply_rules(
	struct server_rule const *rules,
	long rule_count,
	struct game_variant *variant,
	struct game_variant_options *options,
	boolean *team_vehicles_given,
	boolean *vehicle_counts_given,
	char *reason,
	long reason_size)
{
	long engine_bit = variant->game_engine_index >= game_engine_ctf && variant->game_engine_index <= game_engine_race ?
		1L << (variant->game_engine_index - game_engine_ctf) : 0;
	long index;

	for (index = 0; index < rule_count; index++)
	{
		struct server_rule_definition const *definition = server_rule_find(rules[index].key);

		if (!definition)
		{
			snprintf(reason, (size_t)reason_size, "no rule is named %s", rules[index].key);
			return FALSE;
		}
		if (!(definition->engines & engine_bit))
		{
			snprintf(reason, (size_t)reason_size, "%s is a %s gametype's rule, and this one is %s", definition->key,
				definition->table, server_gametype_engine_name(variant->game_engine_index));
			return FALSE;
		}
		server_gametype_rule(&rules[index], variant, options, team_vehicles_given, vehicle_counts_given);
	}
	return TRUE;
}

boolean server_gametype_build(
	char const *gametype,
	struct server_rule const *rules,
	long rule_count,
	struct game_variant *variant,
	struct game_variant_options *options,
	char *reason,
	long reason_size)
{
	boolean team_vehicles_given = FALSE;
	boolean vehicle_counts_given = FALSE;
	struct universal_variant before;

	csmemset(variant, 0, sizeof(*variant));
	csmemset(options, 0, sizeof(*options));
	reason[0] = 0;
	if (!strncmp(gametype, "file=", 5))
	{
		/* a gametype a player's game saved, copied into the gametypes folder
		(the .toml files' own: beside config.toml first, server_toml.c's
		server_gametypes_folder) */
		byte block[SAVED_GAME_FILE_BLOCK_SIZE];
		int size;

		if (!server_gametype_file_name_valid(gametype + 5))
		{
			snprintf(reason, (size_t)reason_size, "\"%s\" is not a gametype file's name", gametype + 5);
			return FALSE;
		}
		size = server_gametype_saved_read(gametype + 5, block, sizeof(block));
		if (size < 0 || !playlist_profile_get_from_data_with_options(block, (unsigned long)size, variant, options))
		{
			snprintf(reason, (size_t)reason_size, "gametypes/%s.gametype is missing, or not a saved gametype",
				gametype + 5);
			return FALSE;
		}
		if (variant->game_engine_index < game_engine_ctf || variant->game_engine_index > game_engine_race)
		{
			snprintf(reason, (size_t)reason_size, "gametypes/%s.gametype has no game engine", gametype + 5);
			return FALSE;
		}
	}
	else if (!server_gametype_base(gametype, variant, options))
	{
		struct server_gametype_file file;
		int absent;

		if (!server_gametype_file_read(gametype, &file, &absent))
		{
			if (absent)
			{
				snprintf(reason, (size_t)reason_size,
					"no gametype is named %s (a built-in's, OpenCE's, or gametypes/%s.toml)", gametype, gametype);
			}
			else if (file.error_line > 0)
			{
				snprintf(reason, (size_t)reason_size, "gametypes/%s.toml line %d: %s", gametype, file.error_line,
					file.error);
			}
			else
			{
				snprintf(reason, (size_t)reason_size, "gametypes/%s.toml: %s", gametype, file.error);
			}
			return FALSE;
		}
		if (!server_gametype_base(file.base, variant, options))
		{
			snprintf(reason, (size_t)reason_size, "gametypes/%s.toml: its base, %s, is no built-in gametype", gametype,
				file.base);
			return FALSE;
		}
		if (file.engine_table[0] && strcmp(file.engine_table, server_gametype_engine_name(variant->game_engine_index)))
		{
			snprintf(reason, (size_t)reason_size, "gametypes/%s.toml: [%s] is not its base's game engine (%s)",
				gametype, file.engine_table, server_gametype_engine_name(variant->game_engine_index));
			return FALSE;
		}
		if (!server_gametype_apply_rules(file.rules, file.rule_count, variant, options, &team_vehicles_given,
			&vehicle_counts_given, reason, reason_size))
		{
			char why[SERVER_REASON_SIZE];

			snprintf(why, sizeof(why), "%s", reason);
			snprintf(reason, (size_t)reason_size, "gametypes/%s.toml: %s", gametype, why);
			return FALSE;
		}
		if (file.name[0])
		{
			long index;

			csmemset(variant->human_readable_game_description, 0, sizeof(variant->human_readable_game_description));
			for (index = 0; file.name[index] && index < (long)NUMBEROF(variant->human_readable_game_description) - 1;
				index++)
			{
				variant->human_readable_game_description[index] = (wchar_t)(unsigned char)file.name[index];
			}
		}
	}
	before = variant->universal_variant;
	if (!server_gametype_apply_rules(rules, rule_count, variant, options, &team_vehicles_given, &vehicle_counts_given,
		reason, reason_size))
	{
		return FALSE;
	}
	/* (each team's vehicles: the vehicle set's unless said, a count's
	making them custom) */
	if (vehicle_counts_given && !team_vehicles_given)
		options->vehicle_set[0] = options->vehicle_set[1] = VARIANT_VEHICLE_SET_CUSTOM;
	else if (!team_vehicles_given && before.vehicle_set != variant->universal_variant.vehicle_set)
		options->vehicle_set[0] = options->vehicle_set[1] = (byte)variant->universal_variant.vehicle_set;
	/* (CTF always has teams) */
	if (variant->game_engine_index == game_engine_ctf)
		variant->universal_variant.teams = TRUE;
	game_modes_options_sanitize(options);
	return TRUE;
}

/* (the web's: server_http.c) a gametype file's text checked as the
rotation would take it: TRUE, else why and its line */
int server_gametype_check_text(
	char const *text,
	int length,
	char *reason,
	long reason_size,
	int *line)
{
	struct server_gametype_file file;
	struct game_variant variant;
	struct game_variant_options options;
	boolean team_vehicles_given = FALSE, vehicle_counts_given = FALSE;

	*line = 0;
	if (!server_gametype_file_parse(text, length, &file))
	{
		snprintf(reason, (size_t)reason_size, "%s", file.error);
		*line = file.error_line;
		return FALSE;
	}
	if (!server_gametype_base(file.base, &variant, &options))
	{
		snprintf(reason, (size_t)reason_size, "base = \"%s\" is no built-in gametype", file.base);
		return FALSE;
	}
	if (file.engine_table[0] && strcmp(file.engine_table, server_gametype_engine_name(variant.game_engine_index)))
	{
		snprintf(reason, (size_t)reason_size, "[%s] is not its base's game engine (%s)", file.engine_table,
			server_gametype_engine_name(variant.game_engine_index));
		return FALSE;
	}
	return server_gametype_apply_rules(file.rules, file.rule_count, &variant, &options, &team_vehicles_given,
		&vehicle_counts_given, reason, reason_size);
}

/* (the web's) the gametype files there are: "files":[{name, base,
description}] */
void server_gametype_files_json(
	struct server_json *json)
{
	char folder[1024];
	char name[256];
	void *directory;

	server_gametypes_folder(folder, sizeof(folder));
	server_json_array_begin(json, "files");
	directory = posix_directory_open(folder);
	while (directory && posix_directory_next(directory, name, sizeof(name)))
	{
		size_t length = strlen(name);
		struct server_gametype_file file;
		char base_name[64];
		int absent;

		if (length < 6 || length - 5 >= sizeof(base_name) || strcmp(name + length - 5, ".toml"))
			continue;
		snprintf(base_name, sizeof(base_name), "%.*s", (int)(length - 5), name);
		server_json_object_begin(json, NULL);
		server_json_string(json, "name", base_name);
		if (server_gametype_file_read(base_name, &file, &absent))
		{
			server_json_string(json, "base", file.base);
			server_json_string(json, "description", file.name);
			server_json_null(json, "error");
		}
		else
		{
			server_json_string(json, "base", "");
			server_json_string(json, "description", "");
			server_json_string(json, "error", file.error);
		}
		server_json_object_end(json);
	}
	if (directory)
		posix_directory_close(directory);
	server_json_array_end(json);
}

void server_gametype_check_entry(
	struct server_entry *entry)
{
	struct game_variant variant;
	struct game_variant_options options;
	char reason[SERVER_REASON_SIZE];

	if (entry->rejected[0] || !strcmp(entry->gametype, "coop"))
		return;
	if (!server_gametype_build(entry->gametype, entry->rules, entry->rule_count, &variant, &options, reason,
		sizeof(reason)))
	{
		snprintf(entry->rejected, sizeof(entry->rejected), "%s", reason);
	}
}

#else

boolean server_gametype_build(char const *gametype, struct server_rule const *rules, long rule_count,
	struct game_variant *variant, struct game_variant_options *options, char *reason, long reason_size)
{
	return FALSE;
}

void server_gametype_check_entry(struct server_entry *entry)
{
}

char const *server_gametype_builtin_name(long index)
{
	return NULL;
}

char const *server_gametype_engine_name(long game_engine_index)
{
	return "none";
}

#endif
