/*
MENDICANT_MAIN.C

MENDICANT AI's calls from the game's sources (mendicant.h), and what runs
around them: when the AI runs, each map's start and end, the game tick, each
actor's decision, and in training the episodes (the scenario director's
side: the trainer's knobs on each actor, the living counted, deaths by
vitality, the end of a fight, the difficulty).

The AI runs only when ai.mendicant.enabled is set, the game is a campaign or
co-op game (no multiplayer game engine) and this machine is not a client of
another's game: mendicant_active. Otherwise every call here does nothing, or
exactly what the stock call would.

This is the hook library's ai_brain.c (its constructor, decide wrappers,
game_tick wrappers and training code) with each address, patch and raw
offset replaced by the source's own functions, hooks and struct members.
The AI Master Chief and theater mode are left out: in training, a scenario
fight with chief.present runs as a fight without a Chief, and whole-level
episodes end at episode_ticks with the fight at the level's start.
*/

#include "cseries.h"
#include "memory/data.h"
#include "math/real_math.h"
#include "ai/ai.h"
#include "ai/actors.h"
#include "ai/actions.h"
#include "ai/actor_types.h"
#include "ai/props.h"
#include "ai/encounters.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/actor_definitions.h"
#include "ai/path.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "networking/network_game_globals.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "physics/collisions.h"

#include <stdio.h>
#include <string.h>

#include "mendicant_internal.h"

/* port/linux/src/platform.h */
void platform_log(char const *format, ...);

/* ---------- constants */

/* the training bridge, its runs and the logs: desktop Linux only */
#if !defined(HALO_WINDOWS) && !defined(HALO_ANDROID) && !defined(HALO_WEB)
#define MENDICANT_DESKTOP 1
#endif

#define MENDICANT_SAMPLE_TICKS 15   /* 2 samples a second at 30 ticks a second */
#define MENDICANT_ALIVE_TICKS 300   /* sampled in the last 10 seconds: still alive */
#define MENDICANT_ACTOR_TYPE_SLOTS 12
#define MENDICANT_PLAYER_NEAR 30.0f /* the Covenant counted near the player */

/* training_skip's bits */
enum
{
	_mendicant_skip_frame_bit = 1,
	_mendicant_skip_sound_bit = 2,
	_mendicant_skip_mixer_bit = 4,
};

/* the scenario's end_reason */
enum
{
	_mendicant_end_time = 0,
	_mendicant_end_covenant_gone,
	_mendicant_end_humans_gone,
	_mendicant_end_level_reset,
};

/* actor_flags' bits (the trainer's per-actor behaviour) */
enum
{
	_mendicant_flag_disable_charging = 1,
	_mendicant_flag_forced_to_charge = 2,
	_mendicant_flag_berserk = 4,
	_mendicant_flag_playfighting = 8,
};


/* ---------- the actor types (actor_types.c:93-104), in the hook library's
order: the sample log's type column */

extern struct actor_type_definition actor_type_elite;
extern struct actor_type_definition actor_type_jackal;
extern struct actor_type_definition actor_type_grunt;
extern struct actor_type_definition actor_type_hunter;
extern struct actor_type_definition actor_type_engineer;
extern struct actor_type_definition actor_type_marine;
extern struct actor_type_definition actor_type_crew;
extern struct actor_type_definition actor_type_flood;
extern struct actor_type_definition actor_type_infection;
extern struct actor_type_definition actor_type_carrier;
extern struct actor_type_definition actor_type_sentinel;
extern struct actor_type_definition actor_type_mounted_weapon;

static struct actor_type_definition const *const mendicant_actor_type_slots[MENDICANT_ACTOR_TYPE_SLOTS] =
{
	&actor_type_elite,
	&actor_type_jackal,
	&actor_type_grunt,
	&actor_type_hunter,
	&actor_type_engineer,
	&actor_type_marine,
	&actor_type_crew,
	&actor_type_flood,
	&actor_type_infection,
	&actor_type_carrier,
	&actor_type_sentinel,
	&actor_type_mounted_weapon,
};

/* the next map's difficulty (main.c:712, in no header) */
extern short global_difficulty_level;

/* actions.c:393, in no header (declared as there) */
boolean actor_action_handle_lost_contact(
	long actor_index);

/* ---------- platform (port/linux/src/posix_mendicant.c) */

#ifdef MENDICANT_DESKTOP
void posix_mendicant_sleep_microseconds(unsigned long microseconds);
#endif

/* ---------- globals */

boolean mendicant_active = FALSE;
boolean mendicant_training = FALSE;
boolean mendicant_episode_over = FALSE;
FILE *mendicant_drill_file = NULL;
FILE *mendicant_move_test_file = NULL;

