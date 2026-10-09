/*
TAG_SCHEMA_SCENARIO.C

The schemas (tag_schema.h) of the scenario (scnr): its object placements and
palettes, its AI (encounters, command lists, conversations), its scripts,
its recorded animations, its cutscene data, its netgame data and its
structure bsps.

What the game never reads is left out: the editor's data and comments, the
child scenarios, the scenario's functions, its decals (the structure bsps
hold the decals the game places), the placements' permutations
(object_add_scenario_permutation is empty) and the references the scenario
keeps from before it had blocks (ugly_structure_bsp, unloved_globals,
bad_sky).
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"
#include "ai/actions.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/encounters.h"
#include "bitmaps/bitmap_group.h"
#include "cache/predicted_resources.h"
#include "cutscene/recorded_animation_definitions.h"
#include "devices/device_light_fixtures.h"
#include "devices/device_machines.h"
#include "devices/devices.h"
#include "game/game.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "items/equipment.h"
#include "memory/data.h"
#include "objects/object_types.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "text/text_group.h"
#include "units/dialogue_definitions.h"

#include <string.h>

/* ---------- constants */

/* the game's, which their units keep to themselves */
enum
{
	/* encounters.c */
	MAXIMUM_ENCOUNTERS_PER_SCENARIO = 128,
	MAXIMUM_FIRING_POSITIONS_PER_ENCOUNTER = 512,
	/* encounters.c (squad_datum's required_locations and unused_locations
	are 32 bits each) */
	MAXIMUM_STARTING_LOCATIONS_PER_SQUAD = 32,
	/* action_alert.c (unavailable_positions is 32 bits) */
	MAXIMUM_MOVE_POSITIONS_PER_SQUAD = 32,
	/* encounters.c: a firing position's group is a letter, and indexes
	group_distances_squared */
	NUMBER_OF_FIRING_POSITION_GROUP_LETTERS = 26,
	/* encounters.c */
	NUMBER_OF_ENCOUNTER_SEARCH_BEHAVIORS = 3,
	NUMBER_OF_SQUAD_UNIQUE_LEADER_TYPES = 5,
	/* actions.c (global_ai_default_state_names) */
	NUMBER_OF_ACTOR_DEFAULT_STATES = 12,
	/* encounters.c, encounter_test_rule */
	NUMBER_OF_PLATOON_RULES = 10,
	/* actions.h: the obey action's command indices are bytes, NONE 0xFF */
	MAXIMUM_COMMANDS_PER_COMMAND_LIST = 255,
	/* ai_communication.c (conversation_datum's arrays) */
	MAXIMUM_PARTICIPANTS_PER_CONVERSATION = 8,
	MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT = 6,
	NUMBER_OF_CONVERSATION_PARTICIPANT_SELECTION_TYPES = 8,
	NUMBER_OF_CONVERSATION_LINE_ADDRESS_TYPES = 3,
	/* hs.c */
	MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO = 19001,
	HS_SYNTAX_DATA_SIZE = 0x38 + MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO * 0x14,
	HS_SYNTAX_DATA_SIGNATURE = 'd@t@',
	/* hs.c: the map's globals share the 0x400 globals with the game's 443
	(hs_globals_external.c) */
	MAXIMUM_HS_GLOBALS_PER_SCENARIO = 0x400 - 443,
	/* hs_compile.c: the strings' last bytes are the console's */
	HS_STRING_CONSTANTS_CONSOLE_SIZE = 0x400,
	_hs_syntax_node_primitive_bit = 0,
	/* recorded_animations.c (playback_codec) */
	RECORDED_ANIMATION_VERSION = 4,
	MAXIMUM_UNIT_CONTROL_DATA_VERSION = 4,
	/* recorded_animation_playback.c: an event's type is 6 bits, its time
	delta 2 */
	_recorded_event_end = 1,
	NUMBER_OF_RECORDED_EVENT_TYPES = 23,
	/* scenario.c */
	NUMBER_OF_TRIGGER_VOLUME_TYPES = 2,
	/* ui_widget_group.c */
	NUMBER_OF_SCENARIO_TYPES = 3,
	/* draw_string.c */
	NUMBER_OF_TEXT_JUSTIFICATIONS = 3,

	/* the tool's limits, where the game has none of its own */
	MAXIMUM_PREDICTED_RESOURCES = 1024,
	MAXIMUM_STARTING_PROFILES_PER_SCENARIO = 256,
	MAXIMUM_BSP_SWITCH_TRIGGER_VOLUMES_PER_SCENARIO = 256,
	MAXIMUM_DETAIL_OBJECT_COLLECTION_PALETTE_ENTRIES = 32,
	MAXIMUM_ACTOR_PALETTE_ENTRIES = 64,
	MAXIMUM_ENCOUNTER_PLAYER_STARTING_LOCATIONS = 256,
	MAXIMUM_COMMAND_LISTS_PER_SCENARIO = 256,
	MAXIMUM_POINTS_PER_COMMAND_LIST = 64,
	MAXIMUM_AI_REFERENCES_PER_SCENARIO = 128,
	MAXIMUM_CONVERSATIONS_PER_SCENARIO = 128,
	MAXIMUM_LINES_PER_CONVERSATION = 32,
	/* recorded_animation_definitions.c */
	MAXIMUM_RECORDED_ANIMATIONS_PER_SCENARIO = 1024,
	MAXIMUM_RECORDED_ANIMATION_EVENT_STREAM_SIZE = 0x200000,
	/* hs_scenario_definitions.c */
	MAXIMUM_HS_SCRIPTS_PER_SCENARIO = 512,
	MAXIMUM_HS_REFERENCES_PER_SCENARIO = 256,
	MAXIMUM_HS_SOURCE_FILES_PER_SCENARIO = 8,
	MAXIMUM_HS_SOURCE_FILE_SIZE = 0x40000,
	MAXIMUM_HS_STRING_CONSTANTS_SIZE = 0x40000,
};

/* ---------- structures */

/* a block of tag references (the scenario's skies, the ai's actor
palette) */
struct scenario_tag_reference
{
	struct tag_reference reference;
};

/* each placement's element, as object_types.c strides them (the
scenario_*_datum the place procs take, and what follows them) */

struct scenery_placement
{
	struct scenario_object_datum object;
	struct scenario_object_permutation permutation;
};

/* (bipeds.c, units.c, vehicles.c) */
struct unit_placement
{
	struct scenario_object_datum object;
	struct scenario_object_permutation permutation;
	real body_vitality;
	unsigned long flags;
	byte unused[0x28];
};

struct equipment_placement
{
	struct scenario_equipment_datum equipment;
};

struct machine_placement
{
	struct scenario_machine_datum machine;
	byte unused[0xC];
};

/* (device_controls.c's scenario_control_datum) */
struct control_placement
{
	struct scenario_object_datum object;
	struct scenario_device_datum device;
	word flags;
	short unused;
	short custom_name_index;
	byte unused2[0xA];
};

struct light_fixture_placement
{
	struct scenario_light_fixture_datum light_fixture;
	byte unused[0x10];
};

struct sound_scenery_placement
{
	struct scenario_object_datum object;
};

/* devices.c */
struct scenario_device_group
{
	char name[32];
	real initial_value;
	unsigned long flags;
	long unused[3];
};

/* players.c */
struct scenario_bsp_switch_trigger_volume
{
	short trigger_volume_index;
	short source_structure_bsp_index;
	short destination_structure_bsp_index;
	short cutscene_flag_index;
};

