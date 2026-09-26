/*
CACHE_FILE_FORMATS.C

Halo Custom Edition caches, OpenSauce extensions and Custom Edition resource
maps (cache_file_formats.h; docs/custom_edition_caches.md).

Sources for the layouts, cited per structure below:
- OpenSauce (GPL-3.0, Kornner Studios; the snapshot examined is the
  OpenSauce-master archive of the upstream Mercurial repository, whose newest
  version file names OpenSauce 4.0.0): the cache header, the OpenSauce header,
  tag index, tag instance, resource map ("data file") header and item, the
  structure BSP header and reference, and the bitmap, sound and HUD message
  tag layouts, all with static size assertions.
- BlamLib, in the same archive: the font and unicode string list layouts.
- Reclaimer (GPL-3.0, Gravemind2401; the Reclaimer-master archive): how
  bitmap tags and pixel data are found in bitmaps.map.
No code from either project is used: they are documentation here, and this
repository is CC0. Facts that neither source states were established by
reading the stock Custom Edition maps; each is marked "observed" below and
listed with its evidence in docs/custom_edition_caches.md.
*/

/* ---------- headers */

#include "cache_file_formats.h"

#include <stdlib.h>
#include <string.h>

/* ---------- constants */

/* four-character codes, as the files store them (little-endian) */
#define CACHE_HEADER_SIGNATURE 'head'
#define CACHE_FOOTER_SIGNATURE 'foot'
#define TAG_INDEX_SIGNATURE 'tags'
#define STRUCTURE_BSP_SIGNATURE 'sbsp'
#define OPENSAUCE_HEADER_SIGNATURE 'yelo'

#define BITMAP_GROUP_TAG 'bitm'
#define SOUND_GROUP_TAG 'snd!'
#define FONT_GROUP_TAG 'font'
#define UNICODE_STRING_LIST_GROUP_TAG 'ustr'
#define HUD_MESSAGE_TEXT_GROUP_TAG 'hmt '
#define SCENARIO_GROUP_TAG 'scnr'
#define STRUCTURE_BSP_GROUP_TAG 'sbsp'
#define PROJECT_YELLOW_GROUP_TAG 'yelo'
#define PROJECT_YELLOW_GLOBALS_GROUP_TAG 'gelo'

#define NO_TAG_INDEX (-1)
#define ABSOLUTE_INDEX_MASK 0xFFFFUL

/* the cache header (OpenSauce cache_files_structures.hpp, s_cache_header) */
#define CACHE_HEADER_VERSION_OFFSET 0x04
#define CACHE_HEADER_FILE_LENGTH_OFFSET 0x08
#define CACHE_HEADER_COMPRESSED_LENGTH_OFFSET 0x0C
#define CACHE_HEADER_TAG_DATA_OFFSET_OFFSET 0x10
#define CACHE_HEADER_TAG_DATA_SIZE_OFFSET 0x14
#define CACHE_HEADER_NAME_OFFSET 0x20
#define CACHE_HEADER_BUILD_OFFSET 0x40
#define CACHE_HEADER_SCENARIO_TYPE_OFFSET 0x60
#define CACHE_HEADER_CHECKSUM_OFFSET 0x64
#define CACHE_HEADER_OPENSAUCE_OFFSET 0x70
#define CACHE_HEADER_FOOTER_OFFSET 0x7FC

/* the OpenSauce header (OpenSauce cache_files_structures_yelo.hpp,
s_cache_header_yelo). Its build_info holds a time_t: the source does not
define _USE_32BIT_TIME_T, so it is 64 bits aligned to 8, and in every
OpenSauce cache examined the build string does start 8 bytes after the
timestamp. */
#define OPENSAUCE_VERSION_OFFSET 0x04
#define OPENSAUCE_FLAGS_OFFSET 0x06
#define OPENSAUCE_PROJECT_YELLOW_VERSION_OFFSET 0x08
#define OPENSAUCE_PROJECT_YELLOW_GLOBALS_VERSION_OFFSET 0x09
#define OPENSAUCE_MEMORY_UPGRADE_AMOUNT_OFFSET 0x0C
#define OPENSAUCE_DEFINITIONS_SIZE_OFFSET 0x10
#define OPENSAUCE_DEFINITIONS_DECOMPRESSED_SIZE_OFFSET 0x14
#define OPENSAUCE_DEFINITIONS_OFFSET_OFFSET 0x18
#define OPENSAUCE_DEFINITIONS_BUILD_OFFSET 0x20
#define OPENSAUCE_MOD_NAME_OFFSET 0x40
#define OPENSAUCE_BUILD_STAGE_OFFSET 0x62
#define OPENSAUCE_BUILD_REVISION_OFFSET 0x64
#define OPENSAUCE_BUILD_TIMESTAMP_OFFSET 0x68
#define OPENSAUCE_BUILD_STRING_OFFSET 0x70
#define OPENSAUCE_TOOLS_VERSION_OFFSET 0x90
#define OPENSAUCE_MINIMUM_VERSION_OFFSET 0xA4
#define OPENSAUCE_RESOURCE_OFFSETS_OFFSET 0xB8
#define OPENSAUCE_HEADER_BYTES 0xC8
/* s_cache_header_yelo::k_version and k_version_minimum_build */
#define OPENSAUCE_HEADER_VERSION 1
#define OPENSAUCE_HEADER_VERSION_WITH_MINIMUM_BUILD 2
/* project_yellow::k_version and project_yellow_globals::k_version */
#define OPENSAUCE_PROJECT_YELLOW_VERSION 2
#define OPENSAUCE_PROJECT_YELLOW_GLOBALS_VERSION 2
/* K_MEMORY_UPGRADE_INCREASE_AMOUNT */
#define OPENSAUCE_MEMORY_UPGRADE_AMOUNT 1.5f

/* the tag index at the start of the tag data (OpenSauce
cache_files_structures.hpp, s_cache_tag_header and s_cache_tag_instance) */
#define TAG_INDEX_BYTES 0x28
#define TAG_INDEX_INSTANCES_OFFSET 0x00
#define TAG_INDEX_SCENARIO_OFFSET 0x04
#define TAG_INDEX_COUNT_OFFSET 0x0C
#define TAG_INDEX_VERTEX_DATA_OFFSET_OFFSET 0x14
#define TAG_INDEX_INDEX_DATA_OFFSET_OFFSET 0x1C
#define TAG_INDEX_MODEL_DATA_SIZE_OFFSET 0x20
#define TAG_INDEX_SIGNATURE_OFFSET 0x24
#define TAG_INSTANCE_BYTES 0x20
#define TAG_INSTANCE_GROUP_OFFSET 0x00
#define TAG_INSTANCE_HANDLE_OFFSET 0x0C
#define TAG_INSTANCE_NAME_OFFSET 0x10
#define TAG_INSTANCE_ADDRESS_OFFSET 0x14
#define TAG_INSTANCE_IN_RESOURCE_MAP_OFFSET 0x18
/* the longest tag path a name may hold, terminator excluded */
#define TAG_NAME_MAXIMUM_LENGTH 255

/* tag blocks and tag data, as in this build (tag_files/tag_groups.h) */
#define TAG_BLOCK_BYTES 12
#define TAG_BLOCK_COUNT_OFFSET 0
#define TAG_BLOCK_ADDRESS_OFFSET 4
#define TAG_BLOCK_DEFINITION_OFFSET 8
#define TAG_DATA_BYTES 20
#define TAG_DATA_SIZE_OFFSET 0
#define TAG_DATA_FLAGS_OFFSET 4
#define TAG_DATA_FILE_OFFSET_OFFSET 8
#define TAG_DATA_ADDRESS_OFFSET 12
#define TAG_DATA_DEFINITION_OFFSET 16

/* the scenario's structure BSP block (OpenSauce scenario_definitions.hpp;
the offset holds in every map examined) */
#define SCENARIO_STRUCTURE_BSPS_OFFSET 0x5A4
#define SCENARIO_BYTES 0x5B0
#define STRUCTURE_BSP_REFERENCE_BYTES 0x20
#define STRUCTURE_BSP_REFERENCE_FILE_OFFSET_OFFSET 0x00
#define STRUCTURE_BSP_REFERENCE_SIZE_OFFSET 0x04
#define STRUCTURE_BSP_REFERENCE_ADDRESS_OFFSET 0x08
#define STRUCTURE_BSP_REFERENCE_TAG_INDEX_OFFSET 0x1C
/* OpenSauce's "max count: 32" for scenario_structure_bsp_reference */
#define MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO 32
/* structure_bsp_header (OpenSauce structure_bsp_definitions.hpp): the bsp
pointer, two vertex buffer arrays (Xbox only) and the signature */
#define STRUCTURE_BSP_HEADER_BYTES 0x18
#define STRUCTURE_BSP_HEADER_BSP_OFFSET 0x00
#define STRUCTURE_BSP_HEADER_VERTEX_BUFFERS_OFFSET 0x04
#define STRUCTURE_BSP_HEADER_LIGHTMAP_VERTEX_BUFFERS_OFFSET 0x0C
#define STRUCTURE_BSP_HEADER_SIGNATURE_OFFSET 0x14

