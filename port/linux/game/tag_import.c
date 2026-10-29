/*
TAG_IMPORT.C

The tag importer (tag_import.h): a level's tags copied in from other maps,
read from the tag packs (tag_pack.c). Each map read is a donor: its packed
tag section read whole, its tag table walked from each root asked for
through every tag reference and tag index the tag schema
(tag_schema_*.c) names, which gathers what the root needs (the "wanted"
tags; a tag the level has, or a scenario, structure, globals or vehicle,
is not copied but named as the level's), and then each wanted tag copied:
its root struct, its blocks and data, each reference's name. The copies
are appended to the level's table in donor order, then by name, so two
machines asking for the same roots in the same order append the same tags
at the same indices; then every reference and tag index of theirs is
pointed at the level's tags, the copies or the level's own. A model's
parts get vertex and index buffers made here; a bitmap's pixels and a
sound's samples are read from the pack in play (tag_import_donor_read,
which the cache's reads ask first: cache_files_windows.c); the few pixels a
pack has not got (a HUD of ui\, which a pack may leave out) are
read from the map as the level loads.

A root and everything it needs, or none of it: a model taken without its
shaders is drawn with a shader that is not there, and the game stops. A
level's copies are at most IMPORT_MAX tags in COPY_BUDGET bytes; past that
none of them are kept.

Each level's donors read only from packs. The menus, outside a level,
may peek at a map (tag_import_peek), from its pack or else the map itself.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cseries.h"
#include "cseries/errors.h"
#include "cseries/cseries_windows.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "game/game.h"
#include "interface/progress_bar.h"
#include "main/main.h"
#include "models/model_definitions.h"
#include "rasterizer/rasterizer_geometry.h"
#include "sound/sound_definitions.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"

#include "halo_port_limits.h"
#include "tag_import.h"
#include "tag_pack.h"
#include "tag_schema.h"

#include <xtl.h>
/* the port's zlib, the same one that unpacks a map while precaching it
(source/cache/cache_files_decompress_windows.c) */
#include "../../third_party/zlib/zlib_prefixed.h"

void platform_log(char const *format, ...);
extern void *cache_files_tag_instances(long *count);
extern void cache_files_set_tag_instances(void *instances, long count);

/* the addresses a map file gives its tag pointers, wherever this process
keeps the tag cache (halo_port_window.h) */
#define DONOR_WINDOW 0x803A6000UL

/* the most maps a level copies from (the nine other campaign maps of a
randomized level, and a mode's) */
#define DONOR_COUNT 16
/* the most packs a level reads */
#define PACK_COUNT 8
#define WANTED_MAX 8192
#define IMPORT_MAX 2048
#define COPY_BUDGET (32UL * 1024UL * 1024UL)
#define WALK_DEPTH 32

struct tag_import_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

struct tag_import_header
{
	struct tag_import_instance *tag_instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long vertex_buffer_count;
	void *vertex_buffers;
	long index_buffer_count;
	void *index_buffers;
	unsigned long signature;
};

typedef char tag_import_instance_size[sizeof(struct tag_import_instance) == 0x20 ? 1 : -1];
typedef char tag_import_tag_count_offset[offsetof(struct tag_import_header, tag_count) == 0xC ? 1 : -1];

/* one bitmap or sound range in a pack. file_offset is the uncompressed map
offset the game asks for; data_at is where those bytes sit in the pack.
Twelve bytes, little endian, matching tools/tag_pack.py */
struct pack_span
{
	long file_offset;
	long size;
	long data_at;
};

typedef char pack_span_size[sizeof(struct pack_span) == 12 ? 1 : -1];

struct pack_entry
{
	char name[TAG_PACK_MAP_NAME_SIZE];
	long file_length;
	long tag_offset;
	long tag_size;
	long tags_at;
	long resource_count;
	long resources_at;
};

/* a range of a map's pixels the pack has not got, read from the map */
struct donor_extra
{
	long offset;
	long size;
	unsigned char *bytes;
};

struct tag_import_donor
{
	/* CreateFileA, not fopen: the Switch keeps map files in memory, and a
	d:\maps path is only translated by the file calls the game uses */
	HANDLE file;
	/* the tag section only. The disc file is a header plus a zlib stream;
	this is the inflated tag bytes, not the whole map */
	unsigned char *bytes;
	unsigned long size;
	long file_length;
	long tag_offset;
	int compressed;
	/* the pack this map is read from, when it has it, and whether its
	pixels and samples come from there, not this map file */
	struct pack *pack;
	int from_pack;
	struct pack_span *spans;
	long span_count;
	/* the pixels of the bitmaps copied from it that the pack has not got
	(one that keeps no bitmap of the ui folder, a HUD's: a stock HUD
	copied for a weapon, or the sniper rifle's), read from the map as
	the level loads (donor_extras_fill) */
	struct donor_extra *extras;
	long extra_count;
	long extra_capacity;
	long count;
	struct tag_import_instance *instances;
	unsigned char *seen;
	long *datum_of;
	int keep;
	/* a peek's (the menus'): read from the map itself when its pack is not
	written. A level's never is */
	int map_allowed;
	/* ... and whether it opened its pack, which it closes as it does */
	int opened_pack;
	char leaf[TAG_PACK_MAP_NAME_SIZE];
};

struct wanted
{
	short donor;
	long absolute;
	long group;
	char name[256];
};

/* a tag another map's copy of which is wanted already: it is that copy */
struct wanted_alias
{
	short donor;
	long absolute;
	short target_donor;
	long target_absolute;
};

/* the wanted tags by group and name, so each is copied from one map. Twice
WANTED_MAX, a power of two; a slot holds an index into wanted, plus one */
#define WANTED_SLOTS 16384

struct copied_tag
{
	short donor;
	void *root;
	struct tag_schema_definition const *schema;
};

struct held_block
{
	struct held_block *next;
};

/* the loaded map's tag, as cache_files.c has it. The semantics header only
names it (menu_tags.c defines it the same way) */
struct cache_file_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

typedef char tag_import_tag_table_size[
	sizeof(struct cache_file_tag_instance) == 0x20 ? 1 : -1];

/* a pack, read (tag_pack.h): its manifest's name, and what it holds */
struct pack
{
	char name[64];
	unsigned long content;
	HANDLE file;
	struct pack_entry entries[TAG_PACK_MAPS];
	int entry_count;
	/* 1 open, -1 not there (nothing is read from it), 0 not tried */
	int state;
};

static struct tag_import_donor donors[DONOR_COUNT];
static int donor_count;
static struct pack packs[PACK_COUNT];
static int pack_count;
static struct wanted *wanted;
static long wanted_count;
static struct copied_tag copied[IMPORT_MAX];
static long copied_count;
static struct held_block *held;
static unsigned long held_bytes;
static int import_stopped;
static long walk_steps;
static long *wanted_slots;
static struct wanted_alias *aliases;
static long alias_count;
/* a root (a palette's actor, a pool's weapon) and what it needs did not fit:
none of it was taken, and no root after it is */
static int wanted_full;
static int import_progress_open;
static struct cache_file_tag_instance *saved_instances;
static long saved_count;
static struct cache_file_tag_instance *grown_instances;
static long grown_count;
static long import_absolute[IMPORT_MAX];
static short import_donor[IMPORT_MAX];
static long import_count;
/* a bit for each absolute tag index in import_absolute
(tag_import_tag_imported, asked for every bitmap drawn) */
static unsigned char import_marked[0x10000 / 8];
/* the most bytes of pixels a level reads from the maps for the packs
(donor_extras_fill), all its donors', and how many it has */
#define DONOR_EXTRA_BYTES_MAX (16L * 1024L * 1024L)
static long donor_extra_bytes;
/* how many copied bitmap and sound ranges the packs do not hold, and what
the reads during play actually returned. The detail lines stop after a few
dozen so a level does not fill the log */
static long resource_checked;
static long resource_uncovered;
static long resource_empty;
static long resource_logged;
static long donor_reads;
static long donor_misses;
static long donor_logged;
static int donor_saw_bitmap;
static int donor_saw_sound;
static unsigned char donor_noted[IMPORT_MAX];

static void *donor_address(struct tag_import_donor *donor, void *pointer)
{
	unsigned long address = (unsigned long)pointer;

	if (!pointer || address < DONOR_WINDOW || address - DONOR_WINDOW >= donor->size)
		return NULL;

	return donor->bytes + (address - DONOR_WINDOW);
}

static int range_in(struct tag_import_donor *donor, void const *pointer, unsigned long bytes)
{
	unsigned char const *start = pointer;

	if (!pointer)
		return bytes == 0;
	if (start < donor->bytes || start > donor->bytes + donor->size)
		return 0;
	if ((unsigned long)(donor->bytes + donor->size - start) < bytes)
		return 0;

	return 1;
}

static char const *instance_name(struct tag_import_donor *donor, struct tag_import_instance *instance)
{
	char const *name = donor_address(donor, instance->name);
	unsigned long room;
	void const *end;

	if (!name || !range_in(donor, name, 1))
		return NULL;
	room = (unsigned long)(donor->bytes + donor->size - (unsigned char const *)name);
	end = memchr(name, 0, room > 256 ? 256 : room);
	if (!end)
		return NULL;

	return name;
}

static struct tag_schema_definition const *schema_of(unsigned long group)
{
	long list;

	for (list = 0; tag_schema_group_lists[list]; list++)
	{
		struct tag_schema_group const *groups = tag_schema_group_lists[list];
		long index;

		for (index = 0; groups[index].group_tag; index++)
		{
			if (groups[index].group_tag == group)
				return groups[index].definition;
		}
	}

	return NULL;
}

static void *grab(unsigned long size)
{
	struct held_block *block;

	if (!size || import_stopped || held_bytes > COPY_BUDGET || size > 0x7fffffffUL - sizeof(*block))
	{
		import_stopped = 1;
		return NULL;
	}
	block = malloc(sizeof(*block) + size);
	if (!block)
	{
		import_stopped = 1;
		return NULL;
	}
	block->next = held;
	held = block;
	held_bytes += size;

	return block + 1;
}

