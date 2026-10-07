/*
TAG_SCHEMA_EFFECTS.C

The schemas (tag_schema.h) of effects, damage, sounds, the interface and the
globals: effe, jpt!, cdmg, foot, mgs2, trak; snd!, lsnd, snde; the hud's
(grhi, unhi, wphi, hud#, hudg, hmt , metr); the menus' (DeLa, Soul, vcky),
fonts and strings (font, str#, ustr); matg and mply.
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"
#include "bitmaps/bitmap_group.h"
#include "effects/effect_definitions.h"
#include "effects/material_effect_definitions.h"
#include "game/game_globals.h"
#include "interface/interface.h"
#include "interface/unit_hud_interface_definition.h"
#include "interface/weapon_hud_interface_definition.h"
#include "math/periodic_functions.h"
#include "objects/damage_effect_definitions.h"
#include "objects/objects.h"
#include "objects/widgets/light_volumes.h"
#include "sound/sound_definitions.h"
#include "sound/sound_environment_definitions.h"
#include "text/font_group.h"
#include "text/text_group.h"

#include <string.h>

/* ---------- constants */

/* the game's, which their units keep to themselves */
enum
{
	/* periodic_functions.c */
	NUMBER_OF_PERIODIC_FUNCTIONS = 12,

	/* effects.c */
	NUMBER_OF_EFFECT_ENVIRONMENTS = 4,
	NUMBER_OF_EFFECT_DISPOSITIONS = 3,
	NUMBER_OF_EFFECT_CAMERA_MODES = 4,
	NUMBER_OF_EFFECT_PARTICLE_DISTRIBUTION_FUNCTIONS = 6,

	/* player_effects.c */
	NUMBER_OF_SCREEN_FLASH_TYPES = 7,
	NUMBER_OF_SCREEN_FLASH_PRIORITIES = 3,

	/* sound_classes.c, sound_manager.c */
	NUMBER_OF_SOUND_CLASSES = 51,
	NUMBER_OF_SOUND_SAMPLE_RATES = 2,
	NUMBER_OF_SOUND_ENCODINGS = 2,
	NUMBER_OF_SOUND_COMPRESSION_TYPES = 4,
	_sound_encoding_stereo = 1,
	_sound_compression_none = 0,
	_sound_compression_xbox_adpcm = 1,
	/* (an xbox adpcm block, of each channel: sound_dsound_xbox.c) */
	SOUND_COMPRESSED_BLOCK_SIZE = 36,
	/* (a pitch range's played permutations are bits of a long:
	sound_definitions.c, try_to_reset_permutations) */
	MAXIMUM_PLAYED_PERMUTATIONS_PER_PITCH_RANGE = 32,
	/* (the sound cache: sound_dsound_xbox.c's SOUND_CACHE_SIZE) */
	SOUND_CACHE_SIZE = 0x400000,

	/* damage.c */
	NUMBER_OF_DAMAGE_SIDE_EFFECTS = 4,
	NUMBER_OF_DAMAGE_CATEGORIES = 13,

	/* hud_draw.c, rasterizer_xbox.c */
	NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_EFFECTOR_TYPES = 4,
	_hud_multitexture_overlay_effector_type_tint = 0,
	_hud_multitexture_overlay_effector_type_horizontal_offset,
	_hud_multitexture_overlay_effector_type_vertical_offset,
	NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_EFFECTOR_DESTINATIONS = 4,
	_hud_multitexture_overlay_effector_destination_primary_map = 1,
	NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_EFFECTOR_SOURCES = 8,
	NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_BLEND_FUNCTIONS = 5,
	NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS = 8,
	MAXIMUM_MULTITEXTURE_OVERLAY_MAPS = 3,
	/* (sequence 0's frames: the digits, the decimal point, the colon, the
	minus sign, metres and kilometres) */
	NUMBER_OF_HUD_NUMBER_FRAMES = 15,

	/* hud_unit.c */
	NUMBER_OF_AUXILARY_OVERLAY_TYPE_BITS = 16,
	/* (a type no overlay shown has: there is a bit for teams only) */
	_auxilary_overlay_type_never = NUMBER_OF_AUXILARY_OVERLAY_TYPE_BITS - 1,
	NUMBER_OF_AUXILARY_METER_TYPES = 1,
	MAXIMUM_WARNING_SOUNDS_PER_UNIT_HUD = 12,

	/* hud_weapon.c */
	NUMBER_OF_WEAPON_HUD_FLASH_REFERENCES = 8,
	NUMBER_OF_WEAPON_HUD_MAP_TYPES = 3,
	NUMBER_OF_WEAPON_HUD_CROSSHAIR_STATES = 19,
	MAXIMUM_WEAPON_HUD_DEFINITION_DEPTH = 16,
	_hud_element_runtime_invalid_bit = 0,

	/* hud_messaging.c */
	NUMBER_OF_HUD_MESSAGE_TYPES = 2,
	_hud_message_type_text = 0,
	_hud_message_type_icon,
	NUMBER_OF_HUD_ICON_TYPES = 40,

	/* hud_nav_points.c */
	NUMBER_OF_WAYPOINT_TYPES = 3,

	/* font_group.c, rasterizer_text.c: a table for each high byte, an entry
	for each low one; a character's pixels are copied into the hardware
	character cache's bitmap of HARDWARE_CHARACTER_CACHE_BITMAP_WIDTH by
	HARDWARE_CHARACTER_CACHE_BITMAP_HEIGHT */
	MAXIMUM_FONT_CHARACTER_TABLES = 256,
	MAXIMUM_CHARACTERS_PER_FONT_CHARACTER_TABLE = 256,
	MAXIMUM_CHARACTERS_PER_FONT = SHORT_MAX,
	HARDWARE_CHARACTER_CACHE_BITMAP_WIDTH = 128,
	HARDWARE_CHARACTER_CACHE_BITMAP_HEIGHT = 128,
	/* (a cell's borders, a texel each side) */
	HARDWARE_CHARACTER_CACHE_BORDERS = 2,

	/* ui_widget.c, ui_widget_text_search_and_replace_functions.c */
	NUMBER_OF_UI_WIDGET_TYPES = 7,
	_ui_widget_type_column_list = 3,
	NUMBER_OF_WIDGET_CONTROLLERS = 5,
	NUMBER_OF_WIDGET_TEXT_JUSTIFICATIONS = 3,
	_widget_replace_function_null = 0,
	_widget_replace_function_local_player = 1,

	/* virtual_keyboard.c */
	NUMBER_OF_VIRTUAL_KEYS = 44,

	/* units.h */
	NUMBER_OF_UNIT_GRENADE_TYPES = 2,

	/* the tool's limits, where the game has none of its own */
	MAXIMUM_MATERIAL_EFFECTS = 13,
	MAXIMUM_MATERIALS_PER_MATERIAL_EFFECT = 33,
	MAXIMUM_CAMERA_TRACK_CONTROL_POINTS = 16,
	MAXIMUM_LIGHT_VOLUME_FRAMES = 2,
	MAXIMUM_MULTITEXTURE_OVERLAYS_PER_HUD_ELEMENT = 30,
	MAXIMUM_EFFECTORS_PER_MULTITEXTURE_OVERLAY = 30,
	MAXIMUM_ITEMS_PER_HUD_OVERLAY = 16,
	MAXIMUM_ITEMS_PER_HUD_CROSSHAIR = 64,
	MAXIMUM_AUXILARY_OVERLAYS_PER_UNIT_HUD = 16,
	MAXIMUM_AUXILARY_METERS_PER_UNIT_HUD = 16,
	MAXIMUM_ELEMENTS_PER_WEAPON_HUD = 16,
	MAXIMUM_CROSSHAIRS_PER_WEAPON_HUD = 19,
	MAXIMUM_SCREEN_EFFECTS_PER_WEAPON_HUD = 1,
	MAXIMUM_HUD_MESSAGE_TEXT_DATA_SIZE = 2 * (UNSIGNED_SHORT_MAX + 1),
	MAXIMUM_HUD_MESSAGE_ELEMENTS = 8192,
	MAXIMUM_HUD_MESSAGES = 1024,
	MAXIMUM_HUD_BUTTON_ICONS = 18,
	MAXIMUM_HUD_WAYPOINT_ARROWS = 16,
	MAXIMUM_FONT_PIXELS_SIZE = 0x800000,
	MAXIMUM_GAME_DATA_INPUTS_PER_WIDGET = 64,
	MAXIMUM_EVENT_HANDLERS_PER_WIDGET = 32,
	MAXIMUM_SEARCH_AND_REPLACE_FUNCTIONS_PER_WIDGET = 32,
	MAXIMUM_CONDITIONAL_WIDGETS_PER_WIDGET = 32,
	MAXIMUM_CHILD_WIDGETS_PER_WIDGET = 32,
	MAXIMUM_WIDGETS_PER_COLLECTION = 32,
	/* (how deep widgets' children go: the loading recurses through each) */
	MAXIMUM_WIDGET_DEPTH = 32,
	MAXIMUM_MULTIPLAYER_SCENARIO_DESCRIPTIONS = 32,
	MAXIMUM_GLOBAL_SOUNDS = 2,
	MAXIMUM_LOOK_FUNCTION_VALUES = 16,
	MAXIMUM_GLOBAL_WEAPONS = 20,
	MAXIMUM_CHEAT_POWERUPS = 20,
	MAXIMUM_MULTIPLAYER_VEHICLES = 20,
	MAXIMUM_MULTIPLAYER_SOUNDS = 60,
	MAXIMUM_PARTICLE_EFFECTS_PER_BREAKABLE_SURFACE = 32,
};

/* ---------- structures */

/* effects.c's */
struct effect_particles_definition
{
	short environment;
	short disposition;
	short camera_mode;
	short unused006;
	short location_index;
	short unused00a;
	real_euler_angles2d direction;
	real_vector3d offset;
	real_vector3d runtime_direction;
	long unused02c[10];
	struct tag_reference particle;
	unsigned long flags;
	short distribution_function;
	short unused06a;
	short count_lower_bound;
	short count_upper_bound;
	real distribution_radius_lower_bound;
	real distribution_radius_upper_bound;
	long unused078[3];
	real velocity_lower_bound;
	real velocity_upper_bound;
	real velocity_cone_angle;
	real angular_velocity_lower_bound;
	real angular_velocity_upper_bound;
	long unused098[2];
	real radius_lower_bound;
	real radius_upper_bound;
	long unused0a8[2];
	real_argb_color tint_lower_bound;
	real_argb_color tint_upper_bound;
	long unused0d0[4];
	unsigned long scale_a_flags;
	unsigned long scale_b_flags;
};

struct effect_location_definition
{
	char marker_name[32];
};

typedef char verify_effect_particles_definition_size[sizeof(struct effect_particles_definition) == 0xE8 ? 1 : -1];

/* following_camera.c's */
struct camera_track_control_point
{
	real_vector3d position;
	real_quaternion orientation;
	long unused[8];
};

struct camera_track_definition
{
	unsigned long flags;
	struct tag_block control_points;
	long unused[8];
};

typedef char verify_camera_track_control_point_size[sizeof(struct camera_track_control_point) == 0x3C ? 1 : -1];
typedef char verify_camera_track_definition_size[sizeof(struct camera_track_definition) == 0x30 ? 1 : -1];

/* hud_draw.c's */
struct number_hud_element_definition
{
	struct hud_placement_definition placement;
	struct hud_color_definition colors;
	char digits;
	byte number_flags;
	char fractional_digits;
	byte pad;
	long unused[3];
};

struct hud_number_definition
{
	struct tag_reference number_bitmap;
	char character_width;
	char screen_width;
	char x_offset;
	char y_offset;
	char decimal_point_width;
	char colon_width;
	short pad;
	long unused[19];
};

struct weapon_hud_overlay_item
{
	struct hud_placement_definition placement;
	struct hud_color_definition colors;
	short frame_rate;
	short pad;
	short sequence_index;
	short type;
	long flags;
	long unused[14];
};

struct weapon_hud_overlay_definition
{
	struct tag_reference bitmap;
	struct tag_block items;
};

struct multitexture_overlay_hud_element_effector_definition
{
	long unused0[16];
	short destination_type;
	short destination;
	short source;
	word pad46;
	real in_bounds[2];
	real out_bounds[2];
	long unused58[16];
	real_rgb_color tint_color_lower_bounds;
	real_rgb_color tint_color_upper_bounds;
	short periodic_function;
	word padB2;
	real periodic_function_period;
	real periodic_function_phase;
	long unusedBC[8];
};

struct multitexture_overlay_hud_element_definition
{
	word flags;
	short type;
	short framebuffer_blend_function;
	word pad06;
	long unused08[8];
	word map_flags[3];
	short map_blending_function[2];
	short pad32;
	real_vector2d map_scale[3];
	real_vector2d map_offset[3];
	struct tag_reference map[3];
	short map_clamp[3];
	short pad9A;
	long unused9C[46];
	struct tag_block functions;
	long unused160[32];
};

typedef char verify_number_hud_element_definition_size[sizeof(struct number_hud_element_definition) == 0x54 ? 1 : -1];
typedef char verify_hud_number_definition_size[sizeof(struct hud_number_definition) == 0x64 ? 1 : -1];
typedef char verify_weapon_hud_overlay_item_size[sizeof(struct weapon_hud_overlay_item) == 0x88 ? 1 : -1];
typedef char verify_multitexture_overlay_hud_element_effector_definition_size[
	sizeof(struct multitexture_overlay_hud_element_effector_definition) == 0xDC ? 1 : -1];
typedef char verify_multitexture_overlay_hud_element_definition_size[
	sizeof(struct multitexture_overlay_hud_element_definition) == 0x1E0 ? 1 : -1];

/* hud_weapon.c's */
struct grenade_count_panel_definition
{
	struct static_hud_element_definition background;
	struct number_hud_element_definition numbers;
	short flash_cutoff;
	short pad;
};

struct grenade_hud_interface_definition
{
	struct hud_absolute_placement_definition absolute_placement;
	struct static_hud_element_definition background;
	struct grenade_count_panel_definition grenade_count_panel;
	struct weapon_hud_overlay_definition overlays;
	struct tag_block warning_sounds;
	long unused0[17];
	struct tag_reference messaging_icon_bitmap;
	long unused1[12];
};

struct weapon_hud_element_header
{
	short state_type;
	short runtime_flags;
	short use_on_map_type;
	short pad;
	long unused[7];
};

struct weapon_hud_static_element
{
	struct weapon_hud_element_header header;
	struct static_hud_element_definition static_element;
	long unused[10];
};

struct weapon_hud_meter_element
{
	struct weapon_hud_element_header header;
	struct meter_hud_element_definition meter_element;
	long unused[10];
};

