/*
PORT_TAG_PACKS.C

The tag packs' manifests (port_tag_packs.h), read once from the files
tools/embed_assets.py embedded from port/assets/tag_packs. Each is

	name = "<owner>_<what>"           its file's name, and maps/<name>.pack
	version = 1                       raised by hand when the keep rules
	                                  mean something else than they say
	legacy_file_names = ["old.pack"]  renamed to the pack's at start-up
	maps = ["c40"]                    map files, at most 32, 31 letters
	minimum_bitmaps = 1               a map with fewer is not read right
	minimum_sounds = 0
	wanted = ["always"]               or "setting:<key>", "condition:<name>"

	[keep]                            whose pixels and samples are kept,
	include_contains = ["flood"]      by the tag's name (any case): when
	include_prefixes = ["effects\\"]  there are includes, one of them;
	exclude_contains = []             and none of the excludes
	exclude_bitmap_prefixes = []
	exclude_sound_prefixes = []

	[text]                            the main menu's words for it
	notice = "..."
	building = "..."
	ready = "..."
	failed = "..."

What a pack holds is hashed (port_tag_packs_content) from its version, maps,
minimums and keep rules, in that order, each lower-cased: tools/tag_pack.py
hashes the same text, so a pack it writes is one the game reads.
*/

#include "platform.h"
#include "port_tag_packs.h"
#include "tomlc17.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXIMUM_TAG_PACKS 32
#define MAXIMUM_NAME_LENGTH 64
#define MAXIMUM_LEGACY 4
#define MAXIMUM_WANTED 8
#define MAXIMUM_RULES 32
#define MAXIMUM_RULE_LENGTH 128
#define MAXIMUM_TEXT_LENGTH 256

enum
{
	RULE_INCLUDE_CONTAINS,
	RULE_INCLUDE_PREFIXES,
	RULE_EXCLUDE_CONTAINS,
	RULE_EXCLUDE_BITMAP_PREFIXES,
	RULE_EXCLUDE_SOUND_PREFIXES,
	RULE_LISTS
};

static const char *const rule_keys[RULE_LISTS] =
{
	"include_contains", "include_prefixes", "exclude_contains", "exclude_bitmap_prefixes", "exclude_sound_prefixes",
};

enum
{
	TEXT_NOTICE,
	TEXT_BUILDING,
	TEXT_READY,
	TEXT_FAILED,
	TEXTS
};

static const char *const text_keys[TEXTS] = { "notice", "building", "ready", "failed" };

struct rule_list
{
	char rules[MAXIMUM_RULES][MAXIMUM_RULE_LENGTH];
	long count;
};

struct tag_pack
{
	char name[MAXIMUM_NAME_LENGTH];
	long version;
	char maps[PORT_TAG_PACK_MAPS][PORT_TAG_PACK_MAP_LENGTH];
	long map_count;
	char legacy[MAXIMUM_LEGACY][MAXIMUM_NAME_LENGTH + 8];
	long legacy_count;
	char wanted[MAXIMUM_WANTED][MAXIMUM_NAME_LENGTH];
	long wanted_count;
	long minimum_bitmaps;
	long minimum_sounds;
	struct rule_list rules[RULE_LISTS];
	char texts[TEXTS][MAXIMUM_TEXT_LENGTH];
	unsigned long content;
};

static struct tag_pack *packs;
static long pack_count;
static int packs_read;

/* ---------- reading */

static void lower_copy(char *destination, size_t size, const char *source)
{
	size_t index;

	for (index = 0; source[index] && index + 1 < size; index++)
		destination[index] = (char)tolower((unsigned char)source[index]);
	destination[index] = 0;
}

static int name_usable(const char *name)
{
	const char *at;

	if (!name[0] || !strchr(name, '_') || name[0] == '_')
		return 0;
	for (at = name; *at; at++)
	{
		if (!(*at >= 'a' && *at <= 'z') && !(*at >= '0' && *at <= '9') && *at != '_')
			return 0;
	}
	return 1;
}

/* the strings of an array key, lower-cased, at most `maximum` of `length`;
how many, or -1 when the key is there and is not an array of strings */
static long strings_read(toml_datum_t table, const char *key, const char *path, char *into, long maximum,
	size_t length, int lower)
{
	toml_datum_t value = toml_get(table, key);
	long count = 0;
	int index;

	if (value.type == TOML_UNKNOWN)
		return 0;
	if (value.type != TOML_ARRAY)
	{
		platform_log("tag packs: %s line %d: %s is not a list", path, value.lineno, key);
		return -1;
	}
	for (index = 0; index < value.u.arr.size; index++)
	{
		toml_datum_t item = value.u.arr.elem[index];

		if (item.type != TOML_STRING || !item.u.s[0] || strlen(item.u.s) >= length)
		{
			platform_log("tag packs: %s line %d: an entry of %s is not a string of at most %u letters", path,
				item.lineno, key, (unsigned)length - 1);
			return -1;
		}
		if (count >= maximum)
		{
			platform_log("tag packs: %s: more than %ld in %s", path, maximum, key);
			return -1;
		}
		if (lower)
			lower_copy(into + count * length, length, item.u.s);
		else
			snprintf(into + count * length, length, "%s", item.u.s);
		count++;
	}
	return count;
}