static void held_free(void)
{
	while (held)
	{
		struct held_block *next = held->next;

		free(held);
		held = next;
	}
	held_bytes = 0;
}

static int leave_group(long group)
{
	return group == 'vehi' || group == 'scnr' || group == 'sbsp' || group == (long)'matg';
}

static void want(struct tag_import_donor *donor, long datum);

/* a field whose declared bytes run past the element is skipped: the schema
can name an offset the donor tag does not have */
/* whether a walk of this schema can reach a tag reference. Geometry,
collision nodes and animation frames cannot, and walking every one of them
is what made a donor's tags fault or never return */
static int schema_has_tags(struct tag_schema_definition const *schema, int depth)
{
	struct tag_schema_field const *field;
	static struct tag_schema_definition const *visiting[16];
	int visit;
	int has;

	if (!schema || !schema->fields || depth > 8)
		return 0;
	for (visit = 0; visit < depth && visit < 16; visit++)
	{
		if (visiting[visit] == schema)
			return 0;
	}
	if (depth < 16)
		visiting[depth] = schema;
	has = 0;
	for (field = schema->fields; field->type != _tag_schema_terminator; field++)
	{
		if (field->type == _tag_schema_reference || field->type == _tag_schema_tag_index)
		{
			has = 1;
			break;
		}
		if ((field->type == _tag_schema_struct || field->type == _tag_schema_block) &&
			schema_has_tags(field->definition, depth + 1))
		{
			has = 1;
			break;
		}
	}

	return has;
}

static int field_inside(struct tag_schema_definition const *schema, struct tag_schema_field const *field)
{
	unsigned long end;

	if (!schema || !field || schema->size <= 0 || field->count <= 0 || field->size <= 0 || field->offset < 0)
		return 0;
	if ((unsigned long)field->offset >= (unsigned long)schema->size)
		return 0;
	if ((unsigned long)field->size >
		((unsigned long)schema->size - (unsigned long)field->offset) / (unsigned long)field->count)
		return 0;
	end = (unsigned long)field->offset + (unsigned long)field->count * (unsigned long)field->size;

	return end <= (unsigned long)schema->size;
}

static void walk_want(struct tag_import_donor *donor, void *element, struct tag_schema_definition const *schema, int depth)
{
	struct tag_schema_field const *field;

	if (walk_steps > 400000)
		wanted_full = 1;
	if (wanted_full || import_stopped)
		return;
	if (depth >= WALK_DEPTH || !element || !schema || schema->size <= 0 ||
		!range_in(donor, element, (unsigned long)schema->size))
		return;
	walk_steps++;
	for (field = schema->fields; field && field->type != _tag_schema_terminator; field++)
	{
		long item;

		if (!field_inside(schema, field))
			continue;
		for (item = 0; item < field->count; item++)
		{
			void *place = (char *)element + field->offset + item * field->size;

			if (field->type == _tag_schema_reference)
				want(donor, ((struct tag_reference *)place)->index);
			else if (field->type == _tag_schema_tag_index)
				want(donor, *(long *)place);
			else if (field->type == _tag_schema_struct)
			{
				if (schema_has_tags(field->definition, 0))
					walk_want(donor, place, field->definition, depth + 1);
			}
			else if (field->type == _tag_schema_block)
			{
				struct tag_block *block = place;
				struct tag_schema_definition const *child = field->definition;
				void *elements = donor_address(donor, block->address);
				long index;
				long count = block->count;

				if (!child || count <= 0 || !elements || child->size <= 0 || count > 4096 ||
					!schema_has_tags(child, 0) ||
					(unsigned long)count > 0x1000000UL / (unsigned long)child->size ||
					!range_in(donor, elements, (unsigned long)count * (unsigned long)child->size))
					continue;
				for (index = 0; index < count; index++)
					walk_want(donor, (char *)elements + index * (unsigned long)child->size, child, depth + 1);
			}
		}
	}
}

static unsigned long wanted_hash(long group, char const *name)
{
	unsigned long hash = (unsigned long)group * 0x9E3779B1UL;

	for (; *name; name++)
	{
		char character = *name;

		if (character >= 'A' && character <= 'Z')
			character = (char)(character - 'A' + 'a');
		hash = (hash ^ (unsigned char)character) * 0x01000193UL;
	}

	return hash & (WANTED_SLOTS - 1);
}

/* the wanted tag of this group and name, or NONE; with insert, the slot it
goes in is taken by `index` when there is none */
static long wanted_find(long group, char const *name, long insert)
{
	unsigned long slot = wanted_hash(group, name);

	for (;;)
	{
		long held = wanted_slots[slot];

		if (!held)
		{
			if (insert != NONE)
				wanted_slots[slot] = insert + 1;
			return NONE;
		}
		if (wanted[held - 1].group == group && !_stricmp(wanted[held - 1].name, name))
			return held - 1;
		slot = (slot + 1) & (WANTED_SLOTS - 1);
	}
}

static long wanted_find(long group, char const *name, long insert);

/* the table again, of the wanted_count entries: the one time a root does
not fit, and what it added goes */
static void wanted_slots_rebuild(void)
{
	long index;

	memset(wanted_slots, 0, WANTED_SLOTS * sizeof(*wanted_slots));
	for (index = 0; index < wanted_count; index++)
		wanted_find(wanted[index].group, wanted[index].name, index);
}

static void want(struct tag_import_donor *donor, long datum)
{
	long absolute = datum & 0xFFFF;
	long other;
	struct tag_import_instance *instance;
	char const *name;
	long existing;

	if (datum == NONE || absolute < 0 || absolute >= donor->count || donor->seen[absolute])
		return;
	donor->seen[absolute] = 1;
	instance = donor->instances + absolute;
	name = instance_name(donor, instance);
	if (leave_group(instance->group_tag))
	{
		donor->datum_of[absolute] = name ? tag_loaded(instance->group_tag, name) : NONE;
		return;
	}
	existing = name ? tag_loaded(instance->group_tag, name) : NONE;
	if (existing != NONE)
	{
		donor->datum_of[absolute] = existing;
		return;
	}
	if (!name || wanted_count >= WANTED_MAX || alias_count >= WANTED_MAX)
	{
		wanted_full = 1;
		return;
	}
	/* (the same tag from another map: the copy of it already wanted, which
	brings what it needs from its own map) */
	other = wanted_find(instance->group_tag, name, wanted_count);
	if (other != NONE)
	{
		aliases[alias_count].donor = (short)(donor - donors);
		aliases[alias_count].absolute = absolute;
		aliases[alias_count].target_donor = wanted[other].donor;
		aliases[alias_count].target_absolute = wanted[other].absolute;
		alias_count++;
		return;
	}
	snprintf(wanted[wanted_count].name, sizeof(wanted[wanted_count].name), "%s", name);
	wanted[wanted_count].donor = (short)(donor - donors);
	wanted[wanted_count].absolute = absolute;
	wanted[wanted_count].group = instance->group_tag;
	wanted_count++;
}

/* a root and every tag it needs, or none of them: a model taken without its
shaders is drawn with a shader that is not there, and the game stops */
static int want_root(struct tag_import_donor *donor, long absolute)
{
	long start = wanted_count;
	long start_aliases = alias_count;
	long queued = wanted_count;

	if (wanted_full)
		return 0;
	want(donor, absolute);
	while (queued < wanted_count && !wanted_full)
	{
		struct wanted *entry = wanted + queued;
		struct tag_import_donor *owner = donors + entry->donor;
		struct tag_import_instance *instance = owner->instances + entry->absolute;
		struct tag_schema_definition const *schema = schema_of((unsigned long)instance->group_tag);

		queued++;
		if (schema)
			walk_want(owner, donor_address(owner, instance->base_address), schema, 0);
	}
	if (!wanted_full && wanted_count <= IMPORT_MAX)
		return 1;
	/* (what this root added goes; seen stays set, which no later root asks
	about, there being none) */
	wanted_count = start;
	alias_count = start_aliases;
	wanted_slots_rebuild();
	wanted_full = 1;
	error(_error_silent, "tag import: %s.map's next tags do not fit; the level takes the %ld before them",
		donor->leaf, wanted_count);

	return 0;
}

static int wanted_less(void const *left, void const *right)
{
	struct wanted const *a = left;
	struct wanted const *b = right;
	int order;

	/* map first, so the copy reads each map's tags once. The name then
	keeps two machines appending the same tag at the same index */
	if (a->donor != b->donor)
		return a->donor - b->donor;
	order = _stricmp(a->name, b->name);
	if (order)
		return order;

	return a->absolute < b->absolute ? -1 : a->absolute > b->absolute;
}

static void copy_fields(struct tag_import_donor *donor, void *element, struct tag_schema_definition const *schema, int depth);

