/* Optional bot objectives, adapted from thelinkin3000/halo-ce-universal
d92cc93385edf03d0a4a21cfe4c58dd578a3e786 (AGPL-3.0-or-later, with inherited GPL-3.0-only tactics). Its CTF and hill
decisions draw on id Software's and ioquake3 contributors' Quake III Arena bot rules. Precision-weapon
classification and tactics derive from BirchWoodGod/halo-ce-vita.
See docs/reading/bots_and_android_features.md for the provenance chain.

The existing bots.c owns combat, spawning and pathfinding. This module
selects destinations; it never changes engine rules or replicated state. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "memory/data.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "objects/objects.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "items/equipment_definitions.h"
#include "units/units.h"
#include "tag_files/tag_groups.h"
#include "networking/network_game_globals.h"
#include "bots.h"
#include "bots_combat.h"
#include "bots_objectives.h"
#include "bots_tactics.h"
#include "bots_vehicles.h"
#include "enhanced_bots.h"
#include "infection.h"
#include "game_modes.h"
#include <math.h>
#include <string.h>

long config_integer(char const *name);

enum { _bots_goal_none, _bots_goal_objective };
struct bots_bot
{
	long player_index;
	unsigned long random;
	short goal_kind;
	real_point3d goal_point;
	long goal_surface_index;
	real_point3d guard_point;
	long guard_until_tick;
	real_point3d hill_point;
	long hill_id;
	long hill_until_tick;
	long aim_target, aim_until;
	boolean aim_head, aim_feet;
};

static struct
{
	struct bots_bot bots[BOTS_MAXIMUM];
	long roam_point_count;
	real_point3d const *roam_points;
	struct
	{
		long tick;
		boolean present[2], at_home[2], home_to_score;
		real_point3d position[2], home[2];
		long carrier_player_index[2];
	} ctf;
	struct
	{
		long tick, point_count, hill_id;
		boolean present;
		real_point3d points[12], center;
		real bottom, top;
	} king;
} bots_globals;

static boolean bots_king_guard(struct bots_bot *bot, struct player_datum *player);
static boolean bots_king_active(void);

/* BirchWoodGod/thelinkin3000 difficulty rules, kept separate from stock bots.
 * Preserve the four named levels when using OpenCE's six-level ordering.
 * Recruit shares Easy's enhanced tactics; its base combat is weaker.
 * Veteran interpolates Heroic and Legendary, as the base skill table does. */
static struct
{
	real head, feet, heal, heal_fighting, powerup_reach;
} const enhanced_skills[NUMBER_OF_BOTS_SKILLS] =
{
	{ 0.15f, 0.0f, 0.35f, 0.00f, 15.0f }, /* Recruit */
	{ 0.15f, 0.0f, 0.35f, 0.00f, 15.0f }, /* Easy */
	{ 0.40f, 0.4f, 0.50f, 0.25f, 25.0f },
	{ 0.70f, 0.8f, 0.65f, 0.35f, 30.0f },
	{ 0.85f, 0.9f, 0.725f, 0.40f, 35.0f }, /* Veteran */
	{ 1.00f, 1.0f, 0.80f, 0.45f, 40.0f },
};

boolean bots_trace_enabled(void)
{
	return config_integer("debug.bot_trace") > 0;
}

static long bots_surface_below(real_point3d const *point, real_point3d *ground)
{
	return bots_routes_ground(point, ground);
}

static void bots_set_goal(struct bots_bot *bot, short kind, real_point3d const *point, long surface)
{
	bot->goal_kind = kind;
	bot->goal_point = *point;
	bot->goal_surface_index = surface;
}

static unsigned long bots_random(struct bots_bot *bot)
{
	/* (xorshift32) */
	unsigned long x = bot->random ? bot->random : 0x9E3779B9UL;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	x &= 0xFFFFFFFFUL;
	bot->random = x;
	return x;
}

static real bots_random_real(struct bots_bot *bot)
{
	return (real)(bots_random(bot) & 0xFFFFFF) / (real)0x1000000;
}

static real bots_distance2d(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y;

	return (real)sqrt(dx * dx + dy * dy);
}

static real bots_distance3d(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

	return (real)sqrt(dx * dx + dy * dy + dz * dz);
}

static boolean bots_ctf_active(void)
{
	return game_engine_running() && game_engine_get_variant()->game_engine_index == game_engine_ctf;
}

