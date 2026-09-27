/*
CACHE_FILE_FORMATS.H

Recognition, validation and loading of the Halo 1 map files that this build
did not ship with: Halo Custom Edition caches (version 609), the OpenSauce
extension of their header (".yelo" maps), and the Custom Edition resource
maps bitmaps.map, sounds.map and loc.map. docs/custom_edition_caches.md
defines what each milestone (recognize, load, run) means, which of them this
module reaches, and the evidence behind every layout used here.

The code is standalone C with fixed-width types: the same file is compiled
into the native game builds and into the host tool
port/tools/cache_file_report.c, which the tests drive
(tools/test_cache_file_formats.py). Map files are untrusted input: every
field is read little-endian from a bounds-checked buffer, never through a
structure laid over file data, and every offset, count and size is checked
against the file or buffer it refers to before it is used.
*/

#ifndef __CACHE_FILE_FORMATS_H
#define __CACHE_FILE_FORMATS_H

/* ---------- headers */

#include <stddef.h>
#include <stdint.h>

/* ---------- constants */

#define CACHE_FILE_HEADER_BYTES 0x800
/* the name, build and mod name fields: at most 31 characters and a NUL */
#define CACHE_FILE_STRING_BYTES 32

#define CACHE_FILE_VERSION_XBOX 5
#define CACHE_FILE_VERSION_CUSTOM_EDITION 609

/* Halo PC and Custom Edition keep tag data at a fixed address, as the Xbox
does at 0x803A6000: 0x40440000, above a 0x440000-byte game state at
0x40000000, with 23 MB of room, or 1.5 times that for caches built with
OpenSauce's memory upgrades (OpenSauce cache_constants.hpp,
blam_memory_upgrades.hpp). Every map examined puts its structure BSPs at the
top of that room. */
#define CUSTOM_EDITION_TAG_CACHE_ADDRESS 0x40440000UL
#define CUSTOM_EDITION_TAG_CACHE_BYTES 0x01700000UL
#define CUSTOM_EDITION_TAG_CACHE_BYTES_UPGRADED 0x02280000UL
#define CUSTOM_EDITION_CACHE_FILE_MAXIMUM_BYTES 0x18000000UL
#define CUSTOM_EDITION_CACHE_FILE_MAXIMUM_BYTES_UPGRADED 0x24000000UL

enum cache_file_format
{
	_cache_file_format_unrecognized,
	/* version 5: the format this build runs, when its build string is
	01.01.14.2342 (cache_file_header_verify decides that, not this module) */
	_cache_file_format_xbox_cache,
	/* a cache header with a version this module does not know */
	_cache_file_format_other_cache,
	_cache_file_format_custom_edition_cache,
	_cache_file_format_resource_map,

	NUMBER_OF_CACHE_FILE_FORMATS
};

/* the values stored in a resource map's first field (OpenSauce
data_file.hpp, data_file_reference_type) */
enum resource_map_type
{
	_resource_map_none,
	_resource_map_bitmaps,
	_resource_map_sounds,
	_resource_map_locale,

	NUMBER_OF_RESOURCE_MAP_TYPES
};

/* the flags of the OpenSauce header (OpenSauce
cache_files_structures_yelo.hpp, s_cache_header_yelo::s_flags) */
enum opensauce_cache_flag
{
	_opensauce_cache_uses_memory_upgrades_bit,
	_opensauce_cache_uses_mod_data_files_bit,
	_opensauce_cache_is_protected_bit,
	_opensauce_cache_uses_game_state_upgrades_bit,
	_opensauce_cache_has_compression_parameters_bit,

	NUMBER_OF_OPENSAUCE_CACHE_FLAGS
};

/* findings that do not stop a load but must be reported */
enum custom_edition_warning
{
	/* the header checksum is not the CRC-32 of the data it covers
	(OpenSauce cache_files_yelo.cpp, CalculateChecksumFromMemoryMap) */
	_custom_edition_warning_checksum_mismatch_bit,
	/* bytes after the end the header declares that nothing refers to */
	_custom_edition_warning_trailing_data_bit,
	/* the map holds OpenSauce's project_yellow ('yelo') or
	project_yellow_globals ('gelo') tags */
	_custom_edition_warning_opensauce_tags_bit,

