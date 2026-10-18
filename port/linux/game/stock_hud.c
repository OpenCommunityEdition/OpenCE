/*
STOCK_HUD.C

A Custom Edition map draws its own HUD (its weapon and unit
interface tags, and the bitmap sheets those point at). The singleplayer
HUD is the Xbox one in the stock maps, which is also what the high-res
redraw stands for. Those maps are loaded here from bloodgulch, added to
the tag table, and the map's weapons, bipeds, vehicle seats and grenades
are pointed at them. The map's own HUD tags stay in the table and are not
drawn.

The stock tags' pointers are Xbox tag-cache addresses, so the tag bytes
are copied to that cache rather than relocated. Their bitmap pixel
offsets are already file offsets (the cached flag is set, which is the
texture cache treating them as ready to read), so the group's file offset
is not added again. Pixels are served from the decompressed map.

The high-res redraw is registered on these bitmaps by pointer. Looking
the tag up by name would find the map's own bitmap of that name, whose
pixels are not the ones the redraw was made for.
*/

#include "cseries.h"
#include "cseries/cseries_windows.h"
#include "cache/cache_files.h"
#include "cache/physical_memory_map.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "bitmaps/bitmap_group.h"
#include "game/game_globals.h"
#include "interface/interface.h"
#include "items/weapon_definitions.h"
#include "units/unit_definitions.h"
#include "tag_schema.h"
#include "cache_file_formats.h"
#include "stock_hud.h"

#include "../../third_party/zlib/zlib_prefixed.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

/* the stock multiplayer map: it holds the campaign HUD and every weapon
and vehicle HUD the singleplayer maps use */
#define STOCK_HUD_MAP "bloodgulch.map"
/* a stock map's tag header (cache_files.c, CACHE_FILE_TAG_HEADER_SIGNATURE) */
#define STOCK_TAG_HEADER_SIGNATURE 'tags'
#define STOCK_MAP_HEADER_SIGNATURE 'head'
#define STOCK_MAP_VERSION 5
/* larger than bloodgulch, smaller than the tag cache would be asked to hold
twice */
#define STOCK_MAP_MAXIMUM_BYTES (64 * 1024 * 1024)
#define STOCK_COMPRESSED_MAXIMUM_BYTES (32 * 1024 * 1024)
#define STOCK_TAG_COUNT_MAXIMUM 8192
#define STOCK_BLOCK_COUNT_MAXIMUM 4096
#define STOCK_WALK_DEPTH_MAXIMUM 16
/* a tail shorter than this is not a weapon name ("gun", "mp") */
#define STOCK_TAIL_MINIMUM 4

/* the loaded tag table's entry, as cache_files.c lays it out */
struct stock_tag_instance
{
	long group_tag;
	long parent_group_tags[2];
	long tag_index;
	char *name;
	void *base_address;
	unsigned long unused[2];
};

struct stock_tag_header
{
	struct stock_tag_instance *tag_instances;
	long scenario_tag_index;
	unsigned long checksum;
	long tag_count;
	long vertex_buffer_count;
	void *vertex_buffers;
	long index_buffer_count;
	void *index_buffers;
	unsigned long signature;
};

typedef char stock_tag_instance_is_0x20[sizeof(struct stock_tag_instance) == 0x20 ? 1 : -1];
typedef char stock_tag_header_signature_at_0x20[offsetof(struct stock_tag_header, signature) == 0x20 ? 1 : -1];

/* ---------- globals */

static struct
{
	byte *image;
	long image_bytes;
	byte *tag_base;
	long tag_bytes;
	struct stock_tag_instance *stock_instances;
	long stock_count;
	/* 0 none, 1 still to walk, 2 walked */
	byte *marked;
	long *new_index;
	struct stock_tag_instance *instances;
	struct stock_tag_instance *previous;
	long previous_count;
	long first;
	long included;
	boolean active;
} stock_hud;

/* ---------- prototypes */

static void stock_hud_drop(void);
static boolean stock_bytes_fit(void const *pointer, long bytes);
static long stock_absolute(long datum);
static void stock_mark(long absolute);
static char stock_fold(char character);
static boolean stock_folded_equal(char const *left, char const *right);
static boolean stock_folded_has(char const *haystack, char const *needle);
static char const *stock_tail(char const *name);
static long stock_find(unsigned long group_tag, char const *name, boolean tails, boolean contained);
static long stock_match(unsigned long group_tag, char const *first, char const *second);
static void stock_reference_set(struct tag_reference *reference, long tag_index);
static struct tag_schema_definition const *stock_definition(unsigned long group_tag);
static boolean stock_walk(byte *base, struct tag_schema_definition const *definition, boolean rewrite, long depth);
static void *stock_map_load(long *tag_offset, long *tag_size);
static boolean stock_tags_place(long tag_offset, long tag_size);
static boolean stock_table_grow(long extra);
static void stock_bitmap_prepare(struct stock_tag_instance *instance);
static void stock_retarget(void);
static void stock_hires_register(void);

