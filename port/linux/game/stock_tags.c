/*
STOCK_TAGS.C

Mutators ship no retail data: an add-on names the stock tags it uses by their
groups and names (its imports, addon_file.h), and a tag edit names the tags it
copies from or refers to. A stock map has them all; a Custom Edition map may
not. This unit gives such a map the tags it lacks, from the player's own stock
maps. It does nothing unless a Custom Edition map is loaded and a mutator asks
for a tag the map lacks, and every entry to it returns at once otherwise.

A stock map is inflated, and its tags read to where they were linked to run:
the Xbox tag cache, which a Custom Edition map's tags (at 0x40440000) leave
free. There the validator checks them all, as it does when the map itself is
played, and a PAL map's are put back to the NTSC maps' values (pal_tags.c), so
that their pointers can be followed and their values are every machine's.

The tag asked for, and each tag it uses that the playing map has not (found by
group and name, the map's own first), are copied out through their groups'
schemas (tag_walk.c): each block and data is given a copy, each reference and
tag index the tag it names in the playing map's table, each bitmap's pixels and
sound's samples are kept in memory here (the map's file is let go of by
stock_tags_finish), and each model part is given buffers of its own
(custom_edition_stock_model_make). Either every tag a tag needs is added, or
none is. A scenario or a structure bsp is never added: a tag that leads to one
is refused.

An add-on's imports are found as the map's tags load, before the add-ons' and
the menus' tags, so their indices depend only on the map and the add-ons on, as
the add-ons' do. A tag edit's come later (a client has them only from its
host), so they go after the menus' tags, whose number can be this machine's
alone: at a fixed distance after the tags before the menus', which the menus'
do not reach, and so at the same index on every machine playing the same edits.

Bitmaps here are drawn as the Xbox's are, in the Xbox's channels, on a map whose
others are Halo PC's (addons_bitmap_is_halo_pc). What is not done: a PAL map's
first-person animations are not paced as pal_tags.c paces them on a PAL map,
and only the multiplayer maps are looked in (every add-on's imports are in each
of them: tools/addon_build.c's --stock).
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_files/tag_groups.h"
#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "cache/physical_memory_map.h"
#include "models/model_definitions.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"
#include "stock_tags.h"
#include "tag_schema.h"
#include "tag_walk.h"
#include "../../third_party/zlib/zlib_prefixed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

enum
{
	/* the most tags added in one place in the table (the Falcon's imports are
	56, and what they use) */
	MAXIMUM_STOCK_TAGS = 4096,
	/* how far after the tags before the menus' those of stock_tags_find go */
	STOCK_TAGS_AFTER_MENUS_DISTANCE = 8192,
	MAXIMUM_STOCK_GEOMETRIES = 256,
	/* the names no stock map has, kept so that they are looked for once */
	MAXIMUM_STOCK_MISSES = 64,
	MAXIMUM_STOCK_TAG_NAME_BYTES = 256,
	STOCK_ARENA_CHUNK_BYTES = 0x100000,
	/* the most bytes of pixels and samples kept */
	MAXIMUM_STOCK_RESOURCE_BYTES = 0x20000000,

	CACHE_FILE_HEADER_SIGNATURE_TAG = 'head',
	CACHE_FILE_FOOTER_SIGNATURE_TAG = 'foot',

	BITMAP_TAG_GROUP = 'bitm',
	SOUND_TAG_GROUP = 'snd!',
	MODEL_TAG_GROUP = 'mode',
	SCENARIO_TAG_GROUP = 'scnr',
	STRUCTURE_BSP_TAG_GROUP = 'sbsp',

	/* a sound's pitch ranges and their permutations, and where a permutation
	names its own tag (addons.c's) */
	SOUND_PITCH_RANGES_OFFSET = 0x98,
	SOUND_PITCH_RANGE_BYTES = 0x48,
	SOUND_PITCH_RANGE_PERMUTATIONS_OFFSET = 0x3C,
	SOUND_PERMUTATION_BYTES = 0x7C,
	SOUND_PERMUTATION_CACHE_TAG_INDEX_OFFSET = 0x34,
	SOUND_PERMUTATION_RUNTIME_TAG_INDEX_OFFSET = 0x3C,
};

/* the multiplayer maps, in the order they are looked in */
static char const *const stock_map_names[] =
{
	"bloodgulch", "beavercreek", "boardingaction", "carousel", "chillout", "damnation",
	"hangemhigh", "longest", "prisoner", "putput", "ratrace", "sidewinder", "wizard",
};

