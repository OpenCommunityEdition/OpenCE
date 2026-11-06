/*
MENDICANT_COMMAND.C

MENDICANT AI's command layer (M2), its actor and bridge side: which mode each
side plays in (the game's own AI, the scripted drill or the models), the
orders and roles each actor is under, the request to the models and the
answer applied, the drill's update (Marines enrolled by encounter, the
Covenant under the Elite within reach, a new leader when one dies, a lone
survivor joining another squad), and discipline: an actor under a live
command does not panic, go berserk, rush the enemy or search alone.

The units' doctrine itself (the chains of steps each unit runs) is in
mendicant_units.c. Ported from the ai_brain hook library (ai_brain.c's
command parts); its behaviour is the same.
*/

#define MENDICANT_COMMAND_PRIVATE

#include "cseries.h"
#include "memory/data.h"
#include "ai/ai.h"
#include "ai/actors.h"
#include "ai/props.h"
#include "ai/actions.h"
#include "ai/ai_scenario_definitions.h"
#include "objects/objects.h"
#include "units/units.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mendicant_internal.h"

/* source/ai/actions.c (declared there, not in a header) */
boolean actor_action_handle_lost_contact(long actor_index);

/* ---------- constants */

#define COMMAND_DRILL_UPDATE_TICKS 30
#define COMMAND_RADIANS_TO_DEGREES 57.29578f
/* the positions offered to the models for a member: around the flank of the
target, within reach */
#define COMMAND_FLANK_ANGLE_BEST 90.0f
#define COMMAND_FLANK_TARGET_DISTANCE_MINIMUM 5.0f
#define COMMAND_FLANK_TARGET_DISTANCE_MAXIMUM 30.0f
#define COMMAND_FLANK_TRAVEL_MAXIMUM 35.0f
#define COMMAND_FLANK_HEIGHT_MAXIMUM 8.0f
/* an actor's combat status from which a lost target is searched for */
#define COMMAND_COMBAT_STATUS_ENGAGED 2

/* ---------- globals */

/* the position each actor under orders keeps, by absolute actor index */
long order_actor[MENDICANT_MAX_ACTORS];
short order_position[MENDICANT_MAX_ACTORS];
/* each actor's part in its unit's drill (ROLE_*), which sets how it moves and fires */
char actor_role[MENDICANT_MAX_ACTORS];
/* going in: a grenade first */
unsigned char actor_grenade_due[MENDICANT_MAX_ACTORS];
/* where a member may fight from without an order: x, y, z, radius (0: none) */
real actor_leash[MENDICANT_MAX_ACTORS][4];
long leash_corrections;
/* where a watching member looks */
real actor_face[MENDICANT_MAX_ACTORS][3];
/* its unit's assaulters' nearest range to the enemy */
real unit_assault_range[MENDICANT_MAX_ACTORS];
/* the tick a unit last commanded the actor */
long commanded_time[MENDICANT_MAX_ACTORS];
/* a request is out to the models */
boolean bridge_pending;

/* its element in its unit: base, maneuver or reserve */
signed char actor_team_of[MENDICANT_MAX_ACTORS];
/* sent somewhere by its step */
unsigned char actor_mover[MENDICANT_MAX_ACTORS];
/* the unit each actor is in: the full actor index, and the unit's key (an
Elite commander's actor slot, or MENDICANT_MAX_ACTORS + encounter) */
long enrolled_actor[MENDICANT_MAX_ACTORS];
short enrolled_key[MENDICANT_MAX_ACTORS];
/* a late model: its decision goes to the drill */
boolean bridge_lost_last;

/* under a live command: no panic, no lone pursuit */
static unsigned char actor_disciplined[MENDICANT_MAX_ACTORS];
static long bridge_request_time;
static long pending_members[MENDICANT_MAX_SQUAD];
static short pending_positions[MENDICANT_MAX_SQUAD][BRIDGE_CANDIDATES];
/* the drill's random numbers (xorshift32): it runs on across maps, as the
hook library's did */
static unsigned long drill_random = 2463534242UL;
static long drill_update_time;
/* what discipline declined this episode: panics, berserks, charges, lone pursuits */
static long discipline_declined[4];

/* ---------- small helpers */

/* the game's own datum lookups; NULL for a stale or free index */
static struct actor_datum *command_actor_get(long actor_index)
{
	if (!actor_data || !actor_data->valid || (short)actor_index >= actor_data->count)
		return NULL;
	return (struct actor_datum *)datum_try_and_get(actor_data, actor_index);
}

static struct prop_datum *command_prop_get(long prop_index)
{
	if (!prop_data || !prop_data->valid || (short)prop_index >= prop_data->count)
		return NULL;
	return (struct prop_datum *)datum_try_and_get(prop_data, prop_index);
}

static void *command_object_get(long object_index)
{
	struct object_header_datum *header;

	if (!object_header_data || !object_header_data->valid || (short)object_index >= object_header_data->count)
		return NULL;
	header = (struct object_header_datum *)datum_try_and_get(object_header_data, object_index);
	return header ? header->datum : NULL;
}

