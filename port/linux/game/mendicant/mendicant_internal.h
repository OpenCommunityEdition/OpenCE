/* mendicant_internal.h: the declarations between MENDICANT AI's modules
(port/linux/game/mendicant; docs/mendicant_port.md). Nothing here is used by
the game's sources, which include mendicant.h alone. Include after cseries.h
and math/real_math.h. */
#ifndef HALO_MENDICANT_INTERNAL_H
#define HALO_MENDICANT_INTERNAL_H

#include <stdio.h>

#include "mendicant.h"

/* ---------- the platform's part (mendicant_main.c, mendicant_config.c,
mendicant_bridge.c) and what it calls in the others */

struct actor_datum;
struct charge_state_data;

#define MENDICANT_MAX_ACTORS 1024
#define MENDICANT_MAX_ENCOUNTERS 256
#define MENDICANT_MAX_SQUAD 32
#define MENDICANT_MAX_PLAYERS HALO_PORT_MAXIMUM_NETWORK_PLAYERS /* the players array (players.c) */
#define MENDICANT_UNIT_KEYS (MENDICANT_MAX_ACTORS + MENDICANT_MAX_ENCOUNTERS)
#define TEAM_PLAYER 1
#define TEAM_HUMAN 2
#define TEAM_COVENANT 3
#define TEAMS 4
#define MODE_GAME 0  /* the game's own AI */
#define MODE_DRILL 1 /* the scripted drill */
#define MODE_MODEL 2 /* the models */
#define BRIDGE_MAGIC 0x41494231
#define BRIDGE_CANDIDATES 24
#define BRIDGE_FEATURES 10
#define BRIDGE_TEAM_FEATURES 8
#define PLAN_FEATURES 16
#define PLAN_EXTRA 32
#define CHIEF_FEATURES 32
#define ENTITIES 32
#define ENTITY_FEATURES 12
#define BRIDGE_TIMEOUT_TICKS 30 /* a model a second late loses the decision to the drill */

/* The model bridge: the request the command layer writes and the answer the
models give, field for field ai_brain.c's (magic 0x41494231, 34948 bytes), so
trainer.py and brain.py (64-bit) read it unchanged. Packed to 4: the game's
-malign-double would move step_mask (mendicant_bridge.c checks the offsets).
The game publishes request_sequence last; the model answers with
response_sequence last. long is 32 bits in every build (ILP32). */
#pragma pack(push, 4)
struct model_bridge
{
	unsigned long magic;
	volatile unsigned long request_sequence;
	volatile unsigned long response_sequence;
	long time;
	long encounter;
	long member_count;
	real team[BRIDGE_TEAM_FEATURES];
	short candidate_count[MENDICANT_MAX_SQUAD];
	short choice[MENDICANT_MAX_SQUAD]; /* candidate index; candidate 0 is staying put */
	real features[MENDICANT_MAX_SQUAD][BRIDGE_CANDIDATES][BRIDGE_FEATURES];
	short positions[MENDICANT_MAX_SQUAD][BRIDGE_CANDIDATES]; /* so two enemies never share one */
	long team_index; /* the side the request is for */

	/* training: the trainer sets these, then reloads the map */
	short team_mode[TEAMS];
	unsigned long seed;
	long episode_ticks;
	/* the game fills these in when the fight is over, then counts the episode */
	real damage_taken[TEAMS];
	long actors_seen[TEAMS];
	long actors_alive[TEAMS];
	long end_time;
	volatile unsigned long episodes;

	/* the AI Master Chief (left out: kept for the layout; chief_actor -1) */
	long chief_wanted;
	long chief_actor;
	real chief_position[3];
	real chief_vitality;
	long chief_state[8];

	/* the strategic model: the game asks for a drill for an Elite's unit, the model picks one */
	real plan_features[PLAN_FEATURES];
	long plan_request;
	long plan_drill;
	long plan_side;
	long plan_tempo;
	/* how this unit's last plan went, given with its next request */
	long plan_has_outcome;
	real plan_outcome_own;
	real plan_outcome_enemy;
	/* its unit's personality, its memory of its last plans, and the enemy it faces */
	real plan_extra[PLAN_EXTRA];

