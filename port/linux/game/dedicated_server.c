/*
DEDICATED_SERVER.C

The dedicated server (dedicated_server.h; port/linux/README.md, "Dedicated
server"): the game run as a host with no window, sound or player of its own
(server.dedicated, which port_config.c turns the window, the sound, the
self-updater, Discord and the telnet console off for), which other machines
join as they join any host: on the LAN, by its invite link, or from the server
browser. It hosts the game as a player's machine does, with the same netcode
and messages, so any client of this network version can join it. Called from
the main loop every frame (main.c), beside network_test_update.

- Hosting: as the automated tests' host does (network_test.c: the pregame
  screen's fast setup, then the map and gametype), but with no player: the
  server's own machine only hosts. Its lobby counts down by itself
  (network_server_manager.c) once sv_minplayers players are in, for
  sv_start_delay seconds; others join the game in progress. A game over, its
  scores are shown sv_postgame seconds, and the lobby takes the map cycle's
  next game (sv_mapcycle_add). A game nobody is left in ends after
  sv_end_empty seconds, and the server waits in its lobby, a few frames a
  second, for the next player.
- Commands: Halo PC's dedicated server's (haloceded) names where they mean
  the same: sv_name, sv_maxplayers, sv_password, sv_public, sv_mapcycle_add,
  sv_map, sv_kick, sv_ban, sv_players... ("help" lists them). From
  server.init_file at the start and from the standard input
  (port/linux/src/dedicated_console.c); never a script, a shell or a file a
  client names, and no remote console. The password is never shown or
  logged.
- Security: the connections an address makes are counted
  (DEDICATED_JOINS_PER_MINUTE), on top of what every host checks; bans are
  bans.txt's lines (network_distributed.c), which the server keeps across
  restarts.
*/

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_server_manager.h"
#include "networking/network_server_manager_internal.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "cache/cache_files.h"
#include "custom_edition_cache.h"
#include "dedicated_server.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/p2p.h" /* port: port/linux/src/p2p.c, p2p_lobby.c */
#include "../src/dedicated_console.h" /* port: port/linux/src/dedicated_console.c */

/* the platform layer's (port/linux/src/port_config.c) */
const char *config_string(char const *name);
int config_boolean(char const *name);
double config_real(char const *name);
void platform_log(char const *format, ...);
/* xbox_files.c's */
const char *platform_data_root(void);
/* main.c's */
short main_get_solo_level_from_name(char const *name);
extern boolean debug_no_drawing;

/* ---------- constants */

enum
{
	/* the map cycle's most games */
	MAXIMUM_CYCLE_ENTRIES = 64,
	MAP_NAME_SIZE = 32,
	GAMETYPE_NAME_SIZE = 24,
	LEVEL_PATH_SIZE = 128,
	COMMAND_SIZE = DEDICATED_CONSOLE_LINE_SIZE,
	MAXIMUM_ARGUMENTS = 8,
	/* sv_name's: the game's name (network_game's, 16 wide characters with
	its end) */
	SERVER_NAME_SIZE = 16,
	/* sv_password's (the server browser's listing seals its invite with it:
	p2p_lobby.c) */
	PASSWORD_SIZE = 64,
	/* the console's commands taken a frame (the file's all at once: they
	set the server up) */
	MAXIMUM_CONSOLE_COMMANDS_PER_FRAME = 4,
	/* connections an address may make a minute; its allowance comes back one
	each JOIN_REFILL_SECONDS */
	DEDICATED_JOINS_PER_MINUTE = 6,
	JOIN_REFILL_SECONDS = 60 / DEDICATED_JOINS_PER_MINUTE,
	MAXIMUM_JOIN_ADDRESSES = 128,
	/* sv_postgame's least: the scores' seconds when nobody is in the game */
	MINIMUM_POSTGAME_SECONDS = 3,
	/* a frame's least milliseconds while no other machine is connected:
	the game's frames are 30 a second (display.interpolation is off), these
	about 10 */
	IDLE_FRAME_MILLISECONDS = 100,
	/* bans.txt's lines the ban commands read */
	MAXIMUM_BAN_LINES = 1024,
	BAN_LINE_SIZE = 512,
};

/* the server's states (network_server_manager.c's) */
enum
{
	_server_pregame = 0,
	_server_ingame,
	_server_postgame,
};

#define BANS_FILE "d:\\bans.txt"

/* ---------- structures */

struct cycle_entry
{
	char map[MAP_NAME_SIZE];
	char gametype[GAMETYPE_NAME_SIZE];
};

struct join_address
{
	unsigned long address;
	long allowance;
	unsigned long refill_time;
};

/* ---------- globals */

static struct
{
	/* server.dedicated, read once (-1: not yet) */
	long active;
	boolean started;
	real uptime;
	real menu_seconds;
	unsigned long last_frame_time;
	/* hosting: the server made (fast setup), its map and gametype set in
	the lobby, and how long the lobby has been open */
	boolean hosting;
	boolean map_set;
	real lobby_seconds;
	real map_checked_seconds;
	/* the cycle and the game played of it; a game named by sv_map, played
	next (once) */
	struct cycle_entry cycle[MAXIMUM_CYCLE_ENTRIES];
	long cycle_count;
	long cycle_index;
	struct cycle_entry next_game;
	boolean has_next_game;
	struct cycle_entry playing;
	char level_path[LEVEL_PATH_SIZE];
	/* a game: seconds played, its scores shown, its end asked for (the time
	limit's, sv_end_game's), and with nobody in it; back in the lobby the
	cycle goes on (or, restart, plays it again) */
	real game_seconds;
	real postgame_seconds;
	real empty_seconds;
	boolean end_asked;
	boolean back_to_lobby;
	boolean restart;
	short last_state;
	/* the players seen, for the joined and left lines */
	char seen_names[HALO_PORT_MAXIMUM_NETWORK_PLAYERS][16];
	char invite[P2P_LISTING_INVITE_SIZE];
	/* settings (the commands') */
	char name[SERVER_NAME_SIZE];
	long maximum_players;
	boolean public_game;
	char password[PASSWORD_SIZE];
	long minimum_players;
	long start_delay;
	long postgame_seconds_setting;
	long end_empty_seconds;
	long time_limit_minutes;
	long score_limit;
	struct join_address joins[MAXIMUM_JOIN_ADDRESSES];
} dedicated =
{
	-1,
};

