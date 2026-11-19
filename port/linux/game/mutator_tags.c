/*
MUTATOR_TAGS.C

A mutator's tag edits (docs/mutators.md, "Tag edits"): values that a mutator
sets in the tags of the map a game plays, after the map has loaded and been
checked (tag_validate.c), before any of its objects is made. A mutator is
still only data: it can set no field but those listed here, each checked
against its type and range, so it does no more than a map built with those
values would.

An edit names a tag (its group, by a name of this file's, and its name), a
field in it (a path: a field's name, then for a block the element's index or
name, "triggers.0.rounds_between_tracers") and a value: a number, a name (a
tag's, an enum's, flags' separated by commas) or, for a block, elements of
the same block in another tag of the group, copied. A tag, element or tag
named that the map does not have is passed over and logged, on every
machine alike, since every machine of a game plays the same map: the rest
are made. A tag that an edit only names (one it copies from, or refers to) and
that a Custom Edition map lacks is first taken from a stock map (stock_tags.c).

A pose (_mutator_tag_edit_pose: object_pose.c) comes as an edit too, so that
it is checked, sent and played as they are: its group is an object group's
name, its tag one tag of it or none for all of them, its path a node's name,
its number the node's scale and its elements none or its offset, three
thousandths of a world unit. A death (_mutator_tag_edit_death: object_death.c)
does too: its group is a unit group's name, its tag an effect's name or none,
its text "vanish" or "stays" (the body), and nothing else. So does an
explosion's decal (_mutator_tag_edit_explosion: explosion_decals.c): its tag
is a decal's name, and it has nothing else.

The edits come as bytes (port_mutators.c writes them, network_mutators.c
sends the host's), each of them:

	byte kind;              (_mutator_tag_edit_number, _text, _copy, _pose, _death or _explosion)
	byte element_count;     (a copy's elements, or 0 for all of them)
	byte append;            (a copy's: added after the block's own)
	byte unused;
	real number;            (little-endian)
	char group[], tag[], path[], text[];   (each ended by a 0)
	short elements[element_count];         (little-endian)

Each edit is undone (its old bytes put back) before the next game's are
made, unless the map's tags have loaded again since (mutator_tags_loaded):
a client undoes its last game's before it plays the host's.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "bitmaps/bitmap_group.h"
#include "effects/decal_definitions.h"
#include "effects/effect_definitions.h"
#include "game/game_globals.h"
#include "interface/hud_definitions.h"
#include "interface/weapon_hud_interface_definition.h"
#include "items/projectile_definitions.h"
#include "items/weapon_definitions.h"
#include "main/console.h"
#include "models/model_definitions.h"
#include "math/real_math.h"
#include "objects/damage_effect_definitions.h"
#include "sound/sound_definitions.h"
#include "cache/cache_files.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"

#include "mutators.h"
#include "explosion_decals.h"
#include "object_death.h"
#include "object_pose.h"
#include "stock_tags.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* ---------- constants */

enum
{
	_mutator_field_short,
	_mutator_field_real,
	/* degrees in a mutator, radians in the tag */
	_mutator_field_angle,
	/* seconds, with the step a tick that the game takes (at the field's
	runtime offset) worked out as the tools do: 1/(seconds x 30), 1 for 0 */
	_mutator_field_time,
	/* world units a second in a mutator, a tick in the tag */
	_mutator_field_speed,
	/* a real of an array, by its index or the field's names */
	_mutator_field_array,
	/* a short: one of the field's names */
	_mutator_field_enum,
	/* an unsigned long (or a word, by its size): some of the field's names,
	each a bit */
	_mutator_field_flags,
	/* a tag of the field's group, by its name, or none ("") */
	_mutator_field_reference,
	/* a block whose elements' fields are the definition's, by index or by
	the field's names */
	_mutator_field_block,
	/* a block whose elements a mutator copies from another tag's */
	_mutator_field_copy,
};

/* a tag edit's tag that is every tag of its group, and what follows it
for those of them whose enum field has one of the values ("*?type=scratch,burn"),
or, of projectiles, those a weapon's trigger fires ("*?fired") */
#define MUTATOR_TAG_EVERY "*"
#define MUTATOR_TAG_WHERE "*?"
#define MUTATOR_TAG_WHERE_FIRED "fired"

enum
{
	WEAPON_HUD_INTERFACE_GROUP_TAG = 'wphi',
	/* (hud_weapon.c's struct weapon_hud_crosshairs_element, which no header
	declares) */
	WEAPON_HUD_CROSSHAIRS_ELEMENT_SIZE = 0x68,
	/* (tag_schema_effects.c's) */
	MAXIMUM_CROSSHAIRS_PER_WEAPON_HUD = 19,
	MAXIMUM_SCREEN_EFFECTS_PER_WEAPON_HUD = 1,

	/* the most edits a game undoes, and the bytes of the elements its copies
	make */
	MAXIMUM_MUTATOR_TAG_UNDOS = 4096,
	MUTATOR_TAG_ELEMENT_BYTES = 0x10000,
	/* the most bytes an edit's undo keeps (a reference's) */
	MUTATOR_TAG_UNDO_BYTES = 16,
	/* the most segments in an edit's path */
	MAXIMUM_MUTATOR_TAG_PATH_SEGMENTS = 8,
	/* a group's name, a tag's, a path's and a text's most bytes, ended */
	MUTATOR_TAG_GROUP_LENGTH = 32,
	MUTATOR_TAG_NAME_LENGTH = 256,
	MUTATOR_TAG_PATH_LENGTH = 64,
	MUTATOR_TAG_TEXT_LENGTH = 256,
	MAXIMUM_MUTATOR_TAG_COPY_ELEMENTS = 64,
};

/* ---------- structures */

struct mutator_tag_definition;

/* (decals.c's struct decal_definition and its shader's, which no header
declares: the fields a mutator sets, where they are) */
struct mutator_decal_definition
{
	word flags;
	short type;
	byte unused004[0x018 - 0x004];
	real radius_lower_bound;
	real radius_upper_bound;
	byte unused020[0x02C - 0x020];
	real intensity_lower_bound;
	real intensity_upper_bound;
	real_rgb_color color_lower_bound;
	real_rgb_color color_upper_bound;
	byte unused04C[0x0C0 - 0x04C];
	short framebuffer_blend_function;
	byte unused0C2[0x0D8 - 0x0C2];
	struct tag_reference map;
	byte unused0E8[0x10C - 0x0E8];
};

/* (0x10C bytes, as the game's tag reference is 16) */
typedef char mutator_decal_definition_size_check[
	sizeof(struct mutator_decal_definition) == 0x0FC + sizeof(struct tag_reference) ? 1 : -1];
typedef char mutator_decal_definition_map_check[offsetof(struct mutator_decal_definition, map) == 0x0D8 ? 1 : -1];
typedef char mutator_decal_definition_color_check[
	offsetof(struct mutator_decal_definition, color_lower_bound) == 0x034 &&
	offsetof(struct mutator_decal_definition, color_upper_bound) == 0x040 ? 1 : -1];