void platform_log(char const *format, ...);
void hud_hires_tags_unloaded(void);
void hud_hires_register_bitmap(char const *tag_name, struct bitmap_data *bitmap, long bitmap_index);
void halo_custom_edition_texels_channels(void const *texels, unsigned char channel_order);

/* ---------- private code */

/* the game's free halts on a null pointer, and only accepts what its malloc
made (debug_memory.c) */
static void stock_hud_drop(
	void)
{
	system_free(stock_hud.instances);
	system_free(stock_hud.image);
	system_free(stock_hud.marked);
	system_free(stock_hud.new_index);
	csmemset(&stock_hud, 0, sizeof(stock_hud));

	return;
}

static boolean stock_bytes_fit(
	void const *pointer,
	long bytes)
{
	unsigned long address = (unsigned long)pointer;
	unsigned long base = (unsigned long)stock_hud.tag_base;

	if (!pointer || bytes < 0 || !stock_hud.tag_base)
		return FALSE;
	if (address < base || address - base > (unsigned long)stock_hud.tag_bytes)
		return FALSE;
	if ((unsigned long)stock_hud.tag_bytes - (address - base) < (unsigned long)bytes)
		return FALSE;

	return TRUE;
}

static long stock_absolute(
	long datum)
{
	long absolute;

	if (datum == NONE)
		return NONE;
	absolute = datum & 0xFFFF;
	if (absolute < 0 || absolute >= stock_hud.stock_count)
		return NONE;
	if (stock_hud.stock_instances[absolute].tag_index != datum)
		return NONE;

	return absolute;
}

static void stock_mark(
	long absolute)
{
	unsigned long group_tag;
	void *base;

	if (absolute < 0 || absolute >= stock_hud.stock_count || stock_hud.marked[absolute])
		return;
	group_tag = (unsigned long)stock_hud.stock_instances[absolute].group_tag;
	base = stock_hud.stock_instances[absolute].base_address;
	if (group_tag != 'unhi' && group_tag != 'wphi' && group_tag != 'grhi' &&
		group_tag != 'hud#' && group_tag != 'bitm')
		return;
	if (!stock_bytes_fit(base, 4))
		return;
	stock_hud.marked[absolute] = 1;

	return;
}

static char stock_fold(
	char character)
{
	if (character >= 'A' && character <= 'Z')
		return (char)(character - 'A' + 'a');
	if (character == '_')
		return ' ';

	return character;
}

static boolean stock_folded_equal(
	char const *left,
	char const *right)
{
	if (!left || !right)
		return FALSE;
	while (*left && *right)
	{
		if (stock_fold(*left) != stock_fold(*right))
			return FALSE;
		left++;
		right++;
	}

	return *left == 0 && *right == 0;
}

/* needle is a whole name inside haystack, spaces and underscores the same */
static boolean stock_folded_has(
	char const *haystack,
	char const *needle)
{
	long haystack_length;
	long needle_length;
	long start;
	long index;

	if (!haystack || !needle)
		return FALSE;
	haystack_length = (long)strlen(haystack);
	needle_length = (long)strlen(needle);
	if (needle_length < STOCK_TAIL_MINIMUM || needle_length > haystack_length)
		return FALSE;
	for (start = 0; start <= haystack_length - needle_length; start++)
	{
		for (index = 0; index < needle_length; index++)
		{
			if (stock_fold(haystack[start + index]) != stock_fold(needle[index]))
				break;
		}
		if (index == needle_length)
			return TRUE;
	}

	return FALSE;
}

static char const *stock_tail(
	char const *name)
{
	char const *tail = name;
	char const *cursor;

	if (!name)
		return NULL;
	for (cursor = name; *cursor; cursor++)
	{
		if (*cursor == '\\' || *cursor == '/')
			tail = cursor + 1;
	}

	return tail;
}

static boolean stock_hud_bitmap_name(
	char const *name)
{
	static char const prefix[] = "ui\\hud\\";
	long index;

	if (!name)
		return FALSE;
	for (index = 0; prefix[index]; index++)
	{
		if (stock_fold(name[index]) != stock_fold(prefix[index]))
			return FALSE;
	}

	return TRUE;
}