/* cinematics.c */
struct scenario_cutscene_title
{
	long flags;
	char name[TAG_STRING_LENGTH+1];
	long pad24;
	rectangle2d bounds;
	short text_index;
	word style;
	word justification;
	word pad36;
	unsigned long text_flags;
	pixel32 foreground_color;
	pixel32 shadow_color;
	real fade_in_time;
	real up_time;
	real fade_out_time;
	byte unused50[0x10];
};

/* ai_communication.c */
struct scenario_conversation_participant
{
	word pad00;
	word flags;
	short selection_type;
	short actor_type;
	short preexisting_object_name_index;
	short new_attach_object_name_index;
	byte unknown0C[0x0C];
	short dialogue_variants[MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT];
	char ai_index_name[32];
	long runtime_ai_index;
	byte unknown48[0x0C];
};

/* rasterizer_xbox_detail_objects.c */
struct scenario_detail_object_collection_palette_entry
{
	struct tag_reference collection;
	byte reserved10[0x20];
};

struct scenario_conversation_line
{
	word flags;
	short participant_index;
	short address_type;
	short address_participant_index;
	long unknown08;
	real delay_time;
	byte unknown10[0x0C];
	struct tag_reference dialogue[MAXIMUM_DIALOGUE_VARIANTS_PER_CONVERSATION_PARTICIPANT];
};

/* a recorded animation's event stream, as recorded_animation_playback.c and
recorded_animation_playback_v1.c read it: the unit's control (the fields of
each unit control data version up to its own, recorded_animation_initialize.c),
then (version 4) the facing, aiming and looking controllers, then events
up to the end event */
enum
{
	RECORDED_UNIT_CONTROL_VERSION0_SIZE = 1 + 1 + 2 + 2 + 2 + 8 + 12 + 12 + 12,
	RECORDED_ANIMATION_STATE_SIZE = 12,
	RECORDED_EVENT_V1_HEADER_SIZE = 4,
};

typedef char verify_scenery_placement_size[sizeof(struct scenery_placement) == 0x48 ? 1 : -1];
typedef char verify_unit_placement_size[sizeof(struct unit_placement) == 0x78 ? 1 : -1];
typedef char verify_weapon_placement_size[sizeof(struct scenario_weapon_datum) == 0x5C ? 1 : -1];
typedef char verify_equipment_placement_size[sizeof(struct equipment_placement) == 0x28 ? 1 : -1];
typedef char verify_machine_placement_size[sizeof(struct machine_placement) == 0x40 ? 1 : -1];
typedef char verify_control_placement_size[sizeof(struct control_placement) == 0x40 ? 1 : -1];
typedef char verify_light_fixture_placement_size[sizeof(struct light_fixture_placement) == 0x58 ? 1 : -1];
typedef char verify_sound_scenery_placement_size[sizeof(struct sound_scenery_placement) == 0x28 ? 1 : -1];
typedef char verify_scenario_object_palette_entry_size[sizeof(struct scenario_object_palette_entry) == 0x30 ? 1 : -1];
typedef char verify_scenario_object_name_size[sizeof(struct scenario_object_name) == 0x24 ? 1 : -1];
typedef char verify_scenario_detail_object_collection_palette_entry_size[
	sizeof(struct scenario_detail_object_collection_palette_entry) == 0x30 ? 1 : -1];
typedef char verify_encounter_player_starting_location_size[
	sizeof(struct encounter_player_starting_location) == 0x34 ? 1 : -1];
typedef char verify_scenario_device_group_size[sizeof(struct scenario_device_group) == 0x34 ? 1 : -1];
typedef char verify_scenario_bsp_switch_trigger_volume_size[sizeof(struct scenario_bsp_switch_trigger_volume) == 0x8 ? 1 : -1];
typedef char verify_scenario_cutscene_flag_size[sizeof(struct scenario_cutscene_flag) == 0x5C ? 1 : -1];
typedef char verify_scenario_cutscene_camera_point_size[sizeof(struct scenario_cutscene_camera_point) == 0x68 ? 1 : -1];
typedef char verify_scenario_cutscene_title_size[sizeof(struct scenario_cutscene_title) == 0x60 ? 1 : -1];
typedef char verify_scenario_starting_profile_size[sizeof(struct scenario_starting_profile) == 0x68 ? 1 : -1];
typedef char verify_scenario_netgame_flag_size[sizeof(struct scenario_netgame_flag) == 0x94 ? 1 : -1];
typedef char verify_scenario_netgame_equipment_size[sizeof(struct scenario_netgame_equipment) == 0x90 ? 1 : -1];
typedef char verify_scenario_starting_equipment_size[sizeof(struct scenario_starting_equipment) == 0xCC ? 1 : -1];
typedef char verify_encounter_definition_size[sizeof(struct encounter_definition) == 0xB0 ? 1 : -1];
typedef char verify_squad_definition_size[sizeof(struct squad_definition) == 0xE8 ? 1 : -1];
typedef char verify_move_position_definition_size[sizeof(struct move_position_definition) == 0x50 ? 1 : -1];
typedef char verify_firing_position_definition_size[sizeof(struct firing_position_definition) == 0x18 ? 1 : -1];
typedef char verify_ai_command_list_definition_size[sizeof(struct ai_command_list_definition) == 0x60 ? 1 : -1];
typedef char verify_ai_command_definition_size[sizeof(struct ai_command_definition) == 0x20 ? 1 : -1];
typedef char verify_ai_command_point_definition_size[sizeof(struct ai_command_point_definition) == 0x14 ? 1 : -1];
typedef char verify_ai_animation_reference_definition_size[
	sizeof(struct ai_animation_reference_definition) == 0x3C ? 1 : -1];
typedef char verify_ai_script_reference_definition_size[sizeof(struct ai_script_reference_definition) == 0x28 ? 1 : -1];
typedef char verify_ai_recording_reference_definition_size[
	sizeof(struct ai_recording_reference_definition) == 0x28 ? 1 : -1];
typedef char verify_ai_conversation_size[sizeof(struct ai_conversation) == 0x74 ? 1 : -1];
typedef char verify_scenario_conversation_participant_size[
	sizeof(struct scenario_conversation_participant) == 0x54 ? 1 : -1];
typedef char verify_scenario_conversation_line_size[sizeof(struct scenario_conversation_line) == 0x7C ? 1 : -1];
typedef char verify_recorded_animation_definition_size[sizeof(struct recorded_animation_definition) == 0x40 ? 1 : -1];
typedef char verify_hs_script_size[sizeof(struct hs_script) == 0x5C ? 1 : -1];
typedef char verify_hs_global_size[sizeof(struct hs_global) == 0x5C ? 1 : -1];
typedef char verify_hs_reference_size[sizeof(struct hs_reference) == 0x28 ? 1 : -1];
typedef char verify_hs_source_file_size[sizeof(struct hs_source_file) == 0x34 ? 1 : -1];
typedef char verify_hs_syntax_node_size[sizeof(struct hs_syntax_node) == 0x14 ? 1 : -1];
typedef char verify_data_array_size[sizeof(struct data_array) == 0x38 ? 1 : -1];
typedef char verify_scenario_structure_bsp_references_offset[
	offsetof(struct scenario, structure_bsp_references) == 0x5A4 ? 1 : -1];
typedef char verify_scenario_size[sizeof(struct scenario) == 0x5B0 ? 1 : -1];

/* ---------- globals */

/* the script nodes reached so far, and those still to walk
(scenario_check_scripts) */
static unsigned long hs_nodes_reached[(MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO + 31) / 32];
static short hs_nodes_to_walk[MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO];

