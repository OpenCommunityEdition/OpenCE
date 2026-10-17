/*
PORT_MODS.C

The mods (port_mods.h): folders in the data root's mods folder, each with a
mod.toml saying what it is and a tags folder of Halo PC tag files, as the
Halo Editing Kit and Invader write them (tags/sound/.../name.sound). The
game plays a map's tags from the files of the mods that are on
(port/linux/game/mods.c): only those of this machine's sounds, bitmaps and
text, so that a mod changes nothing another machine of a network game sees.

Every mod in the folder is on, in a load order: the folders' names'
alphabetical order, except that the mods a mod must load after (those it
asks to load after, and those that ask to load before it) move up to just
before it. The Mods menu (menu_functions.c) turns mods off and moves them,
and keeps that in the mods folder's load_order.toml (the folders' names in
order, then those turned off); a mod not in it yet takes its place in the
default order, on the side of the others it asks for. A file of several
mods is the one of the mod loaded last, of those that are on and for the
map.

The mods played are scanned when a map loads and when the console asks
(mods_reload): their mod.toml read, and each one's tags folder listed once
into a table of its files by their tag path, lower case with backslashes
("sound\sfx\weapons\assault rifle\fire.sound"). A map's tag is looked up in
that table by its name, so no name a map holds ever makes a path: a mod's
files are only the ones its folder has.

mod.toml (every key may be left out):

	name = "High-res audio"       # what the menu shows (else its folder's name)
	id = "hd_audio"               # what other mods name it by (else its folder's name)
	version = "1.0"
	authors = ["...", "..."]      # or one string
	description = "..."
	maps = ["a10", "bloodgulch"]  # the maps it is for (else every map)
	load_after = ["hd_textures"]  # mods it plays over, which load before it
	load_before = ["fixes"]       # mods that play over it
	incompatible = ["other_hud"]  # mods it cannot be played with
	format = 1                    # the mods' format it is made for

A folder without mod.toml is not a mod, and one made for a later format than
this build's is not played. Anything else wrong is logged, and the rest used.
When mods that are on name others that are on as incompatible, the game asks
at startup whether to play them anyway, turn those named off, or quit
(port_mods_conflicts).
*/

#include "platform.h"
#include "port_config.h"
#include "port_mods.h"
#include "posix.h"
#include "tomlc17.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HALO_WEB
/* (port/web/src/web_library.js) */
void web_js_post(int kind, const char *text);
#endif

/* ---------- constants */

/* the mods' format this build plays */
#define MODS_FORMAT 1

#define MAXIMUM_MODS 256
#define MAXIMUM_MOD_FOLDER_LENGTH 64
/* the most files the mods played may have, and the deepest their folders go
(a folder linking to one above it stops there) */
#define MAXIMUM_MOD_FILES 262144
#define MAXIMUM_FOLDER_DEPTH 24
/* and the most folders they may have (a folder linking to one above it
twice would otherwise be walked 2 to the depth times) */
#define MAXIMUM_MOD_FOLDERS 65536
#define MAXIMUM_PATH_LENGTH 1024
#define MAXIMUM_MAPS_PER_MOD 64
#define FILE_BUCKET_COUNT 16384
/* what the file reads go through on their way into the Xbox's memory, which
the renderer watches (memory_watch.c) */
#define READ_BOUNCE_SIZE 0x10000
/* the most a folder name's character takes in load_order.toml (\\u00XX) */
#define TOML_CHARACTER_LENGTH 6

/* ---------- structures */

/* the mods a mod names (by their ids or folders) */
struct mod_names
{
	char **names;
	long count;
};

struct port_mod
{
	char folder[MAXIMUM_MOD_FOLDER_LENGTH];
	char id[MAXIMUM_MOD_FOLDER_LENGTH];
	char name[128];
	char version[32];
	char authors[128];
	char description[256];
	/* NULL: every map */
	char **maps;
	long map_count;
	struct mod_names load_after;
	struct mod_names load_before;
	struct mod_names incompatible;
	/* turned off in the Mods menu */
	int disabled;
	/* its place in the load order */
	long position;
	/* its place in the order its files were listed in (later wins), or -1:
	not played */
	long order;
	/* why it is not played whatever the menu says, or NULL */
	const char *refusal;
	long file_count;
};

struct port_mod_file
{
	long next;
	unsigned long hash;
	long mod;
	/* lower case, backslashes */
	char *key;
	char *path;
};

/* ---------- globals */

static struct
{
	struct port_mod mods[MAXIMUM_MODS];
	long mod_count;
	/* the mods by their places in the load order */
	long ordered[MAXIMUM_MODS];
	long played_count;
	struct port_mod_file *files;
	long file_count;
	long file_capacity;
	long buckets[FILE_BUCKET_COUNT];
	long folder_count;
	unsigned long scans;
	int full_reported;
	/* load_order.toml as it was last kept, when it could not be written:
	what the mods are ordered by until it can (NULL: the file) */
	char *unsaved_order;
} port_mods;

/* ---------- private code */

static char lower(char character)
{
	return character >= 'A' && character <= 'Z' ? (char)(character - 'A' + 'a') : character;
}

static unsigned long key_hash(const char *key)
{
	unsigned long hash = 2166136261UL;

	for (; *key; key++)
		hash = (hash ^ (unsigned char)*key) * 16777619UL;
	return hash;
}

/* `text` as a key: lower case, backslashes; 0 when it does not fit */
static int key_make(const char *text, char *key, size_t size)
{
	size_t length = 0;

	for (; *text; text++)
	{
		if (length + 1 >= size)
			return 0;
		key[length++] = *text == '/' ? '\\' : lower(*text);
	}
	key[length] = 0;
	return 1;
}

