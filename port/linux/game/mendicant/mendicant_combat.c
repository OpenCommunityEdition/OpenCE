/*
MENDICANT_COMBAT.C

MENDICANT AI's combat (M4, with mendicant_vehicles.c): how an actor under
orders fights once the game has set its orders, and how every actor aims,
picks targets and throws grenades while the AI runs.

- Roles: after the game's control of a fight, uncover or guard action, an
  ordered actor's role (its part in its unit's drill) changes how it moves
  and fires: crouched support fire at where the enemy was last seen, a low
  silent flank, an upright assault with a grenade first, an ambush that
  holds fire, a marksman's aimed shots, a base of fire that lifts as its
  assaulters close, a grenadier, a grenade runner.
- Grenades: a role only asks for a grenade; it is thrown through the game's
  own test and throw within two seconds, or dropped. A throw the game
  switched on that would land under 3.5 world units from the thrower or a
  living friend is refused (mendicant_grenade_throw_check).
- Aim: each actor's aiming error and burst origin are set just before it
  decides, from ai.mendicant.aim, the difficulty, its movement and how long
  it has had its target in sight. The error belongs to the actor's firing
  variant (shared tag data), so the original is kept and each actor's own
  value is written from it. A marksman with a plasma pistol charges his
  shots; Marines with an assault rifle fire longer bursts with shorter
  pauses (ai.mendicant.marine_fire).
- Targets: Marines go for Elites unless a Grunt or Jackal is hurting them,
  Elites for the Master Chief, Grunts for the Marines unless the Chief is
  hurting them or nearest.
- Soldiers on their own: Covenant no Elite commands and Marines no squad
  holds ask their per-enemy model, an encounter's free soldiers together,
  every two seconds while they have an enemy; its stance head picks the
  role they fight by.

Ported from the ai_brain hook library (ai_brain.c's grenades, apply_role,
aim, targeting and individuals_update) with the same behaviour.
*/

#include "cseries.h"
#include "memory/data.h"
#include "game/game.h"
#include "ai/ai.h"
#include "ai/actors.h"
#include "ai/actor_definitions.h"
#include "ai/actions.h"
#include "ai/props.h"
#include "objects/objects.h"
#include "units/units.h"
#include "tag_files/tag_files.h"

#include "mendicant_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* port/linux/src/port_config.c */
void platform_log(char const *format, ...);

/* not in a header: declared as actions.c declares them */
extern boolean actor_action_test_grenade(long actor_index);
extern boolean actor_action_try_to_throw_grenade(long actor_index, boolean known_trajectory);

/* ---------- constants */

/* a role's grenade waits this long for the game's own test to pass */
#define GRENADE_WAIT_TICKS 60
/* a throw planned to land nearer the thrower or a living friend is refused
(in fights, 26% of the grenades going off under 3 world units from their
thrower hurt it, 1-3% further out) */
#define GRENADE_SAFE_RADIUS 3.5f
/* the firing variants whose originals are kept */
#define AIM_VARIANTS 96
/* how often an encounter's free soldiers ask their models */
#define INDIVIDUAL_TICKS 60

#define combat_minimum(a, b) ((a) < (b) ? (a) : (b))
#define combat_maximum(a, b) ((a) > (b) ? (a) : (b))

/* ---------- globals */

/* a role's grenade waiting for the game's own plan (its time), or -1 */
static long grenade_wait_since[MENDICANT_MAX_ACTORS];

/* each firing variant's own values, and what was last written there: a
value that is not ours any more means the game loaded the level's tags
again, and the new value is the original. Kept across maps for that
reason. */
static struct
{
	struct actor_variant_definition *variant;
	real error, radius, written_error, written_radius;
	boolean special_saved;
	short special_mode, special_situation;
	real special_chance, special_delay;
	real burst_duration[2], burst_separation[2];
} aim_original[AIM_VARIANTS];
static int aimed_count;
static int aim_debug_lines;
static long aim_target[MENDICANT_MAX_ACTORS], aim_target_since[MENDICANT_MAX_ACTORS];
/* more accurate on Heroic and Legendary, less on Easy */
static const real aim_difficulty[4] = { 1.4f, 1.0f, 0.8f, 0.65f };

