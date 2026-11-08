/*
MCC_IMPORT_PATCH.C

What the MCC import changes in an MCC map's tags before it writes them as
the disc's (mcc_import.c): the importer's own fixes of what Halo PC and MCC
changed, which need no data of the disc's, and the operations of the map's
patch, which give its tags the disc's values where they differ
(.local/step2_fingerprint.md; the prototype that writes them is
.local/mcc_check/patch_make.c). Both walk the tags by the checks' schemas
(tag_schema.h).

An operation is a line of tab-separated fields: its kind, the tag
("group:path", a group's spaces written as underscores), a path from the
tag's root ("root.unit_seats[3].weapon_classes[0]") and its values:
  set       bytes written at an offset of an element (hex)
  ref       a reference or tag index set to a tag ("group:path" or NONE)
  data      a data field's bytes replaced (hex)
  truncate  a block cut to a count
  block     a block's elements replaced (serialized, below)
  keyed     a block rebuilt from these names (its elements', or the
            "element" operations' that follow, for names it lacks)
  tag       a whole tag added (serialized), one of the disc's assets
  donor     a whole tag added that MCC has in another map (the donor),
            as that map's import leaves it: written by that import into
            the user's data (mcc_import_operations_export), never carried
and, read by the import itself (mcc_import.c, mcc_import_hud):
  bitmap_size  the disc's width and height of a HUD bitmap's first image
  sprite_sizes the disc's size of a HUD sprite sheet's sprites (the first
               of each sequence: "122x31,88x14,...")
  hud_values   a HUD interface whose placements the operations give as the
               disc's (only how the HUD looks: the fingerprint has none)
An element is serialized as its bytes, then for each of its pointer fields
in schema order: a block's count and elements, a data field's size and
bytes, a reference's or tag index's name (16-bit length and text), an inline
structure's own fields.
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "tag_schema.h"
#include "mcc_import.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	/* the room new blocks and data take, per map */
	MCC_PATCH_ARENA_BYTES = 0x01000000,
	MCC_PATCH_MAXIMUM_DONORS = 16,
	MCC_PATCH_SCENARIO_TYPE_OFFSET = 0x3C,
	MCC_PATCH_SCENARIO_SCRIPTS_OFFSET = 0x49C,
	MCC_PATCH_SCENARIO_GLOBALS_OFFSET = 0x4A8,
	/* the scenario's script syntax data, a data array of nodes */
	MCC_PATCH_SCENARIO_SYNTAX_DATA_OFFSET = 0x474,
	/* (struct data_array, data.h: first free index, count, actual count) */
	MCC_PATCH_SCRIPT_FIRST_FREE_OFFSET = 0x2C,
	MCC_PATCH_SCRIPT_COUNT_OFFSET = 0x2E,
	MCC_PATCH_SCRIPT_ACTUAL_COUNT_OFFSET = 0x30,
	MCC_PATCH_SCRIPT_NODES_OFFSET = 0x38,
};

/* ---------- structures */

struct mcc_patch_operation
{
	char *kind;
	char *tag;
	char *path;
	char *a;
	char *b;
};

struct mcc_patch_reader
{
	char const *hex;
	long at;
	boolean failed;
};

/* ---------- globals */

static struct
{
	char *text;
	/* the files of the tags the donor lines name (mcc_patch_donor_path) */
	char *donor_texts[MCC_PATCH_MAXIMUM_DONORS];
	long donor_text_count;
	struct mcc_patch_operation *operations;
	long operation_count;
	byte *arena;
	unsigned long arena_used;
	long fixed[8];
	long applied[8];
	long failed;
} mcc_patch_globals;

/* ---------- private code: messages */

#define mcc_patch_log(...) (error(_error_silent, __VA_ARGS__), fprintf(stderr, __VA_ARGS__), fputc(10, stderr))

/* ---------- private code: strings */

/* a string's length: the operations' hex runs past the most the game's
strlen (csstrlen) takes */
static unsigned long mcc_patch_length(
	char const *text)
{
	unsigned long length = 0;

	while (text[length])
		length++;

	return length;
}

/* ---------- private code: the schemas */

static struct tag_schema_group const *mcc_patch_schema(
	unsigned long group_tag)
{
	struct tag_schema_group const *const *list;
	struct tag_schema_group const *group;

	for (list = tag_schema_group_lists; *list; list++)
	{
		for (group = *list; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group;
		}
	}

	return NULL;
}

static boolean mcc_patch_definition_is(
	struct tag_schema_definition const *definition,
	char const *name,
	long size)
{
	return definition->size == size && !strcmp(definition->name, name);
}

/* ---------- private code: the fixes */

typedef void (*mcc_patch_visit_proc)(struct tag_schema_definition const *definition, byte *element);

/* calls `visit` on the element and on every element of every block and
structure under it */
static void mcc_patch_visit(
	struct tag_schema_definition const *definition,
	byte *element,
	mcc_patch_visit_proc visit)
{
	struct tag_schema_field const *field;

	visit(definition, element);
	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short index;

		for (index = 0; index < field->count; index++)
		{
			byte *at = element + field->offset + index * field->size;

			if (field->type == _tag_schema_struct)
			{
				mcc_patch_visit(field->definition, at, visit);
			}
			else if (field->type == _tag_schema_block)
			{
				struct tag_block *block = (struct tag_block *)at;
				struct tag_schema_definition const *element_definition = field->definition;
				long element_index;

				for (element_index = 0; block->address && element_index < block->count; element_index++)
					mcc_patch_visit(element_definition, (byte *)block->address + element_index * element_definition->size, visit);
			}
		}
	}

	return;
}