/* ---------- structures */

/* the game's tag table's entry (cache_files.c's cache_file_tag_instance) */
struct stock_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

/* a stock map's tags' header (cache_files.c's cache_file_tag_header) */
struct stock_tag_header
{
	struct stock_tag_instance *instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long vertex_buffer_count;
	void *vertex_buffers;
	long index_buffer_count;
	void *index_buffers;
	unsigned long signature;
};

/* a stock map's header (cache_files.c's cache_file_header) */
struct stock_file_header
{
	unsigned long header_signature;
	long version;
	long file_length;
	byte reservedC[4];
	long tag_data_offset;
	long tag_data_size;
	byte reserved18[8];
	char name[0x20];
	char build[0x20];
	byte reserved60[4];
	unsigned long checksum;
	byte reserved68[0x794];
	unsigned long footer_signature;
};

typedef char verify_stock_tag_instance_size[sizeof(struct stock_tag_instance) == 0x20 ? 1 : -1];
typedef char verify_stock_file_header_size[sizeof(struct stock_file_header) == CACHE_FILE_HEADER_BYTES ? 1 : -1];

/* the tags added in one place in the table: entries after those of the table
they replaced (parent), first of them at index first */
struct stock_table
{
	struct stock_tag_instance *instances;
	long first;
	long count;
	struct stock_tag_instance *parent;
	long parent_count;
};

struct stock_arena_chunk
{
	struct stock_arena_chunk *next;
	unsigned long used;
	unsigned long size;
};

struct stock_walk
{
	struct stock_table *table;
	boolean failed;
};

/* ---------- globals */

/* TRUE once a tag has been added: until then nothing here is in use (the
game's hooks test it: stock_tags.h) */
boolean stock_tags_in_map;

static struct
{
	struct stock_table before_menus;
	struct stock_table after_menus;
	struct stock_arena_chunk *arena;
	byte *resources;
	unsigned long resource_bytes;
	unsigned long resource_capacity;
	void *geometries[MAXIMUM_STOCK_GEOMETRIES];
	short geometry_count;

	/* the stock map held (tag_header NULL: none): its file inflated, its tags
	in the Xbox tag cache, and which tag each of them became */
	short map_index;
	byte *file;
	long file_length;
	struct stock_tag_header *tag_header;
	long *handles;

	/* this find's tags, by their stock map's indices, in the order added */
	long pending[MAXIMUM_STOCK_TAGS];
	long pending_count;

	/* what this map has asked for that no stock map has */
	long miss_groups[MAXIMUM_STOCK_MISSES];
	char miss_names[MAXIMUM_STOCK_MISSES][MAXIMUM_STOCK_TAG_NAME_BYTES];
	short miss_count;
	/* for the log: how many tags have been added, and read */
	long added_count;
	long reads;
} stock_tags_globals;

/* ---------- prototypes/CACHE_FILES.C */

void *cache_files_tag_instances(long *count);
void cache_files_set_tag_instances(void *instances, long count);

/* ---------- prototypes/MENU_TAGS.C */

long menu_tags_first_index(void);

/* ---------- prototypes/PAL_TAGS.C */

void pal_tags_loaded(char const *build);

/* ---------- private code */

/* a group tag's four characters, for the log */
static char const *group_text(
	long group_tag)
{
	static char text[5];

	text[0] = (char)(group_tag >> 24);
	text[1] = (char)(group_tag >> 16);
	text[2] = (char)(group_tag >> 8);
	text[3] = (char)group_tag;
	text[4] = 0;

	return text;
}

static void *arena_allocate(
	unsigned long size)
{
	struct stock_arena_chunk *chunk = stock_tags_globals.arena;
	unsigned long header = (sizeof(*chunk) + 15) & ~15UL;
	void *address;

	size = (size + 15) & ~15UL;
	if (!chunk || size > chunk->size - chunk->used)
	{
		unsigned long bytes = MAX(size, (unsigned long)STOCK_ARENA_CHUNK_BYTES);

		chunk = malloc(header + bytes);
		if (!chunk)
			return NULL;
		chunk->next = stock_tags_globals.arena;
		chunk->used = 0;
		chunk->size = bytes;
		stock_tags_globals.arena = chunk;
	}
	address = (byte *)chunk + header + chunk->used;
	chunk->used += size;

	return address;
}