static long individual_next[MENDICANT_MAX_ENCOUNTERS];
static int individual_cursor;

/* the fighting styles a soldier's own model chooses among */
static const int stance_roles[STANCES] = { ROLE_NONE, ROLE_SUPPORT, ROLE_ASSAULT, ROLE_FLANKER, ROLE_MARKSMAN };

/* ---------- private code */

static struct actor_datum *combat_actor(long actor_index)
{
	if (actor_index == NONE || !actor_data || !actor_data->valid)
		return NULL;
	return (struct actor_datum *)datum_try_and_get(actor_data, actor_index);
}

/* the actor in an absolute slot, empty or not */
static struct actor_datum *combat_actor_at(int index)
{
	return (struct actor_datum *)((char *)actor_data->data + index * actor_data->size);
}

static struct prop_datum *combat_prop(long prop_index)
{
	if (prop_index == NONE || !prop_data || !prop_data->valid)
		return NULL;
	return (struct prop_datum *)datum_try_and_get(prop_data, prop_index);
}

/* an object of any type, read as a unit as the hook library read it */
static struct unit_datum *combat_unit(long object_index)
{
	if (object_index == NONE || !object_header_data || !object_header_data->valid)
		return NULL;
	return (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_all);
}

static char const *combat_tag_name(long tag_index)
{
	char const *name;

	if (tag_index == NONE)
		return "?";
	name = tag_get_name(tag_index);
	return name ? name : "?";
}

/* the tag name of the weapon a unit holds, or NULL */
static char const *combat_weapon_name(struct unit_datum *unit)
{
	short slot = unit->unit.current_weapon_index;
	struct unit_datum *weapon = slot >= 0 && slot < 4 ? combat_unit(unit->unit.weapon_object_indices[slot]) : NULL;

	return weapon ? combat_tag_name(weapon->definition_index) : NULL;
}

/* A grenade only where it can go: the enemy 6-12 world units away (AI throws
land 1-12 off), no friend within 5 of where it is aimed, and nothing in the
first 5 of the throw (a cover lip right in front would drop it at the
thrower's feet). */
static boolean grenade_clear(struct actor_datum *actor, struct prop_datum *prop)
{
	real range;
	real_point3d const *target;
	real_point3d from, to;
	real dx, dy, dz, length;
	int side, index, count;
	struct unit_datum *chief;

	if (!prop)
		return FALSE;
	range = prop->distance;
	if (range < 6.0f || range > 12.0f)
		return FALSE;
	count = actor_data->count;
	side = mendicant_unit_side(actor);
	target = &prop->last_perceived_body_position;
	for (index = 0; index < count; index++)
	{
		struct actor_datum *other = combat_actor_at(index);
		real x, y;

		if (!other->identifier || other == actor || mendicant_unit_side(other) != side)
			continue;
		x = other->input.pathfinding_point.x - target->x;
		y = other->input.pathfinding_point.y - target->y;
		if (x * x + y * y < 5.0f * 5.0f)
			return FALSE;
	}
	if (side == SIDE_MARINES)
	{
		/* nor on any player (every one in co-op) */
		long players[MENDICANT_MAX_PLAYERS];
		int player_count = mendicant_player_units(players, MENDICANT_MAX_PLAYERS);
		int player;

		for (player = 0; player < player_count; player++)
		{
			real x, y;

			chief = combat_unit(players[player]);
			if (!chief)
				continue;
			x = chief->object.position.x - target->x;
			y = chief->object.position.y - target->y;
			if (x * x + y * y < 5.0f * 5.0f)
				return FALSE;
		}
	}
	from.x = actor->input.pathfinding_point.x;
	from.y = actor->input.pathfinding_point.y;
	from.z = actor->input.pathfinding_point.z + 1.0f;
	dx = prop->last_perceived_body_position.x - from.x;
	dy = prop->last_perceived_body_position.y - from.y;
	dz = prop->last_perceived_body_position.z + 0.5f - from.z;
	length = (real)sqrt(dx * dx + dy * dy + dz * dz) + 0.001f;
	to.x = from.x + dx / length * 5.0f;
	to.y = from.y + dy / length * 5.0f;
	to.z = from.z + dz / length * 5.0f + 1.0f;
	return mendicant_line_blocked(&from, &to) == 0;
}

