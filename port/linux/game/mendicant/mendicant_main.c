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

#include "mendicant.h"

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

/* ---------- the members that replace the hook library's raw offsets
(checked with the game's flags: a change upstream fails the build, not the game) */

#define MENDICANT_CHECK(name, type, member, offset) typedef char mendicant_offset_##name[offsetof(struct type, member) == (offset) ? 1 : -1];
MENDICANT_CHECK(a1, actor_datum, meta.type, 0x4)
MENDICANT_CHECK(a2, actor_datum, meta.swarm, 0x6)
MENDICANT_CHECK(a3, actor_datum, meta.active, 0x8)
MENDICANT_CHECK(a4, actor_datum, meta.unit_index, 0x18)
MENDICANT_CHECK(a5, actor_datum, meta.encounter_index, 0x34)
MENDICANT_CHECK(a6, actor_datum, meta.squad_index, 0x3a)
MENDICANT_CHECK(a7, actor_datum, meta.platoon_index, 0x3c)
MENDICANT_CHECK(a8, actor_datum, meta.team_index, 0x3e)
MENDICANT_CHECK(a9, actor_datum, meta.first_prop_index, 0x50)
MENDICANT_CHECK(a10, actor_datum, meta.variant_definition_index, 0x5c)
MENDICANT_CHECK(a11, actor_datum, state.action, 0x6c)
MENDICANT_CHECK(a12, actor_datum, state.combat_status, 0x6e)
MENDICANT_CHECK(a13, actor_datum, state.action_data.fight.firing_position_timer, 0x9c)
MENDICANT_CHECK(a14, actor_datum, state.action_data.uncover.uncover_done, 0x9d)
MENDICANT_CHECK(a15, actor_datum, state.action_data.uncover.pursuit_location.type, 0xa4)
MENDICANT_CHECK(a16, actor_datum, input.vehicle_index, 0x158)
MENDICANT_CHECK(a17, actor_datum, input.vehicle_passenger, 0x160)
MENDICANT_CHECK(a18, actor_datum, input.pathfinding_surface_index, 0x164)
MENDICANT_CHECK(a19, actor_datum, input.pathfinding_point, 0x168)
MENDICANT_CHECK(a20, actor_datum, input.body_vitality, 0x1b8)
MENDICANT_CHECK(a21, actor_datum, input.shield_vitality, 0x1bc)
MENDICANT_CHECK(a22, actor_datum, input.recent_body_damage, 0x1c0)
MENDICANT_CHECK(a23, actor_datum, input.recent_shield_damage, 0x1c4)
MENDICANT_CHECK(a24, actor_datum, external_orders.playfighting, 0x1ca)
MENDICANT_CHECK(a25, actor_datum, external_orders.disable_charging, 0x1cb)
MENDICANT_CHECK(a26, actor_datum, situation.known_enemies, 0x1ec)
MENDICANT_CHECK(a27, actor_datum, situation.visible_reachable_enemies, 0x1ed)
MENDICANT_CHECK(a28, actor_datum, situation.area_friends, 0x200)
MENDICANT_CHECK(a29, actor_datum, situation.close_friends, 0x245)
MENDICANT_CHECK(a30, actor_datum, target.target_type, 0x268)
MENDICANT_CHECK(a31, actor_datum, target.target_prop_index, 0x270)
MENDICANT_CHECK(a32, actor_datum, emotions.forced_to_charge, 0x375)
MENDICANT_CHECK(a33, actor_datum, emotions.berserk, 0x378)
MENDICANT_CHECK(a34, actor_datum, firing_positions.current_position_index, 0x3b8)
MENDICANT_CHECK(a35, actor_datum, orders.move.stationary_crouch, 0x426)
MENDICANT_CHECK(a36, actor_datum, orders.move.moving_crouch, 0x427)
MENDICANT_CHECK(a37, actor_datum, orders.combat.shoot_at_target, 0x454)
MENDICANT_CHECK(a38, actor_datum, orders.combat.override_firing_restrictions, 0x457)
MENDICANT_CHECK(a39, actor_datum, orders.combat.override_burst_duration, 0x458)
MENDICANT_CHECK(a40, actor_datum, orders.combat.throw_grenade, 0x45c)
MENDICANT_CHECK(a41, actor_datum, orders.combat.use_manual_target_point, 0x45d)
MENDICANT_CHECK(a42, actor_datum, orders.combat.target_point, 0x460)
MENDICANT_CHECK(a43, actor_datum, control.path.destination_orders.destination_type, 0x46c)
MENDICANT_CHECK(a44, actor_datum, control.path.path.valid, 0x4a8)
MENDICANT_CHECK(a45, actor_datum, control.moving, 0x504)
MENDICANT_CHECK(a46, actor_datum, control.grenade_trying_to_throw, 0x6a0)
MENDICANT_CHECK(a47, actor_datum, control.grenade_current_target, 0x6a8)
MENDICANT_CHECK(a48, actor_datum, control.grenade_current_aim_vector, 0x6bc)
MENDICANT_CHECK(a49, actor_datum, control.grenade_current_aim_speed, 0x6c8)
MENDICANT_CHECK(p1, prop_datum, next_prop_index, 0x8)
MENDICANT_CHECK(p2, prop_datum, unit_index, 0x18)
MENDICANT_CHECK(p3, prop_datum, actor_index, 0x1c)
MENDICANT_CHECK(p4, prop_datum, enemy, 0x60)
MENDICANT_CHECK(p5, prop_datum, currently_damaging_me, 0x74)
MENDICANT_CHECK(p6, prop_datum, last_perceived_body_position, 0x80)
MENDICANT_CHECK(p7, prop_datum, vehicle_index, 0x110)
MENDICANT_CHECK(p8, prop_datum, distance, 0x11c)
MENDICANT_CHECK(p9, prop_datum, player, 0x12e)
MENDICANT_CHECK(d1, data_array, maximum_count, 0x20)
MENDICANT_CHECK(d2, data_array, size, 0x22)
MENDICANT_CHECK(d3, data_array, valid, 0x24)
MENDICANT_CHECK(d4, data_array, count, 0x2e)
MENDICANT_CHECK(d5, data_array, data, 0x34)
MENDICANT_CHECK(o1, object_header_datum, type, 3)
MENDICANT_CHECK(o2, object_header_datum, datum, 8)
MENDICANT_CHECK(u1, unit_datum, object.position, 12)
MENDICANT_CHECK(u2, unit_datum, object.translational_velocity, 24)
MENDICANT_CHECK(u3, unit_datum, object.up.k, 56)
MENDICANT_CHECK(u4, unit_datum, object.angular_velocity, 60)
MENDICANT_CHECK(u5, unit_datum, object.maximum_body_vitality, 0x88)
MENDICANT_CHECK(u6, unit_datum, object.maximum_shield_vitality, 0x8c)
MENDICANT_CHECK(u7, unit_datum, object.body_vitality, 144)
MENDICANT_CHECK(u8, unit_datum, object.shield_vitality, 148)
MENDICANT_CHECK(u9, unit_datum, object.parent_object_index, 204)
MENDICANT_CHECK(u10, unit_datum, unit.desired_facing_vector, 468)
MENDICANT_CHECK(u11, unit_datum, unit.aiming_vector, 492)
MENDICANT_CHECK(u12, unit_datum, unit.throttle, 552)
MENDICANT_CHECK(u13, unit_datum, unit.current_weapon_index, 674)
MENDICANT_CHECK(u14, unit_datum, unit.weapon_object_indices, 680)
MENDICANT_CHECK(u15, unit_datum, unit.grenade_counts, 718)
MENDICANT_CHECK(u16, unit_datum, unit.current_zoom_level, 720)
MENDICANT_CHECK(u17, unit_datum, unit.driver_object_index, 0x2d4)
MENDICANT_CHECK(u18, unit_datum, unit.gunner_object_index, 0x2d8)
MENDICANT_CHECK(pl1, player_datum, unit_index, 52)
MENDICANT_CHECK(s1, scenario, ai_encounters.count, 0x42c)
MENDICANT_CHECK(s2, scenario, ai_encounters.address, 0x430)
MENDICANT_CHECK(e1, encounter_definition, runtime_structure_bsp_reference_index, 0x7e)
MENDICANT_CHECK(e2, encounter_definition, squads, 0x80)
MENDICANT_CHECK(e3, encounter_definition, firing_positions, 0x98)
MENDICANT_CHECK(q1, squad_definition, firing_position_groups, 0x54)
MENDICANT_CHECK(f1, firing_position_definition, group_index, 0xc)
MENDICANT_CHECK(f2, firing_position_definition, cluster_index, 0xe)
MENDICANT_CHECK(f3, firing_position_definition, surface_index, 0x14)
MENDICANT_CHECK(ed1, encounter_datum, force_active, 0xc)
MENDICANT_CHECK(v1, actor_variant_definition, ranged_combat.projectile_error_angle, 124)
MENDICANT_CHECK(v2, actor_variant_definition, ranged_combat.burst_geometry.burst_origin_radius, 204)
MENDICANT_CHECK(v3, actor_variant_definition, ranged_combat.burst_geometry.burst_duration_lower_bound, 224)
MENDICANT_CHECK(v4, actor_variant_definition, ranged_combat.burst_geometry.burst_separation_lower_bound, 232)
MENDICANT_CHECK(v5, actor_variant_definition, ranged_combat.special_fire_mode, 340)
MENDICANT_CHECK(v6, actor_variant_definition, ranged_combat.special_fire_situation, 342)
MENDICANT_CHECK(v7, actor_variant_definition, ranged_combat.special_fire_chance, 344)
MENDICANT_CHECK(v8, actor_variant_definition, ranged_combat.special_fire_delay, 348)
MENDICANT_CHECK(c1, collision_result, location.cluster_index, 0x10)
MENDICANT_CHECK(c2, collision_result, surface_index, 0x44)
MENDICANT_CHECK(ps1, path_state, input.start_valid, 16)
MENDICANT_CHECK(ps2, path_state, input.start_point, 20)
MENDICANT_CHECK(ps3, path_state, input.start_surface_index, 32)
MENDICANT_CHECK(ps4, path_state, debug, 72)
MENDICANT_CHECK(ps5, path_state, destination_valid, 76)
MENDICANT_CHECK(ps6, path_state, destination.point, 80)
MENDICANT_CHECK(ps7, path_state, destination.surface_index, 92)
MENDICANT_CHECK(ps8, path_state, destination.target_radius, 96)
MENDICANT_CHECK(ps9, path_state, structure, 100)
MENDICANT_CHECK(pr1, path_result, endpoint.point, 4)
MENDICANT_CHECK(pr2, path_result, endpoint.surface_index, 16)
typedef char mendicant_size_path_state[sizeof(struct path_state) == 82060 ? 1 : -1];
typedef char mendicant_size_path_result[sizeof(struct path_result) <= 92 ? 1 : -1];
typedef char mendicant_size_firing_position_definition[sizeof(struct firing_position_definition) == 0x18 ? 1 : -1];
typedef char mendicant_size_encounter_definition[sizeof(struct encounter_definition) == 0xb0 ? 1 : -1];
typedef char mendicant_size_squad_definition[sizeof(struct squad_definition) == 0xe8 ? 1 : -1];

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
and not a client of another's */
static boolean mendicant_host_runs_ai(
	void)
{
	return !game_engine_running() && !network_game_distributed_client();
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