/* the actor at an absolute index of the actor array (its identifier 0: free) */
static struct actor_datum *command_actor_at(long index)
{
	return (struct actor_datum *)((byte *)actor_data->data + actor_data->size * index);
}

static real command_distance_to(struct actor_datum *actor, real x, real y, real z)
{
	real dx = actor->input.pathfinding_point.x - x;
	real dy = actor->input.pathfinding_point.y - y;
	real dz = actor->input.pathfinding_point.z - z;

	return (real)sqrt(dx * dx + dy * dy + dz * dz);
}

static real command_range_to(struct actor_datum *actor, real const *target)
{
	real x = actor->input.pathfinding_point.x - target[0];
	real y = actor->input.pathfinding_point.y - target[1];

	return (real)sqrt(x * x + y * y);
}

static real command_signed_angle(real from_x, real from_y, real to_x, real to_y)
{
	return (real)atan2(from_x * to_y - from_y * to_x, from_x * to_x + from_y * to_y) * COMMAND_RADIANS_TO_DEGREES;
}

static boolean command_actor_is_elite_leader(struct actor_datum *actor)
{
	return actor->meta.team_index == TEAM_COVENANT && MENDICANT_ACTOR_IS_ELITE(actor->meta.type) &&
		actor->meta.active && !actor->meta.swarm && actor->input.body_vitality > 0.0f;
}

static short command_change_firing_position(long actor_index, short position)
{
	/* (the firing position argument is unused: actor_firing_position.c) */
	return actor_change_firing_position(actor_index, position, NULL, NONE, NULL, FALSE);
}

/* ---------- the command layer's services (mendicant.h) */

real mendicant_command_actor_distance_squared(struct actor_datum *a, struct actor_datum *b)
{
	real x = a->input.pathfinding_point.x - b->input.pathfinding_point.x;
	real y = a->input.pathfinding_point.y - b->input.pathfinding_point.y;
	real z = a->input.pathfinding_point.z - b->input.pathfinding_point.z;

	return x * x + y * y + z * z;
}

/* an actor's own encounter holds the positions it may take */
struct firing_position_definition *mendicant_command_member_positions(struct actor_datum *actor, int *count)
{
	struct firing_position_definition *positions;

	*count = 0;
	positions = mendicant_encounter_firing_positions(actor->meta.encounter_index, count);
	if (*count > MENDICANT_MAX_POSITIONS)
		*count = MENDICANT_MAX_POSITIONS;
	return positions;
}

void mendicant_command_clear_member_orders(struct mendicant_member *members, int count)
{
	int index;

	for (index = 0; index < count; index++)
	{
		order_actor[(short)members[index].actor_index] = NONE;
		actor_role[(short)members[index].actor_index] = ROLE_NONE;
	}
}

/* the player's team (the Chief) fights, and is commanded, with the Marines */
int mendicant_side_of(short team)
{
	return team == TEAM_PLAYER ? TEAM_HUMAN : team;
}

int mendicant_team_mode(short team)
{
	int side = mendicant_side_of(team);

	if (side < 0 || side >= TEAMS)
		return MODE_GAME;
	if (mendicant_bridge && mendicant_training)
		return mendicant_episode_over ? MODE_GAME : mendicant_bridge->team_mode[side];
	if (mendicant_bridge)
		return side == TEAM_COVENANT || side == TEAM_HUMAN ? MODE_MODEL : MODE_GAME;
	return mendicant_active && side == TEAM_COVENANT ? MODE_DRILL : MODE_GAME;
}

int mendicant_unit_side(struct actor_datum *actor)
{
	return mendicant_side_of(actor->meta.team_index) == TEAM_COVENANT ? MENDICANT_SIDE_COVENANT : MENDICANT_SIDE_MARINES;
}

/* the drill may command it: its side is not the game's, it is awake, in an
encounter, on foot, and fighting, searching or guarding */
boolean mendicant_actor_can_drill(struct actor_datum *actor)
{
	short action = actor->state.action;

	return mendicant_team_mode(actor->meta.team_index) != MODE_GAME && actor->meta.active &&
		!actor->meta.swarm && actor->meta.encounter_index != NONE && actor->input.vehicle_index == NONE &&
		(action == _actor_action_fight || action == _actor_action_uncover || action == _actor_action_guard);
}

void mendicant_set_role(long actor_index, int role)
{
	int slot = (short)actor_index;

	if (slot < 0 || slot >= MENDICANT_MAX_ACTORS)
		return;
	if ((role == ROLE_ASSAULT || role == ROLE_GRENADIER) && actor_role[slot] != role)
		actor_grenade_due[slot] = 1; /* going in: a grenade first */
	actor_role[slot] = (char)role;
}

/* an order to a position ends a free-movement order from a drill or from
following the Chief, so the drill has the actor again */
void mendicant_order(long actor_index, short position)
{
	int slot = (short)actor_index;

	if (slot >= 0 && slot < MENDICANT_MAX_ACTORS)
		mendicant_go_release_drill(actor_index);
	order_actor[(short)actor_index] = actor_index;
	order_position[(short)actor_index] = position;
}

