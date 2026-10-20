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

debug.network_test_ragdoll (game.ragdolls' golden frames,
tools/test_macos_frames.py): that many seconds of frames shown in (a 60th
of a second each, the frames the captures count), a frag grenade's
explosion (its detonation effect, damage and all) kills a biped of the
first player's kind standing six metres before the camera, and eight
seconds later a vehicle rolls into its body from the side;
debug.network_test_ragdoll_bullet kills it with an assault rifle's bullet
from the camera instead (game.dismemberment's frames: a bullet tears
nothing).

debug.network_test_debris (game.physics_debris' golden frames): that many
seconds of frames shown in, an assault rifle held a metre above the ground
three world units before the camera, across the view, fires twenty rounds at
its rate of fire, each its own firing effect (weapons\assault rifle\effects\
fire bullet: the flash, the sound and the casing the tag ejects), and is then
let fall.

debug.network_test_camera holds the view at the debug camera of camera.txt
in the data root (director_load_camera), for frames captured the same run
to run.

Called from the main loop every frame (main.c).
*/

#include "cseries.h"
#include "main/main.h"
#include "interface/player_ui.h"
#include "saved games/player_profile.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "networking/network_client_manager.h"
#include "networking/network_server_manager.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "items/projectile_definitions.h"
#include "items/items.h"
#include "objects/damage.h"
#include "scenario/scenario.h"
#include "tag_files/tag_files.h"
#include "camera/observer.h"
#include "camera/director.h"
#include "effects/effects.h"
#include "effects/effect_definitions.h"
#include "effects/particles.h"
#include "physics/collisions.h"
#include "physics_debris.h"

#include "../src/p2p.h" /* port: port/linux/src/p2p.c, p2p_lobby.c */
#include "network_chat.h"
#include "network_voice.h"
#include "halo_keyboard.h"
#include "ragdolls.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* the platform layer's (port/linux/src/port_config.c) */
const char *config_string(char const *name);
int config_boolean(char const *name);
double config_real(char const *name);
long config_integer(char const *name);
int config_write(char const *name, char const *value);
void platform_log(char const *format, ...);
/* xbox_kernel.c's */
unsigned long platform_frames_shown(void);
/* damage.c's */
void damage_kill_object_for_player(long object_index, long player_index);
/* network_distributed.c's */
void network_distributed_statistics(long *sent, long *received, long *corrections);
void network_distributed_item_statistics(long *creates, long *deletes, long *failures, long *removed);
void network_damage_statistics(long *sent_reports, long *dealt_reports, long *rejected_reports, long *replayed_events);
/* xinput_sdl.c's */
void test_input_hold_action(int hold);
void test_input_press(unsigned long buttons);

enum
{
	_network_test_off,
	_network_test_host,
	_network_test_join,
	_network_test_browse,
};

/* debug.network_test_crowd's rows of bipeds and casings thrown */
enum
{
	NETWORK_TEST_CROWD_COLUMNS = 5,
	NETWORK_TEST_CROWD_ROWS = 6,
	NETWORK_TEST_CROWD_BIPEDS = NETWORK_TEST_CROWD_COLUMNS * NETWORK_TEST_CROWD_ROWS,
	NETWORK_TEST_CROWD_CASINGS = 200,
};

static struct
{
	boolean checked;
	short mode;
	char map_name[64];
	char variant_name[64];
	/* ... the variant of this game, of variant_name's list; and the seconds
	the last game's scores have been shown */
	short variant_index;
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
	real shoot_interval;
	real vehicle_time;
	real pickup_time;
	char pickup_weapon[64];
	/* debug.network_test_camera: the debug camera of the data root's
	camera.txt, loaded each second */
	boolean camera;
	/* debug.network_test_chat's, and the lines said this game */
	real chat_time;
	short chat_lines_said;
	long score_to_win;
	long logged_time;
	boolean public_game;
	boolean browsing;
	boolean browse_listed;
	float browse_seconds;
	/* debug.network_test_ragdoll's: when, how far the scene has gone, its
	biped, where it stands and the view's right */
	real ragdoll_time;
	boolean ragdoll_bullet;
	short ragdoll_stage;
	long ragdoll_biped_index;
	real_point3d ragdoll_point;
	real_vector3d ragdoll_right;
	/* debug.network_test_debris's: when, the rounds fired, the rifle and
	where it is held */
	real debris_time;
	short debris_rounds;
	long debris_weapon_index;
	real_point3d debris_point;
	real_vector3d debris_forward;
	/* debug.network_test_crowd's: when, how far the scene has gone, its
	bipeds and the casings thrown */
	real crowd_time;
	short crowd_stage;
	short crowd_killed;
	short crowd_casings;
	long crowd_tick;
	long crowd_biped_indices[NETWORK_TEST_CROWD_BIPEDS];
	real_point3d crowd_center;
	real_point3d crowd_casing_origin;
} network_test;

/* the variant at the index of the list (copied to name), FALSE past its end */
static boolean network_test_variant(
	short index,
	char *name,
	size_t size)
{
	char const *variant = network_test.variant_name;

	for (; index > 0 && variant; index--)
	{
		variant = strchr(variant, ',');
		if (variant)
			variant++;
	}
	if (!variant || !*variant)
		return FALSE;
	if (name)
	{
		size_t length = strcspn(variant, ",");

		snprintf(name, size, "%.*s", (int)length, variant);
	}
	return TRUE;
}

static void network_test_read_settings(
	void)
{
	char const *setting = config_string("debug.network_test");

	network_test.checked = TRUE;
	if (!strncmp(setting, "host:", 5) && setting[5])
	{
		char *colon;

		network_test.mode = _network_test_host;
		snprintf(network_test.map_name, sizeof(network_test.map_name), "%s", setting + 5);
		snprintf(network_test.variant_name, sizeof(network_test.variant_name), "slayer");
		colon = strchr(network_test.map_name, ':');
		if (colon)
		{
			*colon = 0;
			snprintf(network_test.variant_name, sizeof(network_test.variant_name), "%s", colon + 1);
		}
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
	network_test.chat_time = (real)config_real("debug.network_test_chat");
	network_test.ragdoll_time = (real)config_real("debug.network_test_ragdoll");
	network_test.ragdoll_bullet = config_boolean("debug.network_test_ragdoll_bullet");
	network_test.ragdoll_biped_index = NONE;
	network_test.debris_time = (real)config_real("debug.network_test_debris");
	network_test.debris_weapon_index = NONE;
	network_test.crowd_time = (real)config_real("debug.network_test_crowd");
	snprintf(network_test.pickup_weapon, sizeof(network_test.pickup_weapon), "%s",
		config_string("debug.network_test_pickup_weapon"));
	network_test.camera = config_boolean("debug.network_test_camera") ? TRUE : FALSE;
	network_test.score_to_win = (long)config_integer("debug.network_test_score");
	if (network_test.mode != _network_test_off)
		platform_log("network test: %s", setting);
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
		long ground_items = 0;

		object_iterator_new(&objects, _object_mask_weapon | _object_mask_equipment, 0);
		while (object_iterator_next(&objects))
		{
			struct item_datum *item = item_get(objects.index);

			if (item->object.parent_object_index == NONE &&
				TEST_FLAG(item->object.flags, _object_connected_to_map_bit) &&
				!TEST_FLAG(item->item.flags, _item_attached_to_unit_bit))
			{
				ground_items++;
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
static void network_test_shoot(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;

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
		data_iterator_new(&targets, player_data);
		while ((candidate = (struct player_datum *)data_iterator_next(&targets)) != NULL)
		{
			if (candidate != player && candidate->unit_index != NONE)
			{
				target = candidate;
				break;
			}
		}
		unit = unit_get(player->unit_index);
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
static void network_test_gather(
	boolean leave_last)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *first = NULL;
	struct player_datum *last = NULL;
	short count = 0;

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
		count++;
		unit = object_get(player->unit_index);
		if (unit->object.parent_object_index != NONE || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			continue;
		dx = unit->object.position.x - center->object.position.x;
		dy = unit->object.position.y - center->object.position.y;
		dz = unit->object.position.z - center->object.position.z;
		if (dx * dx + dy * dy + dz * dz <= 20.0f * 20.0f)
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
				position.x += offsets[index][0];
				position.y += offsets[index][1];
				position.z += 0.2f;
				above = position;
				above.z += 0.7f;
				scenario_location_from_point(&feet, &position);
				scenario_location_from_point(&head, &above);
				if (feet.cluster_index != NONE && head.cluster_index != NONE)
					break;
			}
			if (try_index >= (short)NUMBEROF(offsets))
				continue;
		}
		object_set_position(player->unit_index, &position, NULL, NULL);
		platform_log("network test: the host brings player %ld near the first",
			(long)(player - (struct player_datum *)player_data->data));
	}
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

/* debug.network_test_debris's scene: a second before its time an assault
rifle is made three world units before the camera, a metre above the ground,
pointing to the view's left (its ejection port away from the camera, so the
casings fly ahead of it, in view); from its time it fires a round every four
frames (15 a second), held where it is, twenty in all, then it falls. Logged:
the casings left after the burst and how many are Jolt's bodies, and those at
rest 290 frames in. (A casing out of view, or too small to draw, for 15 ticks
is deleted, as in the stock game: the scene keeps them near) */
static void network_test_debris(
	void)
{
	long frame = (long)platform_frames_shown();
	long start = (long)(network_test.debris_time * 60.0f);

	if (network_test.debris_rounds == 0 && network_test.debris_weapon_index == NONE && frame >= start - 60)
	{
		long weapon_definition_index = tag_loaded(WEAPON_DEFINITION_TAG, "weapons\\assault rifle\\assault rifle");
		struct observer_result const *camera = observer_get_camera(0);
		struct object_placement_data data;
		struct collision_result collision;
		real_vector3d forward;
		real_vector3d down;
		real_point3d above;

		network_test.debris_rounds = -1;
		if (weapon_definition_index == NONE)
		{
			platform_log("network test: no assault rifle, for the debris");
			return;
		}
		forward = camera->forward;
		forward.k = 0.0f;
		normalize3d(&forward);
		/* (the view's left) */
		network_test.debris_forward.i = -forward.j;
		network_test.debris_forward.j = forward.i;
		network_test.debris_forward.k = 0.0f;
		point_from_line3d(&camera->position, &forward, 3.0f, &above);
		above.z += 2.0f;
		down.i = 0.0f;
		down.j = 0.0f;
		down.k = -50.0f;
		network_test.debris_point = above;
		if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
			&above, &down, NONE, &collision))
		{
			network_test.debris_point = collision.point;
		}
		network_test.debris_point.z += 1.0f / 3.048f;
		object_placement_data_new(&data, weapon_definition_index, NONE);
		data.position = network_test.debris_point;
		data.forward = network_test.debris_forward;
		data.up = *global_up3d;
		network_test.debris_weapon_index = object_new(&data);
		network_test.debris_rounds = network_test.debris_weapon_index == NONE ? -1 : 0;
		platform_log("network test: an assault rifle %lx held at (%.2f %.2f %.2f)", network_test.debris_weapon_index,
			network_test.debris_point.x, network_test.debris_point.y, network_test.debris_point.z);
	}
	if (network_test.debris_rounds >= 0 && network_test.debris_rounds < 20 &&
		network_test.debris_weapon_index != NONE && frame >= start + 4 * network_test.debris_rounds)
	{
		long effect_index = tag_loaded(EFFECT_DEFINITION_TAG, "weapons\\assault rifle\\effects\\fire bullet");

		/* (held where it is as it fires) */
		object_set_position(network_test.debris_weapon_index, &network_test.debris_point,
			&network_test.debris_forward, global_up3d);
		object_get(network_test.debris_weapon_index)->object.translational_velocity = *global_zero_vector3d;
		object_get(network_test.debris_weapon_index)->object.angular_velocity = *global_zero_vector3d;
		if (effect_index == NONE)
		{
			platform_log("network test: no assault rifle firing effect, for the debris");
			network_test.debris_rounds = -1;
			return;
		}
		effect_new_from_object(effect_index, NONE, network_test.debris_weapon_index, NONE, 1.0f, 0.0f, NULL, NULL);
		network_test.debris_rounds++;
		if (network_test.debris_rounds == 20)
		{
			long casing_index = tag_loaded(PARTICLE_TAG, "effects\\particles\\solid\\casing h ar bullet");
			struct data_iterator iterator;
			struct particle_datum *particle;
			short casings = 0;
			short bodies = 0;

			data_iterator_new(&iterator, particle_data);
			while ((particle = (struct particle_datum *)data_iterator_next(&iterator)) != NULL)
			{
				if (particle->definition_index == casing_index)
				{
					casings++;
					if (physics_debris_particle_is_body(iterator.datum_index))
						bodies++;
				}
			}
			platform_log("network test: the assault rifle has fired twenty rounds, %d casings left, %d of them Jolt's bodies",
				casings, bodies);
		}
	}
	/* (the casings left to be drawn, near the frames at rest) */
	if (network_test.debris_rounds == 20 && frame >= start + 290)
	{
		long casing_index = tag_loaded(PARTICLE_TAG, "effects\\particles\\solid\\casing h ar bullet");
		struct data_iterator iterator;
		struct particle_datum *particle;
		short casings = 0;
		short at_rest = 0;
		short bodies = 0;

		data_iterator_new(&iterator, particle_data);
		while ((particle = (struct particle_datum *)data_iterator_next(&iterator)) != NULL)
		{
			if (particle->definition_index == casing_index)
			{
				casings++;
				if (TEST_FLAG(particle->flags, _particle_datum_at_rest_bit))
					at_rest++;
				if (physics_debris_particle_is_body(iterator.datum_index))
					bodies++;
			}
		}
		platform_log("network test: %d casings left, %d of them at rest, %d Jolt's bodies", casings, at_rest, bodies);
		network_test.debris_rounds = 21;
	}
	/* (held until it has fired) */
	if (network_test.debris_rounds > 0 && network_test.debris_rounds < 20 && network_test.debris_weapon_index != NONE)
	{
		object_set_position(network_test.debris_weapon_index, &network_test.debris_point,
			&network_test.debris_forward, global_up3d);
		object_get(network_test.debris_weapon_index)->object.translational_velocity = *global_zero_vector3d;
	}
}

/* debug.network_test_crowd's scene, the benchmark's (tools/test_macos_benchmark.py):
two seconds before its time thirty bipeds of the first player's kind stand
on the ground in six rows of five, a world unit apart, eight world units
before the camera; from its time three a tick are shot dead (their shields
gone, an assault rifle's bullet from the camera), and ten casings a tick,
two hundred in all, are thrown up from a world unit above the ground three
world units before the camera, as near as a gun's are (further, they are
smaller on the screen than their tag's cutoff, are not drawn, and go). Its
time is the game's, in ticks, not frames shown: the menus show thousands of
frames before the map loads, and debug.network_test_camera's camera is held
only from the game's first second. By the ticks past its start too: a fast
machine shows several frames a tick, and a tick queues no more than
RAGDOLL_DEATHS_MAX deaths. Logged: how many died and were thrown, and
two seconds of ticks on the ragdolls and the casings that are Jolt's
bodies */
static void network_test_crowd(
	void)
{
	long tick = game_time_get();
	long start = (long)(network_test.crowd_time * TICKS_PER_SECOND);
	struct player_datum *first = NULL;
	struct data_iterator iterator;

	data_iterator_new(&iterator, player_data);
	first = (struct player_datum *)data_iterator_next(&iterator);
	if (network_test.crowd_stage == 0 && tick >= start - 2 * TICKS_PER_SECOND && first && first->unit_index != NONE)
	{
		struct observer_result const *camera = observer_get_camera(0);
		real_vector3d forward;
		real_vector3d right;
		short biped;

		forward = camera->forward;
		forward.k = 0.0f;
		normalize3d(&forward);
		right.i = forward.j;
		right.j = -forward.i;
		right.k = 0.0f;
		point_from_line3d(&camera->position, &forward, 8.0f + 0.5f * (NETWORK_TEST_CROWD_ROWS - 1),
			&network_test.crowd_center);
		point_from_line3d(&camera->position, &forward, 3.0f, &network_test.crowd_casing_origin);
		for (biped = 0; biped < NETWORK_TEST_CROWD_BIPEDS; biped++)
		{
			struct object_placement_data data;
			struct collision_result collision;
			real_vector3d down;
			real_point3d above;

			point_from_line3d(&camera->position, &forward, 8.0f + (real)(biped / NETWORK_TEST_CROWD_COLUMNS), &above);
			point_from_line3d(&above, &right, (real)(biped % NETWORK_TEST_CROWD_COLUMNS) -
				0.5f * (NETWORK_TEST_CROWD_COLUMNS - 1), &above);
			above.z += 2.0f;
			down.i = 0.0f;
			down.j = 0.0f;
			down.k = -50.0f;
			if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
				&above, &down, NONE, &collision))
			{
				above = collision.point;
			}
			object_placement_data_new(&data, object_get(first->unit_index)->definition_index, NONE);
			data.position = above;
			data.position.z += 0.05f;
			data.forward.i = -forward.i;
			data.forward.j = -forward.j;
			data.forward.k = 0.0f;
			data.up = *global_up3d;
			network_test.crowd_biped_indices[biped] = object_new(&data);
			if (biped == NETWORK_TEST_CROWD_BIPEDS / 2)
				network_test.crowd_center.z = above.z;
		}
		{
			struct collision_result collision;
			real_vector3d down;

			network_test.crowd_casing_origin.z = camera->position.z;
			down.i = 0.0f;
			down.j = 0.0f;
			down.k = -50.0f;
			if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
				&network_test.crowd_casing_origin, &down, NONE, &collision))
			{
				network_test.crowd_casing_origin = collision.point;
			}
			network_test.crowd_casing_origin.z += 1.0f;
		}
		network_test.crowd_stage = 1;
		platform_log("network test: %d bipeds stand at (%.2f %.2f %.2f)", NETWORK_TEST_CROWD_BIPEDS,
			network_test.crowd_center.x, network_test.crowd_center.y, network_test.crowd_center.z);
	}
	if (network_test.crowd_stage == 1 && tick >= start && tick != network_test.crowd_tick)
	{
		long bullet_index = tag_loaded(PROJECTILE_DEFINITION_TAG, "weapons\\assault rifle\\bullet");
		long casing_index = tag_loaded(PARTICLE_TAG, "effects\\particles\\solid\\casing h ar bullet");
		struct observer_result const *camera = observer_get_camera(0);
		short count;

		if (bullet_index == NONE || projectile_definition_get(bullet_index)->projectile.impact_damage.index == NONE ||
			casing_index == NONE)
		{
			platform_log("network test: no assault rifle bullet or casing, for the crowd");
			network_test.crowd_stage = 3;
			return;
		}
		network_test.crowd_tick = game_time_get();
		/* (three a tick, fewer than a tick's deaths queued, RAGDOLL_DEATHS_MAX) */
		for (count = 0; count < 3 && network_test.crowd_killed < NETWORK_TEST_CROWD_BIPEDS; count++)
		{
			long biped_index = network_test.crowd_biped_indices[network_test.crowd_killed++];
			struct object_datum *biped;
			struct damage_data damage;

			if (biped_index == NONE || !(biped = object_try_and_get_and_verify_type(biped_index, _object_mask_biped)))
				continue;
			biped->object.shield_vitality = 0.0f;
			biped->object.body_vitality = 0.05f;
			damage_data_new(&damage, projectile_definition_get(bullet_index)->projectile.impact_damage.index);
			damage.origin = camera->position;
			damage.epicenter = biped->object.position;
			damage.epicenter.z += 0.5f;
			vector_from_points3d(&damage.origin, &damage.epicenter, &damage.direction);
			normalize3d(&damage.direction);
			object_cause_damage(&damage, biped_index, NONE, NONE, NONE, NULL);
		}
		for (count = 0; count < 10 && network_test.crowd_casings < NETWORK_TEST_CROWD_CASINGS; count++)
		{
			struct new_particle_data data;

			memset(&data, 0, sizeof(data));
			data.definition_index = casing_index;
			data.object_index = NONE;
			data.node_index = NONE;
			data.local_player_index = NONE;
			data.position = network_test.crowd_casing_origin;
			data.direction = *global_up3d;
			/* (spread on a grid of ten by twenty, up to 0.7 world units a second
			each way, to stay in view) */
			data.velocity.i = 0.15f * ((real)(network_test.crowd_casings % 10) - 4.5f);
			data.velocity.j = 0.075f * ((real)(network_test.crowd_casings / 10) - 9.5f);
			data.velocity.k = 2.0f;
			data.radius = 0.015f;
			data.color.alpha = 1.0f;
			data.color.rgb.red = 1.0f;
			data.color.rgb.green = 1.0f;
			data.color.rgb.blue = 1.0f;
			particle_new(&data);
			network_test.crowd_casings++;
		}
		if (network_test.crowd_killed == NETWORK_TEST_CROWD_BIPEDS && network_test.crowd_casings == NETWORK_TEST_CROWD_CASINGS)
		{
			short dead = 0;
			short biped;

			for (biped = 0; biped < NETWORK_TEST_CROWD_BIPEDS; biped++)
			{
				long biped_index = network_test.crowd_biped_indices[biped];

				if (biped_index != NONE && object_try_and_get_and_verify_type(biped_index, _object_mask_biped) &&
					TEST_FLAG(object_get(biped_index)->object.damage_flags, _object_dead_bit))
				{
					dead++;
				}
			}
			network_test.crowd_stage = 2;
			platform_log("network test: the crowd: %d of %d bipeds dead, %d casings thrown", dead,
				NETWORK_TEST_CROWD_BIPEDS, NETWORK_TEST_CROWD_CASINGS);
		}
	}
	if (network_test.crowd_stage == 2 && game_time_get() - network_test.crowd_tick >= 2 * TICKS_PER_SECOND)
	{
		long casing_index = tag_loaded(PARTICLE_TAG, "effects\\particles\\solid\\casing h ar bullet");
		struct data_iterator particles;
		struct particle_datum *particle;
		short ragdolled = 0;
		short casings = 0;
		short bodies = 0;
		short biped;

		for (biped = 0; biped < NETWORK_TEST_CROWD_BIPEDS; biped++)
		{
			if (network_test.crowd_biped_indices[biped] != NONE &&
				ragdolls_of_object(network_test.crowd_biped_indices[biped]))
			{
				ragdolled++;
			}
		}
		data_iterator_new(&particles, particle_data);
		while ((particle = (struct particle_datum *)data_iterator_next(&particles)) != NULL)
		{
			if (particle->definition_index == casing_index)
			{
				casings++;
				if (physics_debris_particle_is_body(particles.datum_index))
					bodies++;
			}
		}
		network_test.crowd_stage = 3;
		platform_log("network test: the crowd: %d ragdolls, %d casings left, %d of them Jolt's bodies", ragdolled,
			casings, bodies);
	}
}

