/*
VR.H

The headset (OpenXR) side of the Android VR build (HALO_VR): the host owns
the OpenXR session (port/android/host/host_xr.c); this side drives its
frames from the game loop and draws into its swapchain images. Every
function is a no-op returning 0 when the build or vr.enabled leaves VR
off.
*/

#ifndef __HALO_LINUX_VR_H
#define __HALO_LINUX_VR_H

#ifdef HALO_VR

/* Maximum per-scope translation in the VR menu (metres). */
#define VR_SCOPE_ADJUST_LIMIT_METRES 0.30f

/* 1 once the OpenXR session exists (after the GL context does) */
int vr_active(void);
/* sets up the session; called once the GL context is current */
void vr_initialize(void);
/* takes up the settings changed in play (the pause menu's VR settings),
and counts the changes for those who keep settings (vr_settings_generation) */
void vr_reload_settings(void);
int vr_settings_generation(void);
/* the runtime's name for the headset ("Meta Quest 3", "Oculus Quest2"...);
empty before the session exists */
const char *vr_system_name(void);
/* vr.probe_seconds: dim test colours in each eye before the game starts */
void vr_probe(void);
/* the pixels per unit of the game's 640x480 screen in the headset: its
render targets match the eyes' resolution (d3d8_gl.c); 0 before the
session exists */
int vr_screen_scale(float scale[2]);
/* shows the frame the game drew, read from framebuffer `source` (whose
colour is `texture`) of width x height pixels (row 0 at the top), and ends
the runtime's frame; replaces the flat screen's blit and swap
(D3DDevice_Present). 0 when the headset took no frame (its session is not
running: the app shown in a window, the headset off): the caller then shows
it in the window as a flat build does */
int vr_present(unsigned int source, unsigned int texture, int width, int height);
int vr_crosshair_enabled(void);
int vr_crosshair_ready(void);
void vr_crosshair_source(unsigned int texture, float u0, float v0, float u1, float v1);
/* the headset's controllers as an Xbox pad: 1 with their state when the
session has input focus */
int vr_controller(unsigned int *buttons, float trigger[2], float thumb[4]);

/* ---------- stereo (port/linux/game/vr_render.c)

Halo's axes: +x forward, +y left, +z up, in world units of 10 feet. */

/* begins the runtime's frame (waiting for it) and says whether the game
may draw it in stereo: the session shows it, the head is tracked and
vr.stereo allows it. The answer holds until the frame is presented. */
int vr_stereo_begin(void);
/* eye 0 (left) or 1: its position, forward and up for a viewer at
`position` facing `forward` (only its heading counts: the head gives the
pitch and roll), and its frustum bounds (left, right, bottom, top) for a
90-degree vertical field of view on a screen of the aspect given
(render_camera_build_frustum); 0 outside a stereo frame */
int vr_eye_view(int eye, const float position[3], const float forward[3], float aspect,
	float out_position[3], float out_forward[3], float out_up[3], float bounds[4]);
/* the head between the eyes, as vr_eye_view gives an eye */
int vr_head_view(const float position[3], const float forward[3],
	float out_position[3], float out_forward[3], float out_up[3]);
/* the frustum bounds of the HUD's head-locked panel (vr.hud_distance,
vr.hud_width) seen from the head, as vr_eye_view gives an eye's: the HUD
pass drawn from the head with them places its markers over what they
mark */
void vr_hud_bounds(float aspect, float bounds[4]);
/* copies the eye drawn into framebuffer `source` (width x height, row 0
at the top) into the eye's image */
void vr_resolve_eye(int eye, unsigned int source, int width, int height);
/* cutscenes (vr.cinema_3d): begins the runtime's frame and says whether
this one is drawn as a 3D screen, each eye's image from the game's camera
moved aside by half the eye separation */
int vr_cinema_begin(void);
/* eye 0 or 1 of a cutscene frame: how far to move the camera along its
right (world units) and how far to turn its frustum toward the other eye
(a tangent); 0 outside one */
int vr_cinema_eye(int eye, float *offset_units, float *convergence_tangent);
/* vr.timing: the start (end 0) and end (end 1) of a stereo pass: 0 and 1
the eyes, 2 the HUD, 5 the scope (halo_vr.h's _vr_render_pass_*) */
void vr_pass_mark(int pass, int end);

