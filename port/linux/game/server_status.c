/*
SERVER_STATUS.C

A dedicated server's status (server_director.h; docs/dedicated_server.md,
"The status"): the server, its game and its players as JSON, for the API,
the remote console's status events, the status file, sv_status --json and
the master servers. Addresses and hardware ids are written only for the
owner, or with server.log_addresses; the invite only for admins.
*/

#include "cseries.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_server_manager.h"
#include "networking/network_server_manager_internal.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "tag_files/tag_files.h"
#include "units/units.h"
#include "network_chat.h"
#include "network_distributed.h"
#include "server_director.h"
#include "server_gametypes.h"

#include "../src/p2p.h" /* port: port/linux/src/p2p.c */
#include "../src/server_console.h" /* port: port/linux/src/server_console.c */
#include "../src/server_events.h" /* port: port/linux/src/server_events.c */
#include "../src/server_http.h" /* port: port/linux/src/server_http.c */
#include "../src/server_json.h" /* port: port/linux/src/server_json.c */
#include "../src/server_master.h" /* port: port/linux/src/server_master.c */
#include "../src/posix_server.h" /* port: port/linux/src/posix_server.c */
#include "../src/port_config.h" /* port: port/linux/src/port_config.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HALO_SERVER_BUILD

/* server_director.c's */
long long server_director_joined_time(long slot);
/* port/linux/src/updater.c's */
long updater_server_available(void);

enum
{
	MAXIMUM_SLOTS = HALO_PORT_MAXIMUM_NETWORK_PLAYERS,
};

static char const *const server_state_names[NUMBER_OF_SERVER_STATES] =
	{ "waiting", "hosting", "lobby", "loading", "playing", "postgame", "stopping" };

/* the scores last seen (server_status_scores_changed) */
static long server_status_last_scores[MAXIMUM_SLOTS + 2];

char const *server_status_state_name(
	short state)
{
	return state >= 0 && state < NUMBER_OF_SERVER_STATES ? server_state_names[state] : "waiting";
}

static struct network_game *server_game(
	void)
{
	struct network_game_server *network_server = global_network_game_server_get();

	return network_server ? network_game_server_get_game(network_server) : NULL;
}

boolean server_status_player_name(
	long slot,
	char *name,
	long size)
{
	struct network_game *game = server_game();
	long index;

	if (!game || slot < 0 || slot >= MAXIMUM_SLOTS || !network_player_is_valid(&game->players[slot]))
	{
		if (size > 0)
			name[0] = 0;
		return FALSE;
	}
	for (index = 0; index < (long)NUMBEROF(game->players[slot].name) && game->players[slot].name[index] &&
		index < size - 1; index++)
	{
		name[index] = player_name_character_ascii(game->players[slot].name[index]);
	}
	name[index] = 0;
	return TRUE;
}

/* (the web's: server_http.c) whether the slot holds the player of that name
(the page's view may be old); the slot's player's name in current either
way (empty for none) */
int server_status_player_matches(
	long slot,
	char const *name,
	char *current,
	long size)
{
	if (!server_status_player_name(slot, current, size))
		return FALSE;
	return !strcmp(current, name);
}

boolean server_status_team_scores(
	long *red,
	long *blue)
{
	struct network_game *game = server_game();

	if (!game || !game->variant.universal_variant.teams || !game_engine_running())
		return FALSE;
	*red = game_engine_get_team_score(0);
	*blue = game_engine_get_team_score(1);
	return TRUE;
}

/* the player in the running game of the network game's slot, NULL if none */
static struct player_datum *server_status_datum(
	long slot)
{
	struct network_game *game = server_game();

	if (!game || !game_engine_running() || !network_player_is_valid(&game->players[slot]))
		return NULL;
	return distributed_player(game->players[slot].player_list_index);
}