static void copy_fields(struct tag_import_donor *donor, void *element, struct tag_schema_definition const *schema, int depth)
{
	struct tag_schema_field const *field;

	if (depth >= WALK_DEPTH || !element || !schema || schema->size <= 0)
		return;
	for (field = schema->fields; field->type != _tag_schema_terminator; field++)
	{
		long item;

		if (!field_inside(schema, field))
			continue;
		for (item = 0; item < field->count; item++)
		{
			void *place = (char *)element + field->offset + item * field->size;

			if (field->type == _tag_schema_reference)
			{
				struct tag_reference *reference = place;
				char const *source = donor_address(donor, reference->name);
				unsigned long room;
				void const *end;
				char *copy;

				reference->name = "";
				reference->name_length = 0;
				if (!source || !range_in(donor, source, 1))
				{
					reference->index = NONE;
					continue;
				}
				room = (unsigned long)(donor->bytes + donor->size - (unsigned char const *)source);
				end = memchr(source, 0, room > 256 ? 256 : room);
				if (!end)
					continue;
				copy = grab((unsigned long)strlen(source) + 1);
				if (!copy)
					continue;
				strcpy(copy, source);
				reference->name = copy;
				reference->name_length = (long)strlen(copy);
			}
			else if (field->type == _tag_schema_data || field->type == _tag_schema_file_data)
			{
				struct tag_data *data = place;

				data->definition = NULL;
				if (field->type == _tag_schema_file_data || data->size <= 0)
				{
					data->address = NULL;
					continue;
				}
				{
					void *source = donor_address(donor, data->address);
					void *bytes;

					if (!source || data->size > 0x1000000 ||
						!range_in(donor, source, (unsigned long)data->size))
					{
						data->size = 0;
						data->address = NULL;
						continue;
					}
					bytes = grab((unsigned long)data->size);
					if (!bytes)
					{
						data->size = 0;
						data->address = NULL;
						continue;
					}
					memcpy(bytes, source, (unsigned long)data->size);
					data->address = bytes;
				}
			}
			else if (field->type == _tag_schema_reset)
			{
				short size = field->size;
				long value = field->target_offset;

				if (size > 4)
					memset(place, 0, (unsigned long)size);
				else if (size > 0)
				{
					memset(place, 0, (unsigned long)size);
					memcpy(place, &value, (unsigned long)size);
				}
			}
			else if (field->type == _tag_schema_struct)
				copy_fields(donor, place, field->definition, depth + 1);
			else if (field->type == _tag_schema_block)
			{
				struct tag_block *block = place;
				struct tag_schema_definition const *child = field->definition;
				void *source = donor_address(donor, block->address);
				void *elements;
				long index;
				long count = block->count;

				block->definition = NULL;
				block->address = NULL;
				if (!child || count <= 0 || !source || child->size <= 0 || count > 100000 ||
					(unsigned long)count > 0x1000000UL / (unsigned long)child->size ||
					!range_in(donor, source, (unsigned long)count * (unsigned long)child->size))
				{
					block->count = 0;
					continue;
				}
				elements = grab((unsigned long)count * (unsigned long)child->size);
				if (!elements)
				{
					block->count = 0;
					continue;
				}
				memcpy(elements, source, (unsigned long)count * (unsigned long)child->size);
				block->address = elements;
				for (index = 0; index < count; index++)
					copy_fields(donor, (char *)elements + index * (unsigned long)child->size, child, depth + 1);
			}
		}
	}
}

static long retarget_datum(struct tag_import_donor *donor, long datum)
{
	long absolute = datum & 0xFFFF;

	if (datum == NONE || absolute < 0 || absolute >= donor->count)
		return NONE;

	return donor->datum_of[absolute];
}

static void retarget_fields(struct tag_import_donor *donor, void *element, struct tag_schema_definition const *schema, int depth)
{
	struct tag_schema_field const *field;

	if (depth >= WALK_DEPTH || !element || !schema || schema->size <= 0)
		return;
	for (field = schema->fields; field->type != _tag_schema_terminator; field++)
	{
		long item;

		if (!field_inside(schema, field))
			continue;
		for (item = 0; item < field->count; item++)
		{
			void *place = (char *)element + field->offset + item * field->size;

			if (field->type == _tag_schema_reference)
			{
				struct tag_reference *reference = place;
				long old = reference->index;
				long index = reference->name && reference->name[0] ?
					tag_loaded(reference->group_tag, reference->name) : NONE;

				if (index == NONE)
					index = retarget_datum(donor, old);
				reference->index = index;
				if (index != NONE)
				{
					long count = 0;
					struct cache_file_tag_instance *instances = cache_files_tag_instances(&count);
					long absolute = index & 0xFFFF;

					reference->name = tag_get_name(index);
					reference->name_length = reference->name ? (long)strlen(reference->name) : 0;
					if (instances && absolute >= 0 && absolute < count &&
						tag_loaded(reference->group_tag, reference->name) != index)
						reference->group_tag = (unsigned long)instances[absolute].group_tag;
				}
			}
			else if (field->type == _tag_schema_tag_index)
				*(long *)place = retarget_datum(donor, *(long *)place);
			else if (field->type == _tag_schema_struct)
				retarget_fields(donor, place, field->definition, depth + 1);
			else if (field->type == _tag_schema_block)
			{
				struct tag_block *block = place;
				struct tag_schema_definition const *child = field->definition;
				long index;

				if (!child || !block->address || block->count <= 0)
					continue;
				for (index = 0; index < block->count; index++)
					retarget_fields(donor, (char *)block->address + index * (unsigned long)child->size, child, depth + 1);
			}
		}
	}
}

static int donor_file_at(HANDLE file, long offset)
{
	DWORD result = SetFilePointer(file, offset, NULL, FILE_BEGIN);

	return result != INVALID_SET_FILE_POINTER || GetLastError() == 0;
}

static int donor_file_read(HANDLE file, void *buffer, unsigned long size)
{
	unsigned long done = 0;

	while (done < size)
	{
		DWORD chunk = (DWORD)(size - done);
		DWORD got = 0;

		if (chunk > 0x40000)
			chunk = 0x40000;
		if (!ReadFile(file, (char *)buffer + done, chunk, &got, NULL) || got == 0)
			return 0;
		done += got;
	}

	return 1;
}

static void donor_bytes_free(struct tag_import_donor *donor)
{
	if (donor->bytes)
		free(donor->bytes);
	donor->bytes = NULL;
}

/* `start` and `size` are offsets in the inflated file. The disc file's
header is plain; everything from 0x800 on is the zlib stream precaching
inflates. Output before `start` is discarded, so the whole map is never
held */
static int inflate_span(HANDLE file, long start, long size, unsigned char *destination)
{
	z_stream stream;
	unsigned char input[0x2000];
	unsigned char discard[0x2000];
	long produced;
	long end;
	int result;
	unsigned int asked;
	long filled;
	DWORD got;

	if (!destination || start < 0x800 || size <= 0)
		return 0;
	if (!donor_file_at(file, 0x800))
		return 0;
	memset(&stream, 0, sizeof(stream));
	if (inflateInit(&stream) != Z_OK)
		return 0;
	produced = 0x800;
	end = start + size;
	result = Z_OK;
	while (produced < end && result != Z_STREAM_END)
	{
		if (stream.avail_in == 0)
		{
			got = 0;
			if (!ReadFile(file, input, (DWORD)sizeof(input), &got, NULL) || got == 0)
				break;
			stream.next_in = (Bytef *)input;
			stream.avail_in = got;
		}
		if (produced < start)
		{
			long skip = start - produced;

			if (skip > (long)sizeof(discard))
				skip = (long)sizeof(discard);
			stream.next_out = (Bytef *)discard;
			asked = (unsigned int)skip;
		}
		else
		{
			long want = end - produced;

			if (want > 0x4000)
				want = 0x4000;
			stream.next_out = (Bytef *)(destination + (produced - start));
			asked = (unsigned int)want;
		}
		stream.avail_out = asked;
		result = inflate(&stream, Z_NO_FLUSH);
		if (result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR)
			break;
		filled = (long)(asked - stream.avail_out);
		if (filled <= 0)
			break;
		produced += filled;
		if (result == Z_BUF_ERROR)
			result = Z_OK;
	}
	inflateEnd(&stream);

	return produced >= end;
}

static int pack_read_u32(struct pack *pack, long at, unsigned long *value)
{
	unsigned char bytes[4];
	DWORD got = 0;

	if (!donor_file_at(pack->file, at) || !ReadFile(pack->file, bytes, 4, &got, NULL) || got != 4)
		return 0;
	memcpy(value, bytes, 4);

	return 1;
}

static void pack_close(struct pack *pack)
{
	if (pack->file && pack->file != INVALID_HANDLE_VALUE)
		CloseHandle(pack->file);
	pack->file = NULL;
	pack->entry_count = 0;
	pack->state = 0;
}

/* each map the pack has is still the map in maps/. One that was replaced
(another disc's, or a fresh copy) would have its pixels read at the old
offsets */
static int pack_matches_maps(struct pack *pack)
{
	int index;

	for (index = 0; index < pack->entry_count; index++)
	{
		struct pack_entry *entry = pack->entries + index;
		long file_length, tag_offset, tag_size;

		if (!tag_pack_map_header(entry->name, &file_length, &tag_offset, &tag_size, NULL))
			continue;
		if (file_length != entry->file_length || tag_offset != entry->tag_offset || tag_size != entry->tag_size)
		{
			error(_error_silent, "tag import: %s.map is not the one in %s.pack", entry->name, pack->name);
			return 0;
		}
	}

	return 1;
}

/* a pack (tag_pack.h): 'rpak', format 2, what it holds, how many maps, then
a 56-byte record per map (its name, file length, tag offset, tag size,
where the tag bytes are, how many resource spans, where that table is) */
static int pack_load(struct pack *pack, char const *path)
{
	unsigned long magic = 0;
	unsigned long version = 0;
	unsigned long content = 0;
	unsigned long count = 0;
	unsigned long index;
	DWORD open_error;

	pack->file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (pack->file == INVALID_HANDLE_VALUE)
	{
		open_error = GetLastError();
		error(_error_silent, "tag import: no %s (error %lu)", path, (unsigned long)open_error);
		pack->file = NULL;
		return 0;
	}
	if (!pack_read_u32(pack, 0, &magic) || !pack_read_u32(pack, 4, &version) ||
		!pack_read_u32(pack, 8, &content) || !pack_read_u32(pack, 12, &count) ||
		magic != 'rpak' || version != TAG_PACK_FORMAT_VERSION || content != pack->content ||
		count > (unsigned long)TAG_PACK_MAPS)
	{
		error(_error_silent, "tag import: %s is not a pack of this build's %s (format %lu, content %08lx)", path,
			pack->name, version, content);
		pack_close(pack);
		return 0;
	}
	for (index = 0; index < count; index++)
	{
		unsigned char record[TAG_PACK_RECORD_SIZE];
		struct pack_entry *entry = pack->entries + index;
		long at = TAG_PACK_HEADER_SIZE + (long)index * TAG_PACK_RECORD_SIZE;

		if (!donor_file_at(pack->file, at) || !donor_file_read(pack->file, record, sizeof(record)))
		{
			error(_error_silent, "tag import: %s directory is short", path);
			pack_close(pack);
			return 0;
		}
		memset(entry, 0, sizeof(*entry));
		memcpy(entry->name, record, TAG_PACK_MAP_NAME_SIZE - 1);
		memcpy(&entry->file_length, record + 32, 4);
		memcpy(&entry->tag_offset, record + 36, 4);
		memcpy(&entry->tag_size, record + 40, 4);
		memcpy(&entry->tags_at, record + 44, 4);
		memcpy(&entry->resource_count, record + 48, 4);
		memcpy(&entry->resources_at, record + 52, 4);
	}
	pack->entry_count = (int)count;
	if (!pack_matches_maps(pack))
	{
		pack_close(pack);
		return 0;
	}

	return 1;
}