static long stock_find(
	unsigned long group_tag,
	char const *name,
	boolean tails,
	boolean contained)
{
	long index;
	long best = NONE;
	long best_length = 0;
	char const *want;

	if (!name || !name[0])
		return NONE;
	want = tails || contained ? stock_tail(name) : name;
	if (!want || !want[0])
		return NONE;
	for (index = 0; index < stock_hud.stock_count; index++)
	{
		char const *tag_name;
		char const *have;
		long length;

		if (stock_hud.new_index[index] == NONE)
			continue;
		if ((unsigned long)stock_hud.stock_instances[index].group_tag != group_tag)
			continue;
		tag_name = stock_hud.stock_instances[index].name;
		if (!stock_bytes_fit(tag_name, 1))
			continue;
		have = tails || contained ? stock_tail(tag_name) : tag_name;
		if (!have || !have[0])
			continue;
		if (contained)
		{
			if (!stock_folded_has(want, have))
				continue;
			length = (long)strlen(have);
			if (length > best_length)
			{
				best = stock_hud.new_index[index];
				best_length = length;
			}
		}
		else if (stock_folded_equal(want, have))
		{
			return stock_hud.new_index[index];
		}
	}

	return best;
}

static long stock_match(
	unsigned long group_tag,
	char const *first,
	char const *second)
{
	long found;

	found = stock_find(group_tag, first, FALSE, FALSE);
	if (found == NONE)
		found = stock_find(group_tag, second, FALSE, FALSE);
	if (found == NONE)
		found = stock_find(group_tag, first, TRUE, FALSE);
	if (found == NONE)
		found = stock_find(group_tag, second, TRUE, FALSE);
	if (found == NONE)
		found = stock_find(group_tag, first, FALSE, TRUE);
	if (found == NONE)
		found = stock_find(group_tag, second, FALSE, TRUE);

	return found;
}

static void stock_reference_set(
	struct tag_reference *reference,
	long tag_index)
{
	long absolute;

	if (!reference || tag_index == NONE)
		return;
	absolute = tag_index & 0xFFFF;
	if (absolute < stock_hud.first || absolute >= stock_hud.first + stock_hud.included)
		return;
	reference->group_tag = (unsigned long)stock_hud.instances[absolute].group_tag;
	reference->name = stock_hud.instances[absolute].name;
	reference->name_length = reference->name ? (long)strlen(reference->name) : 0;
	reference->index = tag_index;

	return;
}

static struct tag_schema_definition const *stock_definition(
	unsigned long group_tag)
{
	struct tag_schema_group const *const *list;

	for (list = tag_schema_group_lists; *list; list++)
	{
		struct tag_schema_group const *group;

		for (group = *list; group->group_tag; group++)
		{
			if (group->group_tag == group_tag)
				return group->definition;
		}
	}

	return NULL;
}

/* one tag's fields. rewrite replaces stock datum indices with the injected
tags' and drops a block's definition pointer (it is an address from the
tool, which the game would follow). the mark walk only records bitmaps and
HUD tags a field names */
static boolean stock_walk(
	byte *base,
	struct tag_schema_definition const *definition,
	boolean rewrite,
	long depth)
{
	struct tag_schema_field const *field;

	if (!definition || !definition->fields || depth > STOCK_WALK_DEPTH_MAXIMUM)
		return FALSE;
	if (!stock_bytes_fit(base, definition->size))
		return FALSE;
	for (field = definition->fields; field->type != _tag_schema_terminator; field++)
	{
		short index;

		for (index = 0; index < field->count; index++)
		{
			byte *address = base + field->offset + index * field->size;

			if (field->type == _tag_schema_struct)
			{
				stock_walk(address, field->definition, rewrite, depth + 1);
			}
			else if (field->type == _tag_schema_block)
			{
				struct tag_block *block = (struct tag_block *)address;
				struct tag_schema_definition const *element = field->definition;
				long element_index;

				if (rewrite)
					block->definition = NULL;
				if (!element || block->count <= 0 || block->count > STOCK_BLOCK_COUNT_MAXIMUM ||
					element->size <= 0 || element->size > 65536 ||
					!stock_bytes_fit(block->address, block->count * element->size))
					continue;
				for (element_index = 0; element_index < block->count; element_index++)
				{
					stock_walk(
						(byte *)block->address + element_index * element->size,
						element,
						rewrite,
						depth + 1);
				}
			}
			else if (field->type == _tag_schema_data || field->type == _tag_schema_file_data)
			{
				if (rewrite)
					((struct tag_data *)address)->definition = NULL;
			}
			else if (field->type == _tag_schema_reset && rewrite)
			{
				if (field->size > 4)
					csmemset(address, 0, field->size);
				else if (field->size == 4)
					*(long *)address = field->target_offset;
				else if (field->size == 2)
					*(short *)address = (short)field->target_offset;
				else if (field->size == 1)
					*address = (byte)field->target_offset;
			}
			else if (field->type == _tag_schema_reference || field->type == _tag_schema_tag_index)
			{
				long *tag_index = field->type == _tag_schema_tag_index ?
					(long *)address : &((struct tag_reference *)address)->index;
				long absolute = stock_absolute(*tag_index);
				unsigned long group_tag = field->type == _tag_schema_reference ?
					((struct tag_reference *)address)->group_tag : 0;

				if (!rewrite)
				{
					if (absolute != NONE)
						stock_mark(absolute);
					continue;
				}
				if (absolute != NONE && stock_hud.new_index[absolute] != NONE)
				{
					*tag_index = stock_hud.new_index[absolute];
				}
				else if (group_tag == 'bitm' || group_tag == 'unhi' || group_tag == 'wphi' ||
					group_tag == 'grhi' || group_tag == 'hud#')
				{
					/* the map's own sheet, in this layout, is the HUD that
					was being drawn */
					*tag_index = NONE;
				}
				else if (field->type == _tag_schema_reference &&
					stock_bytes_fit(((struct tag_reference *)address)->name, 1))
				{
					*tag_index = tag_loaded(group_tag, ((struct tag_reference *)address)->name);
				}
				else
				{
					*tag_index = NONE;
				}
			}
		}
	}

	return TRUE;
}