static void *arena_copy(
	void const *source,
	unsigned long size)
{
	void *copy = arena_allocate(size);

	if (copy)
		csmemcpy(copy, source, size);

	return copy;
}

/* size bytes at offset in the stock map's file kept: their offset among the
pixels and samples, or -1 */
static long resource_add(
	long offset,
	long size)
{
	unsigned long start = stock_tags_globals.resource_bytes;

	if (offset < 0 || size < 0 || offset > stock_tags_globals.file_length ||
		size > stock_tags_globals.file_length - offset ||
		(unsigned long)size > (unsigned long)MAXIMUM_STOCK_RESOURCE_BYTES - start)
	{
		return -1;
	}
	if (start + size > stock_tags_globals.resource_capacity)
	{
		unsigned long capacity = MAX(start + size, stock_tags_globals.resource_capacity * 2);
		byte *resources = malloc(capacity);

		if (!resources)
			return -1;
		if (start)
			csmemcpy(resources, stock_tags_globals.resources, start);
		if (stock_tags_globals.resources)
			free(stock_tags_globals.resources);
		stock_tags_globals.resources = resources;
		stock_tags_globals.resource_capacity = capacity;
	}
	csmemcpy(stock_tags_globals.resources + start, stock_tags_globals.file + offset, size);
	stock_tags_globals.resource_bytes = start + size;

	return (long)start;
}

static void stock_map_close(
	void)
{
	if (stock_tags_globals.file)
		free(stock_tags_globals.file);
	if (stock_tags_globals.handles)
		free(stock_tags_globals.handles);
	stock_tags_globals.file = NULL;
	stock_tags_globals.handles = NULL;
	stock_tags_globals.tag_header = NULL;

	return;
}

/* the stock map's file inflated, or NULL (said) */
static byte *stock_map_file_read(
	char const *path,
	struct stock_file_header *header)
{
	FILE *file = fopen(path, "rb");
	byte *inflated = NULL;
	byte *compressed = NULL;
	long size;

	if (!file)
	{
		error(_error_silent, "stock tags: %s cannot be opened", path);
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < CACHE_FILE_HEADER_BYTES || fseek(file, 0, SEEK_SET) ||
		fread(header, CACHE_FILE_HEADER_BYTES, 1, file) != 1 ||
		header->header_signature != CACHE_FILE_HEADER_SIGNATURE_TAG ||
		header->footer_signature != CACHE_FILE_FOOTER_SIGNATURE_TAG ||
		header->version != CACHE_FILE_VERSION_XBOX || header->file_length < CACHE_FILE_HEADER_BYTES ||
		size > header->file_length)
	{
		error(_error_silent, "stock tags: %s is not an Xbox map", path);
		fclose(file);
		return NULL;
	}
	inflated = malloc(header->file_length);
	compressed = malloc(size - CACHE_FILE_HEADER_BYTES + 1);
	if (!inflated || !compressed || fread(compressed, size - CACHE_FILE_HEADER_BYTES, 1, file) != 1)
	{
		error(_error_silent, "stock tags: %s cannot be read (%ld bytes)", path, (long)header->file_length);
		if (inflated)
			free(inflated);
		inflated = NULL;
	}
	else
	{
		csmemcpy(inflated, header, CACHE_FILE_HEADER_BYTES);
		if (size == header->file_length)
		{
			csmemcpy(inflated + CACHE_FILE_HEADER_BYTES, compressed, size - CACHE_FILE_HEADER_BYTES);
		}
		else
		{
			uLongf inflated_size = (uLongf)(header->file_length - CACHE_FILE_HEADER_BYTES);

			if (uncompress(inflated + CACHE_FILE_HEADER_BYTES, &inflated_size, compressed,
				(uLong)(size - CACHE_FILE_HEADER_BYTES)) != Z_OK ||
				inflated_size != (uLongf)(header->file_length - CACHE_FILE_HEADER_BYTES))
			{
				error(_error_silent, "stock tags: %s cannot be inflated", path);
				free(inflated);
				inflated = NULL;
			}
		}
	}
	if (compressed)
		free(compressed);
	fclose(file);

	return inflated;
}