static boolean bots_ctf_stand(long team, real_point3d *point)
{
	struct scenario *scenario = global_scenario_get();
	long index;
	if (game_engine_get_variant()->game_engine_variant.ctf.assault)
		team = 1 - team;
	for (index = 0; scenario && index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *flag =
			TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index, struct scenario_netgame_flag);
		if (flag->type == _netgame_flag_ctf_flag && flag->team_index == team)
		{
			*point = flag->position;
			return TRUE;
		}
	}
	return FALSE;
}

static void bots_ctf_update(void)
{
	long now = game_time_get();
	long team;

	if (bots_globals.ctf.tick == now)
		return;
	bots_globals.ctf.tick = now;
	bots_globals.ctf.home_to_score = game_engine_get_variant()->game_engine_variant.ctf.flag_at_home_to_score &&
		game_engine_get_variant()->game_engine_variant.ctf.single_flag_time == 0;
	for (team = 0; team < 2; team++)
	{
		bots_globals.ctf.present[team] = game_engine_ctf_flag_state(team, &bots_globals.ctf.position[team],
			&bots_globals.ctf.carrier_player_index[team], &bots_globals.ctf.at_home[team], &bots_globals.ctf.home[team]);
		if (!bots_globals.ctf.present[team])
			bots_ctf_stand(team, &bots_globals.ctf.home[team]);
	}
}

static boolean bots_ctf_carrying(struct bots_bot *bot)
{
	long team;

	if (!bots_ctf_active())
		return FALSE;
	bots_ctf_update();
	for (team = 0; team < 2; team++)
	{
		if (bots_globals.ctf.present[team] && bots_globals.ctf.carrier_player_index[team] == bot->player_index)
			return TRUE;
	}
	return FALSE;
}

static boolean bots_ctf_defender(struct bots_bot *bot, struct player_datum *player)
{
	long team = player->team_index;
	long own_bot_index = (long)(bot - bots_globals.bots);
	struct data_iterator iterator;
	struct player_datum *other;
	long team_size = 0;
	long rank = 0;
	long defenders;

	/* (one flag's assault: the team without one attacks it, the other defends) */
	if (!bots_globals.ctf.present[team])
		return FALSE;
	if (!bots_globals.ctf.present[1 - team])
		return TRUE;
	data_iterator_new(&iterator, player_data);
	while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (other->team_index != team)
			continue;
		team_size++;
		if (bots_player_owned(other->network_player_data.machine_index) &&
			other->network_player_data.machine_index - BOTS_FIRST_MACHINE < own_bot_index)
		{
			rank++;
		}
	}
	defenders = team_size >= 2 ? MAX(1, team_size * 2 / 5) : 0;
	return rank < defenders;
}

static boolean bots_ctf_set_goal(struct bots_bot *bot, real_point3d const *point, real moved)
{
	real_point3d ground;
	long surface_index;

	if (bot->goal_kind == _bots_goal_objective && bots_distance3d(&bot->goal_point, point) < moved)
		return TRUE;
	surface_index = bots_surface_below(point, &ground);
	if (surface_index == NONE)
		return FALSE;
	bots_set_goal(bot, _bots_goal_objective, &ground, surface_index);
	return TRUE;
}

static boolean bots_ctf_urgent_goal(struct bots_bot *bot, struct player_datum *player, real_point3d const *position)
{
	long team = player->team_index;
	long other = 1 - team;
	boolean defender;

	if (team != 0 && team != 1)
		return FALSE;
	bots_ctf_update();
	defender = bots_ctf_defender(bot, player);
	/* carrying theirs: home */
	if (bots_globals.ctf.present[other] && bots_globals.ctf.carrier_player_index[other] == bot->player_index)
		return bots_ctf_set_goal(bot, &bots_globals.ctf.home[team], 1.0f);
	/* (a gametype whose captures need the team's own flag at home, ours
	carried off while a team mate carries theirs: everyone after our flag,
	or the carrier waits at home for ever, Quake III's flag standoff) */
	if (bots_globals.ctf.home_to_score && bots_globals.ctf.present[team] && bots_globals.ctf.present[other] &&
		bots_globals.ctf.carrier_player_index[team] != NONE && bots_globals.ctf.carrier_player_index[other] != NONE)
	{
		return bots_ctf_set_goal(bot, &bots_globals.ctf.position[team], 4.0f);
	}
	if (bots_globals.ctf.present[team] && !bots_globals.ctf.at_home[team])
	{
		real distance = bots_distance3d(position, &bots_globals.ctf.position[team]);

		/* its own carried off: after the carrier */
		if (bots_globals.ctf.carrier_player_index[team] != NONE && (defender || distance < 25.0f))
			return bots_ctf_set_goal(bot, &bots_globals.ctf.position[team], 4.0f);
		/* dropped: to it (to touch it home, or to guard it) */
		if (bots_globals.ctf.carrier_player_index[team] == NONE && (defender || distance < 30.0f))
			return bots_ctf_set_goal(bot, &bots_globals.ctf.position[team], 1.5f);
	}
	return FALSE;
}

