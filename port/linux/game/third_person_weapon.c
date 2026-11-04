/* The host and predicting clients solve shots from the same world markers. */
#include "cseries.h"
#include "third_person.h"
#include "camera/director.h"
#include "game/game.h"
#include "game/players.h"
#include "units/units.h"
#include "physics/collisions.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "units/unit_definitions.h"
#include "effects/effect_definitions.h"

char const *config_string(char const *name);
void platform_log(char const *format, ...);

static boolean muzzle_player(short player, long unit_index)
{
	struct unit_datum *unit;
	director_perspective perspective;
	if (!third_person_active(player) ||
		unit_index == NONE || player_control_get_unit_index(player) != unit_index ||
		player_control_get_zoom_level(player) != NONE)
		return FALSE;
	unit = unit_try_and_get(unit_index);
	return unit && unit->object.parent_object_index == NONE &&
		!TEST_FLAG(unit->object.damage_flags, _object_dead_bit) &&
		!director_desired_perspective(unit_index, &perspective);
}

static boolean muzzle_network_player(long unit_index)
{
	struct unit_datum *unit = unit_index != NONE ? unit_try_and_get(unit_index) : NULL;
	director_perspective perspective;
	return unit && unit->unit.player_index != NONE &&
		unit->object.parent_object_index == NONE &&
		!TEST_FLAG(unit->object.damage_flags, _object_dead_bit) &&
		unit->unit.current_zoom_level == NONE &&
		third_person_policy() != _third_person_forced_off &&
		TEST_FLAG(unit->unit.control_flags, THIRD_PERSON_INPUT_BIT) &&
		/* A stale or forged input bit cannot override an authored camera. */
		!director_desired_perspective(unit_index, &perspective);
}

static short muzzle_preview_trigger(struct weapon_datum *weapon, struct weapon_definition *definition)
{
	/* A charged secondary shot leaves a different muzzle on the pistol. */
	return definition->weapon.triggers.count > 1 &&
		((weapon->weapon.triggers[0].state == _trigger_charged &&
		  TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, 0, struct weapon_trigger_definition)->spew_time <= 0.0f) ||
		 weapon->weapon.triggers[1].state == _trigger_charging ||
		 weapon->weapon.triggers[1].state == _trigger_charged ||
		 weapon->weapon.triggers[1].state == _trigger_spewing) ? 1 : 0;
}

/* Authored alternating firing effects can name a different barrel, as
   the Plasma Rifle does. Only accept trigger markers, never smoke/ejection
   locations; missing or custom names retain the ordinary trigger marker. */
static char const *muzzle_marker(long weapon_index, short trigger_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *definition = weapon_definition_get(weapon->definition_index);
	struct weapon_trigger_definition *trigger;
	struct weapon_firing_effect_definition *firing;
	struct effect_definition *effect;
	char const *name;
	char const *fallback = trigger_index ? "secondary trigger" : "primary trigger";
	short index = weapon->weapon.triggers[trigger_index].firing_effect_index;
	if (definition->weapon.triggers.count <= trigger_index)
		return fallback;
	trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, trigger_index, struct weapon_trigger_definition);
	if (index < 0 || index >= trigger->firing_effects.count)
		return fallback;
	firing = TAG_BLOCK_GET_ELEMENT(&trigger->firing_effects, index, struct weapon_firing_effect_definition);
	if (firing->effects[0].index == NONE)
		return fallback;
	effect = (struct effect_definition *)tag_get(EFFECT_DEFINITION_TAG, firing->effects[0].index);
	if (!effect->locations.count)
		return fallback;
	/* Effect locations are 32-byte marker names. The tag loader validates
	   this block, and object_get_marker_by_name still verifies the marker. */
	name = (char const *)effect->locations.address;
	if (!strncmp(name, fallback, strlen(fallback)) &&
		(name[strlen(fallback)] == '\0' ||
		 (name[strlen(fallback)] >= '0' && name[strlen(fallback)] <= '9')))
		return name;
	return fallback;
}

