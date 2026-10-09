/*
HALO_VR.H

The game's side of stereo rendering in the VR build (HALO_VR;
port/linux/game/vr_render.c, over port/linux/src/vr_frame.c).

A stereo frame renders the one player window three times in render_frame's
window loop: the left and right eyes (each resolved into its swapchain
image as it finishes), then the HUD alone on a transparent ground, which
the headset shows on a layer ahead of the head. While a hand-aimed weapon
is zoomed, a fourth pass between the eyes and the HUD renders the scope's
view along the gun, which the headset shows on a layer held at the gun. The game advances its
per-frame state once: systems that move on as they draw do so for the left
eye only.
*/

#ifndef __HALO_VR_H
#define __HALO_VR_H

#ifdef HALO_VR
boolean vr_render_script_message(char const *message);
void vr_render_reset_vehicle_view(void);
union real_point3d;
boolean vr_script_can_see_point(long unit_index, const union real_point3d *point, real field_of_view, boolean *result);
/* Head-only tutorial directions: up/down/left/right bits 0/1/2/3. */
unsigned int vr_head_look_actions(void);
void vr_head_look_reset(void);

struct render_window;
struct render_camera;
union real_rectangle2d;

enum
{
	_vr_render_pass_none = -1,
	_vr_render_pass_left_eye,
	_vr_render_pass_right_eye,
	_vr_render_pass_hud,
	/* a cutscene's eyes, for the 3D screen (each followed by the console
	window's letterbox, then resolved) */
	_vr_render_pass_cinema_left_eye,
	_vr_render_pass_cinema_right_eye,
	/* the zoomed view along a hand-aimed gun, for its scope (resolved as it
	finishes, before the HUD pass clears the target) */
	_vr_render_pass_scope,
};

/* which pass the window being rendered is */
extern int vr_render_pass;

/* Native HUD draw redirection: -1 hidden, 0 stock fallback, 1 captured.
 * Capture preserves weapon sprite/frame/color evaluation in hud_weapon.c. */
int halo_vr_crosshair_begin(void);
void halo_vr_crosshair_end(void);

#define VR_RENDER_EYE() (vr_render_pass == _vr_render_pass_left_eye || vr_render_pass == _vr_render_pass_right_eye)
#define VR_RENDER_HUD() (vr_render_pass == _vr_render_pass_hud)
/* test26: the HUD tapped away (port/linux/src/vr_frame.c): the player's
state, weapon and waypoints undrawn; prompts, messages and the reticle stay */
int vr_hud_hidden(void);
#define VR_HUD_HIDDEN() vr_hud_hidden()
#define VR_RENDER_SCOPE() (vr_render_pass == _vr_render_pass_scope)
/* a view of the world for the headset alone: no HUD drawn over it, and no
fog screen (which assumes the game's own camera and frustum) */
#define VR_RENDER_VIEW() (VR_RENDER_EYE() || VR_RENDER_SCOPE())
/* the passes after the first, which must not advance per-frame state */
#define VR_RENDER_REPEAT() (vr_render_pass == _vr_render_pass_right_eye || \
	vr_render_pass == _vr_render_pass_hud || vr_render_pass == _vr_render_pass_cinema_right_eye || \
	vr_render_pass == _vr_render_pass_scope)