/* the alphabetical order of two names, whatever their case */
static int names_compare(const char *first, const char *second)
{
	for (; *first && lower(*first) == lower(*second); first++, second++)
	{
	}
	return (unsigned char)lower(*first) - (unsigned char)lower(*second);
}

static int names_equal(const char *first, const char *second)
{
	for (; *first && *second; first++, second++)
	{
		if (lower(*first) != lower(*second))
			return 0;
	}
	return *first == *second;
}

static void names_dispose(struct mod_names *names)
{
	long index;

	for (index = 0; index < names->count; index++)
		free(names->names[index]);
	free(names->names);
	names->names = NULL;
	names->count = 0;
}

static void mod_dispose(struct port_mod *mod)
{
	long index;

	for (index = 0; index < mod->map_count; index++)
		free(mod->maps[index]);
	free(mod->maps);
	mod->maps = NULL;
	mod->map_count = 0;
	names_dispose(&mod->load_after);
	names_dispose(&mod->load_before);
	names_dispose(&mod->incompatible);
}

/* whether the mod `mod` is the one `name` names: its id, or its folder */
static int mod_named(const struct port_mod *mod, const char *name)
{
	return names_equal(mod->id, name) || names_equal(mod->folder, name);
}

/* whether a mod names `other` among `names` */
static int names_name(const struct mod_names *names, const struct port_mod *other)
{
	long index;

	for (index = 0; index < names->count; index++)
	{
		if (mod_named(other, names->names[index]))
			return 1;
	}
	return 0;
}

/* a list of strings of mod.toml (or of load_order.toml) */
static void toml_names(toml_datum_t table, const char *key, const char *path, struct mod_names *names)
{
	toml_datum_t value = toml_get(table, key);
	long index;

	if (value.type == TOML_UNKNOWN)
		return;
	if (value.type != TOML_ARRAY)
	{
		platform_log("mods: %s line %d: %s is not a list of mods", path, value.lineno, key);
		return;
	}
	names->names = value.u.arr.size ? calloc((size_t)value.u.arr.size, sizeof(*names->names)) : NULL;
	for (index = 0; names->names && index < value.u.arr.size && index < MAXIMUM_MODS; index++)
	{
		if (value.u.arr.elem[index].type != TOML_STRING)
		{
			platform_log("mods: %s line %d: an entry of %s is not a string", path, value.u.arr.elem[index].lineno, key);
			continue;
		}
		names->names[names->count] = strdup(value.u.arr.elem[index].u.s);
		if (names->names[names->count])
			names->count++;
	}
}

static void files_dispose(void)
{
	long index;

	for (index = 0; index < port_mods.file_count; index++)
	{
		free(port_mods.files[index].key);
		free(port_mods.files[index].path);
	}
	free(port_mods.files);
	port_mods.files = NULL;
	port_mods.file_count = 0;
	port_mods.file_capacity = 0;
	for (index = 0; index < FILE_BUCKET_COUNT; index++)
		port_mods.buckets[index] = -1;
}

/* text cut short where it ends in part of a UTF-8 character */
static void utf8_trim(char *text)
{
	size_t length = strlen(text);
	size_t lead = length;
	unsigned char first;
	size_t need;

	while (lead > 0 && length - lead < 3 && ((unsigned char)text[lead - 1] & 0xC0) == 0x80)
		lead--;
	if (!lead)
		return;
	first = (unsigned char)text[--lead];
	need = first >= 0xF0 ? 4 : first >= 0xE0 ? 3 : first >= 0xC0 ? 2 : 1;
	if (length - lead < need)
		text[lead] = 0;
}

/* text as mod.toml has it, whole UTF-8 characters, and its control
characters (but its line breaks, if `lines`) spaces: what the menus, the
message box and the web page's dialog show of it */
static void text_clean(char *text, int lines)
{
	utf8_trim(text);
	for (; *text; text++)
	{
		if (((unsigned char)*text < 0x20 && !(lines && *text == '\n')) || *text == 0x7F)
			*text = ' ';
	}
}

static void text_copy(char *destination, size_t size, const char *source)
{
	snprintf(destination, size, "%s", source ? source : "");
	text_clean(destination, 0);
}

/* a string key of mod.toml, or NULL (and logged when it is not a string) */
static const char *toml_text(toml_datum_t table, const char *key, const char *path)
{
	toml_datum_t value = toml_get(table, key);

	if (value.type == TOML_UNKNOWN)
		return NULL;
	if (value.type != TOML_STRING)
	{
		platform_log("mods: %s line %d: %s is not a string", path, value.lineno, key);
		return NULL;
	}
	return value.u.s;
}