	/* the AI Chief's own model (left out: kept for the layout) */
	volatile unsigned long chief_request_sequence;
	volatile unsigned long chief_response_sequence;
	real chief_features[CHIEF_FEATURES];
	long chief_intent;
	long chief_has_outcome;
	real chief_outcome_damage;
	real chief_outcome_kills;
	real chief_outcome_body;
	real chief_style[3];

	/* the scenario director (scenario_mode 1): the trainer sets a fight up on a fresh map while
	the game is paused, then asks for the episode to start; it is timed from that moment */
	long scenario_mode;
	volatile unsigned long setup_request; /* trainer: the setup is in place */
	volatile unsigned long setup_ack;     /* game: the episode has started */
	long pause_tick;                      /* trainer: pause the game at this clock after a load */
	volatile unsigned long map_loads;     /* game: counts clock restarts */
	volatile long paused_at;              /* game: the clock it paused at, -1 until then */
	long episode_start_time;              /* game */
	long difficulty;                      /* trainer: 0 easy .. 3 legendary, -1 as it is */
	real vitality_scale[TEAMS][2];        /* trainer: body and shield multipliers per team, 0 is 1 */
	real aggression[TEAMS];               /* trainer: (not applied) */
	long actor_flags[TEAMS];              /* trainer: per-actor behaviour bits */
	real chief_start[3];                  /* trainer: (the AI Chief: not applied) */
	long chief_start_valid;
	long chief_hold;
	real chief_vitality_scale;
	long wipe_end_ticks;                  /* trainer: end once a side has been gone this long, 0 never */
	long allow_count;                     /* trainer: the encounters of the scenario, 0 = no strays */
	short allow[64];
	volatile unsigned long stray_mask[8]; /* game: encounters with living actors not in the scenario */
	volatile long knobbed[TEAMS];         /* game: actors given the scenario's knobs */
	volatile long living[TEAMS];          /* game: living actors per team now */
	volatile long end_reason;             /* game: 0 time, 1 Covenant gone, 2 humans gone, 3 level reset */
	unsigned long scenario_id;            /* trainer: echoed in logs */

	/* game: how the command went this episode */
	volatile long chains_started, chains_completed, chains_failed;
	volatile long solo_samples, member_samples, leash_corrections;

	/* a plan request is a new strategy (kind 0) or the next step of one (kind 1), within the
	steps the doctrine allows (step_mask, bit STEPS for "done"); the answer is plan_step */
	long plan_kind;
	unsigned long long step_mask;
	long plan_step;

	/* game: what discipline declined this episode: panics, berserks, charges, lone pursuits */
	volatile long discipline_declined[4];

	/* with each plan request, who is around the unit (the rest zero) */
	real plan_entities[ENTITIES][ENTITY_FEATURES];
	/* training only (never an input): the step the nearest opposing unit is running, -1 none */
	long enemy_step_label;
	/* model: with a request of soldiers on their own, each one's way of fighting */
	short stance[MENDICANT_MAX_SQUAD];
};
#pragma pack(pop)

/* every knob, read at each map's start (mendicant_config.c) */
struct mendicant_config
{
	boolean enabled, training, discipline, targeting, individuals, aim_difficulty, aim_moving,
		aim_debug, vehicle_test, models;
	long training_skip, seed;
	real aim, marine_fire, command_radius, variety;
	char bridge[256], weights[128], sample_log[256], drill_log[256], player_log[256], move_test_log[256];
};

/* ---- platform: mendicant_config.c */
extern struct mendicant_config mendicant_config;
boolean mendicant_config_read(void);     /* TRUE when anything changed since the last read */

/* ---- platform: mendicant_bridge.c */
extern struct model_bridge *mendicant_bridge; /* NULL: the drill alone (no models, no shared memory) */
void mendicant_bridge_open(void);        /* each map: shared memory, the in-process struct, or none */
boolean mendicant_bridge_shared(void);   /* a trainer or brain.py answers (desktop Linux) */
/* a request is complete (written by the command layer): request_sequence
published, then answered now in-process, or in training waited for (200 ms) */
void mendicant_bridge_submit(void);
boolean mendicant_bridge_answered(void); /* response_sequence == request_sequence */