/* resource maps (OpenSauce data_file_structures.hpp, s_data_file_header and
s_data_file_item) */
#define RESOURCE_MAP_HEADER_BYTES 0x10
#define RESOURCE_MAP_NAMES_OFFSET_OFFSET 0x04
#define RESOURCE_MAP_INDEX_OFFSET_OFFSET 0x08
#define RESOURCE_MAP_COUNT_OFFSET 0x0C
#define RESOURCE_MAP_ITEM_BYTES 0x0C
#define RESOURCE_MAP_ITEM_NAME_OFFSET 0x00
#define RESOURCE_MAP_ITEM_SIZE_OFFSET 0x04
#define RESOURCE_MAP_ITEM_DATA_OFFSET 0x08

/* bitmap group (OpenSauce bitmap_group.hpp: s_bitmap_group 0x6C,
s_bitmap_group_sequence 0x40, s_bitmap_group_sprite 0x20, s_bitmap_data 0x30) */
#define BITMAP_GROUP_BYTES 0x6C
#define BITMAP_DATA_FLAGS_OFFSET 0x0E
#define BITMAP_DATA_PIXELS_OFFSET_OFFSET 0x18
#define BITMAP_DATA_PIXELS_SIZE_OFFSET 0x1C
/* s_bitmap_data::_flags::in_data_file: the pixels are in bitmaps.map */
#define BITMAP_DATA_IN_RESOURCE_MAP_BIT 8

/* sound (OpenSauce sound_definitions.hpp: sound_definition 0xA4,
s_sound_pitch_range 0x48, s_sound_permutation 0x7C) */
#define SOUND_DEFINITION_BYTES 0xA4
#define SOUND_PITCH_RANGES_OFFSET 0x98
#define SOUND_PITCH_RANGE_BYTES 0x48
#define SOUND_PERMUTATION_BYTES 0x7C
#define SOUND_PERMUTATION_SAMPLES_OFFSET 0x40
/* s_sound_permutation::_samples_in_data_file_bit */
#define SOUND_SAMPLES_IN_RESOURCE_MAP_BIT 0

/* font (BlamLib Misc.cs, font_group: 156 bytes, 36 bytes of padding after
the heights; the block offsets below hold for every font in loc.map) */
#define FONT_BYTES 0x9C
#define FONT_STYLE_REFERENCES_OFFSET 0x3C
#define FONT_STYLE_REFERENCE_COUNT 4
#define TAG_REFERENCE_BYTES 16
#define TAG_REFERENCE_INDEX_OFFSET 12

#define UNICODE_STRING_LIST_BYTES 0x0C
#define HUD_MESSAGE_TEXT_BYTES 0x80

/* CRC-32 (reflected polynomial 0x04C11DB7), updated from 0xFFFFFFFF with no
final inversion, as OpenSauce's Memory::CRC and CalculateChecksum do */
#define CRC32_POLYNOMIAL 0xEDB88320UL
#define CRC32_INITIAL 0xFFFFFFFFUL
#define CHECKSUM_READ_CHUNK_BYTES 0x10000

/* ---------- structures */

struct element_layout;

/* a tag block field and the layout of its elements */
struct block_layout
{
	uint32_t offset;
	uint32_t element_bytes;
	struct element_layout const *element;
};

struct load_state;

typedef enum cache_file_status (*element_check_proc)(
	struct load_state *state,
	uint32_t element_offset);

/* the pointers an element holds: tag blocks and tag data fields */
struct element_layout
{
	uint32_t bytes;
	struct block_layout const *blocks;
	int block_count;
	uint32_t const *data_offsets;
	int data_count;
	element_check_proc check;
};

struct load_state
{
	struct cache_file_source *map;
	struct resource_map *const *resource_maps;
	struct custom_edition_load_report *report;
	uint8_t *tag_cache;
	/* the bytes of tag_cache in use, and the most that may be used: the
	lowest structure BSP address, relative to the tag cache */
	uint32_t used_bytes;
	uint32_t usable_bytes;
	uint32_t file_length;
	int32_t tag_index;
};

/* ---------- prototypes */

static enum cache_file_status bitmap_data_check(
	struct load_state *state,
	uint32_t element_offset);
static enum cache_file_status sound_permutation_check(
	struct load_state *state,
	uint32_t element_offset);

/* ---------- globals */

static struct element_layout const plain_element_layout = { 0, NULL, 0, NULL, 0, NULL };

static struct block_layout const bitmap_sequence_blocks[] =
{
	/* sprites */
	{ 0x34, 0x20, &plain_element_layout },
};
static struct element_layout const bitmap_sequence_layout =
{
	0x40, bitmap_sequence_blocks, 1, NULL, 0, NULL
};
static struct element_layout const bitmap_data_layout =
{
	0x30, NULL, 0, NULL, 0, bitmap_data_check
};
static struct block_layout const bitmap_group_blocks[] =
{
	/* sequences, bitmaps */
	{ 0x54, 0x40, &bitmap_sequence_layout },
	{ 0x60, 0x30, &bitmap_data_layout },
};
static uint32_t const bitmap_group_data[] =
{
	/* compressed color plate data, processed pixel data */
	0x1C,
	0x30,
};
static struct element_layout const bitmap_group_layout =
{
	BITMAP_GROUP_BYTES, bitmap_group_blocks, 2, bitmap_group_data, 2, NULL
};

static uint32_t const sound_permutation_data[] =
{
	/* mouth data, subtitle data; the samples are streamed from a file */
	0x54,
	0x68,
};
static struct element_layout const sound_permutation_layout =
{
	SOUND_PERMUTATION_BYTES, NULL, 0, sound_permutation_data, 2, sound_permutation_check
};
static struct block_layout const sound_pitch_range_blocks[] =
{
	/* permutations */
	{ 0x3C, SOUND_PERMUTATION_BYTES, &sound_permutation_layout },
};
static struct element_layout const sound_pitch_range_layout =
{
	SOUND_PITCH_RANGE_BYTES, sound_pitch_range_blocks, 1, NULL, 0, NULL
};
static struct block_layout const sound_definition_blocks[] =
{
	{ SOUND_PITCH_RANGES_OFFSET, SOUND_PITCH_RANGE_BYTES, &sound_pitch_range_layout },
};
static struct element_layout const sound_definition_layout =
{
	SOUND_DEFINITION_BYTES, sound_definition_blocks, 1, NULL, 0, NULL
};

static struct block_layout const font_character_table_blocks[] =
{
	/* character indices */
	{ 0x00, 2, &plain_element_layout },
};
static struct element_layout const font_character_table_layout =
{
	TAG_BLOCK_BYTES, font_character_table_blocks, 1, NULL, 0, NULL
};
static struct block_layout const font_blocks[] =
{
	/* character tables, characters */
	{ 0x30, TAG_BLOCK_BYTES, &font_character_table_layout },
	{ 0x7C, 20, &plain_element_layout },
};
static uint32_t const font_data[] =
{
	/* pixels */
	0x88,
};
static struct element_layout const font_layout =
{
	FONT_BYTES, font_blocks, 2, font_data, 1, NULL
};

static uint32_t const unicode_string_reference_data[] =
{
	0x00,
};
static struct element_layout const unicode_string_reference_layout =
{
	TAG_DATA_BYTES, NULL, 0, unicode_string_reference_data, 1, NULL
};
static struct block_layout const unicode_string_list_blocks[] =
{
	{ 0x00, TAG_DATA_BYTES, &unicode_string_reference_layout },
};
static struct element_layout const unicode_string_list_layout =
{
	UNICODE_STRING_LIST_BYTES, unicode_string_list_blocks, 1, NULL, 0, NULL
};

static struct block_layout const hud_message_text_blocks[] =
{
	/* message elements, messages */
	{ 0x14, 2, &plain_element_layout },
	{ 0x20, 0x40, &plain_element_layout },
};
static uint32_t const hud_message_text_data[] =
{
	/* text */
	0x00,
};
static struct element_layout const hud_message_text_layout =
{
	HUD_MESSAGE_TEXT_BYTES, hud_message_text_blocks, 2, hud_message_text_data, 1, NULL
};

static char const *const cache_file_status_descriptions[NUMBER_OF_CACHE_FILE_STATUSES] =
{
	"ok",
	"the file could not be read",
	"out of memory",
	"the file is too small to hold a header",
	"the file is larger than any map can be",
	"the header or footer signature is wrong",
	"the cache version is not one this build knows",
	"a name or build string in the header is not terminated",
	"the file length in the header does not fit the file or the size limit",
	"the cache is compressed, which Custom Edition caches never are",
	"the tag data range in the header does not fit the file or the tag cache",
	"the OpenSauce header is not valid (version, tag versions or memory upgrade)",
	"the OpenSauce header sets flags OpenSauce does not define",
	"the OpenSauce tag definitions lie outside the file",
	"the tag index signature is not 'tags'",
	"the tag instances do not fit in the tag data",
	"a tag handle does not match its position in the index",
	"a tag name lies outside the tag data or is not terminated",
	"a tag's address lies outside the tag data",
	"the scenario tag is missing, misplaced or not a scenario",
	"the model vertex and index data do not fit in the file",
	"a tag of a group that resource maps never hold is marked as held by one",
	"a tag block or tag data field lies outside the loaded tags",
	"the scenario's structure BSP block is not valid",
	"a structure BSP does not fit in the file or in the tag cache",
	"a structure BSP header is not valid",
	"a resource map header is not valid",
	"a resource map is not of the type needed",
	"a resource map entry lies outside the file or its name is not terminated",
	"a map needs a resource map that was not supplied",
	"a tag's entry is missing from its resource map",
	"a tag held by a resource map does not have the documented layout",
	"bitmap pixels or sound samples lie outside their file",
	"the tag data and the tags held by resource maps do not fit below the structure BSP",
};

