/*
NETWORK_DAMAGE.C

The distributed netcode's damage (port/linux/NETCODE.md, stage 5, and what
being hit looks and feels like on the clients).

- The host deals all damage. Every tick it sends its clients the damage it
  dealt to units: whom, with what, from where, how much, and to what
  effect. A client replays what that damage does beside the harm (the
  host's shields and health come with the units' states): a player's
  screen flash and shake, the unit's flinch, pain sound, knockback and
  stun, the scope it was knocked out of, and whom the HUD shows it was hit
  by. A killing blow it replays whole, so that the body falls as the shot
  had it and the kill is announced with the host's killer.
- A client deals no damage itself. What its own players' shots, grenades,
  melee and vehicles hit it reports to the host, which deals it once it has
  checked it: the report is of that machine's player, the damage one that
  player's weapons (now or lately), grenades or vehicle deal, the target
  about where the host had it when the shooter saw it (the host keeps a
  second of where players' units and vehicles were, and looks back as far
  as that machine's round trip), the impact at the target, and no more
  reports than the weapon that deals them fires (an explosion's hits are
  one). What the shooter saw hit, hits. The host's own copies of a client's
  projectiles deal nothing (the client's report does).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "effects/effect_definitions.h"
#include "physics/collision_model_definitions.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "scenario/scenario.h"
#include "units/units.h"
#include "network_distributed.h"

#include <math.h>

/* damage.c's */
void damage_replay_player_effect(long player_index, struct damage_data *damage, real total_damage);
void damage_replay_aftermath(long object_index, struct damage_data *damage, unsigned long being_damaged_flags,
	real shield_damage, real body_damage, real body_damage_multiplier, short body_part);
void damage_replay_kill(long object_index, struct damage_data *damage, short node_index, short region_index,
	short material_index);
/* network_distributed.c's */
void distributed_set_death(short dead_player_index, byte killing_player_index, boolean friendly_fire,
	boolean killed_by_vehicle);
boolean distributed_get_death(short dead_player_index, byte *killing_player_index, boolean *friendly_fire,
	boolean *killed_by_vehicle);
/* cache_files.c's */
boolean tag_index_is_group(long tag_index, long group_tag);

enum
{
	MAXIMUM_DAMAGE_EVENTS_PER_TICK = 256,
	MAXIMUM_HIT_REPORTS_PER_TICK = 256,
	MAXIMUM_ENTRIES_PER_MESSAGE = 64,
	/* the weapons a player has carried lately, whose damage the host takes
	from them (a grenade lands, a rocket arrives, after its thrower has
	died or dropped the launcher) */
	MAXIMUM_RECENT_WEAPONS = 8,
	RECENT_WEAPON_TICKS = 10 * TICKS_PER_SECOND,
	/* a player's hits: a bucket of this many seconds of fire, filling at a
	second a second, a hit taking what one of its weapon's projectiles
	takes to fire (at twice the weapon's fastest rate: a shotgun's
	pellets, a spray); an explosion's hits are one, however many it hits */
	HIT_REPORT_BURST_SECONDS = 3,
	HIT_REPORT_RATE_MARGIN = 2,
	/* the hits a second of what no trigger's rate says: melee, a
	grenade, a vehicle, a weapon's own detonation */
	MELEE_HITS_PER_SECOND = 4,
	GRENADE_HITS_PER_SECOND = 4,
	VEHICLE_HITS_PER_SECOND = 15,
	EFFECT_HITS_PER_SECOND = 4,
	/* the explosions of a tick told apart */
	MAXIMUM_EXPLOSIONS_PER_TICK = 32,
	/* what weapons and grenades deal, remembered (a power of two) */
	DAMAGE_RATE_CACHE_SIZE = 1024,
	/* the damage events kept for killing blows */
	RESERVED_KILL_EVENTS = 64,
	/* tags within tags followed looking for a damage effect */
	MAXIMUM_TAG_DEPTH = 4,
	/* the ticks of where players' units and vehicles were, a power of two
	(about a second) */
	TARGET_HISTORY_TICKS = 32,
	/* ... looked back over beyond a shooter's round trip (the frame drawn a
	tick behind, the report's own tick) */
	TARGET_HISTORY_SLACK_TICKS = 3,
	/* ... and the longest round trip looked back over (a client tells its
	own: a longer one does not look further back) */
	TARGET_HISTORY_MAXIMUM_ROUND_TRIP_TICKS = 15,
};

enum
{
	_damage_event_player_effect,
	_damage_event_aftermath,
	_damage_event_kill,
};

/* struct distributed_damage_event kill flags */
enum
{
	_damage_event_friendly_fire_bit = 0,
	_damage_event_killed_by_vehicle_bit,
};

/* world units: how far the host may have the target from where the
shooter saw it (with how far it moves in half a second), and the impact
from the target */
#define REPORT_TARGET_TOLERANCE 3.0f
#define REPORT_TARGET_LEAD_TICKS 15.0f
#define REPORT_IMPACT_TOLERANCE 2.0f
/* ... and from where the host had a player's unit or vehicle then (with how
far it moves in a few ticks: a client's copy runs a little ahead of the
host's word on it) */
#define REPORT_HISTORY_TOLERANCE 2.0f
#define REPORT_HISTORY_LEAD_TICKS 3.0f
/* how far from the origin anything in a report is (world units), and the
largest damage scale a client's hit has */
#define REPORT_WORLD_BOUND 32768.0f
#define REPORT_MAXIMUM_SCALE 1.5f