/* Kind A (.local/mcc_vs_xbox.md): what MCC holds otherwise than the disc
where the disc's value follows from MCC's. */
static void mcc_patch_fix(
	struct tag_schema_definition const *definition,
	byte *element)
{
	/* Halo PC's "client side only" trigger flag (bit 13), and its bit 14:
	the Xbox's flags end at 12 */
	if (mcc_patch_definition_is(definition, "weapon_trigger", definition->size))
	{
		unsigned long flags = *(unsigned long *)element;

		if (flags & 0x6000)
		{
			*(unsigned long *)element = flags & ~0x6000UL;
			mcc_patch_globals.fixed[0]++;
		}
	}
	/* a unit's 12 bytes at 288 the Xbox leaves unused, which MCC fills */
	else if (mcc_patch_definition_is(definition, "unit", 372))
	{
		static byte const zero[12];

		if (memcmp(element + 288, zero, sizeof(zero)))
		{
			csmemset(element + 288, 0, sizeof(zero));
			mcc_patch_globals.fixed[1]++;
		}
	}
	/* Halo PC numbers the effect flags otherwise: its bit 2 is the Xbox's
	bit 1, must be deterministic (bit 1 is never set in MCC) */
	else if (mcc_patch_definition_is(definition, "effect", definition->size) && definition->size > 4)
	{
		unsigned long flags = *(unsigned long *)element;

		if (flags & 0x4)
		{
			*(unsigned long *)element = (flags & ~0x6UL) | 0x2;
			mcc_patch_globals.fixed[3]++;
		}
	}
	/* values Bungie's tools computed, as they computed them (in double
	precision, then rounded): an actor's awareness deltas, 1 / (time * 30) */
	else if (mcc_patch_definition_is(definition, "actor_definition", definition->size))
	{
		short index;

		for (index = 0; index < 3; index++)
		{
			float time, delta;

			csmemcpy(&time, element + 0x60 + index * 4, 4);
			if (time > 0.0f)
			{
				delta = (float)(1.0 / ((double)time * 30.0));
				if (memcmp(element + 0x6C + index * 4, &delta, 4))
				{
					csmemcpy(element + 0x6C + index * 4, &delta, 4);
					mcc_patch_globals.fixed[4]++;
				}
			}
		}
	}
	/* an effect's particles' direction: the unit vector of their yaw and
	pitch */
	else if (mcc_patch_definition_is(definition, "effect_particles", 0xE8))
	{
		float angles[2], direction[3];

		csmemcpy(angles, element + 0x0C, 8);
		direction[0] = (float)(cos((double)angles[0]) * cos((double)angles[1]));
		direction[1] = (float)(sin((double)angles[0]) * cos((double)angles[1]));
		direction[2] = (float)sin((double)angles[1]);
		if (memcmp(element + 0x20, direction, 12))
		{
			csmemcpy(element + 0x20, direction, 12);
			mcc_patch_globals.fixed[6]++;
		}
	}
	/* a point physics' mass over radius cubed (density * 118613.34) and
	buoyancies in water and air ((medium - density) / (medium + density)) */
	else if (mcc_patch_definition_is(definition, "point_physics", 0x40))
	{
		float density, mass, water, air;

		csmemcpy(&density, element + 0x20, 4);
		if (density > 0.0f)
		{
			mass = density * 118613.34f;
			water = (float)((1.0 - (double)density) / (1.0 + (double)density));
			air = (float)(((double)0.0011f - (double)density) / ((double)0.0011f + (double)density));
			if (memcmp(element + 4, &mass, 4) || memcmp(element + 8, &water, 4) || memcmp(element + 12, &air, 4))
				mcc_patch_globals.fixed[5]++;
			csmemcpy(element + 4, &mass, 4);
			csmemcpy(element + 8, &water, 4);
			csmemcpy(element + 12, &air, 4);
		}
	}
	/* an actor variant's 24 bytes at 52 the Xbox leaves unused */
	else if (mcc_patch_definition_is(definition, "actor_variant_definition", definition->size))
	{
		static byte const zero[24];

		if (memcmp(element + 52, zero, sizeof(zero)))
		{
			csmemset(element + 52, 0, sizeof(zero));
			mcc_patch_globals.fixed[2]++;
		}
	}

	return;
}

/* ---------- private code: the operations' space */

static void *mcc_patch_allocate(
	unsigned long size)
{
	unsigned long offset = (mcc_patch_globals.arena_used + 15) & ~15UL;

	if (!mcc_patch_globals.arena)
	{
		mcc_patch_globals.arena = malloc(MCC_PATCH_ARENA_BYTES);
		mcc_patch_globals.arena_used = 0;
		offset = 0;
		if (!mcc_patch_globals.arena)
			return NULL;
	}
	if (offset > MCC_PATCH_ARENA_BYTES || size > MCC_PATCH_ARENA_BYTES - offset)
		return NULL;
	csmemset(mcc_patch_globals.arena + offset, 0, size);
	mcc_patch_globals.arena_used = offset + size;

	return mcc_patch_globals.arena + offset;
}

/* ---------- private code: tags by name */

static unsigned long mcc_patch_group(
	char const *text)
{
	unsigned long group = 0;
	short index;

	for (index = 0; index < 4 && text[index]; index++)
		group = group << 8 | (unsigned char)(text[index] == '_' ? ' ' : text[index]);

	return group;
}

/* the tag "group:path" names: its index in the table, NONE for "NONE", -2
for a tag the table lacks */
static long mcc_patch_tag(
	struct mcc_import_tags const *tags,
	char const *text)
{
	unsigned long group;
	long index;

	if (!strcmp(text, "NONE") || mcc_patch_length(text) < 6 || text[4] != ':')
		return NONE;
	group = mcc_patch_group(text);
	for (index = 0; index < tags->count; index++)
	{
		if ((unsigned long)tags->instances[index].group_tag == group && !strcmp(tags->instances[index].name, text + 5))
			return index;
	}

	return -2;
}

/* a reference (or tag index) at `address` set to the tag `text` names */
static boolean mcc_patch_set_reference(
	struct mcc_import_tags const *tags,
	byte *address,
	boolean is_reference,
	char const *text)
{
	long index = mcc_patch_tag(tags, text);
	long handle = index >= 0 ? tags->instances[index].tag_index : NONE;

	if (index == -2)
	{
		mcc_patch_log("mcc import: no tag %s", text);
		return FALSE;
	}
	if (!is_reference)
	{
		*(long *)address = handle;
		return TRUE;
	}
	{
		struct tag_reference *reference = (struct tag_reference *)address;

		if (index >= 0)
		{
			reference->group_tag = (unsigned long)tags->instances[index].group_tag;
			reference->name = tags->instances[index].name;
			reference->name_length = (long)mcc_patch_length(tags->instances[index].name);
		}
		else
		{
			reference->name = NULL;
			reference->name_length = 0;
		}
		reference->index = handle;
	}

	return TRUE;
}