static char const *const cache_file_format_descriptions[NUMBER_OF_CACHE_FILE_FORMATS] =
{
	"not a Halo map file",
	"Xbox cache",
	"cache of an unknown version",
	"Custom Edition cache",
	"Custom Edition resource map",
};

static char const *const resource_map_type_descriptions[NUMBER_OF_RESOURCE_MAP_TYPES] =
{
	"none",
	"bitmaps",
	"sounds",
	"loc",
};

static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

/* ---------- private code */

static uint16_t read_u16(
	uint8_t const *bytes)
{
	return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

static uint32_t read_u32(
	uint8_t const *bytes)
{
	return (uint32_t)bytes[0] |
		((uint32_t)bytes[1] << 8) |
		((uint32_t)bytes[2] << 16) |
		((uint32_t)bytes[3] << 24);
}

static int32_t read_s32(
	uint8_t const *bytes)
{
	uint32_t value = read_u32(bytes);

	/* two's complement without relying on an out-of-range conversion */
	return value <= INT32_MAX ? (int32_t)value : -(int32_t)(~value) - 1;
}

static int16_t read_s16(
	uint8_t const *bytes)
{
	uint16_t value = read_u16(bytes);

	return value <= INT16_MAX ? (int16_t)value : (int16_t)(-(int32_t)(uint16_t)~value - 1);
}

static float read_f32(
	uint8_t const *bytes)
{
	uint32_t bits = read_u32(bytes);
	float value;

	memcpy(&value, &bits, sizeof(value));

	return value;
}

static void write_u32(
	uint8_t *bytes,
	uint32_t value)
{
	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);
	bytes[2] = (uint8_t)(value >> 16);
	bytes[3] = (uint8_t)(value >> 24);

	return;
}

static int flag_is_set(
	uint32_t flags,
	int bit)
{
	return (flags >> bit) & 1;
}

/* nonzero when [offset, offset + size) lies within [0, limit) */
static int range_fits(
	uint32_t offset,
	uint32_t size,
	uint32_t limit)
{
	return offset <= limit && size <= limit - offset;
}

/* copies a fixed 32-byte string field, returning nonzero when it holds a
terminator (a name of at most 31 characters) */
static int copy_string_field(
	char *destination,
	uint8_t const *source)
{
	int terminated = memchr(source, 0, CACHE_FILE_STRING_BYTES) != NULL;

	memcpy(destination, source, CACHE_FILE_STRING_BYTES);
	destination[CACHE_FILE_STRING_BYTES - 1] = 0;

	return terminated;
}

static void crc32_table_initialize(
	void)
{
	uint32_t byte_value;

	for (byte_value = 0; byte_value < 256; byte_value++)
	{
		uint32_t crc = byte_value;
		int bit;

		for (bit = 0; bit < 8; bit++)
		{
			crc = (crc & 1) ? (crc >> 1) ^ CRC32_POLYNOMIAL : crc >> 1;
		}
		crc32_table[byte_value] = crc;
	}
	crc32_table_initialized = 1;

	return;
}

static uint32_t crc32_update(
	uint32_t crc,
	uint8_t const *bytes,
	uint32_t size)
{
	uint32_t index;

	if (!crc32_table_initialized)
	{
		crc32_table_initialize();
	}
	for (index = 0; index < size; index++)
	{
		crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[index]) & 0xFF];
	}

	return crc;
}

static enum cache_file_status crc32_update_from_file(
	struct cache_file_source *source,
	uint32_t *crc,
	uint32_t offset,
	uint32_t size)
{
	uint8_t *buffer;
	enum cache_file_status status = _cache_file_status_ok;

	if (!range_fits(offset, size, source->size))
	{
		return _cache_file_status_read_failed;
	}
	buffer = malloc(CHECKSUM_READ_CHUNK_BYTES);
	if (!buffer)
	{
		return _cache_file_status_out_of_memory;
	}
	while (size > 0)
	{
		uint32_t chunk_bytes = size < CHECKSUM_READ_CHUNK_BYTES ? size : CHECKSUM_READ_CHUNK_BYTES;

		if (!source->read(source->context, offset, chunk_bytes, buffer))
		{
			status = _cache_file_status_read_failed;
			break;
		}
		*crc = crc32_update(*crc, buffer, chunk_bytes);
		offset += chunk_bytes;
		size -= chunk_bytes;
	}
	free(buffer);

	return status;
}

static void opensauce_header_read(
	uint8_t const *bytes,
	struct opensauce_cache_header *header)
{
	int resource_index;

	header->version = read_s16(bytes + OPENSAUCE_VERSION_OFFSET);
	header->flags = read_u16(bytes + OPENSAUCE_FLAGS_OFFSET);
	header->project_yellow_version = bytes[OPENSAUCE_PROJECT_YELLOW_VERSION_OFFSET];
	header->project_yellow_globals_version = bytes[OPENSAUCE_PROJECT_YELLOW_GLOBALS_VERSION_OFFSET];
	header->memory_upgrade_amount = read_f32(bytes + OPENSAUCE_MEMORY_UPGRADE_AMOUNT_OFFSET);
	header->definitions_size = read_u32(bytes + OPENSAUCE_DEFINITIONS_SIZE_OFFSET);
	header->definitions_decompressed_size = read_u32(bytes + OPENSAUCE_DEFINITIONS_DECOMPRESSED_SIZE_OFFSET);
	header->definitions_offset = read_u32(bytes + OPENSAUCE_DEFINITIONS_OFFSET_OFFSET);
	header->build_stage = read_s16(bytes + OPENSAUCE_BUILD_STAGE_OFFSET);
	header->build_revision = read_u32(bytes + OPENSAUCE_BUILD_REVISION_OFFSET);
	header->build_timestamp = (int64_t)read_s32(bytes + OPENSAUCE_BUILD_TIMESTAMP_OFFSET + 4) * 0x100000000LL +
		(int64_t)read_u32(bytes + OPENSAUCE_BUILD_TIMESTAMP_OFFSET);
	header->tools_version_major = bytes[OPENSAUCE_TOOLS_VERSION_OFFSET];
	header->tools_version_minor = bytes[OPENSAUCE_TOOLS_VERSION_OFFSET + 1];
	header->tools_version_build = read_u16(bytes + OPENSAUCE_TOOLS_VERSION_OFFSET + 2);
	header->minimum_version_major = bytes[OPENSAUCE_MINIMUM_VERSION_OFFSET];
	header->minimum_version_minor = bytes[OPENSAUCE_MINIMUM_VERSION_OFFSET + 1];
	header->minimum_version_build = read_u16(bytes + OPENSAUCE_MINIMUM_VERSION_OFFSET + 2);
	for (resource_index = 0; resource_index < 4; resource_index++)
	{
		header->resource_offsets[resource_index] =
			read_u32(bytes + OPENSAUCE_RESOURCE_OFFSETS_OFFSET + resource_index * 4);
	}

	return;
}

/* the checks OpenSauce's s_cache_header_yelo::IsValid makes, except the
minimum OpenSauce version, which only concerns OpenSauce itself */
static enum cache_file_status opensauce_header_verify(
	uint8_t const *bytes,
	struct cache_file_identity *identity)
{
	struct opensauce_cache_header *header = &identity->opensauce;
	float amount;

	if (!copy_string_field(header->definitions_build, bytes + OPENSAUCE_DEFINITIONS_BUILD_OFFSET) ||
		!copy_string_field(header->mod_name, bytes + OPENSAUCE_MOD_NAME_OFFSET) ||
		!copy_string_field(header->build_string, bytes + OPENSAUCE_BUILD_STRING_OFFSET))
	{
		return _cache_file_status_unterminated_string;
	}
	opensauce_header_read(bytes, header);
	amount = header->memory_upgrade_amount;
	if ((header->version != OPENSAUCE_HEADER_VERSION &&
		header->version != OPENSAUCE_HEADER_VERSION_WITH_MINIMUM_BUILD) ||
		header->project_yellow_version != OPENSAUCE_PROJECT_YELLOW_VERSION ||
		header->project_yellow_globals_version != OPENSAUCE_PROJECT_YELLOW_GLOBALS_VERSION ||
		!(amount >= 0.0f && amount <= OPENSAUCE_MEMORY_UPGRADE_AMOUNT))
	{
		return _cache_file_status_bad_opensauce_header;
	}
	if (header->flags >> NUMBER_OF_OPENSAUCE_CACHE_FLAGS)
	{
		return _cache_file_status_unknown_opensauce_flags;
	}
	if (flag_is_set(header->flags, _opensauce_cache_uses_mod_data_files_bit) && !header->mod_name[0])
	{
		return _cache_file_status_bad_opensauce_header;
	}
	if (header->definitions_size &&
		(header->definitions_offset < identity->file_length ||
		!range_fits(header->definitions_offset, header->definitions_size, identity->file_size) ||
		!header->definitions_decompressed_size))
	{
		return _cache_file_status_bad_opensauce_definitions_range;
	}