static long server_status_score(
	long slot)
{
	struct network_game *game = server_game();

	if (!server_status_datum(slot) || !game_engine || !game_engine->get_player_score)
		return 0;
	return game_engine->get_player_score(DATUM_INDEX_TO_ABSOLUTE_INDEX(game->players[slot].player_list_index),
		_get_score_individual);
}

boolean server_status_scores_changed(
	void)
{
	boolean changed = FALSE;
	long slot, red = 0, blue = 0;

	for (slot = 0; slot < MAXIMUM_SLOTS; slot++)
	{
		long score = server_status_score(slot);

		if (score != server_status_last_scores[slot])
		{
			server_status_last_scores[slot] = score;
			changed = TRUE;
		}
	}
	server_status_team_scores(&red, &blue);
	if (red != server_status_last_scores[MAXIMUM_SLOTS] || blue != server_status_last_scores[MAXIMUM_SLOTS + 1])
	{
		server_status_last_scores[MAXIMUM_SLOTS] = red;
		server_status_last_scores[MAXIMUM_SLOTS + 1] = blue;
		changed = TRUE;
	}
	return changed;
}

void server_status_player(
	struct server_json *json,
	long slot,
	int role)
{
	struct network_game *game = server_game();
	struct network_player const *player;
	struct player_datum *datum = server_status_datum(slot);
	char name[16];

	if (!server_status_player_name(slot, name, sizeof(name)))
		return;
	player = &game->players[slot];
	server_json_object_begin(json, NULL);
	server_json_integer(json, "slot", slot);
	server_json_string(json, "name", name);
	server_json_integer(json, "team", game->variant.universal_variant.teams ? (long long)player->team_index : -1);
	server_json_integer(json, "score", server_status_score(slot));
	server_json_integer(json, "kills", datum ? datum->statistics.kills[0] : 0);
	server_json_integer(json, "deaths", datum ? datum->statistics.deaths : 0);
	server_json_integer(json, "assists", datum ? datum->statistics.assists[0] : 0);
	{
		long ping = datum ? distributed_player_ping(player->player_list_index) : NONE;

		if (ping == NONE)
			server_json_null(json, "ping");
		else
			server_json_integer(json, "ping", ping);
	}
	server_json_integer(json, "machine", player->machine_index);
	server_json_integer(json, "joined", server_director_joined_time(slot) / 1000);
	{
		wchar_t wide[12];

		csmemcpy(wide, player->name, sizeof(wide));
		server_json_boolean(json, "muted", network_chat_host_name_muted(wide));
	}
	/* (its machine's build, as it told it: null from a build that does not) */
	{
		struct distributed_client_build build;

		if (distributed_machine_client_build(player->machine_index, &build))
		{
			char const *line = build.mods;

			server_json_object_begin(json, "client");
			server_json_integer(json, "build", strtol(build.number, NULL, 10));
			server_json_string(json, "flavor", build.flavor);
			server_json_string(json, "version", build.version);
			server_json_string(json, "repository", build.repository);
			server_json_array_begin(json, "mods");
			while (*line)
			{
				char const *end = strchr(line, '\n');
				int length = end ? (int)(end - line) : (int)strlen(line);

				if (length > 0)
					server_json_string_length(json, NULL, line, length);
				line += length + (end ? 1 : 0);
			}
			server_json_array_end(json);
			server_json_object_end(json);
		}
		else
		{
			server_json_null(json, "client");
		}
	}
	if (role >= _server_role_owner || config_boolean("server.log_addresses"))
	{
		unsigned long address = distributed_machine_real_address(player->machine_index);
		char text[32];
		char const *hardware_id = network_game_server_machine_hardware_id(player->machine_index);

		snprintf(text, sizeof(text), "%lu.%lu.%lu.%lu", (address >> 24) & 255, (address >> 16) & 255,
			(address >> 8) & 255, address & 255);
		server_json_string(json, "address", address ? text : "");
		server_json_string(json, "hwid", hardware_id ? hardware_id : "");
		/* (and its Discord account: a verified one's, a pass that checked,
		else what the machine says of itself) */
		{
			char discord_id[32], discord_name[48];
			boolean verified = network_distributed_machine_discord(player->machine_index, discord_id,
				sizeof(discord_id), discord_name, sizeof(discord_name));

			server_json_string(json, "discord_id", discord_id);
			server_json_string(json, "discord_name", discord_name);
			server_json_boolean(json, "discord_verified", verified);
		}
	}
	server_json_object_end(json);
}