/* the pack (written at the main menu: tag_pack.c). Not there, stale, or no
longer the maps', and nothing is read from it */
static int pack_open(struct pack *pack)
{
	char const *path;

	if (pack->state == 1)
		return 1;
	if (pack->state < 0)
		return 0;
	path = tag_pack_file_name(pack->name);
	if (!path || !pack_load(pack, path))
	{
		error(_error_silent, "tag import: %s.pack is not written; nothing is read from it", pack->name);
		pack->state = -1;
		return 0;
	}
	pack->state = 1;
	error(_error_silent, "tag import: using %s (%d maps)", path, pack->entry_count);

	return 1;
}

/* the level's reading of a pack, by its manifest's name: NULL when no
manifest has that name, or there is no room */
static struct pack *pack_named(char const *name)
{
	struct pack *pack;
	int index;

	for (index = 0; index < pack_count; index++)
	{
		if (!strcmp(packs[index].name, name))
			return packs + index;
	}
	if (!tag_pack_file_name(name) || pack_count >= PACK_COUNT)
	{
		error(_error_silent, "tag import: no pack %s (no manifest has that name, or %d are read already)", name,
			PACK_COUNT);
		return NULL;
	}
	pack = packs + pack_count++;
	memset(pack, 0, sizeof(*pack));
	snprintf(pack->name, sizeof(pack->name), "%s", name);
	pack->content = tag_pack_content(name);

	return pack;
}

static void packs_close(void)
{
	int index;

	for (index = 0; index < pack_count; index++)
		pack_close(packs + index);
	pack_count = 0;
}

static struct pack_entry *pack_find(struct pack *pack, char const *leaf)
{
	int index;

	if (!pack || !pack_open(pack))
		return NULL;
	for (index = 0; index < pack->entry_count; index++)
	{
		if (!_stricmp(pack->entries[index].name, leaf))
			return pack->entries + index;
	}

	return NULL;
}

static int pack_read_tags(struct tag_import_donor *donor, struct pack_entry *entry)
{
	unsigned long span_bytes;

	donor->file_length = entry->file_length;
	donor->tag_offset = entry->tag_offset;
	donor->compressed = 0;
	if (!donor->from_pack)
	{
		if (entry->resource_count < 0 || entry->resource_count > 100000 ||
			entry->tag_size <= 0 || entry->tag_size > (long)HALO_PORT_TAG_CACHE_SIZE ||
			entry->tags_at < 0 || entry->resources_at < 0)
		{
			error(_error_silent, "tag import: %s.map in the pack is not usable", donor->leaf);
			return 0;
		}
		if (entry->resource_count > 0)
		{
			span_bytes = (unsigned long)entry->resource_count * sizeof(*donor->spans);
			donor->spans = malloc(span_bytes);
			if (!donor->spans)
			{
				error(_error_silent, "tag import: no room for %s.map's pack ranges", donor->leaf);
				return 0;
			}
			if (!donor_file_at(donor->pack->file, entry->resources_at) ||
				!donor_file_read(donor->pack->file, donor->spans, span_bytes))
			{
				error(_error_silent, "tag import: could not read %s.map's pack ranges", donor->leaf);
				free(donor->spans);
				donor->spans = NULL;
				return 0;
			}
			donor->span_count = entry->resource_count;
		}
		donor->from_pack = 1;
	}
	donor->bytes = malloc((unsigned long)entry->tag_size);
	donor->size = (unsigned long)entry->tag_size;
	if (!donor->bytes)
	{
		error(_error_silent, "tag import: no room for %s.map's packed tags (%ld bytes)", donor->leaf, entry->tag_size);
		return 0;
	}
	if (!donor_file_at(donor->pack->file, entry->tags_at) ||
		!donor_file_read(donor->pack->file, donor->bytes, (unsigned long)entry->tag_size))
	{
		error(_error_silent, "tag import: could not read %s.map's packed tags", donor->leaf);
		donor_bytes_free(donor);
		return 0;
	}
	error(_error_silent, "tag import: %s.map from %s.pack (%ld byte tags)", donor->leaf, donor->pack->name,
		entry->tag_size);

	return 1;
}

/* 1 when one pack range holds the whole read, -1 when it holds the start
only, 0 when the offset is not in any range */
static int span_covers(struct tag_import_donor *donor, long offset, long size)
{
	long low;
	long high;
	struct pack_span *span;

	if (!donor || !donor->from_pack || !donor->spans || donor->span_count <= 0 || offset < 0 || size <= 0)
		return 0;
	low = 0;
	high = donor->span_count;
	while (low < high)
	{
		long mid = low + (high - low) / 2;

		if (donor->spans[mid].file_offset <= offset)
			low = mid + 1;
		else
			high = mid;
	}
	if (low == 0)
		return 0;
	span = donor->spans + (low - 1);
	if (span->size <= 0 || offset < span->file_offset || offset >= span->file_offset + span->size)
		return 0;
	if (size > span->file_offset + span->size - offset)
		return -1;

	return 1;
}

static long pack_copy(struct tag_import_donor *donor, long offset, long size, void *buffer)
{
	unsigned char *out = buffer;
	long filled = 0;
	HANDLE file = donor->pack ? donor->pack->file : NULL;

	if (!donor->spans || donor->span_count <= 0 || !file || file == INVALID_HANDLE_VALUE)
		return 0;
	while (size > 0)
	{
		long low = 0;
		long high = donor->span_count;
		struct pack_span *span;
		long into;
		long have;

		while (low < high)
		{
			long mid = low + (high - low) / 2;

			if (donor->spans[mid].file_offset <= offset)
				low = mid + 1;
			else
				high = mid;
		}
		if (low == 0)
			return filled;
		span = donor->spans + (low - 1);
		if (span->size <= 0 || offset < span->file_offset || offset >= span->file_offset + span->size)
			return filled;
		into = offset - span->file_offset;
		have = span->size - into;
		if (have > size)
			have = size;
		if (span->data_at < 0 || into < 0)
			return filled;
		if (!donor_file_at(file, span->data_at + into) ||
			!donor_file_read(file, out, (unsigned long)have))
			return filled;
		offset += have;
		size -= have;
		out += have;
		filled += have;
	}

	return filled;
}

/* the tag bytes only. datum_of stays, so a tag found while the file was open
still retargets after the bytes are gone */
static int load_donor_bytes(struct tag_import_donor *donor)
{
	char path[512];
	unsigned char header[0x800];
	long version;
	long file_length;
	long tag_offset;
	long tag_size;
	unsigned long signature;
	DWORD open_error;
	DWORD physical;
	struct tag_import_header *tags;
	struct pack_entry *packed;

	if (donor->bytes)
		return 1;
	packed = pack_find(donor->pack, donor->leaf);
	if (packed)
	{
		snprintf(path, sizeof(path), "%s.pack", donor->pack->name);
		if (!pack_read_tags(donor, packed))
			return 0;
	}
	else if (!donor->map_allowed)
	{
		error(_error_silent, "tag import: %s.pack has not got %s.map: nothing is copied from it",
			donor->pack ? donor->pack->name : "(no pack)", donor->leaf);
		return 0;
	}
	else
	{
	snprintf(path, sizeof(path), "%s%s.map", cache_files_map_directory(), donor->leaf);
	if (!donor->file || donor->file == INVALID_HANDLE_VALUE)
	{
		donor->file = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (donor->file == INVALID_HANDLE_VALUE)
		{
			open_error = GetLastError();
			error(_error_silent, "tag import: could not open %s (error %lu)", path, (unsigned long)open_error);
			donor->file = NULL;
			return 0;
		}
	}
	if (!donor_file_at(donor->file, 0) || !donor_file_read(donor->file, header, sizeof(header)))
	{
		error(_error_silent, "tag import: could not read the header of %s", path);
		return 0;
	}
	memcpy(&signature, header, 4);
	memcpy(&version, header + 4, 4);
	memcpy(&file_length, header + 8, 4);
	memcpy(&tag_offset, header + 0x10, 4);
	memcpy(&tag_size, header + 0x14, 4);
	if (signature != 'head' || version != 5 || file_length < (long)sizeof(header) ||
		file_length > 0x11600000 || tag_offset < 0 || tag_size < (long)sizeof(struct tag_import_header) ||
		tag_size > (long)HALO_PORT_TAG_CACHE_SIZE || tag_size > file_length || tag_offset > file_length - tag_size)
	{
		error(_error_silent,
			"tag import: %s is not an Xbox map (signature %08lx, version %ld, %ld bytes, tags %ld at %ld)",
			path, signature, version, file_length, tag_size, tag_offset);
		return 0;
	}
	donor->file_length = file_length;
	donor->tag_offset = tag_offset;
	physical = GetFileSize(donor->file, NULL);
	donor->compressed = physical == INVALID_FILE_SIZE || (long)physical < file_length;
	error(_error_silent, "tag import: unpacking %s (%ld byte map, file is %lu, tags %ld at %ld)",
		path, file_length, (unsigned long)physical, tag_size, tag_offset);
	donor->bytes = malloc((unsigned long)tag_size);
	donor->size = (unsigned long)tag_size;
	if (!donor->bytes)
	{
		error(_error_silent, "tag import: no room for %s tags (%ld bytes)", path, tag_size);
		return 0;
	}
	if (donor->compressed)
	{
		if (tag_offset < 0x800 || !inflate_span(donor->file, tag_offset, tag_size, donor->bytes))
		{
			error(_error_silent, "tag import: could not decompress %s", path);
			donor_bytes_free(donor);
			return 0;
		}
	}
	else if (!donor_file_at(donor->file, tag_offset) ||
		!donor_file_read(donor->file, donor->bytes, (unsigned long)tag_size))
	{
		error(_error_silent, "tag import: could not read %ld bytes of tags from %s", tag_size, path);
		donor_bytes_free(donor);
		return 0;
	}
	}
	tags = (struct tag_import_header *)donor->bytes;
	if (tags->signature != 'tags' || tags->tag_count <= 0 || tags->tag_count > 100000)
	{
		error(_error_silent, "tag import: %s tags are not a tag section (signature %08lx, count %ld)",
			path, tags->signature, tags->tag_count);
		donor_bytes_free(donor);
		return 0;
	}
	donor->count = tags->tag_count;
	donor->instances = donor_address(donor, tags->tag_instances);
	if (!donor->instances ||
		!range_in(donor, donor->instances, (unsigned long)donor->count * sizeof(*donor->instances)))
	{
		error(_error_silent, "tag import: %s tag table is not in its tag section (%ld tags at %08lx)",
			path, tags->tag_count, (unsigned long)tags->tag_instances);
		donor_bytes_free(donor);
		donor->instances = NULL;
		return 0;
	}

	return 1;
}