static boolean muzzle_position(long weapon_index, long unit_index, short trigger_index,
	real_point3d *position, real_point3d *obstruction, boolean *blocked)
{
	struct object_marker marker;
	real_point3d eye;
	real_vector3d delta;
	real length;
	struct collision_result collision;
	*blocked = FALSE;
	if (weapon_index == NONE || trigger_index < 0 || trigger_index > 1)
		return FALSE;
	if (!object_get_marker_by_name(weapon_index, muzzle_marker(weapon_index, trigger_index), &marker, 1) &&
		!object_get_marker_by_name(weapon_index,
			trigger_index ? "secondary trigger" : "primary trigger", &marker, 1))
		return FALSE;
	unit_get_camera_position(unit_index, &eye);
	*position = marker.matrix.position;
	vector_from_points3d(&eye, position, &delta);
	length = magnitude3d(&delta);
	/* Do not let a barrel through a wall spawn a shot beyond that wall. */
	if (length > 0.001f && collision_test_vector(_collision_test_for_projectiles_flags,
		&eye, &delta, unit_index, &collision))
	{
		point_from_line3d(&eye, &delta, MAX(0.0f, collision.t - 0.01f / length), position);
		*obstruction = collision.point;
		*blocked = TRUE;
	}
	return TRUE;
}

/* Both the preview and actual shot use this solver. The input ray is the
   stock weapon ray (aim assistance has already run for a real shot). */
static boolean muzzle_aim(long weapon_index, long unit_index, short trigger_index,
	real_point3d *origin, real_vector3d *forward, real_point3d *target)
{
	real_point3d muzzle, eye, obstruction;
	real_vector3d ray;
	boolean blocked;
	struct collision_result collision;
	if (!muzzle_position(weapon_index, unit_index, trigger_index, &muzzle, &obstruction, &blocked))
		return FALSE;
	scale_vector3d(forward, 1000.0f, &ray);
	point_from_line3d(origin, &ray, 1.0f, target);
	if (collision_test_vector(_collision_test_for_projectiles_flags, origin, &ray, unit_index, &collision))
		*target = collision.point;
	/* The stock offset can start beyond nearby cover. Check the segment
	   leading to that origin as well, rather than aiming through the wall. */
	unit_get_camera_position(unit_index, &eye);
	vector_from_points3d(&eye, origin, &ray);
	if (magnitude3d(&ray) > 0.001f && collision_test_vector(_collision_test_for_projectiles_flags,
		&eye, &ray, unit_index, &collision))
		*target = collision.point;
	if (blocked)
		*target = obstruction;
	vector_from_points3d(&muzzle, target, &ray);
	/* A target behind the barrel must not turn the shot backwards. Keep
	   its original direction; the preview traces that same forward path. */
	if ((blocked || dot_product3d(&ray, forward) > 0.001f) && normalize3d(&ray) > 0.001f)
		*forward = ray;
	*origin = muzzle;
	scale_vector3d(forward, 1000.0f, &ray);
	point_from_line3d(origin, &ray, 1.0f, target);
	if (collision_test_vector(_collision_test_for_projectiles_flags, origin, &ray, unit_index, &collision))
		*target = collision.point;
	return TRUE;
}

boolean third_person_muzzle_shot(long weapon_index, long unit_index, short trigger_index,
	real_point3d *origin, real_vector3d *forward)
{
	short player;
	real_point3d target;
	if (game_connection() != _game_connection_local)
	{
		if (!muzzle_network_player(unit_index))
			return FALSE;
		if (!muzzle_aim(weapon_index, unit_index, trigger_index, origin, forward, &target))
			return FALSE;
		if (config_string("debug.network_test")[0])
			platform_log("network muzzle: player %ld trigger %d origin %.4f %.4f %.4f",
				(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_get(unit_index)->unit.player_index), (int)trigger_index,
				origin->x, origin->y, origin->z);
		return TRUE;
	}
	for (player = 0; player < MAXIMUM_LOCAL_PLAYERS; player++)
		if (muzzle_player(player, unit_index))
			return muzzle_aim(weapon_index, unit_index, trigger_index, origin, forward, &target);
	return FALSE;
}

