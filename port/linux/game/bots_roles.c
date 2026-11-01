/* Source: thelinkin3000/halo-ce-universal, commit
 * d92cc93385edf03d0a4a21cfe4c58dd578a3e786, port/linux/game/bots_roles.c.
 * Derived from BirchWoodGod/halo-ce-vita (GPL-3.0-only).
 * Used only by GulchCE's optional enhanced bots.
 */
/* Optional enhanced bots' team roles, lane assignment and precision-weapon
vantage points. CTF defenders follow enhanced_bots.c's objective policy;
King bots stay on the hill. Bots with suitable weapons may hold a vantage
point until hurt, low on ammunition, approached by an enemy, or needed for
an urgent objective. The original bots.c still owns combat and paths.

Roles refresh once a second. Route and vantage preparation advances in
bounded steps from bots_tactics_update; diagnostics report its actual cost.
HALO_BOT_ROLES=0 disables roles and vantage choices for diagnostic runs. */

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "networking/network_game_globals.h"
#include "infection.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"

#include "bots.h"
#include "bots_tactics.h"
#include "bots_objectives.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* enhanced_bots.c: the objective policy that assigns defenders. */
boolean bots_quake_defender(long bot_index);
unsigned long system_milliseconds(void);

/* ---------- constants */

#define BOTS_TACTICS_PI 3.14159265f
/* how near its vantage point a bot is there */
#define BOTS_VANTAGE_ARRIVED 2.0f
/* an enemy this near: off the vantage point, to fight (a middle precision
weapon's bot: nearer) */
#define BOTS_VANTAGE_ENEMY_NEAR 9.0f
#define BOTS_VANTAGE_ENEMY_NEAR_MID 6.0f
/* a middle precision weapon's bot: a vantage point this near it at most */
#define BOTS_VANTAGE_MID_REACH 35.0f
/* how much further a bot sees through a long precision weapon's scope at
its vantage point */
#define BOTS_VANTAGE_SCOPE_SIGHT 1.5f
/* the most teams the roles are counted by */
#define BOTS_TACTICS_TEAMS 16

/* ---------- globals */

struct bots_tactics_bot bots_tactics_bots[BOTS_MAXIMUM];
struct bots_tactics_counts bots_tactics_counts;

static struct
{
	long game;
	/* the map's lanes and vantage points made, since when, the work's time */
	boolean prepared;
	long prepare_ticks;
	unsigned long prepare_ms;
	unsigned long prepare_most_ms;
	/* when the roles are next given; their last line (halo.log's, when it
	changes) */
	long roles_tick;
	char roles_line[600];
	/* each team's bots */
	long team_bots[BOTS_TACTICS_TEAMS];
} bots_tactics;

/* ---------- small helpers */

boolean bots_tactics_enabled(void)
{
	static int enabled = -1;

	if (enabled < 0)
	{
		char const *setting = getenv("HALO_BOT_ROLES");

		enabled = setting && setting[0] && !atoi(setting) ? 0 : 1;
	}
	return enabled != 0;
}

long bots_tactics_game(void)
{
	return bots_tactics.game;
}

unsigned long bots_tactics_random(struct bots_tactics_bot *state)
{
	/* (xorshift32) */
	unsigned long x = state->random ? state->random : 0x9E3779B9UL;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	x &= 0xFFFFFFFFUL;
	state->random = x;
	return x;
}

/* seconds, from `low` to `high`, in ticks */
static long bots_tactics_random_ticks(struct bots_tactics_bot *state, long low, long high)
{
	return TICKS_PER_SECOND * low + (long)(bots_tactics_random(state) % (unsigned long)(TICKS_PER_SECOND * (high - low) + 1));
}

static real bots_tactics_distance2d(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y;

	return (real)sqrt(dx * dx + dy * dy);
}

static char const *bots_role_name(short role)
{
	static char const *const names[NUMBER_OF_BOTS_ROLES] = { "attacker", "defender", "roamer" };

	return role >= 0 && role < NUMBER_OF_BOTS_ROLES ? names[role] : "?";
}

