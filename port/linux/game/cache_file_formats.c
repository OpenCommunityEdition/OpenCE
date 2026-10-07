/*
CACHE_FILE_FORMATS.C

Halo Custom Edition caches and Custom Edition resource maps
(cache_file_formats.h; docs/custom_edition_caches.md).

Sources for the layouts, cited per structure below:
- OpenSauce (GPL-3.0, Kornner Studios; the snapshot examined is the
  OpenSauce-master archive of the upstream Mercurial repository, whose newest
  version file names OpenSauce 4.0.0): the cache header, tag index, tag
  instance, resource map ("data file") header and item, the structure BSP
  header and reference, and the bitmap, sound and HUD message tag layouts,
  all with static size assertions.
- BlamLib, in the same archive: the font and unicode string list layouts.
- Reclaimer (GPL-3.0, Gravemind2401; the Reclaimer-master archive): how
  bitmap tags and pixel data are found in bitmaps.map.
No code from either project is used: they are documentation here, and this
repository is CC0. Facts that neither source states were established by
reading the sample Custom Edition maps; the comments below say so where they
are used ("observed", "every map examined"), and
docs/custom_edition_caches.md lists each with its evidence.
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
/* the signature of OpenSauce's own header, at CACHE_HEADER_OPENSAUCE_OFFSET,
where a Custom Edition cache has padding: such a cache is refused */
#define OPENSAUCE_HEADER_SIGNATURE 'yelo'

#define BITMAP_GROUP_TAG 'bitm'
#define SOUND_GROUP_TAG 'snd!'
#define FONT_GROUP_TAG 'font'
#define UNICODE_STRING_LIST_GROUP_TAG 'ustr'
#define HUD_MESSAGE_TEXT_GROUP_TAG 'hmt '
#define SCENARIO_GROUP_TAG 'scnr'
#define STRUCTURE_BSP_GROUP_TAG 'sbsp'
#define GBXMODEL_GROUP_TAG 'mod2'
/* OpenSauce's project_yellow and project_yellow_globals: refused */
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

/* the tag index at the start of the tag data (OpenSauce
cache_files_structures.hpp, s_cache_tag_header and s_cache_tag_instance) */
#define TAG_INDEX_BYTES 0x28
#define TAG_INDEX_INSTANCES_OFFSET 0x00
#define TAG_INDEX_SCENARIO_OFFSET 0x04
#define TAG_INDEX_CHECKSUM_OFFSET 0x08
#define TAG_INDEX_COUNT_OFFSET 0x0C
#define TAG_INDEX_VERTEX_DATA_OFFSET_OFFSET 0x14
#define TAG_INDEX_INDEX_DATA_OFFSET_OFFSET 0x1C
#define TAG_INDEX_MODEL_DATA_SIZE_OFFSET 0x20
#define TAG_INDEX_SIGNATURE_OFFSET 0x24
#define TAG_INSTANCE_BYTES 0x20
#define TAG_INSTANCE_GROUP_OFFSET 0x00
#define TAG_INSTANCE_PARENT_GROUP_OFFSET 0x04
#define NO_GROUP_TAG 0xFFFFFFFFUL
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
/* the structure BSP's lightmaps and their materials (OpenSauce
structure_bsp_definitions.hpp: structure_bsp 0x288, structure_bsp_lightmap
0x20, structure_bsp_material 0x100; this build's structure_bsp_definitions.h
has the same offsets). A material's vertices are uncompressed in Custom
Edition caches (environment vertices of 56 bytes, then lightmap vertices of
20), and it has lightmap vertices exactly when its lightmap has a bitmap
(every map examined). */
#define STRUCTURE_BSP_BYTES 0x288
#define STRUCTURE_BSP_LIGHTMAPS_OFFSET 0x104
#define STRUCTURE_BSP_LIGHTMAP_BYTES 0x20
#define STRUCTURE_BSP_LIGHTMAP_BITMAP_INDEX_OFFSET 0x00
#define STRUCTURE_BSP_LIGHTMAP_MATERIALS_OFFSET 0x14
#define STRUCTURE_BSP_MATERIAL_BYTES 0x100
#define STRUCTURE_BSP_MATERIAL_VERTEX_TYPE_OFFSET 0xB0
#define STRUCTURE_BSP_MATERIAL_VERTEX_COUNT_OFFSET 0xB4
#define STRUCTURE_BSP_MATERIAL_LIGHTMAP_VERTEX_COUNT_OFFSET 0xC8
#define STRUCTURE_BSP_MATERIAL_UNCOMPRESSED_VERTICES_OFFSET 0xD8
#define STRUCTURE_BSP_ENVIRONMENT_VERTEX_BYTES 56
#define STRUCTURE_BSP_LIGHTMAP_VERTEX_BYTES 20
/* the unit vectors of an environment vertex (normal, binormal and tangent,
after its position) and of a lightmap vertex (its incident radiosity) */
#define STRUCTURE_BSP_ENVIRONMENT_VERTEX_VECTORS_OFFSET 12
#define STRUCTURE_BSP_ENVIRONMENT_VERTEX_VECTOR_COUNT 3
#define STRUCTURE_BSP_LIGHTMAP_VERTEX_VECTORS_OFFSET 0
#define STRUCTURE_BSP_LIGHTMAP_VERTEX_VECTOR_COUNT 1
/* This build compresses those vectors, and its compressor asserts that each
component is within [-1, 1] give or take its rounding: this far past 1 is
safely within that. Every vector of the maps examined is within 1.0001. */
#define MAXIMUM_UNIT_VECTOR_COMPONENT 1.005f
/* the vertex and triangle buffer types of this build's
rasterizer_geometry.h */
#define VERTEX_TYPE_ENVIRONMENT_UNCOMPRESSED 0
#define VERTEX_TYPE_MODEL_UNCOMPRESSED 4
#define TRIANGLE_BUFFER_TYPE_PRECOMPILED_STRIP 1
/* strips index vertices with 16 bits */
#define MAXIMUM_VERTICES_PER_BUFFER 0xFFFF

/* gbxmodel (OpenSauce model_definitions.hpp: gbxmodel_definition 0xE8, its
markers 0x40 and their instances 0x20, nodes 0x9C, regions 0x4C,
permutations 0x58 and their markers 0x50, geometries 0x30, parts 0x84 and
shader references 0x20). A part's triangle and vertex buffer fields say
where its strip and its vertices lie in the model data (Reclaimer
Halo1/GbxmodelTag.cs, ReadPCMeshes): the strip, count + 2 16-bit indices, at
its offset from the index data, the vertices, uncompressed and 68 bytes each,
at theirs from the vertex data. */
#define GBXMODEL_BYTES 0xE8
#define GBXMODEL_GEOMETRY_BYTES 0x30
#define GBXMODEL_PART_BYTES 0x84
#define GBXMODEL_PART_TRIANGLE_BUFFER_TYPE_OFFSET 0x44
#define GBXMODEL_PART_STRIP_TRIANGLE_COUNT_OFFSET 0x48
#define GBXMODEL_PART_STRIP_OFFSET_OFFSET 0x4C
#define GBXMODEL_PART_VERTEX_BUFFER_TYPE_OFFSET 0x54
#define GBXMODEL_PART_VERTEX_COUNT_OFFSET 0x58
#define GBXMODEL_PART_VERTEX_OFFSET_OFFSET 0x64
#define GBXMODEL_PART_LOCAL_NODE_COUNT_OFFSET 0x6B
#define GBXMODEL_MAXIMUM_LOCAL_NODES 22
#define GBXMODEL_VERTEX_BYTES 0x44
#define STRIP_INDEX_BYTES 2

/* shaders (OpenSauce shader_definitions.hpp): every shader starts with the
0x28-byte base, whose type is at 0x24. A transparent chicago extended shader
('scex', 0x78 bytes) is a transparent chicago shader ('schi', 0x6C) up to its
maps, which it has twice, for four and for two texture stages, before its
extra flags. */
#define SHADER_BYTES 0x28
#define SHADER_TYPE_OFFSET 0x24
#define TRANSPARENT_CHICAGO_GROUP_TAG 'schi'
#define TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG 'scex'
#define TRANSPARENT_CHICAGO_BYTES 0x6C
#define TRANSPARENT_CHICAGO_EXTENDED_BYTES 0x78
/* a model shader's flags follow the base, detail after reflection first */
#define SHADER_MODEL_GROUP_TAG 'soso'
#define SHADER_MODEL_FLAGS_OFFSET 0x28
#define SHADER_MODEL_DETAIL_AFTER_REFLECTION_BIT 0
#define TRANSPARENT_CHICAGO_EXTRA_LAYERS_OFFSET 0x48
#define TRANSPARENT_CHICAGO_MAPS_OFFSET 0x54
#define TRANSPARENT_CHICAGO_EXTRA_FLAGS_OFFSET 0x60
#define TRANSPARENT_CHICAGO_EXTENDED_TWO_STAGE_MAPS_OFFSET 0x60
#define TRANSPARENT_CHICAGO_EXTENDED_EXTRA_FLAGS_OFFSET 0x6C
#define TRANSPARENT_CHICAGO_MAP_BYTES 0xDC

/* animation graphs (OpenSauce model_animation_definitions.hpp; this
build's model_animation_definitions.h has the same offsets): the object
overlays, whose first field names an animation of the graph or none */
#define ANIMATION_GRAPH_GROUP_TAG 'antr'
#define ANIMATION_GRAPH_BYTES 0x80
#define ANIMATION_GRAPH_OBJECT_OVERLAYS_OFFSET 0x00
#define ANIMATION_GRAPH_ANIMATIONS_OFFSET 0x74
#define ANIMATION_GRAPH_NODES_OFFSET 0x68
#define ANIMATION_GRAPH_NODE_BYTES 0x40
/* an animation: its node count, then its frame info, default data and
frame data, which the game reads where they are. The game poses an
animation's nodes in arrays of 64 (model_animation_definitions.h,
MAXIMUM_NODES_PER_ANIMATION), so a graph or animation of more is refused. */
#define ANIMATION_BYTES 0xB4
#define ANIMATION_NODE_COUNT_OFFSET 0x2C
#define MAXIMUM_NODES_PER_ANIMATION 64
#define GBXMODEL_BYTES 0xE8
#define GBXMODEL_NODES_OFFSET 0xB8
#define GBXMODEL_NODE_BYTES 0x9C
/* a node's next sibling, first child and parent (shorts), in models and
animation graphs alike */
#define NODE_LINKS_OFFSET 0x20
#define MAXIMUM_REPAIRED_NODES 64
#define ANIMATION_GRAPH_OBJECT_OVERLAY_BYTES 0x14
#define ANIMATION_GRAPH_OBJECT_OVERLAY_ANIMATION_INDEX_OFFSET 0x00
#define NO_BLOCK_INDEX (-1)

/* HUD placements (this build's unit_hud_interface_definition.h and
hud_weapon.c; OpenSauce hud_definitions.hpp, s_hud_element, and
weapon_hud_interface_definition.hpp agree): an element's offset, scale and
scaling flags, of which Halo PC added the high resolution scale. Where they
lie in the unit, weapon and grenade HUD interfaces, in the elements of
their blocks and in the HUD globals' messages. */
#define HUD_PLACEMENT_WIDTH_SCALE_OFFSET 0x04
#define HUD_PLACEMENT_HEIGHT_SCALE_OFFSET 0x08
#define HUD_PLACEMENT_SCALING_FLAGS_OFFSET 0x0C
#define HUD_SCALING_USE_HIGH_RESOLUTION_SCALE_BIT 2
/* a static or meter element: its placement, then the bitmap it draws (none:
an element without one, a number or the motion sensor's blips) */
#define HUD_ELEMENT_BITMAP_OFFSET 0x24
#define NO_HUD_BITMAP (-1)
/* Halo PC's bitmap flags that halve the scale of a HUD element drawing the
bitmap, as the element's own high resolution scale does (Invader's
bitmap.json: "half hud scale", "force hud use highres scale") */
#define BITMAP_GROUP_FLAGS_OFFSET 0x06
#define BITMAP_HALF_HUD_SCALE_BIT 4
#define BITMAP_FORCE_HUD_HIGH_RESOLUTION_SCALE_BIT 7
#define UNIT_HUD_INTERFACE_GROUP_TAG 'unhi'
#define UNIT_HUD_INTERFACE_BYTES 0x56C
#define UNIT_HUD_INTERFACE_AUXILIARY_OVERLAYS_OFFSET 0x3A4
#define UNIT_HUD_INTERFACE_AUXILIARY_OVERLAY_BYTES 0x84
#define UNIT_HUD_INTERFACE_AUXILIARY_METERS_OFFSET 0x3CC
#define UNIT_HUD_INTERFACE_AUXILIARY_METER_BYTES 0x144
#define UNIT_HUD_INTERFACE_AUXILIARY_METER_BACKGROUND_OFFSET 0x14
#define UNIT_HUD_INTERFACE_AUXILIARY_METER_METER_OFFSET 0x7C
#define WEAPON_HUD_INTERFACE_GROUP_TAG 'wphi'
#define WEAPON_HUD_INTERFACE_BYTES 0x17C
#define WEAPON_HUD_INTERFACE_STATICS_OFFSET 0x60
#define WEAPON_HUD_INTERFACE_METERS_OFFSET 0x6C
#define WEAPON_HUD_INTERFACE_CROSSHAIRS_OFFSET 0x84
#define WEAPON_HUD_INTERFACE_OVERLAYS_OFFSET 0x90
#define WEAPON_HUD_STATIC_OR_METER_BYTES 0xB4
/* statics, meters and numbers start with a 0x24-byte header */
#define WEAPON_HUD_ELEMENT_PLACEMENT_OFFSET 0x24
/* crosshairs and overlays: a bitmap and a block of items, each item
starting with its placement */
#define WEAPON_HUD_CROSSHAIRS_OR_OVERLAYS_BYTES 0x68
#define WEAPON_HUD_CROSSHAIRS_OR_OVERLAYS_BITMAP_OFFSET 0x24
#define WEAPON_HUD_ITEMS_OFFSET 0x34
#define WEAPON_HUD_CROSSHAIR_ITEM_BYTES 0x6C
#define WEAPON_HUD_OVERLAY_ITEM_BYTES 0x88
#define GRENADE_HUD_INTERFACE_GROUP_TAG 'grhi'
#define GRENADE_HUD_INTERFACE_BYTES 0x1F8
#define GRENADE_HUD_INTERFACE_OVERLAY_BITMAP_OFFSET 0x14C
#define GRENADE_HUD_INTERFACE_OVERLAY_ITEMS_OFFSET 0x15C
#define UNIT_HUD_INTERFACE_BLIPS_OFFSET 0x35C
/* Where a meter element of the Xbox's has its multitexture overlays (a
block, which the Xbox's tags leave empty), Halo PC's meters have a minimum
alpha and padding (Invader's unit_hud_interface.json and
weapon_hud_interface.json): read as a block, its count is the alpha's bits.
The unit HUD's shield and health meters, an auxiliary meter's meter, and a
weapon HUD meter's after its 0x24-byte header. */
#define HUD_METER_OVERLAYS_OFFSET 0x58
#define UNIT_HUD_INTERFACE_SHIELD_METER_OFFSET 0xF4
#define UNIT_HUD_INTERFACE_HEALTH_METER_OFFSET 0x1E4
#define HUD_GLOBALS_GROUP_TAG 'hudg'
#define HUD_GLOBALS_BYTES 0x450
#define HUD_GLOBALS_MESSAGING_PLACEMENT_OFFSET 0x24
/* the HUD digits, named by the first of the globals' interface bitmaps
(Chimera's jason_jones_hacks.cpp): their bitmap, then their metrics in
bytes (character width, screen width, x and y offset, decimal point and
colon width) */
#define GLOBALS_GROUP_TAG 'matg'
#define GLOBALS_INTERFACE_BITMAPS_OFFSET 0x140
#define INTERFACE_BITMAPS_HUD_DIGITS_OFFSET 0xB0
#define HUD_NUMBER_GROUP_TAG 'hud#'
#define HUD_NUMBER_BYTES 0x64
#define HUD_NUMBER_METRICS_OFFSET 0x10
#define HUD_NUMBER_METRIC_COUNT 6