struct mutator_tag_field
{
	char const *name;
	short type;
	/* an element's size, for a block or a copy; else the field's */
	short size;
	long offset;
	real minimum;
	real maximum;
	/* a reference's group */
	unsigned long group_tag;
	/* an enum's or flags' names, or a block's elements' */
	char const *const *names;
	short name_count;
	/* a copy's most elements */
	short maximum_count;
	/* a block's elements' fields */
	struct mutator_tag_definition const *definition;
	/* a time's step a tick, where it is in the field's structure */
	long runtime_offset;
};

struct mutator_tag_definition
{
	char const *name;
	unsigned long group_tag;
	struct mutator_tag_field const *fields;
	short field_count;
};

struct mutator_tag_edit
{
	byte kind;
	byte element_count;
	byte append;
	real number;
	char const *group;
	char const *tag;
	char const *path;
	char const *text;
	byte const *elements;
};

struct mutator_tag_undo
{
	void *address;
	short size;
	byte bytes[MUTATOR_TAG_UNDO_BYTES];
};

/* ---------- globals */

/* a reference to no tag's name */
static char mutator_tags_no_name[1];

static char const *const mutator_material_names[NUMBER_OF_MATERIAL_TYPES] =
{
	"dirt", "sand", "stone", "snow", "wood", "hollow_metal", "thin_metal", "thick_metal", "rubber", "glass",
	"force_field", "grunt", "hunter_armor", "hunter_skin", "elite", "jackal", "jackal_energy_shield", "engineer",
	"engineer_force_field", "flood_combat_form", "flood_carrier_form", "cyborg", "cyborg_energy_shield",
	"armored_human", "human", "sentinel", "monitor", "plastic", "water", "leaves", "elite_energy_shield", "ice",
	"hunter_shield",
};

static char const *const mutator_projectile_response_names[NUMBER_OF_PROJECTILE_MATERIAL_RESPONSES] =
{
	"disappear", "detonate", "reflect", "overpenetrate", "attach",
};

static char const *const mutator_potential_response_flag_names[] =
{
	"only_against_units",
};

/* (decal_definitions.h's enum decal_definition_flag) */
static char const *const mutator_decal_flag_names[] =
{
	"geometry_inherited_by_next_decal_in_chain", "color_interpolate_in_hsv",
	"color_interpolate_along_farthest_hue_path", "no_random_rotation", "water_effect", "snap_to_axis",
	"incremental_counter", "animation_loop", "preserve_aspect",
};

/* (decal_definitions.h's enum decal_type) */
static char const *const mutator_decal_type_names[] =
{
	"scratch", "splatter", "burn", "painted_sign",
};

static char const *const mutator_color_component_names[] =
{
	"red", "green", "blue",
};

/* (a shader's framebuffer blend functions: hud_draw.c's) */
static char const *const mutator_framebuffer_blend_function_names[] =
{
	"alpha_blend", "multiply", "double_multiply", "add", "subtract", "component_min", "component_max",
	"alpha_multiply_add",
};

/* (hud_weapon.c's enum weapon_crosshair_type) */
static char const *const mutator_crosshair_type_names[] =
{
	"aim", "zoom", "charge", "flash_ammo", "flash_heat", "flash_total_ammo", "flash_total_battery", "reload",
	"fired_with_no_ammo", "threw_with_no_grenade", "flash_ammo_none_for_reload", "flash_secondary_ammo",
	"flash_secondary_total_ammo", "secondary_reload", "fired_secondary_with_no_ammo",
	"flash_secondary_ammo_none_for_reload", "primary_trigger_ready", "secondary_trigger_ready",
	"flash_fired_battery_depleted",
};

#define MUTATOR_FIELD(structure, field, type, minimum, maximum) \
	{ #field, (type), (short)sizeof(((structure *)0)->field), (long)offsetof(structure, field), (minimum), (maximum), \
		0, NULL, 0, 0, NULL }
#define MUTATOR_ENUM_FIELD(structure, field, names) \
	{ #field, _mutator_field_enum, (short)sizeof(((structure *)0)->field), (long)offsetof(structure, field), 0.f, 0.f, \
		0, (names), NUMBEROF(names), 0, NULL }
#define MUTATOR_FLAGS_FIELD(name, structure, field, names) \
	{ (name), _mutator_field_flags, (short)sizeof(((structure *)0)->field), (long)offsetof(structure, field), 0.f, \
		0.f, 0, (names), NUMBEROF(names), 0, NULL }
#define MUTATOR_REFERENCE_FIELD(structure, field, group_tag) \
	{ #field, _mutator_field_reference, (short)sizeof(struct tag_reference), (long)offsetof(structure, field), 0.f, \
		0.f, (group_tag), NULL, 0, 0, NULL }
#define MUTATOR_BLOCK_FIELD(structure, field, element, names, name_count, definition) \
	{ #field, _mutator_field_block, (short)sizeof(element), (long)offsetof(structure, field), 0.f, 0.f, 0, (names), \
		(name_count), 0, &(definition) }
#define MUTATOR_TIME_FIELD(structure, field, runtime_field, maximum) \
	{ #field, _mutator_field_time, (short)sizeof(real), (long)offsetof(structure, field), 0.f, (maximum), 0, NULL, 0, 0, \
		NULL, (long)offsetof(structure, runtime_field) }
#define MUTATOR_ARRAY_FIELD(structure, field, names, minimum, maximum) \
	{ #field, _mutator_field_array, (short)sizeof(((structure *)0)->field[0]), (long)offsetof(structure, field), \
		(minimum), (maximum), 0, (names), NUMBEROF(names), 0, NULL }
#define MUTATOR_COPY_FIELD(structure, field, element_size, maximum_count) \
	{ #field, _mutator_field_copy, (short)(element_size), (long)offsetof(structure, field), 0.f, 0.f, 0, NULL, 0, \
		(maximum_count), NULL }

static struct mutator_tag_field const mutator_weapon_magazine_fields[] =
{
	MUTATOR_FIELD(struct weapon_magazine_definition, rounds_recharged_per_second, _mutator_field_short, 0.f, 32767.f),
	MUTATOR_FIELD(struct weapon_magazine_definition, rounds_total_initial, _mutator_field_short, 0.f, 32767.f),
	MUTATOR_FIELD(struct weapon_magazine_definition, rounds_total_maximum, _mutator_field_short, 0.f, 32767.f),
	MUTATOR_FIELD(struct weapon_magazine_definition, rounds_loaded_maximum, _mutator_field_short, 0.f, 32767.f),
	MUTATOR_FIELD(struct weapon_magazine_definition, rounds_reloaded, _mutator_field_short, 0.f, 32767.f),
	MUTATOR_FIELD(struct weapon_magazine_definition, chamber_time, _mutator_field_real, 0.f, 60.f),
};

static struct mutator_tag_definition const mutator_weapon_magazine_definition =
{
	"magazine", 0, mutator_weapon_magazine_fields, NUMBEROF(mutator_weapon_magazine_fields)
};

