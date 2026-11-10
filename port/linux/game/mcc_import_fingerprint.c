/*
MCC_IMPORT_FINGERPRINT.C

The fingerprint of what the simulation reads in an Xbox map, made from the
map the MCC import writes (mcc_import.c), to compare with the disc's: equal,
the map is the disc's and plays with the disc's players (version 0); else it
is an MCC map (MCC_MAP_VERSION). halo --fingerprint <map> prints a map's,
the disc's or an imported one's, as port/assets/mcc/<map>.fp has the disc's
(mcc_import_fingerprint_print).

A canonical stream of the map's simulation data is hashed (SHA-256), in
sections, each with its own hash so that a difference can be found:

	ORDER     the disc's tags in index order: their group and path (tags of
	          the import's after them must not be simulated)
	TAG       each tag of a simulation group, walked with the tag schemas
	SCENARIO  the scenario, without its scripts' data and the editor's and
	          the BSPs' file fields
	SCRIPTS   a multiplayer scenario's scripts and globals as trees of tokens
	BSP       each structure BSP's collision, leaves, surfaces, clusters,
	          portals, breakable surfaces and pathfinding
	MAP       the hash of all of the above lines

In an element, the bytes of blocks, data, references and the like are zeroed
and written after it, a tag handle as the tag's group and path; strings are
cut at their terminator, and fields the game never reads are zeroed
(mcc_fingerprint_fields.h, which tools/mcc_fingerprint_fields.py writes from
the structures' layouts).

A map given to --fingerprint is any file: every count, offset and name of
it is checked before it is followed.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "tag_schema.h"
#include "mcc_import.h"
#include "mcc_sha256.h"
#include "memory/zlib/zlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcc_fingerprint_fields.h"

/* ---------- the schemas, by the names this code was first written with */

#define schema_definition tag_schema_definition
#define schema_field tag_schema_field
#define schema_group tag_schema_group
#define _schema_block _tag_schema_block
#define _schema_data _tag_schema_data
#define _schema_file_data _tag_schema_file_data
#define _schema_reference _tag_schema_reference
#define _schema_tag_index _tag_schema_tag_index
#define _schema_block_index _tag_schema_block_index
#define _schema_enum _tag_schema_enum
#define _schema_string _tag_schema_string
#define _schema_reset _tag_schema_reset
#define _schema_struct _tag_schema_struct
#define _schema_check _tag_schema_check
#define _schema_terminator _tag_schema_terminator

/* ---------- the lines */

static struct
{
	char *text;
	unsigned long length;
	unsigned long size;
} mcc_fingerprint_lines;

static void mcc_fingerprint_line(
	char const *line)
{
	unsigned long length = 0;

	while (line[length])
		length++;
	if (mcc_fingerprint_lines.length + length + 2 > mcc_fingerprint_lines.size)
	{
		unsigned long size = (mcc_fingerprint_lines.size + length + 2) * 2;
		char *grown = malloc(size);

		if (!grown)
			return;
		if (mcc_fingerprint_lines.text)
		{
			csmemcpy(grown, mcc_fingerprint_lines.text, mcc_fingerprint_lines.length);
			free(mcc_fingerprint_lines.text);
		}
		mcc_fingerprint_lines.text = grown;
		mcc_fingerprint_lines.size = size;
	}
	csmemcpy(mcc_fingerprint_lines.text + mcc_fingerprint_lines.length, line, length);
	mcc_fingerprint_lines.length += length;
	mcc_fingerprint_lines.text[mcc_fingerprint_lines.length++] = 10;
	mcc_fingerprint_lines.text[mcc_fingerprint_lines.length] = 0;

	return;
}

/* ---------- the scope */

static unsigned long const SIMULATION_GROUPS[] =
{
	'weap', 'proj', 'eqip', 'garb', 'bipd', 'vehi', 'scen', 'mach', 'ctrl', 'lifi', 'plac', 'ssce',
	'jpt!', 'coll', 'phys', 'antr', 'matg', 'itmc', 'actr', 'actv', 'effe', 'mode', 0
};

/* fields left out wherever they are (by the schema's field name) */
static char const *const EXCLUDED_FIELDS[] =
{
	"predicted_resources",
	/* a model's drawing */
	"geometries", "shaders",
	/* the scenario: its scripts (SCRIPTS), the editor's data, the BSPs' file
	fields (BSP), drawing */
	"hs_syntax_data", "hs_string_constants", "hs_scripts", "hs_globals", "hs_references", "source_files",
	"editor_scenario_data", "comments", "structure_bsps", "structure_bsp_references", "decals", "decal_palette",
	"detail_object_collection_palette", "skies",
	/* the globals' drawing data */
	"rasterizer_data", "first_person_interface", "interface_bitmaps",
	NULL
};