/* the multiplayer messages: string 100 is the hint that the BACK button
shows the score, which this build copies verbatim
(game_engine.c, _game_engine_message_press_back_for_score) and Halo PC
formats with the name of its score key: 'Hold "%s" for score' */
#define MULTIPLAYER_GAME_TEXT_NAME "ui\\multiplayer_game_text"
#define MULTIPLAYER_GAME_TEXT_SCORE_HINT_INDEX 100

/* widget definitions (ui_widget.c's struct ui_widget_definition): their
event handlers and child widgets. Halo PC's widgets run functions past the
Xbox's 102 (ui_widget_event_handler_functions.c), which this build has not */
#define UI_WIDGET_DEFINITION_GROUP_TAG 'DeLa'
#define UI_WIDGET_DEFINITION_BYTES 0x3EC
#define UI_WIDGET_EVENT_HANDLERS_OFFSET 0x54
#define UI_WIDGET_EVENT_HANDLER_BYTES 0x48
#define UI_WIDGET_EVENT_HANDLER_FLAGS_OFFSET 0x00
#define UI_WIDGET_EVENT_HANDLER_FUNCTION_OFFSET 0x06
#define UI_WIDGET_EVENT_HANDLER_RUN_FUNCTION_FLAG 0x80
#define UI_WIDGET_CHILD_WIDGETS_OFFSET 0x3E0
#define UI_WIDGET_CHILD_BYTES 0x50
#define UI_WIDGET_CHILD_VERTICAL_OFFSET 0x36
#define XBOX_WIDGET_FUNCTION_COUNT 102
/* the multiplayer pause menu's list, and the Xbox's two items of it */
#define MULTIPLAYER_PAUSE_LIST_NAME "ui\\shell\\multiplayer_game\\pause_game\\mp_pause_list"
#define MULTIPLAYER_PAUSE_RESUME_NAME "resume_game_button"
#define MULTIPLAYER_PAUSE_QUIT_NAME "quit_netgame_button"
#define UNICODE_STRING_LIST_BYTES 0x0C
#define UNICODE_STRING_LIST_STRINGS_OFFSET 0x00
#define SCORE_KEY_PLACEHOLDER_CHARACTERS 4

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
#define BITMAP_GROUP_BITMAPS_OFFSET 0x60
/* the group's processed pixel data: a tag data field, whose file offset this
build adds to each bitmap's (texture_cache_bitmap_new), where Halo PC's
bitmaps' offsets are their own (timberland's waterfall has one) */
#define BITMAP_GROUP_PIXEL_DATA_OFFSET 0x30
#define BITMAP_DATA_BYTES 0x30
#define BITMAP_DATA_WIDTH_OFFSET 0x04
#define BITMAP_DATA_HEIGHT_OFFSET 0x06
#define BITMAP_DATA_TYPE_OFFSET 0x0A
#define BITMAP_DATA_FORMAT_OFFSET 0x0C
#define BITMAP_DATA_FLAGS_OFFSET 0x0E
#define BITMAP_TYPE_2D 0
/* the first of the formats a linear bitmap cannot have: DXT1, 3 and 5, and P8 */
#define BITMAP_FORMAT_DXT1 14
#define BITMAP_DATA_POWER_OF_TWO_DIMENSIONS_BIT 0
#define BITMAP_DATA_LINEAR_BIT 4
#define BITMAP_DATA_PIXELS_OFFSET_OFFSET 0x18
#define BITMAP_DATA_PIXELS_SIZE_OFFSET 0x1C
/* the fields the game fills while it draws a bitmap (bitmap_group.h): the
tag the bitmap belongs to, its texture cache block, and its hardware
texture and pixels once they are loaded */
#define BITMAP_DATA_TAG_INDEX_OFFSET 0x20
#define BITMAP_DATA_CACHE_BLOCK_INDEX_OFFSET 0x24
#define BITMAP_DATA_HARDWARE_FORMAT_OFFSET 0x28
#define BITMAP_DATA_BASE_ADDRESS_OFFSET 0x2C
/* s_bitmap_data::_flags::in_data_file: the pixels are in bitmaps.map */
#define BITMAP_DATA_IN_RESOURCE_MAP_BIT 8

/* sound (OpenSauce sound_definitions.hpp: sound_definition 0xA4,
s_sound_pitch_range 0x48, s_sound_permutation 0x7C) */
#define SOUND_DEFINITION_BYTES 0xA4
#define SOUND_PITCH_RANGES_OFFSET 0x98
#define SOUND_PITCH_RANGE_BYTES 0x48
#define SOUND_PITCH_RANGE_PERMUTATIONS_OFFSET 0x3C
#define SOUND_PERMUTATION_BYTES 0x7C
#define SOUND_PERMUTATION_SAMPLES_OFFSET 0x40
/* s_sound_permutation::_samples_in_data_file_bit */
#define SOUND_SAMPLES_IN_RESOURCE_MAP_BIT 0
/* The fields of a sound held by sounds.map that the map's copy of its
header leaves zero (every such sound of the maps examined), and that the
entry's copy holds as the Xbox maps of build 2276 do for the same sounds
(this build's sound_definitions.h names them) */
#define SOUND_SAMPLE_RATE_OFFSET 0x06
#define SOUND_ENCODING_OFFSET 0x6C
#define SOUND_COMPRESSION_OFFSET 0x6E
#define SOUND_LONGEST_PERMUTATION_LENGTH_OFFSET 0x84
/* a permutation's compression, and the fields the game fills while it plays
it (xbox_sound_cache.c: its cache block and base address, and the tag it
names in reads and messages) */
#define SOUND_PERMUTATION_COMPRESSION_OFFSET 0x28
#define SOUND_PERMUTATION_CACHE_BLOCK_INDEX_OFFSET 0x2C
#define SOUND_PERMUTATION_CACHE_BASE_ADDRESS_OFFSET 0x30
#define SOUND_PERMUTATION_CACHE_TAG_INDEX_OFFSET 0x34
#define SOUND_PERMUTATION_RUNTIME_TAG_INDEX_OFFSET 0x3C
/* the compressions this build plays or refuses cleanly (sound_manager.c:
none and Xbox ADPCM; Custom Edition also has Ogg Vorbis, 3) */
#define SOUND_COMPRESSION_NONE 0
#define SOUND_COMPRESSION_XBOX_ADPCM 1

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

struct shader_group_type
{
	uint32_t group_tag;
	int16_t custom_edition_type;
	int16_t type;
};

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
	/* the map's own tag data (the first used bytes), and where its tag
	instances end within it: a tag the map keeps a header of (a resource
	map's sound) must keep it in the tag data after the instances, so that
	filling it in writes over neither the instances nor a tag placed from a
	resource map */
	uint32_t tag_data_bytes;
	uint32_t instances_end;
	uint32_t file_length;
	/* the model data in the file: vertices, then from `index_data_offset`
	the strips */
	uint32_t model_data_offset;
	uint32_t model_index_data_offset;
	uint32_t model_data_size;
	int32_t tag_index;
};

/* ---------- prototypes */

static enum cache_file_status bitmap_data_check(
	struct load_state *state,
	uint32_t element_offset);
static enum cache_file_status sound_permutation_check(
	struct load_state *state,
	uint32_t element_offset);
static enum cache_file_status gbxmodel_part_check(
	struct load_state *state,
	uint32_t element_offset);
static enum cache_file_status animation_graph_check(
	struct load_state *state,
	uint32_t element_offset);
static enum cache_file_status animation_check(
	struct load_state *state,
	uint32_t element_offset);

/* ---------- globals */

static struct element_layout const plain_element_layout = { 0, NULL, 0, NULL, 0, NULL };

/* the placements the unit and grenade HUD interfaces hold themselves */
/* the unit and grenade HUD interfaces' elements that draw a bitmap (their
blips and numbers do not: UNIT_HUD_INTERFACE_BLIPS_OFFSET, and the
grenade count's numbers at 0xF4) */
static uint32_t const unit_hud_interface_elements[] =
{
	/* background, shield background and meter, health background and
	meter, motion sensor background and foreground */
	0x24, 0x8C, 0xF4, 0x17C, 0x1E4, 0x26C, 0x2D4,
};
static uint32_t const grenade_hud_interface_elements[] =
{
	/* background, grenade count background */
	0x24, 0x8C,
};

/* the score key's name in Halo PC's hint, "%s" in quotes, and the Xbox
button that takes its place, in UTF-16 */
static uint16_t const score_key_placeholder[SCORE_KEY_PLACEHOLDER_CHARACTERS] = { '"', '%', 's', '"' };
static uint16_t const score_button_name[SCORE_KEY_PLACEHOLDER_CHARACTERS] = { 'B', 'A', 'C', 'K' };

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
	{ BITMAP_GROUP_BITMAPS_OFFSET, BITMAP_DATA_BYTES, &bitmap_data_layout },
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
	{ SOUND_PITCH_RANGE_PERMUTATIONS_OFFSET, SOUND_PERMUTATION_BYTES, &sound_permutation_layout },
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

static struct block_layout const model_marker_blocks[] =
{
	/* instances */
	{ 0x34, 0x20, &plain_element_layout },
};
static struct element_layout const model_marker_layout =
{
	0x40, model_marker_blocks, 1, NULL, 0, NULL
};
static struct block_layout const model_permutation_blocks[] =
{
	/* markers */
	{ 0x4C, 0x50, &plain_element_layout },
};
static struct element_layout const model_permutation_layout =
{
	0x58, model_permutation_blocks, 1, NULL, 0, NULL
};
static struct block_layout const model_region_blocks[] =
{
	/* permutations */
	{ 0x40, 0x58, &model_permutation_layout },
};
static struct element_layout const model_region_layout =
{
	0x4C, model_region_blocks, 1, NULL, 0, NULL
};
static struct block_layout const gbxmodel_part_blocks[] =
{
	/* uncompressed vertices, compressed vertices, triangles: empty in every
	map examined, the geometry being in the model data */
	{ 0x20, GBXMODEL_VERTEX_BYTES, &plain_element_layout },
	{ 0x2C, 0x20, &plain_element_layout },
	{ 0x38, 3 * STRIP_INDEX_BYTES, &plain_element_layout },
};
static struct element_layout const gbxmodel_part_layout =
{
	GBXMODEL_PART_BYTES, gbxmodel_part_blocks, 3, NULL, 0, gbxmodel_part_check
};
static struct block_layout const gbxmodel_geometry_blocks[] =
{
	/* parts */
	{ 0x24, GBXMODEL_PART_BYTES, &gbxmodel_part_layout },
};
static struct element_layout const gbxmodel_geometry_layout =
{
	GBXMODEL_GEOMETRY_BYTES, gbxmodel_geometry_blocks, 1, NULL, 0, NULL
};
static struct block_layout const gbxmodel_blocks[] =
{
	/* markers, nodes, regions, geometries, shaders */
	{ 0xAC, 0x40, &model_marker_layout },
	{ 0xB8, 0x9C, &plain_element_layout },
	{ 0xC4, 0x4C, &model_region_layout },
	{ 0xD0, GBXMODEL_GEOMETRY_BYTES, &gbxmodel_geometry_layout },
	{ 0xDC, 0x20, &plain_element_layout },
};
static struct element_layout const gbxmodel_layout =
{
	GBXMODEL_BYTES, gbxmodel_blocks, 5, NULL, 0, NULL
};