static void release_donor_blob(struct tag_import_donor *donor)
{
	donor_bytes_free(donor);
	donor->instances = NULL;
	if (donor->seen)
		free(donor->seen);
	donor->seen = NULL;
	if (!donor->keep && donor->file && donor->file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(donor->file);
		donor->file = NULL;
	}
}

static int open_donor(struct tag_import_donor *donor, char const *leaf, struct pack *pack, int map_allowed)
{
	long index;

	memset(donor, 0, sizeof(*donor));
	snprintf(donor->leaf, sizeof(donor->leaf), "%s", leaf);
	donor->pack = pack;
	donor->map_allowed = map_allowed;
	if (!load_donor_bytes(donor))
		return 0;
	/* malloc, not calloc: free() is the debug allocator, and a block it did
	not hand out faults when released */
	donor->seen = malloc((unsigned long)donor->count);
	donor->datum_of = malloc((unsigned long)donor->count * sizeof(*donor->datum_of));
	if (!donor->seen || !donor->datum_of)
	{
		error(_error_silent, "tag import: no room for %s.map's tag list (%ld tags)", donor->leaf, donor->count);
		return 0;
	}
	memset(donor->seen, 0, (unsigned long)donor->count);
	for (index = 0; index < donor->count; index++)
		donor->datum_of[index] = NONE;

	return 1;
}

static void close_donor_bytes(struct tag_import_donor *donor)
{
	long index;

	release_donor_blob(donor);
	if (donor->spans)
		free(donor->spans);
	donor->spans = NULL;
	donor->span_count = 0;
	for (index = 0; index < donor->extra_count; index++)
	{
		if (donor->extras[index].bytes)
			free(donor->extras[index].bytes);
	}
	if (donor->extras)
		free(donor->extras);
	donor->extras = NULL;
	donor->extra_count = 0;
	donor->extra_capacity = 0;
	donor->from_pack = 0;
	if (donor->datum_of)
		free(donor->datum_of);
	donor->datum_of = NULL;
}

static int donor_bytes(struct tag_import_donor *donor)
{
	int index;

	if (donor->bytes && donor->instances)
		return 1;
	for (index = 0; index < donor_count; index++)
	{
		if (donors + index != donor)
			release_donor_blob(donors + index);
	}

	return load_donor_bytes(donor);
}

static void donors_close(void)
{
	int index;

	for (index = 0; index < donor_count; index++)
	{
		close_donor_bytes(donors + index);
		if (donors[index].file && donors[index].file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(donors[index].file);
			donors[index].file = NULL;
		}
	}
	donor_count = 0;
	donor_extra_bytes = 0;
	packs_close();
}

static void remember_import(long absolute, short donor_index)
{
	if (import_count >= IMPORT_MAX)
		return;
	import_absolute[import_count] = absolute;
	import_donor[import_count] = donor_index;
	import_count++;
	import_marked[(absolute & 0xFFFF) >> 3] |= (unsigned char)(1 << (absolute & 7));
}

static void imports_forget(void)
{
	import_count = 0;
	memset(import_marked, 0, sizeof(import_marked));
}

/* a model's draw buffers are addresses in the donor map. The part is drawn
from a vertex buffer made here; one that cannot be made is not drawn */
struct tag_import_model_geometry
{
	byte reserved[0x24];
	struct tag_block parts;
};

struct tag_import_model_part
{
	unsigned long flags;
	short shader_index;
	char previous_part_index;
	char next_part_index;
	short centroid_primary_node_index;
	short centroid_secondary_node_index;
	real centroid_primary_node_weight;
	real centroid_secondary_node_weight;
	real_point3d centroid;
	struct tag_block uncompressed_vertices;
	struct tag_block compressed_vertices;
	struct tag_block triangles;
	struct triangle_buffer triangle_buffer;
	struct vertex_buffer vertex_buffer;
};

typedef char tag_import_model_part_size[sizeof(struct tag_import_model_part) == 0x68 ? 1 : -1];

/* the parts given buffers here, whose buffers go when the level does. The
map's own are released with its tags (tags_header_deregister_vertex_and_index_buffers);
these were not, and each level load kept the last one's: the vertices in
contiguous memory, which the textures need too, and the indices in the heap */
#define BOUND_BLOCK_PARTS 256

struct bound_block
{
	struct bound_block *next;
	long count;
	struct tag_import_model_part *parts[BOUND_BLOCK_PARTS];
};

static struct bound_block *bound_blocks;
static long bound_part_count;
static unsigned long bound_vertex_bytes;

static void bound_remember(struct tag_import_model_part *part, unsigned long vertex_bytes)
{
	struct bound_block *block = bound_blocks;

	if (!block || block->count >= BOUND_BLOCK_PARTS)
	{
		block = malloc(sizeof(*block));
		if (!block)
			return;
		block->next = bound_blocks;
		block->count = 0;
		bound_blocks = block;
	}
	block->parts[block->count++] = part;
	bound_part_count++;
	bound_vertex_bytes += vertex_bytes;
}

static void bound_release(void)
{
	while (bound_blocks)
	{
		struct bound_block *next = bound_blocks->next;
		long index;

		for (index = 0; index < bound_blocks->count; index++)
		{
			struct tag_import_model_part *part = bound_blocks->parts[index];

			rasterizer_triangle_buffer_delete(&part->triangle_buffer);
			rasterizer_vertex_buffer_delete(&part->vertex_buffer);
			csmemset(&part->triangle_buffer, 0, sizeof(part->triangle_buffer));
			csmemset(&part->vertex_buffer, 0, sizeof(part->vertex_buffer));
		}
		free(bound_blocks);
		bound_blocks = next;
	}
	bound_part_count = 0;
	bound_vertex_bytes = 0;
}

static void *buffer_record_data(struct tag_import_donor *donor, void *hardware_format, unsigned long bytes)
{
	unsigned char *record = donor_address(donor, hardware_format);
	void *data;

	if (!record || !range_in(donor, record, 12) || !bytes)
		return NULL;
	data = donor_address(donor, *(void **)(record + 4));
	if (!data || !range_in(donor, data, bytes))
		return NULL;

	return data;
}

static void bind_part(struct tag_import_donor *donor, struct tag_import_model_part *part)
{
	struct vertex_buffer *vertex = &part->vertex_buffer;
	struct triangle_buffer *triangle = &part->triangle_buffer;
	void *format;
	void *vertices;
	void *indices;
	long vertex_size;
	long index_bytes;
	long count;
	short type;
	short triangle_type;
	long triangle_count;

	format = vertex->hardware_format;
	count = vertex->count;
	type = vertex->type;
	triangle_type = triangle->type;
	triangle_count = triangle->count;
	vertex_size = rasterizer_geometry_get_vertex_size(type);
	if (!format || count <= 0 || vertex_size <= 0 || count > 65535 ||
		(unsigned long)count > 0x1000000UL / (unsigned long)vertex_size)
	{
		csmemset(vertex, 0, sizeof(*vertex));
		csmemset(triangle, 0, sizeof(*triangle));
		return;
	}
	vertices = buffer_record_data(donor, format, (unsigned long)count * (unsigned long)vertex_size);
	if (triangle_type == 0)
		index_bytes = triangle_count > 0 ? triangle_count * 6 : 0;
	else if (triangle_type == 1)
		index_bytes = triangle_count > 0 ? triangle_count * 2 + 4 : 0;
	else
		index_bytes = 0;
	indices = index_bytes > 0 ? buffer_record_data(donor, triangle->hardware_format, (unsigned long)index_bytes) : NULL;
	if (!vertices || !indices)
	{
		csmemset(vertex, 0, sizeof(*vertex));
		csmemset(triangle, 0, sizeof(*triangle));
		return;
	}
	if (!rasterizer_vertex_buffer_new(vertex, type, count, vertices, count * vertex_size))
	{
		csmemset(vertex, 0, sizeof(*vertex));
		csmemset(triangle, 0, sizeof(*triangle));
		return;
	}
	if (!rasterizer_triangle_buffer_new(triangle, triangle_type, triangle_count, indices))
	{
		rasterizer_vertex_buffer_delete(vertex);
		csmemset(vertex, 0, sizeof(*vertex));
		csmemset(triangle, 0, sizeof(*triangle));
		return;
	}
	bound_remember(part, (unsigned long)count * (unsigned long)vertex_size);
}

static void bind_model(struct tag_import_donor *donor, struct model *model)
{
	long geometry_index;

	if (!model)
		return;
	for (geometry_index = 0; geometry_index < model->geometries.count; geometry_index++)
	{
		struct tag_import_model_geometry *geometry = tag_block_get_element_with_size(
			&model->geometries, geometry_index, sizeof(*geometry));
		long part_index;

		if (!geometry)
			continue;
		for (part_index = 0; part_index < geometry->parts.count; part_index++)
		{
			struct tag_import_model_part *part = tag_block_get_element_with_size(
				&geometry->parts, part_index, sizeof(*part));

			if (part)
				bind_part(donor, part);
		}
	}
}