static void unquoted(char *destination, long size, char const *text);

/* ---------- output */

static void say(
	char const *format,
	...)
{
	char line[512];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	dedicated_console_print("server: %s", line);
}

/* ---------- addresses */

/* a game's address (host byte order) as the internet sees it: an internet
play peer's (100.64.0.0/10) its real one, as p2p.c knows it (0 if none) */
static unsigned long real_address(
	unsigned long address)
{
	if ((address & 0xFFC00000) == 0x64400000)
	{
		unsigned long network = (address >> 24) | ((address >> 8) & 0xFF00) | ((address << 8) & 0xFF0000) |
			(address << 24);
		unsigned long real = p2p_peer_endpoint_address(network);

		return (real >> 24) | ((real >> 8) & 0xFF00) | ((real << 8) & 0xFF0000) | (real << 24);
	}
	return address;
}

static void address_text(
	unsigned long address,
	char *text,
	long size)
{
	if (address)
		snprintf(text, size, "%lu.%lu.%lu.%lu", (address >> 24) & 255, (address >> 16) & 255, (address >> 8) & 255,
			address & 255);
	else
		snprintf(text, size, "unknown");
}

/* a.b.c.d (each 0 to 255, nothing else) as an address (host byte order) */
static boolean parse_address(
	char const *text,
	unsigned long *address)
{
	unsigned long parts[4];
	long index;

	for (index = 0; index < 4; index++)
	{
		long digits = 0;

		parts[index] = 0;
		while (*text >= '0' && *text <= '9' && digits < 3)
		{
			parts[index] = parts[index] * 10 + (unsigned long)(*text++ - '0');
			digits++;
		}
		if (!digits || parts[index] > 255)
			return FALSE;
		if (index < 3 && *text++ != '.')
			return FALSE;
	}
	if (*text)
		return FALSE;
	*address = parts[0] << 24 | parts[1] << 16 | parts[2] << 8 | parts[3];
	return TRUE;
}

/* ---------- names */

/* a map's name as the operator gives it: letters, digits, "_" and "-",
nothing that is a path */
static boolean map_name_valid(
	char const *name)
{
	long length = 0;

	for (; name[length]; length++)
	{
		char character = name[length];

		if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') || character == '_' || character == '-'))
		{
			return FALSE;
		}
	}
	return length > 0 && length < MAP_NAME_SIZE;
}

static void lowercase(
	char *text)
{
	for (; *text; text++)
		if (*text >= 'A' && *text <= 'Z')
			*text = (char)(*text - 'A' + 'a');
}

/* a gametype of the game's own (game_engine_get_variant_by_name), by its
name or Halo PC's, with a name for the lobby and the server browser (the
variant's, 11 characters); FALSE if none */
static boolean gametype_variant(
	char const *name,
	struct game_variant *variant)
{
	static struct
	{
		char const *name;
		char const *shown;
	} const shown_names[] =
	{
		{ "slayer", "Slayer" },
		{ "team_slayer", "Team Slayer" },
		{ "ctf", "CTF" },
		{ "ironctf", "Iron CTF" },
		{ "king", "King" },
		{ "team_king", "Team King" },
		{ "oddball", "Oddball" },
		{ "team_oddball", "Team Ball" },
		{ "race", "Race" },
		{ "team_race", "Team Race" },
		{ "rally", "Rally" },
		{ "elimination", "Elimination" },
		{ "stalker", "Stalker" },
		{ "accumulation", "Accumulate" },
	};
	static struct
	{
		char const *alias;
		char const *name;
	} const aliases[] =
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
	char lower[GAMETYPE_NAME_SIZE];
	long index;

	if (csstrlen(name) >= sizeof(lower))
		return FALSE;
	csstrcpy(lower, name);
	lowercase(lower);
	for (index = 0; index < (long)NUMBEROF(aliases); index++)
		if (!csstrcmp(lower, aliases[index].alias))
			csstrcpy(lower, aliases[index].name);
	game_engine_get_variant_by_name(variant, lower);
	if (!variant->game_engine_index)
		return FALSE;
	for (index = 0; index < (long)NUMBEROF(shown_names); index++)
	{
		if (!csstrcmp(lower, shown_names[index].name))
		{
			long character;

			for (character = 0; shown_names[index].shown[character] &&
				character < (long)NUMBEROF(variant->human_readable_game_description) - 1; character++)
			{
				variant->human_readable_game_description[character] = (wchar_t)shown_names[index].shown[character];
			}
			variant->human_readable_game_description[character] = 0;
		}
	}
	return TRUE;
}

/* the level of a multiplayer map the server can play, by its name: an Xbox
map of the maps folder (levels\test\<name>\<name>), else a Custom Edition
map of custom_maps (custom_maps\<name>, with game.custom_edition on); FALSE
(why said) if there is none */
static boolean map_level(
	char const *map,
	char *path,
	long size,
	char const *command)
{
	char file[256];
	char message[256] = "";
	HANDLE handle;

	if (!csstrcasecmp(map, "ui"))
	{
		say("%s: ui is the main menu's map", command);
		return FALSE;
	}
	snprintf(file, sizeof(file), "%s%s.map", cache_files_map_directory(), map);
	handle = CreateFileA(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		if (main_get_solo_level_from_name(map) != NONE)
		{
			say("%s: %s is a campaign level, not a multiplayer map", command, map);
			return FALSE;
		}
		snprintf(path, size, "levels\\test\\%s\\%s", map, map);
		return TRUE;
	}
	snprintf(path, size, "custom_maps\\%s", map);
	if (custom_edition_cache_present(path, 0, message, sizeof(message)))
	{
		if (custom_edition_cache_campaign(map))
		{
			say("%s: %s is a campaign map, not a multiplayer map", command, map);
			return FALSE;
		}
		return TRUE;
	}
	say("%s: there is no multiplayer map %s in the maps folder%s%s", command, map, message[0] ? ": " : "", message);
	return FALSE;
}

/* a player's name in ASCII */
static void player_name(
	struct network_player const *player,
	char *text,
	long size)
{
	long index;

	for (index = 0; index < (long)NUMBEROF(player->name) && player->name[index] && index < size - 1; index++)
		text[index] = player_name_character_ascii(player->name[index]);
	text[index] = 0;
}