unsigned long mendicant_drill_rand(void)
{
	drill_random ^= (drill_random << 13) & 0xFFFFFFFFUL;
	drill_random ^= drill_random >> 17;
	drill_random ^= (drill_random << 5) & 0xFFFFFFFFUL;
	drill_random &= 0xFFFFFFFFUL;
	return drill_random;
}

/* a request is out to the models and not answered yet */
boolean mendicant_bridge_waiting(void)
{
	return bridge_pending;
}

void mendicant_orders_clear_all(void)
{
	csmemset(order_actor, 0xff, sizeof(order_actor));
}

/* ---------- the request to the models */

/* a member's candidates: staying put first, then the free positions around
the target's flank, best first */
static void bridge_candidates(int member, struct actor_datum *actor, real const *target, real axis_x, real axis_y)
{
	struct model_bridge *bridge = mendicant_bridge;
	struct
	{
		real score;
		int position;
		real angle, travel, range, height;
	} best[BRIDGE_CANDIDATES];
	struct firing_position_definition *positions;
	int found = 0, position_count = 0, position, count, index;
	long encounter_index;
	real body, shield, friends, enemies, current_x, current_y, current_range, current_angle;
	float *feature;

	if (actor->meta.encounter_index == NONE || !mendicant_actor_can_drill(actor))
	{
		/* no positions to offer (no encounter, in a vehicle, busy): only staying put */
		pending_positions[member][0] = NONE;
		bridge->positions[member][0] = NONE;
		memset(bridge->features[member][0], 0, sizeof(bridge->features[member][0]));
		bridge->features[member][0][9] = 1.0f;
		bridge->candidate_count[member] = 1;
		return;
	}
	encounter_index = actor->meta.encounter_index;
	positions = mendicant_command_member_positions(actor, &position_count);

	for (position = 0; positions && position < position_count; position++)
	{
		real const *point = positions[position].position.n;
		real x = point[0] - target[0], y = point[1] - target[1];
		real range = (real)sqrt(x * x + y * y);
		real height = point[2] - target[2];
		real travel = command_distance_to(actor, point[0], point[1], point[2]);
		real angle, score;
		int slot;

		if (range < COMMAND_FLANK_TARGET_DISTANCE_MINIMUM || range > COMMAND_FLANK_TARGET_DISTANCE_MAXIMUM ||
			travel > COMMAND_FLANK_TRAVEL_MAXIMUM || (real)fabs(height) > COMMAND_FLANK_HEIGHT_MAXIMUM)
			continue;

		angle = command_signed_angle(axis_x, axis_y, x, y);
		score = (real)fabs((real)fabs(angle) - COMMAND_FLANK_ANGLE_BEST) + travel * 0.5f;
		/* keeps the best candidates, sorted; one slot is left for staying put */
		if (found < BRIDGE_CANDIDATES - 1)
			slot = found++;
		else if (score < best[BRIDGE_CANDIDATES - 2].score)
			slot = BRIDGE_CANDIDATES - 2;
		else
			continue;
		while (slot > 0 && best[slot - 1].score > score)
		{
			best[slot] = best[slot - 1];
			slot--;
		}
		best[slot].score = score;
		best[slot].position = position;
		best[slot].angle = angle;
		best[slot].travel = travel;
		best[slot].range = range;
		best[slot].height = height;
	}

	body = actor->input.body_vitality;
	shield = actor->input.shield_vitality;
	friends = actor->situation.close_friends;
	enemies = actor->situation.visible_reachable_enemies;
	current_x = actor->input.pathfinding_point.x - target[0];
	current_y = actor->input.pathfinding_point.y - target[1];
	current_range = (real)sqrt(current_x * current_x + current_y * current_y);
	current_angle = command_signed_angle(axis_x, axis_y, current_x, current_y);
	count = 1;

	/* candidate 0 is always staying put */
	feature = bridge->features[member][0];
	pending_positions[member][0] = NONE;
	bridge->positions[member][0] = NONE;
	feature[0] = current_angle / 90.0f;
	feature[1] = 0.0f;
	feature[2] = current_range / 30.0f;
	feature[3] = 0.0f;
	feature[4] = 0.0f;
	feature[5] = body;
	feature[6] = shield;
	feature[7] = friends / 8.0f;
	feature[8] = enemies / 4.0f;
	feature[9] = 1.0f;

	for (index = 0; index < found && count < BRIDGE_CANDIDATES; index++)
	{
		if (mendicant_firing_position_owned(encounter_index, (short)best[index].position))
			continue;
		feature = bridge->features[member][count];
		pending_positions[member][count] = (short)best[index].position;
		bridge->positions[member][count] = (short)best[index].position;
		feature[0] = best[index].angle / 90.0f;
		feature[1] = ((real)fabs(best[index].angle) - (real)fabs(current_angle)) / 90.0f;
		feature[2] = best[index].range / 30.0f;
		feature[3] = best[index].travel / 35.0f;
		feature[4] = best[index].height / 8.0f;
		feature[5] = body;
		feature[6] = shield;
		feature[7] = friends / 8.0f;
		feature[8] = enemies / 4.0f;
		feature[9] = 0.0f;
		count++;
	}
	bridge->candidate_count[member] = (short)count;
}