/* the precache screen has already closed. Open it again and fill it as
the tags are copied, so a stop is a bar that stops moving */
static void import_progress(long done, long total)
{
	real fraction;

	if (total <= 0)
		return;
	fraction = (real)done / (real)total;
	if (fraction < 0.001f)
		fraction = 0.001f;
	if (fraction > 1.0f)
		fraction = 1.0f;
	error(_error_silent, "tag import: copying tag %ld of %ld", done, total);
	if (!import_progress_open)
	{
		progress_bar_begin(TRUE);
		import_progress_open = 1;
	}
	game_loading_progress_show(TRUE, fraction);
	main_pregame_render();
	main_present_frame();
}

static void import_progress_end(void)
{
	if (!import_progress_open)
		return;
	progress_bar_end();
	game_loading_progress_show(FALSE, 1.0f);
	import_progress_open = 0;
}

/* a range of pixels the pack has not got, to be read from the map */
static void donor_extra_add(struct tag_import_donor *donor, long offset, long size)
{
	long index;

	if (offset < 0x800 || size > DONOR_EXTRA_BYTES_MAX - donor_extra_bytes)
		return;
	for (index = 0; index < donor->extra_count; index++)
	{
		if (donor->extras[index].offset == offset && donor->extras[index].size >= size)
			return;
	}
	if (donor->extra_count == donor->extra_capacity)
	{
		long capacity = donor->extra_capacity ? donor->extra_capacity * 2 : 32;
		struct donor_extra *grown = realloc(donor->extras, (unsigned long)capacity * sizeof(*grown));

		if (!grown)
			return;
		donor->extras = grown;
		donor->extra_capacity = capacity;
	}
	donor->extras[donor->extra_count].offset = offset;
	donor->extras[donor->extra_count].size = size;
	donor->extras[donor->extra_count].bytes = NULL;
	donor->extra_count++;
	donor_extra_bytes += size;
}

static int donor_extra_less(void const *left, void const *right)
{
	struct donor_extra const *a = left;
	struct donor_extra const *b = right;

	return a->offset < b->offset ? -1 : a->offset > b->offset;
}

/* the ranges inflated from a disc map's zlib stream in one pass, each
copied out as the stream passes it (sorted by offset); whether it reached
the last */
static int extras_inflate(HANDLE file, struct donor_extra *extras, long count)
{
	z_stream stream;
	unsigned char input[0x2000];
	unsigned char output[0x4000];
	long produced = 0x800;
	long end = 0;
	long first = 0;
	long index;
	int result = Z_OK;
	DWORD got;

	for (index = 0; index < count; index++)
	{
		if (extras[index].offset + extras[index].size > end)
			end = extras[index].offset + extras[index].size;
	}
	if (!donor_file_at(file, 0x800))
		return 0;
	memset(&stream, 0, sizeof(stream));
	if (inflateInit(&stream) != Z_OK)
		return 0;
	while (produced < end && result != Z_STREAM_END)
	{
		long filled;

		if (stream.avail_in == 0)
		{
			got = 0;
			if (!ReadFile(file, input, (DWORD)sizeof(input), &got, NULL) || got == 0)
				break;
			stream.next_in = (Bytef *)input;
			stream.avail_in = got;
		}
		stream.next_out = (Bytef *)output;
		stream.avail_out = (unsigned int)sizeof(output);
		result = inflate(&stream, Z_NO_FLUSH);
		if (result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR)
			break;
		filled = (long)(sizeof(output) - stream.avail_out);
		if (filled <= 0 && stream.avail_in)
			break;
		for (index = first; index < count && extras[index].offset < produced + filled; index++)
		{
			struct donor_extra *extra = extras + index;
			long from = extra->offset > produced ? extra->offset : produced;
			long to = extra->offset + extra->size < produced + filled ? extra->offset + extra->size : produced + filled;

			if (from < to)
				memcpy(extra->bytes + (from - extra->offset), output + (from - produced), (unsigned long)(to - from));
		}
		produced += filled;
		while (first < count && extras[first].offset + extras[first].size <= produced)
			first++;
		if (result == Z_BUF_ERROR)
			result = Z_OK;
	}
	inflateEnd(&stream);

	return produced >= end;
}

/* the pixels the pack has not got of the bitmaps copied from a donor,
read from its map (a few HUD bitmaps: the stream is inflated once, to the
last of them); none when the map cannot be read, which draws them black */
static void donor_extras_fill(struct tag_import_donor *donor)
{
	char path[512];
	HANDLE file;
	DWORD physical;
	unsigned long started;
	long bytes = 0;
	long index;
	int read;

	if (!donor->from_pack || donor->extra_count <= 0)
		return;
	qsort(donor->extras, (unsigned long)donor->extra_count, sizeof(*donor->extras), donor_extra_less);
	for (index = 0; index < donor->extra_count; index++)
	{
		donor->extras[index].bytes = malloc((unsigned long)donor->extras[index].size);
		if (!donor->extras[index].bytes)
			break;
		bytes += donor->extras[index].size;
	}
	snprintf(path, sizeof(path), "%s%s.map", cache_files_map_directory(), donor->leaf);
	file = index == donor->extra_count ? CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL) :
		INVALID_HANDLE_VALUE;
	read = 0;
	started = system_milliseconds();
	if (file != INVALID_HANDLE_VALUE)
	{
		physical = GetFileSize(file, NULL);
		if (physical != INVALID_FILE_SIZE && (long)physical >= donor->file_length)
		{
			for (read = 1, index = 0; read && index < donor->extra_count; index++)
			{
				read = donor_file_at(file, donor->extras[index].offset) &&
					donor_file_read(file, donor->extras[index].bytes, (unsigned long)donor->extras[index].size);
			}
		}
		else
			read = extras_inflate(file, donor->extras, donor->extra_count);
		CloseHandle(file);
	}
	if (!read)
	{
		error(_error_silent, "tag import: the %ld ranges the pack has not got of %s cannot be read from %s",
			donor->extra_count, donor->leaf, path);
		for (index = 0; index < donor->extra_count; index++)
		{
			if (donor->extras[index].bytes)
				free(donor->extras[index].bytes);
			donor->extras[index].bytes = NULL;
		}
		return;
	}
	error(_error_silent, "tag import: %ld ranges the pack has not got (%ld bytes) read from %s in %lu ms",
		donor->extra_count, bytes, path, system_milliseconds() - started);
}

/* bytes at `offset` of a donor's map from the ranges read for the pack
(donor_extras_fill): how many */
static long extras_copy(struct tag_import_donor *donor, long offset, long size, unsigned char *buffer)
{
	long index;

	for (index = 0; index < donor->extra_count; index++)
	{
		struct donor_extra const *extra = donor->extras + index;

		if (extra->bytes && offset >= extra->offset && offset < extra->offset + extra->size)
		{
			long have = extra->offset + extra->size - offset;

			if (have > size)
				have = size;
			memcpy(buffer, extra->bytes + (offset - extra->offset), (unsigned long)have);
			return have;
		}
	}

	return 0;
}

/* a copied bitmap's or sound's file range, checked against the pack before
the tag bytes are dropped. A miss here is a black texture or an empty sample */
static void note_range(struct tag_import_donor *donor, char const *kind, char const *name, long index, long offset, long size)
{
	int cover;
	long probe;

	if (size < 0)
		size = 0;
	probe = size > 0 ? size : 1;
	cover = span_covers(donor, offset, probe);
	resource_checked++;
	if (size <= 0)
		resource_empty++;
	if (cover == 1)
		return;
	resource_uncovered++;
	if (donor->from_pack && size > 0 && !strcmp(kind, "bitm"))
		donor_extra_add(donor, offset, size);
	if (resource_logged >= 48)
		return;
	resource_logged++;
	error(_error_silent, "tag import: %s '%s' #%ld offset %ld size %ld %s (%s.map, %ld ranges)",
		kind, name ? name : "?", index, offset, size,
		cover < 0 ? "only partly in the pack" : "not in the pack",
		donor->leaf, donor->span_count);
}

static void note_bitmap(struct tag_import_donor *donor, char const *name, void *root)
{
	struct bitmap_group *group = root;
	long index;
	long count;

	if (!group)
		return;
	count = group->bitmaps.count;
	/* the pixels are at pixels_offset. pixel_data.file_offset is not a base
	on these caches (adding it lands off the pixels, and the blocks come out
	as noise). A loaded map never adds it; clear it so nothing does later */
	if (count > 256)
		count = 256;
	for (index = 0; index < count; index++)
	{
		struct bitmap_data *bitmap;

		bitmap = tag_block_get_element_with_size(&group->bitmaps, index, sizeof(*bitmap));
		if (!bitmap)
			continue;
		note_range(donor, "bitm", name, index, bitmap->pixels_offset, bitmap->pixels_size);
	}
	group->pixel_data.file_offset = 0;
}

static void note_sound(struct tag_import_donor *donor, char const *name, void *root, long datum)
{
	struct sound_definition *sound = root;
	long range_index;
	long range_count;

	if (!sound)
		return;
	range_count = sound->pitch_ranges.count;
	if (range_count > 64)
		range_count = 64;
	for (range_index = 0; range_index < range_count; range_index++)
	{
		struct sound_pitch_range *range;
		long permutation_index;
		long permutation_count;

		range = tag_block_get_element_with_size(&sound->pitch_ranges, range_index, sizeof(*range));
		if (!range)
			continue;
		permutation_count = range->permutations.count;
		if (permutation_count > 256)
			permutation_count = 256;
		for (permutation_index = 0; permutation_index < permutation_count; permutation_index++)
		{
			struct sound_permutation *permutation;

			permutation = tag_block_get_element_with_size(
				&range->permutations, permutation_index, sizeof(*permutation));
			if (!permutation)
				continue;
			/* the sample read goes through cache_tag_index (unknown2). A
			loaded map's check writes this sound's tag there; a copy still
			names the donor's, so the read never reaches the pack */
			permutation->unknown2 = (unsigned long)datum;
			permutation->unknown3 = (unsigned long)datum;
			note_range(donor, "snd!", name, permutation_index,
				permutation->samples.file_offset, permutation->samples.size);
		}
	}
}

