/*
NETWORK_TEST.C

Automated system link sessions for testing the netcode without the menus
(debug.network_test in config.toml, HALO_NETWORK_TEST):

- "host:<map>[:<variant>[,<variant>...]]" hosts a game on that
  multiplayer map (bloodgulch, ...) with one of the built-in game variants
  (slayer by default; game_engine_get_variant_by_name), as the pregame
  screen's fast setup does, and starts it debug.network_test_start seconds
  later; with more variants, once a game is over (debug.network_test_score
  makes it short) the next, as the host's button on the scores does. A map
  with a path is that level (custom_maps\a30, a Custom Edition map's), and
  the variant "coop" makes the game network co-op on it, at normal
  difficulty, as the Map screen does for a campaign level;
- "join" searches for games and joins the first it finds, as picking it in
  the system link list does;
- "browse" joins the game with the most players that the server browser
  lists after five seconds (internet play's public games: a host's with
  debug.network_test_public), co-op ones last, as picking it there does,
  and then as "join". It logs what it lists.

Once the game runs, every second each machine logs where every player's
unit is, so the machines' views of the game can be compared.

Scripted play for the netcode's parts the bots' wandering does not reach:
debug.network_test_kill (the host kills the last player every so often),
debug.network_test_shoot (every so often each machine's player hits the
next with their weapon's projectile: a client's through its report to the
host) and debug.network_test_vehicle (the host seats the last player as a
vehicle's driver that many seconds in, takes them out 15 seconds on, and 5
seconds later stands them at a vehicle's driver's entrance, where a joining
machine's player holds the action button, as getting in does)
and debug.network_test_pickup (the last player stands on a weapon lying
about that many seconds in, and a joining machine's player holds the action
button a second later, to pick it up).

Called from the main loop every frame (main.c).
*/

#include "cseries.h"
#include "infection.h"
#include "network_infection.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "networking/network_server_manager.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/bipeds.h"
#include "units/unit_definitions.h"
#include "units/vehicle_definitions.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "items/items.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_files.h"
#include "saved games/playlist_profile.h"
#include "text/unicode.h"
#include "camera/observer.h"
#include "physics/collisions.h"

#include "../src/p2p.h" /* port: port/linux/src/p2p.c, p2p_lobby.c */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the platform layer's (port/linux/src/port_config.c) */
const char *config_string(char const *name);
int config_boolean(char const *name);
double config_real(char const *name);
long config_integer(char const *name);
void platform_log(char const *format, ...);
/* damage.c's */
void damage_kill_object_for_player(long object_index, long player_index);
/* network_distributed.c's */
void network_distributed_statistics(long *sent, long *received, long *corrections);
void network_distributed_item_statistics(long *creates, long *deletes, long *failures, long *removed);
void network_damage_statistics(long *sent_reports, long *dealt_reports, long *rejected_reports, long *replayed_events);
/* xinput_sdl.c's */
void test_input_hold_action(int hold);
void test_input_objective_actions(int enabled);
void test_input_combat_actions(int action);
void test_input_scoreboard(int show);
void player_control_set_facing(short local_player_index, real_vector3d const *direction);
long distributed_player_ping(short player_index);

/* Initial selection fixture only: subsequent infection still requires an
   actual death. NONE leaves ordinary random selection unchanged. */
long network_test_infection_initial_player(void);

enum
{
	_network_test_off,
	_network_test_host,
	_network_test_join,
	_network_test_browse,
	_network_test_map_name_length = sizeof(((struct network_game_map *)0)->name),
};

static struct
{
	boolean checked;
	short mode;
	char map_name[_network_test_map_name_length];
	/* Owned, NUL-separated tokens, retained for this process's test session. */
	char *variant_name;
	size_t variant_count;
	/* ... the variant of this game, of variant_name's list; and the seconds
	the last game's scores have been shown */
	size_t variant_index;
	real postgame_seconds;
	boolean game_over;
	real start_delay;
	real menu_seconds;
	boolean set_up;
	real setup_seconds;
	boolean started;
	boolean joined;
	boolean map_set;
	boolean player_added;
	real joined_seconds;
	boolean team_set;
	real kill_interval;
	long infection_kills;
	real shoot_interval;
	real vehicle_time;
	real pickup_time;
	char *pickup_weapon;
	long score_to_win;
	long logged_time;
	boolean public_game;
	boolean browsing;
	boolean browse_listed;
	float browse_seconds;
} network_test;

/* No fixed-size copy: both the list and each complete token are retained. */
static char const *network_test_variant(
	size_t index)
{
	char const *variant = network_test.variant_name;

	if (index >= network_test.variant_count)
		return NULL;
	for (; index > 0; index--)
		variant += strlen(variant) + 1;
	return variant;
}

static boolean network_test_parse_host(
	char const *setting)
{
	size_t map_length = strcspn(setting, ":");
	size_t count = 0, path_length;
	char const *variants = setting[map_length] ? setting + map_length + 1 : "slayer";
	char *copy, *token, *comma;

	/* The actual network map field must hold the complete expanded path. */
	if (!map_length || map_length >= sizeof(network_test.map_name))
		return FALSE;
	path_length = memchr(setting, '\\', map_length) ? map_length :
		strlen("levels\\test\\") + map_length + 1 + map_length;
	if (path_length >= sizeof(network_test.map_name))
		return FALSE;
	copy = (char *)malloc(strlen(variants) + 1);
	if (!copy)
		return FALSE;
	strcpy(copy, variants);
	token = copy;
	do
	{
		comma = strchr(token, ',');
		if (comma)
			*comma = 0;
		if (!*token)
		{
			free(copy);
			return FALSE;
		}
		count++;
		token = comma ? comma + 1 : NULL;
	} while (token);
	memcpy(network_test.map_name, setting, map_length);
	network_test.map_name[map_length] = 0;
	network_test.variant_name = copy;
	network_test.variant_count = count;
	return TRUE;
}

static void network_test_read_settings(
	void)
{
	char const *setting = config_string("debug.network_test");

	network_test.checked = TRUE;
	if (!strncmp(setting, "host:", 5))
	{
		if (network_test_parse_host(setting + 5))
			network_test.mode = _network_test_host;
		else
			platform_log("network test: invalid host setting (empty map/variant, map path too long, or allocation failed); test disabled");
	}
	else if (!strcmp(setting, "join"))
	{
		network_test.mode = _network_test_join;
	}
	else if (!strcmp(setting, "browse"))
	{
		network_test.mode = _network_test_browse;
	}
	network_test.public_game = config_boolean("debug.network_test_public") != 0;
	network_test.start_delay = (real)config_real("debug.network_test_start");
	network_test.kill_interval = (real)config_real("debug.network_test_kill");
	network_test.shoot_interval = (real)config_real("debug.network_test_shoot");
	network_test.vehicle_time = (real)config_real("debug.network_test_vehicle");
	network_test.pickup_time = (real)config_real("debug.network_test_pickup");
	setting = config_string("debug.network_test_pickup_weapon");
	network_test.pickup_weapon = (char *)malloc(strlen(setting) + 1);
	if (network_test.pickup_weapon)
		strcpy(network_test.pickup_weapon, setting);
	else
	{
		network_test.mode = _network_test_off;
		platform_log("network test: cannot allocate pickup filter; test disabled");
	}
	network_test.score_to_win = (long)config_integer("debug.network_test_score");
	if (network_test.mode != _network_test_off)
		platform_log("network test: %s", config_string("debug.network_test"));
}

/* appends to a line, cut short when it is full */
static void network_test_append(
	char *line,
	int size,
	int *length,
	char const *format,
	...)
{
	va_list arguments;
	int written;

	if (*length >= size - 1)
		return;
	va_start(arguments, format);
	written = vsnprintf(line + *length, (size_t)(size - *length), format, arguments);
	va_end(arguments);
	if (written > 0)
		*length = MIN(*length + written, size - 1);
}

/* every player's unit, as this machine sees it */
static void network_test_log_players(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	char line[4096];
	int length = 0;

	line[0] = 0;
	/* (each player's name, once a game: for tests that name a player, such
	as the host's ban command) */
	{
		static wchar_t named[HALO_PORT_MAXIMUM_NETWORK_PLAYERS][12];

		if (game_time_get() < 2 * TICKS_PER_SECOND)
			csmemset(named, 0, sizeof(named));
		data_iterator_new(&iterator, player_data);
		while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		{
			long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
			char name[13];
			short index;

			if (absolute_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS ||
				!csmemcmp(named[absolute_index], player->name, sizeof(named[absolute_index])))
			{
				continue;
			}
			csmemcpy(named[absolute_index], player->name, sizeof(named[absolute_index]));
			for (index = 0; index < 12 && player->name[index]; index++)
				name[index] = player->name[index] >= 32 && player->name[index] < 127 ? (char)player->name[index] : '?';
			name[index] = 0;
			platform_log("network test: player %ld is named %s", absolute_index, name);
		}
	}
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->unit_index != NONE)
		{
			struct object_datum *object = object_get(player->unit_index);
			struct unit_datum *unit = unit_get(player->unit_index);
			short slot;

			/* (riding: where its vehicle is) */
			struct object_datum *placed = object->object.parent_object_index != NONE ?
				object_get(object->object.parent_object_index) : object;

			network_test_append(line, (int)sizeof(line), &length, " player %ld: (%.3f %.3f %.3f) h%.2f/%.2f%s%s g%d/%d w",
				(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index), placed->object.position.x,
				placed->object.position.y, placed->object.position.z, object->object.body_vitality,
				object->object.shield_vitality, placed != object ? " riding" : "",
				TEST_FLAG(unit->unit.flags, _unit_active_camouflaged_bit) ? " camo" : "",
				unit->unit.grenade_counts[0], unit->unit.grenade_counts[1]);
			if (infection_enabled())
				network_test_append(line, (int)sizeof(line), &length, " ready%d speed%.2f",
					unit->unit.current_weapon_index, player->speed_multiplier);
			if (infection_enabled() && !strncmp(config_string("debug.test_input"), "look:drop", 9))
				network_test_append(line, (int)sizeof(line), &length, " controls%lx held%lx",
					(unsigned long)unit->unit.control_flags,
					(unsigned long)unit_inventory_get_weapon(player->unit_index, unit->unit.current_weapon_index));
			/* where it aims (yaw and pitch, degrees), its animation state and
			how hard it is moving */
			network_test_append(line, (int)sizeof(line), &length, " a%.0f/%.0f f%.0f l%.0f/%.0f as%d/%d st%d thr%.2f",
				atan2(unit->unit.aiming_vector.j, unit->unit.aiming_vector.i) * 57.29578,
				asin(PIN(unit->unit.aiming_vector.k, -1.0f, 1.0f)) * 57.29578,
				atan2(object->object.forward.j, object->object.forward.i) * 57.29578,
				atan2(unit->unit.looking_vector.j, unit->unit.looking_vector.i) * 57.29578,
				asin(PIN(unit->unit.looking_vector.k, -1.0f, 1.0f)) * 57.29578,
				(int)unit->unit.animation.aiming_screen_index, (int)unit->unit.animation.looking_screen_index,
				(int)unit->unit.animation.state,
				sqrt(unit->unit.throttle.i * unit->unit.throttle.i + unit->unit.throttle.j * unit->unit.throttle.j));
			for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
			{
				long weapon_index = unit->unit.weapon_object_indices[slot];

				if (weapon_index != NONE)
				{
					struct weapon_datum *weapon = weapon_get(weapon_index);

					network_test_append(line, (int)sizeof(line), &length, " %lx:%d",
						(unsigned long)weapon->definition_index & 0xFFFF, weapon->weapon.magazines[0].rounds_total +
						weapon->weapon.magazines[0].rounds_loaded);
				}
			}
		}
		else
		{
			network_test_append(line, (int)sizeof(line), &length, " player %ld: dead",
				(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index));
		}
		/* the game type's score and the kills and deaths */
		network_test_append(line, (int)sizeof(line), &length, " s%ld k%d d%d f%d t%ld m%d",
			game_engine && game_engine->get_player_score ?
				game_engine->get_player_score(iterator.datum_index, _get_score_individual) : -1L,
			player->statistics.kills[0], player->statistics.deaths, player->statistics.friendly_fire_kills, (long)player->team_index,
			(int)player->network_player_data.machine_index);
	}
	{
		long sent, received, corrections;

		struct object_iterator objects;
		long ground_items = 0, ground_skulls = 0;

		object_iterator_new(&objects, _object_mask_weapon | _object_mask_equipment, 0);
		while (object_iterator_next(&objects))
		{
			struct item_datum *item = item_get(objects.index);

			if (item->object.parent_object_index == NONE &&
				TEST_FLAG(item->object.flags, _object_connected_to_map_bit) &&
				!TEST_FLAG(item->item.flags, _item_attached_to_unit_bit))
			{
				ground_items++;
				if (infection_enabled() && item->definition_index == get_ball_definition_index())
					ground_skulls++;
			}
		}
		network_distributed_statistics(&sent, &received, &corrections);
		long creates, deletes, failures, removed;

		long sent_reports, dealt_reports, rejected_reports, replayed_events;

		network_distributed_item_statistics(&creates, &deletes, &failures, &removed);
		network_damage_statistics(&sent_reports, &dealt_reports, &rejected_reports, &replayed_events);
		/* this machine's player and where its camera is (a player that never
		spawns leaves it where it began) */
		long local_player_index = local_player_get_player_index(0);
		struct observer_result const *camera = observer_get_camera(0);
		if (infection_enabled() && !strncmp(config_string("debug.test_input"), "look:drop", 9))
			platform_log("infection drop test: ground_skulls=%ld ball=%lx", ground_skulls,
				(unsigned long)DATUM_INDEX_TO_ABSOLUTE_INDEX(get_ball_definition_index()));

		platform_log("network test: tick %ld%s | items %ld (+%ld -%ld !%ld x%ld) | %s to %ld | sent %ld received %ld corrected %ld"
			" | hits %ld dealt %ld rejected %ld replayed %ld | local %ld camera (%.1f %.1f %.1f) respawn %ld",
			game_time_get(), line, ground_items, creates, deletes, failures, removed,
			game_engine_can_score() ? "playing" : "game over",
			game_engine_running() ? (long)game_engine_get_variant()->universal_variant.score_to_win : 0L,
			sent, received, corrections,
			sent_reports, dealt_reports, rejected_reports, replayed_events,
			local_player_index == NONE ? -1L : (long)DATUM_INDEX_TO_ABSOLUTE_INDEX(local_player_index),
			camera ? camera->position.x : 0.0f, camera ? camera->position.y : 0.0f, camera ? camera->position.z : 0.0f,
			local_player_index == NONE ? 0L : (long)player_get(local_player_index)->respawn_timer);
	}
}