/* ---------- the server's game */

static struct network_game *server_game(
	void)
{
	struct network_game_server *server = global_network_game_server_get();

	return server ? network_game_server_get_game(server) : NULL;
}

static short server_state(
	void)
{
	struct network_game_server *server = global_network_game_server_get();

	return server ? (short)network_game_server_get_state(server, NULL) : NONE;
}

static long player_count(
	void)
{
	struct network_game *game = server_game();
	long count = 0;
	long index;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		if (network_player_is_valid(&game->players[index]))
			count++;
	return count;
}

/* the other machines connected (joined, if joined), not the server's own */
static long machine_count(
	boolean joined)
{
	struct network_game_server *server = global_network_game_server_get();
	long count = 0;
	long index;

	for (index = 0; server && index < HALO_PORT_MAXIMUM_NETWORK_MACHINES; index++)
	{
		struct network_game_server_client_machine *machine = network_game_server_get_client_machine_at_index(server,
			index);

		if (machine && !network_game_server_client_machine_is_local(server, machine) &&
			(joined ? network_game_server_client_machine_is_joined_to_game(server, machine) :
				network_game_server_get_client_connection(machine) != NULL))
		{
			count++;
		}
	}
	return count;
}

/* the game the lobby plays next: sv_map's, else the cycle's */
static struct cycle_entry const *next_entry(
	void)
{
	static struct cycle_entry const fallback = { "bloodgulch", "slayer" };

	if (dedicated.has_next_game)
		return &dedicated.next_game;
	if (dedicated.cycle_count)
		return &dedicated.cycle[dedicated.cycle_index % dedicated.cycle_count];
	return &fallback;
}

/* the name and the most players, for this game and the next (sv_name,
sv_maxplayers) */
static void apply_settings(
	void)
{
	wchar_t name[SERVER_NAME_SIZE];
	long index;

	for (index = 0; index < SERVER_NAME_SIZE - 1 && dedicated.name[index]; index++)
		name[index] = (wchar_t)(unsigned char)dedicated.name[index];
	name[index] = 0;
	network_game_server_port_set_settings(name, dedicated.maximum_players);
}

/* sets the lobby's map and gametype (the server's lobby: pregame) */
static boolean apply_entry(
	struct cycle_entry const *entry)
{
	struct network_game_server *server = global_network_game_server_get();
	struct game_variant variant;

	if (!server || !gametype_variant(entry->gametype, &variant) ||
		!map_level(entry->map, dedicated.level_path, sizeof(dedicated.level_path), "the cycle"))
	{
		return FALSE;
	}
	network_game_server_change_map_name(server, dedicated.level_path);
	if (dedicated.score_limit > 0)
		variant.universal_variant.score_to_win = dedicated.score_limit;
	player_ui_set_game_variant(&variant);
	network_game_server_change_game_variant(server, &variant);
	dedicated.playing = *entry;
	say("the next game: %s on %s", entry->gametype, entry->map);
	return TRUE;
}

/* how long the scores are shown: sv_postgame, or its least (3 s) when no
machine is in the game to see them */
static real postgame_seconds_wanted(
	void)
{
	return machine_count(TRUE) > 0 ? (real)dedicated.postgame_seconds_setting : (real)MINIMUM_POSTGAME_SECONDS;
}

/* ends the game being played (sv_end_game, the time limit, nobody left) */
static boolean end_game(
	char const *why,
	boolean restart)
{
	if (server_state() != _server_ingame || !game_engine_running() || !game_engine_can_score())
		return FALSE;
	if (!dedicated.end_asked)
	{
		dedicated.end_asked = TRUE;
		dedicated.restart = restart;
		say("the game ends (%s)", why);
		game_engine_end_game();
	}
	return TRUE;
}

/* the players who came and went, said */
static void note_players(
	void)
{
	struct network_game *game = server_game();
	long index;

	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		char name[16] = "";

		if (game && network_player_is_valid(&game->players[index]))
			player_name(&game->players[index], name, sizeof(name));
		if (strcmp(name, dedicated.seen_names[index]))
		{
			if (dedicated.seen_names[index][0])
				say("player #%ld %s left", index, dedicated.seen_names[index]);
			if (name[0])
			{
				char address[32];

				address_text(real_address(network_game_server_machine_address(game->players[index].machine_index)),
					address, sizeof(address));
				say("player #%ld %s joined from %s", index, name, address);
			}
			csstrcpy(dedicated.seen_names[index], name);
		}
	}
}

/* the invite link, as internet play makes it (and anew: going private, a
new password) */
static void note_invite(
	void)
{
	char invite[P2P_LISTING_INVITE_SIZE];

	if (!p2p_invite_link(invite, sizeof(invite)))
		invite[0] = 0;
	if (strcmp(invite, dedicated.invite))
	{
		csstrcpy(dedicated.invite, invite);
		if (invite[0])
		{
			say("hosting; join with the invite %s%s", invite,
				dedicated.public_game ? ", or from the server browser" : "");
		}
	}
}

/* the listing as the commands left it: public or private, and its password
(a private game has none: its invite joins it) */
static void apply_listing(
	void)
{
	p2p_set_hosting_allowed(TRUE);
	p2p_set_hosting_public(dedicated.public_game);
	p2p_set_hosting_password(dedicated.public_game && dedicated.password[0] ? dedicated.password : NULL);
}

/* ---------- commands */