	NUMBER_OF_CUSTOM_EDITION_WARNINGS
};

enum cache_file_status
{
	_cache_file_status_ok,

	/* reading and memory */
	_cache_file_status_read_failed,
	_cache_file_status_out_of_memory,
	_cache_file_status_file_too_small,
	_cache_file_status_file_too_large,

	/* the cache file header */
	_cache_file_status_bad_signatures,
	_cache_file_status_unsupported_version,
	_cache_file_status_unterminated_string,
	_cache_file_status_bad_file_length,
	_cache_file_status_compressed_cache,
	_cache_file_status_bad_tag_data_range,

	/* the OpenSauce header */
	_cache_file_status_bad_opensauce_header,
	_cache_file_status_unknown_opensauce_flags,
	_cache_file_status_bad_opensauce_definitions_range,

	/* the tag index */
	_cache_file_status_bad_tag_index_signature,
	_cache_file_status_bad_tag_instances_range,
	_cache_file_status_bad_tag_handle,
	_cache_file_status_bad_tag_name,
	_cache_file_status_bad_tag_address,
	_cache_file_status_bad_scenario_tag,
	_cache_file_status_bad_model_data_range,
	_cache_file_status_unexpected_external_tag,
	_cache_file_status_bad_tag_layout,

	/* structure BSPs */
	_cache_file_status_bad_structure_bsp_block,
	_cache_file_status_bad_structure_bsp_range,
	_cache_file_status_bad_structure_bsp_header,
	_cache_file_status_bad_structure_bsp_geometry,

	/* model geometry */
	_cache_file_status_bad_model_part,

	/* conversion */
	_cache_file_status_bad_shader_type,
	_cache_file_status_bad_script_nodes,

	/* resource maps and the tags they hold */
	_cache_file_status_bad_resource_map_header,
	_cache_file_status_wrong_resource_map_type,
	_cache_file_status_bad_resource_item,
	_cache_file_status_missing_resource_map,
	_cache_file_status_missing_resource_item,
	_cache_file_status_bad_resource_layout,
	_cache_file_status_bad_resource_data_range,
	_cache_file_status_tag_cache_overflow,

	NUMBER_OF_CACHE_FILE_STATUSES
};

/* ---------- structures */

/* Reads `size` bytes at `offset` into `buffer`, returning nonzero only when
every byte was read. */
typedef int (*cache_file_read_proc)(
	void *context,
	uint32_t offset,
	uint32_t size,
	void *buffer);

struct cache_file_source
{
	void *context;
	cache_file_read_proc read;
	uint32_t size;
};

/* the OpenSauce header at offset 0x70 of a Custom Edition cache header */
struct opensauce_cache_header
{
	int16_t version;
	uint16_t flags;
	uint8_t project_yellow_version;
	uint8_t project_yellow_globals_version;
	float memory_upgrade_amount;
	/* the zlib-compressed tag definitions of the OpenSauce editing kit,
	appended after the cache data */
	uint32_t definitions_size;
	uint32_t definitions_decompressed_size;
	uint32_t definitions_offset;
	char definitions_build[CACHE_FILE_STRING_BYTES];
	char mod_name[CACHE_FILE_STRING_BYTES];
	int16_t build_stage;
	uint32_t build_revision;
	int64_t build_timestamp;
	char build_string[CACHE_FILE_STRING_BYTES];
	uint8_t tools_version_major;
	uint8_t tools_version_minor;
	uint16_t tools_version_build;
	uint8_t minimum_version_major;
	uint8_t minimum_version_minor;
	uint16_t minimum_version_build;
	/* OpenSauce resource storage: compression parameters, tag symbols,
	string ids, tag string to string id tables */
	uint32_t resource_offsets[4];
};

struct cache_file_identity
{
	enum cache_file_format format;
	uint32_t file_size;

	/* cache files */
	int32_t version;
	uint32_t file_length;
	uint32_t compressed_file_length;
	uint32_t tag_data_offset;
	uint32_t tag_data_size;
	int16_t scenario_type;
	uint32_t checksum;
	char name[CACHE_FILE_STRING_BYTES];
	char build[CACHE_FILE_STRING_BYTES];
	int has_opensauce_header;
	struct opensauce_cache_header opensauce;

