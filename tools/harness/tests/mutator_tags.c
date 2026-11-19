/*
The mutators' tag edits (port/linux/game/mutator_tags.c) in a fake map's tags,
made from mutator.toml by port/linux/src/port_mutators.c: the built-in
accurized MA5B's and revolved plasma rifle's every value, undone; a time's
step a tick worked out with it; tags, elements and tags named that a
map lacks passed over; edits refused for a field, value or path this build
does not take; and edits' bytes that are not edits' refused whole.
*/

#include "harness.h"
#include <math.h>
#include <stddef.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
/* (the library is C11: the harness builds C99) */
#define static_assert _Static_assert
#include "tomlc17.inc"
#pragma clang diagnostic pop

typedef unsigned short word;
#define csmemcpy memcpy
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define NUMBEROF(array) (sizeof(array) / sizeof(array[0]))
#define _pi ((real)3.14159265358979323846f)
enum { TICKS_PER_SECOND = 30 };
enum { _error_log };
static int logged_errors;
#define error(kind, ...) ((void)(kind), logged_errors++)
#define console_printf(clear, ...) ((void)(clear))
#include "enums.inc"
enum
{
	WEAPON_DEFINITION_TAG = 'weap',
	PROJECTILE_DEFINITION_TAG = 'proj',
	SOUND_DEFINITION_TAG = 'snd!',
	EFFECT_DEFINITION_TAG = 'effe',
	DAMAGE_EFFECT_DEFINITION_TAG = 'jpt!',
	HUD_GROUP_TAG = 'wphi',
	DECAL_GROUP_TAG = 'deca',
	BITMAP_GROUP_TAG = 'bitm',
	MODELS_GROUP_TAG = 'mode',
};
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
typedef struct { real red, green, blue; } real_rgb_color;
struct real_orientation;

/* the tags' structures: the fields mutator_tags.c sets, by the game's names */
struct tag_reference { unsigned long group_tag; char *name; long name_length; long index; };
struct tag_block { long count; void *address; void *definition; };
struct weapon_magazine_definition
{
	unsigned long flags;
	short rounds_recharged_per_second;
	short rounds_total_initial;
	short rounds_total_maximum;
	short rounds_loaded_maximum;
	short rounds_reloaded;
	short pad;
	real chamber_time;
};
struct weapon_trigger_definition
{
	unsigned long flags;
	real initial_rate_of_fire;
	real final_rate_of_fire;
	real rate_of_fire_acceleration_time;
	real rate_of_fire_deceleration_time;
	real error_acceleration_time;
	real error_deceleration_time;
	real projectile_error_angle_lower_bound;
	real projectile_error_angle_upper_bound;
	real heat_generated_per_round;
	real age_generated_per_round;
	short rounds_between_tracers;
	short pad;
	real runtime_rate_of_fire_acceleration_time;
	real runtime_rate_of_fire_deceleration_time;
	real runtime_error_acceleration_time;
	real runtime_error_deceleration_time;
	struct tag_reference projectile;
};
struct weapon_definition
{
	struct
	{
		long flags;
		real heat_recovery_threshold;
		real heat_loss_per_second;
		short pad;
		short zoom_level_count;
		real zoom_magnification_minimum;
		real zoom_magnification_maximum;
		struct tag_reference zoom_in_sound;
		struct tag_reference zoom_out_sound;
		real age_heat_recovery_penalty;
		real age_rate_of_fire_penalty;
		real age_misfire_start;
		real age_misfire_chance;
		real active_camo_ding;
		real active_camo_regrowth_rate;
		struct tag_block magazines;
		struct tag_block triggers;
	} weapon;
};
struct projectile_material_response_definition
{
	word flags;
	short default_response;
	struct tag_reference default_effect;
	short potential_response;
	word potential_flags;
	real potential_skip_fraction;
	real potential_angle_lower_bound;
	real potential_angle_upper_bound;
	struct tag_reference potential_effect;
};
struct projectile_definition
{
	struct { struct tag_reference model; } object;
	struct
	{
		real maximum_range;
		real air_gravity_scale;
		real water_gravity_scale;
		real air_damage_range_lower_bound;
		real air_damage_range_upper_bound;
		real water_damage_range_lower_bound;
		real water_damage_range_upper_bound;
		real initial_velocity;
		real final_velocity;
		struct tag_block material_responses;
	} projectile;
};
struct damage_effect_definition
{
	real falloff_radius;
	struct
	{
		real damage_minimum;
		real damage_lower_bound;
		real damage_upper_bound;
		real active_camouflage_damage;
		real material_modifiers[40];
	} damage;
};
struct weapon_hud_interface_definition
{
	struct tag_reference parent_hud;
	struct tag_block crosshairs;
	unsigned long valid_crosshair_types_flags;
	struct tag_block screen_effects;
};
struct hud_screen_effect_definition { word mask_flags; word pad; struct tag_reference mask_fullscreen; };

/* the map: its tags, by index (with a salt, as the game's) */
#define CROSSHAIR_BYTES 0x68
enum
{
	_tag_rifle, _tag_bullet, _tag_rifle_hud, _tag_pistol_hud, _tag_zoom_in, _tag_zoom_out, _tag_stone, _tag_hollow,
	_tag_thin, _tag_thick, _tag_ice, _tag_plasma, _tag_bolt, _tag_bolt_damage, _tag_plasma_hud, _tag_splat, _tag_glow,
	_tag_teleport, _tag_splotch, _tag_sign, _tag_ball, NUMBER_OF_TAGS
};
#define TAG_INDEX(index) ((long)(0xE1740000UL | (unsigned long)(index)))
static struct { unsigned long group_tag; char name[80]; void *data; int present; } tags[NUMBER_OF_TAGS];
static struct weapon_definition rifle;
static struct weapon_magazine_definition magazines[2];
static struct weapon_trigger_definition triggers[2];
static struct projectile_definition bullet;
static struct projectile_material_response_definition responses[NUMBER_OF_MATERIAL_TYPES + 1];
static struct weapon_hud_interface_definition rifle_hud, pistol_hud;
static byte rifle_crosshairs[1][CROSSHAIR_BYTES], pistol_crosshairs[3][CROSSHAIR_BYTES];
static struct hud_screen_effect_definition pistol_effects[1];
static int other_tag_data;
/* the plasma rifle's */
static struct weapon_definition plasma;
static struct weapon_trigger_definition plasma_triggers[1];
static struct projectile_definition bolt;
static struct projectile_material_response_definition bolt_responses[NUMBER_OF_MATERIAL_TYPES];
static struct damage_effect_definition bolt_damage;
static struct weapon_hud_interface_definition plasma_hud;
static byte plasma_crosshairs[2][CROSSHAIR_BYTES];

/* cache_files.h's iterator over the map's tags of a group */
struct tag_iterator { unsigned long group_tag; int next; };
static void tag_iterator_new(struct tag_iterator *iterator, long group_tag)
{
	iterator->group_tag = (unsigned long)group_tag;
	iterator->next = 0;
}
static long tag_iterator_next(struct tag_iterator *iterator)
{
	while (iterator->next < NUMBER_OF_TAGS)
	{
		int index = iterator->next++;

		if (tags[index].present && tags[index].group_tag == iterator->group_tag)
			return TAG_INDEX(index);
	}
	return NONE;
}