/* ---- platform: mendicant_main.c */
extern boolean mendicant_active;         /* enabled, campaign or co-op, and this machine runs the AI */
extern boolean mendicant_training;       /* ai.mendicant.training with the shared-memory bridge */
extern boolean mendicant_episode_over;
extern FILE *mendicant_drill_file;       /* NULL unless ai.mendicant.drill_log */
extern FILE *mendicant_move_test_file;   /* NULL unless ai.mendicant.move_test_log */
long mendicant_time(void);               /* game_time_get() */
long mendicant_player_unit(void);        /* absolute player 0's unit, or NONE (the models' "Chief") */
int mendicant_player_units(long *units, int maximum); /* every living player's unit, local and remote (co-op) */
long mendicant_player_unit_nearest(real_point3d const *point); /* the nearest living player's unit, else player 0's */
void mendicant_training_tally(long actor_index, struct actor_datum *actor); /* sample_actor's tally */

/* ---- what the platform calls in the other modules */
/* M1 runtime (mendicant_models.c) */
boolean mendicant_runtime_load(void);
void mendicant_runtime_dispose(void);
boolean mendicant_runtime_ready(void);
void mendicant_runtime_answer(struct model_bridge *bridge);
/* M2 command (mendicant_command.c); the discipline bodies behind the source
hooks mendicant_veto_panic, _veto_berserk, _action_charge_setup and
_handle_lost_contact (charge setup and lost contact call the originals) */
boolean mendicant_discipline_veto_panic(long actor_index);
boolean mendicant_discipline_veto_berserk(long actor_index);
boolean mendicant_discipline_charge_setup(long actor_index, short goal, struct charge_state_data *state_data);
boolean mendicant_discipline_lost_contact(long actor_index);
void mendicant_command_reset(void);
void mendicant_command_episode_reset(unsigned long seed);
void mendicant_command_episode_report(struct model_bridge *bridge);
void mendicant_orders_clear_all(void);
void mendicant_discipline_before_decide(long actor_index);
void mendicant_drill_update(void);
/* M3 movement (mendicant_move.c) */
void mendicant_movement_reset(void);
void mendicant_movement_revert(void);
void mendicant_movement_dispose(void);
void mendicant_movement_before_save(void);
short mendicant_encounter_own_position_count(long encounter_index);
void mendicant_movement_tick_begin(void);
void mendicant_movement_tick_end(void);
short mendicant_select_position(long actor_index,
	struct firing_position_evaluation_context *evaluation_context,
	struct firing_position *best_firing_position, long *current_owner,
	struct path_state *area_path_state, boolean *area_path_state_valid);
void mendicant_guard_change(long actor_index, short *firing_position_index,
	long *previous_owner, boolean *cached_path_available);
boolean mendicant_go_redirect(short kind, long actor_index, real_point3d const *point, boolean *result);
void mendicant_move_test(long actor_index);
void mendicant_movement_report(struct model_bridge *bridge);
/* M4 combat (mendicant_combat.c, mendicant_vehicles.c) */
void mendicant_combat_reset(void);
void mendicant_aim_actor(long actor_index);
void mendicant_aim_restore(void);
void mendicant_apply_role(long actor_index);
real mendicant_targeting_scale(long actor_index, long prop_index, real weight);
boolean mendicant_grenade_lands_on_friends(struct actor_datum *actor);
void mendicant_grenade_throw_check(long actor_index, boolean throw_ordered_before); /* mendicant_grenade_throw_checked's body */
void mendicant_vehicles_update(void);
void mendicant_vehicle_try(void);

/* M1 runtime (mendicant_models.c) needs from M2 command: the doctrine as
the decision mask reads it (mendicant_doctrine.h stays private to M2) */
int mendicant_doctrine_chains(void);          /* CHAINS (24) */
int mendicant_doctrine_step_choices(void);    /* STEPS + 1 (33) */
void mendicant_doctrine_chain_covenant(unsigned char *chain_covenant); /* 1 for a Covenant chain */
unsigned long mendicant_doctrine_hash(void);  /* FNV-1a, export_models.py's doctrine_hash text */

/* ---------- what the modules share */

struct unit_datum;
struct firing_position_definition;