	/* resource maps */
	enum resource_map_type resource_map_type;
	int32_t resource_item_count;
};

struct resource_map_item
{
	uint32_t data_offset;
	uint32_t size;
	char const *name;
};

/* an open resource map: its index and names, read once */
struct resource_map
{
	struct cache_file_source *source;
	enum resource_map_type type;
	int32_t item_count;
	struct resource_map_item *items;
	char *names;
};

struct custom_edition_load_report
{
	struct cache_file_identity identity;
	enum cache_file_status status;
	/* where the first fatal problem was found: a tag index or NONE (-1), and
	a file offset or runtime address as the status describes */
	int32_t problem_tag_index;
	uint32_t problem_location;

	uint32_t tag_cache_bytes;
	int32_t tag_count;
	int32_t scenario_tag_index;
	/* the model vertex and strip data, which no tag holds: its offset in the
	file, where its strips start within it, and its size */
	uint32_t model_data_offset;
	uint32_t model_index_data_offset;
	uint32_t model_data_bytes;
	/* bytes of tag data read from the map, then of tags read from resource
	maps and placed after it */
	uint32_t tag_data_bytes;
	uint32_t resource_tag_bytes;
	int32_t resource_tag_counts[NUMBER_OF_RESOURCE_MAP_TYPES];
	int32_t structure_bsp_count;
	uint32_t largest_structure_bsp_bytes;
	uint32_t lowest_structure_bsp_address;
	int32_t structure_bsp_materials_checked;
	int32_t bitmap_data_ranges_checked;
	int32_t sound_sample_ranges_checked;
	int32_t relocated_pointer_count;
	uint32_t computed_checksum;
	uint32_t trailing_bytes;
	uint32_t warnings;
};

/* Where Halo PC keeps what a texture holds in other channels than this
build reads it from (docs/custom_edition_caches.md). The renderer samples a
texture in the order the game names for it (port/linux/src/xbox_textures.c,
halo_custom_edition_texels_channels). */
enum custom_edition_channel_order
{
	/* as this build reads them */
	_custom_edition_channels_xbox,
	/* a model shader's multipurpose masks: the auxiliary (detail) mask,
	self-illumination, specular and color change in red, green, blue and
	alpha, which this build's model shaders read from alpha, green, red and
	blue */
	_custom_edition_channels_multipurpose,
	/* a HUD meter: its shape in color and the order it fills in alpha,
	where this build's meters take the fill order from color and discard
	what has no alpha */
	_custom_edition_channels_hud_meter,

	NUMBER_OF_CUSTOM_EDITION_CHANNEL_ORDERS
};

/* what custom_edition_cache_convert changed */
struct custom_edition_conversion_report
{
	/* shaders whose type this build numbers differently, and transparent
	chicago extended shaders made transparent chicago ones */
	int32_t shaders_retyped;
	int32_t chicago_extended_shaders;
	/* bitmaps given their own tag and the state of a bitmap not yet drawn */
	int32_t bitmaps_prepared;
	/* 1 when the scenario's script syntax nodes, upgraded by OpenSauce, were
	made this build's number */
	int32_t script_nodes_reduced;
	/* animation graph object overlays that named an animation the graph
	does not have, made to name none */
	int32_t animation_overlays_disabled;
	/* sounds in a compression this build cannot decode (Ogg Vorbis), made
	unplayable */
	int32_t sounds_undecodable;
	/* HUD element placements with Halo PC's high resolution scale, whose
	scale was halved */
	int32_t hud_placements_rescaled;
	/* 1 when the multiplayer hint that a key shows the score was made to
	name the Xbox button */
	int32_t score_hint_converted;
	/* the tag of the problem, when there was one, else NONE (-1) */
	int32_t problem_tag_index;
};

/* ---------- prototypes */

char const *cache_file_status_describe(
	enum cache_file_status status);
char const *cache_file_format_describe(
	enum cache_file_format format);
char const *resource_map_type_describe(
	enum resource_map_type type);

/* Classifies a file from its first bytes and checks every header field that
can be checked against the file size alone. */
enum cache_file_status cache_file_identify(
	struct cache_file_source *source,
	struct cache_file_identity *identity);