/* ---------- private code: serialized elements */

static int mcc_patch_hex(
	char c)
{
	return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}

static void mcc_patch_read_bytes(
	struct mcc_patch_reader *reader,
	byte *out,
	long size)
{
	long index;

	for (index = 0; index < size; index++)
	{
		int high = mcc_patch_hex(reader->hex[reader->at]);
		int low = high >= 0 ? mcc_patch_hex(reader->hex[reader->at + 1]) : -1;

		if (high < 0 || low < 0)
		{
			reader->failed = TRUE;
			return;
		}
		out[index] = (byte)(high << 4 | low);
		reader->at += 2;
	}

	return;
}

static unsigned long mcc_patch_read_u32(
	struct mcc_patch_reader *reader)
{
	byte bytes[4] = { 0 };

	mcc_patch_read_bytes(reader, bytes, 4);

	return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
		(unsigned long)bytes[3] << 24;
}

static void mcc_patch_read_pointers(
	struct mcc_import_tags *tags,
	struct mcc_patch_reader *reader,
	struct tag_schema_definition const *definition,
	byte *element);

static void mcc_patch_read_element(
	struct mcc_import_tags *tags,
	struct mcc_patch_reader *reader,
	struct tag_schema_definition const *definition,
	byte *element)
{
	mcc_patch_read_bytes(reader, element, definition->size);
	mcc_patch_read_pointers(tags, reader, definition, element);

	return;
}

static void mcc_patch_read_pointers(
	struct mcc_import_tags *tags,
	struct mcc_patch_reader *reader,
	struct tag_schema_definition const *definition,
	byte *element)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator && !reader->failed; field++)
	{
		short index;

		for (index = 0; index < field->count && !reader->failed; index++)
		{
			byte *at = element + field->offset + index * field->size;

			switch (field->type)
			{
			case _tag_schema_block:
			{
				struct tag_block *block = (struct tag_block *)at;
				struct tag_schema_definition const *element_definition = field->definition;
				unsigned long count = mcc_patch_read_u32(reader), element_index;
				byte *elements = NULL;

				if (count)
				{
					elements = mcc_patch_allocate(count * (unsigned long)element_definition->size);
					if (!elements)
					{
						reader->failed = TRUE;
						return;
					}
				}
				block->count = (long)count;
				block->address = elements;
				block->definition = NULL;
				for (element_index = 0; element_index < count && !reader->failed; element_index++)
					mcc_patch_read_element(tags, reader, element_definition, elements + element_index * element_definition->size);
				break;
			}
			case _tag_schema_data:
			{
				struct tag_data *data = (struct tag_data *)at;
				unsigned long size = mcc_patch_read_u32(reader);
				byte *bytes = NULL;

				if (size)
				{
					bytes = mcc_patch_allocate(size);
					if (!bytes)
					{
						reader->failed = TRUE;
						return;
					}
					mcc_patch_read_bytes(reader, bytes, (long)size);
				}
				data->size = (long)size;
				data->address = bytes;
				data->definition = NULL;
				break;
			}
			case _tag_schema_reference:
			case _tag_schema_tag_index:
			{
				byte length_bytes[2] = { 0 };
				char text[400];
				long length;

				mcc_patch_read_bytes(reader, length_bytes, 2);
				length = length_bytes[0] | length_bytes[1] << 8;
				if (length >= (long)sizeof(text))
				{
					reader->failed = TRUE;
					return;
				}
				mcc_patch_read_bytes(reader, (byte *)text, length);
				text[length] = 0;
				if (!mcc_patch_set_reference(tags, at, field->type == _tag_schema_reference, text))
					mcc_patch_globals.failed++;
				break;
			}
			case _tag_schema_struct:
				mcc_patch_read_pointers(tags, reader, field->definition, at);
				break;
			default:
				break;
			}
		}
	}

	return;
}

/* (an element written as mcc_patch_read_element reads it) */
static void mcc_patch_write_bytes(
	FILE *file,
	byte const *bytes,
	long size)
{
	static char const digits[] = "0123456789abcdef";
	long index;

	for (index = 0; index < size; index++)
	{
		fputc(digits[bytes[index] >> 4], file);
		fputc(digits[bytes[index] & 15], file);
	}

	return;
}

static void mcc_patch_write_u32(
	FILE *file,
	unsigned long value)
{
	byte bytes[4];

	bytes[0] = (byte)value;
	bytes[1] = (byte)(value >> 8);
	bytes[2] = (byte)(value >> 16);
	bytes[3] = (byte)(value >> 24);
	mcc_patch_write_bytes(file, bytes, 4);

	return;
}

/* a tag handle written as the name of its tag ("group:path", NONE) */
static void mcc_patch_write_name(
	struct mcc_import_tags const *tags,
	FILE *file,
	long handle)
{
	char text[400];
	byte length_bytes[2];
	long index, length;

	for (index = 0; handle != NONE && index < tags->count && tags->instances[index].tag_index != handle; index++)
		;
	if (handle == NONE || index == tags->count)
		strcpy(text, "NONE");
	else
	{
		unsigned long group = (unsigned long)tags->instances[index].group_tag;
		short character;

		snprintf(text, sizeof(text), "%c%c%c%c:%s", (char)(group >> 24), (char)(group >> 16), (char)(group >> 8),
			(char)group, tags->instances[index].name);
		for (character = 0; character < 4; character++)
		{
			if (text[character] == ' ')
				text[character] = '_';
		}
	}
	length = (long)mcc_patch_length(text);
	length_bytes[0] = (byte)length;
	length_bytes[1] = (byte)(length >> 8);
	mcc_patch_write_bytes(file, length_bytes, 2);
	mcc_patch_write_bytes(file, (byte const *)text, length);

	return;
}

static void mcc_patch_write_pointers(
	struct mcc_import_tags const *tags,
	FILE *file,
	struct tag_schema_definition const *definition,
	byte const *element);