static void command_help(
	void)
{
	static char const *const lines[] =
	{
		"commands (server.init_file and this console; haloceded's names):",
		"  sv_name <name>                   the game's name, in the server browser (15 characters)",
		"  sv_maxplayers <2-128>            the most players (16 by default)",
		"  sv_password [password]           the server browser asks for it (none without one;",
		"                                   the invite link still joins)",
		"  sv_public <0|1>                  listed in the server browser, or joined by invite only",
		"  sv_mapcycle_add <map> <gametype> a game for the cycle (gametypes: slayer, team_slayer, ctf,",
		"                                   ironctf, king, team_king, oddball, team_oddball, race,",
		"                                   team_race, rally, elimination, stalker, accumulation)",
		"  sv_mapcycle_del <#>              takes a game out of the cycle",
		"  sv_mapcycle_clear, sv_mapcycle   empties, lists the cycle",
		"  sv_mapcycle_begin                the cycle again from its first game",
		"  sv_map <map> <gametype>          plays that game now (the cycle goes on after it)",
		"  sv_map_next, sv_map_reset        the cycle's next game now; the game again",
		"  sv_end_game                      ends the game (its scores, then the next)",
		"  sv_timelimit <minutes>           a game's length (0: until its score)",
		"  sv_scorelimit <score>            the score that wins (0: the gametype's)",
		"  sv_minplayers <n>                players a game needs to start (1)",
		"  sv_start_delay <seconds>         the lobby's countdown once they are in (10)",
		"  sv_postgame <seconds>            the scores shown after a game (10; 3 if nobody is in it)",
		"  sv_end_empty <seconds>           a game nobody is in ends after this (30; 0 never)",
		"  sv_players, sv_status            the players (#, name, address); the server",
		"  sv_kick <#|name>                 drops a player (they may join again)",
		"  sv_ban <#|name>                  drops a player and bans their address and hardware id",
		"  sv_ban_ip <a.b.c.d>              bans an address",
		"  sv_banlist, sv_unban <#>         bans.txt's bans; takes one out",
		"  quit                             stops the server",
	};
	long index;

	for (index = 0; index < (long)NUMBEROF(lines); index++)
		dedicated_console_print("%s", lines[index]);
}

static void command_status(
	void)
{
	struct network_game *game = server_game();
	short state = server_state();
	long minutes = (long)(dedicated.uptime / 60.0f);

	say("%s, up %ldh%02ldm; %s", dedicated.name[0] ? dedicated.name : "(the machine's name)", minutes / 60,
		minutes % 60,
		state == _server_pregame ? "in the lobby" : state == _server_ingame ? "playing" :
		state == _server_postgame ? "showing the scores" : "starting");
	say("%s%s; internet play %s", dedicated.public_game ? "public" : "private (by invite only)",
		dedicated.password[0] && dedicated.public_game ? ", with a password" : "",
		config_boolean("network.online") ? "on" : "off (system link only)");
	if (dedicated.invite[0])
		say("invite %s", dedicated.invite);
	if (game)
	{
		say("%s on %s, %ld of %d players, %ld machine%s", dedicated.playing.gametype[0] ? dedicated.playing.gametype :
			"(none yet)", dedicated.playing.map[0] ? dedicated.playing.map : "(none yet)", player_count(),
			(int)game->maximum_players, machine_count(TRUE), machine_count(TRUE) == 1 ? "" : "s");
	}
}

static void command_players(
	void)
{
	struct network_game *game = server_game();
	long index;
	long count = 0;

	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct network_player const *player = &game->players[index];
		char name[16];
		char address[32];
		char const *hardware_id;

		if (!network_player_is_valid((struct network_player *)player))
			continue;
		player_name(player, name, sizeof(name));
		address_text(real_address(network_game_server_machine_address(player->machine_index)), address,
			sizeof(address));
		hardware_id = network_game_server_machine_hardware_id(player->machine_index);
		say("#%-3ld %-12s team %d  machine %d  %s  hwid %s", index, name, (int)player->team_index,
			(int)player->machine_index, address, hardware_id && hardware_id[0] ? hardware_id : "none");
		count++;
	}
	if (!count)
		say("no players");
}

/* bans.txt's lines (empty ones left out), as the ban commands read them */
static char ban_lines[MAXIMUM_BAN_LINES][BAN_LINE_SIZE];

/* bans.txt's lines, read into ban_lines; how many, and whether the file
holds more (or a longer line) than they do */
static long read_bans(
	boolean *whole)
{
	FILE *file = fopen(BANS_FILE, "r");
	char line[BAN_LINE_SIZE];
	long count = 0;

	*whole = TRUE;
	if (!file)
		return 0;
	while (fgets(line, sizeof(line), file))
	{
		if (!strchr(line, '\n') && !feof(file))
			*whole = FALSE;
		line[strcspn(line, "\r\n")] = 0;
		if (!line[0])
			continue;
		if (count == MAXIMUM_BAN_LINES)
		{
			*whole = FALSE;
			break;
		}
		csstrcpy(ban_lines[count++], line);
	}
	fclose(file);
	return count;
}

static void command_banlist(
	void)
{
	boolean whole;
	long count = read_bans(&whole);
	long index;

	for (index = 0; index < count; index++)
		say("ban #%ld: %s", index, ban_lines[index]);
	if (!count)
		say("no bans (bans.txt in the data folder)");
	if (!whole)
		say("(bans.txt has more, or longer lines, than these: the first %d)", MAXIMUM_BAN_LINES);
}

static void command_unban(
	char const *argument)
{
	boolean whole;
	long count = read_bans(&whole);
	char *end;
	long number = argument ? strtol(argument, &end, 10) : -1;
	FILE *file;
	long index;

	if (!argument || *end || number < 0 || number >= count)
	{
		say("sv_unban: give a ban's number (sv_banlist lists them)");
		return;
	}
	/* (the file is written again from what was read: never less of it) */
	if (!whole)
	{
		say("sv_unban: bans.txt has more, or longer lines, than the server reads (%d); take the line out by hand",
			MAXIMUM_BAN_LINES);
		return;
	}
	file = fopen(BANS_FILE, "w");
	if (!file)
	{
		say("sv_unban: cannot write bans.txt");
		return;
	}
	for (index = 0; index < count; index++)
		if (index != number)
			fprintf(file, "%s\n", ban_lines[index]);
	fclose(file);
	say("unbanned: %s", ban_lines[number]);
}

static void command_ban_address(
	char const *argument)
{
	struct network_game *game = server_game();
	unsigned long address;
	char text[32];
	char when[32] = "";
	time_t now = time(NULL);
	struct tm *local = localtime(&now);
	FILE *file;
	long index;

	if (!argument || !parse_address(argument, &address))
	{
		say("sv_ban_ip: give an address, a.b.c.d");
		return;
	}
	address_text(address, text, sizeof(text));
	if (local)
		strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", local);
	file = fopen(BANS_FILE, "a");
	if (!file)
	{
		say("sv_ban_ip: cannot write bans.txt");
		return;
	}
	/* (bans.txt's line, as network_distributed.c writes it) */
	fprintf(file, "%s\tip=%s\thwid=none\tdiscord_username=none\tdiscord_id=none\tplayers=\treason=banned by the "
		"server's operator\n", when, text);
	fclose(file);
	say("banned %s", text);
	/* (and its players dropped now) */
	for (index = 0; game && index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		if (network_player_is_valid(&game->players[index]) &&
			real_address(network_game_server_machine_address(game->players[index].machine_index)) == address)
		{
			network_game_server_kick_machine_of_player(game->players[index].machine_index, FALSE);
		}
	}
}