static struct mutator_tag_field const mutator_weapon_trigger_fields[] =
{
	MUTATOR_FIELD(struct weapon_trigger_definition, initial_rate_of_fire, _mutator_field_real, 0.f, 300.f),
	MUTATOR_FIELD(struct weapon_trigger_definition, final_rate_of_fire, _mutator_field_real, 0.f, 300.f),
	MUTATOR_TIME_FIELD(struct weapon_trigger_definition, rate_of_fire_acceleration_time,
		runtime_rate_of_fire_acceleration_time, 60.f),
	MUTATOR_TIME_FIELD(struct weapon_trigger_definition, rate_of_fire_deceleration_time,
		runtime_rate_of_fire_deceleration_time, 60.f),
	MUTATOR_FIELD(struct weapon_trigger_definition, rounds_between_tracers, _mutator_field_short, 0.f, 32767.f),
	MUTATOR_TIME_FIELD(struct weapon_trigger_definition, error_acceleration_time, runtime_error_acceleration_time, 60.f),
	MUTATOR_TIME_FIELD(struct weapon_trigger_definition, error_deceleration_time, runtime_error_deceleration_time, 60.f),
	MUTATOR_FIELD(struct weapon_trigger_definition, projectile_error_angle_lower_bound, _mutator_field_angle, 0.f, 180.f),
	MUTATOR_FIELD(struct weapon_trigger_definition, projectile_error_angle_upper_bound, _mutator_field_angle, 0.f, 180.f),
	MUTATOR_FIELD(struct weapon_trigger_definition, heat_generated_per_round, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct weapon_trigger_definition, age_generated_per_round, _mutator_field_real, 0.f, 1.f),
};

static struct mutator_tag_definition const mutator_weapon_trigger_definition =
{
	"trigger", 0, mutator_weapon_trigger_fields, NUMBEROF(mutator_weapon_trigger_fields)
};

static struct mutator_tag_field const mutator_weapon_fields[] =
{
	MUTATOR_FIELD(struct weapon_definition, weapon.heat_recovery_threshold, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.heat_loss_per_second, _mutator_field_real, 0.f, 30.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.zoom_level_count, _mutator_field_short, 0.f, 8.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.zoom_magnification_minimum, _mutator_field_real, 1.f, 32.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.zoom_magnification_maximum, _mutator_field_real, 1.f, 32.f),
	MUTATOR_REFERENCE_FIELD(struct weapon_definition, weapon.zoom_in_sound, SOUND_DEFINITION_TAG),
	MUTATOR_REFERENCE_FIELD(struct weapon_definition, weapon.zoom_out_sound, SOUND_DEFINITION_TAG),
	MUTATOR_FIELD(struct weapon_definition, weapon.age_heat_recovery_penalty, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.age_rate_of_fire_penalty, _mutator_field_real, 0.f, 10.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.age_misfire_start, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.age_misfire_chance, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.active_camo_ding, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct weapon_definition, weapon.active_camo_regrowth_rate, _mutator_field_real, 0.f, 1.f),
	MUTATOR_BLOCK_FIELD(struct weapon_definition, weapon.magazines, struct weapon_magazine_definition, NULL, 0,
		mutator_weapon_magazine_definition),
	MUTATOR_BLOCK_FIELD(struct weapon_definition, weapon.triggers, struct weapon_trigger_definition, NULL, 0,
		mutator_weapon_trigger_definition),
};

static struct mutator_tag_field const mutator_projectile_material_response_fields[] =
{
	MUTATOR_ENUM_FIELD(struct projectile_material_response_definition, default_response,
		mutator_projectile_response_names),
	MUTATOR_REFERENCE_FIELD(struct projectile_material_response_definition, default_effect, EFFECT_DEFINITION_TAG),
	MUTATOR_ENUM_FIELD(struct projectile_material_response_definition, potential_response,
		mutator_projectile_response_names),
	MUTATOR_FLAGS_FIELD("potential_flags", struct projectile_material_response_definition, potential_flags,
		mutator_potential_response_flag_names),
	MUTATOR_FIELD(struct projectile_material_response_definition, potential_skip_fraction, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct projectile_material_response_definition, potential_angle_lower_bound, _mutator_field_angle,
		-90.f, 90.f),
	MUTATOR_FIELD(struct projectile_material_response_definition, potential_angle_upper_bound, _mutator_field_angle,
		-90.f, 90.f),
	MUTATOR_REFERENCE_FIELD(struct projectile_material_response_definition, potential_effect, EFFECT_DEFINITION_TAG),
};

static struct mutator_tag_definition const mutator_projectile_material_response_definition =
{
	"material response", 0, mutator_projectile_material_response_fields,
	NUMBEROF(mutator_projectile_material_response_fields)
};

static struct mutator_tag_field const mutator_projectile_fields[] =
{
	MUTATOR_REFERENCE_FIELD(struct projectile_definition, object.model, MODELS_GROUP_TAG),
	MUTATOR_FIELD(struct projectile_definition, projectile.maximum_range, _mutator_field_real, 0.f, 10000.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.air_gravity_scale, _mutator_field_real, 0.f, 10.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.water_gravity_scale, _mutator_field_real, 0.f, 10.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.air_damage_range_lower_bound, _mutator_field_real, 0.f,
		10000.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.air_damage_range_upper_bound, _mutator_field_real, 0.f,
		10000.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.water_damage_range_lower_bound, _mutator_field_real, 0.f,
		10000.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.water_damage_range_upper_bound, _mutator_field_real, 0.f,
		10000.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.initial_velocity, _mutator_field_speed, 0.f, 3000.f),
	MUTATOR_FIELD(struct projectile_definition, projectile.final_velocity, _mutator_field_speed, 0.f, 3000.f),
	MUTATOR_BLOCK_FIELD(struct projectile_definition, projectile.material_responses,
		struct projectile_material_response_definition, mutator_material_names, NUMBEROF(mutator_material_names),
		mutator_projectile_material_response_definition),
};

static struct mutator_tag_field const mutator_weapon_hud_interface_fields[] =
{
	MUTATOR_FLAGS_FIELD("valid_crosshair_types", struct weapon_hud_interface_definition, valid_crosshair_types_flags,
		mutator_crosshair_type_names),
	MUTATOR_COPY_FIELD(struct weapon_hud_interface_definition, crosshairs, WEAPON_HUD_CROSSHAIRS_ELEMENT_SIZE,
		MAXIMUM_CROSSHAIRS_PER_WEAPON_HUD),
	MUTATOR_COPY_FIELD(struct weapon_hud_interface_definition, screen_effects, sizeof(struct hud_screen_effect_definition),
		MAXIMUM_SCREEN_EFFECTS_PER_WEAPON_HUD),
};

static struct mutator_tag_field const mutator_damage_effect_fields[] =
{
	MUTATOR_FIELD(struct damage_effect_definition, damage.damage_minimum, _mutator_field_real, 0.f, 10000.f),
	MUTATOR_FIELD(struct damage_effect_definition, damage.damage_lower_bound, _mutator_field_real, 0.f, 10000.f),
	MUTATOR_FIELD(struct damage_effect_definition, damage.damage_upper_bound, _mutator_field_real, 0.f, 10000.f),
	MUTATOR_FIELD(struct damage_effect_definition, damage.active_camouflage_damage, _mutator_field_real, 0.f, 1.f),
	MUTATOR_ARRAY_FIELD(struct damage_effect_definition, damage.material_modifiers, mutator_material_names, 0.f, 10.f),
};