/* ---------- private code */

/* the placements of a type that has them (object_types.c) */
static struct tag_block const *scenario_object_placements(
	struct scenario const *scenario,
	short object_type)
{
	switch (object_type)
	{
	case _object_type_biped:
		return &scenario->bipeds;
	case _object_type_vehicle:
		return &scenario->vehicles;
	case _object_type_weapon:
		return &scenario->weapons;
	case _object_type_equipment:
		return &scenario->equipment;
	case _object_type_scenery:
		return &scenario->scenery;
	case _object_type_machine:
		return &scenario->machines;
	case _object_type_control:
		return &scenario->controls;
	case _object_type_light_fixture:
		return &scenario->light_fixtures;
	case _object_type_sound_scenery:
		return &scenario->sound_scenery;
	}

	return NULL;
}

/* a predicted resource's tag is of its type, and a bitmap's index one of
its bitmaps (predicted_resources_precache) */
static boolean predicted_resource_check(
	struct tag_validation *validation,
	void *base)
{
	struct predicted_resource *resource = base;

	if (resource->tag_index == NONE)
		return TRUE;
	if (resource->type == _predicted_resource_bitmap)
	{
		struct bitmap_group const *bitmap = tag_validate_tag_get(validation, resource->tag_index, 'bitm');

		if (bitmap && tag_validate_contains(validation, bitmap, sizeof(*bitmap)) &&
			resource->resource_index >= 0 && resource->resource_index < bitmap->bitmaps.count)
		{
			return TRUE;
		}
	}
	else if (tag_validate_tag_get(validation, resource->tag_index, 'snd!'))
	{
		return TRUE;
	}
	tag_validate_correct(validation, "is %08lx's resource %d, which it does not have: none",
		resource->tag_index, resource->resource_index);
	resource->tag_index = NONE;

	return TRUE;
}

/* an object name's placement, which the tools set and object_new_by_name
creates it from: none, or a placement of a type that has them */
static boolean scenario_check_object_names(
	struct tag_validation *validation,
	void *base)
{
	struct scenario *scenario = base;
	long name_index;

	for (name_index = 0; name_index < scenario->object_names.count; name_index++)
	{
		struct scenario_object_name *name = (struct scenario_object_name *)scenario->object_names.address + name_index;
		struct tag_block const *placements = NULL;

		if (name->runtime_object_type == NONE && name->runtime_scenario_datum_index == NONE)
			continue;
		if (name->runtime_object_type >= 0 && name->runtime_object_type < NUMBER_OF_OBJECT_TYPES)
			placements = scenario_object_placements(scenario, name->runtime_object_type);
		if (placements &&
			name->runtime_scenario_datum_index >= 0 && name->runtime_scenario_datum_index < placements->count)
		{
			continue;
		}
		tag_validate_correct(validation, "object name %ld is placement %d of type %d, which is not there: none",
			name_index, name->runtime_scenario_datum_index, name->runtime_object_type);
		name->runtime_object_type = NONE;
		name->runtime_scenario_datum_index = NONE;
	}

	return TRUE;
}

/* whether index is one of the syntax nodes in use (memory/data.c,
datum_get) */
static boolean hs_node_valid(
	struct data_array const *nodes,
	long index)
{
	short absolute_index = (short)index;
	short identifier = (short)(index >> 16);
	struct hs_syntax_node const *node;

	if (absolute_index < 0 || absolute_index >= nodes->count)
		return FALSE;
	node = (struct hs_syntax_node const *)(nodes + 1) + absolute_index;

	return node->datum_header && (!identifier || identifier == node->datum_header);
}

/* a link to a node (a script's or global's root, a call's first node, the
next node): none, or a node no other link reaches */
static boolean hs_node_link(
	struct data_array const *nodes,
	long index,
	boolean none_allowed)
{
	short absolute_index = (short)index;

	if (index == NONE)
		return none_allowed;
	if (!hs_node_valid(nodes, index) ||
		TEST_FLAG(hs_nodes_reached[absolute_index / 32], absolute_index % 32))
	{
		return FALSE;
	}
	SET_FLAG(hs_nodes_reached[absolute_index / 32], absolute_index % 32, TRUE);

	return TRUE;
}

/* the scripts' syntax: hs.c checks its header and hs_compile_postprocess
each node's type, function, script and constants, but the runtime follows
the links between nodes as they are. Every link must name a node in use, no
node may be reached twice, and every node in use must be reached from a
script's or global's root or a node no link reaches (so that none is in a
cycle). If not, no script runs (the syntax data is let go of; hs.c then
runs none, where it finds one script damaged it stops only that one) */
static boolean scenario_check_scripts(
	struct tag_validation *validation,
	void *base)
{
	struct scenario *scenario = base;
	struct data_array const *nodes = scenario->hs_syntax_data.address;
	struct hs_syntax_node const *node_data;
	long walk_count;
	long index;
	char const *error = NULL;

	/* (hs.c finds a header that is not its own damaged, and runs no
	scripts) */
	if (scenario->hs_syntax_data.size != HS_SYNTAX_DATA_SIZE ||
		nodes->signature != HS_SYNTAX_DATA_SIGNATURE ||
		nodes->maximum_count != MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO ||
		nodes->size != sizeof(struct hs_syntax_node) ||
		nodes->count < 0 || nodes->count > nodes->maximum_count)
	{
		return TRUE;
	}
	node_data = (struct hs_syntax_node const *)(nodes + 1);

	memset(hs_nodes_reached, 0, sizeof(hs_nodes_reached));
	for (index = 0; index < scenario->hs_scripts.count && !error; index++)
	{
		struct hs_script const *script = (struct hs_script const *)scenario->hs_scripts.address + index;

		if (!hs_node_link(nodes, script->root_expression_index, TRUE))
			error = "a script's root is not a node, or is another's";
	}
	for (index = 0; index < scenario->hs_globals.count && !error; index++)
	{
		struct hs_global const *global = (struct hs_global const *)scenario->hs_globals.address + index;

		if (!hs_node_link(nodes, global->initialization_expression_index, TRUE))
			error = "a global's root is not a node, or is another's";
	}
	for (index = 0; index < nodes->count && !error; index++)
	{
		struct hs_syntax_node const *node = &node_data[index];

		if (!node->datum_header)
			continue;
		if (!hs_node_link(nodes, node->next_node_index, TRUE) ||
			(!TEST_FLAG(node->flags, _hs_syntax_node_primitive_bit) && !hs_node_link(nodes, node->data, FALSE)))
		{
			error = "a node links to one that is not a node, or is another's";
		}
	}

	/* (every node is reached at most once, so the walk takes each once) */
	walk_count = 0;
	for (index = 0; index < nodes->count && !error; index++)
	{
		if (node_data[index].datum_header && !TEST_FLAG(hs_nodes_reached[index / 32], index % 32))
			hs_nodes_to_walk[walk_count++] = (short)index;
	}
	for (index = 0; index < scenario->hs_scripts.count && !error; index++)
	{
		long root = ((struct hs_script const *)scenario->hs_scripts.address)[index].root_expression_index;

		if (root != NONE)
			hs_nodes_to_walk[walk_count++] = (short)root;
	}
	for (index = 0; index < scenario->hs_globals.count && !error; index++)
	{
		long root = ((struct hs_global const *)scenario->hs_globals.address)[index].initialization_expression_index;

		if (root != NONE)
			hs_nodes_to_walk[walk_count++] = (short)root;
	}
	memset(hs_nodes_reached, 0, sizeof(hs_nodes_reached));
	while (walk_count > 0 && !error)
	{
		short node_index = hs_nodes_to_walk[--walk_count];
		struct hs_syntax_node const *node = &node_data[node_index];

		SET_FLAG(hs_nodes_reached[node_index / 32], node_index % 32, TRUE);
		if (node->next_node_index != NONE)
			hs_nodes_to_walk[walk_count++] = (short)node->next_node_index;
		if (!TEST_FLAG(node->flags, _hs_syntax_node_primitive_bit))
			hs_nodes_to_walk[walk_count++] = (short)node->data;
	}
	for (index = 0; index < nodes->count && !error; index++)
	{
		if (node_data[index].datum_header && !TEST_FLAG(hs_nodes_reached[index / 32], index % 32))
			error = "nodes are in a cycle";
	}

	if (error)
	{
		tag_validate_correct(validation, "has scripts whose syntax is damaged (%s): no script runs", error);
		scenario->hs_syntax_data.size = 0;
		scenario->hs_syntax_data.address = NULL;
		scenario->hs_scripts.count = 0;
		scenario->hs_scripts.address = NULL;
		scenario->hs_globals.count = 0;
		scenario->hs_globals.address = NULL;
	}

	/* the strings a node names end before the console's bytes (hs.c checks
	each node's offset is before them) */
	if (scenario->hs_string_constants.size > HS_STRING_CONSTANTS_CONSOLE_SIZE)
	{
		char *strings = scenario->hs_string_constants.address;
		long end = scenario->hs_string_constants.size - HS_STRING_CONSTANTS_CONSOLE_SIZE - 1;

		if (strings[end])
		{
			tag_validate_correct(validation, "has script strings that do not end: ended");
			strings[end] = 0;
		}
	}

	return TRUE;
}

