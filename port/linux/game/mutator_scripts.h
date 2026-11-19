/* A mutator's private native HS program and requested view.
 * Entry scripts: startup, frame, tick, stop; each call has a transition budget.
 * Optional player_tick/player_frame callbacks run once per live player, serially;
 * game_player_current() returns its absolute index (0..127). player_stop runs
 * before a departed/replaced player's row is cleared, including salted reuse.
 * state_get_long/state_get_real(player, field) and state_set_long/state_set_real
 * (player, field, value) provide script-private 128 x 32 typed fields, zeroed on
 * new players; invalid indices read zero and writes are ignored. Long fields can
 * hold booleans/handles; use handle+1 if zero should represent NONE.
 * math_sqrt(real), math_divide(real,real) return zero for invalid/nonfinite or
 * zero-divisor inputs. math_equals_string compares contents; object_root safely
 * follows at most 128 parent handles and returns NONE on invalid/cyclic chains.
 * Host API uses module_function names, typed native HS scalars (no tables).
 *
 * Local frame calls: player_alive(), player_in_vehicle(), player_local(),
 * input_held(string), input_claim(string), input_button(long,boolean),
 * input_jump(), input_crouch(), camera_free(), camera_first_person(),
 * camera_view(real distance, real height, boolean backward, boolean crosshair).
 * game_time() returns ticks, game_host() authority; game_players(long ordinal)
 * enumerates player indices from zero, returning -1 at the end.
 * player_unit_of(player), player_button(player,button), player_crouch(player),
 * player_aim(player). Object handles and player indices are longs; -1 is none.
 * object_position/velocity(handle) and player_aim(player) return success and
 * stash real components in result_x(), result_y(), result_z(). world_ray takes
 * start xyz, displacement xyz, ignored object (-1 for none), returning success
 * and stashing hit xyz plus result_object() (-1 for world geometry).
 * object_parent/alive/kind/tag/acceleration_scale(handle) read object state;
 * object_set_velocity(handle,x,y,z) and object_turn_around(handle) preserve
 * prediction and host-only authority restrictions. kind/tag return strings.
 *
 * model_create(tag,x,y,z,owner) returns a local handle or -1; model_place(h,x,y,z),
 * model_forward(h,x,y,z), model_node(h,node,x,y,z),
 * model_node_from_view(h,node,x,y,z), model_face_view(h), model_exists(h),
 * model_delete_function(h). Handles belong to their creating program.
 * draw_line/from_view use six real coordinates and packed RGB long 0xRRGGBB.
 * settings_real/string/boolean(name) read current settings with zero/empty/false
 * defaults. Returned setting strings remain valid until the program closes.
 * Vector output stash belongs to the program, replaced by its next vector call.
 */
#ifndef MUTATOR_SCRIPTS_H
#define MUTATOR_SCRIPTS_H

/* the scripts of a game: a new set begun, each key ("script.<digest>") added, and
those no longer in it closed. Others stay as they are, between games */
void mutator_scripts_begin(void);
void mutator_scripts_add(char const *key);
void mutator_scripts_commit(void);
short mutator_scripts_keys(char keys[][32], short maximum);

/* each script's frame, once a frame for each local player */
void mutator_scripts_update(short local_player_index);

/* the view the scripts asked for this frame. Only this machine's camera follows
it: the camera used for shooting stays the first-person one */
boolean mutator_view_active(short local_player_index);
boolean mutator_view_backward(short local_player_index);
boolean mutator_view_crosshair(short local_player_index);
real mutator_view_distance(short local_player_index);
real mutator_view_height(short local_player_index);

/* the settings of the scripts running read again, a player having changed them */
void mutator_scripts_settings_changed(void);

/* whether a script has claimed a control this frame (input_claim, by
halo_keyboard.h's name: "DPad Up"), which the game's own uses of it leave to
the script */
boolean mutator_scripts_control_claimed(char const *name);

/* every script's tick(), each game tick on every machine, after the players'
input has gone to their units */
void mutator_scripts_tick(void);
/* the buttons scripts set (input.button): the first local player's, and each
player's as this machine has them, set from what the network brings */
byte mutator_scripts_local_buttons(void);
byte mutator_scripts_player_buttons(short player_index);
void mutator_scripts_note_buttons(short player_index, byte buttons);
/* the lines the scripts drew this frame, into a window from this camera */
void mutator_scripts_render(short local_player_index, real_point3d const *camera_position,
	real_vector3d const *camera_forward, real_vector3d const *camera_up);

/* the objects scripts made (model.create), on this machine alone: posed from this
camera before a window is drawn; never sent, picked up or saved (deleted before
a game is saved, forgotten once one is loaded); and made as they are, whatever
the game type's weapons */
void mutator_scripts_render_begin(short local_player_index, real_point3d const *camera_position,
	real_vector3d const *camera_forward, real_vector3d const *camera_up);
extern short mutator_scripts_model_count;
boolean mutator_scripts_object_local_found(long object_index);
#define mutator_scripts_object_local(object_index) \
	(mutator_scripts_model_count > 0 && mutator_scripts_object_local_found(object_index))
boolean mutator_scripts_creating_model(void);
void mutator_scripts_before_save(void);
void mutator_scripts_after_load(void);

/* Compile against the exact host API, without game callbacks. */
struct mutator_hs_result;
boolean mutator_scripts_validate_source(const char *source, long length,
    struct mutator_hs_result *result);
/* halo --check-script <file.hsc>: 0 when the text compiles, else 1 */
int mutator_scripts_check_text(const char *name, const char *source, long length);

#endif