/* a BSP's fields in the fingerprint (its root's) */
/* a model's fields in the fingerprint: what the simulation reads of it (its
geometry and shaders are drawing; a gbxmodel has the same root, struct model) */
static char const *const MODEL_FIELDS[] =
{
	"markers", "nodes", "regions", NULL
};

/* the disc's tag count: ORDER hashes that many */
static long order_limit;

static char const *const BSP_FIELDS[] =
{
	"collision_bsp", "collision_materials", "nodes", "leaves", "surface_references", "clusters",
	"cluster_portals", "breakable_surfaces", "pathfinding_surfaces", "pathfinding_edges", "markers", NULL
};

static int name_in(char const *name, char const *const *list)
{
	int i;
	for (i = 0; name && list[i]; i++)
		if (!strcmp(name, list[i]))
			return 1;
	return 0;
}

static int simulation_group(unsigned long group)
{
	int i;
	for (i = 0; SIMULATION_GROUPS[i]; i++)
		if (SIMULATION_GROUPS[i] == group)
			return 1;
	return 0;
}

/* ---------- the map */

#define XBOX_TAG_ADDRESS 0x803A6000UL

struct space
{
	unsigned long address;
	unsigned char *bytes;
	unsigned long size;
};

struct tag
{
	unsigned long group;
	char const *name;
	unsigned long address;
	unsigned long handle;
};

static struct
{
	unsigned char *file;
	unsigned long file_size;
	struct space tags;
	struct space bsp;
	int in_bsp;
	struct tag *table;
	long count;
	long scenario;
} map;

static unsigned long u32(void const *p)
{
	unsigned char const *b = p;
	return (unsigned long)b[0] | (unsigned long)b[1] << 8 | (unsigned long)b[2] << 16 | (unsigned long)b[3] << 24;
}

static unsigned char *translate(unsigned long address, unsigned long size)
{
	struct space *spaces[2];
	int i;

	spaces[0] = map.in_bsp ? &map.bsp : &map.tags;
	spaces[1] = &map.tags;
	for (i = 0; i < 2; i++)
	{
		struct space *s = spaces[i];
		if (address >= s->address && address - s->address <= s->size && size <= s->size - (address - s->address))
			return s->bytes + (address - s->address);
	}
	return NULL;
}

static long handle_index(unsigned long word)
{
	long index = (long)(word & 0xFFFF);
	if (word == 0xFFFFFFFFUL || index >= map.count || map.table[index].handle != word)
		return -1;
	return index;
}

static struct schema_group const *schema_get(unsigned long group)
{
	int i, j;
	for (i = 0; tag_schema_group_lists[i]; i++)
		for (j = 0; tag_schema_group_lists[i][j].group_tag; j++)
			if (tag_schema_group_lists[i][j].group_tag == group)
				return &tag_schema_group_lists[i][j];
	return NULL;
}

/* `count` elements of `size` bytes at `address`, or NULL (as translate), also
when their size does not fit in 32 bits: a count of a map's is untrusted */
static unsigned char *translate_array(unsigned long address, unsigned long count, unsigned long size)
{
	if (size && count > 0xFFFFFFFFUL / size)
		return NULL;
	return translate(address, count * size);
}

/* the hex of the first `count` bytes of `digest` into `hex` (2 * count + 1) */
static void digest_hex(unsigned char const *digest, int count, char *hex)
{
	int i;

	for (i = 0; i < count; i++)
		snprintf(hex + i * 2, 3, "%02x", digest[i]);
}

/* ---------- the stream */

static struct mcc_sha256 section;
static struct mcc_sha256 whole;

static void put(void const *bytes, long size)
{
	mcc_sha256_add(&section, bytes, (int)size);
}

static void put_text(char const *text)
{
	put(text, (long)strlen(text) + 1);
}

static void put_tag(unsigned long handle)
{
	char text[400];
	long index = handle_index(handle);

	if (handle == 0xFFFFFFFFUL)
		strcpy(text, "NONE");
	else if (index < 0)
		strcpy(text, "BAD");
	else
	{
		unsigned long g = map.table[index].group;
		snprintf(text, sizeof(text), "%c%c%c%c:%s", (int)(g >> 24), (int)(g >> 16 & 0xFF), (int)(g >> 8 & 0xFF),
			(int)(g & 0xFF), map.table[index].name);
	}
	put_text(text);
}