/* each of this machine's players hits the next player with their weapon's
projectile, as its impact would, when it is within the projectile's reach
(as a real shot is: the host checks it) */
/* Exercise real damage and death callbacks. The generic first/last test
   can hit teammates, which Infection's friendly-fire rules correctly block.
   First infect a human, then kill a zombie to check its respawn, then infect
   the remaining humans. Never assign a role or score from the test. */
static void network_test_infection_kill(void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	long humans[HALO_PORT_MAXIMUM_NETWORK_PLAYERS], zombies[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long human_count = 0, zombie_count = 0, victim, killer;
	short deaths;
	boolean queued = FALSE;
	if (infection_state.phase != _distributed_infection_active) return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		if (player->quit_out_of_game || player->unit_index == NONE ||
			TEST_FLAG(unit_get(player->unit_index)->object.damage_flags, _object_dead_bit)) continue;
		if (player->team_index == 0) humans[human_count++] = iterator.datum_index;
		if (player->team_index == 1) zombies[zombie_count++] = iterator.datum_index;
	}
	if (!human_count || !zombie_count) return;
	if (network_test.infection_kills == 1)
	{
		victim = zombies[zombie_count - 1];
		killer = humans[0];
	}
	else
	{
		victim = humans[human_count - 1];
		killer = zombies[0];
	}
	deaths = player_get(victim)->statistics.deaths;
	if (player_get(killer)->local_player_index != NONE)
		damage_kill_object_for_player(player_get(victim)->unit_index, killer);
	else
	{
		/* A remote player's damage must originate at that client. For this
		   host-driven lifecycle test use the normal environmental death path,
		   which is processed on the next tick, without bypassing hit authority. */
		unit_kill(player_get(victim)->unit_index);
		queued = TRUE;
		platform_log("infection test: queued environmental death slot=%ld", (long)DATUM_INDEX_TO_ABSOLUTE_INDEX(victim));
	}
	platform_log("infection test: damage victim=%ld killer=%ld deaths_before=%d deaths_after=%d",
		(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(victim), (long)DATUM_INDEX_TO_ABSOLUTE_INDEX(killer),
		deaths, player_get(victim)->statistics.deaths);
	if (queued || player_get(victim)->statistics.deaths > deaths) network_test.infection_kills++;
}

static void network_test_shoot(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;

	/* Leave time for sustained voluntary-drop attempts before the melee
	   scenario finishes. The host still gathers each second for valid reach. */
	if (infection_enabled() && !strncmp(config_string("debug.test_input"), "look:drop", 9) &&
		game_time_get() < 10 * TICKS_PER_SECOND) return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		struct data_iterator targets;
		struct player_datum *target = NULL;
		struct player_datum *candidate;
		struct unit_datum *unit;
		long weapon_index;
		struct weapon_definition *weapon;
		struct weapon_trigger_definition *trigger;
		long damage_index = NONE;
		real reach = 0.0f;
		boolean melee;
		struct damage_data damage;
		struct object_datum *target_object;
		real_vector3d direction;

		if (player->local_player_index == NONE || player->unit_index == NONE)
			continue;
		/* Infection's opt-in shooting test exercises skull damage reports.
		   Humans hold fire so a round can finish by melee alone. */
		if (infection_enabled() && (infection_state.phase != _distributed_infection_active || player->team_index != 1))
			continue;
		data_iterator_new(&targets, player_data);
		while ((candidate = (struct player_datum *)data_iterator_next(&targets)) != NULL)
		{
			if (candidate != player && candidate->unit_index != NONE &&
				(!infection_enabled() || candidate->team_index != player->team_index))
			{
				target = candidate;
				break;
			}
		}
		unit = unit_get(player->unit_index);
		if (infection_enabled())
			platform_log("infection melee test: attacker=%ld current=%d desired=%d target=%ld",
				(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index), unit->unit.current_weapon_index,
				unit->unit.desired_weapon_index, target ? target->unit_index : NONE);
		if (!target || unit->unit.current_weapon_index == NONE ||
			TEST_FLAG(object_get(target->unit_index)->object.damage_flags, _object_dead_bit))
		{
			continue;
		}
		weapon_index = unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
		if (weapon_index == NONE)
			continue;
		weapon = weapon_definition_get(object_get(weapon_index)->definition_index);
		if (weapon->weapon.triggers.count > 0)
		{
			trigger = TAG_BLOCK_GET_ELEMENT(&weapon->weapon.triggers, 0, struct weapon_trigger_definition);
			if (trigger->projectile.index != NONE)
			{
				struct projectile_definition *projectile = projectile_definition_get(trigger->projectile.index);

				damage_index = projectile->projectile.impact_damage.index;
				/* (how far it flies: its range, or at its speed for as long as
				its timer runs; none for no bound, network_damage.c) */
				if (projectile->projectile.maximum_range > 0.0f)
					reach = projectile->projectile.maximum_range;
				else if (projectile->projectile.detonation_timer_starts == 0)
				{
					reach = projectile->projectile.timer_upper_bound * TICKS_PER_SECOND *
						MAX(projectile->projectile.initial_velocity, projectile->projectile.final_velocity);
				}
			}
		}
		/* (else its melee: a blow's epicenter is its striker's, as
		unit_cause_player_melee_damage has it) */
		melee = damage_index == NONE;
		if (melee)
			damage_index = weapon->weapon.melee_attack_damage.index;
		if (damage_index == NONE)
			continue;
		target_object = object_get(target->unit_index);
		/* (a blow reaches only a target at hand, as the host checks) */
		if (melee)
		{
			real dx = target_object->object.position.x - unit->object.position.x;
			real dy = target_object->object.position.y - unit->object.position.y;
			real dz = target_object->object.position.z - unit->object.position.z;

			if (dx * dx + dy * dy + dz * dz > 1.5f * 1.5f)
				continue;
		}
		/* (a shot reaches no further than its projectile flies, with a margin
		for how far the target is from where the shot started) */
		else if (reach > 0.0f)
		{
			real dx = target_object->object.position.x - unit->object.position.x;
			real dy = target_object->object.position.y - unit->object.position.y;
			real dz = target_object->object.position.z - unit->object.position.z;

			if (dx * dx + dy * dy + dz * dz > 0.9f * reach * 0.9f * reach)
				continue;
		}
		damage_data_new(&damage, damage_index);
		damage.multiplier = melee ? infection_melee_multiplier(player->unit_index) : 1.0f;
		damage.owner_player_index = iterator.datum_index;
		damage.owner_object_index = player->unit_index;
		damage.owner_team_index = unit->object.owner_team_index;
		damage.origin = melee ? unit->object.bounding_sphere_center : target_object->object.position;
		damage.epicenter = melee ? unit->object.bounding_sphere_center : target_object->object.position;
		direction.i = target_object->object.position.x - unit->object.position.x;
		direction.j = target_object->object.position.y - unit->object.position.y;
		direction.k = target_object->object.position.z - unit->object.position.z;
		normalize3d(&direction);
		damage.direction = direction;
		damage.scale = 1.0f;
		scenario_location_from_point(&damage.location, &damage.epicenter);
		object_cause_damage(&damage, target->unit_index, NONE, NONE, NONE, NULL);
		platform_log("network test: player %ld shoots player %ld",
			(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index), (long)(target - (struct player_datum *)player_data->data));
	}
}

/* the host seats the last player as the nearest vehicle's driver, or out */
/* the host brings the players far from the first (on foot) near it, so
that they are within their weapons' reach (network_test_shoot shoots only
so near, as a player does); the host moving a client's player, as a
teleporter does */
/* The combat fixture must not put two valid BSP leaves on opposite sides
   of a wall. Check the firing corridor, not just the endpoints' clusters. */
static boolean network_test_combat_clear(real_point3d const *from, real_point3d const *to)
{
	real_point3d start = *from;
	real_vector3d direction;
	struct collision_result collision;
	long flags = FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) | FLAG(_collision_test_structure_bit) |
		FLAG(_collision_test_objects_bit) | _collision_test_objects_sight_blocking_flags;
	short height;
	direction.i = to->x - from->x;
	direction.j = to->y - from->y;
	direction.k = to->z - from->z;
	for (height = 1; height <= 2; height++)
	{
		start.z = from->z + 0.3f * height;
		if (collision_test_vector(flags, &start, &direction, NONE, &collision))
			return FALSE;
	}
	return TRUE;
}

/* Classic option matrix: movement uses ordinary local controller input. */
static boolean network_test_combat_input(void)
{
	return !strcmp(config_string("debug.test_input"), "look:combat") ||
		!strcmp(config_string("debug.test_input"), "look:classic") ||
		!strncmp(config_string("debug.test_input"), "look:inventory", 14) ||
		!strcmp(config_string("debug.test_input"), "look:vehicle");
}

static long inventory_profile_index;
static boolean inventory_profile_pending;
static struct game_variant_options inventory_profile_options;
static struct game_variant inventory_profile_variant;
static long resource_corpse = NONE, resource_weapons[2], resource_ammo[2], resource_grenades;

/* Opt-in verdict evidence counts registered players, including protocol
   stand-ins. The session manifest identifies which machines run full engines. */
void network_test_verdict_log(char const *reason, long killer, long victim, boolean friendly)
{
	struct data_iterator iterator;
	struct player_datum *player, *attacker = player_try_and_get(killer), *dead = player_try_and_get(victim);
	long humans = 0, zombies = 0, active = 0, registered = 0;
	if (strncmp(config_string("debug.test_input"), "look:party-verdict", 18)) return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		if (player->quit_out_of_game) continue;
		registered++;
		if (player->team_index == 0) humans++;
		if (player->team_index == 1) zombies++;
		if (player->unit_index != NONE && !TEST_FLAG(unit_get(player->unit_index)->object.damage_flags, _object_dead_bit)) active++;
	}
	platform_log("verdict audit: reason=%s game=%ld tick=%ld infection=%d engine=%d teams=%d limit=%d killer=%ld victim=%ld killer_team=%d victim_team=%d friendly=%d registered=%ld active=%ld red=%ld blue=%ld phase=%d winner=%d remaining=%ld client=%d",
		reason, network_game_get_number_of_games_played(), game_time_get(), infection_enabled(),
		game_engine_get_variant()->game_engine_index, game_engine_get_variant()->universal_variant.teams,
		game_engine_get_variant()->universal_variant.score_to_win, killer, victim,
		attacker ? attacker->team_index : NONE, dead ? dead->team_index : NONE, friendly,
		registered, active, humans, zombies, infection_state.phase, infection_state.winner,
		infection_enabled() ? infection_remaining_ticks() : 0L, network_game_distributed_client());
}

static void network_test_save_variant(struct game_variant const *variant)
{
	wchar_t name[64];
	if (strcmp(config_string("debug.test_input"), "look:inventory-profile") &&
		strcmp(config_string("debug.test_input"), "look:inventory-ammo") &&
		strcmp(config_string("debug.test_input"), "look:inventory-objectives") &&
		strcmp(config_string("debug.test_input"), "look:inventory-provenance")) return;
	inventory_profile_variant = *variant;
	inventory_profile_options = *player_ui_get_game_variant_options();
	usprintf(name, L"Inventory audit %lu", (unsigned long)network_test.variant_index);
	inventory_profile_index = playlist_profile_new(0, name);
	inventory_profile_pending = inventory_profile_index != NONE;
	if (inventory_profile_pending)
		playlist_profile_save_with_options(inventory_profile_index, &inventory_profile_variant, &inventory_profile_options);
	else platform_log("inventory profile: FAIL create");
}

static void network_test_reload_variant(void)
{
	struct game_variant_options loaded;
	boolean base_off, selected;
	extern boolean ui_widget_port_gametype_choose(long profile_index);
	if (!inventory_profile_pending || network_test.setup_seconds < 3.0f) return;
	/* The actual asynchronous profile writer and checksum, not a test codec. */
	if (!playlist_profile_get_options(inventory_profile_index, &loaded) ||
		csmemcmp(&loaded, &inventory_profile_options, sizeof(loaded))) return;
	inventory_profile_pending = FALSE;
	player_ui_set_game_variant(&inventory_profile_variant);
	base_off = !player_ui_get_game_variant_options()->infection_enabled;
	selected = ui_widget_port_gametype_choose(inventory_profile_index);
	platform_log("inventory profile: %s game=%lu base_off=%d selected=%d infection=%d options_equal=%d",
		base_off && selected && !csmemcmp(player_ui_get_game_variant_options(), &loaded, sizeof(loaded)) ? "PASS" : "FAIL",
		(unsigned long)network_test.variant_index, base_off, selected, player_ui_get_game_variant_options()->infection_enabled,
		!csmemcmp(player_ui_get_game_variant_options(), &loaded, sizeof(loaded)));
}

/* Opt-in host evidence, after remote report validation as well as local hits.
   The two stages bracket actual engine damage; this does not inject damage. */
void network_test_damage_log(char const *stage, struct damage_data const *damage, long target, boolean remote)
{
	struct player_datum *attacker;
	struct unit_datum *unit, *victim;
	struct damage_effect_definition *effect;
	long weapon, definition;
	if ((strncmp(config_string("debug.test_input"), "look:inventory", 14) &&
		strcmp(config_string("debug.test_input"), "look:vehicle") &&
		strncmp(config_string("debug.test_input"), "look:party-verdict", 18)) || network_test.mode != _network_test_host)
		return;
	attacker = player_try_and_get(damage->owner_player_index);
	unit = attacker && attacker->unit_index != NONE ? unit_try_and_get(attacker->unit_index) : NULL;
	victim = unit_try_and_get(target);
	if (!unit || !victim) return;
	if (!strcmp(config_string("debug.test_input"), "look:inventory-resources") && !strcmp(stage, "before") &&
		victim->object.owner_team_index == 0)
	{
		long slot;
		resource_corpse = target;
		resource_grenades = victim->unit.grenade_counts[0] + victim->unit.grenade_counts[1];
		for (slot = 0; slot < 2; slot++)
		{
			long item = victim->unit.weapon_object_indices[slot];
			resource_weapons[slot] = item;
			resource_ammo[slot] = item == NONE ? 0 : weapon_get(item)->weapon.magazines[0].rounds_total +
				weapon_get(item)->weapon.magazines[0].rounds_loaded;
		}
	}
	weapon = unit_inventory_get_weapon(attacker->unit_index, unit->unit.current_weapon_index);
	definition = weapon != NONE ? object_get(weapon)->definition_index : NONE;
	effect = damage_effect_definition_get(damage->definition_index);
	platform_log("damage audit: {\"stage\":\"%s\",\"game\":%ld,\"tick\":%ld,\"attacker\":%ld,\"machine\":%d,"
		"\"team\":%ld,\"zombie\":%d,\"weapon\":%ld,\"skull\":%d,\"effect\":%ld,\"remote\":%d,"
		"\"multiplier\":%.3f,\"scale\":%.3f,\"speed\":%.3f,\"airborne\":%d,\"target\":%ld,\"target_team\":%d,"
		"\"body\":%.4f,\"shield\":%.4f,\"max_shield\":%.4f,\"flags\":%lu,\"damage_min\":%.3f,\"damage_max\":%.3f,"
		"\"origin\":[%.3f,%.3f,%.3f],\"epicenter\":[%.3f,%.3f,%.3f]}",
		stage, network_game_get_number_of_games_played(), game_time_get(), damage->owner_player_index,
		attacker->network_player_data.machine_index, attacker->team_index, infection_unit_is_zombie(attacker->unit_index),
		definition, definition != NONE && definition == get_ball_definition_index(), damage->definition_index, remote,
		damage->multiplier, damage->scale, attacker->speed_multiplier,
		unit->object.type == _object_type_biped ? biped_get(attacker->unit_index)->biped.airborne_ticks : 0,
		target, victim->object.owner_team_index, victim->object.body_vitality, victim->object.shield_vitality,
		victim->object.maximum_shield_vitality, damage->flags, effect->damage.damage_minimum, effect->damage.damage_upper_bound,
		damage->origin.x, damage->origin.y, damage->origin.z,
		damage->epicenter.x, damage->epicenter.y, damage->epicenter.z);
	if (!strcmp(stage, "before"))
		platform_log("damage audit tags: weapon=%s effect=%s", definition == NONE ? "unarmed" : tag_get_name(definition),
			tag_get_name(damage->definition_index));
	{
		struct object_iterator items;
		long count = 0;
		object_iterator_new(&items, _object_mask_equipment, 0);
		while (object_iterator_next(&items)) count++;
		platform_log("resource audit: stage=%s game=%ld tick=%ld target=%ld grenades=%d,%d equipment=%ld",
			stage, network_game_get_number_of_games_played(), game_time_get(), target,
			victim->unit.grenade_counts[0], victim->unit.grenade_counts[1], count);
	}
}