static uint32_t const animation_data[] =
{
	/* frame info, default data, frame data */
	0x48, 0x8C, 0xA0,
};
static struct element_layout const animation_layout =
{
	ANIMATION_BYTES, NULL, 0, animation_data, 3, animation_check
};
static struct block_layout const animation_graph_blocks[] =
{
	/* nodes, animations */
	{ ANIMATION_GRAPH_NODES_OFFSET, ANIMATION_GRAPH_NODE_BYTES, &plain_element_layout },
	{ ANIMATION_GRAPH_ANIMATIONS_OFFSET, ANIMATION_BYTES, &animation_layout },
};
static struct element_layout const animation_graph_layout =
{
	ANIMATION_GRAPH_BYTES, animation_graph_blocks, 2, NULL, 0, animation_graph_check
};

static struct block_layout const transparent_chicago_blocks[] =
{
	/* extra layers (shader references), maps */
	{ TRANSPARENT_CHICAGO_EXTRA_LAYERS_OFFSET, TAG_REFERENCE_BYTES, &plain_element_layout },
	{ TRANSPARENT_CHICAGO_MAPS_OFFSET, TRANSPARENT_CHICAGO_MAP_BYTES, &plain_element_layout },
};
static struct element_layout const transparent_chicago_layout =
{
	TRANSPARENT_CHICAGO_BYTES, transparent_chicago_blocks, 2, NULL, 0, NULL
};
static struct block_layout const transparent_chicago_extended_blocks[] =
{
	/* extra layers, four-stage maps, two-stage maps */
	{ TRANSPARENT_CHICAGO_EXTRA_LAYERS_OFFSET, TAG_REFERENCE_BYTES, &plain_element_layout },
	{ TRANSPARENT_CHICAGO_MAPS_OFFSET, TRANSPARENT_CHICAGO_MAP_BYTES, &plain_element_layout },
	{ TRANSPARENT_CHICAGO_EXTENDED_TWO_STAGE_MAPS_OFFSET, TRANSPARENT_CHICAGO_MAP_BYTES, &plain_element_layout },
};
static struct element_layout const transparent_chicago_extended_layout =
{
	TRANSPARENT_CHICAGO_EXTENDED_BYTES, transparent_chicago_extended_blocks, 3, NULL, 0, NULL
};

/* The value of the type field each shader group has in Custom Edition, which
inserted transparent chicago extended at 7 (OpenSauce
shader_definitions.hpp, e_shader_type), and in this build (shaders.c); a
chicago extended shader becomes a chicago shader. The Xbox maps of build 2276
have this build's values for the groups they hold. */
static struct shader_group_type const shader_group_types[] =
{
	{ 'senv', 3, 3 },
	{ 'soso', 4, 4 },
	{ 'sotr', 5, 5 },
	{ TRANSPARENT_CHICAGO_GROUP_TAG, 6, 6 },
	{ TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG, 7, 6 },
	{ 'swat', 8, 7 },
	{ 'sgla', 9, 8 },
	{ 'smet', 10, 9 },
	{ 'spla', 11, 10 },
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
	"an OpenSauce map, which this build does not run",
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
	"a structure BSP's lightmaps or their materials do not have the documented layout",
	"a model part's strip or vertices lie outside the model data or are not of the kind Custom Edition writes",
	"an animation graph or one of its animations has more nodes than this build can pose",
	"a shader's type is not the one Custom Edition gives its group",
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

static void write_u16(
	uint8_t *bytes,
	uint16_t value)
{
	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);