static void *stock_map_load(
	long *tag_offset,
	long *tag_size)
{
	char xbox_path[96];
	FILE *file;
	byte *compressed = NULL;
	byte *image = NULL;
	long compressed_size;
	z_stream stream;
	int result;
	unsigned long signature;
	long version;
	long file_length;
	long data_offset;
	long data_size;

	snprintf(xbox_path, sizeof(xbox_path), "%s%s", cache_files_map_directory(), STOCK_HUD_MAP);
	/* fopen translates an Xbox path itself. a host path is translated
	again, and the stock map is then not found */
	file = fopen(xbox_path, "rb");
	if (!file)
	{
		platform_log("stock hud: %s is not in the maps folder", STOCK_HUD_MAP);
		return NULL;
	}
	if (fread(&signature, 4, 1, file) != 1 ||
		fread(&version, 4, 1, file) != 1 ||
		fread(&file_length, 4, 1, file) != 1 ||
		fseek(file, 4, SEEK_CUR) != 0 ||
		fread(&data_offset, 4, 1, file) != 1 ||
		fread(&data_size, 4, 1, file) != 1)
	{
		fclose(file);
		platform_log("stock hud: %s could not be read", STOCK_HUD_MAP);
		return NULL;
	}
	if (signature != STOCK_MAP_HEADER_SIGNATURE || version != STOCK_MAP_VERSION ||
		file_length < 0x800 + data_size || data_offset < 0x800 || data_size <= 0 ||
		data_size > TAG_CACHE_SIZE || file_length > STOCK_MAP_MAXIMUM_BYTES ||
		data_offset > file_length - data_size)
	{
		fclose(file);
		platform_log("stock hud: %s is not the stock map", STOCK_HUD_MAP);
		return NULL;
	}
	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return NULL;
	}
	compressed_size = ftell(file) - 0x800;
	if (compressed_size <= 0 || compressed_size > STOCK_COMPRESSED_MAXIMUM_BYTES ||
		fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		platform_log("stock hud: %s could not be read", STOCK_HUD_MAP);
		return NULL;
	}
	compressed = (byte *)system_malloc(compressed_size);
	image = (byte *)system_malloc(file_length);
	if (!compressed || !image ||
		fread(image, 1, 0x800, file) != 0x800 ||
		fread(compressed, 1, (size_t)compressed_size, file) != (size_t)compressed_size)
	{
		system_free(compressed);
		system_free(image);
		fclose(file);
		platform_log("stock hud: %s could not be read", STOCK_HUD_MAP);
		return NULL;
	}
	fclose(file);
	csmemset(&stream, 0, sizeof(stream));
	stream.next_in = compressed;
	stream.avail_in = (uInt)compressed_size;
	stream.next_out = image + 0x800;
	stream.avail_out = (uInt)(file_length - 0x800);
	if (inflateInit(&stream) != Z_OK)
	{
		system_free(compressed);
		system_free(image);
		platform_log("stock hud: %s could not be decompressed", STOCK_HUD_MAP);
		return NULL;
	}
	result = inflate(&stream, Z_FINISH);
	{
		unsigned long produced = stream.total_out;

		inflateEnd(&stream);
		system_free(compressed);
		compressed = NULL;
		if (result != Z_STREAM_END || produced != (unsigned long)(file_length - 0x800))
		{
			system_free(image);
			platform_log("stock hud: %s could not be decompressed", STOCK_HUD_MAP);
			return NULL;
		}
	}
	*tag_offset = data_offset;
	*tag_size = data_size;
	stock_hud.image = image;
	stock_hud.image_bytes = file_length;

	return image;
}