static void section_begin(char const *kind, char const *name)
{
	(void)kind;
	(void)name;
	mcc_sha256_begin(&section);
}

static void section_end(char const *kind, char const *name)
{
	unsigned char digest[32];
	char line[600], hex[33];

	mcc_sha256_end(&section, digest);
	digest_hex(digest, 16, hex);
	snprintf(line, sizeof(line), "%s\t%s\t%s", kind, name ? name : "", hex);
	mcc_fingerprint_line(line);
	mcc_sha256_add(&whole, line, (int)strlen(line));
}

/* ---------- the walk */

static void walk_element(struct schema_definition const *d, unsigned char *element,
	char const *const *only_fields);

static void walk_fields(struct schema_definition const *d, unsigned char *element, char const *const *only_fields)
{
	struct schema_field const *f;
	short n;

	for (f = d->fields; f->type != _schema_terminator; f++)
	{
		if (f->type == _schema_check || name_in(f->name, EXCLUDED_FIELDS))
			continue;
		if (only_fields && !name_in(f->name, only_fields))
			continue;
		for (n = 0; n < f->count; n++)
		{
			unsigned char *p = element + f->offset + n * f->size;

			switch (f->type)
			{
			case _schema_block:
			{
				struct schema_definition const *e = f->definition;
				long count = (long)u32(p), i;
				unsigned char *elements;

				put(&count, 4);
				if (count <= 0)
					break;
				elements = translate_array(u32(p + 4), (unsigned long)count, (unsigned long)e->size);
				if (!elements)
				{
					put_text("OUTSIDE");
					break;
				}
				for (i = 0; i < count; i++)
					walk_element(e, elements + i * e->size, NULL);
				break;
			}
			case _schema_data:
			{
				long size = (long)u32(p);
				unsigned char *bytes;

				put(&size, 4);
				if (size <= 0)
					break;
				bytes = translate(u32(p + 12), (unsigned long)size);
				if (bytes)
					put(bytes, size);
				else
					put_text("OUTSIDE");
				break;
			}
			case _schema_file_data:
				put(p, 4);
				break;
			case _schema_reference:
				put_tag(u32(p + 12));
				break;
			case _schema_tag_index:
				put_tag(u32(p));
				break;
			case _schema_struct:
				walk_element(f->definition, p, NULL);
				break;
			default:
				break;
			}
		}
	}
}

/* how deep the walk is in blocks and structures: the schemas nest a few
levels, a map's blocks that name themselves for ever */
static int walk_depth;

enum
{
	MAXIMUM_WALK_DEPTH = 64
};