/* commanded: an Elite (or the Marines' squad) leads this unit, so its
strategic model plans for it. Without one every soldier decides alone: the
models get no team plan for them. */
void mendicant_bridge_request(long key, struct mendicant_member *members, int count,
	real_point3d const *target, real axis_x, real axis_y, boolean commanded, long time)
{
	struct model_bridge *bridge = mendicant_bridge;
	real body = 0.0f, shield = 0.0f, visible = 0.0f, spread = 0.0f;
	int index;

	for (index = 0; index < count; index++)
	{
		struct actor_datum *actor = members[index].actor;

		pending_members[index] = members[index].actor_index;
		body += actor->input.body_vitality;
		shield += actor->input.shield_vitality;
		visible += actor->situation.visible_reachable_enemies > 0;
		spread += (real)fabs(command_signed_angle(axis_x, axis_y, actor->input.pathfinding_point.x - target->x,
			actor->input.pathfinding_point.y - target->y));
		bridge_candidates(index, actor, target->n, axis_x, axis_y);
	}

	bridge->time = time;
	bridge->encounter = key;
	bridge->member_count = count;
	bridge->team[0] = count / 12.0f;
	bridge->team[1] = body / count;
	bridge->team[2] = shield / count;
	bridge->team[3] = visible / count;
	bridge->team[4] = spread / count / 90.0f;
	bridge->team[5] = (real)sqrt(axis_x * axis_x + axis_y * axis_y) / 30.0f;
	bridge->team_index = mendicant_side_of(members[0].actor->meta.team_index);
	bridge->team[6] = bridge->team_index == TEAM_COVENANT;
	bridge->team[7] = commanded ? 1.0f : 0.0f;
	bridge_request_time = time;
	bridge_pending = TRUE;
	/* published (and in training waited for, or answered here by the models) */
	mendicant_bridge_submit();
}

/* the answer: a strategy or step for a unit, or each soldier's position (and,
on his own, his way of fighting) */
static void bridge_apply(long time)
{
	struct model_bridge *bridge = mendicant_bridge;
	int moved = 0, held = 0, rejected = 0, index;

	if (bridge->plan_request && bridge->encounter >= 0 && bridge->encounter < MENDICANT_UNIT_KEYS)
	{
		/* the strategic model chose for a unit: those of it still in it */
		static struct mendicant_member members[MENDICANT_MAX_SQUAD];
		int count = 0, key = bridge->encounter;

		for (index = 0; index < bridge->member_count; index++)
		{
			struct actor_datum *actor = command_actor_get(pending_members[index]);
			int slot = (short)pending_members[index];

			if (actor && enrolled_actor[slot] == pending_members[index] && enrolled_key[slot] == key)
			{
				members[count].actor_index = pending_members[index];
				members[count].actor = actor;
				count++;
			}
		}
		mendicant_units_plan_answer(key, members, count, time);
		return;
	}
	for (index = 0; index < bridge->member_count; index++)
		order_actor[(short)pending_members[index]] = NONE;

	for (index = 0; index < bridge->member_count; index++)
	{
		long actor_index = pending_members[index];
		struct actor_datum *actor = command_actor_get(actor_index);
		int choice = bridge->choice[index];
		int position, stance;

		if (!actor || !mendicant_actor_can_drill(actor) || choice < 0 || choice >= bridge->candidate_count[index])
		{
			rejected++;
			continue;
		}

		position = pending_positions[index][choice];
		stance = bridge->team[7] < 0.5f ? bridge->stance[index] : 0;
		if (bridge->team[7] < 0.5f)
		{
			/* a soldier on his own: his model's way of fighting, through the role
			that makes it (held, he keeps his own position under the role) */
			mendicant_apply_stance(actor_index, stance);
			if (stance > 0 && stance < STANCES && position < 0 && actor->firing_positions.current_position_index >= 0)
				mendicant_order(actor_index, actor->firing_positions.current_position_index);
		}
		if (position < 0)
		{
			/* candidate 0: no order, the game's own AI decides for this one */
			held++;
		}
		else if (!mendicant_firing_position_owned(actor->meta.encounter_index, (short)position) &&
			(actor->state.action == _actor_action_fight || actor->state.action == _actor_action_uncover) &&
			command_change_firing_position(actor_index, (short)position) == position)
		{
			mendicant_order(actor_index, (short)position);
			moved++;
		}
		else
		{
			rejected++;
		}
	}
	if (mendicant_drill_file)
	{
		fprintf(mendicant_drill_file, "%ld,model,%ld,squad=%ld,commanded=%d,moved=%d,held=%d,rejected=%d,latency=%ld\n",
			time, (long)bridge->encounter, (long)bridge->member_count, bridge->team[7] > 0.5f, moved, held,
			rejected, time - bridge_request_time);
	}
}