static void mcc_patch_write_element(
	struct mcc_import_tags const *tags,
	FILE *file,
	struct tag_schema_definition const *definition,
	byte const *element)
{
	mcc_patch_write_bytes(file, element, definition->size);
	mcc_patch_write_pointers(tags, file, definition, element);

	return;
}

static void mcc_patch_write_pointers(
	struct mcc_import_tags const *tags,
	FILE *file,
	struct tag_schema_definition const *definition,
	byte const *element)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short index;

		for (index = 0; index < field->count; index++)
		{
			byte const *at = element + field->offset + index * field->size;

			switch (field->type)
			{
			case _tag_schema_block:
			{
				struct tag_block const *block = (struct tag_block const *)at;
				struct tag_schema_definition const *element_definition = field->definition;
				long count = block->count > 0 && block->address ? block->count : 0, element_index;

				mcc_patch_write_u32(file, (unsigned long)count);
				for (element_index = 0; element_index < count; element_index++)
				{
					mcc_patch_write_element(tags, file, element_definition,
						(byte const *)block->address + element_index * element_definition->size);
				}
				break;
			}
			case _tag_schema_data:
			{
				struct tag_data const *data = (struct tag_data const *)at;
				long size = data->size > 0 && data->address ? data->size : 0;

				mcc_patch_write_u32(file, (unsigned long)size);
				mcc_patch_write_bytes(file, data->address, size);
				break;
			}
			case _tag_schema_reference:
				mcc_patch_write_name(tags, file, ((struct tag_reference const *)at)->index);
				break;
			case _tag_schema_tag_index:
				mcc_patch_write_name(tags, file, *(long const *)at);
				break;
			case _tag_schema_struct:
				mcc_patch_write_pointers(tags, file, field->definition, at);
				break;
			default:
				break;
			}
		}
	}

	return;
}

/* ---------- private code: paths */

/* resolves `path` ("root.a[1].b") from a tag's root: the element it ends on
(*definition, the return value), or, when it ends on a field (a block, data
or reference without an index), that field (*field) at the address
returned, in the element *definition */
static byte *mcc_patch_resolve(
	struct tag_schema_definition const *root_definition,
	byte *root,
	char const *path,
	struct tag_schema_definition const **definition,
	struct tag_schema_field const **found)
{
	struct tag_schema_definition const *current = root_definition;
	byte *element = root;
	char const *cursor = path;

	*found = NULL;
	if (strncmp(cursor, "root", 4))
		return NULL;
	cursor += 4;
	while (*cursor == '.')
	{
		char const *name = ++cursor;
		long length = 0, index = -1;
		struct tag_schema_field const *field, *match = NULL;

		/* (a field's name may hold dots, "unit.unit": the longest that fits) */
		for (field = current->fields; field->type != _tag_schema_terminator; field++)
		{
			long field_length = field->name ? (long)mcc_patch_length(field->name) : 0;

			if (field_length > length && !strncmp(name, field->name, (size_t)field_length) &&
				(name[field_length] == 0 || name[field_length] == '.' || name[field_length] == '['))
			{
				length = field_length;
				match = field;
			}
		}
		if (!match)
			return NULL;
		cursor = name + length;
		if (*cursor == '[')
		{
			index = strtol(cursor + 1, (char **)&cursor, 10);
			if (*cursor != ']')
				return NULL;
			cursor++;
		}
		if (match->type == _tag_schema_struct)
		{
			element += match->offset + (index > 0 ? index : 0) * match->size;
			current = match->definition;
			continue;
		}
		if (match->type == _tag_schema_block && index >= 0 && match->count == 1)
		{
			struct tag_block *block = (struct tag_block *)(element + match->offset);
			struct tag_schema_definition const *element_definition = match->definition;

			if (index >= block->count || !block->address)
				return NULL;
			element = (byte *)block->address + index * element_definition->size;
			current = element_definition;
			continue;
		}
		/* a field: the path ends here */
		if (*cursor)
			return NULL;
		*found = match;
		*definition = current;
		return element + match->offset + (index > 0 ? index : 0) * match->size;
	}
	*definition = current;

	return *cursor ? NULL : element;
}

/* ---------- private code: operations */

/* "tag": a whole tag added to the table (the disc's order then puts it in
its place) */
static boolean mcc_patch_add_tag(
	struct mcc_import_tags *tags,
	struct mcc_patch_operation const *operation)
{
	unsigned long group = mcc_patch_group(operation->tag);
	struct tag_schema_group const *schema = mcc_patch_schema(group);
	struct mcc_patch_reader reader;
	struct mcc_import_tag_instance *instance;
	size_t length = mcc_patch_length(operation->tag + 5);
	byte *root;
	char *name;

	if (mcc_patch_tag(tags, operation->tag) >= 0)
		return TRUE;
	if (!schema || !schema->definition || tags->count >= tags->maximum_count)
		return FALSE;
	name = mcc_patch_allocate((unsigned long)length + 1);
	root = mcc_patch_allocate((unsigned long)schema->definition->size);
	if (!name || !root)
		return FALSE;
	csmemcpy(name, operation->tag + 5, (unsigned long)length + 1);
	instance = &tags->instances[tags->count];
	csmemset(instance, 0, sizeof(*instance));
	instance->group_tag = (long)group;
	instance->parent_group_tags[0] = (long)schema->parent_group_tags[0];
	instance->parent_group_tags[1] = (long)schema->parent_group_tags[1];
	instance->tag_index = (long)(((unsigned long)(0xE174 + tags->count) << 16) | (unsigned long)tags->count);
	instance->name = name;
	instance->base_address = root;
	tags->count++;
	/* (its own references may name it: added first, read after) */
	reader.hex = operation->a;
	reader.at = 0;
	reader.failed = FALSE;
	mcc_patch_read_element(tags, &reader, schema->definition, root);

	return !reader.failed;
}