/* The format of a cache header already in memory (CACHE_FILE_HEADER_BYTES
bytes), from its signatures and version alone, and whether it carries an
OpenSauce header: for callers that hold a header but not its file. */
enum cache_file_format cache_file_header_format(
	void const *header,
	int *has_opensauce_header);

/* The room for tag data a Custom Edition cache needs at
CUSTOM_EDITION_TAG_CACHE_ADDRESS: 23 MB, or 1.5 times that with memory
upgrades. */
uint32_t custom_edition_tag_cache_bytes(
	struct cache_file_identity const *identity);

enum cache_file_status resource_map_open(
	struct cache_file_source *source,
	enum resource_map_type expected_type,
	struct resource_map *map);
void resource_map_close(
	struct resource_map *map);

/* Loads a Custom Edition cache into `tag_cache`, which stands for the
`tag_cache_bytes` bytes at CUSTOM_EDITION_TAG_CACHE_ADDRESS and must be at
least custom_edition_tag_cache_bytes() long. The map's tag data is copied to
the start, the tags kept in resource maps are placed after it with their
addresses resolved, and every structure BSP, bitmap and sound sample range is
checked. `resource_maps` is indexed by resource_map_type; entries may be NULL
when the map needs none of that type. The structure BSPs themselves are not
loaded: the game loads one at a time when it switches to it. Returns the
first fatal problem; the report describes the map either way. */
enum cache_file_status custom_edition_cache_load(
	struct cache_file_source *map,
	struct resource_map *const resource_maps[NUMBER_OF_RESOURCE_MAP_TYPES],
	uint8_t *tag_cache,
	uint32_t tag_cache_bytes,
	struct custom_edition_load_report *report);

/* Gives the tags of a tag cache custom_edition_cache_load filled
(`loaded_bytes` of it in use) this build's layouts and values where only
their bytes need to change: every shader's type as this build numbers them,
transparent chicago extended shaders made transparent chicago shaders,
bitmaps and sound permutations in the state of ones not yet drawn or played
and naming their own tags, sounds this build cannot decode made unplayable,
animation overlays naming animations that do not exist made to name none,
and OpenSauce's upgraded script node array made this build's size when its
nodes fit. Returns the first problem: tags already converted stay
converted. */
enum cache_file_status custom_edition_cache_convert(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	struct custom_edition_conversion_report *report);

/* In a tag cache custom_edition_cache_load filled (`loaded_bytes` of it in
use), the definition of the next tag of group `group_tag` after tag
`*tag_index` (NONE, -1, to start from the first): `*tag_index` becomes its
index, and NULL is returned when no tag after it has a definition of
`definition_bytes` bytes within the tag cache. */
void *custom_edition_cache_tag_next(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t group_tag,
	uint32_t definition_bytes,
	int32_t *tag_index);

/* The definition of the tag `handle` names in a tag cache
custom_edition_cache_load filled, or NULL unless that is a tag of group
`group_tag` whose definition of `definition_bytes` bytes lies within the tag
cache: any value may be asked about. */
void *custom_edition_cache_tag_get(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t handle,
	uint32_t group_tag,
	uint32_t definition_bytes);

/* The element `element_index` of `element_bytes` bytes of the tag block at
`block`, in a tag of a tag cache custom_edition_cache_load filled, or NULL
unless the block's elements lie within the tag cache: any value may be asked
about. */
void *custom_edition_cache_block_element(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	void const *block,
	int32_t element_index,
	uint32_t element_bytes);

/* The path of tag `tag_index` of a tag cache custom_edition_cache_load
filled, for messages. */
char const *custom_edition_cache_tag_name(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	int32_t tag_index);

/* Makes every tag of group `group_tag` a tag of `new_group_tag` (their parent
groups stay as they are), returning how many there were. */
int32_t custom_edition_cache_tags_regroup(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t group_tag,
	uint32_t new_group_tag);

/* In a tag cache custom_edition_cache_load filled (`loaded_bytes` of it in
use), moves the file offsets of bitmap pixels and sound samples kept in
bitmaps.map and sounds.map by `bitmaps_offset` and `sounds_offset`, and
clears the flags that said they were kept there: afterwards every offset
counts in one combined space (the map, then bitmaps.map, then sounds.map)
that a single read function can serve. */
void custom_edition_cache_combine_resource_offsets(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t bitmaps_offset,
	uint32_t sounds_offset);

#endif
