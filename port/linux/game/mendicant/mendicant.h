/* mendicant.h: MENDICANT AI's calls from the game's sources (port/linux/game/mendicant)

Every function does nothing, or exactly what the stock call would, unless
ai.mendicant.enabled is set and this machine runs the AI (the host of a
campaign or co-op game). */
#ifndef HALO_MENDICANT_H
#define HALO_MENDICANT_H

struct actor_type_definition;
struct firing_position_evaluation_context;
struct firing_position;
struct path_state;
struct charge_state_data;

/* mendicant_move_redirect's kinds: the five actor_move_* entries */
enum
{
	_mendicant_move_firing_position = 0,
	_mendicant_move_move_position,
	_mendicant_move_prop,
	_mendicant_move_halt,
	_mendicant_move_point,
};

/* a map's start and end (ai.c) */
void mendicant_initialize_for_new_map(void);
void mendicant_dispose_from_old_map(void);

/* the game tick (game_time.c around game_tick, game.c after the host's ai_update) */
void mendicant_game_tick_begin(void);
void mendicant_game_tick_end(void);
void mendicant_ai_update_end(void);

/* every decision of every actor (actor_types.c) */
void mendicant_decide_before(long actor_index, struct actor_type_definition const *definition);
void mendicant_decide_after(long actor_index);

/* actions (actions.c) */
void mendicant_action_control_after(long actor_index, short action);
boolean mendicant_veto_panic(long actor_index);
boolean mendicant_veto_berserk(long actor_index);
boolean mendicant_action_charge_setup(long actor_index, short goal, struct charge_state_data *state_data);
boolean mendicant_handle_lost_contact(long actor_index);
void mendicant_grenade_throw_checked(long actor_index, boolean throw_ordered_before);

/* firing positions (action_fight.c, action_uncover.c: in place of
actor_active_select_firing_position; action_guard.c: before its change) */
short mendicant_select_firing_position(long actor_index,
	struct firing_position_evaluation_context *evaluation_context,
	struct firing_position *best_firing_position, long *current_owner,
	struct path_state *area_path_state, boolean *area_path_state_valid);
void mendicant_guard_position(long actor_index, short *firing_position_index,
	long *previous_owner, boolean *cached_path_available);

/* movement (actor_moving.c): TRUE when an ordered actor went to its
waypoint instead; *result is then what the entry returns */
boolean mendicant_move_redirect(short kind, long actor_index, real_point3d const *point, boolean *result);

/* target weights (actor_perception.c) */
real mendicant_target_weight(long actor_index, long prop_index, real weight);

/* training without a picture or sound (main.c) */
boolean mendicant_skip_render_frame(void);
boolean mendicant_skip_sound_render(void);

/* ---------- between MENDICANT AI's modules: the platform's part
(mendicant_main.c, mendicant_config.c, mendicant_bridge.c) and what it calls in
the others. Nothing here is used by the game's sources. */
#ifndef HALO_MENDICANT_PLATFORM_DECLARATIONS
#define HALO_MENDICANT_PLATFORM_DECLARATIONS

struct actor_datum;

#ifndef MENDICANT_MAX_ACTORS
#define MENDICANT_MAX_ACTORS 1024
#endif
#ifndef MENDICANT_MAX_ENCOUNTERS
#define MENDICANT_MAX_ENCOUNTERS 256
#endif
#ifndef MENDICANT_MAX_SQUAD
#define MENDICANT_MAX_SQUAD 32
#endif
#ifndef MENDICANT_UNIT_KEYS
#define MENDICANT_UNIT_KEYS (MENDICANT_MAX_ACTORS + MENDICANT_MAX_ENCOUNTERS)
#endif
#ifndef TEAM_PLAYER
#define TEAM_PLAYER 1
#endif
#ifndef TEAM_HUMAN
#define TEAM_HUMAN 2
#endif
#ifndef TEAM_COVENANT
#define TEAM_COVENANT 3
#endif
#ifndef TEAMS
#define TEAMS 4
#endif
#ifndef MODE_GAME
#define MODE_GAME 0  /* the game's own AI */
#define MODE_DRILL 1 /* the scripted drill */
#define MODE_MODEL 2 /* the models */
#endif
#ifndef BRIDGE_MAGIC
#define BRIDGE_MAGIC 0x41494231
#endif
#ifndef BRIDGE_CANDIDATES
#define BRIDGE_CANDIDATES 24
#endif
#ifndef BRIDGE_FEATURES
#define BRIDGE_FEATURES 10
#endif
#ifndef BRIDGE_TEAM_FEATURES
#define BRIDGE_TEAM_FEATURES 8
#endif
#ifndef PLAN_FEATURES
#define PLAN_FEATURES 16
#endif
#ifndef PLAN_EXTRA
#define PLAN_EXTRA 32
#endif
#ifndef CHIEF_FEATURES
#define CHIEF_FEATURES 32
#endif
#ifndef ENTITIES
#define ENTITIES 32
#endif
#ifndef ENTITY_FEATURES
#define ENTITY_FEATURES 12
#endif
#ifndef BRIDGE_TIMEOUT_TICKS
#define BRIDGE_TIMEOUT_TICKS 30 /* a model a second late loses the decision to the drill */
#endif