/* a player by number (sv_players') or name (as the console's kick and ban) */
static void command_drop(
	char const *argument,
	boolean ban)
{
	struct network_game *game = server_game();
	char *end;
	long number;

	if (!argument || !argument[0])
	{
		say("%s: give a player's number (sv_players) or name", ban ? "sv_ban" : "sv_kick");
		return;
	}
	number = strtol(argument, &end, 10);
	if (!*end && argument[0] != '-')
	{
		if (!game || number < 0 || number >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS ||
			!network_player_is_valid(&game->players[number]))
		{
			say("%s: no player #%ld (sv_players lists them)", ban ? "sv_ban" : "sv_kick", number);
		}
		else if (network_game_server_kick_machine_of_player(game->players[number].machine_index, ban))
			say("%s player #%ld", ban ? "banned" : "kicked", number);
		return;
	}
	{
		char name[64];

		unquoted(name, sizeof(name), argument);
		if (ban ? network_game_server_ban_player(name) : network_game_server_kick_player(name))
			say("%s %s", ban ? "banned" : "kicked", name);
		else
			say("%s: no one player named %s (sv_players lists them)", ban ? "sv_ban" : "sv_kick", name);
	}
}

/* an integer setting's argument within its bounds: FALSE (said) if not */
static boolean integer_argument(
	char const *command,
	char const *argument,
	long minimum,
	long maximum,
	long *value)
{
	char *end;
	long number = argument ? strtol(argument, &end, 10) : 0;

	if (!argument || *end || number < minimum || number > maximum)
	{
		say("%s: give a number from %ld to %ld", command, minimum, maximum);
		return FALSE;
	}
	*value = number;
	return TRUE;
}

/* printable ASCII only, at most size - 1 of it */
static void printable(
	char *destination,
	long size,
	char const *source)
{
	long length = 0;

	for (; source && *source && length < size - 1; source++)
		if (*source >= 32 && *source < 127)
			destination[length++] = *source;
	destination[length] = 0;
}

/* the words of a command: separated by spaces, a quoted one as it is;
how many */
static long split(
	char *line,
	char **words,
	long maximum)
{
	long count = 0;
	char *cursor = line;

	while (*cursor && count < maximum)
	{
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (!*cursor)
			break;
		if (*cursor == '"')
		{
			words[count++] = ++cursor;
			while (*cursor && *cursor != '"')
				cursor++;
		}
		else
		{
			words[count++] = cursor;
			while (*cursor && *cursor != ' ' && *cursor != '\t')
				cursor++;
		}
		if (*cursor)
			*cursor++ = 0;
	}
	return count;
}

/* the text after the command's word (sv_name's, sv_password's), quotes and
all */
static char const *rest_of_line(
	char const *line)
{
	while (*line == ' ' || *line == '\t')
		line++;
	while (*line && *line != ' ' && *line != '\t')
		line++;
	while (*line == ' ' || *line == '\t')
		line++;
	return line;
}

/* a quoted text's inside, else the text */
static void unquoted(
	char *destination,
	long size,
	char const *text)
{
	char kept[COMMAND_SIZE];
	long length;

	printable(kept, sizeof(kept), text);
	length = (long)csstrlen(kept);
	while (length > 0 && kept[length - 1] == ' ')
		kept[--length] = 0;
	if (length >= 2 && kept[0] == '"' && kept[length - 1] == '"')
	{
		kept[length - 1] = 0;
		printable(destination, size, kept + 1);
	}
	else
		printable(destination, size, kept);
}

/* a console's command in the log, as typed; a password's command (its
name has "password" in it) without its rest */
static void log_command(
	char const *name,
	char const *line)
{
	if (stristr(name, "password"))
		platform_log("server: the console: %s (the rest not shown)", name);
	else
		platform_log("server: the console: %s", line);
}