	return _cache_file_status_ok;
}

static enum cache_file_status custom_edition_header_verify(
	uint8_t const *bytes,
	struct cache_file_identity *identity)
{
	enum cache_file_status status;
	uint32_t maximum_file_length;

	identity->has_opensauce_header =
		read_u32(bytes + CACHE_HEADER_OPENSAUCE_OFFSET) == OPENSAUCE_HEADER_SIGNATURE;
	if (identity->has_opensauce_header)
	{
		status = opensauce_header_verify(bytes + CACHE_HEADER_OPENSAUCE_OFFSET, identity);
		if (status != _cache_file_status_ok)
		{
			return status;
		}
	}
	if (identity->compressed_file_length)
	{
		return _cache_file_status_compressed_cache;
	}
	maximum_file_length = identity->has_opensauce_header &&
		flag_is_set(identity->opensauce.flags, _opensauce_cache_uses_memory_upgrades_bit) ?
		CUSTOM_EDITION_CACHE_FILE_MAXIMUM_BYTES_UPGRADED :
		CUSTOM_EDITION_CACHE_FILE_MAXIMUM_BYTES;
	if (identity->file_length < CACHE_FILE_HEADER_BYTES ||
		identity->file_length > maximum_file_length ||
		identity->file_length > identity->file_size)
	{
		return _cache_file_status_bad_file_length;
	}
	if (identity->tag_data_offset < CACHE_FILE_HEADER_BYTES ||
		identity->tag_data_size < TAG_INDEX_BYTES ||
		identity->tag_data_size > custom_edition_tag_cache_bytes(identity) ||
		!range_fits(identity->tag_data_offset, identity->tag_data_size, identity->file_length))
	{
		return _cache_file_status_bad_tag_data_range;
	}

	return _cache_file_status_ok;
}

static enum cache_file_status cache_header_identify(
	uint8_t const *bytes,
	uint32_t byte_count,
	struct cache_file_identity *identity)
{
	if (byte_count < CACHE_FILE_HEADER_BYTES)
	{
		return _cache_file_status_file_too_small;
	}
	if (read_u32(bytes + CACHE_HEADER_FOOTER_OFFSET) != CACHE_FOOTER_SIGNATURE)
	{
		return _cache_file_status_bad_signatures;
	}
	identity->version = read_s32(bytes + CACHE_HEADER_VERSION_OFFSET);
	identity->file_length = read_u32(bytes + CACHE_HEADER_FILE_LENGTH_OFFSET);
	identity->compressed_file_length = read_u32(bytes + CACHE_HEADER_COMPRESSED_LENGTH_OFFSET);
	identity->tag_data_offset = read_u32(bytes + CACHE_HEADER_TAG_DATA_OFFSET_OFFSET);
	identity->tag_data_size = read_u32(bytes + CACHE_HEADER_TAG_DATA_SIZE_OFFSET);
	identity->scenario_type = read_s16(bytes + CACHE_HEADER_SCENARIO_TYPE_OFFSET);
	identity->checksum = read_u32(bytes + CACHE_HEADER_CHECKSUM_OFFSET);
	switch (identity->version)
	{
	case CACHE_FILE_VERSION_XBOX:
		identity->format = _cache_file_format_xbox_cache;
		break;
	case CACHE_FILE_VERSION_CUSTOM_EDITION:
		identity->format = _cache_file_format_custom_edition_cache;
		break;
	default:
		identity->format = _cache_file_format_other_cache;
		break;
	}
	if (!copy_string_field(identity->name, bytes + CACHE_HEADER_NAME_OFFSET) ||
		!copy_string_field(identity->build, bytes + CACHE_HEADER_BUILD_OFFSET))
	{
		return _cache_file_status_unterminated_string;
	}
	switch (identity->format)
	{
	case _cache_file_format_xbox_cache:
		/* the Xbox header is this build's own; cache_file_header_verify
		judges it */
		return _cache_file_status_ok;
	case _cache_file_format_custom_edition_cache:
		return custom_edition_header_verify(bytes, identity);
	default:
		return _cache_file_status_unsupported_version;
	}
}

static enum cache_file_status resource_map_header_identify(
	uint8_t const *bytes,
	struct cache_file_identity *identity)
{
	uint32_t type = read_u32(bytes);
	uint32_t names_offset = read_u32(bytes + RESOURCE_MAP_NAMES_OFFSET_OFFSET);
	uint32_t index_offset = read_u32(bytes + RESOURCE_MAP_INDEX_OFFSET_OFFSET);
	int32_t item_count = read_s32(bytes + RESOURCE_MAP_COUNT_OFFSET);

	if (type <= _resource_map_none || type >= NUMBER_OF_RESOURCE_MAP_TYPES)
	{
		identity->format = _cache_file_format_unrecognized;
		return _cache_file_status_ok;
	}
	identity->format = _cache_file_format_resource_map;
	identity->resource_map_type = (enum resource_map_type)type;
	identity->resource_item_count = item_count;
	/* the item data, then the names, then the index table */
	if (names_offset < RESOURCE_MAP_HEADER_BYTES ||
		index_offset < names_offset ||
		item_count < 0 ||
		index_offset > identity->file_size ||
		(uint32_t)item_count > (identity->file_size - index_offset) / RESOURCE_MAP_ITEM_BYTES)
	{
		return _cache_file_status_bad_resource_map_header;
	}

	return _cache_file_status_ok;
}

static enum cache_file_status load_fail(
	struct load_state *state,
	enum cache_file_status status,
	uint32_t location)
{
	state->report->status = status;
	state->report->problem_tag_index = state->tag_index;
	state->report->problem_location = location;

	return status;
}

/* the offset in the tag cache of the runtime address `address`, when
`size` bytes there lie within the bytes in use */
static int tag_cache_offset(
	struct load_state const *state,
	uint32_t address,
	uint32_t size,
	uint32_t *offset)
{
	if (address < CUSTOM_EDITION_TAG_CACHE_ADDRESS ||
		!range_fits(address - CUSTOM_EDITION_TAG_CACHE_ADDRESS, size, state->used_bytes))
	{
		return 0;
	}
	*offset = address - CUSTOM_EDITION_TAG_CACHE_ADDRESS;

	return 1;
}

/* the name of a tag instance, terminated within the tag data (checked at
every use, since loading writes into the tag data) */
static char const *tag_name_get(
	struct load_state const *state,
	uint8_t const *instance)
{
	uint32_t name_address = read_u32(instance + TAG_INSTANCE_NAME_OFFSET);
	uint32_t name_offset;
	uint32_t search_bytes;

	if (!tag_cache_offset(state, name_address, 1, &name_offset) ||
		name_offset >= state->report->tag_data_bytes)
	{
		return NULL;
	}
	search_bytes = state->report->tag_data_bytes - name_offset;
	if (search_bytes > TAG_NAME_MAXIMUM_LENGTH + 1)
	{
		search_bytes = TAG_NAME_MAXIMUM_LENGTH + 1;
	}

	return memchr(state->tag_cache + name_offset, 0, search_bytes) ?
		(char const *)state->tag_cache + name_offset :
		NULL;
}

static enum cache_file_status bitmap_data_check(
	struct load_state *state,
	uint32_t element_offset)
{
	uint8_t const *bitmap = state->tag_cache + element_offset;
	uint32_t pixels_offset = read_u32(bitmap + BITMAP_DATA_PIXELS_OFFSET_OFFSET);
	uint32_t pixels_size = read_u32(bitmap + BITMAP_DATA_PIXELS_SIZE_OFFSET);
	uint32_t limit;

	/* Reclaimer (Halo1/BitmapTag.cs): the pixels are in bitmaps.map when
	the bitmap says so, else in the map itself */
	if (flag_is_set(read_u16(bitmap + BITMAP_DATA_FLAGS_OFFSET), BITMAP_DATA_IN_RESOURCE_MAP_BIT))
	{
		struct resource_map const *bitmaps = state->resource_maps[_resource_map_bitmaps];

		if (!bitmaps)
		{
			return load_fail(state, _cache_file_status_missing_resource_map, element_offset);
		}
		limit = bitmaps->source->size;
	}
	else
	{
		limit = state->file_length;
	}
	if (!range_fits(pixels_offset, pixels_size, limit))
	{
		return load_fail(state, _cache_file_status_bad_resource_data_range, pixels_offset);
	}
	state->report->bitmap_data_ranges_checked++;

	return _cache_file_status_ok;
}