/* a colour's red, green and blue, each 0 to 1 */
#define MUTATOR_COLOR_FIELD(structure, field) \
	{ #field, _mutator_field_array, (short)sizeof(real), (long)offsetof(structure, field), 0.f, 1.f, 0, \
		mutator_color_component_names, NUMBEROF(mutator_color_component_names), 0, NULL }

static struct mutator_tag_field const mutator_decal_fields[] =
{
	MUTATOR_FLAGS_FIELD("flags", struct mutator_decal_definition, flags, mutator_decal_flag_names),
	MUTATOR_ENUM_FIELD(struct mutator_decal_definition, type, mutator_decal_type_names),
	MUTATOR_FIELD(struct mutator_decal_definition, radius_lower_bound, _mutator_field_real, 0.f, 10.f),
	MUTATOR_FIELD(struct mutator_decal_definition, radius_upper_bound, _mutator_field_real, 0.f, 10.f),
	MUTATOR_FIELD(struct mutator_decal_definition, intensity_lower_bound, _mutator_field_real, 0.f, 1.f),
	MUTATOR_FIELD(struct mutator_decal_definition, intensity_upper_bound, _mutator_field_real, 0.f, 1.f),
	MUTATOR_COLOR_FIELD(struct mutator_decal_definition, color_lower_bound),
	MUTATOR_COLOR_FIELD(struct mutator_decal_definition, color_upper_bound),
	MUTATOR_ENUM_FIELD(struct mutator_decal_definition, framebuffer_blend_function,
		mutator_framebuffer_blend_function_names),
	MUTATOR_REFERENCE_FIELD(struct mutator_decal_definition, map, BITMAP_GROUP_TAG),
};

/* (each material's modifier is in the array) */
typedef char mutator_material_modifiers_assert[
	NUMBEROF(((struct damage_effect_definition *)0)->damage.material_modifiers) >= NUMBER_OF_MATERIAL_TYPES ? 1 : -1];

/* each group a mutator can edit, by the name a mutator gives it */
static struct mutator_tag_definition const mutator_tag_groups[] =
{
	{ "weapon", WEAPON_DEFINITION_TAG, mutator_weapon_fields, NUMBEROF(mutator_weapon_fields) },
	{ "projectile", PROJECTILE_DEFINITION_TAG, mutator_projectile_fields, NUMBEROF(mutator_projectile_fields) },
	{ "weapon_hud_interface", WEAPON_HUD_INTERFACE_GROUP_TAG, mutator_weapon_hud_interface_fields,
		NUMBEROF(mutator_weapon_hud_interface_fields) },
	{ "damage_effect", DAMAGE_EFFECT_DEFINITION_TAG, mutator_damage_effect_fields,
		NUMBEROF(mutator_damage_effect_fields) },
	{ "decal", DECAL_GROUP_TAG, mutator_decal_fields, NUMBEROF(mutator_decal_fields) },
};

static struct
{
	struct mutator_tag_undo undos[MAXIMUM_MUTATOR_TAG_UNDOS];
	long undo_count;
	/* the elements the copies made, in blocks of the tags until the next
	map's tags load */
	long element_bytes[MUTATOR_TAG_ELEMENT_BYTES / sizeof(long)];
	long element_bytes_used;
	short made;
	short passed_over;
} mutator_tags;

/* ---------- private code */

static struct mutator_tag_definition const *mutator_tags_group_find(char const *name)
{
	short index;

	for (index = 0; index < NUMBEROF(mutator_tag_groups); index++)
	{
		if (!strcmp(mutator_tag_groups[index].name, name))
			return &mutator_tag_groups[index];
	}
	return NULL;
}

/* a field's name in a mutator: its structure's, without the structure it is
in ("weapon.zoom_level_count" is "zoom_level_count") */
static char const *mutator_tags_field_name(struct mutator_tag_field const *field)
{
	char const *dot = strrchr(field->name, '.');

	return dot ? dot + 1 : field->name;
}

static struct mutator_tag_field const *mutator_tags_field_find(struct mutator_tag_definition const *definition,
	char const *name, size_t length)
{
	short index;

	for (index = 0; index < definition->field_count; index++)
	{
		char const *field_name = mutator_tags_field_name(&definition->fields[index]);

		if (strlen(field_name) == length && !strncmp(field_name, name, length))
			return &definition->fields[index];
	}
	return NULL;
}

/* the index of `name` among names, or NONE */
static short mutator_tags_name_find(char const *const *names, short name_count, char const *name, size_t length)
{
	short index;

	for (index = 0; index < name_count; index++)
	{
		if (strlen(names[index]) == length && !strncmp(names[index], name, length))
			return index;
	}
	return NONE;
}

/* a block's element: the index of `name` among names, or a number (0 to
32767), or NONE */
static short mutator_tags_index_find(char const *const *names, short name_count, char const *name, size_t length)
{
	short index = mutator_tags_name_find(names, name_count, name, length);
	long number = 0;

	if (index != NONE)
		return index;
	if (!length || length > 5)
		return NONE;
	for (index = 0; index < (short)length; index++)
	{
		if (name[index] < '0' || name[index] > '9')
			return NONE;
		number = number * 10 + (name[index] - '0');
	}
	return number <= 32767 ? (short)number : NONE;
}

/* names, separated by commas, as bits of names: FALSE if one is not of them */
static boolean mutator_tags_flags_find(char const *const *names, short name_count, char const *text,
	unsigned long *bits)
{
	*bits = 0;
	while (*text)
	{
		char const *end = strchr(text, ',');
		size_t length = end ? (size_t)(end - text) : strlen(text);
		short index = mutator_tags_name_find(names, name_count, text, length);

		if (index == NONE)
			return FALSE;
		*bits |= 1UL << index;
		text += length;
		if (*text == ',')
			text++;
	}
	return TRUE;
}

