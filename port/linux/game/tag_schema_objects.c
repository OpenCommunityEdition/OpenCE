/*
TAG_SCHEMA_OBJECTS.C

The schemas (tag_schema.h) of objects, units, items, devices and the AI's
definitions (obje and every group that inherits from it, actr, actv, udlg,
itmc).
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"
#include "ai/actor_definitions.h"
#include "ai/actor_types.h"
#include "cache/predicted_resources.h"
#include "devices/device_controls.h"
#include "devices/device_definitions.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "items/equipment_definitions.h"
#include "items/item_definitions.h"
#include "items/projectile_definitions.h"
#include "items/weapon_definitions.h"
#include "items/weapon_export_function_mode.h"
#include "items/weapons.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "objects/object_types.h"
#include "math/periodic_functions.h"
#include "units/biped_definitions.h"
#include "units/dialogue_definitions.h"
#include "units/unit_definitions.h"
#include "units/units.h"

/* ---------- constants */

/* the game's, which their units keep to themselves */
enum
{
	/* periodic_functions.c */
	NUMBER_OF_PERIODIC_FUNCTIONS = 12,
	/* weapons.c */
	MAXIMUM_NUMBER_OF_TRIGGERS_PER_WEAPON = 2,
	MAXIMUM_NUMBER_OF_MAGAZINES_PER_WEAPON = 2,
	NUMBER_OF_TRIGGER_OVERCHARGED_ACTIONS = 3,
	NUMBER_OF_WEAPON_SECONDARY_TRIGGER_MODES = 5,
	/* weapons.c, projectile_distribute: a point or a horizontal fan */
	NUMBER_OF_PROJECTILE_DISTRIBUTION_FUNCTIONS = 2,
	/* weapons.c, weapon_compute_movement_penalty's switch */
	NUMBER_OF_WEAPON_MOVEMENT_PENALTY_MODES = 3,
	/* ai.c (the volume of a unit's sounds, of a weapon's or projectile's
	noise) */
	NUMBER_OF_AI_SOUND_VOLUMES = 5,
	/* units.c, unit_export_function_values */
	NUMBER_OF_UNIT_FUNCTION_MODES = 8,
	/* bipeds.c, biped_export_function_values: none, flying speed */
	NUMBER_OF_BIPED_FUNCTION_MODES = 2,
	/* vehicles.c */
	NUMBER_OF_VEHICLE_TYPES = 7,
	NUMBER_OF_VEHICLE_FUNCTIONS = 37,
	/* units.c's unit datum's seat_power[2] */
	MAXIMUM_POWERED_SEATS_PER_UNIT = 2,
	/* unit_dialogue.c, unit_find_dialogue_variant's variant_indices[16] */
	MAXIMUM_DIALOGUE_VARIANTS_PER_UNIT = 16,
	/* projectiles.c */
	NUMBER_OF_PROJECTILE_DETONATION_TIMER_STARTS = 3,
	NUMBER_OF_PROJECTILE_EXPORT_FUNCTION_MODES = 4,
	/* devices.c, device_export_function_values */
	NUMBER_OF_DEVICE_FUNCTION_MODES = 7,
	/* device_machines.c: door, platform, gear */
	NUMBER_OF_MACHINE_TYPES = 3,
	/* device_controls.c, control_toggle's switch (its default asserts) */
	NUMBER_OF_CONTROL_TYPES = 4,
	/* device_controls.c: touched, destroyed */
	NUMBER_OF_CONTROL_TRIGGERS = 2,

	/* the tool's limits, where the game has none of its own */
	MAXIMUM_WIDGETS_PER_OBJECT = 4,
	MAXIMUM_PREDICTED_RESOURCES = 1024,
	MAXIMUM_PERMUTATIONS_PER_CHANGE_COLOR = 32,
	MAXIMUM_AMMUNITION_OBJECTS_PER_MAGAZINE = 8,
	MAXIMUM_FIRING_EFFECTS_PER_TRIGGER = 8,
	MAXIMUM_CAMERA_TRACKS_PER_UNIT_CAMERA = 2,
	MAXIMUM_HUDS_PER_UNIT = 2,
	MAXIMUM_SEATS_PER_UNIT = 16,
	MAXIMUM_CONTACT_POINTS_PER_BIPED = 2,
	MAXIMUM_PERMUTATIONS_PER_ITEM_COLLECTION = 32767,
};

/* ---------- structures */

/* following_camera.c */
struct unit_camera_track
{
	struct tag_reference track;
	long unused[3];
};

typedef char verify_unit_camera_track_size[sizeof(struct unit_camera_track) == 0x1C ? 1 : -1];

/* units.c */
struct unit_initial_weapon
{
	struct tag_reference weapon;
	long unused[5];
};

typedef char verify_unit_initial_weapon_size[sizeof(struct unit_initial_weapon) == 0x24 ? 1 : -1];

/* unit_definitions.h's struct unit_dialogue_variant, its dialogue tag index
the index of a reference */
struct unit_dialogue_variant_definition
{
	short variant_number;
	short pad;
	long unused;
	struct tag_reference dialogue;
};

typedef char verify_unit_dialogue_variant_definition_size[
	sizeof(struct unit_dialogue_variant_definition) == sizeof(struct unit_dialogue_variant) ? 1 : -1];
typedef char verify_unit_dialogue_variant_definition_dialogue_offset[
	offsetof(struct unit_dialogue_variant_definition, dialogue.index) ==
		offsetof(struct unit_dialogue_variant, dialogue_index) ? 1 : -1];

/* bipeds.c */
struct biped_contact_point
{
	byte unused[32];
	char marker_name[32];
};

typedef char verify_biped_contact_point_size[sizeof(struct biped_contact_point) == 0x40 ? 1 : -1];