/* The game throws along the arc actor_action_test_grenade planned (the
control's grenade target, aim vector and speed): setting throw_grenade alone
throws along whatever is left there, which for an actor that never planned a
throw is nothing. So a role only asks for a grenade; it is thrown once the
game's own test passes, through the game's own throw. */
static void grenade_ask(int index)
{
	grenade_wait_since[index] = mendicant_time();
	actor_grenade_due[index] = 0;
}

static void grenade_wait(long actor_index, int index, struct actor_datum *actor)
{
	long time;

	if (grenade_wait_since[index] < 0)
		return;
	time = mendicant_time();
	if (time < grenade_wait_since[index] || time - grenade_wait_since[index] > GRENADE_WAIT_TICKS)
	{
		grenade_wait_since[index] = -1;
		return;
	}
	if (actor->orders.combat.throw_grenade || actor->control.grenade_trying_to_throw)
		return; /* already throwing */
	if (!actor_action_test_grenade(actor_index))
	{
		/* the game found no target it would throw at (not seen just now,
		outside its own grenade range, or the encounter threw lately): the
		role's grenade still goes to where the enemy was last seen, if the
		game's arc planner finds a clear arc there */
		struct prop_datum *prop = combat_prop(actor->target.target_prop_index);
		struct actor_variant_definition *variant;
		real_point3d point;

		if (!prop || actor->input.vehicle_index != NONE)
			return;
		point = prop->last_perceived_body_position;
		point.z += 0.2f;
		variant = actor_variant_definition_get(actor->meta.variant_definition_index);
		if (!actor_combat_plan_grenade_trajectory(actor_index, variant->grenade_combat.trajectory_type, &point,
			actor->target.target_prop_index, prop->vehicle_index))
			return;
	}
	actor->control.grenade_trying_to_throw = TRUE;
	/* carries mendicant_grenade_throw_check through its source hook */
	actor_action_try_to_throw_grenade(actor_index, TRUE);
	grenade_wait_since[index] = -1;
}

static real aim_factor(long actor_index, struct actor_datum *actor)
{
	real factor = mendicant_config.aim;
	int index;

	if (mendicant_config.aim_difficulty)
	{
		short difficulty = game_difficulty_level_get();

		if (difficulty >= 0 && difficulty < 4)
			factor *= aim_difficulty[difficulty];
	}
	if (mendicant_config.aim_moving)
	{
		struct unit_datum *unit = combat_unit(actor->meta.unit_index);
		struct unit_datum *vehicle = unit && unit->object.parent_object_index != NONE ?
			combat_unit(unit->object.parent_object_index) : NULL;

		if (vehicle)
		{
			/* riding (a Warthog's gunner): as steady as the vehicle is
			driven, by its speed and how hard it turns; 0.7 parked or
			cruising straight, up to 1.5 bouncing and swerving */
			real x = vehicle->object.translational_velocity.i;
			real y = vehicle->object.translational_velocity.j;
			real z = vehicle->object.translational_velocity.k;
			real speed = (real)sqrt(x * x + y * y + z * z) * 30.0f;
			real wx = vehicle->object.angular_velocity.i;
			real wy = vehicle->object.angular_velocity.j;
			real wz = vehicle->object.angular_velocity.k;
			real turning = (real)sqrt(wx * wx + wy * wy + wz * wz) * 30.0f; /* radians a second */
			real rough = combat_minimum(combat_maximum(speed / 15.0f + turning / 2.0f, 0.0f), 1.0f);

			factor *= 0.7f + 0.8f * rough;
		}
		else if (unit)
		{
			real x = unit->object.translational_velocity.i, y = unit->object.translational_velocity.j;
			real speed = (real)sqrt(x * x + y * y) * 30.0f; /* world units a second */
			real moving = speed < 0.3f ? 0.0f : speed > 2.0f ? 1.0f : (speed - 0.3f) / 1.7f;

			factor *= 0.75f + 0.75f * moving;
		}
	}
	/* time on target: wild at a target just seen, settling the longer it
	stays in sight (1.4 at first, 1.0 after 1.5 s, 0.7 from 4 s) */
	index = (short)actor_index;
	if (index >= 0 && index < MENDICANT_MAX_ACTORS && index < actor_data->count)
	{
		long time = mendicant_time();
		long target = actor->target.target_prop_index;
		real seconds, settle;

		if (actor->situation.visible_reachable_enemies <= 0 || target == NONE || target != aim_target[index])
		{
			aim_target[index] = actor->situation.visible_reachable_enemies > 0 ? target : NONE;
			aim_target_since[index] = time;
		}
		seconds = (time - aim_target_since[index]) / 30.0f;
		settle = seconds < 1.5f ? 1.4f - 0.4f * seconds / 1.5f :
			seconds < 4.0f ? 1.0f - 0.3f * (seconds - 1.5f) / 2.5f : 0.7f;
		factor *= settle;
	}
	return factor;
}

