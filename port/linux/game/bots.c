/*
BOTS.C

Computer players (bots) for local multiplayer games (bots.count): the lobby
of a local (split screen) Slayer game takes as many as the setting asks for
(network_server_manager.c adds them), and each plays as a player does -
spawns, respawns, scores, is in the scores and the game's rules - with its
controls from here instead of a controller: the game tick takes every bot's
action from bots_update_actions (players_update_before_game), where a
player's comes from its pad. A bot has no view: nothing is drawn for it.

What a bot does each tick, in a player's terms:

- It looks for enemies (every third tick, the bots in turn): the nearest
  player of the other team (anyone in a game without teams) within its
  sight's reach and field of view whom a line of sight reaches (the AI's own
  sight test: the level, vehicles, scenery and machines block it), nearer
  for one in active camouflage; and it notices who shot it.
- It reacts after a moment (its skill's reaction time), then turns towards
  the target at a limited speed, with an aim error that settles while it
  keeps the target in sight (and leads a moving target by its weapon's
  projectile speed), and fires in bursts while its aim is on the target
  (released between bursts, so semi-automatic weapons fire too); it throws
  a grenade now and then at middle range and strikes with its weapon close
  up. It strafes while fighting, jumps now and then, and keeps its distance.
- Otherwise it goes where an enemy was last seen, else about the map (the
  player starting locations, the weapon and item spawns), most often to the
  place nearest a living enemy, as a player reads the motion tracker; now
  and then it picks up a better weapon lying near (the action button, as a
  player holds it), and swaps to its other weapon when this one is empty.
- It walks the paths the campaign's AI finds: the engine's own pathfinding
  over the level's walkable surfaces (source/ai/path.c), which every stock
  multiplayer map carries (tool builds it with the structure BSP: Blood
  Gulch has 2807 walkable surfaces of 4916; only the AI's placements,
  encounters and firing positions are missing). A bot that stops moving
  jumps and sidesteps, then finds another path. The multiplayer maps were
  never made for the AI, and the bots' searches read their own copy of the
  walkable surfaces: those with no room above them (a floor going on under
  a wall: Longest's bases) closed a few a tick from the game's start, and
  those under scenery or a machine the path's obstacle avoidance once found
  no way past (Sidewinder's blast door between the bases) closed from then
  on. A goal on ground the walkable surfaces do not reach is gone to as near
  as they go and on with jumps; an item it finds no way to is left be a
  while. Bots in each other's way give way by their order.

Skill (bots.skill): recruit, easy, normal, heroic, veteran, legendary - how
soon it reacts, how far and how wide it sees, how large its aim error is and
how fast that settles, how fast it turns, how tight its shots must be before
it fires, its bursts and its grenades. easy, normal, heroic and legendary
are the levels the bots always had; recruit (for new players) is below
easy, and veteran is halfway from heroic to legendary.

Fighting by level: a bot fights as a player of its level does, by
bots_combat.c's rules - a reaction and a look that are
never the same twice, an aim that trails the target at the lowest levels
and keeps an error at range, headshots once the shields are down, the
pistol paced at range and the sniper rifle zoomed (again, after a hit),
blows and grenades by level (a throw that leads the target's path, one at
the spot it was last seen), strafes and halts of the level's rhythm (short
jiggles at the top), jumps and crouches, a step in for a blow, a run in to
finish a won fight, a retreat along a path when losing (as its shields pop,
at the top), no sight of players far above or below where it looks, and
high ground when it roams. halo.log's minute line is followed by the
fight's counts.

The simulation: a bot decides from the game state at the tick and its own
random numbers (seeded per bot and game), and its path searches run in the
tick, so the bots do the same in two runs of the same input. Nothing of a
bot is in the game state: multiplayer games are never saved. Bots are in
local games only, which no other machine sees: no message of the network
protocol changes, and bots_update_actions does nothing unless a local game
runs. halo.log has the map's preparation and each minute of play's bots
line: the searches and how many failed, the times a bot stood stuck, shots
and each bot's kills and deaths; debug.bot_trace logs each bot's state.
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/bipeds.h"
#include "units/unit_definitions.h"
#include "items/items.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "networking/network_game_globals.h"
#include "ai/path.h"
#include "devices/devices.h"
#include "devices/device_definitions.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"

#include "bots.h"
#include "bots_combat.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* port/linux/src/port_config.c */
const char *config_string(char const *name);
long config_integer(char const *name);
void platform_log(char const *format, ...);

/* ---------- constants */

#define BOTS_PI 3.14159265f
#define BOTS_DEGREES (BOTS_PI / 180.0f)
/* the most places a bot roams to (player starting locations, item spawns) */
#define BOTS_MAXIMUM_ROAM_POINTS 96
/* the path searches the bots make in one tick (each up to the engine's
1024 surfaces); a bot whose search waits asks again next tick */
#define BOTS_SEARCHES_A_TICK 3
/* the bots' size as the path searches know it (a biped's radius) */
#define BOTS_PATH_RADIUS 0.25f
/* how near a path's step a bot is when it has reached it (across) */
#define BOTS_STEP_REACHED 0.35f
/* a pathfinding surface's flag: walkable (path.c's) */
#define BOTS_PATHFINDING_WALKABLE_BIT 6
/* the items a bot remembers it found no way to, at once */
#define BOTS_UNREACHABLE_ITEMS 8

/* where a bot is going */
enum
{
	_bots_goal_none = 0,
	_bots_goal_enemy,
	_bots_goal_roam,
	/* a better weapon to pick up */
	_bots_goal_item,
};

/* the fight's counts in halo.log's minute line */
enum
{
	_bots_combat_count_grenades = 0,
	_bots_combat_count_blows,
	_bots_combat_count_blows_held,
	_bots_combat_count_head_aims,
	_bots_combat_count_retreats,
	_bots_combat_count_charges,
	_bots_combat_count_jumps,
	_bots_combat_count_halts,
	_bots_combat_count_paced,
	_bots_combat_count_rescopes,
	_bots_combat_count_prenades,
	_bots_combat_count_high_ground,
	_bots_combat_count_out_of_view,
	NUMBER_OF_BOTS_COMBAT_COUNTS,
};

/* ---------- structures */

struct bots_skill_definition
{
	char const *name;
	/* seconds from first seeing an enemy to firing */
	real reaction_seconds;
	/* how far it sees an enemy (world units), and how wide (degrees) */
	real sight_distance;
	real field_of_view_degrees;
	/* the aim error on taking a target (degrees), and the seconds it takes
	to settle to a third */
	real aim_error_degrees;
	real aim_settle_seconds;
	/* the most it turns a tick (degrees) */
	real turn_degrees_per_tick;
	/* fires while the aim is within this many times the target's size */
	real fire_cone;
	/* how much of a moving target's lead it takes (0 none, 1 all) */
	real lead;
	/* the chance a second of throwing a grenade at middle range (normal's;
	bots_combat_grenade's rules start from it) */
	real grenade_chance;
	/* the ticks of a burst, and of a pause between bursts */
	short burst_ticks;
	short pause_ticks;
};

/* the levels (bots_combat.h's order): easy, normal, heroic and legendary
are the bots' first levels as they were (their jumps in a fight are now
bots_combat_jump_delay's); recruit is below easy, and veteran halfway from
heroic to legendary in every number */
static struct bots_skill_definition const bots_skills[NUMBER_OF_BOTS_SKILLS] =
{
	{ "recruit", 1.4f, 16.0f, 80.0f, 22.0f, 2.5f, 5.0f, 2.6f, 0.0f, 0.0f, 4, 24 },
	{ "easy", 0.75f, 22.0f, 100.0f, 12.0f, 1.4f, 8.0f, 2.2f, 0.3f, 0.04f, 6, 14 },
	{ "normal", 0.45f, 32.0f, 120.0f, 7.0f, 0.9f, 13.0f, 1.8f, 0.7f, 0.10f, 9, 9 },
	{ "heroic", 0.30f, 42.0f, 140.0f, 4.0f, 0.6f, 20.0f, 1.5f, 0.8f, 0.16f, 12, 6 },
	{ "veteran", 0.24f, 48.0f, 150.0f, 3.1f, 0.48f, 25.0f, 1.4f, 0.9f, 0.19f, 14, 5 },
	{ "legendary", 0.18f, 55.0f, 160.0f, 2.2f, 0.35f, 30.0f, 1.25f, 1.0f, 0.22f, 16, 4 },
};

/* the ticks of its target's places a bot keeps (the aim that trails it) */
#define BOTS_TRACK_TICKS 16

/* a bot's fight by level, started over at each spawn */
struct bots_bot_combat
{
	struct bots_combat_retreat retreat;
	struct bots_combat_crouch crouch;
	/* the end of the strafe it last saw (a new strafe: a halt, maybe), and
	the halt's end */
	long last_strafe_until_tick;
	long halt_until_tick;
	/* the fight's last tick (a new fight after 2 s without one), and its
	next jump */
	long last_fight_tick;
	long next_jump_tick;
	/* the tick it strikes, waiting within reach (NONE: not waiting) */
	long melee_ready_tick;
	/* it runs in to finish its target */
	boolean charging;
	/* it aims at its target's head */
	boolean head_aim;
	/* a grenade in its hand until then: the aim leads the target's path by
	this much */
	long throw_until_tick;
	real throw_lead;
	/* a hit knocked its zoom out: no zoom (and past 10 units no shot) until
	then */
	long rescope_until_tick;
	/* the tick it began holding a shot for its weapon's error to settle
	(NONE: not holding) */
	long pace_since_tick;
	/* the throw at the spot its target was last seen: the sighting it was
	weighed for, and the aim held on the spot until then */
	long prenade_seen_tick;
	long prenade_until_tick;
	real_point3d prenade_point;
	/* its target's places, a tick each */
	struct
	{
		long tick;
		long player_index;
		real_point3d point;
	} track[BOTS_TRACK_TICKS];
};

struct bots_bot
{
	long player_index;
	long unit_index;
	unsigned long random;

	/* where it looks (radians) */
	real yaw;
	real pitch;

	/* its target */
	long target_player_index;
	long target_seen_tick;
	long target_first_seen_tick;
	real_point3d target_last_position;
	long target_last_surface_index;
	real aim_error_yaw;
	real aim_error_pitch;
	long next_look_tick;
	/* the last time it was hurt, and its vitality then */
	real last_vitality;
	long hurt_tick;

	/* where it is going */
	short goal_kind;
	real_point3d goal_point;
	long goal_surface_index;
	long goal_tick;
	short roam_index;
	/* the roaming place is towards an enemy (looked at again sooner) */
	boolean hunting;
	/* the item it fetches (_bots_goal_item), and when it last pressed the
	action button for it; when it first pressed it for this item */
	long item_index;
	long item_press_tick;
	long item_first_press_tick;
	/* the path there (steps of the engine's path), and when it was found */
	struct path_result path;
	long path_tick;
	long path_failures;
	/* off a surface closed to the searches (beside a door): away from what
	closed it, until then */
	long escape_until_tick;
	real escape_direction;
	/* its path ends short of the goal (the search found no way there: a
	platform the walkable surfaces do not reach); at its end, straight on
	with jumps until then */
	boolean path_short;
	long climb_until_tick;
	long climb_start_tick;
	real climb_best_distance;
	/* where it was last tick (a teleporter's jump) */
	real_point3d last_position;
	/* an item it found no way to (on a ledge), not fetched again until then */
	long unreachable_item_indices[BOTS_UNREACHABLE_ITEMS];
	long unreachable_item_until_ticks[BOTS_UNREACHABLE_ITEMS];
	long unreachable_item_next;
	/* whether it moves, checked once a second */
	real_point3d stuck_position;
	long stuck_tick;
	short stuck_count;
	long unstick_until_tick;
	real unstick_direction;

	/* fighting: the side it strafes to and until when; bursts */
	real strafe_sign;
	long strafe_until_tick;
	long burst_until_tick;
	long pause_until_tick;
	long grenade_tick;
	long melee_tick;
	long item_check_tick;

	/* the buttons held last tick (a press is a tick held, then released) */
	unsigned long previous_control_flags;

	struct bots_bot_combat combat;

	/* (debug.bot_trace) its path searches */
	long searches;
};

/* ---------- globals */

static struct
{
	boolean map_ready;
	long roam_point_count;
	real_point3d roam_points[BOTS_MAXIMUM_ROAM_POINTS];
	long roam_surfaces[BOTS_MAXIMUM_ROAM_POINTS];
	struct bots_bot bots[BOTS_MAXIMUM];
	/* the skill setting, read once a tick for every bot */
	short skill;
	/* the fight's counts for halo.log */
	long combat_counts[NUMBER_OF_BOTS_COMBAT_COUNTS];
	/* the path search's state (80 KB: the engine's 1024 nodes, heap and
	table), allocated once a game has bots; the searches made this tick */
	struct path_state *search;
	long tick_searches;

	/* the statistics of halo.log's line */
	long ticks;
	long searches;
	long search_failures;
	/* searches that stopped short of the goal (a path to as near as they
	came), and those that ran out of nodes */
	long search_partials;
	long search_overflows;
	/* paths taken without the obstacle avoidance, which found no way */
	long search_unavoided;
	/* the paths whose smoothing went nowhere, taken unsmoothed */
	long search_unsmoothed;
	/* the times a bot stood stuck while it walked, and the goals given up
	after three */
	long stuck_events;
	long stuck_giveups;
	long last_report_tick;
} bots_globals;

/* the walkable surfaces the bots' searches read: a copy of the structure's
pathfinding surfaces, less those learnt to be in the way (scenery and
machines) and those with no room above them */
static struct
{
	struct structure_bsp const *source;
	struct structure_bsp structure;
	byte *walkable;
	/* the objects whose surfaces are closed */
	long learned[32];
	long learned_count;
	long closed_count;
	/* the surfaces looked at for room above them, and those closed */
	long scan_index;
	boolean scan_done;
	long roofed_count;
	long scan_ticks;
} bots_paths;

/* ---------- settings */

long bots_wanted_count(void)
{
	long count = config_integer("bots.count");

	return count < 0 ? 0 : count > BOTS_MAXIMUM ? BOTS_MAXIMUM : count;
}

short bots_skill(void)
{
	char const *name = config_string("bots.skill");
	short skill;

	for (skill = 0; skill < NUMBER_OF_BOTS_SKILLS; skill++)
	{
		if (!strcmp(name, bots_skills[skill].name))
			return skill;
	}
	return _bots_skill_normal;
}

short bots_teams(void)
{
	return strcmp(config_string("bots.teams"), "against") ? _bots_teams_even : _bots_teams_against;
}

int bots_machine_is_bot(long machine_index)
{
	return machine_index >= BOTS_FIRST_MACHINE && machine_index < BOTS_FIRST_MACHINE + BOTS_MAXIMUM;
}