/* main_game_render: makes a single player window (followed by the console
window) into the eyes, the scope (while zoomed), the HUD and the console
window when this frame is drawn in stereo, or a cutscene's into each eye's
view and console window; returns the window count to render (at most
MAXIMUM_WINDOWS + 1) */
short vr_render_windows(struct render_window *windows, short window_count);
/* the graphics settings (config.toml [graphics]) applied to the game's
effect switches, once a frame (port/linux/game/vr_graphics.c) */
void vr_graphics_apply(void);
/* render_frame, around each window */
void vr_render_window_begin(short window_index);
void vr_render_window_end(short window_index);
/* render_player_frame: the eye's frustum bounds replace the window's */
void vr_render_frustum_bounds(union real_rectangle2d *bounds);
/* the camera the first-person weapon is posed from: the head's in stereo */
void vr_render_weapon_camera(struct render_camera *camera);
/* trigger_create_projectiles: a player's weapon fired a projectile (the
local player's hands feel it) */
void vr_render_weapon_fired(long weapon_index, long player_index);
/* test22: a local player's shot as the engine aimed it (weapons.c): from
`origin`, the aim it was given and the direction it left in (after
player_aim_projectile's turn), whether it left from the hand; logged on
a new weapon and at most every ten seconds, against the reticle */
union real_point3d;
union real_vector3d;
void vr_render_shot_diagnostic(long weapon_index, long player_index, union real_point3d const *origin,
	union real_vector3d const *aimed, union real_vector3d const *shot, int from_hand);
/* first_person_weapon_build_node_matrices: the arms of the first-person
weapon posed for the hand that aims (vr.arms) */
struct real_matrix4x3;
struct animation_graph;
void vr_render_first_person_ik(struct real_matrix4x3 *matrices, struct animation_graph *graph,
    long unit, long weapon, unsigned native_arms);
/* player_control_update: the head aims the first local player (vr.h) */
void vr_player_control_facing(short local_player_index);
/* vr.vehicle_view "first_person": vehicles seen from their seat, which the
director then treats as first person (the player's body unseen) */
int vr_render_first_person_vehicles(void);
/* the first-person weapon is not shown: seen from a driver's or gunner's
seat */
int vr_render_hide_first_person_weapon(void);
int vr_render_first_person_gun_hidden(void);
int vr_render_immersive_cutscene(void);
int vr_render_grab_point(long unit_index, union real_point3d *point);
/* handle_one_player_input: what the headset's gestures ask of the first
local player this frame (VR_RENDER_ACTION_*; vr.h's VR_ACTION_*) */
#define VR_RENDER_ACTION_MELEE 0x1u
#define VR_RENDER_ACTION_FLASHLIGHT 0x2u
#define VR_RENDER_ACTION_CROUCH 0x4u
#define VR_RENDER_ACTION_SWITCH_WEAPON 0x8u
#define VR_RENDER_ACTION_ZOOM 0x10u        /* (vr.diag_zoom_seconds) */
#define VR_RENDER_ACTION_DROP_WEAPON 0x20u /* vr.weapons "physical": let go */
#define VR_RENDER_ACTION_GRAB_WEAPON 0x40u /* vr.weapons "physical": gripped */
/* players.c: the weapon lying nearest `hand` within `radius` (world units),
picked up as the action button would (swapping the gun held when the hands
are full); FALSE when none or the game decides pickups elsewhere */
boolean player_vr_grab_weapon(long player_index, union real_point3d const *hand, real radius);
/* the weapon hand's place in the world (vr_render_hand_origin) and the
world units in a metre, for grabbing */
real vr_render_units_per_metre(void);
/* render_objects.c: the player's own biped drawn in first person (vr.body
"full"), and the bones it is drawn with (posed to the headset; the game's
own are left as they are) */
boolean vr_render_full_body(long object_index);
boolean vr_render_hands_only(void);
struct real_matrix4x3 *vr_render_body_matrices(long object_index);
/* player_control.c, each tick: the hands' blows (vr.melee "impact") */
void vr_render_impact_melee(short local_player_index);
real vr_player_collision_radius(long unit_index, real stock);
real vr_player_sprint_scale(long unit_index);
/* units.c: a blow by the hand along `sweep` from `from`, `scale` as hard;
TRUE when it struck something */
boolean unit_vr_impact_melee(long unit_index, union real_point3d const *from, union real_vector3d const *sweep,
	real scale, boolean weapon_hand, real radius);