static enum cache_file_status sound_permutation_check(
	struct load_state *state,
	uint32_t element_offset)
{
	uint8_t const *samples = state->tag_cache + element_offset + SOUND_PERMUTATION_SAMPLES_OFFSET;
	int32_t size = read_s32(samples + TAG_DATA_SIZE_OFFSET);
	uint32_t file_offset = read_u32(samples + TAG_DATA_FILE_OFFSET_OFFSET);
	uint32_t limit;

	if (size < 0)
	{
		return load_fail(state, _cache_file_status_bad_resource_data_range, element_offset);
	}
	if (flag_is_set(read_u32(samples + TAG_DATA_FLAGS_OFFSET), SOUND_SAMPLES_IN_RESOURCE_MAP_BIT))
	{
		struct resource_map const *sounds = state->resource_maps[_resource_map_sounds];

		if (!sounds)
		{
			return load_fail(state, _cache_file_status_missing_resource_map, element_offset);
		}
		limit = sounds->source->size;
	}
	else
	{
		limit = state->file_length;
	}
	if (!range_fits(file_offset, (uint32_t)size, limit))
	{
		return load_fail(state, _cache_file_status_bad_resource_data_range, file_offset);
	}
	state->report->sound_sample_ranges_checked++;

	return _cache_file_status_ok;
}

/* Walks the element at `element_offset` of the tag cache and everything its
blocks hold, requiring every block and every present tag data field to lie
within the bytes in use. Tag data with a size but no address is data the
cache build left out (bitmaps' compressed color plates), which the game
never reads. */
static enum cache_file_status element_verify(
	struct load_state *state,
	uint32_t element_offset,
	struct element_layout const *layout,
	int depth)
{
	enum cache_file_status status = _cache_file_status_ok;
	int field_index;

	/* the layouts nest at most three deep; this also stops runaway input */
	if (depth > 4)
	{
		return load_fail(state, _cache_file_status_bad_tag_layout, element_offset);
	}
	for (field_index = 0; field_index < layout->data_count; field_index++)
	{
		uint8_t const *data = state->tag_cache + element_offset + layout->data_offsets[field_index];
		int32_t size = read_s32(data + TAG_DATA_SIZE_OFFSET);
		uint32_t address = read_u32(data + TAG_DATA_ADDRESS_OFFSET);
		uint32_t offset;

		if (size < 0 || (address && !tag_cache_offset(state, address, (uint32_t)size, &offset)))
		{
			return load_fail(state, _cache_file_status_bad_tag_layout, element_offset);
		}
	}
	for (field_index = 0; field_index < layout->block_count && status == _cache_file_status_ok; field_index++)
	{
		struct block_layout const *block = &layout->blocks[field_index];
		uint8_t const *field = state->tag_cache + element_offset + block->offset;
		int32_t count = read_s32(field + TAG_BLOCK_COUNT_OFFSET);
		uint32_t address = read_u32(field + TAG_BLOCK_ADDRESS_OFFSET);
		uint32_t offset;
		int32_t element_index;

		if (count == 0)
		{
			continue;
		}
		if (count < 0 ||
			(uint32_t)count > UINT32_MAX / block->element_bytes ||
			!tag_cache_offset(state, address, (uint32_t)count * block->element_bytes, &offset))
		{
			return load_fail(state, _cache_file_status_bad_tag_layout, element_offset);
		}
		for (element_index = 0; element_index < count && status == _cache_file_status_ok; element_index++)
		{
			status = element_verify(
				state,
				offset + (uint32_t)element_index * block->element_bytes,
				block->element,
				depth + 1);
		}
	}
	if (status == _cache_file_status_ok && layout->check)
	{
		status = layout->check(state, element_offset);
	}

	return status;
}

/* Turns the addresses in an element read from a resource map, which count
from `item_base` within the `item_bytes` of the item, into runtime addresses
of the item placed at `item_address`. The editing kit's definition pointers
are cleared: they refer to nothing in this process. */
static enum cache_file_status element_relocate(
	struct load_state *state,
	uint8_t *item,
	uint32_t item_bytes,
	uint32_t element_offset,
	struct element_layout const *layout,
	uint32_t item_address,
	int depth)
{
	enum cache_file_status status = _cache_file_status_ok;
	int field_index;

	if (depth > 4 || !range_fits(element_offset, layout->bytes, item_bytes))
	{
		return load_fail(state, _cache_file_status_bad_resource_layout, element_offset);
	}
	for (field_index = 0; field_index < layout->data_count; field_index++)
	{
		uint8_t *data = item + element_offset + layout->data_offsets[field_index];
		int32_t size = read_s32(data + TAG_DATA_SIZE_OFFSET);
		uint32_t address = read_u32(data + TAG_DATA_ADDRESS_OFFSET);

		if (size < 0 || (address && !range_fits(address, (uint32_t)size, item_bytes)))
		{
			return load_fail(state, _cache_file_status_bad_resource_layout, element_offset);
		}
		if (address)
		{
			write_u32(data + TAG_DATA_ADDRESS_OFFSET, item_address + address);
			state->report->relocated_pointer_count++;
		}
		write_u32(data + TAG_DATA_DEFINITION_OFFSET, 0);
	}
	for (field_index = 0; field_index < layout->block_count && status == _cache_file_status_ok; field_index++)
	{
		struct block_layout const *block = &layout->blocks[field_index];
		uint8_t *field = item + element_offset + block->offset;
		int32_t count = read_s32(field + TAG_BLOCK_COUNT_OFFSET);
		uint32_t address = read_u32(field + TAG_BLOCK_ADDRESS_OFFSET);
		int32_t element_index;

		write_u32(field + TAG_BLOCK_DEFINITION_OFFSET, 0);
		if (count == 0)
		{
			continue;
		}
		if (count < 0 ||
			(uint32_t)count > UINT32_MAX / block->element_bytes ||
			!range_fits(address, (uint32_t)count * block->element_bytes, item_bytes))
		{
			return load_fail(state, _cache_file_status_bad_resource_layout, element_offset);
		}
		write_u32(field + TAG_BLOCK_ADDRESS_OFFSET, item_address + address);
		state->report->relocated_pointer_count++;
		for (element_index = 0; element_index < count && status == _cache_file_status_ok; element_index++)
		{
			status = element_relocate(
				state,
				item,
				item_bytes,
				address + (uint32_t)element_index * block->element_bytes,
				block->element,
				item_address,
				depth + 1);
		}
	}

	return status;
}

/* room for `size` more bytes of resource-held tags after what is in use */
static enum cache_file_status tag_cache_allocate(
	struct load_state *state,
	uint32_t size,
	uint32_t *offset)
{
	uint32_t aligned = (state->used_bytes + 3) & ~(uint32_t)3;

	if (aligned < state->used_bytes || !range_fits(aligned, size, state->usable_bytes))
	{
		return load_fail(state, _cache_file_status_tag_cache_overflow, state->used_bytes);
	}
	*offset = aligned;
	state->report->resource_tag_bytes += size + (aligned - state->used_bytes);
	state->used_bytes = aligned + size;

	return _cache_file_status_ok;
}

static struct resource_map_item const *resource_map_find(
	struct resource_map const *map,
	char const *name)
{
	int32_t item_index;

	for (item_index = 0; item_index < map->item_count; item_index++)
	{
		if (!strcmp(map->items[item_index].name, name))
		{
			return &map->items[item_index];
		}
	}

	return NULL;
}

/* Bitmaps, fonts, unicode string lists and HUD message text held by a
resource map: the tag's address field is the index of its entry, whose name
is the tag's path (observed for every such tag of the stock maps; Reclaimer
reads bitmaps the same way). The entry is the whole tag, its addresses
counting from the start of the entry. */
static enum cache_file_status resource_tag_load(
	struct load_state *state,
	uint8_t *instance,
	char const *name,
	enum resource_map_type type,
	struct element_layout const *layout)
{
	struct resource_map const *map = state->resource_maps[type];
	uint32_t item_index = read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET);
	struct resource_map_item const *item;
	enum cache_file_status status;
	uint32_t offset;

	if (!map)
	{
		return load_fail(state, _cache_file_status_missing_resource_map, item_index);
	}
	if (item_index >= (uint32_t)map->item_count || strcmp(map->items[item_index].name, name))
	{
		return load_fail(state, _cache_file_status_missing_resource_item, item_index);
	}
	item = &map->items[item_index];
	status = tag_cache_allocate(state, item->size, &offset);
	if (status != _cache_file_status_ok)
	{
		return status;
	}
	if (!map->source->read(map->source->context, item->data_offset, item->size, state->tag_cache + offset))
	{
		return load_fail(state, _cache_file_status_read_failed, item->data_offset);
	}
	status = element_relocate(
		state,
		state->tag_cache + offset,
		item->size,
		0,
		layout,
		CUSTOM_EDITION_TAG_CACHE_ADDRESS + offset,
		0);
	if (status == _cache_file_status_ok && layout == &font_layout)
	{
		int reference_index;

		/* a style reference would name a tag index of whatever map the
		resource map was built with; every stock font has none */
		for (reference_index = 0; reference_index < FONT_STYLE_REFERENCE_COUNT; reference_index++)
		{
			uint8_t const *reference = state->tag_cache + offset + FONT_STYLE_REFERENCES_OFFSET +
				reference_index * TAG_REFERENCE_BYTES;

			if (read_s32(reference + TAG_REFERENCE_INDEX_OFFSET) != NO_TAG_INDEX)
			{
				return load_fail(state, _cache_file_status_bad_resource_layout, item->data_offset);
			}
		}
	}
	if (status == _cache_file_status_ok)
	{
		write_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET, CUSTOM_EDITION_TAG_CACHE_ADDRESS + offset);
		state->report->resource_tag_counts[type]++;
	}

	return status;
}