void bots_network_player(long bot_index, struct network_player *player)
{
	/* (the Marines of Halo's campaign; eleven characters at most) */
	static char const *const names[BOTS_MAXIMUM] =
	{
		"Johnson", "Jenkins", "Mendoza", "Bisenti", "Stacker", "Polaski", "Chips", "Kojo",
		"Rawley", "Riley", "Wellsley", "Hall", "Fitzgerald", "Locklear", "Mitchell",
	};
	char const *name = names[bot_index % BOTS_MAXIMUM];
	long index;

	csmemset(player, 0, sizeof(*player));
	for (index = 0; index < (long)NUMBEROF(player->name) - 1 && name[index]; index++)
		player->name[index] = (wchar_t)name[index];
	player->name[index] = 0;
	player->primary_color_index = NONE;
	player->icon_index = 0;
	player->machine_index = (char)(BOTS_FIRST_MACHINE + bot_index);
	player->controller_index = 0;
	player->team_index = 0;
	player->player_list_index = NONE;
}

/* ---------- small helpers */

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

/* 0 to 1 */
static real bots_random_real(struct bots_bot *bot)
{
	return (real)(bots_random(bot) & 0xFFFFFF) / (real)0x1000000;
}

/* -1 to 1 */
static real bots_random_signed(struct bots_bot *bot)
{
	return bots_random_real(bot) * 2.0f - 1.0f;
}

static real bots_angle_difference(real a, real b)
{
	real difference = (real)fmod(a - b, 2.0f * BOTS_PI);

	if (difference > BOTS_PI)
		difference -= 2.0f * BOTS_PI;
	else if (difference < -BOTS_PI)
		difference += 2.0f * BOTS_PI;
	return difference;
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

/* the surface of the level under a point (a step up included), and the
ground point there; NONE over nothing */
static long bots_surface_below(real_point3d const *point, real_point3d *ground)
{
	struct collision_result collision;
	real_point3d start = *point;
	real_vector3d down = { 0.0f, 0.0f, -6.0f };

	start.z += 0.4f;
	if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
		&start, &down, NONE, &collision) && collision.type == _collision_result_structure)
	{
		if (ground)
			*ground = collision.point;
		return collision.surface_index;
	}
	return NONE;
}

/* the surface a unit stands on (a biped's support surface), and its point */
static long bots_unit_surface(long unit_index, real_point3d *point)
{
	struct object_datum *object = object_get(unit_index);
	struct biped_datum *biped = biped_try_and_get(unit_index);

	*point = object->object.position;
	if (object->object.parent_object_index != NONE)
		return NONE;
	if (biped && biped->biped.support_surface_index != NONE)
		return biped->biped.support_surface_index;
	return bots_surface_below(&object->object.position, point);
}

/* the unit's current weapon, NONE without one */
static long bots_current_weapon(struct unit_datum *unit)
{
	if (unit->unit.current_weapon_index < 0 || unit->unit.current_weapon_index >= MAXIMUM_WEAPONS_PER_UNIT)
		return NONE;
	return unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
}

/* whether a weapon has no rounds left (one with magazines) */
static boolean bots_weapon_empty(long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *definition = weapon_definition_get(weapon->definition_index);

	if (definition->weapon.magazines.count <= 0)
		return FALSE;
	return weapon->weapon.magazines[0].rounds_loaded + weapon->weapon.magazines[0].rounds_total <= 0;
}

/* how full the weapon's magazine is, 0 to 1 (1 for one without) */
static real bots_weapon_loaded(long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	struct weapon_definition *definition = weapon_definition_get(weapon->definition_index);
	struct weapon_magazine_definition *magazine;

	if (definition->weapon.magazines.count <= 0)
		return 1.0f;
	magazine = TAG_BLOCK_GET_ELEMENT(&definition->weapon.magazines, 0, struct weapon_magazine_definition);
	if (magazine->rounds_loaded_maximum <= 0)
		return 1.0f;
	return (real)weapon->weapon.magazines[0].rounds_loaded / (real)magazine->rounds_loaded_maximum;
}

/* the speed of the weapon's projectile (world units a tick; 0 for one
that hits at once or none) */
static real bots_projectile_speed_compute(long definition_index)
{
	struct weapon_definition *definition = weapon_definition_get(definition_index);

	if (definition->weapon.triggers.count > 0)
	{
		struct weapon_trigger_definition *trigger =
			TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, 0, struct weapon_trigger_definition);

		if (trigger->projectile.index != NONE)
		{
			struct projectile_definition *projectile = projectile_definition_get(trigger->projectile.index);

			return 0.5f * (projectile->projectile.initial_velocity + projectile->projectile.final_velocity);
		}
	}
	return 0.0f;
}

/* how much a bot wants a weapon (the stock weapons, by their tags' names),
-1 for the game's own (a flag, a ball) */
static short bots_weapon_value_compute(long definition_index)
{
	static struct
	{
		char const *name;
		short value;
	} const values[] =
	{
		{ "rocket", 9 }, { "sniper", 8 }, { "shotgun", 8 }, { "fuel rod", 7 }, { "plasma_cannon", 7 },
		{ "plasma pistol", 3 }, { "pistol", 7 }, { "assault rifle", 6 }, { "needler", 5 }, { "plasma rifle", 5 },
		{ "flamethrower", 4 }, { "flag", -1 }, { "ball", -1 }, { "skull", -1 },
	};
	char const *name = tag_get_name(definition_index);
	long index;

	if (!name)
		return 4;
	for (index = 0; index < (long)NUMBEROF(values); index++)
	{
		if (strstr(name, values[index].name))
			return values[index].value;
	}
	return 4;
}

/* how a bot fires the weapon: the ticks between presses of a weapon fired a
press at a time (a latched trigger: the pistols, the sniper rifle, the
rocket launcher; or one that charges while held: the plasma pistol), 0 for
one held down in bursts */
static short bots_weapon_tap_ticks_compute(long definition_index)
{
	struct weapon_definition *definition = weapon_definition_get(definition_index);
	struct weapon_trigger_definition *trigger;
	real rate;

	if (definition->weapon.triggers.count <= 0)
		return 0;
	trigger = TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, 0, struct weapon_trigger_definition);
	if (!TEST_FLAG(trigger->flags, _weapon_trigger_latched_bit) && trigger->charging_time <= 0.0f)
		return 0;
	rate = trigger->initial_rate_of_fire > 0.0f ? trigger->initial_rate_of_fire : 3.0f;
	return (short)PIN((long)((real)TICKS_PER_SECOND / rate + 0.5f), 2, TICKS_PER_SECOND * 2);
}

/* the distance a bot keeps from its enemy with the weapon */
static real bots_weapon_range_compute(long definition_index)
{
	char const *name = tag_get_name(definition_index);

	if (!name)
		return 6.0f;
	if (strstr(name, "shotgun") || strstr(name, "flamethrower"))
		return 2.0f;
	if (strstr(name, "sniper"))
		return 18.0f;
	if (strstr(name, "rocket") || strstr(name, "fuel rod") || strstr(name, "plasma_cannon"))
		return 12.0f;
	if (strstr(name, "plasma pistol") || strstr(name, "needler"))
		return 5.0f;
	if (strstr(name, "pistol"))
		return 9.0f;
	return 7.0f;
}

/* how a weapon definition aims (the fight by level): whether it kills with
a shot to the head (the pistol, the sniper rifle), whether its shots are
true only zoomed (the sniper rifle: its trigger's error when unzoomed), and
whether its error grows as it fires (the pistol) */
static void bots_weapon_aim_compute(long definition_index, boolean *precision, boolean *sniper, boolean *blooms)
{
	struct weapon_definition *definition = weapon_definition_get(definition_index);
	char const *name = tag_get_name(definition_index);

	*precision = name && ((strstr(name, "pistol") && !strstr(name, "plasma")) || strstr(name, "sniper"));
	*sniper = FALSE;
	*blooms = FALSE;
	if (definition->weapon.triggers.count > 0)
	{
		struct weapon_trigger_definition *trigger =
			TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, 0, struct weapon_trigger_definition);

		*sniper = TEST_FLAG(trigger->flags, _weapon_trigger_use_error_when_unzoomed_bit) &&
			definition->weapon.zoom_level_count > 0;
		*blooms = trigger->projectile_error_angle_upper_bound > trigger->projectile_error_angle_lower_bound + 0.001f;
	}
}

/* what the bots know of a weapon definition (its value, the distance kept
with it, how it fires, its projectile's speed, how it aims), made once a map: the
functions above read only its tag, its name and its trigger, which a map
never changes (a bot asked them of its weapon every tick: a dozen name
searches and a few tag lookups) */
#define BOTS_WEAPON_KINDS 32
struct bots_weapon_kind
{
	long definition_index;
	short value;
	short tap_ticks;
	real range;
	real projectile_speed;
	boolean precision;
	boolean sniper;
	boolean blooms;
};

static struct
{
	long count;
	struct bots_weapon_kind kinds[BOTS_WEAPON_KINDS];
	/* (the one made when the table is full) */
	struct bots_weapon_kind spare;
} bots_weapon_kinds;

static struct bots_weapon_kind const *bots_weapon_kind(long definition_index)
{
	struct bots_weapon_kind *kind;
	long index;

	for (index = 0; index < bots_weapon_kinds.count; index++)
	{
		if (bots_weapon_kinds.kinds[index].definition_index == definition_index)
			return &bots_weapon_kinds.kinds[index];
	}
	kind = bots_weapon_kinds.count < BOTS_WEAPON_KINDS ? &bots_weapon_kinds.kinds[bots_weapon_kinds.count++] :
		&bots_weapon_kinds.spare;
	kind->definition_index = definition_index;
	kind->value = bots_weapon_value_compute(definition_index);
	kind->tap_ticks = bots_weapon_tap_ticks_compute(definition_index);
	kind->range = bots_weapon_range_compute(definition_index);
	kind->projectile_speed = bots_projectile_speed_compute(definition_index);
	bots_weapon_aim_compute(definition_index, &kind->precision, &kind->sniper, &kind->blooms);
	return kind;
}

static short bots_weapon_value(long definition_index)
{
	return bots_weapon_kind(definition_index)->value;
}

static real bots_projectile_speed(long weapon_index)
{
	return bots_weapon_kind(weapon_get(weapon_index)->definition_index)->projectile_speed;
}

static short bots_weapon_tap_ticks(long weapon_index)
{
	return bots_weapon_kind(weapon_get(weapon_index)->definition_index)->tap_ticks;
}

static real bots_weapon_range(long weapon_index)
{
	return weapon_index != NONE ? bots_weapon_kind(weapon_get(weapon_index)->definition_index)->range : 6.0f;
}

/* whether the bot found no way to the item a while ago (left be thirty
seconds), and the item so remembered (the oldest of those kept forgotten) */
static boolean bots_item_unreachable(struct bots_bot const *bot, long item_index)
{
	long index;

	for (index = 0; index < BOTS_UNREACHABLE_ITEMS; index++)
	{
		if (bot->unreachable_item_indices[index] == item_index && game_time_get() < bot->unreachable_item_until_ticks[index])
			return TRUE;
	}
	return FALSE;
}

static void bots_item_set_unreachable(struct bots_bot *bot, long item_index)
{
	long slot;

	/* (one remembered already: kept longer) */
	for (slot = 0; slot < BOTS_UNREACHABLE_ITEMS; slot++)
	{
		if (bot->unreachable_item_indices[slot] == item_index && game_time_get() < bot->unreachable_item_until_ticks[slot])
		{
			bot->unreachable_item_until_ticks[slot] = game_time_get() + TICKS_PER_SECOND * 30;
			return;
		}
	}
	slot = bot->unreachable_item_next;
	bot->unreachable_item_indices[slot] = item_index;
	bot->unreachable_item_until_ticks[slot] = game_time_get() + TICKS_PER_SECOND * 30;
	bot->unreachable_item_next = (slot + 1) % BOTS_UNREACHABLE_ITEMS;
}

/* a weapon lying near the bot worth fetching: one better than the one it
holds; NONE for none */
static long bots_find_item(struct bots_bot *bot, struct unit_datum *unit, real_point3d const *position,
	real maximum_distance)
{
	struct object_iterator iterator;
	long best_index = NONE;
	real best_score = REAL_MAX;
	long current = bots_current_weapon(unit);
	short current_value = current != NONE ? bots_weapon_value(weapon_get(current)->definition_index) : 0;

	object_iterator_new(&iterator, _object_mask_weapon, 0);
	while (object_iterator_next(&iterator))
	{
		struct item_datum *item = item_get(iterator.index);
		real distance, score;
		short value;
		short slot;
		boolean held = FALSE;

		if (item->object.parent_object_index != NONE ||
			bots_item_unreachable(bot, iterator.index) ||
			!TEST_FLAG(item->object.flags, _object_connected_to_map_bit) ||
			TEST_FLAG(item->item.flags, _item_attached_to_unit_bit))
		{
			continue;
		}
		distance = bots_distance3d(&item->object.position, position);
		if (distance > maximum_distance)
			continue;
		value = bots_weapon_value(item->definition_index);
		/* (one it holds already: only its ammunition, which walking over
		takes) */
		for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		{
			long other = unit->unit.weapon_object_indices[slot];

			if (other != NONE && weapon_get(other)->definition_index == item->definition_index)
				held = TRUE;
		}
		if (held || value <= current_value)
			continue;
		score = distance - (real)(value - current_value) * 3.0f;
		if (score < best_score)
		{
			best_score = score;
			best_index = iterator.index;
		}
	}
	return best_index;
}

/* ---------- the map */

static void bots_add_roam_point(real_point3d const *point)
{
	real_point3d ground;
	long surface_index;

	if (bots_globals.roam_point_count >= BOTS_MAXIMUM_ROAM_POINTS)
		return;
	surface_index = bots_surface_below(point, &ground);
	if (surface_index == NONE)
		return;
	bots_globals.roam_points[bots_globals.roam_point_count] = ground;
	bots_globals.roam_surfaces[bots_globals.roam_point_count] = surface_index;
	bots_globals.roam_point_count++;
}

/* the bots' walkable surfaces let go */
static void bots_paths_release(void)
{
	if (bots_paths.walkable)
		free(bots_paths.walkable);
	csmemset(&bots_paths, 0, sizeof(bots_paths));
}

/* the surface's corners (at most BOTS_SURFACE_CORNERS), from its ring of
edges; 0 for one not the bsp's */
#define BOTS_SURFACE_CORNERS 16
static long bots_surface_corners(struct collision_bsp const *bsp, long surface_index, real_point3d *corners)
{
	struct collision_surface const *surface;
	long edge_index, count = 0;

	if (surface_index < 0 || surface_index >= bsp->surfaces.count)
		return 0;
	surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
	edge_index = surface->first_edge_index;
	do
	{
		struct collision_edge const *edge;
		boolean right;

		if (edge_index < 0 || edge_index >= bsp->edges.count)
			return 0;
		edge = TAG_BLOCK_GET_ELEMENT(&bsp->edges, edge_index, struct collision_edge);
		right = edge->surface_indices[1] == surface_index;
		if (!VALID_INDEX(edge->vertex_indices[right], bsp->vertices.count))
			return 0;
		corners[count++] = TAG_BLOCK_GET_ELEMENT(&bsp->vertices, edge->vertex_indices[right], struct collision_vertex)->point;
		edge_index = edge->edge_indices[right];
	} while (edge_index != surface->first_edge_index && count < BOTS_SURFACE_CORNERS);
	return count;
}