struct weapon_hud_number_element
{
	struct weapon_hud_element_header header;
	struct number_hud_element_definition number_element;
	word weapon_flags;
	short pad;
	long unused[9];
};

struct weapon_hud_overlays_element
{
	short state_type;
	short runtime_flags;
	short use_on_map_type;
	short pad;
	long unused[7];
	struct weapon_hud_overlay_definition overlays;
	long unused2[10];
};

struct weapon_hud_crosshair_definition
{
	struct tag_reference bitmap;
	struct tag_block items;
};

struct weapon_hud_crosshairs_element
{
	short crosshair_type;
	short runtime_flags;
	short use_on_map_type;
	short pad;
	long unused[7];
	struct weapon_hud_crosshair_definition crosshairs;
	long unused2[10];
};

struct weapon_hud_crosshair_item
{
	struct hud_placement_definition placement;
	struct hud_color_definition colors;
	short frame_rate;
	short sequence_index;
	unsigned long flags;
	long unused[8];
};

typedef char verify_grenade_hud_interface_definition_size[sizeof(struct grenade_hud_interface_definition) == 0x1F8 ? 1 : -1];
typedef char verify_weapon_hud_static_element_size[sizeof(struct weapon_hud_static_element) == 0xB4 ? 1 : -1];
typedef char verify_weapon_hud_meter_element_size[sizeof(struct weapon_hud_meter_element) == 0xB4 ? 1 : -1];
typedef char verify_weapon_hud_number_element_size[sizeof(struct weapon_hud_number_element) == 0xA0 ? 1 : -1];
typedef char verify_weapon_hud_overlays_element_size[sizeof(struct weapon_hud_overlays_element) == 0x68 ? 1 : -1];
typedef char verify_weapon_hud_crosshairs_element_size[sizeof(struct weapon_hud_crosshairs_element) == 0x68 ? 1 : -1];
typedef char verify_weapon_hud_crosshair_item_size[sizeof(struct weapon_hud_crosshair_item) == 0x6C ? 1 : -1];
typedef char verify_weapon_hud_interface_definition_size[sizeof(struct weapon_hud_interface_definition) == 0x17C ? 1 : -1];

/* hud_sounds.c's */
struct hud_sound_definition
{
	struct tag_reference sound;
	unsigned long state_flags;
	real scale;
	byte reserved[32];
};

/* hud_nav_points.c's */
struct hud_waypoint_arrow
{
	char name[0x20];
	long unused0[2];
	unsigned long color;
	real opacity;
	real fade;
	short sequence_indices[3];
	short pad;
	long unused1[4];
	unsigned long flags;
	long unused2[6];
};

/* hud_messaging.c's */
struct hud_state_message_definition
{
	char name[32];
	word text_start_index;
	word element_start_index;
	byte element_count;
	byte pad25[3];
	long unused28[6];
};

struct hud_state_message_element
{
	byte type;
	byte data;
};

struct hud_message_text_definition
{
	struct tag_data text_data;
	struct tag_block elements;
	struct tag_block messages;
	long unused2C[21];
};

typedef char verify_hud_sound_definition_size[sizeof(struct hud_sound_definition) == 0x38 ? 1 : -1];
typedef char verify_hud_waypoint_arrow_size[sizeof(struct hud_waypoint_arrow) == 0x68 ? 1 : -1];
typedef char verify_hud_state_message_definition_size[sizeof(struct hud_state_message_definition) == 0x40 ? 1 : -1];
typedef char verify_hud_message_text_definition_size[sizeof(struct hud_message_text_definition) == 0x80 ? 1 : -1];

/* draw_string.c's (and rasterizer_text.c's) */
struct font_character
{
	word character;
	short character_width;
	short bitmap_width;
	short bitmap_height;
	short bitmap_origin_x;
	short bitmap_origin_y;
	short hardware_character_index;
	word pad;
	long pixels_offset;
};

/* (a character table's entry) */
struct font_character_index
{
	short index;
};

typedef char verify_font_character_size[sizeof(struct font_character) == FONT_CHARACTER_SIZE ? 1 : -1];
typedef char verify_font_header_size[sizeof(struct font_header) == 0x9C ? 1 : -1];

/* ui_widget.c's */
struct ui_widget_event_handler_reference
{
	long flags;
	short event_type;
	short function;
	struct tag_reference widget_tag;
	struct tag_reference sound_effect;
	char script[32];
};

struct ui_widget_child_reference
{
	struct tag_reference widget_tag;
	char name[32];
	long flags;
	short custom_controller_index;
	short vertical_offset;
	short horizontal_offset;
	byte unknown03A[0x50 - 0x3A];
};

struct ui_widget_conditional_reference
{
	struct tag_reference widget_tag;
	char name[32];
	long flags;
	short custom_controller_index;
	byte unknown036[0x50 - 0x36];
};

struct ui_widget_game_data_input_reference
{
	short function;
	byte unknown002[0x24 - 0x02];
};

struct ui_widget_search_and_replace_reference
{
	char search_string[32];
	short replace_function;
};

struct ui_widget_definition
{
	short type;
	short controller_index;
	char name[32];
	rectangle2d bounds;
	long flags;
	long milliseconds_to_auto_close;
	long auto_close_fade_time;
	struct tag_reference background_bitmap;
	struct tag_block game_data_inputs;
	struct tag_block event_handlers;
	struct tag_block search_and_replace_functions;
	byte unknown06C[0xEC - 0x6C];
	struct tag_reference text_label_string_list;
	struct tag_reference text_font;
	real_argb_color text_color;
	short justification;
	word text_box_flags;
	byte unknown120[0x12E - 0x120];
	short string_list_index;
	short horizontal_offset;
	short vertical_offset;
	byte unknown134[0x150 - 0x134];
	long list_flags;
	struct tag_reference list_header_bitmap;
	struct tag_reference list_footer_bitmap;
	rectangle2d list_header_bounds;
	rectangle2d list_footer_bounds;
	byte unknown184[0x1A4 - 0x184];
	struct tag_reference extended_description_widget;
	byte unknown1B4[0x2D4 - 0x1B4];
	struct tag_block conditional_widgets;
	byte unknown2E0[0x3E0 - 0x2E0];
	struct tag_block child_widgets;
};

typedef char verify_ui_widget_event_handler_reference_size[sizeof(struct ui_widget_event_handler_reference) == 0x48 ? 1 : -1];
typedef char verify_ui_widget_child_reference_size[sizeof(struct ui_widget_child_reference) == 0x50 ? 1 : -1];
typedef char verify_ui_widget_conditional_reference_size[sizeof(struct ui_widget_conditional_reference) == 0x50 ? 1 : -1];
typedef char verify_ui_widget_game_data_input_reference_size[sizeof(struct ui_widget_game_data_input_reference) == 0x24 ? 1 : -1];
typedef char verify_ui_widget_search_and_replace_reference_size[sizeof(struct ui_widget_search_and_replace_reference) == 0x22 ? 1 : -1];
typedef char verify_ui_widget_definition_size[sizeof(struct ui_widget_definition) == 0x3EC ? 1 : -1];

/* (a ui widget collection: the engine has no structure of it; menu_tags.c
reads it as a block of tag references) */
struct ui_widget_collection_widget
{
	struct tag_reference widget;
};

struct ui_widget_collection
{
	struct tag_block widgets;
};

/* virtual_keyboard.c's */
struct virtual_keyboard_key
{
	short keycode;
	wchar_t character;
	wchar_t shift_character;
	wchar_t caps_character;
	wchar_t symbols_character;
	wchar_t shift_caps_character;
	wchar_t shift_symbols_character;
	wchar_t caps_symbols_character;
	struct tag_reference unselected_background_bitmap_tag;
	struct tag_reference selected_background_bitmap_tag;
	struct tag_reference active_background_bitmap_tag;
	struct tag_reference sticky_background_bitmap_tag;
};

struct virtual_keyboard_definition
{
	struct tag_reference font_tag;
	struct tag_reference background_bitmap_tag;
	struct tag_reference special_key_labels_string_list_tag;
	struct tag_block keys;
};

typedef char verify_virtual_keyboard_key_size[sizeof(struct virtual_keyboard_key) == 0x50 ? 1 : -1];
typedef char verify_virtual_keyboard_definition_size[sizeof(struct virtual_keyboard_definition) == 0x3C ? 1 : -1];

/* (a multiplayer scenario description: multiplayer_scenario_description.h
has its element only to the path; the tool's is the path and a pad) */
struct multiplayer_scenario_description_entry
{
	struct tag_reference descriptive_bitmap;
	struct tag_reference displayed_map_name;
	char scenario_tag_path[32];
	long pad;
};

struct multiplayer_scenario_description
{
	struct tag_block scenarios;
};

typedef char verify_multiplayer_scenario_description_entry_size[
	sizeof(struct multiplayer_scenario_description_entry) == 0x44 ? 1 : -1];

/* (a block of tag references) */
struct tag_reference_element
{
	struct tag_reference reference;
};

/* physics.c's (and damage.c's, bipeds.c's) */
struct game_globals_falling_damage
{
	long unused0[2];
	real falling_distance_lower_bound;
	real falling_distance_upper_bound;
	struct tag_reference falling_damage;
	long terminal_velocity_unused[2];
	real maximum_distance;
	struct tag_reference maximum_distance_damage;
	struct tag_reference vehicle_hit_environment_damage_effect;
	struct tag_reference vehicle_killed_unit_damage_effect;
	struct tag_reference vehicle_collision_damage;
	struct tag_reference flaming_death_damage;
	long unused7c[4];
	real runtime_maximum_falling_velocity;
	real runtime_minimum_damage_velocity;
	real runtime_maximum_damage_velocity;
};

/* breakable_surfaces.h's */
struct breakable_surface_particle_effect
{
	struct tag_reference particle;
	unsigned long flags;
	real density;
	real velocity_scale_lower_bound;
	real velocity_scale_upper_bound;
	real velocity_cone_angle;
	real angular_velocity_lower_bound;
	real angular_velocity_upper_bound;
	unsigned long unused1[2];
	real radius_lower_bound;
	real radius_upper_bound;
	unsigned long unused2[2];
	real_argb_color tint_lower_bound;
	real_argb_color tint_upper_bound;
	unsigned long unused3[7];
};

typedef char verify_game_globals_falling_damage_size[sizeof(struct game_globals_falling_damage) == 0x98 ? 1 : -1];
typedef char verify_breakable_surface_particle_effect_size[sizeof(struct breakable_surface_particle_effect) == 0x80 ? 1 : -1];
typedef char verify_material_definition_size[sizeof(struct material_definition) == 0x374 ? 1 : -1];
typedef char verify_game_globals_size[sizeof(struct game_globals) == 0x1AC ? 1 : -1];

/* (a static element or meter drawn only if its bitmap is not none: the
checks there differ) */
struct optional_static_hud_element
{
	struct static_hud_element_definition element;
};

struct optional_meter_hud_element
{
	struct meter_hud_element_definition element;
};

/* ---------- prototypes */

static boolean effect_part_check(struct tag_validation *validation, void *base);
static boolean sound_check(struct tag_validation *validation, void *base);
static boolean multitexture_overlay_check(struct tag_validation *validation, void *base);
static boolean optional_static_hud_element_check(struct tag_validation *validation, void *base);
static boolean optional_meter_hud_element_check(struct tag_validation *validation, void *base);
static boolean auxilary_overlay_check(struct tag_validation *validation, void *base);
static boolean auxilary_meter_check(struct tag_validation *validation, void *base);
static boolean weapon_hud_static_element_check(struct tag_validation *validation, void *base);
static boolean weapon_hud_meter_element_check(struct tag_validation *validation, void *base);
static boolean weapon_hud_interface_check(struct tag_validation *validation, void *base);
static boolean hud_message_text_check(struct tag_validation *validation, void *base);
static boolean hud_globals_check(struct tag_validation *validation, void *base);
static boolean font_check(struct tag_validation *validation, void *base);
static boolean string_list_entry_check(struct tag_validation *validation, void *base);
static boolean unicode_string_list_entry_check(struct tag_validation *validation, void *base);
static boolean ui_widget_search_and_replace_check(struct tag_validation *validation, void *base);
static boolean ui_widget_check(struct tag_validation *validation, void *base);
static boolean virtual_keyboard_check(struct tag_validation *validation, void *base);
static boolean game_globals_interface_tag_references_check(struct tag_validation *validation, void *base);
static boolean game_globals_check(struct tag_validation *validation, void *base);

/* ---------- globals */

/* effects */