static boolean network_test_classic_variant(char const *name, long *speed, long *shields, long *machine, long *respawn)
{
	char extra;
	return name && sscanf(name, "infection-classic-%ld-%ld-%ld-%ld%c", speed, shields, machine, respawn, &extra) == 4 &&
		(*speed == 0 || (*speed >= 100 && *speed <= 200)) && *shields >= 0 && *shields <= 1 &&
		*machine >= 0 && *machine <= 1 && *respawn >= 0 && *respawn <= 10;
}

static void network_test_gather(
	boolean leave_last)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *first = NULL;
	struct player_datum *last = NULL;
	short count = 0;
	boolean combat = network_test_combat_input() || !strncmp(config_string("debug.test_input"), "look:party-verdict", 18);
	boolean close = infection_enabled() || !strncmp(config_string("debug.test_input"), "look:inventory", 14);

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (!first)
			first = player;
		last = player;
	}
	if (!first || first->unit_index == NONE)
		return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		struct object_datum const *center = object_get(first->unit_index);
		struct object_datum *unit;
		real_point3d position;
		real dx, dy, dz;

		if (player == first || player->unit_index == NONE || (leave_last && player == last))
			continue;
		if (!strcmp(config_string("debug.test_input"), "look:inventory-resources") &&
			player->network_player_data.machine_index == 2) continue;
		count++;
		/* A client's own predicted position intentionally tolerates host
		   differences up to three metres, larger than melee reach. The
		   opt-in drop/melee fixture places that local unit at the same mark. */
		if (network_test.mode != _network_test_host && player->local_player_index == NONE)
			continue;
		unit = object_get(player->unit_index);
		if (unit->object.parent_object_index != NONE || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		dx = unit->object.position.x - center->object.position.x;
		dy = unit->object.position.y - center->object.position.y;
		dz = unit->object.position.z - center->object.position.z;
		/* Real-input combat needs an unobstructed short shot; the old 20 m
		   gathering radius could leave Slayer targets behind a building. */
		if (dx * dx + dy * dy + dz * dz <= (close ? 1.0f : combat ? 4.0f : 20.0f * 20.0f) &&
			(!combat || network_test_combat_clear(&center->object.position, &unit->object.position)))
			continue;
		/* (beside it, where the map is open: at its feet and its head) */
		{
			static real const offsets[][2] = { { 1.5f, 0.0f }, { -1.5f, 0.0f }, { 0.0f, 1.5f }, { 0.0f, -1.5f },
				{ 1.5f, 1.5f }, { -1.5f, -1.5f }, { 1.5f, -1.5f }, { -1.5f, 1.5f } };
			short try_index;

			for (try_index = 0; try_index < (short)NUMBEROF(offsets); try_index++)
			{
				short index = (short)((count + try_index) % (short)NUMBEROF(offsets));
				struct location feet, head;
				real_point3d above;

				position = center->object.position;
				position.x += offsets[index][0] * (close ? 0.5f : 1.0f);
				position.y += offsets[index][1] * (close ? 0.5f : 1.0f);
				position.z += 0.2f;
				above = position;
				above.z += 0.7f;
				scenario_location_from_point(&feet, &position);
				scenario_location_from_point(&head, &above);
				if (feet.cluster_index != NONE && head.cluster_index != NONE &&
					(!combat || network_test_combat_clear(&center->object.position, &position)))
					break;
			}
			if (try_index >= (short)NUMBEROF(offsets))
				continue;
		}
		object_set_position(player->unit_index, &position, NULL, NULL);
		platform_log("network test: fixture places player %ld near the first",
			(long)(player - (struct player_datum *)player_data->data));
	}
}

long network_test_infection_initial_player(void)
{
	char const *name;
	struct data_iterator iterator;
	struct player_datum *player;
	short machine;
	if (network_test.mode != _network_test_host ||
		(!network_test_combat_input() &&
		 strncmp(config_string("debug.test_input"), "look:party", 10) &&
		 strcmp(config_string("debug.test_input"), "look:population") &&
		 strcmp(config_string("debug.test_input"), "look:grenade"))) return NONE;
	name = network_test_variant(network_test.variant_index);
	if (!name) return NONE;
	if (!strcmp(name, "infection-combat-host")) machine = 0;
	else if (!strcmp(name, "infection-combat-client")) machine = 1;
	else if (!strncmp(name, "infection-stock", 15)) machine = strstr(name, "remote") ? 1 : 0;
	else if (!strncmp(name, "infection-verdict", 17)) machine = strstr(name, "remote") ? 1 : 0;
	else if (!strncmp(name, "infection-seat-", 15))
	{
		long type, seat, alpha;
		if (sscanf(name, "infection-seat-%ld-%ld-%ld", &type, &seat, &alpha) != 3 || alpha < 0 || alpha > 1) return NONE;
		machine = (short)alpha;
	}
	else
	{
		long speed, shields, chosen, respawn;
		if (!network_test_classic_variant(name, &speed, &shields, &chosen, &respawn)) return NONE;
		machine = (short)chosen;
	}
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
		if (player->network_player_data.machine_index == machine) return iterator.datum_index;
	return NONE;
}

/* Positions and aim are test setup. Fire/melee go through XInput, player
   control, weapon simulation, damage authority, death and ordinary respawn.
   This driver never applies damage or edits teams, kills or deaths. */
/* Real controller melee against an occupied, unchanged map vehicle. The
   fixture positions only the attacker; collision queries choose an exposed
   angle and are logged independently of the melee damage selection. */
static void network_test_vehicle_combat(void)
{
	struct data_iterator iterator;
	struct player_datum *player, *zombie = NULL, *human = NULL;
	struct object_iterator objects;
	long type, seat, alpha, vehicle = NONE, now = game_time_get(), game = network_game_get_number_of_games_played();
	static long previous_game = NONE, seated_tick, last_probe, logged_death;
	char const *name = network_test_variant(network_test.variant_index);
	struct unit_datum *attacker, *victim;
	struct object_marker target_head, own_head;
	real_point3d position, origin;
	real_vector3d ray, aim;
	struct collision_result hit;
	long attempt;
	boolean exposed = FALSE;
	test_input_combat_actions(0);
	/* Clients get the same parameters from test_input_pickup_weapon. */
	if (network_test.mode != _network_test_host)
	{
		long next;
		name = config_string("debug.network_test_pickup_weapon");
		for (next = 0; next < game && name; next++) { name = strchr(name, ','); if (name) name++; }
	}
	if (!name || sscanf(name, "infection-seat-%ld-%ld-%ld", &type, &seat, &alpha) != 3) return;
	if (!infection_enabled() || infection_state.phase < _distributed_infection_active || !game_engine_can_score()) return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
		if (player->network_player_data.machine_index == alpha) zombie = player; else human = player;
	if (!zombie || !human) return;
	if (previous_game != game) { previous_game = game; seated_tick = NONE; last_probe = NONE; logged_death = FALSE; }
	if (human->statistics.deaths)
	{
		if (!logged_death) { platform_log("vehicle probe: victim died game=%ld tick=%ld", game, now); logged_death = TRUE; }
		return;
	}
	if (zombie->unit_index == NONE || human->unit_index == NONE) return;
	attacker = unit_get(zombie->unit_index); victim = unit_get(human->unit_index);
	if (TEST_FLAG(attacker->object.damage_flags, _object_dead_bit)) return;
	vehicle = victim->object.parent_object_index;
	if (vehicle == NONE && network_test.mode == _network_test_host)
	{
		object_iterator_new(&objects, _object_mask_vehicle, 0);
		while (object_iterator_next(&objects))
			if (strstr(tag_get_name(object_get(objects.index)->definition_index),
				type == 0 ? "warthog" : type == 1 ? "ghost" : "scorpion"))
			{ vehicle = objects.index; break; }
		if (vehicle != NONE && unit_enter_seat(human->unit_index, vehicle, (short)seat))
			platform_log("vehicle probe: seated type=%ld seat=%ld label=%s machine=%d vehicle=%ld",
				type, seat, TAG_BLOCK_GET_ELEMENT(&unit_definition_get(object_get(vehicle)->definition_index)->unit.seats,
				seat, struct unit_seat)->label, human->network_player_data.machine_index, vehicle);
		else { if (now % 30 == 0) platform_log("vehicle probe: cannot seat type=%ld seat=%ld", type, seat); return; }
	}
	if (victim->object.parent_object_index == NONE) return;
	if (seated_tick == NONE) seated_tick = now;
	object_get_marker_by_name(human->unit_index, "head", &target_head, 1);
	object_get_marker_by_name(zombie->unit_index, "head", &own_head, 1);
	/* Search ordinary short rays around the exposed head. A hull hit stays a
	   hull hit; no collider is hidden or disabled to manufacture a target. */
	for (attempt = 0; attempt < 24; attempt++)
	{
		static real const direction[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{.707f,.707f},{-.707f,.707f},{.707f,-.707f},{-.707f,-.707f}};
		real height = (attempt / 8) * .3f;
		real horizontal = (real)sqrt(.65f * .65f - height * height);
		real_point3d feet;
		real_vector3d body = { 0.0f, 0.0f, .35f };
		origin = target_head.matrix.position;
		origin.x += direction[attempt % 8][0] * horizontal;
		origin.y += direction[attempt % 8][1] * horizontal;
		origin.z += height;
		feet = origin;
		feet.z -= .45f;
		/* Reject setup locations intersecting a hull before placing a biped. */
		if (collision_test_pill(_collision_test_for_bipeds_living_flags, &feet, &body, .15f, zombie->unit_index, &hit)) continue;
		vector_from_points3d(&origin, &target_head.matrix.position, &ray);
		normalize3d(&ray); scale_vector3d(&ray, .8f, &ray);
		if (collision_test_vector(_collision_test_for_projectiles_flags, &origin, &ray, zombie->unit_index, &hit) &&
			hit.type == _collision_result_object && hit.object_index == human->unit_index)
		{ exposed = TRUE; break; }
	}
	if (!exposed)
	{
		if (now % 30 == 0) platform_log("vehicle probe: protected type=%ld seat=%ld", type, seat);
		if (now - seated_tick > 10 * TICKS_PER_SECOND && network_test.mode == _network_test_host)
		{ platform_log("vehicle probe: protected complete type=%ld seat=%ld", type, seat); game_engine_end_game(); }
		return;
	}
	/* First two seconds: the same aim but outside native melee reach. */
	if (now - seated_tick < 60)
	{
		origin.x -= ray.i * 2.0f; origin.y -= ray.j * 2.0f; origin.z -= ray.k * 2.0f;
		if (now - seated_tick == 59)
			platform_log("vehicle probe: range control health=%.3f/%.3f", victim->object.body_vitality, victim->object.shield_vitality);
	}
	position.x = origin.x - (own_head.matrix.position.x - attacker->object.position.x);
	position.y = origin.y - (own_head.matrix.position.y - attacker->object.position.y);
	position.z = origin.z - (own_head.matrix.position.z - attacker->object.position.z);
	if (last_probe == NONE || now - last_probe >= 5)
	{
		if (network_test.mode == _network_test_host || zombie->local_player_index != NONE)
			object_set_position(zombie->unit_index, &position, NULL, NULL);
		platform_log("vehicle probe: exposed type=%ld seat=%ld target=%ld tick=%ld health=%.3f/%.3f distance=.65",
			type, seat, human->unit_index, now, victim->object.body_vitality, victim->object.shield_vitality);
		last_probe = now;
	}
	if (zombie->local_player_index != NONE && now - seated_tick > 15)
	{
		aim = ray; normalize3d(&aim); player_control_set_facing(zombie->local_player_index, &aim);
		test_input_combat_actions(2);
	}
	if (now - seated_tick > 20 * TICKS_PER_SECOND && network_test.mode == _network_test_host)
	{ platform_log("vehicle probe: FAIL no seated damage/death"); game_engine_end_game(); }
}

/* Three-engine resource probe: the ordinary combat driver kills machine 1.
   A receiver starts with room for ammo/grenades and one vacant weapon slot.
   All tested resources are created by unit_died, never by this fixture. */