/* this map's modules were reset for the AI (L1 found it should run) */
static boolean mendicant_map_ready = FALSE;
/* the movement module has the tick (maneuver slots swapped in) */
static boolean mendicant_tick_began = FALSE;

/* the logs, and the paths they were opened with */
static FILE *mendicant_sample_file = NULL;
static FILE *mendicant_player_file = NULL;
#ifdef MENDICANT_DESKTOP
static char mendicant_sample_path[sizeof(mendicant_config.sample_log)];
static char mendicant_drill_path[sizeof(mendicant_config.drill_log)];
static char mendicant_player_path[sizeof(mendicant_config.player_log)];
static char mendicant_move_test_path[sizeof(mendicant_config.move_test_log)];
#endif
static long mendicant_player_sample_time = -1000;

/* each episode's tally, for the score: by absolute actor index */
static long mendicant_last_sample_time[MENDICANT_MAX_ACTORS];
static long mendicant_actor_sampled[MENDICANT_MAX_ACTORS]; /* the full actor index last sampled in each slot */
static char mendicant_actor_team[MENDICANT_MAX_ACTORS];
static real mendicant_team_damage[TEAMS];
static long mendicant_episode_last_time = -1;

/* the scenario director's side */
static short mendicant_first_sight[MENDICANT_MAX_ACTORS];
static long mendicant_side_seen_time[2];
static long mendicant_side_had[2];

/* ---------- private code: the game's data */

/* an actor by its absolute index (the slot the hook library walked) */
static struct actor_datum *mendicant_actor_at(
	short absolute_index)
{
	return (struct actor_datum *)((byte *)actor_data->data + absolute_index * actor_data->size);
}

/* an actor by its datum index, or NULL (also for a stale one) */
static struct actor_datum *mendicant_actor_get(
	long actor_index)
{
	if (!actor_data || !actor_data->valid || actor_index == NONE)
		return NULL;
	return (struct actor_datum *)datum_try_and_get(actor_data, actor_index);
}

/* an object's datum, or NULL */
static struct object_datum *mendicant_object_get(
	long object_index)
{
	struct object_header_datum *header;

	if (object_index == NONE || !object_header_data || !object_header_data->valid)
		return NULL;
	header = (struct object_header_datum *)datum_try_and_get(object_header_data, object_index);
	return header ? header->datum : NULL;
}

/* the AI runs here: a campaign or co-op game (no multiplayer game engine),
not a client of another's, and not a film played back (the models' seed comes
from the clock, so a replay would not play as it was recorded) */
static boolean mendicant_host_runs_ai(
	void)
{
	return !game_engine_running() && !network_game_distributed_client() &&
		game_connection() != _game_connection_film_playback;
}

/* ---------- private code: the sample and player logs */

/* sample_actor's tally: at most twice a second per actor; TRUE when this
call counted it */
static boolean mendicant_sample_tally(
	long actor_index,
	struct actor_datum *actor)
{
	short index = (short)actor_index;
	long time;
	short team;

	if (!actor || index < 0 || index >= MENDICANT_MAX_ACTORS)
		return FALSE;

	time = game_time_get();
	if (mendicant_last_sample_time[index] && time >= mendicant_last_sample_time[index] &&
		time - mendicant_last_sample_time[index] < MENDICANT_SAMPLE_TICKS)
	{
		return FALSE;
	}
	mendicant_last_sample_time[index] = time;
	mendicant_actor_sampled[index] = actor_index;

	team = actor->meta.team_index;
	if (team >= 0 && team < TEAMS)
	{
		mendicant_actor_team[index] = (char)team;
		mendicant_team_damage[team] += actor->input.recent_body_damage + actor->input.recent_shield_damage;
	}

	return TRUE;
}

/* what each enemy knew and what it chose, before its decision */
static void mendicant_sample_actor(
	short type,
	long actor_index)
{
	struct actor_datum *actor = mendicant_actor_get(actor_index);

	if (!mendicant_sample_tally(actor_index, actor) || !mendicant_sample_file)
		return;

	fprintf(mendicant_sample_file,
		"%ld,%d,%d,%d,%ld,%d,%d,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d,%ld,%d,%d,%d\n",
		game_time_get(), (int)(short)actor_index, (int)type,
		(int)actor->meta.team_index,
		actor->meta.encounter_index,
		(int)actor->meta.squad_index,
		(int)actor->meta.platoon_index,
		actor->input.pathfinding_point.x, actor->input.pathfinding_point.y, actor->input.pathfinding_point.z,
		actor->input.body_vitality, actor->input.shield_vitality,
		actor->input.recent_body_damage, actor->input.recent_shield_damage,
		(int)(signed char)actor->situation.known_enemies,
		(int)(signed char)actor->situation.visible_reachable_enemies,
		(int)(signed char)actor->situation.area_friends,
		(int)(signed char)actor->situation.close_friends,
		(int)actor->target.target_type,
		actor->target.target_prop_index,
		(int)actor->firing_positions.current_position_index,
		(int)actor->state.action, /* before this decision */
		(int)actor->state.combat_status);
	fflush(mendicant_sample_file); /* a crash or kill must not lose the samples */

	return;
}