static void import_wanted(void)
{
	long existing = 0;
	struct cache_file_tag_instance *instances = cache_files_tag_instances(&existing);
	long salt = 0;
	long index;
	short copying;
	short failed;

	copying = -1;
	failed = -1;
	resource_checked = 0;
	resource_uncovered = 0;
	resource_empty = 0;
	resource_logged = 0;
	if (!instances || wanted_count <= 0)
		return;
	/* (want_root keeps it to IMPORT_MAX: a list cut short here would copy
	tags without some of those they name) */
	if (wanted_count > IMPORT_MAX)
	{
		error(_error_silent, "tag import: %ld tags wanted, past %d; none copied", wanted_count, IMPORT_MAX);
		return;
	}
	qsort(wanted, (unsigned long)wanted_count, sizeof(*wanted), wanted_less);
	grown_instances = malloc((unsigned long)(existing + wanted_count) * sizeof(*grown_instances));
	if (!grown_instances)
	{
		platform_log("tag import: no room for the imported tags");
		return;
	}
	memcpy(grown_instances, instances, (unsigned long)existing * sizeof(*grown_instances));
	for (index = 0; index < existing; index++)
	{
		long instance_salt = (unsigned long)instances[index].tag_index >> 16;

		if (instance_salt > salt)
			salt = instance_salt;
	}
	saved_instances = instances;
	saved_count = existing;
	grown_count = existing;
	cache_files_set_tag_instances(grown_instances, grown_count);
	salt++;
	import_progress(0, wanted_count);
	for (index = 0; index < wanted_count && !import_stopped; index++)
	{
		struct wanted *entry = wanted + index;
		struct tag_import_donor *donor;
		struct tag_import_instance *source;
		struct tag_schema_definition const *schema;
		char const *name;
		void *root_source;
		void *root;
		char *name_copy;
		long datum;
		struct cache_file_tag_instance *dest;

		if (entry->donor < 0 || entry->donor >= donor_count || !entry->name[0])
			continue;
		donor = donors + entry->donor;
		if ((index & 15) == 0)
			import_progress(index, wanted_count);
		if (entry->donor != copying)
		{
			copying = entry->donor;
			error(_error_silent, "tag import: copying %s.map", donor->leaf);
		}
		if (!donor_bytes(donor) || !donor->datum_of || !donor->instances ||
			entry->absolute < 0 || entry->absolute >= donor->count)
		{
			if (failed != entry->donor)
			{
				failed = entry->donor;
				error(_error_silent, "tag import: could not read %s.map again", donor->leaf);
			}
			continue;
		}
		source = donor->instances + entry->absolute;
		schema = schema_of((unsigned long)source->group_tag);
		name = entry->name;
		if (!schema)
		{
			platform_log("tag import: left out %s, which has no schema", name);
			continue;
		}
		if (tag_loaded(source->group_tag, name) != NONE)
		{
			donor->datum_of[entry->absolute] = tag_loaded(source->group_tag, name);
			continue;
		}
		if (grown_count > 0xFFFE || copied_count >= IMPORT_MAX)
		{
			import_stopped = 1;
			break;
		}
		root_source = donor_address(donor, source->base_address);
		if (!root_source || schema->size <= 0 || !range_in(donor, root_source, (unsigned long)schema->size))
			continue;
		root = grab((unsigned long)schema->size);
		name_copy = grab((unsigned long)strlen(name) + 1);
		if (!root || !name_copy)
			break;
		memcpy(root, root_source, (unsigned long)schema->size);
		strcpy(name_copy, name);
		copy_fields(donor, root, schema, 0);
		if (source->group_tag == BITMAP_GROUP_TAG)
			note_bitmap(donor, name, root);
		if (source->group_tag == 'mode')
			bind_model(donor, root);
		datum = (salt << 16) | grown_count;
		if (source->group_tag == SOUND_DEFINITION_TAG)
			note_sound(donor, name, root, datum);
		dest = grown_instances + grown_count;
		memset(dest, 0, sizeof(*dest));
		dest->group_tag = source->group_tag;
		dest->parent_group_tags[0] = source->parent_group_tags[0];
		dest->parent_group_tags[1] = source->parent_group_tags[1];
		dest->tag_index = datum;
		dest->name = name_copy;
		dest->base_address = root;
		donor->datum_of[entry->absolute] = datum;
		donor->keep = 1;
		remember_import(grown_count, entry->donor);
		copied[copied_count].donor = entry->donor;
		copied[copied_count].root = root;
		copied[copied_count].schema = schema;
		copied_count++;
		grown_count++;
		cache_files_set_tag_instances(grown_instances, grown_count);
	}
	import_progress(wanted_count, wanted_count);
	import_progress_end();
	error(_error_silent, "tag import: %ld pixel and sample ranges, %ld not in the pack, %ld empty",
		resource_checked, resource_uncovered, resource_empty);
	for (index = 0; index < donor_count; index++)
	{
		release_donor_blob(donors + index);
		donor_extras_fill(donors + index);
	}
	if (import_stopped)
	{
		/* (out of room part way: tags copied without some they name would
		be drawn with what is not there. The level keeps only its own) */
		error(_error_silent, "tag import: ran out of room after %ld tags; none are kept", copied_count);
		cache_files_set_tag_instances(saved_instances, saved_count);
		free(grown_instances);
		grown_instances = NULL;
		saved_instances = NULL;
		saved_count = 0;
		grown_count = 0;
		bound_release();
		held_free();
		copied_count = 0;
		imports_forget();
		return;
	}
	/* a tag wanted from one map and found in another is that map's copy */
	for (index = 0; index < alias_count; index++)
	{
		struct wanted_alias *alias = aliases + index;

		if (alias->donor >= 0 && alias->donor < donor_count && donors[alias->donor].datum_of &&
			alias->target_donor >= 0 && alias->target_donor < donor_count && donors[alias->target_donor].datum_of)
			donors[alias->donor].datum_of[alias->absolute] =
				donors[alias->target_donor].datum_of[alias->target_absolute];
	}
	for (index = 0; index < copied_count; index++)
		retarget_fields(donors + copied[index].donor, copied[index].root, copied[index].schema, 0);
	if (copied_count)
		error(_error_silent, "tag import: imported %ld tags", copied_count);
}

static void wanted_free(void)
{
	if (wanted)
		free(wanted);
	if (wanted_slots)
		free(wanted_slots);
	if (aliases)
		free(aliases);
	wanted = NULL;
	wanted_slots = NULL;
	aliases = NULL;
	alias_count = 0;
}

/* ---------- a level's import */

int tag_import_begin(void)
{
	wanted_free();
	walk_steps = 0;
	wanted_full = 0;
	wanted_count = 0;
	alias_count = 0;
	wanted = malloc(WANTED_MAX * sizeof(*wanted));
	wanted_slots = malloc(WANTED_SLOTS * sizeof(*wanted_slots));
	aliases = malloc(WANTED_MAX * sizeof(*aliases));
	if (!wanted || !wanted_slots || !aliases)
	{
		error(_error_silent, "tag import: no room to gather tags");
		wanted_free();
		return 0;
	}
	memset(wanted, 0, WANTED_MAX * sizeof(*wanted));
	memset(wanted_slots, 0, WANTED_SLOTS * sizeof(*wanted_slots));

	return 1;
}

struct tag_import_donor *tag_import_open(char const *pack_name, char const *map)
{
	struct pack *pack;
	struct tag_import_donor *donor;
	int index;

	if (!wanted || !pack_name || !map)
		return NULL;
	pack = pack_named(pack_name);
	if (!pack)
		return NULL;
	/* (a map of the pack read already for this level: its bytes again) */
	for (index = 0; index < donor_count; index++)
	{
		donor = donors + index;
		if (donor->pack != pack || _stricmp(donor->leaf, map))
			continue;
		if (!donor_bytes(donor))
			return NULL;
		/* (let go with its bytes: what was wanted of it before is known by
		name, wanted_find) */
		if (!donor->seen)
		{
			donor->seen = malloc((unsigned long)donor->count);
			if (!donor->seen)
				return NULL;
			memset(donor->seen, 0, (unsigned long)donor->count);
		}
		return donor;
	}
	if (donor_count >= DONOR_COUNT)
	{
		error(_error_silent, "tag import: no room to read %s.map of %s.pack", map, pack_name);
		return NULL;
	}
	donor = donors + donor_count;
	if (!open_donor(donor, map, pack, 0))
	{
		close_donor_bytes(donor);
		return NULL;
	}
	donor_count++;
	error(_error_silent, "tag import: read %s.map of %s.pack (%ld tags)", map, pack_name, donor->count);

	return donor;
}

int tag_import_want(struct tag_import_donor *donor, long absolute)
{
	if (!donor || !donor->instances || absolute < 0 || absolute >= donor->count)
		return 0;
	return want_root(donor, absolute);
}

void tag_import_release(struct tag_import_donor *donor)
{
	if (donor)
		release_donor_blob(donor);
}

static long donor_find(struct tag_import_donor *donor, long group, char const *name)
{
	long index;

	for (index = 0; index < donor->count; index++)
	{
		struct tag_import_instance *instance = donor->instances + index;
		char const *instance_name_text;

		if (instance->group_tag != group)
			continue;
		instance_name_text = instance_name(donor, instance);
		if (instance_name_text && !_stricmp(instance_name_text, name))
			return index;
	}
	return NONE;
}

int tag_import_roots(char const *pack, char const *map, struct tag_import_root const *roots, int count)
{
	struct tag_import_donor *donor;
	int root;
	int missing = 0;
	int found = 0;

	for (root = 0; root < count; root++)
		missing += tag_loaded(roots[root].group, roots[root].name) == NONE;
	if (!missing)
		return count;
	error(_error_silent, "tag import: reading %d tags from %s.map of %s.pack", missing, map, pack);
	donor = tag_import_open(pack, map);
	if (!donor)
		return count - missing;
	for (root = 0; root < count; root++)
	{
		long index;

		if (tag_loaded(roots[root].group, roots[root].name) != NONE)
		{
			found++;
			continue;
		}
		index = donor_find(donor, roots[root].group, roots[root].name);
		if (index == NONE)
			error(_error_silent, "tag import: %s.map has no %s", map, roots[root].name);
		else if (want_root(donor, index))
			found++;
	}
	release_donor_blob(donor);

	return found;
}