static boolean network_test_resource_recovery(void)
{
	struct data_iterator iterator;
	struct player_datum *p, *players[3] = {NULL,NULL,NULL}, *receiver;
	static long prepared = NONE, death_tick = NONE, last_move = NONE, last_audit = NONE;
	long now = game_time_get(), index, count = 0, items[8], elapsed;
	struct object_iterator objects;
	struct unit_datum *unit;
	test_input_combat_actions(0);
	if (!game_engine_can_score()) return TRUE;
	data_iterator_new(&iterator, player_data);
	while ((p = (struct player_datum *)data_iterator_next(&iterator)))
		if (VALID_INDEX(p->network_player_data.machine_index, 3)) players[p->network_player_data.machine_index] = p;
	if (!players[0] || !players[1] || !(receiver = players[2]) || receiver->unit_index == NONE) return TRUE;
	if (prepared != receiver->unit_index && network_test.mode == _network_test_host)
	{
		long weapon;
		unit = unit_get(receiver->unit_index);
		unit->unit.desired_weapon_index = 1;
		if (unit->unit.current_weapon_index != 1) return TRUE;
		unit_drop_current_weapon(receiver->unit_index, TRUE);
		weapon = unit_inventory_get_weapon(receiver->unit_index, 0);
		if (weapon != NONE) weapon_get(weapon)->weapon.magazines[0].rounds_total = 0;
		csmemset(unit->unit.grenade_counts, 0, sizeof(unit->unit.grenade_counts));
		prepared = receiver->unit_index;
		platform_log("resource fixture: receiver capacity prepared at original spawn unit=%ld", prepared);
	}
	if (!players[1]->statistics.deaths) return FALSE;
	if (death_tick == NONE) death_tick = now;
	elapsed = now - death_tick;
	if (network_test.mode != _network_test_host) return TRUE;
	object_iterator_new(&objects, _object_mask_equipment, 0);
	while (object_iterator_next(&objects))
		if (object_get(objects.index)->object.owner_object_index == resource_corpse && count < 6) items[count++] = objects.index;
	for (index = 0; index < 2; index++) if (resource_weapons[index] != NONE &&
		object_try_and_get(resource_weapons[index]) && object_get(resource_weapons[index])->object.parent_object_index == NONE)
		items[count++] = resource_weapons[index];
	if (elapsed == 30 && last_audit != elapsed)
	{
		last_audit = elapsed;
		platform_log("resource drops: count=%ld expected=%ld corpse=%ld weapons=%ld,%ld ammo=%ld,%ld",
			count, resource_grenades + 2, resource_corpse, resource_weapons[0], resource_weapons[1], resource_ammo[0], resource_ammo[1]);
		for (index = 0; index < 2; index++)
		{
			struct weapon_datum *weapon = weapon_get(resource_weapons[index]);
			platform_log("resource dropped weapon: item=%ld reserve=%d loaded=%d", resource_weapons[index],
				weapon->weapon.magazines[0].rounds_total, weapon->weapon.magazines[0].rounds_loaded);
		}
	}
	/* Zombie contact first, then the living human, through ordinary pickup. */
	if (count && elapsed >= 30 && elapsed < 510 && (last_move == NONE || now - last_move >= 5))
	{
		struct player_datum *collector = elapsed < 210 ? players[0] : receiver;
		if (collector->unit_index != NONE)
		{
			long contact = items[((elapsed - 30) / 30) % count];
			real_point3d position = object_get(contact)->object.position;
			object_set_position(collector->unit_index, &position, NULL, NULL);
			if (elapsed < 210 && elapsed % 30 == 0) platform_log("resource zombie touches: item=%ld", contact);
		}
		last_move = now;
	}
	if (elapsed == 209 && last_audit != elapsed)
	{
		last_audit = elapsed;
		unit = unit_get(players[0]->unit_index);
		platform_log("resource zombie contact: remaining=%ld expected=%ld grenades=%d,%d weapons=%ld",
			count, resource_grenades+2, unit->unit.grenade_counts[0], unit->unit.grenade_counts[1], unit_get_weapon_count(players[0]->unit_index));
	}
	if (elapsed == 510 && last_audit != elapsed)
	{
		last_audit = elapsed;
		unit = unit_get(receiver->unit_index);
		platform_log("resource recovery: remaining=%ld grenades=%d,%d weapons=%ld",
			count, unit->unit.grenade_counts[0], unit->unit.grenade_counts[1], unit_get_weapon_count(receiver->unit_index));
		for (index = 0; index < MAXIMUM_WEAPONS_PER_UNIT; index++)
		{
			long item = unit->unit.weapon_object_indices[index];
			if (item != NONE) platform_log("resource receiver weapon: item=%ld reserve=%d loaded=%d", item,
				weapon_get(item)->weapon.magazines[0].rounds_total, weapon_get(item)->weapon.magazines[0].rounds_loaded);
		}
		game_engine_end_game();
	}
	return TRUE;
}

/* Only arrange contacts and controller input. Tested weapons are the map's
   ordinary objects; never create them or refill their ammunition here. */
static void network_test_ammo_provenance(void)
{
	static long last_game = NONE, selected = NONE, last_second = NONE, dropped = NONE, donor = NONE;
	static real_point3d home, spawn, zombie_home;
	struct data_iterator iterator;
	struct player_datum *player, *human = NULL, *zombie = NULL;
	struct unit_datum *unit;
	struct object_iterator weapons;
	struct weapon_datum *weapon;
	real_vector3d sky = {0.0f, .1f, .995f};
	real_point3d position;
	long game = network_game_get_number_of_games_played(), second = game_time_get() / TICKS_PER_SECOND;
	long kind = 2, reserve, slot, duplicate_second;
	char const *spec = config_string("debug.network_test_pickup_weapon");
	static char const *list[] = {"weapons\\pistol\\pistol", "weapons\\shotgun\\shotgun", "weapons\\assault rifle\\assault rifle", "weapons\\sniper rifle\\sniper rifle"};
	test_input_combat_actions(0);
	for (slot = 0; slot < game && spec; slot++) { spec = strchr(spec, ','); if (spec) spec++; }
	if (!game_engine_can_score() || !spec || sscanf(spec, "%ld", &kind) != 1 || kind < 0 || kind > 3) return;
	reserve = game_variant_options_get()->ammo_reserves[kind];
	/* The sniper's native item respawn is longer than the other fixtures. */
	duplicate_second = kind == 3 ? 125 : 45;
	if (game != last_game) { last_game = game; selected = dropped = donor = last_second = NONE; }
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		if (player->network_player_data.machine_index == 1) human = player;
		if (player->network_player_data.machine_index == 0 && infection_enabled()) zombie = player;
	}
	if (!human || human->unit_index == NONE || second < 6) return;
	unit = unit_get(human->unit_index);
	if (selected == NONE)
	{
		object_iterator_new(&weapons, _object_mask_weapon, 0);
		while (object_iterator_next(&weapons))
		{
			weapon = weapon_get(weapons.index);
			if (weapon->object.parent_object_index != NONE || weapon->object.owner_player_index != NONE ||
				strcmp(tag_get_name(weapon->definition_index), list[kind])) continue;
			selected = weapons.index; home = unit->object.position; spawn = weapon->object.position;
			if (zombie && zombie->unit_index != NONE) zombie_home = unit_get(zombie->unit_index)->object.position;
			platform_log("provenance map: game=%ld item=%ld reserve=%d loaded=%d kind=%ld configured=%ld", game, selected,
				weapon->weapon.magazines[0].rounds_total, weapon->weapon.magazines[0].rounds_loaded, kind, reserve);
			break;
		}
	}
	weapon = selected == NONE ? NULL : weapon_try_and_get(selected);
	if (!weapon) return;
	if (zombie && zombie->unit_index != NONE && second >= 7 && second <= 10 &&
		(network_test.mode == _network_test_host || zombie->local_player_index != NONE))
	{
		position = second < 10 ? spawn : zombie_home; position.z += .1f;
		object_set_position(zombie->unit_index, &position, NULL, NULL);
	}
	if ((second >= 11 && second < 16) || (second >= 34 && second < 39))
	{
		if (network_test.mode == _network_test_host || human->local_player_index != NONE)
		{
			position = weapon->object.position; position.z += .1f;
			if (weapon->object.parent_object_index == NONE) object_set_position(human->unit_index, &position, NULL, NULL);
		}
	}
	if (human->local_player_index != NONE)
	{
		player_control_set_facing(human->local_player_index, &sky);
		if (second >= 17 && second < 20 && reserve != 0) test_input_combat_actions(1);
		if (second >= 21 && second < 25) test_input_combat_actions(16);
	}
	/* Leave the spawn before its next scheduled generation, otherwise a
	   fresh duplicate can be collected while testing the old manual drop. */
	if (second >= 26 && second < 30 && (network_test.mode == _network_test_host || human->local_player_index != NONE))
		object_set_position(human->unit_index, &home, NULL, NULL);
	if (second == 30 && dropped == NONE && network_test.mode == _network_test_host)
	{
		for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
			if (unit->unit.weapon_object_indices[slot] == selected)
			{
				if (unit->unit.current_weapon_index == slot && unit_drop_current_weapon(human->unit_index, TRUE)) dropped = selected;
			}
	}
	if (second >= 31 && second < 34 && (network_test.mode == _network_test_host || human->local_player_index != NONE))
	{
		position = home; position.x += 3.0f;
		object_set_position(human->unit_index, &position, NULL, NULL);
	}
	if (second >= 40 && donor == NONE)
	{
		object_iterator_new(&weapons, _object_mask_weapon, 0);
		while (object_iterator_next(&weapons))
		{
			struct weapon_datum *other = weapon_get(weapons.index);
			if (weapons.index != selected && other->object.parent_object_index == NONE &&
				other->object.owner_player_index == NONE && other->definition_index == weapon->definition_index &&
				distance_squared3d(&spawn, &other->object.position) < 1.0f) { donor = weapons.index; break; }
		}
	}
	if (second >= duplicate_second && second < duplicate_second + 10 && donor != NONE &&
		(network_test.mode == _network_test_host || human->local_player_index != NONE))
	{
		position = spawn; position.z += .1f;
		object_set_position(human->unit_index, &position, NULL, NULL);
	}
	if (last_second != second)
	{
		struct weapon_datum *source = donor != NONE ? weapon_try_and_get(donor) : NULL;
		last_second = second;
		platform_log("provenance sample: game=%ld second=%ld item=%ld reserve=%d loaded=%d parent=%ld human=%ld dropped=%ld", game, second,
			selected, weapon->weapon.magazines[0].rounds_total, weapon->weapon.magazines[0].rounds_loaded,
			weapon->object.parent_object_index, human->unit_index, dropped);
		if (donor != NONE) platform_log("provenance duplicate: game=%ld second=%ld source=%ld source_reserve=%d target_reserve=%d loaded=%d", game,
			second, donor, source ? source->weapon.magazines[0].rounds_total : 0,
			weapon->weapon.magazines[0].rounds_total, weapon->weapon.magazines[0].rounds_loaded);
		if (second >= 30 && network_test.mode == _network_test_host)
		{
			object_iterator_new(&weapons, _object_mask_weapon, 0);
			while (object_iterator_next(&weapons))
			{
				struct weapon_datum *other = weapon_get(weapons.index);
				if (weapons.index != selected && other->object.parent_object_index == NONE &&
					other->object.owner_player_index == NONE && other->definition_index == weapon->definition_index &&
					distance_squared3d(&spawn, &other->object.position) < 1.0f)
					platform_log("provenance respawn: game=%ld second=%ld item=%ld reserve=%d loaded=%d", game, second, weapons.index,
					other->weapon.magazines[0].rounds_total, other->weapon.magazines[0].rounds_loaded);
			}
		}
	}
	if (second >= duplicate_second + 20 && network_test.mode == _network_test_host)
	{
		platform_log("provenance complete: fixture end after native pickups and manual drop");
		game_engine_end_game();
	}
}

/* Opt-in objective regression: place a real player at existing objectives,
   then let pickup, scoring and victory run normally. Never write scores,
   objective state, inventories or tags. The early death uses the ordinary
   damage/death path; it is a fixture death, not evidence of a weapon hit. */
static void network_test_native_objectives(void)
{
	static long previous_game = NONE, previous_second = NONE;
	struct data_iterator iterator;
	struct player_datum *player, *runner = NULL;
	struct scenario *scenario = global_scenario_get();
	long game = network_game_get_number_of_games_played(), second = game_time_get() / TICKS_PER_SECOND;
	long engine = game_engine_get_variant()->game_engine_index, i, count = 0;
	real_point3d destination = {0.0f, 0.0f, 0.0f};
	boolean place = FALSE;
	test_input_combat_actions(0);
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		if (player->network_player_data.machine_index == 1) runner = player;
		if (player->local_player_index != NONE && player->unit_index != NONE &&
			player->network_player_data.machine_index == 1 && second >= 20 &&
			(engine == game_engine_ctf || engine == game_engine_oddball) &&
			!game_engine_player_has_flag(iterator.datum_index)) test_input_combat_actions(16);
	}
	if (game == previous_game && second == previous_second) return;
	previous_game = game; previous_second = second;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		platform_log("native objective: game=%ld second=%ld engine=%ld machine=%d score=%ld won=%ld over=%d infection=%d reserve=%d map=%s",
			game, second, engine, player->network_player_data.machine_index,
			game_engine->get_player_score(iterator.datum_index, _get_score_individual),
			game_engine_did_player_win(iterator.datum_index), !game_engine_can_score(), infection_enabled(),
			game_variant_options_get()->ammo_reserves[2], tag_get_name(global_scenario_index));
		if (network_test.mode == _network_test_host && second == 5 &&
			player->network_player_data.machine_index == 0 && player->unit_index != NONE)
		{
			platform_log("native objective: fixture world death game=%ld", game);
			unit_kill(player->unit_index);
		}
	}
	if (network_test.mode != _network_test_host || !game_engine_can_score() || second < 20 ||
		!runner || runner->unit_index == NONE) return;
	if (engine == game_engine_king && game_engine_get_goal_in_use(0))
	{
		game_engine_get_goal_position(&destination, 0);
		place = TRUE;
	}
	else if (engine == game_engine_race)
	{
		long order, next = NONE, last = NONE, step;
		for (i = 0; i < scenario->netgame_flags.count; i++)
		{
			struct scenario_netgame_flag *flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, i, struct scenario_netgame_flag);
			if (flag->type == _netgame_flag_race_track) count++;
		}
		if (!count) return;
		step = ((second - 20) / 3) % count;
		for (order = 0; order <= step; order++)
		{
			next = NONE;
			for (i = 0; i < scenario->netgame_flags.count; i++)
			{
				struct scenario_netgame_flag *flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, i, struct scenario_netgame_flag);
				if (flag->type == _netgame_flag_race_track && flag->team_index > last &&
					(next == NONE || flag->team_index < next)) { next = flag->team_index; destination = flag->position; }
			}
			last = next;
		}
		place = next != NONE;
	}
	else if (engine == game_engine_ctf && game_engine_player_has_flag(unit_get(runner->unit_index)->unit.player_index))
	{
		for (i = 0; i < scenario->netgame_flags.count; i++)
		{
			struct scenario_netgame_flag *flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, i, struct scenario_netgame_flag);
			if (flag->type == _netgame_flag_ctf_flag && flag->team_index == runner->team_index)
			{ destination = flag->position; place = TRUE; break; }
		}
	}
	else if (engine == game_engine_ctf || engine == game_engine_oddball)
	{
		struct object_iterator objects;
		object_iterator_new(&objects, _object_mask_weapon, 0);
		while (object_iterator_next(&objects))
		{
			struct weapon_datum *weapon = weapon_get(objects.index);
			if (weapon->object.parent_object_index == NONE &&
				((engine == game_engine_ctf && weapon_is_flag(objects.index) && weapon->object.owner_team_index != runner->team_index) ||
				 (engine == game_engine_oddball && weapon->definition_index == get_ball_definition_index())))
			{ destination = weapon->object.position; place = TRUE; break; }
		}
	}
	if (place)
	{
		destination.z += 0.1f;
		object_set_position(runner->unit_index, &destination, NULL, NULL);
	}
}