/* the bytes an event of a recorded animation of version 4 has after its
header (recorded_animation_playback.c's apply_funcs) */
static long recorded_event_size(
	long event_type)
{
	if (event_type < 2)
		return 0;
	if (event_type < 4)
		return 1;
	if (event_type < 6)
		return 2;
	if (event_type < 7)
		return 8;
	if (event_type < 15)
		return 2;

	return 4;
}

/* the same, with its header, of versions 1 to 3
(recorded_animation_playback_v1.c) */
static long recorded_event_v1_size(
	long event_type)
{
	if (event_type < 2 || event_type == 7 || event_type == 8)
		return 4;
	if (event_type < 6)
		return 6;
	if (event_type < 7 || event_type >= 16)
		return 12;

	return 16;
}

/* a dry run of an event stream, as the game plays it: NULL if it is
playable (its version known, every event of a known type and inside the
stream, up to the end event), what is wrong otherwise. (A time delta the
game asserts is short or long enough plays as it is) */
static char const *recorded_animation_decode(
	struct recorded_animation_definition const *animation)
{
	byte const *stream = animation->event_stream.address;
	long size = animation->event_stream.size;
	long offset;
	short version_index;

	if (animation->version < 1 || animation->version > RECORDED_ANIMATION_VERSION)
		return "has an unknown version";
	if (animation->unit_control_data_version > MAXIMUM_UNIT_CONTROL_DATA_VERSION)
		return "has an unknown unit control version";

	/* (recorded_animation_initialize_unit_control) */
	offset = RECORDED_UNIT_CONTROL_VERSION0_SIZE;
	for (version_index = 1; version_index < animation->unit_control_data_version; version_index++)
		offset += version_index == 1 ? 4 : 2;
	if (animation->version == RECORDED_ANIMATION_VERSION)
		offset += RECORDED_ANIMATION_STATE_SIZE;

	for (;;)
	{
		long event_type;

		if (animation->version == RECORDED_ANIMATION_VERSION)
		{
			byte header;

			if (offset >= size)
				return "ends inside an event";
			header = stream[offset];
			event_type = header >> 2;
			/* (its time since the last: none or a tick, a byte or a word) */
			switch (header & 3)
			{
			case 0:
			case 1:
				offset += 1;
				break;
			case 2:
				if (size - offset < 2)
					return "ends inside an event";
				offset += 2;
				break;
			default:
				if (size - offset < 3)
					return "ends inside an event";
				offset += 3;
				break;
			}
			if (event_type >= NUMBER_OF_RECORDED_EVENT_TYPES)
				return "has an event of an unknown type";
			if (event_type != _recorded_event_end)
				offset += recorded_event_size(event_type);
		}
		else
		{
			if (size - offset < RECORDED_EVENT_V1_HEADER_SIZE)
				return "ends inside an event";
			event_type = *(short const *)(stream + offset);
			if (event_type < 0 || event_type >= NUMBER_OF_RECORDED_EVENT_TYPES)
				return "has an event of an unknown type";
			if (event_type != _recorded_event_end)
				offset += recorded_event_v1_size(event_type);
		}
		if (offset > size)
			return "ends inside an event";
		if (event_type == _recorded_event_end)
			break;
	}

	return NULL;
}

/* a recorded animation that cannot be played back is made one the game
does not play (recorded_animations.c plays only a known version) */
static boolean recorded_animation_check(
	struct tag_validation *validation,
	void *base)
{
	struct recorded_animation_definition *animation = base;
	char const *error;

	/* (one already marked unplayable, as below, stays so) */
	if (!animation->version && !animation->length_in_ticks && !animation->event_stream.size &&
		!animation->event_stream.address)
	{
		return TRUE;
	}
	error = recorded_animation_decode(animation);
	if (error)
	{
		tag_validate_correct(validation, "%s: not played", error);
		animation->version = 0;
		animation->length_in_ticks = 0;
		animation->event_stream.size = 0;
		animation->event_stream.address = NULL;
	}

	return TRUE;
}

/* the placements' object part: its palette entry (none, or one of its
type's palette) and its name */
#define SCENARIO_OBJECT_FIELDS(type, object, palette) \
	TAG_SCHEMA_BLOCK_INDEX(type, object.palette_entry_index, TAG_SCHEMA_ROOT, \
		offsetof(struct scenario, palette), FLAG(_tag_schema_none_bit)), \
	TAG_SCHEMA_BLOCK_INDEX(type, object.name_index, TAG_SCHEMA_ROOT, \
		offsetof(struct scenario, object_names), FLAG(_tag_schema_none_bit))

/* a device's groups (devices.c, device_add_scenario_information): none, or
the scenario's */
#define SCENARIO_DEVICE_FIELDS(type, device) \
	TAG_SCHEMA_BLOCK_INDEX(type, device.power_group_index, TAG_SCHEMA_ROOT, \
		offsetof(struct scenario, device_groups), FLAG(_tag_schema_none_bit)), \
	TAG_SCHEMA_BLOCK_INDEX(type, device.position_group_index, TAG_SCHEMA_ROOT, \
		offsetof(struct scenario, device_groups), FLAG(_tag_schema_none_bit))

/* ---------- schemas */

static struct tag_schema_field const empty_fields[] =
{
	TAG_SCHEMA_END
};

/* sky references, predicted resources */

static struct tag_schema_field const sky_reference_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct scenario_tag_reference, reference, TAG_SCHEMA_GROUPS('sky ')),
	TAG_SCHEMA_END
};