static boolean mcc_patch_keyed(
	struct mcc_import_tags *tags,
	long operation_index,
	struct tag_schema_field const *field,
	byte *target)
{
	struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[operation_index];
	struct tag_block *block = (struct tag_block *)target;
	struct tag_schema_definition const *element_definition = field->definition;
	struct tag_schema_field const *key_field;
	long key = 0, name_count = 1, name_index;
	byte const *old = block->count > 0 ? block->address : NULL;
	byte *elements;
	char const *cursor;

	for (key_field = element_definition->fields; key_field->type != _tag_schema_terminator; key_field++)
	{
		if (key_field->type == _tag_schema_string && key_field->offset == 0)
			key = key_field->size;
	}
	for (cursor = operation->a; *cursor; cursor++)
		name_count += *cursor == '|';
	elements = mcc_patch_allocate((unsigned long)(name_count * element_definition->size));
	if (!key || !elements)
		return FALSE;
	cursor = operation->a;
	for (name_index = 0; name_index < name_count; name_index++)
	{
		char name[64];
		long length = 0, old_index, other;
		byte *slot = elements + name_index * element_definition->size;

		while (cursor[length] && cursor[length] != '|' && length < 63)
		{
			name[length] = cursor[length];
			length++;
		}
		name[length] = 0;
		cursor += length + (cursor[length] == '|');
		for (old_index = 0; old && old_index < block->count; old_index++)
		{
			if (!strncmp((char const *)old + old_index * element_definition->size, name, (size_t)key))
				break;
		}
		if (old && old_index < block->count)
		{
			csmemcpy(slot, old + old_index * element_definition->size, element_definition->size);
			continue;
		}
		/* the disc's element, from its operation */
		for (other = operation_index + 1; other < mcc_patch_globals.operation_count; other++)
		{
			struct mcc_patch_operation const *element_operation = &mcc_patch_globals.operations[other];

			if (!strcmp(element_operation->kind, "element") && !strcmp(element_operation->tag, operation->tag) &&
				!strcmp(element_operation->path, operation->path) && !strcmp(element_operation->a, name))
			{
				break;
			}
		}
		if (other == mcc_patch_globals.operation_count)
		{
			mcc_patch_log("mcc import: %s %s: no element %s", operation->tag, operation->path, name);
			return FALSE;
		}
		{
			struct mcc_patch_reader reader;

			reader.hex = mcc_patch_globals.operations[other].b;
			reader.at = 0;
			reader.failed = FALSE;
			mcc_patch_read_element(tags, &reader, element_definition, slot);
			if (reader.failed)
				return FALSE;
		}
	}
	block->count = name_count;
	block->address = elements;

	return TRUE;
}

/* one operation on a tag's root `root` of `schema` */
static boolean mcc_patch_apply(
	struct mcc_import_tags *tags,
	long operation_index,
	struct tag_schema_definition const *root_definition,
	byte *root)
{
	struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[operation_index];
	struct tag_schema_definition const *definition = NULL;
	struct tag_schema_field const *field = NULL;
	byte *target;

	target = mcc_patch_resolve(root_definition, root, operation->path, &definition, &field);
	if (!target)
	{
		mcc_patch_log("mcc import: no %s in %s", operation->path, operation->tag);
		return FALSE;
	}
	if (!strcmp(operation->kind, "set") && !field)
	{
		struct mcc_patch_reader reader;
		long offset = strtol(operation->a, NULL, 16), size = (long)mcc_patch_length(operation->b) / 2;

		if (offset < 0 || offset + size > definition->size)
			return FALSE;
		reader.hex = operation->b;
		reader.at = 0;
		reader.failed = FALSE;
		mcc_patch_read_bytes(&reader, target + offset, size);
		mcc_patch_globals.applied[0]++;
		return !reader.failed;
	}
	if (!strcmp(operation->kind, "ref") && field)
	{
		mcc_patch_globals.applied[1]++;
		return mcc_patch_set_reference(tags, target, field->type == _tag_schema_reference, operation->a);
	}
	if (!strcmp(operation->kind, "data") && field && field->type == _tag_schema_data)
	{
		struct tag_data *data = (struct tag_data *)target;
		unsigned long size;
		struct mcc_patch_reader reader;
		byte *bytes;

		size = operation->a ? (unsigned long)mcc_patch_length(operation->a) / 2 : 0;
		bytes = size ? mcc_patch_allocate(size) : NULL;

		if (size && !bytes)
			return FALSE;
		reader.hex = operation->a ? operation->a : "";
		reader.at = 0;
		reader.failed = FALSE;
		mcc_patch_read_bytes(&reader, bytes, (long)size);
		data->size = (long)size;
		data->address = bytes;
		mcc_patch_globals.applied[2]++;
		return !reader.failed;
	}
	if (!strcmp(operation->kind, "truncate") && field && field->type == _tag_schema_block)
	{
		struct tag_block *block = (struct tag_block *)target;
		long count = strtol(operation->a, NULL, 10);

		if (count < 0 || count > block->count)
			return FALSE;
		block->count = count;
		mcc_patch_globals.applied[3]++;
		return TRUE;
	}
	if (!strcmp(operation->kind, "block") && field && field->type == _tag_schema_block)
	{
		struct tag_block *block = (struct tag_block *)target;
		struct tag_schema_definition const *element_definition = field->definition;
		struct mcc_patch_reader reader;
		unsigned long count, element_index;
		byte *elements;

		reader.hex = operation->a;
		reader.at = 0;
		reader.failed = FALSE;
		count = mcc_patch_read_u32(&reader);
		elements = count ? mcc_patch_allocate(count * (unsigned long)element_definition->size) : NULL;
		if (count && !elements)
			return FALSE;
		for (element_index = 0; element_index < count && !reader.failed; element_index++)
			mcc_patch_read_element(tags, &reader, element_definition, elements + element_index * element_definition->size);
		block->count = (long)count;
		block->address = elements;
		mcc_patch_globals.applied[4]++;
		return !reader.failed;
	}
	if (!strcmp(operation->kind, "keyed") && field && field->type == _tag_schema_block)
	{
		mcc_patch_globals.applied[5]++;
		return mcc_patch_keyed(tags, operation_index, field, target);
	}

	return FALSE;
}

/* ---------- private code: models */

static struct tag_schema_field const *mcc_patch_field(
	struct tag_schema_definition const *definition,
	char const *name)
{
	struct tag_schema_field const *field;

	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		if (field->name && !strcmp(field->name, name))
			return field;
	}

	return NULL;
}