	return;
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

static void write_f32(
	uint8_t *bytes,
	float value)
{
	uint32_t bits;

	memcpy(&bits, &value, sizeof(bits));
	write_u32(bytes, bits);

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

static enum cache_file_status custom_edition_header_verify(
	uint8_t const *bytes,
	struct cache_file_identity *identity)
{
	if (read_u32(bytes + CACHE_HEADER_OPENSAUCE_OFFSET) == OPENSAUCE_HEADER_SIGNATURE)
	{
		return _cache_file_status_opensauce_cache;
	}
	if (identity->compressed_file_length)
	{
		return _cache_file_status_compressed_cache;
	}
	/* Invader leaves the file length 0 (blood_covenantv3), which Halo PC
	never reads: the cache is the whole file */
	if (!identity->file_length)
	{
		identity->file_length = identity->file_size;
	}
	if (identity->file_length < CACHE_FILE_HEADER_BYTES ||
		identity->file_length > CUSTOM_EDITION_CACHE_FILE_MAXIMUM_BYTES ||
		identity->file_length > identity->file_size)
	{
		return _cache_file_status_bad_file_length;
	}
	if (identity->tag_data_offset < CACHE_FILE_HEADER_BYTES ||
		identity->tag_data_size < TAG_INDEX_BYTES ||
		identity->tag_data_size > CUSTOM_EDITION_TAG_CACHE_BYTES ||
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

/* A gbxmodel part's strip and vertices: of the kinds Custom Edition writes,
within the index and vertex parts of the model data, aligned for reading as
16-bit indices and floats, and few enough vertices for 16-bit indices. What
the strip and the vertices hold is checked when they are converted
(custom_edition_cache.h). */
static enum cache_file_status gbxmodel_part_check(
	struct load_state *state,
	uint32_t element_offset)
{
	uint8_t const *part = state->tag_cache + element_offset;
	int32_t strip_triangle_count = read_s32(part + GBXMODEL_PART_STRIP_TRIANGLE_COUNT_OFFSET);
	uint32_t strip_offset = read_u32(part + GBXMODEL_PART_STRIP_OFFSET_OFFSET);
	int32_t vertex_count = read_s32(part + GBXMODEL_PART_VERTEX_COUNT_OFFSET);
	uint32_t vertex_offset = read_u32(part + GBXMODEL_PART_VERTEX_OFFSET_OFFSET);
	uint32_t index_data_bytes = state->model_data_size - state->model_index_data_offset;

	if (read_s16(part + GBXMODEL_PART_TRIANGLE_BUFFER_TYPE_OFFSET) != TRIANGLE_BUFFER_TYPE_PRECOMPILED_STRIP ||
		read_s16(part + GBXMODEL_PART_VERTEX_BUFFER_TYPE_OFFSET) != VERTEX_TYPE_MODEL_UNCOMPRESSED ||
		part[GBXMODEL_PART_LOCAL_NODE_COUNT_OFFSET] > GBXMODEL_MAXIMUM_LOCAL_NODES ||
		strip_triangle_count < 1 ||
		(uint32_t)strip_triangle_count + 2 > index_data_bytes / STRIP_INDEX_BYTES ||
		!range_fits(strip_offset, ((uint32_t)strip_triangle_count + 2) * STRIP_INDEX_BYTES, index_data_bytes) ||
		((state->model_index_data_offset + strip_offset) & (STRIP_INDEX_BYTES - 1)) ||
		vertex_count < 1 ||
		vertex_count > MAXIMUM_VERTICES_PER_BUFFER ||
		!range_fits(vertex_offset, (uint32_t)vertex_count * GBXMODEL_VERTEX_BYTES, state->model_index_data_offset) ||
		(vertex_offset & 3))
	{
		return load_fail(state, _cache_file_status_bad_model_part, element_offset);
	}

	return _cache_file_status_ok;
}

static enum cache_file_status animation_graph_check(
	struct load_state *state,
	uint32_t element_offset)
{
	uint8_t const *nodes = state->tag_cache + element_offset + ANIMATION_GRAPH_NODES_OFFSET;

	if (read_s32(nodes + TAG_BLOCK_COUNT_OFFSET) > MAXIMUM_NODES_PER_ANIMATION)
	{
		return load_fail(state, _cache_file_status_bad_animation_nodes, element_offset);
	}

	return _cache_file_status_ok;
}

static enum cache_file_status animation_check(
	struct load_state *state,
	uint32_t element_offset)
{
	int16_t node_count = read_s16(state->tag_cache + element_offset + ANIMATION_NODE_COUNT_OFFSET);

	if (node_count < 0 || node_count > MAXIMUM_NODES_PER_ANIMATION)
	{
		return load_fail(state, _cache_file_status_bad_animation_nodes, element_offset);
	}

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
own data_file_read instead.) The entry's copy of the header is used for the
fields the map's copy leaves zero, its sample rate, encoding, compression
and longest permutation length, which it holds as the Xbox maps of build
2276 do. Its pointers and tag references belong to the editing kit and to
the map that sounds.map was built with (docs/custom_edition_caches.md). */
static enum cache_file_status resource_sound_load(
	struct load_state *state,
	uint8_t *instance,
	char const *name)
{
	struct resource_map const *map = state->resource_maps[_resource_map_sounds];
	uint32_t header_address = read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET);
	struct resource_map_item const *item;
	uint8_t entry_header[SOUND_DEFINITION_BYTES];
	uint8_t *header;
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
	/* (in the map's own tag data, after its tag instances: the header is
	written into below) */
	if (!tag_cache_offset(state, header_address, SOUND_DEFINITION_BYTES, &header_offset) ||
		header_offset < state->instances_end ||
		!range_fits(header_offset, SOUND_DEFINITION_BYTES, state->tag_data_bytes))
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
	if (!map->source->read(map->source->context, item->data_offset, SOUND_DEFINITION_BYTES, entry_header) ||
		!map->source->read(
			map->source->context,
			item->data_offset + SOUND_DEFINITION_BYTES,
			data_bytes,
			state->tag_cache + offset))
	{
		return load_fail(state, _cache_file_status_read_failed, item->data_offset);
	}
	header = state->tag_cache + header_offset;
	memcpy(header + SOUND_SAMPLE_RATE_OFFSET, entry_header + SOUND_SAMPLE_RATE_OFFSET, 2);
	memcpy(header + SOUND_ENCODING_OFFSET, entry_header + SOUND_ENCODING_OFFSET, 2);
	memcpy(header + SOUND_COMPRESSION_OFFSET, entry_header + SOUND_COMPRESSION_OFFSET, 2);
	memcpy(header + SOUND_LONGEST_PERMUTATION_LENGTH_OFFSET, entry_header + SOUND_LONGEST_PERMUTATION_LENGTH_OFFSET, 4);
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

/* the layout of a group whose tags resource maps may hold, or NULL: only
these groups ever are (OpenSauce cache_files.cpp, cache_file_data_load) */
static struct element_layout const *resource_group_layout(
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

/* the layout of a group whose tags are checked after loading, or NULL: the
groups resource maps hold, and those converted for this build */
static struct element_layout const *checked_group_layout(
	uint32_t group_tag)
{
	switch (group_tag)
	{
	case GBXMODEL_GROUP_TAG:
		return &gbxmodel_layout;
	case ANIMATION_GRAPH_GROUP_TAG:
		return &animation_graph_layout;
	case TRANSPARENT_CHICAGO_GROUP_TAG:
		return &transparent_chicago_layout;
	case TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG:
		return &transparent_chicago_extended_layout;
	default:
		return resource_group_layout(group_tag);
	}
}

/* The `count` elements of `element_bytes` a tag block points to, in memory
of `size` bytes loaded at `address` (a structure BSP, or the tag cache):
nonzero when they lie within it, aligned for the game to read as
structures, with `*offset` their offset in it. */
static int loaded_block_get(
	uint8_t const *block,
	uint32_t address,
	uint32_t size,
	uint32_t element_bytes,
	int32_t *count,
	uint32_t *offset)
{
	uint32_t block_address = read_u32(block + TAG_BLOCK_ADDRESS_OFFSET);

	*count = read_s32(block + TAG_BLOCK_COUNT_OFFSET);
	*offset = block_address - address;

	return *count == 0 ||
		(*count > 0 &&
		(uint32_t)*count <= size / element_bytes &&
		block_address >= address &&
		range_fits(*offset, (uint32_t)*count * element_bytes, size) &&
		!(*offset & 3));
}

/* nonzero when the `vector_count` vectors at `vectors_offset` of each of
the `vertex_count` vertices of `vertex_bytes` at `vertices` are unit-range */
static int unit_vectors_valid(
	uint8_t const *vertices,
	int32_t vertex_count,
	uint32_t vertex_bytes,
	uint32_t vectors_offset,
	int vector_count)
{
	int32_t vertex_index;

	for (vertex_index = 0; vertex_index < vertex_count; vertex_index++)
	{
		uint8_t const *vectors = vertices + (uint32_t)vertex_index * vertex_bytes + vectors_offset;
		int component_index;

		for (component_index = 0; component_index < 3 * vector_count; component_index++)
		{
			float component = read_f32(vectors + component_index * 4);

			/* a NaN fails both comparisons */
			if (!(component >= -MAXIMUM_UNIT_VECTOR_COMPONENT && component <= MAXIMUM_UNIT_VECTOR_COMPONENT))
			{
				return 0;
			}
		}
	}

	return 1;
}

/* The lightmaps of a structure BSP (`size` bytes read from the file into
`bsp`, which load at `address`) and their materials, whose vertices are
converted for this build when the BSP is loaded (custom_edition_cache.h):
each material's uncompressed vertices within the BSP, of the size its vertex
counts give, and lightmap vertices exactly when its lightmap has a bitmap. */
static enum cache_file_status structure_bsp_geometry_verify(
	struct load_state *state,
	uint8_t const *bsp,
	uint32_t size,
	uint32_t address)
{
	uint32_t structure_offset = read_u32(bsp + STRUCTURE_BSP_HEADER_BSP_OFFSET) - address;
	uint32_t lightmaps_offset;
	int32_t lightmap_count;
	int32_t lightmap_index;

	if (!range_fits(structure_offset, STRUCTURE_BSP_BYTES, size) ||
		!loaded_block_get(
			bsp + structure_offset + STRUCTURE_BSP_LIGHTMAPS_OFFSET,
			address,
			size,
			STRUCTURE_BSP_LIGHTMAP_BYTES,
			&lightmap_count,
			&lightmaps_offset))
	{
		return load_fail(state, _cache_file_status_bad_structure_bsp_geometry, address);
	}
	for (lightmap_index = 0; lightmap_index < lightmap_count; lightmap_index++)
	{
		uint8_t const *lightmap = bsp + lightmaps_offset + (uint32_t)lightmap_index * STRUCTURE_BSP_LIGHTMAP_BYTES;
		int has_bitmap = read_s16(lightmap + STRUCTURE_BSP_LIGHTMAP_BITMAP_INDEX_OFFSET) != -1;
		uint32_t materials_offset;
		int32_t material_count;
		int32_t material_index;

		if (!loaded_block_get(
			lightmap + STRUCTURE_BSP_LIGHTMAP_MATERIALS_OFFSET,
			address,
			size,
			STRUCTURE_BSP_MATERIAL_BYTES,
			&material_count,
			&materials_offset))
		{
			return load_fail(state, _cache_file_status_bad_structure_bsp_geometry, address + lightmaps_offset);
		}
		for (material_index = 0; material_index < material_count; material_index++)
		{
			uint32_t material_offset = materials_offset + (uint32_t)material_index * STRUCTURE_BSP_MATERIAL_BYTES;
			uint8_t const *material = bsp + material_offset;
			uint8_t const *vertices = material + STRUCTURE_BSP_MATERIAL_UNCOMPRESSED_VERTICES_OFFSET;
			int32_t vertex_count = read_s32(material + STRUCTURE_BSP_MATERIAL_VERTEX_COUNT_OFFSET);
			int32_t lightmap_vertex_count = read_s32(material + STRUCTURE_BSP_MATERIAL_LIGHTMAP_VERTEX_COUNT_OFFSET);
			int32_t vertices_size = read_s32(vertices + TAG_DATA_SIZE_OFFSET);
			uint32_t vertices_address = read_u32(vertices + TAG_DATA_ADDRESS_OFFSET);

			/* the counts bound the size, which bounds the counts */
			if (read_s16(material + STRUCTURE_BSP_MATERIAL_VERTEX_TYPE_OFFSET) != VERTEX_TYPE_ENVIRONMENT_UNCOMPRESSED ||
				vertex_count < 0 ||
				vertex_count > MAXIMUM_VERTICES_PER_BUFFER ||
				lightmap_vertex_count != (has_bitmap ? vertex_count : 0) ||
				vertices_size != vertex_count * STRUCTURE_BSP_ENVIRONMENT_VERTEX_BYTES +
					lightmap_vertex_count * STRUCTURE_BSP_LIGHTMAP_VERTEX_BYTES ||
				(vertices_size &&
				(vertices_address < address ||
				!range_fits(vertices_address - address, (uint32_t)vertices_size, size) ||
				(vertices_address & 3))))
			{
				return load_fail(state, _cache_file_status_bad_structure_bsp_geometry, address + material_offset);
			}
			if (vertices_size &&
				(!unit_vectors_valid(
					bsp + (vertices_address - address),
					vertex_count,
					STRUCTURE_BSP_ENVIRONMENT_VERTEX_BYTES,
					STRUCTURE_BSP_ENVIRONMENT_VERTEX_VECTORS_OFFSET,
					STRUCTURE_BSP_ENVIRONMENT_VERTEX_VECTOR_COUNT) ||
				!unit_vectors_valid(
					bsp + (vertices_address - address) + (uint32_t)vertex_count * STRUCTURE_BSP_ENVIRONMENT_VERTEX_BYTES,
					lightmap_vertex_count,
					STRUCTURE_BSP_LIGHTMAP_VERTEX_BYTES,
					STRUCTURE_BSP_LIGHTMAP_VERTEX_VECTORS_OFFSET,
					STRUCTURE_BSP_LIGHTMAP_VERTEX_VECTOR_COUNT)))
			{
				return load_fail(state, _cache_file_status_bad_structure_bsp_geometry, vertices_address);
			}
			state->report->structure_bsp_materials_checked++;
		}
	}

	return _cache_file_status_ok;
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
		uint8_t *bsp;
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
		bsp = malloc((size_t)size);
		if (!bsp)
		{
			return load_fail(state, _cache_file_status_out_of_memory, (uint32_t)size);
		}
		if (!state->map->read(state->map->context, (uint32_t)file_offset, (uint32_t)size, bsp))
		{
			free(bsp);
			return load_fail(state, _cache_file_status_read_failed, (uint32_t)file_offset);
		}
		bsp_pointer = read_u32(bsp + STRUCTURE_BSP_HEADER_BSP_OFFSET);
		/* PC caches have no Xbox vertex buffer arrays */
		if (read_u32(bsp + STRUCTURE_BSP_HEADER_SIGNATURE_OFFSET) != STRUCTURE_BSP_SIGNATURE ||
			read_u32(bsp + STRUCTURE_BSP_HEADER_VERTEX_BUFFERS_OFFSET) ||
			read_u32(bsp + STRUCTURE_BSP_HEADER_LIGHTMAP_VERTEX_BUFFERS_OFFSET) ||
			bsp_pointer < bsp_address + STRUCTURE_BSP_HEADER_BYTES ||
			bsp_pointer - bsp_address >= (uint32_t)size)
		{
			free(bsp);
			return load_fail(state, _cache_file_status_bad_structure_bsp_header, (uint32_t)file_offset);
		}
		status = structure_bsp_geometry_verify(state, bsp, (uint32_t)size, bsp_address);
		free(bsp);
		if (status != _cache_file_status_ok)
		{
			return status;
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
	void const *header)
{
	uint8_t const *bytes = header;
	enum cache_file_format format;

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
	report->tag_cache_bytes = CUSTOM_EDITION_TAG_CACHE_BYTES;
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
	state.tag_data_bytes = identity->tag_data_size;
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
	state.instances_end = instances_offset + (uint32_t)tag_count * TAG_INSTANCE_BYTES;

	/* the model vertex and index data (in the file; the index data offset
	counts from the vertex data) */
	state.model_data_offset = read_u32(tag_index + TAG_INDEX_VERTEX_DATA_OFFSET_OFFSET);
	state.model_index_data_offset = read_u32(tag_index + TAG_INDEX_INDEX_DATA_OFFSET_OFFSET);
	state.model_data_size = read_u32(tag_index + TAG_INDEX_MODEL_DATA_SIZE_OFFSET);
	if (state.model_data_offset < CACHE_FILE_HEADER_BYTES ||
		!range_fits(state.model_data_offset, state.model_data_size, identity->file_length) ||
		state.model_index_data_offset > state.model_data_size)
	{
		return load_fail(&state, _cache_file_status_bad_model_data_range, state.model_data_offset);
	}
	report->model_data_offset = state.model_data_offset;
	report->model_index_data_offset = state.model_index_data_offset;
	report->model_data_bytes = state.model_data_size;

	/* every tag instance, and the tags held by resource maps */
	for (tag_index_value = 0; tag_index_value < tag_count; tag_index_value++)
	{
		uint8_t *instance = tag_instances + (uint32_t)tag_index_value * TAG_INSTANCE_BYTES;
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
			return load_fail(&state, _cache_file_status_opensauce_cache, group_tag);
		}
		if (read_u32(instance + TAG_INSTANCE_IN_RESOURCE_MAP_OFFSET))
		{
			if (!resource_group_layout(group_tag))
			{
				return load_fail(&state, _cache_file_status_unexpected_external_tag, group_tag);
			}
			if (group_tag == SOUND_GROUP_TAG &&
				!tag_cache_offset(&state, address, SOUND_DEFINITION_BYTES, &offset))
			{
				return load_fail(&state, _cache_file_status_bad_tag_address, address);
			}
		}
		else if (group_tag == STRUCTURE_BSP_GROUP_TAG)
		{
			/* structure BSPs alone have no address until they are loaded;
			Invader writes the one each loads at (cursed-damnation), which
			is past the tags */
			write_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET, 0);
		}
		else if (!address || !tag_cache_offset(&state, address, 1, &offset))
		{
			return load_fail(&state, _cache_file_status_bad_tag_address, address);
		}
	}
	state.tag_index = NO_TAG_INDEX;

	/* the scenario and its structure BSPs */
	scenario_handle = read_u32(tag_index + TAG_INDEX_SCENARIO_OFFSET);
	report->scenario_tag_index = (int32_t)(scenario_handle & ABSOLUTE_INDEX_MASK);
	/* Map protection can rename the scenario's group (to 'prot', say). Halo
	PC used whatever tag the header named, so give it the scenario's group. */
	if ((scenario_handle & ABSOLUTE_INDEX_MASK) < (uint32_t)tag_count)
	{
		uint8_t *scenario_instance = tag_instances + (scenario_handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES;

		if (read_u32(scenario_instance + TAG_INSTANCE_HANDLE_OFFSET) == scenario_handle &&
			read_u32(scenario_instance + TAG_INSTANCE_GROUP_OFFSET) != SCENARIO_GROUP_TAG)
		{
			write_u32(scenario_instance + TAG_INSTANCE_GROUP_OFFSET, SCENARIO_GROUP_TAG);
			write_u32(scenario_instance + TAG_INSTANCE_PARENT_GROUP_OFFSET, NO_GROUP_TAG);
			write_u32(scenario_instance + TAG_INSTANCE_PARENT_GROUP_OFFSET + 4, NO_GROUP_TAG);
			report->scenario_regrouped = 1;
		}
	}
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
	status = crc32_update_from_file(map, &checksum, state.model_data_offset, state.model_data_size);
	if (status != _cache_file_status_ok)
	{
		return load_fail(&state, status, state.model_data_offset);
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
	address must now lie within the loaded tags, each bitmap's pixels and
	sound's samples within their file, and each model part's geometry
	within the model data */
	for (tag_index_value = 0; tag_index_value < tag_count; tag_index_value++)
	{
		uint8_t const *instance = tag_instances + (uint32_t)tag_index_value * TAG_INSTANCE_BYTES;
		struct element_layout const *layout = checked_group_layout(read_u32(instance + TAG_INSTANCE_GROUP_OFFSET));
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

	/* anything after the cache data */
	data_end = identity->file_length;
	if (identity->file_size > data_end)
	{
		report->trailing_bytes = identity->file_size - data_end;
		report->warnings |= 1UL << _custom_edition_warning_trailing_data_bit;
	}
	report->status = _cache_file_status_ok;

	return _cache_file_status_ok;
}

/* The tag instances of a tag cache custom_edition_cache_load filled (the
state's tag_cache and used_bytes), and their count, or NULL. The walks of the
public functions below take that image as checked, but still check every
block they follow. */
static uint8_t *loaded_tag_instances(
	struct load_state const *state,
	int32_t *tag_count)
{
	uint32_t instances_offset;

	if (state->used_bytes < TAG_INDEX_BYTES)
	{
		return NULL;
	}
	*tag_count = read_s32(state->tag_cache + TAG_INDEX_COUNT_OFFSET);
	if (*tag_count <= 0 ||
		(uint32_t)*tag_count > ABSOLUTE_INDEX_MASK + 1 ||
		!tag_cache_offset(
			state,
			read_u32(state->tag_cache + TAG_INDEX_INSTANCES_OFFSET),
			(uint32_t)*tag_count * TAG_INSTANCE_BYTES,
			&instances_offset))
	{
		return NULL;
	}

	return state->tag_cache + instances_offset;
}

static void loaded_state_initialize(
	struct load_state *state,
	uint8_t *tag_cache,
	uint32_t loaded_bytes)
{
	memset(state, 0, sizeof(*state));
	state->tag_cache = tag_cache;
	state->used_bytes = loaded_bytes;
	state->tag_index = NO_TAG_INDEX;

	return;
}

void *custom_edition_cache_tag_next(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t group_tag,
	uint32_t definition_bytes,
	int32_t *tag_index)
{
	struct load_state state;
	uint8_t const *instances;
	int32_t tag_count;
	int32_t index;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	if (!instances || *tag_index >= tag_count)
	{
		return NULL;
	}
	for (index = *tag_index < 0 ? 0 : *tag_index + 1; index < tag_count; index++)
	{
		uint8_t const *instance = instances + (uint32_t)index * TAG_INSTANCE_BYTES;
		uint32_t offset;

		if (read_u32(instance + TAG_INSTANCE_GROUP_OFFSET) == group_tag &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), definition_bytes, &offset))
		{
			*tag_index = index;
			return tag_cache + offset;
		}
	}
	*tag_index = tag_count;

	return NULL;
}

void *custom_edition_cache_tag_get(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t handle,
	uint32_t group_tag,
	uint32_t definition_bytes)
{
	struct load_state state;
	uint8_t const *instances;
	uint8_t const *instance;
	uint32_t offset;
	int32_t tag_count;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	if (!instances || (handle & ABSOLUTE_INDEX_MASK) >= (uint32_t)tag_count)
	{
		return NULL;
	}
	instance = instances + (handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES;

	return read_u32(instance + TAG_INSTANCE_HANDLE_OFFSET) == handle &&
		read_u32(instance + TAG_INSTANCE_GROUP_OFFSET) == group_tag &&
		tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), definition_bytes, &offset) ?
		tag_cache + offset :
		NULL;
}

void *custom_edition_cache_block_element(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	void const *block,
	int32_t element_index,
	uint32_t element_bytes)
{
	struct load_state state;
	int32_t element_count;
	uint32_t elements_offset;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	if ((uint8_t const *)block < tag_cache ||
		!range_fits((uint32_t)((uint8_t const *)block - tag_cache), TAG_BLOCK_BYTES, loaded_bytes) ||
		element_bytes == 0 ||
		!loaded_block_get(block, CUSTOM_EDITION_TAG_CACHE_ADDRESS, loaded_bytes, element_bytes, &element_count, &elements_offset) ||
		element_index < 0 ||
		element_index >= element_count)
	{
		return NULL;
	}

	return tag_cache + elements_offset + (uint32_t)element_index * element_bytes;
}

void *custom_edition_cache_data_get(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	void const *data,
	uint32_t *size)
{
	struct load_state state;
	uint8_t const *field = data;
	int32_t data_size;
	uint32_t data_offset;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	if (field < tag_cache ||
		!range_fits((uint32_t)(field - tag_cache), TAG_DATA_BYTES, loaded_bytes))
	{
		return NULL;
	}
	data_size = read_s32(field + TAG_DATA_SIZE_OFFSET);
	if (data_size <= 0 ||
		!tag_cache_offset(&state, read_u32(field + TAG_DATA_ADDRESS_OFFSET), (uint32_t)data_size, &data_offset))
	{
		return NULL;
	}
	*size = (uint32_t)data_size;

	return tag_cache + data_offset;
}

char const *custom_edition_cache_tag_name(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	int32_t tag_index)
{
	struct load_state state;
	uint8_t const *instances;
	uint32_t name_offset;
	int32_t tag_count;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	if (!instances ||
		tag_index < 0 ||
		tag_index >= tag_count ||
		!tag_cache_offset(
			&state,
			read_u32(instances + (uint32_t)tag_index * TAG_INSTANCE_BYTES + TAG_INSTANCE_NAME_OFFSET),
			1,
			&name_offset) ||
		!memchr(tag_cache + name_offset, 0, loaded_bytes - name_offset))
	{
		return "<no name>";
	}

	return (char const *)tag_cache + name_offset;
}

int custom_edition_cache_tag_in_resource_map(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	int32_t tag_index)
{
	struct load_state state;
	uint8_t const *instances;
	int32_t tag_count;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	return instances && tag_index >= 0 && tag_index < tag_count &&
		read_u32(instances + (uint32_t)tag_index * TAG_INSTANCE_BYTES + TAG_INSTANCE_IN_RESOURCE_MAP_OFFSET) != 0;
}

int32_t custom_edition_cache_tags_regroup(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t group_tag,
	uint32_t new_group_tag)
{
	struct load_state state;
	uint8_t *instances;
	int32_t tag_count;
	int32_t tag_index;
	int32_t regrouped_count = 0;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	if (!instances)
	{
		return 0;
	}
	for (tag_index = 0; tag_index < tag_count; tag_index++)
	{
		uint8_t *instance = instances + (uint32_t)tag_index * TAG_INSTANCE_BYTES;

		if (read_u32(instance + TAG_INSTANCE_GROUP_OFFSET) == group_tag)
		{
			write_u32(instance + TAG_INSTANCE_GROUP_OFFSET, new_group_tag);
			regrouped_count++;
		}
	}

	return regrouped_count;
}

static struct shader_group_type const *shader_group_type_get(
	uint32_t group_tag)
{
	size_t index;

	for (index = 0; index < sizeof(shader_group_types) / sizeof(shader_group_types[0]); index++)
	{
		if (shader_group_types[index].group_tag == group_tag)
		{
			return &shader_group_types[index];
		}
	}

	return NULL;
}

/* Makes the chicago extended shader `shader` a chicago shader: it keeps its
four-stage maps, or its two-stage maps when it has no four-stage ones (this
build's renderer draws four stages), and its extra flags move to where a
chicago shader has them. The two-stage maps are left unreferenced. */
static void transparent_chicago_extended_convert(
	uint8_t *shader)
{
	uint8_t two_stage_maps[TAG_BLOCK_BYTES];
	uint32_t extra_flags = read_u32(shader + TRANSPARENT_CHICAGO_EXTENDED_EXTRA_FLAGS_OFFSET);

	memcpy(two_stage_maps, shader + TRANSPARENT_CHICAGO_EXTENDED_TWO_STAGE_MAPS_OFFSET, TAG_BLOCK_BYTES);
	if (!read_s32(shader + TRANSPARENT_CHICAGO_MAPS_OFFSET + TAG_BLOCK_COUNT_OFFSET))
	{
		memcpy(shader + TRANSPARENT_CHICAGO_MAPS_OFFSET, two_stage_maps, TAG_BLOCK_BYTES);
	}
	write_u32(shader + TRANSPARENT_CHICAGO_EXTRA_FLAGS_OFFSET, extra_flags);
	/* the rest of a chicago shader is padding */
	memset(
		shader + TRANSPARENT_CHICAGO_EXTRA_FLAGS_OFFSET + 4,
		0,
		TRANSPARENT_CHICAGO_BYTES - (TRANSPARENT_CHICAGO_EXTRA_FLAGS_OFFSET + 4));

	return;
}

static int power_of_two(
	uint16_t value)
{
	return value && !(value & (value - 1));
}

/* Halo PC draws a 2D bitmap of any size; this build swizzles all but linear
ones, which takes power-of-two sides. An uncompressed one that has not got
them is drawn as linear (its first level: linear bitmaps have no others),
as Halo PC's texture is: birdcage's needler plasma, 3840 by 64. Compressed
and palettized ones cannot be linear, and are left as they were. */
static void bitmap_npot_make_linear(
	uint8_t *bitmap,
	struct custom_edition_conversion_report *report)
{
	uint16_t flags = read_u16(bitmap + BITMAP_DATA_FLAGS_OFFSET);
	uint16_t width = read_u16(bitmap + BITMAP_DATA_WIDTH_OFFSET);
	uint16_t height = read_u16(bitmap + BITMAP_DATA_HEIGHT_OFFSET);

	if (read_u16(bitmap + BITMAP_DATA_TYPE_OFFSET) != BITMAP_TYPE_2D ||
		read_u16(bitmap + BITMAP_DATA_FORMAT_OFFSET) >= BITMAP_FORMAT_DXT1 || flag_is_set(flags, BITMAP_DATA_LINEAR_BIT) ||
		!width || !height || (power_of_two(width) && power_of_two(height)))
	{
		return;
	}
	flags = (uint16_t)((flags | 1u << BITMAP_DATA_LINEAR_BIT) & ~(1u << BITMAP_DATA_POWER_OF_TWO_DIMENSIONS_BIT));
	write_u16(bitmap + BITMAP_DATA_FLAGS_OFFSET, flags);
	report->bitmaps_made_linear++;

	return;
}

/* Gives every bitmap of the bitmap tag at `group_offset` (tag `handle`) the
state the game expects of a bitmap it has not drawn yet: its tag is its own
(bitmaps.map holds the handles of whatever map it was built with), and it
has no texture cache block, hardware texture or pixels. One of a size only
Halo PC draws is made linear (bitmap_npot_make_linear). */
static void bitmaps_prepare(
	struct load_state const *state,
	uint32_t group_offset,
	uint32_t handle,
	struct custom_edition_conversion_report *report)
{
	int32_t bitmap_count;
	uint32_t bitmaps_offset;
	int32_t bitmap_index;

	write_u32(state->tag_cache + group_offset + BITMAP_GROUP_PIXEL_DATA_OFFSET + TAG_DATA_FILE_OFFSET_OFFSET, 0);
	if (!loaded_block_get(
		state->tag_cache + group_offset + BITMAP_GROUP_BITMAPS_OFFSET,
		CUSTOM_EDITION_TAG_CACHE_ADDRESS,
		state->used_bytes,
		BITMAP_DATA_BYTES,
		&bitmap_count,
		&bitmaps_offset))
	{
		return;
	}
	for (bitmap_index = 0; bitmap_index < bitmap_count; bitmap_index++)
	{
		uint8_t *bitmap = state->tag_cache + bitmaps_offset + (uint32_t)bitmap_index * BITMAP_DATA_BYTES;

		write_u32(bitmap + BITMAP_DATA_TAG_INDEX_OFFSET, handle);
		write_u32(bitmap + BITMAP_DATA_CACHE_BLOCK_INDEX_OFFSET, (uint32_t)NO_TAG_INDEX);
		write_u32(bitmap + BITMAP_DATA_HARDWARE_FORMAT_OFFSET, 0);
		write_u32(bitmap + BITMAP_DATA_BASE_ADDRESS_OFFSET, 0);
		bitmap_npot_make_linear(bitmap, report);
		report->bitmaps_prepared++;
	}

	return;
}

/* Gives every permutation of the sound at `sound_offset` (tag `handle`)
the state the game expects of one it has not played: its tag is its own
(sounds.map holds the handles of whatever map it was built with), with no
cache block or samples. A sound whose compression this build cannot decode
is made unplayable: this build plays Xbox ADPCM (and refuses uncompressed
sounds), and Custom Edition also has Ogg Vorbis, which would be decoded as
ADPCM noise. With no pitch ranges the game neither plays nor loads it
(sound_manager.c, sound_definition_is_playable). */
static void sound_prepare(
	struct load_state const *state,
	uint32_t sound_offset,
	uint32_t handle,
	struct custom_edition_conversion_report *report)
{
	uint8_t *sound = state->tag_cache + sound_offset;
	int16_t compression = read_s16(sound + SOUND_COMPRESSION_OFFSET);
	int decodable = compression == SOUND_COMPRESSION_NONE || compression == SOUND_COMPRESSION_XBOX_ADPCM;
	int32_t pitch_range_count;
	uint32_t pitch_ranges_offset;
	int32_t pitch_range_index;

	if (!loaded_block_get(
		sound + SOUND_PITCH_RANGES_OFFSET,
		CUSTOM_EDITION_TAG_CACHE_ADDRESS,
		state->used_bytes,
		SOUND_PITCH_RANGE_BYTES,
		&pitch_range_count,
		&pitch_ranges_offset))
	{
		return;
	}
	for (pitch_range_index = 0; pitch_range_index < pitch_range_count; pitch_range_index++)
	{
		uint8_t const *pitch_range = state->tag_cache + pitch_ranges_offset + (uint32_t)pitch_range_index * SOUND_PITCH_RANGE_BYTES;
		int32_t permutation_count;
		uint32_t permutations_offset;
		int32_t permutation_index;

		if (!loaded_block_get(
			pitch_range + SOUND_PITCH_RANGE_PERMUTATIONS_OFFSET,
			CUSTOM_EDITION_TAG_CACHE_ADDRESS,
			state->used_bytes,
			SOUND_PERMUTATION_BYTES,
			&permutation_count,
			&permutations_offset))
		{
			continue;
		}
		for (permutation_index = 0; permutation_index < permutation_count; permutation_index++)
		{
			uint8_t *permutation = state->tag_cache + permutations_offset + (uint32_t)permutation_index * SOUND_PERMUTATION_BYTES;
			int16_t permutation_compression = read_s16(permutation + SOUND_PERMUTATION_COMPRESSION_OFFSET);

			write_u32(permutation + SOUND_PERMUTATION_CACHE_BLOCK_INDEX_OFFSET, (uint32_t)NO_TAG_INDEX);
			write_u32(permutation + SOUND_PERMUTATION_CACHE_BASE_ADDRESS_OFFSET, 0);
			write_u32(permutation + SOUND_PERMUTATION_CACHE_TAG_INDEX_OFFSET, handle);
			write_u32(permutation + SOUND_PERMUTATION_RUNTIME_TAG_INDEX_OFFSET, handle);
			if (permutation_compression != SOUND_COMPRESSION_NONE && permutation_compression != SOUND_COMPRESSION_XBOX_ADPCM)
			{
				decodable = 0;
			}
		}
	}
	if (!decodable)
	{
		write_u32(sound + SOUND_PITCH_RANGES_OFFSET + TAG_BLOCK_COUNT_OFFSET, 0);
		report->sounds_undecodable++;
	}

	return;
}

/* A weapon's four exported functions: Halo PC has two inputs this build has
not, primary and secondary firing on (17 and 18, Invader's weapon.json),
after its primary and secondary firing, which they are made. */
#define WEAPON_GROUP_TAG 'weap'
#define WEAPON_BYTES 0x508
#define WEAPON_FUNCTION_MODES_OFFSET 0x330
#define WEAPON_FUNCTION_MODE_COUNT 4
#define WEAPON_FUNCTION_PRIMARY_FIRING 15
#define WEAPON_FUNCTION_PRIMARY_FIRING_ON 17
#define WEAPON_FUNCTION_SECONDARY_FIRING_ON 18

static void weapon_functions_convert(
	uint8_t *weapon,
	struct custom_edition_conversion_report *report)
{
	int index;

	for (index = 0; index < WEAPON_FUNCTION_MODE_COUNT; index++)
	{
		uint8_t *mode = weapon + WEAPON_FUNCTION_MODES_OFFSET + index * 2;
		int16_t value = read_s16(mode);

		if (value == WEAPON_FUNCTION_PRIMARY_FIRING_ON || value == WEAPON_FUNCTION_SECONDARY_FIRING_ON)
		{
			write_u16(mode, (uint16_t)(value - WEAPON_FUNCTION_PRIMARY_FIRING_ON + WEAPON_FUNCTION_PRIMARY_FIRING));
			report->weapon_functions_converted++;
		}
	}

	return;
}

/* Object overlays of the animation graph at `graph_offset` that name an
animation the graph does not have are made to name none, which the game
skips (objects.c, object_compute_node_matrices). Maps built with the editing
kit can have them; Custom Edition reads past the graph's animations there,
and this build asserts. */
static void animation_graph_overlays_repair(
	struct load_state const *state,
	uint32_t graph_offset,
	struct custom_edition_conversion_report *report)
{
	uint8_t const *graph = state->tag_cache + graph_offset;
	int32_t animation_count = read_s32(graph + ANIMATION_GRAPH_ANIMATIONS_OFFSET + TAG_BLOCK_COUNT_OFFSET);
	int32_t overlay_count;
	uint32_t overlays_offset;
	int32_t overlay_index;

	if (!loaded_block_get(
		graph + ANIMATION_GRAPH_OBJECT_OVERLAYS_OFFSET,
		CUSTOM_EDITION_TAG_CACHE_ADDRESS,
		state->used_bytes,
		ANIMATION_GRAPH_OBJECT_OVERLAY_BYTES,
		&overlay_count,
		&overlays_offset))
	{
		return;
	}
	for (overlay_index = 0; overlay_index < overlay_count; overlay_index++)
	{
		uint8_t *overlay = state->tag_cache + overlays_offset + (uint32_t)overlay_index * ANIMATION_GRAPH_OBJECT_OVERLAY_BYTES;
		int16_t animation_index = read_s16(overlay + ANIMATION_GRAPH_OBJECT_OVERLAY_ANIMATION_INDEX_OFFSET);

		if (animation_index != NO_BLOCK_INDEX && (animation_index < 0 || animation_index >= animation_count))
		{
			write_u16(overlay + ANIMATION_GRAPH_OBJECT_OVERLAY_ANIMATION_INDEX_OFFSET, (uint16_t)NO_BLOCK_INDEX);
			report->animation_overlays_disabled++;
		}
	}

	return;
}

/* Repairs a model's or animation graph's node tree (the nodes block at
`block`). The game walks the tree from node 0 by next sibling and first child,
so a link that loops back never ends and one past the nodes reads past the
array. Some Custom Edition maps have both (a sibling link back to the pelvis,
parents off by 256). Links past the nodes and links to a node already reached
are cut; a parent past the nodes becomes the parent found in the walk. */
static void node_links_repair(
	struct load_state const *state,
	uint8_t const *block,
	uint32_t node_bytes,
	struct custom_edition_conversion_report *report)
{
	int32_t node_count;
	uint32_t nodes_offset;
	uint8_t reached[MAXIMUM_REPAIRED_NODES];
	int16_t parents[MAXIMUM_REPAIRED_NODES];
	int16_t queue[MAXIMUM_REPAIRED_NODES];
	int32_t read_index = 0, write_index = 0, index;

	if (!loaded_block_get(block, CUSTOM_EDITION_TAG_CACHE_ADDRESS, state->used_bytes, node_bytes,
			&node_count, &nodes_offset) ||
		node_count <= 0 || node_count > MAXIMUM_REPAIRED_NODES)
	{
		return;
	}
	for (index = 0; index < node_count; index++)
	{
		uint8_t *links = state->tag_cache + nodes_offset + (uint32_t)index * node_bytes + NODE_LINKS_OFFSET;
		int link;

		for (link = 0; link < 2; link++)
		{
			int16_t linked = read_s16(links + link * 2);

			if (linked != NO_BLOCK_INDEX && (linked < 0 || linked >= node_count))
			{
				write_u16(links + link * 2, (uint16_t)NO_BLOCK_INDEX);
				report->node_links_cut++;
			}
		}
	}
	memset(reached, 0, sizeof(reached));
	reached[0] = 1;
	parents[0] = NO_BLOCK_INDEX;
	queue[write_index++] = 0;
	while (read_index < write_index)
	{
		int16_t node_index = queue[read_index++];
		uint8_t *links = state->tag_cache + nodes_offset + (uint32_t)node_index * node_bytes + NODE_LINKS_OFFSET;
		int link;

		/* link 0 is the next sibling (same parent), link 1 the first child */
		for (link = 0; link < 2; link++)
		{
			int16_t linked = read_s16(links + link * 2);

			if (linked == NO_BLOCK_INDEX)
				continue;
			if (reached[linked])
			{
				write_u16(links + link * 2, (uint16_t)NO_BLOCK_INDEX);
				report->node_links_cut++;
				continue;
			}
			reached[linked] = 1;
			parents[linked] = link ? node_index : parents[node_index];
			queue[write_index++] = linked;
		}
	}
	for (index = 0; index < node_count; index++)
	{
		uint8_t *parent = state->tag_cache + nodes_offset + (uint32_t)index * node_bytes + NODE_LINKS_OFFSET + 4;
		int16_t parent_index = read_s16(parent);

		if (parent_index != NO_BLOCK_INDEX && (parent_index < 0 || parent_index >= node_count))
		{
			write_u16(parent, (uint16_t)(reached[index] ? parents[index] : NO_BLOCK_INDEX));
			report->node_links_cut++;
		}
	}
}

/* the overlays block of the meter element at meter: Halo PC's minimum
alpha, made an empty block */
static void hud_meter_overlays_clear(
	uint8_t *meter,
	struct custom_edition_conversion_report *report)
{
	uint8_t *block = meter + HUD_METER_OVERLAYS_OFFSET;

	if (read_s32(block + TAG_BLOCK_COUNT_OFFSET) || read_u32(block + TAG_BLOCK_ADDRESS_OFFSET) ||
		read_u32(block + TAG_BLOCK_DEFINITION_OFFSET))
	{
		memset(block, 0, TAG_BLOCK_BYTES);
		report->hud_meter_alphas_cleared++;
	}

	return;
}

/* every meter element of the HUD definition of group_tag at offset */
static void hud_meters_convert(
	struct load_state const *state,
	uint32_t group_tag,
	uint32_t offset,
	struct custom_edition_conversion_report *report)
{
	uint8_t *definition = state->tag_cache + offset;
	int32_t count;
	uint32_t elements_offset;
	int32_t index;

	switch (group_tag)
	{
	case UNIT_HUD_INTERFACE_GROUP_TAG:
		hud_meter_overlays_clear(definition + UNIT_HUD_INTERFACE_SHIELD_METER_OFFSET, report);
		hud_meter_overlays_clear(definition + UNIT_HUD_INTERFACE_HEALTH_METER_OFFSET, report);
		if (loaded_block_get(definition + UNIT_HUD_INTERFACE_AUXILIARY_METERS_OFFSET, CUSTOM_EDITION_TAG_CACHE_ADDRESS,
			state->used_bytes, UNIT_HUD_INTERFACE_AUXILIARY_METER_BYTES, &count, &elements_offset))
		{
			for (index = 0; index < count; index++)
			{
				hud_meter_overlays_clear(state->tag_cache + elements_offset +
					(uint32_t)index * UNIT_HUD_INTERFACE_AUXILIARY_METER_BYTES + UNIT_HUD_INTERFACE_AUXILIARY_METER_METER_OFFSET,
					report);
			}
		}
		break;
	case WEAPON_HUD_INTERFACE_GROUP_TAG:
		if (loaded_block_get(definition + WEAPON_HUD_INTERFACE_METERS_OFFSET, CUSTOM_EDITION_TAG_CACHE_ADDRESS,
			state->used_bytes, WEAPON_HUD_STATIC_OR_METER_BYTES, &count, &elements_offset))
		{
			for (index = 0; index < count; index++)
			{
				hud_meter_overlays_clear(state->tag_cache + elements_offset +
					(uint32_t)index * WEAPON_HUD_STATIC_OR_METER_BYTES + WEAPON_HUD_ELEMENT_PLACEMENT_OFFSET, report);
			}
		}
		break;
	default:
		break;
	}

	return;
}

/* Halo PC draws a HUD element whose placement has the high resolution scale
flag at half the size of its bitmap, and Custom Edition's HUD bitmaps are
made for that: in Blood Gulch every flagged element draws a bitmap twice the
size of the one its Xbox counterpart draws, and every other element one of
the same size (docs/custom_edition_caches.md). A bitmap may ask the same of
every element that draws it (hud_bitmap_halves_scale). This build has no
such flags (hud_draw.c draws a bitmap at its size times the placement's
scale), so the scale takes them in.

Halo PC reads the placement's flag only on statics, meters and numbers, as
Chimera found (adapted from Chimera, by SnowyMouse): crosshair and overlay
items are halved by their bitmap's flags alone. A number keeps its flag,
which hud_draw_numbers reads: its scale is not its digits'. */
static void hud_placement_convert(
	uint8_t *placement,
	int bitmap_halves_scale,
	struct custom_edition_conversion_report *report)
{
	uint16_t flags = read_u16(placement + HUD_PLACEMENT_SCALING_FLAGS_OFFSET);

	if (flag_is_set(flags, HUD_SCALING_USE_HIGH_RESOLUTION_SCALE_BIT) || bitmap_halves_scale)
	{
		write_f32(placement + HUD_PLACEMENT_WIDTH_SCALE_OFFSET, read_f32(placement + HUD_PLACEMENT_WIDTH_SCALE_OFFSET) * 0.5f);
		write_f32(placement + HUD_PLACEMENT_HEIGHT_SCALE_OFFSET, read_f32(placement + HUD_PLACEMENT_HEIGHT_SCALE_OFFSET) * 0.5f);
		write_u16(
			placement + HUD_PLACEMENT_SCALING_FLAGS_OFFSET,
			(uint16_t)(flags & ~(1U << HUD_SCALING_USE_HIGH_RESOLUTION_SCALE_BIT)));
		report->hud_placements_rescaled++;
	}

	return;
}

/* whether the bitmap a HUD element draws (its tag reference) has Halo PC's
half HUD scale or force HUD high resolution scale flag */
static int hud_bitmap_halves_scale(
	struct load_state const *state,
	uint8_t const *reference)
{
	int32_t tag_count;
	uint8_t const *instances = loaded_tag_instances(state, &tag_count);
	int32_t handle = read_s32(reference + TAG_REFERENCE_INDEX_OFFSET);
	uint8_t const *instance;
	uint32_t offset;
	uint16_t flags;

	if (!instances || handle == NO_TAG_INDEX || ((uint32_t)handle & ABSOLUTE_INDEX_MASK) >= (uint32_t)tag_count)
	{
		return 0;
	}
	instance = instances + ((uint32_t)handle & ABSOLUTE_INDEX_MASK) * TAG_INSTANCE_BYTES;
	if (read_u32(instance + TAG_INSTANCE_GROUP_OFFSET) != BITMAP_GROUP_TAG ||
		!tag_cache_offset(state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), BITMAP_GROUP_BYTES, &offset))
	{
		return 0;
	}
	flags = read_u16(state->tag_cache + offset + BITMAP_GROUP_FLAGS_OFFSET);

	return flag_is_set(flags, BITMAP_HALF_HUD_SCALE_BIT) || flag_is_set(flags, BITMAP_FORCE_HUD_HIGH_RESOLUTION_SCALE_BIT);
}

/* the element at `element`, whose bitmap is `bitmap_offset` bytes on
(NO_HUD_BITMAP: none) */
static void hud_element_convert(
	struct load_state const *state,
	uint8_t *element,
	int32_t bitmap_offset,
	struct custom_edition_conversion_report *report)
{
	hud_placement_convert(
		element,
		bitmap_offset != NO_HUD_BITMAP && hud_bitmap_halves_scale(state, element + bitmap_offset),
		report);

	return;
}

/* the placement at `placement_offset` in each element of the block at
`block`, when the block lies within the tag cache: each drawing the bitmap
`bitmap_offset` bytes after its placement (NO_HUD_BITMAP: none of its own),
or the items of a bitmap that halves their scale */
static void hud_placement_block_convert(
	struct load_state const *state,
	uint8_t const *block,
	uint32_t element_bytes,
	uint32_t placement_offset,
	int32_t bitmap_offset,
	int items_of_halving_bitmap,
	struct custom_edition_conversion_report *report)
{
	int32_t element_count;
	uint32_t elements_offset;
	int32_t element_index;

	if (!loaded_block_get(block, CUSTOM_EDITION_TAG_CACHE_ADDRESS, state->used_bytes, element_bytes, &element_count, &elements_offset))
	{
		return;
	}
	for (element_index = 0; element_index < element_count; element_index++)
	{
		uint8_t *placement = state->tag_cache + elements_offset + (uint32_t)element_index * element_bytes + placement_offset;

		if (items_of_halving_bitmap)
			hud_placement_convert(placement, 1, report);
		else
			hud_element_convert(state, placement, bitmap_offset, report);
	}

	return;
}

/* the placements of the items of each weapon HUD crosshair or overlay in
the block at `block` whose bitmap halves their scale */
static void weapon_hud_items_convert(
	struct load_state const *state,
	uint8_t const *block,
	uint32_t item_bytes,
	struct custom_edition_conversion_report *report)
{
	int32_t element_count;
	uint32_t elements_offset;
	int32_t element_index;

	if (!loaded_block_get(
		block,
		CUSTOM_EDITION_TAG_CACHE_ADDRESS,
		state->used_bytes,
		WEAPON_HUD_CROSSHAIRS_OR_OVERLAYS_BYTES,
		&element_count,
		&elements_offset))
	{
		return;
	}
	for (element_index = 0; element_index < element_count; element_index++)
	{
		uint8_t const *element = state->tag_cache + elements_offset +
			(uint32_t)element_index * WEAPON_HUD_CROSSHAIRS_OR_OVERLAYS_BYTES;

		if (hud_bitmap_halves_scale(state, element + WEAPON_HUD_CROSSHAIRS_OR_OVERLAYS_BITMAP_OFFSET))
			hud_placement_block_convert(state, element + WEAPON_HUD_ITEMS_OFFSET, item_bytes, 0, NO_HUD_BITMAP, 1, report);
	}

	return;
}

/* the size of a HUD definition whose placements hud_placements_convert
converts, or 0 for any other group */
static uint32_t hud_definition_bytes(
	uint32_t group_tag)
{
	switch (group_tag)
	{
	case UNIT_HUD_INTERFACE_GROUP_TAG:
		return UNIT_HUD_INTERFACE_BYTES;
	case WEAPON_HUD_INTERFACE_GROUP_TAG:
		return WEAPON_HUD_INTERFACE_BYTES;
	case GRENADE_HUD_INTERFACE_GROUP_TAG:
		return GRENADE_HUD_INTERFACE_BYTES;
	case HUD_GLOBALS_GROUP_TAG:
		return HUD_GLOBALS_BYTES;
	default:
		return 0;
	}
}

/* every placement of the HUD definition of group `group_tag` at `offset` */
static void hud_placements_convert(
	struct load_state const *state,
	uint32_t group_tag,
	uint32_t offset,
	struct custom_edition_conversion_report *report)
{
	uint8_t *definition = state->tag_cache + offset;
	size_t index;

	switch (group_tag)
	{
	case UNIT_HUD_INTERFACE_GROUP_TAG:
		for (index = 0; index < sizeof(unit_hud_interface_elements) / sizeof(unit_hud_interface_elements[0]); index++)
			hud_element_convert(state, definition + unit_hud_interface_elements[index], HUD_ELEMENT_BITMAP_OFFSET, report);
		hud_element_convert(state, definition + UNIT_HUD_INTERFACE_BLIPS_OFFSET, NO_HUD_BITMAP, report);
		hud_placement_block_convert(state, definition + UNIT_HUD_INTERFACE_AUXILIARY_OVERLAYS_OFFSET,
			UNIT_HUD_INTERFACE_AUXILIARY_OVERLAY_BYTES, 0, HUD_ELEMENT_BITMAP_OFFSET, 0, report);
		hud_placement_block_convert(state, definition + UNIT_HUD_INTERFACE_AUXILIARY_METERS_OFFSET,
			UNIT_HUD_INTERFACE_AUXILIARY_METER_BYTES, UNIT_HUD_INTERFACE_AUXILIARY_METER_BACKGROUND_OFFSET,
			HUD_ELEMENT_BITMAP_OFFSET, 0, report);
		hud_placement_block_convert(state, definition + UNIT_HUD_INTERFACE_AUXILIARY_METERS_OFFSET,
			UNIT_HUD_INTERFACE_AUXILIARY_METER_BYTES, UNIT_HUD_INTERFACE_AUXILIARY_METER_METER_OFFSET,
			HUD_ELEMENT_BITMAP_OFFSET, 0, report);
		break;
	case WEAPON_HUD_INTERFACE_GROUP_TAG:
		hud_placement_block_convert(state, definition + WEAPON_HUD_INTERFACE_STATICS_OFFSET,
			WEAPON_HUD_STATIC_OR_METER_BYTES, WEAPON_HUD_ELEMENT_PLACEMENT_OFFSET, HUD_ELEMENT_BITMAP_OFFSET, 0, report);
		hud_placement_block_convert(state, definition + WEAPON_HUD_INTERFACE_METERS_OFFSET,
			WEAPON_HUD_STATIC_OR_METER_BYTES, WEAPON_HUD_ELEMENT_PLACEMENT_OFFSET, HUD_ELEMENT_BITMAP_OFFSET, 0, report);
		weapon_hud_items_convert(state, definition + WEAPON_HUD_INTERFACE_CROSSHAIRS_OFFSET, WEAPON_HUD_CROSSHAIR_ITEM_BYTES, report);
		weapon_hud_items_convert(state, definition + WEAPON_HUD_INTERFACE_OVERLAYS_OFFSET, WEAPON_HUD_OVERLAY_ITEM_BYTES, report);
		break;
	case GRENADE_HUD_INTERFACE_GROUP_TAG:
		for (index = 0; index < sizeof(grenade_hud_interface_elements) / sizeof(grenade_hud_interface_elements[0]); index++)
			hud_element_convert(state, definition + grenade_hud_interface_elements[index], HUD_ELEMENT_BITMAP_OFFSET, report);
		if (hud_bitmap_halves_scale(state, definition + GRENADE_HUD_INTERFACE_OVERLAY_BITMAP_OFFSET))
			hud_placement_block_convert(state, definition + GRENADE_HUD_INTERFACE_OVERLAY_ITEMS_OFFSET,
				WEAPON_HUD_OVERLAY_ITEM_BYTES, 0, NO_HUD_BITMAP, 1, report);
		break;
	case HUD_GLOBALS_GROUP_TAG:
		hud_element_convert(state, definition + HUD_GLOBALS_MESSAGING_PLACEMENT_OFFSET, NO_HUD_BITMAP, report);
		break;
	default:
		break;
	}

	return;
}

/* Halo PC's hint that a key shows the score names the key where it reads
"%s" in quotes. This build copies the hint as it is, so the Xbox button it
reads for the score takes the place of the placeholder and its quotes: BACK,
which the native builds read from F1 and the controller's Back button
(port/linux/src/xinput_sdl.c). */
static void multiplayer_score_hint_convert(
	struct load_state const *state,
	uint32_t string_list_offset,
	struct custom_edition_conversion_report *report)
{
	int32_t string_count;
	uint32_t strings_offset;
	uint8_t const *string;
	int32_t string_bytes;
	uint32_t text_offset;
	uint32_t character_index;

	if (!loaded_block_get(
			state->tag_cache + string_list_offset + UNICODE_STRING_LIST_STRINGS_OFFSET,
			CUSTOM_EDITION_TAG_CACHE_ADDRESS,
			state->used_bytes,
			TAG_DATA_BYTES,
			&string_count,
			&strings_offset) ||
		string_count <= MULTIPLAYER_GAME_TEXT_SCORE_HINT_INDEX)
	{
		return;
	}
	string = state->tag_cache + strings_offset + MULTIPLAYER_GAME_TEXT_SCORE_HINT_INDEX * TAG_DATA_BYTES;
	string_bytes = read_s32(string + TAG_DATA_SIZE_OFFSET);
	if (string_bytes <= 0 ||
		!tag_cache_offset(state, read_u32(string + TAG_DATA_ADDRESS_OFFSET), (uint32_t)string_bytes, &text_offset))
	{
		return;
	}
	for (character_index = 0;
		character_index + SCORE_KEY_PLACEHOLDER_CHARACTERS <= (uint32_t)string_bytes / sizeof(uint16_t);
		character_index++)
	{
		uint8_t *text = state->tag_cache + text_offset + character_index * sizeof(uint16_t);
		int placeholder_index = 0;

		while (placeholder_index < SCORE_KEY_PLACEHOLDER_CHARACTERS &&
			read_u16(text + placeholder_index * sizeof(uint16_t)) == score_key_placeholder[placeholder_index])
		{
			placeholder_index++;
		}
		if (placeholder_index == SCORE_KEY_PLACEHOLDER_CHARACTERS)
		{
			for (placeholder_index = 0; placeholder_index < SCORE_KEY_PLACEHOLDER_CHARACTERS; placeholder_index++)
			{
				write_u16(text + placeholder_index * sizeof(uint16_t), score_button_name[placeholder_index]);
			}
			report->score_hint_converted = 1;
			return;
		}
	}

	return;
}

/* Halo PC's widgets run its own functions besides the Xbox's, numbered past
them: this build has none of those (its menus' own are other numbers, from
PC_MENU_FUNCTION_BASE), and a handler running one would only log an invalid
function. Those handlers run none. */
static void widget_pc_functions_clear(
	struct load_state const *state,
	uint32_t widget_offset,
	struct custom_edition_conversion_report *report)
{
	int32_t handler_count;
	uint32_t handlers_offset;
	int32_t handler_index;

	if (!loaded_block_get(
			state->tag_cache + widget_offset + UI_WIDGET_EVENT_HANDLERS_OFFSET,
			CUSTOM_EDITION_TAG_CACHE_ADDRESS,
			state->used_bytes,
			UI_WIDGET_EVENT_HANDLER_BYTES,
			&handler_count,
			&handlers_offset))
	{
		return;
	}
	for (handler_index = 0; handler_index < handler_count; handler_index++)
	{
		uint8_t *handler = state->tag_cache + handlers_offset + (uint32_t)handler_index * UI_WIDGET_EVENT_HANDLER_BYTES;
		uint32_t flags = read_u32(handler + UI_WIDGET_EVENT_HANDLER_FLAGS_OFFSET);

		if ((flags & UI_WIDGET_EVENT_HANDLER_RUN_FUNCTION_FLAG) &&
			read_s16(handler + UI_WIDGET_EVENT_HANDLER_FUNCTION_OFFSET) >= XBOX_WIDGET_FUNCTION_COUNT)
		{
			write_u32(handler + UI_WIDGET_EVENT_HANDLER_FLAGS_OFFSET, flags & ~UI_WIDGET_EVENT_HANDLER_RUN_FUNCTION_FLAG);
			report->widget_functions_cleared++;
		}
	}

	return;
}

/* whether a tag's name ends in `item` (the last part of its path) */
static int tag_name_item_is(
	char const *name,
	char const *item)
{
	char const *last = strrchr(name, '\\');

	return !strcmp(last ? last + 1 : name, item);
}

/* Halo PC's multiplayer pause menu has its game options and settings
between the Xbox's resume and quit, and they open screens of Halo PC's
functions (none here, widget_pc_functions_clear). The list keeps the Xbox's
two, moved to the middle of its box of rows. */
static void multiplayer_pause_list_convert(
	struct load_state const *state,
	uint32_t widget_offset,
	struct custom_edition_conversion_report *report)
{
	uint8_t *block = state->tag_cache + widget_offset + UI_WIDGET_CHILD_WIDGETS_OFFSET;
	int32_t child_count;
	uint32_t children_offset;
	uint8_t *children;
	int16_t row;
	int32_t child_index;
	int32_t kept = 0;

	if (!loaded_block_get(
			block,
			CUSTOM_EDITION_TAG_CACHE_ADDRESS,
			state->used_bytes,
			UI_WIDGET_CHILD_BYTES,
			&child_count,
			&children_offset) ||
		child_count < 2)
	{
		return;
	}
	children = state->tag_cache + children_offset;
	/* (the rows' spacing, from the first two) */
	row = (int16_t)(read_s16(children + UI_WIDGET_CHILD_BYTES + UI_WIDGET_CHILD_VERTICAL_OFFSET) -
		read_s16(children + UI_WIDGET_CHILD_VERTICAL_OFFSET));
	for (child_index = 0; child_index < child_count; child_index++)
	{
		uint8_t *child = children + (uint32_t)child_index * UI_WIDGET_CHILD_BYTES;
		uint32_t handle = read_u32(child + TAG_REFERENCE_INDEX_OFFSET);
		char const *name = custom_edition_cache_tag_name(state->tag_cache, state->used_bytes, (int32_t)(handle & 0xFFFF));

		if (name && (tag_name_item_is(name, MULTIPLAYER_PAUSE_RESUME_NAME) || tag_name_item_is(name, MULTIPLAYER_PAUSE_QUIT_NAME)))
		{
			if (kept != child_index)
				memmove(children + (uint32_t)kept * UI_WIDGET_CHILD_BYTES, child, UI_WIDGET_CHILD_BYTES);
			kept++;
		}
	}
	if (kept == 0 || kept == child_count)
	{
		return;
	}
	for (child_index = 0; child_index < kept; child_index++)
	{
		write_u16(children + (uint32_t)child_index * UI_WIDGET_CHILD_BYTES + UI_WIDGET_CHILD_VERTICAL_OFFSET,
			(uint16_t)(row * (child_index + (child_count - kept) / 2)));
	}
	write_u32(block + TAG_BLOCK_COUNT_OFFSET, (uint32_t)kept);
	report->pause_menu_trimmed = 1;

	return;
}

/* ---------- Halo PC behaviours */

#define BEHAVIOUR(name) (1U << _custom_edition_behaviour_##name)

static struct custom_edition_behaviour_map
{
	char const *map_name;
	uint32_t tags_checksum;
	uint32_t behaviours;
} const custom_edition_behaviour_maps[] =
{
#include "custom_edition_behaviours.inc"
};

#undef BEHAVIOUR

static char const *const custom_edition_behaviour_names[NUMBER_OF_CUSTOM_EDITION_BEHAVIOURS] =
{
	"gearbox_chicago_multiply",
	"gearbox_meters",
	"gearbox_multitexture_blend_modes",
	"alternate_bump_attenuation",
	"gearbox_bump_attenuation",
	"invert_detail_after_reflection",
	"embedded_lua",
	"hud_number_scale",
	"disable_bitmap_hud_scale_flags",
	"old_widescreen_fix",
	"gearbox_shader_environment_types",
	"block_multitexture_overlays",
};

/* the behaviours the map named `map_name` relies on, found as Chimera finds
them: by its name in lower case and its tag data checksum */
static uint32_t custom_edition_behaviours_find(
	char const *map_name,
	uint32_t tags_checksum)
{
	char name[CACHE_FILE_STRING_BYTES];
	size_t index;

	for (index = 0; index + 1 < sizeof(name) && map_name[index]; index++)
	{
		char character = map_name[index];

		name[index] = (char)(character >= 'A' && character <= 'Z' ? character - 'A' + 'a' : character);
	}
	name[index] = 0;
	for (index = 0; index < sizeof(custom_edition_behaviour_maps) / sizeof(custom_edition_behaviour_maps[0]); index++)
	{
		struct custom_edition_behaviour_map const *map = &custom_edition_behaviour_maps[index];

		if (map->tags_checksum == tags_checksum && !strcmp(map->map_name, name))
			return map->behaviours;
	}

	return 0;
}

/* disable_bitmap_hud_scale_flags: every bitmap's Halo PC HUD scale flags
cleared, before the HUD placements are converted by them */
static void bitmap_hud_scale_flags_clear(
	struct load_state const *state)
{
	uint32_t const scale_flags = 1U << BITMAP_HALF_HUD_SCALE_BIT | 1U << BITMAP_FORCE_HUD_HIGH_RESOLUTION_SCALE_BIT;
	int32_t tag_index = NO_TAG_INDEX;
	uint8_t *bitmap;

	while ((bitmap = custom_edition_cache_tag_next(
		state->tag_cache, state->used_bytes, BITMAP_GROUP_TAG, BITMAP_GROUP_BYTES, &tag_index)) != NULL)
	{
		write_u16(bitmap + BITMAP_GROUP_FLAGS_OFFSET,
			(uint16_t)(read_u16(bitmap + BITMAP_GROUP_FLAGS_OFFSET) & ~scale_flags));
	}

	return;
}

/* hud_number_scale: the HUD digits' metrics halved, rounding up, and their
bitmap given Halo PC's half HUD scale, so every number's digits are drawn
at half size (adapted from Chimera, by SnowyMouse) */
static void hud_digits_halve(
	struct load_state const *state)
{
	int32_t tag_index = NO_TAG_INDEX;
	uint8_t *globals = custom_edition_cache_tag_next(state->tag_cache, state->used_bytes, GLOBALS_GROUP_TAG,
		GLOBALS_INTERFACE_BITMAPS_OFFSET + TAG_BLOCK_BYTES, &tag_index);
	uint8_t *interface_bitmaps = globals ?
		custom_edition_cache_block_element(state->tag_cache, state->used_bytes,
			globals + GLOBALS_INTERFACE_BITMAPS_OFFSET, 0, INTERFACE_BITMAPS_HUD_DIGITS_OFFSET + TAG_REFERENCE_BYTES) :
		NULL;
	uint8_t *digits = interface_bitmaps ?
		custom_edition_cache_tag_get(state->tag_cache, state->used_bytes,
			read_u32(interface_bitmaps + INTERFACE_BITMAPS_HUD_DIGITS_OFFSET + TAG_REFERENCE_INDEX_OFFSET),
			HUD_NUMBER_GROUP_TAG, HUD_NUMBER_BYTES) :
		NULL;
	uint8_t *bitmap;
	int metric;

	if (!digits)
	{
		return;
	}
	for (metric = 0; metric < HUD_NUMBER_METRIC_COUNT; metric++)
	{
		int value = (int8_t)digits[HUD_NUMBER_METRICS_OFFSET + metric];

		digits[HUD_NUMBER_METRICS_OFFSET + metric] = (uint8_t)(value >= 0 ? (value + 1) / 2 : value / 2);
	}
	bitmap = custom_edition_cache_tag_get(state->tag_cache, state->used_bytes,
		read_u32(digits + TAG_REFERENCE_INDEX_OFFSET), BITMAP_GROUP_TAG, BITMAP_GROUP_BYTES);
	if (bitmap)
	{
		write_u16(bitmap + BITMAP_GROUP_FLAGS_OFFSET,
			(uint16_t)(read_u16(bitmap + BITMAP_GROUP_FLAGS_OFFSET) | 1U << BITMAP_HALF_HUD_SCALE_BIT));
	}

	return;
}

char const *custom_edition_behaviour_name(
	short behaviour)
{
	return behaviour >= 0 && behaviour < NUMBER_OF_CUSTOM_EDITION_BEHAVIOURS ?
		custom_edition_behaviour_names[behaviour] :
		"unknown";
}

enum cache_file_status custom_edition_cache_convert(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	char const *map_name,
	struct custom_edition_conversion_report *report)
{
	struct load_state state;
	uint8_t const *instances;
	int32_t tag_count;
	int32_t tag_index;

	memset(report, 0, sizeof(*report));
	report->problem_tag_index = NO_TAG_INDEX;
	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	if (!instances)
	{
		return _cache_file_status_bad_tag_instances_range;
	}
	report->behaviours = custom_edition_behaviours_find(map_name, read_u32(tag_cache + TAG_INDEX_CHECKSUM_OFFSET));
	if (flag_is_set(report->behaviours, _custom_edition_behaviour_disable_bitmap_hud_scale_flags))
	{
		bitmap_hud_scale_flags_clear(&state);
	}
	if (flag_is_set(report->behaviours, _custom_edition_behaviour_hud_number_scale))
	{
		hud_digits_halve(&state);
	}
	for (tag_index = 0; tag_index < tag_count; tag_index++)
	{
		uint8_t const *instance = instances + (uint32_t)tag_index * TAG_INSTANCE_BYTES;
		uint32_t group_tag = read_u32(instance + TAG_INSTANCE_GROUP_OFFSET);
		struct shader_group_type const *shader_type = shader_group_type_get(group_tag);
		uint32_t offset;

		if (group_tag == SHADER_MODEL_GROUP_TAG &&
			flag_is_set(report->behaviours, _custom_edition_behaviour_invert_detail_after_reflection) &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), SHADER_MODEL_FLAGS_OFFSET + 2, &offset))
		{
			write_u16(tag_cache + offset + SHADER_MODEL_FLAGS_OFFSET,
				(uint16_t)(read_u16(tag_cache + offset + SHADER_MODEL_FLAGS_OFFSET) ^ 1U << SHADER_MODEL_DETAIL_AFTER_REFLECTION_BIT));
		}

		if (group_tag == BITMAP_GROUP_TAG &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), BITMAP_GROUP_BYTES, &offset))
		{
			bitmaps_prepare(&state, offset, read_u32(instance + TAG_INSTANCE_HANDLE_OFFSET), report);
		}
		if (group_tag == WEAPON_GROUP_TAG &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), WEAPON_BYTES, &offset))
		{
			weapon_functions_convert(tag_cache + offset, report);
		}
		if (group_tag == ANIMATION_GRAPH_GROUP_TAG &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), ANIMATION_GRAPH_BYTES, &offset))
		{
			animation_graph_overlays_repair(&state, offset, report);
			node_links_repair(&state, state.tag_cache + offset + ANIMATION_GRAPH_NODES_OFFSET, ANIMATION_GRAPH_NODE_BYTES,
				report);
		}
		if (group_tag == GBXMODEL_GROUP_TAG &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), GBXMODEL_BYTES, &offset))
		{
			node_links_repair(&state, state.tag_cache + offset + GBXMODEL_NODES_OFFSET, GBXMODEL_NODE_BYTES, report);
		}
		if (group_tag == SOUND_GROUP_TAG &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), SOUND_DEFINITION_BYTES, &offset))
		{
			sound_prepare(&state, offset, read_u32(instance + TAG_INSTANCE_HANDLE_OFFSET), report);
		}
		if (hud_definition_bytes(group_tag) &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), hud_definition_bytes(group_tag), &offset))
		{
			hud_placements_convert(&state, group_tag, offset, report);
			hud_meters_convert(&state, group_tag, offset, report);
		}
		if (group_tag == UNICODE_STRING_LIST_GROUP_TAG &&
			!strcmp(custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index), MULTIPLAYER_GAME_TEXT_NAME) &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), UNICODE_STRING_LIST_BYTES, &offset))
		{
			multiplayer_score_hint_convert(&state, offset, report);
		}
		if (group_tag == UI_WIDGET_DEFINITION_GROUP_TAG &&
			tag_cache_offset(&state, read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET), UI_WIDGET_DEFINITION_BYTES, &offset))
		{
			widget_pc_functions_clear(&state, offset, report);
			if (!strcmp(custom_edition_cache_tag_name(tag_cache, loaded_bytes, tag_index), MULTIPLAYER_PAUSE_LIST_NAME))
				multiplayer_pause_list_convert(&state, offset, report);
		}
		if (!shader_type)
		{
			continue;
		}
		if (!tag_cache_offset(
				&state,
				read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET),
				group_tag == TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG ? TRANSPARENT_CHICAGO_EXTENDED_BYTES : SHADER_BYTES,
				&offset))
		{
			report->problem_tag_index = tag_index;
			return _cache_file_status_bad_shader_type;
		}
		/* (the group says what the shader is; a type field saying otherwise,
		as map protection leaves them, is given the group's) */
		if (read_s16(tag_cache + offset + SHADER_TYPE_OFFSET) != shader_type->custom_edition_type)
		{
			report->shaders_mistyped++;
		}
		write_u16(tag_cache + offset + SHADER_TYPE_OFFSET, (uint16_t)shader_type->type);
		if (shader_type->type != shader_type->custom_edition_type)
		{
			report->shaders_retyped++;
		}
		if (group_tag == TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG)
		{
			transparent_chicago_extended_convert(tag_cache + offset);
		}
	}
	report->chicago_extended_shaders = custom_edition_cache_tags_regroup(
		tag_cache,
		loaded_bytes,
		TRANSPARENT_CHICAGO_EXTENDED_GROUP_TAG,
		TRANSPARENT_CHICAGO_GROUP_TAG);

	return _cache_file_status_ok;
}