/* the damage flags a client's hit has: what its own object_cause_damage is
given (neither the host's instant kills nor its passengers' passthrough) */
#define REPORT_DAMAGE_FLAGS (FLAG(_damage_area_of_effect_bit) | FLAG(_damage_create_localized_effect_bit) | \
	FLAG(_damage_from_weapon_bit) | FLAG(_damage_damaged_one_object_bit))

/* the host's struct damage_data, as the other machines have it */
struct distributed_damage
{
	long definition_index;
	unsigned long flags;
	byte owner_player_index;
	byte pad;
	short owner_team_index;
	long owner_object_index;
	real_point3d origin;
	real_point3d epicenter;
	real_vector3d direction;
	real scale;
	real multiplier;
	real material_effect_scale;
	short material_type;
	short pad1;
};

struct distributed_damage_event
{
	byte kind;
	/* the player effect's player; the killing blow's killer */
	byte player_index;
	byte kill_flags;
	byte pad;
	long object_index;
	struct distributed_damage damage;
	unsigned long being_damaged_flags;
	real shield_damage;
	real body_damage;
	real body_damage_multiplier;
	real total_damage;
	short body_part;
	short node_index;
	short region_index;
	short material_index;
};

struct distributed_hit_report
{
	long object_index;
	struct distributed_damage damage;
	/* where the shooter had the target */
	real_point3d target_position;
	real_vector3d object_normal;
	short node_index;
	short region_index;
	short material_index;
	boolean has_normal;
	byte pad;
};

struct distributed_damage_event_message
{
	struct distributed_message_header header;
	struct distributed_damage_event events[MAXIMUM_ENTRIES_PER_MESSAGE];
};

struct distributed_hit_report_message
{
	struct distributed_message_header header;
	struct distributed_hit_report reports[MAXIMUM_ENTRIES_PER_MESSAGE];
};

/* the vehicle damage in the globals' falling damage block, as vehicles.c
has it */
struct distributed_falling_damage
{
	byte unused0[0x2c];
	struct tag_reference maximum_distance_damage;
	struct tag_reference vehicle_hit_environment_damage_effect;
	struct tag_reference vehicle_killed_unit_damage_effect;
	struct tag_reference vehicle_collision_damage;
	struct tag_reference flaming_death_damage;
};

/* ---------- globals */

/* the host: the damage dealt this tick */
static struct distributed_damage_event damage_events[MAXIMUM_DAMAGE_EVENTS_PER_TICK];
static short damage_event_count;
/* ... dealing a client's report */
static boolean damage_dealing_report;
/* ... each player's weapons of late, and hits left */
static struct
{
	long definition_indices[MAXIMUM_RECENT_WEAPONS];
	long times[MAXIMUM_RECENT_WEAPONS];
	long grenade_times[NUMBER_OF_UNIT_GRENADE_TYPES];
	long vehicle_time;
	real hit_seconds;
	long hit_seconds_time;
} damage_players[MAXIMUM_TRACKED_PLAYERS];
/* ... the explosions whose hits came this tick, each paid for once */
static struct
{
	long time;
	short count;
	struct
	{
		short player_index;
		long definition_index;
		real_point3d epicenter;
	} explosions[MAXIMUM_EXPLOSIONS_PER_TICK];
} damage_explosions;
/* ... where each player's unit and vehicle were at each of the last ticks */
static struct damage_history_tick
{
	long time;
	struct
	{
		long object_index;
		real_point3d position;
		real speed;
	} objects[MAXIMUM_TRACKED_PLAYERS][2];
} damage_history[TARGET_HISTORY_TICKS];
/* ... what each weapon or grenade deals, as the tags have it (their walk
is long): the hits a second of a damage effect from it */
static struct damage_rate_cache_entry
{
	long source_index;
	long damage_index;
	real rate;
} damage_rate_cache[DAMAGE_RATE_CACHE_SIZE];
/* for the automated tests' reports (network_test.c) */
static long damage_rejected_reports;
static long damage_dealt_reports;
static long damage_sent_reports;
static long damage_replayed_events;

/* a client: its players' hits this tick */
static struct distributed_hit_report damage_reports[MAXIMUM_HIT_REPORTS_PER_TICK];
static short damage_report_count;
/* ... replaying the host's killing blow (its player effect came before) */
static boolean damage_replaying_kill;

/* ---------- common */

word network_damage_entry_size(
	byte type)
{
	return type == _distributed_message_damage_events ?
		sizeof(struct distributed_damage_event) : sizeof(struct distributed_hit_report);
}

static void distributed_damage_from_data(
	struct damage_data const *damage,
	struct distributed_damage *result)
{
	csmemset(result, 0, sizeof(*result));
	result->definition_index = damage->definition_index;
	result->flags = damage->flags;
	result->owner_player_index = distributed_player_to_byte(damage->owner_player_index);
	result->owner_team_index = damage->owner_team_index;
	result->owner_object_index = damage->owner_object_index;
	result->origin = damage->origin;
	result->epicenter = damage->epicenter;
	result->direction = damage->direction;
	result->scale = damage->scale;
	result->multiplier = damage->multiplier;
	result->material_effect_scale = damage->material_effect_scale;
	result->material_type = damage->material_type;
}

/* the damage as this machine has it, FALSE if it cannot be */
static boolean distributed_damage_to_data(
	struct distributed_damage const *damage,
	struct damage_data *result)
{
	if (!tag_index_is_group(damage->definition_index, DAMAGE_EFFECT_DEFINITION_TAG))
		return FALSE;
	damage_data_new(result, damage->definition_index);
	result->flags = damage->flags;
	result->owner_player_index = distributed_player_from_byte(damage->owner_player_index);
	result->owner_team_index = damage->owner_team_index;
	result->owner_object_index = distributed_object_index_valid(damage->owner_object_index) &&
		object_try_and_get(damage->owner_object_index) ? damage->owner_object_index : NONE;
	result->origin = damage->origin;
	result->epicenter = damage->epicenter;
	result->direction = damage->direction;
	result->scale = damage->scale;
	result->multiplier = damage->multiplier;
	result->material_effect_scale = damage->material_effect_scale;
	result->material_type = damage->material_type;
	scenario_location_from_point(&result->location, &result->epicenter);
	return TRUE;
}