static void command(
	char const *line,
	boolean from_file)
{
	/* the line (printable ASCII), and a copy split into its words */
	char whole[COMMAND_SIZE];
	char copy[COMMAND_SIZE];
	char *words[MAXIMUM_ARGUMENTS] = { 0 };
	long count;
	char const *name;
	char const *argument;
	long value;

	printable(whole, sizeof(whole), line);
	csstrcpy(copy, whole);
	count = split(copy, words, MAXIMUM_ARGUMENTS);
	/* (nothing, or a comment: ";", "#" or "//") */
	if (!count || words[0][0] == ';' || words[0][0] == '#' || (words[0][0] == '/' && words[0][1] == '/'))
		return;
	lowercase(words[0]);
	name = words[0];
	argument = count > 1 ? words[1] : NULL;
	/* (the console's commands in the log; a password never) */
	if (!from_file)
		log_command(name, whole);

	if (!csstrcmp(name, "help") || !csstrcmp(name, "sv_help") || !csstrcmp(name, "?"))
		command_help();
	else if (!csstrcmp(name, "quit") || !csstrcmp(name, "exit") || !csstrcmp(name, "sv_quit"))
	{
		say("stopping");
		exit(EXIT_SUCCESS);
	}
	else if (!csstrcmp(name, "sv_name"))
	{
		char text[SERVER_NAME_SIZE];

		unquoted(text, sizeof(text), rest_of_line(whole));
		if (!text[0])
			say("sv_name: give a name");
		else
		{
			csstrcpy(dedicated.name, text);
			apply_settings();
			say("name: %s%s", text, server_state() > _server_pregame ? " (from the next game)" : "");
		}
	}
	else if (!csstrcmp(name, "sv_maxplayers"))
	{
		if (integer_argument(name, argument, 2, HALO_PORT_MAXIMUM_NETWORK_PLAYERS, &value))
		{
			dedicated.maximum_players = value;
			apply_settings();
			say("most players: %ld%s", value, server_state() > _server_pregame ? " (from the next game)" : "");
		}
	}
	else if (!csstrcmp(name, "sv_password"))
	{
		char text[PASSWORD_SIZE];

		unquoted(text, sizeof(text), rest_of_line(whole));
		csstrcpy(dedicated.password, text);
		if (dedicated.hosting)
			apply_listing();
		say(text[0] ? "password set (joining from the server browser asks for it; the invite link still joins)" :
			"no password");
		/* (the copies of the line on the stack, cleared) */
		csmemset(text, 0, sizeof(text));
		csmemset(whole, 0, sizeof(whole));
		csmemset(copy, 0, sizeof(copy));
	}
	else if (!csstrcmp(name, "sv_public"))
	{
		if (integer_argument(name, argument, 0, 1, &value))
		{
			dedicated.public_game = (boolean)value;
			if (dedicated.hosting)
				apply_listing();
			say(value ? "public: listed in the server browser" : "private: joined by its invite only");
			if (value && (!config_boolean("network.online") || !config_boolean("network.public_lobby")))
				say("(not listed while network.online or network.public_lobby is off)");
		}
	}
	else if (!csstrcmp(name, "sv_mapcycle_add"))
	{
		struct game_variant variant;
		char path[LEVEL_PATH_SIZE];

		if (count < 3)
			say("sv_mapcycle_add: give a map and a gametype (sv_mapcycle_add bloodgulch ctf)");
		else if (!map_name_valid(words[1]))
			say("sv_mapcycle_add: %s is not a map's name", words[1]);
		else if (!gametype_variant(words[2], &variant))
			say("sv_mapcycle_add: %s is not a gametype (help lists them)", words[2]);
		else if (dedicated.cycle_count == MAXIMUM_CYCLE_ENTRIES)
			say("sv_mapcycle_add: the cycle is full (%d games)", MAXIMUM_CYCLE_ENTRIES);
		else if (map_level(words[1], path, sizeof(path), name))
		{
			struct cycle_entry *entry = &dedicated.cycle[dedicated.cycle_count++];

			lowercase(words[1]);
			lowercase(words[2]);
			csstrcpy(entry->map, words[1]);
			csstrncpy(entry->gametype, words[2], sizeof(entry->gametype) - 1);
			entry->gametype[sizeof(entry->gametype) - 1] = 0;
			say("cycle #%ld: %s on %s", dedicated.cycle_count - 1, entry->gametype, entry->map);
		}
	}
	else if (!csstrcmp(name, "sv_mapcycle_del"))
	{
		if (!dedicated.cycle_count)
			say("sv_mapcycle_del: the cycle is empty");
		else if (integer_argument(name, argument, 0, dedicated.cycle_count - 1, &value))
		{
			long index;

			say("cycle: %s on %s taken out", dedicated.cycle[value].gametype, dedicated.cycle[value].map);
			for (index = value; index + 1 < dedicated.cycle_count; index++)
				dedicated.cycle[index] = dedicated.cycle[index + 1];
			dedicated.cycle_count--;
			if (dedicated.cycle_index > value)
				dedicated.cycle_index--;
		}
	}
	else if (!csstrcmp(name, "sv_mapcycle_clear"))
	{
		dedicated.cycle_count = 0;
		dedicated.cycle_index = 0;
		say("the cycle is empty (sv_mapcycle_add adds games; Blood Gulch slayer until then)");
	}
	else if (!csstrcmp(name, "sv_mapcycle") || !csstrcmp(name, "sv_mapcycle_begin"))
	{
		long index;

		if (!csstrcmp(name, "sv_mapcycle_begin"))
		{
			dedicated.cycle_index = 0;
			dedicated.has_next_game = FALSE;
			if (!end_game("the cycle starts over", FALSE) && server_state() == _server_pregame)
				dedicated.map_set = FALSE;
		}
		for (index = 0; index < dedicated.cycle_count; index++)
			say("cycle #%ld: %s on %s%s", index, dedicated.cycle[index].gametype, dedicated.cycle[index].map,
				index == dedicated.cycle_index % dedicated.cycle_count ? " (next)" : "");
		if (!dedicated.cycle_count)
			say("the cycle is empty (Blood Gulch slayer)");
	}
	else if (!csstrcmp(name, "sv_map"))
	{
		struct game_variant variant;
		char path[LEVEL_PATH_SIZE];

		if (count < 3)
			say("sv_map: give a map and a gametype (sv_map bloodgulch ctf)");
		else if (!map_name_valid(words[1]))
			say("sv_map: %s is not a map's name", words[1]);
		else if (!gametype_variant(words[2], &variant))
			say("sv_map: %s is not a gametype (help lists them)", words[2]);
		else if (map_level(words[1], path, sizeof(path), name))
		{
			lowercase(words[1]);
			lowercase(words[2]);
			csstrcpy(dedicated.next_game.map, words[1]);
			csstrncpy(dedicated.next_game.gametype, words[2], sizeof(dedicated.next_game.gametype) - 1);
			dedicated.next_game.gametype[sizeof(dedicated.next_game.gametype) - 1] = 0;
			dedicated.has_next_game = TRUE;
			if (!end_game("sv_map", FALSE))
			{
				if (server_state() == _server_pregame)
					dedicated.map_set = FALSE;
				say("next: %s on %s", words[2], words[1]);
			}
		}
	}
	else if (!csstrcmp(name, "sv_map_next"))
	{
		if (!end_game("sv_map_next", FALSE))
			say("sv_map_next: no game is being played");
	}
	else if (!csstrcmp(name, "sv_map_reset"))
	{
		if (!end_game("sv_map_reset", TRUE))
			say("sv_map_reset: no game is being played");
	}
	else if (!csstrcmp(name, "sv_end_game"))
	{
		if (!end_game("sv_end_game", FALSE))
			say("sv_end_game: no game is being played");
	}
	else if (!csstrcmp(name, "sv_timelimit"))
	{
		if (integer_argument(name, argument, 0, 24 * 60, &dedicated.time_limit_minutes))
			say(dedicated.time_limit_minutes ? "a game lasts %ld minutes at most" : "a game lasts until its score",
				dedicated.time_limit_minutes);
	}
	else if (!csstrcmp(name, "sv_scorelimit"))
	{
		if (integer_argument(name, argument, 0, 999, &dedicated.score_limit))
			say(dedicated.score_limit ? "the score to win: %ld (from the next game)" :
				"the score to win: the gametype's (from the next game)", dedicated.score_limit);
	}
	else if (!csstrcmp(name, "sv_minplayers"))
	{
		if (integer_argument(name, argument, 1, HALO_PORT_MAXIMUM_NETWORK_PLAYERS, &dedicated.minimum_players))
			say("a game starts with %ld player%s", dedicated.minimum_players, dedicated.minimum_players == 1 ? "" : "s");
	}
	else if (!csstrcmp(name, "sv_start_delay"))
	{
		if (integer_argument(name, argument, 1, 120, &dedicated.start_delay))
			say("the lobby counts down %ld seconds", dedicated.start_delay);
	}
	else if (!csstrcmp(name, "sv_postgame"))
	{
		if (integer_argument(name, argument, MINIMUM_POSTGAME_SECONDS, 120, &dedicated.postgame_seconds_setting))
			say("the scores are shown %ld seconds", dedicated.postgame_seconds_setting);
	}
	else if (!csstrcmp(name, "sv_end_empty"))
	{
		if (integer_argument(name, argument, 0, 3600, &dedicated.end_empty_seconds))
			say(dedicated.end_empty_seconds ? "a game nobody is in ends after %ld seconds" :
				"a game nobody is in goes on", dedicated.end_empty_seconds);
	}
	else if (!csstrcmp(name, "sv_port"))
		say("sv_port: network.tunnel_port (HALO_NET_TUNNEL_PORT) is internet play's UDP port");
	else if (!csstrcmp(name, "sv_players"))
		command_players();
	else if (!csstrcmp(name, "sv_status") || !csstrcmp(name, "status"))
		command_status();
	else if (!csstrcmp(name, "sv_kick"))
		command_drop(argument ? rest_of_line(whole) : NULL, FALSE);
	else if (!csstrcmp(name, "sv_ban"))
		command_drop(argument ? rest_of_line(whole) : NULL, TRUE);
	else if (!csstrcmp(name, "sv_ban_ip"))
		command_ban_address(argument);
	else if (!csstrcmp(name, "sv_banlist"))
		command_banlist();
	else if (!csstrcmp(name, "sv_unban"))
		command_unban(argument);
	else if (!csstrcmp(name, "sv_rcon_password") || !csstrcmp(name, "rcon") || !csstrcmp(name, "sv_rcon"))
		say("%s: there is no remote console; commands are typed here, on the server's machine", name);
	else
		say("%s: not a command (help lists them)", name);
}