/* debug.network_test_ragdoll's scene, by the game's ticks: two seconds
before its time a biped of the first player's kind stands on the ground six
metres before the camera, facing it; at its time, its shield gone and its
health nearly, a frag grenade goes off at its left foot; eight seconds on,
the nearest vehicle rolls at its body from the view's right */
static void network_test_ragdoll(
	void)
{
	/* (by the frames shown, which the macOS app's captures count, not the
	game's time, which has the map's load in it) */
	long frame = (long)platform_frames_shown();
	long start = (long)(network_test.ragdoll_time * 60.0f);
	struct player_datum *first = NULL;
	struct data_iterator iterator;

	data_iterator_new(&iterator, player_data);
	first = (struct player_datum *)data_iterator_next(&iterator);
	if (network_test.ragdoll_stage == 0 && frame >= start - 2 * 60 &&
		first && first->unit_index != NONE)
	{
		struct observer_result const *camera = observer_get_camera(0);
		struct object_placement_data data;
		struct collision_result collision;
		real_vector3d forward;
		real_vector3d down;
		real_point3d above;

		forward = camera->forward;
		forward.k = 0.0f;
		normalize3d(&forward);
		network_test.ragdoll_right.i = forward.j;
		network_test.ragdoll_right.j = -forward.i;
		network_test.ragdoll_right.k = 0.0f;
		point_from_line3d(&camera->position, &forward, 6.0f, &above);
		above.z += 2.0f;
		down.i = 0.0f;
		down.j = 0.0f;
		down.k = -50.0f;
		network_test.ragdoll_point = above;
		if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
			&above, &down, NONE, &collision))
		{
			network_test.ragdoll_point = collision.point;
		}
		object_placement_data_new(&data, object_get(first->unit_index)->definition_index, NONE);
		data.position = network_test.ragdoll_point;
		data.position.z += 0.05f;
		data.forward.i = -forward.i;
		data.forward.j = -forward.j;
		data.forward.k = 0.0f;
		data.up = *global_up3d;
		network_test.ragdoll_biped_index = object_new(&data);
		network_test.ragdoll_stage = 1;
		platform_log("network test: a biped %lx stands at (%.2f %.2f %.2f)", network_test.ragdoll_biped_index,
			network_test.ragdoll_point.x, network_test.ragdoll_point.y, network_test.ragdoll_point.z);
	}
	if (network_test.ragdoll_stage == 1 && frame >= start)
	{
		long projectile_index = tag_loaded(PROJECTILE_DEFINITION_TAG, "weapons\\frag grenade\\frag grenade");
		long effect_index = projectile_index != NONE ?
			projectile_definition_get(projectile_index)->projectile.effect.index : NONE;
		/* (the markers a projectile detonates with, projectiles.c) */
		char const *marker_names[2];
		real_point3d points[2];
		real_vector3d forwards[2];
		real_vector3d forward;

		network_test.ragdoll_stage = 2;
		if (network_test.ragdoll_biped_index == NONE || effect_index == NONE)
		{
			platform_log("network test: no biped or no frag grenade, for the ragdoll");
			return;
		}
		{
			struct object_datum *biped = object_get(network_test.ragdoll_biped_index);

			biped->object.shield_vitality = 0.0f;
			biped->object.body_vitality = 0.05f;
		}
		if (network_test.ragdoll_bullet)
		{
			/* (an assault rifle's bullet, from the camera into its chest) */
			long bullet_index = tag_loaded(PROJECTILE_DEFINITION_TAG, "weapons\\assault rifle\\bullet");
			struct observer_result const *camera = observer_get_camera(0);
			struct damage_data damage;

			if (bullet_index == NONE || projectile_definition_get(bullet_index)->projectile.impact_damage.index == NONE)
			{
				platform_log("network test: no assault rifle bullet, for the ragdoll");
				return;
			}
			damage_data_new(&damage, projectile_definition_get(bullet_index)->projectile.impact_damage.index);
			damage.origin = camera->position;
			damage.epicenter = network_test.ragdoll_point;
			damage.epicenter.z += 0.5f;
			vector_from_points3d(&damage.origin, &damage.epicenter, &damage.direction);
			normalize3d(&damage.direction);
			object_cause_damage(&damage, network_test.ragdoll_biped_index, NONE, NONE, NONE, NULL);
			platform_log("network test: a bullet strikes (%.2f %.2f %.2f): the biped %s", damage.epicenter.x,
				damage.epicenter.y, damage.epicenter.z,
				TEST_FLAG(object_get(network_test.ragdoll_biped_index)->object.damage_flags, _object_dead_bit) ?
					"dies" : "lives");
			return;
		}
		marker_names[0] = "";
		marker_names[1] = "gravity";
		/* (between the camera and the biped, which it throws away from the
		camera) */
		forward.i = -network_test.ragdoll_right.j;
		forward.j = network_test.ragdoll_right.i;
		forward.k = 0.0f;
		point_from_line3d(&network_test.ragdoll_point, &forward, -0.4f, &points[0]);
		points[0].z += 0.1f;
		points[1] = points[0];
		forwards[0] = *global_up3d;
		forwards[1] = *global_down3d;
		effect_new_unattached_from_markers(effect_index, NONE, NULL, 2, marker_names, points, forwards, 0.0f, 0.0f,
			NULL, NULL, TRUE);
		platform_log("network test: a frag grenade goes off at (%.2f %.2f %.2f): the biped %s", points[0].x,
			points[0].y, points[0].z, TEST_FLAG(object_get(network_test.ragdoll_biped_index)->object.damage_flags, _object_dead_bit) ?
				"dies" : "lives");
	}
	if (network_test.ragdoll_stage == 2 && frame >= start + 8 * 60)
	{
		struct object_iterator vehicles;
		struct ragdoll const *ragdoll = network_test.ragdoll_biped_index != NONE ?
			ragdolls_of_object(network_test.ragdoll_biped_index) : NULL;
		real_point3d body = ragdoll && ragdoll->posed ? ragdoll->poses[0].position : network_test.ragdoll_point;
		long vehicle_index = NONE;
		real nearest = 0.0f;

		network_test.ragdoll_stage = 3;
		if (ragdoll)
			platform_log("network test: the ragdoll's torn joints %lx", ragdoll->broken_joints);
		object_iterator_new(&vehicles, _object_mask_vehicle, 0);
		while (object_iterator_next(&vehicles))
		{
			real distance = distance_squared3d(&body, &object_get(vehicles.index)->object.position);

			if (vehicle_index == NONE || distance < nearest)
			{
				vehicle_index = vehicles.index;
				nearest = distance;
			}
		}
		if (vehicle_index == NONE)
		{
			platform_log("network test: no vehicle, for the ragdoll");
			return;
		}
		{
			struct object_datum *vehicle = object_get(vehicle_index);
			struct collision_result collision;
			real_point3d start;
			real_vector3d forward;
			real_vector3d down;

			/* (from beyond the body, on the ground there, down the slope
			towards it and the camera) */
			forward.i = -network_test.ragdoll_right.j;
			forward.j = network_test.ragdoll_right.i;
			forward.k = 0.0f;
			point_from_line3d(&body, &forward, 4.0f, &start);
			start.z += 10.0f;
			down.i = 0.0f;
			down.j = 0.0f;
			down.k = -50.0f;
			if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
				&start, &down, NONE, &collision))
			{
				start = collision.point;
			}
			start.z += 0.6f;
			vector_from_points3d(&start, &body, &forward);
			forward.k = 0.0f;
			normalize3d(&forward);
			object_set_position(vehicle_index, &start, &forward, global_up3d);
			/* (about 18 m/s, in world units a tick) */
			scale_vector3d(&forward, 0.2f, &vehicle->object.translational_velocity);
			/* (a vehicle at rest runs no physics, so would not move) */
			SET_FLAG(vehicle->object.flags, _object_at_rest_bit, FALSE);
			object_activate(vehicle_index);
			platform_log("network test: vehicle %lx rolls from (%.2f %.2f %.2f) at the body at (%.2f %.2f %.2f)%s",
				vehicle_index, start.x, start.y, start.z, body.x, body.y, body.z, ragdoll && ragdoll->posed ? ", its ragdoll's" : ragdoll ? ", its ragdoll unposed" :
				", no ragdoll");
		}
	}
}