static void bridge_poll(long time)
{
	struct model_bridge *bridge = mendicant_bridge;

	if (!bridge_pending)
		return;
	if (mendicant_bridge_answered())
	{
		bridge_pending = FALSE;
		bridge_apply(time);
	}
	else if (time < bridge_request_time || time - bridge_request_time >= BRIDGE_TIMEOUT_TICKS)
	{
		/* a model a second late loses the decision to the drill */
		bridge_pending = FALSE;
		bridge_lost_last = TRUE;
		mendicant_units_retry(bridge->encounter, time);
		if (mendicant_drill_file)
			fprintf(mendicant_drill_file, "%ld,model_late,%ld\n", time, (long)bridge->encounter);
	}
}

/* ---------- the drill's update */

/* one battle unit: an Elite's command, the Marines of an encounter, or the
leaderless Covenant of an encounter */
struct command_battle_unit
{
	int key;
	int count;
	struct mendicant_member members[MENDICANT_MAX_SQUAD];
};

/* a Covenant unit lives while one of its Elites does: the senior one left
takes over, or with none left, the unit and its coordination are gone */
static void command_succession(int actor_count, long time)
{
	int index, other;

	for (index = 0; index < actor_count; index++)
	{
		long member = enrolled_actor[index];
		struct actor_datum *actor, *leader;
		int key, heir = -1;
		boolean leaderless = FALSE;

		if (member == NONE)
			continue;
		actor = command_actor_get(member);
		key = enrolled_key[index];
		if (!actor || actor->input.body_vitality <= 0.0f)
		{
			enrolled_actor[index] = NONE; /* dead */
			actor_disciplined[index] = 0;
			continue;
		}
		if (key >= MENDICANT_MAX_ACTORS)
			continue;
		leader = command_actor_get(mendicant_units_leader(key));
		if (leader && (command_actor_is_elite_leader(leader) ||
			(mendicant_units_leaderless(key) && leader->input.body_vitality > 0.0f)))
			continue;
		for (other = 0; other < actor_count && heir < 0; other++)
		{
			struct actor_datum *candidate = enrolled_actor[other] != NONE && enrolled_key[other] == key ?
				command_actor_get(enrolled_actor[other]) : NULL;

			if (candidate && command_actor_is_elite_leader(candidate))
				heir = other;
		}
		/* no Elite left: the survivors re-form under the senior of them and go on
		plotting, but without an officer's discipline (they may still break and run) */
		for (other = 0; other < actor_count && heir < 0; other++)
		{
			struct actor_datum *candidate = enrolled_actor[other] != NONE && enrolled_key[other] == key ?
				command_actor_get(enrolled_actor[other]) : NULL;

			if (candidate && candidate->input.body_vitality > 0.0f)
			{
				heir = other;
				leaderless = TRUE;
			}
		}
		if (heir >= 0)
		{
			/* the plan goes on under the next Elite */
			int new_key = heir;

			mendicant_units_inherit(new_key, key, enrolled_actor[heir], leaderless);
			for (other = 0; other < actor_count; other++)
			{
				if (enrolled_actor[other] != NONE && enrolled_key[other] == key)
				{
					enrolled_key[other] = (short)new_key;
					if (leaderless)
						actor_disciplined[other] = 0;
				}
			}
			mendicant_units_stand_down(key);
			if (mendicant_drill_file)
				fprintf(mendicant_drill_file, "%ld,new_leader,%d,from=%d\n", time, new_key, key);
		}
		else
		{
			/* the last Elite is dead: no command; each decides alone again */
			for (other = 0; other < actor_count; other++)
			{
				if (enrolled_actor[other] != NONE && enrolled_key[other] == key)
				{
					order_actor[other] = NONE;
					actor_role[other] = ROLE_NONE;
					actor_leash[other][3] = 0.0f;
					actor_disciplined[other] = 0;
					enrolled_actor[other] = NONE;
				}
			}
			if (mendicant_units_stand_down(key) && mendicant_drill_file)
				fprintf(mendicant_drill_file, "%ld,leader_dead,%d\n", time, key);
		}
	}
}