static boolean bots_ctf_choose_goal(struct bots_bot *bot, struct player_datum *player, real_point3d const *position)
{
	long team = player->team_index;
	long other = 1 - team;
	long now = game_time_get();
	boolean defender;

	if (team != 0 && team != 1)
		return FALSE;
	if (bots_ctf_urgent_goal(bot, player, position))
		return TRUE;
	defender = bots_ctf_defender(bot, player);
	if (!defender && bots_globals.ctf.present[other])
	{
		long carrier = bots_globals.ctf.carrier_player_index[other];

		/* a team mate has it: with the carrier */
		if (carrier != NONE)
			return bots_ctf_set_goal(bot, &bots_globals.ctf.position[other], 4.0f);
		/* at home or dropped: to it */
		return bots_ctf_set_goal(bot, &bots_globals.ctf.position[other], 1.5f);
	}
	if (defender && bots_globals.ctf.present[team])
	{
		/* about its flag: a roaming place near it at a time, or the flag's place */
		if (now >= bot->guard_until_tick || bots_distance2d(&bot->guard_point, &bots_globals.ctf.home[team]) > 16.0f)
		{
			long index, tries;

			bot->guard_point = bots_globals.ctf.home[team];
			for (tries = 0; tries < 8 && bots_globals.roam_point_count > 0; tries++)
			{
				index = (long)(bots_random(bot) % (unsigned long)bots_globals.roam_point_count);
				if (bots_distance2d(&bots_globals.roam_points[index], &bots_globals.ctf.home[team]) < 14.0f)
				{
					bot->guard_point = bots_globals.roam_points[index];
					break;
				}
			}
			bot->guard_until_tick = now + TICKS_PER_SECOND * (6 + (long)(bots_random(bot) % 6));
		}
		return bots_ctf_set_goal(bot, &bot->guard_point, 1.5f);
	}
	return FALSE;
}

boolean bots_quake_defender(long bot_index)
{
	struct bots_bot *bot;
	struct player_datum *player;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return FALSE;
	bot = &bots_globals.bots[bot_index];
	if (bot->player_index == NONE || (player = player_try_and_get(bot->player_index)) == NULL)
		return FALSE;
	if (bots_ctf_active())
	{
		if (player->team_index != 0 && player->team_index != 1)
			return FALSE;
		bots_ctf_update();
		return bots_ctf_defender(bot, player);
	}
	if (bots_king_active())
		return bots_king_guard(bot, player);
	return FALSE;
}

static boolean bots_king_active(void)
{
	return game_engine_running() && game_engine_get_variant()->game_engine_index == game_engine_king;
}

static void bots_king_update(void)
{
	long now = game_time_get();

	if (bots_globals.king.tick == now)
		return;
	bots_globals.king.tick = now;
	bots_globals.king.present = game_engine_king_hill(bots_globals.king.points,
		(long)NUMBEROF(bots_globals.king.points), &bots_globals.king.point_count, &bots_globals.king.bottom,
		&bots_globals.king.top, &bots_globals.king.center, &bots_globals.king.hill_id);
}

static boolean bots_king_on_hill(long unit_index)
{
	bots_king_update();
	return bots_globals.king.present && unit_index != NONE &&
		game_engine_king_point_on_hill(&object_get(unit_index)->object.bounding_sphere_center);
}

static boolean bots_king_guard(struct bots_bot *bot, struct player_datum *player)
{
	long own_bot_index = (long)(bot - bots_globals.bots);
	struct data_iterator iterator;
	struct player_datum *other;
	long team_size = 0;
	long rank = 0;

	if (!game_engine_has_teams())
		return FALSE;
	data_iterator_new(&iterator, player_data);
	while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (other->team_index != player->team_index)
			continue;
		team_size++;
		if (bots_player_owned(other->network_player_data.machine_index) &&
			other->network_player_data.machine_index - BOTS_FIRST_MACHINE < own_bot_index)
		{
			rank++;
		}
	}
	return rank < (team_size >= 8 ? 2 : team_size >= 5 ? 1 : 0);
}