/* reads the mod.toml of the mod in `folder`; 0 when there is none */
static int mod_read(struct port_mod *mod, const char *root, const char *folder)
{
	static const char *const keys[] =
	{
		"name", "id", "version", "authors", "description", "maps", "load_after", "load_before", "incompatible", "format",
	};
	char path[MAXIMUM_PATH_LENGTH];
	size_t size = 0;
	char *text;
	toml_result_t result;
	toml_datum_t value;
	int index;

	memset(mod, 0, sizeof(*mod));
	mod->order = -1;
	text_copy(mod->folder, sizeof(mod->folder), folder);
	text_copy(mod->id, sizeof(mod->id), folder);
	text_copy(mod->name, sizeof(mod->name), folder);
	snprintf(path, sizeof(path), "%s/%s/mod.toml", root, folder);
	text = config_file_read(path, &size);
	if (!text)
	{
		platform_log("mods: %s/%s has no mod.toml: it is not a mod", root, folder);
		return 0;
	}
	result = toml_parse(text, (int)size);
	free(text);
	if (!result.ok)
	{
		platform_log("mods: %s: %s; it is played without what it says", path, result.errmsg);
		toml_free(result);
		return 1;
	}

	text_copy(mod->name, sizeof(mod->name), toml_text(result.toptab, "name", path));
	if (!mod->name[0])
		text_copy(mod->name, sizeof(mod->name), folder);
	text_copy(mod->id, sizeof(mod->id), toml_text(result.toptab, "id", path));
	if (!mod->id[0])
		text_copy(mod->id, sizeof(mod->id), folder);
	toml_names(result.toptab, "load_after", path, &mod->load_after);
	toml_names(result.toptab, "load_before", path, &mod->load_before);
	toml_names(result.toptab, "incompatible", path, &mod->incompatible);
	text_copy(mod->version, sizeof(mod->version), toml_text(result.toptab, "version", path));
	snprintf(mod->description, sizeof(mod->description), "%s",
		toml_text(result.toptab, "description", path) ? toml_text(result.toptab, "description", path) : "");
	text_clean(mod->description, 1);
	value = toml_get(result.toptab, "authors");
	if (value.type == TOML_STRING)
	{
		text_copy(mod->authors, sizeof(mod->authors), value.u.s);
	}
	else if (value.type == TOML_ARRAY)
	{
		for (index = 0; index < value.u.arr.size; index++)
		{
			size_t length = strlen(mod->authors);

			if (value.u.arr.elem[index].type == TOML_STRING)
				snprintf(mod->authors + length, sizeof(mod->authors) - length, "%s%s", length ? ", " : "",
					value.u.arr.elem[index].u.s);
		}
	}
	else if (value.type != TOML_UNKNOWN)
	{
		platform_log("mods: %s line %d: authors is not a string or a list of them", path, value.lineno);
	}
	text_clean(mod->authors, 0);
	value = toml_get(result.toptab, "maps");
	if (value.type == TOML_ARRAY && value.u.arr.size > 0)
	{
		long count = value.u.arr.size < MAXIMUM_MAPS_PER_MOD ? value.u.arr.size : MAXIMUM_MAPS_PER_MOD;

		if (value.u.arr.size > MAXIMUM_MAPS_PER_MOD)
			platform_log("mods: %s line %d: more than %d maps: those after them are left out", path, value.lineno,
				MAXIMUM_MAPS_PER_MOD);
		mod->maps = calloc((size_t)count, sizeof(*mod->maps));
		for (index = 0; mod->maps && index < count; index++)
		{
			if (value.u.arr.elem[index].type != TOML_STRING)
			{
				platform_log("mods: %s line %d: a map is not a string", path, value.u.arr.elem[index].lineno);
				continue;
			}
			mod->maps[mod->map_count] = strdup(value.u.arr.elem[index].u.s);
			if (mod->maps[mod->map_count])
				mod->map_count++;
		}
		/* (a list of nothing but mistakes is for no map) */
		if (!mod->map_count)
			mod->refusal = "its list of maps names none";
	}
	else if (value.type != TOML_UNKNOWN && value.type != TOML_ARRAY)
	{
		platform_log("mods: %s line %d: maps is not a list of maps", path, value.lineno);
	}
	value = toml_get(result.toptab, "format");
	if (value.type == TOML_INT64 && value.u.int64 > MODS_FORMAT)
	{
		platform_log("mods: %s is made for a later build (format %lld): it is not played", path,
			(long long)value.u.int64);
		mod->refusal = "made for a later build";
	}
	else if (value.type != TOML_UNKNOWN && value.type != TOML_INT64)
	{
		platform_log("mods: %s line %d: format is not a number", path, value.lineno);
	}
	for (index = 0; index < result.toptab.u.tab.size; index++)
	{
		size_t key;

		for (key = 0; key < sizeof(keys) / sizeof(keys[0]); key++)
		{
			if (!strcmp(result.toptab.u.tab.key[index], keys[key]))
				break;
		}
		if (key == sizeof(keys) / sizeof(keys[0]))
			platform_log("mods: %s line %d: unknown key %s", path, result.toptab.u.tab.value[index].lineno,
				result.toptab.u.tab.key[index]);
	}
	toml_free(result);
	return 1;
}

/* the mods in the data root's mods folder, in their folders' order */
static void mods_find(const char *root)
{
	void *directory = posix_directory_open(root);
	char name[256];

	port_mods.mod_count = 0;
	if (!directory)
		return;
	while (posix_directory_next(directory, name, sizeof(name)))
	{
		char path[MAXIMUM_PATH_LENGTH];
		struct posix_file_information information;
		struct port_mod mod;
		long index;

		snprintf(path, sizeof(path), "%s/%s", root, name);
		if (name[0] == '.' || posix_stat(path, &information) != 0 || !(information.flags & _posix_file_is_directory))
			continue;
		if (strlen(name) >= MAXIMUM_MOD_FOLDER_LENGTH)
		{
			platform_log("mods: the folder name %s is longer than %d characters: it is not a mod", name,
				MAXIMUM_MOD_FOLDER_LENGTH - 1);
			continue;
		}
		if (port_mods.mod_count == MAXIMUM_MODS)
		{
			platform_log("mods: more than %d folders in %s: %s and those after it are not mods", MAXIMUM_MODS, root,
				name);
			break;
		}
		if (!mod_read(&mod, root, name))
			continue;
		/* (sorted by folder, alphabetically: the default load order, the
		same on every system) */
		for (index = port_mods.mod_count; index > 0 && names_compare(port_mods.mods[index - 1].folder, mod.folder) > 0;
			index--)
			port_mods.mods[index] = port_mods.mods[index - 1];
		port_mods.mods[index] = mod;
		port_mods.mod_count++;
	}
	posix_directory_close(directory);
}

