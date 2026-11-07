/*
MCC_IMPORT_FINGERPRINT.C

The fingerprint of what the simulation reads in an Xbox map, made from the
map the MCC import writes (mcc_import.c), to compare with the disc's: equal,
the map is the disc's and plays with the disc's players (version 0); else it
is an MCC map (MCC_MAP_VERSION). A prototype's code (.local/mcc_check/
sim_fingerprint.c, which makes the disc's), brought over as it is.

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
(mcc_fingerprint_fields.h, generated from the structures' layouts).
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "tag_schema.h"
#include "mcc_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcc_fingerprint_fields.h"

/* ---------- the schemas, by the prototype's names */

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

/* ---------- SHA-256 (as port/linux/src/p2p_crypto.c) */

struct sha256
{
	unsigned int state[8];
	unsigned char block[64];
	unsigned long long length;
	int used;
};

static const unsigned int sha256_constants[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

#define ROTATE_RIGHT(value, count) (((value) >> (count)) | ((value) << (32 - (count))))

static void sha256_block(struct sha256 *context, const unsigned char *block)
{
	unsigned int words[64];
	unsigned int a, b, c, d, e, f, g, h;
	int index;

	for (index = 0; index < 16; index++)
	{
		words[index] = (unsigned int)block[index * 4] << 24 | (unsigned int)block[index * 4 + 1] << 16 |
			(unsigned int)block[index * 4 + 2] << 8 | (unsigned int)block[index * 4 + 3];
	}
	for (index = 16; index < 64; index++)
	{
		unsigned int s0 = ROTATE_RIGHT(words[index - 15], 7) ^ ROTATE_RIGHT(words[index - 15], 18) ^
			(words[index - 15] >> 3);
		unsigned int s1 = ROTATE_RIGHT(words[index - 2], 17) ^ ROTATE_RIGHT(words[index - 2], 19) ^
			(words[index - 2] >> 10);

		words[index] = words[index - 16] + s0 + words[index - 7] + s1;
	}
	a = context->state[0]; b = context->state[1]; c = context->state[2]; d = context->state[3];
	e = context->state[4]; f = context->state[5]; g = context->state[6]; h = context->state[7];
	for (index = 0; index < 64; index++)
	{
		unsigned int s1 = ROTATE_RIGHT(e, 6) ^ ROTATE_RIGHT(e, 11) ^ ROTATE_RIGHT(e, 25);
		unsigned int choice = (e & f) ^ (~e & g);
		unsigned int first = h + s1 + choice + sha256_constants[index] + words[index];
		unsigned int s0 = ROTATE_RIGHT(a, 2) ^ ROTATE_RIGHT(a, 13) ^ ROTATE_RIGHT(a, 22);
		unsigned int majority = (a & b) ^ (a & c) ^ (b & c);
		unsigned int second = s0 + majority;

		h = g; g = f; f = e; e = d + first;
		d = c; c = b; b = a; a = first + second;
	}
	context->state[0] += a; context->state[1] += b; context->state[2] += c; context->state[3] += d;
	context->state[4] += e; context->state[5] += f; context->state[6] += g; context->state[7] += h;
}

static void sha256_begin(struct sha256 *context)
{
	static const unsigned int initial[8] =
	{
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
	};

	memcpy(context->state, initial, sizeof(initial));
	context->length = 0;
	context->used = 0;
}

static void sha256_add(struct sha256 *context, const void *data, int size)
{
	const unsigned char *bytes = data;

	context->length += (unsigned long long)size;
	while (size > 0)
	{
		int count = 64 - context->used < size ? 64 - context->used : size;

		memcpy(context->block + context->used, bytes, (size_t)count);
		context->used += count;
		bytes += count;
		size -= count;
		if (context->used == 64)
		{
			sha256_block(context, context->block);
			context->used = 0;
		}
	}
}

static void sha256_end(struct sha256 *context, unsigned char *digest)
{
	unsigned long long bits = context->length * 8;
	unsigned char length[8];
	int index;

	for (index = 0; index < 8; index++)
		length[index] = (unsigned char)(bits >> (56 - index * 8));
	sha256_add(context, "\x80", 1);
	while (context->used != 56)
		sha256_add(context, "", 1);
	sha256_add(context, length, 8);
	for (index = 0; index < 8; index++)
	{
		digest[index * 4] = (unsigned char)(context->state[index] >> 24);
		digest[index * 4 + 1] = (unsigned char)(context->state[index] >> 16);
		digest[index * 4 + 2] = (unsigned char)(context->state[index] >> 8);
		digest[index * 4 + 3] = (unsigned char)context->state[index];
	}
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

/* the tag order to fingerprint, when not the table's (an importer's: the
disc's order, its tags taken by group and path) */
static int v5_size_only;
/* the disc's tag count (--order-count): ORDER hashes that many */
static long order_limit;
static struct tag *order_table;
static long order_count;

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

/* ---------- the stream */

static struct sha256 section;
static struct sha256 whole;
static char const *dump_prefix;
static int dumping;

static void put(void const *bytes, long size)
{
	sha256_add(&section, bytes, (int)size);
	if (dumping)
	{
		long i;
		for (i = 0; i < size; i++)
			printf("%02x", ((unsigned char const *)bytes)[i]);
	}
}

static void put_text(char const *text)
{
	put(text, (long)strlen(text) + 1);
	if (dumping)
		printf(" [%s]\n", text);
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
	dumping = dump_prefix && !strncmp(dump_prefix, kind, strlen(kind)) &&
		(!name || strstr(name, dump_prefix + strlen(kind)) || strlen(dump_prefix) == strlen(kind));
	sha256_begin(&section);
	if (dumping)
		printf("--- %s %s\n", kind, name ? name : "");
}

static void section_end(char const *kind, char const *name)
{
	unsigned char digest[32];
	char line[600];
	int i, n;

	sha256_end(&section, digest);
	n = snprintf(line, sizeof(line), "%s\t%s\t", kind, name ? name : "");
	for (i = 0; i < 16; i++)
		n += snprintf(line + n, sizeof(line) - (size_t)n, "%02x", digest[i]);
	mcc_fingerprint_line(line);
	sha256_add(&whole, line, (int)strlen(line));
	dumping = 0;
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
				elements = translate(u32(p + 4), (unsigned long)(count * e->size));
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

static void walk_element(struct schema_definition const *d, unsigned char *element, char const *const *only_fields)
{
	unsigned char *bytes = malloc((size_t)d->size + 4);
	struct schema_field const *f;
	long o;
	short n;

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
	if (dumping)
		printf("\n{%s}", d->name);
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
	walk_fields(d, element, only_fields);
}

/* ---------- the scripts: trees of tokens (as .local/mcc_check/mcc_scripts_compare.py) */

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
	unsigned char *syntax = translate(u32(scenario + 0x474 + 12), u32(scenario + 0x474));
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
	section_begin("SCRIPTS", NULL);
	count = u32(scenario + 0x49C);
	elements = count ? translate(u32(scenario + 0x49C + 4), count * 0x5C) : NULL;
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
	elements = count ? translate(u32(scenario + 0x4A8 + 4), count * 0x5C) : NULL;
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

/* gets BSP `b` of the scenario's structure_bsps (`reference`, its element)
into `space` (its address, bytes and size): nonzero when it could */
typedef int (*bsp_get_proc)(unsigned long b, unsigned char *reference, struct space *space);

/* the fingerprint of the map in `map` (its tags, table and scenario filled) */
static void fingerprint_map(bsp_get_proc bsp_get)
{
	unsigned char *scenario;
	struct schema_group const *scenario_schema, *bsp_schema;
	long i;
	unsigned char digest[32];
	char line[200];
	int n;

	sha256_begin(&whole);
	section_begin("ORDER", NULL);
	/* the disc's tags (order_limit of them, when the map has more): an
	import writes after them tags of its own that something not simulated
	still names (bitmaps, shaders, widgets), which the network never names;
	a simulated one there fails the order */
	for (i = 0; i < (order_table ? order_count : map.count); i++)
	{
		struct tag *t = order_table ? &order_table[i] : &map.table[i];
		if (order_limit && i >= order_limit)
		{
			if (simulation_group(t->group))
			{
				printf("# order: a simulated tag after the disc's: %s\n", t->name);
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
	for (i = 0; i < (order_table ? order_count : map.count); i++)
	{
		struct tag *t = order_table ? &order_table[i] : &map.table[i];
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
			printf("# no schema: %s\n", name);
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
	scenario = translate(map.table[map.scenario].address, (unsigned long)scenario_schema->definition->size);
	section_begin("SCENARIO", map.table[map.scenario].name);
	walk_element(scenario_schema->definition, scenario, NULL);
	section_end("SCENARIO", map.table[map.scenario].name);
	scripts(scenario);

	bsp_schema = schema_get('sbsp');
	{
		unsigned long count = u32(scenario + 0x5A4);
		unsigned char *references = translate(u32(scenario + 0x5A8), count * 0x20);
		unsigned long b;

		for (b = 0; references && b < count; b++)
		{
			unsigned char *reference = references + b * 0x20;
			long index = handle_index(u32(reference + 0x1C));
			unsigned char *root;
			char const *name = index >= 0 ? map.table[index].name : "?";

			if (!bsp_get(b, reference, &map.bsp))
			{
				printf("# bsp %s unreadable\n", name);
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

	sha256_end(&whole, digest);
	n = snprintf(line, sizeof(line), "MAP	%s	", map.table[map.scenario].name);
	for (i = 0; i < 32; i++)
		n += snprintf(line + n, sizeof(line) - (size_t)n, "%02x", digest[i]);
	mcc_fingerprint_line(line);
}

/* fills map.table from the tags' header (at map.tags) */
static void map_table_read(void)
{
	unsigned char *rows;
	long i;

	map.in_bsp = 0;
	map.count = (long)u32(map.tags.bytes + 12);
	map.scenario = (long)(u32(map.tags.bytes + 4) & 0xFFFF);
	rows = translate(u32(map.tags.bytes), (unsigned long)map.count * 0x20);
	/* (the game's free does not take NULL) */
	if (map.table)
		free(map.table);
	/* (the game's malloc, which the game's free takes: calloc is the C
	library's) */
	map.table = malloc((size_t)map.count * sizeof(struct tag) + 1);
	if (map.table)
		csmemset(map.table, 0, (unsigned long)map.count * sizeof(struct tag));
	for (i = 0; i < map.count; i++)
	{
		unsigned char *row = rows + i * 0x20;
		unsigned char *name = translate(u32(row + 0x10), 1);
		map.table[i].group = u32(row);
		map.table[i].handle = u32(row + 0x0C);
		map.table[i].name = name ? (char const *)name : "?";
		map.table[i].address = u32(row + 0x14);
	}
}

/* ---------- public code */

static struct mcc_fingerprint_bsp const *mcc_fingerprint_bsps;
static long mcc_fingerprint_bsp_count;

/* (bsp_get_proc) the BSP the import wrote for the scenario's reference `b` */
static int mcc_fingerprint_bsp_get(
	unsigned long b,
	unsigned char *reference,
	struct space *space)
{
	(void)reference;
	if ((long)b >= mcc_fingerprint_bsp_count || !mcc_fingerprint_bsps[b].bytes)
		return 0;
	space->address = mcc_fingerprint_bsps[b].address;
	space->size = mcc_fingerprint_bsps[b].size;
	space->bytes = mcc_fingerprint_bsps[b].bytes;
	return 1;
}

char const *mcc_import_fingerprint(
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
	order_limit = disc_tag_count;
	order_table = NULL;
	order_count = 0;
	mcc_fingerprint_bsps = bsps;
	mcc_fingerprint_bsp_count = bsp_count;
	fingerprint_map(mcc_fingerprint_bsp_get);
	if (map.table)
		free(map.table);
	map.table = NULL;

	return mcc_fingerprint_lines.text ? mcc_fingerprint_lines.text : "";
}