/* ---------- the server's hooks */

boolean dedicated_server_active(
	void)
{
#if defined(HALO_ANDROID) || defined(HALO_WEB)
	return FALSE;
#else
	if (dedicated.active < 0)
		dedicated.active = config_boolean("server.dedicated") ? TRUE : FALSE;
	return (boolean)dedicated.active;
#endif
}

boolean dedicated_server_lobby_ready(
	void)
{
	return dedicated_server_active() && dedicated.hosting && dedicated.map_set && !dedicated.back_to_lobby;
}

long dedicated_server_countdown_milliseconds(
	void)
{
	return dedicated.start_delay * 1000 - 1;
}

long dedicated_server_minimum_players(
	void)
{
	return dedicated.minimum_players;
}

boolean dedicated_server_join_allowed(
	unsigned long address)
{
	unsigned long now = (unsigned long)time(NULL);
	struct join_address *slot = NULL;
	struct join_address *oldest = &dedicated.joins[0];
	long index;

	if (!dedicated_server_active())
		return TRUE;
	address = real_address(address);
	if (!address)
		return TRUE;
	for (index = 0; index < MAXIMUM_JOIN_ADDRESSES; index++)
	{
		struct join_address *entry = &dedicated.joins[index];

		if (entry->address == address)
		{
			slot = entry;
			break;
		}
		if (entry->refill_time < oldest->refill_time)
			oldest = entry;
	}
	if (!slot)
	{
		/* (the address least lately counted makes room: full again by now) */
		slot = oldest;
		slot->address = address;
		slot->allowance = DEDICATED_JOINS_PER_MINUTE;
		slot->refill_time = now;
	}
	while (slot->allowance < DEDICATED_JOINS_PER_MINUTE && now - slot->refill_time >= JOIN_REFILL_SECONDS)
	{
		slot->allowance++;
		slot->refill_time += JOIN_REFILL_SECONDS;
	}
	if (slot->allowance >= DEDICATED_JOINS_PER_MINUTE)
		slot->refill_time = now;
	if (slot->allowance <= 0)
	{
		char text[32];

		address_text(address, text, sizeof(text));
		platform_log("server: %s connects too often (%d a minute); refused", text, DEDICATED_JOINS_PER_MINUTE);
		return FALSE;
	}
	slot->allowance--;
	return TRUE;
}

/* ---------- each frame */