static boolean stock_tags_place(
	long tag_offset,
	long tag_size)
{
	struct stock_tag_header *header;

	stock_hud.tag_base = (byte *)physical_memory_get_tag_cache_base_address();
	if (!stock_hud.tag_base || tag_size <= 0 || tag_size > TAG_CACHE_SIZE)
		return FALSE;
	/* the previous map (the menus' ui.map) still occupies this cache, and
	nothing in a custom map reads it: the menus copied out what they keep */
	cache_files_enable_writes();
	csmemset(stock_hud.tag_base, 0xCD, TAG_CACHE_SIZE);
	csmemcpy(stock_hud.tag_base, stock_hud.image + tag_offset, tag_size);
	stock_hud.tag_bytes = tag_size;
	header = (struct stock_tag_header *)stock_hud.tag_base;
	if (header->signature != STOCK_TAG_HEADER_SIGNATURE ||
		header->tag_count <= 0 || header->tag_count > STOCK_TAG_COUNT_MAXIMUM ||
		!stock_bytes_fit(header->tag_instances, header->tag_count * (long)sizeof(struct stock_tag_instance)))
		return FALSE;
	stock_hud.stock_instances = header->tag_instances;
	stock_hud.stock_count = header->tag_count;

	return TRUE;
}

static boolean stock_table_grow(
	long extra)
{
	long existing = 0;
	struct stock_tag_instance *current;
	struct stock_tag_instance *grown;
	long index;
	unsigned long salt = 0;

	current = (struct stock_tag_instance *)cache_files_tag_instances(&existing);
	if (!current || extra <= 0 || existing < 0 || existing > 0xFFFE - extra)
		return FALSE;
	grown = (struct stock_tag_instance *)system_malloc((existing + extra) * (long)sizeof(*grown));
	if (!grown)
		return FALSE;
	csmemcpy(grown, current, (size_t)existing * sizeof(*grown));
	csmemset(grown + existing, 0, (size_t)extra * sizeof(*grown));
	for (index = 0; index < existing; index++)
	{
		unsigned long instance_salt = (unsigned long)current[index].tag_index >> 16;

		if (instance_salt > salt)
			salt = instance_salt;
	}
	if (salt > 0xFFFE - (unsigned long)extra)
	{
		system_free(grown);
		return FALSE;
	}
	stock_hud.previous = current;
	stock_hud.previous_count = existing;
	stock_hud.instances = grown;
	stock_hud.first = existing;
	cache_files_set_tag_instances(grown, existing + extra);

	return TRUE;
}

static void stock_bitmap_prepare(
	struct stock_tag_instance *instance)
{
	struct bitmap_group *group;
	long index;

	if (!instance || instance->group_tag != 'bitm')
		return;
	group = (struct bitmap_group *)instance->base_address;
	if (!group || group->bitmaps.count <= 0 || group->bitmaps.count > STOCK_BLOCK_COUNT_MAXIMUM)
		return;
	for (index = 0; index < group->bitmaps.count; index++)
	{
		struct bitmap_data *bitmap = (struct bitmap_data *)(
			(byte *)group->bitmaps.address + index * (long)sizeof(*bitmap));

		if (!stock_bytes_fit(bitmap, (long)sizeof(*bitmap)))
			return;
		/* the texture cache reads this tag, and treats a stale base as
		pixels it already holds */
		bitmap->tag_index = instance->tag_index;
		bitmap->cache_block_index = NONE;
		bitmap->base_address = NULL;
		bitmap->hardware_format = NULL;
	}

	return;
}

static void stock_retarget_hud_block(
	struct tag_block *block,
	char const *owner_name,
	boolean biped)
{
	long index;

	if (!block || block->count <= 0)
		return;
	for (index = 0; index < block->count; index++)
	{
		struct unit_hud_reference *hud = TAG_BLOCK_GET_ELEMENT(block, index, struct unit_hud_reference);
		long matched = NONE;

		if (!hud)
			continue;
		if (biped)
		{
			/* one local player draws HUD 0, which on the stock maps is the
			singleplayer HUD. a custom map's own first HUD is the one
			that was showing */
			matched = stock_find('unhi', index == 0 ? "ui\\hud\\cyborg" : "ui\\hud\\cyborg_mp", FALSE, FALSE);
			if (matched == NONE)
				matched = stock_find('unhi', "ui\\hud\\cyborg", FALSE, FALSE);
		}
		else
		{
			matched = stock_match('unhi', hud->hud.name, owner_name);
			if (matched == NONE)
				matched = stock_find('unhi', "ui\\hud\\cyborg", FALSE, FALSE);
		}
		stock_reference_set(&hud->hud, matched);
	}

	return;
}