static struct tag_schema_field const effect_location_fields[] =
{
	TAG_SCHEMA_STRING(struct effect_location_definition, marker_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const effect_location_schema =
	TAG_SCHEMA_DEFINITION(effect_location, struct effect_location_definition, effect_location_fields);

/* (a part is made as its base class says: effects.c, effect_generate_part.
A part's or particle's location index is not listed: retail effects have
ones that are none or past their locations, which the game skips) */
static struct tag_schema_field const effect_part_fields[] =
{
	TAG_SCHEMA_ENUM(struct effect_part_definition, environment, NUMBER_OF_EFFECT_ENVIRONMENTS, 0),
	TAG_SCHEMA_ENUM(struct effect_part_definition, disposition, NUMBER_OF_EFFECT_DISPOSITIONS, 0),
	TAG_SCHEMA_REFERENCE(struct effect_part_definition, reference,
		TAG_SCHEMA_GROUPS('pctl', 'snd!', 'obje', 'deca', 'jpt!', 'ligh')),
	TAG_SCHEMA_CHECK(effect_part_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const effect_part_schema =
	TAG_SCHEMA_DEFINITION(effect_part, struct effect_part_definition, effect_part_fields);

static struct tag_schema_field const effect_particles_fields[] =
{
	TAG_SCHEMA_ENUM(struct effect_particles_definition, environment, NUMBER_OF_EFFECT_ENVIRONMENTS, 0),
	TAG_SCHEMA_ENUM(struct effect_particles_definition, disposition, NUMBER_OF_EFFECT_DISPOSITIONS, 0),
	TAG_SCHEMA_ENUM(struct effect_particles_definition, camera_mode, NUMBER_OF_EFFECT_CAMERA_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct effect_particles_definition, particle, TAG_SCHEMA_GROUPS('part')),
	TAG_SCHEMA_ENUM(struct effect_particles_definition, distribution_function,
		NUMBER_OF_EFFECT_PARTICLE_DISTRIBUTION_FUNCTIONS, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const effect_particles_schema =
	TAG_SCHEMA_DEFINITION(effect_particles, struct effect_particles_definition, effect_particles_fields);

/* (an effect counts each event's particles in a byte each:
effect_datum.particle_counts) */
static struct tag_schema_field const effect_event_fields[] =
{
	TAG_SCHEMA_BLOCK(struct effect_event_definition, parts, effect_part_schema, MAXIMUM_EFFECT_PARTS_PER_EVENT),
	TAG_SCHEMA_BLOCK(struct effect_event_definition, particles, effect_particles_schema,
		MAXIMUM_EFFECT_PARTICLES_PER_EVENT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const effect_event_schema =
	TAG_SCHEMA_DEFINITION(effect_event, struct effect_event_definition, effect_event_fields);

/* (locations: effect_datum.location_datum_indices; events: the tool's) */
static struct tag_schema_field const effect_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct effect_definition, loop_start_index, TAG_SCHEMA_ROOT,
		offsetof(struct effect_definition, events), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK_INDEX(struct effect_definition, loop_stop_index, TAG_SCHEMA_ROOT,
		offsetof(struct effect_definition, events), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_BLOCK(struct effect_definition, locations, effect_location_schema, MAXIMUM_EFFECT_LOCATIONS),
	TAG_SCHEMA_BLOCK(struct effect_definition, events, effect_event_schema, MAXIMUM_EFFECT_EVENTS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const effect_schema =
	TAG_SCHEMA_DEFINITION(effect, struct effect_definition, effect_fields);

/* damage (player_effects.c, damage.c) */

static struct tag_schema_field const damage_fields[] =
{
	TAG_SCHEMA_ENUM(struct damage_definition, side_effect, NUMBER_OF_DAMAGE_SIDE_EFFECTS, 0),
	TAG_SCHEMA_ENUM(struct damage_definition, category, NUMBER_OF_DAMAGE_CATEGORIES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const damage_schema =
	TAG_SCHEMA_DEFINITION(damage, struct damage_definition, damage_fields);

static struct tag_schema_field const vibrate_frequency_fields[] =
{
	TAG_SCHEMA_ENUM(struct vibrate_frequency_definition, fade_function, NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const vibrate_frequency_schema =
	TAG_SCHEMA_DEFINITION(vibrate_frequency, struct vibrate_frequency_definition, vibrate_frequency_fields);

static struct tag_schema_field const damage_effect_fields[] =
{
	TAG_SCHEMA_ENUM(struct damage_effect_definition, screen_flash.type, NUMBER_OF_SCREEN_FLASH_TYPES, 0),
	TAG_SCHEMA_ENUM(struct damage_effect_definition, screen_flash.priority, NUMBER_OF_SCREEN_FLASH_PRIORITIES, 0),
	TAG_SCHEMA_ENUM(struct damage_effect_definition, screen_flash.fade_function, NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_STRUCT_ARRAY(struct damage_effect_definition, vibrate.vibrate_frequencies, vibrate_frequency_schema),
	TAG_SCHEMA_ENUM(struct damage_effect_definition, camera_impulse.temporary_transition,
		NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct damage_effect_definition, camera_shake.falloff_transition_function,
		NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct damage_effect_definition, camera_shake.periodic_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct damage_effect_definition, sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_STRUCT(struct damage_effect_definition, damage, damage_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const damage_effect_schema =
	TAG_SCHEMA_DEFINITION(damage_effect, struct damage_effect_definition, damage_effect_fields);

static struct tag_schema_field const continuous_damage_effect_fields[] =
{
	TAG_SCHEMA_ENUM(struct continuous_damage_effect_definition, camera_shake.periodic_function,
		NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_STRUCT(struct continuous_damage_effect_definition, damage, damage_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const continuous_damage_effect_schema =
	TAG_SCHEMA_DEFINITION(continuous_damage_effect, struct continuous_damage_effect_definition,
		continuous_damage_effect_fields);

/* material effects (material_effects.c: effects and materials are looked
for by index, against their counts) */

static struct tag_schema_field const material_effect_material_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct material_effect_material, effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct material_effect_material, sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const material_effect_material_schema =
	TAG_SCHEMA_DEFINITION(material_effect_material, struct material_effect_material, material_effect_material_fields);

static struct tag_schema_field const material_effect_fields[] =
{
	TAG_SCHEMA_BLOCK(struct material_effect, materials, material_effect_material_schema,
		MAXIMUM_MATERIALS_PER_MATERIAL_EFFECT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const material_effect_schema =
	TAG_SCHEMA_DEFINITION(material_effect, struct material_effect, material_effect_fields);

static struct tag_schema_field const material_effects_fields[] =
{
	TAG_SCHEMA_BLOCK(struct material_effects_definition, effects, material_effect_schema, MAXIMUM_MATERIAL_EFFECTS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const material_effects_schema =
	TAG_SCHEMA_DEFINITION(material_effects, struct material_effects_definition, material_effects_fields);

/* camera tracks (following_camera.c: four control points from one the
pitch gives, each looked for against the count) */

static struct tag_schema_field const camera_track_control_point_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const camera_track_control_point_schema =
	TAG_SCHEMA_DEFINITION(camera_track_control_point, struct camera_track_control_point,
		camera_track_control_point_fields);

static struct tag_schema_field const camera_track_fields[] =
{
	TAG_SCHEMA_BLOCK(struct camera_track_definition, control_points, camera_track_control_point_schema,
		MAXIMUM_CAMERA_TRACK_CONTROL_POINTS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const camera_track_schema =
	TAG_SCHEMA_DEFINITION(camera_track, struct camera_track_definition, camera_track_fields);

/* sound environments (nothing but numbers) */

static struct tag_schema_field const sound_environment_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sound_environment_schema =
	TAG_SCHEMA_DEFINITION(sound_environment, struct sound_environment_definition, sound_environment_fields);

/* light volumes (light_volumes.c: the frames' first is drawn; a source is
none or an object's function, less one) */

static struct tag_schema_field const light_volume_frame_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const light_volume_frame_schema =
	TAG_SCHEMA_DEFINITION(light_volume_frame, struct light_volume_frame, light_volume_frame_fields);

static struct tag_schema_field const light_volume_fields[] =
{
	TAG_SCHEMA_STRING(struct light_volume_definition, attachment_marker),
	TAG_SCHEMA_ENUM(struct light_volume_definition, brightness_scale_source, NUMBER_OF_OBJECT_FUNCTION_REFERENCES, 0),
	TAG_SCHEMA_REFERENCE(struct light_volume_definition, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct light_volume_definition, frame_animation_source, NUMBER_OF_OBJECT_FUNCTION_REFERENCES, 0),
	TAG_SCHEMA_BLOCK(struct light_volume_definition, frames, light_volume_frame_schema, MAXIMUM_LIGHT_VOLUME_FRAMES),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const light_volume_schema =
	TAG_SCHEMA_DEFINITION(light_volume, struct light_volume_definition, light_volume_fields);

/* sounds (sound_manager.c, sound_definitions.c, xbox_sound_cache.c; the
pitch ranges and permutations are the tool's, the game has no arrays of
them) */

/* (the cache fields are the sound cache's, which sets them as it loads a
permutation's samples from the map file: xbox_sound_cache.c's
cache_block_index and cache_base_address. Its tag indices are the sound's
own: sound_check) */
static struct tag_schema_field const sound_permutation_fields[] =
{
	TAG_SCHEMA_STRING(struct sound_permutation, name),
	TAG_SCHEMA_BLOCK_INDEX(struct sound_permutation, next_permutation_index, 1,
		offsetof(struct sound_pitch_range, permutations), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_RESET(struct sound_permutation, unknown0, NONE),
	TAG_SCHEMA_RESET(struct sound_permutation, unknown1, 0),
	TAG_SCHEMA_FILE_DATA(struct sound_permutation, samples, SOUND_CACHE_SIZE),
	TAG_SCHEMA_DATA(struct sound_permutation, mouth_data, MAXIMUM_SOUND_MOUTH_DATA_SIZE),
	TAG_SCHEMA_DATA(struct sound_permutation, subtitle_data, MAXIMUM_SOUND_SUBTITLE_DATA_SIZE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sound_permutation_schema =
	TAG_SCHEMA_DEFINITION(sound_permutation, struct sound_permutation, sound_permutation_fields);

/* (what was played, and the permutation the cache could not play, are the
game's) */
static struct tag_schema_field const sound_pitch_range_fields[] =
{
	TAG_SCHEMA_STRING(struct sound_pitch_range, name),
	TAG_SCHEMA_RESET(struct sound_pitch_range, played_permutation_mask, 0),
	TAG_SCHEMA_RESET(struct sound_pitch_range, previous_permutation_index, NONE),
	TAG_SCHEMA_RESET(struct sound_pitch_range, forced_permutation_index, NONE),
	TAG_SCHEMA_BLOCK(struct sound_pitch_range, permutations, sound_permutation_schema,
		MAXIMUM_PERMUTATIONS_PER_PITCH_RANGE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sound_pitch_range_schema =
	TAG_SCHEMA_DEFINITION(sound_pitch_range, struct sound_pitch_range, sound_pitch_range_fields);

/* (promotions are counted, and scripts' sounds kept, by the game) */
static struct tag_schema_field const sound_fields[] =
{
	TAG_SCHEMA_ENUM(struct sound_definition, sound_class, NUMBER_OF_SOUND_CLASSES, 0),
	TAG_SCHEMA_ENUM(struct sound_definition, sample_rate, NUMBER_OF_SOUND_SAMPLE_RATES, 0),
	TAG_SCHEMA_ENUM(struct sound_definition, encoding, NUMBER_OF_SOUND_ENCODINGS, 0),
	TAG_SCHEMA_ENUM(struct sound_definition, compression, NUMBER_OF_SOUND_COMPRESSION_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct sound_definition, promotion_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_RESET(struct sound_definition, promotion_counter, 0),
	TAG_SCHEMA_RESET(struct sound_definition, promotion_time, 0),
	TAG_SCHEMA_RESET(struct sound_definition, scripting_time, NONE),
	TAG_SCHEMA_RESET(struct sound_definition, scripting_sound_index, NONE),
	TAG_SCHEMA_BLOCK(struct sound_definition, pitch_ranges, sound_pitch_range_schema, MAXIMUM_PITCH_RANGES_PER_SOUND),
	TAG_SCHEMA_CHECK(sound_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sound_schema =
	TAG_SCHEMA_DEFINITION(sound, struct sound_definition, sound_fields);

/* looping sounds (sound_manager.c: tracks and details are kept in a looping
sound's arrays of MAXIMUM_TRACKS_PER_LOOPING_SOUND and
MAXIMUM_DETAIL_SOUNDS_PER_LOOPING_SOUND) */

static struct tag_schema_field const looping_sound_track_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct looping_sound_track, start_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct looping_sound_track, loop_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct looping_sound_track, stop_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct looping_sound_track, alternate_loop_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct looping_sound_track, alternate_stop_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const looping_sound_track_schema =
	TAG_SCHEMA_DEFINITION(looping_sound_track, struct looping_sound_track, looping_sound_track_fields);

static struct tag_schema_field const looping_sound_detail_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct looping_sound_detail, sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const looping_sound_detail_schema =
	TAG_SCHEMA_DEFINITION(looping_sound_detail, struct looping_sound_detail, looping_sound_detail_fields);

/* (a script's looping sound is the game's: game_sound.c) */
static struct tag_schema_field const looping_sound_fields[] =
{
	TAG_SCHEMA_RESET(struct looping_sound_definition, runtime_scripting_sound_index, NONE),
	TAG_SCHEMA_REFERENCE(struct looping_sound_definition, continuous_damage_effect, TAG_SCHEMA_GROUPS('cdmg')),
	TAG_SCHEMA_BLOCK(struct looping_sound_definition, tracks, looping_sound_track_schema,
		MAXIMUM_TRACKS_PER_LOOPING_SOUND),
	TAG_SCHEMA_BLOCK(struct looping_sound_definition, details, looping_sound_detail_schema,
		MAXIMUM_DETAIL_SOUNDS_PER_LOOPING_SOUND),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const looping_sound_schema =
	TAG_SCHEMA_DEFINITION(looping_sound, struct looping_sound_definition, looping_sound_fields);

/* the hud (hud_draw.c, hud_unit.c, hud_weapon.c, hud_messaging.c,
hud_nav_points.c, hud_sounds.c). A static element or meter whose bitmap
gives no bitmap is not drawn (the drawing reads the bitmap it is given:
hud_bitmap_check and the checks after it) */

static struct tag_schema_field const multitexture_overlay_effector_fields[] =
{
	TAG_SCHEMA_ENUM(struct multitexture_overlay_hud_element_effector_definition, destination_type,
		NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_EFFECTOR_TYPES, 0),
	TAG_SCHEMA_ENUM(struct multitexture_overlay_hud_element_effector_definition, destination,
		NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_EFFECTOR_DESTINATIONS, 0),
	TAG_SCHEMA_ENUM(struct multitexture_overlay_hud_element_effector_definition, source,
		NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_EFFECTOR_SOURCES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const multitexture_overlay_effector_schema =
	TAG_SCHEMA_DEFINITION(multitexture_overlay_effector, struct multitexture_overlay_hud_element_effector_definition,
		multitexture_overlay_effector_fields);

/* (the framebuffer blend function indexes the rasterizer's tables) */
static struct tag_schema_field const multitexture_overlay_fields[] =
{
	TAG_SCHEMA_ENUM(struct multitexture_overlay_hud_element_definition, framebuffer_blend_function,
		NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM_ARRAY(struct multitexture_overlay_hud_element_definition, map_blending_function,
		NUMBER_OF_HUD_MULTITEXTURE_OVERLAY_BLEND_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE_ARRAY(struct multitexture_overlay_hud_element_definition, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct multitexture_overlay_hud_element_definition, functions,
		multitexture_overlay_effector_schema, MAXIMUM_EFFECTORS_PER_MULTITEXTURE_OVERLAY),
	TAG_SCHEMA_CHECK(multitexture_overlay_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const multitexture_overlay_schema =
	TAG_SCHEMA_DEFINITION(multitexture_overlay, struct multitexture_overlay_hud_element_definition,
		multitexture_overlay_fields);

static struct tag_schema_field const static_hud_element_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct static_hud_element_definition, interface_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct static_hud_element_definition, multitexture_overlays, multitexture_overlay_schema,
		MAXIMUM_MULTITEXTURE_OVERLAYS_PER_HUD_ELEMENT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const static_hud_element_schema =
	TAG_SCHEMA_DEFINITION(static_hud_element, struct static_hud_element_definition, static_hud_element_fields);

/* (one drawn only if its bitmap is not none) */
static struct tag_schema_field const optional_static_hud_element_fields[] =
{
	TAG_SCHEMA_STRUCT(struct optional_static_hud_element, element, static_hud_element_schema),
	TAG_SCHEMA_CHECK(optional_static_hud_element_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const optional_static_hud_element_schema =
	TAG_SCHEMA_DEFINITION(optional_static_hud_element, struct optional_static_hud_element,
		optional_static_hud_element_fields);

static struct tag_schema_field const meter_hud_element_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct meter_hud_element_definition, meter_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct meter_hud_element_definition, multitexture_overlays, multitexture_overlay_schema,
		MAXIMUM_MULTITEXTURE_OVERLAYS_PER_HUD_ELEMENT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const meter_hud_element_schema =
	TAG_SCHEMA_DEFINITION(meter_hud_element, struct meter_hud_element_definition, meter_hud_element_fields);

static struct tag_schema_field const optional_meter_hud_element_fields[] =
{
	TAG_SCHEMA_STRUCT(struct optional_meter_hud_element, element, meter_hud_element_schema),
	TAG_SCHEMA_CHECK(optional_meter_hud_element_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const optional_meter_hud_element_schema =
	TAG_SCHEMA_DEFINITION(optional_meter_hud_element, struct optional_meter_hud_element,
		optional_meter_hud_element_fields);

static struct tag_schema_field const hud_absolute_placement_fields[] =
{
	TAG_SCHEMA_ENUM(struct hud_absolute_placement_definition, corner, NUMBER_OF_HUD_ANCHORS, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_absolute_placement_schema =
	TAG_SCHEMA_DEFINITION(hud_absolute_placement, struct hud_absolute_placement_definition,
		hud_absolute_placement_fields);

static struct tag_schema_field const hud_sound_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct hud_sound_definition, sound, TAG_SCHEMA_GROUPS('snd!', 'lsnd')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_sound_schema =
	TAG_SCHEMA_DEFINITION(hud_sound, struct hud_sound_definition, hud_sound_fields);

static struct tag_schema_field const weapon_hud_overlay_item_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_overlay_item_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_overlay_item, struct weapon_hud_overlay_item, weapon_hud_overlay_item_fields);

static struct tag_schema_field const weapon_hud_overlay_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct weapon_hud_overlay_definition, bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct weapon_hud_overlay_definition, items, weapon_hud_overlay_item_schema,
		MAXIMUM_ITEMS_PER_HUD_OVERLAY),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_overlay_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_overlay, struct weapon_hud_overlay_definition, weapon_hud_overlay_fields);

/* grenade huds (the warning sounds and messaging icon are not read) */

static struct tag_schema_field const grenade_count_panel_fields[] =
{
	TAG_SCHEMA_STRUCT(struct grenade_count_panel_definition, background, optional_static_hud_element_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const grenade_count_panel_schema =
	TAG_SCHEMA_DEFINITION(grenade_count_panel, struct grenade_count_panel_definition, grenade_count_panel_fields);

static struct tag_schema_field const grenade_hud_interface_fields[] =
{
	TAG_SCHEMA_STRUCT(struct grenade_hud_interface_definition, absolute_placement, hud_absolute_placement_schema),
	TAG_SCHEMA_STRUCT(struct grenade_hud_interface_definition, background, optional_static_hud_element_schema),
	TAG_SCHEMA_STRUCT(struct grenade_hud_interface_definition, grenade_count_panel, grenade_count_panel_schema),
	TAG_SCHEMA_STRUCT(struct grenade_hud_interface_definition, overlays, weapon_hud_overlay_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const grenade_hud_interface_schema =
	TAG_SCHEMA_DEFINITION(grenade_hud_interface, struct grenade_hud_interface_definition,
		grenade_hud_interface_fields);

/* unit huds */

static struct tag_schema_field const metered_panel_fields[] =
{
	TAG_SCHEMA_STRUCT(struct metered_panel_definition, background, optional_static_hud_element_schema),
	TAG_SCHEMA_STRUCT(struct metered_panel_definition, meter, optional_meter_hud_element_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const metered_panel_schema =
	TAG_SCHEMA_DEFINITION(metered_panel, struct metered_panel_definition, metered_panel_fields);

/* (its type is a bit of the overlay types shown, of a word: hud_unit.c) */
static struct tag_schema_field const auxilary_overlay_fields[] =
{
	TAG_SCHEMA_STRUCT(struct auxilary_overlay_definition, static_element, static_hud_element_schema),
	TAG_SCHEMA_ENUM(struct auxilary_overlay_definition, type, NUMBER_OF_AUXILARY_OVERLAY_TYPE_BITS, 0),
	TAG_SCHEMA_CHECK(auxilary_overlay_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const auxilary_overlay_schema =
	TAG_SCHEMA_DEFINITION(auxilary_overlay, struct auxilary_overlay_definition, auxilary_overlay_fields);

/* (its type indexes the unit hud's auxilary_flash_time[1] and
auxilary_values[1]: hud_unit.c) */
static struct tag_schema_field const auxilary_meter_fields[] =
{
	TAG_SCHEMA_ENUM(struct auxilary_meter_definition, type, NUMBER_OF_AUXILARY_METER_TYPES, 0),
	TAG_SCHEMA_STRUCT(struct auxilary_meter_definition, panel, metered_panel_schema),
	TAG_SCHEMA_CHECK(auxilary_meter_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const auxilary_meter_schema =
	TAG_SCHEMA_DEFINITION(auxilary_meter, struct auxilary_meter_definition, auxilary_meter_fields);

static struct tag_schema_field const motion_sensor_panel_fields[] =
{
	TAG_SCHEMA_STRUCT(struct motion_sensor_panel_definition, background, optional_static_hud_element_schema),
	TAG_SCHEMA_STRUCT(struct motion_sensor_panel_definition, foreground, optional_static_hud_element_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const motion_sensor_panel_schema =
	TAG_SCHEMA_DEFINITION(motion_sensor_panel, struct motion_sensor_panel_definition, motion_sensor_panel_fields);

/* (warning sounds: the unit hud's last_sound_handles[12], hud_unit.c) */
static struct tag_schema_field const unit_hud_interface_fields[] =
{
	TAG_SCHEMA_STRUCT(struct unit_hud_interface_definition, absolute_placement, hud_absolute_placement_schema),
	TAG_SCHEMA_STRUCT(struct unit_hud_interface_definition, background, optional_static_hud_element_schema),
	TAG_SCHEMA_STRUCT(struct unit_hud_interface_definition, shield_meter, metered_panel_schema),
	TAG_SCHEMA_STRUCT(struct unit_hud_interface_definition, health_meter, metered_panel_schema),
	TAG_SCHEMA_STRUCT(struct unit_hud_interface_definition, motion_sensor, motion_sensor_panel_schema),
	TAG_SCHEMA_STRUCT(struct unit_hud_interface_definition, auxilary_panel.absolute_placement,
		hud_absolute_placement_schema),
	TAG_SCHEMA_BLOCK(struct unit_hud_interface_definition, auxilary_panel.auxilary_overlays, auxilary_overlay_schema,
		MAXIMUM_AUXILARY_OVERLAYS_PER_UNIT_HUD),
	TAG_SCHEMA_BLOCK(struct unit_hud_interface_definition, warning_sounds, hud_sound_schema,
		MAXIMUM_WARNING_SOUNDS_PER_UNIT_HUD),
	TAG_SCHEMA_BLOCK(struct unit_hud_interface_definition, auxilary_meters, auxilary_meter_schema,
		MAXIMUM_AUXILARY_METERS_PER_UNIT_HUD),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_hud_interface_schema =
	TAG_SCHEMA_DEFINITION(unit_hud_interface, struct unit_hud_interface_definition, unit_hud_interface_fields);

/* weapon huds (an element's state indexes the weapon hud's
NUMBER_OF_WEAPON_HUD_FLASH_REFERENCES states, its map type is a bit of
three, a crosshair's type one of NUMBER_OF_WEAPON_HUD_CROSSHAIR_STATES:
hud_weapon.c; the warning sounds are not read) */

static struct tag_schema_field const weapon_hud_element_header_fields[] =
{
	TAG_SCHEMA_ENUM(struct weapon_hud_element_header, state_type, NUMBER_OF_WEAPON_HUD_FLASH_REFERENCES, 0),
	TAG_SCHEMA_ENUM(struct weapon_hud_element_header, use_on_map_type, NUMBER_OF_WEAPON_HUD_MAP_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_element_header_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_element_header, struct weapon_hud_element_header,
		weapon_hud_element_header_fields);

static struct tag_schema_field const weapon_hud_static_element_fields[] =
{
	TAG_SCHEMA_STRUCT(struct weapon_hud_static_element, header, weapon_hud_element_header_schema),
	TAG_SCHEMA_STRUCT(struct weapon_hud_static_element, static_element, static_hud_element_schema),
	TAG_SCHEMA_CHECK(weapon_hud_static_element_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_static_element_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_static_element, struct weapon_hud_static_element,
		weapon_hud_static_element_fields);

static struct tag_schema_field const weapon_hud_meter_element_fields[] =
{
	TAG_SCHEMA_STRUCT(struct weapon_hud_meter_element, header, weapon_hud_element_header_schema),
	TAG_SCHEMA_STRUCT(struct weapon_hud_meter_element, meter_element, meter_hud_element_schema),
	TAG_SCHEMA_CHECK(weapon_hud_meter_element_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_meter_element_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_meter_element, struct weapon_hud_meter_element,
		weapon_hud_meter_element_fields);

static struct tag_schema_field const weapon_hud_number_element_fields[] =
{
	TAG_SCHEMA_STRUCT(struct weapon_hud_number_element, header, weapon_hud_element_header_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_number_element_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_number_element, struct weapon_hud_number_element,
		weapon_hud_number_element_fields);

static struct tag_schema_field const weapon_hud_overlays_element_fields[] =
{
	TAG_SCHEMA_ENUM(struct weapon_hud_overlays_element, state_type, NUMBER_OF_WEAPON_HUD_FLASH_REFERENCES, 0),
	TAG_SCHEMA_ENUM(struct weapon_hud_overlays_element, use_on_map_type, NUMBER_OF_WEAPON_HUD_MAP_TYPES, 0),
	TAG_SCHEMA_STRUCT(struct weapon_hud_overlays_element, overlays, weapon_hud_overlay_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_overlays_element_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_overlays_element, struct weapon_hud_overlays_element,
		weapon_hud_overlays_element_fields);

static struct tag_schema_field const weapon_hud_crosshair_item_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_crosshair_item_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_crosshair_item, struct weapon_hud_crosshair_item,
		weapon_hud_crosshair_item_fields);

static struct tag_schema_field const weapon_hud_crosshairs_element_fields[] =
{
	TAG_SCHEMA_ENUM(struct weapon_hud_crosshairs_element, crosshair_type, NUMBER_OF_WEAPON_HUD_CROSSHAIR_STATES, 0),
	TAG_SCHEMA_ENUM(struct weapon_hud_crosshairs_element, use_on_map_type, NUMBER_OF_WEAPON_HUD_MAP_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct weapon_hud_crosshairs_element, crosshairs.bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct weapon_hud_crosshairs_element, crosshairs.items, weapon_hud_crosshair_item_schema,
		MAXIMUM_ITEMS_PER_HUD_CROSSHAIR),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_crosshairs_element_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_crosshairs_element, struct weapon_hud_crosshairs_element,
		weapon_hud_crosshairs_element_fields);

/* (only the first screen effect is used: interface.c) */
static struct tag_schema_field const hud_screen_effect_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct hud_screen_effect_definition, mask_fullscreen, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct hud_screen_effect_definition, mask_splitscreen, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_screen_effect_schema =
	TAG_SCHEMA_DEFINITION(hud_screen_effect, struct hud_screen_effect_definition, hud_screen_effect_fields);

static struct tag_schema_field const weapon_hud_interface_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct weapon_hud_interface_definition, parent_hud, TAG_SCHEMA_GROUPS('wphi')),
	TAG_SCHEMA_STRUCT(struct weapon_hud_interface_definition, absolute_placement, hud_absolute_placement_schema),
	TAG_SCHEMA_BLOCK(struct weapon_hud_interface_definition, statics, weapon_hud_static_element_schema,
		MAXIMUM_ELEMENTS_PER_WEAPON_HUD),
	TAG_SCHEMA_BLOCK(struct weapon_hud_interface_definition, meters, weapon_hud_meter_element_schema,
		MAXIMUM_ELEMENTS_PER_WEAPON_HUD),
	TAG_SCHEMA_BLOCK(struct weapon_hud_interface_definition, numbers, weapon_hud_number_element_schema,
		MAXIMUM_ELEMENTS_PER_WEAPON_HUD),
	TAG_SCHEMA_BLOCK(struct weapon_hud_interface_definition, crosshairs, weapon_hud_crosshairs_element_schema,
		MAXIMUM_CROSSHAIRS_PER_WEAPON_HUD),
	TAG_SCHEMA_BLOCK(struct weapon_hud_interface_definition, overlays, weapon_hud_overlays_element_schema,
		MAXIMUM_ELEMENTS_PER_WEAPON_HUD),
	TAG_SCHEMA_BLOCK(struct weapon_hud_interface_definition, screen_effects, hud_screen_effect_schema,
		MAXIMUM_SCREEN_EFFECTS_PER_WEAPON_HUD),
	TAG_SCHEMA_CHECK(weapon_hud_interface_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_hud_interface_schema =
	TAG_SCHEMA_DEFINITION(weapon_hud_interface, struct weapon_hud_interface_definition,
		weapon_hud_interface_fields);

/* hud numbers (found through the globals' interface tags: game_globals_check) */

static struct tag_schema_field const hud_number_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct hud_number_definition, number_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_number_schema =
	TAG_SCHEMA_DEFINITION(hud_number, struct hud_number_definition, hud_number_fields);

/* hud message text (hud_messaging.c: hud_message_text_check) */

static struct tag_schema_field const hud_state_message_element_fields[] =
{
	TAG_SCHEMA_ENUM(struct hud_state_message_element, type, NUMBER_OF_HUD_MESSAGE_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_state_message_element_schema =
	TAG_SCHEMA_DEFINITION(hud_state_message_element, struct hud_state_message_element,
		hud_state_message_element_fields);

static struct tag_schema_field const hud_state_message_fields[] =
{
	TAG_SCHEMA_STRING(struct hud_state_message_definition, name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_state_message_schema =
	TAG_SCHEMA_DEFINITION(hud_state_message, struct hud_state_message_definition, hud_state_message_fields);

static struct tag_schema_field const hud_message_text_fields[] =
{
	TAG_SCHEMA_DATA(struct hud_message_text_definition, text_data, MAXIMUM_HUD_MESSAGE_TEXT_DATA_SIZE),
	TAG_SCHEMA_BLOCK(struct hud_message_text_definition, elements, hud_state_message_element_schema,
		MAXIMUM_HUD_MESSAGE_ELEMENTS),
	TAG_SCHEMA_BLOCK(struct hud_message_text_definition, messages, hud_state_message_schema,
		MAXIMUM_HUD_MESSAGES),
	TAG_SCHEMA_CHECK(hud_message_text_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_message_text_schema =
	TAG_SCHEMA_DEFINITION(hud_message_text, struct hud_message_text_definition, hud_message_text_fields);

/* hud globals */

static struct tag_schema_field const icon_hud_element_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const icon_hud_element_schema =
	TAG_SCHEMA_DEFINITION(icon_hud_element, struct icon_hud_element_definition, icon_hud_element_fields);

static struct tag_schema_field const hud_waypoint_arrow_fields[] =
{
	TAG_SCHEMA_STRING(struct hud_waypoint_arrow, name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_waypoint_arrow_schema =
	TAG_SCHEMA_DEFINITION(hud_waypoint_arrow, struct hud_waypoint_arrow, hud_waypoint_arrow_fields);

/* (button icons: an icon type, hud_messaging.c; arrows: the tool's) */
static struct tag_schema_field const hud_globals_fields[] =
{
	TAG_SCHEMA_STRUCT(struct hud_globals_definition, messaging.absolute_placement, hud_absolute_placement_schema),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, messaging.single_player_font, TAG_SCHEMA_GROUPS('font')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, messaging.multi_player_font, TAG_SCHEMA_GROUPS('font')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, messaging.hud_item_messages, TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, messaging.messaging_icons, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, messaging.alternate_icon_text, TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_BLOCK(struct hud_globals_definition, messaging.button_icons, icon_hud_element_schema,
		MAXIMUM_HUD_BUTTON_ICONS),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, messaging.hud_messages, TAG_SCHEMA_GROUPS('hmt ')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, waypoint.arrow_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct hud_globals_definition, waypoint.arrows, hud_waypoint_arrow_schema,
		MAXIMUM_HUD_WAYPOINT_ARROWS),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, defaults.default_weapon_hud, TAG_SCHEMA_GROUPS('wphi')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, damage_indicators.indicator_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, carnage_report_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct hud_globals_definition, checkpoint_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_CHECK(hud_globals_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const hud_globals_schema =
	TAG_SCHEMA_DEFINITION(hud_globals, struct hud_globals_definition, hud_globals_fields);

/* meters (metr): the game reads none of them */

/* fonts (font_group.c, draw_string.c, rasterizer_text.c: a character is
found through the table of its high byte, whose entry is an index into the
characters, both looked for against their counts; its pixels are read
from the pixels at its offset, unchecked: font_check) */

static struct tag_schema_field const font_character_index_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct font_character_index, index, TAG_SCHEMA_ROOT,
		offsetof(struct font_header, characters), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const font_character_index_schema =
	TAG_SCHEMA_DEFINITION(font_character_index, struct font_character_index, font_character_index_fields);

/* (a table is used only if it has an entry for each low byte) */
static struct tag_schema_field const font_character_table_fields[] =
{
	TAG_SCHEMA_BLOCK(struct font_character_table, character_indices, font_character_index_schema,
		MAXIMUM_CHARACTERS_PER_FONT_CHARACTER_TABLE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const font_character_table_schema =
	TAG_SCHEMA_DEFINITION(font_character_table, struct font_character_table, font_character_table_fields);

/* (the character's place in the rasterizer's character cache is the
rasterizer's, and its pad the cache's mark: rasterizer_text.c) */
static struct tag_schema_field const font_character_fields[] =
{
	TAG_SCHEMA_RESET(struct font_character, hardware_character_index, NONE),
	TAG_SCHEMA_RESET(struct font_character, pad, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const font_character_schema =
	TAG_SCHEMA_DEFINITION(font_character, struct font_character, font_character_fields);

/* (character tables: one for each high byte of a 16-bit character;
characters: a table's entries are shorts) */
static struct tag_schema_field const font_fields[] =
{
	TAG_SCHEMA_BLOCK(struct font_header, character_tables, font_character_table_schema,
		MAXIMUM_FONT_CHARACTER_TABLES),
	TAG_SCHEMA_REFERENCE_ARRAY(struct font_header, style_fonts, TAG_SCHEMA_GROUPS('font')),
	TAG_SCHEMA_BLOCK(struct font_header, characters, font_character_schema, MAXIMUM_CHARACTERS_PER_FONT),
	TAG_SCHEMA_DATA(struct font_header, pixels, MAXIMUM_FONT_PIXELS_SIZE),
	TAG_SCHEMA_CHECK(font_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const font_schema =
	TAG_SCHEMA_DEFINITION(font, struct font_header, font_fields);

/* string lists (text_group.c: a string is looked for against the count, and
terminated in its data as it is got; its index is a short) */

static struct tag_schema_field const string_list_entry_fields[] =
{
	TAG_SCHEMA_DATA(struct string_list_entry, string, SHORT_MAX),
	TAG_SCHEMA_CHECK(string_list_entry_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const string_list_entry_schema =
	TAG_SCHEMA_DEFINITION(string_list_entry, struct string_list_entry, string_list_entry_fields);

static struct tag_schema_field const string_list_fields[] =
{
	TAG_SCHEMA_BLOCK(struct string_list, strings, string_list_entry_schema, SHORT_MAX),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const string_list_schema =
	TAG_SCHEMA_DEFINITION(string_list, struct string_list, string_list_fields);

static struct tag_schema_field const unicode_string_list_entry_fields[] =
{
	TAG_SCHEMA_DATA(struct string_list_entry, string, 2 * SHORT_MAX),
	TAG_SCHEMA_CHECK(unicode_string_list_entry_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unicode_string_list_entry_schema =
	TAG_SCHEMA_DEFINITION(unicode_string_list_entry, struct string_list_entry, unicode_string_list_entry_fields);

static struct tag_schema_field const unicode_string_list_fields[] =
{
	TAG_SCHEMA_BLOCK(struct string_list, strings, unicode_string_list_entry_schema, SHORT_MAX),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unicode_string_list_schema =
	TAG_SCHEMA_DEFINITION(unicode_string_list, struct string_list, unicode_string_list_fields);

/* ui widgets (ui_widget.c: every block is walked to its count; the
functions are looked for against their tables (or the port's names) by
the game; a widget's children are loaded with it, each with its own,
with no end but the tags': ui_widget_check) */

static struct tag_schema_field const ui_widget_game_data_input_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_game_data_input_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_game_data_input, struct ui_widget_game_data_input_reference,
		ui_widget_game_data_input_fields);

static struct tag_schema_field const ui_widget_event_handler_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct ui_widget_event_handler_reference, widget_tag, TAG_SCHEMA_GROUPS('DeLa')),
	TAG_SCHEMA_REFERENCE(struct ui_widget_event_handler_reference, sound_effect, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_STRING(struct ui_widget_event_handler_reference, script),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_event_handler_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_event_handler, struct ui_widget_event_handler_reference,
		ui_widget_event_handler_fields);

static struct tag_schema_field const ui_widget_search_and_replace_fields[] =
{
	TAG_SCHEMA_STRING(struct ui_widget_search_and_replace_reference, search_string),
	TAG_SCHEMA_CHECK(ui_widget_search_and_replace_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_search_and_replace_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_search_and_replace, struct ui_widget_search_and_replace_reference,
		ui_widget_search_and_replace_fields);

static struct tag_schema_field const ui_widget_conditional_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct ui_widget_conditional_reference, widget_tag, TAG_SCHEMA_GROUPS('DeLa')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_conditional_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_conditional, struct ui_widget_conditional_reference,
		ui_widget_conditional_fields);

static struct tag_schema_field const ui_widget_child_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct ui_widget_child_reference, widget_tag, TAG_SCHEMA_GROUPS('DeLa')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_child_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_child, struct ui_widget_child_reference, ui_widget_child_fields);

/* (the controller indexes the four players' widgets, or is any: an
index past them is none of the launch's cases, which leaves the player
unset: ui_widget_launch_widget) */
static struct tag_schema_field const ui_widget_fields[] =
{
	TAG_SCHEMA_ENUM(struct ui_widget_definition, type, NUMBER_OF_UI_WIDGET_TYPES, 0),
	TAG_SCHEMA_ENUM(struct ui_widget_definition, controller_index, NUMBER_OF_WIDGET_CONTROLLERS, 0),
	TAG_SCHEMA_STRING(struct ui_widget_definition, name),
	TAG_SCHEMA_REFERENCE(struct ui_widget_definition, background_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct ui_widget_definition, game_data_inputs, ui_widget_game_data_input_schema,
		MAXIMUM_GAME_DATA_INPUTS_PER_WIDGET),
	TAG_SCHEMA_BLOCK(struct ui_widget_definition, event_handlers, ui_widget_event_handler_schema,
		MAXIMUM_EVENT_HANDLERS_PER_WIDGET),
	TAG_SCHEMA_BLOCK(struct ui_widget_definition, search_and_replace_functions, ui_widget_search_and_replace_schema,
		MAXIMUM_SEARCH_AND_REPLACE_FUNCTIONS_PER_WIDGET),
	TAG_SCHEMA_REFERENCE(struct ui_widget_definition, text_label_string_list, TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_REFERENCE(struct ui_widget_definition, text_font, TAG_SCHEMA_GROUPS('font')),
	TAG_SCHEMA_ENUM(struct ui_widget_definition, justification, NUMBER_OF_WIDGET_TEXT_JUSTIFICATIONS, 0),
	TAG_SCHEMA_REFERENCE(struct ui_widget_definition, list_header_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct ui_widget_definition, list_footer_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct ui_widget_definition, extended_description_widget, TAG_SCHEMA_GROUPS('DeLa')),
	TAG_SCHEMA_BLOCK(struct ui_widget_definition, conditional_widgets, ui_widget_conditional_schema,
		MAXIMUM_CONDITIONAL_WIDGETS_PER_WIDGET),
	TAG_SCHEMA_BLOCK(struct ui_widget_definition, child_widgets, ui_widget_child_schema,
		MAXIMUM_CHILD_WIDGETS_PER_WIDGET),
	TAG_SCHEMA_CHECK(ui_widget_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_schema =
	TAG_SCHEMA_DEFINITION(ui_widget, struct ui_widget_definition, ui_widget_fields);

/* ui widget collections (only the port reads them: menu_tags.c) */

static struct tag_schema_field const ui_widget_collection_widget_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct ui_widget_collection_widget, widget, TAG_SCHEMA_GROUPS('DeLa')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_collection_widget_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_collection_widget, struct ui_widget_collection_widget,
		ui_widget_collection_widget_fields);

static struct tag_schema_field const ui_widget_collection_fields[] =
{
	TAG_SCHEMA_BLOCK(struct ui_widget_collection, widgets, ui_widget_collection_widget_schema,
		MAXIMUM_WIDGETS_PER_COLLECTION),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const ui_widget_collection_schema =
	TAG_SCHEMA_DEFINITION(ui_widget_collection, struct ui_widget_collection, ui_widget_collection_fields);

/* virtual keyboards (virtual_keyboard.c: the keys are read by place,
each of the NUMBER_OF_VIRTUAL_KEYS, unchecked: virtual_keyboard_check) */

static struct tag_schema_field const virtual_keyboard_key_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_key, unselected_background_bitmap_tag, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_key, selected_background_bitmap_tag, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_key, active_background_bitmap_tag, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_key, sticky_background_bitmap_tag, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const virtual_keyboard_key_schema =
	TAG_SCHEMA_DEFINITION(virtual_keyboard_key, struct virtual_keyboard_key, virtual_keyboard_key_fields);

static struct tag_schema_field const virtual_keyboard_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_definition, font_tag, TAG_SCHEMA_GROUPS('font')),
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_definition, background_bitmap_tag, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct virtual_keyboard_definition, special_key_labels_string_list_tag,
		TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_BLOCK(struct virtual_keyboard_definition, keys, virtual_keyboard_key_schema, NUMBER_OF_VIRTUAL_KEYS),
	TAG_SCHEMA_CHECK(virtual_keyboard_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const virtual_keyboard_schema =
	TAG_SCHEMA_DEFINITION(virtual_keyboard, struct virtual_keyboard_definition, virtual_keyboard_fields);

/* multiplayer scenario descriptions (the game reads none of them; the
tool's element) */

static struct tag_schema_field const multiplayer_scenario_description_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct multiplayer_scenario_description_entry, descriptive_bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct multiplayer_scenario_description_entry, displayed_map_name, TAG_SCHEMA_GROUPS('ustr')),
	TAG_SCHEMA_STRING(struct multiplayer_scenario_description_entry, scenario_tag_path),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const multiplayer_scenario_description_schema =
	TAG_SCHEMA_DEFINITION(multiplayer_scenario_description, struct multiplayer_scenario_description_entry,
		multiplayer_scenario_description_fields);

static struct tag_schema_field const multiplayer_scenario_descriptions_fields[] =
{
	TAG_SCHEMA_BLOCK(struct multiplayer_scenario_description, scenarios, multiplayer_scenario_description_schema,
		MAXIMUM_MULTIPLAYER_SCENARIO_DESCRIPTIONS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const multiplayer_scenario_descriptions_schema =
	TAG_SCHEMA_DEFINITION(multiplayer_scenario_descriptions, struct multiplayer_scenario_description,
		multiplayer_scenario_descriptions_fields);

/* the globals (game_globals.c and their readers: most blocks' first
element is read without its count, or a constant's, which past the block
is the empty element; game_globals_check) */

static struct tag_schema_field const tag_reference_sound_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct tag_reference_element, reference, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const tag_reference_sound_schema =
	TAG_SCHEMA_DEFINITION(tag_reference_sound, struct tag_reference_element, tag_reference_sound_fields);

static struct tag_schema_field const tag_reference_camera_track_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct tag_reference_element, reference, TAG_SCHEMA_GROUPS('trak')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const tag_reference_camera_track_schema =
	TAG_SCHEMA_DEFINITION(tag_reference_camera_track, struct tag_reference_element,
		tag_reference_camera_track_fields);

static struct tag_schema_field const tag_reference_item_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct tag_reference_element, reference, TAG_SCHEMA_GROUPS('item')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const tag_reference_item_schema =
	TAG_SCHEMA_DEFINITION(tag_reference_item, struct tag_reference_element, tag_reference_item_fields);

static struct tag_schema_field const tag_reference_object_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct tag_reference_element, reference, TAG_SCHEMA_GROUPS('obje')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const tag_reference_object_schema =
	TAG_SCHEMA_DEFINITION(tag_reference_object, struct tag_reference_element, tag_reference_object_fields);

static struct tag_schema_field const look_function_value_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const look_function_value_schema =
	TAG_SCHEMA_DEFINITION(look_function_value, real, look_function_value_fields);

static struct tag_schema_field const game_globals_player_control_fields[] =
{
	TAG_SCHEMA_BLOCK(struct game_globals_player_control, look_function, look_function_value_schema,
		MAXIMUM_LOOK_FUNCTION_VALUES),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_player_control_schema =
	TAG_SCHEMA_DEFINITION(game_globals_player_control, struct game_globals_player_control,
		game_globals_player_control_fields);

static struct tag_schema_field const game_globals_difficulty_information_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_difficulty_information_schema =
	TAG_SCHEMA_DEFINITION(game_globals_difficulty_information, struct game_globals_difficulty_information,
		game_globals_difficulty_information_fields);

static struct tag_schema_field const game_globals_grenade_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_grenade, throwing_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct game_globals_grenade, hud_interface, TAG_SCHEMA_GROUPS('grhi')),
	TAG_SCHEMA_REFERENCE(struct game_globals_grenade, item, TAG_SCHEMA_GROUPS('eqip')),
	TAG_SCHEMA_REFERENCE(struct game_globals_grenade, projectile, TAG_SCHEMA_GROUPS('proj')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_grenade_schema =
	TAG_SCHEMA_DEFINITION(game_globals_grenade, struct game_globals_grenade, game_globals_grenade_fields);

static struct tag_schema_field const game_globals_rasterizer_data_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, distance_attenuation, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, vector_normalization, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, atmospheric_fog_density, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, planar_fog_density, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, linear_corner_fade, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, active_camouflage_distortion,
		TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, glow, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE_ARRAY(struct game_globals_rasterizer_data, default_textures, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE_ARRAY(struct game_globals_rasterizer_data, test, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, screen_effect_video_scanline_map,
		TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, screen_effect_video_noise_map,
		TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct game_globals_rasterizer_data, distance_attenuation_2d_for_the_pc,
		TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_rasterizer_data_schema =
	TAG_SCHEMA_DEFINITION(game_globals_rasterizer_data, struct game_globals_rasterizer_data,
		game_globals_rasterizer_data_fields);

/* (interface.h's tags, each of its group) */
#define INTERFACE_TAG_REFERENCE(index, groups) \
	TAG_SCHEMA_REFERENCE(struct game_globals_interface_tag_references, interface_tag_references[index], groups)

static struct tag_schema_field const game_globals_interface_tag_references_fields[] =
{
	INTERFACE_TAG_REFERENCE(_interface_font_system, TAG_SCHEMA_GROUPS('font')),
	INTERFACE_TAG_REFERENCE(_interface_font_terminal, TAG_SCHEMA_GROUPS('font')),
	INTERFACE_TAG_REFERENCE(_interface_color_table_screen, TAG_SCHEMA_GROUPS('colo')),
	INTERFACE_TAG_REFERENCE(_interface_color_table_hud, TAG_SCHEMA_GROUPS('colo')),
	INTERFACE_TAG_REFERENCE(_interface_color_table_editor, TAG_SCHEMA_GROUPS('colo')),
	INTERFACE_TAG_REFERENCE(_interface_color_table_dialog, TAG_SCHEMA_GROUPS('colo')),
	INTERFACE_TAG_REFERENCE(_interface_hud_globals, TAG_SCHEMA_GROUPS('hudg')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_motion_sweep, TAG_SCHEMA_GROUPS('bitm')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_motion_sweep_mask, TAG_SCHEMA_GROUPS('bitm')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_multiplayer_hud, TAG_SCHEMA_GROUPS('bitm')),
	INTERFACE_TAG_REFERENCE(_interface_string_list_localization, TAG_SCHEMA_GROUPS('str#')),
	INTERFACE_TAG_REFERENCE(_interface_hud_digits, TAG_SCHEMA_GROUPS('hud#')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_motion_blip, TAG_SCHEMA_GROUPS('bitm')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_iface_map1, TAG_SCHEMA_GROUPS('bitm')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_iface_map2, TAG_SCHEMA_GROUPS('bitm')),
	INTERFACE_TAG_REFERENCE(_interface_bitmap_iface_map3, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(game_globals_interface_tag_references_check),
	TAG_SCHEMA_END
};

#undef INTERFACE_TAG_REFERENCE

static struct tag_schema_definition const game_globals_interface_tag_references_schema =
	TAG_SCHEMA_DEFINITION(game_globals_interface_tag_references, struct game_globals_interface_tag_references,
		game_globals_interface_tag_references_fields);

static struct tag_schema_field const game_globals_vehicle_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_vehicle, vehicle, TAG_SCHEMA_GROUPS('unit')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_vehicle_schema =
	TAG_SCHEMA_DEFINITION(game_globals_vehicle, struct game_globals_vehicle, game_globals_vehicle_fields);

/* (its sounds are game_engine_multiplayer_sounds.c's, looked for against
the count) */
static struct tag_schema_field const game_globals_multiplayer_information_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_multiplayer_information, flag, TAG_SCHEMA_GROUPS('item')),
	TAG_SCHEMA_REFERENCE(struct game_globals_multiplayer_information, unit, TAG_SCHEMA_GROUPS('unit')),
	TAG_SCHEMA_BLOCK(struct game_globals_multiplayer_information, vehicles, game_globals_vehicle_schema,
		MAXIMUM_MULTIPLAYER_VEHICLES),
	TAG_SCHEMA_REFERENCE(struct game_globals_multiplayer_information, hill_shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_REFERENCE(struct game_globals_multiplayer_information, flag_shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_REFERENCE(struct game_globals_multiplayer_information, ball, TAG_SCHEMA_GROUPS('item')),
	TAG_SCHEMA_BLOCK(struct game_globals_multiplayer_information, sounds, tag_reference_sound_schema,
		MAXIMUM_MULTIPLAYER_SOUNDS),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_multiplayer_information_schema =
	TAG_SCHEMA_DEFINITION(game_globals_multiplayer_information, struct game_globals_multiplayer_information,
		game_globals_multiplayer_information_fields);

static struct tag_schema_field const game_globals_player_information_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_player_information, player_unit, TAG_SCHEMA_GROUPS('unit')),
	TAG_SCHEMA_REFERENCE(struct game_globals_player_information, coop_respawn_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_player_information_schema =
	TAG_SCHEMA_DEFINITION(game_globals_player_information, struct game_globals_player_information,
		game_globals_player_information_fields);

static struct tag_schema_field const game_globals_first_person_interface_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_first_person_interface, hands, TAG_SCHEMA_GROUPS('mode')),
	TAG_SCHEMA_REFERENCE(struct game_globals_first_person_interface, night_vision_off_on_effect,
		TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct game_globals_first_person_interface, night_vision_on_off_effect,
		TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_first_person_interface_schema =
	TAG_SCHEMA_DEFINITION(game_globals_first_person_interface, struct game_globals_first_person_interface,
		game_globals_first_person_interface_fields);

/* (its runtime velocities are the tool's, from its distances) */
static struct tag_schema_field const game_globals_falling_damage_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct game_globals_falling_damage, falling_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct game_globals_falling_damage, maximum_distance_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct game_globals_falling_damage, vehicle_hit_environment_damage_effect,
		TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct game_globals_falling_damage, vehicle_killed_unit_damage_effect,
		TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct game_globals_falling_damage, vehicle_collision_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct game_globals_falling_damage, flaming_death_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_falling_damage_schema =
	TAG_SCHEMA_DEFINITION(game_globals_falling_damage, struct game_globals_falling_damage,
		game_globals_falling_damage_fields);

static struct tag_schema_field const breakable_surface_particle_effect_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct breakable_surface_particle_effect, particle, TAG_SCHEMA_GROUPS('part')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const breakable_surface_particle_effect_schema =
	TAG_SCHEMA_DEFINITION(breakable_surface_particle_effect, struct breakable_surface_particle_effect,
		breakable_surface_particle_effect_fields);

static struct tag_schema_field const material_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct material_definition, breakable_surface.effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct material_definition, breakable_surface.sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_BLOCK(struct material_definition, breakable_surface.particle_effects,
		breakable_surface_particle_effect_schema, MAXIMUM_PARTICLE_EFFECTS_PER_BREAKABLE_SURFACE),
	TAG_SCHEMA_REFERENCE(struct material_definition, melee_hit_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const material_schema =
	TAG_SCHEMA_DEFINITION(material, struct material_definition, material_fields);

/* (sounds: entering and leaving water; grenades: by grenade type;
materials: by material type; the rest: their first; the playlist is not
read) */
static struct tag_schema_field const game_globals_fields[] =
{
	TAG_SCHEMA_BLOCK(struct game_globals, sounds, tag_reference_sound_schema, MAXIMUM_GLOBAL_SOUNDS),
	TAG_SCHEMA_BLOCK(struct game_globals, camera, tag_reference_camera_track_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, player_control, game_globals_player_control_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, difficulty_information, game_globals_difficulty_information_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, grenades, game_globals_grenade_schema, NUMBER_OF_UNIT_GRENADE_TYPES),
	TAG_SCHEMA_BLOCK(struct game_globals, rasterizer_data, game_globals_rasterizer_data_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, interface_tag_references, game_globals_interface_tag_references_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, weapon_list, tag_reference_item_schema, MAXIMUM_GLOBAL_WEAPONS),
	TAG_SCHEMA_BLOCK(struct game_globals, cheat_powerups, tag_reference_object_schema, MAXIMUM_CHEAT_POWERUPS),
	TAG_SCHEMA_BLOCK(struct game_globals, multiplayer_information, game_globals_multiplayer_information_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, player_information, game_globals_player_information_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, first_person_interface, game_globals_first_person_interface_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, falling_damage, game_globals_falling_damage_schema, 1),
	TAG_SCHEMA_BLOCK(struct game_globals, materials, material_schema, NUMBER_OF_MATERIAL_TYPES),
	TAG_SCHEMA_CHECK(game_globals_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const game_globals_schema =
	TAG_SCHEMA_DEFINITION(game_globals, struct game_globals, game_globals_fields);

/* the groups */

struct tag_schema_group const tag_schema_effect_groups[] =
{
	{ 'effe', { NONE, NONE }, &effect_schema },
	{ 'jpt!', { NONE, NONE }, &damage_effect_schema },
	{ 'cdmg', { NONE, NONE }, &continuous_damage_effect_schema },
	{ 'foot', { NONE, NONE }, &material_effects_schema },
	{ 'snd!', { NONE, NONE }, &sound_schema },
	{ 'lsnd', { NONE, NONE }, &looping_sound_schema },
	{ 'snde', { NONE, NONE }, &sound_environment_schema },
	{ 'grhi', { NONE, NONE }, &grenade_hud_interface_schema },
	{ 'unhi', { NONE, NONE }, &unit_hud_interface_schema },
	{ 'wphi', { NONE, NONE }, &weapon_hud_interface_schema },
	{ 'hud#', { NONE, NONE }, &hud_number_schema },
	{ 'hudg', { NONE, NONE }, &hud_globals_schema },
	{ 'hmt ', { NONE, NONE }, &hud_message_text_schema },
	{ 'metr', { NONE, NONE }, NULL },
	{ 'mgs2', { NONE, NONE }, &light_volume_schema },
	{ 'font', { NONE, NONE }, &font_schema },
	{ 'str#', { NONE, NONE }, &string_list_schema },
	{ 'ustr', { NONE, NONE }, &unicode_string_list_schema },
	{ 'DeLa', { NONE, NONE }, &ui_widget_schema },
	{ 'Soul', { NONE, NONE }, &ui_widget_collection_schema },
	{ 'vcky', { NONE, NONE }, &virtual_keyboard_schema },
	{ 'trak', { NONE, NONE }, &camera_track_schema },
	{ 'matg', { NONE, NONE }, &game_globals_schema },
	{ 'mply', { NONE, NONE }, &multiplayer_scenario_descriptions_schema },
	{ 0 }
};

/* ---------- private code */

/* a part's base class is what the game makes of its tag (effects.c,
effect_generate_part): it must be the tag's */
static boolean effect_part_check(
	struct tag_validation *validation,
	void *base)
{
	static unsigned long const base_classes[] = { 'pctl', 'snd!', 'obje', 'deca', 'jpt!', 'ligh' };
	struct effect_part_definition *part = base;
	short index;

	if (part->reference.index == NONE)
		return TRUE;
	for (index = 0; index < (short)NUMBEROF(base_classes); index++)
	{
		if (tag_validate_tag_get(validation, part->reference.index, base_classes[index]))
		{
			if (part->runtime_base_class_tag != base_classes[index])
			{
				tag_validate_correct(validation, "has a part of base class %08lx for a tag of '%c%c%c%c'",
					part->runtime_base_class_tag,
					(char)(base_classes[index] >> 24), (char)(base_classes[index] >> 16),
					(char)(base_classes[index] >> 8), (char)base_classes[index]);
				part->runtime_base_class_tag = base_classes[index];
			}
			return TRUE;
		}
	}
	/* (the values pass let none other through) */
	part->reference.index = NONE;

	return TRUE;
}

/* the tag index of the tag of group_tag whose root is at root, from the
tags' header (where the tags start: the region the checks may look in
begins with it), or NONE */
static long tag_index_get(
	struct tag_validation *validation,
	void const *root,
	unsigned long group_tag)
{
	/* (cache_files.c's struct cache_file_tag_instance and the start of its
	struct cache_file_tag_header, as tag_validate.c has them) */
	struct tag_instance
	{
		unsigned long group_tag;
		unsigned long parent_group_tags[2];
		long tag_index;
		char *name;
		void *base_address;
		unsigned long unused[2];
	};
	struct tag_header
	{
		struct tag_instance *instances;
		long scenario_tag_index;
		unsigned long checksum;
		long tag_count;
	};
	unsigned long low = 0;
	unsigned long high = (unsigned long)root;
	struct tag_header const *header;
	long index;

	if (!tag_validate_contains(validation, root, 1))
		return NONE;
	/* (the region's first byte) */
	while (low < high)
	{
		unsigned long middle = low + (high - low) / 2;

		if (tag_validate_contains(validation, (void const *)middle, 1))
			high = middle;
		else
			low = middle + 1;
	}
	header = (struct tag_header const *)low;
	if (!tag_validate_contains(validation, header, sizeof(*header)) ||
		header->tag_count <= 0 || header->tag_count > UNSIGNED_SHORT_MAX ||
		!tag_validate_contains(validation, header->instances,
			(unsigned long)header->tag_count * sizeof(struct tag_instance)))
	{
		return NONE;
	}
	for (index = 0; index < header->tag_count; index++)
	{
		struct tag_instance const *instance = &header->instances[index];

		if (instance->base_address == root && instance->group_tag == group_tag)
			return instance->tag_index;
	}

	return NONE;
}

/* a sound: its permutations' tag indices are its own (the sound cache reads
their samples as the tag's, and names it by them); samples of less than a
block of each channel cannot be played (the first packet would end past
them: sound_dsound_xbox.c, dsound_channel_queue_packet), nor can a pitch
range choose from more permutations than its played bits hold; a promotion
is counted in a long, and promotes for ever if the count never fills
(sound_manager.c, sound_definition_promotion) */
static boolean sound_check(
	struct tag_validation *validation,
	void *base)
{
	struct sound_definition *definition = base;
	long tag_index = NONE;
	boolean tag_index_known = FALSE;
	long block_size = SOUND_COMPRESSED_BLOCK_SIZE * (definition->encoding == _sound_encoding_stereo ? 2 : 1);
	long range_index;

	if (definition->promotion_sound.index != NONE &&
		(definition->promotion_count < 0 || definition->longest_permutation_length < 0 ||
			(definition->longest_permutation_length &&
				definition->promotion_count > LONG_MAX / definition->longest_permutation_length)))
	{
		tag_validate_correct(validation, "promotes after %d of its %ld milliseconds: none",
			definition->promotion_count, definition->longest_permutation_length);
		definition->promotion_sound.index = NONE;
	}
	for (range_index = 0; range_index < definition->pitch_ranges.count; range_index++)
	{
		struct sound_pitch_range *range = (struct sound_pitch_range *)definition->pitch_ranges.address + range_index;
		long most = MIN(range->permutations.count, MAXIMUM_PLAYED_PERMUTATIONS_PER_PITCH_RANGE);
		long permutation_index;

		if (range->permutations.count &&
			(range->actual_permutation_count < 1 || range->actual_permutation_count > most))
		{
			tag_validate_correct(validation, "has pitch range %ld choosing from %d of %ld permutations",
				range_index, range->actual_permutation_count, range->permutations.count);
			range->actual_permutation_count = (short)PIN(range->actual_permutation_count, 1, most);
		}
		for (permutation_index = 0; permutation_index < range->permutations.count; permutation_index++)
		{
			struct sound_permutation *permutation =
				(struct sound_permutation *)range->permutations.address + permutation_index;

			/* (cache_tag_index and runtime_tag_index) */
			if (tag_validate_tag_get(validation, (long)permutation->unknown2, SOUND_DEFINITION_TAG) != base ||
				permutation->unknown3 != permutation->unknown2)
			{
				if (!tag_index_known)
				{
					tag_index = tag_index_get(validation, base, SOUND_DEFINITION_TAG);
					tag_index_known = TRUE;
				}
				tag_validate_correct(validation, "has permutation %ld of pitch range %ld of tag %08lx, not %08lx",
					permutation_index, range_index, permutation->unknown2, tag_index);
				permutation->unknown2 = (unsigned long)tag_index;
				permutation->unknown3 = (unsigned long)tag_index;
			}
			/* (only that permutation: one of no samples is not loaded,
			xbox_sound_cache.c) */
			if (definition->compression == _sound_compression_xbox_adpcm &&
				permutation->samples.size > 0 && permutation->samples.size < block_size)
			{
				tag_validate_correct(validation, "has permutation %ld of pitch range %ld of %ld bytes: not played",
					permutation_index, range_index, permutation->samples.size);
				permutation->samples.size = 0;
			}
		}
	}

	return TRUE;
}

/* the bitmap group bitmap_index names, if it is one */
static struct bitmap_group *bitmap_group_get_checked(
	struct tag_validation *validation,
	long bitmap_index)
{
	return tag_validate_tag_get(validation, bitmap_index, 'bitm');
}

/* whether the game finds a bitmap for a frame of a sequence of a bitmap
group (bitmap_group.c, bitmap_group_get_bitmap_from_sequence) */
static boolean bitmap_from_sequence_exists(
	struct tag_validation *validation,
	long bitmap_index,
	short sequence_index,
	short frame_index)
{
	struct bitmap_group *group = bitmap_group_get_checked(validation, bitmap_index);
	long index = NONE;

	if (!group)
		return FALSE;
	if (group->sequences.count > 0 && sequence_index >= 0)
	{
		struct bitmap_group_sequence *sequence =
			(struct bitmap_group_sequence *)group->sequences.address + sequence_index % group->sequences.count;

		if (!tag_validate_contains(validation, sequence, sizeof(*sequence)))
			return FALSE;
		if (sequence->bitmap_count > 0)
		{
			index = (short)(frame_index % sequence->bitmap_count + sequence->first_bitmap_index);
		}
		else if (sequence->sprites.count && frame_index >= 0)
		{
			/* (a frame past the sprites is the empty sprite's: bitmap 0) */
			index = 0;
			if (frame_index < sequence->sprites.count)
			{
				struct bitmap_group_sprite *sprite = (struct bitmap_group_sprite *)sequence->sprites.address + frame_index;

				if (!tag_validate_contains(validation, sprite, sizeof(*sprite)))
					return FALSE;
				index = sprite->bitmap_index;
			}
		}
	}
	if (index == NONE)
		index = frame_index;

	return index >= 0 && index < group->bitmaps.count;
}

/* whether the hud finds a bitmap, and a sprite's bounds, for a frame of a
sequence (hud_draw.c, hud_retrieve_bitmap_and_bounding_rect and
get_sprite_clip_rect) */
static boolean hud_bitmap_exists(
	struct tag_validation *validation,
	long bitmap_index,
	short sequence_index,
	short frame_index,
	boolean *sprite_exists)
{
	struct bitmap_group *group = bitmap_group_get_checked(validation, bitmap_index);
	struct bitmap_group_sequence *sequence;
	boolean exists;

	*sprite_exists = FALSE;
	if (!group || sequence_index < 0 || sequence_index >= group->sequences.count)
		return FALSE;
	sequence = (struct bitmap_group_sequence *)group->sequences.address + sequence_index;
	if (!tag_validate_contains(validation, sequence, sizeof(*sequence)))
		return FALSE;
	frame_index &= 0x7FFF;
	if (sequence->sprites.count > 0)
	{
		struct bitmap_group_sprite *sprite =
			(struct bitmap_group_sprite *)sequence->sprites.address + frame_index % sequence->sprites.count;

		if (!tag_validate_contains(validation, sprite, sizeof(*sprite)))
			return FALSE;
		exists = sprite->bitmap_index >= 0 && sprite->bitmap_index < group->bitmaps.count;
		*sprite_exists = exists;
	}
	else
	{
		exists = bitmap_from_sequence_exists(validation, bitmap_index, sequence_index, frame_index);
	}

	return exists;
}

/* an effector offsets a map the overlay has (hud_draw.c,
hud_draw_multitexture_overlay: one whose bitmap is not found has no
offset), or tints it */
static boolean multitexture_overlay_check(
	struct tag_validation *validation,
	void *base)
{
	struct multitexture_overlay_hud_element_definition *overlay = base;
	boolean map_exists[MAXIMUM_MULTITEXTURE_OVERLAY_MAPS];
	long index;

	for (index = 0; index < MAXIMUM_MULTITEXTURE_OVERLAY_MAPS; index++)
		map_exists[index] = bitmap_from_sequence_exists(validation, overlay->map[index].index, 0, 0);
	for (index = 0; index < overlay->functions.count; index++)
	{
		struct multitexture_overlay_hud_element_effector_definition *effector =
			(struct multitexture_overlay_hud_element_effector_definition *)overlay->functions.address + index;
		short map_index = (short)(effector->destination - _hud_multitexture_overlay_effector_destination_primary_map);

		if ((effector->destination_type == _hud_multitexture_overlay_effector_type_horizontal_offset ||
				effector->destination_type == _hud_multitexture_overlay_effector_type_vertical_offset) &&
			map_index >= 0 && map_index < MAXIMUM_MULTITEXTURE_OVERLAY_MAPS && !map_exists[map_index])
		{
			tag_validate_correct(validation, "has function %ld offsetting map %d, which has no bitmap: a tint",
				index, map_index);
			effector->destination_type = _hud_multitexture_overlay_effector_type_tint;
		}
	}

	return TRUE;
}

/* (a static element is drawn with its sequence's first frame:
hud_draw_static_element; a meter too: hud_draw_meter) */
static boolean optional_static_hud_element_check(
	struct tag_validation *validation,
	void *base)
{
	struct static_hud_element_definition *element = base;

	if (element->interface_bitmap.index != NONE &&
		!bitmap_from_sequence_exists(validation, element->interface_bitmap.index, element->sequence_index, 0))
	{
		tag_validate_correct(validation, "has no bitmap in sequence %d of its bitmap: none", element->sequence_index);
		element->interface_bitmap.index = NONE;
	}

	return TRUE;
}

static boolean optional_meter_hud_element_check(
	struct tag_validation *validation,
	void *base)
{
	struct meter_hud_element_definition *element = base;

	if (element->meter_bitmap.index != NONE &&
		!bitmap_from_sequence_exists(validation, element->meter_bitmap.index, element->sequence_index, 0))
	{
		tag_validate_correct(validation, "has no bitmap in sequence %d of its meter bitmap: none",
			element->sequence_index);
		element->meter_bitmap.index = NONE;
	}

	return TRUE;
}

/* (an overlay is drawn whatever its bitmap: hud_unit.c) */
static boolean auxilary_overlay_check(
	struct tag_validation *validation,
	void *base)
{
	struct auxilary_overlay_definition *overlay = base;

	if (overlay->type != _auxilary_overlay_type_never &&
		!bitmap_from_sequence_exists(validation, overlay->static_element.interface_bitmap.index,
			overlay->static_element.sequence_index, 0))
	{
		tag_validate_correct(validation, "has no bitmap: not drawn");
		overlay->type = _auxilary_overlay_type_never;
	}

	return TRUE;
}

/* (a meter's flash time is kept modulo twice its background's flash
period in ticks, fast_ftol(flash_period*30): hud_unit.c) */
static boolean auxilary_meter_check(
	struct tag_validation *validation,
	void *base)
{
	struct auxilary_meter_definition *meter = base;
	real ticks = meter->panel.background.colors.flash_period * 30.0f;
	real ticks_magnitude = ticks < 0.0f ? -ticks : ticks;

	/* (and no more than a long holds twice: fast_ftol of more, or of no
	number, is 0x80000000, and twice that 0) */
	if (!(ticks_magnitude >= 1.0f && ticks_magnitude < 1073741824.0f))
	{
		tag_validate_correct(validation, "flashes every %f seconds: every second",
			meter->panel.background.colors.flash_period);
		meter->panel.background.colors.flash_period = 1.0f;
	}

	return TRUE;
}

static boolean weapon_hud_static_element_check(
	struct tag_validation *validation,
	void *base)
{
	struct weapon_hud_static_element *element = base;

	if (!TEST_FLAG(element->header.runtime_flags, _hud_element_runtime_invalid_bit) &&
		!bitmap_from_sequence_exists(validation, element->static_element.interface_bitmap.index,
			element->static_element.sequence_index, 0))
	{
		tag_validate_correct(validation, "has no bitmap: not drawn");
		SET_FLAG(element->header.runtime_flags, _hud_element_runtime_invalid_bit, TRUE);
	}

	return TRUE;
}

static boolean weapon_hud_meter_element_check(
	struct tag_validation *validation,
	void *base)
{
	struct weapon_hud_meter_element *element = base;

	if (!TEST_FLAG(element->header.runtime_flags, _hud_element_runtime_invalid_bit) &&
		!bitmap_from_sequence_exists(validation, element->meter_element.meter_bitmap.index,
			element->meter_element.sequence_index, 0))
	{
		tag_validate_correct(validation, "has no bitmap: not drawn");
		SET_FLAG(element->header.runtime_flags, _hud_element_runtime_invalid_bit, TRUE);
	}

	return TRUE;
}

/* a weapon hud's parents are drawn first, each through its own, with no
end but none (hud_weapon.c, render_weapon_hud); the other walks stop at
MAXIMUM_WEAPON_HUD_DEFINITION_DEPTH. So a hud and its parents are no more
than that, which also ends a ring of them */
static boolean weapon_hud_interface_check(
	struct tag_validation *validation,
	void *base)
{
	struct weapon_hud_interface_definition *hud = base;
	short depth;

	for (depth = 1; hud->parent_hud.index != NONE; depth++)
	{
		struct weapon_hud_interface_definition *parent =
			tag_validate_tag_get(validation, hud->parent_hud.index, 'wphi');

		if (!parent || depth >= MAXIMUM_WEAPON_HUD_DEFINITION_DEPTH)
		{
			tag_validate_correct(validation, "has %s parent huds at %d: none",
				parent ? "more than the game's" : "a bad", depth);
			hud->parent_hud.index = NONE;
			break;
		}
		hud = parent;
	}

	return TRUE;
}

/* a message's text elements are drawn as strings each from where the one
before ended, read to their terminator (hud_messaging.c,
hud_messaging_draw): the text ends with one, so that none reads past it,
and so does each span; a span at the very end, of no characters, would be
past the text and is cut with what follows. Icons are of the game's types */
static boolean hud_message_text_check(
	struct tag_validation *validation,
	void *base)
{
	struct hud_message_text_definition *definition = base;
	wchar_t *text = definition->text_data.address;
	long length;
	long message_index;

	if (definition->text_data.size % sizeof(wchar_t))
	{
		tag_validate_correct(validation, "has %ld bytes of text: cut to whole characters",
			definition->text_data.size);
		definition->text_data.size -= definition->text_data.size % sizeof(wchar_t);
		if (!definition->text_data.size)
			definition->text_data.address = NULL;
	}
	length = definition->text_data.size / (long)sizeof(wchar_t);
	if (length && text[length - 1])
	{
		tag_validate_correct(validation, "has text with no terminator: terminated");
		text[length - 1] = 0;
	}
	for (message_index = 0; message_index < definition->messages.count; message_index++)
	{
		struct hud_state_message_definition *message =
			(struct hud_state_message_definition *)definition->messages.address + message_index;
		long position = message->text_start_index;
		short element_index;

		for (element_index = 0; element_index < message->element_count; element_index++)
		{
			long index = message->element_start_index + element_index;
			struct hud_state_message_element *element;

			if (index >= definition->elements.count)
			{
				tag_validate_correct(validation, "has message %ld with elements past its %ld: cut to %d elements",
					message_index, definition->elements.count, element_index);
				message->element_count = (byte)element_index;
				break;
			}
			element = (struct hud_state_message_element *)definition->elements.address + index;
			if (element->type == _hud_message_type_icon && element->data >= NUMBER_OF_HUD_ICON_TYPES)
			{
				tag_validate_correct(validation, "has element %ld an icon of type %d: 0", index, element->data);
				element->data = 0;
			}
			if (element->type != _hud_message_type_text)
				continue;
			if (!element->data && position >= length)
			{
				tag_validate_correct(validation, "has message %ld with text past its text: cut to %d elements",
					message_index, element_index);
				message->element_count = (byte)element_index;
				break;
			}
			if (element->data && position + element->data <= length && text[position + element->data - 1])
			{
				tag_validate_correct(validation, "has message %ld with text %d not terminated: terminated",
					message_index, element_index);
				text[position + element->data - 1] = 0;
			}
			position += element->data;
		}
	}

	return TRUE;
}

/* the waypoint arrows are drawn with their sprites' bounds (hud_nav_points.c:
one with a bitmap and no sprite is drawn with none) */
static boolean hud_globals_check(
	struct tag_validation *validation,
	void *base)
{
	struct hud_globals_definition *definition = base;
	long arrow_index;

	for (arrow_index = 0;
		arrow_index < definition->waypoint.arrows.count && definition->waypoint.arrow_bitmap.index != NONE;
		arrow_index++)
	{
		struct hud_waypoint_arrow *arrow = (struct hud_waypoint_arrow *)definition->waypoint.arrows.address + arrow_index;
		short type;

		for (type = 0; type < NUMBER_OF_WAYPOINT_TYPES; type++)
		{
			boolean sprite_exists;

			if (hud_bitmap_exists(validation, definition->waypoint.arrow_bitmap.index, arrow->sequence_indices[type], 0,
					&sprite_exists) &&
				!sprite_exists)
			{
				tag_validate_correct(validation, "has arrow %ld with no sprite in sequence %d: no arrow bitmap",
					arrow_index, arrow->sequence_indices[type]);
				definition->waypoint.arrow_bitmap.index = NONE;
				break;
			}
		}
	}

	return TRUE;
}

/* a character's pixels are its bitmap's width by its height, one byte
each, at its offset into the font's pixels, copied into the hardware
character cache's bitmap, in a cell of them and a texel's border each side
that moves the cache's next cell on (rasterizer_text.c, draw_string.c): a
character whose pixels are not all in the font's, are bigger than that
bitmap, or whose cell is less than none has none (retail fonts have
characters -2 wide and 0 high, which have no pixels and an empty cell) */
static boolean font_check(
	struct tag_validation *validation,
	void *base)
{
	struct font_header *font = base;
	long index;

	for (index = 0; index < font->characters.count; index++)
	{
		struct font_character *character = (struct font_character *)font->characters.address + index;
		boolean has_pixels = character->bitmap_width > 0 && character->bitmap_height > 0;

		if (character->bitmap_width < -HARDWARE_CHARACTER_CACHE_BORDERS ||
			character->bitmap_height < -HARDWARE_CHARACTER_CACHE_BORDERS ||
			character->bitmap_width > HARDWARE_CHARACTER_CACHE_BITMAP_WIDTH ||
			character->bitmap_height > HARDWARE_CHARACTER_CACHE_BITMAP_HEIGHT ||
			(has_pixels &&
				(character->pixels_offset < 0 || character->pixels_offset > font->pixels.size ||
					(long)character->bitmap_width * character->bitmap_height >
						font->pixels.size - character->pixels_offset)))
		{
			tag_validate_correct(validation, "has character %ld of %d by %d pixels at %ld of its %ld: none",
				index, character->bitmap_width, character->bitmap_height, character->pixels_offset,
				font->pixels.size);
			character->bitmap_width = 0;
			character->bitmap_height = 0;
			character->pixels_offset = 0;
		}
	}

	return TRUE;
}

/* (a string is terminated in its data as it is got: text_group.c) */
static boolean string_list_entry_check(
	struct tag_validation *validation,
	void *base)
{
	struct string_list_entry *entry = base;
	char *string = entry->string.address;

	if (entry->string.size && string[entry->string.size - 1])
	{
		tag_validate_correct(validation, "is not terminated: terminated");
		string[entry->string.size - 1] = 0;
	}

	return TRUE;
}

/* whether a string of length characters (the last a terminator) read as
draw_string.c's parse_unicode_string reads it, which takes the character
after a '|' as part of it, ends in it; if not, *bad is the character to
end it at */
static boolean unicode_string_ends(
	wchar_t const *string,
	long length,
	long *bad)
{
	long index = 0;

	while (index < length)
	{
		if (!string[index])
			return TRUE;
		if (string[index] == '|')
		{
			if (index + 1 >= length - 1)
			{
				*bad = index;
				return FALSE;
			}
			index += 2;
		}
		else
		{
			index++;
		}
	}
	*bad = length - 1;

	return FALSE;
}

/* ends a string of length characters (it ends at the latest with its last)
so that it is read to an end in it: whether it had to */
static boolean unicode_string_end(
	wchar_t *string,
	long length)
{
	boolean ended = FALSE;
	long bad;

	while (length > 0 && !unicode_string_ends(string, length, &bad))
	{
		string[bad] = 0;
		ended = TRUE;
	}

	return ended;
}

/* (a string is terminated, at its last whole character, as it is got:
text_group.c; it is then read as its characters say: unicode_string_end) */
static boolean unicode_string_list_entry_check(
	struct tag_validation *validation,
	void *base)
{
	struct string_list_entry *entry = base;
	wchar_t *string = entry->string.address;
	long length;

	if (entry->string.size % sizeof(wchar_t))
	{
		tag_validate_correct(validation, "has %ld bytes: cut to whole characters", entry->string.size);
		entry->string.size -= entry->string.size % sizeof(wchar_t);
		if (!entry->string.size)
			entry->string.address = NULL;
	}
	length = entry->string.size / (long)sizeof(wchar_t);
	if (length && string[length - 1])
	{
		tag_validate_correct(validation, "is not terminated: terminated");
		string[length - 1] = 0;
	}
	if (unicode_string_end(string, length))
		tag_validate_correct(validation, "ends in a '|': ended before it");

	return TRUE;
}

/* a text's search is replaced until the text has it no more, from its
start when the replacement is no longer (ui_widget.c, search_and_replace):
so a replacement the same as its search never ends. The replacements are
none, the local player's number or "?" (the second), "<invalid>" (the
rest: ui_widget_text_search_and_replace_functions.c) */
static boolean ui_widget_search_and_replace_check(
	struct tag_validation *validation,
	void *base)
{
	static char const *const local_player_replacements[] = { "1", "2", "3", "4", "?" };
	struct ui_widget_search_and_replace_reference *reference = base;
	boolean forever = FALSE;
	short index;

	if (reference->replace_function == _widget_replace_function_local_player)
	{
		for (index = 0; index < (short)NUMBEROF(local_player_replacements); index++)
		{
			if (!strcmp(reference->search_string, local_player_replacements[index]))
				forever = TRUE;
		}
	}
	else if (reference->replace_function != _widget_replace_function_null)
	{
		forever = !strcmp(reference->search_string, "<invalid>");
	}
	if (forever)
	{
		tag_validate_correct(validation, "replaces '%s' with itself: with nothing", reference->search_string);
		reference->replace_function = _widget_replace_function_null;
	}

	return TRUE;
}

/* the widgets reached as a widget's are loaded (ui_widget.c,
ui_widget_load_children_recursive): its children, and a column list's
extended description, each with its own, with no end but the tags. So they
are a tree, or a graph with no ring, no deeper than MAXIMUM_WIDGET_DEPTH:
a reference that closes a ring or goes deeper is none. The walk is the
tags' graph, each widget once, its depth below it kept as it is done */

enum
{
	_widget_walk_unseen = 0,
	_widget_walk_open,
	_widget_walk_done,

	/* (any tag's place in the table: a short) */
	MAXIMUM_WIDGET_WALK_TAGS = UNSIGNED_SHORT_MAX + 1,
};

struct widget_walk_frame
{
	struct ui_widget_definition *widget;
	short tag_index;
	short reference_index;
	short height;
};

static struct
{
	byte states[MAXIMUM_WIDGET_WALK_TAGS];
	byte heights[MAXIMUM_WIDGET_WALK_TAGS];
	unsigned short seen[MAXIMUM_WIDGET_WALK_TAGS];
	long seen_count;
	struct widget_walk_frame frames[MAXIMUM_WIDGET_DEPTH + 1];
} widget_walk;

/* a widget's reference_index-th reference to a widget loaded with it, or
NULL past them */
static struct tag_reference *widget_loaded_reference(
	struct ui_widget_definition *widget,
	short reference_index)
{
	if (reference_index < widget->child_widgets.count)
		return &((struct ui_widget_child_reference *)widget->child_widgets.address + reference_index)->widget_tag;
	if (reference_index == widget->child_widgets.count && widget->type == _ui_widget_type_column_list)
		return &widget->extended_description_widget;

	return NULL;
}

static void widget_walk_see(
	short tag_index,
	byte state)
{
	unsigned short index = (unsigned short)tag_index;

	if (widget_walk.states[index] == _widget_walk_unseen)
		widget_walk.seen[widget_walk.seen_count++] = index;
	widget_walk.states[index] = state;

	return;
}

static boolean ui_widget_check(
	struct tag_validation *validation,
	void *base)
{
	short depth = 0;
	long index;

	widget_walk.frames[0].widget = base;
	widget_walk.frames[0].tag_index = NONE;
	widget_walk.frames[0].reference_index = 0;
	widget_walk.frames[0].height = 0;
	/* (the widget's own place in the table, known from a widget's
	reference to it if it is reached again; until then it is marked by
	its frame) */
	while (depth >= 0)
	{
		struct widget_walk_frame *frame = &widget_walk.frames[depth];
		struct tag_reference *reference = widget_loaded_reference(frame->widget, frame->reference_index++);
		struct ui_widget_definition *child;
		unsigned short child_index;
		boolean ring = FALSE;
		short frame_index;

		if (!reference)
		{
			/* (done: its depth below it is its parent's, less one) */
			if (frame->tag_index != NONE)
			{
				widget_walk_see(frame->tag_index, _widget_walk_done);
				widget_walk.heights[(unsigned short)frame->tag_index] = (byte)frame->height;
			}
			if (depth > 0)
				widget_walk.frames[depth - 1].height = MAX(widget_walk.frames[depth - 1].height, frame->height + 1);
			depth--;
			continue;
		}
		if (reference->index == NONE)
			continue;
		child = tag_validate_tag_get(validation, reference->index, 'DeLa');
		if (!child)
			continue;
		child_index = (unsigned short)reference->index;
		for (frame_index = 0; frame_index <= depth; frame_index++)
		{
			if (widget_walk.frames[frame_index].widget == child)
				ring = TRUE;
		}
		if (ring || widget_walk.states[child_index] == _widget_walk_open)
		{
			tag_validate_correct(validation, "has a widget loading itself through %08lx: none", reference->index);
			reference->index = NONE;
			continue;
		}
		if (widget_walk.states[child_index] == _widget_walk_done)
		{
			if (depth + 1 + widget_walk.heights[child_index] > MAXIMUM_WIDGET_DEPTH)
			{
				tag_validate_correct(validation, "has widgets loading more than %d deep through %08lx: none",
					MAXIMUM_WIDGET_DEPTH, reference->index);
				reference->index = NONE;
				continue;
			}
			frame->height = MAX(frame->height, widget_walk.heights[child_index] + 1);
			continue;
		}
		if (depth + 1 > MAXIMUM_WIDGET_DEPTH)
		{
			tag_validate_correct(validation, "has widgets loading more than %d deep through %08lx: none",
				MAXIMUM_WIDGET_DEPTH, reference->index);
			reference->index = NONE;
			continue;
		}
		widget_walk_see((short)child_index, _widget_walk_open);
		depth++;
		widget_walk.frames[depth].widget = child;
		widget_walk.frames[depth].tag_index = (short)child_index;
		widget_walk.frames[depth].reference_index = 0;
		widget_walk.frames[depth].height = 0;
	}
	for (index = 0; index < widget_walk.seen_count; index++)
		widget_walk.states[widget_walk.seen[index]] = _widget_walk_unseen;
	widget_walk.seen_count = 0;

	return TRUE;
}

/* (the keys are read by place: virtual_keyboard.c) */
static boolean virtual_keyboard_check(
	struct tag_validation *validation,
	void *base)
{
	struct virtual_keyboard_definition *keyboard = base;

	if (keyboard->keys.count < NUMBER_OF_VIRTUAL_KEYS)
	{
		tag_validate_refuse(validation, "has %ld keys, not the game's %d", keyboard->keys.count,
			NUMBER_OF_VIRTUAL_KEYS);
		return FALSE;
	}

	return TRUE;
}

/* the hud's numbers are drawn with the frames of the first sequence of the
hud number's bitmap, unchecked (hud_draw.c, hud_draw_numbers): a hud
number without them is none (which the drawing checks) */
static boolean game_globals_interface_tag_references_check(
	struct tag_validation *validation,
	void *base)
{
	struct game_globals_interface_tag_references *references = base;
	struct tag_reference *reference = &references->interface_tag_references[_interface_hud_digits];
	struct hud_number_definition *hud_number = tag_validate_tag_get(validation, reference->index, 'hud#');
	short frame_index;

	if (!hud_number)
		return TRUE;
	for (frame_index = 0; frame_index < NUMBER_OF_HUD_NUMBER_FRAMES; frame_index++)
	{
		boolean sprite_exists;

		if (!hud_bitmap_exists(validation, hud_number->number_bitmap.index, 0, frame_index, &sprite_exists))
		{
			tag_validate_correct(validation, "has hud digits with no bitmap for frame %d: none", frame_index);
			reference->index = NONE;
			break;
		}
	}

	return TRUE;
}

/* the blocks whose first element is read through its address, or asked
for and used unchecked: a player's look function (player_control.c,
evaluate_piecewise_linear_function, from its address with its count),
the rasterizer's data (rasterizer_common.c) and the interface's tags
(interface.c, interface_tag_references_get) */
static boolean game_globals_check(
	struct tag_validation *validation,
	void *base)
{
	struct game_globals *globals = base;
	struct game_globals_player_control *player_control = globals->player_control.address;

	if (!globals->player_control.count || !player_control->look_function.count)
	{
		tag_validate_refuse(validation, "has no look function");
		return FALSE;
	}
	if (!globals->rasterizer_data.count || !globals->interface_tag_references.count)
	{
		tag_validate_refuse(validation, "has no rasterizer data or interface tags");
		return FALSE;
	}

	return TRUE;
}