void custom_edition_cache_combine_resource_offsets(
	uint8_t *tag_cache,
	uint32_t loaded_bytes,
	uint32_t bitmaps_offset,
	uint32_t sounds_offset)
{
	struct load_state state;
	uint8_t const *instances;
	int32_t tag_count;
	int32_t tag_index;

	loaded_state_initialize(&state, tag_cache, loaded_bytes);
	instances = loaded_tag_instances(&state, &tag_count);
	if (!instances)
	{
		return;
	}
	for (tag_index = 0; tag_index < tag_count; tag_index++)
	{
		uint8_t const *instance = instances + (uint32_t)tag_index * TAG_INSTANCE_BYTES;
		uint32_t group_tag = read_u32(instance + TAG_INSTANCE_GROUP_OFFSET);
		uint32_t address = read_u32(instance + TAG_INSTANCE_ADDRESS_OFFSET);
		uint32_t tag_offset;

		if (group_tag == BITMAP_GROUP_TAG && tag_cache_offset(&state, address, BITMAP_GROUP_BYTES, &tag_offset))
		{
			uint8_t const *bitmaps = tag_cache + tag_offset + BITMAP_GROUP_BITMAPS_OFFSET;
			int32_t bitmap_count = read_s32(bitmaps + TAG_BLOCK_COUNT_OFFSET);
			uint32_t bitmaps_block_offset;
			int32_t bitmap_index;

			if (bitmap_count <= 0 ||
				(uint32_t)bitmap_count > UINT32_MAX / BITMAP_DATA_BYTES ||
				!tag_cache_offset(
					&state,
					read_u32(bitmaps + TAG_BLOCK_ADDRESS_OFFSET),
					(uint32_t)bitmap_count * BITMAP_DATA_BYTES,
					&bitmaps_block_offset))
			{
				continue;
			}
			for (bitmap_index = 0; bitmap_index < bitmap_count; bitmap_index++)
			{
				uint8_t *bitmap = tag_cache + bitmaps_block_offset + (uint32_t)bitmap_index * BITMAP_DATA_BYTES;
				uint16_t flags = read_u16(bitmap + BITMAP_DATA_FLAGS_OFFSET);

				if (flag_is_set(flags, BITMAP_DATA_IN_RESOURCE_MAP_BIT))
				{
					write_u32(
						bitmap + BITMAP_DATA_PIXELS_OFFSET_OFFSET,
						read_u32(bitmap + BITMAP_DATA_PIXELS_OFFSET_OFFSET) + bitmaps_offset);
					write_u16(bitmap + BITMAP_DATA_FLAGS_OFFSET, (uint16_t)(flags & ~(1U << BITMAP_DATA_IN_RESOURCE_MAP_BIT)));
				}
			}
		}
		else if (group_tag == SOUND_GROUP_TAG && tag_cache_offset(&state, address, SOUND_DEFINITION_BYTES, &tag_offset))
		{
			uint8_t const *pitch_ranges = tag_cache + tag_offset + SOUND_PITCH_RANGES_OFFSET;
			int32_t pitch_range_count = read_s32(pitch_ranges + TAG_BLOCK_COUNT_OFFSET);
			uint32_t pitch_ranges_offset;
			int32_t pitch_range_index;

			if (pitch_range_count <= 0 ||
				(uint32_t)pitch_range_count > UINT32_MAX / SOUND_PITCH_RANGE_BYTES ||
				!tag_cache_offset(
					&state,
					read_u32(pitch_ranges + TAG_BLOCK_ADDRESS_OFFSET),
					(uint32_t)pitch_range_count * SOUND_PITCH_RANGE_BYTES,
					&pitch_ranges_offset))
			{
				continue;
			}
			for (pitch_range_index = 0; pitch_range_index < pitch_range_count; pitch_range_index++)
			{
				uint8_t const *permutations = tag_cache + pitch_ranges_offset +
					(uint32_t)pitch_range_index * SOUND_PITCH_RANGE_BYTES + SOUND_PITCH_RANGE_PERMUTATIONS_OFFSET;
				int32_t permutation_count = read_s32(permutations + TAG_BLOCK_COUNT_OFFSET);
				uint32_t permutations_offset;
				int32_t permutation_index;

				if (permutation_count <= 0 ||
					(uint32_t)permutation_count > UINT32_MAX / SOUND_PERMUTATION_BYTES ||
					!tag_cache_offset(
						&state,
						read_u32(permutations + TAG_BLOCK_ADDRESS_OFFSET),
						(uint32_t)permutation_count * SOUND_PERMUTATION_BYTES,
						&permutations_offset))
				{
					continue;
				}
				for (permutation_index = 0; permutation_index < permutation_count; permutation_index++)
				{
					uint8_t *samples = tag_cache + permutations_offset +
						(uint32_t)permutation_index * SOUND_PERMUTATION_BYTES + SOUND_PERMUTATION_SAMPLES_OFFSET;
					uint32_t flags = read_u32(samples + TAG_DATA_FLAGS_OFFSET);

					if (flag_is_set(flags, SOUND_SAMPLES_IN_RESOURCE_MAP_BIT))
					{
						write_u32(
							samples + TAG_DATA_FILE_OFFSET_OFFSET,
							read_u32(samples + TAG_DATA_FILE_OFFSET_OFFSET) + sounds_offset);
						write_u32(samples + TAG_DATA_FLAGS_OFFSET, flags & ~(1UL << SOUND_SAMPLES_IN_RESOURCE_MAP_BIT));
					}
				}
			}
		}
	}

	return;
}