static long tag_loaded(long group_tag, char const *name)
{
	int index;

	for (index = 0; index < NUMBER_OF_TAGS; index++)
	{
		if (tags[index].present && tags[index].group_tag == (unsigned long)group_tag && !strcasecmp(tags[index].name, name))
			return TAG_INDEX(index);
	}
	return NONE;
}
static void *tag_get(long group_tag, long tag_index)
{
	long index = DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index);

	CHECK(index < NUMBER_OF_TAGS && tags[index].present && tags[index].group_tag == (unsigned long)group_tag &&
		(unsigned long)tag_index == (unsigned long)TAG_INDEX(index), "tag_get of a tag the map has not");
	return tags[index].data;
}
static char *tag_get_name(long tag_index)
{
	return tags[DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_index)].name;
}

/* the platform layer's, as test_port_mutators.py fakes it (no mutators folder) */
struct fake_folder { char const *name; char const *text; unsigned long size; };
static struct fake_folder folders[32];
/* (files beside them, as test_port_mutators.py's stubs read them: none) */
struct fake_file { char const *path; char const *bytes; unsigned long size; };
static struct fake_file files[1];
static int file_count;
static int folder_count, listed, reads, logged;
static char setting[300], written[300];

/* the stock maps (stock_tags.c): the zoom sound, when one has it, and what was asked */
static int stock_has_zoom, stock_finds, stock_finishes;
static long stock_tags_find(long group_tag, char const *name)
{
	stock_finds++;
	if (stock_has_zoom && group_tag == (long)tags[_tag_zoom_in].group_tag && !strcasecmp(tags[_tag_zoom_in].name, name))
	{
		tags[_tag_zoom_in].present = 1;
		return TAG_INDEX(_tag_zoom_in);
	}
	return NONE;
}
static void stock_tags_finish(void)
{
	stock_finishes++;
}

/* object_pose.c's (test_mutators.py tests it): the groups a pose names, and
the poses made */
#include "object_pose_header.inc"
static struct { long group_tag; char tag[80]; char node[40]; real scale; real_vector3d offset; } poses[8];
static int pose_count, pose_clears;
static long object_pose_group_tag(char const *name)
{
	return !strcmp(name, "biped") ? 'bipd' : !strcmp(name, "unit") ? 'unit' : NONE;
}
static void object_pose_clear(void)
{
	pose_clears++;
	pose_count = 0;
}
static boolean object_pose_add(long group_tag, char const *tag_name, char const *node_name, real scale,
	real_vector3d const *offset, char *reason, unsigned long size)
{
	/* (no model of the map has a tail) */
	if (!strcmp(node_name, "tail"))
	{
		snprintf(reason, size, "no model has a node tail");
		return FALSE;
	}
	if (pose_count < 8)
	{
		poses[pose_count].group_tag = group_tag;
		snprintf(poses[pose_count].tag, sizeof(poses[pose_count].tag), "%s", tag_name);
		snprintf(poses[pose_count].node, sizeof(poses[pose_count].node), "%s", node_name);
		poses[pose_count].scale = scale;
		poses[pose_count++].offset = *offset;
	}
	return TRUE;
}

/* explosion_decals.c's: the decal every explosion leaves */
static long explosion_decal_index = NONE;
static real explosion_decal_radius;
static int explosion_clears;
static void explosion_decal_clear(void)
{
	explosion_clears++;
	explosion_decal_index = NONE;
}
static void explosion_decal_add(long decal_index, real decal_radius)
{
	explosion_decal_index = decal_index;
	explosion_decal_radius = decal_radius;
}

/* object_death.c's: the deaths made */
static struct { long group_tag; long effect_index; boolean vanish; } deaths[4];
static int death_count, death_clears;
static void object_death_clear(void)
{
	death_clears++;
	death_count = 0;
}
static boolean object_death_add(long group_tag, long effect_index, boolean vanish, char *reason, unsigned long size)
{
	(void)reason;
	(void)size;
	if (death_count < 4)
	{
		deaths[death_count].group_tag = group_tag;
		deaths[death_count].effect_index = effect_index;
		deaths[death_count++].vanish = vanish;
	}
	return TRUE;
}

#include "mutators_header.inc"
#include "mutator_tags.inc"
#include "port_mutators_source.inc"

/* the grunt's blood splat and a painted sign (mutator_tags.c's own layout
of a decal's) */
static struct mutator_decal_definition splat, sign;

static void tag(int index, unsigned long group_tag, char const *name, void *data)
{
	tags[index].group_tag = group_tag;
	snprintf(tags[index].name, sizeof(tags[index].name), "%s", name);
	tags[index].data = data;
	tags[index].present = 1;
}

static void folder(char const *name, char const *text)
{
	folders[folder_count].name = name;
	folders[folder_count].text = text;
	folders[folder_count++].size = text ? (unsigned long)strlen(text) : 0;
}