/* how a real Master Chief moves and fights: absolute player 0, twice a second */
static void mendicant_player_sample(
	void)
{
	long time = game_time_get();
	struct player_datum *player;
	struct unit_datum *unit;
	struct object_datum *weapon = NULL;
	short weapon_slot;
	long near = 0;
	long aware = 0;

	if (time >= mendicant_player_sample_time && time - mendicant_player_sample_time < MENDICANT_SAMPLE_TICKS)
		return;
	mendicant_player_sample_time = time;

	if (!player_data || !player_data->valid || player_data->count < 1)
		return;
	player = (struct player_datum *)player_data->data;
	if (!player->identifier)
		return;
	unit = (struct unit_datum *)mendicant_object_get(player->unit_index);
	if (!unit)
		return;

	/* the Covenant near you, and how many of them know of you */
	if (actor_data && actor_data->valid)
	{
		short index;

		for (index = 0; index < actor_data->count; index++)
		{
			struct actor_datum *actor = mendicant_actor_at(index);
			real x, y;

			if (!actor->identifier || actor->meta.team_index != TEAM_COVENANT)
				continue;
			x = actor->input.pathfinding_point.x - unit->object.position.x;
			y = actor->input.pathfinding_point.y - unit->object.position.y;
			if (x * x + y * y < MENDICANT_PLAYER_NEAR * MENDICANT_PLAYER_NEAR)
			{
				near++;
				aware += (signed char)actor->situation.known_enemies > 0;
			}
		}
	}
	weapon_slot = unit->unit.current_weapon_index;
	if (weapon_slot >= 0 && weapon_slot < 4)
		weapon = mendicant_object_get(unit->unit.weapon_object_indices[weapon_slot]);
	fprintf(mendicant_player_file, "%ld,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%ld,%d,%d,%d,%d,%ld,%ld,%d\n",
		time, (int)global_structure_bsp_index,
		unit->object.position.x, unit->object.position.y, unit->object.position.z,
		unit->object.translational_velocity.i, unit->object.translational_velocity.j, unit->object.translational_velocity.k,
		unit->object.body_vitality, unit->object.shield_vitality,
		unit->unit.aiming_vector.i, unit->unit.aiming_vector.j, unit->unit.aiming_vector.k,
		unit->unit.throttle.i, unit->unit.throttle.j, /* movement input */
		weapon ? weapon->definition_index : NONE,  /* weapon's tag */
		(int)(signed char)unit->unit.grenade_counts[0], (int)(signed char)unit->unit.grenade_counts[1],
		(int)(signed char)unit->unit.current_zoom_level,
		unit->object.parent_object_index != NONE, /* in a vehicle */
		near, aware, (int)(unit->object.body_vitality <= 0.0f));
	fflush(mendicant_player_file);

	return;
}

#ifdef MENDICANT_DESKTOP
/* the logs' paths are the host's, as the hook library's were (the game's
fopen, port/linux/include/stdio.h, takes Xbox paths under the data root) */
#undef fopen

/* a log at path, opened once per path ("" closes it) */
static void mendicant_log_open(
	FILE **file,
	char *opened_path,
	size_t opened_size,
	char const *path,
	char const *header)
{
	if (*file && !strcmp(opened_path, path))
		return;
	if (*file)
	{
		fclose(*file);
		*file = NULL;
	}
	opened_path[0] = 0;
	if (!path[0])
		return;
	*file = fopen(path, "w");
	if (!*file)
	{
		platform_log("mendicant: cannot open the log %s", path);
		return;
	}
	strncpy(opened_path, path, opened_size - 1);
	opened_path[opened_size - 1] = 0;
	if (header)
	{
		fputs(header, *file);
		fflush(*file);
	}

	return;
}
#endif