/* vehicles.c */
struct vehicle_definition
{
	struct unit_definition unit;
	unsigned long flags;
	short vehicle_type;
	short pad2f6;
	real unknown2f8;
	real unknown2fc;
	real unknown300;
	real unknown304;
	real unknown308;
	real unknown30c;
	real wheel_circumference;
	real unknown314;
	real unknown318;
	short function_modes[4];
	byte unknown324[0xc];
	real unknown330;
	real unknown334;
	byte unused338[8];
	real unknown340;
	real unknown344;
	byte unused348[0x1c];
	real unknown364;
	byte unknown368[0x48];
	struct tag_reference suspension_sound;
	struct tag_reference crash_sound;
	struct tag_reference material_effects;
	struct tag_reference effect;
};

typedef char verify_vehicle_definition_size[sizeof(struct vehicle_definition) == 0x3F0 ? 1 : -1];

/* actors.c */
struct actor_variant_change_colors
{
	real_rgb_color color_lower_bound;
	real_rgb_color color_upper_bound;
	unsigned long unused[2];
};

typedef char verify_actor_variant_change_colors_size[sizeof(struct actor_variant_change_colors) == 0x20 ? 1 : -1];

/* unit_dialogue.c */
struct dialogue_definition
{
	short vocalization_enum_version;
	word pad;
	long unused[3];
	struct tag_reference vocalizations[NUMBER_OF_DIALOGUE_VOCALIZATION_TYPES];
	struct tag_reference unused_vocalizations[47];
};

typedef char verify_dialogue_definition_size[sizeof(struct dialogue_definition) == 0x1010 ? 1 : -1];

/* ---------- globals */

static struct tag_schema_definition const predicted_resource_schema;

/* ---------- checks */

/* an object's type is its group's: the game takes an object's type from
it (object_type_definition_get) and reads the rest of its definition as
that type's */
static boolean object_type_check(
	struct tag_validation *validation,
	void *base,
	short type)
{
	struct object_definition *definition = base;

	if (definition->object.type != type)
	{
		tag_validate_correct(validation, "is of type %d in a tag of type %d's group: %d",
			definition->object.type, type, type);
		definition->object.type = type;
	}

	return TRUE;
}

static boolean biped_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_biped);
}

static boolean vehicle_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_vehicle);
}

static boolean weapon_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_weapon);
}

static boolean equipment_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_equipment);
}

static boolean garbage_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_garbage);
}

static boolean projectile_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_projectile);
}

static boolean scenery_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_scenery);
}

static boolean sound_scenery_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_sound_scenery);
}

static boolean placeholder_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_placeholder);
}

static boolean machine_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_machine);
}

static boolean control_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_control);
}

static boolean light_fixture_type_check(struct tag_validation *validation, void *base)
{
	return object_type_check(validation, base, _object_type_light_fixture);
}

/* how many nodes an object's model has (what its node matrices are), NONE
if it has no model */
static long object_node_count(
	struct tag_validation *validation,
	struct object_definition const *definition)
{
	struct model const *model;

	if (definition->object.model.index == NONE)
		return NONE;
	model = tag_validate_tag_get(validation, definition->object.model.index, MODELS_GROUP_TAG);
	if (!model || !tag_validate_contains(validation, model, sizeof(*model)) || model->nodes.count < 0)
		return 0;

	return MIN(model->nodes.count, MAXIMUM_NODES_PER_MODEL);
}

/* a node index into an object's model's nodes (object_get_node_matrix):
NONE or one of them */
static void object_node_index_check(
	struct tag_validation *validation,
	struct object_definition const *definition,
	short *node_index,
	char const *name)
{
	long node_count = object_node_count(validation, definition);

	if (*node_index != NONE && node_count != NONE && (*node_index < 0 || *node_index >= node_count))
	{
		tag_validate_correct(validation, "has %s %d, past its model's %ld nodes: none", name, *node_index, node_count);
		*node_index = NONE;
	}

	return;
}

/* the pelvis and head nodes the tool found in the biped's model */
static boolean biped_nodes_check(
	struct tag_validation *validation,
	void *base)
{
	struct biped_definition *definition = base;

	object_node_index_check(validation, (struct object_definition const *)definition,
		&definition->biped.runtime_pelvis_node_index, "runtime_pelvis_node_index");
	object_node_index_check(validation, (struct object_definition const *)definition,
		&definition->biped.runtime_head_node_index, "runtime_head_node_index");

	return TRUE;
}

static boolean machine_nodes_check(
	struct tag_validation *validation,
	void *base)
{
	struct machine_definition *definition = base;

	object_node_index_check(validation, (struct object_definition const *)definition,
		&definition->machine.elevator_node_index, "elevator_node_index");

	return TRUE;
}

/* a shotgun's reload reads its first magazine without asking whether it
has one (first_person_weapons.c) */
static boolean weapon_magazines_check(
	struct tag_validation *validation,
	void *base)
{
	struct weapon_definition *definition = base;

	if (definition->weapon.weapon_type == _weapon_type_shotgun && definition->weapon.magazines.count < 1)
	{
		tag_validate_correct(validation, "is a shotgun without a magazine: undefined");
		definition->weapon.weapon_type = _weapon_type_undefined;
	}

	return TRUE;
}

/* ---------- schemas */

/* objects */