static void walk_element(struct schema_definition const *d, unsigned char *element, char const *const *only_fields)
{
	unsigned char *bytes;
	struct schema_field const *f;
	long o;
	short n;

	if (walk_depth >= MAXIMUM_WALK_DEPTH)
	{
		put_text("TOO DEEP");
		return;
	}
	bytes = malloc((size_t)d->size + 4);
	if (!bytes)
	{
		put_text("NO MEMORY");
		return;
	}
	memcpy(bytes, element, (size_t)d->size);
	for (f = d->fields; f->type != _schema_terminator; f++)
	{
		for (n = 0; n < f->count; n++)
		{
			long start = f->offset + n * f->size;
			long length = f->size;
			if (start < 0 || start >= d->size)
				continue;
			/* (a structure may run past a definition cut short, as the
			control tag's is: masked up to the end) */
			if (start + length > d->size)
			{
				if (f->type != _schema_struct)
					continue;
				length = d->size - start;
			}
			switch (f->type)
			{
			case _schema_block:
			case _schema_data:
			case _schema_file_data:
			case _schema_reference:
			case _schema_tag_index:
			case _schema_reset:
			case _schema_struct:
				memset(bytes + start, 0, (size_t)length);
				break;
			case _schema_string:
			{
				unsigned char *end = memchr(bytes + start, 0, (size_t)f->size);
				if (end)
					memset(end, 0, (size_t)(bytes + start + f->size - end));
				break;
			}
			default:
				break;
			}
		}
	}
	/* strings the schemas don't list (string_fields.h, from the structures'
	layouts): cut at the terminator */
	{
		int k;
		for (k = 0; string_fields[k].definition; k++)
		{
			unsigned char *start, *end;
			if (string_fields[k].definition_size != d->size || strcmp(string_fields[k].definition, d->name) ||
				string_fields[k].offset + string_fields[k].size > d->size)
				continue;
			start = bytes + string_fields[k].offset;
			end = memchr(start, 0, (size_t)string_fields[k].size);
			if (end)
				memset(end, 0, (size_t)(start + string_fields[k].size - end));
		}
	}
	/* fields the game never reads (ignored_fields.h: unused, pad): zeroed */
	{
		int k;
		for (k = 0; ignored_fields[k].definition; k++)
		{
			if (ignored_fields[k].definition_size != d->size || strcmp(ignored_fields[k].definition, d->name) ||
				ignored_fields[k].offset + ignored_fields[k].size > d->size)
				continue;
			memset(bytes + ignored_fields[k].offset, 0, (size_t)ignored_fields[k].size);
		}
	}
	/* tag handles no schema lists (written after as the tag's path). (No
	guess at pointers: ignored_fields.h names every pointer the schemas
	don't, from the layouts; a word of packed shorts can look like one) */
	for (o = 0; o + 4 <= d->size; o += 4)
	{
		unsigned long w = u32(bytes + o);
		if (!w || w == 0xFFFFFFFFUL)
			continue;
		if (handle_index(w) >= 0)
			memset(bytes + o, 0, 4);
	}
	if (!only_fields)
		put(bytes, d->size);
	for (o = 0; o + 4 <= d->size; o += 4)
	{
		unsigned long w = u32(element + o);
		if (w && w != 0xFFFFFFFFUL && handle_index(w) >= 0 && u32(bytes + o) == 0)
		{
			/* (only words the masks above left: a handle no schema lists) */
			int listed = 0;
			for (f = d->fields; f->type != _schema_terminator && !listed; f++)
				for (n = 0; n < f->count; n++)
					if (o >= f->offset + n * f->size && o < f->offset + (n + 1) * f->size &&
						f->type != _schema_block_index && f->type != _schema_enum && f->type != _schema_string &&
						f->type != _schema_check)
						listed = 1;
			if (!listed)
				put_tag(w);
		}
	}
	free(bytes);
	walk_depth++;
	walk_fields(d, element, only_fields);
	walk_depth--;
}

/* ---------- the scripts: trees of tokens */

static unsigned char *hs_nodes;
static long hs_node_count;
static unsigned char *hs_strings;
static long hs_strings_size;

static void hs_text(long offset)
{
	if (offset < 0 || offset >= hs_strings_size)
		put_text("?");
	else
	{
		char buffer[512];
		long n = 0;
		while (offset + n < hs_strings_size && hs_strings[offset + n] && n < 511)
		{
			buffer[n] = (char)hs_strings[offset + n];
			n++;
		}
		buffer[n] = 0;
		put_text(buffer);
	}
}

static void hs_tree(unsigned long index, int depth)
{
	unsigned char *node;
	short type, flags;
	unsigned long data;
	long string_offset;

	if (index == 0xFFFFFFFFUL || depth > 400 || (long)(index & 0xFFFF) >= hs_node_count)
	{
		put_text("NONE");
		return;
	}
	node = hs_nodes + (index & 0xFFFF) * 0x14;
	type = (short)(u32(node + 4) & 0xFFFF);
	flags = (short)(u32(node + 4) >> 16);
	string_offset = (long)u32(node + 0x0C);
	data = u32(node + 0x10);
	if (!(u32(node) & 0xFFFF))
	{
		put_text("EMPTY");
		return;
	}
	if (!(flags & 1))
	{
		unsigned long child = data;
		int guard = 0;
		put_text("(");
		while (child != 0xFFFFFFFFUL && guard++ < 10000 && (long)(child & 0xFFFF) < hs_node_count)
		{
			hs_tree(child, depth + 1);
			child = u32(hs_nodes + (child & 0xFFFF) * 0x14 + 8);
		}
		put_text(")");
		return;
	}
	put(&type, 2);
	if ((flags & 6) || type == 2 || type == 10 || type == 9)
		hs_text(string_offset);
	else if (type >= 20 && type <= 27)
		put_tag(data); /* (sound .. object_definition: hs.h) */
	else if (type == 5)
	{
		unsigned char value = (unsigned char)(data & 0xFF);
		put(&value, 1);
	}
	else if (type == 7)
	{
		unsigned short value = (unsigned short)(data & 0xFFFF);
		put(&value, 2);
	}
	else if (type == 6 || type == 8)
		put(&data, 4);
	else
	{
		unsigned short value = (unsigned short)(data & 0xFFFF);
		put(&value, 2);
	}
}