static void mendicant_logs_open(
	void)
{
#ifdef MENDICANT_DESKTOP
	mendicant_log_open(&mendicant_sample_file, mendicant_sample_path, sizeof(mendicant_sample_path),
		mendicant_config.sample_log,
		"time,actor,type,team,encounter,squad,platoon,x,y,z,body,shield,"
		"recent_body_damage,recent_shield_damage,known_enemies,visible_enemies,"
		"area_friends,close_friends,target_type,target,firing_position,action,"
		"combat_status\n");
	mendicant_log_open(&mendicant_player_file, mendicant_player_path, sizeof(mendicant_player_path),
		mendicant_config.player_log,
		"time,part,x,y,z,velocity_x,velocity_y,velocity_z,body,shield,aim_x,aim_y,aim_z,"
		"move_forward,move_left,weapon_tag,frag_grenades,plasma_grenades,zoom,in_vehicle,"
		"covenant_near,covenant_aware,dead\n");
	mendicant_log_open(&mendicant_drill_file, mendicant_drill_path, sizeof(mendicant_drill_path),
		mendicant_config.drill_log, NULL);
	mendicant_log_open(&mendicant_move_test_file, mendicant_move_test_path, sizeof(mendicant_move_test_path),
		mendicant_config.move_test_log, NULL);
#endif

	return;
}

/* ---------- private code: training's episodes */

static void mendicant_episode_start(
	void)
{
	struct model_bridge *bridge = mendicant_bridge;

	memset(mendicant_last_sample_time, 0, sizeof(mendicant_last_sample_time));
	memset(mendicant_actor_sampled, 0xff, sizeof(mendicant_actor_sampled));
	memset(mendicant_actor_team, -1, sizeof(mendicant_actor_team));
	memset(mendicant_team_damage, 0, sizeof(mendicant_team_damage));
	/* the command layer's part: orders, units, plan memory, roles, enrolment,
	leashes, discipline, its counters and its random numbers */
	mendicant_command_episode_reset(bridge->seed);
	mendicant_episode_over = FALSE;
	/* the AI Chief is left out: his reports stay "none" */
	bridge->chief_actor = NONE;
	memset(bridge->chief_state, 0, sizeof(bridge->chief_state));
	/* the map starts every fight the same way; the trainer's seed makes each one differ */
	*get_global_random_seed_address() ^= bridge->seed;
	*get_global_local_random_seed_address() ^= bridge->seed * 2654435761UL;

	return;
}

static void mendicant_episode_end(
	long time)
{
	struct model_bridge *bridge = mendicant_bridge;
	short team;
	short index;

	for (team = 0; team < TEAMS; team++)
	{
		bridge->damage_taken[team] = mendicant_team_damage[team];
		bridge->actors_seen[team] = 0;
		bridge->actors_alive[team] = 0;
	}
	for (index = 0; index < MENDICANT_MAX_ACTORS; index++)
	{
		struct actor_datum *still;
		struct object_datum *body = NULL;

		team = mendicant_actor_team[index];
		if (team < 0 || team >= TEAMS)
			continue;
		bridge->actors_seen[team]++;
		/* alive: its body still has life in it (an actor that stopped deciding, out of an
		active encounter, is not dead: counting "not sampled lately" as dead gave 12-15 false
		human deaths a whole-level fight, data/marine_research) */
		still = mendicant_actor_sampled[index] != NONE ? mendicant_actor_get(mendicant_actor_sampled[index]) : NULL;
		if (still)
			body = mendicant_object_get(still->meta.unit_index);
		bridge->actors_alive[team] += (body && body->object.body_vitality > 0.0f) ||
			(!still && time - mendicant_last_sample_time[index] <= MENDICANT_ALIVE_TICKS);
	}
	bridge->end_time = time;
	mendicant_movement_report(bridge);      /* the drill log's movement line */
	mendicant_command_episode_report(bridge); /* chains, samples, leashes, discipline */
	mendicant_episode_over = TRUE;
	mendicant_orders_clear_all();
	__sync_synchronize();
	bridge->episodes++;

	return;
}

static void mendicant_scale_vitality(
	long unit_index,
	real body,
	real shield)
{
	struct object_datum *unit = mendicant_object_get(unit_index);

	if (!unit)
		return;
	if (body > 0.0f)
		unit->object.maximum_body_vitality *= body;
	if (shield > 0.0f)
		unit->object.maximum_shield_vitality *= shield;

	return;
}

