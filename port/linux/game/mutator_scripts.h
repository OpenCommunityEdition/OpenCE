/* A mutator's Lua script, and the view it asks for (mutator_scripts.c). */
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

/* whether a script has claimed a control this frame (input.claim, by
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

#endif