static struct tag_schema_field const object_attachment_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct object_attachment_definition, type,
		TAG_SCHEMA_GROUPS('ligh', 'mgs2', 'cont', 'pctl', 'effe', 'lsnd')),
	TAG_SCHEMA_STRING(struct object_attachment_definition, marker_name),
	TAG_SCHEMA_ENUM(struct object_attachment_definition, primary_scale_function_reference,
		NUMBER_OF_OBJECT_FUNCTION_REFERENCES, 0),
	TAG_SCHEMA_ENUM(struct object_attachment_definition, secondary_scale_function_reference,
		NUMBER_OF_OBJECT_FUNCTION_REFERENCES, 0),
	TAG_SCHEMA_ENUM(struct object_attachment_definition, change_color_reference,
		NUMBER_OF_OBJECT_FUNCTION_REFERENCES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const object_attachment_schema =
	TAG_SCHEMA_DEFINITION(object_attachment, struct object_attachment_definition, object_attachment_fields);

static struct tag_schema_field const object_widget_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct object_definition_widget, type,
		TAG_SCHEMA_GROUPS('ant!', 'flag', 'glw!', 'mgs2', 'elec')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const object_widget_schema =
	TAG_SCHEMA_DEFINITION(object_widget, struct object_definition_widget, object_widget_fields);

/* (a function's inputs are none, an incoming function or an outgoing one:
objects.c, OBJECT_INCOMING_FUNCTION_GET_VALUE; its runtime values are the
tool's, which the game reads: not reset) */
static struct tag_schema_field const object_function_fields[] =
{
	TAG_SCHEMA_ENUM(struct object_function_definition, scale_period_by_function_index,
		1 + NUMBER_OF_INCOMING_OBJECT_FUNCTIONS + NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, function_type, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, scale_function_by_function_index,
		1 + NUMBER_OF_INCOMING_OBJECT_FUNCTIONS + NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, wobble_function_type, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, map_result_to_transition_function,
		NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, add_function_index,
		1 + NUMBER_OF_INCOMING_OBJECT_FUNCTIONS + NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, scale_result_by_function_index,
		1 + NUMBER_OF_INCOMING_OBJECT_FUNCTIONS + NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_function_definition, bounds_mode, NUMBER_OF_OBJECT_FUNCTION_BOUNDS_MODES, 0),
	TAG_SCHEMA_BLOCK_INDEX(struct object_function_definition, turn_off_with_function_index, TAG_SCHEMA_ROOT,
		offsetof(struct object_definition, object.functions), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_STRING(struct object_function_definition, usage),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const object_function_schema =
	TAG_SCHEMA_DEFINITION(object_function, struct object_function_definition, object_function_fields);

static struct tag_schema_field const object_change_color_permutation_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const object_change_color_permutation_schema =
	TAG_SCHEMA_DEFINITION(object_change_color_permutation, struct object_change_color_permutation,
		object_change_color_permutation_fields);

/* (a change color is darkened and scaled by none, an incoming function or
an outgoing one: objects.c, OBJECT_INCOMING_FUNCTION_GET_VALUE) */
static struct tag_schema_field const object_change_color_fields[] =
{
	TAG_SCHEMA_ENUM(struct object_change_color_definition, darken_by,
		1 + NUMBER_OF_INCOMING_OBJECT_FUNCTIONS + NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct object_change_color_definition, scaled_by,
		1 + NUMBER_OF_INCOMING_OBJECT_FUNCTIONS + NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, 0),
	TAG_SCHEMA_BLOCK(struct object_change_color_definition, permutations, object_change_color_permutation_schema,
		MAXIMUM_PERMUTATIONS_PER_CHANGE_COLOR),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const object_change_color_schema =
	TAG_SCHEMA_DEFINITION(object_change_color, struct object_change_color_definition, object_change_color_fields);

/* (icon_text_index is an index into the hud globals' messages, which the
hud looks up by itself) */
static struct tag_schema_field const object_fields[] =
{
	TAG_SCHEMA_ENUM(struct _object_definition, type, NUMBER_OF_OBJECT_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct _object_definition, model, TAG_SCHEMA_GROUPS('mode')),
	TAG_SCHEMA_REFERENCE(struct _object_definition, animation_graph, TAG_SCHEMA_GROUPS('antr')),
	TAG_SCHEMA_REFERENCE(struct _object_definition, collision_model, TAG_SCHEMA_GROUPS('coll')),
	TAG_SCHEMA_REFERENCE(struct _object_definition, physics, TAG_SCHEMA_GROUPS('phys')),
	TAG_SCHEMA_REFERENCE(struct _object_definition, modifier_shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_REFERENCE(struct _object_definition, creation_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_ENUM_ARRAY(struct _object_definition, function_modes, NUMBER_OF_OBJECT_FUNCTION_MODES, 0),
	TAG_SCHEMA_BLOCK(struct _object_definition, attachments, object_attachment_schema,
		MAXIMUM_NUMBER_OF_ATTACHMENTS_PER_OBJECT),
	TAG_SCHEMA_BLOCK(struct _object_definition, widgets, object_widget_schema, MAXIMUM_WIDGETS_PER_OBJECT),
	TAG_SCHEMA_BLOCK(struct _object_definition, functions, object_function_schema,
		NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS),
	TAG_SCHEMA_BLOCK(struct _object_definition, change_colors, object_change_color_schema,
		MAXIMUM_CHANGE_COLORS_PER_MODEL),
	TAG_SCHEMA_BLOCK(struct _object_definition, predicted_resources, predicted_resource_schema,
		MAXIMUM_PREDICTED_RESOURCES),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const object_schema =
	TAG_SCHEMA_DEFINITION(object, struct _object_definition, object_fields);

/* (an object of no more than its object part, as a group's root) */
static struct tag_schema_field const object_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct object_definition, object, object_schema),
	TAG_SCHEMA_END
};

/* predicted resources (predicted_resources.c) */

static struct tag_schema_field const predicted_resource_fields[] =
{
	TAG_SCHEMA_ENUM(struct predicted_resource, type, _predicted_resource_sound + 1, 0),
	TAG_SCHEMA_TAG_INDEX(struct predicted_resource, tag_index, TAG_SCHEMA_GROUPS('bitm', 'snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const predicted_resource_schema =
	TAG_SCHEMA_DEFINITION(predicted_resource, struct predicted_resource, predicted_resource_fields);

/* units */

static struct tag_schema_field const unit_camera_track_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct unit_camera_track, track, TAG_SCHEMA_GROUPS('trak')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_camera_track_schema =
	TAG_SCHEMA_DEFINITION(unit_camera_track, struct unit_camera_track, unit_camera_track_fields);

static struct tag_schema_field const unit_camera_fields[] =
{
	TAG_SCHEMA_STRING(struct unit_camera, marker_name),
	TAG_SCHEMA_STRING(struct unit_camera, submerged_marker_name),
	TAG_SCHEMA_BLOCK(struct unit_camera, unit_camera_tracks, unit_camera_track_schema,
		MAXIMUM_CAMERA_TRACKS_PER_UNIT_CAMERA),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_camera_schema =
	TAG_SCHEMA_DEFINITION(unit_camera, struct unit_camera, unit_camera_fields);

static struct tag_schema_field const unit_hud_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct unit_hud_reference, hud, TAG_SCHEMA_GROUPS('unhi')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_hud_schema =
	TAG_SCHEMA_DEFINITION(unit_hud, struct unit_hud_reference, unit_hud_fields);

static struct tag_schema_field const unit_dialogue_variant_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct unit_dialogue_variant_definition, dialogue, TAG_SCHEMA_GROUPS('udlg')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_dialogue_variant_schema =
	TAG_SCHEMA_DEFINITION(unit_dialogue_variant, struct unit_dialogue_variant_definition,
		unit_dialogue_variant_fields);

static struct tag_schema_field const unit_powered_seat_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_powered_seat_schema =
	TAG_SCHEMA_DEFINITION(unit_powered_seat, struct powered_seat_definition, unit_powered_seat_fields);

static struct tag_schema_field const unit_initial_weapon_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct unit_initial_weapon, weapon, TAG_SCHEMA_GROUPS('weap')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_initial_weapon_schema =
	TAG_SCHEMA_DEFINITION(unit_initial_weapon, struct unit_initial_weapon, unit_initial_weapon_fields);

static struct tag_schema_field const unit_seat_fields[] =
{
	TAG_SCHEMA_STRING(struct unit_seat, label),
	TAG_SCHEMA_STRING(struct unit_seat, marker_name),
	TAG_SCHEMA_STRUCT(struct unit_seat, camera, unit_camera_schema),
	TAG_SCHEMA_BLOCK(struct unit_seat, seat_huds, unit_hud_schema, MAXIMUM_HUDS_PER_UNIT),
	TAG_SCHEMA_REFERENCE(struct unit_seat, built_in_actor_reference, TAG_SCHEMA_GROUPS('actv')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_seat_schema =
	TAG_SCHEMA_DEFINITION(unit_seat, struct unit_seat, unit_seat_fields);

/* (blip_type is checked where it is read: motion_sensor.c; the runtime ping
ticks are the tool's, which the game reads) */
static struct tag_schema_field const unit_fields[] =
{
	TAG_SCHEMA_ENUM(struct _unit_definition, default_team, NUMBER_OF_SOLO_CAMPAIGN_TEAMS, 0),
	TAG_SCHEMA_ENUM(struct _unit_definition, constant_sound, NUMBER_OF_AI_SOUND_VOLUMES, 0),
	TAG_SCHEMA_REFERENCE(struct _unit_definition, integrated_light_toggle_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_ENUM_ARRAY(struct _unit_definition, function_modes, NUMBER_OF_UNIT_FUNCTION_MODES, 0),
	TAG_SCHEMA_STRUCT(struct _unit_definition, camera, unit_camera_schema),
	TAG_SCHEMA_REFERENCE(struct _unit_definition, spawned_actor_variant, TAG_SCHEMA_GROUPS('actv')),
	TAG_SCHEMA_REFERENCE(struct _unit_definition, melee_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_BLOCK(struct _unit_definition, huds, unit_hud_schema, MAXIMUM_HUDS_PER_UNIT),
	TAG_SCHEMA_BLOCK(struct _unit_definition, dialogue_variants, unit_dialogue_variant_schema,
		MAXIMUM_DIALOGUE_VARIANTS_PER_UNIT),
	TAG_SCHEMA_ENUM(struct _unit_definition, grenade_type, NUMBER_OF_UNIT_GRENADE_TYPES, 0),
	TAG_SCHEMA_BLOCK(struct _unit_definition, powered_seats, unit_powered_seat_schema,
		MAXIMUM_POWERED_SEATS_PER_UNIT),
	TAG_SCHEMA_BLOCK(struct _unit_definition, initial_weapons, unit_initial_weapon_schema,
		MAXIMUM_WEAPONS_PER_UNIT),
	TAG_SCHEMA_BLOCK(struct _unit_definition, seats, unit_seat_schema, MAXIMUM_SEATS_PER_UNIT),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const unit_schema =
	TAG_SCHEMA_DEFINITION(unit, struct _unit_definition, unit_fields);

static struct tag_schema_field const unit_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct unit_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct unit_definition, unit, unit_schema),
	TAG_SCHEMA_END
};

/* bipeds */

static struct tag_schema_field const biped_contact_point_fields[] =
{
	TAG_SCHEMA_STRING(struct biped_contact_point, marker_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const biped_contact_point_schema =
	TAG_SCHEMA_DEFINITION(biped_contact_point, struct biped_contact_point, biped_contact_point_fields);

/* (its runtime values are the tool's, which the game reads: the node
indices are checked against the model, biped_nodes_check) */
static struct tag_schema_field const biped_fields[] =
{
	TAG_SCHEMA_ENUM_ARRAY(struct _biped_definition, function_modes, NUMBER_OF_BIPED_FUNCTION_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct _biped_definition, melee_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct _biped_definition, material_effects, TAG_SCHEMA_GROUPS('foot')),
	TAG_SCHEMA_BLOCK(struct _biped_definition, contact_points, biped_contact_point_schema,
		MAXIMUM_CONTACT_POINTS_PER_BIPED),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const biped_schema =
	TAG_SCHEMA_DEFINITION(biped, struct _biped_definition, biped_fields);

static struct tag_schema_field const biped_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct biped_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct biped_definition, unit, unit_schema),
	TAG_SCHEMA_STRUCT(struct biped_definition, biped, biped_schema),
	TAG_SCHEMA_CHECK(biped_type_check),
	TAG_SCHEMA_CHECK(biped_nodes_check),
	TAG_SCHEMA_END
};

/* vehicles */

static struct tag_schema_field const vehicle_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct vehicle_definition, unit.object, object_schema),
	TAG_SCHEMA_STRUCT(struct vehicle_definition, unit.unit, unit_schema),
	TAG_SCHEMA_ENUM(struct vehicle_definition, vehicle_type, NUMBER_OF_VEHICLE_TYPES, 0),
	TAG_SCHEMA_ENUM_ARRAY(struct vehicle_definition, function_modes, NUMBER_OF_VEHICLE_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct vehicle_definition, suspension_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct vehicle_definition, crash_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct vehicle_definition, material_effects, TAG_SCHEMA_GROUPS('foot')),
	TAG_SCHEMA_REFERENCE(struct vehicle_definition, effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_CHECK(vehicle_type_check),
	TAG_SCHEMA_END
};

/* items (function_modes are not read: the game reads an item's functions
through its object part; hud_message_index is looked up by the hud, in the
hud globals' item messages) */

static struct tag_schema_field const item_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct _item_definition, material_effects, TAG_SCHEMA_GROUPS('foot')),
	TAG_SCHEMA_REFERENCE(struct _item_definition, collision_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct _item_definition, detonating_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct _item_definition, detonation_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const item_schema =
	TAG_SCHEMA_DEFINITION(item, struct _item_definition, item_fields);

static struct tag_schema_field const item_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct item_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct item_definition, item, item_schema),
	TAG_SCHEMA_END
};

/* (a garbage is an item, and nothing else the game reads) */
static struct tag_schema_field const garbage_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct item_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct item_definition, item, item_schema),
	TAG_SCHEMA_CHECK(garbage_type_check),
	TAG_SCHEMA_END
};

/* weapons */

static struct tag_schema_field const weapon_ammunition_object_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct weapon_ammunition_object, object, TAG_SCHEMA_GROUPS('eqip')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_ammunition_object_schema =
	TAG_SCHEMA_DEFINITION(weapon_ammunition_object, struct weapon_ammunition_object, weapon_ammunition_object_fields);

static struct tag_schema_field const weapon_magazine_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct weapon_magazine_definition, reloading_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct weapon_magazine_definition, chambering_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_BLOCK(struct weapon_magazine_definition, ammunition_objects, weapon_ammunition_object_schema,
		MAXIMUM_AMMUNITION_OBJECTS_PER_MAGAZINE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_magazine_schema =
	TAG_SCHEMA_DEFINITION(weapon_magazine, struct weapon_magazine_definition, weapon_magazine_fields);

struct weapon_firing_effect
{
	short shot_count_lower_bound;
	short shot_count_upper_bound;
	long unused[8];
	struct tag_reference firing_effect;
	struct tag_reference misfire_effect;
	struct tag_reference empty_effect;
	struct tag_reference firing_damage;
	struct tag_reference misfire_damage;
	struct tag_reference empty_damage;
};

typedef char verify_weapon_firing_effect_size[sizeof(struct weapon_firing_effect) == 0x84 ? 1 : -1];

static struct tag_schema_field const weapon_firing_effect_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct weapon_firing_effect, firing_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct weapon_firing_effect, misfire_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct weapon_firing_effect, empty_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct weapon_firing_effect, firing_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct weapon_firing_effect, misfire_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct weapon_firing_effect, empty_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_firing_effect_schema =
	TAG_SCHEMA_DEFINITION(weapon_firing_effect, struct weapon_firing_effect, weapon_firing_effect_fields);

/* (its runtime values are the tool's, which the game reads) */
static struct tag_schema_field const weapon_trigger_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct weapon_trigger_definition, magazine_index, TAG_SCHEMA_ROOT,
		offsetof(struct weapon_definition, weapon.magazines), FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct weapon_trigger_definition, firing_noise, NUMBER_OF_AI_SOUND_VOLUMES, 0),
	TAG_SCHEMA_ENUM(struct weapon_trigger_definition, overcharged_action, NUMBER_OF_TRIGGER_OVERCHARGED_ACTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct weapon_trigger_definition, charging_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_ENUM(struct weapon_trigger_definition, projectile_distribution_function,
		NUMBER_OF_PROJECTILE_DISTRIBUTION_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct weapon_trigger_definition, projectile, TAG_SCHEMA_GROUPS('obje')),
	TAG_SCHEMA_BLOCK(struct weapon_trigger_definition, firing_effects, weapon_firing_effect_schema,
		MAXIMUM_FIRING_EFFECTS_PER_TRIGGER),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_trigger_schema =
	TAG_SCHEMA_DEFINITION(weapon_trigger, struct weapon_trigger_definition, weapon_trigger_fields);

/* (unused_block is not read) */
static struct tag_schema_field const weapon_fields[] =
{
	TAG_SCHEMA_STRING(struct _weapon_definition, label),
	TAG_SCHEMA_ENUM(struct _weapon_definition, secondary_trigger_mode, NUMBER_OF_WEAPON_SECONDARY_TRIGGER_MODES, 0),
	TAG_SCHEMA_ENUM_ARRAY(struct _weapon_definition, function_modes, NUMBER_OF_WEAPON_FUNCTION_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, ready_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, overheated_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, detonation_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, melee_attack_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, melee_attack_response, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, ai_firing_parameters, TAG_SCHEMA_GROUPS('actv')),
	TAG_SCHEMA_ENUM(struct _weapon_definition, movement_penalty_mode, NUMBER_OF_WEAPON_MOVEMENT_PENALTY_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, integrated_light_on_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, integrated_light_off_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, interface_definition.first_person_model, TAG_SCHEMA_GROUPS('mode')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, interface_definition.first_person_animations,
		TAG_SCHEMA_GROUPS('antr')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, interface_definition.hud_interface, TAG_SCHEMA_GROUPS('wphi')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, pickup_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, zoom_in_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct _weapon_definition, zoom_out_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_ENUM(struct _weapon_definition, weapon_type, NUMBER_OF_WEAPON_TYPES, 0),
	TAG_SCHEMA_BLOCK(struct _weapon_definition, predicted_resources, predicted_resource_schema,
		MAXIMUM_PREDICTED_RESOURCES),
	TAG_SCHEMA_BLOCK(struct _weapon_definition, magazines, weapon_magazine_schema,
		MAXIMUM_NUMBER_OF_MAGAZINES_PER_WEAPON),
	TAG_SCHEMA_BLOCK(struct _weapon_definition, triggers, weapon_trigger_schema, MAXIMUM_NUMBER_OF_TRIGGERS_PER_WEAPON),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weapon_schema =
	TAG_SCHEMA_DEFINITION(weapon, struct _weapon_definition, weapon_fields);

static struct tag_schema_field const weapon_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct weapon_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct weapon_definition, item, item_schema),
	TAG_SCHEMA_STRUCT(struct weapon_definition, weapon, weapon_schema),
	TAG_SCHEMA_CHECK(weapon_type_check),
	TAG_SCHEMA_CHECK(weapon_magazines_check),
	TAG_SCHEMA_END
};

/* equipment */

static struct tag_schema_field const equipment_fields[] =
{
	TAG_SCHEMA_ENUM(struct _equipment_definition, powerup_type, NUMBER_OF_EQUIPMENT_POWERUP_TYPES, 0),
	TAG_SCHEMA_ENUM(struct _equipment_definition, grenade_type, NUMBER_OF_UNIT_GRENADE_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct _equipment_definition, pickup_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const equipment_schema =
	TAG_SCHEMA_DEFINITION(equipment, struct _equipment_definition, equipment_fields);

static struct tag_schema_field const equipment_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct equipment_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct equipment_definition, item, item_schema),
	TAG_SCHEMA_STRUCT(struct equipment_definition, equipment, equipment_schema),
	TAG_SCHEMA_CHECK(equipment_type_check),
	TAG_SCHEMA_END
};

/* projectiles */

static struct tag_schema_field const projectile_material_response_fields[] =
{
	TAG_SCHEMA_ENUM(struct projectile_material_response_definition, default_response,
		NUMBER_OF_PROJECTILE_MATERIAL_RESPONSES, 0),
	TAG_SCHEMA_REFERENCE(struct projectile_material_response_definition, default_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_ENUM(struct projectile_material_response_definition, potential_response,
		NUMBER_OF_PROJECTILE_MATERIAL_RESPONSES, 0),
	TAG_SCHEMA_REFERENCE(struct projectile_material_response_definition, potential_effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_ENUM(struct projectile_material_response_definition, scale_effects_by,
		NUMBER_OF_PROJECTILE_MATERIAL_EFFECT_SCALES, 0),
	TAG_SCHEMA_REFERENCE(struct projectile_material_response_definition, detonation_effect,
		TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const projectile_material_response_schema =
	TAG_SCHEMA_DEFINITION(projectile_material_response, struct projectile_material_response_definition,
		projectile_material_response_fields);

/* (a material response is looked up by material type, past the block's end
the default one: projectiles.c) */
static struct tag_schema_field const projectile_fields[] =
{
	TAG_SCHEMA_ENUM(struct _projectile_definition, detonation_timer_starts,
		NUMBER_OF_PROJECTILE_DETONATION_TIMER_STARTS, 0),
	TAG_SCHEMA_ENUM(struct _projectile_definition, impact_noise, NUMBER_OF_AI_SOUND_VOLUMES, 0),
	TAG_SCHEMA_ENUM_ARRAY(struct _projectile_definition, function_inputs, NUMBER_OF_PROJECTILE_EXPORT_FUNCTION_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct _projectile_definition, super_detonation, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct _projectile_definition, effect, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_ENUM(struct _projectile_definition, detonation_noise, NUMBER_OF_AI_SOUND_VOLUMES, 0),
	TAG_SCHEMA_REFERENCE(struct _projectile_definition, detonation_started, TAG_SCHEMA_GROUPS('effe')),
	TAG_SCHEMA_REFERENCE(struct _projectile_definition, flyby_sound, TAG_SCHEMA_GROUPS('snd!')),
	TAG_SCHEMA_REFERENCE(struct _projectile_definition, attached_detonation_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_REFERENCE(struct _projectile_definition, impact_damage, TAG_SCHEMA_GROUPS('jpt!')),
	TAG_SCHEMA_BLOCK(struct _projectile_definition, material_responses, projectile_material_response_schema,
		NUMBER_OF_MATERIAL_TYPES),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const projectile_schema =
	TAG_SCHEMA_DEFINITION(projectile, struct _projectile_definition, projectile_fields);

static struct tag_schema_field const projectile_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct projectile_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct projectile_definition, projectile, projectile_schema),
	TAG_SCHEMA_CHECK(projectile_type_check),
	TAG_SCHEMA_END
};

/* scenery, sound scenery, placeholders: an object part and padding */

static struct tag_schema_field const scenery_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct object_definition, object, object_schema),
	TAG_SCHEMA_CHECK(scenery_type_check),
	TAG_SCHEMA_END
};

static struct tag_schema_field const sound_scenery_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct object_definition, object, object_schema),
	TAG_SCHEMA_CHECK(sound_scenery_type_check),
	TAG_SCHEMA_END
};

static struct tag_schema_field const placeholder_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct object_definition, object, object_schema),
	TAG_SCHEMA_CHECK(placeholder_type_check),
	TAG_SCHEMA_END
};

/* devices (the runtime values are the tool's, which the game reads) */

static struct tag_schema_field const device_fields[] =
{
	TAG_SCHEMA_ENUM_ARRAY(struct _device_definition, function_modes, NUMBER_OF_DEVICE_FUNCTION_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct _device_definition, positive_start_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _device_definition, negative_start_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _device_definition, positive_stop_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _device_definition, negative_stop_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _device_definition, depowered_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _device_definition, repowered_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _device_definition, delay_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const device_schema =
	TAG_SCHEMA_DEFINITION(device, struct _device_definition, device_fields);

static struct tag_schema_field const device_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct device_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct device_definition, device, device_schema),
	TAG_SCHEMA_END
};

/* (a light fixture is a device and padding) */
static struct tag_schema_field const light_fixture_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct device_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct device_definition, device, device_schema),
	TAG_SCHEMA_CHECK(light_fixture_type_check),
	TAG_SCHEMA_END
};

/* (collision_response is not read; the elevator node is checked against
the model, machine_nodes_check) */
static struct tag_schema_field const machine_fields[] =
{
	TAG_SCHEMA_ENUM(struct _machine_definition, type, NUMBER_OF_MACHINE_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const machine_schema =
	TAG_SCHEMA_DEFINITION(machine, struct _machine_definition, machine_fields);

static struct tag_schema_field const machine_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct machine_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct machine_definition, device, device_schema),
	TAG_SCHEMA_STRUCT(struct machine_definition, machine, machine_schema),
	TAG_SCHEMA_CHECK(machine_type_check),
	TAG_SCHEMA_CHECK(machine_nodes_check),
	TAG_SCHEMA_END
};

static struct tag_schema_field const control_fields[] =
{
	TAG_SCHEMA_ENUM(struct _control_definition, type, NUMBER_OF_CONTROL_TYPES, 0),
	TAG_SCHEMA_ENUM(struct _control_definition, triggers_when, NUMBER_OF_CONTROL_TRIGGERS, 0),
	TAG_SCHEMA_REFERENCE(struct _control_definition, on_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _control_definition, off_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct _control_definition, denied_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_END
};

/* (it ends where the tag does: before reserved88, as control_definition_schema) */
static struct tag_schema_definition const control_schema =
	{ "control", offsetof(struct _control_definition, reserved88), control_fields };

static struct tag_schema_field const control_definition_fields[] =
{
	TAG_SCHEMA_STRUCT(struct control_definition, object, object_schema),
	TAG_SCHEMA_STRUCT(struct control_definition, device, device_schema),
	TAG_SCHEMA_STRUCT(struct control_definition, control, control_schema),
	TAG_SCHEMA_CHECK(control_type_check),
	TAG_SCHEMA_END
};

/* actors (the unused references are not read; the other enums are only
compared) */

static struct tag_schema_field const actor_definition_fields[] =
{
	TAG_SCHEMA_ENUM(struct actor_definition, type, NUMBER_OF_ACTOR_TYPES, 0),
	TAG_SCHEMA_END
};

/* actor variants */

static struct tag_schema_field const actor_variant_change_colors_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const actor_variant_change_colors_schema =
	TAG_SCHEMA_DEFINITION(actor_variant_change_colors, struct actor_variant_change_colors,
		actor_variant_change_colors_fields);

/* (grenade_combat's grenade_type, trajectory_type and stimulus_type are not
checked: the multiplayer maps' 'vehicles\warthog\warthog gunner', read
there only as the warthog gun's ai_firing_parameters, has an older
layout's 'proj' reference where they are) */
static struct tag_schema_field const actor_variant_definition_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct actor_variant_definition, actor_reference, TAG_SCHEMA_GROUPS('actr')),
	TAG_SCHEMA_REFERENCE(struct actor_variant_definition, unit_reference, TAG_SCHEMA_GROUPS('unit')),
	TAG_SCHEMA_REFERENCE(struct actor_variant_definition, major_upgrade_reference, TAG_SCHEMA_GROUPS('actv')),
	TAG_SCHEMA_REFERENCE(struct actor_variant_definition, ranged_combat.reference, TAG_SCHEMA_GROUPS('weap')),
	TAG_SCHEMA_REFERENCE(struct actor_variant_definition, items.equipment_reference, TAG_SCHEMA_GROUPS('eqip')),
	TAG_SCHEMA_BLOCK(struct actor_variant_definition, change_colors, actor_variant_change_colors_schema,
		NUMBER_OF_OBJECT_CHANGE_COLORS),
	TAG_SCHEMA_END
};

/* dialogue */

/* the vocalizations from first to last (an array of references, as
TAG_SCHEMA_REFERENCE_ARRAY) */
#define DIALOGUE_VOCALIZATIONS(first, last) \
	{ _tag_schema_reference, sizeof(struct tag_reference), (last) - (first) + 1, FLAG(_tag_schema_none_bit), \
		TAG_SCHEMA_OFFSET(struct dialogue_definition, vocalizations[first], struct tag_reference), 0, 0, 0, \
		TAG_SCHEMA_GROUPS('snd!'), NULL, "vocalizations" }

/* the vocalizations that are the tag's padding between its groups of
sounds (all zeros in every retail dialogue): the game reads them as any
other vocalization, so they are reset to none */
static short const dialogue_padding_vocalizations[][2] =
{
	{ 3, 5 }, { 20, 20 }, { 25, 28 }, { 46, 48 }, { 77, 79 }, { 93, 95 }, { 106, 107 }, { 121, 122 },
	{ 144, 147 }, { 171, 176 }, { 184, 187 }, { 193, 196 }, { 205, NUMBER_OF_DIALOGUE_VOCALIZATION_TYPES - 1 },
};

static boolean dialogue_padding_check(
	struct tag_validation *validation,
	void *base)
{
	struct dialogue_definition *definition = base;
	short range;

	(void)validation;
	for (range = 0; range < (short)NUMBEROF(dialogue_padding_vocalizations); range++)
	{
		short index;

		for (index = dialogue_padding_vocalizations[range][0];
			index <= dialogue_padding_vocalizations[range][1];
			index++)
		{
			definition->vocalizations[index].index = NONE;
		}
	}

	return TRUE;
}

static struct tag_schema_field const dialogue_definition_fields[] =
{
	DIALOGUE_VOCALIZATIONS(0, 2),
	DIALOGUE_VOCALIZATIONS(6, 19),
	DIALOGUE_VOCALIZATIONS(21, 24),
	DIALOGUE_VOCALIZATIONS(29, 45),
	DIALOGUE_VOCALIZATIONS(49, 76),
	DIALOGUE_VOCALIZATIONS(80, 92),
	DIALOGUE_VOCALIZATIONS(96, 105),
	DIALOGUE_VOCALIZATIONS(108, 120),
	DIALOGUE_VOCALIZATIONS(123, 143),
	DIALOGUE_VOCALIZATIONS(148, 170),
	DIALOGUE_VOCALIZATIONS(177, 183),
	DIALOGUE_VOCALIZATIONS(188, 192),
	DIALOGUE_VOCALIZATIONS(197, 204),
	TAG_SCHEMA_CHECK(dialogue_padding_check),
	TAG_SCHEMA_END
};

/* item collections */

static struct tag_schema_field const item_permutation_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct item_permutation_definition, item, TAG_SCHEMA_GROUPS('item')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const item_permutation_schema =
	TAG_SCHEMA_DEFINITION(item_permutation, struct item_permutation_definition, item_permutation_fields);

static struct tag_schema_field const item_collection_definition_fields[] =
{
	TAG_SCHEMA_BLOCK(struct item_collection_definition, permutations, item_permutation_schema,
		MAXIMUM_PERMUTATIONS_PER_ITEM_COLLECTION),
	TAG_SCHEMA_END
};

/* the groups' roots */

static struct tag_schema_definition const object_definition_schema =
	TAG_SCHEMA_DEFINITION(object_definition, struct object_definition, object_definition_fields);
static struct tag_schema_definition const unit_definition_schema =
	TAG_SCHEMA_DEFINITION(unit_definition, struct unit_definition, unit_definition_fields);
static struct tag_schema_definition const biped_definition_schema =
	TAG_SCHEMA_DEFINITION(biped_definition, struct biped_definition, biped_definition_fields);
static struct tag_schema_definition const vehicle_definition_schema =
	TAG_SCHEMA_DEFINITION(vehicle_definition, struct vehicle_definition, vehicle_definition_fields);
static struct tag_schema_definition const item_definition_schema =
	TAG_SCHEMA_DEFINITION(item_definition, struct item_definition, item_definition_fields);
static struct tag_schema_definition const weapon_definition_schema =
	TAG_SCHEMA_DEFINITION(weapon_definition, struct weapon_definition, weapon_definition_fields);
static struct tag_schema_definition const equipment_definition_schema =
	TAG_SCHEMA_DEFINITION(equipment_definition, struct equipment_definition, equipment_definition_fields);
static struct tag_schema_definition const garbage_definition_schema =
	TAG_SCHEMA_DEFINITION(garbage_definition, struct item_definition, garbage_definition_fields);
static struct tag_schema_definition const projectile_definition_schema =
	TAG_SCHEMA_DEFINITION(projectile_definition, struct projectile_definition, projectile_definition_fields);
static struct tag_schema_definition const scenery_definition_schema =
	TAG_SCHEMA_DEFINITION(scenery_definition, struct object_definition, scenery_definition_fields);
static struct tag_schema_definition const sound_scenery_definition_schema =
	TAG_SCHEMA_DEFINITION(sound_scenery_definition, struct object_definition, sound_scenery_definition_fields);
static struct tag_schema_definition const placeholder_definition_schema =
	TAG_SCHEMA_DEFINITION(placeholder_definition, struct object_definition, placeholder_definition_fields);
static struct tag_schema_definition const device_definition_schema =
	TAG_SCHEMA_DEFINITION(device_definition, struct device_definition, device_definition_fields);
static struct tag_schema_definition const machine_definition_schema =
	TAG_SCHEMA_DEFINITION(machine_definition, struct machine_definition, machine_definition_fields);
/* (a control tag ends at its denied effect: control_definition's reserved
bytes after it are not in the tag) */
static struct tag_schema_definition const control_definition_schema =
	{ "control_definition", offsetof(struct control_definition, control.reserved88), control_definition_fields };
static struct tag_schema_definition const light_fixture_definition_schema =
	TAG_SCHEMA_DEFINITION(light_fixture_definition, struct device_definition, light_fixture_definition_fields);
static struct tag_schema_definition const actor_definition_schema =
	TAG_SCHEMA_DEFINITION(actor_definition, struct actor_definition, actor_definition_fields);
static struct tag_schema_definition const actor_variant_definition_schema =
	TAG_SCHEMA_DEFINITION(actor_variant_definition, struct actor_variant_definition, actor_variant_definition_fields);
static struct tag_schema_definition const dialogue_definition_schema =
	TAG_SCHEMA_DEFINITION(dialogue_definition, struct dialogue_definition, dialogue_definition_fields);
static struct tag_schema_definition const item_collection_definition_schema =
	TAG_SCHEMA_DEFINITION(item_collection_definition, struct item_collection_definition,
		item_collection_definition_fields);

struct tag_schema_group const tag_schema_object_groups[] =
{
	{ 'obje', { NONE, NONE }, &object_definition_schema },
	{ 'unit', { 'obje', NONE }, &unit_definition_schema },
	{ 'bipd', { 'unit', 'obje' }, &biped_definition_schema },
	{ 'vehi', { 'unit', 'obje' }, &vehicle_definition_schema },
	{ 'item', { 'obje', NONE }, &item_definition_schema },
	{ 'weap', { 'item', 'obje' }, &weapon_definition_schema },
	{ 'eqip', { 'item', 'obje' }, &equipment_definition_schema },
	{ 'garb', { 'item', 'obje' }, &garbage_definition_schema },
	{ 'proj', { 'obje', NONE }, &projectile_definition_schema },
	{ 'scen', { 'obje', NONE }, &scenery_definition_schema },
	{ 'ssce', { 'obje', NONE }, &sound_scenery_definition_schema },
	{ 'plac', { 'obje', NONE }, &placeholder_definition_schema },
	{ 'devi', { 'obje', NONE }, &device_definition_schema },
	{ 'mach', { 'devi', 'obje' }, &machine_definition_schema },
	{ 'ctrl', { 'devi', 'obje' }, &control_definition_schema },
	{ 'lifi', { 'devi', 'obje' }, &light_fixture_definition_schema },
	{ 'actr', { NONE, NONE }, &actor_definition_schema },
	{ 'actv', { NONE, NONE }, &actor_variant_definition_schema },
	{ 'udlg', { NONE, NONE }, &dialogue_definition_schema },
	{ 'itmc', { NONE, NONE }, &item_collection_definition_schema },
	{ 0 }
};