/* the unit's current weapon, NONE without one */
static long bots_tactics_weapon(long unit_index)
{
	struct unit_datum *unit = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);

	if (!unit || unit->unit.current_weapon_index < 0 || unit->unit.current_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT)
		return NONE;
	return unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
}

/* the precision of the unit's current weapon (bots.c's, from its tag) */
static short bots_tactics_precision(long unit_index)
{
	return bots_weapon_precision(bots_tactics_weapon(unit_index));
}

/* whether the precision weapon held still keeps the bot at the vantage
point it went to for the precision `wanted`: a long or heavy one's for a
long or heavy one, a middle one's for any precision weapon */
static boolean bots_tactics_still_precise(short wanted, short held)
{
	if (wanted == _bots_precision_mid)
		return held == _bots_precision_mid || held == _bots_precision_long;
	return held == _bots_precision_long || held == _bots_precision_heavy;
}

/* the precision weapon the bot would hold a vantage point for, by its role
and the game: none when its part keeps it elsewhere (King's attackers in
the hill; a CTF defender but with a long one, over its stand; a middle one
a roamer's alone) */
static short bots_tactics_weapon_wants(struct bots_tactics_bot const *state, short precision)
{
	if (precision == _bots_precision_none || bots_tactics.game == game_engine_king || infection_enabled())
	{
		return _bots_precision_none;
	}
	switch (precision)
	{
	case _bots_precision_long:
		return _bots_precision_long;
	case _bots_precision_heavy:
		return state->role != _bots_role_defender ? _bots_precision_heavy : _bots_precision_none;
	case _bots_precision_mid:
		return state->role == _bots_role_roamer ? _bots_precision_mid : _bots_precision_none;
	}
	return _bots_precision_none;
}

static char const *bots_precision_name(short precision)
{
	return precision == _bots_precision_long ? "long precision" : precision == _bots_precision_mid ? "middle precision" :
		precision == _bots_precision_heavy ? "heavy" : "none";
}

static char const *bots_vantage_left_name(short why)
{
	static char const *const names[NUMBER_OF_BOTS_VANTAGE_LEFT] =
		{ "time", "enemy near", "ammunition", "hurt", "weapon", "objective", "other" };

	return why >= 0 && why < NUMBER_OF_BOTS_VANTAGE_LEFT ? names[why] : "?";
}

/* whether a weapon is nearly empty: what is left of it half a magazine at
most (one with a magazine that loads rounds) */
static boolean bots_tactics_weapon_low(long weapon_index)
{
	struct weapon_datum *weapon;
	struct weapon_definition *definition;
	struct weapon_magazine_definition *magazine;

	if (weapon_index == NONE)
		return FALSE;
	weapon = weapon_get(weapon_index);
	definition = weapon_definition_get(weapon->definition_index);
	if (definition->weapon.magazines.count <= 0)
		return FALSE;
	magazine = TAG_BLOCK_GET_ELEMENT(&definition->weapon.magazines, 0, struct weapon_magazine_definition);
	/* (a battery's weapon, the plasma pistol's, loads no rounds) */
	if (magazine->rounds_loaded_maximum <= 0)
		return FALSE;
	return weapon->weapon.magazines[0].rounds_loaded + weapon->weapon.magazines[0].rounds_total <=
		MAX(1, magazine->rounds_loaded_maximum / 2);
}

/* ---------- the roles */