/* enrolment: Marines by encounter; the Covenant under an Elite within reach.
An Elite with no unit near starts its own. Once in a unit, a member stays in it. */
static void command_enrol(int actor_count)
{
	real radius_squared = mendicant_config.command_radius * mendicant_config.command_radius;
	long chief = mendicant_player_unit();
	int pass, index, other;

	for (pass = 0; pass < 3; pass++)
	{
		for (index = 0; index < actor_count; index++)
		{
			struct actor_datum *actor = command_actor_at(index);
			int identifier = actor->identifier;
			int team, encounter, key = -1;
			int sizes[2];

			if (!identifier || enrolled_actor[index] == (((long)identifier << 16) | index))
				continue;
			team = mendicant_side_of(actor->meta.team_index);
			encounter = (short)actor->meta.encounter_index;
			if (actor->meta.unit_index == chief || actor->input.body_vitality <= 0.0f ||
				mendicant_team_mode(actor->meta.team_index) == MODE_GAME || !actor->meta.active)
				continue;
			if (team == TEAM_HUMAN && pass == 0 && encounter >= 0 && encounter < MENDICANT_MAX_ENCOUNTERS &&
				MENDICANT_ACTOR_IS_MARINE(actor->meta.type))
			{
				key = MENDICANT_MAX_ACTORS + encounter;
			}
			else if (team == TEAM_COVENANT && pass > 0)
			{
				boolean elite = command_actor_is_elite_leader(actor);
				real best = radius_squared;

				if ((pass == 1) != elite)
					continue; /* Elites first, then the rest join them */
				for (other = 0; other < actor_count; other++)
				{
					struct actor_datum *candidate = enrolled_actor[other] != NONE &&
						enrolled_key[other] < MENDICANT_MAX_ACTORS ? command_actor_get(enrolled_actor[other]) : NULL;
					real distance;

					if (!candidate || !command_actor_is_elite_leader(candidate))
						continue;
					distance = mendicant_command_actor_distance_squared(actor, candidate);
					if (distance <= best)
					{
						best = distance;
						key = enrolled_key[other];
					}
				}
				if (key < 0 && !elite)
				{
					/* no Elite near: the nearest band of leaderless Covenant, still plotting */
					real near_distance = radius_squared;

					for (other = 0; other < actor_count; other++)
					{
						struct actor_datum *candidate = enrolled_actor[other] != NONE &&
							enrolled_key[other] < MENDICANT_MAX_ACTORS && mendicant_units_leaderless(enrolled_key[other]) ?
							command_actor_get(enrolled_actor[other]) : NULL;
						real distance;

						if (!candidate)
							continue;
						distance = mendicant_command_actor_distance_squared(actor, candidate);
						if (distance <= near_distance)
						{
							near_distance = distance;
							key = enrolled_key[other];
						}
					}
				}
				if (key < 0)
				{
					key = index; /* a new unit, under this Elite (or, with none, this one) */
					mendicant_units_found(key, ((long)identifier << 16) | index, !elite);
				}
			}
			if (key < 0)
				continue;
			enrolled_actor[index] = ((long)identifier << 16) | index;
			enrolled_key[index] = (short)key;
			actor_disciplined[index] = key >= MENDICANT_MAX_ACTORS || !mendicant_units_leaderless(key);
			/* a newcomer joins the smaller team */
			sizes[0] = sizes[1] = 0;
			for (other = 0; other < actor_count; other++)
			{
				if (other != index && enrolled_actor[other] != NONE && enrolled_key[other] == key)
					sizes[actor_team_of[other] & 1]++;
			}
			actor_team_of[index] = sizes[1] < sizes[0];
		}
	}
}

/* a lone survivor is never left standing: the last Marine of a squad joins
the nearest other squad (or falls in on the Chief); with no one to join he
fights on his own, free */
static void command_lone_survivors(struct command_battle_unit *units, int unit_count, long time)
{
	int index, other;

	for (index = 0; index < unit_count; index++)
	{
		struct actor_datum *actor;
		struct unit_datum *chief;
		long actor_index;
		int slot, join = -1;
		real best = 40.0f * 40.0f;

		if (units[index].count != 1 || units[index].key < MENDICANT_MAX_ACTORS)
			continue;
		actor = units[index].members[0].actor;
		actor_index = units[index].members[0].actor_index;
		slot = (short)actor_index;
		for (other = 0; other < unit_count; other++)
		{
			real distance;

			if (other == index || units[other].key < MENDICANT_MAX_ACTORS || units[other].count < 1 ||
				mendicant_unit_side(units[other].members[0].actor) != mendicant_unit_side(actor))
				continue;
			distance = mendicant_command_actor_distance_squared(actor, units[other].members[0].actor);
			if (distance < best)
			{
				best = distance;
				join = other;
			}
		}
		if (join >= 0)
		{
			/* into the other squad: its encounter (its positions), its plan, its smaller element */
			struct command_battle_unit *target = &units[join];

			actor_change_encounter(actor_index, target->members[0].actor->meta.encounter_index,
				target->members[0].actor->meta.squad_index);
			enrolled_key[slot] = (short)target->key;
			actor_team_of[slot] = mendicant_units_element_size(target->members, target->count, ELEMENT_BASE) <=
				mendicant_units_element_size(target->members, target->count, ELEMENT_MANEUVER) ?
				ELEMENT_BASE : ELEMENT_MANEUVER;
			order_actor[slot] = NONE;
			if (target->count < MENDICANT_MAX_SQUAD)
				target->members[target->count++] = units[index].members[0];
			units[index].count = 0;
			if (mendicant_drill_file)
				fprintf(mendicant_drill_file, "%ld,joined,%d,from=%d\n", time, target->key, units[index].key);
			continue;
		}
		chief = (struct unit_datum *)command_object_get(mendicant_player_unit());
		if (chief && command_range_to(actor, chief->object.position.n) < 100.0f)
		{
			/* falls in on the Chief: fights at his side, leashed to him */
			order_actor[slot] = NONE;
			actor_role[slot] = ROLE_NONE;
			memcpy(actor_leash[slot], chief->object.position.n, 3 * sizeof(real));
			actor_leash[slot][3] = 8.0f;
			units[index].count = 0;
			continue;
		}
		/* no one left to fight with: released to the game, as when leadership is gone */
		order_actor[slot] = NONE;
		actor_role[slot] = ROLE_NONE;
		actor_leash[slot][3] = 0.0f;
		actor_disciplined[slot] = 0;
		units[index].count = 0;
	}
}