/* Sounds held by sounds.map: the map keeps the tag's 0xA4-byte header, its
pitch range block with a count and no address, and the tag's address field
points at that header. The entry named by the tag's path repeats the header
and then holds the pitch ranges and their permutations, whose addresses
count from the first pitch range. (Observed in every stock map: OpenSauce
leaves this path of cache_file_data_load unimplemented and hooks the game's
own data_file_read instead.) */
static enum cache_file_status resource_sound_load(
	struct load_state *state,
	uint8_t *instance,
	char const *name)
{
	struct resource_map const *map = state->resource_maps[_resource_map_sounds];
	uint32_t header_address = read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET);
	struct resource_map_item const *item;
	uint8_t *pitch_ranges_field;
	uint32_t header_offset;
	uint32_t data_bytes;
	uint32_t offset;
	int32_t pitch_range_count;
	int32_t pitch_range_index;
	enum cache_file_status status;

	if (!map)
	{
		return load_fail(state, _cache_file_status_missing_resource_map, header_address);
	}
	if (!tag_cache_offset(state, header_address, SOUND_DEFINITION_BYTES, &header_offset))
	{
		return load_fail(state, _cache_file_status_bad_tag_address, header_address);
	}
	item = resource_map_find(map, name);
	if (!item)
	{
		return load_fail(state, _cache_file_status_missing_resource_item, header_address);
	}
	pitch_ranges_field = state->tag_cache + header_offset + SOUND_PITCH_RANGES_OFFSET;
	pitch_range_count = read_s32(pitch_ranges_field + TAG_BLOCK_COUNT_OFFSET);
	if (item->size < SOUND_DEFINITION_BYTES ||
		pitch_range_count < 0 ||
		(uint32_t)pitch_range_count > (item->size - SOUND_DEFINITION_BYTES) / SOUND_PITCH_RANGE_BYTES)
	{
		return load_fail(state, _cache_file_status_bad_resource_layout, item->data_offset);
	}
	data_bytes = item->size - SOUND_DEFINITION_BYTES;
	status = tag_cache_allocate(state, data_bytes, &offset);
	if (status != _cache_file_status_ok)
	{
		return status;
	}
	if (!map->source->read(
		map->source->context,
		item->data_offset + SOUND_DEFINITION_BYTES,
		data_bytes,
		state->tag_cache + offset))
	{
		return load_fail(state, _cache_file_status_read_failed, item->data_offset);
	}
	for (pitch_range_index = 0; pitch_range_index < pitch_range_count; pitch_range_index++)
	{
		status = element_relocate(
			state,
			state->tag_cache + offset,
			data_bytes,
			(uint32_t)pitch_range_index * SOUND_PITCH_RANGE_BYTES,
			&sound_pitch_range_layout,
			CUSTOM_EDITION_TAG_CACHE_ADDRESS + offset,
			1);
		if (status != _cache_file_status_ok)
		{
			return status;
		}
	}
	if (pitch_range_count)
	{
		write_u32(pitch_ranges_field + TAG_BLOCK_ADDRESS_OFFSET, CUSTOM_EDITION_TAG_CACHE_ADDRESS + offset);
		state->report->relocated_pointer_count++;
	}
	state->report->resource_tag_counts[_resource_map_sounds]++;

	return _cache_file_status_ok;
}

static struct element_layout const *group_layout(
	uint32_t group_tag)
{
	switch (group_tag)
	{
	case BITMAP_GROUP_TAG:
		return &bitmap_group_layout;
	case SOUND_GROUP_TAG:
		return &sound_definition_layout;
	case FONT_GROUP_TAG:
		return &font_layout;
	case UNICODE_STRING_LIST_GROUP_TAG:
		return &unicode_string_list_layout;
	case HUD_MESSAGE_TEXT_GROUP_TAG:
		return &hud_message_text_layout;
	default:
		return NULL;
	}
}

static enum cache_file_status structure_bsps_verify(
	struct load_state *state,
	uint8_t const *tag_instances,
	int32_t tag_count,
	uint32_t scenario_offset,
	uint32_t *checksum)
{
	uint8_t const *block = state->tag_cache + scenario_offset + SCENARIO_STRUCTURE_BSPS_OFFSET;
	int32_t count = read_s32(block + TAG_BLOCK_COUNT_OFFSET);
	uint32_t address = read_u32(block + TAG_BLOCK_ADDRESS_OFFSET);
	uint32_t tag_cache_bytes = state->report->tag_cache_bytes;
	uint32_t checksum_offset = CACHE_FILE_HEADER_BYTES;
	uint32_t block_offset;
	int32_t bsp_index;

	state->usable_bytes = tag_cache_bytes;
	if (count < 0 ||
		count > MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO ||
		(count && !tag_cache_offset(state, address, (uint32_t)count * STRUCTURE_BSP_REFERENCE_BYTES, &block_offset)))
	{
		return load_fail(state, _cache_file_status_bad_structure_bsp_block, address);
	}
	state->report->structure_bsp_count = count;
	for (bsp_index = 0; bsp_index < count; bsp_index++)
	{
		uint8_t const *reference = state->tag_cache + block_offset + (uint32_t)bsp_index * STRUCTURE_BSP_REFERENCE_BYTES;
		int32_t file_offset = read_s32(reference + STRUCTURE_BSP_REFERENCE_FILE_OFFSET_OFFSET);
		int32_t size = read_s32(reference + STRUCTURE_BSP_REFERENCE_SIZE_OFFSET);
		uint32_t bsp_address = read_u32(reference + STRUCTURE_BSP_REFERENCE_ADDRESS_OFFSET);
		uint32_t tag_handle = read_u32(reference + STRUCTURE_BSP_REFERENCE_TAG_INDEX_OFFSET);
		uint32_t tag_index = tag_handle & ABSOLUTE_INDEX_MASK;
		uint8_t header[STRUCTURE_BSP_HEADER_BYTES];
		uint32_t bsp_pointer;
		enum cache_file_status status;

		/* the tag the reference names: a structure BSP, not yet loaded */
		if (tag_index >= (uint32_t)tag_count ||
			read_u32(tag_instances + tag_index * TAG_INSTANCE_BYTES + TAG_INSTANCE_HANDLE_OFFSET) != tag_handle ||
			read_u32(tag_instances + tag_index * TAG_INSTANCE_BYTES + TAG_INSTANCE_GROUP_OFFSET) != STRUCTURE_BSP_GROUP_TAG ||
			read_u32(tag_instances + tag_index * TAG_INSTANCE_BYTES + TAG_INSTANCE_ADDRESS_OFFSET))
		{
			return load_fail(state, _cache_file_status_bad_structure_bsp_block, tag_handle);
		}
		/* in the file, and at the top of the tag cache above the tag data,
		where it is loaded when the game switches to it */
		if (file_offset < CACHE_FILE_HEADER_BYTES ||
			size < STRUCTURE_BSP_HEADER_BYTES ||
			!range_fits((uint32_t)file_offset, (uint32_t)size, state->file_length) ||
			bsp_address < CUSTOM_EDITION_TAG_CACHE_ADDRESS + state->used_bytes ||
			!range_fits(bsp_address - CUSTOM_EDITION_TAG_CACHE_ADDRESS, (uint32_t)size, tag_cache_bytes))
		{
			return load_fail(state, _cache_file_status_bad_structure_bsp_range, (uint32_t)file_offset);
		}
		if (!state->map->read(state->map->context, (uint32_t)file_offset, sizeof(header), header))
		{
			return load_fail(state, _cache_file_status_read_failed, (uint32_t)file_offset);
		}
		bsp_pointer = read_u32(header + STRUCTURE_BSP_HEADER_BSP_OFFSET);
		/* PC caches have no Xbox vertex buffer arrays */
		if (read_u32(header + STRUCTURE_BSP_HEADER_SIGNATURE_OFFSET) != STRUCTURE_BSP_SIGNATURE ||
			read_u32(header + STRUCTURE_BSP_HEADER_VERTEX_BUFFERS_OFFSET) ||
			read_u32(header + STRUCTURE_BSP_HEADER_LIGHTMAP_VERTEX_BUFFERS_OFFSET) ||
			bsp_pointer < bsp_address + STRUCTURE_BSP_HEADER_BYTES ||
			bsp_pointer - bsp_address >= (uint32_t)size)
		{
			return load_fail(state, _cache_file_status_bad_structure_bsp_header, (uint32_t)file_offset);
		}
		if ((uint32_t)size > state->report->largest_structure_bsp_bytes)
		{
			state->report->largest_structure_bsp_bytes = (uint32_t)size;
		}
		if (bsp_address - CUSTOM_EDITION_TAG_CACHE_ADDRESS < state->usable_bytes)
		{
			state->usable_bytes = bsp_address - CUSTOM_EDITION_TAG_CACHE_ADDRESS;
			state->report->lowest_structure_bsp_address = bsp_address;
		}
		/* OpenSauce's CalculateChecksum takes the structure BSPs as packed
		one after another from the end of the header */
		if (!range_fits(checksum_offset, (uint32_t)size, state->map->size))
		{
			return load_fail(state, _cache_file_status_bad_structure_bsp_range, checksum_offset);
		}
		status = crc32_update_from_file(state->map, checksum, checksum_offset, (uint32_t)size);
		if (status != _cache_file_status_ok)
		{
			return load_fail(state, status, checksum_offset);
		}
		checksum_offset += (uint32_t)size;
	}

	return _cache_file_status_ok;
}