void server_status_players(
	struct server_json *json,
	int role)
{
	struct network_game *game = server_game();
	long slot;

	server_json_array_begin(json, "players");
	for (slot = 0; game && slot < MAXIMUM_SLOTS; slot++)
	{
		if (network_player_is_valid(&game->players[slot]))
			server_status_player(json, slot, role);
	}
	server_json_array_end(json);
}

static void server_status_entry(
	struct server_json *json,
	char const *key,
	struct server_entry const *entry)
{
	long index;

	if (!entry)
	{
		server_json_null(json, key);
		return;
	}
	server_json_object_begin(json, key);
	server_json_string(json, "text", entry->text);
	server_json_string(json, "map", entry->map_name);
	server_json_string(json, "gametype", entry->gametype);
	server_json_object_begin(json, "rules");
	for (index = 0; index < entry->rule_count; index++)
		server_json_integer(json, entry->rules[index].key, entry->rules[index].value);
	server_json_object_end(json);
	if (entry->rejected[0])
		server_json_string(json, "rejected", entry->rejected);
	else
		server_json_null(json, "rejected");
	server_json_object_end(json);
}

/* the rotation: its entries, the one played and the next */
void server_status_rotation(
	struct server_json *json)
{
	struct server_rotation const *rotation = server_director_rotation();
	long index;

	server_json_integer(json, "index", server_director_index());
	server_json_array_begin(json, "entries");
	for (index = 0; index < rotation->count; index++)
		server_status_entry(json, NULL, &rotation->entries[index]);
	server_json_array_end(json);
	server_status_entry(json, "next", server_director_next());
}

void server_status_identity(
	struct server_json *json)
{
	char instance[64];
	char key_id[2 * SERVER_MASTER_KEY_ID_SIZE + 1];

	server_master_identity(instance, sizeof(instance), key_id, sizeof(key_id));
	server_json_integer(json, "pid", (long long)server_mode_process_id());
	server_json_integer(json, "started", (long long)server_mode_started());
	server_json_string(json, "instance", instance);
	server_json_string(json, "region", config_string("server.region"));
	server_json_integer(json, "tunnel_port", config_boolean("network.online") ? config_integer("network.tunnel_port") : 0);
	server_json_string(json, "publisher_key_id", key_id);
	server_json_integer(json, "web_port", config_integer("server.web_port"));
}