static void network_test_combat(void)
{
	static long previous_game = NONE, previous_stage = NONE, stage_tick, placed_tick = NONE;
	struct data_iterator iterator;
	struct player_datum *player, *host = NULL, *client = NULL, *attacker, *victim, *zombie = NULL;
	long now = game_time_get(), game = network_game_get_number_of_games_played(), stage;
	boolean infection = infection_enabled();
	if (!strcmp(config_string("debug.test_input"), "look:inventory-objectives") &&
		game_engine_get_variant()->game_engine_index != game_engine_slayer)
	{ network_test_native_objectives(); return; }
	if (!strcmp(config_string("debug.test_input"), "look:inventory-off") ||
		!strcmp(config_string("debug.test_input"), "look:inventory-scope") ||
		!strcmp(config_string("debug.test_input"), "look:inventory-menu-toggle"))
	{
		static long logged_game = NONE;
		test_input_combat_actions(0);
		if (game_engine_can_score() && now >= 10 * TICKS_PER_SECOND && logged_game != game)
		{
			struct object_iterator objects;
			logged_game = game;
			platform_log("ammo map: game=%ld name=%s", game, tag_get_name(global_scenario_index));
			object_iterator_new(&objects, _object_mask_weapon, 0);
			while (object_iterator_next(&objects))
			{
				struct weapon_datum *weapon = weapon_get(objects.index);
				struct weapon_definition *definition = weapon_definition_get(weapon->definition_index);
				if (definition->weapon.magazines.count)
				{
					struct weapon_magazine_definition *magazine = TAG_BLOCK_GET_ELEMENT(
						&definition->weapon.magazines, 0, struct weapon_magazine_definition);
					platform_log("ammo off: game=%ld engine=%ld infection=%d option=%d item=%ld parent=%ld reserve=%d loaded=%d "
						"initial=%d native_cap=%d cap=%d override=%d", game, (long)game_engine_get_variant()->game_engine_index,
						infection, game_variant_options_get()->ammo_reserves[2], objects.index, weapon->object.parent_object_index,
						weapon->weapon.magazines[0].rounds_total, weapon->weapon.magazines[0].rounds_loaded,
						magazine->rounds_total_initial, magazine->rounds_total_maximum,
						game_engine_weapon_reserve_limit(weapon->definition_index, 0, magazine->rounds_total_maximum),
						game_engine_weapon_reserve_limit(weapon->definition_index, 0, NONE));
				}
			}
		}
		if (game_engine_can_score() && now >= 20 * TICKS_PER_SECOND && network_test.mode == _network_test_host)
			game_engine_end_game(); /* fixture end, not a game-rule verdict */
		return;
	}
	if (!strcmp(config_string("debug.test_input"), "look:vehicle")) { network_test_vehicle_combat(); return; }
	if (!strcmp(config_string("debug.test_input"), "look:inventory-provenance")) { network_test_ammo_provenance(); return; }
	if (!strcmp(config_string("debug.test_input"), "look:inventory-resources") && network_test_resource_recovery()) return;
	test_input_combat_actions(0);
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		if (player->quit_out_of_game) continue;
		if (player->network_player_data.machine_index == 0) host = player;
		else if (player->network_player_data.machine_index == 1) client = player;
		if (player->team_index == 1) zombie = player;
	}
	if (!host || !client || !game_engine_can_score() ||
		(infection && infection_state.phase != _distributed_infection_active)) return;
	if (!strcmp(config_string("debug.test_input"), "look:inventory-grenades"))
	{
		struct player_datum *local = host->local_player_index != NONE ? host : client;
		real_vector3d forward = {0.0f, .995f, .1f};
		if (local->unit_index != NONE && (!infection || local->team_index == 0))
		{
			player_control_set_facing(local->local_player_index, &forward);
			if (now >= 18 * TICKS_PER_SECOND && now < 19 * TICKS_PER_SECOND) test_input_combat_actions(64);
		}
		if (now >= 32 * TICKS_PER_SECOND && network_test.mode == _network_test_host) game_engine_end_game();
		return;
	}
	/* Opt-in ammunition consumption: fire into the sky, reload, switch, repeat.
	   No inventory or ammo writes: all actions pass through the controller. */
	if (!strcmp(config_string("debug.test_input"), "look:inventory-ammo") && now < 24 * TICKS_PER_SECOND)
	{
		struct player_datum *local = host->local_player_index != NONE ? host : client;
		long second = now / TICKS_PER_SECOND;
		real_vector3d sky = { 0.0f, .1f, .995f };
		if (local->unit_index != NONE && (!infection || local->team_index == 0))
		{
			player_control_set_facing(local->local_player_index, &sky);
			if ((second >= 6 && second < 9) || (second >= 16 && second < 19)) test_input_combat_actions(1);
			else if ((second >= 9 && second < 13) || (second >= 19 && second < 23)) test_input_combat_actions(16);
			else if (second == 14) test_input_combat_actions(32);
		}
		return;
	}
	if (infection)
	{
		if (!zombie) return;
		stage = zombie->statistics.deaths ? 1 : 0;
		attacker = stage ? zombie : (zombie == host ? client : host);
	}
	else
	{
		stage = !client->statistics.deaths ? 0 : (!host->statistics.deaths ? 1 : 2);
		attacker = stage ? client : host;
	}
	/* Exercise movement/jumps before staged combat. No gather/teleport is
	   called during this phase; normal map collision remains enabled. */
	if (!strcmp(config_string("debug.test_input"), "look:classic") && now < 35 * TICKS_PER_SECOND)
	{
		struct player_datum *local = host->local_player_index != NONE ? host : client;
		if (local->local_player_index != NONE && local->unit_index != NONE)
		{
			real_vector3d direction = { 0.0f, 0.0f, 0.0f };
			long side = (now / (6 * TICKS_PER_SECOND)) % 4;
			if (side == 0) direction.i = 1.0f;
			else if (side == 1) direction.j = 1.0f;
			else if (side == 2) direction.i = -1.0f;
			else direction.j = -1.0f;
			player_control_set_facing(local->local_player_index, &direction);
			test_input_combat_actions(4 | 8);
		}
		return;
	}
	victim = attacker == host ? client : host;
	if (previous_game != game || previous_stage != stage)
	{
		previous_game = game; previous_stage = stage; stage_tick = now; placed_tick = NONE;
		platform_log("combat stage: game=%ld stage=%ld attacker_machine=%d input_only=1", game, stage,
			attacker->network_player_data.machine_index);
	}
	if (host->unit_index == NONE || client->unit_index == NONE ||
		TEST_FLAG(unit_get(host->unit_index)->object.damage_flags, _object_dead_bit) ||
		TEST_FLAG(unit_get(client->unit_index)->object.damage_flags, _object_dead_bit)) return;
	if (placed_tick == NONE || now - placed_tick >= TICKS_PER_SECOND)
	{
		network_test_gather(FALSE);
		placed_tick = now;
	}
	if (stage == 2)
	{
		if (network_test.mode == _network_test_host && now - stage_tick > 10 * TICKS_PER_SECOND)
		{
			platform_log("combat complete: bidirectional Slayer kills and live respawns; ending fixture round");
			game_engine_end_game();
		}
		return;
	}
	if (attacker->local_player_index != NONE && now - stage_tick > 2 * TICKS_PER_SECOND && now > 5 * TICKS_PER_SECOND)
	{
		struct unit_datum *unit = unit_get(attacker->unit_index);
		struct unit_datum *target = unit_get(victim->unit_index);
		real_vector3d direction;
		real_point3d origin = unit->object.bounding_sphere_center;
		real length;
		/* Guns aim from the camera, above the unit's bounding center. A
		   center-to-center vector fires over nearby targets. Melee's cone
		   remains centered on the unit, as the normal melee test expects. */
		if ((!infection || stage == 0) && unit->unit.current_weapon_index != NONE)
			unit_get_camera_position(attacker->unit_index, &origin);
		direction.i = target->object.bounding_sphere_center.x - origin.x;
		direction.j = target->object.bounding_sphere_center.y - origin.y;
		direction.k = target->object.bounding_sphere_center.z - origin.z;
		length = (real)sqrt(direction.i * direction.i + direction.j * direction.j + direction.k * direction.k);
		if (length > 0.01f)
		{
			direction.i /= length; direction.j /= length; direction.k /= length;
			player_control_set_facing(attacker->local_player_index, &direction);
			test_input_combat_actions((infection && stage == 1) || unit->unit.current_weapon_index == NONE ?
				(!strcmp(config_string("debug.test_input"), "look:classic") ? (2 | 4) : 2) : 1);
		}
	}
}

/* Three full engines, native spawn positions and controller movement. Unlike
   the short two-player fixture this never gathers/teleports units or applies
   damage. A blocked walking route is a failed test, not a reason to edit it. */
static void network_test_party(void)
{
	static long last_game = NONE, last_stage = NONE, progress_tick, local_unit = NONE, escape_until, stage_ready;
	static short initial_alpha = NONE;
	static real_point3d progress_position;
	struct player_datum *players[3] = {NULL, NULL, NULL};
	struct data_iterator iterator;
	struct player_datum *player, *attacker, *victim;
	struct unit_datum *unit, *target;
	real_point3d origin;
	real_vector3d direction;
	real length, dx, dy;
	long game = network_game_get_number_of_games_played(), now = game_time_get();
	long stage, cycle, minimum_seconds;
	short i, alpha = NONE, attack, defend, actions;
	boolean infection = infection_enabled(), all_alive = TRUE, escaping = FALSE, melee;
	boolean verdict = !strncmp(config_string("debug.test_input"), "look:party-verdict", 18);
	test_input_combat_actions(0);
	if (!game_engine_can_score()) return;
	if (verdict && strstr(config_string("debug.test_input"), "idle")) return;
	/* A short burst is enough to test blocked friendly damage; keep the
	   finite magazine for the later zombie kill. */
	if (verdict && infection && now >= 12 * TICKS_PER_SECOND && now < 35 * TICKS_PER_SECOND) return;
	if (verdict) network_test_gather(FALSE);
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
		if (!player->quit_out_of_game && player->network_player_data.machine_index >= 0 &&
			player->network_player_data.machine_index < 3)
			players[player->network_player_data.machine_index] = player;
	for (i = 0; i < 3; i++)
	{
		if (!players[i]) return;
		if (players[i]->unit_index == NONE ||
			TEST_FLAG(unit_get(players[i]->unit_index)->object.damage_flags, _object_dead_bit)) all_alive = FALSE;
		if (players[i]->team_index == 1) alpha = i;
	}
	if (!all_alive) return;
	if (infection)
	{
		/* The named fixture pins alpha 0 or 1. The other human first kills
		   it with a gun; after its respawn alpha infects both humans. */
		if (infection_state.phase != _distributed_infection_active) return;
		if (game != last_game) initial_alpha = alpha;
		alpha = initial_alpha;
		if (alpha < 0 || alpha > 1) return;
		stage = !players[alpha]->statistics.deaths ? 0 : (players[1-alpha]->team_index == 0 ? 1 : 2);
		attack = stage ? alpha : 1-alpha;
		defend = stage == 2 ? 2 : (stage ? 1-alpha : alpha);
		if (verdict)
		{
			/* Attempt the selected friendly-fire rule, then kill the zombie,
			   respawn it and infect the remaining humans. */
			stage = now < 35 * TICKS_PER_SECOND ? 0 : (!players[alpha]->statistics.deaths ? 1 :
				(players[1-alpha]->team_index == 0 ? 2 : 3));
			attack = stage >= 2 ? alpha : 1-alpha;
			defend = !stage || stage == 3 ? 2 : (stage == 1 ? alpha : 1-alpha);
		}
	}
	else
	{
		cycle = MIN(players[0]->statistics.deaths, MIN(players[1]->statistics.deaths, players[2]->statistics.deaths));
		stage = players[1]->statistics.deaths == cycle ? 0 : (players[2]->statistics.deaths == cycle ? 1 : 2);
		attack = (short)stage; defend = (attack + 1) % 3;
		minimum_seconds = !strcmp(config_string("debug.test_input"), "look:party-soak") ? 300 : 30;
		if (cycle && now >= minimum_seconds * TICKS_PER_SECOND)
		{
			if (network_test.mode == _network_test_host)
			{
				platform_log("party complete: game=%ld three live respawns, native controller combat", game);
				game_engine_end_game();
			}
			return;
		}
		stage += cycle * 3;
		if (verdict && game_engine_get_variant()->universal_variant.teams)
		{
			short ally = NONE, enemy = NONE;
			attack = players[0]->team_index == players[1]->team_index ? 0 :
				(players[0]->team_index == players[2]->team_index ? 0 : 1);
			for (i = 0; i < 3; i++) if (i != attack)
			{
				if (players[i]->team_index == players[attack]->team_index) ally = i; else enemy = i;
			}
			if (ally == NONE || enemy == NONE) return;
			defend = !players[ally]->statistics.deaths ? ally : enemy;
			stage = players[ally]->statistics.deaths ? 1 + players[enemy]->statistics.deaths : 0;
		}
	}
	melee = infection && (verdict ? stage >= 2 : stage != 0);
	if (game != last_game || stage != last_stage)
	{
		last_game = game; last_stage = stage; local_unit = NONE;
		/* Leave live respawns observable before the next real controller hit. */
		stage_ready = now + 3 * TICKS_PER_SECOND;
		platform_log("party stage: game=%ld stage=%ld attacker=%d victim=%d", game, stage, attack, defend);
	}
	if (verdict && now < stage_ready) return;
	attacker = players[attack]; victim = players[defend];
	if (attacker->local_player_index == NONE || now < 6 * TICKS_PER_SECOND) return;
	unit = unit_get(attacker->unit_index); target = unit_get(victim->unit_index);
	origin = unit->object.bounding_sphere_center;
	if (!melee) unit_get_camera_position(attacker->unit_index, &origin);
	direction.i = target->object.bounding_sphere_center.x - origin.x;
	direction.j = target->object.bounding_sphere_center.y - origin.y;
	direction.k = target->object.bounding_sphere_center.z - origin.z;
	length = (real)sqrt(direction.i*direction.i + direction.j*direction.j + direction.k*direction.k);
	if (length < 0.01f) return;
	direction.i /= length; direction.j /= length; direction.k /= length;
	dx = unit->object.position.x - progress_position.x;
	dy = unit->object.position.y - progress_position.y;
	if (local_unit != attacker->unit_index || dx*dx + dy*dy > 0.25f)
	{
		if (local_unit != attacker->unit_index) escape_until = 0;
		local_unit = attacker->unit_index; progress_position = unit->object.position; progress_tick = now;
	}
	if (length > 2.0f && now - progress_tick > 3 * TICKS_PER_SECOND)
		escape_until = now + 10 * TICKS_PER_SECOND;
	if (now < escape_until && !network_test_combat_clear(&unit->object.position, &target->object.position))
	{
		/* Keep one direction until the route clears instead of oscillating
		   against a wall whenever a small sideways step makes progress. */
		dx = direction.i; direction.i = -direction.j; direction.j = dx; direction.k = 0;
		if ((game + attack) & 1) { direction.i = -direction.i; direction.j = -direction.j; }
		escaping = TRUE;
	}
	else escape_until = 0;
	player_control_set_facing(attacker->local_player_index, &direction);
	/* A clear sight line can still cross a low ledge that blocks walking. */
	actions = length > (melee ? 0.65f : 2.0f) ? 4 | 8 : 0;
	if (escaping) actions |= 8;
	if (!escaping && length < (melee ? 1.1f : 4.0f)) actions |= melee ? 2 : 1;
	test_input_combat_actions(actions);
}