/* debug.network_test_name: the name the machine's first player takes (its
profile's, which it joins with), in ASCII */
static void network_test_set_name(
	void)
{
	char const *name = config_string("debug.network_test_name");
	struct player_profile profile;
	short index;

	if (!name || !name[0])
		return;
	player_ui_get_active_player_profile(0, &profile);
	for (index = 0; name[index] && index < MAXIMUM_PLAYER_PROFILE_NAME_LENGTH - 1; index++)
		profile.player_name[index] = (wchar_t)(unsigned char)name[index];
	profile.player_name[index] = 0;
	player_ui_set_active_player_profile(0, player_ui_get_active_player_profile_index(0), &profile);
	platform_log("network test: this machine's player is named %s", name);
}

/* debug.network_test_chat: the host seats the last two players in one
vehicle, the nearest to the first of them with seats for both (any, a map's
own), for the vehicle chat (two joining machines' players, of one team: they
start near each other) */
static void network_test_seat_together(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct object_iterator vehicles;
	long units[2];
	long best_index = NONE;
	real best_distance = 0.0f;
	short count = 0;
	short index;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		if (player->unit_index == NONE)
			continue;
		units[0] = count ? units[1] : NONE;
		units[1] = player->unit_index;
		count++;
	}
	if (count < 2)
		return;
	object_iterator_new(&vehicles, _object_mask_vehicle, 0);
	while (object_iterator_next(&vehicles))
	{
		real distance;

		if (unit_definition_get(object_get(vehicles.index)->definition_index)->unit.seats.count < 2)
			continue;
		distance = distance_squared3d(&object_get(units[0])->object.position, &object_get(vehicles.index)->object.position);
		if (best_index == NONE || distance < best_distance)
		{
			best_index = vehicles.index;
			best_distance = distance;
		}
	}
	if (best_index == NONE)
	{
		platform_log("network test: no vehicle with two seats");
		return;
	}
	for (index = 0; index < (short)NUMBEROF(units); index++)
	{
		short seat_index;

		for (seat_index = 0;
			seat_index < unit_definition_get(object_get(best_index)->definition_index)->unit.seats.count; seat_index++)
		{
			if (!unit_seat_filled(best_index, seat_index) && unit_enter_seat(units[index], best_index, seat_index))
				break;
		}
	}
	platform_log("network test: the last two players ride vehicle %lx: %s, %s", best_index,
		object_get(units[0])->object.parent_object_index == best_index ? "in" : "out",
		object_get(units[1])->object.parent_object_index == best_index ? "in" : "out");
}

