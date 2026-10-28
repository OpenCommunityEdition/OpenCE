/*
SERVER_TOML.H

A dedicated server's gametype files (server_toml.c): gametypes/<name>.toml
in the data root, read with tomlc17 into its base gametype, its name and the
rules it sets (server_rotation.h's), each checked to be a rule of the table
it is in and within its range. port/linux/game/server_gametypes.c builds the
game's variant from them.

    base = "team_slayer"   # a built-in or OpenCE gametype: its rules are the defaults
    name = "BTB Slayer"    # 11 characters, shown in the lobby and the server browser
    [universal]            # any game engine's rules
    score_to_win = 50
    [options]              # the PC version's options
    time_limit = 15
    friendly_fire = "off"
    [slayer]               # the base's game engine's: ctf, slayer, king, oddball, race
    no_death_bonus = true
*/

#ifndef __HALO_LINUX_SERVER_TOML_H
#define __HALO_LINUX_SERVER_TOML_H

#include "server_rotation.h"

enum
{
	SERVER_GAMETYPE_NAME_SIZE = 12,
	/* the largest gametype file read */
	SERVER_GAMETYPE_FILE_MAXIMUM_SIZE = 64 * 1024,
};

struct server_gametype_file
{
	char base[SERVER_GAMETYPE_SIZE];
	/* the name shown (empty: the base's) */
	char name[SERVER_GAMETYPE_NAME_SIZE];
	/* the engine table it has ("" if none): the base must be of it */
	char engine_table[16];
	struct server_rule rules[SERVER_RULES_MAXIMUM];
	int rule_count;
	/* why it cannot be used (empty: it can), and the line */
	char error[SERVER_REASON_SIZE];
	int error_line;
};

/* a gametype file's text read: 1 if it can be used, else 0 with error set */
int server_gametype_file_parse(const char *text, int length, struct server_gametype_file *file);
/* the gametype file of the name (gametypes/<name>.toml in the data root):
1 if it is there and can be used; 0 with error set (or empty if there is
no such file, and absent set) */
int server_gametype_file_read(const char *name, struct server_gametype_file *file, int *absent);
/* the gametypes folder (the data root's), with no separator after it */
void server_gametypes_folder(char *path, int size);
/* the first size bytes of the saved gametype <name>.gametype in that folder
(a dedicated server's file=<name> entries: the game's files take Xbox
paths only): how many were read, -1 if it cannot be */
int server_gametype_saved_read(const char *name, void *data, int size);
/* whether a name may be a gametype file's: letters, digits, "_" and "-" */
int server_gametype_file_name_valid(const char *name);

#endif