static void scripts(unsigned char *scenario)
{
	unsigned long syntax_size = u32(scenario + 0x474);
	unsigned char *syntax = syntax_size >= 0x38 ? translate(u32(scenario + 0x474 + 12), syntax_size) : NULL;
	unsigned long count, i;
	unsigned char *elements;

	/* only multiplayer scenarios (type 1): in network co-op only the host
	runs scripts (game.c: clients skip hs_update), so a campaign map's
	scripts are not part of what the machines must agree on */
	if ((u32(scenario + 0x3C) & 0xFFFF) != 1)
		return;
	hs_strings_size = (long)u32(scenario + 0x488);
	hs_strings = hs_strings_size ? translate(u32(scenario + 0x488 + 12), (unsigned long)hs_strings_size) : NULL;
	if (!hs_strings)
		hs_strings_size = 0;
	hs_nodes = syntax ? syntax + 0x38 : NULL;
	hs_node_count = syntax ? (short)(u32(syntax + 0x2C) >> 16) : 0;
	/* (no more than the data has room for) */
	hs_node_count = MIN(hs_node_count, syntax ? (long)((syntax_size - 0x38) / 0x14) : 0);
	section_begin("SCRIPTS", NULL);
	count = u32(scenario + 0x49C);
	elements = count ? translate_array(u32(scenario + 0x49C + 4), count, 0x5C) : NULL;
	for (i = 0; elements && i < count; i++)
	{
		unsigned char *script = elements + i * 0x5C;
		char name[33];
		memcpy(name, script, 32);
		name[32] = 0;
		put_text(name);
		put(script + 32, 4);
		hs_tree(u32(script + 0x24), 0);
	}
	count = u32(scenario + 0x4A8);
	elements = count ? translate_array(u32(scenario + 0x4A8 + 4), count, 0x5C) : NULL;
	for (i = 0; elements && i < count; i++)
	{
		unsigned char *global = elements + i * 0x5C;
		char name[33];
		memcpy(name, global, 32);
		name[32] = 0;
		put_text(name);
		put(global + 32, 2);
		hs_tree(u32(global + 0x28), 0);
	}
	section_end("SCRIPTS", NULL);
}

/* ---------- a map */

/* the structure BSPs, in the scenario's order (the import's, or a map file's:
mcc_import_fingerprint_print) */
static struct mcc_fingerprint_bsp const *mcc_fingerprint_bsps;
static long mcc_fingerprint_bsp_count;

/* BSP `b` of the scenario's structure_bsps into `space` (its address, bytes
and size): nonzero when it was given */
static int mcc_fingerprint_bsp_get(
	unsigned long b,
	struct space *space)
{
	if ((long)b >= mcc_fingerprint_bsp_count || !mcc_fingerprint_bsps[b].bytes)
		return 0;
	space->address = mcc_fingerprint_bsps[b].address;
	space->size = mcc_fingerprint_bsps[b].size;
	space->bytes = mcc_fingerprint_bsps[b].bytes;
	return 1;
}

/* the map fingerprinted, for the log */
static char const *fingerprint_map_name = "";