/* Observe native spawn decisions without writing any inventory or map data. */
void network_test_spawn_inventory_log(long player_index, char const *stage, long flags)
{
	struct player_datum *player;
	struct unit_datum *unit;
	struct scenario *scenario;
	struct game_variant const *variant;
	long start, matching = NONE, equipment_flags = 0;
	if (strcmp(config_string("debug.test_input"), "look:inventory-grenades")) return;
	player = player_get(player_index);
	unit = player->unit_index == NONE ? NULL : unit_try_and_get(player->unit_index);
	if (!unit) return;
	scenario = global_scenario_get();
	variant = game_engine_get_variant();
	for (start = 0; start < scenario->scenario_starting_equipment.count; start++)
	{
		struct scenario_starting_equipment *equipment = TAG_BLOCK_GET_ELEMENT(
			&scenario->scenario_starting_equipment, start, struct scenario_starting_equipment);
		if (match_game_type(game_engine ? game_engine->type : NONE, 4, equipment->game_type))
		{
			matching = start; equipment_flags = equipment->flags; break;
		}
	}
	platform_log("grenade spawn: stage=%s game=%ld player=%ld machine=%d unit=%ld team=%ld infection=%d "
		"weapon_set=%ld variant_flags=%ld population_flags=%ld equipment=%ld equipment_flags=%ld frag=%d plasma=%d client=%d",
		stage, network_game_get_number_of_games_played(), player_index, player->network_player_data.machine_index,
		player->unit_index, player->team_index, infection_enabled(), variant->universal_variant.weapon_set,
		variant->universal_variant.flags, flags, matching, equipment_flags,
		unit->unit.grenade_counts[0], unit->unit.grenade_counts[1], network_game_distributed_client());
}

static void network_test_combat_log(void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	long hits_sent, hits_dealt, hits_rejected, hits_replayed;
	struct scenario *scenario = global_scenario_get();
	long start, matching_starts = 0;
	network_damage_statistics(&hits_sent, &hits_dealt, &hits_rejected, &hits_replayed);
	for (start = 0; start < scenario->players.count; start++)
		if (match_game_type(game_engine ? game_engine->type : NONE, 4,
			TAG_BLOCK_GET_ELEMENT(&scenario->players, start, struct player_starting_location)->game_types))
			matching_starts++;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		struct unit_datum *unit = player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
		struct observer_result const *camera = player->local_player_index != NONE ? observer_get_camera(player->local_player_index) : NULL;
		long weapon = unit ? unit_inventory_get_weapon(player->unit_index, unit->unit.current_weapon_index) : NONE;
		long count = 0, slot, ammo = 0, spawn_candidates = -1;
		long inventory[MAXIMUM_WEAPONS_PER_UNIT];
		real distance = -1.0f;
		for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		{
			long item = unit ? unit->unit.weapon_object_indices[slot] : NONE;
			struct object_datum *object = object_try_and_get_and_verify_type(item, _object_mask_weapon);
			inventory[slot] = object ? object->definition_index : NONE;
			if (object && !strcmp(config_string("debug.test_input"), "look:inventory-ammo") &&
				weapon_definition_get(object->definition_index)->weapon.magazines.count)
			{
				struct weapon_magazine *mag = &weapon_get(item)->weapon.magazines[0];
				struct weapon_magazine_definition *def = TAG_BLOCK_GET_ELEMENT(
					&weapon_definition_get(object->definition_index)->weapon.magazines, 0, struct weapon_magazine_definition);
				platform_log("ammo audit: {\"game\":%ld,\"tick\":%ld,\"machine\":%d,\"team\":%ld,\"slot\":%ld,"
					"\"item\":%ld,\"reserve\":%d,\"loaded\":%d,\"state\":%d,\"initial\":%d,\"clip\":%d,\"maximum\":%d,\"type\":%ld}",
					network_game_get_number_of_games_played(), game_time_get(), player->network_player_data.machine_index,
					player->team_index, slot, item, mag->rounds_total, mag->rounds_loaded, mag->state,
					def->rounds_total_initial, def->rounds_loaded_maximum, def->rounds_total_maximum,
					weapon_definition_index_to_list_index(object->definition_index));
			}
			if (object && game_time_get() <= 5 * TICKS_PER_SECOND &&
				!strncmp(config_string("debug.test_input"), "look:inventory", 14))
				platform_log("inventory tag: game=%ld machine=%d slot=%ld definition=%ld name=%s",
					network_game_get_number_of_games_played(), player->network_player_data.machine_index,
					slot, object->definition_index, tag_get_name(object->definition_index));
		}
		if (unit)
		{
			for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++) if (unit->unit.weapon_object_indices[slot] != NONE) count++;
			if (camera)
			{
				real x = camera->position.x - unit->object.position.x, y = camera->position.y - unit->object.position.y,
					z = camera->position.z - unit->object.position.z;
				distance = (real)sqrt(x*x+y*y+z*z);
			}
		}
		if (weapon != NONE) ammo = weapon_get(weapon)->weapon.magazines[0].rounds_total + weapon_get(weapon)->weapon.magazines[0].rounds_loaded;
		if (!unit && (!strncmp(config_string("debug.test_input"), "look:population", 15) ||
			!strncmp(config_string("debug.test_input"), "bot:", 4)))
		{
			struct scenario *scenario = global_scenario_get();
			spawn_candidates = 0;
			/* Pure ratings only: do not call the randomized spawn selector. */
			for (slot = 0; slot < scenario->players.count; slot++)
				if (game_engine_get_starting_location_rating(iterator.datum_index,
					TAG_BLOCK_GET_ELEMENT(&scenario->players, slot, struct player_starting_location)) > 0.0f)
					spawn_candidates++;
		}
		platform_log("combat sample: {\"game\":%ld,\"tick\":%ld,\"player\":%ld,\"machine\":%d,\"local\":%d,\"unit\":%ld,"
			"\"alive\":%d,\"attached\":%d,\"body\":%.3f,\"shield\":%.3f,\"team\":%ld,\"kills\":%d,\"deaths\":%d,\"score\":%ld,"
			"\"weapon\":%ld,\"weapons\":%ld,\"skull\":%d,\"ammo\":%ld,\"controls\":%ld,\"respawn\":%ld,\"ping\":%ld,"
			"\"camera_cluster\":%d,\"camera_distance\":%.3f,\"bsp\":%d,\"over\":%d,\"phase\":%ld,\"winner\":%ld,"
			"\"position\":[%.3f,%.3f,%.3f],\"camera\":[%.3f,%.3f,%.3f],\"color\":[%.3f,%.3f,%.3f],"
			"\"base_color\":[%.3f,%.3f,%.3f],"
			"\"hits_sent\":%ld,\"hits_dealt\":%ld,\"hits_rejected\":%ld,\"hits_replayed\":%ld,\"spawn_candidates\":%ld,"
			"\"quit\":%d,\"player_datum\":%ld,\"matching_starts\":%ld,\"speed\":%.3f,\"max_shield\":%.3f,\"biped_flags\":%ld,"
			"\"inventory\":[%ld,%ld,%ld,%ld],\"grenades\":[%d,%d],\"throw_state\":%d,\"grenade_index\":%d}",
			network_game_get_number_of_games_played(), game_time_get(), (long)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index),
			player->network_player_data.machine_index, player->local_player_index != NONE, player->unit_index,
			unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit), unit && unit->unit.player_index == iterator.datum_index,
			unit ? unit->object.body_vitality : 0.0f, unit ? unit->object.shield_vitality : 0.0f, (long)player->team_index,
			player->statistics.kills[0], player->statistics.deaths,
			game_engine && game_engine->get_player_score ? game_engine->get_player_score(iterator.datum_index, _get_score_individual) : -1L,
			weapon != NONE ? object_get(weapon)->definition_index : NONE, count,
			weapon != NONE && object_get(weapon)->definition_index == get_ball_definition_index(), ammo,
			unit ? (long)unit->unit.control_flags : 0L, (long)player->respawn_timer,
			distributed_player_ping((short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index)),
			camera ? camera->location.cluster_index : -1, distance, global_structure_bsp_index_get(), !game_engine_can_score(),
			infection_enabled() ? infection_state.phase : -1L, infection_enabled() ? infection_state.winner : -1L,
			unit ? unit->object.position.x : 0.0f, unit ? unit->object.position.y : 0.0f, unit ? unit->object.position.z : 0.0f,
			camera ? camera->position.x : 0.0f, camera ? camera->position.y : 0.0f, camera ? camera->position.z : 0.0f,
			unit ? unit->object.outgoing_change_colors[0].red : 0.0f,
			unit ? unit->object.outgoing_change_colors[0].green : 0.0f,
			unit ? unit->object.outgoing_change_colors[0].blue : 0.0f,
			unit ? unit->object.base_change_colors[0].red : 0.0f,
			unit ? unit->object.base_change_colors[0].green : 0.0f,
			unit ? unit->object.base_change_colors[0].blue : 0.0f,
			hits_sent, hits_dealt, hits_rejected, hits_replayed, spawn_candidates,
			player->quit_out_of_game, iterator.datum_index, matching_starts,
			player->speed_multiplier, unit ? unit->object.maximum_shield_vitality : 0.0f,
			unit && unit->object.type == _object_type_biped ? (long)((struct biped_datum *)unit)->biped.flags : 0L,
			inventory[0], inventory[1], inventory[2], inventory[3],
			unit ? unit->unit.grenade_counts[0] : 0, unit ? unit->unit.grenade_counts[1] : 0,
			unit ? unit->unit.grenade_throw_state : 0, unit ? unit->unit.current_grenade_index : NONE);
	}
}

/* Controlled environmental deaths for population lifecycle diagnostics only.
   Normal host death/respawn paths; no placement, damage-stat or inventory edits. */
static void network_test_population_death(void)
{
	static long last_tick = NONE;
	struct data_iterator iterator;
	struct player_datum *player;
	long now = game_time_get();
	if (network_game_distributed_client() || now <= 0 || now == last_tick ||
		now % (30 * TICKS_PER_SECOND) || !game_engine_can_score()) return;
	last_tick = now;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
		if (player->network_player_data.machine_index == (now / (30 * TICKS_PER_SECOND)) % 2 &&
			player->unit_index != NONE && !player->quit_out_of_game &&
			!TEST_FLAG(unit_get(player->unit_index)->object.damage_flags, _object_dead_bit))
		{
			unit_kill(player->unit_index);
			platform_log("population test: environmental death machine=%d tick=%ld",
				player->network_player_data.machine_index, now);
		}
}

/* Reproduce an alpha conversion between grenade wind-up and hand creation.
   Begin through the normal grenade API, without editing counts/animations. */
static void network_test_grenade_conversion(void)
{
	static long attempted_game = NONE;
	long game = network_game_get_number_of_games_played();
	long chosen, left;
	struct player_datum *player;
	if (network_test.mode != _network_test_host || !infection_enabled() ||
		infection_state.phase != _distributed_infection_countdown || attempted_game == game) return;
	left = infection_state.phase_tick + game_variant_options_get()->infection_countdown_seconds * TICKS_PER_SECOND - game_time_get();
	if (left > 1 || left < 0) return;
	chosen = network_test_infection_initial_player();
	player = chosen != NONE ? player_try_and_get(chosen) : NULL;
	if (!player || player->unit_index == NONE) return;
	attempted_game = game;
	platform_log("grenade conversion probe: tick=%ld remaining=%ld player=%ld began=%d",
		game_time_get(), left, chosen, unit_throw_grenade_begin(player->unit_index, NULL));
}

static void network_test_vehicle(
	boolean enter)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *last = NULL;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		last = player;
	if (!last || last->unit_index == NONE)
		return;
	if (!enter)
	{
		if (object_get(last->unit_index)->object.parent_object_index != NONE)
		{
			unit_exit_seat_end(last->unit_index);
			platform_log("network test: the last player leaves the vehicle");
		}
		return;
	}
	{
		struct object_iterator vehicles;
		long nearest_index = NONE;
		real nearest_distance = 0.0f;
		real_point3d const *position = &object_get(last->unit_index)->object.position;
		short seat_index;

		object_iterator_new(&vehicles, _object_mask_vehicle, 0);
		while (object_iterator_next(&vehicles))
		{
			real_point3d const *vehicle_position = &object_get(vehicles.index)->object.position;
			real distance = distance_squared3d(position, vehicle_position);

			if (nearest_index == NONE || distance < nearest_distance)
			{
				nearest_index = vehicles.index;
				nearest_distance = distance;
			}
		}
		if (nearest_index == NONE)
		{
			platform_log("network test: no vehicle");
			return;
		}
		for (seat_index = 0; seat_index < unit_definition_get(object_get(nearest_index)->definition_index)->unit.seats.count; seat_index++)
		{
			if (unit_seat_is_driver(nearest_index, seat_index) &&
				unit_enter_seat(last->unit_index, nearest_index, seat_index))
			{
				platform_log("network test: the last player drives vehicle %lx", nearest_index);
				return;
			}
		}
		platform_log("network test: the last player cannot drive vehicle %lx", nearest_index);
	}
}