void server_status_write(
	struct server_json *json,
	int role)
{
	struct network_game *game = server_game();
	struct network_game_server *network_server = global_network_game_server_get();
	struct server_entry const *current = server_director_current();
	char variant[16] = "";
	long players = 0, slot, red, blue;
	long countdown = NONE;
	double longest, mean;

	for (slot = 0; game && slot < MAXIMUM_SLOTS; slot++)
		players += network_player_is_valid(&game->players[slot]) != 0;
	if (game)
	{
		long index;

		for (index = 0; index < 11 && game->variant.human_readable_game_description[index]; index++)
			variant[index] = player_name_character_ascii(game->variant.human_readable_game_description[index]);
		variant[index] = 0;
	}
	if (network_server && server_director_state() == _server_state_lobby)
		countdown = network_game_server_port_countdown_milliseconds(network_server);
	server_json_object_begin(json, NULL);
	server_json_boolean(json, "ok", TRUE);
	server_json_integer(json, "time", posix_server_unix_milliseconds());
	server_json_string(json, "name", config_string("server.name"));
	server_json_string(json, "state", server_status_state_name(server_director_state()));
	server_json_string(json, "build", server_mode_build());
	server_json_integer(json, "network_version", HALO_PORT_NETWORK_VERSION);
	server_json_integer(json, "uptime", (long long)server_mode_uptime());
	server_json_integer(json, "games_hosted", server_director_games_hosted());
	server_json_string(json, "map", current ? current->map_name : "");
	server_json_string(json, "level", current ? current->level : "");
	server_json_string(json, "gametype", current ? current->gametype : "");
	server_json_string(json, "variant", variant);
	server_json_boolean(json, "teams", game ? game->variant.universal_variant.teams : FALSE);
	server_json_integer(json, "players", players);
	server_json_integer(json, "max_players", game ? game->maximum_players : config_integer("server.max_players"));
	server_json_integer(json, "machines", network_server ?
		network_game_server_port_remote_machine_count(network_server, TRUE) : 0);
	server_json_integer(json, "countdown", countdown == NONE ? -1 : (countdown + 999) / 1000);
	server_json_integer(json, "game_seconds", (long long)server_director_game_seconds());
	if (server_status_team_scores(&red, &blue))
	{
		server_json_array_begin(json, "team_scores");
		server_json_integer(json, NULL, red);
		server_json_integer(json, NULL, blue);
		server_json_array_end(json);
	}
	else
	{
		server_json_null(json, "team_scores");
	}
	server_json_object_begin(json, "listing");
	server_json_boolean(json, "online", config_boolean("network.online"));
	server_json_boolean(json, "public", config_boolean("server.public"));
	server_json_boolean(json, "password", config_string("server.password")[0] != 0);
	{
		char invite[P2P_LISTING_INVITE_SIZE] = "";

		if (role >= _server_role_admin)
			p2p_invite_link(invite, sizeof(invite));
		server_json_string(json, "invite", invite);
	}
	server_json_object_end(json);
	server_json_object_begin(json, "rotation");
	{
		struct server_rotation const *rotation = server_director_rotation();

		server_json_integer(json, "index", server_director_index());
		server_json_integer(json, "count", rotation->count);
		server_status_entry(json, "next", server_director_next());
	}
	server_json_object_end(json);
	server_mode_frame_times(&longest, &mean);
	server_json_real(json, "frame_ms_max", longest);
	server_json_real(json, "frame_ms_avg", mean);
	server_json_real(json, "tick_ms_avg", mean);
	server_master_status(json);
	server_json_object_begin(json, "update");
	{
		/* (server.auto_update's last look: port/linux/src/updater.c) */
		long available = updater_server_available();
		char build[32] = "";

		if (available)
			snprintf(build, sizeof(build), "build-%ld", available);
		server_json_boolean(json, "available", available != 0);
		server_json_string(json, "build", build);
	}
	server_json_object_end(json);
	server_director_drain_json(json);
	server_status_identity(json);
	server_http_capabilities(json, "capabilities");
	server_json_object_end(json);
}

/* ---------- the maps and the gametypes */

/* the multiplayer maps' names as the game shows them (the Xbox's, and Halo
PC's own); another map's is its file's */
static struct
{
	char const *name;
	char const *title;
} const server_map_titles[] =
{
	{ "beavercreek", "Battle Creek" },
	{ "bloodgulch", "Blood Gulch" },
	{ "boardingaction", "Boarding Action" },
	{ "carousel", "Derelict" },
	{ "chillout", "Chill Out" },
	{ "damnation", "Damnation" },
	{ "dangercanyon", "Danger Canyon" },
	{ "deathisland", "Death Island" },
	{ "gephyrophobia", "Gephyrophobia" },
	{ "hangemhigh", "Hang 'Em High" },
	{ "icefields", "Ice Fields" },
	{ "infinity", "Infinity" },
	{ "longest", "Longest" },
	{ "prisoner", "Prisoner" },
	{ "putput", "Chiron TL-34" },
	{ "ratrace", "Rat Race" },
	{ "sidewinder", "Sidewinder" },
	{ "timberland", "Timberland" },
	{ "wizard", "Wizard" },
};