static boolean bots_king_place(struct bots_bot *bot, real_point3d *place)
{
	long tries;

	for (tries = 0; tries < 6; tries++)
	{
		real_point3d const *corner = &bots_globals.king.points[bots_random(bot) %
			(unsigned long)bots_globals.king.point_count];
		real along = bots_random_real(bot) * 0.75f;
		real_point3d probe;

		probe.x = bots_globals.king.center.x + (corner->x - bots_globals.king.center.x) * along;
		probe.y = bots_globals.king.center.y + (corner->y - bots_globals.king.center.y) * along;
		probe.z = bots_globals.king.center.z + (corner->z - bots_globals.king.center.z) * along + 0.5f;
		if (bots_surface_below(&probe, place) != NONE)
		{
			/* (a place the game counts: a player's middle half a unit up) */
			real_point3d middle = *place;

			middle.z += 0.5f;
			if (game_engine_king_point_on_hill(&middle))
				return TRUE;
		}
	}
	return FALSE;
}

static boolean bots_king_choose_goal(struct bots_bot *bot, struct player_datum *player, real_point3d const *position)
{
	long now = game_time_get();
	real_point3d ground;
	long surface_index;

	bots_king_update();
	if (!bots_globals.king.present || bots_globals.king.point_count <= 0)
		return FALSE;
	if (bots_king_guard(bot, player))
	{
		if (now >= bot->guard_until_tick || bots_distance2d(&bot->guard_point, &bots_globals.king.center) > 18.0f)
		{
			long index, tries;

			bot->guard_point = bots_globals.king.center;
			for (tries = 0; tries < 10 && bots_globals.roam_point_count > 0; tries++)
			{
				real distance;

				index = (long)(bots_random(bot) % (unsigned long)bots_globals.roam_point_count);
				distance = bots_distance2d(&bots_globals.roam_points[index], &bots_globals.king.center);
				if (distance >= 8.0f && distance <= 16.0f)
				{
					bot->guard_point = bots_globals.roam_points[index];
					break;
				}
			}
			bot->guard_until_tick = now + TICKS_PER_SECOND * (8 + (long)(bots_random(bot) % 7));
		}
		surface_index = bots_surface_below(&bot->guard_point, &ground);
		if (surface_index == NONE)
			return FALSE;
		bots_set_goal(bot, _bots_goal_objective, &ground, surface_index);
		return TRUE;
	}
	/* (a new place: the hill moved, this one reached, or its time up: a few
	seconds on the hill, longer on the way) */
	if (bot->hill_id != bots_globals.king.hill_id || now >= bot->hill_until_tick ||
		bot->goal_kind != _bots_goal_objective || bots_distance2d(position, &bot->hill_point) < 1.0f)
	{
		boolean on_hill = bots_king_on_hill(player->unit_index);

		if (!bots_king_place(bot, &bot->hill_point))
			bot->hill_point = bots_globals.king.center;
		bot->hill_id = bots_globals.king.hill_id;
		bot->hill_until_tick = now + TICKS_PER_SECOND * (on_hill ? 2 + (long)(bots_random(bot) % 3) : 12);
	}
	surface_index = bots_surface_below(&bot->hill_point, &ground);
	if (surface_index == NONE)
		return FALSE;
	bots_set_goal(bot, _bots_goal_objective, &ground, surface_index);
	return TRUE;
}

#define BOTS_PRECISION_SPEED 6.0f
#define BOTS_PRECISION_ERROR 0.0175f
#define BOTS_PRECISION_SCOPE 4.0f
#define BOTS_HEAVY_SPEED 1.0f
static short bots_weapon_precision_compute(long definition_index)
{
	struct weapon_definition *definition = weapon_definition_get(definition_index);
	struct weapon_trigger_definition *trigger;
	struct projectile_definition *projectile;
	real speed;

	if (definition->weapon.triggers.count <= 0)
		return _bots_precision_none;
	trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, 0, struct weapon_trigger_definition);
	if (trigger->projectile.index == NONE)
		return _bots_precision_none;
	projectile = projectile_definition_get(trigger->projectile.index);
	speed = 0.5f * (projectile->projectile.initial_velocity + projectile->projectile.final_velocity);
	if (definition->weapon.zoom_level_count < 1 || trigger->projectiles_per_shot > 1)
		return _bots_precision_none;
	if (speed >= BOTS_PRECISION_SPEED && trigger->projectile_error_angle_lower_bound <= BOTS_PRECISION_ERROR)
	{
		return definition->weapon.zoom_level_count >= 2 ||
			definition->weapon.zoom_magnification_maximum >= BOTS_PRECISION_SCOPE ? _bots_precision_long : _bots_precision_mid;
	}
	return speed > 0.0f && speed < BOTS_HEAVY_SPEED ? _bots_precision_heavy : _bots_precision_none;
}