#ifndef MENDICANT_MAX_POSITIONS
#define MENDICANT_MAX_POSITIONS 512
#endif
#ifndef MENDICANT_MAX_UNITS
#define MENDICANT_MAX_UNITS 64
#endif
#ifndef STANCES
#define STANCES 5 /* the ways of fighting a soldier's own model chooses among (bridge->stance) */
#endif
/* mendicant_unit_side's sides (the doctrine's SIDE_COVENANT and SIDE_MARINES) */
#define MENDICANT_SIDE_COVENANT 0
#define MENDICANT_SIDE_MARINES 1
/* actor_type_definitions maps each type to its behaviour: two types each are Elites and Marines */
#define MENDICANT_ACTOR_IS_ELITE(type) ((type) == 0 || (type) == 5)
#define MENDICANT_ACTOR_IS_MARINE(type) ((type) == 6 || (type) == 7)

/* each actor's part in its unit's drill, which sets how it moves and fires */
enum { ROLE_NONE, ROLE_SUPPORT, ROLE_FLANKER, ROLE_ASSAULT, ROLE_HIDDEN, ROLE_BAIT,
	ROLE_SECURITY, ROLE_OVERWATCH, ROLE_SHIFT_FIRE, ROLE_GRENADIER, ROLE_RUNNER, ROLE_MARKSMAN, ROLES };
/* the cover the next position searches prefer (mendicant_cover_want) */
enum { MENDICANT_COVER_NONE = 0, MENDICANT_COVER_FIGHTING, MENDICANT_COVER_CONCEALED };
/* a unit's elements */
enum { ELEMENT_BASE, ELEMENT_MANEUVER, ELEMENT_RESERVE, ELEMENTS };

struct mendicant_member { long actor_index; struct actor_datum *actor; };
/* a step's reference: the target (or a stand-in), and the axis from it to the unit's middle */
struct mendicant_geometry { real_point3d const *target; real axis_x, axis_y; };

/* ---- M2 command (mendicant_command.c): what the other modules use */
extern long order_actor[MENDICANT_MAX_ACTORS];
extern short order_position[MENDICANT_MAX_ACTORS];
extern char actor_role[MENDICANT_MAX_ACTORS];
extern unsigned char actor_grenade_due[MENDICANT_MAX_ACTORS];
extern real actor_leash[MENDICANT_MAX_ACTORS][4];
extern real actor_face[MENDICANT_MAX_ACTORS][3];
extern real unit_assault_range[MENDICANT_MAX_ACTORS];
extern long commanded_time[MENDICANT_MAX_ACTORS];
extern long leash_corrections;
extern boolean bridge_pending;
int mendicant_team_mode(short team);      /* MODE_GAME / MODE_DRILL / MODE_MODEL */
int mendicant_side_of(short team);
int mendicant_unit_side(struct actor_datum *actor);   /* MENDICANT_SIDE_COVENANT / _MARINES */
boolean mendicant_actor_can_drill(struct actor_datum *actor);
void mendicant_order(long actor_index, short position); /* releases a go order by drill or follow first */
void mendicant_set_role(long actor_index, int role);
unsigned long mendicant_drill_rand(void);
boolean mendicant_bridge_waiting(void);  /* a request is out and not answered */
/* candidates, team features, then mendicant_bridge_submit (individuals use it too) */
void mendicant_bridge_request(long key, struct mendicant_member *members, int count,
	real_point3d const *target, real axis_x, real axis_y, boolean commanded, long time);
/* (mendicant_units.c) */
extern real position_reach_centre[3], position_reach; /* a Covenant defender's post, around a step's orders */
void mendicant_centre_of(struct mendicant_member *members, int count, int element, real_point3d *centre);

/* the drill's sides (mendicant_doctrine.h has the same enum; the command layer
includes that, so it sees these as MENDICANT_SIDE_* only) */
#ifndef MENDICANT_COMMAND_PRIVATE
enum { SIDE_COVENANT = MENDICANT_SIDE_COVENANT, SIDE_MARINES = MENDICANT_SIDE_MARINES };
#endif
#define ACTOR_IS_ELITE(type) MENDICANT_ACTOR_IS_ELITE(type)
#define ACTOR_IS_MARINE(type) MENDICANT_ACTOR_IS_MARINE(type)
#define ACTOR_IS_GRUNT(type) ((type) == 2 || (type) == 14)
#define ACTOR_IS_JACKAL(type) ((type) == 1)