/* every bot's role and lane, once a second; halo.log's roles line when they
change */
static void bots_roles_assign(void)
{
	boolean teams = game_engine_has_teams();
	long attackers[BOTS_TACTICS_TEAMS];
	char line[600];
	int length = 0;
	long index;

	csmemset(bots_tactics.team_bots, 0, sizeof(bots_tactics.team_bots));
	csmemset(attackers, 0, sizeof(attackers));
	for (index = 0; index < BOTS_MAXIMUM; index++)
	{
		struct bots_tactics_bot *state = &bots_tactics_bots[index];
		struct player_datum *player = state->player_index != NONE ? player_try_and_get(state->player_index) : NULL;

		state->team = NONE;
		if (!player)
			continue;
		state->team = teams ? PIN(player->team_index, 0, BOTS_TACTICS_TEAMS - 1) : 0;
		bots_tactics.team_bots[state->team]++;
	}
	for (index = 0; index < BOTS_MAXIMUM; index++)
	{
		struct bots_tactics_bot *state = &bots_tactics_bots[index];
		short role, lane = 0;

		if (state->team == NONE)
			continue;
		role = bots_tactics_enabled() && bots_quake_defender(index) ? _bots_role_defender :
			bots_tactics.game == game_engine_slayer ? _bots_role_roamer : _bots_role_attacker;
		if (role == _bots_role_attacker || role == _bots_role_roamer)
		{
			state->rank = (short)attackers[state->team]++;
			lane = bots_routes_lane_for_rank(state->rank);
		}
		if (role != state->role)
		{
			bots_vantage_release(index);
			state->role = role;
		}
		if (lane != state->lane)
		{
			state->lane = lane;
			state->waypoint = NONE;
			csmemset(state->passed, 0, sizeof(state->passed));
			state->lane_given_up = FALSE;
		}
		if (length < (int)sizeof(line) - 48)
		{
			length += snprintf(line + length, sizeof(line) - (size_t)length, "%sbot %ld team %ld %s", length ? ", " : "",
				index + 1, state->team, bots_role_name(role));
			if (role == _bots_role_attacker || role == _bots_role_roamer)
				length += snprintf(line + length, sizeof(line) - (size_t)length, " (%s)", bots_routes_lane_name(lane));
		}
	}
	line[length] = 0;
	if (length && strcmp(line, bots_tactics.roles_line))
	{
		strcpy(bots_tactics.roles_line, line);
		platform_log("bots: roles (routes %s): %s", bots_routes_mode() == _bots_routes_lanes ? "lanes" :
			bots_routes_mode() == _bots_routes_mixed ? "mixed" : "direct", line);
	}
}

/* ---------- what bots.c tells */

void bots_tactics_initialize_for_new_map(void)
{
	long index;

	bots_routes_reset();
	bots_vantage_dispose();
	csmemset(&bots_tactics_counts, 0, sizeof(bots_tactics_counts));
	bots_tactics.prepared = FALSE;
	bots_tactics.prepare_ticks = 0;
	bots_tactics.prepare_ms = 0;
	bots_tactics.prepare_most_ms = 0;
	bots_tactics.roles_tick = 0;
	bots_tactics.roles_line[0] = 0;
	for (index = 0; index < BOTS_MAXIMUM; index++)
	{
		csmemset(&bots_tactics_bots[index], 0, sizeof(bots_tactics_bots[index]));
		bots_tactics_bots[index].player_index = NONE;
		bots_tactics_bots[index].team = NONE;
		bots_tactics_bots[index].vantage = NONE;
		bots_tactics_bots[index].last_vantage = NONE;
		bots_tactics_bots[index].waypoint = NONE;
	}
}

void bots_tactics_dispose_from_old_map(void)
{
	bots_routes_reset();
	bots_vantage_dispose();
	bots_tactics.prepared = FALSE;
}