/* the mod `name` names (its id or folder), or -1 */
static long mod_find(const char *name)
{
	long index;

	for (index = 0; index < port_mods.mod_count; index++)
	{
		if (mod_named(&port_mods.mods[index], name))
			return index;
	}
	return -1;
}

/* whether the mod `first` asks to load before the mod `second`: `first`
names it in load_before, or `second` names `first` in load_after */
static int mod_wants_before(long first, long second)
{
	const struct port_mod *one = &port_mods.mods[first];
	const struct port_mod *other = &port_mods.mods[second];

	return first != second && (names_name(&one->load_before, other) || names_name(&other->load_after, one));
}

/* places `mod` in the default order (order_default), after the mods it
must load after, placed first, alphabetically */
static void order_place(long mod, char *state, long *order, long *count, int *ring_reported)
{
	long other;

	/* (1 being placed, 2 placed: a mod met again while it is being placed
	asks in a ring, which is left where it closes) */
	if (state[mod] == 2)
		return;
	if (state[mod] == 1)
	{
		if (!*ring_reported)
			platform_log("mods: %s and others ask to load before and after each other: they load alphabetically",
				port_mods.mods[mod].folder);
		*ring_reported = 1;
		return;
	}
	state[mod] = 1;
	for (other = 0; other < port_mods.mod_count; other++)
	{
		if (mod_wants_before(other, mod))
			order_place(other, state, order, count, ring_reported);
	}
	state[mod] = 2;
	order[(*count)++] = mod;
}

/* The default load order, into `order`: the mods alphabetically (as they
are found, mods_find), but with the mods each must load after moved up to
just before it. Mods that ask in a ring load alphabetically where it
closes. */
static void order_default(long *order)
{
	char state[MAXIMUM_MODS];
	long count = 0;
	long mod;
	int ring_reported = 0;

	memset(state, 0, sizeof(state));
	for (mod = 0; mod < port_mods.mod_count; mod++)
		order_place(mod, state, order, &count, &ring_reported);
}

static const char *order_file(const char *root, char *path, size_t size)
{
	snprintf(path, size, "%s/load_order.toml", root);
	return path;
}

/* Gives every mod its place in the load order and whether it is turned off:
the order load_order.toml keeps, each mod it does not name yet after the
mod before it in the default order (or first), and the default order with
no file. */
static void mods_order(const char *root)
{
	char path[MAXIMUM_PATH_LENGTH];
	long defaults[MAXIMUM_MODS];
	char placed[MAXIMUM_MODS];
	struct mod_names kept = { NULL, 0 };
	struct mod_names disabled = { NULL, 0 };
	size_t size = 0;
	char *text;
	long count = 0;
	long index;

	order_default(defaults);
	order_file(root, path, sizeof(path));
	text = port_mods.unsaved_order ? strdup(port_mods.unsaved_order) : config_file_read(path, &size);
	if (text && port_mods.unsaved_order)
		size = strlen(text);
	if (text)
	{
		toml_result_t result = toml_parse(text, (int)size);

		free(text);
		if (result.ok)
		{
			toml_names(result.toptab, "order", path, &kept);
			toml_names(result.toptab, "disabled", path, &disabled);
		}
		else
		{
			platform_log("mods: %s: %s; the mods load in their default order", path, result.errmsg);
		}
		toml_free(result);
	}
	memset(placed, 0, sizeof(placed));
	for (index = 0; index < kept.count; index++)
	{
		long mod = -1;
		long other;

		for (other = 0; other < port_mods.mod_count && mod < 0; other++)
		{
			if (names_equal(port_mods.mods[other].folder, kept.names[index]))
				mod = other;
		}
		if (mod >= 0 && !placed[mod])
		{
			placed[mod] = 1;
			port_mods.ordered[count++] = mod;
		}
	}
	/* (the mods it does not name, where the default order puts them: after
	the one before them there that is placed, but not after a placed mod
	they must load before, nor before one they must load after) */
	for (index = 0; index < port_mods.mod_count; index++)
	{
		long mod = defaults[index];
		long at = 0;
		long before;
		long other;

		if (placed[mod])
			continue;
		for (before = index - 1; before >= 0; before--)
		{
			if (!placed[defaults[before]])
				continue;
			for (other = 0; other < count; other++)
			{
				if (port_mods.ordered[other] == defaults[before])
					at = other + 1;
			}
			break;
		}
		for (other = 0; other < count; other++)
		{
			if (mod_wants_before(mod, port_mods.ordered[other]) && at > other)
				at = other;
		}
		for (other = count - 1; other >= 0; other--)
		{
			if (mod_wants_before(port_mods.ordered[other], mod) && at <= other)
			{
				at = other + 1;
				break;
			}
		}
		for (other = count; other > at; other--)
			port_mods.ordered[other] = port_mods.ordered[other - 1];
		port_mods.ordered[at] = mod;
		placed[mod] = 1;
		count++;
	}
	for (index = 0; index < port_mods.mod_count; index++)
	{
		struct port_mod *mod = &port_mods.mods[port_mods.ordered[index]];
		long other;

		mod->position = index;
		mod->disabled = 0;
		for (other = 0; other < disabled.count; other++)
		{
			if (names_equal(mod->folder, disabled.names[other]))
				mod->disabled = 1;
		}
	}
	names_dispose(&kept);
	names_dispose(&disabled);
}

/* whether a mod is on (whatever the map) */
static int mod_on(const struct port_mod *mod)
{
	return !mod->disabled && !mod->refusal;
}