static struct
{
	long count;
	long definitions[32];
	short precision[32];
} precision_cache;

short bots_weapon_precision(long weapon_index)
{
	struct weapon_datum *weapon;
	long index;
	short precision;
	if (weapon_index == NONE)
		return _bots_precision_none;
	weapon = weapon_get(weapon_index);
	for (index = 0; index < precision_cache.count; index++)
		if (precision_cache.definitions[index] == weapon->definition_index)
			return precision_cache.precision[index];
	precision = bots_weapon_precision_compute(weapon->definition_index);
	if (precision_cache.count < (long)NUMBEROF(precision_cache.definitions))
	{
		index = precision_cache.count++;
		precision_cache.definitions[index] = weapon->definition_index;
		precision_cache.precision[index] = precision;
	}
	return precision;
}

#include "bots_infection.inl"

void enhanced_bots_dispose_from_old_map(void)
{
	bots_tactics_dispose_from_old_map();
	bots_vehicles_reset();
	precision_cache.count = 0;
	csmemset(&bots_infection, 0, sizeof(bots_infection));
	csmemset(&bots_globals, 0, sizeof(bots_globals));
}

void enhanced_bots_initialize_for_new_map(void)
{
	long index;
	csmemset(&bots_globals, 0, sizeof(bots_globals));
	precision_cache.count = 0;
	csmemset(&bots_infection, 0, sizeof(bots_infection));
	bots_vehicles_reset();
	bots_tactics_initialize_for_new_map();
	bots_globals.ctf.tick = bots_globals.king.tick = NONE;
	for (index = 0; index < BOTS_MAXIMUM; index++)
		bots_globals.bots[index].player_index = NONE;
}

void enhanced_bots_update(long count, real_point3d const *points)
{
	bots_globals.roam_point_count = count;
	bots_globals.roam_points = points;
	bots_tactics_update();
}

void enhanced_bots_reset(long index, long player_index)
{
	struct bots_bot *bot = &bots_globals.bots[index];
	csmemset(bot, 0, sizeof(*bot));
	bot->player_index = player_index;
	bot->hill_id = NONE;
	bot->aim_target = NONE;
	bot->random = 0x2545F491UL * (unsigned long)(index + 1) ^ (unsigned long)network_game_get_random_seed();
	bots_tactics_bot_reset(index, player_index);
}

boolean enhanced_bots_goal(long index, long player_index, real_point3d const *position,
	boolean urgent_only, real_point3d *goal, long *surface)
{
	struct bots_bot *bot = &bots_globals.bots[index];
	struct player_datum *player = player_get(player_index);
	boolean found = FALSE;

	bots_tactics_check(index, player_index);
	if (bots_ctf_active())
		found = bots_ctf_urgent_goal(bot, player, position);
	if (bots_king_active())
		found = bots_king_choose_goal(bot, player, position);
	if (infection_enabled() && (!urgent_only || infection_unit_is_zombie(player->unit_index)))
		found = bots_infection_goal(bot, player, position);
	if (found)
		bots_tactics_objective(index);
	if (!found && !urgent_only)
	{
		if (bots_tactics_goal(index, player_index, position, goal, surface))
			return TRUE;
		if (bots_ctf_active())
			found = bots_ctf_choose_goal(bot, player, position);
	}
	if (found)
	{
		*goal = bot->goal_point;
		*surface = bot->goal_surface_index;
	}
	return found;
}

boolean enhanced_bots_advancing(long index, long unit_index)
{
	return bots_ctf_carrying(&bots_globals.bots[index]) ||
		(bots_king_active() && !bots_king_on_hill(unit_index)) ||
		(infection_enabled() && infection_unit_is_zombie(unit_index));
}