/* whether the object stands across the surface: a line over it at a biped's
knees or chest, from its middle to a little past an edge's, meets it, or
its middle is under it */
static boolean bots_surface_blocked(long object_index, real_point3d const *corners, long corner_count,
	real_point3d const *middle)
{
	static real const heights[] = { 0.35f, 1.0f };
	unsigned long flags = FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit) |
		FLAG(_collision_test_objects_bit) | FLAG(_collision_test_objects_scenery_bit) |
		FLAG(_collision_test_objects_machines_bit);
	struct collision_result collision;
	real_point3d start = *middle;
	real_vector3d up = { 0.0f, 0.0f, 1.6f };
	long height, corner;

	start.z += 0.05f;
	if (collision_test_vector(flags, &start, &up, NONE, &collision) && collision.type == _collision_result_object &&
		collision.object_index == object_index)
	{
		return TRUE;
	}
	for (height = 0; height < (long)NUMBEROF(heights); height++)
	{
		start.z = middle->z + heights[height];
		for (corner = 0; corner < corner_count; corner++)
		{
			real_point3d const *a = &corners[corner], *b = &corners[(corner + 1) % corner_count];
			real_vector3d vector;
			real length, past;

			vector.i = 0.5f * (a->x + b->x) - start.x;
			vector.j = 0.5f * (a->y + b->y) - start.y;
			vector.k = 0.5f * (a->z + b->z) + heights[height] - start.z;
			/* (on a little past the edge: a door may stand on it) */
			length = (real)sqrt(vector.i * vector.i + vector.j * vector.j + vector.k * vector.k);
			if (length < 0.01f)
				continue;
			past = (length + 0.4f) / length;
			vector.i *= past;
			vector.j *= past;
			vector.k *= past;
			if (collision_test_vector(flags, &start, &vector, NONE, &collision) &&
				collision.type == _collision_result_object && collision.object_index == object_index)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* the bots' copy of the structure's walkable surfaces, made (the searches
take it from then on); FALSE without one */
static boolean bots_paths_copy(void)
{
	struct structure_bsp *structure = global_structure_bsp_get();
	long count;

	if (!structure || structure->pathfinding_surfaces.count <= 0 || structure->collision_bsp.count <= 0 ||
		(bots_paths.source && bots_paths.source != structure))
	{
		return FALSE;
	}
	if (bots_paths.walkable)
		return TRUE;
	count = structure->pathfinding_surfaces.count;
	bots_paths.walkable = (byte *)malloc((size_t)count);
	if (!bots_paths.walkable)
		return FALSE;
	csmemcpy(bots_paths.walkable, structure->pathfinding_surfaces.address, (size_t)count);
	bots_paths.structure = *structure;
	bots_paths.structure.pathfinding_surfaces.address = bots_paths.walkable;
	bots_paths.source = structure;
	return TRUE;
}

/* a walkable surface of the bots' copy: its corners, its middle and how far
its corners are from it; FALSE for one not walkable (or not the bsp's) */
static boolean bots_paths_walkable_surface(struct collision_bsp const *bsp, long surface_index,
	real_point3d *corners, long *corner_count, real_point3d *middle, real *reach)
{
	long corner;

	if (surface_index >= bots_paths.structure.pathfinding_surfaces.count ||
		!TEST_FLAG(bots_paths.walkable[surface_index], BOTS_PATHFINDING_WALKABLE_BIT))
	{
		return FALSE;
	}
	*corner_count = bots_surface_corners(bsp, surface_index, corners);
	if (*corner_count < 3)
		return FALSE;
	middle->x = middle->y = middle->z = 0.0f;
	for (corner = 0; corner < *corner_count; corner++)
	{
		middle->x += corners[corner].x / (real)*corner_count;
		middle->y += corners[corner].y / (real)*corner_count;
		middle->z += corners[corner].z / (real)*corner_count;
	}
	*reach = 0.0f;
	for (corner = 0; corner < *corner_count; corner++)
		*reach = MAX(*reach, bots_distance3d(middle, &corners[corner]));
	return TRUE;
}

/* the walkable surfaces an object stands across closed to the bots' searches */
static void bots_paths_close_object(long object_index, char const *why)
{
	struct object_datum *object = object_get(object_index);
	struct collision_bsp const *bsp;
	long surface_index, closed = 0;

	if (bots_paths.learned_count >= (long)NUMBEROF(bots_paths.learned) || !bots_paths_copy())
		return;
	bots_paths.learned[bots_paths.learned_count++] = object_index;
	bsp = TAG_BLOCK_GET_ELEMENT(&bots_paths.structure.collision_bsp, 0, struct collision_bsp);
	for (surface_index = 0; surface_index < bsp->surfaces.count; surface_index++)
	{
		real_point3d corners[BOTS_SURFACE_CORNERS], middle;
		real reach;
		long corner_count;

		if (bots_paths_walkable_surface(bsp, surface_index, corners, &corner_count, &middle, &reach) &&
			bots_distance3d(&middle, &object->object.bounding_sphere_center) <
				object->object.bounding_sphere_radius + reach + 0.5f &&
			bots_surface_blocked(object_index, corners, corner_count, &middle))
		{
			bots_paths.walkable[surface_index] &= (byte)~FLAG(BOTS_PATHFINDING_WALKABLE_BIT);
			closed++;
		}
	}
	bots_paths.closed_count += closed;
	platform_log("bots: %s %s at (%.1f %.1f %.1f): %ld walkable surfaces closed", why,
		tag_get_name(object->definition_index), object->object.bounding_sphere_center.x,
		object->object.bounding_sphere_center.y, object->object.bounding_sphere_center.z, closed);
}

/* the walkable surfaces with no room above them closed, a few a tick from
the game's start: a floor that goes on under a wall (Longest's bases: the
floor's strips under the walls joined the rooms either side, and a team's
bots walked into the wall a whole game). A surface is closed whose middle,
or most of the points halfway from it to its corners, have the level less
than half a biped's height above them */
#define BOTS_PATHS_SCAN_A_TICK 32
static void bots_paths_scan(void)
{
	struct collision_bsp const *bsp;
	long scanned;

	if (bots_paths.scan_done || !bots_paths_copy())
		return;
	bots_paths.scan_ticks++;
	bsp = TAG_BLOCK_GET_ELEMENT(&bots_paths.structure.collision_bsp, 0, struct collision_bsp);
	for (scanned = 0; scanned < BOTS_PATHS_SCAN_A_TICK; scanned++)
	{
		long surface_index = bots_paths.scan_index++;
		real_point3d corners[BOTS_SURFACE_CORNERS], middle;
		real reach;
		long corner_count, sample, covered = 0;

		if (surface_index >= bsp->surfaces.count || surface_index >= bots_paths.structure.pathfinding_surfaces.count)
		{
			bots_paths.scan_done = TRUE;
			platform_log("bots: %ld walkable surfaces with no room above closed (%ld ticks, %d surfaces a tick)",
				bots_paths.roofed_count, bots_paths.scan_ticks, BOTS_PATHS_SCAN_A_TICK);
			return;
		}
		if (!bots_paths_walkable_surface(bsp, surface_index, corners, &corner_count, &middle, &reach))
			continue;
		/* (the middle first: covered, the surface is; else most of the
		points between it and the corners) */
		for (sample = -1; sample < corner_count; sample++)
		{
			unsigned long flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
				FLAG(_collision_test_back_facing_surfaces_bit);
			real_vector3d up = { 0.0f, 0.0f, 0.35f };
			struct collision_result collision;
			real_point3d point = middle;

			if (sample >= 0)
			{
				point.x = 0.5f * (middle.x + corners[sample].x);
				point.y = 0.5f * (middle.y + corners[sample].y);
				point.z = 0.5f * (middle.z + corners[sample].z);
			}
			point.z += 0.05f;
			if (collision_test_vector(flags, &point, &up, NONE, &collision))
			{
				if (sample < 0)
				{
					covered = corner_count;
					break;
				}
				covered++;
			}
		}
		if (covered * 2 > corner_count)
		{
			bots_paths.walkable[surface_index] &= (byte)~FLAG(BOTS_PATHFINDING_WALKABLE_BIT);
			bots_paths.roofed_count++;
		}
	}
}

static boolean bots_paths_learned(long object_index)
{
	long index;

	for (index = 0; index < bots_paths.learned_count; index++)
	{
		if (bots_paths.learned[index] == object_index)
			return TRUE;
	}
	return FALSE;
}

/* whether the point is on or in the object (by its bounding sphere) */
static boolean bots_object_holds(struct object_datum *object, real_point3d const *point)
{
	return bots_distance2d(&object->object.bounding_sphere_center, point) < object->object.bounding_sphere_radius + 0.4f &&
		fabs(object->object.bounding_sphere_center.z - point->z) < object->object.bounding_sphere_radius + 1.0f;
}

/* the scenery or machine (with a collision model) the point is on or in,
the nearest; NONE for none */
static long bots_object_at(real_point3d const *point)
{
	struct object_iterator iterator;
	long best_index = NONE;
	real best_distance = REAL_MAX;

	object_iterator_new(&iterator, _object_mask_scenery | _object_mask_machine, 0);
	while (object_iterator_next(&iterator))
	{
		struct object_datum *object = object_get(iterator.index);
		real distance;

		if (object_definition_get(object->definition_index)->object.collision_model.index == NONE ||
			!bots_object_holds(object, point))
		{
			continue;
		}
		distance = bots_distance3d(&object->object.bounding_sphere_center, point);
		if (distance < best_distance)
		{
			best_distance = distance;
			best_index = iterator.index;
		}
	}
	return best_index;
}

/* whether one of the map's game places (a flag's stand, a teleporter, a
hill's corner, a ball's spawn) is on or in the object: never in the way */
static boolean bots_object_marks_place(struct object_datum *object)
{
	struct scenario *scenario = global_scenario_get();
	long index;

	for (index = 0; scenario && index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *flag =
			TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index, struct scenario_netgame_flag);

		if (bots_object_holds(object, &flag->position))
			return TRUE;
	}
	return FALSE;
}

/* whether the point stands on the object (the level or another object
under it first) */
static boolean bots_object_supports(long object_index, real_point3d const *point)
{
	struct collision_result collision;
	real_point3d start = *point;
	real_vector3d down = { 0.0f, 0.0f, -1.2f };

	start.z += 0.4f;
	return collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_objects_bit) | FLAG(_collision_test_objects_scenery_bit) |
		FLAG(_collision_test_objects_machines_bit), &start, &down, NONE, &collision) &&
		collision.type == _collision_result_object && collision.object_index == object_index;
}

/* a path's obstacle avoidance found no way past where the bot is: the
scenery and machines about it (not the bipeds and vehicles, which move) are
in the way, their surfaces closed to the searches from now on (Sidewinder's
blast door between the bases is scenery: the search, over the structure
alone, goes through it, the shortest way, and the avoidance then finds no
way past; more than half of the searches across that map failed so). A
rock or a tree the avoidance goes around is never in the way. How many
were learnt */
static long bots_paths_learn(real_point3d const *position, real_point3d const *goal)
{
	struct object_iterator iterator;
	long learned = 0;

	object_iterator_new(&iterator, _object_mask_scenery | _object_mask_machine, 0);
	while (object_iterator_next(&iterator))
	{
		struct object_datum *object = object_get(iterator.index);
		struct object_definition *definition = object_definition_get(object->definition_index);

		/* (not what the bot or its goal stands on or in: a flag's stand, a
		teleporter's base; what the bot stands beside, within its bounding
		sphere but on the level, is learnt: a bot pressed against
		Sidewinder's blast door before it was learnt never learnt it) */
		if (definition->object.collision_model.index == NONE ||
			bots_distance2d(&object->object.bounding_sphere_center, position) > object->object.bounding_sphere_radius + 4.5f ||
			fabs(object->object.bounding_sphere_center.z - position->z) > object->object.bounding_sphere_radius + 2.0f ||
			(bots_object_holds(object, position) && bots_object_supports(iterator.index, position)) ||
			bots_object_holds(object, goal) ||
			bots_paths_learned(iterator.index) || bots_object_marks_place(object))
		{
			continue;
		}
		bots_paths_close_object(iterator.index, "in the way:");
		learned++;
	}
	return learned;
}

/* the way off a closed surface: away from the nearest of what closed it */
static boolean bots_paths_escape_direction(real_point3d const *position, real *direction)
{
	long index, nearest = NONE;
	real nearest_distance = REAL_MAX;

	for (index = 0; index < bots_paths.learned_count; index++)
	{
		struct object_datum *object = object_try_and_get(bots_paths.learned[index]);
		real distance;

		if (!object)
			continue;
		distance = bots_distance2d(&object->object.bounding_sphere_center, position);
		if (distance < nearest_distance && distance < object->object.bounding_sphere_radius + 4.0f)
		{
			nearest_distance = distance;
			nearest = index;
		}
	}
	if (nearest == NONE)
		return FALSE;
	{
		struct object_datum *object = object_get(bots_paths.learned[nearest]);

		*direction = (real)atan2(position->y - object->object.bounding_sphere_center.y,
			position->x - object->object.bounding_sphere_center.x);
	}
	return TRUE;
}

/* the bots' searches' walkable surfaces, the structure's to begin with,
less those of a machine its definition calls a pathfinding obstacle (not an
elevator, nor a door open now that is none open) */
static void bots_prepare_paths(void)
{
	struct object_iterator iterator;

	bots_paths_release();
	object_iterator_new(&iterator, _object_mask_machine, 0);
	while (object_iterator_next(&iterator))
	{
		struct object_datum *object = object_get(iterator.index);
		struct machine_definition *definition = machine_definition_get(object->definition_index);

		if (!TEST_FLAG(definition->machine.flags, _machine_is_pathfinding_obstacle_bit) ||
			TEST_FLAG(definition->machine.flags, _machine_is_elevator_bit) ||
			(device_get_position(iterator.index) > 0.5f &&
				TEST_FLAG(definition->machine.flags, _machine_is_not_pathfinding_obstacle_when_open_bit)))
		{
			continue;
		}
		bots_paths_close_object(iterator.index, "a machine in the way:");
	}
}