/* ---------- aiming with the head (port/linux/game/vr_render.c)

The view's heading is the player's, turned by the right stick (vr.snap_turn
or vr.smooth_turn_speed) rather than by the game: the game's facing follows
the head. */

/* begins the runtime's frame and gives the direction the player aims, for
a game whose facing has the yaw given; 0 when the head does not aim (no
stereo this frame). The heading takes up the game's yaw when the game
turned the player itself (a script, a respawn, another pad's stick), or is
`base_heading` (radians) when given: a vehicle's seat, which the view
turns with. Source: -1 native stick, 0 head, 1 configured weapon aim,
2 physical right controller, 3 physical left controller. Returns 0 on lost
selected-controller tracking so the caller retains native facing. */
int vr_aim(float game_yaw, int seated, int hand_may_aim, const float *base_heading, float out_forward[3]);
/* Aim source: -1 native stick, 0 head, 1 weapon, 2 right, 3 left. */
int vr_script_head_view(const float position[3], float out_position[3], float out_forward[3]);
/* 1 while the head aims: magnetism and the right stick leave the view alone */
int vr_aiming(void);
/* the heading the eyes are turned by (Halo's x, y) */
int vr_heading_forward(float out_forward[3]);

/* ---------- aiming with the hand (vr.aim = "hand")

On foot the right controller aims: vr_aim gives its direction (seated,
the head still aims), the left stick moves relative to the head, and the
first-person weapon is posed in the hand. */

/* 1 while the hand aims */
int vr_hand_aiming(void);
/* the hand's aiming ray from where the game's camera is (`position`): its
origin in the world and direction; 0 unless the hand aims */
int vr_hand_ray(const float position[3], float out_origin[3], float out_direction[3]);
/* the camera the first-person weapon's model is posed from, for it to sit
in the right hand; 0 unless the hand aims */
int vr_weapon_view(const float position[3], float out_position[3], float out_forward[3], float out_up[3]);
/* a hand's grip (0 left, 1 right) in the world, seen from where the game's
camera is (`position`); 0 when it is not tracked */
int vr_hand_world(int hand, const float position[3], float out_position[3], float out_forward[3], float out_up[3]);
/* as vr_hand_world, turned by the visible hand's own orientation (vr.hand_*) */
int vr_hand_pose(int hand, const float position[3], float out_position[3], float out_forward[3], float out_up[3]);
/* vr.hand_tracking: 0 body IK, 1 floating hands, 2 floating hands and arms */
int vr_hand_tracking_mode(void);
/* the game's world units per metre (vr.world_scale) */
float vr_units_per_metre(void);

/* ---------- the scope (vr.scope)

While a hand-aimed weapon is zoomed, its view along the gun is rendered
into a pass of its own and shown on a small layer held at the gun, as
through a sight; the eyes stay unzoomed. */

#define VR_SCOPE_ROUND 1   /* a round sight (the pistol's) */
#define VR_SCOPE_SNIPER 2  /* the sniper rifle's wide one, nearer the eye */
#define VR_SCOPE_ROCKET 3  /* the rocket launcher's, on its left side */
/* the scope's camera, from where the game's camera is (`position`): the
hand's aim, rolled with the gun; and the pixels its image wants. 0 when
there is none to draw this frame (vr.scope off, the hand not aiming) */
int vr_scope_view(const float position[3], float out_position[3], float out_forward[3], float out_up[3],
	int *out_pixels);
/* copies the scope's view, the square at x, y of `size` pixels in
`texture` (width x height, row 0 at the top), into the scope's image
through its sight's shape (VR_SCOPE_*); the layer then shows this frame */
void vr_resolve_scope(unsigned int texture, int width, int height, int x, int y, int size, int shape);
/* ---------- gestures (vr_frame.c) */