static void stock_retarget(
	void)
{
	struct tag_iterator iterator;
	long tag_index;
	long weapons = 0;
	long units = 0;
	long grenades = 0;
	struct stock_tag_instance *stock_globals = NULL;
	long index;

	tag_iterator_new(&iterator, 'weap');
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct weapon_definition *weapon;
		long matched;
		long absolute = tag_index & 0xFFFF;

		if (absolute >= stock_hud.first)
			continue;
		weapon = (struct weapon_definition *)tag_get('weap', tag_index);
		if (!weapon)
			continue;
		/* the stock weapon HUD is named the weapon, so the weapon decides,
		not the map's interface tag */
		matched = stock_match(
			'wphi',
			tag_get_name(tag_index),
			weapon->weapon.interface_definition.hud_interface.name);
		if (matched == NONE)
			matched = stock_find('wphi', "ui\\hud\\empty", FALSE, FALSE);
		if (matched != NONE)
		{
			stock_reference_set(&weapon->weapon.interface_definition.hud_interface, matched);
			weapons++;
		}
	}

	tag_iterator_new(&iterator, 'unit');
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct unit_definition *unit;
		struct stock_tag_instance *instance;
		long absolute = tag_index & 0xFFFF;
		long seat_index;
		boolean biped;

		if (absolute < 0 || absolute >= stock_hud.first)
			continue;
		instance = &stock_hud.instances[absolute];
		biped = instance->group_tag == 'bipd' || instance->parent_group_tags[0] == 'bipd' ||
			instance->parent_group_tags[1] == 'bipd';
		unit = (struct unit_definition *)tag_get('unit', tag_index);
		if (!unit)
			continue;
		stock_retarget_hud_block(&unit->unit.huds, tag_get_name(tag_index), biped);
		units++;
		for (seat_index = 0; seat_index < unit->unit.seats.count; seat_index++)
		{
			struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&unit->unit.seats, seat_index, struct unit_seat);

			if (seat)
				stock_retarget_hud_block(&seat->seat_huds, tag_get_name(tag_index), FALSE);
		}
	}

	for (index = 0; index < stock_hud.stock_count; index++)
	{
		if (stock_hud.stock_instances[index].group_tag == 'matg')
		{
			stock_globals = &stock_hud.stock_instances[index];
			break;
		}
	}
	tag_iterator_new(&iterator, 'matg');
	tag_index = tag_iterator_next(&iterator);
	if (tag_index != NONE && (tag_index & 0xFFFF) < stock_hud.first && stock_globals &&
		stock_bytes_fit(stock_globals->base_address, (long)sizeof(struct game_globals)))
	{
		struct game_globals *live = (struct game_globals *)tag_get('matg', tag_index);
		struct game_globals *source = (struct game_globals *)stock_globals->base_address;
		static short const slots[] = {
			_interface_hud_digits,
			_interface_bitmap_motion_sweep,
			_interface_bitmap_motion_sweep_mask,
			_interface_bitmap_multiplayer_hud,
			_interface_bitmap_motion_blip
		};
		long slot_index;

		if (live && live->interface_tag_references.count > 0 &&
			source->interface_tag_references.count > 0 &&
			stock_bytes_fit(source->interface_tag_references.address,
				(long)sizeof(struct game_globals_interface_tag_references)))
		{
			struct game_globals_interface_tag_references *live_refs = TAG_BLOCK_GET_ELEMENT(
				&live->interface_tag_references, 0, struct game_globals_interface_tag_references);
			struct game_globals_interface_tag_references *source_refs =
				(struct game_globals_interface_tag_references *)source->interface_tag_references.address;

			if (live_refs && source_refs)
			{
				for (slot_index = 0; slot_index < (long)(sizeof(slots) / sizeof(slots[0])); slot_index++)
				{
					long absolute = stock_absolute(source_refs->interface_tag_references[slots[slot_index]].index);
					long injected = absolute == NONE ? NONE : stock_hud.new_index[absolute];

					if (injected != NONE)
						stock_reference_set(&live_refs->interface_tag_references[slots[slot_index]], injected);
				}
			}
		}
		if (live && live->grenades.count > 0)
		{
			for (index = 0; index < live->grenades.count; index++)
			{
				struct game_globals_grenade *grenade = TAG_BLOCK_GET_ELEMENT(
					&live->grenades, index, struct game_globals_grenade);
				long matched;

				if (!grenade)
					continue;
				matched = stock_match('grhi', grenade->hud_interface.name, grenade->item.name);
				if (matched == NONE &&
					(stock_folded_has(grenade->hud_interface.name, "plasma") ||
						stock_folded_has(grenade->item.name, "plasma")))
					matched = stock_find('grhi', "ui\\hud\\plasma_flare", FALSE, FALSE);
				if (matched == NONE)
					matched = stock_find('grhi', index == 0 ? "ui\\hud\\default" : "ui\\hud\\plasma_flare", FALSE, FALSE);
				if (matched == NONE)
					matched = stock_find('grhi', "ui\\hud\\default", FALSE, FALSE);
				if (matched != NONE)
				{
					stock_reference_set(&grenade->hud_interface, matched);
					grenades++;
				}
			}
		}
	}
	platform_log("stock hud: %ld tags from %s, %ld weapons, %ld units, %ld grenades",
		stock_hud.included, STOCK_HUD_MAP, weapons, units, grenades);

	return;
}