/* the places a bot roams to: the item spawns and the player starting
locations (once the level's BSP is in) */
static void bots_prepare_map(void)
{
	struct scenario *scenario = global_scenario_get();
	long index;

	bots_globals.map_ready = TRUE;
	bots_globals.roam_point_count = 0;
	bots_prepare_paths();
	if (!scenario)
		return;
	for (index = 0; index < scenario->netgame_equipment.count; index++)
	{
		struct scenario_netgame_equipment *equipment =
			TAG_BLOCK_GET_ELEMENT(&scenario->netgame_equipment, index, struct scenario_netgame_equipment);

		bots_add_roam_point(&equipment->position);
	}
	for (index = 0; index < scenario->players.count; index += 2)
	{
		struct player_starting_location *location =
			TAG_BLOCK_GET_ELEMENT(&scenario->players, index, struct player_starting_location);

		bots_add_roam_point(&location->position);
	}
	{
		real_point3d low = { REAL_MAX, REAL_MAX, REAL_MAX }, high = { -REAL_MAX, -REAL_MAX, -REAL_MAX };

		for (index = 0; index < bots_globals.roam_point_count; index++)
		{
			low.x = MIN(low.x, bots_globals.roam_points[index].x);
			low.y = MIN(low.y, bots_globals.roam_points[index].y);
			high.x = MAX(high.x, bots_globals.roam_points[index].x);
			high.y = MAX(high.y, bots_globals.roam_points[index].y);
		}
		platform_log("bots: %ld places to roam on this map, over %.0f units", bots_globals.roam_point_count,
			bots_globals.roam_point_count > 0 ? (double)bots_distance2d(&low, &high) : 0.0);
	}
}

void bots_initialize_for_new_map(void)
{
	long index;

	csmemset(bots_globals.bots, 0, sizeof(bots_globals.bots));
	for (index = 0; index < BOTS_MAXIMUM; index++)
		bots_globals.bots[index].player_index = NONE;
	bots_globals.map_ready = FALSE;
	bots_globals.roam_point_count = 0;
	bots_globals.ticks = 0;
	bots_globals.searches = 0;
	bots_globals.search_failures = 0;
	bots_globals.search_partials = 0;
	bots_globals.search_overflows = 0;
	bots_globals.search_unavoided = 0;
	bots_globals.search_unsmoothed = 0;
	bots_globals.stuck_events = 0;
	bots_globals.stuck_giveups = 0;
	csmemset(bots_globals.combat_counts, 0, sizeof(bots_globals.combat_counts));
	/* (the first line a minute into the game) */
	bots_globals.last_report_tick = 0;
	/* (the weapon definitions are the new map's) */
	bots_weapon_kinds.count = 0;
}

void bots_dispose_from_old_map(void)
{
	bots_paths_release();
	bots_globals.map_ready = FALSE;
}

/* ---------- path searches */

/* the search's way to its destination (or as near as it came): each
surface's entry point (path_state_build_path's raw steps, at most 64), the
endpoint last. TRUE with steps */
static boolean bots_path_raw_steps(struct path_state *state, struct path_step *raw_steps, short *raw_step_count,
	boolean *steps_finish_path, real_point3d *endpoint)
{
	short node_index, child_index = NONE;

	*steps_finish_path = TRUE;
	*raw_step_count = 0;
	if (!state->destination_valid || state->node_count <= 0)
		return FALSE;
	node_index = path_node_from_hash_table(state, state->destination.surface_index);
	*endpoint = state->destination.point;
	if (node_index == NONE)
	{
		node_index = state->closest_node_index;
		*endpoint = state->closest_point;
	}
	if (node_index < 0 || node_index >= state->node_count)
		return FALSE;
	*raw_step_count = (short)MIN(state->node_list[node_index].depth + 1, 64);
	while (node_index != NONE)
	{
		struct path_node *node;

		if (node_index < 0 || node_index >= state->node_count)
			return FALSE;
		node = &state->node_list[node_index];
		if (node->depth >= 64)
			*steps_finish_path = FALSE;
		else if (node->depth >= 0 && node->depth < *raw_step_count)
		{
			raw_steps[node->depth].surface_index = node->surface_index;
			raw_steps[node->depth].point = child_index == NONE ? *endpoint : state->node_list[child_index].entry_point;
		}
		else
			return FALSE;
		child_index = node_index;
		node_index = node->parent_node_index;
	}
	return *raw_step_count > 0;
}

/* the path as path_state_build_path makes it, the steps smoothed, but not
taken round the objects about (the obstacle avoidance found no way round:
the other bots crowding a narrow way out of a base, Longest's, all waiting
on each other); the bots push past each other, and one that stops moving
sidesteps and jumps. TRUE with a path */
static boolean bots_build_path_unavoided(struct path_state *state, struct path_result *path)
{
	struct path_step raw_steps[64];
	struct path_step smoothed_steps[MAXIMUM_SMOOTHED_PATH_STEPS];
	short raw_step_count, smoothed_step_count = 0;
	boolean steps_finish_path;
	real_point3d endpoint;

	csmemset(path, 0, sizeof(*path));
	if (!bots_path_raw_steps(state, raw_steps, &raw_step_count, &steps_finish_path, &endpoint))
		return FALSE;
	path_smooth(state, raw_step_count, raw_steps, &smoothed_step_count, smoothed_steps, &steps_finish_path);
	if (smoothed_step_count <= 0)
		return FALSE;
	smoothed_step_count = MIN(smoothed_step_count, MAXIMUM_SMOOTHED_PATH_STEPS);
	path->valid = TRUE;
	path->endpoint.point = endpoint;
	path->endpoint.surface_index = smoothed_steps[smoothed_step_count - 1].surface_index;
	path->steps_finish_path = steps_finish_path;
	path->step_count = (char)smoothed_step_count;
	path->step_index = 0;
	csmemcpy(path->steps, smoothed_steps, smoothed_step_count * sizeof(struct path_step));
	return TRUE;
}

/* a path that goes nowhere: short of its end, every step where the bot
stands. The smoothing (path_smooth) gave one at the foot of a steep bank:
all four of its steps the same point a few centimetres from the bot, which
it took as reached at once and searched again, the same, while it slid
about (Sidewinder's banks up to the side ledges) */
static boolean bots_path_stalls(struct path_result const *path, real_point3d const *start)
{
	return path->valid && !path->steps_finish_path && path->step_count > 0 &&
		bots_distance2d(&path->steps[path->step_count - 1].point, start) <= BOTS_STEP_REACHED;
}

/* a stalling path's steps instead the search's own, unsmoothed: the next
surfaces' entry points beyond where the bot stands (up the bank), four at
most. TRUE when made */
static boolean bots_path_unsmoothed(struct path_state *state, real_point3d const *start, struct path_result *path)
{
	struct path_step raw_steps[64];
	short raw_step_count, step, count = 0;
	boolean steps_finish_path;
	real_point3d endpoint;
	struct path_result result;

	if (!bots_path_raw_steps(state, raw_steps, &raw_step_count, &steps_finish_path, &endpoint))
		return FALSE;
	csmemset(&result, 0, sizeof(result));
	for (step = 0; step < raw_step_count && count < (short)NUMBEROF(result.steps); step++)
	{
		if (!count && bots_distance2d(&raw_steps[step].point, start) <= BOTS_STEP_REACHED)
			continue;
		result.steps[count++] = raw_steps[step];
	}
	if (!count)
		return FALSE;
	result.valid = TRUE;
	result.steps_finish_path = steps_finish_path && step >= raw_step_count;
	result.step_count = (char)count;
	result.endpoint.point = endpoint;
	result.endpoint.surface_index = result.steps[count - 1].surface_index;
	*path = result;
	return TRUE;
}

/* a search's answer made a path: the way to the goal (or as near as the
search came); when the path's obstacle avoidance finds no way, again with
what the goal or the bot stands on (a flag's stand, a teleporter's base) no
obstacle, and failing that, the scenery and machines about the bot are
learnt to be in the way (the next searches go round them) and this once the
path is taken without the avoidance. TRUE with a path */
static boolean bots_search_build(struct path_state *state, real_point3d const *start, real_point3d const *goal,
	struct path_result *path)
{
	long object_index;

	if (state->node_count <= 0)
		return FALSE;
	if (path_state_build_path(state, path) && path->step_count > 0)
		return TRUE;
	object_index = bots_object_at(goal);
	if (object_index == NONE)
		object_index = bots_object_at(start);
	if (object_index != NONE)
	{
		state->input.ignore_target_object_index = object_index;
		csmemset(path, 0, sizeof(*path));
		if (path_state_build_path(state, path) && path->step_count > 0)
			return TRUE;
	}
	/* (something new learnt: a new search, round it) */
	if (bots_paths_learn(start, goal) > 0)
		return FALSE;
	if (!bots_build_path_unavoided(state, path))
		return FALSE;
	bots_globals.search_unavoided++;
	return TRUE;
}

/* a path from where the bot's unit is to its goal, searched now (a few a
tick: a bot past them asks again next tick), into the bot's path */
static void bots_search(struct bots_bot *bot, long unit_index)
{
	struct path_state *state;
	struct path_input input;
	struct path_result path;
	real_point3d start;
	long start_surface_index;
	boolean reached;

	if (bots_globals.tick_searches >= BOTS_SEARCHES_A_TICK || bot->goal_surface_index == NONE)
		return;
	if (!bots_globals.search)
	{
		bots_globals.search = (struct path_state *)malloc(sizeof(struct path_state));
		if (!bots_globals.search)
			return;
	}
	state = bots_globals.search;
	start_surface_index = bots_unit_surface(unit_index, &start);
	if (start_surface_index == NONE)
		return;
	/* (on a surface closed to the searches, whose way out may be closed
	too: away from what closed it first, a second) */
	if (bots_paths.walkable && start_surface_index < bots_paths.structure.pathfinding_surfaces.count &&
		!TEST_FLAG(bots_paths.walkable[start_surface_index], BOTS_PATHFINDING_WALKABLE_BIT) &&
		bots_paths_escape_direction(&start, &bot->escape_direction))
	{
		bot->escape_until_tick = game_time_get() + TICKS_PER_SECOND;
		bot->path.valid = FALSE;
		bot->path_tick = game_time_get();
		return;
	}
	bots_globals.tick_searches++;
	bots_globals.searches++;
	bot->searches++;
	path_input_new(&input, BOTS_PATH_RADIUS, FALSE, unit_index);
	path_input_set_start(&input, &start, start_surface_index);
	path_state_new(&input, state, NULL);
	/* (the walkable surfaces less those closed by what is in the way) */
	if (bots_paths_copy() && state->structure == bots_paths.source)
		state->structure = &bots_paths.structure;
	/* (a path to as near the goal as the search reaches: a long way across a
	large level is more surfaces than a search holds, and the bot searches
	again from there) */
	path_state_destination(state, &bot->goal_point, bot->goal_surface_index, 1000.0f);
	path_state_find(state);
	reached = state->node_count > 0 && path_node_from_hash_table(state, bot->goal_surface_index) != NONE;
	if (state->node_count >= PATH_NODE_LIST_SIZE)
		bots_globals.search_overflows++;
	if (!reached)
		bots_globals.search_partials++;
	csmemset(&path, 0, sizeof(path));
	if (bots_search_build(state, &start, &bot->goal_point, &path))
	{
		if (bots_path_stalls(&path, &start) && bots_path_unsmoothed(state, &start, &path))
			bots_globals.search_unsmoothed++;
		bot->path = path;
		bot->path.step_index = 0;
		bot->path_failures = 0;
		bot->path_short = !reached;
		if (!bot->path_short)
			bot->climb_until_tick = 0;
	}
	else
	{
		bot->path.valid = FALSE;
		bot->path_failures++;
		bots_globals.search_failures++;
	}
	/* (an item it finds no way to: another, and not this one a while) */
	if (bot->goal_kind == _bots_goal_item && !reached)
	{
		bots_item_set_unreachable(bot, bot->item_index);
		bot->goal_kind = _bots_goal_none;
		bot->path.valid = FALSE;
	}
	bot->path_tick = game_time_get();
}

/* ---------- perception */

/* whether the bot sees the point from its eyes (the AI's sight test) */
static boolean bots_line_of_sight(long unit_index, real_point3d const *eye, real_point3d const *point, long target_unit_index)
{
	struct collision_result collision;
	real_vector3d vector;

	vector.i = point->x - eye->x;
	vector.j = point->y - eye->y;
	vector.k = point->z - eye->z;
	if (!collision_test_vector(_collision_test_for_line_of_sight_flags, eye, &vector, unit_index, &collision))
		return TRUE;
	/* (the target's vehicle: seen) */
	if (collision.type == _collision_result_object && target_unit_index != NONE &&
		(collision.object_index == target_unit_index ||
			collision.object_index == object_get(target_unit_index)->object.parent_object_index))
	{
		return TRUE;
	}
	return FALSE;
}

static boolean bots_is_enemy(struct player_datum *player, struct player_datum *other)
{
	if (player == other || other->unit_index == NONE)
		return FALSE;
	if (game_engine_has_teams() && player->team_index == other->team_index)
		return FALSE;
	return TRUE;
}

/* the enemy the bot sees best now, NONE for none: of the enemies in its
sight's reach and view, the one of the least score (nearest, most ahead, the
one it fights favoured) a line of sight reaches, the first in the players'
order of those of that score. The enemies are scored first and their lines
of sight cast from the least score up, until one reaches its enemy, which is
the answer. A score that is not a number (none is: the players' positions
are), or more enemies than kept, take the players' order */
struct bots_target_candidate
{
	long player_index;
	long unit_index;
	real score;
	real_point3d point;
};

/* an enemy's score as a target (its point the bounding sphere's centre);
FALSE for one out of reach or view, or not an enemy */
static boolean bots_target_score(struct bots_bot *bot, struct player_datum *player, struct player_datum *other,
	long player_index, real_point3d const *eye, struct bots_skill_definition const *skill, boolean hurt,
	struct bots_target_candidate *candidate)
{
	struct object_datum *object;
	struct unit_datum *other_unit;
	real distance, reach, bearing;
	real half_view = 0.5f * skill->field_of_view_degrees * BOTS_DEGREES;

	if (!bots_is_enemy(player, other))
		return FALSE;
	object = object_get(other->unit_index);
	other_unit = unit_get(other->unit_index);
	if (TEST_FLAG(object->object.damage_flags, _object_dead_bit))
		return FALSE;
	candidate->point = object->object.bounding_sphere_center;
	distance = bots_distance3d(eye, &candidate->point);
	reach = skill->sight_distance;
	/* (active camouflage: seen only close) */
	if (TEST_FLAG(other_unit->unit.flags, _unit_active_camouflaged_bit))
		reach *= 0.3f;
	/* (the one it fights: a little further) */
	if (player_index == bot->target_player_index)
		reach *= 1.25f;
	if (distance > reach)
		return FALSE;
	bearing = (real)fabs(bots_angle_difference((real)atan2(candidate->point.y - eye->y, candidate->point.x - eye->x),
		bot->yaw));
	/* (out of view: unless close by, it shot the bot, or it is the one
	the bot fights) */
	if (bearing > half_view && distance > 3.0f && !hurt && player_index != bot->target_player_index)
		return FALSE;
	/* (nor far above or below where it looks, on the same terms: a player
	running by looks ahead, not up) */
	if (distance > 3.0f && !hurt && player_index != bot->target_player_index)
	{
		real dx = candidate->point.x - eye->x, dy = candidate->point.y - eye->y;
		real elevation = (real)atan2(candidate->point.z - eye->z, (real)sqrt(dx * dx + dy * dy));

		if (!bots_combat_in_vertical_view(bots_globals.skill, elevation / BOTS_DEGREES, bot->pitch / BOTS_DEGREES))
		{
			bots_globals.combat_counts[_bots_combat_count_out_of_view]++;
			return FALSE;
		}
	}
	candidate->player_index = player_index;
	candidate->unit_index = other->unit_index;
	candidate->score = distance + bearing * 4.0f - (player_index == bot->target_player_index ? 6.0f : 0.0f);
	return TRUE;
}