unsigned long vr_render_actions(short local_player_index);
/* the first-person weapon is drawn mirrored (held in the left hand):
first_person_weapons.c brackets its drawing with halo_vr_mirror_winding */
int vr_render_first_person_mirrored(void);
void halo_vr_mirror_winding(int mirrored);
/* rasterizer_xbox.c (test22): whether the model part about to be drawn
mirrors (its first node matrix's determinant is negative): while the
winding is turned over, only such parts turn it */
void halo_vr_skinning_mirrored(int mirrored);
/* 1 in a stereo frame, whose eyes see the world unmagnified (a zoom shows
in the scope, or not at all): the first-person weapon stays in view while
zoomed, and the HUD draws no zoom mask */
int vr_render_unzoomed_view(void);
/* biped_update_moving: vr.roomscale moves the first local player's pill
(`position`, `height`, `width` as biped_get_physics_pill gives them) by
where the head walked this tick, as far as the world lets it */
void vr_render_room_scale(long biped_index, union real_point3d *position, real height, real width);
/* motion_sensor_update: the yaw (radians) the first local player's motion
sensor turns with, the head's, not the hand's aim; 0 to leave the
facing's */
int vr_render_motion_sensor_yaw(short local_player_index, real *yaw);
/* the pause menu's VR settings (port/linux/game/vr_menu.c): its widget tags
made as a map's tags load (cache_files.c), and the widget code's calls for
their text (a game data input function) and their changes (event handler
functions, one past the end of each table) */
#define VR_MENU_GAME_DATA_FUNCTION 41
#define VR_MENU_NEXT_FUNCTION 102
#define VR_MENU_PREVIOUS_FUNCTION 103
void vr_menu_tags_loaded(void);
void vr_menu_tags_unloaded(void);
/* Releases the local avatar's filtered draw buffers before map teardown. */
void vr_body_geometry_dispose(void);
/* the text of the widget with that definition, if it is one of the menu's */
boolean vr_menu_setting_text(long definition_tag_index, wchar_t *text, long size);
boolean vr_menu_is_setting(long definition_tag_index);
boolean vr_menu_is_screen(long definition_tag_index);
/* steps the widget's setting by `step` values, if it is one of the menu's */
boolean vr_menu_setting_change(long definition_tag_index, long step);
/* 1 while the head aims: no magnetism dragging the view */
int vr_render_aiming(void);
/* 1 while the right hand aims (vr.aim "hand"): no crosshair on the HUD */
int vr_render_hand_aiming(void);
/* unit_adjust_projectile_ray: where the local player's shots start when
the hand aims in a local game (the hand, unless a wall is in between);
0 to leave the game's camera */
int vr_render_hand_origin(long unit_index, union real_point3d *origin);
/* source/game/aim_assist.c (test21): a player's shot from `position`
turned toward where the camera's line hits, within the weapon's deviation
cone, as player_aim_projectile turns it (no autoaim target); FALSE when
the weapon has no aim assist. For the reticle only: changes nothing */
union real_vector3d;
boolean vr_aim_assist_converge(long player_index, union real_point3d const *camera_position,
	union real_vector3d const *camera_direction, union real_point3d const *position, union real_vector3d *direction);
/* clears the target being drawn to transparent black (the HUD pass) */
void halo_vr_clear_transparent(void);
/* copies the back buffer into an eye's image (port/linux/src/d3d8_gl.c) */
void halo_vr_resolve_eye(int eye);
/* copies the scope's view from the back buffer, the part of the 640x480
screen given, into the scope's image through its sight (vr.h's VR_SCOPE_*;
port/linux/src/d3d8_gl.c) */
void halo_vr_resolve_scope(short x0, short y0, short x1, short y1, int shape);

#else

#define VR_RENDER_EYE() 0
#define VR_RENDER_HUD() 0
#define VR_HUD_HIDDEN() 0
#define VR_RENDER_SCOPE() 0
#define VR_RENDER_VIEW() 0
#define VR_RENDER_REPEAT() 0

#endif

#ifdef HALO_VR
void vr_preview_player_projectile(long player_index, union real_point3d const *position, union real_vector3d *direction);
boolean weapon_vr_preview_primary_ray(long weapon_index, long player_index, union real_point3d *origin, union real_vector3d *direction);
#endif

#endif