static int prop_actor_type(struct prop_datum *prop)
{
	struct actor_datum *actor = prop->actor_index == NONE ? NULL : combat_actor(prop->actor_index);

	return actor ? actor->meta.type : -1;
}

/* ---------- public code */

void mendicant_combat_reset(
	void)
{
	int index;

	for (index = 0; index < MENDICANT_MAX_ACTORS; index++)
		grenade_wait_since[index] = -1;
	memset(aim_target, 0, sizeof(aim_target));
	memset(aim_target_since, 0, sizeof(aim_target_since));
	memset(individual_next, 0, sizeof(individual_next));
	individual_cursor = 0;
	mendicant_vehicles_reset();
}

/* The game's own collateral check counts only the friends the thrower knows
about (its props), and the grenade lands a second after the decision. */
boolean mendicant_grenade_lands_on_friends(
	struct actor_datum *actor)
{
	real_point3d const *target = &actor->control.grenade_current_target;
	real x = target->x - actor->input.pathfinding_point.x;
	real y = target->y - actor->input.pathfinding_point.y;
	real z = target->z - actor->input.pathfinding_point.z;
	int index, count, side;

	if (x * x + y * y + z * z < GRENADE_SAFE_RADIUS * GRENADE_SAFE_RADIUS)
		return TRUE;
	count = actor_data->count;
	side = mendicant_unit_side(actor);
	for (index = 0; index < count; index++)
	{
		struct actor_datum *other = combat_actor_at(index);
		struct unit_datum *unit;

		if (!other->identifier || other == actor || mendicant_unit_side(other) != side)
			continue;
		unit = combat_unit(other->meta.unit_index);
		if (!unit || unit->object.body_vitality <= 0.0f)
			continue;
		x = target->x - unit->object.position.x;
		y = target->y - unit->object.position.y;
		z = target->z - unit->object.position.z;
		if (x * x + y * y + z * z < GRENADE_SAFE_RADIUS * GRENADE_SAFE_RADIUS)
			return TRUE;
	}
	return FALSE;
}

/* The body of mendicant_grenade_throw_checked (the end of
actor_action_try_to_throw_grenade): a throw this call switched on that would
land on the thrower or a friend is refused. */
void mendicant_grenade_throw_check(
	long actor_index,
	boolean throw_ordered_before)
{
	struct actor_datum *actor = combat_actor(actor_index);

	if (actor && !throw_ordered_before && actor->orders.combat.throw_grenade &&
		mendicant_grenade_lands_on_friends(actor))
	{
		actor->orders.combat.throw_grenade = FALSE;
		actor->control.grenade_trying_to_throw = FALSE;
	}
}