static long bots_find_target(struct bots_bot *bot, struct player_datum *player, real_point3d const *eye)
{
	struct bots_skill_definition const *skill = &bots_skills[bots_globals.skill];
	struct data_iterator iterator;
	struct player_datum *other;
	boolean hurt = game_time_get() - bot->hurt_tick < TICKS_PER_SECOND * 2;
	struct bots_target_candidate candidates[BOTS_MAXIMUM + MAXIMUM_LOCAL_PLAYERS];
	long candidate_count = 0, index;
	boolean ordered = TRUE;

	data_iterator_new(&iterator, player_data);
	while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL && ordered)
	{
		if (candidate_count >= (long)NUMBEROF(candidates))
			ordered = FALSE;
		else if (bots_target_score(bot, player, other, iterator.datum_index, eye, skill, hurt, &candidates[candidate_count]))
		{
			real score = candidates[candidate_count].score;

			if (score != score)
				ordered = FALSE;
			else if (score < REAL_MAX)
				candidate_count++;
		}
	}
	if (!ordered)
	{
		/* (the players' order, each scoring less than the best so far) */
		struct bots_target_candidate candidate;
		long best_index = NONE;
		real best_score = REAL_MAX;

		data_iterator_new(&iterator, player_data);
		while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		{
			if (!bots_target_score(bot, player, other, iterator.datum_index, eye, skill, hurt, &candidate) ||
				candidate.score >= best_score ||
				!bots_line_of_sight(player->unit_index, eye, &candidate.point, candidate.unit_index))
			{
				continue;
			}
			best_score = candidate.score;
			best_index = candidate.player_index;
		}
		return best_index;
	}
	while (candidate_count > 0)
	{
		long least = 0;

		/* (the least score, the first of it in the players' order) */
		for (index = 1; index < candidate_count; index++)
		{
			if (candidates[index].score < candidates[least].score)
				least = index;
		}
		if (bots_line_of_sight(player->unit_index, eye, &candidates[least].point, candidates[least].unit_index))
			return candidates[least].player_index;
		/* (out, the others kept in their order) */
		candidate_count--;
		for (index = least; index < candidate_count; index++)
			candidates[index] = candidates[index + 1];
	}
	return NONE;
}

/* ---------- moving */

static void bots_set_goal(struct bots_bot *bot, short kind, real_point3d const *point, long surface_index)
{
	/* (the same goal, near enough: the path stands) */
	if (bot->goal_kind == kind && bot->goal_surface_index == surface_index &&
		bots_distance3d(&bot->goal_point, point) < 1.5f)
	{
		return;
	}
	bot->goal_kind = kind;
	bot->goal_point = *point;
	bot->goal_surface_index = surface_index;
	bot->goal_tick = game_time_get();
	bot->path.valid = FALSE;
	bot->path_failures = 0;
	bot->climb_until_tick = 0;
}

/* a roaming place picked at random (index), or a higher one (VETERAN and
up): two more places at random, not within 6 units,
and the one of the most credit for its height above the bot */
static short bots_roam_higher(struct bots_bot *bot, real_point3d const *position, short index)
{
	short best = index, tries;
	real best_credit = bots_combat_high_ground(bots_globals.skill, bots_globals.roam_points[index].z - position->z);

	if (bots_globals.skill < _bots_skill_veteran)
		return index;
	for (tries = 0; tries < 2; tries++)
	{
		short other = (short)(bots_random(bot) % (unsigned long)bots_globals.roam_point_count);
		real credit;

		if (bots_distance2d(&bots_globals.roam_points[other], position) < 6.0f)
			continue;
		credit = bots_combat_high_ground(bots_globals.skill, bots_globals.roam_points[other].z - position->z);
		if (credit > best_credit)
		{
			best_credit = credit;
			best = other;
		}
	}
	if (best != index)
		bots_globals.combat_counts[_bots_combat_count_high_ground]++;
	return best;
}

/* the next place to roam to: most often towards an enemy (the roaming place
nearest a living enemy, as a player knows where the other team's base is
and reads the motion tracker), else anywhere */
static void bots_pick_roam_goal(struct bots_bot *bot, real_point3d const *position)
{
	long tries;

	if (bots_globals.roam_point_count <= 0)
		return;
	bot->hunting = FALSE;
	if (bots_random_real(bot) < bots_combat_hunt_chance(bots_globals.skill))
	{
		struct player_datum *player = player_get(bot->player_index);
		struct data_iterator iterator;
		struct player_datum *other;
		long enemies[BOTS_MAXIMUM + MAXIMUM_LOCAL_PLAYERS];
		long enemy_count = 0;

		data_iterator_new(&iterator, player_data);
		while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL &&
			enemy_count < (long)NUMBEROF(enemies))
		{
			if (bots_is_enemy(player, other))
				enemies[enemy_count++] = other->unit_index;
		}
		if (enemy_count > 0)
		{
			struct object_datum *enemy = object_get(enemies[bots_random(bot) % (unsigned long)enemy_count]);
			long index, nearest = NONE;
			real nearest_distance = REAL_MAX;

			for (index = 0; index < bots_globals.roam_point_count; index++)
			{
				real distance = bots_distance2d(&bots_globals.roam_points[index], &enemy->object.position) +
					bots_random_real(bot) * 6.0f;

				/* (a place above the enemy counts as nearer, from veteran) */
				distance -= bots_combat_high_ground(bots_globals.skill,
					bots_globals.roam_points[index].z - enemy->object.position.z);

				if (distance < nearest_distance && bots_distance2d(&bots_globals.roam_points[index], position) > 4.0f)
				{
					nearest_distance = distance;
					nearest = index;
				}
			}
			if (nearest != NONE)
			{
				bot->roam_index = (short)nearest;
				bot->hunting = TRUE;
				bots_set_goal(bot, _bots_goal_roam, &bots_globals.roam_points[nearest], bots_globals.roam_surfaces[nearest]);
				return;
			}
		}
	}
	for (tries = 0; tries < 6; tries++)
	{
		short index = (short)(bots_random(bot) % (unsigned long)bots_globals.roam_point_count);

		/* (not where it is) */
		if (bots_distance2d(&bots_globals.roam_points[index], position) < 6.0f && tries < 5)
			continue;
		index = bots_roam_higher(bot, position, index);
		bot->roam_index = index;
		bots_set_goal(bot, _bots_goal_roam, &bots_globals.roam_points[index], bots_globals.roam_surfaces[index]);
		return;
	}
}

/* where the bot goes: the enemy last seen, a better weapon near, or about
the map */
static void bots_choose_goal(struct bots_bot *bot, long player_index, real_point3d const *position, boolean fighting)
{
	long now = game_time_get();

	if (!fighting && bot->target_player_index != NONE && bot->target_last_surface_index != NONE &&
		now - bot->target_seen_tick < TICKS_PER_SECOND * 8)
	{
		bots_set_goal(bot, _bots_goal_enemy, &bot->target_last_position, bot->target_last_surface_index);
		return;
	}
	/* a better weapon near (looked for once a second; given up after a
	while) */
	if (bot->goal_kind == _bots_goal_item)
	{
		struct item_datum *item = (struct item_datum *)object_try_and_get_and_verify_type(bot->item_index,
			_object_mask_weapon);

		if (item && item->object.parent_object_index == NONE && now - bot->goal_tick < TICKS_PER_SECOND * 15 &&
			(bot->item_first_press_tick < bot->goal_tick || now - bot->item_first_press_tick < TICKS_PER_SECOND * 3))
		{
			return;
		}
		/* (one it did not get, standing on it or not: left be a while) */
		if (item && item->object.parent_object_index == NONE)
			bots_item_set_unreachable(bot, bot->item_index);
		bot->goal_kind = _bots_goal_none;
		bot->item_check_tick = now + TICKS_PER_SECOND * 2;
	}
	if (now >= bot->item_check_tick)
	{
		long item_index = bots_find_item(bot, unit_get(player_get(player_index)->unit_index), position, fighting ? 8.0f : 20.0f);

		bot->item_check_tick = now + TICKS_PER_SECOND;
		if (item_index != NONE)
		{
			real_point3d ground;
			long surface_index = bots_surface_below(&object_get(item_index)->object.position, &ground);

			if (surface_index != NONE)
			{
				bots_set_goal(bot, _bots_goal_item, &ground, surface_index);
				bot->item_index = item_index;
				return;
			}
		}
	}
	if (bot->goal_kind != _bots_goal_roam || bots_distance2d(&bot->goal_point, position) < 1.5f ||
		now - bot->goal_tick > TICKS_PER_SECOND * (bot->hunting ? 12 : 40))
	{
		bots_pick_roam_goal(bot, position);
	}
}

/* the direction the bot walks (radians), following its path; FALSE when
it has nowhere to go */
static boolean bots_follow_path(struct bots_bot *bot, long unit_index, real_point3d const *position, real *direction)
{
	long now = game_time_get();

	if (bot->goal_kind == _bots_goal_none)
		return FALSE;
	if (now < bot->escape_until_tick)
	{
		*direction = bot->escape_direction;
		return TRUE;
	}
	/* (at the end of a path short of its goal: straight on, jumping, while
	it comes nearer (the walkable surfaces miss ramps a biped walks up:
	Prisoner's, Wizard's); then somewhere else) */
	if (bot->climb_until_tick)
	{
		real distance = bots_distance3d(&bot->goal_point, position);

		if (distance < bot->climb_best_distance - 0.5f && now - bot->climb_start_tick < TICKS_PER_SECOND * 20)
		{
			bot->climb_best_distance = distance;
			bot->climb_until_tick = now + TICKS_PER_SECOND * 3;
		}
		if (now < bot->climb_until_tick && distance > 1.0f)
		{
			*direction = (real)atan2(bot->goal_point.y - position->y, bot->goal_point.x - position->x);
			return TRUE;
		}
		bot->climb_until_tick = 0;
		if (distance > 1.0f)
		{
			bot->goal_kind = _bots_goal_none;
			return FALSE;
		}
	}
	/* (a new path now and then: the objects about move, and the goal) */
	if ((!bot->path.valid || now - bot->path_tick > TICKS_PER_SECOND * 4) &&
		now - bot->path_tick > TICKS_PER_SECOND / 3)
	{
		bots_search(bot, unit_index);
	}
	if (bot->path.valid)
	{
		while (bot->path.step_index < bot->path.step_count)
		{
			struct path_step *step = &bot->path.steps[(long)bot->path.step_index];

			if (bots_distance2d(&step->point, position) > BOTS_STEP_REACHED)
				break;
			bot->path.step_index++;
		}
		if (bot->path.step_index >= bot->path.step_count)
		{
			bot->path.valid = FALSE;
			/* (as near as the search came, the goal on above or beyond:
			on towards it) */
			if (bot->path.steps_finish_path && bot->path_short && bots_distance3d(&bot->goal_point, position) > 1.5f &&
				!bot->climb_until_tick)
			{
				bot->climb_start_tick = now;
				bot->climb_until_tick = now + TICKS_PER_SECOND * 3;
				bot->climb_best_distance = bots_distance3d(&bot->goal_point, position);
				*direction = (real)atan2(bot->goal_point.y - position->y, bot->goal_point.x - position->x);
				return TRUE;
			}
			/* (the whole way walked: there) */
			if (bot->path.steps_finish_path)
				return FALSE;
			bots_search(bot, unit_index);
		}
		else
		{
			struct path_step *step = &bot->path.steps[(long)bot->path.step_index];

			*direction = (real)atan2(step->point.y - position->y, step->point.x - position->x);
			return TRUE;
		}
	}
	/* (no path yet, or none found: straight there) */
	if (bots_distance2d(&bot->goal_point, position) < 1.0f)
		return FALSE;
	*direction = (real)atan2(bot->goal_point.y - position->y, bot->goal_point.x - position->x);
	return TRUE;
}

/* ---------- a bot's tick */

static void bots_bot_reset(struct bots_bot *bot, long player_index, long bot_index)
{
	csmemset(bot, 0, sizeof(*bot));
	bot->player_index = player_index;
	bot->unit_index = NONE;
	bot->target_player_index = NONE;
	bot->target_last_surface_index = NONE;
	bot->goal_surface_index = NONE;
	bot->random = 0x2545F491UL * (unsigned long)(bot_index + 1) ^ (unsigned long)network_game_get_random_seed();
	bot->strafe_sign = 1.0f;
}

/* a bot's tick: what it knows of itself, and the controls it builds */
struct bots_tick
{
	struct bots_skill_definition const *skill;
	struct player_datum *player;
	struct unit_datum *unit;
	struct object_datum *object;
	long now;
	/* the unit's eyes (its head marker), found when first needed: the look
	(every third tick) and the fight (bots_tick_eye) */
	boolean eye_found;
	real_point3d eye;
	real_point3d position;
	long weapon_index;
	/* the controls: buttons, where it walks, how hard */
	unsigned long flags;
	boolean moving;
	real move_direction;
	real throttle_scale;
	/* it has a target in sight, this far away */
	boolean fighting;
	real target_distance;
	struct player_action *action;
};

/* the unit's eyes: its head marker, as unit_get_head_position finds it
(the marker's matrix from the object's node matrices, which nothing in the
bot's tick changes, so it is the same found now or at the tick's start) */
static real_point3d const *bots_tick_eye(struct bots_tick *tick)
{
	if (!tick->eye_found)
	{
		unit_get_head_position(tick->player->unit_index, &tick->eye);
		tick->eye_found = TRUE;
	}
	return &tick->eye;
}

/* ---------- fighting by level (bots_combat.c's rules) */

/* a bot's fight started over (a spawn) */
static void bots_level_reset(struct bots_bot_combat *combat)
{
	long index;

	csmemset(combat, 0, sizeof(*combat));
	bots_combat_retreat_reset(&combat->retreat);
	combat->last_fight_tick = -TICKS_PER_SECOND * 10;
	combat->melee_ready_tick = NONE;
	combat->pace_since_tick = NONE;
	combat->prenade_seen_tick = NONE;
	for (index = 0; index < BOTS_TRACK_TICKS; index++)
		combat->track[index].tick = NONE;
}