/* the field `path` names in definition, and in the tag at `base` (if not
NULL) its address, following each block's element: FALSE (said into reason)
if the path names no field, or, in the tag, an element the block has not */
static struct mutator_tag_field const *mutator_tags_path_find(struct mutator_tag_definition const *definition,
	char const *path, byte *base, byte **address, char *reason, unsigned long size)
{
	char const *segment = path;
	short segments = 0;

	while (definition)
	{
		char const *dot = strchr(segment, '.');
		size_t length = dot ? (size_t)(dot - segment) : strlen(segment);
		struct mutator_tag_field const *field;

		if (++segments > MAXIMUM_MUTATOR_TAG_PATH_SEGMENTS || !length)
			break;
		field = mutator_tags_field_find(definition, segment, length);
		if (!field)
		{
			snprintf(reason, size, "%.24s has no field %.*s", definition->name, (int)MIN(length, 48), segment);
			return NULL;
		}
		if (field->type == _mutator_field_array)
		{
			/* (its element, by index or name, last) */
			short index = dot ? mutator_tags_index_find(field->names, field->name_count, dot + 1, strlen(dot + 1)) :
				NONE;

			if (index == NONE || index >= field->name_count)
			{
				snprintf(reason, size, "%.48s: name one of %.24s's elements", path, mutator_tags_field_name(field));
				return NULL;
			}
			if (base)
				*address = base + field->offset + (long)index * field->size;
			return field;
		}
		if (field->type != _mutator_field_block)
		{
			if (dot)
				break;
			if (base)
				*address = base + field->offset;
			return field;
		}
		/* (a block's element) */
		{
			char const *element = dot ? dot + 1 : NULL;
			char const *element_end;
			size_t element_length;
			short index;

			if (!element)
			{
				snprintf(reason, size, "%.48s is a block: name an element of it", path);
				return NULL;
			}
			element_end = strchr(element, '.');
			element_length = element_end ? (size_t)(element_end - element) : strlen(element);
			index = mutator_tags_index_find(field->names, field->name_count, element, element_length);
			if (index == NONE || !element_end)
			{
				snprintf(reason, size, "%.48s: %.*s is not an element of %.24s, or names no field of it", path,
					(int)MIN(element_length, 32), element, mutator_tags_field_name(field));
				return NULL;
			}
			if (base)
			{
				struct tag_block *block = (struct tag_block *)(base + field->offset);

				if (index >= block->count || !block->address)
				{
					snprintf(reason, size, "%.48s: the tag's %.24s has %ld elements", path,
						mutator_tags_field_name(field), (long)block->count);
					return NULL;
				}
				base = (byte *)block->address + (long)index * field->size;
			}
			definition = field->definition;
			segment = element_end + 1;
		}
	}
	snprintf(reason, size, "%.48s is not a field's path", path);
	return NULL;
}

/* an edit read from `data` at *offset, moved past it: FALSE if the bytes
are not an edit's */
static boolean mutator_tags_read_text(byte const *data, long size, long *offset, long maximum, char const **text)
{
	long index;

	for (index = *offset; index < size && index - *offset < maximum; index++)
	{
		if (!data[index])
		{
			*text = (char const *)data + *offset;
			*offset = index + 1;
			return TRUE;
		}
	}
	return FALSE;
}

static boolean mutator_tags_read(byte const *data, long size, long *offset, struct mutator_tag_edit *edit)
{
	long at = *offset;
	unsigned long bits;

	if (size - at < 8)
		return FALSE;
	edit->kind = data[at];
	edit->element_count = data[at + 1];
	edit->append = data[at + 2];
	bits = (unsigned long)data[at + 4] | ((unsigned long)data[at + 5] << 8) | ((unsigned long)data[at + 6] << 16) |
		((unsigned long)data[at + 7] << 24);
	csmemcpy(&edit->number, &bits, sizeof(edit->number));
	at += 8;
	if (edit->kind >= NUMBER_OF_MUTATOR_TAG_EDIT_KINDS || edit->append > 1 || data[*offset + 3] ||
		edit->element_count > MAXIMUM_MUTATOR_TAG_COPY_ELEMENTS ||
		!mutator_tags_read_text(data, size, &at, MUTATOR_TAG_GROUP_LENGTH, &edit->group) ||
		!mutator_tags_read_text(data, size, &at, MUTATOR_TAG_NAME_LENGTH, &edit->tag) ||
		!mutator_tags_read_text(data, size, &at, MUTATOR_TAG_PATH_LENGTH, &edit->path) ||
		!mutator_tags_read_text(data, size, &at, MUTATOR_TAG_TEXT_LENGTH, &edit->text) ||
		size - at < 2 * (long)edit->element_count)
	{
		return FALSE;
	}
	edit->elements = data + at;
	*offset = at + 2 * edit->element_count;
	return TRUE;
}

static short mutator_tags_element(struct mutator_tag_edit const *edit, short index)
{
	return (short)(edit->elements[2 * index] | (edit->elements[2 * index + 1] << 8));
}

/* "names: a, b, c" into reason */
static void mutator_tags_say_names(char *reason, unsigned long size, char const *path, char const *what,
	struct mutator_tag_field const *field)
{
	short index;

	snprintf(reason, size, "%.48s %s", path, what);
	for (index = 0; index < field->name_count; index++)
	{
		size_t length = strlen(reason);

		snprintf(reason + length, size - length, "%s%s", index ? ", " : " ", field->names[index]);
	}
}

/* whether an edit's value is one its field takes (said into reason if not) */
static boolean mutator_tags_value_valid(struct mutator_tag_field const *field, struct mutator_tag_edit const *edit,
	char const *path, char *reason, unsigned long size)
{
	unsigned long bits;
	short index;

	switch (field->type)
	{
	case _mutator_field_short:
	case _mutator_field_real:
	case _mutator_field_angle:
	case _mutator_field_time:
	case _mutator_field_speed:
	case _mutator_field_array:
		/* (not a NaN, not infinite) */
		if (edit->kind != _mutator_tag_edit_number || !(edit->number == edit->number &&
			edit->number - edit->number == 0.f))
		{
			snprintf(reason, size, "%.48s is a number", path);
			return FALSE;
		}
		if (edit->number < field->minimum || edit->number > field->maximum ||
			(field->type == _mutator_field_short && edit->number != (real)(short)edit->number))
		{
			snprintf(reason, size, "%.48s must be %s from %g to %g", path,
				field->type == _mutator_field_short ? "a whole number" : "a number", field->minimum, field->maximum);
			return FALSE;
		}
		return TRUE;

	case _mutator_field_enum:
		if (edit->kind == _mutator_tag_edit_text &&
			mutator_tags_name_find(field->names, field->name_count, edit->text, strlen(edit->text)) != NONE)
		{
			return TRUE;
		}
		mutator_tags_say_names(reason, size, path, "is one of", field);
		return FALSE;

	case _mutator_field_flags:
		if (edit->kind == _mutator_tag_edit_text &&
			mutator_tags_flags_find(field->names, field->name_count, edit->text, &bits))
		{
			return TRUE;
		}
		mutator_tags_say_names(reason, size, path, "is a list of", field);
		return FALSE;

	case _mutator_field_reference:
		if (edit->kind == _mutator_tag_edit_text)
			return TRUE;
		snprintf(reason, size, "%.48s is a tag's name", path);
		return FALSE;

	case _mutator_field_copy:
		if (edit->kind != _mutator_tag_edit_copy || !edit->text[0])
		{
			snprintf(reason, size, "%.48s is copied: from = a tag's name", path);
			return FALSE;
		}
		for (index = 0; index < edit->element_count; index++)
		{
			if (mutator_tags_element(edit, index) < 0 || mutator_tags_element(edit, index) >= field->maximum_count)
			{
				snprintf(reason, size, "%.48s has at most %d elements", path, field->maximum_count);
				return FALSE;
			}
		}
		return TRUE;
	}
	snprintf(reason, size, "%.48s cannot be set", path);
	return FALSE;
}