/* the first mod that is on and names `mod` as incompatible, or -1 */
static long mod_incompatible_with(long mod)
{
	long index;

	for (index = 0; index < port_mods.mod_count; index++)
	{
		if (index != mod && mod_on(&port_mods.mods[index]) &&
			names_name(&port_mods.mods[index].incompatible, &port_mods.mods[mod]))
		{
			return index;
		}
	}
	return -1;
}

/* the first mod that is on that `mod` (on) loads on the wrong side of, as
it asks itself (load_before, load_after), or -1; *before says which side it
asks for */
static long mod_out_of_order_with(long mod, int *before)
{
	const struct port_mod *one = &port_mods.mods[mod];
	long index;

	if (!mod_on(one))
		return -1;
	for (index = 0; index < port_mods.mod_count; index++)
	{
		const struct port_mod *other = &port_mods.mods[index];

		if (index == mod || !mod_on(other))
			continue;
		if (names_name(&one->load_before, other) && one->position > other->position)
		{
			*before = 1;
			return index;
		}
		if (names_name(&one->load_after, other) && one->position < other->position)
		{
			*before = 0;
			return index;
		}
	}
	return -1;
}

/* a mod's folder name as a TOML basic string's characters, into `text`
(its control characters escaped, which TOML reads back as they were) */
static void toml_string_append(char *text, size_t size, const char *name)
{
	size_t length = strlen(text);

	for (; *name && length + TOML_CHARACTER_LENGTH < size; name++)
	{
		unsigned char character = (unsigned char)*name;

		if (character < 0x20 || character == 0x7F)
		{
			snprintf(text + length, size - length, "\\u%04X", character);
			length += 6;
			continue;
		}
		if (character == '"' || character == '\\')
			text[length++] = '\\';
		text[length++] = (char)character;
	}
	text[length] = 0;
}

/* writes `text` to the file `path`, whole: to a file beside it first, which
then takes its place (so that a write cut short leaves the file as it was) */
static int text_file_write(const char *path, const char *text)
{
	char temporary[MAXIMUM_PATH_LENGTH];
	int written;
#ifdef HALO_ARM64_GUEST
	FILE *file;
#endif

	if (snprintf(temporary, sizeof(temporary), "%s.new", path) >= (int)sizeof(temporary))
		return 0;
#ifdef HALO_ARM64_GUEST
	/* (the guest's own C library: SDL's are the host's) */
	file = fopen(temporary, "wb");
	if (!file)
		return 0;
	written = fwrite(text, 1, strlen(text), file) == strlen(text);
	written = fclose(file) == 0 && written;
	if (written)
		written = rename(temporary, path) == 0;
	if (!written)
		remove(temporary);
#else
	written = SDL_SaveFile(temporary, text, strlen(text));
	if (written)
		written = SDL_RenamePath(temporary, path);
	if (!written)
		SDL_RemovePath(temporary);
#endif
	return written;
}

static long file_add(long mod, const char *key, const char *path)
{
	struct port_mod_file *file;

	if (port_mods.file_count == MAXIMUM_MOD_FILES)
	{
		if (!port_mods.full_reported)
			platform_log("mods: more than %d files: %s and those after it are left out", MAXIMUM_MOD_FILES, path);
		port_mods.full_reported = 1;
		return -1;
	}
	if (port_mods.file_count == port_mods.file_capacity)
	{
		long capacity = port_mods.file_capacity ? port_mods.file_capacity * 2 : 1024;
		struct port_mod_file *grown = realloc(port_mods.files, (size_t)capacity * sizeof(*grown));

		if (!grown)
		{
			platform_log("mods: out of memory for the list of their files");
			return -1;
		}
		port_mods.files = grown;
		port_mods.file_capacity = capacity;
	}
	file = &port_mods.files[port_mods.file_count];
	file->key = strdup(key);
	file->path = strdup(path);
	if (!file->key || !file->path)
	{
		free(file->key);
		free(file->path);
		platform_log("mods: out of memory for the list of their files");
		return -1;
	}
	file->mod = mod;
	file->hash = key_hash(key);
	file->next = port_mods.buckets[file->hash % FILE_BUCKET_COUNT];
	port_mods.buckets[file->hash % FILE_BUCKET_COUNT] = port_mods.file_count;
	port_mods.mods[mod].file_count++;
	return port_mods.file_count++;
}

/* lists the files of the folder `path`, `key` their tag path's start */
static void files_list(long mod, const char *path, const char *key, int depth)
{
	void *directory;
	char name[256];

	if (depth > MAXIMUM_FOLDER_DEPTH)
	{
		platform_log("mods: %s is more than %d folders deep: its files are left out", path, MAXIMUM_FOLDER_DEPTH);
		return;
	}
	/* (no more once the files or folders are as many as are listed) */
	if (port_mods.full_reported)
		return;
	if (++port_mods.folder_count > MAXIMUM_MOD_FOLDERS)
	{
		platform_log("mods: more than %d folders: %s and those after it are left out", MAXIMUM_MOD_FOLDERS, path);
		port_mods.full_reported = 1;
		return;
	}
	directory = posix_directory_open(path);
	if (!directory)
		return;
	while (posix_directory_next(directory, name, sizeof(name)))
	{
		char child_path[MAXIMUM_PATH_LENGTH];
		char child_key[MAXIMUM_PATH_LENGTH];
		struct posix_file_information information;
		int fits;

		if (name[0] == '.')
			continue;
		fits = snprintf(child_path, sizeof(child_path), "%s/%s", path, name) < (int)sizeof(child_path) &&
			snprintf(child_key, sizeof(child_key), "%s%s%s", key, *key ? "\\" : "", name) < (int)sizeof(child_key) &&
			key_make(child_key, child_key, sizeof(child_key));
		if (!fits)
		{
			platform_log("mods: the path of %s/%s is too long: it is left out", path, name);
			continue;
		}
		if (posix_stat(child_path, &information) != 0)
			continue;
		if (information.flags & _posix_file_is_directory)
			files_list(mod, child_path, child_key, depth + 1);
		else
			file_add(mod, child_key, child_path);
	}
	posix_directory_close(directory);
}