/* after an actor's decision: the models' answer, then once a second every
unit's command, the soldiers on their own, and the Marines with the Chief */
void mendicant_drill_update(void)
{
	static struct command_battle_unit units[MENDICANT_MAX_UNITS];
	long time = mendicant_time();
	int actor_count, unit_count = 0, index, other, member;

	if (mendicant_bridge)
		bridge_poll(time);
	if (time >= drill_update_time && time - drill_update_time < COMMAND_DRILL_UPDATE_TICKS)
		return;
	drill_update_time = time;

	if (!actor_data || !actor_data->valid)
		return;
	mendicant_maneuver_check_map();
	actor_count = actor_data->count;

	command_succession(actor_count, time);
	command_enrol(actor_count);

	for (index = 0; index < actor_count; index++)
	{
		struct actor_datum *actor = command_actor_at(index);
		int identifier = actor->identifier;
		struct command_battle_unit *unit = NULL;
		int key;

		if (!identifier || enrolled_actor[index] != (((long)identifier << 16) | index))
			continue;
		key = enrolled_key[index];
		for (other = 0; other < unit_count; other++)
		{
			if (units[other].key == key)
				unit = &units[other];
		}
		if (!unit)
		{
			if (unit_count == MENDICANT_MAX_UNITS)
				continue;
			unit = &units[unit_count++];
			unit->key = key;
			unit->count = 0;
		}
		if (unit->count < MENDICANT_MAX_SQUAD)
		{
			unit->members[unit->count].actor_index = ((long)identifier << 16) | index;
			unit->members[unit->count].actor = actor;
			unit->count++;
		}
	}

	command_lone_survivors(units, unit_count, time);
	for (index = 0; index < unit_count; index++)
	{
		for (member = 0; member < units[index].count; member++)
			commanded_time[(short)units[index].members[member].actor_index] = time;
		mendicant_units_update(units[index].key, units[index].members, units[index].count, time);
	}
	mendicant_individuals_update(time); /* the soldiers no unit commands: their own models */
	mendicant_follow_chief(time); /* last: nobody's order keeps a Marine back from the Chief */
	if (mendicant_drill_file)
		fflush(mendicant_drill_file);
}

/* ---------- discipline: an actor under a live command (a Marine squad, or a
Covenant unit while one of its Elites lives) does not panic, go berserk,
charge or search alone, unless its role is a charge (an assault, a bait, a
planned grenade runner). When the last Elite dies, all of it stops and the
game's own behaviour returns. */

static boolean actor_is_disciplined(long actor_index)
{
	int slot = (short)actor_index;
	struct actor_datum *actor, *leader;
	int key;

	if (slot < 0 || slot >= MENDICANT_MAX_ACTORS || !actor_disciplined[slot] || enrolled_actor[slot] != actor_index)
		return FALSE;
	actor = command_actor_get(actor_index);
	if (!actor || actor->meta.swarm || actor->input.vehicle_index != NONE)
		return FALSE; /* swarms and passengers are the game's */
	key = enrolled_key[slot];
	if (key >= MENDICANT_MAX_ACTORS)
		return TRUE; /* Marines */
	leader = command_actor_get(mendicant_units_leader(key));
	return leader && command_actor_is_elite_leader(leader); /* computed live: the rout must not be missed */
}

static boolean actor_may_charge(long actor_index)
{
	int slot = (short)actor_index;

	return actor_role[slot] == ROLE_ASSAULT || actor_role[slot] == ROLE_BAIT || actor_role[slot] == ROLE_RUNNER;
}

/* actor_action_try_to_panic's entry: TRUE refuses the panic (its return is
then FALSE, as when the flee could not be set up: the caller carries on) */
boolean mendicant_discipline_veto_panic(long actor_index)
{
	if (!actor_is_disciplined(actor_index))
		return FALSE;
	discipline_declined[0]++;
	return TRUE;
}

/* actor_berserk(actor, TRUE) at actions.c's two sites: TRUE refuses it */
boolean mendicant_discipline_veto_berserk(long actor_index)
{
	if (!actor_is_disciplined(actor_index) || actor_may_charge(actor_index))
		return FALSE;
	discipline_declined[1]++;
	return TRUE;
}