/* a stock map's tags in the Xbox tag cache, checked and as the NTSC maps':
FALSE (said) if they cannot be used */
static boolean stock_map_open(
	short map_index)
{
	struct stock_file_header header;
	struct stock_tag_header *tag_header = physical_memory_get_tag_cache_base_address();
	char path[256];
	char const *region;

	stock_map_close();
	snprintf(path, sizeof(path), "%s%s.map", cache_files_map_directory(), stock_map_names[map_index]);
	stock_tags_globals.file = stock_map_file_read(path, &header);
	if (!stock_tags_globals.file)
		return FALSE;
	stock_tags_globals.file_length = header.file_length;
	if (header.tag_data_offset < CACHE_FILE_HEADER_BYTES || header.tag_data_size < (long)sizeof(*tag_header) ||
		header.tag_data_size > TAG_CACHE_SIZE || header.tag_data_offset > header.file_length - header.tag_data_size)
	{
		error(_error_silent, "stock tags: %s's tags are outside it", path);
		stock_map_close();
		return FALSE;
	}
	csmemcpy(tag_header, stock_tags_globals.file + header.tag_data_offset, header.tag_data_size);
	if (!tag_validate_stock_tags(tag_header, header.tag_data_size, header.file_length, stock_map_names[map_index]))
	{
		error(_error_silent, "stock tags: %s's tags are refused by the validator (above)", path);
		stock_map_close();
		return FALSE;
	}
	stock_tags_globals.handles = malloc(tag_header->tag_count * sizeof(*stock_tags_globals.handles));
	if (!stock_tags_globals.handles)
	{
		stock_map_close();
		return FALSE;
	}
	csmemset(stock_tags_globals.handles, 0xFF, tag_header->tag_count * sizeof(*stock_tags_globals.handles));
	stock_tags_globals.tag_header = tag_header;
	stock_tags_globals.map_index = map_index;

	/* (a PAL map's values put back as when it is played, then pal_tags.c told
	again that none of the map played is PAL) */
	region = cache_files_build_region(header.build);
	if (region && !strcmp(region, "PAL"))
	{
		long count;
		void *instances = cache_files_tag_instances(&count);

		cache_files_set_tag_instances(tag_header->instances, tag_header->tag_count);
		pal_tags_loaded(header.build);
		cache_files_set_tag_instances(instances, count);
		pal_tags_loaded("");
	}

	return TRUE;
}

/* a stock map's tag of a group and name, or NONE */
static long stock_map_tag_find(
	long group_tag,
	char const *name)
{
	struct stock_tag_header const *tag_header = stock_tags_globals.tag_header;
	long index;

	for (index = 0; tag_header && index < tag_header->tag_count; index++)
	{
		if (tag_header->instances[index].group_tag == group_tag && !_stricmp(tag_header->instances[index].name, name))
			return index;
	}

	return NONE;
}

/* the handle of the table's next tag in this find */
static long table_next_handle(
	struct stock_table const *table)
{
	long absolute_index = table->first + table->count + stock_tags_globals.pending_count;

	return DATUM_INDEX_NEW(absolute_index, absolute_index & UNSIGNED_SHORT_MAX);
}

/* the handle in the playing map's table of a tag of the stock map, by its
handle there: the map's own of its group and name, else one added with this
find, NONE if it names none */
static long stock_handle_make(
	struct stock_walk *walk,
	long stock_handle)
{
	struct stock_tag_header const *tag_header = stock_tags_globals.tag_header;
	struct stock_tag_instance const *instance;
	long stock_index;
	long found;

	if (stock_handle == NONE)
		return NONE;
	stock_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(stock_handle);
	if (stock_index >= tag_header->tag_count)
		return NONE;
	if (stock_tags_globals.handles[stock_index] != NONE)
		return stock_tags_globals.handles[stock_index];
	instance = &tag_header->instances[stock_index];
	found = tag_loaded(instance->group_tag, instance->name);
	if (found != NONE)
	{
		stock_tags_globals.handles[stock_index] = found;
		return found;
	}
	if (instance->group_tag == SCENARIO_TAG_GROUP || instance->group_tag == STRUCTURE_BSP_TAG_GROUP ||
		!instance->base_address)
	{
		error(_error_silent, "stock tags: '%s' leads to a tag of a map's own level", instance->name);
		walk->failed = TRUE;
		return NONE;
	}
	if (walk->table->first + walk->table->count + stock_tags_globals.pending_count >= UNSIGNED_SHORT_MAX ||
		walk->table->count + stock_tags_globals.pending_count >= MAXIMUM_STOCK_TAGS)
	{
		error(_error_silent, "stock tags: more than %d tags", MAXIMUM_STOCK_TAGS);
		walk->failed = TRUE;
		return NONE;
	}
	found = table_next_handle(walk->table);
	stock_tags_globals.pending[stock_tags_globals.pending_count++] = stock_index;
	stock_tags_globals.handles[stock_index] = found;

	return found;
}

