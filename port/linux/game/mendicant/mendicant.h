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

/* the settings screens wrote ai.mendicant.* (menu_functions.c's "port
settings save"): the knobs read during play take their new values now; those a
map's start sets up (on, the models, the seed, the bridge, training, the logs)
keep theirs until the next map */
void mendicant_settings_changed(void);

/* a saved game or checkpoint is loaded (game_state.c's after-load procs), the
AI on or off */
void mendicant_game_state_after_load(void);
/* a checkpoint or saved game is written (game_state.c's before-save procs):
no encounter is saved held active by the AI */
void mendicant_game_state_before_save(void);

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
/* (the mixer's thread, port/linux/src/dsound_sdl.c, which declares it itself) */
int mendicant_skip_mixer(void);

#endif