/* action_charge_setup at actions.c's melee and close-range sites: no rush at
the target (goal 0), and a melee (goal 2) only at its feet */
boolean mendicant_discipline_charge_setup(long actor_index, short goal, struct charge_state_data *state_data)
{
	if (actor_is_disciplined(actor_index) && !actor_may_charge(actor_index))
	{
		struct actor_datum *actor = command_actor_get(actor_index);
		struct prop_datum *prop = actor ? command_prop_get(actor->target.target_prop_index) : NULL;

		if (goal == 0 || (goal == 2 && (!prop || prop->distance > 2.5f)))
		{
			discipline_declined[2]++;
			return FALSE;
		}
	}
	return action_charge_setup(actor_index, goal, state_data);
}

/* actor_action_handle_lost_contact at actions.c's combat status and exit
pursuit sites: a disciplined actor that loses sight of its target stays in
the fight with its team (its leash and the plan place it) instead of being
picked to search alone */
boolean mendicant_discipline_lost_contact(long actor_index)
{
	struct actor_datum *actor = command_actor_get(actor_index);

	if (actor && actor_is_disciplined(actor_index) && actor->state.combat_status >= COMMAND_COMBAT_STATUS_ENGAGED &&
		actor->target.target_prop_index != NONE && !actor->input.vehicle_passenger)
	{
		short action = actor->state.action;

		if (action == _actor_action_fight || action == _actor_action_guard)
			return TRUE;
		if (action == _actor_action_uncover && actor->state.action_data.uncover.pursuit_location.type == _pursuit_location_target &&
			!actor->state.action_data.uncover.uncover_done && order_actor[(short)actor_index] == actor_index)
			return FALSE; /* an uncover its plan gave it */
		if (action == _actor_action_alert || action == _actor_action_flee || action == _actor_action_uncover ||
			action == _actor_action_search || action == _actor_action_wait || action == _actor_action_avoid)
		{
			static struct action_state_data data;

			discipline_declined[3]++;
			memset(&data, 0, sizeof(data));
			actor_action_change(actor_index, _actor_action_fight, &data);
			return TRUE;
		}
	}
	return actor_action_handle_lost_contact(actor_index);
}

/* every decision, before the game's: a disciplined actor that may not charge
is not forced to */
void mendicant_discipline_before_decide(long actor_index)
{
	struct actor_datum *actor = command_actor_get(actor_index);

	if (!actor || !actor_is_disciplined(actor_index) || actor_may_charge(actor_index))
		return;
	actor->emotions.forced_to_charge = FALSE;
	if (actor->emotions.berserk)
		actor_berserk(actor_index, FALSE);
}

/* ---------- state */

/* the state the hook library started with: nobody enrolled, nobody ordered */
void mendicant_command_reset(void)
{
	csmemset(order_actor, 0xff, sizeof(order_actor));
	csmemset(order_position, 0, sizeof(order_position));
	csmemset(actor_role, 0, sizeof(actor_role));
	csmemset(actor_grenade_due, 0, sizeof(actor_grenade_due));
	csmemset(actor_leash, 0, sizeof(actor_leash));
	leash_corrections = 0;
	csmemset(actor_face, 0, sizeof(actor_face));
	csmemset(unit_assault_range, 0, sizeof(unit_assault_range));
	csmemset(commanded_time, 0, sizeof(commanded_time));
	bridge_pending = FALSE;
	csmemset(actor_team_of, 0, sizeof(actor_team_of));
	csmemset(actor_mover, 0, sizeof(actor_mover));
	csmemset(enrolled_actor, 0xff, sizeof(enrolled_actor));
	csmemset(enrolled_key, 0, sizeof(enrolled_key));
	bridge_lost_last = FALSE;
	csmemset(actor_disciplined, 0, sizeof(actor_disciplined));
	bridge_request_time = 0;
	csmemset(pending_members, 0, sizeof(pending_members));
	csmemset(pending_positions, 0, sizeof(pending_positions));
	drill_update_time = 0;
	csmemset(discipline_declined, 0, sizeof(discipline_declined));
	mendicant_units_reset();
}

/* a training episode starts: every unit and order gone, the counters at 0,
and the drill's random numbers moved on by the trainer's seed */
void mendicant_command_episode_reset(unsigned long seed)
{
	csmemset(order_actor, 0xff, sizeof(order_actor));
	csmemset(actor_role, 0, sizeof(actor_role));
	csmemset(enrolled_actor, 0xff, sizeof(enrolled_actor));
	csmemset(actor_leash, 0, sizeof(actor_leash));
	csmemset(actor_disciplined, 0, sizeof(actor_disciplined));
	mendicant_units_episode_reset();
	leash_corrections = 0;
	csmemset(discipline_declined, 0, sizeof(discipline_declined));
	drill_random ^= (seed * 2246822519UL + 1) & 0xFFFFFFFFUL;
	bridge_pending = FALSE;
	bridge_lost_last = FALSE;
	drill_update_time = 0;
}

/* a training episode ends: how the command went */
void mendicant_command_episode_report(struct model_bridge *bridge)
{
	int index;

	mendicant_units_report(bridge);
	bridge->leash_corrections = leash_corrections;
	for (index = 0; index < 4; index++)
		bridge->discipline_declined[index] = discipline_declined[index];
}