static int mod_for_map(const struct port_mod *mod, const char *map_name)
{
	long index;

	if (!mod->maps)
		return 1;
	for (index = 0; index < mod->map_count; index++)
	{
		if (map_name && names_equal(mod->maps[index], map_name))
			return 1;
	}
	return 0;
}

static const char *mods_root(char *path, size_t size)
{
	snprintf(path, size, "%s/mods", platform_data_root());
	return path;
}

/* ---------- public code */

long port_mods_scan(void)
{
	char root[MAXIMUM_PATH_LENGTH];
	long mod_index;

	for (mod_index = 0; mod_index < port_mods.mod_count; mod_index++)
		mod_dispose(&port_mods.mods[mod_index]);
	files_dispose();
	port_mods.full_reported = 0;
	port_mods.folder_count = 0;
	port_mods.played_count = 0;
	port_mods.scans++;
	mods_root(root, sizeof(root));
	mods_find(root);
	mods_order(root);
	for (mod_index = 0; mod_index < port_mods.mod_count; mod_index++)
	{
		struct port_mod *mod = &port_mods.mods[mod_index];
		char path[MAXIMUM_PATH_LENGTH];

		/* (those on play in their places in the load order) */
		mod->order = mod->disabled || mod->refusal ? -1 : mod->position;
		if (mod->order < 0)
			continue;
		port_mods.played_count++;
		snprintf(path, sizeof(path), "%s/%s/tags", root, mod->folder);
		files_list(mod_index, path, "", 0);
		platform_log("mods: %s (%s%s%s) plays %ld tag files%s", mod->folder, mod->name, mod->version[0] ? " " : "",
			mod->version, mod->file_count, mod->maps ? " on the maps it names" : "");
	}
	if (port_mods.mod_count)
	{
		char order[1024] = "";

		for (mod_index = 0; mod_index < port_mods.mod_count; mod_index++)
		{
			const struct port_mod *mod = &port_mods.mods[port_mods.ordered[mod_index]];
			size_t length = strlen(order);

			snprintf(order + length, sizeof(order) - length, "%s%s%s", mod_index ? ", " : "", mod->folder,
				mod->disabled ? " (off)" : mod->refusal ? " (not played)" : "");
		}
		platform_log("mods: load order %s", order);
	}
	return port_mods.played_count;
}

long port_mods_file_find(const char *tag_file, const char *map_name)
{
	char key[MAXIMUM_PATH_LENGTH];
	unsigned long hash;
	long found = -1;
	long index;

	if (!port_mods.file_count || !tag_file || !key_make(tag_file, key, sizeof(key)))
		return -1;
	hash = key_hash(key);
	for (index = port_mods.buckets[hash % FILE_BUCKET_COUNT]; index >= 0; index = port_mods.files[index].next)
	{
		const struct port_mod_file *file = &port_mods.files[index];
		const struct port_mod *mod = &port_mods.mods[file->mod];

		if (file->hash == hash && !strcmp(file->key, key) && mod_for_map(mod, map_name) &&
			(found < 0 || mod->order > port_mods.mods[port_mods.files[found].mod].order))
		{
			found = index;
		}
	}
	return found;
}

int port_mods_file_stamp(long file, unsigned long *size, unsigned long stamp[2])
{
	struct posix_file_information information;

	if (file < 0 || file >= port_mods.file_count || posix_stat(port_mods.files[file].path, &information) != 0 ||
		information.size_high)
	{
		return 0;
	}
	*size = information.size_low;
	stamp[0] = information.modification_seconds;
	stamp[1] = information.modification_nanoseconds;
	return 1;
}

int port_mods_file_read(long file, unsigned long offset, void *buffer, unsigned long size)
{
	const char *path;
	int bounce = platform_is_contiguous(buffer);
	unsigned char *staging = NULL;
	unsigned long done = 0;
	int read = 1;
#ifdef HALO_ARM64_GUEST
	FILE *stream;
#else
	SDL_IOStream *stream;
#endif

	if (file < 0 || file >= port_mods.file_count)
		return 0;
	path = port_mods.files[file].path;
	if (bounce && !(staging = malloc(READ_BOUNCE_SIZE)))
		return 0;
#ifdef HALO_ARM64_GUEST
	stream = fopen(path, "rb");
	if (!stream || fseek(stream, (long)offset, SEEK_SET) != 0)
		read = 0;
#else
	/* (SDL's, for UTF-8 paths on Windows) */
	stream = SDL_IOFromFile(path, "rb");
	if (!stream || SDL_SeekIO(stream, (Sint64)offset, SDL_IO_SEEK_SET) < 0)
		read = 0;
#endif
	while (read && done < size)
	{
		unsigned long wanted = size - done;
		unsigned char *destination = (unsigned char *)buffer + done;

		if (bounce && wanted > READ_BOUNCE_SIZE)
			wanted = READ_BOUNCE_SIZE;
#ifdef HALO_ARM64_GUEST
		read = fread(bounce ? staging : destination, 1, wanted, stream) == wanted;
#else
		read = SDL_ReadIO(stream, bounce ? staging : destination, wanted) == wanted;
#endif
		if (read && bounce)
		{
			memory_watch_prepare_write(destination, wanted);
			memcpy(destination, staging, wanted);
		}
		done += wanted;
	}
	if (stream)
	{
#ifdef HALO_ARM64_GUEST
		fclose(stream);
#else
		SDL_CloseIO(stream);
#endif
	}
	free(staging);
	return read;
}