/* what the gestures ask of the game this frame, each taken once:
VR_ACTION_* */
#define VR_ACTION_MELEE 0x1u          /* a hand swung */
#define VR_ACTION_FLASHLIGHT 0x2u     /* the off hand brought to the head */
#define VR_ACTION_CROUCH 0x4u         /* the head lowered (held while it is) */
#define VR_ACTION_SWITCH_WEAPON 0x8u  /* the grip at a shoulder holster */
/* (0x10 is vr_render.c's diagnostic zoom) */
#define VR_ACTION_DROP_WEAPON 0x20u   /* physical weapons: the gun let go */
#define VR_ACTION_GRAB_WEAPON 0x40u   /* physical weapons: the weapon hand gripped */
unsigned int vr_take_actions(void);
/* vr.weapons "physical" may act this tick: a local game, or
vr.physical_multiplayer (the game says, from vr_render_actions) */
void vr_set_physical_allowed(int allowed);
/* vr.melee "impact": the hands strike what they sweep through, where the
game allows it (not a network game's client, whose blows the host would
not see; the game says, from vr_render_actions); 1 while in effect */
void vr_set_impact_melee_allowed(int allowed);
int vr_impact_melee(void);
/* test21: physical melee is off in network games unless vr.melee_multiplayer */
void vr_set_network_game(int network);
/* test21: seated, either stick click sounds a driver's horn (the crouch control) */
int vr_horn_held(void);
/* a hand's (0 left, 1 right) recent peak speed about the room, in metres a
second, and vr.melee_speed, the speed a blow needs */
float vr_hand_speed(int hand);
float vr_melee_speed(void);
/* vr.fingers: a hand's fingers as they rest on its controller - thumb,
index, middle, ring and little - each 0 open to 1 curled, eased; 0 when off
or the hand is untracked */
int vr_finger_pose(int hand, float curls[4]);
double vr_pose_time(void);
float vr_sprint_effort(void);
void vr_support_near(int near_weapon);
/* the hand holding the weapon: 0 left, 1 right */
int vr_weapon_hand(void);
/* 1 while the gun is held in both hands */
int vr_two_handed(void);
/* physical weapons: the weapon the game has in the player's hands (its
object index, -1 none), each tick, for what the hand holds; and 1 while
the hand holds nothing (a gun put away or let fall: the first-person gun
is not drawn, the hand is free) */
void vr_note_weapon(long weapon_index);
/* test21: per-gun aim adjustment. The held gun's kind (from its tag's name,
each tick: vr_set_gun_class, -1 none) selects vr.aim_<key>_up/_right
(degrees, up to VR_GUN_AIM_LIMIT), which turn the shots, the reticle and
the scope off the gun's own aim; the gun is drawn as before */
enum
{
	VR_GUN_OTHER,
	VR_GUN_PISTOL,
	VR_GUN_PLASMA_PISTOL,
	VR_GUN_ASSAULT_RIFLE,
	VR_GUN_PLASMA_RIFLE,
	VR_GUN_SHOTGUN,
	VR_GUN_SNIPER_RIFLE,
	VR_GUN_ROCKET_LAUNCHER,
	VR_GUN_NEEDLER,
	VR_GUN_FUEL_ROD,
	VR_GUN_FLAMETHROWER,
	VR_GUN_CLASSES
};
#define VR_GUN_AIM_LIMIT 10.0f
/* test23: the Quest's remappable buttons (vr.button_*): an action's
button among VR_BUTTON_SOURCE_* ("hold": switch grenades by holding the
grenade button) */
enum
{
	VR_BUTTON_ACTION_JUMP,
	VR_BUTTON_ACTION_ACTION,
	VR_BUTTON_ACTION_MELEE,
	VR_BUTTON_ACTION_CROUCH,
	VR_BUTTON_ACTION_SWITCH_WEAPON,
	VR_BUTTON_ACTION_GRENADE,
	VR_BUTTON_ACTION_SWITCH_GRENADE,
	/* test26: shows or hides the reticle (the crosshair) for the session;
	it starts shown */
	VR_BUTTON_ACTION_RETICLE,
	VR_BUTTON_ACTIONS
};
enum
{
	VR_BUTTON_SOURCE_NONE,
	VR_BUTTON_SOURCE_A,
	VR_BUTTON_SOURCE_B,
	VR_BUTTON_SOURCE_X,
	VR_BUTTON_SOURCE_Y,
	VR_BUTTON_SOURCE_RIGHT_STICK,
	VR_BUTTON_SOURCE_LEFT_STICK,
	VR_BUTTON_SOURCE_GRIP,
	/* test26: the turning stick held down (it never turns; not while seated) */
	VR_BUTTON_SOURCE_RIGHT_STICK_DOWN,
	VR_BUTTON_SOURCE_HOLD,
	VR_BUTTON_SOURCES
};
const char *vr_button_action_key(int action);
int vr_button_default(int action);
const char *vr_button_source_value(int source);
int vr_button_source_of(const char *value, int fallback);
int vr_gun_class_of_name(const char *name);
/* test24b: the comfort vignette (vr.vignette, vr.vignette_when) */
enum { VR_VIGNETTE_MOVING, VR_VIGNETTE_TURNING, VR_VIGNETTE_ALWAYS };
#define VR_VIGNETTE_FEATHER 0.35f
float vr_vignette_aperture(float strength, float amount, float corner);
int vr_vignette_shown(void);
/* test25: vr.vehicle_tilt, 0 to 1 */
float vr_vehicle_tilt(void);
/* Validated local seat identity from input and stereo rendering. Role is 0
passenger/on foot, 1 driver, 2 gunner. Entry/exit/seat transfer recentres once. */
void vr_vehicle_seat(long unit_index, long vehicle_index, int seat_index, int role);
/* test26: the HUD hidden by its head tap (vr.hud_tap_distance), the
reticle by its button (vr.button_reticle): the session's, both start
shown; menus, prompts and messages always show */
int vr_hud_hidden(void);
/* test29: the HUD page's HUD row */
void vr_set_hud_hidden(int hidden);
int vr_reticle_hidden(void);
void vr_set_gun_class(int kind);
int vr_gun_class(void);
const char *vr_gun_class_label(int kind);
const char *vr_gun_class_key(int kind);
int vr_hand_empty(void);
/* vr.cutscenes "immersive": cutscenes seen around the player from their
camera; the head's yaw in the headset's space (radians, left positive),
for lining a cutscene's camera up with where the head looks; and a moment
of black (a camera's cut), `amount` 0 to 1 */
int vr_cinema_immersive(void);
float vr_head_local_yaw(void);
void vr_view_blink(float amount);
/* ---------- room-scale (vr.roomscale)

Walking about the room walks the player: each tick on foot the player is
moved by how far the head went from where they stood. */