/* After the game sets an ordered actor's orders for its fight, uncover or
guard action, its role changes how it moves and fires. */
void mendicant_apply_role(
	long actor_index)
{
	int index = (short)actor_index;
	int role;
	struct actor_datum *actor;
	struct prop_datum *prop;
	boolean sees;

	if (index < 0 || index >= MENDICANT_MAX_ACTORS || order_actor[index] != actor_index)
		return;
	role = actor_role[index];
	actor = combat_actor(actor_index);
	if (!actor || role == ROLE_NONE)
		return;
	prop = combat_prop(actor->target.target_prop_index);
	sees = actor->situation.visible_reachable_enemies > 0;

	switch (role)
	{
	case ROLE_SUPPORT:
		/* pins the enemy: crouched, firing long bursts, at where it was last
		seen if not in sight */
		actor->orders.move.stationary_crouch = TRUE;
		actor->orders.combat.shoot_at_target = TRUE;
		actor->orders.combat.override_firing_restrictions = TRUE;
		actor->orders.combat.override_burst_duration = 1.5f;
		if (!sees && prop)
		{
			actor->orders.combat.use_manual_target_point = TRUE;
			actor->orders.combat.target_point = prop->last_perceived_body_position;
			actor->orders.combat.target_point.z += 0.6f; /* chest high */
		}
		break;
	case ROLE_FLANKER:
		/* gets round unseen: low, and not giving itself away */
		actor->orders.move.moving_crouch = TRUE;
		if (!sees)
			actor->orders.combat.shoot_at_target = FALSE;
		break;
	case ROLE_ASSAULT:
		/* goes in fast and upright, a grenade first */
		actor->orders.move.moving_crouch = FALSE;
		actor->orders.move.stationary_crouch = FALSE;
		if (actor_grenade_due[index] && sees && grenade_clear(actor, prop))
			grenade_ask(index);
		break;
	case ROLE_HIDDEN:
		/* the ambush: crouched and silent until the enemy is in the kill zone
		(12 world units), unless it has found them first (they are being hit) */
		actor->orders.move.stationary_crouch = TRUE;
		if ((!prop || prop->distance > 12.0f) && actor->input.recent_body_damage <= 0.0f &&
			actor->input.recent_shield_damage <= 0.0f)
			actor->orders.combat.shoot_at_target = FALSE;
		break;
	case ROLE_MARKSMAN:
		/* the support element's marksman: crouched in cover, deliberate shots
		at what it sees */
		actor->orders.move.stationary_crouch = TRUE;
		actor->orders.combat.override_firing_restrictions = TRUE;
		actor->orders.combat.override_burst_duration = 0.6f;
		actor->orders.combat.shoot_at_target = sees;
		break;
	case ROLE_BAIT:
		actor->orders.combat.shoot_at_target = TRUE;
		break;
	case ROLE_SECURITY:
	case ROLE_OVERWATCH:
		/* watches its sector: crouched, firing only at what it sees, looking
		where it was told */
		actor->orders.move.stationary_crouch = TRUE;
		actor->orders.combat.override_firing_restrictions = TRUE;
		actor->orders.combat.override_burst_duration = role == ROLE_OVERWATCH ? 1.5f : 0.8f;
		if (!sees)
		{
			actor->orders.combat.shoot_at_target = FALSE;
			actor->orders.combat.use_manual_target_point = TRUE;
			actor->orders.combat.target_point.x = actor_face[index][0];
			actor->orders.combat.target_point.y = actor_face[index][1];
			actor->orders.combat.target_point.z = actor_face[index][2];
		}
		break;
	case ROLE_SHIFT_FIRE:
	{
		/* a base of fire that lifts as its own assaulters close on the enemy */
		real closing = unit_assault_range[index];

		actor->orders.move.stationary_crouch = TRUE;
		actor->orders.combat.override_firing_restrictions = TRUE;
		actor->orders.combat.override_burst_duration = 1.2f;
		actor->orders.combat.shoot_at_target = closing > 4.0f;
		if (prop && closing < 8.0f)
		{
			actor->orders.combat.use_manual_target_point = TRUE;
			actor->orders.combat.target_point = prop->last_perceived_body_position;
			actor->orders.combat.target_point.z += 3.0f; /* over their heads, behind the enemy */
		}
		break;
	}
	case ROLE_GRENADIER:
		actor->orders.move.stationary_crouch = TRUE;
		actor->orders.combat.shoot_at_target = TRUE;
		if (actor_grenade_due[index] && grenade_clear(actor, prop))
			grenade_ask(index);
		break;
	case ROLE_RUNNER:
		/* the grenade runner: upright and fast, and he throws at the last moment */
		actor->orders.move.moving_crouch = FALSE;
		actor->orders.move.stationary_crouch = FALSE;
		if (prop && prop->distance < 3.0f)
			actor->orders.combat.throw_grenade = TRUE;
		break;
	}
	grenade_wait(actor_index, index, actor);
}