const char *port_mods_file_mod(long file)
{
	return file >= 0 && file < port_mods.file_count ? port_mods.mods[port_mods.files[file].mod].folder : "";
}

const char *port_mods_file_path(long file)
{
	return file >= 0 && file < port_mods.file_count ? port_mods.files[file].path : "";
}

long port_mods_count(void)
{
	return port_mods.mod_count;
}

const char *port_mods_folder(long index)
{
	return index >= 0 && index < port_mods.mod_count ? port_mods.mods[index].folder : "";
}

int port_mods_describe(long index, char *text, unsigned long size, const char *map_name)
{
	const struct port_mod *mod;

	if (index < 0 || index >= port_mods.mod_count)
		return 0;
	mod = &port_mods.mods[index];
	snprintf(text, size, "%s: %s%s%s%s%s, %s%s%s", mod->folder, mod->name, mod->version[0] ? " " : "",
		mod->version, mod->authors[0] ? " by " : "", mod->authors,
		mod->refusal ? "not played: " : mod->disabled ? "off" : !mod_for_map(mod, map_name) ? "on, not for this map" :
			"on",
		mod->refusal ? mod->refusal : "",
		mod->order < 0 || mod->file_count ? "" : " (no tags folder, or nothing in it)");
	return 1;
}

unsigned long port_mods_scans(void)
{
	return port_mods.scans;
}

long port_mods_played(void)
{
	return port_mods.played_count;
}

long port_mods_at(long position)
{
	return position >= 0 && position < port_mods.mod_count ? port_mods.ordered[position] : -1;
}

const char *port_mods_name(long index)
{
	return index >= 0 && index < port_mods.mod_count ? port_mods.mods[index].name : "";
}

int port_mods_on(long index)
{
	return index >= 0 && index < port_mods.mod_count && mod_on(&port_mods.mods[index]);
}

int port_mods_status(long index, char *text, unsigned long size)
{
	const struct port_mod *mod;
	long other;
	int before = 0;

	if (text && size)
		text[0] = 0;
	if (index < 0 || index >= port_mods.mod_count)
		return _port_mod_fine;
	mod = &port_mods.mods[index];
	if (mod->refusal)
	{
		if (text)
			snprintf(text, size, "It cannot be played: %s.", mod->refusal);
		return _port_mod_unplayable;
	}
	if ((other = mod_incompatible_with(index)) >= 0)
	{
		if (text)
			snprintf(text, size, "%s says it cannot be played with this mod.", port_mods.mods[other].name);
		return _port_mod_incompatible;
	}
	if ((other = mod_out_of_order_with(index, &before)) >= 0)
	{
		/* (as the Mods menu shows them: the mod loaded last at the top) */
		if (text)
			snprintf(text, size, "It asks to be %s %s, but is %s it.", before ? "below" : "above",
				port_mods.mods[other].name, before ? "above" : "below");
		return _port_mod_out_of_order;
	}
	return _port_mod_fine;
}

int port_mods_details(long index, char *text, unsigned long size)
{
	const struct port_mod *mod;
	size_t length;
	long map;

	if (!text || !size)
		return 0;
	text[0] = 0;
	if (index < 0 || index >= port_mods.mod_count)
		return 0;
	mod = &port_mods.mods[index];
	if (mod->version[0])
		snprintf(text, size, "Version %s\n", mod->version);
	length = strlen(text);
	if (mod->authors[0])
		snprintf(text + length, size - length, "By %s\n", mod->authors);
	length = strlen(text);
	snprintf(text + length, size - length, "Folder: %s\n", mod->folder);
	length = strlen(text);
	if (mod->maps)
	{
		snprintf(text + length, size - length, "For:");
		for (map = 0; map < mod->map_count; map++)
		{
			length = strlen(text);
			snprintf(text + length, size - length, "%s %s", map ? "," : "", mod->maps[map]);
		}
		length = strlen(text);
		snprintf(text + length, size - length, "\n");
	}
	length = strlen(text);
	if (mod->description[0])
		snprintf(text + length, size - length, "\n%s", mod->description);
	return 1;
}

long port_mods_move(long position, long places)
{
	long count = port_mods.mod_count;
	long target = position + places;
	long mod;
	long index;

	if (position < 0 || position >= count)
		return position;
	if (target < 0)
		target = 0;
	if (target >= count)
		target = count - 1;
	mod = port_mods.ordered[position];
	if (target > position)
	{
		for (index = position; index < target; index++)
			port_mods.ordered[index] = port_mods.ordered[index + 1];
	}
	else
	{
		for (index = position; index > target; index--)
			port_mods.ordered[index] = port_mods.ordered[index - 1];
	}
	port_mods.ordered[target] = mod;
	for (index = 0; index < count; index++)
		port_mods.mods[port_mods.ordered[index]].position = index;
	return target;
}

void port_mods_turn(long index, int on)
{
	if (index >= 0 && index < port_mods.mod_count)
		port_mods.mods[index].disabled = !on;
}