/* ---------- object_cause_damage (damage.c) */

/* whether this machine deals the damage: a client none (it reports its own
players' hits instead), the host all but its clients' players' (but for
their reports); authorized: a client carrying out the host's word */
boolean network_damage_deals(
	struct damage_data const *damage,
	long object_index,
	short node_index,
	short region_index,
	short material_index,
	real_vector3d const *object_normal,
	boolean authorized)
{
	if (game_connection() == _game_connection_network_client)
	{
		if (authorized)
			return TRUE;
		/* a hit of this machine's own player's, on the host's object */
		if (distributed_player_is_local(damage->owner_player_index) && network_objects_client_has(object_index) &&
			damage_report_count < MAXIMUM_HIT_REPORTS_PER_TICK)
		{
			struct distributed_hit_report *report = &damage_reports[damage_report_count++];

			csmemset(report, 0, sizeof(*report));
			report->object_index = object_index;
			distributed_damage_from_data(damage, &report->damage);
			/* (in the world: a rider's own position is its seat's) */
			object_get_origin(object_index, &report->target_position);
			report->node_index = node_index;
			report->region_index = region_index;
			report->material_index = material_index;
			if (object_normal)
			{
				report->object_normal = *object_normal;
				report->has_normal = TRUE;
			}
		}
		return FALSE;
	}
	if (game_connection() == _game_connection_network_server && !damage_dealing_report &&
		damage->owner_player_index != NONE && player_try_and_get(damage->owner_player_index) &&
		!distributed_player_is_local(damage->owner_player_index))
	{
		return FALSE;
	}
	return TRUE;
}

/* whether a client replaying the host's killing blow leaves out its
players' screen effects (which came as their own) */
boolean network_damage_replaying_kill(
	void)
{
	return damage_replaying_kill;
}

/* the host: a player's screen shaken and flashed by damage */
void network_damage_player_effect(
	long player_index,
	struct damage_data const *damage,
	real total_damage)
{
	struct distributed_damage_event *event;

	if (game_connection() != _game_connection_network_server ||
		damage_event_count >= MAXIMUM_DAMAGE_EVENTS_PER_TICK - RESERVED_KILL_EVENTS ||
		distributed_player_is_local(player_index))
	{
		return;
	}
	event = &damage_events[damage_event_count++];
	csmemset(event, 0, sizeof(*event));
	event->kind = _damage_event_player_effect;
	event->player_index = distributed_player_to_byte(player_index);
	event->object_index = NONE;
	distributed_damage_from_data(damage, &event->damage);
	event->total_damage = total_damage;
}

/* the host: an object damaged, and what that did (object_damage_aftermath
done, so that a kill's killer is known) */
void network_damage_aftermath(
	long object_index,
	struct damage_data const *damage,
	unsigned long being_damaged_flags,
	real shield_damage,
	real body_damage,
	real body_damage_multiplier,
	short body_part,
	short node_index,
	short region_index,
	short material_index)
{
	struct distributed_damage_event *event;
	struct unit_datum *unit;
	boolean kill;

	if (game_connection() != _game_connection_network_server)
		return;
	/* (units only: items and the like the objects' states place) */
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(object_index, _object_mask_unit);
	if (!unit)
		return;
	/* (the last events kept for killing blows) */
	kill = TEST_FLAG(being_damaged_flags, _object_being_damaged_body_depleted_bit) && unit->unit.player_index != NONE;
	if (damage_event_count >= MAXIMUM_DAMAGE_EVENTS_PER_TICK - (kill ? 0 : RESERVED_KILL_EVENTS))
		return;
	event = &damage_events[damage_event_count++];
	csmemset(event, 0, sizeof(*event));
	event->kind = _damage_event_aftermath;
	event->player_index = NO_PLAYER;
	event->object_index = object_index;
	distributed_damage_from_data(damage, &event->damage);
	event->being_damaged_flags = being_damaged_flags;
	event->shield_damage = shield_damage;
	event->body_damage = body_damage;
	event->body_damage_multiplier = body_damage_multiplier;
	event->body_part = body_part;
	event->node_index = node_index;
	event->region_index = region_index;
	event->material_index = material_index;
	/* a player's killing blow, with who the host says dealt it */
	if (kill)
	{
		boolean friendly_fire = FALSE;
		boolean killed_by_vehicle = FALSE;

		event->kind = _damage_event_kill;
		/* (no killer when the game noted none: a death it did not score) */
		if (!distributed_get_death((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit->unit.player_index),
			&event->player_index, &friendly_fire, &killed_by_vehicle))
		{
			event->player_index = NO_PLAYER;
			friendly_fire = FALSE;
			killed_by_vehicle = FALSE;
		}
		SET_FLAG(event->kill_flags, _damage_event_friendly_fire_bit, friendly_fire);
		SET_FLAG(event->kill_flags, _damage_event_killed_by_vehicle_bit, killed_by_vehicle);
	}
}

/* ---------- the host */

/* what damage a projectile's impacts and detonations deal */
static boolean distributed_projectile_deals(long projectile_index, long damage_index, short depth);