/* the fingerprint of the map in `map` (its tags, table and scenario filled) */
static void fingerprint_map(void)
{
	unsigned char *scenario;
	struct schema_group const *scenario_schema, *bsp_schema;
	long i;
	unsigned char digest[32];
	char line[200];

	if (!map.table)
	{
		fprintf(stderr, "mcc import: %s: the tags' index is not in the tags\n", fingerprint_map_name);
		return;
	}
	mcc_sha256_begin(&whole);
	section_begin("ORDER", NULL);
	/* the disc's tags (order_limit of them, when the map has more): an
	import writes after them tags of its own that something not simulated
	still names (bitmaps, shaders, widgets), which the network never names;
	a simulated one there fails the order */
	for (i = 0; i < map.count; i++)
	{
		struct tag *t = &map.table[i];
		if (order_limit && i >= order_limit)
		{
			if (simulation_group(t->group))
			{
				fprintf(stderr, "mcc import: %s: a simulated tag after the disc's: %s\n", fingerprint_map_name, t->name);
				put_text("SIMULATED TAG AFTER THE DISC'S");
			}
			continue;
		}
		put(&t->group, 4);
		put_text(t->name);
	}
	section_end("ORDER", NULL);

	/* in the disc's order where there is one: the importer writes only those
	tags, so a tag MCC added (not in the order) is not in the map */
	for (i = 0; i < map.count; i++)
	{
		struct tag *t = &map.table[i];
		struct schema_group const *schema;
		unsigned char *root;
		char name[300];

		if (!simulation_group(t->group))
			continue;
		schema = schema_get(t->group);
		snprintf(name, sizeof(name), "%c%c%c%c:%s", (int)(t->group >> 24), (int)(t->group >> 16 & 0xFF),
			(int)(t->group >> 8 & 0xFF), (int)(t->group & 0xFF), t->name);
		if (!schema || !schema->definition)
		{
			fprintf(stderr, "mcc import: %s: no schema for %s\n", fingerprint_map_name, name);
			continue;
		}
		root = translate(t->address, (unsigned long)schema->definition->size);
		if (!root)
			continue;
		section_begin("TAG", name);
		walk_element(schema->definition, root, t->group == 'mode' ? MODEL_FIELDS : NULL);
		section_end("TAG", name);
	}

	scenario_schema = schema_get('scnr');
	scenario = scenario_schema && scenario_schema->definition ?
		translate(map.table[map.scenario].address, (unsigned long)scenario_schema->definition->size) : NULL;
	if (!scenario)
	{
		fprintf(stderr, "mcc import: %s: the scenario is not in the tags\n", fingerprint_map_name);
		return;
	}
	section_begin("SCENARIO", map.table[map.scenario].name);
	walk_element(scenario_schema->definition, scenario, NULL);
	section_end("SCENARIO", map.table[map.scenario].name);
	scripts(scenario);

	bsp_schema = schema_get('sbsp');
	{
		unsigned long count = u32(scenario + 0x5A4);
		unsigned char *references = translate_array(u32(scenario + 0x5A8), count, 0x20);
		unsigned long b;

		for (b = 0; references && b < count; b++)
		{
			unsigned char *reference = references + b * 0x20;
			long index = handle_index(u32(reference + 0x1C));
			unsigned char *root;
			char const *name = index >= 0 ? map.table[index].name : "?";

			if (!mcc_fingerprint_bsp_get(b, &map.bsp))
			{
				fprintf(stderr, "mcc import: %s: structure bsp %s unreadable\n", fingerprint_map_name, name);
				continue;
			}
			map.in_bsp = 1;
			root = translate(u32(map.bsp.bytes), (unsigned long)bsp_schema->definition->size);
			section_begin("BSP", name);
			if (root)
				walk_element(bsp_schema->definition, root, BSP_FIELDS);
			section_end("BSP", name);
			map.in_bsp = 0;
		}
	}

	mcc_sha256_end(&whole, digest);
	{
		char hex[65];

		digest_hex(digest, 32, hex);
		snprintf(line, sizeof(line), "MAP	%s	%s", map.table[map.scenario].name, hex);
	}
	mcc_fingerprint_line(line);
}

/* fills map.table from the tags' header (at map.tags) */
static void map_table_read(void)
{
	unsigned char *rows;
	long i;

	map.in_bsp = 0;
	/* (the game's free does not take NULL) */
	if (map.table)
		free(map.table);
	map.table = NULL;
	if (map.tags.size < 16)
	{
		map.count = 0;
		return;
	}
	map.count = (long)u32(map.tags.bytes + 12);
	map.scenario = (long)(u32(map.tags.bytes + 4) & 0xFFFF);
	/* (a map's tags are at most 65,535, and its rows in its tags) */
	rows = map.count > 0 && map.count <= 0xFFFF ? translate(u32(map.tags.bytes), (unsigned long)map.count * 0x20) : NULL;
	if (!rows || map.scenario >= map.count)
	{
		map.count = 0;
		return;
	}
	/* (the game's malloc, which the game's free takes: calloc is the C
	library's) */
	map.table = malloc((size_t)map.count * sizeof(struct tag) + 1);
	if (!map.table)
	{
		map.count = 0;
		return;
	}
	csmemset(map.table, 0, (unsigned long)map.count * sizeof(struct tag));
	for (i = 0; i < map.count; i++)
	{
		unsigned char *row = rows + i * 0x20;
		unsigned char *name = translate(u32(row + 0x10), 1);
		map.table[i].group = u32(row);
		map.table[i].handle = u32(row + 0x0C);
		/* (a name ends in the tags) */
		map.table[i].name = name && memchr(name, 0, map.tags.size - (unsigned long)(name - map.tags.bytes)) ?
			(char const *)name : "?";
		map.table[i].address = u32(row + 0x14);
	}
}