static struct tag_schema_field const predicted_resource_fields[] =
{
	TAG_SCHEMA_ENUM(struct predicted_resource, type, _predicted_resource_sound + 1, 0),
	TAG_SCHEMA_TAG_INDEX(struct predicted_resource, tag_index, TAG_SCHEMA_GROUPS('bitm', 'snd!')),
	TAG_SCHEMA_CHECK(predicted_resource_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sky_reference_schema =
	TAG_SCHEMA_DEFINITION(sky_reference, struct scenario_tag_reference, sky_reference_fields);
static struct tag_schema_definition const predicted_resource_schema =
	TAG_SCHEMA_DEFINITION(predicted_resource, struct predicted_resource, predicted_resource_fields);

/* object names, placements and palettes */

static struct tag_schema_field const object_name_fields[] =
{
	TAG_SCHEMA_STRING(struct scenario_object_name, name),
	TAG_SCHEMA_END
};

static struct tag_schema_field const scenery_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct scenery_placement, object, scenery_palette),
	TAG_SCHEMA_END
};

static struct tag_schema_field const biped_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct unit_placement, object, biped_palette),
	TAG_SCHEMA_END
};

static struct tag_schema_field const vehicle_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct unit_placement, object, vehicle_palette),
	TAG_SCHEMA_END
};

static struct tag_schema_field const equipment_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct equipment_placement, equipment.object, equipment_palette),
	TAG_SCHEMA_END
};

static struct tag_schema_field const weapon_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct scenario_weapon_datum, object, weapon_palette),
	TAG_SCHEMA_END
};

static struct tag_schema_field const machine_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct machine_placement, machine.object, machine_palette),
	SCENARIO_DEVICE_FIELDS(struct machine_placement, machine.device),
	TAG_SCHEMA_END
};

static struct tag_schema_field const control_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct control_placement, object, control_palette),
	SCENARIO_DEVICE_FIELDS(struct control_placement, device),
	TAG_SCHEMA_END
};

static struct tag_schema_field const light_fixture_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct light_fixture_placement, light_fixture.object, light_fixtures_palette),
	SCENARIO_DEVICE_FIELDS(struct light_fixture_placement, light_fixture.device),
	TAG_SCHEMA_END
};

static struct tag_schema_field const sound_scenery_fields[] =
{
	SCENARIO_OBJECT_FIELDS(struct sound_scenery_placement, object, sound_scenery_palette),
	TAG_SCHEMA_END
};