/* the aim point (its target's middle now) where the target was the level's
delay ago (RECRUIT and EASY), so that a target that turns back is missed a
moment, as a person misses it; the places kept a tick each */
static void bots_level_trail(struct bots_bot *bot, long now, real_point3d *aim_point)
{
	struct bots_bot_combat *combat = &bot->combat;
	long delay = bots_combat_track_delay_ticks(bots_globals.skill);
	long slot;

	if (now < 0)
		return;
	slot = now % BOTS_TRACK_TICKS;
	combat->track[slot].tick = now;
	combat->track[slot].player_index = bot->target_player_index;
	combat->track[slot].point = *aim_point;
	if (delay <= 0 || delay >= BOTS_TRACK_TICKS || now < delay)
		return;
	slot = (now - delay) % BOTS_TRACK_TICKS;
	if (combat->track[slot].tick == now - delay && combat->track[slot].player_index == bot->target_player_index)
		*aim_point = combat->track[slot].point;
}

/* the aim point: the head of a target whose shields are down, with a weapon
that kills with a shot to it (NORMAL and up); with a grenade in the hand,
ahead of the target by its flight (HEROIC and up), at most 3.5 units */
static void bots_level_aim_point(struct bots_bot *bot, struct bots_tick *tick, long target_unit_index,
	real_point3d *aim_point)
{
	struct bots_bot_combat *combat = &bot->combat;
	struct object_datum *target = object_get(target_unit_index);
	boolean head = tick->weapon_index != NONE && bots_combat_aim_head(bots_globals.skill,
		target->object.shield_vitality, bots_weapon_kind(weapon_get(tick->weapon_index)->definition_index)->precision);

	if (head)
	{
		unit_get_head_position(target_unit_index, aim_point);
		if (!combat->head_aim)
			bots_globals.combat_counts[_bots_combat_count_head_aims]++;
	}
	combat->head_aim = head;
	if (tick->now < combat->throw_until_tick && combat->throw_lead > 0.0f)
	{
		struct unit_definition *definition = unit_definition_get(tick->object->definition_index);
		real speed = definition->unit.grenade_velocity / (real)TICKS_PER_SECOND;
		real ticks, dx, dy, lead;

		if (speed < 0.05f)
			speed = 0.6f;
		ticks = MIN(8.0f + bots_distance2d(bots_tick_eye(tick), aim_point) / speed, 60.0f) * combat->throw_lead;
		dx = target->object.translational_velocity.i * ticks;
		dy = target->object.translational_velocity.j * ticks;
		lead = (real)sqrt(dx * dx + dy * dy);
		if (lead > 3.5f)
		{
			dx *= 3.5f / lead;
			dy *= 3.5f / lead;
		}
		aim_point->x += dx;
		aim_point->y += dy;
	}
}

/* the aim error once settled: RECRUIT's wander that never settles; and,
for a far target, at least the level's floor (pushed out to 0 to 30% past it,
on its side), so that a shot from far off misses about half the time */
static void bots_level_aim_error(struct bots_bot *bot, struct bots_tick *tick)
{
	real *errors[2];
	real const scale[2] = { 1.0f, 0.6f };
	real floor = bots_combat_range_error_floor(bots_globals.skill, tick->target_distance) * BOTS_DEGREES;
	short index;

	errors[0] = &bot->aim_error_yaw;
	errors[1] = &bot->aim_error_pitch;
	for (index = 0; index < 2; index++)
	{
		real random = bots_random_signed(bot);

		*errors[index] = bots_combat_aim_wander(bots_globals.skill, *errors[index] / BOTS_DEGREES, scale[index], random) *
			BOTS_DEGREES;
		if (floor > 0.0f && (real)fabs(*errors[index]) < floor * scale[index])
		{
			real sign = *errors[index] > 0.0f ? 1.0f : *errors[index] < 0.0f ? -1.0f : random < 0.0f ? -1.0f : 1.0f;

			*errors[index] = sign * floor * scale[index] * (1.0f + 0.3f * (real)fabs(random));
		}
	}
}

/* whether the bot may fire now (NORMAL and up): the sniper rifle zoomed at
a target past 6 units, zoomed again a moment after a hit took the zoom (and
not fired past 10 units meanwhile); the pistol paced at range, its shot held
while its error is over the level's limit, a few ticks at most */
static boolean bots_level_may_fire(struct bots_bot *bot, struct bots_tick *tick)
{
	struct bots_bot_combat *combat = &bot->combat;
	struct bots_weapon_kind const *kind;
	short skill = bots_globals.skill;

	/* (a vehicle's gun: as ever) */
	if (tick->weapon_index == NONE || tick->object->object.parent_object_index != NONE)
		return TRUE;
	kind = bots_weapon_kind(weapon_get(tick->weapon_index)->definition_index);
	if (kind->sniper)
	{
		boolean zooms = bots_combat_zooms(skill, tick->target_distance);

		/* (hurt while zoomed: the game took the zoom) */
		if (bot->hurt_tick == tick->now && tick->unit->unit.current_zoom_level == NONE &&
			tick->now >= combat->rescope_until_tick && zooms)
		{
			combat->rescope_until_tick = tick->now + bots_combat_rescope_ticks(skill, bots_random_real(bot));
			bots_globals.combat_counts[_bots_combat_count_rescopes]++;
		}
		if (tick->now < combat->rescope_until_tick)
			return (boolean)!bots_combat_unzoomed_holds_fire(tick->target_distance);
		if (zooms)
			tick->action->desired_zoom_level = 0;
		return TRUE;
	}
	if (kind->precision && kind->blooms &&
		bots_combat_pace_holds(skill, tick->target_distance, weapon_get(tick->weapon_index)->weapon.triggers[0].error,
			combat->pace_since_tick == NONE ? 0 : tick->now - combat->pace_since_tick))
	{
		if (combat->pace_since_tick == NONE)
		{
			combat->pace_since_tick = tick->now;
			bots_globals.combat_counts[_bots_combat_count_paced]++;
		}
		return FALSE;
	}
	combat->pace_since_tick = NONE;
	return TRUE;
}