static boolean distributed_effect_deals(
	long effect_index,
	long damage_index,
	short depth)
{
	struct effect_definition *effect;
	short event_index;

	if (depth > MAXIMUM_TAG_DEPTH || !tag_index_is_group(effect_index, EFFECT_DEFINITION_TAG))
		return FALSE;
	effect = (struct effect_definition *)tag_get(EFFECT_DEFINITION_TAG, effect_index);
	for (event_index = 0; event_index < effect->events.count; event_index++)
	{
		struct effect_event_definition *event = TAG_BLOCK_GET_ELEMENT(
			&effect->events, event_index, struct effect_event_definition);
		short part_index;

		for (part_index = 0; part_index < event->parts.count; part_index++)
		{
			struct effect_part_definition *part = TAG_BLOCK_GET_ELEMENT(
				&event->parts, part_index, struct effect_part_definition);

			if (part->reference.index == NONE)
				continue;
			if (part->reference.index == damage_index ||
				(part->reference.group_tag == PROJECTILE_DEFINITION_TAG &&
					distributed_projectile_deals(part->reference.index, damage_index, depth + 1)) ||
				(part->reference.group_tag == EFFECT_DEFINITION_TAG &&
					distributed_effect_deals(part->reference.index, damage_index, depth + 1)))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

static boolean distributed_projectile_deals(
	long projectile_index,
	long damage_index,
	short depth)
{
	struct projectile_definition *projectile;
	short response_index;

	if (depth > MAXIMUM_TAG_DEPTH || !tag_index_is_group(projectile_index, PROJECTILE_DEFINITION_TAG))
		return FALSE;
	projectile = projectile_definition_get(projectile_index);
	/* (its effect is the one it detonates with: a grenade's, a rocket's
	explosion) */
	if (projectile->projectile.impact_damage.index == damage_index ||
		projectile->projectile.attached_detonation_damage.index == damage_index ||
		distributed_effect_deals(projectile->projectile.effect.index, damage_index, depth + 1) ||
		distributed_effect_deals(projectile->projectile.super_detonation.index, damage_index, depth + 1) ||
		distributed_effect_deals(projectile->projectile.detonation_started.index, damage_index, depth + 1))
	{
		return TRUE;
	}
	for (response_index = 0; response_index < projectile->projectile.material_responses.count; response_index++)
	{
		struct projectile_material_response_definition *response = TAG_BLOCK_GET_ELEMENT(
			&projectile->projectile.material_responses, response_index,
			struct projectile_material_response_definition);

		if (distributed_effect_deals(response->default_effect.index, damage_index, depth + 1) ||
			distributed_effect_deals(response->potential_effect.index, damage_index, depth + 1) ||
			distributed_effect_deals(response->detonation_effect.index, damage_index, depth + 1))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* how many hits a second the weapon deals of the damage, 0 for none */
static real distributed_weapon_deals(
	long weapon_definition_index,
	long damage_index)
{
	struct weapon_definition *weapon;
	short trigger_index;
	real rate = 0.0f;

	if (!tag_index_is_group(weapon_definition_index, WEAPON_DEFINITION_TAG))
		return 0.0f;
	weapon = weapon_definition_get(weapon_definition_index);
	if (weapon->weapon.melee_attack_damage.index == damage_index)
		rate = MELEE_HITS_PER_SECOND;
	if (distributed_effect_deals(weapon->weapon.detonation_effect.index, damage_index, 0) ||
		distributed_effect_deals(weapon->weapon.overheated_effect.index, damage_index, 0))
	{
		rate = MAX(rate, EFFECT_HITS_PER_SECOND);
	}
	for (trigger_index = 0; trigger_index < weapon->weapon.triggers.count; trigger_index++)
	{
		struct weapon_trigger_definition *trigger = TAG_BLOCK_GET_ELEMENT(
			&weapon->weapon.triggers, trigger_index, struct weapon_trigger_definition);

		if (distributed_projectile_deals(trigger->projectile.index, damage_index, 0))
		{
			real rate_of_fire = MAX(trigger->initial_rate_of_fire, trigger->final_rate_of_fire);

			/* (none is no limit, weapon_trigger_can_fire_again: a shot a tick,
			or a press, the plasma pistol's, as fast as a player taps) */
			if (rate_of_fire <= 0.0001f)
			{
				rate_of_fire = TEST_FLAG(trigger->flags, _weapon_trigger_latched_bit) ?
					TICKS_PER_SECOND / 2.0f : (real)TICKS_PER_SECOND;
			}
			rate = MAX(rate, MAX(rate_of_fire, 1.0f) * MAX(trigger->projectiles_per_shot, 1));
		}
	}
	return rate;
}

/* the source's (a weapon's, a grenade's projectile's) hits a second of the
damage, walking its tags once a game */
static real distributed_source_deals(
	long source_index,
	long damage_index,
	boolean grenade)
{
	unsigned long hash = ((unsigned long)source_index * 2654435761u) ^ ((unsigned long)damage_index * 40503u);
	short probe;

	for (probe = 0; probe < 16; probe++)
	{
		struct damage_rate_cache_entry *entry =
			&damage_rate_cache[(hash + probe) & (DAMAGE_RATE_CACHE_SIZE - 1)];

		if (entry->source_index == source_index && entry->damage_index == damage_index)
			return entry->rate;
		if (entry->source_index == NONE)
		{
			entry->source_index = source_index;
			entry->damage_index = damage_index;
			entry->rate = grenade ?
				(distributed_projectile_deals(source_index, damage_index, 0) ? (real)GRENADE_HITS_PER_SECOND : 0.0f) :
				distributed_weapon_deals(source_index, damage_index);
			return entry->rate;
		}
	}
	return grenade ?
		(distributed_projectile_deals(source_index, damage_index, 0) ? (real)GRENADE_HITS_PER_SECOND : 0.0f) :
		distributed_weapon_deals(source_index, damage_index);
}

/* how many hits a second the player could deal of the damage: their
weapons, lately, their grenades, their vehicle; 0 when none of them deals
it */
static real distributed_player_deals(
	short player_index,
	long damage_index)
{
	struct game_globals *globals = scenario_get_game_globals();
	short index;
	real rate = 0.0f;

	for (index = 0; index < MAXIMUM_RECENT_WEAPONS; index++)
	{
		if (damage_players[player_index].definition_indices[index] != NONE &&
			game_time_get() - damage_players[player_index].times[index] <= RECENT_WEAPON_TICKS)
		{
			rate = MAX(rate, distributed_source_deals(damage_players[player_index].definition_indices[index],
				damage_index, FALSE));
		}
	}
	for (index = 0; index < globals->grenades.count && index < NUMBER_OF_UNIT_GRENADE_TYPES; index++)
	{
		struct game_globals_grenade *grenade = TAG_BLOCK_GET_ELEMENT(&globals->grenades, index,
			struct game_globals_grenade);

		if (game_time_get() - damage_players[player_index].grenade_times[index] <= RECENT_WEAPON_TICKS &&
			grenade->projectile.index != NONE)
		{
			rate = MAX(rate, distributed_source_deals(grenade->projectile.index, damage_index, TRUE));
		}
	}
	if (game_time_get() - damage_players[player_index].vehicle_time <= RECENT_WEAPON_TICKS &&
		globals->falling_damage.count > 0)
	{
		struct distributed_falling_damage *falling_damage = TAG_BLOCK_GET_ELEMENT(&globals->falling_damage, 0,
			struct distributed_falling_damage);

		if (falling_damage->vehicle_killed_unit_damage_effect.index == damage_index ||
			falling_damage->vehicle_collision_damage.index == damage_index)
		{
			rate = MAX(rate, VEHICLE_HITS_PER_SECOND);
		}
	}
	return rate * HIT_REPORT_RATE_MARGIN;
}

/* where each player's unit and vehicle are this tick, noted */
static void distributed_note_targets(
	void)
{
	struct damage_history_tick *tick = &damage_history[game_time_get() & (TARGET_HISTORY_TICKS - 1)];
	short player_index;

	tick->time = game_time_get();
	for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
	{
		long unit_index = distributed_living_unit(distributed_player(player_index));
		short slot;

		tick->objects[player_index][0].object_index = unit_index;
		tick->objects[player_index][1].object_index = unit_index != NONE ?
			object_get(unit_index)->object.parent_object_index : NONE;
		for (slot = 0; slot < 2; slot++)
		{
			long object_index = tick->objects[player_index][slot].object_index;
			struct object_datum *object;

			if (object_index == NONE)
				continue;
			object = object_get(object_index);
			object_get_origin(object_index, &tick->objects[player_index][slot].position);
			tick->objects[player_index][slot].speed = (real)sqrt(
				object->object.translational_velocity.i * object->object.translational_velocity.i +
				object->object.translational_velocity.j * object->object.translational_velocity.j +
				object->object.translational_velocity.k * object->object.translational_velocity.k);
		}
	}
}

/* whether the host had the object (a player's unit or vehicle) within
reach of the position at a tick in the last ticks; NONE when it had no such
object then (not a player's) */
static short distributed_target_seen(
	long object_index,
	real_point3d const *position,
	long ticks)
{
	boolean known = FALSE;
	long back;

	for (back = 0; back <= ticks && back < TARGET_HISTORY_TICKS; back++)
	{
		long time = game_time_get() - back;
		struct damage_history_tick const *tick = &damage_history[time & (TARGET_HISTORY_TICKS - 1)];
		short player_index;

		if (tick->time != time)
			break;
		for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
		{
			short slot;

			for (slot = 0; slot < 2; slot++)
			{
				real reach;
				real dx, dy, dz;

				if (tick->objects[player_index][slot].object_index != object_index)
					continue;
				known = TRUE;
				reach = REPORT_HISTORY_TOLERANCE + REPORT_HISTORY_LEAD_TICKS * tick->objects[player_index][slot].speed;
				dx = position->x - tick->objects[player_index][slot].position.x;
				dy = position->y - tick->objects[player_index][slot].position.y;
				dz = position->z - tick->objects[player_index][slot].position.z;
				if (dx * dx + dy * dy + dz * dz <= reach * reach)
					return TRUE;
			}
		}
	}
	return known ? FALSE : NONE;
}

/* the weapons each player carries, and whether they ride, noted */
static void distributed_note_weapons(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		short player_index = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		long unit_index = distributed_living_unit(player);
		long units[2];
		short unit_number;

		if (player_index >= MAXIMUM_TRACKED_PLAYERS || unit_index == NONE)
			continue;
		units[0] = unit_index;
		units[1] = object_get(unit_index)->object.parent_object_index;
		if (units[1] != NONE)
			damage_players[player_index].vehicle_time = game_time_get();
		{
			struct unit_datum *unit = unit_get(unit_index);
			short grenade_index;

			for (grenade_index = 0; grenade_index < NUMBER_OF_UNIT_GRENADE_TYPES; grenade_index++)
			{
				if (unit->unit.grenade_counts[grenade_index] > 0)
					damage_players[player_index].grenade_times[grenade_index] = game_time_get();
			}
		}
		for (unit_number = 0; unit_number < 2; unit_number++)
		{
			struct unit_datum *unit = units[unit_number] != NONE ?
				(struct unit_datum *)object_try_and_get_and_verify_type(units[unit_number], _object_mask_unit) : NULL;
			short weapon_slot;

			if (!unit)
				continue;
			/* (a vehicle's weapons its driver's and gunner's, not its
			passengers') */
			if (unit_number == 1 && unit->unit.driver_object_index != unit_index &&
				unit->unit.gunner_object_index != unit_index)
			{
				continue;
			}
			for (weapon_slot = 0; weapon_slot < MAXIMUM_WEAPONS_PER_UNIT; weapon_slot++)
			{
				long weapon_index = unit->unit.weapon_object_indices[weapon_slot];
				long definition_index;
				short index;
				short oldest = 0;

				if (weapon_index == NONE || !object_try_and_get(weapon_index))
					continue;
				definition_index = object_get(weapon_index)->definition_index;
				for (index = 0; index < MAXIMUM_RECENT_WEAPONS; index++)
				{
					if (damage_players[player_index].definition_indices[index] == definition_index)
						break;
					if (damage_players[player_index].times[index] < damage_players[player_index].times[oldest])
						oldest = index;
				}
				if (index == MAXIMUM_RECENT_WEAPONS)
				{
					index = oldest;
					damage_players[player_index].definition_indices[index] = definition_index;
				}
				damage_players[player_index].times[index] = game_time_get();
			}
		}
	}
}

/* whether the node, region and material are the object's (or NONE) */
static boolean distributed_damage_indices_valid(
	long object_index,
	short node_index,
	short region_index,
	short material_index)
{
	struct object_datum *object = object_get(object_index);
	long collision_model_index = object_definition_get(object->definition_index)->object.collision_model.index;
	struct collision_model *collision_model = collision_model_index != NONE ?
		collision_model_definition_get(collision_model_index) : NULL;

	if (node_index != NONE && (node_index < 0 || !object_has_node(object_index, node_index)))
		return FALSE;
	if (region_index != NONE && (region_index < 0 || region_index >= MAXIMUM_REGIONS_PER_OBJECT ||
		!collision_model || region_index >= collision_model->resistance.regions.count))
	{
		return FALSE;
	}
	if (material_index != NONE && (material_index < 0 || !collision_model ||
		material_index >= collision_model->resistance.materials.count))
	{
		return FALSE;
	}
	return TRUE;
}

/* whether the damage's numbers are all finite and in the world */
static boolean distributed_damage_numbers_valid(
	struct distributed_damage const *damage)
{
	return distributed_point_valid(&damage->origin, REPORT_WORLD_BOUND) &&
		distributed_point_valid(&damage->epicenter, REPORT_WORLD_BOUND) &&
		distributed_point_valid((real_point3d const *)&damage->direction, 2.0f) &&
		distributed_real_valid(damage->scale) && distributed_real_valid(damage->multiplier) &&
		distributed_real_valid(damage->material_effect_scale);
}

/* whether the object is another player's (their unit, or what carries it:
damage.c's get_player_index_from_object_or_parents), whose damage a
client's player does not deal */
static boolean distributed_object_of_another_player(
	short player_index,
	long object_index)
{
	while (object_index != NONE)
	{
		if (unit_try_and_get(object_index))
		{
			long owner = player_index_from_unit_index(object_index);

			return owner != NONE && DATUM_INDEX_TO_ABSOLUTE_INDEX(owner) != player_index;
		}
		object_index = object_get(object_index)->object.parent_object_index;
	}
	return FALSE;
}

/* whether the host takes the report, which it makes the damage as the
host deals it */
static boolean distributed_report_valid(
	long machine_index,
	struct distributed_hit_report const *report,
	struct damage_data *damage)
{
	short player_index = report->damage.owner_player_index;
	struct player_datum *player;
	struct object_datum *target;
	struct damage_effect_definition *definition;
	real rate;
	real tolerance;
	real dx, dy, dz;
	real impact_distance_squared;
	real reach;

	/* that machine's player */
	if (player_index == NO_PLAYER || player_index >= MAXIMUM_TRACKED_PLAYERS ||
		!(player = distributed_player(player_index)) || !distributed_machine_has_player(machine_index, player_index))
	{
		return FALSE;
	}
	/* one of the host's objects (the index whole: identifier 0 is any
	object at the index) */
	if (!distributed_object_index_valid(report->object_index))
		return FALSE;
	target = (struct object_datum *)object_try_and_get_and_verify_type(report->object_index,
		_object_mask_biped | _object_mask_vehicle | _object_mask_weapon | _object_mask_equipment);
	if (!target || !tag_index_is_group(report->damage.definition_index, DAMAGE_EFFECT_DEFINITION_TAG))
		return FALSE;
	/* numbers the game can take */
	if (!distributed_damage_numbers_valid(&report->damage) ||
		!distributed_point_valid(&report->target_position, REPORT_WORLD_BOUND) ||
		(report->has_normal && !distributed_point_valid((real_point3d const *)&report->object_normal, 2.0f)) ||
		!(report->damage.scale >= 0.0f && report->damage.scale <= REPORT_MAXIMUM_SCALE) ||
		!distributed_damage_indices_valid(report->object_index, report->node_index, report->region_index,
			report->material_index))
	{
		return FALSE;
	}
	/* damage that player could deal */
	rate = distributed_player_deals(player_index, report->damage.definition_index);
	if (rate <= 0.0f)
		return FALSE;
	/* the target about where the host had it when the shooter saw it: a
	player's unit or vehicle as far back as the shooter's round trip, else
	(what no player has) about where it is */
	{
		long ticks = MIN((long)ceil(distributed_machine_round_trip_ticks(machine_index)),
			TARGET_HISTORY_MAXIMUM_ROUND_TRIP_TICKS) + TARGET_HISTORY_SLACK_TICKS;
		short seen = distributed_target_seen(report->object_index, &report->target_position,
			MIN(ticks, TARGET_HISTORY_TICKS - 1));

		if (seen == FALSE)
			return FALSE;
		if (seen == NONE)
		{
			real_point3d origin;

			object_get_origin(report->object_index, &origin);
			tolerance = REPORT_TARGET_TOLERANCE + REPORT_TARGET_LEAD_TICKS *
				(real)sqrt(target->object.translational_velocity.i * target->object.translational_velocity.i +
					target->object.translational_velocity.j * target->object.translational_velocity.j +
					target->object.translational_velocity.k * target->object.translational_velocity.k);
			dx = report->target_position.x - origin.x;
			dy = report->target_position.y - origin.y;
			dz = report->target_position.z - origin.z;
			if (!(dx * dx + dy * dy + dz * dz <= tolerance * tolerance))
				return FALSE;
		}
	}
	/* the impact at the target (an explosion's within its reach) */
	definition = damage_effect_definition_get(report->damage.definition_index);
	reach = target->object.bounding_sphere_radius + REPORT_IMPACT_TOLERANCE;
	if (TEST_FLAG(report->damage.flags, _damage_area_of_effect_bit))
		reach += definition->cutoff_radius;
	dx = report->damage.origin.x - report->target_position.x;
	dy = report->damage.origin.y - report->target_position.y;
	dz = report->damage.origin.z - report->target_position.z;
	impact_distance_squared = dx * dx + dy * dy + dz * dz;
	dx = report->damage.epicenter.x - report->target_position.x;
	dy = report->damage.epicenter.y - report->target_position.y;
	dz = report->damage.epicenter.z - report->target_position.z;
	impact_distance_squared = MIN(impact_distance_squared, dx * dx + dy * dy + dz * dz);
	if (!(impact_distance_squared <= reach * reach))
		return FALSE;
	/* no more than the weapon fires: an explosion's hits of a tick are one */
	{
		real *hit_seconds = &damage_players[player_index].hit_seconds;
		long elapsed = game_time_get() - damage_players[player_index].hit_seconds_time;
		boolean paid = FALSE;

		if (damage_explosions.time != game_time_get())
		{
			damage_explosions.time = game_time_get();
			damage_explosions.count = 0;
		}
		if (TEST_FLAG(report->damage.flags, _damage_area_of_effect_bit))
		{
			short index;

			for (index = 0; index < damage_explosions.count; index++)
			{
				if (damage_explosions.explosions[index].player_index == player_index &&
					damage_explosions.explosions[index].definition_index == report->damage.definition_index &&
					damage_explosions.explosions[index].epicenter.x == report->damage.epicenter.x &&
					damage_explosions.explosions[index].epicenter.y == report->damage.epicenter.y &&
					damage_explosions.explosions[index].epicenter.z == report->damage.epicenter.z)
				{
					paid = TRUE;
					break;
				}
			}
		}
		if (!paid)
		{
			*hit_seconds = MIN((real)HIT_REPORT_BURST_SECONDS, *hit_seconds + (real)elapsed / TICKS_PER_SECOND);
			damage_players[player_index].hit_seconds_time = game_time_get();
			if (*hit_seconds < 1.0f / rate)
				return FALSE;
			*hit_seconds -= 1.0f / rate;
			if (TEST_FLAG(report->damage.flags, _damage_area_of_effect_bit) &&
				damage_explosions.count < MAXIMUM_EXPLOSIONS_PER_TICK)
			{
				damage_explosions.explosions[damage_explosions.count].player_index = player_index;
				damage_explosions.explosions[damage_explosions.count].definition_index = report->damage.definition_index;
				damage_explosions.explosions[damage_explosions.count].epicenter = report->damage.epicenter;
				damage_explosions.count++;
			}
		}
	}
	/* the damage as the host deals it: what a client's hit can be, from the
	player's own */
	if (!distributed_damage_to_data(&report->damage, damage))
		return FALSE;
	damage->flags &= REPORT_DAMAGE_FLAGS;
	damage->multiplier = 1.0f;
	damage->owner_player_index = DATUM_INDEX_NEW(player_index, player->identifier);
	damage->owner_team_index = (short)player->team_index;
	if (damage->owner_object_index != NONE &&
		distributed_object_of_another_player(player_index, damage->owner_object_index))
	{
		damage->owner_object_index = distributed_living_unit(player);
	}
	return TRUE;
}

void network_damage_handle_reports(
	long machine_index,
	void const *entries,
	short count)
{
	struct distributed_hit_report const *reports = (struct distributed_hit_report const *)entries;
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_hit_report const *report = &reports[index];
		struct damage_data damage;

		if (!distributed_report_valid(machine_index, report, &damage))
		{
			damage_rejected_reports++;
			continue;
		}
		damage_dealt_reports++;
		damage_dealing_report = TRUE;
		object_cause_damage(&damage, report->object_index, report->node_index, report->region_index,
			report->material_index, report->has_normal ? &report->object_normal : NULL);
		damage_dealing_report = FALSE;
	}
}

void network_damage_host_tick(
	void)
{
	struct distributed_damage_event_message message;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, DATAGRAM_ENTRIES(struct distributed_damage_event));
	long machine_indices[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
	short machine_count = damage_event_count ?
		distributed_client_machines(machine_indices, HALO_PORT_MAXIMUM_NETWORK_MACHINES) : 0;
	short machine_number;

	distributed_note_weapons();
	distributed_note_targets();
	/* each machine the damage to units, and its own players' screen
	effects (no other machine's) */
	for (machine_number = 0; machine_number < machine_count; machine_number++)
	{
		long machine_index = machine_indices[machine_number];
		short index;
		short count = 0;

		for (index = 0; index < damage_event_count; index++)
		{
			if (damage_events[index].kind == _damage_event_player_effect &&
				!distributed_machine_has_player(machine_index, damage_events[index].player_index))
			{
				continue;
			}
			message.events[count++] = damage_events[index];
			if (count == limit)
			{
				distributed_send_to_machine(machine_index, &message, _distributed_message_damage_events, count,
					(word)(sizeof(message.header) + count * sizeof(struct distributed_damage_event)));
				count = 0;
			}
		}
		if (count)
		{
			distributed_send_to_machine(machine_index, &message, _distributed_message_damage_events, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_damage_event)));
		}
	}
	damage_event_count = 0;
}

/* ---------- a client */

void network_damage_handle_events(
	void const *entries,
	short count)
{
	struct distributed_damage_event const *events = (struct distributed_damage_event const *)entries;
	short index;

	for (index = 0; index < count; index++)
	{
		struct distributed_damage_event const *event = &events[index];
		struct damage_data damage;

		if (!distributed_damage_numbers_valid(&event->damage) || !distributed_damage_to_data(&event->damage, &damage) ||
			!distributed_real_valid(event->shield_damage) || !distributed_real_valid(event->body_damage) ||
			!distributed_real_valid(event->body_damage_multiplier) || !distributed_real_valid(event->total_damage))
		{
			continue;
		}
		damage_replayed_events++;
		switch (event->kind)
		{
		case _damage_event_player_effect:
		{
			long player_index = distributed_player_from_byte(event->player_index);

			if (distributed_player_is_local(player_index))
				damage_replay_player_effect(player_index, &damage, event->total_damage);
			break;
		}
		case _damage_event_aftermath:
			if (network_objects_client_has(event->object_index) &&
				object_try_and_get_and_verify_type(event->object_index, _object_mask_unit))
			{
				/* (what harm it did the units' states bring: no death from here) */
				damage_replay_aftermath(event->object_index, &damage,
					event->being_damaged_flags &
						~(FLAG(_object_being_damaged_body_depleted_bit) | FLAG(_object_being_damaged_killed_instantly_bit)),
					event->shield_damage, event->body_damage, event->body_damage_multiplier, event->body_part);
			}
			break;
		case _damage_event_kill:
			if (network_objects_client_has(event->object_index) &&
				object_try_and_get_and_verify_type(event->object_index, _object_mask_unit) &&
				!TEST_FLAG(object_get(event->object_index)->object.damage_flags, _object_dead_bit) &&
				distributed_damage_indices_valid(event->object_index, event->node_index, event->region_index,
					event->material_index))
			{
				struct unit_datum *unit = unit_get(event->object_index);

				if (unit->unit.player_index != NONE)
				{
					distributed_set_death((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(unit->unit.player_index),
						event->player_index, TEST_FLAG(event->kill_flags, _damage_event_friendly_fire_bit),
						TEST_FLAG(event->kill_flags, _damage_event_killed_by_vehicle_bit));
				}
				damage_replaying_kill = TRUE;
				damage_replay_kill(event->object_index, &damage, event->node_index, event->region_index,
					event->material_index);
				damage_replaying_kill = FALSE;
			}
			break;
		}
	}
}

void network_damage_client_tick(
	void)
{
	struct distributed_hit_report_message message;
	short limit = MIN(MAXIMUM_ENTRIES_PER_MESSAGE, RELIABLE_ENTRIES(struct distributed_hit_report));
	short index;
	short count = 0;

	damage_sent_reports += damage_report_count;
	for (index = 0; index < damage_report_count; index++)
	{
		message.reports[count++] = damage_reports[index];
		if (count == limit)
		{
			distributed_send(&message, _distributed_message_hit_reports, count,
				(word)(sizeof(message.header) + count * sizeof(struct distributed_hit_report)),
				_distributed_to_host_reliably);
			count = 0;
		}
	}
	if (count)
	{
		distributed_send(&message, _distributed_message_hit_reports, count,
			(word)(sizeof(message.header) + count * sizeof(struct distributed_hit_report)),
			_distributed_to_host_reliably);
	}
	damage_report_count = 0;
}

void network_damage_statistics(
	long *sent_reports,
	long *dealt_reports,
	long *rejected_reports,
	long *replayed_events)
{
	*sent_reports = damage_sent_reports;
	*dealt_reports = damage_dealt_reports;
	*rejected_reports = damage_rejected_reports;
	*replayed_events = damage_replayed_events;
}

void network_damage_new_game(
	void)
{
	short player_index;

	damage_event_count = 0;
	damage_report_count = 0;
	damage_dealing_report = FALSE;
	damage_replaying_kill = FALSE;
	csmemset(damage_players, 0, sizeof(damage_players));
	for (player_index = 0; player_index < TARGET_HISTORY_TICKS; player_index++)
		damage_history[player_index].time = NONE;
	for (player_index = 0; player_index < MAXIMUM_TRACKED_PLAYERS; player_index++)
	{
		short index;

		for (index = 0; index < MAXIMUM_RECENT_WEAPONS; index++)
			damage_players[player_index].definition_indices[index] = NONE;
		for (index = 0; index < NUMBER_OF_UNIT_GRENADE_TYPES; index++)
			damage_players[player_index].grenade_times[index] = -RECENT_WEAPON_TICKS - 1;
		damage_players[player_index].vehicle_time = -RECENT_WEAPON_TICKS - 1;
		damage_players[player_index].hit_seconds = HIT_REPORT_BURST_SECONDS;
	}
	damage_explosions.time = NONE;
	damage_explosions.count = 0;
	for (player_index = 0; player_index < DAMAGE_RATE_CACHE_SIZE; player_index++)
		damage_rate_cache[player_index].source_index = NONE;
}