/* the stock map's tags, as the edits find them */
static void world(void)
{
	int index;

	memset(tags, 0, sizeof(tags));
	memset(&rifle, 0, sizeof(rifle));
	memset(magazines, 0x5A, sizeof(magazines));
	magazines[0].rounds_total_initial = 240;
	magazines[0].rounds_total_maximum = 600;
	magazines[0].rounds_loaded_maximum = 60;
	memset(triggers, 0x5A, sizeof(triggers));
	triggers[0].projectile_error_angle_lower_bound = 0.0349066f;
	triggers[0].projectile_error_angle_upper_bound = 0.113446f;
	triggers[0].rounds_between_tracers = 3;
	/* (the rifle fires its bullet; the plasma rifle, here, nothing) */
	triggers[0].projectile.index = TAG_INDEX(_tag_bullet);
	triggers[1].projectile.index = NONE;
	rifle.weapon.zoom_magnification_minimum = 1.f;
	rifle.weapon.zoom_in_sound.index = NONE;
	rifle.weapon.zoom_out_sound.index = NONE;
	rifle.weapon.magazines.count = 1;
	rifle.weapon.magazines.address = magazines;
	rifle.weapon.triggers.count = 1;
	rifle.weapon.triggers.address = triggers;
	memset(&bullet, 0, sizeof(bullet));
	memset(responses, 0, sizeof(responses));
	for (index = 0; index < NUMBER_OF_MATERIAL_TYPES + 1; index++)
	{
		responses[index].default_response = _projectile_material_response_detonate;
		responses[index].default_effect.index = NONE;
		responses[index].potential_effect.index = NONE;
	}
	responses[_material_grunt].default_response = _projectile_material_response_overpenetrate;
	bullet.object.model.index = NONE;
	bullet.projectile.maximum_range = 40.f;
	bullet.projectile.air_gravity_scale = 1.f;
	bullet.projectile.water_gravity_scale = 1.f;
	bullet.projectile.material_responses.count = NUMBER_OF_MATERIAL_TYPES;
	bullet.projectile.material_responses.address = responses;
	memset(&rifle_hud, 0, sizeof(rifle_hud));
	memset(&pistol_hud, 0, sizeof(pistol_hud));
	memset(rifle_crosshairs, 'r', sizeof(rifle_crosshairs));
	for (index = 0; index < 3; index++)
		memset(pistol_crosshairs[index], '0' + index, CROSSHAIR_BYTES);
	memset(pistol_effects, 0, sizeof(pistol_effects));
	pistol_effects[0].mask_flags = 1;
	rifle_hud.valid_crosshair_types_flags = 1;
	rifle_hud.crosshairs.count = 1;
	rifle_hud.crosshairs.address = rifle_crosshairs;
	pistol_hud.valid_crosshair_types_flags = 3;
	pistol_hud.crosshairs.count = 3;
	pistol_hud.crosshairs.address = pistol_crosshairs;
	pistol_hud.screen_effects.count = 1;
	pistol_hud.screen_effects.address = pistol_effects;
	tag(_tag_rifle, WEAPON_DEFINITION_TAG, "weapons\\assault rifle\\assault rifle", &rifle);
	tag(_tag_bullet, PROJECTILE_DEFINITION_TAG, "weapons\\assault rifle\\bullet", &bullet);
	tag(_tag_rifle_hud, HUD_GROUP_TAG, "weapons\\assault rifle\\assault rifle", &rifle_hud);
	tag(_tag_pistol_hud, HUD_GROUP_TAG, "weapons\\pistol\\pistol", &pistol_hud);
	tag(_tag_zoom_in, SOUND_DEFINITION_TAG, "sound\\sfx\\weapons\\sniper rifle\\sniper_2x_zoom", &other_tag_data);
	tag(_tag_zoom_out, SOUND_DEFINITION_TAG, "sound\\sfx\\weapons\\sniper rifle\\sniper_10x_zoom", &other_tag_data);
	tag(_tag_stone, EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\impact stone", &other_tag_data);
	tag(_tag_hollow, EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\impact metal hollow reflect",
		&other_tag_data);
	tag(_tag_thin, EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\impact metal thin reflect", &other_tag_data);
	tag(_tag_thick, EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\impact metal thick reflect",
		&other_tag_data);
	tag(_tag_ice, EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\impact ice", &other_tag_data);
	memset(&plasma, 0, sizeof(plasma));
	memset(plasma_triggers, 0, sizeof(plasma_triggers));
	plasma.weapon.heat_recovery_threshold = 0.25f;
	plasma.weapon.age_rate_of_fire_penalty = 0.2f;
	plasma.weapon.triggers.count = 1;
	plasma.weapon.triggers.address = plasma_triggers;
	plasma_triggers[0].error_acceleration_time = 2.5f;
	plasma_triggers[0].projectile.index = NONE;
	plasma_triggers[0].runtime_error_acceleration_time = 1.f / 75.f;
	plasma_triggers[0].runtime_error_deceleration_time = 1.f;
	plasma_triggers[0].projectile_error_angle_lower_bound = 0.01f;
	memset(&bolt, 0, sizeof(bolt));
	bolt.object.model.index = NONE;
	memset(bolt_responses, 0, sizeof(bolt_responses));
	bolt_responses[_material_hunter_shield].default_response = _projectile_material_response_reflect;
	bolt.projectile.initial_velocity = 50.f / 30.f;
	bolt.projectile.final_velocity = 25.f / 30.f;
	bolt.projectile.material_responses.count = NUMBER_OF_MATERIAL_TYPES;
	bolt.projectile.material_responses.address = bolt_responses;
	memset(&bolt_damage, 0, sizeof(bolt_damage));
	for (index = 0; index < 40; index++)
		bolt_damage.damage.material_modifiers[index] = 0.75f;
	memset(&plasma_hud, 0, sizeof(plasma_hud));
	memset(plasma_crosshairs, 'p', sizeof(plasma_crosshairs));
	plasma_hud.valid_crosshair_types_flags = 0x41;
	plasma_hud.crosshairs.count = 2;
	plasma_hud.crosshairs.address = plasma_crosshairs;
	tag(_tag_plasma, WEAPON_DEFINITION_TAG, "weapons\\plasma rifle\\plasma rifle", &plasma);
	tag(_tag_bolt, PROJECTILE_DEFINITION_TAG, "weapons\\plasma rifle\\bolt", &bolt);
	tag(_tag_bolt_damage, DAMAGE_EFFECT_DEFINITION_TAG, "weapons\\plasma rifle\\bolt", &bolt_damage);
	tag(_tag_plasma_hud, HUD_GROUP_TAG, "weapons\\plasma rifle\\plasma rifle", &plasma_hud);
	memset(&splat, 0, sizeof(splat));
	splat.flags = 1 << 3;
	splat.radius_lower_bound = 0.1f;
	splat.radius_upper_bound = 0.2f;
	splat.framebuffer_blend_function = 1;
	splat.map.index = NONE;
	tag(_tag_splat, DECAL_GROUP_TAG, "effects\\decals\\blood splats\\blood grunt splat", &splat);
	splat.type = 1;
	memset(&sign, 0, sizeof(sign));
	sign.type = 3;
	sign.framebuffer_blend_function = 1;
	sign.map.index = NONE;
	tag(_tag_sign, DECAL_GROUP_TAG, "levels\\test\\bloodgulch\\decals\\red base", &sign);
	tag(_tag_ball, MODELS_GROUP_TAG, "powerups\\active camoflage\\active camoflage", &other_tag_data);
	tag(_tag_glow, BITMAP_GROUP_TAG, "effects\\decals\\blood splats\\bitmaps\\blood splat hunter glow",
		&other_tag_data);
	tag(_tag_teleport, EFFECT_DEFINITION_TAG, "effects\\coop teleport", &other_tag_data);
	tag(_tag_splotch, EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\impact grunt", &other_tag_data);
	mutator_tags_loaded();
}

/* every tag's bytes, to see what an edit changed */
struct snapshot
{
	struct weapon_definition rifle;
	struct weapon_magazine_definition magazines[2];
	struct weapon_trigger_definition triggers[2];
	struct projectile_definition bullet;
	struct projectile_material_response_definition responses[NUMBER_OF_MATERIAL_TYPES + 1];
	struct weapon_hud_interface_definition rifle_hud, pistol_hud;
	struct weapon_definition plasma;
	struct weapon_trigger_definition plasma_triggers[1];
	struct projectile_definition bolt;
	struct projectile_material_response_definition bolt_responses[NUMBER_OF_MATERIAL_TYPES];
	struct damage_effect_definition bolt_damage;
	struct weapon_hud_interface_definition plasma_hud;
};

static void snapshot(struct snapshot *taken)
{
	taken->rifle = rifle;
	memcpy(taken->magazines, magazines, sizeof(magazines));
	memcpy(taken->triggers, triggers, sizeof(triggers));
	taken->bullet = bullet;
	memcpy(taken->responses, responses, sizeof(responses));
	taken->rifle_hud = rifle_hud;
	taken->pistol_hud = pistol_hud;
	taken->plasma = plasma;
	memcpy(taken->plasma_triggers, plasma_triggers, sizeof(plasma_triggers));
	taken->bolt = bolt;
	memcpy(taken->bolt_responses, bolt_responses, sizeof(bolt_responses));
	taken->bolt_damage = bolt_damage;
	taken->plasma_hud = plasma_hud;
}

static int unchanged(struct snapshot const *taken)
{
	struct snapshot now;

	snapshot(&now);
	return !memcmp(&now, taken, sizeof(now));
}

/* the tag edits of the mutators game.mutators turns on */
static long edits_of(char const *mutators, unsigned char *edits, long maximum)
{
	snprintf(setting, sizeof(setting), "%s", mutators);
	port_mutators_scan();
	return port_mutators_tag_edits(edits, maximum);
}

static long find(char const *id)
{
	long index;

	for (index = 0; index < port_mutators_count(); index++)
	{
		if (!strcmp(port_mutators.mutators[index].id, id))
			return index;
	}
	return NONE;
}

/* the reason a folder's mutator cannot be played, or "" */
static char const *refusal(char const *id)
{
	static char text[256];

	port_mutators_status(find(id), text, sizeof(text));
	return text;
}

static int degrees(real radians, real expected)
{
	return radians == expected * (_pi / 180.f);
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	static unsigned char edits[MAXIMUM_MUTATOR_TAG_EDIT_BYTES];
	struct snapshot stock;
	long size;
	int index;

	CHECK(argc == 2, "case");
	(void)reads;
	(void)logged;
	world();
	snapshot(&stock);
	if (!strcmp(case_name, "ma5b"))
	{
		struct projectile_material_response_definition const *stone = &responses[_material_stone];
		static int const ricochets[] = { _material_stone, _material_hollow_metal, _material_thin_metal,
			_material_thick_metal, _material_ice };
		static int const reflects[] = { _material_force_field, _material_hunter_armor, _material_engineer_force_field,
			_material_cyborg_energy_shield, _material_elite_energy_shield };

		size = edits_of("accurized_ma5b", edits, sizeof(edits));
		CHECK(port_mutators_status(find("accurized_ma5b"), NULL, 0) == _port_mutator_fine, "%s",
			refusal("accurized_ma5b"));
		CHECK(size > 0 && port_mutators.mutators[find("accurized_ma5b")].tag_edit_count == 38, "the edits (%ld)",
			port_mutators.mutators[find("accurized_ma5b")].tag_edit_count);
		CHECK(mutator_tags_play(edits, size), "the built-in mutator's edits refused");
		CHECK(mutator_tags.made == 38 && mutator_tags.passed_over == 0, "made %d, passed over %d", mutator_tags.made,
			mutator_tags.passed_over);
		/* the weapon: the pistol's 2x zoom, the sniper rifle's sounds */
		CHECK(rifle.weapon.zoom_level_count == 1 && rifle.weapon.zoom_magnification_minimum == 2.f &&
			rifle.weapon.zoom_magnification_maximum == 2.f, "the zoom");
		CHECK(rifle.weapon.zoom_in_sound.index == TAG_INDEX(_tag_zoom_in) &&
			rifle.weapon.zoom_in_sound.group_tag == SOUND_DEFINITION_TAG &&
			rifle.weapon.zoom_in_sound.name == tags[_tag_zoom_in].name &&
			rifle.weapon.zoom_in_sound.name_length == (long)strlen(tags[_tag_zoom_in].name) &&
			rifle.weapon.zoom_out_sound.index == TAG_INDEX(_tag_zoom_out), "the zoom's sounds");
		/* the trigger: 0.05 to 3 degrees of error, settling in 0.1 seconds, a tracer each 2 rounds */
		CHECK(degrees(triggers[0].projectile_error_angle_lower_bound, 0.05f) &&
			degrees(triggers[0].projectile_error_angle_upper_bound, 3.f), "the error angles: %g %g",
			triggers[0].projectile_error_angle_lower_bound, triggers[0].projectile_error_angle_upper_bound);
		CHECK(triggers[0].error_deceleration_time == 0.1f &&
			triggers[0].runtime_error_deceleration_time == 1.f / (0.1f * 30.f), "the error's settling: %g %g",
			triggers[0].error_deceleration_time, triggers[0].runtime_error_deceleration_time);
		CHECK(triggers[0].rounds_between_tracers == 2 && triggers[0].flags == stock.triggers[0].flags, "the tracers");
		CHECK(!memcmp(&triggers[1], &stock.triggers[1], sizeof(triggers[1])), "past the block's element");
		/* the bullet: 300 world units, CR's ricochets, the stock's overpenetration */
		CHECK(bullet.projectile.maximum_range == 300.f, "the range");
		for (index = 0; index < (int)NUMBEROF(ricochets); index++)
		{
			struct projectile_material_response_definition const *response = &responses[ricochets[index]];

			CHECK(response->potential_response == _projectile_material_response_reflect &&
				response->potential_skip_fraction == 0.9f && degrees(response->potential_angle_upper_bound, 60.f) &&
				response->potential_angle_lower_bound == 0.f && response->potential_flags == 0 &&
				response->potential_effect.group_tag == EFFECT_DEFINITION_TAG &&
				response->default_response == _projectile_material_response_detonate, "material %d's ricochet",
				ricochets[index]);
		}
		CHECK(stone->potential_effect.index == TAG_INDEX(_tag_stone) &&
			responses[_material_ice].potential_effect.index == TAG_INDEX(_tag_ice) &&
			responses[_material_hollow_metal].potential_effect.index == TAG_INDEX(_tag_hollow), "the ricochets' effects");
		for (index = 0; index < (int)NUMBEROF(reflects); index++)
		{
			CHECK(responses[reflects[index]].default_response == _projectile_material_response_reflect &&
				responses[reflects[index]].potential_response == 0, "material %d's reflection", reflects[index]);
		}
		CHECK(responses[_material_grunt].default_response == _projectile_material_response_overpenetrate &&
			!memcmp(&responses[_material_grunt], &stock.responses[_material_grunt], sizeof(responses[0])) &&
			!memcmp(&responses[NUMBER_OF_MATERIAL_TYPES], &stock.responses[NUMBER_OF_MATERIAL_TYPES],
				sizeof(responses[0])), "a material it does not edit");
		/* the HUD: its own crosshair, then the pistol's scope crosshairs, and its mask */
		CHECK(rifle_hud.valid_crosshair_types_flags == 3, "the crosshair types");
		CHECK(rifle_hud.crosshairs.count == 3 && rifle_hud.crosshairs.address != rifle_crosshairs &&
			!memcmp(rifle_hud.crosshairs.address, rifle_crosshairs[0], CROSSHAIR_BYTES) &&
			!memcmp((byte *)rifle_hud.crosshairs.address + CROSSHAIR_BYTES, pistol_crosshairs[1], CROSSHAIR_BYTES) &&
			!memcmp((byte *)rifle_hud.crosshairs.address + 2 * CROSSHAIR_BYTES, pistol_crosshairs[2], CROSSHAIR_BYTES),
			"the crosshairs");
		CHECK(rifle_hud.screen_effects.count == 1 &&
			!memcmp(rifle_hud.screen_effects.address, pistol_effects, sizeof(pistol_effects)), "the scope's mask");
		CHECK(!memcmp(&pistol_hud, &stock.pistol_hud, sizeof(pistol_hud)) && pistol_crosshairs[1][0] == '1',
			"the pistol's own HUD changed");
		/* played again (the next game on the map): made once, not twice */
		CHECK(mutator_tags_play(edits, size) && rifle_hud.crosshairs.count == 3, "the copy made twice");
		mutator_tags_undo();
		CHECK(unchanged(&stock), "the stock tags not put back");
		CHECK(rifle_hud.crosshairs.address == rifle_crosshairs, "the block's own elements not put back");
		/* (the map's tags loaded again: its own bytes, nothing undone) */
		mutator_tags_play(edits, size);
		world();
		snapshot(&stock);
		mutator_tags_undo();
		CHECK(unchanged(&stock), "edits of a map's tags no longer loaded undone");
	}
	else if (!strcmp(case_name, "plasma"))
	{
		struct weapon_trigger_definition const *trigger = &plasma_triggers[0];
		static int const modified[] = { _material_dirt, _material_sand, _material_stone, _material_snow, _material_wood,
			_material_rubber, _material_hunter_skin, _material_cyborg, _material_monitor, _material_plastic,
			_material_water, _material_leaves, _material_ice };

		size = edits_of("revolved_plasma_rifle", edits, sizeof(edits));
		CHECK(port_mutators_status(find("revolved_plasma_rifle"), NULL, 0) == _port_mutator_fine, "%s",
			refusal("revolved_plasma_rifle"));
		CHECK(mutator_tags_play(edits, size) && mutator_tags.made == 47 && mutator_tags.passed_over == 0,
			"made %d, passed over %d", mutator_tags.made, mutator_tags.passed_over);
		/* the weapon: its heat, its age, the camouflage it costs; no zoom */
		CHECK(plasma.weapon.heat_recovery_threshold == 0.15f && plasma.weapon.heat_loss_per_second == 0.36f &&
			plasma.weapon.age_heat_recovery_penalty == 0.f && plasma.weapon.age_rate_of_fire_penalty == 0.f &&
			plasma.weapon.age_misfire_start == 0.85f && plasma.weapon.age_misfire_chance == 0.25f &&
			plasma.weapon.active_camo_ding == 0.25f && plasma.weapon.active_camo_regrowth_rate == 0.075f, "the weapon");
		CHECK(plasma.weapon.zoom_level_count == stock.plasma.weapon.zoom_level_count &&
			plasma.weapon.zoom_in_sound.index == stock.plasma.weapon.zoom_in_sound.index, "a zoom");
		/* the trigger, each time with its step a tick: 1/(0.8 x 30), 1/(0.6 x 30) */
		CHECK(trigger->initial_rate_of_fire == 6.f && trigger->final_rate_of_fire == 9.f &&
			trigger->rounds_between_tracers == 0 && trigger->heat_generated_per_round == 0.0861f &&
			trigger->age_generated_per_round == 0.003f, "the trigger");
		CHECK(trigger->rate_of_fire_acceleration_time == 0.8f &&
			trigger->runtime_rate_of_fire_acceleration_time == 1.f / (0.8f * 30.f) &&
			trigger->rate_of_fire_deceleration_time == 0.6f &&
			trigger->runtime_rate_of_fire_deceleration_time == 1.f / (0.6f * 30.f) &&
			trigger->error_acceleration_time == 0.8f && trigger->runtime_error_acceleration_time == 1.f / (0.8f * 30.f) &&
			trigger->error_deceleration_time == 0.6f && trigger->runtime_error_deceleration_time == 1.f / (0.6f * 30.f),
			"the times' steps: %g %g %g %g", trigger->runtime_rate_of_fire_acceleration_time,
			trigger->runtime_rate_of_fire_deceleration_time, trigger->runtime_error_acceleration_time,
			trigger->runtime_error_deceleration_time);
		CHECK(degrees(trigger->projectile_error_angle_upper_bound, 0.75f) &&
			trigger->projectile_error_angle_lower_bound == 0.01f, "the error");
		/* the bolt: 27 world units a second (0.9 a tick), its ranges */
		CHECK(bolt.projectile.initial_velocity == 0.9f && bolt.projectile.final_velocity == 0.9f, "the speeds: %.9g",
			bolt.projectile.initial_velocity);
		CHECK(bolt.projectile.maximum_range == 45.f && bolt.projectile.air_damage_range_lower_bound == 15.f &&
			bolt.projectile.air_damage_range_upper_bound == 45.f && bolt.projectile.water_damage_range_lower_bound == 15.f &&
			bolt.projectile.water_damage_range_upper_bound == 45.f, "the ranges");
		CHECK(bolt_responses[_material_hunter_shield].default_response == _projectile_material_response_disappear,
			"the hunter's shield");
		/* its damage, and each material's modifier */
		CHECK(bolt_damage.damage.damage_minimum == 13.f && bolt_damage.damage.damage_lower_bound == 13.f &&
			bolt_damage.damage.damage_upper_bound == 13.f && bolt_damage.damage.active_camouflage_damage == 1.f,
			"the damage");
		for (index = 0; index < (int)NUMBEROF(modified); index++)
			CHECK(bolt_damage.damage.material_modifiers[modified[index]] == 1.f, "material %d", modified[index]);
		CHECK(bolt_damage.damage.material_modifiers[_material_hollow_metal] == 0.5f &&
			bolt_damage.damage.material_modifiers[_material_hunter_armor] == 0.25f &&
			bolt_damage.damage.material_modifiers[_material_jackal_energy_shield] == 2.f &&
			bolt_damage.damage.material_modifiers[_material_engineer_force_field] == 2.f, "the other modifiers");
		CHECK(bolt_damage.damage.material_modifiers[_material_elite] == 0.75f &&
			bolt_damage.damage.material_modifiers[_material_hunter_shield] == 0.75f &&
			bolt_damage.damage.material_modifiers[NUMBER_OF_MATERIAL_TYPES] == 0.75f, "a modifier it does not set");
		/* the HUD: its own, without the pistol's scope */
		CHECK(!memcmp(&plasma_hud, &stock.plasma_hud, sizeof(plasma_hud)), "the HUD changed");
		mutator_tags_undo();
		CHECK(unchanged(&stock), "the stock tags not put back");
		/* (a time of 0: a step of 1, as the tools make it) */
		folder("instant", "[[tags]]\ngroup = \"weapon\"\ntag = 'weapons\\plasma rifle\\plasma rifle'\n"
			"[tags.triggers.0]\nerror_acceleration_time = 0\n");
		size = edits_of("instant", edits, sizeof(edits));
		CHECK(mutator_tags_play(edits, size) && trigger->runtime_error_acceleration_time == 1.f, "a time of 0's step");
	}
	else if (!strcmp(case_name, "magazine"))
	{
		/* a magazine's rounds and times, and a projectile's gravity */
		folder("rounds", "format = 2\n[[tags]]\ngroup = \"weapon\"\ntag = 'weapons\\assault rifle\\assault rifle'\n"
			"[tags.magazines.0]\nrounds_recharged_per_second = 3\nrounds_total_initial = 6\n"
			"rounds_total_maximum = 36\nrounds_loaded_maximum = 12\nrounds_reloaded = 1\nchamber_time = 0.5\n"
			"[[tags]]\ngroup = \"projectile\"\ntag = 'weapons\\assault rifle\\bullet'\n"
			"air_gravity_scale = 0\nwater_gravity_scale = 2.5\n");
		size = edits_of("rounds", edits, sizeof(edits));
		CHECK(port_mutators_status(find("rounds"), NULL, 0) == _port_mutator_fine, "%s", refusal("rounds"));
		CHECK(mutator_tags_play(edits, size) && mutator_tags.made == 8 && mutator_tags.passed_over == 0,
			"made %d, passed over %d", mutator_tags.made, mutator_tags.passed_over);
		CHECK(magazines[0].rounds_recharged_per_second == 3 && magazines[0].rounds_total_initial == 6 &&
			magazines[0].rounds_total_maximum == 36 && magazines[0].rounds_loaded_maximum == 12 &&
			magazines[0].rounds_reloaded == 1 && magazines[0].chamber_time == 0.5f &&
			magazines[0].flags == stock.magazines[0].flags && magazines[0].pad == stock.magazines[0].pad,
			"the magazine");
		CHECK(!memcmp(&magazines[1], &stock.magazines[1], sizeof(magazines[1])), "past the block's element");
		CHECK(!memcmp(triggers, stock.triggers, sizeof(triggers)), "the triggers changed");
		CHECK(bullet.projectile.air_gravity_scale == 0.f && bullet.projectile.water_gravity_scale == 2.5f &&
			bullet.projectile.maximum_range == 40.f, "the gravity");
		mutator_tags_undo();
		CHECK(unchanged(&stock), "the stock tags not put back");
		/* (a second magazine the rifle has not: passed over) */
		folder("second_magazine", "format = 2\n[[tags]]\ngroup = \"weapon\"\n"
			"tag = 'weapons\\assault rifle\\assault rifle'\n[tags.magazines.1]\nrounds_total_initial = 1\n");
		size = edits_of("second_magazine", edits, sizeof(edits));
		CHECK(mutator_tags_play(edits, size) && mutator_tags.passed_over == 1 && unchanged(&stock),
			"past the block written");
	}
	else if (!strcmp(case_name, "missing"))
	{
		/* a map without the pistol's HUD or the sniper rifle's zoom: the rest made */
		tags[_tag_pistol_hud].present = 0;
		tags[_tag_zoom_in].present = 0;
		tags[_tag_ice].present = 0;
		size = edits_of("accurized_ma5b", edits, sizeof(edits));
		CHECK(mutator_tags_play(edits, size), "refused");
		CHECK(mutator_tags.passed_over == 4 && mutator_tags.made == 34, "made %d, passed over %d", mutator_tags.made,
			mutator_tags.passed_over);
		CHECK(rifle.weapon.zoom_in_sound.index == NONE && rifle.weapon.zoom_out_sound.index == TAG_INDEX(_tag_zoom_out),
			"a sound the map has not");
		CHECK(rifle_hud.crosshairs.count == 1 && rifle_hud.screen_effects.count == 0 &&
			rifle_hud.valid_crosshair_types_flags == 3, "a HUD the map has not");
		CHECK(responses[_material_ice].potential_effect.index == NONE &&
			responses[_material_ice].potential_response == _projectile_material_response_reflect, "an effect");
		CHECK(rifle.weapon.zoom_level_count == 1 && bullet.projectile.maximum_range == 300.f, "the others");
		/* a map without the weapon: none of its edits made, nothing touched */
		mutator_tags_undo();
		world();
		tags[_tag_rifle].present = 0;
		mutator_tags_play(edits, size);
		CHECK(rifle.weapon.zoom_level_count == 0 && triggers[0].rounds_between_tracers == 3, "a tag the map has not");
		/* an element past the tag's block */
		mutator_tags_undo();
		world();
		folder("second", "[[tags]]\ngroup = \"weapon\"\ntag = 'weapons\\assault rifle\\assault rifle'\n"
			"[tags.triggers.1]\nrounds_between_tracers = 9\n");
		size = edits_of("second", edits, sizeof(edits));
		CHECK(size > 0 && mutator_tags_play(edits, size) && mutator_tags.passed_over == 1, "refused");
		CHECK(triggers[1].rounds_between_tracers == stock.triggers[1].rounds_between_tracers, "past the block written");
		rifle.weapon.triggers.count = 2;
		CHECK(mutator_tags_play(edits, size) && triggers[1].rounds_between_tracers == 9, "the second trigger");
	}
	else if (!strcmp(case_name, "stock"))
	{
		/* a map with every tag: the stock maps not asked for any */
		size = edits_of("accurized_ma5b", edits, sizeof(edits));
		CHECK(mutator_tags_play(edits, size) && !stock_finds && mutator_tags.passed_over == 0, "asked of a whole map");
		/* a tag only named, which a stock map has: added, the edit made, and the
		stock map let go of */
		mutator_tags_undo();
		world();
		tags[_tag_zoom_in].present = 0;
		stock_has_zoom = 1;
		stock_finishes = 0;
		CHECK(mutator_tags_play(edits, size) && mutator_tags.passed_over == 0, "passed over %d", mutator_tags.passed_over);
		CHECK(stock_finds == 1 && rifle.weapon.zoom_in_sound.index == TAG_INDEX(_tag_zoom_in), "the sound of a stock map");
		CHECK(stock_finishes == 1, "the stock map kept");
		/* a tag edited, which a stock map may have: only passed over */
		mutator_tags_undo();
		world();
		tags[_tag_rifle].present = 0;
		stock_finds = 0;
		mutator_tags_play(edits, size);
		CHECK(rifle.weapon.zoom_level_count == 0, "a tag the map has not edited");
		CHECK(mutator_tags.passed_over > 0, "not passed over");
	}
	else if (!strcmp(case_name, "refused"))
	{
		static struct { char const *id; char const *text; char const *reason; } const cases[] =
		{
			{ "group", "[[tags]]\ngroup = \"biped\"\ntag = 'x'\njump_velocity = 1\n", "group biped" },
			{ "field", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\nready_time = 1\n", "no field" },
			{ "range", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\n[tags.triggers.0]\n"
				"projectile_error_angle_upper_bound = 200\n", "from 0 to 180" },
			{ "whole", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\nzoom_level_count = 1.5\n", "whole number" },
			{ "nan", "[[tags]]\ngroup = \"projectile\"\ntag = 'x'\nmaximum_range = nan\n", "is a number" },
			{ "enum", "[[tags]]\ngroup = \"projectile\"\ntag = 'x'\n[tags.material_responses.stone]\n"
				"default_response = \"explode\"\n", "disappear, detonate" },
			{ "material", "[[tags]]\ngroup = \"projectile\"\ntag = 'x'\n[tags.material_responses.lava]\n"
				"default_response = \"reflect\"\n", "not an element" },
			{ "flags", "[[tags]]\ngroup = \"weapon_hud_interface\"\ntag = 'x'\nvalid_crosshair_types = [\"aim\", "
				"\"laser\"]\n", "aim, zoom" },
			{ "block", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\ntriggers = 1\n", "is a block" },
			{ "deep", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\n[tags.zoom_level_count.x]\ny = 1\n",
				"not a field's path" },
			{ "copy", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\nzoom_level_count = { from = 'y' }\n", "a number" },
			{ "copied", "[[tags]]\ngroup = \"weapon_hud_interface\"\ntag = 'x'\ncrosshairs = 'y'\n", "is copied" },
			{ "many", "[[tags]]\ngroup = \"weapon_hud_interface\"\ntag = 'x'\n"
				"screen_effects = { from = 'y', elements = [1] }\n", "at most 1" },
			{ "copy_key", "[[tags]]\ngroup = \"weapon_hud_interface\"\ntag = 'x'\n"
				"crosshairs = { from = 'y', run = 'z' }\n", "not run" },
			{ "nameless", "[[tags]]\ngroup = \"weapon\"\nzoom_level_count = 1\n", "names its group and its tag" },
			{ "reference", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\nzoom_in_sound = 3\n", "a tag's name" },
			{ "nothing", "name = \"Nothing\"\n", "edits no tag, poses nothing and adds nothing" },
			{ "array", "[[tags]]\ngroup = \"damage_effect\"\ntag = 'x'\n[tags.material_modifiers]\nlava = 1\n",
				"name one of material_modifiers" },
			{ "array_range", "[[tags]]\ngroup = \"damage_effect\"\ntag = 'x'\n[tags.material_modifiers]\n"
				"elite = 11\n", "from 0 to 10" },
			{ "array_past", "[[tags]]\ngroup = \"damage_effect\"\ntag = 'x'\n[tags.material_modifiers]\n"
				"33 = 1\n", "name one of" },
			{ "array_whole", "[[tags]]\ngroup = \"damage_effect\"\ntag = 'x'\nmaterial_modifiers = 1\n",
				"name one of" },
			{ "rounds_range", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\n[tags.magazines.0]\n"
				"rounds_total_maximum = 40000\n", "from 0 to 32767" },
			{ "rounds_whole", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\n[tags.magazines.0]\n"
				"rounds_total_initial = 6.5\n", "whole number" },
			{ "rounds_field", "[[tags]]\ngroup = \"weapon\"\ntag = 'x'\n[tags.magazines.0]\n"
				"reload_time = 1\n", "no field" },
			{ "gravity_range", "[[tags]]\ngroup = \"projectile\"\ntag = 'x'\nair_gravity_scale = -1\n",
				"from 0 to 10" },
			{ "later", "format = 8\n[[tags]]\ngroup = \"weapon\"\ntag = 'x'\nzoom_level_count = 1\n", "later build" },
		};
		for (index = 0; index < (int)NUMBEROF(cases); index++)
			folder(cases[index].id, cases[index].text);
		port_mutators_scan();
		for (index = 0; index < (int)NUMBEROF(cases); index++)
		{
			CHECK(port_mutators_status(find(cases[index].id), NULL, 0) == _port_mutator_unplayable &&
				strstr(refusal(cases[index].id), cases[index].reason), "%s: %s", cases[index].id,
				refusal(cases[index].id));
		}
		CHECK(edits_of("group,field,range,enum,flags,copy,many,nothing", edits, sizeof(edits)) == 0,
			"a refused mutator's edits played");
	}
	else if (!strcmp(case_name, "pose"))
	{
		static struct { char const *id; char const *text; char const *reason; } const cases[] =
		{
			{ "large", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\nscale = 5\n", "from 0.25 to 4" },
			{ "nan", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\nscale = nan\n", "from 0.25 to 4" },
			{ "far", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\noffset = [0, 1.5, 0]\n", "from -1 to 1" },
			{ "farther", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\noffset = [0, 1e30, 0]\n", "from -1 to 1" },
			{ "two", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\noffset = [0, 1]\n", "three numbers" },
			{ "text", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\noffset = [0, 'up', 0]\n", "three numbers" },
			{ "tree", "[[pose]]\nobjects = \"tree\"\nnode = \"head\"\n", "the group tree" },
			{ "nodeless", "[[pose]]\nobjects = \"biped\"\nscale = 2\n", "a node" },
			{ "key", "[[pose]]\nobjects = \"biped\"\nnode = \"head\"\nsize = 2\n", "no key size" },
			{ "table", "pose = 3\n", "list of tables" },
			{ "effect", "[effects.moon_gravity]\nscale = 0.2\n", "no effect" },
		};
		real_vector3d none = { 0.f, 0.f, 0.f };

		/* (the stock big head: every biped's head) */
		size = edits_of("big_head", edits, sizeof(edits));
		CHECK(port_mutators_status(find("big_head"), NULL, 0) == _port_mutator_fine && size > 0, "%s",
			refusal("big_head"));
		CHECK(mutator_tags_play(edits, size) && mutator_tags.made == 1 && pose_count == 1 &&
			poses[0].group_tag == 'bipd' && !poses[0].tag[0] && !strcmp(poses[0].node, "head") &&
			poses[0].scale == 2.5f && !memcmp(&poses[0].offset, &none, sizeof(none)), "the big head's pose");
		CHECK(unchanged(&stock), "a pose edited a tag");
		/* (a pose of one tag, moved; one the map has no node for, passed over) */
		folder("long_arms", "[[pose]]\nobjects = \"unit\"\ntag = 'characters\\cyborg\\cyborg'\n"
			"node = \"bip01 l upperarm\"\nscale = 1.35\noffset = [0.01, 0, -0.08]\n"
			"[[pose]]\nobjects = \"biped\"\nnode = \"tail\"\nscale = 0.5\n");
		size = edits_of("long_arms", edits, sizeof(edits));
		CHECK(port_mutators_status(find("long_arms"), NULL, 0) == _port_mutator_fine, "%s", refusal("long_arms"));
		CHECK(mutator_tags_play(edits, size) && pose_count == 1 && mutator_tags.passed_over == 1, "the poses made");
		CHECK(poses[0].group_tag == 'unit' && !strcmp(poses[0].tag, "characters\\cyborg\\cyborg") &&
			!strcmp(poses[0].node, "bip01 l upperarm") && poses[0].scale == 1.35f && poses[0].offset.i == 0.01f &&
			poses[0].offset.j == 0.f && poses[0].offset.k == -0.08f, "the arm's pose");
		CHECK(port_mutators_details(find("long_arms"), (char *)edits, sizeof(edits)) &&
			strstr((char const *)edits, "Poses: 2") && !strstr((char const *)edits, "Tag edits"), "%s", edits);
		/* (each game's poses made again: the last undone) */
		index = pose_clears;
		mutator_tags_undo();
		CHECK(pose_clears == index + 1 && pose_count == 0, "the poses kept past their game");
		mutator_tags_loaded();
		CHECK(pose_clears == index + 2, "the poses kept past their map");
		for (index = 0; index < (int)NUMBEROF(cases); index++)
			folder(cases[index].id, cases[index].text);
		port_mutators_scan();
		for (index = 0; index < (int)NUMBEROF(cases); index++)
		{
			CHECK(port_mutators_status(find(cases[index].id), NULL, 0) == _port_mutator_unplayable &&
				strstr(refusal(cases[index].id), cases[index].reason), "%s: %s", cases[index].id,
				refusal(cases[index].id));
		}
	}
	else if (!strcmp(case_name, "paintball"))
	{
		static struct { char const *id; char const *text; char const *reason; } const cases[] =
		{
			{ "fast", "[[option]]\nid = \"speed\"\nname = \"S\"\nminimum = 25\nmaximum = 200\n[[tags]]\n"
				"group = \"projectile\"\ntag = 'x'\ninitial_velocity = { value = 2000, option = \"speed\" }\n", "at 200%" },
			{ "unnamed", "[[tags]]\ngroup = \"projectile\"\ntag = 'x'\ninitial_velocity = { value = 20, option = \"pace\" }\n",
				"not one of its [[option]]s" },
			{ "between", "[[option]]\nid = \"speed\"\nname = \"S\"\ndefault = 110\nminimum = 25\nmaximum = 200\n"
				"step = 25\n", "whole percents" },
			{ "trees", "[death]\nobjects = \"scenery\"\n", "units" },
			{ "melts", "[death]\nobjects = \"biped\"\nbody = \"melts\"\n", "vanish or stays" },
			{ "bright", "[[tags]]\ngroup = \"decal\"\ntag = 'x'\n[tags.color_lower_bound]\nred = 2\n", "from 0 to 1" },
			{ "wherever", "[[tags]]\ngroup = \"decal\"\ntag = '*'\nwhere = { type = [\"sticker\"] }\nradius_lower_bound = 1\n",
				"in a where is one of" },
			{ "whereof", "[[tags]]\ngroup = \"decal\"\ntag = 'x'\nwhere = { type = [\"burn\"] }\nradius_lower_bound = 1\n",
				"of tag = '*'" },
			{ "boom", "[explosion]\ndecal = 'x'\nsize = 2\n", "[explosion] has a decal" },
			{ "firedecal", "[[tags]]\ngroup = \"decal\"\ntag = '*'\nwhere = { fired = true }\nradius_lower_bound = 1\n",
				"of projectiles" },
		};

		folder("paint", "format = 7\n"
			"[[option]]\nid = \"speed\"\nname = \"SPEED\"\nminimum = 25\nmaximum = 200\nstep = 25\n"
			"[[tags]]\ngroup = \"decal\"\ntag = '*'\nwhere = { type = [\"scratch\", \"splatter\", \"burn\"] }\n"
			"map = 'effects\\decals\\blood splats\\bitmaps\\blood splat hunter glow'\n"
			"framebuffer_blend_function = \"add\"\nflags = [\"color_interpolate_in_hsv\"]\n"
			"[tags.color_upper_bound]\nblue = 0.5\n"
			"[[tags]]\ngroup = \"projectile\"\ntag = 'weapons\\assault rifle\\bullet'\n"
			"initial_velocity = { value = 45, option = \"speed\" }\n"
			"[tags.material_responses.all]\ndefault_effect = 'weapons\\assault rifle\\effects\\impact grunt'\n"
			"[[tags]]\ngroup = \"projectile\"\ntag = '*'\nwhere = { fired = true }\n"
			"model = 'powerups\\active camoflage\\active camoflage'\n"
			"[[tags]]\ngroup = \"projectile\"\ntag = '*'\nmaximum_range = 500\n"
			"[explosion]\ndecal = 'effects\\decals\\blood splats\\blood grunt splat'\n"
			"[death]\nobjects = \"biped\"\neffect = 'effects\\coop teleport'\nbody = \"vanish\"\n");
		size = edits_of("paint", edits, sizeof(edits));
		CHECK(port_mutators_status(find("paint"), NULL, 0) == _port_mutator_fine && size > 0, "%s", refusal("paint"));
		CHECK(mutator_tags_play(edits, size) && !mutator_tags.passed_over, "passed over %d", mutator_tags.passed_over);
		CHECK(splat.framebuffer_blend_function == 3 && splat.flags == 2 && splat.map.index == TAG_INDEX(_tag_glow) &&
			splat.color_upper_bound.blue == 0.5f && splat.radius_upper_bound == 0.2f, "the splat's edits");
		/* (every decal of the types where names: not the map's painted sign) */
		CHECK(sign.framebuffer_blend_function == 1 && sign.map.index == NONE, "a painted sign edited");
		/* (every projectile a weapon fires: the rifle's bullet, not the bolt
		no trigger here fires) */
		CHECK(bullet.object.model.index == TAG_INDEX(_tag_ball) && bolt.object.model.index == NONE,
			"the fired projectiles' model");
		CHECK(bullet.projectile.maximum_range == 500.f && bolt.projectile.maximum_range == 500.f,
			"every projectile's range");
		CHECK(explosion_decal_index == TAG_INDEX(_tag_splat) && explosion_decal_radius == 0.2f,
			"the explosion's decal, and its radius");
		/* (every element the block has: an "all") */
		for (index = 0; index < NUMBER_OF_MATERIAL_TYPES + 1; index++)
		{
			CHECK((responses[index].default_effect.index == TAG_INDEX(_tag_splotch)) == (index < NUMBER_OF_MATERIAL_TYPES),
				"material %d's effect", index);
		}
		CHECK(bullet.projectile.initial_velocity == 45.f / TICKS_PER_SECOND, "the speed at 100%%");
		CHECK(death_count == 1 && deaths[0].group_tag == 'bipd' && deaths[0].effect_index == TAG_INDEX(_tag_teleport) &&
			deaths[0].vanish, "the death");
		/* (its option: a step at a time, kept as it differs from its default) */
		{
			char name[32];
			long value, minimum, maximum, step;

			CHECK(port_mutators_option_count() == 1 &&
				port_mutators_option(0, name, sizeof(name), &value, &minimum, &maximum, &step) &&
				!strcmp(name, "SPEED") && value == 100 && minimum == 25 && maximum == 200 && step == 25, "the option");
			CHECK(!port_mutators_option_set(0, 110) && !port_mutators_option_set(0, 225), "a percent not a step");
			CHECK(port_mutators_option_set(0, 200) && !strcmp(option_setting, "paint.speed=200"), "%s", option_setting);
			size = port_mutators_tag_edits(edits, sizeof(edits));
			CHECK(mutator_tags_play(edits, size) && bullet.projectile.initial_velocity == 90.f / TICKS_PER_SECOND,
				"the speed at 200%%");
			CHECK(port_mutators_option_set(0, 100) && !option_setting[0], "the default kept: %s", option_setting);
		}
		/* (each game's undone: the map's own again, and no death) */
		index = death_clears;
		mutator_tags_undo();
		CHECK(splat.framebuffer_blend_function == 1 && splat.flags == 8 && splat.map.index == NONE &&
			responses[0].default_effect.index == NONE && death_clears == index + 1 && !death_count &&
			bullet.object.model.index == NONE && explosion_decal_index == NONE && explosion_clears > 0, "undone");
		for (index = 0; index < (int)NUMBEROF(cases); index++)
			folder(cases[index].id, cases[index].text);
		port_mutators_scan();
		for (index = 0; index < (int)NUMBEROF(cases); index++)
		{
			CHECK(port_mutators_status(find(cases[index].id), NULL, 0) == _port_mutator_unplayable &&
				strstr(refusal(cases[index].id), cases[index].reason), "%s: %s", cases[index].id,
				refusal(cases[index].id));
		}
	}
	else if (!strcmp(case_name, "bytes"))
	{
		static unsigned char changed[MAXIMUM_MUTATOR_TAG_EDIT_BYTES];
		long first;

		size = edits_of("accurized_ma5b", edits, sizeof(edits));
		/* (the first edit's size: its header, four texts and no elements) */
		first = 8;
		for (index = 0; index < 4; index++)
			first += (long)strlen((char const *)edits + first) + 1;
		CHECK(mutator_tags_play(edits, first) && mutator_tags.made == 1, "one edit");
		mutator_tags_undo();
		CHECK(!mutator_tags_play(edits, first - 1) && unchanged(&stock), "an edit cut short");
		CHECK(!mutator_tags_play(edits, size - 1) && unchanged(&stock), "the last edit cut short: the others made");
		memcpy(changed, edits, size);
		changed[0] = NUMBER_OF_MUTATOR_TAG_EDIT_KINDS;
		CHECK(!mutator_tags_play(changed, size) && unchanged(&stock), "a kind this build has not");
		memcpy(changed, edits, size);
		changed[3] = 1;
		CHECK(!mutator_tags_play(changed, size) && unchanged(&stock), "the unused byte set");
		memcpy(changed, edits, size);
		changed[1] = MAXIMUM_MUTATOR_TAG_COPY_ELEMENTS + 1;
		CHECK(!mutator_tags_play(changed, size) && unchanged(&stock), "too many elements");
		/* (an edit's text not ended within its most bytes) */
		memset(changed, 0, sizeof(changed));
		changed[0] = _mutator_tag_edit_number;
		memset(changed + 8, 'a', MUTATOR_TAG_GROUP_LENGTH + 1);
		CHECK(!mutator_tags_play(changed, sizeof(changed)) && unchanged(&stock), "a group not ended");
		/* (a value out of range on the wire, as a host of a fork could send: the
		first edit is zoom_level_count, of 0 to 8) */
		memcpy(changed, edits, size);
		CHECK(!strcmp((char const *)changed + 8 + strlen("weapon") + 1 + strlen((char const *)changed + 8 +
			strlen("weapon") + 1) + 1, "zoom_level_count"), "the first edit");
		{
			float value = 9.f;
			unsigned long bits;

			memcpy(&bits, &value, sizeof(bits));
			changed[4] = (unsigned char)bits;
			changed[5] = (unsigned char)(bits >> 8);
			changed[6] = (unsigned char)(bits >> 16);
			changed[7] = (unsigned char)(bits >> 24);
		}
		CHECK(!mutator_tags_play(changed, size) && unchanged(&stock), "a value out of range on the wire");
		CHECK(!mutators_tag_edit_check(edits, size, NULL, 0), "more than one edit checked as one");
		CHECK(mutators_tag_edit_check(edits, first, NULL, 0), "one edit refused");
	}
	else
	{
		CHECK(0, "no case %s", case_name);
	}
	return 0;
}