/* whether a unit has its back to a point (it faces more than 114.6 degrees
away from it) */
static boolean bots_unit_back_turned(long unit_index, real_point3d const *point)
{
	struct object_datum *object = (struct object_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	real dx, dy, length, forward_length;

	if (!object)
		return FALSE;
	dx = point->x - object->object.position.x;
	dy = point->y - object->object.position.y;
	length = (real)sqrt(dx * dx + dy * dy);
	forward_length = (real)sqrt(object->object.forward.i * object->object.forward.i +
		object->object.forward.j * object->object.forward.j);
	if (length < 0.01f || forward_length < 0.01f)
		return FALSE;
	return (dx * object->object.forward.i + dy * object->object.forward.j) / (length * forward_length) < -0.416f;
}

/* whether the bot strikes now: within the level's reach it waits the
level's moment first; a blow it chose not to strike is asked again a second
later */
static boolean bots_level_melee(struct bots_bot *bot, struct bots_tick *tick, long target_unit_index)
{
	struct bots_bot_combat *combat = &bot->combat;
	int choice = bots_combat_melee(bots_globals.skill, tick->target_distance, tick->now >= bot->melee_tick,
		bots_unit_back_turned(target_unit_index, &tick->position), object_get(target_unit_index)->object.shield_vitality,
		combat->charging, bots_random_real(bot));

	if (choice == 0)
	{
		combat->melee_ready_tick = NONE;
		return FALSE;
	}
	if (combat->melee_ready_tick == NONE)
		combat->melee_ready_tick = tick->now + bots_combat_melee_delay(bots_globals.skill, bots_random_real(bot));
	if (tick->now < combat->melee_ready_tick)
		return FALSE;
	combat->melee_ready_tick = NONE;
	if (choice == 2)
	{
		bot->melee_tick = tick->now + TICKS_PER_SECOND;
		bots_globals.combat_counts[_bots_combat_count_blows_held]++;
		return FALSE;
	}
	bots_globals.combat_counts[_bots_combat_count_blows]++;
	return TRUE;
}

/* a grenade by the level's rules (the throw's aim leads the target's path
while it is in the hand, from HEROIC) */
static void bots_level_grenade(struct bots_bot *bot, struct bots_tick *tick, long reaction_ticks)
{
	struct bots_bot_combat *combat = &bot->combat;
	short skill = bots_globals.skill;
	long now = tick->now;
	boolean has = tick->unit->unit.grenade_counts[0] > 0 || tick->unit->unit.grenade_counts[1] > 0;

	if (now < bot->grenade_tick || !has)
		return;
	if (!bots_combat_grenade(skill, tick->target_distance, has, now - bot->target_first_seen_tick >= reaction_ticks,
		now - bot->target_first_seen_tick < TICKS_PER_SECOND * 3 / 2,
		bots_combat_about_to_die(tick->object->object.shield_vitality, tick->object->object.body_vitality),
		bots_skills[skill].grenade_chance, bots_random_real(bot)))
	{
		return;
	}
	tick->flags |= FLAG(_unit_control_throw_grenade_bit);
	if (tick->unit->unit.grenade_counts[(long)tick->unit->unit.current_grenade_index] <= 0)
		tick->action->desired_grenade_index = tick->unit->unit.grenade_counts[0] > 0 ? 0 : 1;
	bot->grenade_tick = now + bots_combat_grenade_cooldown(skill, bots_random_real(bot));
	combat->throw_until_tick = now + TICKS_PER_SECOND / 2;
	combat->throw_lead = bots_combat_grenade_lead(skill);
	bots_globals.combat_counts[_bots_combat_count_grenades]++;
}

/* a place to back off to: a roaming place 5 to 35 units off, not towards the
target, the furthest from the target for the least way; FALSE for none */
static boolean bots_level_retreat_goal(struct bots_bot *bot, struct bots_tick *tick, real_point3d const *target)
{
	long index, best = NONE;
	real best_score = -REAL_MAX;
	real tx = target->x - tick->position.x, ty = target->y - tick->position.y;
	real target_distance = (real)sqrt(tx * tx + ty * ty);

	for (index = 0; index < bots_globals.roam_point_count; index++)
	{
		real_point3d const *point = &bots_globals.roam_points[index];
		real px = point->x - tick->position.x, py = point->y - tick->position.y;
		real distance = (real)sqrt(px * px + py * py);
		real score;

		if (distance < 5.0f || distance > 35.0f || bots_globals.roam_surfaces[index] == NONE)
			continue;
		if (target_distance > 0.01f && (px * tx + py * ty) > 0.3f * distance * target_distance)
			continue;
		score = bots_distance2d(point, target) - 0.6f * distance;
		if (score > best_score)
		{
			best_score = score;
			best = index;
		}
	}
	if (best == NONE)
		return FALSE;
	bot->roam_index = (short)best;
	bot->hunting = FALSE;
	bots_set_goal(bot, _bots_goal_roam, &bots_globals.roam_points[best], bots_globals.roam_surfaces[best]);
	return TRUE;
}

/* the fight's moves by level, after the strafe: halts (it stands, keeping
its weapon's distance, at a fight's start and now and then at a new strafe),
a retreat along a path when losing (still shooting; not back to where the
enemy was), a run in to finish a won fight, a step in for a blow, jumps at
the level's rhythm */
static void bots_level_move(struct bots_bot *bot, struct bots_tick *tick, struct object_datum *target)
{
	struct bots_bot_combat *combat = &bot->combat;
	short skill = bots_globals.skill;
	long now = tick->now;
	real_point3d const *target_point = &target->object.bounding_sphere_center;
	real to_target = (real)atan2(target_point->y - tick->position.y, target_point->x - tick->position.x);
	real own_shield = tick->object->object.shield_vitality;
	real target_shield = target->object.shield_vitality;
	real confidence = bots_combat_confidence(own_shield, tick->object->object.body_vitality, target_shield,
		target->object.body_vitality, tick->target_distance, bots_weapon_range(tick->weapon_index),
		tick->weapon_index != NONE ? bots_weapon_loaded(tick->weapon_index) : 1.0f,
		tick->object->object.bounding_sphere_center.z - target_point->z);
	boolean new_fight = now - combat->last_fight_tick > TICKS_PER_SECOND * 2 || now < combat->last_fight_tick;
	int started, ended;
	boolean retreating = bots_combat_retreat_step(&combat->retreat, skill, confidence, own_shield, now,
		bots_combat_hash01((unsigned long)bot->player_index, (unsigned long)combat->retreat.next_allowed), &started, &ended);

	if (new_fight)
	{
		combat->halt_until_tick = bot->target_first_seen_tick + bots_combat_initial_halt_ticks(skill,
			bots_combat_hash01((unsigned long)bot->player_index ^ 0x27D4EB2DUL, (unsigned long)bot->target_first_seen_tick));
		combat->next_jump_tick = now + bots_combat_jump_delay(skill, bots_random_real(bot));
		combat->last_strafe_until_tick = bot->strafe_until_tick;
	}
	else if (bot->strafe_until_tick != combat->last_strafe_until_tick)
	{
		long halt = bots_combat_halt_ticks(skill, bots_random_real(bot), bots_random_real(bot));

		combat->last_strafe_until_tick = bot->strafe_until_tick;
		if (halt > 0)
		{
			combat->halt_until_tick = now + halt;
			bots_globals.combat_counts[_bots_combat_count_halts]++;
		}
	}
	combat->last_fight_tick = now;
	if (now < combat->halt_until_tick)
	{
		real range = bots_weapon_range(tick->weapon_index);

		if (tick->target_distance > range * 1.3f)
			tick->move_direction = to_target;
		else if (tick->target_distance < range * 0.6f)
			tick->move_direction = to_target + BOTS_PI;
		else
			tick->moving = FALSE;
	}
	if (started)
	{
		bots_level_retreat_goal(bot, tick, target_point);
		bots_globals.combat_counts[_bots_combat_count_retreats]++;
	}
	if (ended)
		bot->goal_kind = _bots_goal_none;
	if (retreating)
	{
		real direction;

		bot->target_last_surface_index = NONE;
		bot->item_check_tick = MAX(bot->item_check_tick, now + TICKS_PER_SECOND);
		if (bot->goal_kind == _bots_goal_roam &&
			bots_follow_path(bot, tick->player->unit_index, &tick->position, &direction))
		{
			tick->move_direction = direction;
		}
		else
			tick->move_direction = to_target + BOTS_PI + bot->strafe_sign * 0.5f;
		tick->moving = TRUE;
		tick->throttle_scale = 1.0f;
		combat->charging = FALSE;
	}
	else
	{
		boolean charge = bots_combat_charge(skill, confidence, target_shield, target->object.body_vitality,
			tick->target_distance);

		if (charge)
		{
			tick->move_direction = to_target + bot->strafe_sign * 0.25f;
			tick->moving = TRUE;
			tick->throttle_scale = 1.0f;
			if (!combat->charging)
				bots_globals.combat_counts[_bots_combat_count_charges]++;
		}
		else if (bots_combat_step_in(skill, tick->target_distance, target_shield))
		{
			tick->move_direction = to_target + bot->strafe_sign * 0.3f;
			tick->moving = TRUE;
			tick->throttle_scale = 1.0f;
		}
		combat->charging = charge;
	}
	if (combat->next_jump_tick > 0 && now >= combat->next_jump_tick)
	{
		long delay = bots_combat_jump_delay(skill, bots_random_real(bot));

		combat->next_jump_tick = delay > 0 ? now + delay : 0;
		if (delay > 0)
		{
			tick->flags |= FLAG(_unit_control_jump_bit);
			bots_globals.combat_counts[_bots_combat_count_jumps]++;
		}
	}
}

/* out of a fight (VETERAN and up): once a lost contact, a grenade at the
spot its target was last seen, 0.6 to 3 s after, the spot 5 to 20 units off
and in its line of sight; the aim held on the spot for the throw (half a
second) */
static void bots_level_prenade(struct bots_bot *bot, struct bots_tick *tick)
{
	struct bots_bot_combat *combat = &bot->combat;
	short skill = bots_globals.skill;
	boolean has = tick->unit->unit.grenade_counts[0] > 0 || tick->unit->unit.grenade_counts[1] > 0;
	struct player_datum *target;
	real_point3d point;
	real seconds, distance;

	if (tick->now < combat->prenade_until_tick)
	{
		real_point3d const *eye = bots_tick_eye(tick);
		real dx = combat->prenade_point.x - eye->x, dy = combat->prenade_point.y - eye->y;
		real turn = bots_skills[skill].turn_degrees_per_tick * BOTS_DEGREES;
		real yaw_off = bots_angle_difference((real)atan2(dy, dx), bot->yaw);
		real pitch_off = (real)atan2(combat->prenade_point.z - eye->z, (real)sqrt(dx * dx + dy * dy)) - bot->pitch;

		bot->yaw += PIN(yaw_off, -turn, turn);
		bot->pitch += PIN(pitch_off, -turn, turn);
		/* (the throw once the aim is on the spot) */
		if (combat->prenade_until_tick - tick->now > 1 && (real)fabs(yaw_off) < 0.1f && (real)fabs(pitch_off) < 0.1f)
		{
			tick->flags |= FLAG(_unit_control_throw_grenade_bit);
			if (tick->unit->unit.grenade_counts[(long)tick->unit->unit.current_grenade_index] <= 0)
				tick->action->desired_grenade_index = tick->unit->unit.grenade_counts[0] > 0 ? 0 : 1;
			combat->prenade_until_tick = tick->now + 1;
		}
		return;
	}
	if (skill < _bots_skill_veteran || bot->target_player_index == NONE || bot->target_last_surface_index == NONE ||
		combat->prenade_seen_tick == bot->target_seen_tick || !has || tick->now < bot->grenade_tick ||
		tick->object->object.parent_object_index != NONE)
	{
		return;
	}
	seconds = (real)(tick->now - bot->target_seen_tick) / TICKS_PER_SECOND;
	if (seconds < 0.6f)
		return;
	combat->prenade_seen_tick = bot->target_seen_tick;
	/* (about the target's middle where it stood) */
	point = bot->target_last_position;
	point.z += 0.3f;
	distance = bots_distance3d(bots_tick_eye(tick), &point);
	target = player_try_and_get(bot->target_player_index);
	if (!bots_combat_prenade(skill, seconds, distance, has, bots_random_real(bot)) ||
		!bots_line_of_sight(tick->player->unit_index, bots_tick_eye(tick), &point,
			target && target->unit_index != NONE ? target->unit_index : NONE))
	{
		return;
	}
	combat->prenade_point = point;
	combat->prenade_until_tick = tick->now + TICKS_PER_SECOND / 2;
	bot->grenade_tick = tick->now + bots_combat_grenade_cooldown(skill, bots_random_real(bot));
	bots_globals.combat_counts[_bots_combat_count_prenades]++;
}

/* looking for enemies (every third tick, the bots in turn), and forgetting
the one gone a while */
static void bots_bot_look(struct bots_bot *bot, struct bots_tick *tick)
{
	if (tick->now >= bot->next_look_tick)
	{
		long target = bots_find_target(bot, tick->player, bots_tick_eye(tick));

		bot->next_look_tick = tick->now + 3;
		/* (3 to 5 ticks apart: never a metronome) */
		bot->next_look_tick += bots_combat_look_jitter(bots_combat_hash01((unsigned long)bot->player_index ^ 0x5BD1E995UL,
			(unsigned long)tick->now));
		if (target != NONE)
		{
			struct player_datum *target_player = player_get(target);
			struct object_datum *target_object = object_get(target_player->unit_index);
			real_point3d ground;
			long surface_index;

			/* (a new target: the aim's error as it first turns to it) */
			if (target != bot->target_player_index || tick->now - bot->target_seen_tick > TICKS_PER_SECOND * 2)
			{
				real error = tick->skill->aim_error_degrees * BOTS_DEGREES;

				bot->target_first_seen_tick = tick->now;
				bot->aim_error_yaw = bots_random_signed(bot) * error;
				bot->aim_error_pitch = bots_random_signed(bot) * error * 0.6f;
			}
			bot->target_player_index = target;
			bot->target_seen_tick = tick->now;
			bot->target_last_position = target_object->object.position;
			surface_index = bots_unit_surface(target_player->unit_index, &ground);
			if (surface_index != NONE)
				bot->target_last_surface_index = surface_index;
		}
	}
	if (bot->target_player_index != NONE)
	{
		struct player_datum *target_player = player_try_and_get(bot->target_player_index);

		if (!target_player || target_player->unit_index == NONE ||
			tick->now - bot->target_seen_tick > TICKS_PER_SECOND * 10)
		{
			bot->target_player_index = NONE;
		}
	}
}

/* fighting the target seen in the last half second: aim (turning at its
speed, leading, an error that settles), fire, a grenade, a blow, strafing */
static void bots_bot_fight(struct bots_bot *bot, struct bots_tick *tick)
{
	struct bots_skill_definition const *skill = tick->skill;
	struct player_datum *target_player = player_get(bot->target_player_index);
	struct object_datum *target_object = object_get(target_player->unit_index);
	real_point3d aim_point = target_object->object.bounding_sphere_center;
	long reaction_ticks = (long)(skill->reaction_seconds * TICKS_PER_SECOND);
	long now = tick->now;
	real desired_yaw, desired_pitch, dx, dy, dz, horizontal, turn, yaw_off, pitch_off, cone;
	real settle = (real)exp(-1.0f / (skill->aim_settle_seconds * TICKS_PER_SECOND) * 1.1f);

	tick->fighting = TRUE;
	/* (a reaction of 0.85 to 1.35 of the level's, another each sighting; the
	aim trailing the target at the lowest levels, at the head or ahead of a
	throw) */
	reaction_ticks = bots_combat_reaction_ticks(bots_globals.skill, reaction_ticks,
		bots_combat_hash01((unsigned long)bot->player_index, (unsigned long)bot->target_first_seen_tick));
	bots_level_trail(bot, now, &aim_point);
	bots_level_aim_point(bot, tick, target_player->unit_index, &aim_point);
	bots_tick_eye(tick);
	tick->target_distance = bots_distance3d(&tick->eye, &aim_point);
	/* (leading a moving target by the projectile's flight) */
	if (tick->weapon_index != NONE && skill->lead > 0.0f)
	{
		real speed = bots_projectile_speed(tick->weapon_index);

		if (speed > 0.01f)
		{
			real ticks = MIN(tick->target_distance / speed, 30.0f);

			aim_point.x += target_object->object.translational_velocity.i * ticks * skill->lead;
			aim_point.y += target_object->object.translational_velocity.j * ticks * skill->lead;
			aim_point.z += target_object->object.translational_velocity.k * ticks * skill->lead;
		}
	}
	dx = aim_point.x - tick->eye.x;
	dy = aim_point.y - tick->eye.y;
	dz = aim_point.z - tick->eye.z;
	horizontal = (real)sqrt(dx * dx + dy * dy);
	/* the aim error settles while the target stays in sight, with a tremble */
	bot->aim_error_yaw = bot->aim_error_yaw * settle +
		bots_random_signed(bot) * skill->aim_error_degrees * 0.04f * BOTS_DEGREES;
	bot->aim_error_pitch = bot->aim_error_pitch * settle +
		bots_random_signed(bot) * skill->aim_error_degrees * 0.03f * BOTS_DEGREES;
	/* (recruit's wander, the error kept at range) */
	bots_level_aim_error(bot, tick);
	desired_yaw = (real)atan2(dy, dx) + bot->aim_error_yaw;
	desired_pitch = (real)atan2(dz, horizontal) + bot->aim_error_pitch;
	turn = skill->turn_degrees_per_tick * BOTS_DEGREES;
	yaw_off = bots_angle_difference(desired_yaw, bot->yaw);
	pitch_off = desired_pitch - bot->pitch;
	bot->yaw += PIN(yaw_off, -turn, turn);
	bot->pitch += PIN(pitch_off, -turn, turn);

	/* fire: once it has reacted, while its aim is on the target (its size
	at that distance) */
	yaw_off = (real)fabs(bots_angle_difference((real)atan2(dy, dx), bot->yaw));
	pitch_off = (real)fabs((real)atan2(dz, horizontal) - bot->pitch);
	cone = (real)atan(0.35f / MAX(tick->target_distance, 0.5f)) * skill->fire_cone + 0.01f;
	if (!bots_level_may_fire(bot, tick))
		cone = -1.0f;
	if (now - bot->target_first_seen_tick >= reaction_ticks && now - bot->target_seen_tick <= 3 &&
		yaw_off < cone && pitch_off < cone * 1.5f && tick->weapon_index != NONE)
	{
		short tap_ticks = bots_weapon_tap_ticks(tick->weapon_index);

		if (tap_ticks > 0)
		{
			/* (a press a shot, released between: as fast as the weapon fires,
			a little slower for the less skilled) */
			if (now >= bot->burst_until_tick)
			{
				tick->flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
				bot->burst_until_tick = now + tap_ticks + (long)(bots_random(bot) % (1 + skill->pause_ticks / 3));
			}
		}
		else if (now < bot->burst_until_tick)
			tick->flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
		else if (now >= bot->pause_until_tick)
		{
			/* (bursts, with pauses between) */
			bot->burst_until_tick = now + skill->burst_ticks + (long)(bots_random(bot) % 4);
			bot->pause_until_tick = bot->burst_until_tick + skill->pause_ticks + (long)(bots_random(bot) % 4);
			tick->flags |= FLAG(_unit_control_weapon_primary_trigger_bit);
		}
	}
	/* a blow close up (the level's reach, its moment and its wait) */
	if (bots_level_melee(bot, tick, target_player->unit_index))
	{
		tick->flags |= FLAG(_unit_control_use_equipment_bit);
		bot->melee_tick = now + bots_combat_melee_cooldown(bots_globals.skill, bots_random_real(bot));
	}
	/* a grenade at middle range, by the level's rules */
	bots_level_grenade(bot, tick, reaction_ticks);
	/* strafing, to a side for a while; nearer or further to keep its
	weapon's distance; a jump now and then */
	if (now >= bot->strafe_until_tick)
	{
		bot->strafe_sign = bots_random_real(bot) < 0.5f ? -1.0f : 1.0f;
		/* (the level's strafes, short jiggles at the top) */
		bot->strafe_until_tick = now + bots_combat_strafe_ticks(bots_globals.skill, bots_random_real(bot));
	}
	{
		real to_target = (real)atan2(dy, dx);
		real range = bots_weapon_range(tick->weapon_index);
		real forward = tick->target_distance > range * 1.3f ? 0.8f : tick->target_distance < range * 0.6f ? -0.5f : 0.0f;
		real side = bot->strafe_sign * bots_combat_strafe_strength(bots_globals.skill);

		tick->move_direction = (real)atan2((real)sin(to_target) * forward + (real)sin(to_target + BOTS_PI * 0.5f) * side,
			(real)cos(to_target) * forward + (real)cos(to_target + BOTS_PI * 0.5f) * side);
		tick->moving = TRUE;
		tick->throttle_scale = 1.0f;
	}
	/* crouches, halts, a retreat, a charge, a step in, jumps, by level */
	if (bots_combat_crouch_chance(bots_globals.skill) > 0.0f &&
		bots_combat_crouch_step(&bot->combat.crouch, bots_globals.skill, now, bots_random_real(bot)))
	{
		tick->flags |= FLAG(_unit_control_crouch_modifier_bit);
	}
	bots_level_move(bot, tick, target_object);
}

/* walking to the goal, looking where it goes; there, a look about (and the
last bit straight onto an item); a reload when no one is about */
static void bots_bot_walk(struct bots_bot *bot, struct bots_tick *tick)
{
	real direction;

	if (bots_follow_path(bot, tick->player->unit_index, &tick->position, &direction))
	{
		real turn = 12.0f * BOTS_DEGREES;
		real off = bots_angle_difference(direction, bot->yaw);

		tick->move_direction = direction;
		tick->moving = TRUE;
		bot->yaw += PIN(off, -turn, turn);
		bot->pitch += PIN(-bot->pitch, -0.05f, 0.05f);
		if (bot->climb_until_tick && tick->now < bot->climb_until_tick && (bot->climb_until_tick - tick->now) % 20 == 0)
			tick->flags |= FLAG(_unit_control_jump_bit);
	}
	else
	{
		bot->yaw += 2.0f * BOTS_DEGREES;
		if (bot->goal_kind == _bots_goal_roam || bot->goal_kind == _bots_goal_enemy)
			bot->goal_kind = _bots_goal_none;
		/* (at an item: on to it) */
		if (bot->goal_kind == _bots_goal_item)
		{
			struct object_datum *item = object_try_and_get_and_verify_type(bot->item_index, _object_mask_item);

			if (item && bots_distance2d(&item->object.position, &tick->position) > 0.3f)
			{
				tick->move_direction = (real)atan2(item->object.position.y - tick->position.y,
					item->object.position.x - tick->position.x);
				tick->moving = TRUE;
				tick->throttle_scale = 0.6f;
			}
		}
	}
	if (tick->weapon_index != NONE && bots_weapon_loaded(tick->weapon_index) < 0.4f &&
		tick->now - bot->target_seen_tick > TICKS_PER_SECOND * 2)
	{
		tick->flags |= FLAG(_unit_control_weapon_reload_bit);
	}
}

/* the bots in each other's way (a team's bots leaving their base by the same
door, Longest's, jammed in it a whole game, each going round the others):
the one with the higher index steps back from the other, while the other
moves, so that the first goes through */
static void bots_bot_yield(long bot_index, struct bots_tick *tick)
{
	long index;

	if (!tick->moving || tick->fighting || tick->object->object.parent_object_index != NONE)
		return;
	for (index = 0; index < bot_index; index++)
	{
		struct bots_bot *other = &bots_globals.bots[index];
		struct object_datum *object;
		real dx, dy, distance, velocity;

		if (other->player_index == NONE || other->unit_index == NONE)
			continue;
		object = (struct object_datum *)object_try_and_get_and_verify_type(other->unit_index, _object_mask_unit);
		if (!object || object->object.parent_object_index != NONE)
			continue;
		dx = object->object.position.x - tick->position.x;
		dy = object->object.position.y - tick->position.y;
		distance = (real)sqrt(dx * dx + dy * dy);
		velocity = (real)sqrt(object->object.translational_velocity.i * object->object.translational_velocity.i +
			object->object.translational_velocity.j * object->object.translational_velocity.j);
		if (distance > 1.1f || distance < 0.01f || fabs(object->object.position.z - tick->position.z) > 1.5f ||
			velocity < 0.02f ||
			(dx * (real)cos(tick->move_direction) + dy * (real)sin(tick->move_direction)) < 0.3f * distance)
		{
			continue;
		}
		tick->move_direction = (real)atan2(-dy, -dx);
		tick->throttle_scale = 0.6f;
		return;
	}
}

/* weapons: the other one when this one is empty; at the weapon it fetches,
once the game offers it (the player's action, as the HUD shows it), the
action button held, as a player holds it to swap a weapon (the swap:
players_update_before_game's player_handle_weapon_swap), let go now and
then, so that a second swap can follow a first */
static void bots_bot_weapons(struct bots_bot *bot, struct bots_tick *tick)
{
	if (tick->weapon_index != NONE && bots_weapon_empty(tick->weapon_index))
	{
		short slot;

		for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		{
			long other = tick->unit->unit.weapon_object_indices[slot];

			if (slot != tick->unit->unit.current_weapon_index && other != NONE && !bots_weapon_empty(other))
			{
				tick->action->desired_weapon_index = slot;
				break;
			}
		}
	}
	if (bot->goal_kind == _bots_goal_item)
	{
		struct object_datum *item = object_try_and_get_and_verify_type(bot->item_index, _object_mask_weapon);

		if (item && item->object.parent_object_index == NONE &&
			bots_distance2d(&item->object.position, &tick->position) < 0.8f &&
			fabs(item->object.position.z - tick->position.z) < 1.2f)
		{
			if (tick->now - bot->item_press_tick > TICKS_PER_SECOND)
				bot->item_press_tick = tick->now;
			if (bot->item_first_press_tick < bot->goal_tick)
				bot->item_first_press_tick = tick->now;
			if (tick->now - bot->item_press_tick < TICKS_PER_SECOND / 2 &&
				tick->player->action_object_index == bot->item_index &&
				(tick->player->action_result == _player_action_result_pickup_weapon ||
					tick->player->action_result == _player_action_result_swap_for_weapon ||
					tick->player->action_result == _player_action_result_add_weapon_to_inventory))
			{
				tick->flags |= FLAG(_unit_control_swap_weapons_bit);
			}
		}
	}
}

/* stuck: not moving for a second while it walks - a jump and a sidestep,
then another path; somewhere else after three */
static void bots_bot_unstick(struct bots_bot *bot, struct bots_tick *tick)
{
	long now = tick->now;

	if (now - bot->stuck_tick >= TICKS_PER_SECOND)
	{
		if (tick->moving && bots_distance2d(&tick->position, &bot->stuck_position) < 0.4f &&
			tick->object->object.parent_object_index == NONE)
		{
			bots_globals.stuck_events++;
			bot->stuck_count++;
			bot->unstick_until_tick = now + TICKS_PER_SECOND / 2;
			bot->unstick_direction = tick->move_direction + (bots_random_real(bot) < 0.5f ? -1.0f : 1.0f) * BOTS_PI * 0.5f;
			bot->path.valid = FALSE;
			if (bot->stuck_count >= 3)
			{
				bots_globals.stuck_giveups++;
				bot->goal_kind = _bots_goal_none;
				bot->stuck_count = 0;
				bots_pick_roam_goal(bot, &tick->position);
			}
		}
		else
			bot->stuck_count = 0;
		bot->stuck_position = tick->position;
		bot->stuck_tick = now;
	}
	if (now < bot->unstick_until_tick)
	{
		tick->move_direction = bot->unstick_direction;
		tick->moving = TRUE;
		if (now == bot->unstick_until_tick - TICKS_PER_SECOND / 2)
			tick->flags |= FLAG(_unit_control_jump_bit);
	}
}

/* (debug.bot_trace = <seconds>) each bot's state that often */
static void bots_bot_trace(struct bots_bot *bot, long bot_index, struct bots_tick *tick)
{
	static long trace = -1;
	long now = tick->now;

	if (trace < 0)
		trace = MAX(config_integer("debug.bot_trace"), 0) * TICKS_PER_SECOND;
	if (trace <= 0 || now % trace != bot_index)
		return;
	platform_log("bots: trace %ld bot %ld at (%.1f %.1f %.1f) goal %d (%.1f %.1f %.1f) s%ld path %s %d/%d%s "
		"target %ld seen %ld ago%s stuck %d yaw %.0f throttle %.2f/%.2f flags %lx searches %ld failures %ld",
		now, bot_index + 1, tick->position.x, tick->position.y, tick->position.z, (int)bot->goal_kind,
		bot->goal_point.x, bot->goal_point.y, bot->goal_point.z, bot->goal_surface_index,
		bot->path.valid ? "yes" : "no", (int)bot->path.step_index, (int)bot->path.step_count,
		bot->path.steps_finish_path ? " (all)" : "",
		bot->target_player_index == NONE ? -1L : (long)DATUM_INDEX_TO_ABSOLUTE_INDEX(bot->target_player_index),
		now - bot->target_seen_tick, tick->fighting ? " fighting" : "", (int)bot->stuck_count,
		bot->yaw / BOTS_DEGREES, tick->action->throttle.i, tick->action->throttle.j, tick->action->control_flags,
		bot->searches, bot->path_failures);
	if (tick->weapon_index != NONE)
	{
		struct weapon_datum *weapon = weapon_get(tick->weapon_index);

		platform_log("bots: trace weapon %s loaded %d total %d, aim off %.1f/%.1f degrees, distance %.1f",
			tag_get_name(weapon->definition_index), (int)weapon->weapon.magazines[0].rounds_loaded,
			(int)weapon->weapon.magazines[0].rounds_total, bot->aim_error_yaw / BOTS_DEGREES,
			bot->aim_error_pitch / BOTS_DEGREES, tick->target_distance);
	}
}

static void bots_bot_update(struct bots_bot *bot, long bot_index, struct player_action *action)
{
	struct bots_tick tick;
	real vitality;

	csmemset(&tick, 0, sizeof(tick));
	tick.skill = &bots_skills[bots_globals.skill];
	tick.player = player_get(bot->player_index);
	tick.now = game_time_get();
	tick.throttle_scale = 1.0f;
	tick.action = action;
	csmemset(action, 0, sizeof(*action));
	action->desired_weapon_index = NONE;
	action->desired_grenade_index = NONE;
	action->desired_zoom_level = NONE;
	action->desired_facing.yaw = bot->yaw;
	action->desired_facing.pitch = bot->pitch;

	if (tick.player->unit_index == NONE)
	{
		/* (dead: it forgets its fight and its path; the game respawns it) */
		bot->unit_index = NONE;
		bot->target_player_index = NONE;
		bot->path.valid = FALSE;
		bot->goal_kind = _bots_goal_none;
		bot->previous_control_flags = 0;
		return;
	}
	tick.unit = unit_get(tick.player->unit_index);
	tick.object = object_get(tick.player->unit_index);
	tick.position = tick.object->object.position;
	if (bot->unit_index != tick.player->unit_index)
	{
		/* (spawned: it looks the way its unit faces) */
		bot->unit_index = tick.player->unit_index;
		bot->yaw = (real)atan2(tick.object->object.forward.j, tick.object->object.forward.i);
		bot->pitch = 0.0f;
		bot->last_vitality = tick.object->object.body_vitality + tick.object->object.shield_vitality;
		bot->stuck_position = tick.position;
		bot->stuck_tick = tick.now;
		bot->stuck_count = 0;
		bot->goal_kind = _bots_goal_none;
		bot->next_look_tick = tick.now + (bot_index % 3);
		bot->climb_until_tick = 0;
		bot->path_short = FALSE;
		bot->escape_until_tick = 0;
		bot->last_position = tick.position;
		bots_level_reset(&bot->combat);
	}
	/* (sent on by a teleporter it walked onto: a new path from where it came
	out) */
	if (bots_distance3d(&tick.position, &bot->last_position) > 3.0f && bot->goal_kind != _bots_goal_none &&
		tick.object->object.parent_object_index == NONE)
	{
		bot->path.valid = FALSE;
		bot->path_tick = tick.now - TICKS_PER_SECOND;
		bot->stuck_position = tick.position;
	}
	bot->last_position = tick.position;

	/* hurt: it turns to look for who did it */
	vitality = tick.object->object.body_vitality + tick.object->object.shield_vitality;
	if (vitality < bot->last_vitality - 0.01f)
		bot->hurt_tick = tick.now;
	bot->last_vitality = vitality;

	bots_bot_look(bot, &tick);
	tick.weapon_index = bots_current_weapon(tick.unit);
	if (bot->target_player_index != NONE && tick.now - bot->target_seen_tick <= TICKS_PER_SECOND / 2)
		bots_bot_fight(bot, &tick);
	/* the goal, and the path to it (while fighting, only a weapon close by:
	towards it, still facing the enemy) */
	if (!tick.fighting || bot->goal_kind == _bots_goal_item)
		bots_choose_goal(bot, bot->player_index, &tick.position, tick.fighting);
	if (!tick.fighting)
	{
		bots_bot_walk(bot, &tick);
		bots_bot_yield(bot_index, &tick);
		bots_level_prenade(bot, &tick);
	}
	else if (bot->goal_kind == _bots_goal_item)
	{
		real direction;

		if (bots_follow_path(bot, tick.player->unit_index, &tick.position, &direction))
			tick.move_direction = direction;
	}
	bots_bot_weapons(bot, &tick);
	bots_bot_unstick(bot, &tick);

	if (tick.moving)
	{
		real relative = bots_angle_difference(tick.move_direction, bot->yaw);

		action->throttle.i = (real)cos(relative) * tick.throttle_scale;
		action->throttle.j = (real)sin(relative) * tick.throttle_scale;
	}
	bot->pitch = PIN(bot->pitch, -1.4f, 1.4f);
	bot->yaw = bots_angle_difference(bot->yaw, 0.0f);
	action->desired_facing.yaw = bot->yaw;
	action->desired_facing.pitch = bot->pitch;

	/* a press is one tick held and the next released (the jump, the grenade,
	the blow, the reload), as a player's buttons are; the trigger and the
	swap button may be held */
	{
		unsigned long pressed = FLAG(_unit_control_jump_bit) | FLAG(_unit_control_throw_grenade_bit) |
			FLAG(_unit_control_use_equipment_bit) | FLAG(_unit_control_weapon_reload_bit);

		tick.flags &= ~(bot->previous_control_flags & pressed);
		bot->previous_control_flags = tick.flags;
	}
	action->control_flags = tick.flags;
	action->primary_trigger = TEST_FLAG(tick.flags, _unit_control_weapon_primary_trigger_bit) ? 1.0f : 0.0f;
	bots_bot_trace(bot, bot_index, &tick);
}

/* halo.log's line, a minute into the game and each minute on */
static void bots_report(void)
{
	long now = game_time_get();
	long count = 0, index;
	char line[400];
	int length = 0;

	if (bots_globals.last_report_tick != NONE && now - bots_globals.last_report_tick < TICKS_PER_SECOND * 60)
		return;
	bots_globals.last_report_tick = now;
	line[0] = 0;
	for (index = 0; index < BOTS_MAXIMUM; index++)
	{
		struct bots_bot *bot = &bots_globals.bots[index];
		struct player_datum *player = bot->player_index != NONE ? player_try_and_get(bot->player_index) : NULL;

		if (!player)
			continue;
		count++;
		if (length < (int)sizeof(line) - 40)
			length += snprintf(line + length, sizeof(line) - (size_t)length, " %ld:%d/%d", index + 1,
				(int)player->statistics.kills[0], (int)player->statistics.deaths);
	}
	if (!count)
		return;
	platform_log("bots: tick %ld, %ld bots (%s): searches %ld (%ld failed, %ld short, %ld full, %ld unavoided, "
		"%ld unsmoothed), stuck %ld (goals given up %ld), kills/deaths%s", now, count,
		bots_skills[bots_globals.skill].name, bots_globals.searches, bots_globals.search_failures,
		bots_globals.search_partials, bots_globals.search_overflows, bots_globals.search_unavoided,
		bots_globals.search_unsmoothed, bots_globals.stuck_events, bots_globals.stuck_giveups, line);
	{
		long const *counts = bots_globals.combat_counts;

		platform_log("bots: fights since the map's start: grenades %ld (%ld at a last-seen spot), blows %ld "
			"(%ld held back), head aims %ld, retreats %ld, charges %ld, jumps %ld, halts %ld, shots paced %ld, "
			"zoomed again %ld, high ground picks %ld, enemies out of vertical view %ld",
			counts[_bots_combat_count_grenades] + counts[_bots_combat_count_prenades], counts[_bots_combat_count_prenades],
			counts[_bots_combat_count_blows], counts[_bots_combat_count_blows_held], counts[_bots_combat_count_head_aims],
			counts[_bots_combat_count_retreats], counts[_bots_combat_count_charges], counts[_bots_combat_count_jumps],
			counts[_bots_combat_count_halts], counts[_bots_combat_count_paced], counts[_bots_combat_count_rescopes],
			counts[_bots_combat_count_high_ground], counts[_bots_combat_count_out_of_view]);
	}
}

void bots_update_actions(struct player_action *actions)
{
	struct data_iterator iterator;
	struct player_datum *player;
	long count = 0;

	/* (local games alone: no other machine sees a bot) */
	if (!game_engine_running() || !network_game_is_splitscreen_local())
		return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		long bot_index;
		long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		struct bots_bot *bot;

		if (player->local_player_index != NONE || !bots_machine_is_bot(player->network_player_data.machine_index))
			continue;
		/* (the roaming places and the walkable surfaces once a game has
		bots; the skill once a tick) */
		if (!count)
		{
			if (!bots_globals.map_ready)
				bots_prepare_map();
			bots_globals.skill = bots_skill();
			bots_globals.tick_searches = 0;
			bots_paths_scan();
		}
		bot_index = player->network_player_data.machine_index - BOTS_FIRST_MACHINE;
		bot = &bots_globals.bots[bot_index];
		if (bot->player_index != iterator.datum_index)
			bots_bot_reset(bot, iterator.datum_index, bot_index);
		if (absolute_index >= 0 && absolute_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
			bots_bot_update(bot, bot_index, &actions[absolute_index]);
		count++;
	}
	if (count)
	{
		bots_globals.ticks++;
		bots_report();
	}
}