/* ---------- public code */

char const *mcc_import_fingerprint(
	char const *map_name,
	void *tag_data,
	unsigned long tag_data_size,
	long disc_tag_count,
	struct mcc_fingerprint_bsp const *bsps,
	long bsp_count)
{
	if (mcc_fingerprint_lines.text)
		free(mcc_fingerprint_lines.text);
	csmemset(&mcc_fingerprint_lines, 0, sizeof(mcc_fingerprint_lines));
	csmemset(&map, 0, sizeof(map));
	map.tags.address = XBOX_TAG_ADDRESS;
	map.tags.bytes = tag_data;
	map.tags.size = tag_data_size;
	map_table_read();
	fingerprint_map_name = map_name;
	order_limit = disc_tag_count;
	mcc_fingerprint_bsps = bsps;
	mcc_fingerprint_bsp_count = bsp_count;
	fingerprint_map();
	if (map.table)
		free(map.table);
	map.table = NULL;

	return mcc_fingerprint_lines.text ? mcc_fingerprint_lines.text : "";
}

/* a part of a map file as the game reads it (its body inflated): `size`
bytes from `offset`, into `bytes` */
struct fingerprint_range
{
	unsigned long offset;
	unsigned long size;
	unsigned char *bytes;
};

enum
{
	FINGERPRINT_HEADER_BYTES = 0x800,
	/* (the largest of the Xbox's cache files, a campaign level's) */
	FINGERPRINT_MAXIMUM_MAP_BYTES = 0x11600000,
	FINGERPRINT_MAXIMUM_BSPS = 32,
	FINGERPRINT_CHUNK_BYTES = 0x10000
};

/* the ranges of the map file `file` (`length` bytes as the game reads it;
its body deflated when `deflated`) copied out, reading it once, never whole:
a campaign level is 275 MB, more than a 32-bit process may have in one
piece. TRUE when every range was in it */
static boolean fingerprint_ranges_read(
	FILE *file,
	unsigned long length,
	boolean deflated,
	struct fingerprint_range *ranges,
	short count)
{
	static unsigned char input[FINGERPRINT_CHUNK_BYTES], output[FINGERPRINT_CHUNK_BYTES];
	z_stream stream;
	unsigned long position = FINGERPRINT_HEADER_BYTES, end = 0;
	short index;
	int status = Z_OK;

	for (index = 0; index < count; index++)
	{
		if (ranges[index].offset < FINGERPRINT_HEADER_BYTES || ranges[index].offset > length ||
			ranges[index].size > length - ranges[index].offset)
		{
			return FALSE;
		}
		end = MAX(end, ranges[index].offset + ranges[index].size);
	}
	if (!deflated)
	{
		for (index = 0; index < count; index++)
		{
			if (fseek(file, (long)ranges[index].offset, SEEK_SET) ||
				fread(ranges[index].bytes, 1, ranges[index].size, file) != ranges[index].size)
			{
				return FALSE;
			}
		}
		return TRUE;
	}
	csmemset(&stream, 0, sizeof(stream));
	if (fseek(file, FINGERPRINT_HEADER_BYTES, SEEK_SET) || inflateInit(&stream) != Z_OK)
		return FALSE;
	while (position < end && status == Z_OK)
	{
		unsigned long produced;

		if (!stream.avail_in)
		{
			stream.avail_in = (uInt)fread(input, 1, sizeof(input), file);
			stream.next_in = input;
			if (!stream.avail_in)
				break;
		}
		stream.next_out = output;
		stream.avail_out = sizeof(output);
		status = inflate(&stream, Z_NO_FLUSH);
		if (status != Z_OK && status != Z_STREAM_END)
			break;
		produced = sizeof(output) - stream.avail_out;
		/* (each range's part of what this chunk holds) */
		for (index = 0; index < count; index++)
		{
			struct fingerprint_range *range = &ranges[index];
			unsigned long from = MAX(position, range->offset);
			unsigned long to = MIN(position + produced, range->offset + range->size);

			if (from < to)
				csmemcpy(range->bytes + (from - range->offset), output + (from - position), to - from);
		}
		position += produced;
	}
	inflateEnd(&stream);

	return position >= end;
}