/* Every tick of a scenario's episode: each actor gets the knobs the first time it is seen (those
the level's scripts or ai_spawn_actor add later too), the living are counted, strays found. */
static void mendicant_scenario_actors(
	long time)
{
	struct model_bridge *bridge = mendicant_bridge;
	long living[TEAMS];
	unsigned long strays[8];
	long side_living[2];
	long player_unit = mendicant_player_unit();
	short index;
	short team;
	short side;

	memset(living, 0, sizeof(living));
	memset(strays, 0, sizeof(strays));
	for (index = 0; index < actor_data->count && index < MENDICANT_MAX_ACTORS; index++)
	{
		struct actor_datum *actor = mendicant_actor_at(index);
		short identifier = actor->identifier;
		long unit_index;
		struct object_datum *unit;
		long encounter;
		long flags;

		if (!identifier)
			continue;
		unit_index = actor->meta.unit_index;
		unit = mendicant_object_get(unit_index);
		team = actor->meta.team_index;
		if (team < 0 || team >= TEAMS || !unit || unit->object.body_vitality <= 0.0f)
			continue;
		encounter = actor->meta.encounter_index;
		if (unit_index == player_unit)
			continue; /* the player cannot die: a side is the squads, not him */
		living[team]++;
		if (bridge->allow_count > 0 && encounter != NONE)
		{
			long slot = encounter & 0xffff;
			boolean allowed = FALSE;
			long entry;

			for (entry = 0; entry < bridge->allow_count && entry < 64; entry++)
				allowed |= bridge->allow[entry] == slot;
			if (!allowed && slot < 256)
				strays[slot / 32] |= 1UL << (slot % 32);
		}
		if (mendicant_first_sight[index] == identifier)
			continue;
		mendicant_first_sight[index] = identifier;
		mendicant_scale_vitality(unit_index, bridge->vitality_scale[team][0], bridge->vitality_scale[team][1]);
		flags = bridge->actor_flags[team];
		if (flags & _mendicant_flag_disable_charging)
			actor->external_orders.disable_charging = TRUE;
		if (flags & _mendicant_flag_forced_to_charge)
			actor->emotions.forced_to_charge = TRUE;
		if (flags & _mendicant_flag_berserk)
			actor->emotions.berserk = TRUE;
		if (flags & _mendicant_flag_playfighting)
			actor->external_orders.playfighting = TRUE;
		bridge->knobbed[team]++;
	}
	for (team = 0; team < TEAMS; team++)
		bridge->living[team] = living[team];
	memcpy((void *)bridge->stray_mask, strays, sizeof(strays));

	/* the sides: Covenant, and the humans (Marines, and the player's team) */
	side_living[0] = living[TEAM_COVENANT];
	side_living[1] = living[TEAM_HUMAN] + living[TEAM_PLAYER];
	for (side = 0; side < 2; side++)
	{
		if (side_living[side])
		{
			mendicant_side_seen_time[side] = time;
			mendicant_side_had[side] = 1;
		}
	}
	if (bridge->wipe_end_ticks > 0)
	{
		for (side = 0; side < 2 && !mendicant_episode_over; side++)
		{
			if (mendicant_side_had[side] && time - mendicant_side_seen_time[side] >= bridge->wipe_end_ticks)
			{
				mendicant_episode_end(time);
				bridge->end_reason = side + 1;
			}
		}
	}

	return;
}

/* the trainer's difficulty for this fight, on the level just loaded and on the next */
static void mendicant_apply_difficulty(
	void)
{
	struct model_bridge *bridge = mendicant_bridge;

	if (bridge->difficulty >= _game_difficulty_level_easy && bridge->difficulty <= _game_difficulty_level_impossible)
	{
		game_difficulty_level_set((short)bridge->difficulty);
		global_difficulty_level = (short)bridge->difficulty;
	}

	return;
}

static void mendicant_scenario_update(
	long time,
	boolean restarted)
{
	struct model_bridge *bridge = mendicant_bridge;

	if (restarted)
	{
		/* a fresh map: no episode until the trainer has set the fight up. A level that goes back
		to a checkpoint in the middle of a fight ends it, as "level reset". */
		if (!mendicant_episode_over)
		{
			mendicant_episode_end(time);
			bridge->end_reason = _mendicant_end_level_reset;
		}
		bridge->paused_at = -1;
		bridge->setup_ack = bridge->setup_request;
		mendicant_episode_over = TRUE;
		mendicant_apply_difficulty();
	}
	if (bridge->setup_request != bridge->setup_ack)
	{
		mendicant_episode_start();
		bridge->episode_start_time = time;
		bridge->end_reason = _mendicant_end_time;
		memset(mendicant_first_sight, 0, sizeof(mendicant_first_sight));
		memset((void *)bridge->knobbed, 0, sizeof(bridge->knobbed));
		memset(mendicant_side_had, 0, sizeof(mendicant_side_had));
		mendicant_side_seen_time[0] = mendicant_side_seen_time[1] = time;
		__sync_synchronize();
		bridge->setup_ack = bridge->setup_request;
		return;
	}
	if (mendicant_episode_over)
	{
		if (bridge->pause_tick > 0 && bridge->paused_at < 0 && time >= bridge->pause_tick)
		{
			game_time_set_speed(0.0f);
			bridge->paused_at = time;
		}
		return;
	}
	if (time - bridge->episode_start_time >= bridge->episode_ticks)
	{
		mendicant_episode_end(time);
		bridge->end_reason = _mendicant_end_time;
		return;
	}
	mendicant_scenario_actors(time);

	return;
}