/* debug.network_test_chat: a line typed on the text chat's on-screen
keyboard with the controller's buttons, ticks since it began: each held
for two frames and let go for two, whatever the frame rate (a press of two
ticks falls between two frames of a machine drawing slowly) (the keyboard
opens on the first key, 'q': "gg ez", then D-pad Down to the team, then
Start) */
static void network_test_keyboard_line(
	long ticks)
{
	enum { STEP_FRAMES = 4 };
	static long frames;
	static long last_ticks = -1;
	static unsigned long const steps[] =
	{
		1UL << HALO_CHAT_PAD_UP,
		1UL << HALO_CHAT_PAD_STICK_DOWN,
		1UL << HALO_CHAT_PAD_STICK_RIGHT, 1UL << HALO_CHAT_PAD_STICK_RIGHT, 1UL << HALO_CHAT_PAD_STICK_RIGHT,
		1UL << HALO_CHAT_PAD_STICK_RIGHT,
		1UL << HALO_CHAT_PAD_A, 1UL << HALO_CHAT_PAD_A,
		1UL << HALO_CHAT_PAD_STICK_DOWN, 1UL << HALO_CHAT_PAD_STICK_DOWN,
		1UL << HALO_CHAT_PAD_A,
		1UL << HALO_CHAT_PAD_STICK_UP, 1UL << HALO_CHAT_PAD_STICK_UP, 1UL << HALO_CHAT_PAD_STICK_UP,
		1UL << HALO_CHAT_PAD_STICK_LEFT, 1UL << HALO_CHAT_PAD_STICK_LEFT,
		1UL << HALO_CHAT_PAD_A,
		1UL << HALO_CHAT_PAD_STICK_DOWN, 1UL << HALO_CHAT_PAD_STICK_DOWN,
		1UL << HALO_CHAT_PAD_STICK_LEFT, 1UL << HALO_CHAT_PAD_STICK_LEFT,
		1UL << HALO_CHAT_PAD_A,
		1UL << HALO_CHAT_PAD_DOWN,
		1UL << HALO_CHAT_PAD_START,
	};
	long step;

	/* (frames since it began: again for the next game's line) */
	if (ticks < last_ticks)
		frames = 0;
	last_ticks = ticks;
	step = frames / STEP_FRAMES;
	frames++;
	if (step >= (long)NUMBEROF(steps))
	{
		/* (after the line, let go: a held key would move nothing more) */
		if (step == (long)NUMBEROF(steps))
			test_input_press(0);
		return;
	}
	test_input_press((frames - 1) % STEP_FRAMES < STEP_FRAMES / 2 ? steps[step] : 0);
}