/* A model whose permutations name a level of detail past its geometries
(the disc's indices, set by the operations, on MCC's model, which has fewer
levels: the flamethrower's) gets its last geometry again in their place, so
that the indices stay the disc's and name a geometry of its own. */
static long mcc_patch_model_complete(
	byte *model,
	struct tag_schema_definition const *definition)
{
	struct tag_schema_field const *regions_field = mcc_patch_field(definition, "regions");
	struct tag_schema_field const *permutations_field = regions_field ?
		mcc_patch_field(regions_field->definition, "permutations") : NULL;
	struct tag_schema_field const *indices_field = permutations_field ?
		mcc_patch_field(permutations_field->definition, "geometry_indices") : NULL;
	struct tag_schema_field const *geometries_field = mcc_patch_field(definition, "geometries");
	struct tag_block *regions, *geometries;
	long region_index, highest = NONE, index;
	byte *grown;

	if (!indices_field || indices_field->size != sizeof(short) || !geometries_field)
		return 0;
	regions = (struct tag_block *)(model + regions_field->offset);
	geometries = (struct tag_block *)(model + geometries_field->offset);
	for (region_index = 0; regions->address && region_index < regions->count; region_index++)
	{
		byte *region = (byte *)regions->address + region_index * ((struct tag_schema_definition const *)regions_field->definition)->size;
		struct tag_block *permutations = (struct tag_block *)(region + permutations_field->offset);
		long permutation_index;

		for (permutation_index = 0; permutations->address && permutation_index < permutations->count; permutation_index++)
		{
			byte *permutation = (byte *)permutations->address + permutation_index * ((struct tag_schema_definition const *)permutations_field->definition)->size;
			short level;

			for (level = 0; level < indices_field->count; level++)
			{
				short geometry_index = *(short *)(permutation + indices_field->offset + level * indices_field->size);

				if (geometry_index > highest)
					highest = geometry_index;
			}
		}
	}
	if (highest < geometries->count || geometries->count <= 0 || !geometries->address)
		return 0;
	grown = mcc_patch_allocate((unsigned long)(highest + 1) * ((struct tag_schema_definition const *)geometries_field->definition)->size);
	if (!grown)
		return 0;
	csmemcpy(grown, geometries->address, (unsigned long)geometries->count * ((struct tag_schema_definition const *)geometries_field->definition)->size);
	for (index = geometries->count; index <= highest; index++)
	{
		csmemcpy(grown + index * ((struct tag_schema_definition const *)geometries_field->definition)->size,
			(byte *)geometries->address + (geometries->count - 1) * ((struct tag_schema_definition const *)geometries_field->definition)->size,
			((struct tag_schema_definition const *)geometries_field->definition)->size);
	}
	index = highest + 1 - geometries->count;
	geometries->count = highest + 1;
	geometries->address = grown;

	return index;
}

/* ---------- public code */

void mcc_import_models_complete(
	struct mcc_import_tags *tags)
{
	struct tag_schema_group const *schema = mcc_patch_schema('mode');
	long index, added = 0;

	for (index = 0; schema && schema->definition && index < tags->count; index++)
	{
		if (tags->instances[index].group_tag == 'mode' && tags->instances[index].base_address)
			added += mcc_patch_model_complete(tags->instances[index].base_address, schema->definition);
	}
	if (added)
		mcc_patch_log("mcc import: %ld levels of detail the disc's permutations name given a model's last geometry", added);

	return;
}

void mcc_import_fixes_apply(
	struct mcc_import_tags *tags)
{
	long index;
	byte *scenario = tags->instances[tags->scenario_index].base_address;

	csmemset(mcc_patch_globals.fixed, 0, sizeof(mcc_patch_globals.fixed));
	/* a multiplayer map's scripts and globals are MCC's own: the disc's have
	none */
	if (scenario && *(short *)(scenario + MCC_PATCH_SCENARIO_TYPE_OFFSET) == 1)
	{
		mcc_patch_log("mcc import: multiplayer: MCC's %ld scripts and %ld script globals left out",
			((struct tag_block *)(scenario + MCC_PATCH_SCENARIO_SCRIPTS_OFFSET))->count,
			((struct tag_block *)(scenario + MCC_PATCH_SCENARIO_GLOBALS_OFFSET))->count);
		((struct tag_block *)(scenario + MCC_PATCH_SCENARIO_SCRIPTS_OFFSET))->count = 0;
		((struct tag_block *)(scenario + MCC_PATCH_SCENARIO_GLOBALS_OFFSET))->count = 0;
		/* and their source and syntax nodes, as the disc's multiplayer
		scenarios have none: the game resolves the nodes' names as it loads
		them, and the globals they name are left out */
		{
			struct tag_schema_group const *scenario_schema = mcc_patch_schema('scnr');
			struct tag_schema_field const *source_files = scenario_schema && scenario_schema->definition ?
				mcc_patch_field(scenario_schema->definition, "hs_source_files") : NULL;
			struct tag_data *syntax = (struct tag_data *)(scenario + MCC_PATCH_SCENARIO_SYNTAX_DATA_OFFSET);

			if (source_files && source_files->type == _tag_schema_block)
				((struct tag_block *)(scenario + source_files->offset))->count = 0;
			/* (the node array's first free index, count and actual count:
			an array with none in use, as the disc's) */
			if (syntax->address && syntax->size >= MCC_PATCH_SCRIPT_NODES_OFFSET)
			{
				*(short *)((byte *)syntax->address + MCC_PATCH_SCRIPT_FIRST_FREE_OFFSET) = 0;
				*(short *)((byte *)syntax->address + MCC_PATCH_SCRIPT_COUNT_OFFSET) = 0;
				*(short *)((byte *)syntax->address + MCC_PATCH_SCRIPT_ACTUAL_COUNT_OFFSET) = 0;
			}
		}
	}
	for (index = 0; index < tags->count; index++)
	{
		struct mcc_import_tag_instance *instance = &tags->instances[index];
		struct tag_schema_group const *schema = mcc_patch_schema((unsigned long)instance->group_tag);

		if (instance->base_address && schema && schema->definition)
			mcc_patch_visit(schema->definition, instance->base_address, mcc_patch_fix);
	}
	mcc_patch_log("mcc import: fixed %ld trigger flags, %ld units, %ld actor variants, %ld effect flags, "
		"%ld awareness deltas, %ld point physics, %ld particle directions", mcc_patch_globals.fixed[0],
		mcc_patch_globals.fixed[1], mcc_patch_globals.fixed[2], mcc_patch_globals.fixed[3], mcc_patch_globals.fixed[4],
		mcc_patch_globals.fixed[5], mcc_patch_globals.fixed[6]);