boolean mendicant_carries_plasma_pistol(
	struct unit_datum *unit)
{
	char const *name = combat_weapon_name(unit);

	return name && strstr(name, "plasma pistol") != NULL;
}

boolean mendicant_carries_assault_rifle(
	struct unit_datum *unit)
{
	char const *name = combat_weapon_name(unit);

	return name && strstr(name, "assault rifle") != NULL;
}

/* What sort of weapon a unit carries, from its tag's name: 0 none or
unknown, then close range, sniper, explosive, plasma, rifle. */
int mendicant_weapon_class(
	struct unit_datum *unit)
{
	char const *name = combat_weapon_name(unit);

	if (!name)
		return 0;
	if (strstr(name, "shotgun") || strstr(name, "flame") || strstr(name, "sword"))
		return 1;
	if (strstr(name, "sniper"))
		return 2;
	if (strstr(name, "rocket") || strstr(name, "fuel") || strstr(name, "cannon"))
		return 3;
	if (strstr(name, "plasma") || strstr(name, "needler"))
		return 4;
	return 5;
}

/* Each actor's aim, set just before it decides. */
void mendicant_aim_actor(
	long actor_index)
{
	struct actor_datum *actor = combat_actor(actor_index);
	struct actor_variant_definition *variant;
	struct unit_datum *unit;
	int slot, index, bound;
	real error, radius, factor;
	boolean charge, sustained;
	real marine_fire;

	if (!actor || (mendicant_config.aim == 1.0f && !mendicant_config.aim_difficulty && !mendicant_config.aim_moving))
		return;
	variant = actor_combat_get_firing_variant_definition(actor_index);
	if (!variant)
		return;
	slot = 0;
	while (slot < aimed_count && aim_original[slot].variant != variant)
		slot++;
	error = variant->ranged_combat.projectile_error_angle;
	radius = variant->ranged_combat.burst_geometry.burst_origin_radius;
	if (slot == aimed_count)
	{
		if (aimed_count == AIM_VARIANTS)
			slot = mendicant_drill_rand() % AIM_VARIANTS; /* full: one is forgotten */
		else
			aimed_count++;
		aim_original[slot].variant = variant;
		aim_original[slot].error = error;
		aim_original[slot].radius = radius;
		aim_original[slot].special_saved = FALSE;
	}
	else if (error != aim_original[slot].written_error || radius != aim_original[slot].written_radius)
	{
		aim_original[slot].error = error;
		aim_original[slot].radius = radius;
	}
	factor = aim_factor(actor_index, actor);
	aim_original[slot].written_error = aim_original[slot].error * factor;
	aim_original[slot].written_radius = aim_original[slot].radius * factor;
	if (mendicant_config.aim_debug && aim_debug_lines < 40)
	{
		aim_debug_lines++;
		platform_log("mendicant: aim actor %d variant %p error %.4f -> %.4f (factor %.2f)",
			(short)actor_index, (void *)variant, aim_original[slot].error, aim_original[slot].written_error, factor);
	}
	variant->ranged_combat.projectile_error_angle = aim_original[slot].written_error;
	variant->ranged_combat.burst_geometry.burst_origin_radius = aim_original[slot].written_radius;
	/* a marksman with a plasma pistol charges every shot he has a clear
	sight for; anyone else fires as the game has it */
	if (!aim_original[slot].special_saved)
	{
		aim_original[slot].special_mode = variant->ranged_combat.special_fire_mode;
		aim_original[slot].special_situation = variant->ranged_combat.special_fire_situation;
		aim_original[slot].special_chance = variant->ranged_combat.special_fire_chance;
		aim_original[slot].special_delay = variant->ranged_combat.special_fire_delay;
		aim_original[slot].burst_duration[0] = variant->ranged_combat.burst_geometry.burst_duration_lower_bound;
		aim_original[slot].burst_duration[1] = variant->ranged_combat.burst_geometry.burst_duration_upper_bound;
		aim_original[slot].burst_separation[0] = variant->ranged_combat.burst_geometry.burst_separation_lower_bound;
		aim_original[slot].burst_separation[1] = variant->ranged_combat.burst_geometry.burst_separation_upper_bound;
		aim_original[slot].special_saved = TRUE;
	}
	index = (short)actor_index;
	unit = combat_unit(actor->meta.unit_index);
	charge = index >= 0 && index < MENDICANT_MAX_ACTORS && actor_role[index] == ROLE_MARKSMAN && unit &&
		mendicant_carries_plasma_pistol(unit);
	variant->ranged_combat.special_fire_mode = charge ? 1 : aim_original[slot].special_mode;
	variant->ranged_combat.special_fire_situation = charge ? 1 : aim_original[slot].special_situation;
	variant->ranged_combat.special_fire_chance = charge ? 1.0f : aim_original[slot].special_chance;
	variant->ranged_combat.special_fire_delay = charge ? 0.4f : aim_original[slot].special_delay;
	/* Marines with an assault rifle fire longer bursts with shorter pauses:
	sustained fire that keeps the enemy down */
	marine_fire = combat_maximum(0.1f, mendicant_config.marine_fire);
	sustained = marine_fire != 1.0f && mendicant_side_of(actor->meta.team_index) == TEAM_HUMAN && unit &&
		mendicant_carries_assault_rifle(unit);
	for (bound = 0; bound < 2; bound++)
	{
		real duration = aim_original[slot].burst_duration[bound] * (sustained ? marine_fire : 1.0f);
		real separation = aim_original[slot].burst_separation[bound] / (sustained ? marine_fire : 1.0f);

		if (bound == 0)
		{
			variant->ranged_combat.burst_geometry.burst_duration_lower_bound = duration;
			variant->ranged_combat.burst_geometry.burst_separation_lower_bound = separation;
		}
		else
		{
			variant->ranged_combat.burst_geometry.burst_duration_upper_bound = duration;
			variant->ranged_combat.burst_geometry.burst_separation_upper_bound = separation;
		}
	}
}