/* The host's visibility history needs the same cover-clamped muzzle as
   shoulder shots. No client position is accepted or retained here. */
boolean third_person_muzzle_history(long unit_index, real_point3d *position)
{
	struct unit_datum *unit;
	struct weapon_datum *weapon;
	struct weapon_definition *definition;
	struct weapon_trigger_definition *trigger;
	long weapon_index;
	short trigger_index;
	real_point3d obstruction;
	boolean blocked;
	if (game_connection() != _game_connection_network_server || !muzzle_network_player(unit_index))
		return FALSE;
	unit = unit_get(unit_index);
	weapon_index = unit_inventory_get_weapon(unit_index, unit->unit.current_weapon_index);
	if (weapon_index == NONE)
		return FALSE;
	weapon = weapon_get(weapon_index);
	definition = weapon_definition_get(weapon->definition_index);
	trigger_index = muzzle_preview_trigger(weapon, definition);
	if (definition->weapon.triggers.count <= trigger_index)
		return FALSE;
	trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, trigger_index, struct weapon_trigger_definition);
	if (TEST_FLAG(trigger->flags, _weapon_trigger_uses_weapon_origin_bit) ||
		TEST_FLAG(trigger->flags, _weapon_trigger_projectiles_cannot_be_aimed_bit))
		return FALSE;
	return muzzle_position(weapon_index, unit_index, trigger_index, position, &obstruction, &blocked);
}

void third_person_muzzle_target(short player, real_point3d *target)
{
	long unit_index, weapon_index;
	short trigger_index = 0;
	struct unit_datum *unit;
	struct weapon_datum *weapon;
	struct weapon_definition *definition;
	struct weapon_trigger_definition *trigger;
	struct object_marker marker;
	real_point3d origin;
	real_vector3d forward, right, up;
	real velocity;
	unit_index = player_control_get_unit_index(player);
	if (!muzzle_player(player, unit_index))
		return;
	unit = unit_get(unit_index);
	weapon_index = unit_inventory_get_weapon(unit_index, unit->unit.current_weapon_index);
	if (weapon_index == NONE)
		return;
	weapon = weapon_get(weapon_index);
	definition = weapon_definition_get(weapon->definition_index);
	trigger_index = muzzle_preview_trigger(weapon, definition);
	if (definition->weapon.triggers.count <= trigger_index)
		return;
	trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, trigger_index, struct weapon_trigger_definition);
	/* Preserve explicit custom firing rules, as the actual shot does. */
	if (TEST_FLAG(trigger->flags, _weapon_trigger_uses_weapon_origin_bit) ||
		TEST_FLAG(trigger->flags, _weapon_trigger_projectiles_cannot_be_aimed_bit))
		return;
	if (!object_get_marker_by_name(weapon_index,
		trigger_index ? "secondary trigger" : "primary trigger", &marker, 1))
		return;
	origin = marker.matrix.position;
	forward = unit->unit.aiming_vector;
	unit_adjust_projectile_ray(unit_index, &origin, &forward, &velocity,
		TEST_FLAG(unit_definition_get(unit->definition_index)->unit.flags, _unit_fires_from_camera_bit), TRUE);
	cross_product3d(global_up3d, &forward, &right);
	if (normalize3d(&right) == 0.0f)
		right = *global_left3d;
	cross_product3d(&forward, &right, &up);
	normalize3d(&up);
	point_from_line3d(&origin, &forward, trigger->first_person_weapon_offset.x, &origin);
	point_from_line3d(&origin, &right, trigger->first_person_weapon_offset.y, &origin);
	point_from_line3d(&origin, &up, trigger->first_person_weapon_offset.z, &origin);
	muzzle_aim(weapon_index, unit_index, trigger_index, &origin, &forward, target);
}