/* each of a copied tag's blocks and data given copies of their own, its file
data kept, and what it names made the playing map's */
static void stock_copy_visit(
	void *context,
	struct tag_schema_field const *field,
	void *address)
{
	struct stock_walk *walk = context;

	if (walk->failed)
		return;
	switch (field->type)
	{
	case _tag_schema_block:
	{
		struct tag_block *block = address;
		struct tag_schema_definition const *element = field->definition;

		block->definition = NULL;
		if (block->count > 0 && block->address)
		{
			block->address = arena_copy(block->address, (unsigned long)block->count * element->size);
			if (!block->address)
			{
				block->count = 0;
				walk->failed = TRUE;
			}
		}
		else
		{
			block->count = 0;
			block->address = NULL;
		}
		break;
	}

	case _tag_schema_data:
	{
		struct tag_data *data = address;

		data->definition = NULL;
		if (data->size > 0 && data->address)
		{
			data->address = arena_copy(data->address, data->size);
			if (!data->address)
				walk->failed = TRUE;
		}
		break;
	}

	case _tag_schema_file_data:
	{
		struct tag_data *data = address;

		data->definition = NULL;
		data->address = NULL;
		if (data->size > 0)
		{
			long offset = resource_add(data->file_offset, data->size);

			if (offset < 0)
				walk->failed = TRUE;
			else
				data->file_offset = offset;
		}
		break;
	}

	case _tag_schema_reference:
	{
		struct tag_reference *reference = address;
		struct stock_tag_instance const *instance = NULL;
		long stock_index = reference->index == NONE ? NONE : DATUM_INDEX_TO_ABSOLUTE_INDEX(reference->index);

		if (stock_index != NONE && stock_index < stock_tags_globals.tag_header->tag_count)
			instance = &stock_tags_globals.tag_header->instances[stock_index];
		reference->index = stock_handle_make(walk, reference->index);
		/* (its name is the tag's, which lasts as long as these tags do) */
		if (instance && reference->index != NONE)
		{
			char *name = arena_copy(instance->name, strlen(instance->name) + 1);

			if (!name)
				walk->failed = TRUE;
			else
				reference->name = name;
		}
		else
		{
			reference->name = (char *)"";
			reference->name_length = 0;
		}
		break;
	}

	case _tag_schema_tag_index:
		*(long *)address = stock_handle_make(walk, *(long *)address);
		break;

	default:
		break;
	}

	return;
}

/* a bitmap's pixels kept, and the fields the game sets as it draws one left
for it (as tools/addon_build.c's bitmaps_write) */
static boolean stock_bitmap_finish(
	struct bitmap_group *group,
	long handle)
{
	long index;

	for (index = 0; index < group->bitmaps.count; index++)
	{
		struct bitmap_data *bitmap = (struct bitmap_data *)group->bitmaps.address + index;

		if (bitmap->pixels_size > 0)
		{
			long offset = resource_add(bitmap->pixels_offset, bitmap->pixels_size);

			if (offset < 0)
				return FALSE;
			bitmap->pixels_offset = offset;
		}
		bitmap->tag_index = handle;
		bitmap->hardware_format = NULL;
		bitmap->base_address = NULL;
	}

	return TRUE;
}

/* a sound's permutations naming it (the sound cache reads them by this) */
static void stock_sound_finish(
	byte *root,
	long handle)
{
	struct tag_block const *pitch_ranges = (struct tag_block const *)(root + SOUND_PITCH_RANGES_OFFSET);
	long element;

	for (element = 0; element < pitch_ranges->count; element++)
	{
		struct tag_block const *permutations = (struct tag_block const *)((byte *)pitch_ranges->address +
			element * SOUND_PITCH_RANGE_BYTES + SOUND_PITCH_RANGE_PERMUTATIONS_OFFSET);
		long permutation;

		for (permutation = 0; permutation < permutations->count; permutation++)
		{
			byte *bytes = (byte *)permutations->address + permutation * SOUND_PERMUTATION_BYTES;

			*(long *)(bytes + SOUND_PERMUTATION_CACHE_TAG_INDEX_OFFSET) = handle;
			*(long *)(bytes + SOUND_PERMUTATION_RUNTIME_TAG_INDEX_OFFSET) = handle;
		}
	}

	return;
}