/* whether a pose's group, node, scale and offset are ones it takes */
static boolean mutator_tags_pose_valid(struct mutator_tag_edit const *edit, char *reason, unsigned long size)
{
	short index;

	if (object_pose_group_tag(edit->group) == NONE)
	{
		snprintf(reason, size, "this game cannot pose objects of the group %.32s", edit->group);
		return FALSE;
	}
	if (!edit->path[0] || edit->text[0] || edit->append)
	{
		snprintf(reason, size, "a pose names a node, and nothing else");
		return FALSE;
	}
	/* (not a NaN, not infinite) */
	if (!(edit->number == edit->number && edit->number - edit->number == 0.f) ||
		edit->number < OBJECT_POSE_MINIMUM_SCALE || edit->number > OBJECT_POSE_MAXIMUM_SCALE)
	{
		snprintf(reason, size, "a pose's scale must be from %g to %g", OBJECT_POSE_MINIMUM_SCALE,
			OBJECT_POSE_MAXIMUM_SCALE);
		return FALSE;
	}
	if (edit->element_count != 0 && edit->element_count != 3)
	{
		snprintf(reason, size, "a pose's offset is three numbers");
		return FALSE;
	}
	for (index = 0; index < edit->element_count; index++)
	{
		short element = mutator_tags_element(edit, index);

		if (element < -1000 * OBJECT_POSE_MAXIMUM_OFFSET || element > 1000 * OBJECT_POSE_MAXIMUM_OFFSET)
		{
			snprintf(reason, size, "a pose's offset must be from %g to %g", -OBJECT_POSE_MAXIMUM_OFFSET,
				OBJECT_POSE_MAXIMUM_OFFSET);
			return FALSE;
		}
	}
	return TRUE;
}

/* the path with its first block's element "all" as `index` ("material_responses.all.default_effect" as
"material_responses.3.default_effect"), into `path`: FALSE if it has none */
static boolean mutator_tags_path_each(char const *edit_path, short index, char *path, size_t size)
{
	char const *all = strstr(edit_path, ".all.");

	if (!all)
		return FALSE;
	snprintf(path, size, "%.*s.%d.%s", (int)(all - edit_path), edit_path, index, all + 5);
	return TRUE;
}

/* whether a death's group and body are ones it takes */
static boolean mutator_tags_death_valid(struct mutator_tag_edit const *edit, char *reason, unsigned long size)
{
	long group_tag = object_pose_group_tag(edit->group);

	if (group_tag != 'obje' && group_tag != 'unit' && group_tag != 'bipd' && group_tag != 'vehi')
	{
		snprintf(reason, size, "a death's objects are units: unit, biped or vehicle, not %.32s", edit->group);
		return FALSE;
	}
	if (edit->path[0] || edit->append || edit->element_count || edit->number != 0.f ||
		(strcmp(edit->text, "vanish") && strcmp(edit->text, "stays")))
	{
		snprintf(reason, size, "a death names its objects, an effect and its body (vanish or stays), nothing else");
		return FALSE;
	}
	return TRUE;
}