static void stock_hires_register(
	void)
{
	long index;

	/* the list would otherwise name the previous map's bitmaps, or, if the
	high-res pass had run, this map's own copies of the same tag names */
	hud_hires_tags_unloaded();
	for (index = 0; index < stock_hud.included; index++)
	{
		struct stock_tag_instance *instance = &stock_hud.instances[stock_hud.first + index];
		struct bitmap_group *group;
		long bitmap_index;

		if (instance->group_tag != 'bitm')
			continue;
		group = (struct bitmap_group *)instance->base_address;
		if (!group || group->bitmaps.count <= 0 || group->bitmaps.count > STOCK_BLOCK_COUNT_MAXIMUM)
			continue;
		for (bitmap_index = 0; bitmap_index < group->bitmaps.count; bitmap_index++)
		{
			struct bitmap_data *bitmap = (struct bitmap_data *)(
				(byte *)group->bitmaps.address + bitmap_index * (long)sizeof(*bitmap));

			if (!stock_bytes_fit(bitmap, (long)sizeof(*bitmap)))
				break;
			hud_hires_register_bitmap(instance->name, bitmap, bitmap_index);
		}
	}

	return;
}

static boolean stock_collect(
	void)
{
	long index;
	long included = 0;
	long pending;
	struct stock_tag_instance *globals = NULL;

	stock_hud.marked = (byte *)system_malloc(stock_hud.stock_count);
	stock_hud.new_index = (long *)system_malloc(stock_hud.stock_count * (long)sizeof(long));
	if (!stock_hud.marked || !stock_hud.new_index)
		return FALSE;
	csmemset(stock_hud.marked, 0, (size_t)stock_hud.stock_count);
	for (index = 0; index < stock_hud.stock_count; index++)
		stock_hud.new_index[index] = NONE;
	for (index = 0; index < stock_hud.stock_count; index++)
	{
		struct stock_tag_instance *instance = &stock_hud.stock_instances[index];
		unsigned long group_tag = (unsigned long)instance->group_tag;

		if (group_tag == 'unhi' || group_tag == 'wphi' || group_tag == 'grhi' || group_tag == 'hud#')
			stock_mark(index);
		else if (group_tag == 'bitm' && stock_bytes_fit(instance->name, 1) && stock_hud_bitmap_name(instance->name))
			stock_mark(index);
		else if (group_tag == 'matg')
			globals = instance;
	}
	if (globals && stock_bytes_fit(globals->base_address, (long)sizeof(struct game_globals)))
	{
		struct game_globals *source = (struct game_globals *)globals->base_address;
		static short const slots[] = {
			_interface_hud_digits,
			_interface_bitmap_motion_sweep,
			_interface_bitmap_motion_sweep_mask,
			_interface_bitmap_multiplayer_hud,
			_interface_bitmap_motion_blip
		};

		if (source->interface_tag_references.count > 0 &&
			stock_bytes_fit(source->interface_tag_references.address,
				(long)sizeof(struct game_globals_interface_tag_references)))
		{
			struct game_globals_interface_tag_references *refs =
				(struct game_globals_interface_tag_references *)source->interface_tag_references.address;
			long slot_index;

			for (slot_index = 0; slot_index < (long)(sizeof(slots) / sizeof(slots[0])); slot_index++)
			{
				long absolute = stock_absolute(refs->interface_tag_references[slots[slot_index]].index);

				if (absolute != NONE)
					stock_mark(absolute);
			}
		}
	}
	do
	{
		pending = FALSE;
		for (index = 0; index < stock_hud.stock_count; index++)
		{
			struct tag_schema_definition const *definition;

			if (stock_hud.marked[index] != 1)
				continue;
			stock_hud.marked[index] = 2;
			pending = TRUE;
			definition = stock_definition((unsigned long)stock_hud.stock_instances[index].group_tag);
			if (!definition)
			{
				platform_log("stock hud: no description of a '%c%c%c%c' tag",
					(char)(stock_hud.stock_instances[index].group_tag >> 24),
					(char)(stock_hud.stock_instances[index].group_tag >> 16),
					(char)(stock_hud.stock_instances[index].group_tag >> 8),
					(char)stock_hud.stock_instances[index].group_tag);
				continue;
			}
			stock_walk(stock_hud.stock_instances[index].base_address, definition, FALSE, 0);
		}
	}
	while (pending);
	for (index = 0; index < stock_hud.stock_count; index++)
	{
		if (stock_hud.marked[index])
			included++;
	}
	if (!included || !stock_table_grow(included))
		return FALSE;
	included = 0;
	{
		unsigned long salt = 0;

		for (index = 0; index < stock_hud.previous_count; index++)
		{
			unsigned long instance_salt = (unsigned long)stock_hud.previous[index].tag_index >> 16;

			if (instance_salt > salt)
				salt = instance_salt;
		}
		salt++;
		for (index = 0; index < stock_hud.stock_count; index++)
		{
			struct stock_tag_instance *source;
			struct stock_tag_instance *dest;

			if (!stock_hud.marked[index])
				continue;
			source = &stock_hud.stock_instances[index];
			dest = &stock_hud.instances[stock_hud.first + included];
			dest->group_tag = source->group_tag;
			dest->parent_group_tags[0] = source->parent_group_tags[0];
			dest->parent_group_tags[1] = source->parent_group_tags[1];
			dest->tag_index = (long)((salt << 16) | (unsigned long)(stock_hud.first + included));
			dest->name = source->name;
			dest->base_address = source->base_address;
			stock_hud.new_index[index] = dest->tag_index;
			included++;
		}
	}
	stock_hud.included = included;
	for (index = 0; index < stock_hud.stock_count; index++)
	{
		struct tag_schema_definition const *definition;

		if (!stock_hud.marked[index])
			continue;
		definition = stock_definition((unsigned long)stock_hud.stock_instances[index].group_tag);
		if (definition)
			stock_walk(stock_hud.stock_instances[index].base_address, definition, TRUE, 0);
		stock_bitmap_prepare(&stock_hud.instances[stock_hud.new_index[index] & 0xFFFF]);
	}
	stock_hud.active = TRUE;
	stock_retarget();
	stock_hires_register();
	system_free(stock_hud.marked);
	system_free(stock_hud.new_index);
	stock_hud.marked = NULL;
	stock_hud.new_index = NULL;

	return TRUE;
}