void bots_tactics_update(void)
{
	long now = game_time_get();

	bots_tactics.game = game_engine_get_variant()->game_engine_index;
	/* the map's lanes and vantage points, a bounded step a tick (the work a
	tick fixed, not timed: the same ticks from run to run) */
	if (!bots_tactics.prepared)
	{
		unsigned long started = system_milliseconds(), spent;

		if (!bots_routes_map.ready)
			bots_routes_step();
		else if (bots_vantage_step())
		{
			bots_tactics.prepared = TRUE;
			bots_routes_release_graph();
			platform_log("bots: tactics made in %ld ticks: %lu ms in all, at most %lu ms a tick (1 ms clock) (routes %s, %ld side lanes)",
				bots_tactics.prepare_ticks + 1, bots_tactics.prepare_ms,
				bots_tactics.prepare_most_ms, bots_routes_mode() == _bots_routes_direct ? "direct" :
				bots_routes_mode() == _bots_routes_lanes ? "lanes" : "mixed", bots_routes_map.lane_count);
			bots_tactics.roles_tick = 0;
		}
		spent = system_milliseconds() - started;
		bots_tactics.prepare_ticks++;
		bots_tactics.prepare_ms += spent;
		if (spent > bots_tactics.prepare_most_ms)
			bots_tactics.prepare_most_ms = spent;
	}
	if (now >= bots_tactics.roles_tick || now < bots_tactics.roles_tick - TICKS_PER_SECOND)
	{
		bots_tactics.roles_tick = now + TICKS_PER_SECOND;
		bots_routes_read_setting();
		bots_roles_assign();
	}
}

void bots_tactics_bot_reset(long bot_index, long player_index)
{
	struct bots_tactics_bot *state;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return;
	state = &bots_tactics_bots[bot_index];
	bots_vantage_release(bot_index);
	csmemset(state, 0, sizeof(*state));
	state->player_index = player_index;
	state->random = (0x9E3779B1UL * (unsigned long)(bot_index + 7) ^ (unsigned long)network_game_get_random_seed() ^
		0x5BD1E995UL) & 0xFFFFFFFFUL;
	state->team = NONE;
	state->vantage = NONE;
	state->last_vantage = NONE;
	state->waypoint = NONE;
	state->role = NONE;
	/* (its role and lane at once) */
	bots_tactics.roles_tick = 0;
}

void bots_tactics_bot_dead(long bot_index)
{
	struct bots_tactics_bot *state;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return;
	state = &bots_tactics_bots[bot_index];
	if (state->vantage != NONE)
		bots_tactics_counts.vantage_leaves[_bots_vantage_left_other]++;
	bots_vantage_release(bot_index);
	state->waypoint = NONE;
	csmemset(state->passed, 0, sizeof(state->passed));
	state->lane_given_up = FALSE;
}

/* off its vantage point, for why: the game's goals a while before another */
static void bots_tactics_leave(long bot_index, short why)
{
	struct bots_tactics_bot *state = &bots_tactics_bots[bot_index];
	boolean scoped = state->vantage_weapon == _bots_precision_long;
	short weapon = state->vantage_weapon;
	long now = game_time_get();

	bots_tactics_counts.vantage_leaves[why]++;
	platform_log("bots: bot %ld (%s) off vantage point %d (%s) after %.0f s, held %.0f s%s%s", bot_index + 1,
		bots_role_name(state->role), (int)state->vantage + 1, bots_vantage_left_name(why),
		(double)(now - state->vantage_tick) / TICKS_PER_SECOND,
		state->hold_tick ? (double)(now - state->hold_tick) / TICKS_PER_SECOND : 0.0,
		weapon != _bots_precision_none ? ", there for its " : "", weapon != _bots_precision_none ? bots_precision_name(weapon) : "");
	/* (debug: HALO_BOT_TRACE, each bot off its vantage point, why, its weapon) */
	if (bots_trace_enabled())
	{
		struct player_datum *player = player_try_and_get(state->player_index);
		long weapon_index = player ? bots_tactics_weapon(player->unit_index) : NONE;

		platform_log("bots: trace bot %ld (%s) off vantage point %d (%s) after %ld ticks, held %ld, weapon %s %d/%d", bot_index + 1,
			bots_role_name(state->role), (int)state->vantage + 1, bots_vantage_left_name(why), game_time_get() - state->vantage_tick,
			state->hold_tick ? game_time_get() - state->hold_tick : 0L,
			weapon_index != NONE ? tag_get_name(weapon_get(weapon_index)->definition_index) : "none",
			weapon_index != NONE ? (int)weapon_get(weapon_index)->weapon.magazines[0].rounds_loaded : 0,
			weapon_index != NONE ? (int)weapon_get(weapon_index)->weapon.magazines[0].rounds_total : 0);
	}
	state->rest_weapon = weapon;
	bots_vantage_release(bot_index);
	/* (the game's goals a while: a sniper's, or a long precision weapon's
	bot, soon back at it, sooner after a fight close up; at once after its
	weapon changed, which may be taken up again; a while after the
	objective) */
	if (why == _bots_vantage_left_weapon)
		state->rest_until_tick = now + TICKS_PER_SECOND * 3;
	else if (why == _bots_vantage_left_objective)
		state->rest_until_tick = now + TICKS_PER_SECOND * 10;
	else if (scoped)
		state->rest_until_tick = now + (why == _bots_vantage_left_enemy ? bots_tactics_random_ticks(state, 8, 15) :
			bots_tactics_random_ticks(state, 10, 20));
	else
		state->rest_until_tick = now + bots_tactics_random_ticks(state, 30, 45);
}