/* debug.network_test_chat: the last player (absolute index; NONE), whom the
host mutes; whether it is this machine's; and the first player's machine
(the network's index: the host's), which the last player's mutes */
static short network_test_last_player_index(
	void)
{
	struct data_iterator iterator;
	short last = NONE;

	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator) != NULL)
		last = (short)DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
	return last;
}

static boolean network_test_last_player_local(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *last = NULL;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		last = player;
	return last && last->local_player_index != NONE;
}

static long network_test_first_player_machine(
	void)
{
	struct data_iterator iterator;
	struct player_datum *player;

	data_iterator_new(&iterator, player_data);
	player = (struct player_datum *)data_iterator_next(&iterator);
	return player ? player->network_player_data.machine_index : NONE;
}

/* debug.network_test_chat: the last player's name, in ASCII (the host
mutes it) */
static void network_test_last_player_name(
	char *name,
	short size)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *last = NULL;
	short index;

	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
		last = player;
	for (index = 0; last && index < size - 1 && index < (short)NUMBEROF(last->name) && last->name[index]; index++)
		name[index] = player_name_character_ascii(last->name[index]);
	name[index] = 0;
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

	/* debug.network_test_chat: each client's player types "gg ez" on the
	text chat's on-screen keyboard 32 seconds on, as a controller does, and
	sends it to the team (each frame, not each second, as the rest) */
	if (network_test.chat_time > 0.0f && network_test.mode != _network_test_host && game_in_progress() &&
		!main_menu_loaded &&
		game_time_get() >= (long)(network_test.chat_time * TICKS_PER_SECOND) + 32 * TICKS_PER_SECOND)
	{
		network_test_keyboard_line(game_time_get() - (long)(network_test.chat_time * TICKS_PER_SECOND) -
			32 * TICKS_PER_SECOND);
	}
	/* debug.network_test_debris: the host's casings, by the frame */
	if (network_test.debris_time > 0.0f && network_test.mode == _network_test_host && game_in_progress() &&
		!main_menu_loaded)
	{
		network_test_debris();
	}
	/* debug.network_test_crowd: the host's benchmark scene, by the tick */
	if (network_test.crowd_time > 0.0f && network_test.mode == _network_test_host && game_in_progress() &&
		!main_menu_loaded)
	{
		network_test_crowd();
	}
	/* debug.network_test_ragdoll: the host's scene, by the tick */
	if (network_test.ragdoll_time > 0.0f && network_test.mode == _network_test_host && game_in_progress() &&
		!main_menu_loaded)
	{
		network_test_ragdoll();
	}
	/* the game running: report (from the start of each game: the next
	game's time starts over) */
	if (game_in_progress() && game_time_get() < network_test.logged_time)
		network_test.logged_time = 0;
	if (game_in_progress() && !main_menu_loaded && game_time_get() - network_test.logged_time >= TICKS_PER_SECOND)
	{
		network_test.logged_time = game_time_get();
		network_test_log_players();
		if (network_test.camera)
			director_load_camera();
		if (network_test.shoot_interval > 0.0f &&
			game_time_get() % MAX(1, (long)(network_test.shoot_interval * TICKS_PER_SECOND)) < TICKS_PER_SECOND)
		{
			network_test_shoot();
		}
		/* (the host brings the players near a second before they shoot;
		not the last while the vehicle test has it) */
		if (network_test.mode == _network_test_host && network_test.shoot_interval > 0.0f &&
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
		/* debug.network_test_chat: each machine's player says a line to
		everyone that many seconds in, one to the team two seconds on, and
		one to the vehicle two more on, the host having seated the last two
		players in one vehicle (the others in none) three seconds before;
		then the host mutes the last player for a minute (a client's /mute
		of the host refused), each says a line to everyone (the last
		player's going no further than the host), the host unmutes it, and
		each says another; then each says a line with a link (refused, as
		network.text_chat "on" refuses links), the host turns the text chat
		off (network.text_chat "off"), and each says a line (refused); then
		the host mutes the last player for everyone, as its scoreboard does,
		and each says a line, and the last player's machine mutes the host's
		for itself, as a client's scoreboard does, and each says another */
		if (network_test.chat_time > 0.0f)
		{
			long chat_time = (long)(network_test.chat_time * TICKS_PER_SECOND);
			char const *machine = network_test.mode == _network_test_host ? "the host" : "a client";
			char text[64];

			if (game_time_get() < chat_time - 3 * TICKS_PER_SECOND)
				network_test.chat_lines_said = 0;
			if (network_test.mode == _network_test_host && network_test.chat_lines_said == 0 &&
				game_time_get() >= chat_time - 3 * TICKS_PER_SECOND)
			{
				network_test_seat_together();
				network_test.chat_lines_said = -1;
			}
			if (network_test.chat_lines_said <= 0 && game_time_get() >= chat_time)
			{
				/* (long enough to wrap in the chat's column) */
				snprintf(text, sizeof(text), "hello everyone from %s, long enough to wrap the column", machine);
				network_chat_say(_chat_channel_everyone, text);
				network_test.chat_lines_said = 1;
			}
			if (network_test.chat_lines_said == 1 && game_time_get() >= chat_time + 2 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "hello team from %s", machine);
				network_chat_say(_chat_channel_team, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 2 && game_time_get() >= chat_time + 4 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "hello vehicle from %s", machine);
				network_chat_say(_chat_channel_vehicle, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 3 && game_time_get() >= chat_time + 6 * TICKS_PER_SECOND)
			{
				char name[16];

				network_test_last_player_name(name, sizeof(name));
				if (network_test.mode == _network_test_host)
					snprintf(text, sizeof(text), "/mute %s 1", name);
				else
					snprintf(text, sizeof(text), "/mute host");
				network_chat_say(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 4 && game_time_get() >= chat_time + 8 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "while muted from %s", machine);
				network_chat_say(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 5 && game_time_get() >= chat_time + 10 * TICKS_PER_SECOND)
			{
				char name[16];

				network_test_last_player_name(name, sizeof(name));
				if (network_test.mode == _network_test_host)
				{
					snprintf(text, sizeof(text), "/unmute %s", name);
					network_chat_say(_chat_channel_everyone, text);
				}
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 6 && game_time_get() >= chat_time + 12 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "after unmute from %s", machine);
				network_chat_say(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 7 && game_time_get() >= chat_time + 14 * TICKS_PER_SECOND)
			{
				/* (a client's sent past its own check, for the host to refuse) */
				snprintf(text, sizeof(text), "join us at discord(dot)gg/halo from %s", machine);
				if (network_test.mode == _network_test_host)
					network_chat_say(_chat_channel_everyone, text);
				else
					network_chat_say_unchecked(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 8 && game_time_get() >= chat_time + 16 * TICKS_PER_SECOND)
			{
				if (network_test.mode == _network_test_host && !config_write("network.text_chat", "off"))
					platform_log("network test: could not turn the text chat off");
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 9 && game_time_get() >= chat_time + 18 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "with the chat off from %s", machine);
				if (network_test.mode == _network_test_host)
					network_chat_say(_chat_channel_everyone, text);
				else
					network_chat_say_unchecked(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
			}
			/* (and on again: config.toml is the executable's folder's, which
			every test of it shares) */
			if (network_test.chat_lines_said == 10 && game_time_get() >= chat_time + 20 * TICKS_PER_SECOND)
			{
				if (network_test.mode == _network_test_host && !config_write("network.text_chat", "on"))
					platform_log("network test: could not turn the text chat on");
				network_test.chat_lines_said++;
			}
			/* (the scoreboard's mutes: the host's of the last player for
			everyone, their lines and voice, then the last player's machine's
			own mute of the host's machine, here alone) */
			if (network_test.chat_lines_said == 11 && game_time_get() >= chat_time + 22 * TICKS_PER_SECOND)
			{
				if (network_test.mode == _network_test_host)
					network_chat_host_mute(network_test_last_player_index(), TRUE);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 12 && game_time_get() >= chat_time + 24 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "after the scoreboard mute from %s", machine);
				network_chat_say(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 13 && game_time_get() >= chat_time + 28 * TICKS_PER_SECOND)
			{
				if (network_test.mode == _network_test_host)
					network_chat_host_mute(network_test_last_player_index(), FALSE);
				else if (network_test_last_player_local())
					network_voice_mute_machine(network_test_first_player_machine(), TRUE);
				network_test.chat_lines_said++;
			}
			if (network_test.chat_lines_said == 14 && game_time_get() >= chat_time + 30 * TICKS_PER_SECOND)
			{
				snprintf(text, sizeof(text), "after the local mute from %s", machine);
				network_chat_say(_chat_channel_everyone, text);
				network_test.chat_lines_said++;
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

	/* the next game of the list: once the scores have been shown a while,
	the host's button (the bots may press it first) */
	if (network_test.mode == _network_test_host && network_test.started && game_engine_running() &&
		!main_menu_loaded && !game_engine_can_score())
	{
		network_test.game_over = TRUE;
		if (game_engine_showing_postgame() && global_network_game_server_get())
		{
			network_test.postgame_seconds += seconds;
			if (network_test.postgame_seconds >= 3.0f && network_test_variant(network_test.variant_index + 1, NULL, 0))
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
		if (network_test_variant(network_test.variant_index + 1, NULL, 0))
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
		if (count > 0 && p2p_join_listing(&games[0]))
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
				char path[128];
				char variant_name[64];
				struct game_variant variant;

				/* (a level of its own path, else a multiplayer map's name) */
				if (strchr(network_test.map_name, '\\'))
					snprintf(path, sizeof(path), "%s", network_test.map_name);
				else
					snprintf(path, sizeof(path), "levels\\test\\%s\\%s", network_test.map_name, network_test.map_name);
				network_test_variant(network_test.variant_index, variant_name, sizeof(variant_name));
				if (!strcmp(variant_name, "coop"))
				{
					/* (the Map screen's co-op: menu_functions.c) */
					extern boolean ui_widget_port_cooperative_level_choose(char const *map_name, short difficulty);

					if (!ui_widget_port_cooperative_level_choose(path, _game_difficulty_level_normal))
						platform_log("network test: %s is not a campaign level, for co-op", path);
					platform_log("network test: game %d, co-op", network_test.variant_index + 1);
				}
				else
				{
					network_game_server_change_map_name(global_network_game_server_get(), path);
					/* the variant, as picking the game settings does */
					variant = *game_engine_get_variant_by_name(&variant, variant_name);
					platform_log("network test: game %d, %s", network_test.variant_index + 1, variant_name);
					/* debug.network_test_score: a short game, to test the next */
					if (network_test.score_to_win > 0)
						variant.universal_variant.score_to_win = network_test.score_to_win;
					player_ui_set_game_variant(&variant);
					network_game_server_change_game_variant(global_network_game_server_get(), &variant);
				}
				network_test.map_set = TRUE;
			}
			if (!network_test.player_added && network_test.setup_seconds >= 2.0f && global_network_game_client_get())
			{
				network_test_set_name();
				network_test.player_added = network_game_client_add_player(global_network_game_client_get(), 0);
			}
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
			network_test_set_name();
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