/* every tick after the AI's update, in training: a clock that went back is
a new map (or a checkpoint), and starts or ends the episode */
static void mendicant_training_update(
	void)
{
	struct model_bridge *bridge = mendicant_bridge;
	long time = game_time_get();
	boolean restarted = time < mendicant_episode_last_time || mendicant_episode_last_time < 0;

	mendicant_episode_last_time = time;
	if (restarted)
		bridge->map_loads++; /* the trainer's sign that the level it asked for is up */
	if (bridge->scenario_mode)
	{
		mendicant_scenario_update(time, restarted);
		return;
	}
	if (restarted)
	{
		mendicant_apply_difficulty(); /* whole-level fights: each its own, not the last scenario's */
		mendicant_episode_start();
	}
	if (!mendicant_episode_over && time >= bridge->episode_ticks)
		mendicant_episode_end(time);

	return;
}

/* ---------- public code: for the other modules */

long mendicant_time(
	void)
{
	return game_time_get();
}

/* absolute player 0's unit (not the first valid player), or NONE */
long mendicant_player_unit(
	void)
{
	struct player_datum *player;

	if (!player_data || !player_data->valid || player_data->count < 1)
		return NONE;
	player = (struct player_datum *)player_data->data;
	return player->identifier ? player->unit_index : NONE;
}

int mendicant_player_units(
	long *units,
	int maximum)
{
	int found = 0;
	short index;

	if (!player_data || !player_data->valid)
		return 0;
	for (index = 0; index < player_data->count && found < maximum; index++)
	{
		struct player_datum *player = (struct player_datum *)((byte *)player_data->data + index * player_data->size);
		struct object_datum *unit;

		if (!player->identifier || player->unit_index == NONE)
			continue;
		unit = mendicant_object_get(player->unit_index);
		if (!unit || unit->object.body_vitality <= 0.0f)
			continue;
		units[found++] = player->unit_index;
	}
	return found;
}

long mendicant_player_unit_nearest(
	real_point3d const *point)
{
	long units[MENDICANT_MAX_PLAYERS];
	int count = mendicant_player_units(units, MENDICANT_MAX_PLAYERS);
	long nearest = NONE;
	real best = 0.0f;
	int index;

	if (count == 0)
		return mendicant_player_unit();
	for (index = 0; index < count; index++)
	{
		struct object_datum *unit = mendicant_object_get(units[index]);
		real dx = unit->object.position.x - point->x, dy = unit->object.position.y - point->y;
		real dz = unit->object.position.z - point->z;
		real distance = dx * dx + dy * dy + dz * dz;

		if (nearest == NONE || distance < best)
		{
			nearest = units[index];
			best = distance;
		}
	}
	return nearest;
}

void mendicant_training_tally(
	long actor_index,
	struct actor_datum *actor)
{
	mendicant_sample_tally(actor_index, actor);

	return;
}

/* ---------- public code: a map's start and end (ai.c) */

void mendicant_initialize_for_new_map(
	void)
{
	/* ai_handle_editing (the editor) starts the map again without ending it:
	the level's own firing position lists go back before the copies are freed */
	if (mendicant_map_ready)
		mendicant_movement_dispose();
	mendicant_config_read();
	mendicant_active = FALSE;
	mendicant_map_ready = FALSE;
	mendicant_tick_began = FALSE;
	mendicant_training = FALSE;
	if (!mendicant_config.enabled || !mendicant_host_runs_ai())
		return;

	/* each module's state for a new map: the hook library's constructor state
	(nobody enrolled, no orders, no grenade waits), no go orders, no graph, no
	maneuver slots */
	mendicant_command_reset();
	mendicant_movement_reset();
	mendicant_combat_reset();
	/* the models: loaded once, again when the file changes */
	if (mendicant_config.models)
		mendicant_runtime_load();
	else if (mendicant_runtime_ready())
		mendicant_runtime_dispose();
	mendicant_bridge_open();
	mendicant_training = mendicant_config.training && mendicant_bridge_shared();
	if (mendicant_config.training && !mendicant_training)
		platform_log("mendicant: training needs the model bridge (ai.mendicant.bridge), not training");
	mendicant_logs_open();

	mendicant_map_ready = TRUE;
	mendicant_active = TRUE;

	return;
}