/* why the bot leaves its vantage point now, NONE to stay */
static short bots_tactics_leave_reason(long bot_index, struct player_datum *player, long now)
{
	struct bots_tactics_bot *state = &bots_tactics_bots[bot_index];
	struct object_datum *object = object_try_and_get(player->unit_index);
	long weapon_index = bots_tactics_weapon(player->unit_index);

	/* (its weapon first: the reason that says most) */
	if (state->vantage_weapon != _bots_precision_none)
	{
		short precision = bots_weapon_precision(weapon_index);

		if (!bots_tactics_still_precise(state->vantage_weapon, precision))
			return _bots_vantage_left_weapon;
		/* (a sniper rifle taken up at, or on the way to, a pistol's or a
		rocket launcher's vantage point: held as a sniper rifle's from now) */
		if (precision == _bots_precision_long && state->vantage_weapon != _bots_precision_long)
		{
			state->vantage_weapon = _bots_precision_long;
			state->hold_ticks = (state->hold_tick ? now - state->hold_tick : 0) + bots_tactics_random_ticks(state, 35, 50);
			platform_log("bots: bot %ld (%s) %s vantage point %d for its long precision now, for %.0f s", bot_index + 1,
				bots_role_name(state->role), state->hold_tick ? "holds" : "goes to", (int)state->vantage + 1,
				(double)(state->hold_ticks - (state->hold_tick ? now - state->hold_tick : 0)) / TICKS_PER_SECOND);
		}
	}
	if (bots_tactics_weapon_low(weapon_index))
		return _bots_vantage_left_ammunition;
	if (object && object->object.body_vitality < 0.34f)
		return _bots_vantage_left_hurt;
	if (state->hold_tick && now - state->hold_tick >= state->hold_ticks)
		return _bots_vantage_left_time;
	if (!state->hold_tick && now - state->vantage_tick > TICKS_PER_SECOND * 60)
	{
		SET_FLAG(state->failed_vantages, state->vantage, TRUE);
		state->failed_until_tick = now + TICKS_PER_SECOND * 60;
		bots_tactics_counts.vantage_failures++;
		return _bots_vantage_left_other;
	}
	return NONE;
}