int port_mods_save(void)
{
	char root[MAXIMUM_PATH_LENGTH];
	char path[MAXIMUM_PATH_LENGTH];
	size_t size = 256 + (size_t)port_mods.mod_count * 2 * (TOML_CHARACTER_LENGTH * MAXIMUM_MOD_FOLDER_LENGTH + 4);
	char *text = malloc(size);
	long index;
	int first = 1;
	int written;

	if (!text)
		return 0;
	snprintf(text, size, "# The mods' load order (the first loads first, and each plays over those\n"
		"# before it: the Mods menu shows it the other way up), and the mods turned\n"
		"# off. The Mods menu writes this.\norder = [");
	for (index = 0; index < port_mods.mod_count; index++)
	{
		strcat(text, index ? ", \"" : "\"");
		toml_string_append(text, size, port_mods.mods[port_mods.ordered[index]].folder);
		strcat(text, "\"");
	}
	strcat(text, "]\ndisabled = [");
	for (index = 0; index < port_mods.mod_count; index++)
	{
		const struct port_mod *mod = &port_mods.mods[port_mods.ordered[index]];

		if (!mod->disabled)
			continue;
		strcat(text, first ? "\"" : ", \"");
		toml_string_append(text, size, mod->folder);
		strcat(text, "\"");
		first = 0;
	}
	strcat(text, "]\n");
	posix_make_directory(mods_root(root, sizeof(root)));
	written = text_file_write(order_file(root, path, sizeof(path)), text);
	/* (kept in memory when it cannot be written, so that the mods are still
	played as chosen until the game quits) */
	free(port_mods.unsaved_order);
	port_mods.unsaved_order = NULL;
	if (!written)
	{
		platform_log("mods: cannot write %s: the order and the mods turned off last until the game quits", path);
		port_mods.unsaved_order = text;
		return 0;
	}
	free(text);
	return 1;
}

void port_mods_revert(void)
{
	char root[MAXIMUM_PATH_LENGTH];

	mods_order(mods_root(root, sizeof(root)));
}

int port_mods_conflicts(char *text, unsigned long size)
{
	long index;
	long count = 0;

	if (!size)
		text = NULL;
	if (text)
		text[0] = 0;
	for (index = 0; index < port_mods.mod_count; index++)
	{
		long other = mod_on(&port_mods.mods[index]) ? mod_incompatible_with(index) : -1;

		if (other < 0)
			continue;
		if (text)
		{
			size_t length = strlen(text);

			snprintf(text + length, size - length, "%s%s says it cannot be played with %s.", count ? "\n" : "",
				port_mods.mods[other].name, port_mods.mods[index].name);
		}
		count++;
	}
	if (text)
		utf8_trim(text);
	return (int)count;
}

int port_mods_conflicting_names(char *text, unsigned long size)
{
	long index;
	long count = 0;

	if (!size)
		text = NULL;
	if (text)
		text[0] = 0;
	for (index = 0; index < port_mods.mod_count; index++)
	{
		if (!mod_on(&port_mods.mods[index]) || mod_incompatible_with(index) < 0)
			continue;
		if (text)
		{
			size_t length = strlen(text);

			snprintf(text + length, size - length, "%s%s", count ? ", " : "", port_mods.mods[index].name);
		}
		count++;
	}
	if (text)
		utf8_trim(text);
	return (int)count;
}

void port_mods_disable_conflicting(void)
{
	char marked[MAXIMUM_MODS];
	long index;

	/* (all of those named at once: turning one off may end what named
	another) */
	for (index = 0; index < port_mods.mod_count; index++)
		marked[index] = mod_on(&port_mods.mods[index]) && mod_incompatible_with(index) >= 0;
	for (index = 0; index < port_mods.mod_count; index++)
	{
		if (marked[index])
		{
			platform_log("mods: %s is turned off: a mod says it cannot be played with it", port_mods.mods[index].folder);
			port_mods.mods[index].disabled = 1;
		}
	}
}

void port_mods_startup(void)
{
	char conflicts[2048];
	char names[256];
	char message[2400];
	char disable[96];
	const char *labels[3];
	int answer;

	port_mods_scan();
	if (!port_mods_conflicts(conflicts, sizeof(conflicts)))
		return;
	port_mods_conflicting_names(names, sizeof(names));
	platform_log("mods: %s", conflicts);
	/* (not for runs nobody is watching, as platform_show_message's
	messages are not: they play them) */
	if (config_boolean("debug.hidden_window") || config_boolean("debug.null_renderer") ||
		config_real("debug.exit_after") > 0.0 || *config_string("debug.network_test"))
	{
		platform_log("mods: played as they are, nobody being asked");
		return;
	}
	snprintf(message, sizeof(message),
		"Mods in the mods folder say they cannot be played together:\n\n%s\n\n"
		"Play them all anyway, turn off %s, or quit? (The main menu's Mods turns mods on and off.)",
		conflicts, names);
	snprintf(disable, sizeof(disable), "Turn off %s", names);
	/* (a long list of names is cut short on its button, at a whole
	character) */
	if (strlen(disable) > 40)
	{
		disable[37] = 0;
		utf8_trim(disable);
		strcat(disable, "...");
	}
	labels[0] = "Play anyway";
	labels[1] = disable;
	labels[2] = "Quit";
	answer = platform_ask("Halo: mods that cannot be played together", message, labels, 3, 0);
	if (answer == 1)
	{
		/* (played so even when load_order.toml cannot be written:
		port_mods_save keeps it) */
		port_mods_disable_conflicting();
		port_mods_save();
		port_mods_scan();
	}
	else if (answer == 2)
	{
		platform_log("mods: quitting, as asked, over mods that cannot be played together");
#ifdef HALO_WEB
		/* (the page starts again: a worker's exit does not end the page's
		game) */
		web_js_post(4, "quit");
#endif
		exit(EXIT_SUCCESS);
	}
	else if (answer < 0)
	{
		platform_log("mods: cannot ask (%s): played all anyway", SDL_GetError());
	}
	else
	{
		platform_log("mods: played all anyway");
	}
}