static long integer_read(toml_datum_t table, const char *key, long otherwise)
{
	toml_datum_t value = toml_get(table, key);

	return value.type == TOML_INT64 && value.u.int64 >= 0 && value.u.int64 < 0x7FFFFFFF ? (long)value.u.int64 :
		otherwise;
}

/* ---------- what a pack holds */

static unsigned long hash_text(unsigned long hash, const char *text)
{
	for (; *text; text++)
		hash = ((hash ^ (unsigned char)*text) * 16777619UL) & 0xFFFFFFFFUL;
	return hash;
}

static unsigned long content_hash(const struct tag_pack *pack)
{
	char line[MAXIMUM_RULE_LENGTH + 64];
	unsigned long hash = 0x811C9DC5UL;
	long index;
	int list;

	snprintf(line, sizeof(line), "rpak2\nversion=%ld\n", pack->version);
	hash = hash_text(hash, line);
	for (index = 0; index < pack->map_count; index++)
	{
		snprintf(line, sizeof(line), "map=%s\n", pack->maps[index]);
		hash = hash_text(hash, line);
	}
	snprintf(line, sizeof(line), "minimum_bitmaps=%ld\nminimum_sounds=%ld\n", pack->minimum_bitmaps,
		pack->minimum_sounds);
	hash = hash_text(hash, line);
	for (list = 0; list < RULE_LISTS; list++)
	{
		for (index = 0; index < pack->rules[list].count; index++)
		{
			snprintf(line, sizeof(line), "%s=%s\n", rule_keys[list], pack->rules[list].rules[index]);
			hash = hash_text(hash, line);
		}
	}
	/* (never 0, which a pack's header would read as none) */
	return hash ? hash : 1;
}

/* a manifest; 0 when it is not one */
static int pack_read(struct tag_pack *pack, const char *path, const char *text, size_t size)
{
	char stem[MAXIMUM_NAME_LENGTH];
	const char *leaf = strrchr(path, '/');
	toml_result_t result;
	toml_datum_t value;
	toml_datum_t table;
	size_t length;
	int index;
	int good = 1;

	memset(pack, 0, sizeof(*pack));
	leaf = leaf ? leaf + 1 : path;
	length = strlen(leaf);
	if (length < 6 || strcmp(leaf + length - 5, ".toml") || length - 5 >= sizeof(stem))
		return 0;
	memcpy(stem, leaf, length - 5);
	stem[length - 5] = 0;
	result = toml_parse(text, (int)size);
	if (!result.ok)
	{
		platform_log("tag packs: %s: %s; it is left out", path, result.errmsg);
		toml_free(result);
		return 0;
	}
	value = toml_get(result.toptab, "name");
	if (value.type != TOML_STRING || strcmp(value.u.s, stem) || !name_usable(value.u.s))
	{
		platform_log("tag packs: %s: its name is not its file's, of lower-case letters, digits and _, with "
			"its owner's first (owner_what); it is left out", path);
		toml_free(result);
		return 0;
	}
	snprintf(pack->name, sizeof(pack->name), "%s", value.u.s);
	pack->version = integer_read(result.toptab, "version", 1);
	pack->minimum_bitmaps = integer_read(result.toptab, "minimum_bitmaps", 0);
	pack->minimum_sounds = integer_read(result.toptab, "minimum_sounds", 0);
	pack->map_count = strings_read(result.toptab, "maps", path, pack->maps[0], PORT_TAG_PACK_MAPS,
		PORT_TAG_PACK_MAP_LENGTH, 1);
	pack->legacy_count = strings_read(result.toptab, "legacy_file_names", path, pack->legacy[0], MAXIMUM_LEGACY,
		sizeof(pack->legacy[0]), 0);
	pack->wanted_count = strings_read(result.toptab, "wanted", path, pack->wanted[0], MAXIMUM_WANTED,
		sizeof(pack->wanted[0]), 0);
	if (pack->map_count <= 0 || pack->legacy_count < 0 || pack->wanted_count < 0)
	{
		if (!pack->map_count)
			platform_log("tag packs: %s names no maps; it is left out", path);
		good = 0;
	}
	table = toml_get(result.toptab, "keep");
	for (index = 0; good && index < RULE_LISTS; index++)
	{
		pack->rules[index].count = table.type == TOML_TABLE ? strings_read(table, rule_keys[index], path,
			pack->rules[index].rules[0], MAXIMUM_RULES, MAXIMUM_RULE_LENGTH, 1) : 0;
		good = pack->rules[index].count >= 0;
	}
	table = toml_get(result.toptab, "text");
	for (index = 0; good && index < TEXTS; index++)
	{
		value = table.type == TOML_TABLE ? toml_get(table, text_keys[index]) : table;
		if (value.type == TOML_STRING)
			snprintf(pack->texts[index], sizeof(pack->texts[index]), "%s", value.u.s);
	}
	toml_free(result);
	if (!good)
	{
		platform_log("tag packs: %s is left out", path);
		return 0;
	}
	pack->content = content_hash(pack);
	return 1;
}