	return;
}

/* where the import of the MCC map `donor_map` writes its tag `tag` for other
maps' donor lines (mcc_import_operations_export): the user's own data, never
carried with the game */
#define MCC_PATCH_DONOR_FOLDER "d:\\mcc_import"

static void mcc_patch_donor_path(
	char *path,
	size_t size,
	char const *donor_map,
	char const *tag)
{
	char *character;

	snprintf(path, size, "%s\\%s %s.p2", MCC_PATCH_DONOR_FOLDER, donor_map, tag);
	for (character = path + mcc_patch_length(MCC_PATCH_DONOR_FOLDER) + 1; *character; character++)
	{
		if (*character == '\\' || *character == ':')
			*character = '_';
	}

	return;
}

/* each donor line made the tag operation its file holds, if it is written
(left a donor line otherwise: mcc_import_operations_donors_missing) */
static void mcc_patch_donors_read(
	void)
{
	long index;

	for (index = 0; index < mcc_patch_globals.operation_count; index++)
	{
		struct mcc_patch_operation *operation = &mcc_patch_globals.operations[index];
		char path[512];
		FILE *file;
		long size;
		char *text, *end, *hex;

		if (strcmp(operation->kind, "donor") || mcc_patch_globals.donor_text_count >= MCC_PATCH_MAXIMUM_DONORS)
			continue;
		mcc_patch_donor_path(path, sizeof(path), operation->a, operation->tag);
		file = fopen(path, "rb");
		if (!file)
			continue;
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		text = size > 0 ? malloc((size_t)size + 1) : NULL;
		if (!text || fread(text, 1, (size_t)size, file) != (size_t)size)
		{
			free(text);
			fclose(file);
			continue;
		}
		fclose(file);
		text[size] = 0;
		/* (one line: tag, the tag, "-", its bytes) */
		end = strchr(text, '\n');
		if (end)
			*end = 0;
		hex = strrchr(text, '\t');
		if (strncmp(text, "tag\t", 4) || !hex || strncmp(text + 4, operation->tag, mcc_patch_length(operation->tag)))
		{
			mcc_patch_log("mcc import: %s is not the tag %s", path, operation->tag);
			free(text);
			continue;
		}
		mcc_patch_globals.donor_texts[mcc_patch_globals.donor_text_count++] = text;
		operation->kind = "tag";
		operation->path = "-";
		operation->a = hex + 1;
	}

	return;
}

boolean mcc_import_operations_read(
	char const *map_name)
{
	char path[256];
	FILE *file;
	long size, capacity = 0;
	char *line;

	mcc_import_operations_dispose();
	snprintf(path, sizeof(path), "d:\\mcc_patches\\%s.p2", map_name);
	file = fopen(path, "rb");
	if (!file)
		return FALSE;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	mcc_patch_globals.text = size > 0 ? malloc((size_t)size + 1) : NULL;
	if (!mcc_patch_globals.text || fread(mcc_patch_globals.text, 1, (size_t)size, file) != (size_t)size)
	{
		fclose(file);
		mcc_import_operations_dispose();
		return FALSE;
	}
	fclose(file);
	mcc_patch_globals.text[size] = 0;

	for (line = mcc_patch_globals.text; line && *line; )
	{
		char *end = strchr(line, '\n');
		char *fields[5] = { NULL, NULL, NULL, NULL, NULL };
		char *cursor = line;
		short field_count = 0;

		if (end)
			*end = 0;
		if (end && end > line && end[-1] == '\r')
			end[-1] = 0;
		if (line[0] && line[0] != '#')
		{
			fields[field_count++] = cursor;
			while (field_count < 5 && (cursor = strchr(cursor, '\t')) != NULL)
			{
				*cursor++ = 0;
				fields[field_count++] = cursor;
			}
			if (mcc_patch_globals.operation_count == capacity)
			{
				struct mcc_patch_operation *grown;

				capacity = capacity ? capacity * 2 : 1024;
				grown = malloc((size_t)capacity * sizeof(*grown));
				if (!grown)
				{
					mcc_import_operations_dispose();
					return FALSE;
				}
				if (mcc_patch_globals.operations)
				{
					csmemcpy(grown, mcc_patch_globals.operations,
						(unsigned long)mcc_patch_globals.operation_count * sizeof(*grown));
					free(mcc_patch_globals.operations);
				}
				mcc_patch_globals.operations = grown;
			}
			mcc_patch_globals.operations[mcc_patch_globals.operation_count].kind = fields[0];
			mcc_patch_globals.operations[mcc_patch_globals.operation_count].tag = fields[1] ? fields[1] : "";
			mcc_patch_globals.operations[mcc_patch_globals.operation_count].path = fields[2] ? fields[2] : "";
			mcc_patch_globals.operations[mcc_patch_globals.operation_count].a = fields[3] ? fields[3] : "";
			mcc_patch_globals.operations[mcc_patch_globals.operation_count].b = fields[4] ? fields[4] : "";
			mcc_patch_globals.operation_count++;
		}
		line = end ? end + 1 : NULL;
	}
	mcc_patch_donors_read();

	return TRUE;
}

boolean mcc_import_operations_export(
	struct mcc_import_tags const *tags,
	char const *donor_map,
	char const *tag)
{
	char path[512];
	long index = mcc_patch_tag(tags, tag);
	struct tag_schema_group const *schema = index >= 0 ?
		mcc_patch_schema((unsigned long)tags->instances[index].group_tag) : NULL;
	FILE *file;

	if (!schema || !schema->definition || !tags->instances[index].base_address)
	{
		mcc_patch_log("mcc import: %s has no %s to give", donor_map, tag);
		return FALSE;
	}
	CreateDirectoryA(MCC_PATCH_DONOR_FOLDER, NULL);
	mcc_patch_donor_path(path, sizeof(path), donor_map, tag);
	file = fopen(path, "wb");
	if (!file)
	{
		mcc_patch_log("mcc import: cannot write %s", path);
		return FALSE;
	}
	fprintf(file, "tag\t%s\t-\t", tag);
	mcc_patch_write_element(tags, file, schema->definition, tags->instances[index].base_address);
	fputc('\n', file);

	return fclose(file) == 0;
}