boolean enhanced_bots_carrying(long index)
{
	return bots_ctf_carrying(&bots_globals.bots[index]);
}

real enhanced_bots_target_priority(struct player_datum *player, long target_index)
{
	struct player_datum *target = player_get(target_index);
	if (bots_ctf_active() && (player->team_index == 0 || player->team_index == 1))
	{
		bots_ctf_update();
		if (bots_globals.ctf.present[player->team_index] &&
			bots_globals.ctf.carrier_player_index[player->team_index] == target_index) return 30.0f;
	}
	return bots_king_active() && bots_king_on_hill(target->unit_index) ? 20.0f : 0.0f;
}

void enhanced_bots_aim(long index, short skill, long target_index, long weapon_index,
	real_point3d const *eye, real_point3d *point)
{
	struct bots_bot *bot = &bots_globals.bots[index];
	long now = game_time_get();
	struct player_datum *target = player_get(target_index);
	struct object_datum *object = object_get(target->unit_index);
	char const *name = weapon_index != NONE ? tag_get_name(weapon_get(weapon_index)->definition_index) : NULL;
	boolean precision = name && (strstr(name, "sniper") || (strstr(name, "pistol") && !strstr(name, "plasma")));
	boolean explosive = name && (strstr(name, "rocket") || strstr(name, "fuel rod") || strstr(name, "plasma_cannon"));
	skill = PIN(skill, _bots_skill_recruit, _bots_skill_legendary);
	if (bot->aim_target != target_index || now >= bot->aim_until)
	{
		bot->aim_target = target_index;
		bot->aim_until = now + TICKS_PER_SECOND * 2 + bots_random(bot) % TICKS_PER_SECOND;
		bot->aim_head = bots_random_real(bot) < enhanced_skills[skill].head;
		bot->aim_feet = bots_random_real(bot) < enhanced_skills[skill].feet;
	}
	if (bot->aim_head && (game_modes_mode() == _game_mode_swat || (precision && object->object.shield_vitality <= 0.0f)))
		unit_get_head_position(target->unit_index, point);
	else if (bot->aim_feet && explosive && point->z <= eye->z + 0.3f)
	{
		real_point3d ground;
		if (bots_surface_below(&object->object.position, &ground) != NONE && object->object.position.z - ground.z < 1.5f)
		{
			*point = ground;
			point->z += 0.1f;
		}
	}
}

boolean enhanced_bots_holding_hill(long unit_index)
{
	return bots_king_active() && bots_king_on_hill(unit_index);
}

boolean enhanced_bots_retreat(struct unit_datum *unit, short skill, long hurt_tick)
{
	return game_time_get() - hurt_tick < TICKS_PER_SECOND && unit->object.shield_vitality <= 0.05f &&
		unit->object.body_vitality < enhanced_skills[PIN(skill, _bots_skill_recruit, _bots_skill_legendary)].heal_fighting + 0.1f;
}

/* BirchWoodGod's health/overshield/camouflage priorities, using the base
bot's item scan, unreachable-item memory and pickup lifecycle. */
real enhanced_bots_powerup_score(struct unit_datum *unit, long definition_index, real distance,
	short skill, boolean fighting, boolean holding)
{
	real threshold, reach = fighting ? 10.0f : enhanced_skills[PIN(skill, _bots_skill_recruit, _bots_skill_legendary)].powerup_reach;
	if (game_engine_get_variant()->game_engine_index != game_engine_slayer) reach = MIN(reach, 12.0f);
	if (holding) reach = MIN(reach, 6.0f);
	switch (equipment_definition_get(definition_index)->equipment.powerup_type)
	{
	case _equipment_powerup_health:
		threshold = fighting ? enhanced_skills[PIN(skill, _bots_skill_recruit, _bots_skill_legendary)].heal_fighting : enhanced_skills[PIN(skill, _bots_skill_recruit, _bots_skill_legendary)].heal;
		if (unit->object.body_vitality < threshold && distance <= (fighting ? 15.0f : 45.0f))
			return distance - (threshold - unit->object.body_vitality) * 40.0f;
		break;
	case _equipment_powerup_overshield:
		if (unit->object.shield_vitality <= 1.0f && distance <= reach)
			return distance - 6.0f;
		break;
	case _equipment_powerup_active_camouflage:
		if (!TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit) && distance <= reach)
			return distance - 6.0f;
		break;
	}
	return REAL_MAX;
}