/* whether a weapon's trigger fires the projectile (a where of "fired") */
static boolean mutator_tags_fired(long projectile_index)
{
	struct tag_iterator iterator;
	long weapon_index;

	tag_iterator_new(&iterator, WEAPON_DEFINITION_TAG);
	while ((weapon_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct weapon_definition const *weapon = (struct weapon_definition const *)tag_get(WEAPON_DEFINITION_TAG,
			weapon_index);
		long trigger;

		for (trigger = 0; trigger < weapon->weapon.triggers.count; trigger++)
		{
			if (((struct weapon_trigger_definition const *)weapon->weapon.triggers.address)[trigger].projectile.index ==
				projectile_index)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* an edit's "*?<field>=<names>": the enum field it is of each tag, and the
names after the = (FALSE, said into reason, if the field is not an enum or
a name not its); TRUE with no field for a tag that is not one */
static boolean mutator_tags_where(struct mutator_tag_definition const *definition, char const *tag,
	struct mutator_tag_field const **field, char const **names, char *reason, unsigned long size)
{
	char path[MUTATOR_TAG_PATH_LENGTH];
	char const *equals;
	unsigned long bits;

	*field = NULL;
	*names = NULL;
	if (strncmp(tag, MUTATOR_TAG_WHERE, strlen(MUTATOR_TAG_WHERE)))
		return TRUE;
	tag += strlen(MUTATOR_TAG_WHERE);
	/* (projectiles a weapon fires: no field, and the names the word) */
	if (!strcmp(tag, MUTATOR_TAG_WHERE_FIRED))
	{
		if (definition->group_tag != PROJECTILE_DEFINITION_TAG)
		{
			snprintf(reason, size, "a where of fired is of projectiles");
			return FALSE;
		}
		*names = tag;
		return TRUE;
	}
	equals = strchr(tag, '=');
	if (!equals || equals == tag || (size_t)(equals - tag) >= sizeof(path))
	{
		snprintf(reason, size, "a where names an enum field and its values");
		return FALSE;
	}
	snprintf(path, sizeof(path), "%.*s", (int)(equals - tag), tag);
	*field = mutator_tags_path_find(definition, path, NULL, NULL, reason, size);
	*names = equals + 1;
	if (!*field || (*field)->type != _mutator_field_enum ||
		!mutator_tags_flags_find((*field)->names, (*field)->name_count, *names, &bits))
	{
		if (*field)
			mutator_tags_say_names(reason, size, path, "in a where is one of", *field);
		return FALSE;
	}
	return TRUE;
}

static boolean mutator_tags_edit_valid(struct mutator_tag_edit const *edit, char *reason, unsigned long size)
{
	struct mutator_tag_definition const *definition = mutator_tags_group_find(edit->group);
	struct mutator_tag_field const *field;

	if (edit->kind == _mutator_tag_edit_pose)
		return mutator_tags_pose_valid(edit, reason, size);
	if (edit->kind == _mutator_tag_edit_death)
		return mutator_tags_death_valid(edit, reason, size);
	if (edit->kind == _mutator_tag_edit_explosion)
	{
		if (!edit->tag[0] || edit->group[0] || edit->path[0] || edit->text[0] || edit->append ||
			edit->element_count || edit->number != 0.f)
		{
			snprintf(reason, size, "an explosion names its decal, and nothing else");
			return FALSE;
		}
		return TRUE;
	}
	if (!definition)
	{
		snprintf(reason, size, "this game cannot edit tags of the group %.32s", edit->group);
		return FALSE;
	}
	if (!edit->tag[0])
	{
		snprintf(reason, size, "a tag edit of %.32s names no tag", edit->group);
		return FALSE;
	}
	{
		struct mutator_tag_field const *where;
		char const *names;

		if (!mutator_tags_where(definition, edit->tag, &where, &names, reason, size))
			return FALSE;
	}
	/* (an "all" element checked as the first) */
	{
		char path[MUTATOR_TAG_PATH_LENGTH + 8];

		if (!mutator_tags_path_each(edit->path, 0, path, sizeof(path)))
			snprintf(path, sizeof(path), "%s", edit->path);
		field = mutator_tags_path_find(definition, path, NULL, NULL, reason, size);
		return field && mutator_tags_value_valid(field, edit, edit->path, reason, size);
	}
}

static boolean mutator_tags_undo_add(void *address, short size)
{
	struct mutator_tag_undo *undo;

	if (mutator_tags.undo_count == MAXIMUM_MUTATOR_TAG_UNDOS || size > MUTATOR_TAG_UNDO_BYTES)
		return FALSE;
	undo = &mutator_tags.undos[mutator_tags.undo_count++];
	undo->address = address;
	undo->size = size;
	csmemcpy(undo->bytes, address, size);
	return TRUE;
}

static void mutator_tags_pass_over(struct mutator_tag_edit const *edit, char const *reason)
{
	mutator_tags.passed_over++;
	error(_error_log, "mutators: the edit of %s %s's %s passed over: %s", edit->group, edit->tag, edit->path, reason);
}

/* a tag an edit names: the map's, or from a stock map if a Custom Edition map
lacks it (stock_tags.c) */
static long mutator_tags_named_find(long group_tag, char const *name)
{
	long index = tag_loaded(group_tag, name);

	return index != NONE ? index : stock_tags_find(group_tag, name);
}

/* a copy: the elements of the block at `path` in the tag named by the
edit's text, in place of (or after) the block's own */
static boolean mutator_tags_copy(struct mutator_tag_field const *field, struct mutator_tag_edit const *edit,
	struct tag_block *block, unsigned long group_tag, char *reason, unsigned long size)
{
	long source_index = mutator_tags_named_find(group_tag, edit->text);
	struct tag_block const *source;
	long count;
	long bytes;
	byte *elements;
	short index;

	if (source_index == NONE)
	{
		snprintf(reason, size, "the map has no %.64s to copy from", edit->text);
		return FALSE;
	}
	source = (struct tag_block const *)((byte const *)tag_get(group_tag, source_index) + field->offset);
	count = (edit->append ? block->count : 0) + (edit->element_count ? edit->element_count : source->count);
	for (index = 0; index < edit->element_count; index++)
	{
		if (mutator_tags_element(edit, index) >= source->count)
		{
			snprintf(reason, size, "%.64s's %s has no element %d", edit->text, mutator_tags_field_name(field),
				mutator_tags_element(edit, index));
			return FALSE;
		}
	}
	if (count > field->maximum_count || (count && !source->address && source->count))
	{
		snprintf(reason, size, "it would have %ld elements, more than %d", count, field->maximum_count);
		return FALSE;
	}
	bytes = (count * field->size + (long)sizeof(long) - 1) & ~((long)sizeof(long) - 1);
	if (bytes > (long)sizeof(mutator_tags.element_bytes) - mutator_tags.element_bytes_used)
	{
		snprintf(reason, size, "the copies have more than %ld bytes", (long)sizeof(mutator_tags.element_bytes));
		return FALSE;
	}
	if (!mutator_tags_undo_add(block, (short)sizeof(*block)))
	{
		snprintf(reason, size, "more than %d edits", MAXIMUM_MUTATOR_TAG_UNDOS);
		return FALSE;
	}
	elements = (byte *)mutator_tags.element_bytes + mutator_tags.element_bytes_used;
	mutator_tags.element_bytes_used += bytes;
	count = 0;
	if (edit->append && block->count)
	{
		csmemcpy(elements, block->address, block->count * field->size);
		count = block->count;
	}
	if (edit->element_count)
	{
		for (index = 0; index < edit->element_count; index++, count++)
		{
			csmemcpy(elements + count * field->size,
				(byte const *)source->address + mutator_tags_element(edit, index) * field->size, field->size);
		}
	}
	else if (source->count)
	{
		csmemcpy(elements + count * field->size, source->address, source->count * field->size);
		count += source->count;
	}
	block->count = count;
	block->address = count ? elements : NULL;
	return TRUE;
}

static boolean mutator_tags_make_one(struct mutator_tag_edit const *edit, long tag_index, char *reason,
	unsigned long size);

/* an edit made in one tag (of an "all" element, in each element the tag's
block has): FALSE (said into reason) if it is passed over */
static boolean mutator_tags_make_elements(struct mutator_tag_edit const *edit, long tag_index, char *reason,
	unsigned long size)
{
	char path[MUTATOR_TAG_PATH_LENGTH + 8];
	struct mutator_tag_edit each = *edit;
	short index;

	if (!mutator_tags_path_each(edit->path, 0, path, sizeof(path)))
		return mutator_tags_make_one(edit, tag_index, reason, size);
	each.path = path;
	for (index = 0; mutator_tags_path_each(edit->path, index, path, sizeof(path)); index++)
	{
		/* (past the block's last element: done, unless it has none) */
		if (!mutator_tags_make_one(&each, tag_index, reason, size))
			return index > 0 && strstr(reason, " elements");
	}
	return TRUE;
}

/* an edit made in the map's tags: in the tag it names, or with "*" in every
tag of the group the map has (the add-ons' too, which are its tags once it
has loaded): FALSE (said into reason) if it is passed over, or with "*" made
in none */
static boolean mutator_tags_make(struct mutator_tag_edit const *edit, char *reason, unsigned long size)
{
	struct mutator_tag_definition const *definition = mutator_tags_group_find(edit->group);
	struct mutator_tag_field const *where;
	char const *names;
	struct tag_iterator iterator;
	long tag_index;
	short made = 0;

	if (edit->kind == _mutator_tag_edit_pose || edit->kind == _mutator_tag_edit_death)
		return mutator_tags_make_one(edit, NONE, reason, size);
	if (edit->kind == _mutator_tag_edit_explosion)
	{
		long decal_index = mutator_tags_named_find(DECAL_GROUP_TAG, edit->tag);
		struct mutator_decal_definition const *decal;

		if (decal_index == NONE)
		{
			snprintf(reason, size, "the map has no %.64s", edit->tag);
			return FALSE;
		}
		decal = (struct mutator_decal_definition const *)tag_get(DECAL_GROUP_TAG, decal_index);
		explosion_decal_add(decal_index, MAX(decal->radius_lower_bound, decal->radius_upper_bound));
		return TRUE;
	}
	if (strcmp(edit->tag, MUTATOR_TAG_EVERY) && strncmp(edit->tag, MUTATOR_TAG_WHERE, strlen(MUTATOR_TAG_WHERE)))
	{
		tag_index = definition ? tag_loaded(definition->group_tag, edit->tag) : NONE;
		if (tag_index == NONE)
		{
			snprintf(reason, size, "the map has no such tag");
			return FALSE;
		}
		return mutator_tags_make_elements(edit, tag_index, reason, size);
	}
	/* (each tag that has what it sets, of the values a where names: one
	without the element is passed by) */
	if (!mutator_tags_where(definition, edit->tag, &where, &names, reason, size))
		return FALSE;
	tag_iterator_new(&iterator, definition->group_tag);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		if (!where && names && !mutator_tags_fired(tag_index))
			continue;
		if (where)
		{
			short value = *(short const *)((byte const *)tag_get(definition->group_tag, tag_index) + where->offset);
			unsigned long bits;

			mutator_tags_flags_find(where->names, where->name_count, names, &bits);
			if (value < 0 || value >= where->name_count || !(bits & (1UL << value)))
				continue;
		}
		if (mutator_tags_make_elements(edit, tag_index, reason, size))
			made++;
		else if (strstr(reason, "more than"))
			return FALSE;
	}
	if (!made)
	{
		snprintf(reason, size, "the map has no tag of the group with it");
		return FALSE;
	}
	return TRUE;
}

static boolean mutator_tags_make_one(struct mutator_tag_edit const *edit, long tag_index, char *reason,
	unsigned long size)
{
	struct mutator_tag_definition const *definition = mutator_tags_group_find(edit->group);
	struct mutator_tag_field const *field;
	byte *address = NULL;
	unsigned long bits;
	real value;

	if (edit->kind == _mutator_tag_edit_pose)
	{
		real_vector3d offset;

		offset.i = offset.j = offset.k = 0.f;
		if (edit->element_count == 3)
		{
			offset.i = (real)mutator_tags_element(edit, 0) / 1000.f;
			offset.j = (real)mutator_tags_element(edit, 1) / 1000.f;
			offset.k = (real)mutator_tags_element(edit, 2) / 1000.f;
		}
		return object_pose_add(object_pose_group_tag(edit->group), edit->tag, edit->path, edit->number, &offset,
			reason, size);
	}
	if (edit->kind == _mutator_tag_edit_death)
	{
		long effect_index = edit->tag[0] ? mutator_tags_named_find(EFFECT_DEFINITION_TAG, edit->tag) : NONE;

		if (edit->tag[0] && effect_index == NONE)
		{
			snprintf(reason, size, "the map has no %.64s", edit->tag);
			return FALSE;
		}
		/* ("object" is every unit: a death is a unit's) */
		return object_death_add(!strcmp(edit->group, "object") ? 'unit' : object_pose_group_tag(edit->group),
			effect_index, !strcmp(edit->text, "vanish"), reason, size);
	}
	field = mutator_tags_path_find(definition, edit->path, (byte *)tag_get(definition->group_tag, tag_index),
		&address, reason, size);
	if (!field)
		return FALSE;
	if (field->type == _mutator_field_copy)
		return mutator_tags_copy(field, edit, (struct tag_block *)address, definition->group_tag, reason, size);
	if (field->type == _mutator_field_reference)
	{
		struct tag_reference *reference = (struct tag_reference *)address;
		long index = edit->text[0] ? mutator_tags_named_find(field->group_tag, edit->text) : NONE;

		if (edit->text[0] && index == NONE)
		{
			snprintf(reason, size, "the map has no %.64s", edit->text);
			return FALSE;
		}
		if (!mutator_tags_undo_add(reference, (short)sizeof(*reference)))
		{
			snprintf(reason, size, "more than %d edits", MAXIMUM_MUTATOR_TAG_UNDOS);
			return FALSE;
		}
		reference->group_tag = field->group_tag;
		reference->name = index == NONE ? mutator_tags_no_name : tag_get_name(index);
		reference->name_length = (long)strlen(reference->name);
		reference->index = index;
		return TRUE;
	}
	if (!mutator_tags_undo_add(address, field->size) || (field->type == _mutator_field_time &&
		!mutator_tags_undo_add(address - field->offset + field->runtime_offset, (short)sizeof(real))))
	{
		snprintf(reason, size, "more than %d edits", MAXIMUM_MUTATOR_TAG_UNDOS);
		return FALSE;
	}
	switch (field->type)
	{
	case _mutator_field_short:
		*(short *)address = (short)edit->number;
		break;

	case _mutator_field_real:
	case _mutator_field_array:
		*(real *)address = edit->number;
		break;

	case _mutator_field_time:
		*(real *)address = edit->number;
		value = edit->number > 0.f ? 1.f / (edit->number * (real)TICKS_PER_SECOND) : 1.f;
		*(real *)(address - field->offset + field->runtime_offset) = value;
		break;

	case _mutator_field_speed:
		value = edit->number / (real)TICKS_PER_SECOND;
		*(real *)address = value;
		break;

	case _mutator_field_angle:
		value = edit->number * (_pi / 180.f);
		*(real *)address = value;
		break;

	case _mutator_field_enum:
		*(short *)address = mutator_tags_name_find(field->names, field->name_count, edit->text, strlen(edit->text));
		break;

	case _mutator_field_flags:
		mutator_tags_flags_find(field->names, field->name_count, edit->text, &bits);
		if (field->size == sizeof(word))
			*(word *)address = (word)bits;
		else
			*(unsigned long *)address = bits;
		break;
	}
	return TRUE;
}

/* ---------- public code */

int mutators_tag_edit_check(
	byte const *edit_bytes,
	long size,
	char *reason,
	unsigned long reason_size)
{
	struct mutator_tag_edit edit;
	long offset = 0;
	char text[160];

	if (!reason || !reason_size)
	{
		reason = text;
		reason_size = sizeof(text);
	}
	reason[0] = 0;
	if (!mutator_tags_read(edit_bytes, size, &offset, &edit) || offset != size)
	{
		snprintf(reason, reason_size, "a tag edit's names are too long");
		return FALSE;
	}
	return mutator_tags_edit_valid(&edit, reason, reason_size);
}

void mutator_tags_loaded(
	void)
{
	/* (the map's own bytes everywhere: nothing to undo) */
	mutator_tags.undo_count = 0;
	mutator_tags.element_bytes_used = 0;
	object_pose_clear();
	object_death_clear();
	explosion_decal_clear();
}

void mutator_tags_undo(
	void)
{
	while (mutator_tags.undo_count > 0)
	{
		struct mutator_tag_undo const *undo = &mutator_tags.undos[--mutator_tags.undo_count];

		csmemcpy(undo->address, undo->bytes, undo->size);
	}
	mutator_tags.element_bytes_used = 0;
	object_pose_clear();
	object_death_clear();
	explosion_decal_clear();
}

boolean mutator_tags_play(
	byte const *edits,
	long size)
{
	struct mutator_tag_edit edit;
	char reason[160];
	long offset = 0;

	mutator_tags_undo();
	/* (all of them checked before any is made) */
	while (offset < size)
	{
		if (!mutator_tags_read(edits, size, &offset, &edit) || !mutator_tags_edit_valid(&edit, reason, sizeof(reason)))
			return FALSE;
	}
	mutator_tags.made = 0;
	mutator_tags.passed_over = 0;
	offset = 0;
	while (offset < size && mutator_tags_read(edits, size, &offset, &edit))
	{
		if (mutator_tags_make(&edit, reason, sizeof(reason)))
			mutator_tags.made++;
		else
			mutator_tags_pass_over(&edit, reason);
	}
	stock_tags_finish();
	if (mutator_tags.made || mutator_tags.passed_over)
	{
		error(_error_log, "mutators: %d tag edits made, %d passed over", mutator_tags.made, mutator_tags.passed_over);
		if (mutator_tags.passed_over)
			console_printf(FALSE, "Mutators: %d tag edits passed over: this map has not their tags (debug.txt)",
				mutator_tags.passed_over);
	}
	return TRUE;
}