/* The weight the game gave a target an actor knows of (the return of
actor_compute_prop_target_weight), scaled by who goes after whom. */
real mendicant_targeting_scale(
	long actor_index,
	long prop_index,
	real weight)
{
	struct actor_datum *actor = combat_actor(actor_index);
	struct prop_datum *prop = combat_prop(prop_index);
	int type, target_type;
	boolean target_is_player;
	real scale = 1.0f;

	if (!mendicant_config.targeting || !actor || !prop || !prop->enemy || weight <= 0.0f)
		return weight;
	type = actor->meta.type;
	target_type = prop_actor_type(prop);
	target_is_player = prop->player;

	if (ACTOR_IS_MARINE(type))
	{
		/* a Grunt or Jackal hurting this Marine comes first; otherwise the Elites */
		boolean pressed = FALSE;
		long other = actor->meta.first_prop_index;

		while (other != NONE)
		{
			struct prop_datum *known = combat_prop(other);
			int known_type;

			if (!known)
				break;
			known_type = prop_actor_type(known);
			if (known->currently_damaging_me && (ACTOR_IS_GRUNT(known_type) || ACTOR_IS_JACKAL(known_type)))
				pressed = TRUE;
			other = known->next_prop_index;
		}
		if (pressed)
			scale = prop->currently_damaging_me && (ACTOR_IS_GRUNT(target_type) || ACTOR_IS_JACKAL(target_type)) ?
				2.0f : 1.0f;
		else if (ACTOR_IS_ELITE(target_type))
			scale = 2.5f;
	}
	else if (ACTOR_IS_ELITE(type))
	{
		if (target_is_player)
			scale = 3.0f;
	}
	else if (ACTOR_IS_GRUNT(type))
	{
		if (target_is_player)
		{
			/* the Chief only if he is hurting them, or nearer than any Marine
			they know of */
			real nearest_marine = 1e30f;
			long other = actor->meta.first_prop_index;

			while (other != NONE)
			{
				struct prop_datum *known = combat_prop(other);

				if (!known)
					break;
				if (ACTOR_IS_MARINE(prop_actor_type(known)) && known->distance < nearest_marine)
					nearest_marine = known->distance;
				other = known->next_prop_index;
			}
			scale = prop->currently_damaging_me || prop->distance < nearest_marine ? 1.5f : 0.5f;
		}
		else if (ACTOR_IS_MARINE(target_type))
		{
			scale = 1.5f;
		}
	}
	return weight * scale;
}