void dedicated_server_update(
	boolean main_menu_loaded,
	real seconds)
{
	struct network_game_server *server;
	char line[COMMAND_SIZE];
	int from_file;
	long taken;
	short state;

	if (!dedicated_server_active())
		return;
	/* (the clock's: a frame's seconds are the game's, held back after a
	long one) */
	{
		static unsigned long started;
		unsigned long now = system_milliseconds();

		if (!started)
			started = now ? now : 1;
		dedicated.uptime = (real)(now - started) / 1000.0f;
	}
	if (dedicated_console_quit_requested())
	{
		say("stopping (a signal)");
		exit(EXIT_SUCCESS);
	}
	if (config_real("debug.exit_after") > 0.0 && dedicated.uptime >= config_real("debug.exit_after"))
	{
		platform_log("exiting after debug.exit_after");
		exit(EXIT_SUCCESS);
	}

	/* the start: the main menu up (its maps found), the command file's
	commands, then the console */
	if (!dedicated.started)
	{
		int found;
		char const *file;

		if (!main_menu_loaded)
			return;
		dedicated.started = TRUE;
		/* (nothing drawn: no one sees it) */
		debug_no_drawing = TRUE;
		dedicated.maximum_players = 16;
		dedicated.public_game = config_boolean("network.host_public");
		dedicated.minimum_players = 1;
		dedicated.start_delay = 10;
		dedicated.postgame_seconds_setting = 10;
		dedicated.end_empty_seconds = 30;
		say("dedicated server, network version %d; data folder %s", HALO_PORT_NETWORK_VERSION, platform_data_root());
		file = dedicated_console_file(&found);
		say(found ? "commands from %s" : "no %s: the defaults (Blood Gulch slayer)", file);
		while (dedicated_console_next(line, sizeof(line), &from_file))
			command(line, from_file);
		if (!dedicated.cycle_count)
			say("the cycle is empty: Blood Gulch slayer (sv_mapcycle_add adds games)");
		dedicated_console_start();
		say("type help for the commands");
	}
	for (taken = 0; taken < MAXIMUM_CONSOLE_COMMANDS_PER_FRAME &&
		dedicated_console_next(line, sizeof(line), &from_file); taken++)
	{
		command(line, from_file);
	}

	server = global_network_game_server_get();
	state = server_state();

	/* (a frame's least time while no other machine is connected) */
	if (!server || (state == _server_pregame && !machine_count(FALSE)))
	{
		unsigned long now = system_milliseconds();
		unsigned long elapsed = now - dedicated.last_frame_time;

		if (elapsed < IDLE_FRAME_MILLISECONDS)
			Sleep(IDLE_FRAME_MILLISECONDS - elapsed);
	}
	dedicated.last_frame_time = system_milliseconds();

	/* hosting: from the main menu, where the game starts and comes back to
	if its server goes (the network lost) */
	if (!server)
	{
		if (!main_menu_loaded)
			return;
		if (dedicated.hosting)
		{
			dedicated.hosting = FALSE;
			dedicated.invite[0] = 0;
			say("the game's server stopped; hosting again");
			dedicated.menu_seconds = 0.0f;
		}
		dedicated.menu_seconds += seconds;
		if (dedicated.menu_seconds < 2.0f)
			return;
		apply_settings();
		player_ui_fast_setup_network_server();
		if (!global_network_game_server_get())
		{
			say("could not host (the game's ports taken by another program? network.address gives each "
				"server on a machine its own); trying again in 5 s");
			dedicated.menu_seconds = -3.0f;
			return;
		}
		dedicated.hosting = TRUE;
		dedicated.map_set = FALSE;
		dedicated.lobby_seconds = 0.0f;
		dedicated.map_checked_seconds = 0.0f;
		dedicated.back_to_lobby = FALSE;
		apply_settings();
		apply_listing();
		return;
	}

	note_invite();
	note_players();

	switch (state)
	{
	case _server_pregame:
		if (!main_menu_loaded)
			break;
		/* (back from a game: the cycle's next, or the same again) */
		if (dedicated.back_to_lobby)
		{
			dedicated.back_to_lobby = FALSE;
			if (dedicated.has_next_game && !dedicated.restart &&
				!strcmp(dedicated.playing.map, dedicated.next_game.map) &&
				!strcmp(dedicated.playing.gametype, dedicated.next_game.gametype))
			{
				/* (sv_map's game played: the cycle goes on after it) */
				dedicated.has_next_game = FALSE;
			}
			/* (a game of the cycle over, or cut short by sv_map's: its next) */
			else if (!dedicated.restart && dedicated.cycle_count)
				dedicated.cycle_index = (dedicated.cycle_index + 1) % dedicated.cycle_count;
			dedicated.restart = FALSE;
			dedicated.map_set = FALSE;
			dedicated.lobby_seconds = 0.0f;
			dedicated.map_checked_seconds = 0.0f;
			/* (as picking the next game's map does: the scores' map choice
			holds the countdown) */
			network_game_server_pause_countdown(server, FALSE);
		}
		dedicated.lobby_seconds += seconds;
		dedicated.game_seconds = 0.0f;
		dedicated.empty_seconds = 0.0f;
		dedicated.postgame_seconds = 0.0f;
		dedicated.end_asked = FALSE;
		if (!dedicated.map_set && dedicated.lobby_seconds >= 1.0f)
		{
			dedicated.map_set = apply_entry(next_entry());
			if (!dedicated.map_set)
			{
				say("the game %s on %s cannot be set up; the cycle's next", next_entry()->gametype, next_entry()->map);
				dedicated.has_next_game = FALSE;
				if (dedicated.cycle_count)
					dedicated.cycle_index = (dedicated.cycle_index + 1) % dedicated.cycle_count;
			}
		}
		/* (the lobby's own widgets may put the map back: set again) */
		else if (dedicated.map_set && dedicated.level_path[0] &&
			dedicated.lobby_seconds - dedicated.map_checked_seconds >= 1.0f)
		{
			dedicated.map_checked_seconds = dedicated.lobby_seconds;
			if (strcmp(main_get_multiplayer_map_name(), dedicated.level_path))
				network_game_server_change_map_name(server, dedicated.level_path);
		}
		break;
	case _server_ingame:
		if (dedicated.last_state != _server_ingame)
		{
			say("the game starts: %s on %s, %ld player%s", dedicated.playing.gametype, dedicated.playing.map,
				player_count(), player_count() == 1 ? "" : "s");
		}
		if (!game_engine_running())
			break;
		/* (the scores, shown a while: then the lobby, for the next game) */
		if (!game_engine_can_score())
		{
			if (game_engine_showing_postgame())
			{
				dedicated.postgame_seconds += seconds;
				if (dedicated.postgame_seconds >= postgame_seconds_wanted() && !dedicated.back_to_lobby)
				{
					dedicated.back_to_lobby = TRUE;
					network_game_server_reset_to_pregame(server);
				}
			}
			break;
		}
		dedicated.game_seconds += seconds;
		if (dedicated.time_limit_minutes > 0 && dedicated.game_seconds >= (real)(dedicated.time_limit_minutes * 60))
			end_game("the time limit", FALSE);
		if (machine_count(TRUE) == 0)
		{
			dedicated.empty_seconds += seconds;
			if (dedicated.end_empty_seconds > 0 && dedicated.empty_seconds >= (real)dedicated.end_empty_seconds)
				end_game("nobody is left", TRUE);
		}
		else
			dedicated.empty_seconds = 0.0f;
		break;
	case _server_postgame:
		dedicated.postgame_seconds += seconds;
		if (dedicated.postgame_seconds >= postgame_seconds_wanted() && !dedicated.back_to_lobby)
		{
			dedicated.back_to_lobby = TRUE;
			network_game_server_reset_to_pregame(server);
		}
		break;
	}
	dedicated.last_state = state;
}