/* (each type's palette names its type's definitions) */
#define SCENARIO_PALETTE_SCHEMA(name, group) \
	static struct tag_schema_field const name##_palette_fields[] = \
	{ \
		TAG_SCHEMA_REFERENCE(struct scenario_object_palette_entry, reference, TAG_SCHEMA_GROUPS(group)), \
		TAG_SCHEMA_END \
	}; \
	static struct tag_schema_definition const name##_palette_schema = \
		TAG_SCHEMA_DEFINITION(name##_palette, struct scenario_object_palette_entry, name##_palette_fields)

SCENARIO_PALETTE_SCHEMA(scenery, 'scen');
SCENARIO_PALETTE_SCHEMA(biped, 'bipd');
SCENARIO_PALETTE_SCHEMA(vehicle, 'vehi');
SCENARIO_PALETTE_SCHEMA(equipment, 'eqip');
SCENARIO_PALETTE_SCHEMA(weapon, 'weap');
SCENARIO_PALETTE_SCHEMA(machine, 'mach');
SCENARIO_PALETTE_SCHEMA(control, 'ctrl');
SCENARIO_PALETTE_SCHEMA(light_fixture, 'lifi');
SCENARIO_PALETTE_SCHEMA(sound_scenery, 'ssce');

static struct tag_schema_definition const object_name_schema =
	TAG_SCHEMA_DEFINITION(object_name, struct scenario_object_name, object_name_fields);
static struct tag_schema_definition const scenery_schema =
	TAG_SCHEMA_DEFINITION(scenery, struct scenery_placement, scenery_fields);
static struct tag_schema_definition const biped_schema =
	TAG_SCHEMA_DEFINITION(biped, struct unit_placement, biped_fields);
static struct tag_schema_definition const vehicle_schema =
	TAG_SCHEMA_DEFINITION(vehicle, struct unit_placement, vehicle_fields);
static struct tag_schema_definition const equipment_schema =
	TAG_SCHEMA_DEFINITION(equipment, struct equipment_placement, equipment_fields);
static struct tag_schema_definition const weapon_schema =
	TAG_SCHEMA_DEFINITION(weapon, struct scenario_weapon_datum, weapon_fields);
static struct tag_schema_definition const machine_schema =
	TAG_SCHEMA_DEFINITION(machine, struct machine_placement, machine_fields);
static struct tag_schema_definition const control_schema =
	TAG_SCHEMA_DEFINITION(control, struct control_placement, control_fields);
static struct tag_schema_definition const light_fixture_schema =
	TAG_SCHEMA_DEFINITION(light_fixture, struct light_fixture_placement, light_fixture_fields);
static struct tag_schema_definition const sound_scenery_schema =
	TAG_SCHEMA_DEFINITION(sound_scenery, struct sound_scenery_placement, sound_scenery_fields);

/* device groups, starting profiles, player starting locations, trigger
volumes */

static struct tag_schema_field const device_group_fields[] =
{
	TAG_SCHEMA_STRING(struct scenario_device_group, name),
	TAG_SCHEMA_END
};

static struct tag_schema_field const starting_profile_fields[] =
{
	TAG_SCHEMA_STRING(struct scenario_starting_profile, name),
	TAG_SCHEMA_REFERENCE(struct scenario_starting_profile, primary_weapon.weapon, TAG_SCHEMA_GROUPS('weap')),
	TAG_SCHEMA_REFERENCE(struct scenario_starting_profile, secondary_weapon.weapon, TAG_SCHEMA_GROUPS('weap')),
	TAG_SCHEMA_END
};

static struct tag_schema_field const trigger_volume_fields[] =
{
	TAG_SCHEMA_ENUM(struct scenario_trigger_volume, type, NUMBER_OF_TRIGGER_VOLUME_TYPES, 0),
	TAG_SCHEMA_STRING(struct scenario_trigger_volume, name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const device_group_schema =
	TAG_SCHEMA_DEFINITION(device_group, struct scenario_device_group, device_group_fields);
static struct tag_schema_definition const starting_profile_schema =
	TAG_SCHEMA_DEFINITION(starting_profile, struct scenario_starting_profile, starting_profile_fields);
static struct tag_schema_definition const player_starting_location_schema =
	TAG_SCHEMA_DEFINITION(player_starting_location, struct player_starting_location, empty_fields);
static struct tag_schema_definition const trigger_volume_schema =
	TAG_SCHEMA_DEFINITION(trigger_volume, struct scenario_trigger_volume, trigger_volume_fields);

/* recorded animations */

static struct tag_schema_field const recorded_animation_fields[] =
{
	TAG_SCHEMA_STRING(struct recorded_animation_definition, name),
	TAG_SCHEMA_DATA(struct recorded_animation_definition, event_stream, MAXIMUM_RECORDED_ANIMATION_EVENT_STREAM_SIZE),
	TAG_SCHEMA_CHECK(recorded_animation_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const recorded_animation_schema =
	TAG_SCHEMA_DEFINITION(recorded_animation, struct recorded_animation_definition, recorded_animation_fields);

/* netgame flags and equipment */

static struct tag_schema_field const netgame_flag_fields[] =
{
	TAG_SCHEMA_ENUM(struct scenario_netgame_flag, type, NUMBER_OF_NETGAME_FLAG_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_field const netgame_equipment_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct scenario_netgame_equipment, item_collection, TAG_SCHEMA_GROUPS('itmc')),
	TAG_SCHEMA_END
};

static struct tag_schema_field const starting_equipment_fields[] =
{
	TAG_SCHEMA_REFERENCE_ARRAY(struct scenario_starting_equipment, item_collection, TAG_SCHEMA_GROUPS('itmc')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const netgame_flag_schema =
	TAG_SCHEMA_DEFINITION(netgame_flag, struct scenario_netgame_flag, netgame_flag_fields);
static struct tag_schema_definition const netgame_equipment_schema =
	TAG_SCHEMA_DEFINITION(netgame_equipment, struct scenario_netgame_equipment, netgame_equipment_fields);
static struct tag_schema_definition const starting_equipment_schema =
	TAG_SCHEMA_DEFINITION(starting_equipment, struct scenario_starting_equipment, starting_equipment_fields);

/* structure bsp switches, decals' and detail objects' palettes */

static struct tag_schema_field const bsp_switch_trigger_volume_fields[] =
{
	/* (its trigger volume and bsps are only compared, or checked where
	they are used: scenario_trigger_volume_test_point,
	main_switch_structure_bsp) */
	TAG_SCHEMA_BLOCK_INDEX(struct scenario_bsp_switch_trigger_volume, cutscene_flag_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, cutscene_flags), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_field const decal_palette_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct scenario_decal_palette_entry, reference, TAG_SCHEMA_GROUPS('deca')),
	TAG_SCHEMA_END
};

static struct tag_schema_field const detail_object_collection_palette_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct scenario_detail_object_collection_palette_entry, collection, TAG_SCHEMA_GROUPS('dobc')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const bsp_switch_trigger_volume_schema =
	TAG_SCHEMA_DEFINITION(bsp_switch_trigger_volume, struct scenario_bsp_switch_trigger_volume,
		bsp_switch_trigger_volume_fields);
static struct tag_schema_definition const decal_palette_schema =
	TAG_SCHEMA_DEFINITION(decal_palette, struct scenario_decal_palette_entry, decal_palette_fields);
static struct tag_schema_definition const detail_object_collection_palette_schema =
	TAG_SCHEMA_DEFINITION(detail_object_collection_palette, struct scenario_detail_object_collection_palette_entry,
		detail_object_collection_palette_fields);

/* ai encounters: squads (their move positions and starting locations),
platoons, firing positions. Their clusters and surfaces are the structure
bsp's, which is not loaded as the scenario is checked: the game checks them
as it uses them */

static struct tag_schema_field const actor_palette_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct scenario_tag_reference, reference, TAG_SCHEMA_GROUPS('actv')),
	TAG_SCHEMA_END
};

static struct tag_schema_field const move_position_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct move_position_definition, animation_reference_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_animation_references), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_field const actor_starting_location_fields[] =
{
	TAG_SCHEMA_ENUM(struct actor_starting_location, default_state, NUMBER_OF_ACTOR_DEFAULT_STATES,
		FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct actor_starting_location, initial_state, NUMBER_OF_ACTOR_DEFAULT_STATES,
		FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct actor_starting_location, actor_variant_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_actor_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct actor_starting_location, command_list_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_command_lists), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const actor_palette_schema =
	TAG_SCHEMA_DEFINITION(actor_palette, struct scenario_tag_reference, actor_palette_fields);
static struct tag_schema_definition const move_position_schema =
	TAG_SCHEMA_DEFINITION(move_position, struct move_position_definition, move_position_fields);
static struct tag_schema_definition const actor_starting_location_schema =
	TAG_SCHEMA_DEFINITION(actor_starting_location, struct actor_starting_location, actor_starting_location_fields);

/* (a squad's platoon and maneuver squad are left out: the retail maps have
squads whose platoon or maneuver squad was deleted, which encounters.c
takes as none) */
static struct tag_schema_field const squad_fields[] =
{
	TAG_SCHEMA_STRING(struct squad_definition, name),
	TAG_SCHEMA_BLOCK_INDEX(struct squad_definition, actor_palette_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_actor_palette), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct squad_definition, initial_state, NUMBER_OF_ACTOR_DEFAULT_STATES, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct squad_definition, default_state, NUMBER_OF_ACTOR_DEFAULT_STATES, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct squad_definition, unique_leader_type, NUMBER_OF_SQUAD_UNIQUE_LEADER_TYPES, 0),
	TAG_SCHEMA_ENUM(struct squad_definition, major_upgrade, NUMBER_OF_ACTOR_MAJOR_UPGRADES, 0),
	TAG_SCHEMA_BLOCK(struct squad_definition, move_positions, move_position_schema, MAXIMUM_MOVE_POSITIONS_PER_SQUAD),
	TAG_SCHEMA_BLOCK(struct squad_definition, starting_locations, actor_starting_location_schema,
		MAXIMUM_STARTING_LOCATIONS_PER_SQUAD),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const squad_schema =
	TAG_SCHEMA_DEFINITION(squad, struct squad_definition, squad_fields);

/* (a rule's platoon is left out: the retail maps have deleted ones, which
encounter_test_rule takes as the whole encounter) */
static struct tag_schema_field const platoon_rule_fields[] =
{
	TAG_SCHEMA_ENUM(struct platoon_rule, rule_type, NUMBER_OF_PLATOON_RULES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const platoon_rule_schema =
	TAG_SCHEMA_DEFINITION(platoon_rule, struct platoon_rule, platoon_rule_fields);

static struct tag_schema_field const platoon_fields[] =
{
	TAG_SCHEMA_STRING(struct platoon_definition, name),
	TAG_SCHEMA_STRUCT(struct platoon_definition, attacking_defending_rule, platoon_rule_schema),
	TAG_SCHEMA_STRUCT(struct platoon_definition, maneuvering_rule, platoon_rule_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_field const firing_position_fields[] =
{
	TAG_SCHEMA_ENUM(struct firing_position_definition, group_index, NUMBER_OF_FIRING_POSITION_GROUP_LETTERS, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const platoon_schema =
	TAG_SCHEMA_DEFINITION(platoon, struct platoon_definition, platoon_fields);
static struct tag_schema_definition const firing_position_schema =
	TAG_SCHEMA_DEFINITION(firing_position, struct firing_position_definition, firing_position_fields);
static struct tag_schema_definition const encounter_player_starting_location_schema =
	TAG_SCHEMA_DEFINITION(encounter_player_starting_location, struct encounter_player_starting_location,
		empty_fields);

static struct tag_schema_field const encounter_fields[] =
{
	TAG_SCHEMA_STRING(struct encounter_definition, name),
	TAG_SCHEMA_ENUM(struct encounter_definition, team_index, NUMBER_OF_SOLO_CAMPAIGN_TEAMS, 0),
	TAG_SCHEMA_ENUM(struct encounter_definition, searching, NUMBER_OF_ENCOUNTER_SEARCH_BEHAVIORS, 0),
	TAG_SCHEMA_BLOCK(struct encounter_definition, squads, squad_schema, MAXIMUM_SQUADS_PER_ENCOUNTER),
	TAG_SCHEMA_BLOCK(struct encounter_definition, platoons, platoon_schema, MAXIMUM_PLATOONS_PER_ENCOUNTER),
	TAG_SCHEMA_BLOCK(struct encounter_definition, firing_positions, firing_position_schema,
		MAXIMUM_FIRING_POSITIONS_PER_ENCOUNTER),
	TAG_SCHEMA_BLOCK(struct encounter_definition, player_starting_locations, encounter_player_starting_location_schema,
		MAXIMUM_ENCOUNTER_PLAYER_STARTING_LOCATIONS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const encounter_schema =
	TAG_SCHEMA_DEFINITION(encounter, struct encounter_definition, encounter_fields);

/* ai command lists. The points' surfaces are the structure bsp's (the
game checks them as it uses them) */

/* (a command's points and loop target, its list's, are left out: the
retail maps have commands whose points were deleted, and action_obey.c
checks each as it uses it) */
static struct tag_schema_field const ai_command_fields[] =
{
	TAG_SCHEMA_ENUM(struct ai_command_definition, atom_type, NUMBER_OF_AI_ATOM_TYPES, 0),
	TAG_SCHEMA_BLOCK_INDEX(struct ai_command_definition, animation_reference_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_animation_references), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct ai_command_definition, script_reference_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_script_references), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct ai_command_definition, recording_reference_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, ai_recording_references), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct ai_command_definition, object_name_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, object_names), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ai_command_schema =
	TAG_SCHEMA_DEFINITION(ai_command, struct ai_command_definition, ai_command_fields);
static struct tag_schema_definition const ai_command_point_schema =
	TAG_SCHEMA_DEFINITION(ai_command_point, struct ai_command_point_definition, empty_fields);

static struct tag_schema_field const ai_command_list_fields[] =
{
	TAG_SCHEMA_STRING(struct ai_command_list_definition, name),
	TAG_SCHEMA_BLOCK(struct ai_command_list_definition, commands, ai_command_schema, MAXIMUM_COMMANDS_PER_COMMAND_LIST),
	TAG_SCHEMA_BLOCK(struct ai_command_list_definition, points, ai_command_point_schema, MAXIMUM_POINTS_PER_COMMAND_LIST),
	TAG_SCHEMA_END
};

static struct tag_schema_field const ai_animation_reference_fields[] =
{
	TAG_SCHEMA_STRING(struct ai_animation_reference_definition, animation_name),
	TAG_SCHEMA_REFERENCE(struct ai_animation_reference_definition, animation_graph, TAG_SCHEMA_GROUPS('antr')),
	TAG_SCHEMA_END
};

static struct tag_schema_field const ai_script_reference_fields[] =
{
	TAG_SCHEMA_STRING(struct ai_script_reference_definition, script_name),
	TAG_SCHEMA_END
};

static struct tag_schema_field const ai_recording_reference_fields[] =
{
	TAG_SCHEMA_STRING(struct ai_recording_reference_definition, recording_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ai_command_list_schema =
	TAG_SCHEMA_DEFINITION(ai_command_list, struct ai_command_list_definition, ai_command_list_fields);
static struct tag_schema_definition const ai_animation_reference_schema =
	TAG_SCHEMA_DEFINITION(ai_animation_reference, struct ai_animation_reference_definition,
		ai_animation_reference_fields);
static struct tag_schema_definition const ai_script_reference_schema =
	TAG_SCHEMA_DEFINITION(ai_script_reference, struct ai_script_reference_definition, ai_script_reference_fields);
static struct tag_schema_definition const ai_recording_reference_schema =
	TAG_SCHEMA_DEFINITION(ai_recording_reference, struct ai_recording_reference_definition,
		ai_recording_reference_fields);

/* ai conversations */

static struct tag_schema_field const conversation_participant_fields[] =
{
	TAG_SCHEMA_ENUM(struct scenario_conversation_participant, selection_type,
		NUMBER_OF_CONVERSATION_PARTICIPANT_SELECTION_TYPES, 0),
	TAG_SCHEMA_BLOCK_INDEX(struct scenario_conversation_participant, preexisting_object_name_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, object_names), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct scenario_conversation_participant, new_attach_object_name_index, TAG_SCHEMA_ROOT,
		offsetof(struct scenario, object_names), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

/* (a line's participants are its conversation's) */
static struct tag_schema_field const conversation_line_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct scenario_conversation_line, participant_index, 1,
		offsetof(struct ai_conversation, participants), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct scenario_conversation_line, address_type, NUMBER_OF_CONVERSATION_LINE_ADDRESS_TYPES, 0),
	TAG_SCHEMA_BLOCK_INDEX(struct scenario_conversation_line, address_participant_index, 1,
		offsetof(struct ai_conversation, participants), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_REFERENCE_ARRAY(struct scenario_conversation_line, dialogue, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const conversation_participant_schema =
	TAG_SCHEMA_DEFINITION(conversation_participant, struct scenario_conversation_participant,
		conversation_participant_fields);
static struct tag_schema_definition const conversation_line_schema =
	TAG_SCHEMA_DEFINITION(conversation_line, struct scenario_conversation_line, conversation_line_fields);

static struct tag_schema_field const conversation_fields[] =
{
	TAG_SCHEMA_STRING(struct ai_conversation, name),
	TAG_SCHEMA_BLOCK(struct ai_conversation, participants, conversation_participant_schema,
		MAXIMUM_PARTICIPANTS_PER_CONVERSATION),
	TAG_SCHEMA_BLOCK(struct ai_conversation, lines, conversation_line_schema, MAXIMUM_LINES_PER_CONVERSATION),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const conversation_schema =
	TAG_SCHEMA_DEFINITION(conversation, struct ai_conversation, conversation_fields);

/* scripts (their syntax and strings: scenario_check_scripts) */

static struct tag_schema_field const hs_script_fields[] =
{
	TAG_SCHEMA_STRING(struct hs_script, name),
	TAG_SCHEMA_ENUM(struct hs_script, script_type, NUMBER_OF_HS_SCRIPT_TYPES, 0),
	TAG_SCHEMA_ENUM(struct hs_script, return_type, NUMBER_OF_HS_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_field const hs_global_fields[] =
{
	TAG_SCHEMA_STRING(struct hs_global, name),
	TAG_SCHEMA_ENUM(struct hs_global, type, NUMBER_OF_HS_TYPES, 0),
	TAG_SCHEMA_END
};

/* (a script's tag constant may name a tag of any group: hs_compile.c
matches the group itself) */
static struct tag_schema_field const hs_reference_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct hs_reference, reference, NULL),
	TAG_SCHEMA_END
};

static struct tag_schema_field const hs_source_file_fields[] =
{
	TAG_SCHEMA_STRING(struct hs_source_file, name),
	TAG_SCHEMA_DATA(struct hs_source_file, source, MAXIMUM_HS_SOURCE_FILE_SIZE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hs_script_schema =
	TAG_SCHEMA_DEFINITION(hs_script, struct hs_script, hs_script_fields);
static struct tag_schema_definition const hs_global_schema =
	TAG_SCHEMA_DEFINITION(hs_global, struct hs_global, hs_global_fields);
static struct tag_schema_definition const hs_reference_schema =
	TAG_SCHEMA_DEFINITION(hs_reference, struct hs_reference, hs_reference_fields);
static struct tag_schema_definition const hs_source_file_schema =
	TAG_SCHEMA_DEFINITION(hs_source_file, struct hs_source_file, hs_source_file_fields);

/* cutscenes */

static struct tag_schema_field const cutscene_flag_fields[] =
{
	TAG_SCHEMA_STRING(struct scenario_cutscene_flag, name),
	TAG_SCHEMA_END
};

static struct tag_schema_field const cutscene_camera_point_fields[] =
{
	TAG_SCHEMA_STRING(struct scenario_cutscene_camera_point, name),
	TAG_SCHEMA_END
};

/* (a title's style is one more than its text style, 0 for plain:
cinematics.c) */
static struct tag_schema_field const cutscene_title_fields[] =
{
	TAG_SCHEMA_STRING(struct scenario_cutscene_title, name),
	TAG_SCHEMA_ENUM(struct scenario_cutscene_title, style, 1 + NUMBER_OF_TEXT_STYLES, 0),
	TAG_SCHEMA_ENUM(struct scenario_cutscene_title, justification, NUMBER_OF_TEXT_JUSTIFICATIONS, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const cutscene_flag_schema =
	TAG_SCHEMA_DEFINITION(cutscene_flag, struct scenario_cutscene_flag, cutscene_flag_fields);
static struct tag_schema_definition const cutscene_camera_point_schema =
	TAG_SCHEMA_DEFINITION(cutscene_camera_point, struct scenario_cutscene_camera_point, cutscene_camera_point_fields);
static struct tag_schema_definition const cutscene_title_schema =
	TAG_SCHEMA_DEFINITION(cutscene_title, struct scenario_cutscene_title, cutscene_title_fields);

/* structure bsps (their file offset, size and address are checked as each
loads: cache_files.c) */

static struct tag_schema_field const structure_bsp_reference_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct scenario_structure_bsp_reference, structure_bsp, TAG_SCHEMA_GROUPS('sbsp')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const structure_bsp_reference_schema =
	TAG_SCHEMA_DEFINITION(structure_bsp_reference, struct scenario_structure_bsp_reference,
		structure_bsp_reference_fields);

/* the scenario */

static struct tag_schema_field const scenario_fields[] =
{
	TAG_SCHEMA_BLOCK(struct scenario, sky_references, sky_reference_schema, MAXIMUM_SKIES_PER_SCENARIO),
	TAG_SCHEMA_ENUM(struct scenario, type, NUMBER_OF_SCENARIO_TYPES, 0),
	TAG_SCHEMA_BLOCK(struct scenario, predicted_ui_resources, predicted_resource_schema, MAXIMUM_PREDICTED_RESOURCES),
	TAG_SCHEMA_BLOCK(struct scenario, object_names, object_name_schema, MAXIMUM_OBJECT_NAMES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, scenery, scenery_schema, MAXIMUM_SCENERY_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, scenery_palette, scenery_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, bipeds, biped_schema, MAXIMUM_BIPED_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, biped_palette, biped_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	/* (the game places them as many as there are: object_types_place_all) */
	TAG_SCHEMA_TOOL_BLOCK(struct scenario, vehicles, vehicle_schema, MAXIMUM_VEHICLE_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, vehicle_palette, vehicle_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, equipment, equipment_schema, MAXIMUM_EQUIPMENT_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, equipment_palette, equipment_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, weapons, weapon_schema, MAXIMUM_WEAPON_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, weapon_palette, weapon_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, device_groups, device_group_schema, MAXIMUM_DEVICE_GROUPS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, machines, machine_schema, MAXIMUM_MACHINE_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, machine_palette, machine_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, controls, control_schema, MAXIMUM_CONTROL_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, control_palette, control_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, light_fixtures, light_fixture_schema, MAXIMUM_LIGHT_FIXTURE_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, light_fixtures_palette, light_fixture_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, sound_scenery, sound_scenery_schema, MAXIMUM_SOUND_SCENERY_DATUMS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, sound_scenery_palette, sound_scenery_palette_schema,
		MAXIMUM_SCENARIO_OBJECT_PALETTE_ENTRIES_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, starting_profiles, starting_profile_schema, MAXIMUM_STARTING_PROFILES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, players, player_starting_location_schema, MAXIMUM_SCENARIO_PLAYERS_PER_BLOCK),
	TAG_SCHEMA_BLOCK(struct scenario, trigger_volumes, trigger_volume_schema, MAXIMUM_TRIGGER_VOLUMES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, recorded_animations, recorded_animation_schema,
		MAXIMUM_RECORDED_ANIMATIONS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, netgame_flags, netgame_flag_schema, MAXIMUM_SCENARIO_NETGAME_FLAGS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, netgame_equipment, netgame_equipment_schema,
		MAXIMUM_SCENARIO_NETGAME_EQUIPMENT_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, scenario_starting_equipment, starting_equipment_schema,
		MAXIMUM_SCENARIO_STARTING_EQUIPMDNG_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, bsp_switch_trigger_volumes, bsp_switch_trigger_volume_schema,
		MAXIMUM_BSP_SWITCH_TRIGGER_VOLUMES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, decal_palette, decal_palette_schema, MAXIMUM_DECAL_PALETTES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, detail_object_collection_palette, detail_object_collection_palette_schema,
		MAXIMUM_DETAIL_OBJECT_COLLECTION_PALETTE_ENTRIES),
	TAG_SCHEMA_BLOCK(struct scenario, ai_actor_palette, actor_palette_schema, MAXIMUM_ACTOR_PALETTE_ENTRIES),
	TAG_SCHEMA_BLOCK(struct scenario, ai_encounters, encounter_schema, MAXIMUM_ENCOUNTERS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, ai_command_lists, ai_command_list_schema, MAXIMUM_COMMAND_LISTS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, ai_animation_references, ai_animation_reference_schema,
		MAXIMUM_AI_REFERENCES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, ai_script_references, ai_script_reference_schema,
		MAXIMUM_AI_REFERENCES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, ai_recording_references, ai_recording_reference_schema,
		MAXIMUM_AI_REFERENCES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, ai_conversations, conversation_schema, MAXIMUM_CONVERSATIONS_PER_SCENARIO),
	TAG_SCHEMA_DATA(struct scenario, hs_syntax_data, HS_SYNTAX_DATA_SIZE),
	TAG_SCHEMA_DATA(struct scenario, hs_string_constants, MAXIMUM_HS_STRING_CONSTANTS_SIZE),
	TAG_SCHEMA_BLOCK(struct scenario, hs_scripts, hs_script_schema, MAXIMUM_HS_SCRIPTS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, hs_globals, hs_global_schema, MAXIMUM_HS_GLOBALS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, hs_references, hs_reference_schema, MAXIMUM_HS_REFERENCES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, hs_source_files, hs_source_file_schema, MAXIMUM_HS_SOURCE_FILES_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, cutscene_flags, cutscene_flag_schema, MAXIMUM_CUTSCENE_FLAGS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, cutscene_camera_points, cutscene_camera_point_schema,
		MAXIMUM_CUTSCENE_CAMERA_POINTS_PER_SCENARIO),
	TAG_SCHEMA_BLOCK(struct scenario, cutscene_chapter_titles, cutscene_title_schema,
		MAXIMUM_CUTSCENE_TITLES_PER_SCENARIO),
	TAG_SCHEMA_REFERENCE(struct scenario, custom_object_names, TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_REFERENCE(struct scenario, ingame_help_text, TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_REFERENCE(struct scenario, hud_messages, TAG_SCHEMA_GROUPS('hmt ')),
	TAG_SCHEMA_BLOCK(struct scenario, structure_bsp_references, structure_bsp_reference_schema,
		MAXIMUM_STRUCTURE_BSPS_PER_SCENARIO),
	TAG_SCHEMA_CHECK(scenario_check_object_names),
	TAG_SCHEMA_CHECK(scenario_check_scripts),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const scenario_schema =
	TAG_SCHEMA_DEFINITION(scenario, struct scenario, scenario_fields);

struct tag_schema_group const tag_schema_scenario_groups[] =
{
	{ 'scnr', { NONE, NONE }, &scenario_schema },
	{ 0 }
};