/* ---------- public code */

char const *cache_file_status_describe(
	enum cache_file_status status)
{
	return status >= 0 && status < NUMBER_OF_CACHE_FILE_STATUSES ?
		cache_file_status_descriptions[status] :
		"unknown status";
}

char const *cache_file_format_describe(
	enum cache_file_format format)
{
	return format >= 0 && format < NUMBER_OF_CACHE_FILE_FORMATS ?
		cache_file_format_descriptions[format] :
		"unknown format";
}

char const *resource_map_type_describe(
	enum resource_map_type type)
{
	return type >= 0 && type < NUMBER_OF_RESOURCE_MAP_TYPES ?
		resource_map_type_descriptions[type] :
		"unknown";
}

uint32_t custom_edition_tag_cache_bytes(
	struct cache_file_identity const *identity)
{
	return identity->has_opensauce_header &&
		flag_is_set(identity->opensauce.flags, _opensauce_cache_uses_memory_upgrades_bit) ?
		CUSTOM_EDITION_TAG_CACHE_BYTES_UPGRADED :
		CUSTOM_EDITION_TAG_CACHE_BYTES;
}

enum cache_file_status cache_file_identify(
	struct cache_file_source *source,
	struct cache_file_identity *identity)
{
	uint8_t header[CACHE_FILE_HEADER_BYTES];
	uint32_t header_bytes = source->size < CACHE_FILE_HEADER_BYTES ? source->size : CACHE_FILE_HEADER_BYTES;

	memset(identity, 0, sizeof(*identity));
	identity->file_size = source->size;
	if (header_bytes < RESOURCE_MAP_HEADER_BYTES)
	{
		return _cache_file_status_file_too_small;
	}
	if (!source->read(source->context, 0, header_bytes, header))
	{
		return _cache_file_status_read_failed;
	}
	if (read_u32(header) == CACHE_HEADER_SIGNATURE)
	{
		return cache_header_identify(header, header_bytes, identity);
	}

	return resource_map_header_identify(header, identity);
}

enum cache_file_format cache_file_header_format(
	void const *header,
	int *has_opensauce_header)
{
	uint8_t const *bytes = header;
	enum cache_file_format format;

	*has_opensauce_header = 0;
	if (read_u32(bytes) != CACHE_HEADER_SIGNATURE ||
		read_u32(bytes + CACHE_HEADER_FOOTER_OFFSET) != CACHE_FOOTER_SIGNATURE)
	{
		return _cache_file_format_unrecognized;
	}
	switch (read_s32(bytes + CACHE_HEADER_VERSION_OFFSET))
	{
	case CACHE_FILE_VERSION_XBOX:
		format = _cache_file_format_xbox_cache;
		break;
	case CACHE_FILE_VERSION_CUSTOM_EDITION:
		format = _cache_file_format_custom_edition_cache;
		*has_opensauce_header =
			read_u32(bytes + CACHE_HEADER_OPENSAUCE_OFFSET) == OPENSAUCE_HEADER_SIGNATURE;
		break;
	default:
		format = _cache_file_format_other_cache;
		break;
	}

	return format;
}

enum cache_file_status resource_map_open(
	struct cache_file_source *source,
	enum resource_map_type expected_type,
	struct resource_map *map)
{
	struct cache_file_identity identity;
	uint8_t header[RESOURCE_MAP_HEADER_BYTES];
	uint8_t *index;
	uint32_t names_offset;
	uint32_t names_bytes;
	uint32_t index_offset;
	int32_t item_index;
	enum cache_file_status status;

	memset(map, 0, sizeof(*map));
	map->source = source;
	status = cache_file_identify(source, &identity);
	if (status != _cache_file_status_ok)
	{
		return status;
	}
	if (identity.format != _cache_file_format_resource_map)
	{
		return _cache_file_status_bad_resource_map_header;
	}
	if (identity.resource_map_type != expected_type)
	{
		return _cache_file_status_wrong_resource_map_type;
	}
	if (!source->read(source->context, 0, sizeof(header), header))
	{
		return _cache_file_status_read_failed;
	}
	names_offset = read_u32(header + RESOURCE_MAP_NAMES_OFFSET_OFFSET);
	index_offset = read_u32(header + RESOURCE_MAP_INDEX_OFFSET_OFFSET);
	names_bytes = index_offset - names_offset;
	map->type = expected_type;
	map->item_count = identity.resource_item_count;
	map->items = calloc((size_t)map->item_count + 1, sizeof(*map->items));
	map->names = malloc((size_t)names_bytes + 1);
	index = malloc((size_t)map->item_count * RESOURCE_MAP_ITEM_BYTES + 1);
	if (!map->items || !map->names || !index)
	{
		free(index);
		resource_map_close(map);
		return _cache_file_status_out_of_memory;
	}
	if (!source->read(source->context, names_offset, names_bytes, map->names) ||
		!source->read(source->context, index_offset, (uint32_t)map->item_count * RESOURCE_MAP_ITEM_BYTES, index))
	{
		free(index);
		resource_map_close(map);
		return _cache_file_status_read_failed;
	}
	map->names[names_bytes] = 0;
	for (item_index = 0; item_index < map->item_count; item_index++)
	{
		uint8_t const *entry = index + (uint32_t)item_index * RESOURCE_MAP_ITEM_BYTES;
		uint32_t name_offset = read_u32(entry + RESOURCE_MAP_ITEM_NAME_OFFSET);
		uint32_t size = read_u32(entry + RESOURCE_MAP_ITEM_SIZE_OFFSET);
		uint32_t data_offset = read_u32(entry + RESOURCE_MAP_ITEM_DATA_OFFSET);

		/* the item's data lies between the header and the names, and its
		name is terminated within the names */
		if (name_offset >= names_bytes ||
			!memchr(map->names + name_offset, 0, names_bytes - name_offset) ||
			data_offset < RESOURCE_MAP_HEADER_BYTES ||
			!range_fits(data_offset, size, names_offset))
		{
			free(index);
			resource_map_close(map);
			return _cache_file_status_bad_resource_item;
		}
		map->items[item_index].data_offset = data_offset;
		map->items[item_index].size = size;
		map->items[item_index].name = map->names + name_offset;
	}
	free(index);

	return _cache_file_status_ok;
}

void resource_map_close(
	struct resource_map *map)
{
	free(map->items);
	free(map->names);
	map->items = NULL;
	map->names = NULL;
	map->item_count = 0;

	return;
}

enum cache_file_status custom_edition_cache_load(
	struct cache_file_source *map,
	struct resource_map *const resource_maps[NUMBER_OF_RESOURCE_MAP_TYPES],
	uint8_t *tag_cache,
	uint32_t tag_cache_bytes,
	struct custom_edition_load_report *report)
{
	struct cache_file_identity *identity = &report->identity;
	struct load_state state;
	uint8_t *tag_index;
	uint8_t *tag_instances;
	uint32_t instances_offset;
	uint32_t scenario_offset;
	uint32_t scenario_handle;
	uint32_t vertex_data_offset;
	uint32_t model_data_size;
	uint32_t checksum = CRC32_INITIAL;
	uint32_t data_end;
	int32_t tag_count;
	int32_t tag_index_value;
	enum cache_file_status status;

	memset(report, 0, sizeof(*report));
	memset(&state, 0, sizeof(state));
	state.map = map;
	state.resource_maps = resource_maps;
	state.report = report;
	state.tag_cache = tag_cache;
	state.tag_index = NO_TAG_INDEX;
	report->problem_tag_index = NO_TAG_INDEX;

	status = cache_file_identify(map, identity);
	if (status == _cache_file_status_ok && identity->format != _cache_file_format_custom_edition_cache)
	{
		status = _cache_file_status_unsupported_version;
	}
	if (status != _cache_file_status_ok)
	{
		return load_fail(&state, status, 0);
	}
	report->tag_cache_bytes = custom_edition_tag_cache_bytes(identity);
	if (tag_cache_bytes < report->tag_cache_bytes)
	{
		return load_fail(&state, _cache_file_status_out_of_memory, tag_cache_bytes);
	}
	state.file_length = identity->file_length;

	/* the tag data, at the start of the tag cache */
	if (!map->read(map->context, identity->tag_data_offset, identity->tag_data_size, tag_cache))
	{
		return load_fail(&state, _cache_file_status_read_failed, identity->tag_data_offset);
	}
	state.used_bytes = identity->tag_data_size;
	report->tag_data_bytes = identity->tag_data_size;