int mcc_import_fingerprint_print(
	char const *map_name)
{
	char path[256];
	unsigned char header[FINGERPRINT_HEADER_BYTES];
	struct fingerprint_range tags = { 0, 0, NULL };
	struct fingerprint_range bsp_ranges[FINGERPRINT_MAXIMUM_BSPS];
	struct mcc_fingerprint_bsp bsps[FINGERPRINT_MAXIMUM_BSPS];
	short bsp_count = 0, index;
	unsigned long length, file_size;
	boolean deflated;
	FILE *file;
	int success = 0;

	csmemset(bsp_ranges, 0, sizeof(bsp_ranges));
	csmemset(bsps, 0, sizeof(bsps));
	snprintf(path, sizeof(path), "d:\\maps\\%s.map", map_name);
	file = fopen(path, "rb");
	if (!file || fread(header, 1, sizeof(header), file) != sizeof(header) || u32(header + 4) != 5)
	{
		fprintf(stderr, "--fingerprint: %s is not an Xbox map (cache version 5)\n", path);
		if (file)
			fclose(file);
		return 1;
	}
	fseek(file, 0, SEEK_END);
	file_size = (unsigned long)ftell(file);
	/* (the body deflated after the header, as the disc's and the import's
	maps are, or not) */
	length = u32(header + 8);
	deflated = file_size != length;
	tags.offset = u32(header + 0x10);
	tags.size = u32(header + 0x14);
	if (length < FINGERPRINT_HEADER_BYTES || length > FINGERPRINT_MAXIMUM_MAP_BYTES || tags.size < 16 ||
		tags.offset > length || tags.size > length - tags.offset ||
		(tags.bytes = malloc(tags.size + 1)) == NULL || !fingerprint_ranges_read(file, length, deflated, &tags, 1))
	{
		fprintf(stderr, "--fingerprint: %s's tags cannot be read from it\n", path);
		goto done;
	}
	/* the BSPs its scenario names, where its references say */
	csmemset(&map, 0, sizeof(map));
	map.tags.address = XBOX_TAG_ADDRESS;
	map.tags.bytes = tags.bytes;
	map.tags.size = tags.size;
	map_table_read();
	if (map.table)
	{
		struct schema_group const *scenario_schema = schema_get('scnr');
		unsigned char *scenario = scenario_schema && scenario_schema->definition ?
			translate(map.table[map.scenario].address, (unsigned long)scenario_schema->definition->size) : NULL;
		unsigned long count = scenario ? u32(scenario + 0x5A4) : 0;
		unsigned char *references = count && count <= FINGERPRINT_MAXIMUM_BSPS ?
			translate(u32(scenario + 0x5A8), count * 0x20) : NULL;

		for (index = 0; references && index < (short)count; index++)
		{
			unsigned char *reference = references + index * 0x20;

			bsp_ranges[index].offset = u32(reference);
			bsp_ranges[index].size = u32(reference + 4);
			bsps[index].address = u32(reference + 8);
			bsps[index].size = bsp_ranges[index].size;
			if (bsp_ranges[index].size && bsp_ranges[index].size < FINGERPRINT_MAXIMUM_MAP_BYTES)
				bsp_ranges[index].bytes = malloc(bsp_ranges[index].size + 1);
			if (!bsp_ranges[index].bytes)
				break;
			bsps[index].bytes = bsp_ranges[index].bytes;
			bsp_count = index + 1;
		}
		/* (read again by mcc_import_fingerprint) */
		free(map.table);
		map.table = NULL;
	}
	if (bsp_count && !fingerprint_ranges_read(file, length, deflated, bsp_ranges, bsp_count))
	{
		fprintf(stderr, "--fingerprint: %s's structure bsps cannot be read from it\n", path);
		goto done;
	}
	{
		/* (every tag the disc's, a disc's map's) */
		char const *lines = mcc_import_fingerprint(map_name, tags.bytes, tags.size, 0, bsps, bsp_count);

		success = lines[0] != 0;
		fputs(lines, stdout);
		if (!success)
			fprintf(stderr, "--fingerprint: %s has no fingerprint (the messages above say why)\n", path);
	}
done:
	fclose(file);
	if (tags.bytes)
		free(tags.bytes);
	for (index = 0; index < bsp_count; index++)
	{
		if (bsp_ranges[index].bytes)
			free(bsp_ranges[index].bytes);
	}

	return success ? 0 : 1;
}