/* The model bridge: the request the command layer writes and the answer the
models give, field for field ai_brain.c's (magic 0x41494231, 34948 bytes), so
trainer.py and brain.py (64-bit) read it unchanged. Packed to 4: the game's
-malign-double would move step_mask (mendicant_bridge.c checks the offsets).
The game publishes request_sequence last; the model answers with
response_sequence last. long is 32 bits in every build (ILP32). */
#ifndef HALO_MENDICANT_MODEL_BRIDGE
#define HALO_MENDICANT_MODEL_BRIDGE
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
#endif

/* every knob, read at each map's start (mendicant_config.c) */
#ifndef HALO_MENDICANT_CONFIG
#define HALO_MENDICANT_CONFIG
struct mendicant_config
{
	boolean enabled, training, discipline, targeting, individuals, aim_difficulty, aim_moving,
		aim_debug, vehicle_test, models;
	long training_skip, seed;
	real aim, marine_fire, command_radius, variety;
	char bridge[256], weights[128], sample_log[256], drill_log[256], player_log[256], move_test_log[256];
};
#endif

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
#ifdef EOF /* (with <stdio.h>) */
extern FILE *mendicant_drill_file;       /* NULL unless ai.mendicant.drill_log */
extern FILE *mendicant_move_test_file;   /* NULL unless ai.mendicant.move_test_log */
#endif
long mendicant_time(void);               /* game_time_get() */
long mendicant_player_unit(void);        /* absolute player 0's unit, or NONE */
void mendicant_training_tally(long actor_index, struct actor_datum *actor); /* sample_actor's tally */

/* ---- what the platform calls in the other modules */
/* M1 runtime (mendicant_runtime.c) */
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
/* M3 movement (mendicant_movement.c, mendicant_maneuver.c, mendicant_positions.c) */
void mendicant_movement_reset(void);
void mendicant_movement_dispose(void);
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
void mendicant_apply_role(long actor_index);
real mendicant_targeting_scale(long actor_index, long prop_index, real weight);
boolean mendicant_grenade_lands_on_friends(struct actor_datum *actor);
void mendicant_grenade_throw_check(long actor_index, boolean throw_ordered_before); /* mendicant_grenade_throw_checked's body */
void mendicant_vehicles_update(void);
void mendicant_vehicle_try(void);

/* M1 runtime (mendicant_models.c) needs from M2 command: the doctrine as
the decision mask reads it (doctrine_table.h stays private to M2) */
#ifndef HALO_MENDICANT_RUNTIME_DOCTRINE
#define HALO_MENDICANT_RUNTIME_DOCTRINE
int mendicant_doctrine_chains(void);          /* CHAINS (24) */
int mendicant_doctrine_step_choices(void);    /* STEPS + 1 (33) */
void mendicant_doctrine_chain_covenant(unsigned char *chain_covenant); /* 1 for a Covenant chain */
unsigned long mendicant_doctrine_hash(void);  /* FNV-1a, export_models.py's doctrine_hash text */
#endif

#endif /* HALO_MENDICANT_PLATFORM_DECLARATIONS */

/* ---------- the command layer's own (M2: mendicant_command.c and
mendicant_units.c, which define MENDICANT_COMMAND_PRIVATE): what its two units
share. What the modules share is in mendicant_internal.h. */
#ifdef MENDICANT_COMMAND_PRIVATE
#ifndef HALO_MENDICANT_COMMAND_DECLARATIONS
#define HALO_MENDICANT_COMMAND_DECLARATIONS

#include "mendicant_internal.h"


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

#endif /* HALO_MENDICANT_COMMAND_DECLARATIONS */
#endif /* MENDICANT_COMMAND_PRIVATE */

#endif