/* both machines stand the last player at the driver's entrance of the
nearest vehicle no one rides (a joining machine's player then holds the
action button, as a player getting in does) */
static void network_test_vehicle_approach(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *last = NULL;
	struct object_iterator vehicles;
	long nearest_index = NONE;
	real nearest_distance = 0.0f;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		last = player;
	if (!last || last->unit_index == NONE || object_get(last->unit_index)->object.parent_object_index != NONE)
		return;
	object_iterator_new(&vehicles, _object_mask_vehicle, 0);
	while (object_iterator_next(&vehicles))
	{
		struct unit_datum *vehicle = unit_get(vehicles.index);
		/* (the same vehicle on every machine: the lowest index) */
		real distance = (real)DATUM_INDEX_TO_ABSOLUTE_INDEX(vehicles.index);

		if (vehicle->unit.driver_object_index != NONE || TEST_FLAG(vehicle->object.damage_flags, _object_dead_bit))
			continue;
		if (nearest_index == NONE || distance < nearest_distance)
		{
			nearest_index = vehicles.index;
			nearest_distance = distance;
		}
	}
	if (nearest_index != NONE)
	{
		short seat_index;

		for (seat_index = 0; seat_index < unit_definition_get(object_get(nearest_index)->definition_index)->unit.seats.count;
			seat_index++)
		{
			real_point3d entrance;
			real_point3d seat;

			if (unit_seat_is_driver(nearest_index, seat_index) &&
				unit_get_seat_entrance_point(last->unit_index, nearest_index, seat_index, &entrance, &seat, NULL))
			{
				object_set_position(last->unit_index, &entrance, NULL, NULL);
				platform_log("network test: the last player stands at vehicle %lx's driver's entrance (%.2f %.2f %.2f)",
					nearest_index, entrance.x, entrance.y, entrance.z);
				return;
			}
		}
	}
	platform_log("network test: no vehicle to get into");
}

/* the host gives the last player a second weapon, lying about */
static void network_test_second_weapon(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *last = NULL;
	struct object_iterator weapons;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		last = player;
	if (!last || last->unit_index == NONE)
		return;
	object_iterator_new(&weapons, _object_mask_weapon, 0);
	while (object_iterator_next(&weapons))
	{
		struct object_datum *weapon = object_get(weapons.index);
		struct unit_datum *unit = unit_get(last->unit_index);
		long current_index = unit->unit.weapon_object_indices[0];

		if (weapon->object.parent_object_index == NONE && TEST_FLAG(weapon->object.flags, _object_connected_to_map_bit) &&
			(current_index == NONE || object_get(current_index)->definition_index != weapon->definition_index) &&
			unit_add_weapon_to_inventory(last->unit_index, weapons.index, TRUE))
		{
			platform_log("network test: the last player takes a second weapon (%lx)", weapon->definition_index);
			/* (and camouflage, as a powerup gives) */
			player_handle_powerup(DATUM_INDEX_NEW(last - (struct player_datum *)player_data->data, last->identifier),
				_player_powerup_active_camouflage, 10 * TICKS_PER_SECOND);
			return;
		}
	}
}

/* both machines stand the last player on the first weapon lying about that
it does not carry (the same object on both: a client's own player is where
it has it, within a tolerance) */
static void network_test_pickup(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *last = NULL;
	struct object_iterator weapons;
	long nearest_index = NONE;
	real nearest_distance = 0.0f;
	struct unit_datum *unit;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		last = player;
	if (!last || last->unit_index == NONE)
		return;
	unit = unit_get(last->unit_index);
	object_iterator_new(&weapons, _object_mask_weapon, 0);
	while (object_iterator_next(&weapons))
	{
		struct object_datum *weapon = object_get(weapons.index);
		short slot;
		boolean carried = FALSE;
		real distance;

		if (weapon->object.parent_object_index != NONE ||
			!TEST_FLAG(weapon->object.flags, _object_connected_to_map_bit))
		{
			continue;
		}
		for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
		{
			long carried_index = unit->unit.weapon_object_indices[slot];

			carried |= carried_index != NONE && object_get(carried_index)->definition_index == weapon->definition_index;
		}
		if (network_test.pickup_weapon[0] && !strstr(tag_get_name(weapon->definition_index), network_test.pickup_weapon))
			continue;
		distance = (real)DATUM_INDEX_TO_ABSOLUTE_INDEX(weapons.index);
		if (!carried && (nearest_index == NONE || distance < nearest_distance))
		{
			nearest_index = weapons.index;
			nearest_distance = distance;
		}
	}
	if (nearest_index != NONE)
	{
		real_point3d position = object_get(nearest_index)->object.position;

		position.z += 0.1f;
		object_set_position(last->unit_index, &position, NULL, NULL);
		platform_log("network test: the last player stands on weapon %lx (%lx)", nearest_index,
			object_get(nearest_index)->definition_index);
	}
}