void mendicant_dispose_from_old_map(
	void)
{
	if (mendicant_map_ready)
	{
		/* first: the level's own firing position lists back before its tags go */
		mendicant_movement_dispose();
	}
	mendicant_active = FALSE;
	mendicant_map_ready = FALSE;
	mendicant_tick_began = FALSE;

	return;
}

/* A loaded game state (a checkpoint revert, a saved game, a core): actors may
hold firing position indices of our maneuver slots, beyond the level's own
lists, from this process or the one that saved. They are flushed whether or not
the AI is on, so the game never reads past a list (a stock state has none). With
the AI on, its orders, units and timers are from another time and start over;
the maneuver copies stay. */
void mendicant_game_state_after_load(
	void)
{
	short index;

	if (actor_data && actor_data->valid && global_scenario)
	{
		for (index = 0; index < actor_data->count; index++)
		{
			struct actor_datum *actor = mendicant_actor_at(index);
			short own, guard = NONE, flee = NONE;

			if (!actor->identifier || actor->meta.encounter_index == NONE)
				continue;
			own = mendicant_encounter_own_position_count(actor->meta.encounter_index);
			if (actor->state.action == _actor_action_guard && actor->state.action_data.guard.guard_location_type == 3)
				guard = actor->state.action_data.guard.guard_firing_position_index;
			if (actor->state.action == _actor_action_flee)
				flee = actor->state.action_data.flee.flee_firing_position_index;
			if (actor->firing_positions.current_position_index >= own || guard >= own || flee >= own)
				actor_flush_position_indices(
					(long)(((unsigned long)(unsigned short)actor->identifier << 16) | (unsigned short)index));
		}
	}
	if (mendicant_map_ready)
	{
		mendicant_command_reset();
		mendicant_combat_reset();
		mendicant_movement_revert();
	}

	return;
}

/* A checkpoint or saved game is written (between ticks): the encounters this
module holds force_active are let go first, so a state loaded later (the AI off,
or in another process) has none held by nobody. The go orders hold them again
at the next tick's start, before the AI updates. */
void mendicant_game_state_before_save(
	void)
{
	if (mendicant_map_ready)
		mendicant_movement_before_save();

	return;
}

/* ---------- public code: the game tick (game_time.c, game.c) */

void mendicant_game_tick_begin(
	void)
{
	/* a co-op join can make this machine a client of another's game */
	mendicant_active = mendicant_map_ready && mendicant_host_runs_ai();
	if (!mendicant_active)
		return;
	/* the whole tick (scripts, AI, objects) runs with the longer position lists;
	free movement's orders are served first */
	mendicant_movement_tick_begin();
	mendicant_tick_began = TRUE;

	return;
}

void mendicant_game_tick_end(
	void)
{
	if (!mendicant_tick_began)
		return;
	/* level loads, resets and saves, between ticks, see only the level's own lists */
	mendicant_movement_tick_end();
	mendicant_tick_began = FALSE;

	return;
}

void mendicant_ai_update_end(
	void)
{
	if (!mendicant_active)
		return;
	mendicant_vehicles_update(); /* the autopilot writes its drivers' controls after the AI's */
	if (mendicant_training)
		mendicant_training_update();

	return;
}

/* ---------- public code: each actor's decision (actor_types.c) */

void mendicant_decide_before(
	long actor_index,
	struct actor_type_definition const *definition)
{
	short type;

	if (!mendicant_active)
		return;
	for (type = 0; type < MENDICANT_ACTOR_TYPE_SLOTS; type++)
	{
		if (mendicant_actor_type_slots[type] == definition)
			break;
	}
	/* (an actor of a type not listed is not sampled: the hook library never saw one) */
	if (type < MENDICANT_ACTOR_TYPE_SLOTS)
		mendicant_sample_actor(type, actor_index);
	mendicant_discipline_before_decide(actor_index);
	mendicant_aim_actor(actor_index);
	if (mendicant_config.vehicle_test)
		mendicant_vehicle_try();
	if (mendicant_player_file)
		mendicant_player_sample();

	return;
}

void mendicant_decide_after(
	long actor_index)
{
	if (!mendicant_active)
		return;
	if (mendicant_move_test_file)
		mendicant_move_test(actor_index);
	mendicant_drill_update();

	return;
}

/* ---------- public code: actions (actions.c) */