/* the find's tag at index among those pending copied into the table's next
entry: FALSE (said) if it cannot be */
static boolean stock_tag_copy(
	struct stock_table *table,
	long pending_index)
{
	struct stock_tag_instance const *source =
		&stock_tags_globals.tag_header->instances[stock_tags_globals.pending[pending_index]];
	struct stock_tag_instance *instance = &table->instances[table->first + table->count + pending_index];
	struct tag_schema_group const *group = tag_walk_group((unsigned long)source->group_tag, FALSE);
	struct stock_walk walk;
	long absolute_index = table->first + table->count + pending_index;
	long handle = DATUM_INDEX_NEW(absolute_index, absolute_index & UNSIGNED_SHORT_MAX);
	byte *root;

	if (!group)
	{
		error(_error_silent, "stock tags: '%s' is of a group that cannot be copied", source->name);
		return FALSE;
	}
	root = arena_copy(source->base_address, group->definition->size);
	instance->name = arena_copy(source->name, strlen(source->name) + 1);
	if (!root || !instance->name)
		return FALSE;
	instance->group_tag = source->group_tag;
	instance->parent_group_tags[0] = source->parent_group_tags[0];
	instance->parent_group_tags[1] = source->parent_group_tags[1];
	instance->tag_index = handle;
	instance->base_address = root;
	instance->unused[0] = 0;
	instance->unused[1] = 0;

	walk.table = table;
	walk.failed = FALSE;
	if (!tag_walk(root, (unsigned long)source->group_tag, FALSE, stock_copy_visit, &walk) || walk.failed)
	{
		error(_error_silent, "stock tags: '%s' cannot be copied", source->name);
		return FALSE;
	}
	if (source->group_tag == BITMAP_TAG_GROUP && !stock_bitmap_finish((struct bitmap_group *)root, handle))
	{
		error(_error_silent, "stock tags: the pixels of '%s' cannot be kept", source->name);
		return FALSE;
	}
	if (source->group_tag == SOUND_TAG_GROUP)
		stock_sound_finish(root, handle);
	if (source->group_tag == MODEL_TAG_GROUP)
	{
		void *geometry;

		if (stock_tags_globals.geometry_count == MAXIMUM_STOCK_GEOMETRIES)
		{
			error(_error_silent, "stock tags: more than %d models", MAXIMUM_STOCK_GEOMETRIES);
			return FALSE;
		}
		geometry = custom_edition_stock_model_make((struct model *)root, source->name);
		if (!geometry)
			return FALSE;
		stock_tags_globals.geometries[stock_tags_globals.geometry_count++] = geometry;
	}

	return TRUE;
}

/* the table's entries made, the game's table replaced with them, the first
time: FALSE if there is no room for them */
static boolean table_make(
	struct stock_table *table,
	long first)
{
	long parent_count;
	struct stock_tag_instance *parent = cache_files_tag_instances(&parent_count);
	long index;

	if (table->instances)
		return TRUE;
	if (!parent || parent_count > first || first > UNSIGNED_SHORT_MAX - MAXIMUM_STOCK_TAGS)
	{
		error(_error_silent, "stock tags: no room for them after the map's %ld tags", parent_count);
		return FALSE;
	}
	table->instances = malloc((first + MAXIMUM_STOCK_TAGS) * sizeof(*table->instances));
	if (!table->instances)
		return FALSE;
	csmemcpy(table->instances, parent, parent_count * sizeof(*parent));
	/* (the room between: entries that are no tag's) */
	for (index = parent_count; index < first; index++)
	{
		struct stock_tag_instance *gap = &table->instances[index];

		csmemset(gap, 0, sizeof(*gap));
		gap->group_tag = NONE;
		gap->parent_group_tags[0] = NONE;
		gap->parent_group_tags[1] = NONE;
		gap->tag_index = DATUM_INDEX_NEW(index, index & UNSIGNED_SHORT_MAX);
		gap->name = (char *)"";
		gap->base_address = tag_empty_data();
	}
	table->first = first;
	table->count = 0;
	table->parent = parent;
	table->parent_count = parent_count;
	cache_files_set_tag_instances(table->instances, first);
	stock_tags_in_map = TRUE;

	return TRUE;
}

static void table_release(
	struct stock_table *table)
{
	if (table->instances)
	{
		cache_files_set_tag_instances(table->parent, table->parent_count);
		free(table->instances);
	}
	csmemset(table, 0, sizeof(*table));