void network_test_update(
	boolean main_menu_loaded,
	real seconds)
{
	if (!network_test.checked)
		network_test_read_settings();
	if (network_test.mode == _network_test_off)
		return;
	if (game_in_progress() && !main_menu_loaded && !strcmp(config_string("debug.test_input"), "look:grenade"))
		network_test_grenade_conversion();
	if (game_in_progress() && !main_menu_loaded && !strcmp(config_string("debug.test_input"), "look:population-death"))
		network_test_population_death();
	/* Exercise the ordinary scoreboard input during rendered regressions. */
	test_input_scoreboard(game_in_progress() && !main_menu_loaded && game_engine_can_score() &&
		(game_time_get() / TICKS_PER_SECOND) % 8 < 3);
	/* Finish the opt-in Oddball fixture after pickup, scoring and voluntary
	   drop have been observed, so the native Carnage Report can be captured.
	   This ends the fixture without awarding points or injecting damage. */
	if (network_test.mode == _network_test_host && game_in_progress() && !main_menu_loaded &&
		!strcmp(config_string("debug.test_input"), "look:objective") &&
		game_engine_get_variant()->game_engine_index == game_engine_oddball && game_engine_can_score() &&
		game_time_get() >= (network_test.pickup_time + 20.0f) * TICKS_PER_SECOND)
	{
		platform_log("objective complete: ending fixture after pickup and drop; scores unchanged");
		game_engine_end_game();
	}
	if (network_test_combat_input())
	{
		if (game_in_progress() && !main_menu_loaded) network_test_combat();
		else test_input_combat_actions(0);
	}
	if (!strncmp(config_string("debug.test_input"), "look:party", 10))
	{
		if (game_in_progress() && !main_menu_loaded) network_test_party();
		else test_input_combat_actions(0);
	}
	{
		long local = game_in_progress() && !main_menu_loaded ? local_player_get_player_index(0) : NONE;
		struct player_datum *player = local != NONE ? player_try_and_get(local) : NULL;
		boolean active = FALSE;
		if (player && player->unit_index != NONE)
		{
			active = !strncmp(config_string("debug.test_input"), "look:drop", 9) &&
				infection_unit_is_zombie(player->unit_index) && infection_state.phase == _distributed_infection_active;
			if (!strcmp(config_string("debug.test_input"), "look:objective") &&
				game_engine_get_variant()->game_engine_index == game_engine_oddball &&
				game_time_get() >= (network_test.pickup_time + 5.0f) * TICKS_PER_SECOND)
			{
				long weapon = unit_inventory_get_weapon(player->unit_index, unit_get(player->unit_index)->unit.current_weapon_index);
				active = weapon != NONE && object_get(weapon)->definition_index == get_ball_definition_index();
			}
		}
		test_input_objective_actions(active);
	}

	/* the game running: report (from the start of each game: the next
	game's time starts over) */
	if (game_in_progress() && game_time_get() < network_test.logged_time)
		network_test.logged_time = 0;
	if (game_in_progress() && !main_menu_loaded && game_time_get() - network_test.logged_time >= TICKS_PER_SECOND)
	{
		network_test.logged_time = game_time_get();
		if (network_test_combat_input() ||
			!strncmp(config_string("debug.test_input"), "look:party", 10) ||
			!strncmp(config_string("debug.test_input"), "look:population", 15) ||
			!strncmp(config_string("debug.test_input"), "bot:", 4) ||
			!strcmp(config_string("debug.test_input"), "look:grenade")) network_test_combat_log();
		infection_log_state();
		network_test_log_players();
		/* Probe the normal pickup/readying gates against a real zombie skull.
		   No fabricated damage or inventory is injected into the human. */
		if (network_test.mode == _network_test_host && infection_enabled() &&
			infection_state.phase == _distributed_infection_active &&
			!strncmp(config_string("debug.test_input"), "look:drop", 9))
		{
			struct data_iterator iterator;
			struct player_datum *player;
			long human = NONE, skull = NONE;
			data_iterator_new(&iterator, player_data);
			while ((player = (struct player_datum *)data_iterator_next(&iterator)))
			{
				if (player->unit_index == NONE) continue;
				if (player->team_index == 0) human = player->unit_index;
				else if (infection_unit_is_zombie(player->unit_index))
					skull = unit_inventory_get_weapon(player->unit_index, unit_get(player->unit_index)->unit.current_weapon_index);
			}
			if (human != NONE && skull != NONE && object_get(skull)->definition_index == get_ball_definition_index())
				platform_log("infection human skull probe: pickup=%d ready=%d", game_engine_allow_pick_up(human, skull),
					unit_can_use_weapon(human, skull));
		}
		if (network_test.shoot_interval > 0.0f &&
			game_time_get() % MAX(1, (long)(network_test.shoot_interval * TICKS_PER_SECOND)) < TICKS_PER_SECOND)
		{
			network_test_shoot();
		}
		/* (the host brings the players near a second before they shoot;
		not the last while the vehicle test has it) */
		if ((network_test.mode == _network_test_host ||
			(infection_enabled() && !strncmp(config_string("debug.test_input"), "look:drop", 9))) &&
			network_test.shoot_interval > 0.0f &&
			(game_time_get() + TICKS_PER_SECOND) % MAX(1, (long)(network_test.shoot_interval * TICKS_PER_SECOND)) <
				TICKS_PER_SECOND)
		{
			long vehicle_time = (long)(network_test.vehicle_time * TICKS_PER_SECOND);

			network_test_gather(network_test.vehicle_time > 0.0f && game_time_get() >= vehicle_time - 2 * TICKS_PER_SECOND);
		}
		if (network_test.pickup_time > 0.0f)
		{
			long pickup_time = (long)(network_test.pickup_time * TICKS_PER_SECOND);

			/* (two weapons first: picking up a third swaps) */
			if (network_test.mode == _network_test_host && game_time_get() >= pickup_time - 3 * TICKS_PER_SECOND &&
				game_time_get() - (pickup_time - 3 * TICKS_PER_SECOND) < TICKS_PER_SECOND)
			{
				network_test_second_weapon();
			}

			if (game_time_get() >= pickup_time && game_time_get() - pickup_time < TICKS_PER_SECOND)
			{
				network_test_pickup();
			}
			/* (standing there for four seconds, the button held from a second
			on) */
			if (network_test.mode == _network_test_join)
			{
				boolean hold = game_time_get() >= pickup_time &&
					game_time_get() < pickup_time + 4 * TICKS_PER_SECOND;

				test_input_hold_action(hold);
			}
		}
		if (network_test.mode == _network_test_host && network_test.vehicle_time > 0.0f)
		{
			long enter_time = (long)(network_test.vehicle_time * TICKS_PER_SECOND);

			if (game_time_get() >= enter_time && game_time_get() - enter_time < TICKS_PER_SECOND)
				network_test_vehicle(TRUE);
			if (game_time_get() >= enter_time + 15 * TICKS_PER_SECOND &&
				game_time_get() - enter_time - 15 * TICKS_PER_SECOND < TICKS_PER_SECOND)
			{
				network_test_vehicle(FALSE);
			}
		}
		/* ... and 20 seconds on, the last player getting in as a player does:
		at the entrance, the button held for three seconds */
		if (network_test.vehicle_time > 0.0f)
		{
			long approach_time = (long)(network_test.vehicle_time * TICKS_PER_SECOND) + 20 * TICKS_PER_SECOND;

			if (game_time_get() >= approach_time && game_time_get() - approach_time < TICKS_PER_SECOND)
				network_test_vehicle_approach();
			if (network_test.mode == _network_test_join && network_test.pickup_time <= 0.0f)
			{
				test_input_hold_action(game_time_get() >= approach_time + TICKS_PER_SECOND &&
					game_time_get() < approach_time + 4 * TICKS_PER_SECOND);
			}
		}
		/* debug.network_test_kill: the host kills the last player every so
		often, to test deaths and respawns reaching the clients */
		if (network_test.mode == _network_test_host && network_test.kill_interval > 0.0f &&
			game_time_get() % MAX(1, (long)(network_test.kill_interval * TICKS_PER_SECOND)) < TICKS_PER_SECOND)
		{
			if (infection_enabled())
				network_test_infection_kill();
			else
			{
			struct data_iterator iterator;
			struct player_datum *player;
			struct player_datum *last = NULL;

			struct player_datum *first = NULL;
			long first_index = NONE;

			data_iterator_new(&iterator, player_data);
			while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
			{
				if (!first)
				{
					first = player;
					first_index = iterator.datum_index;
				}
				last = player;
			}
			if (last && last != first && last->unit_index != NONE && first->unit_index != NONE)
			{
				/* killed by the first player: a kill that scores */
				platform_log("network test: the first player kills the last");
				damage_kill_object_for_player(last->unit_index, first_index);
				/* and picks up a weapon lying about, and two grenades of each kind */
				{
					struct object_iterator objects;
					struct unit_datum *unit = unit_get(first->unit_index);
					long held_index = unit->unit.weapon_object_indices[0];
					long held_definition_index = held_index != NONE ? weapon_get(held_index)->definition_index : NONE;

					object_iterator_new(&objects, _object_mask_weapon, 0);
					while (object_iterator_next(&objects))
					{
						struct weapon_datum *weapon = weapon_get(objects.index);

						if (weapon->object.parent_object_index == NONE &&
							weapon->definition_index != held_definition_index)
						{
							if (unit_add_weapon_to_inventory(first->unit_index, objects.index, TRUE))
								platform_log("network test: the first player picks up a weapon");
							break;
						}
					}
					unit->unit.grenade_counts[0] = 2;
					unit->unit.grenade_counts[1] = 2;
				}
			}
			}
		}
	}

	/* the next game of the list: once the scores have been shown a while,
	the host's button (the bots may press it first) */
	if (network_test.mode == _network_test_host && network_test.started && game_engine_running() &&
		!main_menu_loaded && !game_engine_can_score())
	{
		network_test.game_over = TRUE;
		if (game_engine_showing_postgame() && global_network_game_server_get())
		{
			network_test.postgame_seconds += seconds;
			if (network_test.postgame_seconds >= 3.0f && network_test_variant(network_test.variant_index + 1))
			{
				network_test.postgame_seconds = 0.0f;
				network_game_server_reset_to_pregame(global_network_game_server_get());
			}
		}
	}
	/* (a joining machine's player: the other team again in the next game's
	lobby, whose variant may have teams where the last had none) */
	if (network_test.mode == _network_test_join && network_test.team_set && game_engine_running() &&
		!main_menu_loaded && !game_engine_can_score())
	{
		network_test.game_over = TRUE;
	}
	if (network_test.mode == _network_test_join && network_test.game_over && main_menu_loaded)
	{
		network_test.game_over = FALSE;
		network_test.team_set = FALSE;
		network_test.joined_seconds = 0.0f;
	}
	/* ... back in the lobby, set up as the first was */
	if (network_test.mode == _network_test_host && network_test.game_over && main_menu_loaded)
	{
		network_test.game_over = FALSE;
		network_test.postgame_seconds = 0.0f;
		if (network_test_variant(network_test.variant_index + 1))
		{
			network_test.variant_index++;
			network_test.started = FALSE;
			network_test.map_set = FALSE;
			network_test.setup_seconds = 0.0f;
			network_test.menu_seconds = 0.0f;
			/* (as picking the next game's map does: the scores' map choice
			holds the countdown) */
			if (global_network_game_server_get())
				network_game_server_pause_countdown(global_network_game_server_get(), FALSE);
			platform_log("network test: the next game");
		}
	}

	if (!main_menu_loaded)
		return;
	network_test.menu_seconds += seconds;
	/* (the main menu settling first) */
	if (network_test.menu_seconds < 2.0f)
		return;

	switch (network_test.mode)
	{
	case _network_test_browse:
	{
		struct p2p_listing listed[8], games[8];
		int listed_count, count = 0, pass, index;

		if (!network_test.browsing)
		{
			network_test.browsing = TRUE;
			p2p_lobby_browse(TRUE);
			platform_log("network test: browsing the public games");
		}
		/* (listings arriving meanwhile from every broker) */
		network_test.browse_seconds += seconds;
		listed_count = p2p_lobby_games(listed, NUMBEROF(listed));
		if (!listed_count || network_test.browse_seconds < 5.0f)
			break;
		/* those that can be joined (no password, not failed): the list has
		the most players first; co-op, the game with no game engine, last */
		for (pass = 0; pass < 2; pass++)
		{
			for (index = 0; index < listed_count; index++)
			{
				if (listed[index].invite[0] && !listed[index].failed && !listed[index].engine_type == pass)
					games[count++] = listed[index];
			}
		}
		if (!network_test.browse_listed)
		{
			network_test.browse_listed = TRUE;
			for (index = 0; index < listed_count; index++)
			{
				platform_log("network test: listed %s's game (%s, %s) %d/%d players%s%s", listed[index].name,
					listed[index].map, listed[index].gametype, listed[index].player_count,
					listed[index].maximum_player_count, listed[index].engine_type ? "" : ", co-op",
					listed[index].invite[0] ? "" : ", locked");
			}
		}
		if (count > 0 && p2p_join_invite(games[0].invite))
		{
			p2p_lobby_browse(FALSE);
			platform_log("network test: joining %s's game (%s) from the server browser", games[0].name, games[0].map);
			network_test.mode = _network_test_join;
		}
		break;
	}
	case _network_test_host:
		if (!network_test.set_up)
		{
			network_test.set_up = TRUE;
			main_set_multiplayer_map_name(network_test.map_name);
			player_ui_fast_setup_network_server();
			platform_log("network test: hosting %s", network_test.map_name);
			/* (in the server browser, as Create Game > Internet's PUBLIC) */
			if (network_test.public_game)
				p2p_set_hosting_public(TRUE);
		}
		else if (!network_test.started)
		{
			network_test.setup_seconds += seconds;
			/* the map (fast setup clears it), and a player for controller 1, as
			pressing A in the lobby adds one */
			if (!network_test.map_set && network_test.setup_seconds >= 1.0f && global_network_game_server_get())
			{
				char path[sizeof(network_test.map_name)];
				char const *variant_name = network_test_variant(network_test.variant_index);
				struct game_variant variant;
				extern boolean menu_test_infection_toggle(boolean enabled, boolean set_reserves);

				if (!strcmp(config_string("debug.test_input"), "look:inventory-menu-toggle") && network_test.variant_index)
				{
					boolean saved;
					/* Postgame rotates to Carousel by default; select Prisoner
					   again without replacing the edited variant or its options. */
					snprintf(path, sizeof(path), "levels\\test\\%s\\%s", network_test.map_name, network_test.map_name);
					network_game_server_change_map_name(global_network_game_server_get(), path);
					saved = menu_test_infection_toggle(!(network_test.variant_index & 1), FALSE);
					platform_log("menu toggle: game=%lu applied=%d same_variant=1", (unsigned long)network_test.variant_index, saved);
					if (saved && player_ui_game_variant_specified(&variant)) network_test_save_variant(&variant);
					network_test.map_set = TRUE;
					return;
				}

				/* (a level of its own path, else a multiplayer map's name) */
				if (!strncmp(config_string("debug.test_input"), "look:inventory", 14) &&
					strcmp(config_string("debug.test_input"), "look:inventory-grenades") &&
					strcmp(config_string("debug.test_input"), "look:inventory-off") &&
					strcmp(config_string("debug.test_input"), "look:inventory-scope") && (network_test.variant_index & 1))
					snprintf(path, sizeof(path), "levels\\test\\bloodgulch\\bloodgulch");
				else if (strchr(network_test.map_name, '\\'))
					snprintf(path, sizeof(path), "%s", network_test.map_name);
				else
					snprintf(path, sizeof(path), "levels\\test\\%s\\%s", network_test.map_name, network_test.map_name);
				if (!strcmp(variant_name, "coop"))
				{
					/* (the Map screen's co-op: menu_functions.c) */
					extern boolean ui_widget_port_cooperative_level_choose(char const *map_name, short difficulty);

					if (!ui_widget_port_cooperative_level_choose(path, _game_difficulty_level_normal))
						platform_log("network test: %s is not a campaign level, for co-op", path);
					platform_log("network test: game %lu, co-op", (unsigned long)network_test.variant_index + 1);
				}
				else
				{
					network_game_server_change_map_name(global_network_game_server_get(), path);
					/* the variant, as picking the game settings does */
					network_test.infection_kills = 0;
					variant = *game_engine_get_variant_by_name(&variant,
						!strncmp(variant_name, "infection", 9) || !strncmp(variant_name, "slayer-stock", 12) ? "slayer" : variant_name);
					if (!strncmp(variant_name, "infection", 9))
						variant.universal_variant.teams = TRUE;
					platform_log("network test: game %lu, %s", (unsigned long)network_test.variant_index + 1, variant_name);
					/* debug.network_test_score: a short game, to test the next */
					if (network_test.score_to_win > 0)
						variant.universal_variant.score_to_win = network_test.score_to_win;
					player_ui_set_game_variant(&variant);
					if (!strcmp(variant_name, "slayer-stock-unarmed"))
					{
						struct game_variant_options options;
						game_variant_options_default(&variant, &options);
						options.loadout = _loadout_custom;
						options.primary_weapon = options.secondary_weapon = _loadout_weapon_none;
						player_ui_set_game_variant_options(&options);
					}
					if (strncmp(config_string("debug.test_input"), "look:inventory", 14) && (network_test_combat_input() ||
						!strncmp(config_string("debug.test_input"), "look:party", 10)) &&
						(!strcmp(variant_name, "slayer") || !strcmp(variant_name, "team_slayer")))
					{
						struct game_variant_options options;
						game_variant_options_default(&variant, &options);
						options.loadout = _loadout_custom;
						options.primary_weapon = _loadout_weapon_shotgun;
						options.secondary_weapon = _loadout_weapon_pistol;
						player_ui_set_game_variant_options(&options);
					}
					if (!strncmp(variant_name, "infection", 9))
					{
						struct game_variant_options options;
						game_variant_options_default(&variant, &options);
						options.infection_enabled = TRUE;
						/* Keep historical fixtures explicit; -ff-on tests the
						   native Team Options selection through validation. */
						options.friendly_fire = strstr(variant_name, "-ff-on") ? _friendly_fire_on : _friendly_fire_off;
						if (strstr(variant_name, "-ff-on")) options.friendly_fire_penalty = 10;
						options.time_limit = !strcmp(variant_name, "infection-survival") ? 1 : 5;
						/* Long-running population diagnostics use the existing no-time-limit option. */
						if (!strcmp(variant_name, "infection-soak")) options.time_limit = 0;
						options.loadout = _loadout_custom;
						options.primary_weapon = _loadout_weapon_shotgun;
						options.secondary_weapon = _loadout_weapon_pistol;
						if (!strncmp(variant_name, "infection-seat-", 15))
						{
							long side, kind;
							/* Match Vehicle Options: custom counts use the default remap. */
							variant.universal_variant.vehicle_set = 0;
							player_ui_set_game_variant(&variant);
							for (side = 0; side < 2; side++)
							{
								options.vehicle_set[side] = VARIANT_VEHICLE_SET_CUSTOM;
								for (kind = 0; kind < 3; kind++) options.vehicle_counts[side][kind] = 1;
							}
						}
						{
							long speed, shields, machine, respawn;
							if (network_test_classic_variant(variant_name, &speed, &shields, &machine, &respawn))
							{
								if (speed) options.infection_speed_percent = (byte)speed;
								options.infection_zombie_shields = (byte)shields;
								if (respawn) options.infection_respawn_seconds = (short)respawn;
							}
						}
						/* Exercise the map's native equipment too: the combat
						   fixtures' custom loadout masked missing spawn weapons. */
						if (!strncmp(variant_name, "infection-stock", 15))
						{
							options.loadout = _loadout_category;
							if (strstr(variant_name, "custom") || strstr(variant_name, "unarmed"))
							{
								options.loadout = _loadout_custom;
								options.primary_weapon = strstr(variant_name, "unarmed") ? _loadout_weapon_none : _loadout_weapon_sniper_rifle;
								options.secondary_weapon = strstr(variant_name, "unarmed") ? _loadout_weapon_none : _loadout_weapon_shotgun;
							}
							if (strstr(variant_name, "off")) options.infection_zombie_shields = FALSE;
							SET_FLAG(variant.universal_variant.flags, _game_variant_generic_starting_equipment_bit,
								!strcmp(variant_name, "infection-stock-generic"));
							player_ui_set_game_variant(&variant);
						}
						if (!strcmp(variant_name, "infection-tuned"))
						{
							options.infection_speed_percent = 125;
							options.infection_damage_percent = 150;
							options.infection_respawn_seconds = 3;
							options.infection_countdown_seconds = 3;
						}
						player_ui_set_game_variant_options(&options);
					}
					if (strstr(variant_name, "ammo"))
					{
						struct game_variant_options options = *player_ui_get_game_variant_options();
						options.loadout = _loadout_custom;
						options.primary_weapon = strstr(variant_name, "rifles") ? _loadout_weapon_assault_rifle : _loadout_weapon_pistol;
						options.secondary_weapon = strstr(variant_name, "rifles") ? _loadout_weapon_sniper_rifle : _loadout_weapon_shotgun;
						if (!strstr(variant_name, "normal"))
						{
							options.ammo_reserves[0] = 200; options.ammo_reserves[1] = 120;
							options.ammo_reserves[2] = options.ammo_reserves[3] = 100;
						}
						if (strstr(variant_name, "-map-"))
						{
							long kind, value, slot;
							if (sscanf(strstr(variant_name, "-map-"), "-map-%ld-%ld", &kind, &value) == 2)
							{
								options.primary_weapon = kind == 0 ? _loadout_weapon_assault_rifle : _loadout_weapon_pistol;
								options.secondary_weapon = _loadout_weapon_none;
								for (slot = 0; slot < 4; slot++) options.ammo_reserves[slot] = (short)value;
							}
						}
						player_ui_set_game_variant_options(&options);
					}
					if (!strcmp(config_string("debug.test_input"), "look:inventory-objectives"))
					{
						struct game_variant_options options = *player_ui_get_game_variant_options();
						long slot;
						for (slot = 0; slot < 4; slot++) options.ammo_reserves[slot] = 999;
						player_ui_set_game_variant_options(&options);
					}
					if (!strcmp(config_string("debug.test_input"), "look:inventory-off"))
					{
						struct game_variant_options options = *player_ui_get_game_variant_options();
						long slot;
						options.infection_enabled = FALSE;
						for (slot = 0; slot < 4; slot++) options.ammo_reserves[slot] = 100;
						player_ui_set_game_variant_options(&options);
					}
					if (!strcmp(config_string("debug.test_input"), "look:inventory-scope") ||
						!strcmp(config_string("debug.test_input"), "look:inventory-menu-toggle"))
					{
						struct game_variant_options options = *player_ui_get_game_variant_options();
						long slot;
						options.loadout = _loadout_custom;
						options.primary_weapon = _loadout_weapon_assault_rifle;
						options.secondary_weapon = _loadout_weapon_sniper_rifle;
						if (!strcmp(config_string("debug.test_input"), "look:inventory-scope"))
							for (slot = 0; slot < 4; slot++) options.ammo_reserves[slot] = 999;
						player_ui_set_game_variant_options(&options);
					}
					network_game_server_change_game_variant(global_network_game_server_get(), &variant);
					if (!strcmp(config_string("debug.test_input"), "look:inventory-menu-toggle"))
						platform_log("menu toggle: game=0 applied=%d same_variant=1", menu_test_infection_toggle(TRUE, TRUE));
					network_test_save_variant(&variant);
				}
				network_test.map_set = TRUE;
			}
			network_test_reload_variant();
			if (!network_test.player_added && network_test.setup_seconds >= 2.0f && global_network_game_client_get())
				network_test.player_added = network_game_client_add_player(global_network_game_client_get(), 0);
			if (network_test.setup_seconds >= network_test.start_delay)
			{
				network_test.started = TRUE;
				network_game_client_request_immediate_start();
				platform_log("network test: starting the game");
			}
		}
		break;
	case _network_test_join:
		if (!network_test.set_up)
		{
			network_test.set_up = TRUE;
			dispose_global_network_game_client();
			dispose_global_network_game_server();
			if (create_global_network_game_client())
			{
				game_connection_set(_game_connection_network_client);
				/* (the player joined to multiplayer first, as a player picking
				their profile: the pregame screen then asks for them every
				frame until they are in the settings) */
				player_ui_local_player_joined_multiplayer_game(0);
				platform_log("network test: searching for games");
			}
		}
		else if (!network_test.joined && network_game_client_join_first_available_game())
		{
			network_test.joined = TRUE;
			ui_widgets_close_all();
			ui_widget_load_by_name_or_tag(
				pc_menus_screen("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen"),
				NONE, NULL, NONE, NONE, NONE, NONE);
			platform_log("network test: joining");
		}
		else if (network_test.joined && !network_test.player_added)
		{
			network_test.joined_seconds += seconds;
			if (network_test.joined_seconds >= 3.0f && global_network_game_client_get())
				network_test.player_added = TRUE;
		}
		/* (the other team from the host's player: a team game needs both) */
		else if (network_test.player_added && !network_test.team_set)
		{
			network_test.joined_seconds += seconds;
			if (network_test.joined_seconds >= 5.0f)
				network_test.team_set = network_game_client_set_team(NONE);
		}
		break;
	}
}