static void server_status_map(
	void *context,
	char const *name,
	char const *level,
	char const *kind)
{
	struct server_json *json = context;
	char const *title = name;
	long index;

	/* (a Custom Edition map of a stock map's name is its own) */
	for (index = 0; !strcmp(kind, "xbox") && index < (long)NUMBEROF(server_map_titles); index++)
	{
		if (!strcmp(name, server_map_titles[index].name))
			title = server_map_titles[index].title;
	}
	server_json_object_begin(json, NULL);
	server_json_string(json, "name", name);
	server_json_string(json, "level", level);
	server_json_string(json, "kind", kind);
	server_json_string(json, "title", title);
	server_json_object_end(json);
}

void server_status_maps(
	struct server_json *json)
{
	server_json_array_begin(json, "maps");
	server_maps_each(server_status_map, json);
	server_json_array_end(json);
}

void server_status_gametypes(
	struct server_json *json)
{
	char const *name;
	long index;

	server_json_array_begin(json, "builtin");
	for (index = 0; (name = server_gametype_builtin_name(index)) != NULL; index++)
		server_json_string(json, NULL, name);
	server_json_array_end(json);
	server_json_object_begin(json, "schema");
	server_json_array_begin(json, "fields");
	for (index = 0; index < server_rule_definition_count(); index++)
	{
		struct server_rule_definition const *rule = &server_rule_definitions[index];

		server_json_object_begin(json, NULL);
		server_json_string(json, "key", rule->key);
		server_json_string(json, "alias", rule->alias);
		server_json_string(json, "table", rule->table);
		server_json_string(json, "engine", rule->engines == _server_engine_all ? "all" : rule->table);
		server_json_string(json, "type", rule->type == _server_rule_integer ? "integer" :
			rule->type == _server_rule_boolean ? "boolean" : "choice");
		if (rule->type == _server_rule_integer)
		{
			server_json_integer(json, "min", rule->minimum);
			server_json_integer(json, "max", rule->maximum);
			server_json_null(json, "choices");
		}
		else
		{
			long choice;

			server_json_null(json, "min");
			server_json_null(json, "max");
			if (rule->type == _server_rule_choice)
			{
				server_json_array_begin(json, "choices");
				for (choice = 0; rule->choices[choice]; choice++)
					server_json_string(json, NULL, rule->choices[choice]);
				server_json_array_end(json);
			}
			else
			{
				server_json_null(json, "choices");
			}
		}
		server_json_null(json, "default");
		server_json_string(json, "description", rule->description);
		server_json_object_end(json);
	}
	server_json_array_end(json);
	server_json_object_end(json);
}

/* ---------- the game's events (server_director.h) */

/* the slot of the network game's player whose player is at the absolute
index; NONE if none */
static long server_status_slot_of(
	long absolute_index)
{
	struct network_game *game = server_game();
	long slot;

	for (slot = 0; game && absolute_index != NONE && slot < MAXIMUM_SLOTS; slot++)
	{
		if (network_player_is_valid(&game->players[slot]) &&
			DATUM_INDEX_TO_ABSOLUTE_INDEX(game->players[slot].player_list_index) == absolute_index)
		{
			return slot;
		}
	}
	return NONE;
}

/* the tag name of what killed: a unit's weapon in hand, else the object */
static char const *server_status_killing_tag(
	long object_index)
{
	struct object_datum *object = object_index != NONE ? object_try_and_get(object_index) : NULL;

	if (!object)
		return NULL;
	if (TEST_FLAG(_object_mask_unit, object->object.type))
	{
		struct unit_datum *unit = unit_get(object_index);
		long weapon_index = unit->unit.current_weapon_index != NONE ?
			unit_inventory_get_weapon(object_index, unit->unit.current_weapon_index) : NONE;
		struct object_datum *weapon = weapon_index != NONE ? object_try_and_get(weapon_index) : NULL;

		return weapon ? tag_get_name(weapon->definition_index) : NULL;
	}
	return tag_get_name(object->definition_index);
}

