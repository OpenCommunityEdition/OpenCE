/*
VR.H

The VR mode (HALO_VR; port/linux/README.md, "VR"): what the game's code
(port/linux/game/vr_render.c, render.c, player_control.c), the renderer
(d3d8_gl.c) and the input (xinput_sdl.c) ask of vr.c. Plain C types only:
the game's code includes it as well as the platform layer.

The game draws one view from the centre of the head, its field of view the
union of both eyes', and the renderer draws each of its draws into both eyes
at once (GL_OVR_multiview2), each vertex's clip position moved from the
centre view's to its eye's by a fixed transform per eye (halo_vr_set_eye_
transforms). The HUD and the menus go to a quad layer of their own.

Game axes: forward, left, up (the game's x, y, z at a yaw of 0).
*/

#ifndef __HALO_VR_H
#define __HALO_VR_H

/* the renderer's passes (d3d8_gl.c, halo_vr_pass) */
enum
{
	HALO_VR_PASS_NONE = 0, /* not drawing a VR frame: the flat window */
	HALO_VR_PASS_EYES,     /* the 3D view, into both eyes */
	HALO_VR_PASS_HUD,      /* the HUD and the menus, into the quad layer */
};

struct halo_vr_view
{
	/* the head, from where and how it was at the last recentring (game
	axes, metres) */
	float head_forward[3];
	float head_up[3];
	float head_position[3];
	/* each eye from the head's centre, along the head's axes (metres),
	and its field of view (tangents: left, right, up, down) */
	float eye_offset[2][3];
	float fov[2][4];
	/* the turn of the controllers' stick (radians, the game's yaw) */
	float body_yaw;
	/* the HUD's layer: the tangent of half its vertical field of view, so
	that its camera's projection matches (waypoints, the reticle) */
	float hud_tangent;
	float world_units_per_metre;
	/* the right controller's pointing pose, as the head's (vr.aim =
	"controller": the aim and the first-person weapon follow it) */
	int controller_aim;
	float hand_forward[3];
	float hand_up[3];
	float hand_position[3];
	/* where the weapon's hand is from the eye it is drawn for (metres:
	right, up, forward) */
	float weapon_offset[3];
	/* the first-person hands follow the controllers (vr.hands = "arms" or
	"floating", with vr.aim = "controller"), and arms reach them from the
	shoulders (arms): where each hand is (the controllers' grip poses'
	positions, as hand_position), the left controller's pointing pose (as
	the right's), how far the left hand holds the long gun's foregrip
	(vr.two_handed: 0 to 1, eased), the right trigger's and the left grip's
	pull (0 to 1), and the shoulders from the eyes (vr.shoulders: metres
	out to each side, up, forward) */
	int floating_hands;
	int arms;
	float right_grip_position[3];
	int left_hand_valid;
	float left_hand_forward[3];
	float left_hand_up[3];
	float left_grip_position[3];
	float two_handed;
	float right_trigger;
	float left_grip;
	float shoulder_offset[3];
	/* the time from the last frame's display to this one's (seconds) */
	float seconds;
	/* in a vehicle's seat, the view is the seat's (vr.vehicle_view =
	"first_person"): from the player's head where it sits, else the game's
	camera following the vehicle */
	int vehicle_first_person;
	/* vr.aim = "gamepad": the gamepad aims as on the flat screen, and
	body_yaw turns after the aim (with it or in steps, vr.gamepad_view); the
	first-person weapon and the reticle follow the player's facing */
	int gamepad_aim;
	/* vr.gamepad_view = "camera": the view is pitched with the facing too
	(before the head's turn), as the flat screen's camera is */
	int gamepad_pitch;
};

/* ---------- vr.c */

/* the VR build with vr.enabled */
int halo_vr_enabled(void);
/* the session is open: the game plays in the headset (not flat) */
int halo_vr_running(void);
/* a VR frame is begun and is to be drawn */
int halo_vr_frame_active(void);
/* begins the VR frame before the game's clock is read (main.c): 1 and the
seconds from the last frame's display to this one's when it did */
int halo_vr_frame_clock(float *seconds);
/* the views, located again as late as possible (just before the frame's
draws); 0 without a VR frame */
int halo_vr_view(struct halo_vr_view *view);
/* the player's aim on foot follows the head or the right controller (1,
and yaw, pitch: the game's facing); 2 when the VR mode aims but the facing
stays the game's (vr.aim = "gamepad"); 0 when it does not (no VR frame, a
cinematic). With a long gun, the left hand on its foregrip aims it along
both hands (vr.two_handed). */
int halo_vr_aim(float current_yaw, int long_gun, float *yaw, float *pitch);
/* the aim no longer follows the head (a cinematic, a vehicle's camera
taking over): the next halo_vr_aim starts again from the player's facing */
void halo_vr_aim_release(void);
/* debug.vr_test_hands holds the controllers (vr_hands.c marks them) */
int halo_vr_test_hands(void);

/* the platform layer's (d3d8_gl.c, xinput_sdl.c) */
/* opens the session once the GL context is current; 0 if VR cannot run */
int halo_vr_initialize(void);
void halo_vr_eye_size(int *width, int *height);
void halo_vr_hud_size(int *width, int *height);
/* the swapchain's image for this frame (vr_host.h's VR_SWAPCHAIN_*), a GL
texture: a 2-layer array for the eyes; 0 if there is none */
unsigned int halo_vr_image(int swapchain);
/* the frame is presented with what was drawn (it ends as the next begins,
which halo_vr_frame_active waits for, as vsync would) */
void halo_vr_present(int eyes_drawn, int hud_drawn);
/* the controllers as port 0's gamepad (an XINPUT_GAMEPAD), merged in */
void halo_vr_gamepad(void *gamepad);
void halo_vr_rumble(float left, float right);
/* the menus are up: their layer stays in front of the recentred place,
not of the head */
void halo_vr_menus(int active);
/* the aim follows the head: the controller's magnetism leaves it alone */
int halo_vr_aiming(void);
/* the aim is the gamepad's (vr.aim = "gamepad"), magnetism and all */
int halo_vr_gamepad_aiming(void);
/* whether a gamepad's Back (View) is held, each poll: held a second, it
recentres */
void halo_vr_gamepad_back(int held);

/* the eyes' depth: the projection's range (vr_render.c, in metres) and
that this frame's is in its swapchain (d3d8_gl.c) */
void halo_vr_depth_range(float near_metres, float far_metres);
int halo_vr_depth_wanted(void);
void halo_vr_depth_written(void);

/* ---------- d3d8_gl.c */

void halo_vr_pass(int pass);
/* row-major 4x4s: an eye's clip position from the centre view's, for the
projection the game has just set (rasterizer_set_frustum_z) */
void halo_vr_set_eye_transforms(const float transforms[2][16]);
/* each eye's scale and offset (x, y, then x, y) of the draws in the screen,
from the centre view's (identity: 1, 1, 0, 0), for what the game projects
to the screen itself (vr_render.c, vr_render_screen_point) */
void halo_vr_set_screen_corrections(const float corrections[2][4]);
/* the draws from now on go into that eye's layer alone (0 left, 1 right),
or both at once (-1, multiview) */
void halo_vr_draw_eye(int eye);
/* the sky is being drawn (vr_render.c): with depth clamping, not clipped */
void halo_vr_sky(int sky);

#endif