/* A free vantage point suitable for the weapon: a CTF defender's over its
own stand, a pistol's nearby; while its team has fewer holding one than a third of its bots
(half for a long precision weapon's, a pistol's one apart). None: looked for
again in a few seconds */
static void bots_tactics_seek(long bot_index, real_point3d const *position,
	short weapon_wants)
{
	struct bots_tactics_bot *state = &bots_tactics_bots[bot_index];
	long now = game_time_get();
	real_point3d hill, *objective = NULL;
	long index, holders = 0, middle_holders = 0, most;
	short vantage;

	if (state->role == _bots_role_defender && bots_tactics.game == game_engine_ctf)
	{
		/* A CTF defender keeps sight of its own stand. */
		if (!bots_vantage_stand(state->team, &hill))
			return;
		objective = &hill;
	}
	for (index = 0; index < BOTS_MAXIMUM; index++)
	{
		/* (a pistol's bot, one a team, is not counted: it keeps no sniper
		from a vantage point) */
		if (index != bot_index && bots_tactics_bots[index].vantage != NONE && bots_tactics_bots[index].team == state->team)
		{
			if (bots_tactics_bots[index].vantage_weapon == _bots_precision_mid)
				middle_holders++;
			else
				holders++;
		}
	}
	most = weapon_wants == _bots_precision_long ? MAX(1, (bots_tactics.team_bots[state->team] + 1) / 2) :
		MAX(1, bots_tactics.team_bots[state->team] / 3);
	vantage = (weapon_wants == _bots_precision_mid ? !middle_holders : holders < most) ?
		bots_vantage_choose(bot_index, position, objective,
			weapon_wants == _bots_precision_mid ? BOTS_VANTAGE_MID_REACH : REAL_MAX) : NONE;
	if (vantage == NONE)
	{
		state->rest_until_tick = now + TICKS_PER_SECOND * 5;
		state->rest_weapon = weapon_wants;
		return;
	}
	state->vantage = vantage;
	state->vantage_weapon = weapon_wants;
	state->vantage_tick = now;
	state->hold_tick = 0;
	state->hold_ticks = weapon_wants == _bots_precision_long ?
		bots_tactics_random_ticks(state, 35, 50) :
		weapon_wants == _bots_precision_mid ? bots_tactics_random_ticks(state, 12, 20) : bots_tactics_random_ticks(state, 20, 30);
	bots_tactics_counts.vantages_chosen++;
}

boolean bots_tactics_goal(long bot_index, long player_index, real_point3d const *position, real_point3d *goal,
	long *surface_index)
{
	struct bots_tactics_bot *state;
	struct player_datum *player;
	long now = game_time_get();

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM || !bots_tactics_enabled() || !bots_tactics.prepared ||
		bots_tactics.game == game_engine_race || bots_tactics.game == game_engine_none)
	{
		return FALSE;
	}
	state = &bots_tactics_bots[bot_index];
	player = player_try_and_get(player_index);
	if (!player || player->unit_index == NONE || state->team == NONE)
		return FALSE;
	/* a vantage point it goes to, or holds */
	if (state->vantage != NONE)
	{
		short why = bots_tactics_leave_reason(bot_index, player, now);

		if (why != NONE)
			bots_tactics_leave(bot_index, why);
		else
		{
			bots_vantage_point(state->vantage, goal, surface_index);
			if (!state->hold_tick && bots_tactics_distance2d(position, goal) < BOTS_VANTAGE_ARRIVED &&
				fabs(position->z - goal->z) < 1.5f)
			{
				state->hold_tick = now;
				bots_tactics_counts.vantages_reached++;
				platform_log("bots: bot %ld (%s) at vantage point %d (%.1f %.1f %.1f), for %.0f s%s%s", bot_index + 1,
					bots_role_name(state->role), (int)state->vantage + 1, goal->x, goal->y, goal->z,
					(double)state->hold_ticks / TICKS_PER_SECOND,
					state->vantage_weapon != _bots_precision_none ? ", for its " : "",
					state->vantage_weapon != _bots_precision_none ? bots_precision_name(state->vantage_weapon) : "");
			}
			return TRUE;
		}
	}
	/* one to go to: a sniper's, a precision weapon's (a sniper rifle's, a
	pistol's near) or a rocket launcher's, a defender's over the hill (a CTF
	defender's with a sniper rifle over its stand) */
	{
		/* (no role holds one by itself: Quake III's defenders keep to their
		flag and the hill, bots.c) */
		short weapon_wants =
			bots_tactics_weapon_wants(state, bots_tactics_precision(player->unit_index));

		/* (resting a while from a vantage point, the game's goals first;
		but a sniper rifle taken up since one held for another weapon, a
		pistol or a rocket launcher, goes to one at once) */
		if (now < state->rest_until_tick &&
			!(weapon_wants == _bots_precision_long && state->rest_weapon != _bots_precision_long))
		{
			weapon_wants = _bots_precision_none;
		}
		if (weapon_wants != _bots_precision_none)
			bots_tactics_seek(bot_index, position, weapon_wants);
		if (state->vantage != NONE)
		{
			bots_vantage_point(state->vantage, goal, surface_index);
			return TRUE;
		}
	}
	/* (CTF's defenders keep about their flag by Quake III's rules: bots.c's
	bots_ctf_choose_goal) */
	return FALSE;
}