/* Soldiers on their own: Covenant with no Elite left to command them, and
Marines no squad holds, are not left to the stock AI while the models play:
each one's own small model chooses where he goes among the ground near him,
and its stance head how he fights there. An encounter's free soldiers ask
together, each encounter every 2 s, while they have an enemy to fight. */
void mendicant_individuals_update(
	long time)
{
	int count, pass;

	if (!mendicant_bridge || mendicant_bridge_waiting() || !mendicant_config.individuals)
		return;
	if (!actor_data || !actor_data->valid)
		return;
	count = actor_data->count;
	/* the next encounter (from the cursor on) with free soldiers in a
	fight, due a decision */
	for (pass = 0; pass < MENDICANT_MAX_ENCOUNTERS; pass++)
	{
		int encounter = (individual_cursor + pass) % MENDICANT_MAX_ENCOUNTERS;

		if (time >= individual_next[encounter] || time < individual_next[encounter] - INDIVIDUAL_TICKS)
		{
			static struct mendicant_member members[MENDICANT_MAX_SQUAD];
			int found = 0, index, member;
			struct prop_datum *target_prop = NULL;
			real_point3d const *target;
			real_point3d centre;

			for (index = 0; index < count && index < MENDICANT_MAX_ACTORS && found < MENDICANT_MAX_SQUAD; index++)
			{
				struct actor_datum *actor = combat_actor_at(index);
				struct prop_datum *prop;
				int side;

				if (!actor->identifier || (short)actor->meta.encounter_index != encounter ||
					commanded_time[index] == time || !actor->meta.active || actor->input.body_vitality <= 0.0f ||
					actor->meta.unit_index == mendicant_player_unit() || !mendicant_actor_can_drill(actor) ||
					mendicant_team_mode(actor->meta.team_index) != MODE_MODEL)
					continue;
				side = mendicant_side_of(actor->meta.team_index);
				if (side != TEAM_HUMAN && side != TEAM_COVENANT)
					continue;
				prop = combat_prop(actor->target.target_prop_index);
				if (!prop)
					continue; /* no enemy to fight: the game's own idling and patrols */
				if (!target_prop)
					target_prop = prop;
				members[found].actor_index = ((long)actor->identifier << 16) | index;
				members[found].actor = actor;
				found++;
			}
			if (!found || !target_prop)
				continue;
			individual_cursor = (encounter + 1) % MENDICANT_MAX_ENCOUNTERS;
			individual_next[encounter] = time + INDIVIDUAL_TICKS;
			centre.x = centre.y = centre.z = 0.0f;
			for (member = 0; member < found; member++)
			{
				centre.x += members[member].actor->input.pathfinding_point.x;
				centre.y += members[member].actor->input.pathfinding_point.y;
				centre.z += members[member].actor->input.pathfinding_point.z;
			}
			centre.x /= found;
			centre.y /= found;
			centre.z /= found;
			target = &target_prop->last_perceived_body_position;
			mendicant_bridge->plan_request = 0;
			mendicant_bridge_request(MENDICANT_MAX_ACTORS + encounter, members, found, target,
				centre.x - target->x, centre.y - target->y, FALSE, time);
			return; /* one request at a time */
		}
	}
}

/* A soldier on his own: his model's way of fighting, through the role that
makes it (bridge_apply's stance branch, for a request with team[7] < 0.5;
stance 0 or one out of range is the game's own, no role). */
void mendicant_apply_stance(
	long actor_index,
	int stance)
{
	mendicant_set_role(actor_index, stance > 0 && stance < STANCES ? stance_roles[stance] : ROLE_NONE);
}