long mcc_import_operations_donors_missing(
	char const **donor_maps,
	char const **tags,
	long maximum_count)
{
	long index, count = 0;

	for (index = 0; index < mcc_patch_globals.operation_count && count < maximum_count; index++)
	{
		struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[index];

		if (!strcmp(operation->kind, "donor"))
		{
			donor_maps[count] = operation->a;
			tags[count] = operation->tag;
			count++;
		}
	}

	return count;
}

long mcc_import_operations_apply(
	struct mcc_import_tags *tags,
	char const *bsp_name,
	void *bsp_root)
{
	long index;

	mcc_patch_globals.failed = 0;
	csmemset(mcc_patch_globals.applied, 0, sizeof(mcc_patch_globals.applied));
	/* the tags added first: other operations name them */
	for (index = 0; !bsp_name && index < mcc_patch_globals.operation_count; index++)
	{
		struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[index];

		if (!strcmp(operation->kind, "tag"))
		{
			if (getenv("HALO_MCC_IMPORT_TRACE"))
				fprintf(stderr, "mcc import: adding %s\n", operation->tag);
			if (mcc_patch_add_tag(tags, operation))
				mcc_patch_globals.applied[6]++;
			else
			{
				mcc_patch_log("mcc import: the tag %s cannot be added", operation->tag);
				mcc_patch_globals.failed++;
			}
		}
	}
	for (index = 0; index < mcc_patch_globals.operation_count; index++)
	{
		struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[index];
		struct tag_schema_group const *schema;
		byte *root;

		if (!strcmp(operation->kind, "order") || !strcmp(operation->kind, "tag") ||
			!strcmp(operation->kind, "element") || !strcmp(operation->kind, "missing") ||
			!strcmp(operation->kind, "handle") || !strcmp(operation->kind, "bitmap_size") ||
			!strcmp(operation->kind, "hud_values") || !strcmp(operation->kind, "sprite_sizes") ||
			!strcmp(operation->kind, "donor"))
		{
			continue;
		}
		if (bsp_name ? strcmp(operation->tag, bsp_name) : !strncmp(operation->tag, "sbsp:", 5))
			continue;
		if (bsp_name)
		{
			schema = mcc_patch_schema('sbsp');
			root = bsp_root;
		}
		else
		{
			long tag_index = mcc_patch_tag(tags, operation->tag);

			if (tag_index < 0)
			{
				mcc_patch_globals.failed++;
				continue;
			}
			schema = mcc_patch_schema((unsigned long)tags->instances[tag_index].group_tag);
			root = tags->instances[tag_index].base_address;
		}
		if (getenv("HALO_MCC_IMPORT_TRACE"))
			fprintf(stderr, "mcc import: %s %s %s\n", operation->kind, operation->tag, operation->path);
		if (!schema || !schema->definition || !root || !mcc_patch_apply(tags, index, schema->definition, root))
		{
			mcc_patch_log("mcc import: the operation %s %s %s failed", operation->kind, operation->tag, operation->path);
			mcc_patch_globals.failed++;
		}
	}
	mcc_patch_log("mcc import: %s: %ld set, %ld ref, %ld data, %ld truncate, %ld block, %ld keyed, %ld tags added; "
		"%ld failed", bsp_name ? bsp_name : "tags", mcc_patch_globals.applied[0], mcc_patch_globals.applied[1],
		mcc_patch_globals.applied[2], mcc_patch_globals.applied[3], mcc_patch_globals.applied[4],
		mcc_patch_globals.applied[5], mcc_patch_globals.applied[6], mcc_patch_globals.failed);

	return mcc_patch_globals.failed;
}

boolean mcc_import_operations_bitmap_size(
	char const *name,
	short *width,
	short *height)
{
	long index;

	for (index = 0; index < mcc_patch_globals.operation_count; index++)
	{
		struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[index];

		if (!strcmp(operation->kind, "bitmap_size") && !strncmp(operation->tag, "bitm:", 5) &&
			!strcmp(operation->tag + 5, name))
		{
			*width = (short)strtol(operation->a, NULL, 10);
			*height = (short)strtol(operation->b, NULL, 10);
			return *width > 0 && *height > 0;
		}
	}

	return FALSE;
}

char const *mcc_import_operations_sprite_sizes(
	char const *name)
{
	long index;

	for (index = 0; index < mcc_patch_globals.operation_count; index++)
	{
		struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[index];

		if (!strcmp(operation->kind, "sprite_sizes") && !strncmp(operation->tag, "bitm:", 5) &&
			!strcmp(operation->tag + 5, name))
		{
			return operation->a;
		}
	}

	return NULL;
}

boolean mcc_import_operations_hud_values(
	char const *tag)
{
	long index;

	for (index = 0; index < mcc_patch_globals.operation_count; index++)
	{
		struct mcc_patch_operation const *operation = &mcc_patch_globals.operations[index];

		if (!strcmp(operation->kind, "hud_values") && (!tag || !strcmp(operation->tag, tag)))
			return TRUE;
	}

	return FALSE;
}

void mcc_import_operations_dispose(
	void)
{
	if (mcc_patch_globals.operations)
		free(mcc_patch_globals.operations);
	if (mcc_patch_globals.text)
		free(mcc_patch_globals.text);
	if (mcc_patch_globals.arena)
		free(mcc_patch_globals.arena);
	while (mcc_patch_globals.donor_text_count > 0)
		free(mcc_patch_globals.donor_texts[--mcc_patch_globals.donor_text_count]);
	mcc_patch_globals.operations = NULL;
	mcc_patch_globals.text = NULL;
	mcc_patch_globals.arena = NULL;
	mcc_patch_globals.arena_used = 0;
	mcc_patch_globals.operation_count = 0;

	return;
}