boolean bots_tactics_search_answer(long bot_index, real_point3d const *goal, boolean reached, boolean spent)
{
	struct bots_tactics_bot *state;
	real_point3d point;
	long surface_index;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM || reached || !spent)
		return FALSE;
	if (bots_routes_search_failed(bot_index, goal))
		return TRUE;
	state = &bots_tactics_bots[bot_index];
	if (state->vantage == NONE)
		return FALSE;
	bots_vantage_point(state->vantage, &point, &surface_index);
	if (fabs(point.x - goal->x) + fabs(point.y - goal->y) + fabs(point.z - goal->z) > 0.5f)
		return FALSE;
	SET_FLAG(state->failed_vantages, state->vantage, TRUE);
	state->failed_until_tick = game_time_get() + TICKS_PER_SECOND * 60;
	bots_tactics_counts.vantage_failures++;
	bots_tactics_leave(bot_index, _bots_vantage_left_other);
	state->rest_until_tick = game_time_get() + TICKS_PER_SECOND * 5;
	return TRUE;
}

void bots_tactics_look(long bot_index, real_point3d const *position, long now, real *yaw, real *pitch)
{
	struct bots_tactics_bot *state;
	unsigned long watched = 0;
	long count = bots_places_count(), index;
	struct bots_place const *place;
	real desired_yaw, desired_pitch, off, dx, dy, dz;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM || !bots_tactics.prepared || count <= 0)
		return;
	state = &bots_tactics_bots[bot_index];
	if (state->vantage != NONE && state->hold_tick)
		watched = bots_vantage_sees(state->vantage);
	if (!watched)
		return;
	if (now >= state->look_until_tick || !TEST_FLAG(watched, state->look_place))
	{
		/* (the next place watched, in turn) */
		for (index = 1; index <= count; index++)
		{
			long next = (state->look_place + index) % count;

			if (TEST_FLAG(watched, next) && bots_tactics_distance2d(&bots_place_get(next)->point, position) > 3.0f)
			{
				state->look_place = (short)next;
				break;
			}
		}
		state->look_until_tick = now + TICKS_PER_SECOND * 2 + (long)(bots_tactics_random(state) % (TICKS_PER_SECOND * 2));
	}
	place = bots_place_get(state->look_place);
	dx = place->point.x - position->x;
	dy = place->point.y - position->y;
	dz = place->point.z - (position->z + 0.6f);
	if (dx * dx + dy * dy < 1.0f)
		return;
	desired_yaw = (real)atan2(dy, dx);
	desired_pitch = (real)atan2(dz, sqrt(dx * dx + dy * dy));
	off = (real)fmod(desired_yaw - *yaw, 2.0f * BOTS_TACTICS_PI);
	if (off > BOTS_TACTICS_PI)
		off -= 2.0f * BOTS_TACTICS_PI;
	else if (off < -BOTS_TACTICS_PI)
		off += 2.0f * BOTS_TACTICS_PI;
	/* (turning at six degrees a tick) */
	*yaw += PIN(off, -0.105f, 0.105f);
	*pitch += PIN(desired_pitch - *pitch, -0.05f, 0.05f);
}