static void packs_read_once(void)
{
	unsigned int index;

	if (packs_read)
		return;
	packs_read = 1;
	packs = calloc(MAXIMUM_TAG_PACKS, sizeof(*packs));
	if (!packs)
		return;
	for (index = 0; index < tag_pack_manifests_embedded_count && pack_count < MAXIMUM_TAG_PACKS; index++)
	{
		const struct tag_pack_manifest_embedded *embedded = &tag_pack_manifests_embedded[index];
		long other;

		if (!embedded->path || !pack_read(&packs[pack_count], embedded->path, (const char *)embedded->data,
			embedded->size))
			continue;
		for (other = 0; other < pack_count && strcmp(packs[other].name, packs[pack_count].name); other++)
		{
		}
		if (other < pack_count)
		{
			platform_log("tag packs: %s is named twice; the second is left out", packs[pack_count].name);
			continue;
		}
		platform_log("tag packs: %s, %ld maps, content %08lx", packs[pack_count].name, packs[pack_count].map_count,
			packs[pack_count].content);
		pack_count++;
	}
}

/* ---------- asking */

static const struct tag_pack *pack_get(long pack)
{
	packs_read_once();
	return pack >= 0 && pack < pack_count ? &packs[pack] : NULL;
}

long port_tag_packs_count(void)
{
	packs_read_once();
	return pack_count;
}

const char *port_tag_packs_name(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->name : "";
}

long port_tag_packs_find(const char *name)
{
	long index;

	packs_read_once();
	for (index = 0; name && index < pack_count; index++)
	{
		if (!strcmp(packs[index].name, name))
			return index;
	}
	return -1;
}

unsigned long port_tag_packs_content(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->content : 0;
}

long port_tag_packs_map_count(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->map_count : 0;
}

const char *port_tag_packs_map(long pack, long index)
{
	const struct tag_pack *got = pack_get(pack);

	return got && index >= 0 && index < got->map_count ? got->maps[index] : "";
}

long port_tag_packs_legacy_count(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->legacy_count : 0;
}

const char *port_tag_packs_legacy(long pack, long index)
{
	const struct tag_pack *got = pack_get(pack);

	return got && index >= 0 && index < got->legacy_count ? got->legacy[index] : "";
}

long port_tag_packs_minimum_bitmaps(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->minimum_bitmaps : 0;
}

long port_tag_packs_minimum_sounds(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->minimum_sounds : 0;
}

long port_tag_packs_wanted_count(long pack)
{
	const struct tag_pack *got = pack_get(pack);

	return got ? got->wanted_count : 0;
}

const char *port_tag_packs_wanted(long pack, long index)
{
	const struct tag_pack *got = pack_get(pack);

	return got && index >= 0 && index < got->wanted_count ? got->wanted[index] : "";
}

static int rules_match(const struct rule_list *list, const char *lower, int prefixes)
{
	long index;

	for (index = 0; index < list->count; index++)
	{
		if (prefixes ? !strncmp(lower, list->rules[index], strlen(list->rules[index])) :
			strstr(lower, list->rules[index]) != NULL)
			return 1;
	}
	return 0;
}

int port_tag_packs_keep(long pack, unsigned long group, const char *name)
{
	const struct tag_pack *got = pack_get(pack);
	char lower[256];

	if (!got || !name)
		return 0;
	lower_copy(lower, sizeof(lower), name);
	if ((got->rules[RULE_INCLUDE_CONTAINS].count || got->rules[RULE_INCLUDE_PREFIXES].count) &&
		!rules_match(&got->rules[RULE_INCLUDE_CONTAINS], lower, 0) &&
		!rules_match(&got->rules[RULE_INCLUDE_PREFIXES], lower, 1))
		return 0;
	if (rules_match(&got->rules[RULE_EXCLUDE_CONTAINS], lower, 0))
		return 0;
	if (group == 0x6269746DUL /* 'bitm' */)
		return !rules_match(&got->rules[RULE_EXCLUDE_BITMAP_PREFIXES], lower, 1);
	return !rules_match(&got->rules[RULE_EXCLUDE_SOUND_PREFIXES], lower, 1);
}

const char *port_tag_packs_text(long pack, const char *which)
{
	const struct tag_pack *got = pack_get(pack);
	int index;

	for (index = 0; got && which && index < TEXTS; index++)
	{
		if (!strcmp(which, text_keys[index]))
			return got->texts[index];
	}
	return "";
}