void mendicant_action_control_after(
	long actor_index,
	short action)
{
	if (!mendicant_active)
		return;
	/* the roles change how fighting, uncovering and guarding actors move and fire */
	if (action == _actor_action_fight || action == _actor_action_uncover || action == _actor_action_guard)
		mendicant_apply_role(actor_index);

	return;
}

/* actor_action_try_to_panic's entry: TRUE refuses the panic */
boolean mendicant_veto_panic(
	long actor_index)
{
	if (!mendicant_active || !mendicant_config.discipline)
		return FALSE;
	return mendicant_discipline_veto_panic(actor_index);
}

/* actor_berserk(actor, TRUE) at actions.c's two decision sites: TRUE refuses it */
boolean mendicant_veto_berserk(
	long actor_index)
{
	if (!mendicant_active || !mendicant_config.discipline)
		return FALSE;
	return mendicant_discipline_veto_berserk(actor_index);
}

/* in place of action_charge_setup at the melee and close-range charges */
boolean mendicant_action_charge_setup(
	long actor_index,
	short goal,
	struct charge_state_data *state_data)
{
	if (!mendicant_active || !mendicant_config.discipline)
		return action_charge_setup(actor_index, goal, state_data);
	return mendicant_discipline_charge_setup(actor_index, goal, state_data);
}

/* in place of actor_action_handle_lost_contact in combat status and exit pursuit */
boolean mendicant_handle_lost_contact(
	long actor_index)
{
	if (!mendicant_active || !mendicant_config.discipline)
		return actor_action_handle_lost_contact(actor_index);
	return mendicant_discipline_lost_contact(actor_index);
}

/* actor_action_try_to_throw_grenade's end: a throw it just ordered that would
land within the safe radius of the thrower or a living friend is refused */
void mendicant_grenade_throw_checked(
	long actor_index,
	boolean throw_ordered_before)
{
	if (!mendicant_active)
		return;
	mendicant_grenade_throw_check(actor_index, throw_ordered_before);

	return;
}

/* ---------- public code: firing positions and movement */

short mendicant_select_firing_position(
	long actor_index,
	struct firing_position_evaluation_context *evaluation_context,
	struct firing_position *best_firing_position,
	long *current_owner,
	struct path_state *area_path_state,
	boolean *area_path_state_valid)
{
	if (!mendicant_active)
	{
		return actor_active_select_firing_position(actor_index, evaluation_context, best_firing_position,
			current_owner, area_path_state, area_path_state_valid);
	}
	return mendicant_select_position(actor_index, evaluation_context, best_firing_position,
		current_owner, area_path_state, area_path_state_valid);
}

void mendicant_guard_position(
	long actor_index,
	short *firing_position_index,
	long *previous_owner,
	boolean *cached_path_available)
{
	if (!mendicant_active)
		return;
	mendicant_guard_change(actor_index, firing_position_index, previous_owner, cached_path_available);

	return;
}

boolean mendicant_move_redirect(
	short kind,
	long actor_index,
	real_point3d const *point,
	boolean *result)
{
	if (!mendicant_active)
		return FALSE;
	return mendicant_go_redirect(kind, actor_index, point, result);
}

/* ---------- public code: target weights (actor_perception.c) */

real mendicant_target_weight(
	long actor_index,
	long prop_index,
	real weight)
{
	if (!mendicant_active || !mendicant_config.targeting)
		return weight;
	return mendicant_targeting_scale(actor_index, prop_index, weight);
}

/* ---------- public code: training without a picture or sound (main.c) */

/* in training the frame is skipped, and a game with no fight under way (the
fight over, or paused for setup) sleeps a millisecond a frame instead of
spinning */
boolean mendicant_skip_render_frame(
	void)
{
	if (!mendicant_training || !(mendicant_config.training_skip & _mendicant_skip_frame_bit))
		return FALSE;
#ifdef MENDICANT_DESKTOP
	if (mendicant_episode_over) /* (paused_at stays set through the fight that follows a setup pause) */
		posix_mendicant_sleep_microseconds(1000);
#endif
	return TRUE;
}

boolean mendicant_skip_sound_render(
	void)
{
	return mendicant_training && (mendicant_config.training_skip & _mendicant_skip_sound_bit) != 0;
}

/* the mixer's thread without a device (port/linux/src/dsound_sdl.c), each pass:
the same rule as the frame and the sound (training on, through the trainer's
bridge), so a game with training set and no bridge still mixes. (int: the
platform layer has no boolean; mendicant_training is set between maps.) */
int mendicant_skip_mixer(
	void)
{
	return mendicant_training && (mendicant_config.training_skip & _mendicant_skip_mixer_bit) != 0;
}