void server_event_player_killed(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire)
{
	struct server_json json;
	char const *weapon;

	if (!halo_server_mode() || dead_player_index == NONE)
		return;
	weapon = server_status_killing_tag(killing_object_index);
	server_json_begin(&json);
	server_json_object_begin(&json, NULL);
	server_json_string(&json, "event", "kill");
	server_json_integer(&json, "killer", killing_player_index != NONE ?
		server_status_slot_of(DATUM_INDEX_TO_ABSOLUTE_INDEX(killing_player_index)) : NONE);
	server_json_integer(&json, "victim", server_status_slot_of(DATUM_INDEX_TO_ABSOLUTE_INDEX(dead_player_index)));
	if (weapon)
		server_json_string(&json, "weapon", weapon);
	else
		server_json_null(&json, "weapon");
	server_json_boolean(&json, "friendly_fire", friendly_fire);
	server_json_object_end(&json);
	server_event_add("game", server_json_text(&json));
	server_json_free(&json);
}

void server_event_chat(
	short player_index,
	short channel,
	char const *text)
{
	static char const *const channel_names[] = { "everyone", "team", "vehicle" };
	struct server_json json;
	long slot;
	char name[16];

	if (!halo_server_mode())
		return;
	slot = server_status_slot_of(player_index);
	server_status_player_name(slot, name, sizeof(name));
	server_json_begin(&json);
	server_json_object_begin(&json, NULL);
	server_json_string(&json, "event", "chat");
	server_json_string(&json, "channel", channel >= 0 && channel < (short)NUMBEROF(channel_names) ?
		channel_names[channel] : "everyone");
	server_json_integer(&json, "slot", slot);
	server_json_string(&json, "name", name);
	server_json_string(&json, "text", text);
	server_json_object_end(&json);
	server_event_add("chat", server_json_text(&json));
	server_json_free(&json);
}

void server_event_speed_hack(
	long machine_index,
	char const *names,
	real rate,
	long ahead_ticks,
	char const *action)
{
	struct server_json json;

	if (!halo_server_mode())
		return;
	server_json_begin(&json);
	server_json_object_begin(&json, NULL);
	server_json_string(&json, "event", "speed_hack");
	server_json_integer(&json, "machine", machine_index);
	server_json_string(&json, "names", names);
	server_json_real(&json, "rate", rate);
	server_json_integer(&json, "ahead_ticks", ahead_ticks);
	server_json_string(&json, "action", action);
	server_json_object_end(&json);
	server_event_add("moderation", server_json_text(&json));
	server_json_free(&json);
	/* (one dropped is kept out, its line in bans.txt: its players banned) */
	if (!strcmp(action, "kicked"))
		server_director_player_removed(machine_index, "banned");
}

void server_event_map_share(
	long machine_index,
	char const *map,
	char const *state,
	unsigned long done,
	unsigned long total)
{
	struct server_json json;

	if (!halo_server_mode())
		return;
	server_json_begin(&json);
	server_json_object_begin(&json, NULL);
	server_json_string(&json, "event", "map_share");
	server_json_integer(&json, "machine", machine_index);
	server_json_string(&json, "map", map);
	server_json_string(&json, "state", state);
	server_json_integer(&json, "progress", total ? (long long)((double)done * 100.0 / (double)total) : 0);
	server_json_object_end(&json);
	server_event_add("player", server_json_text(&json));
	server_json_free(&json);
}

#else

void server_event_player_killed(long killing_player_index, long killing_object_index, long dead_player_index,
	boolean friendly_fire)
{
}

void server_event_chat(short player_index, short channel, char const *text)
{
}

void server_event_speed_hack(long machine_index, char const *names, real rate, long ahead_ticks, char const *action)
{
}

void server_event_map_share(long machine_index, char const *map, char const *state, unsigned long done,
	unsigned long total)
{
}

#endif