/* ---------- public code */

void stock_hud_tags_loaded(
	void)
{
	long tag_offset = 0;
	long tag_size = 0;

	/* a map that loaded its tags over the previous one's has already
	replaced the table. the previous stock tags are only bytes now */
	stock_hud_drop();
	if (!stock_map_load(&tag_offset, &tag_size) || !stock_tags_place(tag_offset, tag_size))
	{
		stock_hud_drop();
		return;
	}
	if (!stock_collect())
	{
		platform_log("stock hud: the singleplayer HUD could not be added");
		if (stock_hud.previous)
			stock_hud_tags_unloaded();
		else
			stock_hud_drop();
	}

	return;
}

void stock_hud_tags_unloaded(
	void)
{
	struct stock_tag_instance *grown = stock_hud.instances;
	struct stock_tag_instance *previous = stock_hud.previous;
	long previous_count = stock_hud.previous_count;
	byte *image = stock_hud.image;
	byte *marked = stock_hud.marked;
	long *new_index = stock_hud.new_index;

	if (!grown && !image && !previous && !marked)
		return;
	if (previous)
		cache_files_set_tag_instances(previous, previous_count);
	system_free(grown);
	system_free(image);
	system_free(marked);
	system_free(new_index);
	csmemset(&stock_hud, 0, sizeof(stock_hud));

	return;
}

boolean stock_hud_read(
	long tag_index,
	long offset,
	long size,
	void *buffer)
{
	long absolute;
	struct stock_tag_instance *instance;
	struct bitmap_group *group;
	long index;

	if (!stock_hud.active || !stock_hud.instances)
		return FALSE;
	absolute = tag_index & 0xFFFF;
	if (absolute < stock_hud.first || absolute >= stock_hud.first + stock_hud.included)
		return FALSE;
	instance = &stock_hud.instances[absolute];
	if (instance->tag_index != tag_index)
		return FALSE;
	if (!buffer)
		return TRUE;
	if (offset >= 0 && size >= 0 && stock_hud.image &&
		offset <= stock_hud.image_bytes && size <= stock_hud.image_bytes - offset)
		csmemcpy(buffer, stock_hud.image + offset, (size_t)size);
	else if (size > 0)
		csmemset(buffer, 0, (size_t)size);
	if (instance->group_tag != 'bitm' || size <= 0)
		return TRUE;
	group = (struct bitmap_group *)instance->base_address;
	if (!group || group->bitmaps.count <= 0 || group->bitmaps.count > STOCK_BLOCK_COUNT_MAXIMUM)
		return TRUE;
	for (index = 0; index < group->bitmaps.count; index++)
	{
		struct bitmap_data *bitmap = (struct bitmap_data *)(
			(byte *)group->bitmaps.address + index * (long)sizeof(*bitmap));

		if (!stock_bytes_fit(bitmap, (long)sizeof(*bitmap)))
			break;
		if (bitmap->pixels_offset == offset)
		{
			/* the stock bytes are already swizzled. rebuilding the hardware
			format swizzles them again (it does not look at that flag), which
			scrambles the icons and makes the high-res redraw refuse them.
			a reused cache address may still be listed as a custom map's
			meter, so the order is set back to the Xbox's */
			halo_custom_edition_texels_channels(buffer, _custom_edition_channels_xbox);
			break;
		}
	}

	return TRUE;
}