void tag_import_run(void)
{
	if (wanted)
		import_wanted();
}

void tag_import_end(void)
{
	wanted_free();
}

void tag_import_log(char const *who)
{
	int index;

	donor_reads = 0;
	donor_misses = 0;
	donor_logged = 0;
	donor_saw_bitmap = 0;
	donor_saw_sound = 0;
	memset(donor_noted, 0, sizeof(donor_noted));
	/* the tag blobs are already gone. The pack ranges stay until the level
	unloads: bitmap pixels and sound samples are read from them in play */
	for (index = 0; index < donor_count; index++)
		error(_error_silent, "%s: %s.map keeps %ld pack ranges", who,
			donors[index].leaf, donors[index].from_pack ? donors[index].span_count : 0);
	error(_error_silent, "%s: %ld tags copied, %ld model parts in %lu vertex bytes", who, copied_count,
		bound_part_count, bound_vertex_bytes);
}

void tag_import_level_unloaded(void)
{
	donors_close();
	bound_release();
	held_free();
	if (saved_instances)
		cache_files_set_tag_instances(saved_instances, saved_count);
	if (grown_instances)
		free(grown_instances);
	grown_instances = NULL;
	saved_instances = NULL;
	saved_count = 0;
	grown_count = 0;
	wanted_free();
	wanted_count = 0;
	copied_count = 0;
	imports_forget();
	import_stopped = 0;
}

/* ---------- looking at a donor */

struct tag_import_donor *tag_import_peek(char const *pack_name, char const *map)
{
	struct tag_import_donor *donor;
	struct pack *pack = pack_name ? pack_named(pack_name) : NULL;
	int was_open = pack && pack->state == 1;

	donor = malloc(sizeof(*donor));
	if (!donor)
		return NULL;
	if (!open_donor(donor, map, pack, 1))
	{
		close_donor_bytes(donor);
		if (donor->file && donor->file != INVALID_HANDLE_VALUE)
			CloseHandle(donor->file);
		free(donor);
		/* (the pack left as it was: a level in play reads its pixels from it) */
		if (pack && !was_open)
			pack_close(pack);
		return NULL;
	}
	donor->opened_pack = !was_open;

	return donor;
}

void tag_import_peek_close(struct tag_import_donor *donor)
{
	int opened = donor && donor->opened_pack;
	struct pack *pack;

	if (!donor)
		return;
	pack = donor->pack;
	close_donor_bytes(donor);
	if (donor->file && donor->file != INVALID_HANDLE_VALUE)
		CloseHandle(donor->file);
	free(donor);
	if (pack && opened)
		pack_close(pack);
}

long tag_import_tag_count(struct tag_import_donor *donor)
{
	return donor && donor->instances ? donor->count : 0;
}

long tag_import_tag_group(struct tag_import_donor *donor, long absolute)
{
	if (!donor || !donor->instances || absolute < 0 || absolute >= donor->count)
		return NONE;
	return donor->instances[absolute].group_tag;
}

char const *tag_import_tag_name(struct tag_import_donor *donor, long absolute)
{
	if (!donor || !donor->instances || absolute < 0 || absolute >= donor->count)
		return NULL;
	return instance_name(donor, donor->instances + absolute);
}

void *tag_import_tag_data(struct tag_import_donor *donor, long absolute, unsigned long bytes)
{
	if (!donor || !donor->instances || absolute < 0 || absolute >= donor->count)
		return NULL;
	return tag_import_address(donor, donor->instances[absolute].base_address, bytes);
}

long tag_import_scenario(struct tag_import_donor *donor)
{
	struct tag_import_header *tags;
	long absolute;

	if (!donor || !donor->bytes)
		return NONE;
	tags = (struct tag_import_header *)donor->bytes;
	absolute = tags->scenario_tag_index & 0xFFFF;

	return tags->scenario_tag_index != NONE && absolute < donor->count ? absolute : NONE;
}

void *tag_import_address(struct tag_import_donor *donor, void const *pointer, unsigned long bytes)
{
	void *address;

	if (!donor || !donor->bytes)
		return NULL;
	address = donor_address(donor, (void *)pointer);

	return address && range_in(donor, address, bytes) ? address : NULL;
}

char const *tag_import_string(struct tag_import_donor *donor, void const *pointer)
{
	char const *text = tag_import_address(donor, pointer, 1);
	unsigned long room;

	if (!text)
		return NULL;
	room = (unsigned long)(donor->bytes + donor->size - (unsigned char const *)text);

	return memchr(text, 0, room > 256 ? 256 : room) ? text : NULL;
}

char const *tag_import_donor_map(struct tag_import_donor *donor)
{
	return donor ? donor->leaf : "";
}

void tag_import_tag_table_fingerprint(long *count, unsigned long *hash)
{
	long index;
	struct cache_file_tag_instance *instances = cache_files_tag_instances(count);
	unsigned long value = 0x811C9DC5UL;

	for (index = 0; instances && index < *count; index++)
	{
		char const *name = instances[index].name;
		short byte_index;

		for (byte_index = 0; byte_index < 4; byte_index++)
			value = ((value ^ ((unsigned long)instances[index].group_tag >> (byte_index * 8) & 0xFF)) * 16777619UL) &
				0xFFFFFFFFUL;
		for (; name && *name; name++)
			value = ((value ^ (unsigned char)*name) * 16777619UL) & 0xFFFFFFFFUL;
		value = ((value ^ 0xFF) * 16777619UL) & 0xFFFFFFFFUL;
	}
	if (!instances)
		*count = 0;
	*hash = value;
}

static void fourcc_text(long group, char text[5])
{
	int index;

	text[0] = (char)((group >> 24) & 0xFF);
	text[1] = (char)((group >> 16) & 0xFF);
	text[2] = (char)((group >> 8) & 0xFF);
	text[3] = (char)(group & 0xFF);
	text[4] = 0;
	for (index = 0; index < 4; index++)
	{
		if (text[index] < 32 || text[index] > 126)
			text[index] = '?';
	}
}

static int leading_zero(void const *buffer, long size)
{
	unsigned char const *bytes = buffer;
	long index;
	long check;

	check = size;
	if (check > 32)
		check = 32;
	if (check <= 0)
		return 1;
	for (index = 0; index < check; index++)
	{
		if (bytes[index] != 0)
			return 0;
	}

	return 1;
}

static void log_donor_read(struct tag_import_donor *donor, long slot, long absolute, long offset, long size, long filled,
	void const *buffer)
{
	long tags = 0;
	struct cache_file_tag_instance *instances;
	char const *name;
	long group;
	char kind[5];
	int miss;
	int zero;
	int named;
	int detail;

	donor_reads++;
	miss = filled < size;
	if (miss)
		donor_misses++;
	instances = cache_files_tag_instances(&tags);
	name = "?";
	group = 0;
	if (instances && absolute >= 0 && absolute < tags)
	{
		if (instances[absolute].name)
			name = instances[absolute].name;
		group = instances[absolute].group_tag;
	}
	fourcc_text(group, kind);
	zero = !miss && leading_zero(buffer, filled);
	named = name && (strstr(name, "elite") || strstr(name, "carrier"));
	detail = miss || zero || named ||
		(group == BITMAP_GROUP_TAG && !donor_saw_bitmap) ||
		(group == SOUND_DEFINITION_TAG && !donor_saw_sound);
	if (named && slot >= 0 && slot < IMPORT_MAX && donor_noted[slot] && !miss && !zero)
		detail = 0;
	if (detail && donor_logged < 64)
	{
		donor_logged++;
		if (slot >= 0 && slot < IMPORT_MAX)
			donor_noted[slot] = 1;
		if (!miss && group == BITMAP_GROUP_TAG)
			donor_saw_bitmap = 1;
		if (!miss && group == SOUND_DEFINITION_TAG)
			donor_saw_sound = 1;
		error(_error_silent, "tag import: %s %s '%s' offset %ld size %ld filled %ld donor %s (%ld ranges)",
			miss ? (filled == 0 ? "miss" : "short") : (zero ? "zero" : "ok"),
			kind, name, offset, size, filled, donor->leaf, donor->from_pack ? donor->span_count : 0);
	}
	if (donor_reads == 1 || donor_misses == 1 || (donor_reads % 200) == 0)
		error(_error_silent, "tag import: resource reads %ld, misses %ld", donor_reads, donor_misses);
}

/* whether a tag is one copied from another map, whose bitmap pixels
tag_import_donor_read reads at once (the texture cache keeps them outside
the Xbox's: xbox_texture_cache.c) */
int tag_import_tag_imported(long tag_index)
{
	if (import_count <= 0 || tag_index == NONE)
		return 0;
	return (import_marked[(tag_index & 0xFFFF) >> 3] >> (tag_index & 7)) & 1;
}

int tag_import_donor_read(long tag_index, long offset, long size, void *buffer)
{
	long absolute;
	long index;
	long filled;
	long asked;
	struct tag_import_donor *donor;

	if (import_count <= 0 || tag_index == NONE || !buffer || offset < 0 || size < 0)
		return 0;
	absolute = tag_index & 0xFFFF;
	for (index = 0; index < import_count; index++)
	{
		if (import_absolute[index] == absolute)
			break;
	}
	if (index >= import_count)
		return 0;
	donor = donors + import_donor[index];
	memset(buffer, 0, (unsigned long)size);
	asked = size;
	filled = 0;
	if (offset < donor->file_length)
	{
		if (size > donor->file_length - offset)
			size = donor->file_length - offset;
		/* (a level's donors are all a pack's) */
		filled = pack_copy(donor, offset, size, buffer);
		if (filled < size)
			filled += extras_copy(donor, offset + filled, size - filled, (unsigned char *)buffer + filled);
	}
	log_donor_read(donor, index, absolute, offset, asked, filled, buffer);

	return 1;
}