	return;
}

/* whether this map has asked for the tag, and no stock map had it */
static boolean stock_miss_known(
	long group_tag,
	char const *name)
{
	short index;

	for (index = 0; index < stock_tags_globals.miss_count; index++)
	{
		if (stock_tags_globals.miss_groups[index] == group_tag && !_stricmp(stock_tags_globals.miss_names[index], name))
			return TRUE;
	}

	return FALSE;
}

static void stock_miss_add(
	long group_tag,
	char const *name)
{
	if (stock_tags_globals.miss_count < MAXIMUM_STOCK_MISSES)
	{
		short index = stock_tags_globals.miss_count++;

		stock_tags_globals.miss_groups[index] = group_tag;
		csstrncpy(stock_tags_globals.miss_names[index], name, MAXIMUM_STOCK_TAG_NAME_BYTES - 1);
		stock_tags_globals.miss_names[index][MAXIMUM_STOCK_TAG_NAME_BYTES - 1] = 0;
	}

	return;
}

/* the stock map with the tag held (the one held, then those not looked in
yet, each looked in once for a map): its index there, or NONE */
static long stock_map_with_tag(
	long group_tag,
	char const *name)
{
	long stock_index = stock_tags_globals.tag_header ? stock_map_tag_find(group_tag, name) : NONE;
	short map_index;

	for (map_index = 0; stock_index == NONE && map_index < NUMBEROF(stock_map_names); map_index++)
	{
		if ((!stock_tags_globals.tag_header || map_index != stock_tags_globals.map_index) && stock_map_open(map_index))
			stock_index = stock_map_tag_find(group_tag, name);
	}

	return stock_index;
}

/* the tag, and what it uses, added to the table: its handle, or NONE (said) */
static long stock_tag_add(
	struct stock_table *table,
	long group_tag,
	char const *name)
{
	struct stock_walk walk;
	long stock_index;
	long first_added = table->count;
	long handle;
	long index;
	boolean added = TRUE;

	stock_index = stock_map_with_tag(group_tag, name);
	if (stock_index == NONE)
	{
		error(_error_silent, "stock tags: no multiplayer map has the %s '%s'", group_text(group_tag), name);
		stock_miss_add(group_tag, name);
		return NONE;
	}
	stock_tags_globals.pending_count = 0;
	walk.table = table;
	walk.failed = FALSE;
	handle = stock_handle_make(&walk, stock_tags_globals.tag_header->instances[stock_index].tag_index);
	/* (the walks add to the list as they go) */
	for (index = 0; added && !walk.failed && index < stock_tags_globals.pending_count; index++)
		added = stock_tag_copy(table, index);
	if (!added || walk.failed)
	{
		/* (none of it: each tag's handle forgotten, its copies with the rest
		of the arena until the map goes, what it made never named) */
		for (index = 0; index < stock_tags_globals.pending_count; index++)
			stock_tags_globals.handles[stock_tags_globals.pending[index]] = NONE;
		stock_tags_globals.pending_count = 0;
		stock_miss_add(group_tag, name);
		error(_error_silent, "stock tags: the %s '%s' is not added", group_text(group_tag), name);
		return NONE;
	}
	table->count += stock_tags_globals.pending_count;
	cache_files_set_tag_instances(table->instances, table->first + table->count);
	stock_tags_globals.added_count += stock_tags_globals.pending_count;
	error(_error_silent, "stock tags: the %s '%s' from %s, with %ld tags in all, at %ld", group_text(group_tag),
		name, stock_map_names[stock_tags_globals.map_index], stock_tags_globals.pending_count, table->first + first_added);
	stock_tags_globals.pending_count = 0;

	return handle;
}

static boolean stock_tag_wanted(
	long group_tag,
	char const *name)
{
	/* (a stock map's own tags are all it needs: nothing here runs for one) */
	return custom_edition_cache_tags_loaded() && name && name[0] && !stock_miss_known(group_tag, name);
}

/* whether the tag is one of those added */
static boolean stock_tag_is(
	long tag_index)
{
	long absolute_index;
	struct stock_table const *tables[2];
	short index;

	if (!stock_tags_in_map || tag_index == NONE)
		return FALSE;
	absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);
	tables[0] = &stock_tags_globals.before_menus;
	tables[1] = &stock_tags_globals.after_menus;
	for (index = 0; index < NUMBEROF(tables); index++)
	{
		struct stock_table const *table = tables[index];

		if (table->instances && absolute_index >= table->first && absolute_index < table->first + table->count &&
			table->instances[absolute_index].tag_index == tag_index)
		{
			return TRUE;
		}
	}

	return FALSE;
}

