/*
SERVER_ROTATION.H

A dedicated server's rotation (server.rotation) and gametypes' rules
(server_rotation.c): the grammar read into plain structures, and the table of
the rules a rotation entry (+score=50) or a gametype file
(gametypes/<name>.toml, server_toml.c) may set, with their ranges. Nothing
here touches the game: port/linux/game/server_gametypes.c applies the rules
to a game's variant and options, and checks each against its game engine.

    rotation := entry { ("," | newline) entry }
    entry    := map [ ":" gametype { "+" rule } ]
    map      := name | "custom_maps\" name | a level's path
    gametype := a built-in's name | an OpenCE gametype's | a gametype file's name
                | "file=" a saved gametype's name | "coop"
    rule     := key "=" value
*/

#ifndef __HALO_LINUX_SERVER_ROTATION_H
#define __HALO_LINUX_SERVER_ROTATION_H

enum
{
	SERVER_ROTATION_MAXIMUM_ENTRIES = 64,
	SERVER_ROTATION_MAXIMUM_TEXT = 4096,
	SERVER_RULES_MAXIMUM = 48,
	SERVER_ENTRY_TEXT_SIZE = 256,
	SERVER_MAP_SIZE = 128,
	SERVER_GAMETYPE_SIZE = 64,
	SERVER_RULE_KEY_SIZE = 32,
	SERVER_RULE_VALUE_SIZE = 32,
	SERVER_REASON_SIZE = 160,
};

/* a rule's type */
enum
{
	_server_rule_integer,
	_server_rule_boolean,
	_server_rule_choice,
};

/* the game engines a rule is of */
enum
{
	_server_engine_ctf = 1 << 0,
	_server_engine_slayer = 1 << 1,
	_server_engine_oddball = 1 << 2,
	_server_engine_king = 1 << 3,
	_server_engine_race = 1 << 4,
	_server_engine_all = 0x1F,
};

struct server_rule_definition
{
	const char *key;
	/* a shorter name for the rotation's +rule=value (NULL: none) */
	const char *alias;
	/* the gametype file's table it is in: "universal", "options", or its
	engine's ("ctf", "slayer", "king", "oddball", "race") */
	const char *table;
	int engines;
	int type;
	long minimum;
	long maximum;
	/* a choice's words, NULL ended; the value is the word's index */
	const char *const *choices;
	const char *description;
};

/* the rules, in order; their count */
extern const struct server_rule_definition server_rule_definitions[];
int server_rule_definition_count(void);
/* the rule of a key or alias (in any case), NULL if none */
const struct server_rule_definition *server_rule_find(const char *key);
/* the value text (a number, true/false/yes/no/on/off/1/0, a choice's word or
its index) as the rule's value: 1 if it is one in range, else 0 and why */
int server_rule_value(const struct server_rule_definition *rule, const char *text, long *value, char *reason,
	int reason_size);

struct server_rule
{
	/* the definition's key (an alias is taken as its key) */
	char key[SERVER_RULE_KEY_SIZE];
	long value;
};

enum
{
	/* how an entry's map was given */
	_server_map_name,
	_server_map_custom,
	_server_map_path,
};

struct server_entry
{
	/* as written, spaces trimmed */
	char text[SERVER_ENTRY_TEXT_SIZE];
	/* as given: "bloodgulch", "custom_maps\hugeass", a path */
	char map[SERVER_MAP_SIZE];
	int map_kind;
	/* the level the game plays (levels\test\bloodgulch\bloodgulch,
	custom_maps\hugeass): the map's, once resolved (server_rotation_resolve) */
	char level[SERVER_MAP_SIZE];
	/* the map's own name: bloodgulch, hugeass */
	char map_name[SERVER_MAP_SIZE];
	/* lowercased; "slayer" when none was given */
	char gametype[SERVER_GAMETYPE_SIZE];
	struct server_rule rules[SERVER_RULES_MAXIMUM];
	int rule_count;
	/* why it cannot be played (empty: it can) */
	char rejected[SERVER_REASON_SIZE];
	/* rejected because its map is missing (not because of what it says) */
	int map_missing;
};

struct server_rotation
{
	struct server_entry entries[SERVER_ROTATION_MAXIMUM_ENTRIES];
	int count;
	/* entries past SERVER_ROTATION_MAXIMUM_ENTRIES, or text past
	SERVER_ROTATION_MAXIMUM_TEXT, left out */
	int truncated;
};

/* one entry's text read: 1 if it says something (its rejected says
whether it can be played) */
int server_rotation_parse_entry(const char *text, int length, struct server_entry *entry);
/* the rotation's text read: every entry, rejected ones too */
void server_rotation_parse(const char *text, struct server_rotation *rotation);
/* the number of entries that are not rejected */
int server_rotation_playable(struct server_rotation const *rotation);
/* the rotation as one text, entries separated by ", " */
void server_rotation_text(struct server_rotation const *rotation, char *text, int size);

/* the entry's map found on disk (the data root's maps folder, custom_maps):
its level and map name set, or it rejected (map_missing) */
void server_rotation_resolve(struct server_entry *entry);

/* each multiplayer map there is: the maps folder's (kind "xbox") and
custom_maps' (kind "custom"), by its name, with its level's path; how many */
int server_maps_each(void (*each)(void *context, const char *name, const char *level, const char *kind),
	void *context);

/* a cache file's header: its version (5 for the Xbox's, 7 for Halo PC's,
609 for Custom Edition's) and its scenario's type (0 campaign, 1
multiplayer, 2 the main menu); 0 if it is not a cache file */
int server_map_header(const char *path, long *version, long *scenario_type);

#endif