	/* the tag index */
	tag_index = tag_cache;
	if (read_u32(tag_index + TAG_INDEX_SIGNATURE_OFFSET) != TAG_INDEX_SIGNATURE)
	{
		return load_fail(&state, _cache_file_status_bad_tag_index_signature, identity->tag_data_offset);
	}
	tag_count = read_s32(tag_index + TAG_INDEX_COUNT_OFFSET);
	report->tag_count = tag_count;
	/* the game reads the instances as structures, which must not overlap
	the index or sit at an unaligned address */
	if (tag_count <= 0 ||
		(uint32_t)tag_count > ABSOLUTE_INDEX_MASK + 1 ||
		!tag_cache_offset(
			&state,
			read_u32(tag_index + TAG_INDEX_INSTANCES_OFFSET),
			(uint32_t)tag_count * TAG_INSTANCE_BYTES,
			&instances_offset) ||
		instances_offset < TAG_INDEX_BYTES ||
		(instances_offset & 3))
	{
		return load_fail(&state, _cache_file_status_bad_tag_instances_range, read_u32(tag_index + TAG_INDEX_INSTANCES_OFFSET));
	}
	tag_instances = tag_cache + instances_offset;

	/* the model vertex and index data (in the file; the index data offset
	counts from the vertex data) */
	vertex_data_offset = read_u32(tag_index + TAG_INDEX_VERTEX_DATA_OFFSET_OFFSET);
	model_data_size = read_u32(tag_index + TAG_INDEX_MODEL_DATA_SIZE_OFFSET);
	if (vertex_data_offset < CACHE_FILE_HEADER_BYTES ||
		!range_fits(vertex_data_offset, model_data_size, identity->file_length) ||
		read_u32(tag_index + TAG_INDEX_INDEX_DATA_OFFSET_OFFSET) > model_data_size)
	{
		return load_fail(&state, _cache_file_status_bad_model_data_range, vertex_data_offset);
	}

	/* every tag instance, and the tags held by resource maps */
	for (tag_index_value = 0; tag_index_value < tag_count; tag_index_value++)
	{
		uint8_t const *instance = tag_instances + (uint32_t)tag_index_value * TAG_INSTANCE_BYTES;
		uint32_t group_tag = read_u32(instance + TAG_INSTANCE_GROUP_OFFSET);
		uint32_t handle = read_u32(instance + TAG_INSTANCE_HANDLE_OFFSET);
		uint32_t address = read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET);
		uint32_t offset;

		state.tag_index = tag_index_value;
		if ((handle & ABSOLUTE_INDEX_MASK) != (uint32_t)tag_index_value)
		{
			return load_fail(&state, _cache_file_status_bad_tag_handle, handle);
		}
		if (!tag_name_get(&state, instance))
		{
			return load_fail(&state, _cache_file_status_bad_tag_name, read_u32(instance + TAG_INSTANCE_NAME_OFFSET));
		}
		if (group_tag == PROJECT_YELLOW_GROUP_TAG || group_tag == PROJECT_YELLOW_GLOBALS_GROUP_TAG)
		{
			report->warnings |= 1UL << _custom_edition_warning_opensauce_tags_bit;
		}
		if (read_u32(instance + TAG_INSTANCE_IN_RESOURCE_MAP_OFFSET))
		{
			/* only these groups are ever held by resource maps
			(OpenSauce cache_files.cpp, cache_file_data_load) */
			if (!group_layout(group_tag))
			{
				return load_fail(&state, _cache_file_status_unexpected_external_tag, group_tag);
			}
			if (group_tag == SOUND_GROUP_TAG &&
				!tag_cache_offset(&state, address, SOUND_DEFINITION_BYTES, &offset))
			{
				return load_fail(&state, _cache_file_status_bad_tag_address, address);
			}
		}
		else if (address ?
			!tag_cache_offset(&state, address, 1, &offset) :
			group_tag != STRUCTURE_BSP_GROUP_TAG)
		{
			/* structure BSPs alone have no address until they are loaded */
			return load_fail(&state, _cache_file_status_bad_tag_address, address);
		}
	}
	state.tag_index = NO_TAG_INDEX;

	/* the scenario and its structure BSPs */
	scenario_handle = read_u32(tag_index + TAG_INDEX_SCENARIO_OFFSET);
	report->scenario_tag_index = (int32_t)(scenario_handle & ABSOLUTE_INDEX_MASK);
	if ((scenario_handle & ABSOLUTE_INDEX_MASK) >= (uint32_t)tag_count ||
		read_u32(tag_instances + (scenario_handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES + TAG_INSTANCE_HANDLE_OFFSET) != scenario_handle ||
		read_u32(tag_instances + (scenario_handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES + TAG_INSTANCE_GROUP_OFFSET) != SCENARIO_GROUP_TAG ||
		read_u32(tag_instances + (scenario_handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES + TAG_INSTANCE_IN_RESOURCE_MAP_OFFSET) ||
		!tag_cache_offset(
			&state,
			read_u32(tag_instances + (scenario_handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES + TAG_INSTANCE_ADDRESS_OFFSET),
			SCENARIO_BYTES,
			&scenario_offset))
	{
		return load_fail(&state, _cache_file_status_bad_scenario_tag, scenario_handle);
	}
	status = structure_bsps_verify(&state, tag_instances, tag_count, scenario_offset, &checksum);
	if (status != _cache_file_status_ok)
	{
		return status;
	}

	/* the header checksum covers the structure BSPs, the model data and the
	tag data as the map holds it, before anything below changes it */
	status = crc32_update_from_file(map, &checksum, vertex_data_offset, model_data_size);
	if (status != _cache_file_status_ok)
	{
		return load_fail(&state, status, vertex_data_offset);
	}
	checksum = crc32_update(checksum, tag_cache, identity->tag_data_size);
	report->computed_checksum = checksum;
	if (checksum != identity->checksum)
	{
		report->warnings |= 1UL << _custom_edition_warning_checksum_mismatch_bit;
	}

	/* the tags held by resource maps, placed after the tag data */
	for (tag_index_value = 0; tag_index_value < tag_count; tag_index_value++)
	{
		uint8_t *instance = tag_instances + (uint32_t)tag_index_value * TAG_INSTANCE_BYTES;
		uint32_t group_tag = read_u32(instance + TAG_INSTANCE_GROUP_OFFSET);
		char const *name;

		if (!read_u32(instance + TAG_INSTANCE_IN_RESOURCE_MAP_OFFSET))
		{
			continue;
		}
		state.tag_index = tag_index_value;
		name = tag_name_get(&state, instance);
		if (!name)
		{
			return load_fail(&state, _cache_file_status_bad_tag_name, read_u32(instance + TAG_INSTANCE_NAME_OFFSET));
		}
		switch (group_tag)
		{
		case BITMAP_GROUP_TAG:
			status = resource_tag_load(&state, instance, name, _resource_map_bitmaps, &bitmap_group_layout);
			break;
		case SOUND_GROUP_TAG:
			status = resource_sound_load(&state, instance, name);
			break;
		case FONT_GROUP_TAG:
			status = resource_tag_load(&state, instance, name, _resource_map_locale, &font_layout);
			break;
		case UNICODE_STRING_LIST_GROUP_TAG:
			status = resource_tag_load(&state, instance, name, _resource_map_locale, &unicode_string_list_layout);
			break;
		default:
			status = resource_tag_load(&state, instance, name, _resource_map_locale, &hud_message_text_layout);
			break;
		}
		if (status != _cache_file_status_ok)
		{
			return status;
		}
	}

	/* every tag whose layout this module knows, wherever it came from: each
	address must now lie within the loaded tags, and each bitmap's pixels
	and sound's samples within their file */
	for (tag_index_value = 0; tag_index_value < tag_count; tag_index_value++)
	{
		uint8_t const *instance = tag_instances + (uint32_t)tag_index_value * TAG_INSTANCE_BYTES;
		struct element_layout const *layout = group_layout(read_u32(instance + TAG_INSTANCE_GROUP_OFFSET));
		uint32_t offset;

		if (!layout)
		{
			continue;
		}
		state.tag_index = tag_index_value;
		if (!tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), layout->bytes, &offset))
		{
			return load_fail(&state, _cache_file_status_bad_tag_address, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET));
		}
		status = element_verify(&state, offset, layout, 0);
		if (status != _cache_file_status_ok)
		{
			return status;
		}
	}
	state.tag_index = NO_TAG_INDEX;

	/* anything after the cache data and the OpenSauce definitions */
	data_end = identity->file_length;
	if (identity->has_opensauce_header && identity->opensauce.definitions_size)
	{
		data_end = identity->opensauce.definitions_offset + identity->opensauce.definitions_size;
	}
	if (identity->file_size > data_end)
	{
		report->trailing_bytes = identity->file_size - data_end;
		report->warnings |= 1UL << _custom_edition_warning_trailing_data_bit;
	}
	report->status = _cache_file_status_ok;

	return _cache_file_status_ok;
}