/* ---------- public code */

long stock_tags_find_before_menus(
	long group_tag,
	char const *name)
{
	long count;

	if (!stock_tag_wanted(group_tag, name))
		return NONE;
	if (!stock_tags_globals.before_menus.instances)
	{
		cache_files_tag_instances(&count);
		if (!table_make(&stock_tags_globals.before_menus, count))
			return NONE;
	}

	return stock_tag_add(&stock_tags_globals.before_menus, group_tag, name);
}

long stock_tags_find(
	long group_tag,
	char const *name)
{
	if (!stock_tag_wanted(group_tag, name))
		return NONE;
	if (!stock_tags_globals.after_menus.instances)
	{
		long count;
		long first = menu_tags_first_index() + STOCK_TAGS_AFTER_MENUS_DISTANCE;

		cache_files_tag_instances(&count);
		/* (the menus reach past it only if they number thousands: these tags'
		indices are then this machine's alone) */
		if (count > first)
		{
			error(_error_silent, "stock tags: the table's %ld tags pass %ld: their indices may not be another machine's",
				count, first);
			first = count;
		}
		if (!table_make(&stock_tags_globals.after_menus, first))
			return NONE;
	}

	return stock_tag_add(&stock_tags_globals.after_menus, group_tag, name);
}

void stock_tags_finish(
	void)
{
	stock_map_close();

	return;
}

boolean stock_tags_bitmap_contains(
	struct bitmap_data const *bitmap)
{
	return bitmap && stock_tag_is(bitmap->tag_index);
}

boolean stock_tags_bitmaps_read(
	struct bitmap_data *bitmap,
	void *buffer)
{
	if (!stock_tags_bitmap_contains(bitmap))
		return FALSE;
	stock_tags_read(bitmap->tag_index, bitmap->pixels_offset, bitmap->pixels_size, buffer);
	/* (the Xbox's: the pixels are as the Xbox's maps have them) */
	halo_custom_edition_texels_channels(buffer, _custom_edition_channels_xbox);

	return TRUE;
}

boolean stock_tags_read(
	long tag_index,
	long offset,
	long size,
	void *buffer)
{
	if (!stock_tag_is(tag_index))
		return FALSE;
	/* (those offsets are this unit's own: checked again, the game reading
	what it is given) */
	if (offset < 0 || size < 0 || (unsigned long)offset > stock_tags_globals.resource_bytes ||
		(unsigned long)size > stock_tags_globals.resource_bytes - (unsigned long)offset)
	{
		error(_error_silent, "stock tags: a read of %ld bytes at %ld outside what is kept", size, offset);
		csmemset(buffer, 0, size > 0 ? size : 0);
		return TRUE;
	}
	stock_tags_globals.reads++;
	csmemcpy(buffer, stock_tags_globals.resources + offset, size);

	return TRUE;
}

void stock_tags_menus_unloading(
	void)
{
	table_release(&stock_tags_globals.after_menus);

	return;
}

void stock_tags_unloaded(
	void)
{
	short index;

	if (!stock_tags_in_map && !stock_tags_globals.file && !stock_tags_globals.miss_count)
		return;
	if (stock_tags_in_map)
	{
		error(_error_silent, "stock tags: %ld tags and %lu bytes of pixels and samples let go of, read %ld times",
			stock_tags_globals.added_count, stock_tags_globals.resource_bytes, stock_tags_globals.reads);
	}
	stock_map_close();
	table_release(&stock_tags_globals.after_menus);
	table_release(&stock_tags_globals.before_menus);
	for (index = 0; index < stock_tags_globals.geometry_count; index++)
		custom_edition_addon_models_dispose(stock_tags_globals.geometries[index]);
	while (stock_tags_globals.arena)
	{
		struct stock_arena_chunk *next = stock_tags_globals.arena->next;

		free(stock_tags_globals.arena);
		stock_tags_globals.arena = next;
	}
	if (stock_tags_globals.resources)
		free(stock_tags_globals.resources);
	csmemset(&stock_tags_globals, 0, sizeof(stock_tags_globals));
	stock_tags_in_map = FALSE;

	return;
}