/* ---- what M2 calls in M3 movement (mendicant_move.c) */
void mendicant_maneuver_check_map(void);
void mendicant_follow_chief(long time);
void mendicant_go_release_drill(long actor_index);
void mendicant_cover_want(int cover, real_point3d const *threat); /* a NULL threat keeps the last one */
struct firing_position_definition *mendicant_encounter_firing_positions(long encounter_index, int *count);
boolean mendicant_firing_position_owned(long encounter_index, short position);
boolean mendicant_position_usable(long encounter_index, short position, struct firing_position_definition const *entry);
boolean mendicant_drill_move(struct mendicant_member *member, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);
void mendicant_drill_hold(struct mendicant_member *member, long *used);
boolean mendicant_assign_position(struct mendicant_member *member, short position);
boolean mendicant_go_fallback(struct mendicant_member *member, real_point3d const *point, real tolerance);
int mendicant_maneuver_slot(long encounter_index, real_point3d const *point);
int mendicant_maneuver_near(real_point3d const *point, real radius);
real_point3d const *mendicant_maneuver_pool_point(int candidate);
boolean mendicant_within_reach(real_point3d const *point);
int mendicant_cover_score(real_point3d const *position, real_point3d const *threat);
real mendicant_cover_bonus(real_point3d const *position);

/* ---- what M2 calls in M4 combat (mendicant_combat.c) */
void mendicant_individuals_update(long time);
void mendicant_apply_stance(long actor_index, int stance); /* 1..STANCES-1: its role; else no role */
int mendicant_weapon_class(struct unit_datum *unit);
boolean mendicant_carries_plasma_pistol(struct unit_datum *unit);

/* ---- M3 movement (mendicant_move.c): the rest of its services */
enum { GO_STATUS_NONE = 0, GO_STATUS_MOVING, GO_STATUS_ARRIVED, GO_STATUS_YIELDING,
	GO_STATUS_NO_GROUND = -1, GO_STATUS_NO_PATH = -2, GO_STATUS_GONE = -3 };
int actor_go_to(long actor_index, real_point3d const *point, real tolerance, int urgency);
int actor_go_status(long actor_index);
void actor_go_cancel(long actor_index);
void mendicant_follow_cancel(long actor_index);
int mendicant_line_blocked(real_point3d const *from, real_point3d const *to); /* -1 no BSP, 0 clear, 1 blocked */
int mendicant_drill_position(struct actor_datum *actor, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);

/* ---- M4 combat: the rest of its services */
void mendicant_vehicles_reset(void);
boolean mendicant_carries_assault_rifle(struct unit_datum *unit);

/* ---------- the command layer's own (M2: mendicant_command.c and
mendicant_units.c, which define MENDICANT_COMMAND_PRIVATE): what its two units
share. */
#ifdef MENDICANT_COMMAND_PRIVATE

/* ---- M2 between its two units */
extern signed char actor_team_of[MENDICANT_MAX_ACTORS];
extern unsigned char actor_mover[MENDICANT_MAX_ACTORS];
extern long enrolled_actor[MENDICANT_MAX_ACTORS];
extern short enrolled_key[MENDICANT_MAX_ACTORS];
extern boolean bridge_lost_last;
real mendicant_command_actor_distance_squared(struct actor_datum *a, struct actor_datum *b);
struct firing_position_definition *mendicant_command_member_positions(struct actor_datum *actor, int *count);
void mendicant_command_clear_member_orders(struct mendicant_member *members, int count);
void mendicant_units_update(int key, struct mendicant_member *members, int count, long time);
void mendicant_units_plan_answer(int key, struct mendicant_member *members, int count, long time);
long mendicant_units_leader(int key);
boolean mendicant_units_leaderless(int key);
void mendicant_units_found(int key, long leader, boolean leaderless);
void mendicant_units_inherit(int new_key, int key, long leader, boolean leaderless);
boolean mendicant_units_stand_down(int key);
void mendicant_units_retry(int key, long time);
int mendicant_units_element_size(struct mendicant_member *members, int count, int element);
void mendicant_units_reset(void);
void mendicant_units_episode_reset(void);
void mendicant_units_report(struct model_bridge *bridge);

#endif /* MENDICANT_COMMAND_PRIVATE */

#endif