/* this tick's step (Halo's x, y in world units), 1 when there is one to
take; the caller moves the player (as far as the world lets it) and calls
vr_room_moved. 0 when off, untracked, or taking up a new place to stand */
int vr_room_step(float out_step[2]);
void vr_room_moved(void);
/* the player cannot walk this tick (a vehicle, a cutscene, no player):
walking resumes from wherever the head is then */
void vr_room_hold(void);

/* ---------- the menus' laser pointer

The weapon hand points at the screen the menus are on (the flat screen,
or the HUD's panel for the pause menu) as a mouse would: its trigger
clicks, the right B goes back, and a dot shows where it points. */
struct halo_ui_pointer;
/* halo_ui_pointer_update for the headset (port/linux/include/halo_ui_pointer.h) */
int vr_ui_pointer(int menus_active, struct halo_ui_pointer *pointer);
/* a buzz on a hand (0 left, 1 right), scaled by vr.haptics */
void vr_haptic(int hand, float amplitude, float seconds);
/* the player's weapon zoom level (-1 none), for the aim's smoothing */
void vr_set_zoom_level(int zoom_level);
/* World-space impact point and the same camera anchor used by the stereo eyes. */
void vr_set_reticle_world(const float anchor[3], const float hit[3]);

#else

#define vr_active() 0
#define vr_initialize() ((void)0)
#define vr_probe() ((void)0)
#define vr_screen_scale(scale) 0
#define vr_present(source, texture, width, height) 0
#define vr_controller(buttons, trigger, thumb) 0
#define vr_stereo_begin() 0
#define vr_aiming() 0
#define vr_hand_aiming() 0

#endif

#endif