void bots_tactics_fight(long bot_index, real_point3d const *position, real target_distance, boolean *moving,
	real *direction, real *throttle)
{
	struct bots_tactics_bot *state;
	real_point3d point;
	long surface_index;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return;
	state = &bots_tactics_bots[bot_index];
	if (state->vantage == NONE || !state->hold_tick)
		return;
	if (target_distance < (state->vantage_weapon == _bots_precision_mid ? BOTS_VANTAGE_ENEMY_NEAR_MID : BOTS_VANTAGE_ENEMY_NEAR))
	{
		bots_tactics_leave(bot_index, _bots_vantage_left_enemy);
		return;
	}
	bots_vantage_point(state->vantage, &point, &surface_index);
	if (bots_tactics_distance2d(position, &point) > 1.2f)
	{
		*direction = (real)atan2(point.y - position->y, point.x - position->x);
		*moving = TRUE;
		*throttle = 0.7f;
	}
	else
		*throttle *= 0.3f;
}

void bots_tactics_check(long bot_index, long player_index)
{
	struct player_datum *player;
	short why;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM || bots_tactics_bots[bot_index].vantage == NONE ||
		(player = player_try_and_get(player_index)) == NULL || player->unit_index == NONE)
	{
		return;
	}
	why = bots_tactics_leave_reason(bot_index, player, game_time_get());
	if (why != NONE)
		bots_tactics_leave(bot_index, why);
}

void bots_tactics_objective(long bot_index)
{
	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM || bots_tactics_bots[bot_index].vantage == NONE)
		return;
	bots_tactics_leave(bot_index, _bots_vantage_left_objective);
}

real bots_tactics_sight_scale(long bot_index)
{
	struct bots_tactics_bot const *state;
	struct player_datum *player;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return 1.0f;
	state = &bots_tactics_bots[bot_index];
	if (state->vantage == NONE || !state->hold_tick || state->player_index == NONE ||
		(player = player_try_and_get(state->player_index)) == NULL || player->unit_index == NONE)
	{
		return 1.0f;
	}
	return bots_tactics_precision(player->unit_index) == _bots_precision_long ? BOTS_VANTAGE_SCOPE_SIGHT : 1.0f;
}

static long bots_tactics_leaves_total(void)
{
	long index, total = 0;

	for (index = 0; index < NUMBER_OF_BOTS_VANTAGE_LEFT; index++)
		total += bots_tactics_counts.vantage_leaves[index];
	return total;
}

void bots_tactics_report(long now)
{
	if (!bots_tactics.prepared)
		return;
	platform_log("bots: tactics at tick %ld: routes %s (%ld side lanes): lanes taken %ld, waypoints passed %ld, given up %ld; "
		"vantage points %ld: chosen %ld, reached %ld, left %ld (time %ld, enemy near %ld, ammunition %ld, hurt %ld, weapon %ld, "
		"objective %ld, other %ld), no way %ld%s", now,
		bots_routes_mode() == _bots_routes_direct ? "direct" : bots_routes_mode() == _bots_routes_lanes ? "lanes" : "mixed",
		bots_routes_map.lane_count, bots_tactics_counts.lanes_taken, bots_tactics_counts.waypoints_passed,
		bots_tactics_counts.lanes_given_up, bots_vantage_count(), bots_tactics_counts.vantages_chosen,
		bots_tactics_counts.vantages_reached,
		bots_tactics_leaves_total(),
		bots_tactics_counts.vantage_leaves[_bots_vantage_left_time], bots_tactics_counts.vantage_leaves[_bots_vantage_left_enemy],
		bots_tactics_counts.vantage_leaves[_bots_vantage_left_ammunition], bots_tactics_counts.vantage_leaves[_bots_vantage_left_hurt],
		bots_tactics_counts.vantage_leaves[_bots_vantage_left_weapon], bots_tactics_counts.vantage_leaves[_bots_vantage_left_objective],
		bots_tactics_counts.vantage_leaves[_bots_vantage_left_other], bots_tactics_counts.vantage_failures,
		bots_tactics_enabled() ? "" : " (no roles nor vantage points: HALO_BOT_ROLES=0)");
}
