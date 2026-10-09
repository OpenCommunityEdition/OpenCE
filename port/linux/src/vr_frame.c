/*
VR_FRAME.C

The game loop's side of the OpenXR session (vr.h). The host
(port/android/host/host_xr.c) waits for, begins and ends the runtime's
frames; this side decides what each frame shows and draws it into the
swapchain images, whose GL texture names work in this context as they are.
*/

#ifdef HALO_VR

#include "platform.h"
#include "gl.h"
#include "port_config.h"
#include "vr.h"
#include "vr_alignment.h"

#include "guest_host.h"
#include "halo_android_abi.h"
#include "halo_ui_pointer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* test21: the kinds of gun with their own aim adjustment (VR_GUN_*): the
config key's part and the menu's name */
static const struct { const char *key, *label; } gun_classes[VR_GUN_CLASSES] =
{
	{ "other", "OTHER GUN" },
	{ "pistol", "PISTOL" },
	{ "plasma_pistol", "PLASMA PISTOL" },
	{ "assault_rifle", "ASSAULT RIFLE" },
	{ "plasma_rifle", "PLASMA RIFLE" },
	{ "shotgun", "SHOTGUN" },
	{ "sniper_rifle", "SNIPER RIFLE" },
	{ "rocket_launcher", "ROCKET LAUNCHER" },
	{ "needler", "NEEDLER" },
	{ "fuel_rod", "FUEL ROD" },
	{ "flamethrower", "FLAMETHROWER" },
};

static struct
{
	int initialized, active;
	struct halo_xr_info info;
	GLuint framebuffer;
	/* the runtime's frame begun and not yet ended, and its state */
	int frame_begun;
	struct halo_xr_frame frame;
	/* the eye render size (vr.resolution_scale) */
	int eye_size;
	/* the runtime's recommended eye image (the eye swapchains'
	first size, which host_xr_resize_eyes changes since); vr.fov_mode
	"glasses" (this frame's views cut to it: frame_glasses) and its
	window's half-angles, radians across and down; the runtime's own view
	of each eye last seen, for sizing (fov_known once it has been) */
	unsigned int recommended_width, recommended_height;
	int glasses, frame_glasses;
	float glasses_half[2];
	float runtime_fov[2][4];
	int fov_known;
	/* the eye images' size wanted (choose_eye_size), made at the start of
	the next frame (apply_eye_images), when none is in use */
	unsigned int eye_image_width, eye_image_height;
	int srgb_write_control;
	/* the flat screen (vr.screen_distance, vr.screen_width), metres */
	float screen_distance, screen_width;
	/* the HUD's layer, head-locked (vr.hud_distance, vr.hud_width) */
	float hud_distance, hud_width;
	int stereo_enabled;
	/* world units per metre (vr.world_scale) */
	float units_per_metre;
	/* cutscenes (vr.cutscenes): around the player from the cutscene's camera
	("immersive", vr_cinema_immersive), or as a 3D screen (vr.cinema_*:
	this frame is one) */
	int cinema, cinema_enabled, cinema_immersive;
	float cinema_separation, cinema_convergence, cinema_distance, cinema_width;
	/* what the last frame showed (VR_MODE_*); the screen's place, set where
	the head faced on entering a mode that shows one; the fade from black
	after a change of mode (1 black, 0 none) */
	int mode;
	struct halo_xr_pose screen_pose;
	float fade;
	/* this frame is drawn in stereo; which eyes are in their images */
	int stereo;
	unsigned int eyes_resolved;
	/* the view's heading (radians about Halo's +z); 0 before the head first
	aims or after a recentre */
	float heading, last_aim_yaw;
	int heading_valid, aiming, aiming_last_frame;
	/* vr.snap_turn (radians, 0 for smooth), vr.smooth_turn_speed (radians a second) */
	float snap_turn, smooth_turn_speed;
	/* test24b, comfort: the vignette's strength (vr.vignette, 0 off), what
	shows it (vr.vignette_when), how much of it shows now (eased), and the
	frame's own motion: moving (the stick, the arms' run), turning (smooth),
	and a snap turn's brief pulse */
	float vignette_strength, vignette_amount;
	int vignette_when;
	float comfort_move, comfort_turn, snap_pulse;
	/* test25: vr.vehicle_tilt; what asked for the next recentre (0 the
	system or the headset regaining focus, 1 both sticks, 2 View held); and
	the seat as last logged (-1 none, 0 on foot, 1 seat seen from behind,
	2 seat seen from itself) */
	float vehicle_tilt;
	int recentre_source, seat_logged;
	/* A seat has its own lean origin. Network play does not advance the
	on-foot room-scale origin, so it cannot be reused as the seat's centre. */
	struct {
		int known, index, role, origin_valid, recentre_pending;
		long unit, vehicle;
		float origin[3];
	} vehicle_seat;
	int snap_armed, recentre_held;
	/* vr.aim = "hand": the right controller aims (else the head), and the
	head's and the aim's yaw this frame, for the left stick */
	int hand_aim, hand_aiming, hand_aiming_last_frame, seated;
	float head_yaw, aim_yaw;
	/* the weapon's place relative to the right hand (vr.weapon_offset_*) */
	float weapon_offset[3];
	float alignment_rotation[2][4], alignment_offset[2][3];
	int alignment_grip_aim[2];
	/* test20c: the visible hand's rotation on its controller (vr.hand_*)
	and the one-handed gun's on the aim (vr.weapon_*, mirrored for the left
	hand); separate, so a comfortable hand never tilts the gun */
	float hand_rotation[2][4], weapon_rotation[2][4];
	/* vr.hand_tracking: 0 body IK, 1 floating hands (no arms, any body:
	test20c's meaning, restored in test21), 2 floating hands and arms */
	int hand_tracking;
	/* test20d: left-handed controls (vr.left_handed with vr.mirror_controls
	"auto"): the sticks trade jobs and the face buttons swap hands */
	int controls_mirrored;
	/* the aim this frame: the right controller's, or with both hands on
	the gun the line from the right to the left (vr.two_handed) */
	struct halo_xr_pose aim_pose;
	int two_handed_enabled, two_handed;
	/* the controls (vr.controls): the VR layout or the Xbox pad's; this
	frame's pad as the game is to see it; the View button's hold (seconds,
	for a recentre) and the zoom trigger's state; vr.move_relative */
	int layout_vr;
	unsigned int pad_buttons;
	float pad_trigger[2];
	double view_held;
	int view_recentred, back_pulse, zoom_down;
	int move_relative; /* 0 head, 1 left hand, 2 right hand */
	/* the hand that holds the weapon (vr.left_handed; swapped by bringing
	the palms together and gripping) */
	int weapon_hand;
	/* the gestures: each grip held (with hysteresis), each hand's last
	place, and their thresholds and states (vr.melee_speed,
	vr.flashlight_distance, vr.crouch_height, vr.holsters, vr.two_handed) */
	int grip_held[2], grip_pressed[2], grip_released[2], hands_last_valid;
	/* vr.weapons "physical": the gun held only while gripped (gun_held),
	dropped when no hand grips it; allowed by the game (a local game, or
	vr.physical_multiplayer: vr_set_physical_allowed). The X button's
	press for grenades: how long it is held, and whether a hold switched.
	The trigger pulled on a gun not gripped (a faint tick, and the first few
	times a line in the log) */
	int physical_setting, physical_allowed, gun_held, x_hold_switched;
	/* physical weapons: what the weapon hand holds (HAND_*), the weapon the
	game has in it (vr_note_weapon), a change of it this side asked for and
	what the hand then holds (pending_state, NONE none; until pending_until),
	and how long a gun the game put in the hand has gone ungripped */
	int hand_state, pending_state;
	long noted_weapon;
	double pending_until, now_seconds;
	float ungripped_seconds;
	int ungripped_warned;
	/* vr.melee "impact": the hands strike what they sweep through (the
	game's side, vr_render.c), where the game allows it; each hand's
	recent peak speed (vr_hand_speed) */
	int melee_impact, impact_allowed;
	float hand_speed[2];
	/* vr.arm_run: running by swinging the arms; the effort it takes
	(vr.arm_run_speed) and the push on the stick it makes, eased */
	int arm_run;
	float arm_run_speed, run_push;
	/* each hand's fingers (vr_finger_pose): thumb, index, middle, the ring
	and little fingers, 0 open to 1 curled, eased; vr.fingers; how long the
	"L" (pointing, the thumb up) has been held, for the middle finger */
	int fingers;
	float finger_curl[2][4];
	float finger_velocity[2][4];
	float l_pose_held[2];
	double x_held, grenade_pulse;
	/* test23: the Quest's buttons, remappable (vr.button_*): which button
	(VR_BUTTON_SOURCE_*) does each action (VR_BUTTON_ACTION_*) */
	int button_source[VR_BUTTON_ACTIONS];
	/* the Quest's Touch controllers: two face buttons a hand, no bumpers
	(the grip throws, Y switches weapons) */
	int touch_layout;
	float hands_last[2][3];
	float melee_speed, flashlight_distance, crouch_height;
	int holsters, two_handed_mode; /* 0 off, 1 grip (squeeze), 2 auto lock */
	int support_near;
	double support_time;
	/* test21 auto lock: how long the off hand has rested at the support grip,
	the hands' distance when it locked, and whether it may lock again (only
	after the hand has left the grip since the last release) */
	float support_dwell, support_engaged_apart;
	int support_rearmed;
	/* test21: a network game (vr_set_network_game) and whether physical
	melee works in one (vr.melee_multiplayer, off by default) */
	int network_game, melee_multiplayer;
	/* test21: the held gun's kind (VR_GUN_*, -1 none: vr_set_gun_class) and
	each kind's aim adjustment (vr.aim_<kind>_up/_right, degrees), turning
	the shots, reticle and scope off the gun's own aim (never the gun);
	shot_pose is aim_pose so turned */
	int gun_class, gun_aim_turned[VR_GUN_CLASSES];
	float gun_aim[VR_GUN_CLASSES][2], gun_aim_rotation[VR_GUN_CLASSES][4];
	struct halo_xr_pose shot_pose;
	/* the holsters' reach (vr.holster_size, metres) and which one the
	weapon hand is in (0 none, 1 + HOLSTER_*) */
	float holster_size;
	int holster_zone;
	float melee_rearm;
	int flashlight_armed, crouching, in_holster, two_hand_held;
	/* test26: the HUD tap (vr.hud_tap_distance: the weapon hand brought to
	its own side of the head) and the reticle's button toggle what the
	session shows (both start shown); the reticle button's press, given up
	if the other stick's click joins it (both recentre) or in a seat (stick
	clicks sound the horn); the turning stick held down (a button); the
	wrist HUD (vr.wrist_hud) and whether menus are up (none masked then) */
	float hud_tap_distance;
	int hud_tap_armed, hud_hidden, reticle_hidden, reticle_press, reticle_cancelled, turn_stick_down;
	int wrist_hud, menus_active;
	/* test29: how long the weapon hand has been held still by its temple
	(the HUD tap); the wrist HUD's place adjusted from its default
	(vr.wrist_hud_along/_across/_out, metres) and its size */
	float hud_tap_dwell, hud_tap_release, hud_tap_cooldown;
	float hud_tap_last[3], hud_tap_anchor[3];
	int hud_tap_last_valid;
	float wrist_along, wrist_across, wrist_out, wrist_size;
	/* the aim's smoothing by zoom level (vr_set_zoom_level), as the PC mod
	HaloCEVR's: its direction, eased toward the hand's */
	int zoom_level, smoothed_valid;
	float smoothed[3];
	/* vr.haptics: the strength of every buzz */
	float haptics;
	/* what the gestures ask of the game, for it to take (vr_take_actions) */
	unsigned int actions;
	/* the reticle where the hand's aim meets the world, metres away; 0 hides it */
	float reticle_distance;
	float reticle_position[3];
	int crosshair_enabled;
	float crosshair_size, crosshair_opacity, crosshair_uv[4];
	GLuint crosshair_texture;
	/* the menus' pointer (vr_ui_pointer): where it last met the menus'
	screen (LOCAL), that screen's orientation and its distance, how many
	frames ago (0 none), its place on the 640x480 screen, its trigger's and
	B's state */
	float pointer_hit[3], pointer_orientation[4], pointer_distance;
	int pointer_age;
	short pointer_x, pointer_y;
	int pointer_trigger, pointer_back;
	/* the scope (vr.scope, vr.scope_size): its image this frame, and its
	sight's shape (VR_SCOPE_*) */
	int scope_enabled, scope_resolved, scope_shape;
	float scope_size;
	/* test22: the pistol's [0] and the sniper rifle's [1] scope moved
	(forward, up, right, metres) and sized (a share of scope_size) from
	their usual place (vr.scope_pistol_*, vr.scope_sniper_*) */
	float scope_adjust[2][4];
	/* room-scale (vr.roomscale): the floor's point the player stands on, in
	LOCAL metres (x, z), after the last tick and the one before (frames
	blend them as the game's camera is blended); set aside while the player
	cannot walk (a vehicle, a cutscene), to be taken up again from where
	the head is then */
	int roomscale, room_held;
	/* vr_reload_settings's count, for caches elsewhere */
	int settings_generation;
	float room_previous[2], room_now[2];
	/* diagnostics (vr.force_render, vr.diag_yaw, vr.dump_frame) */
	int force_render;
	float diag_yaw, diag_hand_yaw, diag_walk_speed;
	double diag_walk_start;
	int diag_two_handed;
	long dump_frame, dump_cinema_frame;
	long stereo_frames, cinema_frames;
	/* timing (vr.timing): milliseconds summed over `timed` frames */
	int timing, gpu_finish;
	double frame_start, pass_start, wait_ms, frame_ms, pass_ms[4], gpu_ms, copy_ms, end_ms;
	long timed;
} vr;

#include <time.h>

static double now_ms(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1e3 + now.tv_nsec * 1e-6;
}

int vr_active(void)
{
	return vr.active;
}

static double auto_resolution_scale(const char *system);

/* physical weapons: what the weapon hand holds - a gun the game put there,
supported until first grip, a gun
gripped, or nothing (one put away in a holster or let fall) */
enum
{
	HAND_LOOSE,
	HAND_HELD,
	HAND_EMPTY,
};
static const char *const hand_state_names[] = { "a gun not gripped", "a gun gripped", "nothing" };
/* how long a change of weapon this side asked for is waited for */
#define PENDING_SECONDS 1.0

/* the holsters, in the frame of the head's heading (x right, y up, z back,
metres): over each shoulder, and at each hip */
enum
{
	HOLSTER_LEFT_SHOULDER,
	HOLSTER_RIGHT_SHOULDER,
	HOLSTER_LEFT_HIP,
	HOLSTER_RIGHT_HIP,
	HOLSTER_COUNT,
};
static const float holster_places[HOLSTER_COUNT][3] =
{
	{ -0.18f, -0.15f, 0.15f },
	{ 0.18f, -0.15f, 0.15f },
	{ -0.22f, -0.62f, 0.00f },
	{ 0.22f, -0.62f, 0.00f },
};
static const char *const holster_names[HOLSTER_COUNT] = { "left shoulder", "right shoulder", "left hip", "right hip" };

/* an eye's view cut to the glasses window (vr.fov_mode
"glasses"): vr.glasses_fov_h by vr.glasses_fov_v degrees about the eye's
own forward, never wider than the runtime's (fov: left, right, up, down) */
static void glasses_fov(const float in[4], float out[4])
{
	out[0] = fmaxf(in[0], -vr.glasses_half[0]);
	out[1] = fminf(in[1], vr.glasses_half[0]);
	out[2] = fminf(in[2], vr.glasses_half[1]);
	out[3] = fmaxf(in[3], -vr.glasses_half[1]);
}

/* how much of an eye's image the glasses window keeps, across and down
(the image spans the view's tangents evenly) */
static void glasses_ratio(const float fov[4], float ratio[2])
{
	float cut[4];

	glasses_fov(fov, cut);
	ratio[0] = (tanf(cut[1]) - tanf(cut[0])) / fmaxf(1e-3f, tanf(fov[1]) - tanf(fov[0]));
	ratio[1] = (tanf(cut[2]) - tanf(cut[3])) / fmaxf(1e-3f, tanf(fov[2]) - tanf(fov[3]));
}

/* the eyes' render size, from vr.resolution_scale (or the headset's when 0).
The scale reaches past the runtime's recommendation (the Quest
3's panels are about 1.25 times it) and the eye images the compositor gets
follow it, so a sharper picture reaches the display rather than being
squeezed into the recommended size; with vr.fov_mode "glasses" both the
game's eye and the images cover only the window, at the same sharpness. */
static void choose_eye_size(void)
{
	/* the Quest 3's views (synthesize_views'), until the runtime's are seen */
	static const float typical_fov[2][4] = { { -1.028f, 0.885f, 0.864f, -1.048f }, { -0.885f, 1.028f, 0.864f, -1.048f } };
	double scale = config_real("vr.resolution_scale");
	float ratio[2] = { 1.0f, 1.0f };
	int size, width, height;

	if (scale <= 0.0)
		scale = auto_resolution_scale(vr.info.system);
	if (scale < 0.5) scale = 0.5;
	if (scale > 2.0) scale = 2.0;
	vr.glasses = !strcmp(config_string("vr.fov_mode"), "glasses");
	vr.glasses_half[0] = (float)(fmin(fmax(config_real("vr.glasses_fov_h"), 20.0), 160.0) * 0.5 * 0.017453293);
	vr.glasses_half[1] = (float)(fmin(fmax(config_real("vr.glasses_fov_v"), 20.0), 160.0) * 0.5 * 0.017453293);
	if (vr.glasses)
	{
		int eye;

		ratio[0] = ratio[1] = 0.0f;
		for (eye = 0; eye < 2; eye++)
		{
			float eye_ratio[2];

			glasses_ratio(vr.fov_known ? vr.runtime_fov[eye] : typical_fov[eye], eye_ratio);
			ratio[0] = fmaxf(ratio[0], eye_ratio[0]);
			ratio[1] = fmaxf(ratio[1], eye_ratio[1]);
		}
	}
	width = (int)(vr.recommended_width * scale * ratio[0] + 0.5) & ~1;
	height = (int)(vr.recommended_height * scale * ratio[1] + 0.5) & ~1;
	if (width < 64) width = 64;
	if (height < 64) height = 64;
	/* the game draws each eye square, as wide as its image (as before:
	the copy stretches it to the image's height) */
	size = width;
	if (size != vr.eye_size)
	{
		vr.eye_size = size;
		platform_log("vr: drawing %dx%d per eye", size, size);
	}
	vr.eye_image_width = (unsigned int)width;
	vr.eye_image_height = (unsigned int)height;
	/* (test29, PR #1 merged: the full view at 100% or less keeps the
	runtime's recommended eye images, as before: the game's eye stretched
	into them. Only a scale above 100% or the glasses window remakes them,
	so AUTO's 85% and 70% on the Quest 2 and the first Quest are as they
	were) */
	if (!vr.glasses && scale <= 1.0)
	{
		vr.eye_image_width = vr.recommended_width;
		vr.eye_image_height = vr.recommended_height;
	}
	if (vr.glasses)
		platform_log("vr: glasses field of view %.0fx%.0f degrees: %.0f%% x %.0f%% of each eye's image%s",
			vr.glasses_half[0] * 2.0f * 57.29578f, vr.glasses_half[1] * 2.0f * 57.29578f, ratio[0] * 100.0f,
			ratio[1] * 100.0f, vr.fov_known ? "" : " (the Quest 3's view until the headset's is seen)");
}

/* the eye images remade at the size chosen, between frames (nothing of
them acquired or drawn yet) */
static void apply_eye_images(void)
{
	if (!vr.eye_image_width || (vr.eye_image_width == vr.info.width[HALO_XR_SWAPCHAIN_LEFT] &&
		vr.eye_image_height == vr.info.height[HALO_XR_SWAPCHAIN_LEFT]))
	{
		return;
	}
	if (host_xr_resize_eyes(&vr.info, vr.eye_image_width, vr.eye_image_height) == 0)
	{
		platform_log("vr: eye images %ux%u (the headset recommends %ux%u; %s field of view)", vr.info.width[0],
			vr.info.height[0], vr.recommended_width, vr.recommended_height, vr.glasses ? "glasses" : "full");
	}
	else
	{
		platform_log("vr: eye images stay %ux%u (%ux%u unavailable)", vr.info.width[0], vr.info.height[0],
			vr.eye_image_width, vr.eye_image_height);
	}
	/* asked once: a failure is not retried every frame */
	vr.eye_image_width = vr.info.width[HALO_XR_SWAPCHAIN_LEFT];
	vr.eye_image_height = vr.info.height[HALO_XR_SWAPCHAIN_LEFT];
}

/* each frame's views: the runtime's kept (for sizing), and with the
glasses window each eye cut to it */
static void apply_glasses(void)
{
	int eye, first = !vr.fov_known;

	vr.frame_glasses = 0;
	if (!(vr.frame.flags & HALO_XR_FRAME_VIEWS_VALID))
		return;
	for (eye = 0; eye < 2; eye++)
		memcpy(vr.runtime_fov[eye], vr.frame.fov[eye], sizeof(vr.runtime_fov[eye]));
	vr.fov_known = 1;
	if (first)
	{
		platform_log("vr: the headset's views: left %.3f %.3f %.3f %.3f, right %.3f %.3f %.3f %.3f (radians: left, "
			"right, up, down)", vr.frame.fov[0][0], vr.frame.fov[0][1], vr.frame.fov[0][2], vr.frame.fov[0][3],
			vr.frame.fov[1][0], vr.frame.fov[1][1], vr.frame.fov[1][2], vr.frame.fov[1][3]);
		/* sized again for the headset's own views */
		if (vr.glasses)
			choose_eye_size();
	}
	if (!vr.glasses)
		return;
	for (eye = 0; eye < 2; eye++)
		glasses_fov(vr.runtime_fov[eye], vr.frame.fov[eye]);
	vr.frame_glasses = 1;
}

/* the display's rate, from vr.refresh_rate (0 the runtime's own) */
static void choose_refresh_rate(void)
{
	static double asked;
	double hertz = config_real("vr.refresh_rate");

	if (hertz > 0.0 && hertz != asked)
	{
		asked = hertz;
		platform_log("vr: display at %.0f Hz", host_xr_set_refresh_rate((float)hertz));
	}
}

/* Test20c: rotations that the owner and testers saved in vr.align_* to make
the empty hand comfortable also tilted the gun. Once per config, move such
rotations to vr.hand_* (hand only) and clear them from the controller
correction. A roll flip (|roll| >= 135, the firmware fix) stays put. */
static void migrate_calibration_split(void)
{
	static const char *const sides[] = {"left", "right"}, *const axes[] = {"pitch", "yaw", "roll"};
	int h, a, ok = 1;

	if (config_boolean("vr.calibration_split_applied"))
		return;
	for (h = 0; h < 2; h++)
	{
		char key[64];
		double angle[3];

		for (a = 0; a < 3; a++)
		{
			snprintf(key, sizeof(key), "vr.align_%s_%s", sides[h], axes[a]);
			angle[a] = vr_alignment_bound(config_real(key), 180.f);
		}
		if ((angle[0] == 0.0 && angle[1] == 0.0 && angle[2] == 0.0) || fabs(angle[2]) >= 135.0)
			continue;
		for (a = 0; a < 3; a++)
		{
			snprintf(key, sizeof(key), "vr.hand_%s_%s", sides[h], axes[a]);
			ok = config_write_real(key, angle[a]) && ok;
			snprintf(key, sizeof(key), "vr.align_%s_%s", sides[h], axes[a]);
			ok = config_write_real(key, 0.0) && ok;
		}
		platform_log("vr: %s hand comfort %.1f/%.1f/%.1f moved from controller calibration to hand orientation%s",
			sides[h], angle[0], angle[1], angle[2], ok ? "" : " (save failed)");
	}
	if (ok)
		config_write_boolean("vr.calibration_split_applied", 1);
}

/* Test20d: left-handed play now also mirrors the sticks and face buttons
(vr.mirror_controls "auto"). Someone already playing left-handed keeps the
layout they learned: once per config their controls stay standard. The old
(Test21: "floating" and "floating_arms" are separate modes again.) */
static void migrate_handedness(void)
{
	int ok = 1;

	if (config_boolean("vr.handedness_applied"))
		return;
	if (config_boolean("vr.left_handed"))
	{
		ok = config_write_string("vr.mirror_controls", "off") && ok;
		platform_log("vr: left-handed config kept on standard sticks and buttons (Controls > Mirror Controls: Auto mirrors them)%s",
			ok ? "" : " (save failed)");
	}
	if (ok)
		config_write_boolean("vr.handedness_applied", 1);
}

/* Test21: the first test21 build's reticle was off the shots, so per-gun aim
values set against it (vr.aim_<gun>_up/_right) return to 0 once */
static void migrate_gun_aim_reset(void)
{
	static const char *const axes[] = { "up", "right" };
	char key[64];
	int kind, axis, ok = 1, changed = 0;

	if (config_boolean("vr.aim_reset_applied"))
		return;
	for (kind = 0; kind < VR_GUN_CLASSES; kind++)
	{
		for (axis = 0; axis < 2; axis++)
		{
			snprintf(key, sizeof(key), "vr.aim_%s_%s", gun_classes[kind].key, axes[axis]);
			if (config_real(key) != 0.0)
			{
				ok = config_write_real(key, 0.0) && ok;
				changed++;
			}
		}
	}
	if (changed)
		platform_log("vr: %d per-gun aim value(s) reset to 0 (set against the first test21 reticle)%s",
			changed, ok ? "" : " (save failed)");
	if (ok)
		config_write_boolean("vr.aim_reset_applied", 1);
}

/* Test21: two-hand grip locks automatically when the off hand rests at the
gun's support grip (vr.two_handed "auto", the new default). Configs still on
the old "grip" default move to it once; a later choice is kept. */
/* test26: the left stick's click toggles the reticle by default, and
crouching moved to the turning stick held down. A crouch left on the left
stick (the old default, written to every config.toml) moves there, unless
another action has it (then crouching has no button: the room-scale crouch
stays); a left stick another action has leaves the reticle without one */
static void migrate_reticle_button(void)
{
	int action, ok = 1;
	int crouch = vr_button_source_of(config_string("vr.button_crouch"), VR_BUTTON_SOURCE_RIGHT_STICK_DOWN);
	int reticle = vr_button_source_of(config_string("vr.button_reticle"), VR_BUTTON_SOURCE_LEFT_STICK);

	if (config_boolean("vr.controls_reticle_applied"))
		return;
	if (crouch == VR_BUTTON_SOURCE_LEFT_STICK && reticle == VR_BUTTON_SOURCE_LEFT_STICK)
	{
		crouch = VR_BUTTON_SOURCE_RIGHT_STICK_DOWN;
		for (action = 0; action < VR_BUTTON_ACTIONS; action++)
		{
			if (action != VR_BUTTON_ACTION_CROUCH &&
				vr_button_source_of(config_string(vr_button_action_key(action)), vr_button_default(action)) == crouch)
				crouch = VR_BUTTON_SOURCE_NONE;
		}
		ok = config_write_string("vr.button_crouch", vr_button_source_value(crouch));
		platform_log("vr: crouch moved to %s; the left stick's click shows or hides the reticle (Buttons page)%s",
			crouch == VR_BUTTON_SOURCE_NONE ? "no button (another action has the turning stick held down)" :
			"the turning stick held down", ok ? "" : " (save failed)");
	}
	for (action = 0; reticle != VR_BUTTON_SOURCE_NONE && action < VR_BUTTON_ACTIONS; action++)
	{
		if (action != VR_BUTTON_ACTION_RETICLE &&
			vr_button_source_of(config_string(vr_button_action_key(action)), vr_button_default(action)) == reticle)
		{
			platform_log("vr: %s has %s; the reticle toggle has no button (Buttons page)",
				vr_button_action_key(action), vr_button_source_value(reticle));
			reticle = VR_BUTTON_SOURCE_NONE;
		}
	}
	ok = config_write_string("vr.button_reticle", vr_button_source_value(reticle)) && ok;
	if (ok)
		config_write_boolean("vr.controls_reticle_applied", 1);
}

static void migrate_two_hand_auto(void)
{
	int ok = 1;

	if (config_boolean("vr.two_hand_auto_applied"))
		return;
	if (!strcmp(config_string("vr.two_handed"), "grip"))
	{
		ok = config_write_string("vr.two_handed", "auto");
		platform_log("vr: two-hand grip now locks automatically at the support grip (Controls > Two Hands: Squeeze restores the old way)%s",
			ok ? "" : " (save failed)");
	}
	if (ok)
		config_write_boolean("vr.two_hand_auto_applied", 1);
}

/* test21: a gun's aim adjustment as a turn of its aim (OpenXR: pitch
about x turns the aim up, yaw about y to the left), degrees */
static void gun_aim_quaternion(float up, float right, float out[4])
{
	float degrees[3];

	degrees[0] = up;
	degrees[1] = -right;
	degrees[2] = 0.0f;
	vr_alignment_rotation(degrees, out);
}

void vr_reload_settings(void)
{
	static int left_handed = -1;

	{
		static const char *const sides[] = {"left", "right"}, *const axes[] = {"pitch", "yaw", "roll"};
		static const char *const tracking[] = {"ik", "floating", "floating_arms"};
		float angle[3], weapon[3], mirrored[3];
		char key[64];
		int h, a;

		for (h = 0; h < 2; h++)
		{
			for (a = 0; a < 3; a++)
			{
				snprintf(key, sizeof(key), "vr.hand_%s_%s", sides[h], axes[a]);
				angle[a] = vr_alignment_bound(config_real(key), 180.f);
			}
			vr_alignment_rotation(angle, vr.hand_rotation[h]);
			platform_log("vr: %s hand orientation pitch/yaw/roll %.1f/%.1f/%.1f degrees (hand only)",
				sides[h], angle[0], angle[1], angle[2]);
		}
		for (a = 0; a < 3; a++)
		{
			snprintf(key, sizeof(key), "vr.weapon_%s", axes[a]);
			weapon[a] = vr_alignment_bound(config_real(key), 180.f);
		}
		/* the left hand holds a mirror image: yaw and roll turn the other way */
		mirrored[0] = weapon[0]; mirrored[1] = -weapon[1]; mirrored[2] = -weapon[2];
		vr_alignment_rotation(weapon, vr.weapon_rotation[1]);
		vr_alignment_rotation(mirrored, vr.weapon_rotation[0]);
		vr.weapon_offset[0] = (float)config_real("vr.weapon_offset_right");
		vr.weapon_offset[1] = (float)config_real("vr.weapon_offset_up");
		vr.weapon_offset[2] = (float)config_real("vr.weapon_offset_back");
		for (vr.hand_tracking = 0; vr.hand_tracking < 3 &&
			strcmp(config_string("vr.hand_tracking"), tracking[vr.hand_tracking]); vr.hand_tracking++)
			;
		/* unknown is body IK */
		if (vr.hand_tracking == 3)
			vr.hand_tracking = 0;
		platform_log("vr: gun pitch/yaw/roll %.1f/%.1f/%.1f degrees offset %.3f/%.3f/%.3f m; hand tracking %s",
			weapon[0], weapon[1], weapon[2], vr.weapon_offset[0], vr.weapon_offset[1], vr.weapon_offset[2],
			tracking[vr.hand_tracking]);
	}

    {
        const char *sides[] = {"left", "right"};
        const char *axes[] = {"pitch", "yaw", "roll", "right", "up", "back"};
        int h,a,changed=0;
        for(h=0;h<2;h++) {
            char key[64]; float angle[3],offset[3],rotation[4]; int grip_aim;
            for(a=0;a<6;a++) {
                snprintf(key,sizeof(key),"vr.align_%s_%s",sides[h],axes[a]);
                if(a<3) angle[a]=vr_alignment_bound(config_real(key),180.f);
                else offset[a-3]=vr_alignment_bound(config_real(key),0.2f);
            }
            snprintf(key,sizeof(key),"vr.align_%s_grip_aim",sides[h]);
            grip_aim=config_boolean(key);
            vr_alignment_rotation(angle,rotation);
            changed |= memcmp(rotation,vr.alignment_rotation[h],sizeof(rotation)) != 0 ||
                memcmp(offset,vr.alignment_offset[h],sizeof(offset)) != 0 || grip_aim != vr.alignment_grip_aim[h];
            memcpy(vr.alignment_rotation[h],rotation,sizeof(rotation));
            memcpy(vr.alignment_offset[h],offset,sizeof(offset)); vr.alignment_grip_aim[h]=grip_aim;
            platform_log("vr: %s calibration pitch/yaw/roll %.1f/%.1f/%.1f degrees offset %.3f/%.3f/%.3f m aim=%s",
                sides[h],angle[0],angle[1],angle[2],offset[0],offset[1],offset[2],grip_aim?"grip":"native");
        }
        if(changed) { vr.hands_last_valid=0; vr.smoothed_valid=0; vr.run_push=0.f;
            vr.hand_speed[0]=vr.hand_speed[1]=0.f; vr.two_hand_held=0; vr.melee_rearm=0.5f; }
    }
	/* "immersive", "screen" (3D unless vr.cinema_3d is off) or "flat" */
	vr.cinema_immersive = !strcmp(config_string("vr.cutscenes"), "immersive");
	vr.cinema_enabled = config_boolean("vr.cinema_3d") && strcmp(config_string("vr.cutscenes"), "flat") != 0;
	vr.snap_turn = (float)config_real("vr.snap_turn") * 0.017453293f;
	vr.smooth_turn_speed = (float)config_real("vr.smooth_turn_speed") * 0.017453293f;
	vr.vehicle_tilt = (float)config_real("vr.vehicle_tilt");
	if (!(vr.vehicle_tilt >= 0.0f))
		vr.vehicle_tilt = 0.0f;
	if (vr.vehicle_tilt > 1.0f)
		vr.vehicle_tilt = 1.0f;
	vr.vignette_strength = (float)config_real("vr.vignette");
	if (!(vr.vignette_strength >= 0.0f))
		vr.vignette_strength = 0.0f;
	if (vr.vignette_strength > 1.0f)
		vr.vignette_strength = 1.0f;
	vr.vignette_when = !strcmp(config_string("vr.vignette_when"), "turn") ? VR_VIGNETTE_TURNING :
		!strcmp(config_string("vr.vignette_when"), "always") ? VR_VIGNETTE_ALWAYS : VR_VIGNETTE_MOVING;
	platform_log("vr: comfort: turning %s %.1f, vignette %.2f (%s)", vr.snap_turn > 0.0f ? "snap" : "smooth",
		(vr.snap_turn > 0.0f ? vr.snap_turn : vr.smooth_turn_speed) * 57.29578f, vr.vignette_strength,
		vr.vignette_when == VR_VIGNETTE_TURNING ? "turning" : vr.vignette_when == VR_VIGNETTE_ALWAYS ? "always" : "moving and turning");
	vr.hand_aim = !strcmp(config_string("vr.aim"), "hand");
	vr.crosshair_enabled = strcmp(config_string("vr.crosshair"), "off") != 0;
	vr.crosshair_size = (float)config_real("vr.crosshair_size");
	if (!isfinite(vr.crosshair_size)) vr.crosshair_size = 1.0f;
	vr.crosshair_size = fminf(3.0f, fmaxf(0.25f, vr.crosshair_size));
	vr.crosshair_opacity = (float)config_real("vr.crosshair_opacity");
	if (!isfinite(vr.crosshair_opacity)) vr.crosshair_opacity = 1.0f;
	vr.crosshair_opacity = fminf(1.0f, fmaxf(0.0f, vr.crosshair_opacity));
	platform_log("vr: native crosshair %s size %.2f opacity %.2f (menu pointer independent)",
		vr.crosshair_enabled ? "on" : "off", vr.crosshair_size, vr.crosshair_opacity);
	vr.layout_vr = strcmp(config_string("vr.controls"), "pad") != 0;
	vr.move_relative = !strcmp(config_string("vr.move_relative"), "left") ? 1 :
		!strcmp(config_string("vr.move_relative"), "right") ? 2 : 0;
	{
		const char *mode = config_string("vr.two_handed");

		/* "auto" (test21: locks when the off hand rests at the support grip),
		"grip" (locks when the off hand squeezes there), "off"; an older
		boolean true means auto */
		vr.two_handed_mode = !strcmp(mode, "off") || !strcmp(mode, "false") ? 0 :
			!strcmp(mode, "auto") || !strcmp(mode, "true") ? 2 : 1;
		vr.two_handed_enabled = vr.two_handed_mode != 0;
	}
	/* (the hand the gun is in changes with the setting, else it stays where
	the palms last swapped it) */
	if (left_handed != config_boolean("vr.left_handed"))
	{
		left_handed = config_boolean("vr.left_handed");
		vr.weapon_hand = left_handed ? 0 : 1;
	}
	vr.controls_mirrored = left_handed && strcmp(config_string("vr.mirror_controls"), "off") != 0;
	platform_log("vr: %s-handed; sticks and face buttons %s", left_handed ? "left" : "right",
		vr.controls_mirrored ? "mirrored (move on the right stick, turn on the left)" : "standard (move on the left stick)");
	vr.melee_speed = (float)config_real("vr.melee_speed");
	vr.melee_multiplayer = config_boolean("vr.melee_multiplayer");
	{
		int action;

		for (action = 0; action < VR_BUTTON_ACTIONS; action++)
			vr.button_source[action] = vr_button_source_of(config_string(vr_button_action_key(action)),
				vr_button_default(action));
		/* the menu swaps a taken button; a config edited by hand may still
		give one button two actions: both happen, and the log says so */
		for (action = 0; action < VR_BUTTON_ACTIONS; action++)
		{
			int other, source = vr.button_source[action];

			for (other = action + 1; source != VR_BUTTON_SOURCE_NONE && source != VR_BUTTON_SOURCE_HOLD &&
				other < VR_BUTTON_ACTIONS; other++)
			{
				if (vr.button_source[other] == source)
					platform_log("vr: warning: %s and %s are both on %s; that button does both",
						vr_button_action_key(action), vr_button_action_key(other), vr_button_source_value(source));
			}
		}
	}
	{
		char key[64];
		int kind;

		for (kind = 0; kind < VR_GUN_CLASSES; kind++)
		{
			snprintf(key, sizeof(key), "vr.aim_%s_up", gun_classes[kind].key);
			vr.gun_aim[kind][0] = vr_alignment_bound(config_real(key), VR_GUN_AIM_LIMIT);
			snprintf(key, sizeof(key), "vr.aim_%s_right", gun_classes[kind].key);
			vr.gun_aim[kind][1] = vr_alignment_bound(config_real(key), VR_GUN_AIM_LIMIT);
			gun_aim_quaternion(vr.gun_aim[kind][0], vr.gun_aim[kind][1], vr.gun_aim_rotation[kind]);
			vr.gun_aim_turned[kind] = vr.gun_aim[kind][0] != 0.0f || vr.gun_aim[kind][1] != 0.0f;
			if (vr.gun_aim_turned[kind])
				platform_log("vr: %s aim adjusted %.1f up, %.1f right (degrees; shots, reticle and scope)",
					gun_classes[kind].label, vr.gun_aim[kind][0], vr.gun_aim[kind][1]);
		}
	}
	vr.melee_impact = !strcmp(config_string("vr.melee"), "impact");
	vr.fingers = config_boolean("vr.fingers");
	vr.arm_run = config_boolean("vr.arm_run");
	vr.arm_run_speed = (float)config_real("vr.arm_run_speed");
	if (vr.arm_run_speed < 0.1f)
		vr.arm_run_speed = 0.1f;
	vr.flashlight_distance = (float)config_real("vr.flashlight_distance");
	/* test26: the head taps' reach (0 off), bounded */
	if (!(vr.flashlight_distance >= 0.0f)) vr.flashlight_distance = 0.0f;
	if (vr.flashlight_distance > 0.4f) vr.flashlight_distance = 0.4f;
	vr.hud_tap_distance = (float)config_real("vr.hud_tap_distance");
	if (!(vr.hud_tap_distance >= 0.0f)) vr.hud_tap_distance = 0.0f;
	if (vr.hud_tap_distance > 0.3f) vr.hud_tap_distance = 0.3f;
	vr.hud_tap_armed = vr.hud_tap_last_valid = 0;
	vr.hud_tap_dwell = vr.hud_tap_release = vr.hud_tap_cooldown = 0.0f;
	vr.wrist_hud = config_boolean("vr.wrist_hud");
	/* test29: the wrist HUD's place and size, bounded */
	vr.wrist_along = (float)config_real("vr.wrist_hud_along");
	vr.wrist_across = (float)config_real("vr.wrist_hud_across");
	vr.wrist_out = (float)config_real("vr.wrist_hud_out");
	vr.wrist_size = (float)config_real("vr.wrist_hud_size");
	if (!isfinite(vr.wrist_along)) vr.wrist_along = 0.0f;
	if (!isfinite(vr.wrist_across)) vr.wrist_across = 0.0f;
	if (!isfinite(vr.wrist_out)) vr.wrist_out = 0.0f;
	if (!isfinite(vr.wrist_size)) vr.wrist_size = 1.0f;
	vr.wrist_along = fminf(0.2f, fmaxf(-0.2f, vr.wrist_along));
	vr.wrist_across = fminf(0.2f, fmaxf(-0.2f, vr.wrist_across));
	vr.wrist_out = fminf(0.2f, fmaxf(-0.2f, vr.wrist_out));
	vr.wrist_size = fminf(2.0f, fmaxf(0.5f, vr.wrist_size));
	platform_log("vr: head taps: flashlight %s, HUD %s; wrist HUD %s (moved %.0f/%.0f/%.0f cm along/across/out, size %.0f%%)",
		vr.flashlight_distance > 0.0f ? "the off hand to the head" : "off (its button)",
		vr.hud_tap_distance > 0.0f ? "the weapon hand held by its temple" : "off",
		vr.wrist_hud ? "on (the off hand's wrist)" : "off",
		vr.wrist_along * 100.0f, vr.wrist_across * 100.0f, vr.wrist_out * 100.0f, vr.wrist_size * 100.0f);
	vr.crouch_height = (float)config_real("vr.crouch_height");
	vr.holsters = config_boolean("vr.holsters");
	vr.holster_size = (float)config_real("vr.holster_size");
	if (vr.holster_size < 0.05f) vr.holster_size = 0.05f;
	if (vr.holster_size > 0.6f) vr.holster_size = 0.6f;
	if (vr.physical_setting != !strcmp(config_string("vr.weapons"), "physical"))
	{
		vr.physical_setting = !vr.physical_setting;
		/* (the gun in hand as the setting changes) */
		vr.hand_state = vr.grip_held[vr.weapon_hand] ? HAND_HELD : HAND_LOOSE;
		vr.pending_state = -1;
		vr.gun_held = vr.hand_state == HAND_HELD;
	}
	vr.haptics = (float)config_real("vr.haptics");
	{
		static int previous_scope_setting = -1;
		int enabled = config_boolean("vr.scope");

		vr.scope_enabled = enabled;
		if (previous_scope_setting != enabled)
		{
			platform_log("vr: scope display %s; zoom with the off-hand index trigger (right-handed Quest: left trigger); requires hand aim",
				enabled ? "on" : "off");
			previous_scope_setting = enabled;
		}
	}
	vr.scope_size = (float)config_real("vr.scope_size");
	{
		static const char *const kinds[] = { "pistol", "sniper" };
		static const char *const parts[] = { "forward", "up", "right", "scale" };
		char key[64];
		int kind, part;

		for (kind = 0; kind < 2; kind++)
		{
			for (part = 0; part < 4; part++)
			{
				double value;

				snprintf(key, sizeof(key), "vr.scope_%s_%s", kinds[kind], parts[part]);
				value = config_real(key);
				if (!isfinite(value))
					value = part == 3 ? 1.0 : 0.0;
				vr.scope_adjust[kind][part] = part == 3 ? (float)fmin(2.0, fmax(0.5, value)) :
					(float)fmin(VR_SCOPE_ADJUST_LIMIT_METRES, fmax(-VR_SCOPE_ADJUST_LIMIT_METRES, value));
			}
		}
	}
	platform_log("vr: scope settings: display %s, layer size %.3f m; pistol offset forward/up/right %.3f/%.3f/%.3f m scale %.3f; "
		"sniper offset forward/up/right %.3f/%.3f/%.3f m scale %.3f",
			vr.scope_enabled ? "on" : "off", vr.scope_size,
			vr.scope_adjust[0][0], vr.scope_adjust[0][1], vr.scope_adjust[0][2], vr.scope_adjust[0][3],
			vr.scope_adjust[1][0], vr.scope_adjust[1][1], vr.scope_adjust[1][2], vr.scope_adjust[1][3]);
	if (vr.roomscale != config_boolean("vr.roomscale"))
	{
		vr.roomscale = config_boolean("vr.roomscale");
		vr.room_held = 1;
	}
	/* the picture's size and the display's rate change in play too (the
	pause menu's graphics settings): the screen's targets follow the new
	scale from the next frame (halo_screen_commit, d3d8_gl.c) */
	if (vr.active)
	{
		choose_eye_size();
		choose_refresh_rate();
	}
	vr.settings_generation++;
	/* the settings in force, for the log */
	platform_log("vr: settings: aim %s, weapons %s, body %s, arms %s, fingers %s, melee %s at %.1f m/s, arm run "
		"%s (%.2f m/s), room-scale %s, crouch %.2f m, turn %s, holsters %s (%.2f m), controls %s, cutscenes %s, "
		"resolution %.2f, refresh %.0f Hz, close contact %s, two hands %s, gesture sprint offline up to 1.50x, "
		"move with %s",
		vr.hand_aim ? "hand" : "head", config_string("vr.weapons"),
		config_string("vr.body"), config_string("vr.arms"), vr.fingers ? "on" : "off",
		vr.melee_impact ? "impact" : "swing", vr.melee_speed, vr.arm_run ? "on" : "off", vr.arm_run_speed,
		vr.roomscale ? "on" : "off", vr.crouch_height,
		vr.snap_turn > 0.0f ? "snap" : "smooth", vr.holsters ? "on" : "off", vr.holster_size,
		vr.layout_vr ? "vr" : "pad", config_string("vr.cutscenes"),
		config_real("vr.resolution_scale"), config_real("vr.refresh_rate"),
		config_boolean("vr.close_contact") ? "offline on" : "off", config_string("vr.two_handed"),
		vr.move_relative == 1 ? "the left hand" : vr.move_relative == 2 ? "the right hand" : "the head");
	platform_log("vr: mounted aim settings: vehicle steering %s, turret/gunner aim %s (default right controller)",
		config_string("vr.vehicle_steering"), config_string("vr.turret_aim"));
}

int vr_settings_generation(void)
{
	return vr.settings_generation;
}

const char *vr_system_name(void)
{
	return vr.info.system;
}

/* vr.resolution_scale 0 (the default): a starting point for the headset's
GPU, from the runtime's name for it. The Quest 3, 3S and Pro (Adreno 740 and
650 class) take the runtime's recommended size; the Quest 2 (Adreno 650 at
lower clocks) and the first Quest (Adreno 540) less of it. graphics.preset
"max" asks for a sharper picture than the recommended size on any headset. */
static double auto_resolution_scale(const char *system)
{
	double scale = 1.0;

	if (!strcmp(config_string("graphics.preset"), "max"))
		scale = 1.3;
	else if (strstr(system, "Quest 3") || strstr(system, "Quest3") || strstr(system, "Quest Pro"))
		scale = 1.0;
	else if (strstr(system, "Quest 2") || strstr(system, "Quest2"))
		scale = 0.85;
	else if (strstr(system, "Quest"))
		scale = 0.7;
	platform_log("vr: resolution scale %.2f for %s (vr.resolution_scale 0, automatic)", scale, system);
	return scale;
}

void vr_initialize(void)
{
	if (vr.initialized)
		return;
	vr.initialized = 1;
	config_vr_vehicle_defaults();
	platform_log("vr: HaloCE Quest 1.0.18 release code48 (OpenCE Build 157 / network 24; launcher network-version browser and population selector; upstream analog trigger, Custom Edition spawn facing, PC vehicle set and host-alone lobby start; scope/turret behavior retained; Safe geometry and accepted VR settings retained)");
	platform_log("vr: retained baseline history: HaloCE Quest test30 candidate 1.0.12 (the menus' face buttons as the Xbox's of the same letter: X deletes a profile; the co-op host's server name; test29: OpenCE build 144 netcode, network 21; the HUD head tap held by the temple, HUD and head tap on the HUD page, wrist HUD on the wrist and movable, moving with a hand while holding the gun in both, glasses FOV and resolution to 200%% (PR #1); test28: co-op games entered in progress keep their camera upright, release checks as OpenCE ships, co-op hosted for 2 to 128 players as OpenCE's Server Setup offers, gyro aim on phones as an option; test27: OpenCE build 138 netcode, network 20, with its co-op for up to 16 players; test26: co-op: death screams no longer stop the second player, cutscene characters placed and animated as on the first; HUD head tap, reticle toggle on the left stick click with crouch on the turning stick held down (or crouch kept on the click: Controls), optional wrist HUD, adjustable head taps; impact melee along the gun with a follow-through; fingers bend smoothly against walls; test25: vehicle seat and recentre diagnostics, first-person horizon option and seat glass, settings rows that fit, vehicle offset reset; test24b: comfort vignette, smooth speed and snap angle, SPV1 compatibility note; test24: co-op cutscenes animate for the second player; test23: remappable Quest buttons with the grenade on X; test22: co-op campaign host crash fixed, steady first-person vehicle view, left-hand ammo display, adjustable scopes, shot diagnostics; test21b: floating hands restored, torso-following arms, neck-pivot full body, auto two-hand lock, horn, online melee off, two-hand gun roll, pistol shots from the hand, reticle converges as shots do, per-gun aim, horn from either stick)");
	if (!config_boolean("vr.enabled"))
	{
		platform_log("vr: off (vr.enabled)");
		return;
	}
	/* the flat screen and HUD layer: the game's 640x480 at twice the size */
	if (host_xr_init(&vr.info, 1280, 960) != 0)
	{
		platform_log("vr: OpenXR is unavailable; playing on the flat screen");
		return;
	}
	glGenFramebuffers(1, &vr.framebuffer);
	vr.recommended_width = vr.info.width[HALO_XR_SWAPCHAIN_LEFT];
	vr.recommended_height = vr.info.height[HALO_XR_SWAPCHAIN_LEFT];
	choose_eye_size();
	apply_eye_images();
	vr.touch_layout = strstr(vr.info.system, "Quest") != NULL;
	/* the swapchains are sRGB and the game's picture is gamma-encoded
	already: written without conversion, the compositor shows it as it is */
	vr.srgb_write_control = host_gl_has_extension("GL_EXT_sRGB_write_control");
	vr.screen_distance = (float)config_real("vr.screen_distance");
	vr.screen_width = (float)config_real("vr.screen_width");
	vr.hud_distance = (float)config_real("vr.hud_distance");
	vr.hud_width = (float)config_real("vr.hud_width");
	vr.stereo_enabled = config_boolean("vr.stereo");
	vr.cinema_separation = (float)config_real("vr.cinema_separation");
	vr.cinema_convergence = (float)config_real("vr.cinema_convergence");
	vr.cinema_distance = (float)config_real("vr.cinema_distance");
	vr.cinema_width = (float)config_real("vr.cinema_width");
	vr.mode = -1;
	vr.units_per_metre = (float)config_real("vr.world_scale");
	vr.snap_armed = 1;
	vr.seat_logged = -1;
	vr.weapon_hand = config_boolean("vr.left_handed") ? 0 : 1;
	migrate_calibration_split();
	migrate_handedness();
	migrate_two_hand_auto();
	migrate_gun_aim_reset();
	migrate_reticle_button();
	vr_reload_settings();
	vr.zoom_level = -1;
	vr.flashlight_armed = 1;
	vr.hud_tap_armed = 0;
	vr.noted_weapon = -1;
	vr.gun_class = -1;
	vr.pending_state = -1;
	vr.hand_state = HAND_LOOSE;
	vr.force_render = config_boolean("vr.force_render");
	vr.timing = config_boolean("vr.timing");
	vr.gpu_finish = config_boolean("vr.timing_gpu");
	vr.diag_yaw = (float)config_real("vr.diag_yaw") * 0.017453293f;
	vr.diag_hand_yaw = (float)config_real("vr.diag_hand_yaw") * 0.017453293f;
	vr.diag_walk_speed = (float)config_real("vr.diag_walk_speed");
	vr.room_held = 1;
	vr.diag_two_handed = config_boolean("vr.diag_two_handed");
	vr.dump_frame = config_integer("vr.dump_frame");
	vr.dump_cinema_frame = config_integer("vr.dump_cinema_frame");
	if (vr.units_per_metre <= 0.0f)
		vr.units_per_metre = 1.0f / 3.048f;
	platform_log("vr: drawing %dx%d per eye; GL_EXT_sRGB_write_control %s", vr.eye_size, vr.eye_size,
		vr.srgb_write_control ? "present" : "absent");
	choose_refresh_rate();
	vr.active = 1;
	platform_log("vr: %s on %s, eyes %ux%u, %u images", vr.info.runtime, vr.info.system,
		vr.info.width[0], vr.info.height[0], vr.info.image_count[0]);
}

/* clears the acquired image of a swapchain */
static void clear_swapchain(unsigned int which, float red, float green, float blue, float alpha)
{
	int index = host_xr_acquire(which);

	if (index < 0)
		return;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[which][index], 0);
	glViewport(0, 0, (GLsizei)vr.info.width[which], (GLsizei)vr.info.height[which]);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(red, green, blue, alpha);
	glClear(GL_COLOR_BUFFER_BIT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	host_xr_release(which);
}

int vr_screen_scale(float scale[2])
{
	if (!vr.active)
		return 0;
	/* the eye's square from the 640x480 screen: the frustum takes the
	aspect back out (vr_camera.c) */
	scale[0] = (float)vr.eye_size / 640.0f;
	scale[1] = (float)vr.eye_size / 480.0f;
	return 1;
}

static float wrap_angle(float angle);

static void rotate(const float q[4], const float v[3], float out[3]);
static void to_halo(const float v[3], float cosine, float sine, float out[3]);
static void look_rotation(const float forward[3], const float up[3], float out[4]);

/* vr.weapons "physical" in effect: set, allowed by the game (a local game,
or vr.physical_multiplayer), and the hand aiming */
static int physical_weapons(void)
{
	return vr.physical_setting && vr.physical_allowed && vr.hand_aim;
}

void vr_set_impact_melee_allowed(int allowed)
{
	vr.impact_allowed = allowed != 0;
}

/* test21: physical melee (impact and swing) stays off in network games
unless vr.melee_multiplayer; the melee button always works */
/* test21: the held gun's kind by its tag's name (VR_GUN_OTHER when none
matches: a custom map's gun, or protected names) */
int vr_gun_class_of_name(const char *name)
{
	static const struct { const char *pattern; int kind; } patterns[] =
	{
		{ "plasma pistol", VR_GUN_PLASMA_PISTOL }, /* before "pistol" */
		{ "plasma_pistol", VR_GUN_PLASMA_PISTOL },
		{ "pistol", VR_GUN_PISTOL },
		{ "assault rifle", VR_GUN_ASSAULT_RIFLE },
		{ "assault_rifle", VR_GUN_ASSAULT_RIFLE },
		{ "plasma rifle", VR_GUN_PLASMA_RIFLE },
		{ "plasma_rifle", VR_GUN_PLASMA_RIFLE },
		{ "sniper", VR_GUN_SNIPER_RIFLE },
		{ "shotgun", VR_GUN_SHOTGUN },
		{ "rocket", VR_GUN_ROCKET_LAUNCHER },
		{ "needler", VR_GUN_NEEDLER },
		{ "plasma_cannon", VR_GUN_FUEL_ROD },
		{ "fuel rod", VR_GUN_FUEL_ROD },
		{ "fuel_rod", VR_GUN_FUEL_ROD },
		{ "flamethrower", VR_GUN_FLAMETHROWER },
	};
	int index;

	for (index = 0; name && index < (int)(sizeof(patterns) / sizeof(patterns[0])); index++)
	{
		if (strstr(name, patterns[index].pattern))
			return patterns[index].kind;
	}
	return VR_GUN_OTHER;
}

void vr_set_gun_class(int kind)
{
	kind = kind >= 0 && kind < VR_GUN_CLASSES ? kind : -1;
	if (kind != vr.gun_class && kind >= 0)
		platform_log("vr: holding %s (aim %.1f up, %.1f right)", gun_classes[kind].label,
			vr.gun_aim[kind][0], vr.gun_aim[kind][1]);
	vr.gun_class = kind;
}

int vr_gun_class(void)
{
	return vr.gun_class;
}

const char *vr_gun_class_label(int kind)
{
	return kind >= 0 && kind < VR_GUN_CLASSES ? gun_classes[kind].label : "NO GUN";
}

const char *vr_gun_class_key(int kind)
{
	return kind >= 0 && kind < VR_GUN_CLASSES ? gun_classes[kind].key : NULL;
}

/* test23: the remappable buttons' names: config keys, values and defaults */
static const char *const button_action_keys[VR_BUTTON_ACTIONS] =
{
	"vr.button_jump", "vr.button_action", "vr.button_melee", "vr.button_crouch",
	"vr.button_switch_weapon", "vr.button_grenade", "vr.button_switch_grenade", "vr.button_reticle"
};
static const char *const button_source_values[VR_BUTTON_SOURCES] =
{
	"none", "a", "b", "x", "y", "right_stick", "left_stick", "grip", "right_stick_down", "hold"
};
/* test26: the reticle toggle took the left stick's click, and crouching
the turning stick held down (a room-scale crouch works as ever) */
static const int button_defaults[VR_BUTTON_ACTIONS] =
{
	VR_BUTTON_SOURCE_A, VR_BUTTON_SOURCE_B, VR_BUTTON_SOURCE_RIGHT_STICK, VR_BUTTON_SOURCE_RIGHT_STICK_DOWN,
	VR_BUTTON_SOURCE_Y, VR_BUTTON_SOURCE_X, VR_BUTTON_SOURCE_HOLD, VR_BUTTON_SOURCE_LEFT_STICK
};

const char *vr_button_action_key(int action)
{
	return action >= 0 && action < VR_BUTTON_ACTIONS ? button_action_keys[action] : "";
}

int vr_button_default(int action)
{
	return action >= 0 && action < VR_BUTTON_ACTIONS ? button_defaults[action] : VR_BUTTON_SOURCE_NONE;
}

const char *vr_button_source_value(int source)
{
	return source >= 0 && source < VR_BUTTON_SOURCES ? button_source_values[source] : "none";
}

/* a value's source, `fallback` for an unknown one ("hold" only switches
grenades) */
int vr_button_source_of(const char *value, int fallback)
{
	int source;

	for (source = 0; value && source < VR_BUTTON_SOURCES; source++)
	{
		if (!strcmp(value, button_source_values[source]))
			return source;
	}
	return fallback;
}

void vr_set_network_game(int network)
{
	network = network != 0;
	if (network != vr.network_game)
		platform_log("vr: %s game: physical melee %s", network ? "network" : "local",
			!network || vr.melee_multiplayer ? "on (as set)" : "off (vr.melee_multiplayer; the melee button still works)");
	vr.network_game = network;
}

static int physical_melee_allowed(void)
{
	return vr.melee_speed > 0.0f && (!vr.network_game || vr.melee_multiplayer);
}

int vr_impact_melee(void)
{
	return vr.active && vr.melee_impact && vr.impact_allowed && physical_melee_allowed();
}

/* test21: the horn. A seated driver's horn is the game's crouch control,
which player_control passes only while the move stick is below 98% (and
the Warthog's throttle is that stick), so a stick click while driving was
lost. Seated, either stick click sounds it (the melee click does nothing
for a driver); both together still recentre. */
int vr_horn_held(void)
{
	/* the controllers' own stick clicks (vr.frame.buttons, as vr_aim's
	recentre reads them): the VR layout's pad gives the right stick's as B
	and the zoom trigger's as the right thumb, so a zoom held with a stick
	click read as the recentre and silenced the horn */
	unsigned int both = HALO_XR_BUTTON_LEFT_THUMB | HALO_XR_BUTTON_RIGHT_THUMB;
	unsigned int sticks = vr.frame.buttons & both;

	return vr.active && vr.layout_vr && vr.seated && (vr.frame.flags & HALO_XR_FRAME_FOCUSED) &&
		sticks != 0 && sticks != both;
}

float vr_hand_speed(int hand)
{
	return hand >= 0 && hand < 2 ? vr.hand_speed[hand] : 0.0f;
}

float vr_melee_speed(void)
{
	return vr.melee_speed;
}

double vr_pose_time(void) { return (double)vr.frame.predicted_display_time * 1e-9; }

float vr_sprint_effort(void)
{
	/* Only forward/neutral stick intent. No speed boost for backing away. */
	if (!vr.active || !vr.aiming_last_frame || vr.seated ||
		!(vr.frame.flags & HALO_XR_FRAME_FOCUSED) || vr.frame.thumb[1] < -0.15f ||
		fabsf(vr.frame.thumb[0]) > 0.6f)
		return 0.0f;
	return vr.run_push;
}

int vr_finger_pose(int hand, float curls[4])
{
	if (!vr.active || !vr.fingers || hand < 0 || hand > 1 || !(vr.frame.hand_valid[hand] & 1))
		return 0;
	memcpy(curls, vr.finger_curl[hand], sizeof(vr.finger_curl[hand]));
	return 1;
}

void vr_set_physical_allowed(int allowed)
{
	if (allowed && !vr.physical_allowed)
	{
		vr.hand_state = vr.grip_held[vr.weapon_hand] ? HAND_HELD : HAND_LOOSE;
		vr.pending_state = -1;
		vr.gun_held = vr.hand_state == HAND_HELD;
	}
	vr.physical_allowed = allowed != 0;
}

/* a change of weapon this side asked for (a gun put away, dropped, taken
up): when the game makes it, the hand holds `state` */
static void hand_pending(int state)
{
	vr.pending_state = state;
	vr.pending_until = now_ms() * 1e-3 + PENDING_SECONDS;
}

void vr_note_weapon(long weapon_index)
{
	int state;

	if (weapon_index == vr.noted_weapon)
	{
		/* Drawing at an empty holster must not invent a held weapon. */
		if (weapon_index == -1)
		{
			vr.hand_state = HAND_EMPTY;
			vr.gun_held = 0;
		}
		/* (nothing came of what was asked) */
		if (vr.pending_state >= 0 && now_ms() * 1e-3 > vr.pending_until)
			vr.pending_state = -1;
		return;
	}
	vr.noted_weapon = weapon_index;
	vr.two_hand_held = 0;
	vr.support_near = 0;
	if (weapon_index == -1)
		state = HAND_EMPTY;
	else if (vr.pending_state >= 0)
		state = vr.pending_state;
	/* one the game put there (a level's start, a pickup, a switch on the
	controller): in the hand, gripped if the grip is held */
	else
		state = vr.grip_held[vr.weapon_hand] ? HAND_HELD : HAND_LOOSE;
	vr.pending_state = -1;
	vr.ungripped_seconds = 0.0f;
	vr.ungripped_warned = 0;
	if (state != vr.hand_state && physical_weapons())
		platform_log("vr: the weapon hand holds %s", hand_state_names[state]);
	vr.hand_state = state;
	vr.gun_held = state == HAND_HELD;
}

int vr_hand_empty(void)
{
	return physical_weapons() && vr.hand_state == HAND_EMPTY;
}

int vr_cinema_immersive(void)
{
	return vr.active && vr.cinema_immersive;
}

float vr_head_local_yaw(void)
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	float forward[3];

	rotate(vr.frame.head.orientation, xr_forward, forward);
	return atan2f(-forward[0], -forward[2]);
}

void vr_view_blink(float amount)
{
	if (amount > vr.fade)
		vr.fade = amount;
}

/* the VR layout (vr.controls "vr") on the Xbox pad the game reads, whose
buttons do (input_abstraction.c): A jump, B melee, X action and reload,
Y switch weapons, white flashlight, black switch grenades, left trigger
grenade, right trigger fire, left stick crouch, right stick zoom, start,
back */
/* test23: whether a remappable source is down: "right" is the major
hand's buttons, "left" the other's (as layout_controls has them); the grip
is the weapon hand's, a button only with locked weapons (physical weapons
hold the gun with it) and away from the holsters (where it draws) */
static int touch_source_down(int source, unsigned int right, unsigned int left)
{
	switch (source)
	{
	case VR_BUTTON_SOURCE_A: return (right & HALO_XR_HAND_SOUTH) != 0;
	case VR_BUTTON_SOURCE_B: return (right & HALO_XR_HAND_EAST) != 0;
	case VR_BUTTON_SOURCE_X: return (left & HALO_XR_HAND_SOUTH) != 0;
	case VR_BUTTON_SOURCE_Y: return (left & HALO_XR_HAND_EAST) != 0;
	case VR_BUTTON_SOURCE_RIGHT_STICK: return (right & HALO_XR_HAND_STICK) != 0;
	case VR_BUTTON_SOURCE_LEFT_STICK: return (left & HALO_XR_HAND_STICK) != 0;
	case VR_BUTTON_SOURCE_GRIP: return !physical_weapons() && vr.grip_held[vr.weapon_hand] && !vr.in_holster;
	case VR_BUTTON_SOURCE_RIGHT_STICK_DOWN: return vr.turn_stick_down;
	}
	return 0;
}

/* test26: the hand (0 left, 1 right) a source is on, for its buzz */
static int touch_source_hand(int source)
{
	int major = vr.controls_mirrored ? 0 : 1;

	switch (source)
	{
	case VR_BUTTON_SOURCE_X:
	case VR_BUTTON_SOURCE_Y:
	case VR_BUTTON_SOURCE_LEFT_STICK:
		return 1 - major;
	case VR_BUTTON_SOURCE_GRIP:
		return vr.weapon_hand;
	}
	return major;
}

/* test23: the Quest's buttons from the remappable table (vr.button_*):
jump A, action/reload B, melee the right stick, crouch the left stick,
switch weapons Y, grenade X (a tap throws; held, X switches grenades,
"hold"), as the Quest always had them but the grenade, which was the grip
with locked weapons (a locked gun's grip now does nothing but draw at a
holster). A grenade on the grip throws while it is held, as it used to */
static unsigned int touch_buttons(unsigned int right, unsigned int left, double seconds, int *grenade_down)
{
	static const unsigned int pad[VR_BUTTON_ACTIONS] =
	{
		HALO_XR_BUTTON_A, HALO_XR_BUTTON_X, HALO_XR_BUTTON_B, HALO_XR_BUTTON_LEFT_THUMB,
		HALO_XR_BUTTON_Y, 0, HALO_XR_BUTTON_BLACK, 0
	};
	unsigned int buttons = 0;
	int action, grenade = vr.button_source[VR_BUTTON_ACTION_GRENADE];
	int hold_switches = vr.button_source[VR_BUTTON_ACTION_SWITCH_GRENADE] == VR_BUTTON_SOURCE_HOLD &&
		grenade != VR_BUTTON_SOURCE_GRIP;

	for (action = 0; action < VR_BUTTON_ACTIONS; action++)
	{
		if (action != VR_BUTTON_ACTION_GRENADE && pad[action] &&
			touch_source_down(vr.button_source[action], right, left))
			buttons |= pad[action];
	}
	/* test26: the reticle's button: pressed and let go, the reticle shows
	or hides; not if the other stick's click joined it (both sticks
	recentre) or in a seat (a stick click sounds the horn) */
	{
		int source = vr.button_source[VR_BUTTON_ACTION_RETICLE];

		if (source != VR_BUTTON_SOURCE_NONE && touch_source_down(source, right, left))
		{
			if (!vr.reticle_press)
			{
				vr.reticle_press = 1;
				vr.reticle_cancelled = 0;
			}
			if (vr.seated || ((right & HALO_XR_HAND_STICK) && (left & HALO_XR_HAND_STICK)))
				vr.reticle_cancelled = 1;
		}
		else if (vr.reticle_press)
		{
			vr.reticle_press = 0;
			if (!vr.reticle_cancelled)
			{
				vr.reticle_hidden = !vr.reticle_hidden;
				vr_haptic(touch_source_hand(source), 0.3f, 0.03f);
				platform_log("vr: reticle %s (vr.button_reticle)", vr.reticle_hidden ? "hidden" : "shown");
			}
		}
	}
	if (!hold_switches)
	{
		*grenade_down = touch_source_down(grenade, right, left);
		return buttons;
	}
	/* a tap throws, a hold switches grenades */
	if (touch_source_down(grenade, right, left))
	{
		vr.x_held += seconds;
		if (vr.x_held >= 0.4 && !vr.x_hold_switched)
		{
			buttons |= HALO_XR_BUTTON_BLACK;
			vr.x_hold_switched = 1;
		}
	}
	else
	{
		if (vr.x_held > 0.0 && !vr.x_hold_switched)
			vr.grenade_pulse = 0.1;
		vr.x_held = 0.0;
		vr.x_hold_switched = 0;
	}
	return buttons;
}

static void layout_controls(void)
{
	static const float zoom_on = 0.6f, zoom_off = 0.45f;
	static int previous_zoom_input_state = -1;
	/* "right" is the major hand's buttons, "left" the other's: mirrored
	for left-handed play (vr.controls_mirrored) */
	int major = vr.controls_mirrored ? 0 : 1;
	unsigned int right = vr.frame.hand_buttons[major], left = vr.frame.hand_buttons[1 - major];
	unsigned int buttons = 0;
	double seconds = vr.frame.predicted_display_period * 1e-9;
	int grenade_down = 0;

	if (!vr.layout_vr)
	{
		vr.pad_buttons = vr.frame.buttons;
		memcpy(vr.pad_trigger, vr.frame.trigger, sizeof(vr.pad_trigger));
		return;
	}
	/* test26: the turning stick held nearly straight down, a button (it
	never turns: snap and smooth turns are its sideways), let go a little
	sooner than taken; never in a seat */
	vr.turn_stick_down = !vr.seated && fabsf(vr.frame.thumb[2]) < 0.6f &&
		vr.frame.thumb[3] < (vr.turn_stick_down ? -0.55f : -0.75f);
	if (vr.frame.hand_buttons[1 - vr.weapon_hand] & HALO_XR_HAND_BUMPER)
		buttons |= HALO_XR_BUTTON_WHITE;                                    /* flashlight, the off hand's */
	if ((right | left) & HALO_XR_HAND_MENU) buttons |= HALO_XR_BUTTON_START;
	/* test23: the Quest's Touch controllers take their buttons from the
	remappable table (vr.button_*); other controllers keep their layout */
	if (vr.touch_layout)
	{
		buttons |= touch_buttons(right, left, seconds, &grenade_down);
		left &= ~HALO_XR_HAND_EAST;
	}
	else
	{
	if (right & HALO_XR_HAND_SOUTH) buttons |= HALO_XR_BUTTON_A;           /* jump */
	if (right & HALO_XR_HAND_EAST) buttons |= HALO_XR_BUTTON_X;            /* action, reload */
	if (right & HALO_XR_HAND_WEST) buttons |= HALO_XR_BUTTON_BLACK;        /* switch grenades */
	if (right & HALO_XR_HAND_NORTH) buttons |= HALO_XR_BUTTON_Y;           /* switch weapons */
	if (right & HALO_XR_HAND_STICK) buttons |= HALO_XR_BUTTON_B;           /* melee (and the swing) */
	if (left & HALO_XR_HAND_STICK) buttons |= HALO_XR_BUTTON_LEFT_THUMB;   /* crouch */
	/* controllers with two face buttons a hand (Touch, Index): the left's
	lower switches grenades, its upper goes back (the Quest's: switches
	weapons; both sticks recentre). With physical weapons the grip holds the
	gun, so a tap of the left's lower throws a grenade and a hold of it
	switches grenades */
	if (physical_weapons())
	{
		if (left & HALO_XR_HAND_SOUTH)
		{
			vr.x_held += seconds;
			if (vr.x_held >= 0.4 && !vr.x_hold_switched)
			{
				buttons |= HALO_XR_BUTTON_BLACK;
				vr.x_hold_switched = 1;
			}
		}
		else
		{
			if (vr.x_held > 0.0 && !vr.x_hold_switched)
				vr.grenade_pulse = 0.1;
			vr.x_held = 0.0;
			vr.x_hold_switched = 0;
		}
	}
	else if (left & HALO_XR_HAND_SOUTH)
	{
		buttons |= HALO_XR_BUTTON_BLACK;
	}
	}
	/* test30: in the menus the Quest's face buttons are the Xbox's of the
	same letter, as the screens' prompts name them: A, X and Y (and B for
	a left-handed player). The gameplay table (vr.button_*) gave the
	menus no X (the Quest's X throws a grenade; the game's X, use and
	reload, is on B, which in the menus is the pointer's back), so a
	profile could not be deleted (the profile list's X). The button that
	is the pointer's back (vr_ui_pointer: the major hand's upper, right B)
	gives nothing else, so one press goes back once. No grenade in menus */
	if (vr.menus_active && vr.touch_layout)
	{
		unsigned int physical_right = vr.frame.hand_buttons[1], physical_left = vr.frame.hand_buttons[0];
		unsigned int back = vr.controls_mirrored ? HALO_XR_BUTTON_Y : HALO_XR_BUTTON_B;

		buttons &= ~(HALO_XR_BUTTON_A | HALO_XR_BUTTON_B | HALO_XR_BUTTON_X | HALO_XR_BUTTON_Y | HALO_XR_BUTTON_BLACK);
		if (physical_right & HALO_XR_HAND_SOUTH) buttons |= HALO_XR_BUTTON_A;
		if (physical_right & HALO_XR_HAND_EAST) buttons |= HALO_XR_BUTTON_B;
		if (physical_left & HALO_XR_HAND_SOUTH) buttons |= HALO_XR_BUTTON_X;
		if (physical_left & HALO_XR_HAND_EAST) buttons |= HALO_XR_BUTTON_Y;
		buttons &= ~back;
		grenade_down = 0;
		vr.grenade_pulse = 0.0;
		vr.x_held = 0.0;
		vr.x_hold_switched = 1;
	}
	/* the off hand's trigger zooms */
	/* Record only changes in its useful range, including a partial squeeze
	that never reaches the activation threshold. */
	{
		int off_hand = 1 - vr.weapon_hand;
		float trigger = vr.frame.trigger[off_hand];
		int state;

		if (trigger > zoom_on)
			vr.zoom_down = 1;
		else if (trigger < zoom_off)
			vr.zoom_down = 0;
		state = vr.zoom_down ? 2 : trigger >= 0.20f ? 1 : 0;
		if (state != previous_zoom_input_state)
		{
			platform_log("vr: zoom input %s from %s-hand index trigger (%.2f; press >%.2f, release <%.2f); routed to gamepad zoom",
				state == 2 ? "active" : state == 1 ? "partial" : "released",
				off_hand ? "right" : "left", trigger, zoom_on, zoom_off);
			previous_zoom_input_state = state;
		}
	}
	if (vr.zoom_down)
		buttons |= HALO_XR_BUTTON_RIGHT_THUMB;
	/* View (Touch: the left's upper face button): a press goes back (as the
	button lets go), a hold of a second recentres */
	if ((left & (HALO_XR_HAND_VIEW | HALO_XR_HAND_EAST)))
	{
		vr.view_held += seconds;
		if (vr.view_held >= 1.0 && !vr.view_recentred)
		{
			host_xr_recenter();
			vr.recentre_source = 2;
			vr.heading_valid = 0;
			vr.view_recentred = 1;
			vr_haptic(0, 0.6f, 0.08f);
			vr_haptic(1, 0.6f, 0.08f);
		}
	}
	else
	{
		if (vr.view_held > 0.0 && !vr.view_recentred)
			vr.back_pulse = 2;
		vr.view_held = 0.0;
		vr.view_recentred = 0;
	}
	if (vr.back_pulse > 0)
	{
		buttons |= HALO_XR_BUTTON_BACK;
		vr.back_pulse--;
	}
	vr.pad_buttons = buttons;
	/* the grenade on the weapon hand's bumper (the Quest's: its remapped
	button, touch_buttons; a tap's throw is the pulse below), fire on its
	trigger (with physical weapons, only while the gun is held) */
	vr.pad_trigger[0] = (vr.frame.hand_buttons[vr.weapon_hand] & HALO_XR_HAND_BUMPER) ? 1.0f : 0.0f;
	if (vr.touch_layout && grenade_down)
		vr.pad_trigger[0] = 1.0f;
	if (vr.grenade_pulse > 0.0)
	{
		vr.pad_trigger[0] = 1.0f;
		vr.grenade_pulse -= seconds;
	}
	/* the trigger fires the gun in the hand, gripped or not; an empty hand
(physical weapons: a gun put away or let fall) has nothing to fire */
	vr.pad_trigger[1] = vr.frame.trigger[vr.weapon_hand];
	/* A vehicle weapon is independent of the holstered handheld weapon.
	Seated triggers still pass through the native primary/secondary mapping. */
	if (!vr.seated && physical_weapons() && vr.hand_state == HAND_EMPTY)
		vr.pad_trigger[1] = 0.0f;
}

/* ---------- gestures (after the PC mod HaloCEVR's designs, by LivingFray) */

static float distance3(const float a[3], const float b[3])
{
	float d0 = a[0] - b[0], d1 = a[1] - b[1], d2 = a[2] - b[2];

	return sqrtf(d0 * d0 + d1 * d1 + d2 * d2);
}

/* a point in the frame of the head's heading (LOCAL; x right, y up, z back
of where the head faces, level) */
static void heading_point(const float offset[3], float out[3])
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	float forward[3], yaw, c, s;

	rotate(vr.frame.head.orientation, xr_forward, forward);
	yaw = atan2f(-forward[0], -forward[2]);
	c = cosf(yaw);
	s = sinf(yaw);
	out[0] = vr.frame.head.position[0] + offset[0] * c + offset[2] * s;
	out[1] = vr.frame.head.position[1] + offset[1];
	out[2] = vr.frame.head.position[2] - offset[0] * s + offset[2] * c;
}

/* HUD gesture in head-relative metres: a deliberate hold, followed by a
sustained withdrawal before another hold can toggle. Room-scale motion of
head and hand together must not prevent it; a head turning past a stationary
controller must not count as a stationary hand against the temple. */
#define HUD_TAP_OUT 0.11f
#define HUD_TAP_BACK 0.03f
#define HUD_TAP_SLOW 0.45f
#define HUD_TAP_HOLD_SECONDS 0.30f
#define HUD_TAP_RELEASE_SECONDS 0.25f
#define HUD_TAP_RELEASE_MARGIN 0.08f
#define HUD_TAP_COOLDOWN_SECONDS 0.80f
#define HUD_TAP_HOLD_MARGIN 0.015f
#define HUD_TAP_HOLD_DRIFT 0.025f

static void reset_hud_tap(void)
{
	vr.hud_tap_armed = vr.hud_tap_last_valid = 0;
	vr.hud_tap_dwell = vr.hud_tap_release = vr.hud_tap_cooldown = 0.0f;
}

static void update_hud_tap(int hand, float seconds)
{
	const unsigned int needed = HALO_XR_FRAME_FOCUSED | HALO_XR_FRAME_VIEWS_VALID;
	float relative[3], local[3], inverse[4], target[3], distance, speed, norm = 0.0f;
	int axis;
	if (!(vr.hud_tap_distance > 0.0f) || hand < 0 || hand > 1 ||
		!(vr.frame.hand_valid[hand] & 1) || vr.menus_active ||
		(vr.frame.flags & (needed | HALO_XR_FRAME_RECENTRED)) != needed ||
		!isfinite(seconds) || !(seconds > 0.0f) || seconds > 0.05f)
	{
		reset_hud_tap();
		return;
	}
	for (axis = 0; axis < 4; axis++)
	{
		float value = vr.frame.head.orientation[axis];
		if (!isfinite(value)) { reset_hud_tap(); return; }
		inverse[axis] = axis == 3 ? value : -value;
		norm += value * value;
	}
	if (!(norm > 0.95f && norm < 1.05f)) { reset_hud_tap(); return; }
	for (axis = 0; axis < 3; axis++)
	{
		relative[axis] = vr.frame.grip[hand].position[axis] - vr.frame.head.position[axis];
		if (!isfinite(relative[axis])) { reset_hud_tap(); return; }
	}
	rotate(inverse, relative, local);
	target[0] = hand ? HUD_TAP_OUT : -HUD_TAP_OUT;
	target[1] = 0.0f;
	target[2] = HUD_TAP_BACK;
	distance = distance3(local, target);
	speed = vr.hud_tap_last_valid ? distance3(local, vr.hud_tap_last) / seconds : HUD_TAP_SLOW;
	memcpy(vr.hud_tap_last, local, sizeof(local));
	vr.hud_tap_last_valid = 1;
	vr.hud_tap_cooldown = fmaxf(0.0f, vr.hud_tap_cooldown - seconds);
	if (distance > vr.hud_tap_distance + HUD_TAP_RELEASE_MARGIN)
	{
		vr.hud_tap_dwell = 0.0f;
		vr.hud_tap_release += seconds;
		if (vr.hud_tap_release >= HUD_TAP_RELEASE_SECONDS)
			vr.hud_tap_armed = 1;
		return;
	}
	vr.hud_tap_release = 0.0f;
	if (!vr.hud_tap_armed || vr.hud_tap_cooldown > 0.0f || vr.in_holster ||
		speed >= HUD_TAP_SLOW ||
		(hand ? local[0] < 0.035f : local[0] > -0.035f) ||
		distance >= vr.hud_tap_distance + (vr.hud_tap_dwell > 0.0f ? HUD_TAP_HOLD_MARGIN : 0.0f))
	{
		vr.hud_tap_dwell = 0.0f;
		return;
	}
	if (vr.hud_tap_dwell <= 0.0f)
		memcpy(vr.hud_tap_anchor, local, sizeof(local));
	if (distance3(local, vr.hud_tap_anchor) > HUD_TAP_HOLD_DRIFT)
	{
		vr.hud_tap_dwell = 0.0f;
		return;
	}
	vr.hud_tap_dwell += seconds;
	if (vr.hud_tap_dwell >= HUD_TAP_HOLD_SECONDS)
	{
		vr.hud_hidden = !vr.hud_hidden;
		vr.hud_tap_armed = 0;
		vr.hud_tap_dwell = 0.0f;
		vr.hud_tap_cooldown = HUD_TAP_COOLDOWN_SECONDS;
		vr_haptic(hand, 0.4f, 0.04f);
		platform_log("vr: HUD %s (head tap)", vr.hud_hidden ? "hidden" : "shown");
	}
}

static void update_gestures(void)
{
	static const float melee_rearm_seconds = 0.4f;
	float seconds = (float)(vr.frame.predicted_display_period * 1e-9);
	int w = vr.weapon_hand, o = 1 - vr.weapon_hand, hand;
	int both_tracked = (vr.frame.hand_valid[0] & 1) && (vr.frame.hand_valid[1] & 1);
	float hands_apart = both_tracked ? distance3(vr.frame.grip[0].position, vr.frame.grip[1].position) : 99.0f;

	if (!(vr.frame.flags & HALO_XR_FRAME_FOCUSED))
	{
		reset_hud_tap();
		vr.two_hand_held = 0;
		vr.support_near = 0;
		vr.run_push = 0.0f;
		return;
	}
	/* grips: held over 0.8, let go under 0.7 */
	for (hand = 0; hand < 2; hand++)
	{
		int held = vr.grip_held[hand] ? vr.frame.squeeze[hand] > 0.7f : vr.frame.squeeze[hand] > 0.8f;

		vr.grip_pressed[hand] = held && !vr.grip_held[hand];
		vr.grip_released[hand] = !held && vr.grip_held[hand];
		vr.grip_held[hand] = held;
	}

	/* each hand's speed about the room (metres a second), its peak held over
	the last tenth of a second for the game's ticks (vr_hand_speed: impact
	melee, running) */
	for (hand = 0; hand < 2; hand++)
	{
		vr.hand_speed[hand] *= seconds > 0.0f ? expf(-seconds / 0.1f) : 1.0f;
		if ((vr.frame.hand_valid[hand] & 1) && (vr.hands_last_valid & (1 << hand)) && seconds > 0.0f &&
			!(vr.frame.flags & HALO_XR_FRAME_RECENTRED))
		{
			float speed = distance3(vr.frame.grip[hand].position, vr.hands_last[hand]) / seconds;

			if (speed < 20.0f && speed > vr.hand_speed[hand])
				vr.hand_speed[hand] = speed;
		}
	}

	/* the fingers, from where they rest on the controller: the thumb curled
	while it touches a button, the stick or the thumbrest (lifted, a thumbs
	up); the index finger with the trigger, open off it (pointing); the
	others with the grip. Pointing with the thumb up (an "L") held for a
	second shows the middle finger instead - deliberately, never in passing */
	for (hand = 0; hand < 2; hand++)
	{
		unsigned int touch = vr.frame.hand_buttons[hand];
		float target[4];
		int finger, l_pose;

		target[0] = (touch & HALO_XR_HAND_THUMB_TOUCH) ? 0.75f : 0.0f;
		target[1] = fmaxf(vr.frame.trigger[hand], (touch & HALO_XR_HAND_INDEX_TOUCH) ? 0.22f : 0.0f);
		target[2] = target[3] = vr.frame.squeeze[hand];
		l_pose = !(touch & (HALO_XR_HAND_THUMB_TOUCH | HALO_XR_HAND_INDEX_TOUCH)) && vr.frame.squeeze[hand] > 0.7f;
		vr.l_pose_held[hand] = l_pose ? vr.l_pose_held[hand] + seconds : 0.0f;
		if (vr.l_pose_held[hand] >= 1.0f)
		{
			target[0] = 0.8f;
			target[1] = 1.0f;
			target[2] = 0.0f;
		}
		for (finger = 0; finger < 4; finger++)
		{
			/* Exact critically damped spring: continuous velocity at touch
			transitions, no overshoot from varying headset frame intervals. */
			float dt = seconds > 0.0f ? fminf(seconds, 0.05f) : 0.0f;
			float omega = finger == 0 ? 20.0f : 26.0f;
			float x = vr.finger_curl[hand][finger] - target[finger];
			float v = vr.finger_velocity[hand][finger];
			float j = v + omega * x, decay = expf(-omega * dt);
			vr.finger_curl[hand][finger] = fmaxf(0.0f, fminf(1.0f, target[finger] + (x + j * dt) * decay));
			vr.finger_velocity[hand][finger] = (v - omega * j * dt) * decay;
		}
	}

	/* running by swinging the arms (vr.arm_run): both arms pumping, as the
	slower of the hands moves; holding the gun in both hands, they move
	together and bob with the run, so either hand's. The push on the stick
	follows the effort above vr.arm_run_speed, full at 2.5 times it, rising
	quickly and falling slowly so the strokes run on */
	{
		float push = 0.0f;

		if (vr.arm_run && both_tracked)
		{
			float a = vr.hand_speed[0], b = vr.hand_speed[1];
			float effort = vr.two_hand_held ? (a > b ? a : b) : (a < b ? a : b);

			push = (effort - vr.arm_run_speed) / (1.5f * vr.arm_run_speed);
			push = push < 0.0f ? 0.0f : push > 1.0f ? 1.0f : push;
		}
		if (seconds > 0.0f)
			vr.run_push += (push - vr.run_push) * (1.0f - expf(-seconds * (push > vr.run_push ? 8.0f : 2.5f)));
		if (vr.run_push < 0.01f)
			vr.run_push = 0.0f;
		if (!vr.aiming_last_frame || vr.seated || !both_tracked)
			vr.run_push = 0.0f;
	}

	/* melee: a hand swung up or down faster than vr.melee_speed - unless
	the hands strike what they touch (vr.melee "impact", where the game
	allows it: vr_set_impact_melee_allowed) */
	if (vr.melee_rearm > 0.0f)
		vr.melee_rearm -= seconds;
	if (physical_melee_allowed() && vr.hands_last_valid && !(vr.frame.flags & HALO_XR_FRAME_RECENTRED) && seconds > 0.0f &&
		!(vr.melee_impact && vr.impact_allowed))
	{
		for (hand = 0; hand < 2; hand++)
		{
			float vertical;

			if (!(vr.frame.hand_valid[hand] & 1) || !(vr.hands_last_valid & (1 << hand)))
				continue;
			vertical = (vr.frame.grip[hand].position[1] - vr.hands_last[hand][1]) / seconds;
			if (fabsf(vertical) > vr.melee_speed && vr.melee_rearm <= 0.0f)
			{
				vr.actions |= VR_ACTION_MELEE;
				vr.melee_rearm = melee_rearm_seconds;
				vr_haptic(hand, 0.5f, 0.05f);
			}
		}
	}
	vr.hands_last_valid = 0;
	for (hand = 0; hand < 2; hand++)
	{
		if (vr.frame.hand_valid[hand] & 1)
		{
			memcpy(vr.hands_last[hand], vr.frame.grip[hand].position, sizeof(vr.hands_last[hand]));
			vr.hands_last_valid |= 1 << hand;
		}
	}

	/* the flashlight: the off hand brought to the head (a point 10 cm
	behind the eyes, the head's middle), once each time */
	if (vr.flashlight_distance > 0.0f && (vr.frame.hand_valid[o] & 1))
	{
		static const float behind[3] = { 0.0f, 0.0f, 0.10f };
		float head_middle[3], distance;

		rotate(vr.frame.head.orientation, behind, head_middle);
		head_middle[0] += vr.frame.head.position[0];
		head_middle[1] += vr.frame.head.position[1];
		head_middle[2] += vr.frame.head.position[2];
		distance = distance3(vr.frame.grip[o].position, head_middle);
		if (distance < vr.flashlight_distance && vr.flashlight_armed)
		{
			vr.actions |= VR_ACTION_FLASHLIGHT;
			vr.flashlight_armed = 0;
			vr_haptic(o, 0.4f, 0.04f);
		}
		else if (distance > vr.flashlight_distance + 0.05f)
		{
			vr.flashlight_armed = 1;
		}
	}

	/* test26: the HUD tap: weapon hand at its own temple. Test32 requires
	a steady head-relative hold and a deliberate withdrawal before rearming. */
	update_hud_tap(w, seconds);

	/* crouching: the head lower than standing (its height at the last
	recentre) by vr.crouch_height, until it comes back within 5 cm of it */
	if (vr.crouch_height > 0.0f)
	{
		float drop = -vr.frame.head.position[1];

		if (drop > vr.crouch_height)
			vr.crouching = 1;
		else if (drop < vr.crouch_height - 0.05f)
			vr.crouching = 0;
	}
	else
	{
		vr.crouching = 0;
	}

	/* holsters (vr.holsters), each reaching vr.holster_size: over each
	shoulder and at each hip, in the frame of the head's heading. The weapon
	hand coming into one buzzes; with locked weapons its grip there switches
	weapons (physical weapons: below) */
	if (vr.holsters && (vr.frame.hand_valid[w] & 1))
	{
		int zone = 0, index;
		float nearest = vr.holster_size;

		for (index = 0; index < HOLSTER_COUNT; index++)
		{
			float place[3], distance;

			heading_point(holster_places[index], place);
			distance = distance3(vr.frame.grip[w].position, place);
			if (distance < nearest)
			{
				nearest = distance;
				zone = index + 1;
			}
		}
		if (zone && zone != vr.holster_zone)
			vr_haptic(w, 0.45f, 0.05f);
		vr.holster_zone = zone;
		vr.in_holster = zone != 0;
		if (zone && vr.grip_pressed[w] && !physical_weapons())
		{
			vr.actions |= VR_ACTION_SWITCH_WEAPON;
			vr_haptic(w, 0.6f, 0.06f);
			platform_log("vr: weapons switched at the %s holster", holster_names[zone - 1]);
		}
	}
	else
	{
		vr.in_holster = 0;
		vr.holster_zone = 0;
	}

	/* physical weapons (vr.weapons "physical"): a gun stays in the hand while
	the grip holds it.
	- Let go at a holster, it is put away: the hand is empty, and the other
	  weapon is the one ready in the holsters.
	- Let go with the other hand on it, it passes to that hand.
	- Let go anywhere else, it falls.
	- A gun the game puts in the hand (a level's start, a pickup, a switch
	  on the controller) stays supported until its first deliberate grip.
	- An empty hand gripping at a holster draws the weapon there; gripping
	  by a weapon lying about takes it up (VR_ACTION_GRAB_WEAPON). */
	if (physical_weapons())
	{
		const char *holster = vr.holster_zone ? holster_names[vr.holster_zone - 1] : "";

		if (vr.grip_pressed[w])
		{
			if (vr.hand_state != HAND_EMPTY)
			{
				vr.hand_state = HAND_HELD;
			}
			else if (vr.in_holster && vr.noted_weapon != -1)
			{
				vr.hand_state = HAND_HELD;
				vr.pending_state = -1;
				vr_haptic(w, 0.5f, 0.05f);
				platform_log("vr: a weapon drawn from the %s holster", holster);
			}
			else
			{
				vr.actions |= VR_ACTION_GRAB_WEAPON;
				hand_pending(HAND_HELD);
			}
			vr.ungripped_seconds = 0.0f;
			vr.ungripped_warned = 0;
		}
		else if (vr.grip_released[w] && vr.hand_state == HAND_HELD)
		{
			if (vr.in_holster)
			{
				vr.hand_state = HAND_EMPTY;
				vr.actions |= VR_ACTION_SWITCH_WEAPON;
				hand_pending(HAND_EMPTY);
				vr_haptic(w, 0.6f, 0.06f);
				platform_log("vr: the gun put away in the %s holster", holster);
			}
			else if (vr.two_hand_held && vr.grip_held[o])
			{
				vr.weapon_hand = o;
				vr.two_hand_held = 0;
				vr.hand_state = HAND_HELD;
				vr.gun_held = 1;
				vr_haptic(o, 0.4f, 0.05f);
				platform_log("vr: the gun passes to the %s hand", vr.weapon_hand ? "right" : "left");
				return;
			}
			else
			{
				vr.hand_state = HAND_EMPTY;
				vr.actions |= VR_ACTION_DROP_WEAPON;
				hand_pending(HAND_EMPTY);
				vr_haptic(w, 0.5f, 0.05f);
				platform_log("vr: the gun let go: it falls");
			}
		}
		/* HAND_LOOSE is spawn/pickup support. Only a deliberate first grip
		transitions to HELD; only releasing HELD may drop the weapon. */
		vr.ungripped_seconds = 0.0f;
		vr.ungripped_warned = 0;
		vr.gun_held = vr.hand_state == HAND_HELD;
	}

	/* the off hand's grip: with the palms together, the weapon changes
	hands; otherwise, within reach of the gun, it holds it in both */
	if (vr.grip_pressed[o] && both_tracked)
	{
		if (hands_apart < 0.2f && !vr.grip_held[w])
		{
			vr.weapon_hand = o;
			vr.two_hand_held = 0;
			vr_haptic(0, 0.5f, 0.06f);
			vr_haptic(1, 0.5f, 0.06f);
			platform_log("vr: the weapon changes to the %s hand", vr.weapon_hand ? "right" : "left");
			return;
		}
		if (vr.two_handed_mode >= 1 && !vr.two_hand_held && vr.support_near &&
			vr_pose_time() - vr.support_time < 0.10 && hands_apart < 0.8f && !vr_hand_empty()) {
			vr.two_hand_held = 1;
			vr.support_engaged_apart = hands_apart;
			vr_haptic(o, 0.25f, 0.035f);
			platform_log("vr: support grip engaged (%s hand); attachment fixed until release", o ? "right" : "left");
		}
	}
	/* auto lock: the off hand resting at the gun's support grip (where the
	game's animation puts that hand; vr_support_near) for 0.12 s locks it
	there as a squeeze would, once the hand has left the grip since the
	last release */
	{
		int near = vr.support_near && vr_pose_time() - vr.support_time < 0.10;

		if (!near)
			vr.support_rearmed = 1;
		if (vr.two_handed_mode == 2 && !vr.two_hand_held && near && vr.support_rearmed && both_tracked &&
			vr.aiming_last_frame && !vr_hand_empty() && hands_apart > 0.08f && hands_apart < 0.8f)
		{
			vr.support_dwell += seconds;
			if (vr.support_dwell >= 0.12f)
			{
				vr.two_hand_held = 1;
				vr.support_engaged_apart = hands_apart;
				vr.support_dwell = 0.0f;
				vr_haptic(o, 0.25f, 0.035f);
				platform_log("vr: support grip locked automatically (%s hand); pull the hand away to release",
					o ? "right" : "left");
			}
		}
		else if (!vr.two_hand_held)
			vr.support_dwell = 0.0f;
	}
	if (vr.two_hand_held)
	{
		int release = !both_tracked || !vr.aiming_last_frame || vr_hand_empty();
		const char *why = "";

		if (!release && vr.two_handed_mode == 1 && !vr.grip_held[o])
			release = 1, why = " (grip let go)";
		else if (!release && vr.two_handed_mode == 2)
		{
			/* pulled away: the hands' distance changed by 20 cm since it
			locked, or the off hand left a 60-degree cone ahead of the gun */
			static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
			float aim[3], between[3], along;
			int axis;

			rotate(vr.frame.aim[w].orientation, xr_forward, aim);
			for (axis = 0; axis < 3; axis++)
				between[axis] = vr.frame.grip[o].position[axis] - vr.frame.grip[w].position[axis];
			along = hands_apart > 0.001f ?
				(between[0] * aim[0] + between[1] * aim[1] + between[2] * aim[2]) / hands_apart : 1.0f;
			if (fabsf(hands_apart - vr.support_engaged_apart) > 0.20f || along < 0.5f)
				release = 1, why = " (hand pulled away)";
		}
		if (release)
		{
			platform_log("vr: support grip released%s", why);
			vr.two_hand_held = 0;
			vr.support_rearmed = 0;
			vr.support_dwell = 0.0f;
		}
	}
	else if (!both_tracked || !vr.aiming_last_frame || vr_hand_empty())
		vr.support_dwell = 0.0f;
}

/* zoomed in, the aim is eased toward the hand's: at zoom 1 by about 15
ms, at zoom 2 about 25 ms (the PC mod HaloCEVR's half-life formula,
h = 90 log2(1 - e^(-20s/9)) for s 0.4 and 0.6), steadying a scope */
static void steady_aim(void)
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f }, xr_up[3] = { 0.0f, 1.0f, 0.0f };
	float target[3], up[3], amount, half_life, t, length, seconds;
	int axis;

	rotate(vr.aim_pose.orientation, xr_forward, target);
	amount = vr.zoom_level == 0 ? 0.4f : vr.zoom_level >= 1 ? 0.6f : 0.0f;
	if (amount <= 0.0f || !vr.smoothed_valid)
	{
		memcpy(vr.smoothed, target, sizeof(target));
		vr.smoothed_valid = 1;
		return;
	}
	seconds = (float)(vr.frame.predicted_display_period * 1e-9);
	half_life = 90.0f * log2f(1.0f - expf(-20.0f * amount / 9.0f));
	t = 1.0f - exp2f(seconds * half_life);
	for (axis = 0; axis < 3; axis++)
		vr.smoothed[axis] += (target[axis] - vr.smoothed[axis]) * t;
	length = sqrtf(vr.smoothed[0] * vr.smoothed[0] + vr.smoothed[1] * vr.smoothed[1] + vr.smoothed[2] * vr.smoothed[2]);
	if (length < 1e-5f)
		return;
	for (axis = 0; axis < 3; axis++)
		vr.smoothed[axis] /= length;
	rotate(vr.aim_pose.orientation, xr_up, up);
	look_rotation(vr.smoothed, up, vr.aim_pose.orientation);
}

void vr_set_zoom_level(int zoom_level)
{
	if (vr.zoom_level != zoom_level)
		platform_log("vr: game zoom transition %d -> %d (%s); scope setting %s, weapon hand %s, last sampled two-hand state %s",
			vr.zoom_level, zoom_level, zoom_level < 0 ? "off" : "on", vr.scope_enabled ? "on" : "off",
			vr.weapon_hand ? "right" : "left", vr.two_handed ? "locked" : "one hand");
	vr.zoom_level = zoom_level;
}

void vr_support_near(int near_weapon)
{
	vr.support_near = near_weapon != 0;
	vr.support_time = vr_pose_time();
}

int vr_two_handed(void)
{
	return vr.two_handed;
}

void vr_haptic(int hand, float amplitude, float seconds)
{
	if (vr.haptics <= 0.0f || hand < 0 || hand > 1 || amplitude <= 0.0f)
		return;
	amplitude *= vr.haptics;
	/* the host's controller (it called itself here, without end: every buzz
	hung the game - a menu click, a shot, a holster) */
	host_xr_haptic((unsigned int)hand, amplitude > 1.0f ? 1.0f : amplitude, seconds);
}

unsigned int vr_take_actions(void)
{
	unsigned int actions = vr.actions;

	vr.actions = 0;
	/* the head lowered crouches on foot; seated, the crouch control is a
	driver's horn: a stick click sounds it (vr_horn_held), a lowered head
	does not (test21) */
	if (vr.seated ? vr_horn_held() : vr.crouching)
		actions |= VR_ACTION_CROUCH;
	return actions;
}

/* test24b: the vignette's clear radius (a tangent of the angle from
straight ahead) for a strength (0 to 1) and an amount (0 to 1): at nothing,
the eye's farthest corner (`corner`), so that it grows in from the very
edge; at a full amount clear to about 24, 33 or 39 degrees from straight
ahead for high, medium and low strengths */
float vr_vignette_aperture(float strength, float amount, float corner)
{
	float narrowest = 1.0f - 0.55f * strength;

	if (amount < 0.0f)
		amount = 0.0f;
	if (amount > 1.0f)
		amount = 1.0f;
	if (!(corner > narrowest))
		corner = narrowest;
	return corner - amount * (corner - narrowest);
}

/* test24b: whether the vignette is drawn over the eyes now: set on, the
player in control in stereo gameplay (no cutscene, no 3D screen, no menu
pointer), and some of it showing */
int vr_vignette_shown(void)
{
	return vr.vignette_strength > 0.0f && vr.stereo && !vr.cinema && (vr.aiming || vr.aiming_last_frame) &&
		vr.pointer_age == 0 && vr.vignette_amount > 0.01f;
}

int vr_weapon_hand(void)
{
	return vr.weapon_hand;
}

/* the direction the left stick moves the player in (vr.move_relative), as
a yaw at heading 0: the head's, or a hand's turned in by 20 degrees as
controllers are held */
static int hand_forward(float out[3]);

static float move_yaw(void)
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	int hand = vr.move_relative - 1;
	float local[3], forward[3];

	if (hand < 0 || !(vr.frame.hand_valid[hand] & 2))
		return vr.head_yaw;
	/* test29: both hands on the gun, the gun's line stands for the hand. A
	hand on a gun's front grip is turned to hold it, not pointed where the
	player walks: moving with the left hand, the stick's forward turned
	into a strafe while the gun was held in both hands, until let go (a
	player's report). The head's mode is unchanged */
	if (vr.two_handed && hand_forward(forward))
		return atan2f(forward[1], forward[0]);
	rotate(vr.frame.aim[hand].orientation, xr_forward, local);
	to_halo(local, 1.0f, 0.0f, forward);
	return atan2f(forward[1], forward[0]) + (hand ? 0.349f : -0.349f);
}

int vr_controller(unsigned int *buttons, float trigger[2], float thumb[4])
{
	if (!vr.active || !(vr.frame.flags & HALO_XR_FRAME_FOCUSED))
		return 0;
	*buttons = vr.pad_buttons;
	memcpy(trigger, vr.pad_trigger, sizeof(vr.pad_trigger));
	memcpy(thumb, vr.frame.thumb, sizeof(vr.frame.thumb));
	if (vr.aiming_last_frame)
	{
		/* the right stick turns the heading (vr_aim), the head looks. Its
		up and down still reach the game, which the head's aim overrides
		every frame anyway: scripts that wait for the player to look (the
		first level's look test) watch the stick, not the view */
		thumb[2] = 0.0f;
		/* both sticks pressed together recentre (vr_aim) */
		if ((*buttons & (HALO_XR_BUTTON_LEFT_THUMB | HALO_XR_BUTTON_RIGHT_THUMB)) ==
			(HALO_XR_BUTTON_LEFT_THUMB | HALO_XR_BUTTON_RIGHT_THUMB))
		{
			*buttons &= ~(HALO_XR_BUTTON_LEFT_THUMB | HALO_XR_BUTTON_RIGHT_THUMB);
		}
	}
	/* the arms' run (vr.arm_run) pushes the stick ahead, on foot */
	/* Deliberate stick movement always wins, including backing away while
	gripping. Hand motion must not turn a backward input into forward motion. */
	if (vr.run_push > 0.0f && vr.aiming_last_frame && !vr.seated &&
		fabsf(thumb[0]) < 0.15f && fabsf(thumb[1]) < 0.15f)
		thumb[1] = vr.run_push;
	if (vr.aiming_last_frame && !vr.seated)
	{
		/* the game moves the player relative to the aim; the stick means
		relative to the head (or a hand, vr.move_relative): turn it by that
		yaw from the aim's (x right, y forward; a turn left is positive) */
		float angle = wrap_angle(move_yaw() - vr.aim_yaw);
		float c = cosf(angle), s = sinf(angle);
		float x = thumb[0], y = thumb[1];

		thumb[0] = x * c - y * s;
		thumb[1] = x * s + y * c;
	}
	return 1;
}

/* Called only for a newly acquired, actually recentred runtime frame. */
static void vehicle_recentered(void)
{
	if (!vr.vehicle_seat.known)
		return;
	memcpy(vr.vehicle_seat.origin, vr.frame.head.position, sizeof(vr.vehicle_seat.origin));
	vr.vehicle_seat.origin_valid = vr.seated;
	vr.vehicle_seat.recentre_pending = 0;
}

/* begins the runtime's next frame unless one is begun; 0 when the session
is not running (the host polled and slept) */
static int frame_begin(void)
{
	double start;

	if (vr.frame_begun)
		return 1;
	start = now_ms();
	if (!host_xr_begin_frame(&vr.frame))
	{
		if (vr.frame.flags & HALO_XR_FRAME_EXIT)
		{
			platform_log("vr: the runtime ended the session; exiting");
			exit(0);
		}
		return 0;
	}
	vr.frame_begun = 1;
	/* the views cut to the glasses window (vr.fov_mode), and
	the eye images remade at a new size before any of this frame is drawn */
	apply_glasses();
	apply_eye_images();
	/* a recentre moves the room's origin: walking resumes from the head */
	if (vr.frame.flags & HALO_XR_FRAME_RECENTRED)
	{
		vr.room_previous[0] = vr.room_now[0] = vr.frame.head.position[0];
		vr.room_previous[1] = vr.room_now[1] = vr.frame.head.position[2];
		vr.room_held = 1;
		vehicle_recentered();
	}
	/* left-handed (vr.controls_mirrored): the sticks trade jobs, so the
	off hand moves and the gun hand turns; their clicks follow with the
	face buttons (layout_controls) */
	if (vr.controls_mirrored)
	{
		float move[2] = { vr.frame.thumb[0], vr.frame.thumb[1] };

		vr.frame.thumb[0] = vr.frame.thumb[2];
		vr.frame.thumb[1] = vr.frame.thumb[3];
		vr.frame.thumb[2] = move[0];
		vr.frame.thumb[3] = move[1];
	}
	layout_controls();
    /* Once per freshly acquired frame, before gestures/weapon rays/IK. Never
     * accumulate corrections on the previous corrected frame. */
    {
        int h;
        for(h=0;h<2;h++) {
            if(vr.alignment_grip_aim[h]) {
                if(vr.frame.hand_valid[h]&1) { vr.frame.aim[h]=vr.frame.grip[h]; vr.frame.hand_valid[h]|=2; }
                else vr.frame.hand_valid[h]&=~2u;
            }
            /* one rigid correction per controller: armed (aim) and empty (grip)
             * hands stay consistent with each other (vr_alignment.h) */
            vr.frame.hand_valid[h] = (vr.frame.hand_valid[h] & ~3u) |
                vr_alignment_apply_controller(vr.frame.grip[h].position, vr.frame.grip[h].orientation,
                    vr.frame.aim[h].position, vr.frame.aim[h].orientation, vr.frame.hand_valid[h] & 3u,
                    vr.alignment_rotation[h], vr.alignment_offset[h]);
        }
    }
	update_gestures();
	vr.frame_start = now_ms();
	vr.wait_ms += vr.frame_start - start;
	return 1;
}

void vr_pass_mark(int pass, int end)
{
	double now;

	/* the scope's pass (5) is timed in the fourth slot */
	if (pass == 5)
		pass = 3;
	if (!vr.timing || pass < 0 || pass > 3)
		return;
	now = now_ms();
	if (end)
		vr.pass_ms[pass] += now - vr.pass_start;
	else
		vr.pass_start = now;
}

static void frame_end(const struct halo_xr_layers *layers)
{
	if (!vr.frame_begun)
		return;
	host_xr_end_frame(layers);
	vr.frame_begun = 0;
}

static void dump_image(unsigned int which, int index, const char *name);

/* this frame's images are to be written (vr.dump_frame of stereo play,
vr.dump_cinema_frame of cutscenes) */
static int dumping(void)
{
	return (vr.stereo && vr.dump_frame > 0 && vr.stereo_frames == vr.dump_frame) ||
		(vr.cinema && vr.dump_cinema_frame > 0 && vr.cinema_frames == vr.dump_cinema_frame);
}

static void draw_vignette(unsigned int which);

/* copies framebuffer `source` (row 0 at the top) into the acquired image
of a swapchain, filling it */
static int copy_to_swapchain(unsigned int which, GLuint source, int width, int height)
{
	static const char *const names[] = { "vr-eye0.bmp", "vr-eye1.bmp", "vr-hud.bmp" };
	int index = host_xr_acquire(which);

	if (index < 0)
		return 0;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, source);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[which][index], 0);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (vr.srgb_write_control)
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	/* OpenXR images have row 0 at the bottom */
	glBlitFramebuffer(0, 0, width, height, 0, (GLint)vr.info.height[which], (GLint)vr.info.width[which], 0,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
	/* test24b: the comfort vignette over a gameplay eye (the renderer takes
	up its own GL state anew after an eye: xgpu_gl_state_invalidate) */
	if (which < 2 && vr_vignette_shown())
		draw_vignette(which);
	if (vr.srgb_write_control)
		glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	/* vr.dump_frame: the stereo frame's images, as they are shown */
	if (dumping() && which < 3)
		dump_image(which, index, names[which]);
	host_xr_release(which);
	return 1;
}

/* ---------- stereo */

/* vr.force_render: a head 1.6 m up looking ahead (turned by vr.diag_yaw),
for runs with the headset off (in standby the runtime shows nothing and
tracks nothing) */
static void synthesize_views(void)
{
	static const float fov[2][4] = { { -1.028f, 0.885f, 0.864f, -1.048f }, { -0.885f, 1.028f, 0.864f, -1.048f } };
	float yaw[4] = { 0.0f, sinf(vr.diag_yaw * 0.5f), 0.0f, cosf(vr.diag_yaw * 0.5f) };
	int eye;

	float walk[3] = { 0.0f, 0.0f, 0.0f };

	/* vr.diag_walk_speed: the head walking ahead, metres a second */
	if (vr.diag_walk_speed != 0.0f)
	{
		static const float ahead[3] = { 0.0f, 0.0f, -1.0f };
		float distance;

		if (vr.diag_walk_start <= 0.0)
			vr.diag_walk_start = now_ms();
		distance = vr.diag_walk_speed * (float)((now_ms() - vr.diag_walk_start) * 0.001);
		rotate(yaw, ahead, walk);
		walk[0] *= distance;
		walk[2] *= distance;
		walk[1] = 0.0f;
	}
	memset(&vr.frame.head, 0, sizeof(vr.frame.head));
	memcpy(vr.frame.head.orientation, yaw, sizeof(yaw));
	memcpy(vr.frame.head.position, walk, sizeof(walk));
	for (eye = 0; eye < 2; eye++)
	{
		float offset[3] = { eye ? 0.032f : -0.032f, 0.0f, 0.0f };
		int axis;

		rotate(yaw, offset, vr.frame.eye[eye].position);
		for (axis = 0; axis < 3; axis++)
			vr.frame.eye[eye].position[axis] += walk[axis];
		memcpy(vr.frame.eye[eye].orientation, yaw, sizeof(yaw));
		memcpy(vr.frame.fov[eye], fov[eye], sizeof(fov[eye]));
	}
	/* hands held ahead at the hips' height, the right one turned by
	vr.diag_hand_yaw from the head */
	{
		float hand_yaw = vr.diag_yaw + vr.diag_hand_yaw;
		float turned[4] = { 0.0f, sinf(hand_yaw * 0.5f), 0.0f, cosf(hand_yaw * 0.5f) };
		int hand;

		for (hand = 0; hand < 2; hand++)
		{
			float offset[3] = { hand ? 0.18f : -0.18f, -0.30f, -0.35f };

			int axis;

			rotate(yaw, offset, vr.frame.grip[hand].position);
			for (axis = 0; axis < 3; axis++)
				vr.frame.grip[hand].position[axis] += walk[axis];
			memcpy(vr.frame.aim[hand].position, vr.frame.grip[hand].position, sizeof(offset));
			memcpy(vr.frame.grip[hand].orientation, hand == vr.weapon_hand ? turned : yaw, sizeof(yaw));
			memcpy(vr.frame.aim[hand].orientation, hand == vr.weapon_hand ? turned : yaw, sizeof(yaw));
			vr.frame.hand_valid[hand] = 3;
		}
		if (vr.diag_two_handed)
		{
			/* the left hand on the gun: 30 cm ahead of the right, 10 cm left */
			float ahead[3] = { -0.10f, 0.0f, -0.30f }, turned_ahead[3];

			rotate(turned, ahead, turned_ahead);
			vr.frame.grip[0].position[0] = vr.frame.grip[1].position[0] + turned_ahead[0];
			vr.frame.grip[0].position[1] = vr.frame.grip[1].position[1] + turned_ahead[1];
			vr.frame.grip[0].position[2] = vr.frame.grip[1].position[2] + turned_ahead[2];
		}
	}
	vr.frame.flags |= HALO_XR_FRAME_SHOULD_RENDER | HALO_XR_FRAME_VIEWS_VALID;
}

int vr_stereo_begin(void)
{
	const unsigned int needed = HALO_XR_FRAME_SHOULD_RENDER | HALO_XR_FRAME_VIEWS_VALID;

	if (!vr.active || !frame_begin())
		return 0;
	if (vr.force_render && ((vr.frame.flags & needed) != needed || vr.diag_yaw != 0.0f ||
		vr.diag_walk_speed != 0.0f))
		synthesize_views();
	vr.stereo = vr.stereo_enabled && (vr.frame.flags & needed) == needed;
	return vr.stereo;
}

int vr_cinema_begin(void)
{
	if (!vr.active || !frame_begin())
		return 0;
	if (vr.force_render && !(vr.frame.flags & HALO_XR_FRAME_SHOULD_RENDER))
		synthesize_views();
	vr.cinema = vr.cinema_enabled && (vr.frame.flags & HALO_XR_FRAME_SHOULD_RENDER);
	return vr.cinema;
}

int vr_cinema_eye(int eye, float *offset_units, float *convergence_tangent)
{
	float half = vr.cinema_separation * 0.5f * vr.units_per_metre;

	if (!vr.cinema || eye < 0 || eye > 1)
		return 0;
	/* the left eye left of the camera (negative along its right) */
	*offset_units = eye ? half : -half;
	/* each eye's frustum turned in to meet the other's at the convergence
	distance, which then shows at the screen's depth */
	*convergence_tangent = vr.cinema_convergence > 0.0f ? -*offset_units / vr.cinema_convergence : 0.0f;
	return 1;
}

/* q * v for a unit quaternion (x, y, z, w) */
static void rotate(const float q[4], const float v[3], float out[3])
{
	float tx = 2.0f * (q[1] * v[2] - q[2] * v[1]);
	float ty = 2.0f * (q[2] * v[0] - q[0] * v[2]);
	float tz = 2.0f * (q[0] * v[1] - q[1] * v[0]);

	out[0] = v[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
	out[1] = v[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
	out[2] = v[2] + q[3] * tz + (q[0] * ty - q[1] * tx);
}

/* an OpenXR LOCAL vector (+x right, +y up, -z forward) in Halo's axes for
a viewer whose heading is (cosine, sine) */
static void to_halo(const float v[3], float cosine, float sine, float out[3])
{
	float forward = -v[2], left = -v[0], up = v[1];

	out[0] = forward * cosine - left * sine;
	out[1] = forward * sine + left * cosine;
	out[2] = up;
}

/* how far the head may move the view from the game's camera, in metres:
about as far as the body could lean (the game keeps only its own camera
out of walls) */
#define HEAD_REACH 0.35f

/* port/linux/game/render_interpolation.c: how far this frame is between
the two ticks the game's camera is blended from */
float render_interpolation_fraction(void);

/* the head's offset from the recentred origin (with vr.roomscale, from the
floor's point the player stands on, as far between its last two ticks as
the game's camera is: walking, the camera then carries the head and the
offset only what the last tick has not), limited to HEAD_REACH */
static void head_offset(float out[3])
{
	float head[3], length, scale;

	memcpy(head, vr.frame.head.position, sizeof(head));
	if (vr.vehicle_seat.origin_valid || vr.vehicle_seat.recentre_pending)
	{
		/* Keep head translation relative to this seat, including on a co-op
		client. Do not rebase every frame: leaning must remain six-degree. */
		for (int axis = 0; axis < 3; axis++)
			head[axis] -= vr.vehicle_seat.origin[axis];
	}
	else if (vr.roomscale)
	{
		float t = render_interpolation_fraction();

		head[0] -= vr.room_previous[0] + (vr.room_now[0] - vr.room_previous[0]) * t;
		head[2] -= vr.room_previous[1] + (vr.room_now[1] - vr.room_previous[1]) * t;
	}
	length = sqrtf(head[0] * head[0] + head[1] * head[1] + head[2] * head[2]);
	scale = length > HEAD_REACH ? HEAD_REACH / length : 1.0f;
	out[0] = head[0] * scale;
	out[1] = head[1] * scale;
	out[2] = head[2] * scale;
}

/* ---------- room-scale (vr.roomscale), after the PC mod HaloCEVR's: each
tick the player is moved by how far the head walked from where they stand */

/* a step of more than this (metres) in a tick is no walk (tracking lost
and found, a recentre): it is taken up without moving */
#define ROOM_STEP_LIMIT 0.5f

int vr_room_step(float out_step[2])
{
	float step[3], halo[3];
	const float *head = vr.frame.head.position;

	out_step[0] = out_step[1] = 0.0f;
	if (!vr.roomscale || !vr.active || !vr.heading_valid || vr.vehicle_seat.recentre_pending ||
		(vr.frame.flags & (HALO_XR_FRAME_VIEWS_VALID | HALO_XR_FRAME_RECENTRED)) != HALO_XR_FRAME_VIEWS_VALID)
	{
		vr_room_hold();
		return 0;
	}
	if (vr.room_held)
	{
		vr.room_previous[0] = vr.room_now[0] = head[0];
		vr.room_previous[1] = vr.room_now[1] = head[2];
		vr.room_held = 0;
		return 0;
	}
	step[0] = head[0] - vr.room_now[0];
	step[1] = 0.0f;
	step[2] = head[2] - vr.room_now[1];
	if (step[0] * step[0] + step[2] * step[2] > ROOM_STEP_LIMIT * ROOM_STEP_LIMIT)
	{
		vr.room_held = 1;
		return vr_room_step(out_step);
	}
	to_halo(step, cosf(vr.heading), sinf(vr.heading), halo);
	out_step[0] = halo[0] * vr.units_per_metre;
	out_step[1] = halo[1] * vr.units_per_metre;
	return 1;
}

void vr_room_moved(void)
{
	/* the whole step is taken, even where a wall stopped the player: the
	view stays with the player, not in the wall */
	vr.room_previous[0] = vr.room_now[0];
	vr.room_previous[1] = vr.room_now[1];
	vr.room_now[0] = vr.frame.head.position[0];
	vr.room_now[1] = vr.frame.head.position[2];
}

void vr_room_hold(void)
{
	/* the head leans from where the player last stood until walking resumes */
	vr.room_previous[0] = vr.room_now[0];
	vr.room_previous[1] = vr.room_now[1];
	vr.room_held = 1;
}

/* a view at `offset` (LOCAL metres) turned by `orientation`, for a viewer
at `position` with the heading of `forward` */
static void view(const float offset[3], const float orientation[4], const float position[3],
	const float forward[3], float out_position[3], float out_forward[3], float out_up[3])
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f }, xr_up[3] = { 0.0f, 1.0f, 0.0f };
	float heading = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);
	float cosine = 1.0f, sine = 0.0f;
	float halo[3], local[3];

	if (heading > 1e-4f)
	{
		cosine = forward[0] / heading;
		sine = forward[1] / heading;
	}
	to_halo(offset, cosine, sine, halo);
	out_position[0] = position[0] + halo[0] * vr.units_per_metre;
	out_position[1] = position[1] + halo[1] * vr.units_per_metre;
	out_position[2] = position[2] + halo[2] * vr.units_per_metre;
	rotate(orientation, xr_forward, local);
	to_halo(local, cosine, sine, out_forward);
	rotate(orientation, xr_up, local);
	to_halo(local, cosine, sine, out_up);
}

int vr_eye_view(int eye, const float position[3], const float forward[3], float aspect,
	float out_position[3], float out_forward[3], float out_up[3], float bounds[4])
{
	const struct halo_xr_pose *pose;
	float offset[3];
	int axis;

	if (!vr.stereo || eye < 0 || eye > 1)
		return 0;
	pose = &vr.frame.eye[eye];
	/* the head limited, the eye's own offset from it kept whole */
	head_offset(offset);
	for (axis = 0; axis < 3; axis++)
		offset[axis] += pose->position[axis] - vr.frame.head.position[axis];
	view(offset, pose->orientation, position, forward, out_position, out_forward, out_up);
	/* render_camera_build_frustum: x spans tangent * aspect, y the tangent */
	bounds[0] = tanf(vr.frame.fov[eye][0]) / aspect;
	bounds[1] = tanf(vr.frame.fov[eye][1]) / aspect;
	bounds[2] = tanf(vr.frame.fov[eye][3]);
	bounds[3] = tanf(vr.frame.fov[eye][2]);
	return 1;
}

/* the head's forward in Halo's axes at heading 0 */
static void head_forward(float out[3])
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	float local[3];

	rotate(vr.frame.head.orientation, xr_forward, local);
	to_halo(local, 1.0f, 0.0f, out);
}

static float wrap_angle(float angle)
{
	while (angle > 3.14159265f)
		angle -= 6.28318531f;
	while (angle < -3.14159265f)
		angle += 6.28318531f;
	return angle;
}

/* test24b: how far a stick is pushed, past its dead zone, 0 to 1 */
static float comfort_stick(float x, float y)
{
	float length = sqrtf(x * x + y * y);

	if (!(length > 0.15f))
		return 0.0f;
	length = (length - 0.15f) / 0.6f;
	return length > 1.0f ? 1.0f : length;
}

/* test24b: the vignette's amount this frame, eased toward what the
setting shows it for: in quickly (0.12 s), out gently (0.35 s) */
static void comfort_update(double seconds)
{
	float target = vr.vignette_when == VR_VIGNETTE_ALWAYS ? 1.0f : vr.comfort_turn;

	if (!(seconds > 0.0) || seconds > 0.1)
		seconds = 1.0 / 72.0;
	if (vr.snap_pulse > target)
		target = vr.snap_pulse;
	if (vr.vignette_when == VR_VIGNETTE_MOVING && vr.comfort_move > target)
		target = vr.comfort_move;
	if (vr.vignette_amount < target)
	{
		vr.vignette_amount += (float)(seconds / 0.12);
		if (vr.vignette_amount > target)
			vr.vignette_amount = target;
	}
	else
	{
		vr.vignette_amount -= (float)(seconds / 0.35);
		if (vr.vignette_amount < target)
			vr.vignette_amount = target;
	}
	vr.snap_pulse -= (float)(seconds / 0.25);
	if (vr.snap_pulse < 0.0f)
		vr.snap_pulse = 0.0f;
}

/* the right stick: snap turns, or smooth ones at vr.smooth_turn_speed */
static void turn(void)
{
	float x = vr.frame.thumb[2];
	double seconds = vr.frame.predicted_display_period * 1e-9;

	if (!(vr.frame.flags & HALO_XR_FRAME_FOCUSED))
		return;
	vr.comfort_turn = vr.snap_turn > 0.0f ? 0.0f : comfort_stick(x, 0.0f);
	if (vr.snap_turn > 0.0f)
	{
		if (vr.snap_armed && (x > 0.7f || x < -0.7f))
		{
			/* pushed right turns right: yaw grows to the left */
			vr.heading = wrap_angle(vr.heading + (x > 0.0f ? -vr.snap_turn : vr.snap_turn));
			vr.snap_armed = 0;
			vr.snap_pulse = 1.0f;
		}
		else if (x < 0.3f && x > -0.3f)
		{
			vr.snap_armed = 1;
		}
	}
	else if (x > 0.15f || x < -0.15f)
	{
		vr.heading = wrap_angle(vr.heading - x * vr.smooth_turn_speed * (float)seconds);
	}
}

/* a unit quaternion whose -z is `forward` and whose +y is as near `up`
as that allows */
static void look_rotation(const float forward[3], const float up[3], float out[4])
{
	float z[3] = { -forward[0], -forward[1], -forward[2] }, x[3], y[3], length, trace;

	x[0] = up[1] * z[2] - up[2] * z[1];
	x[1] = up[2] * z[0] - up[0] * z[2];
	x[2] = up[0] * z[1] - up[1] * z[0];
	length = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	if (length < 1e-5f)
	{
		out[0] = out[1] = out[2] = 0.0f;
		out[3] = 1.0f;
		return;
	}
	x[0] /= length; x[1] /= length; x[2] /= length;
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] = z[2] * x[0] - z[0] * x[2];
	y[2] = z[0] * x[1] - z[1] * x[0];
	/* the columns x, y, z as a quaternion */
	trace = x[0] + y[1] + z[2];
	if (trace > 0.0f)
	{
		float r = sqrtf(1.0f + trace) * 2.0f;

		out[3] = 0.25f * r;
		out[0] = (y[2] - z[1]) / r;
		out[1] = (z[0] - x[2]) / r;
		out[2] = (x[1] - y[0]) / r;
	}
	else if (x[0] > y[1] && x[0] > z[2])
	{
		float r = sqrtf(1.0f + x[0] - y[1] - z[2]) * 2.0f;

		out[3] = (y[2] - z[1]) / r;
		out[0] = 0.25f * r;
		out[1] = (y[0] + x[1]) / r;
		out[2] = (z[0] + x[2]) / r;
	}
	else if (y[1] > z[2])
	{
		float r = sqrtf(1.0f + y[1] - x[0] - z[2]) * 2.0f;

		out[3] = (z[0] - x[2]) / r;
		out[0] = (y[0] + x[1]) / r;
		out[1] = 0.25f * r;
		out[2] = (z[1] + y[2]) / r;
	}
	else
	{
		float r = sqrtf(1.0f + z[2] - x[0] - y[1]) * 2.0f;

		out[3] = (x[1] - y[0]) / r;
		out[0] = (z[0] + x[2]) / r;
		out[1] = (z[1] + y[2]) / r;
		out[2] = 0.25f * r;
	}
}

/* the aim this frame: the right controller's, unless the left hand holds
the gun ahead of it (between 12 and 60 cm along the right hand's aim and
within 35 degrees of it), when the gun points from the right hand to the
left, as a rifle held in both does */
static void steady_aim(void);

static void compute_aim_pose(void)
{
	static const float xr_up[3] = { 0.0f, 1.0f, 0.0f };
	float between[3], length, up[3];

	int w = vr.weapon_hand, o = 1 - vr.weapon_hand;

	vr.aim_pose = vr.frame.aim[w];
	/* one-handed: the gun's own angle on the controller (vr.weapon_*);
	both hands' line replaces it below */
	vr_alignment_multiply(vr.frame.aim[w].orientation, vr.weapon_rotation[w], vr.aim_pose.orientation);
	vr.two_handed = 0;
	if (!vr.two_handed_enabled || vr_hand_empty() ||
		(vr.frame.hand_valid[w] & 3) != 3 || !(vr.frame.hand_valid[o] & 1))
		return;
	between[0] = vr.frame.grip[o].position[0] - vr.frame.grip[w].position[0];
	between[1] = vr.frame.grip[o].position[1] - vr.frame.grip[w].position[1];
	between[2] = vr.frame.grip[o].position[2] - vr.frame.grip[w].position[2];
	length = sqrtf(between[0] * between[0] + between[1] * between[1] + between[2] * between[2]);
	/* locked (squeezed, or automatically at the support grip): the off
	hand holds the gun wherever it is, up to an arm's reach apart */
	if (!vr.two_hand_held || length < 0.05f)
		return;
	between[0] /= length;
	between[1] /= length;
	between[2] /= length;
	/* test21: the gun's up from its calibrated one-handed aim (vr.weapon_*,
	set above), not the raw controller: a gun rolled upright with Gun Roll
	(the Quest OS v78 report: hands 180 degrees off) stayed upright in one
	hand but turned upside down again in two */
	rotate(vr.aim_pose.orientation, xr_up, up);
	look_rotation(between, up, vr.aim_pose.orientation);
	vr.two_handed = 1;
}

/* test21: the shots' aim: the gun's, turned by its kind's adjustment
(none: exactly the gun's) */
static void update_shot_pose(void)
{
	vr.shot_pose = vr.aim_pose;
	if (vr.gun_class >= 0 && vr.gun_class < VR_GUN_CLASSES && vr.gun_aim_turned[vr.gun_class])
		vr_alignment_multiply(vr.aim_pose.orientation, vr.gun_aim_rotation[vr.gun_class], vr.shot_pose.orientation);
}

static void update_aim_pose(void)
{
	static int previous_two_handed = -1;
	static int previous_zoom_level = -999;
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	static const float xr_right[3] = { 1.0f, 0.0f, 0.0f };
	static const float xr_up[3] = { 0.0f, 1.0f, 0.0f };
	int two_handed;
	int two_hand_changed, zoom_changed;

	compute_aim_pose();
	two_handed = vr.two_handed;
	two_hand_changed = previous_two_handed != two_handed && (previous_two_handed == 1 || two_handed == 1);
	zoom_changed = previous_zoom_level != vr.zoom_level && previous_zoom_level != -999;
	if (two_hand_changed || (zoom_changed && two_handed))
	{
		if (two_handed)
		{
			float between[3], one_hand_orientation[4], one_hand_forward[3], one_hand_right[3], one_hand_up[3];
			float length, dot, line_right, line_up;
			int w = vr.weapon_hand, o = 1 - vr.weapon_hand, axis;

			for (axis = 0; axis < 3; axis++)
				between[axis] = vr.frame.grip[o].position[axis] - vr.frame.grip[w].position[axis];
			length = sqrtf(between[0] * between[0] + between[1] * between[1] + between[2] * between[2]);
			if (length > 1.0e-5f && isfinite(length))
			{
				for (axis = 0; axis < 3; axis++)
					between[axis] /= length;
				vr_alignment_multiply(vr.frame.aim[w].orientation, vr.weapon_rotation[w], one_hand_orientation);
				rotate(one_hand_orientation, xr_forward, one_hand_forward);
				rotate(one_hand_orientation, xr_right, one_hand_right);
				rotate(one_hand_orientation, xr_up, one_hand_up);
				dot = one_hand_forward[0] * between[0] + one_hand_forward[1] * between[1] +
					one_hand_forward[2] * between[2];
				if (dot > 1.0f) dot = 1.0f;
				if (dot < -1.0f) dot = -1.0f;
				line_right = one_hand_right[0] * between[0] + one_hand_right[1] * between[1] + one_hand_right[2] * between[2];
				line_up = one_hand_up[0] * between[0] + one_hand_up[1] * between[1] + one_hand_up[2] * between[2];
				if (isfinite(dot) && isfinite(line_right) && isfinite(line_up))
					platform_log("vr: two-hand aim %s: weapon hand %s, grip separation %.3f m, "
						"main-hand to grip-line angle %.1f deg (forward/right/up %.3f/%.3f/%.3f), "
						"tracking L/R 0x%x/0x%x, zoom %d, scope %s, weapon class %d; aim behavior unchanged",
						two_hand_changed ? "engaged" : "sampled at zoom change", vr.weapon_hand ? "right" : "left", length,
						acosf(dot) * 57.2957795f, dot, line_right, line_up,
						(unsigned)vr.frame.hand_valid[0], (unsigned)vr.frame.hand_valid[1], vr.zoom_level,
						vr.scope_enabled ? "on" : "off", vr.gun_class);
				else
					platform_log("vr: two-hand aim %s: pose angle unavailable; weapon hand %s, tracking L/R 0x%x/0x%x, "
						"zoom %d, scope %s, weapon class %d; aim behavior unchanged",
						two_hand_changed ? "engaged" : "sampled at zoom change", vr.weapon_hand ? "right" : "left",
						(unsigned)vr.frame.hand_valid[0], (unsigned)vr.frame.hand_valid[1], vr.zoom_level,
						vr.scope_enabled ? "on" : "off", vr.gun_class);
			}
			else
				platform_log("vr: two-hand aim %s: grip distance unavailable; weapon hand %s, tracking L/R 0x%x/0x%x, "
					"zoom %d, scope %s, weapon class %d; aim behavior unchanged",
					two_hand_changed ? "engaged" : "sampled at zoom change", vr.weapon_hand ? "right" : "left",
					(unsigned)vr.frame.hand_valid[0], (unsigned)vr.frame.hand_valid[1], vr.zoom_level,
					vr.scope_enabled ? "on" : "off", vr.gun_class);
		}
		else if (two_hand_changed)
			platform_log("vr: two-hand aim released: weapon hand %s, tracking L/R 0x%x/0x%x, zoom %d, scope %s, "
				"weapon class %d; aim behavior unchanged",
				vr.weapon_hand ? "right" : "left", (unsigned)vr.frame.hand_valid[0],
				(unsigned)vr.frame.hand_valid[1], vr.zoom_level, vr.scope_enabled ? "on" : "off", vr.gun_class);
	}
	previous_two_handed = two_handed;
	previous_zoom_level = vr.zoom_level;
	steady_aim();
	update_shot_pose();
}

/* the weapon hand's aim in Halo's axes at heading 0; 0 untracked */
static int hand_forward(float out[3])
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	float local[3];

	if (!(vr.frame.hand_valid[vr.weapon_hand] & 2))
		return 0;
	rotate(vr.shot_pose.orientation, xr_forward, local);
	to_halo(local, 1.0f, 0.0f, out);
	return 1;
}

/* The guest validates the actual local occupant independently of whether
facing input was allowed this tick. A transition requests one runtime recenter;
until that new frame arrives, the existing pose is centred locally as well. */
void vr_vehicle_seat(long unit_index, long vehicle_index, int seat_index, int role)
{
	int seated = vehicle_index != -1 && seat_index >= 0;
	int was_seated = vr.vehicle_seat.known && vr.vehicle_seat.vehicle != -1;
	int changed = seated != was_seated || (seated &&
		(unit_index != vr.vehicle_seat.unit || vehicle_index != vr.vehicle_seat.vehicle ||
		 seat_index != vr.vehicle_seat.index));

	vr.vehicle_seat.known = 1;
	vr.vehicle_seat.unit = unit_index;
	vr.vehicle_seat.vehicle = seated ? vehicle_index : -1;
	vr.vehicle_seat.index = seated ? seat_index : -1;
	vr.vehicle_seat.role = seated ? role : 0;
	vr.seated = seated;
	if (!changed)
		return;
	reset_hud_tap();
	vr.vehicle_seat.origin_valid = seated;
	vr.vehicle_seat.recentre_pending = vr.active;
	memcpy(vr.vehicle_seat.origin, vr.frame.head.position, sizeof(vr.vehicle_seat.origin));
	vr.room_previous[0] = vr.room_now[0] = vr.frame.head.position[0];
	vr.room_previous[1] = vr.room_now[1] = vr.frame.head.position[2];
	vr.room_held = 1;
	vr.heading_valid = 0;
	if (vr.active)
	{
		vr.recentre_source = !seated ? 4 : was_seated ? 5 : 3;
		host_xr_recenter();
		platform_log("vr: vehicle tracking: %s; role %s, seat %d; position origin %.3f/%.3f/%.3f, runtime recenter queued",
			!seated ? "exit" : was_seated ? "seat transfer" : "enter",
			role == 1 ? "driver" : role == 2 ? "gunner" : seated ? "passenger" : "on foot",
			seated ? seat_index : -1, vr.vehicle_seat.origin[0], vr.vehicle_seat.origin[1], vr.vehicle_seat.origin[2]);
	}
}

/* test25: the vehicle report (a Warthog's view sideways after a
recentre while driving, kept through getting out and in and through
switching views): each recentre (and what asked for it), each seat
entered or left and each view switched while seated, with the angles that
decide the view: the heading the eyes turn from, the head's yaw in the
room, the aim's (a hand's when steering by hand), the game's and the
seat's. Once per event, never per frame */
static void aim_diagnostics(float game_yaw, int seated, int hand_may_aim, const float *base_heading,
	float heading_before)
{
	static int previous_vehicle_source = -99;
	static int previous_vehicle_tracking = -1;
	static const char *const sources[] = { "the system or the headset regaining focus", "both sticks", "View held", "vehicle entry", "vehicle exit", "seat transfer" };
	static const char *const steering[] = { "stick", "on foot", "head", "right hand", "left hand" };
	int state = seated ? (base_heading ? 2 : 1) : 0;
	const char *aim = hand_may_aim >= -1 && hand_may_aim <= 3 ? steering[hand_may_aim + 1] : "?";
	char seat[48] = "";

	if (base_heading)
		snprintf(seat, sizeof(seat), ", seat %.1f", *base_heading * 57.29578f);
	if (seated && (hand_may_aim == 2 || hand_may_aim == 3))
	{
		int hand = hand_may_aim == 2 ? 1 : 0;
		int tracked = (vr.frame.hand_valid[hand] & 2) != 0;
		if (previous_vehicle_source != hand_may_aim || previous_vehicle_tracking != tracked)
		{
			platform_log("vr: seated hand-aim source %s: controller orientation %s; native-facing fallback %s",
				hand ? "right" : "left", tracked ? "tracked" : "unavailable",
				tracked ? "off" : "active");
			previous_vehicle_source = hand_may_aim;
			previous_vehicle_tracking = tracked;
		}
	}
	else
	{
		previous_vehicle_source = -99;
		previous_vehicle_tracking = -1;
	}
	if (vr.frame.flags & HALO_XR_FRAME_RECENTRED)
	{
		platform_log("vr: recentre (%s): %s, heading %.1f -> %.1f, head %.1f, aim %.1f (%s), game %.1f%s",
			sources[vr.recentre_source >= 0 && vr.recentre_source <= 5 ? vr.recentre_source : 0],
			state == 2 ? "seated, first person" : state == 1 ? "seated, third person" : "on foot",
			heading_before * 57.29578f, vr.heading * 57.29578f, vr.head_yaw * 57.29578f, vr.aim_yaw * 57.29578f,
			aim, game_yaw * 57.29578f, seat);
		vr.recentre_source = 0;
	}
	if (state != vr.seat_logged)
	{
		if (vr.seat_logged >= 0)
			platform_log("vr: seat: %s -> %s, heading %.1f -> %.1f, head %.1f, aim %.1f (%s), game %.1f%s",
				vr.seat_logged == 2 ? "first person" : vr.seat_logged == 1 ? "third person" : "on foot",
				state == 2 ? "first person" : state == 1 ? "third person" : "on foot",
				heading_before * 57.29578f, vr.heading * 57.29578f, vr.head_yaw * 57.29578f,
				vr.aim_yaw * 57.29578f, aim, game_yaw * 57.29578f, seat);
		vr.seat_logged = state;
	}
}

float vr_vehicle_tilt(void)
{
	return vr.vehicle_tilt;
}

int vr_aim(float game_yaw, int seated, int hand_may_aim, const float *base_heading, float out_forward[3])
{
	const unsigned int needed = HALO_XR_FRAME_SHOULD_RENDER | HALO_XR_FRAME_VIEWS_VALID;
	unsigned int both = HALO_XR_BUTTON_LEFT_THUMB | HALO_XR_BUTTON_RIGHT_THUMB;
	float head[3], aim[3], head_yaw, cosine, sine, heading_before;
	int steering_valid = 1;

	vr.aiming = 0;
	vr.hand_aiming = 0;
	if (!vr.active || !vr.stereo_enabled || !frame_begin())
		return 0;
	if (vr.force_render && ((vr.frame.flags & needed) != needed || vr.diag_yaw != 0.0f ||
		vr.diag_walk_speed != 0.0f))
		synthesize_views();
	if ((vr.frame.flags & needed) != needed)
		return 0;
	/* both sticks pressed: the way the player faces becomes forward */
	if ((vr.frame.buttons & both) == both)
	{
		if (!vr.recentre_held)
		{
			host_xr_recenter();
			vr.heading_valid = 0;
			vr.recentre_source = 1;
		}
		vr.recentre_held = 1;
	}
	else
	{
		vr.recentre_held = 0;
	}
	head_forward(head);
	update_aim_pose();
	/* the hand aims where the caller allows it (on foot; a driver's seat
	steered by hand), the head otherwise */
	memcpy(aim, head, sizeof(aim));
	if (hand_may_aim >= 2)
	{
		/* Vehicle controls use the selected physical controller, independent
		 * of weapon hand, support grip, scope and weapon smoothing. */
		static const float xr_forward[3] = {0.0f, 0.0f, -1.0f};
		int hand = hand_may_aim == 2 ? 1 : 0;
		float local[3];
		steering_valid = (vr.frame.hand_valid[hand] & 2) != 0;
		if (steering_valid) {
			rotate(vr.frame.aim[hand].orientation, xr_forward, local);
			to_halo(local, 1.0f, 0.0f, aim);
		}
	}
	else if (vr.hand_aim && hand_may_aim == 1 && hand_forward(aim))
		vr.hand_aiming = 1;
	/* the heading is kept so that the aim comes out as the game had it */
	head_yaw = atan2f(aim[1], aim[0]);
	vr.head_yaw = atan2f(head[1], head[0]);
	vr.aim_yaw = head_yaw;
	heading_before = vr.heading;
	if (vr.frame.flags & HALO_XR_FRAME_RECENTRED)
		vr.heading_valid = 0;
	/* the game turned the player itself (a script, a respawn, another pad):
	the heading follows, so that the head faces where the game faced (the
	hand then aims where it points). A seat limits how far its occupant
	turns, and the game holding the aim at that limit is not a turn: in a
	seat the heading is taken up only on getting in or out. */
	if (base_heading)
	{
		/* in a vehicle the view turns with it: the heading is the seat's */
		vr.heading = *base_heading;
		vr.heading_valid = 1;
	}
	else
	{
		if (!vr.heading_valid || !vr.aiming_last_frame || seated != vr.seated ||
			(!seated && fabsf(wrap_angle(game_yaw - vr.last_aim_yaw)) > 0.01f))
		{
			vr.heading = wrap_angle(game_yaw - vr.head_yaw);
			vr.heading_valid = 1;
		}
		if (hand_may_aim >= 0) turn();
	}
	/* test24b: the frame's motion, for the vignette: the move stick (a
	vehicle's throttle when seated), the arms' run on foot, and in a seat a
	stick that steers */
	{
		float move = comfort_stick(vr.frame.thumb[0], vr.frame.thumb[1]);

		if (!seated && vr.run_push > move)
			move = vr.run_push > 1.0f ? 1.0f : vr.run_push;
		vr.comfort_move = move;
		if (seated || base_heading)
			vr.comfort_turn = hand_may_aim < 0 ? comfort_stick(vr.frame.thumb[2], 0.0f) : 0.0f;
		comfort_update(vr.frame.predicted_display_period * 1e-9);
	}
	vr.seated = seated;
	aim_diagnostics(game_yaw, seated, hand_may_aim, base_heading, heading_before);
	/* Stick steering keeps the native look input. Update seat state even
	 * here so entering/exiting never inherits on-foot movement rotation. */
	if (hand_may_aim < 0) return 0;
	/* the game limits its pitch short of straight up or down (85.5
	degrees): so does the aim, keeping its heading */
	{
		float horizontal = sqrtf(aim[0] * aim[0] + aim[1] * aim[1]);
		const float limit = 0.08f; /* cos(85.4 degrees) */

		if (horizontal < limit)
		{
			float x = horizontal > 1e-6f ? aim[0] / horizontal : 1.0f;
			float y = horizontal > 1e-6f ? aim[1] / horizontal : 0.0f;

			aim[0] = x * limit;
			aim[1] = y * limit;
			aim[2] = aim[2] < 0.0f ? -sqrtf(1.0f - limit * limit) : sqrtf(1.0f - limit * limit);
		}
	}
	cosine = cosf(vr.heading);
	sine = sinf(vr.heading);
	out_forward[0] = aim[0] * cosine - aim[1] * sine;
	out_forward[1] = aim[0] * sine + aim[1] * cosine;
	out_forward[2] = aim[2];
	vr.last_aim_yaw = atan2f(out_forward[1], out_forward[0]);
	vr.aiming = 1;
	/* Lost steering tracking holds native facing; head movement must not
	 * unexpectedly steer a vehicle while a controller reconnects. */
	return steering_valid;
}

int vr_aiming(void)
{
	return vr.aiming || vr.aiming_last_frame;
}

int vr_hand_aiming(void)
{
	return vr.hand_aiming || vr.hand_aiming_last_frame;
}

/* the local tracking-space origin of a hand pose, with the same 90 cm
reach clamp as the rendered hand/weapon camera. The compositor scope uses
this too, so it cannot drift away from that camera when the arm is extended. */
#define VR_HAND_REACH_METRES 0.9f

static void tracked_hand_origin(const struct halo_xr_pose *pose, float out[3])
{
	float arm[3], length, scale;
	int axis;

	for (axis = 0; axis < 3; axis++)
		arm[axis] = pose->position[axis] - vr.frame.head.position[axis];
	length = sqrtf(arm[0] * arm[0] + arm[1] * arm[1] + arm[2] * arm[2]);
	scale = length > VR_HAND_REACH_METRES ? VR_HAND_REACH_METRES / length : 1.0f;
	for (axis = 0; axis < 3; axis++)
		out[axis] = vr.frame.head.position[axis] + arm[axis] * scale;
}

/* the hand's grip or aim pose as a view from `position` (the game's
camera, where the head is drawn): the head's offset limited as for the
eyes, the hand's from the head kept whole up to an arm's length */
static int hand_view(const struct halo_xr_pose *pose, const float extra[3], const float position[3],
	float out_position[3], float out_forward[3], float out_up[3])
{
	float offset[3], arm[3], heading[3], length;
	int axis;

	if (!vr.heading_valid)
		return 0;
	for (axis = 0; axis < 3; axis++)
		arm[axis] = pose->position[axis] - vr.frame.head.position[axis];
	if (extra)
	{
		float turned[3];

		rotate(pose->orientation, extra, turned);
		for (axis = 0; axis < 3; axis++)
			arm[axis] += turned[axis];
	}
	length = sqrtf(arm[0] * arm[0] + arm[1] * arm[1] + arm[2] * arm[2]);
	if (length > VR_HAND_REACH_METRES)
	{
		for (axis = 0; axis < 3; axis++)
			arm[axis] *= VR_HAND_REACH_METRES / length;
	}
	head_offset(offset);
	for (axis = 0; axis < 3; axis++)
		offset[axis] += arm[axis];
	vr_heading_forward(heading);
	view(offset, pose->orientation, position, heading, out_position, out_forward, out_up);
	return 1;
}

int vr_hand_ray(const float position[3], float out_origin[3], float out_direction[3])
{
	float up[3];

	if (!vr_hand_aiming() || !(vr.frame.hand_valid[vr.weapon_hand] & 2))
		return 0;
	return hand_view(&vr.shot_pose, NULL, position, out_origin, out_direction, up);
}

int vr_weapon_view(const float position[3], float out_position[3], float out_forward[3], float out_up[3])
{
	struct halo_xr_pose pose;
	float extra[3];

	if (!vr_hand_aiming() || (vr.frame.hand_valid[vr.weapon_hand] & 3) != 3)
		return 0;
	/* the grip's place, turned as the aim is, moved to where the game's
	camera would be for the weapon's model to sit in the hand
	(OpenXR's x right, y up, z back). In the left hand the model is
	mirrored about that camera (vr_render.c), so the grip's sideways
	offset is taken the other way to land in the hand */
	pose = vr.aim_pose;
	memcpy(pose.position, vr.frame.grip[vr.weapon_hand].position, sizeof(pose.position));
	extra[0] = vr.weapon_hand ? -vr.weapon_offset[0] : vr.weapon_offset[0];
	extra[1] = -vr.weapon_offset[1];
	extra[2] = -vr.weapon_offset[2];
	return hand_view(&pose, extra, position, out_position, out_forward, out_up);
}

int vr_hand_world(int hand, const float position[3], float out_position[3], float out_forward[3], float out_up[3])
{
	if (hand < 0 || hand > 1 || !(vr.frame.hand_valid[hand] & 1))
		return 0;
	return hand_view(&vr.frame.grip[hand], NULL, position, out_position, out_forward, out_up);
}

int vr_hand_pose(int hand, const float position[3], float out_position[3], float out_forward[3], float out_up[3])
{
	struct halo_xr_pose pose;

	if (hand < 0 || hand > 1 || !(vr.frame.hand_valid[hand] & 1))
		return 0;
	/* the visible hand's own rotation about the grip (vr.hand_*); the grip's
	place, gestures and the gun are untouched */
	pose = vr.frame.grip[hand];
	vr_alignment_multiply(vr.frame.grip[hand].orientation, vr.hand_rotation[hand], pose.orientation);
	return hand_view(&pose, NULL, position, out_position, out_forward, out_up);
}

int vr_hand_tracking_mode(void)
{
	return vr.hand_tracking;
}

float vr_units_per_metre(void)
{
	return vr.units_per_metre;
}

void vr_set_reticle_world(const float anchor[3], const float hit[3])
{
	float offset[3], relative[3], local[3], cosine = cosf(vr.heading), sine = sinf(vr.heading);
	int axis;
	vr.reticle_distance = 0.0f;
	if (!vr.heading_valid || !isfinite(vr.heading) ||
		!isfinite(vr.units_per_metre) || !(vr.units_per_metre > 0.0f)) return;
	for (axis = 0; axis < 3; axis++) {
		relative[axis] = (hit[axis] - anchor[axis]) / vr.units_per_metre;
		if (!isfinite(relative[axis])) return;
	}
	/* Inverse of view()/to_halo(), including the same room-scale/lean
	 * correction as the stereo eyes. A compositor quad is in XR LOCAL space,
	 * not in the game's world and not necessarily on the raw controller ray. */
	local[0] = relative[0] * sine - relative[1] * cosine;
	local[1] = relative[2];
	local[2] = -(relative[0] * cosine + relative[1] * sine);
	head_offset(offset);
	for (axis = 0; axis < 3; axis++) {
		local[axis] -= offset[axis];
		vr.reticle_position[axis] = vr.frame.head.position[axis] + local[axis];
		if (!isfinite(vr.reticle_position[axis])) return;
	}
	vr.reticle_distance = sqrtf(local[0]*local[0] + local[1]*local[1] + local[2]*local[2]);
	if (!isfinite(vr.reticle_distance)) vr.reticle_distance = 0.0f;
}

int vr_crosshair_enabled(void)
{
	return vr.crosshair_enabled && vr.crosshair_opacity > 0.0f && !vr.reticle_hidden;
}

int vr_hud_hidden(void)
{
	return vr.active && vr.hud_hidden;
}

/* test29: the HUD page's HUD row (vr_menu.c): shows or hides the HUD as the
head tap does, for the session (every start shows it) */
void vr_set_hud_hidden(int hidden)
{
	vr.hud_hidden = hidden != 0;
	reset_hud_tap();
	platform_log("vr: HUD %s (VR settings)", vr.hud_hidden ? "hidden" : "shown");
}

int vr_reticle_hidden(void)
{
	return vr.active && vr.reticle_hidden;
}

void vr_crosshair_source(unsigned int texture, float u0, float v0, float u1, float v1)
{
	vr.crosshair_texture = texture;
	vr.crosshair_uv[0] = u0;
	vr.crosshair_uv[1] = v0;
	vr.crosshair_uv[2] = u1;
	vr.crosshair_uv[3] = v1;
}

/* a dot with a dark rim, drawn each frame into the reticle's image */
static void draw_reticle(void)
{
	static const struct { int size; float colour[4]; } rings[] =
	{
		{ 0, { 0.0f, 0.0f, 0.0f, 0.0f } },
		{ 20, { 0.0f, 0.0f, 0.0f, 0.55f } },
		{ 12, { 0.85f, 0.95f, 1.0f, 0.95f } },
	};
	int index = host_xr_acquire(HALO_XR_SWAPCHAIN_RETICLE);
	int size = (int)vr.info.width[HALO_XR_SWAPCHAIN_RETICLE];
	unsigned int ring;

	if (index < 0)
		return;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[HALO_XR_SWAPCHAIN_RETICLE][index], 0);
	glViewport(0, 0, size, size);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	for (ring = 0; ring < sizeof(rings) / sizeof(rings[0]); ring++)
	{
		int extent = rings[ring].size ? rings[ring].size * size / 64 : size;

		if (rings[ring].size)
		{
			glEnable(GL_SCISSOR_TEST);
			glScissor((size - extent) / 2, (size - extent) / 2, extent, extent);
		}
		glClearColor(rings[ring].colour[0], rings[ring].colour[1], rings[ring].colour[2], rings[ring].colour[3]);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glDisable(GL_SCISSOR_TEST);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	host_xr_release(HALO_XR_SWAPCHAIN_RETICLE);
}

int vr_heading_forward(float out_forward[3])
{
	if (!vr.heading_valid)
		return 0;
	out_forward[0] = cosf(vr.heading);
	out_forward[1] = sinf(vr.heading);
	out_forward[2] = 0.0f;
	return 1;
}

/* Script checks run before stereo rendering: use this frame's tracked head,
 * not the previous rendered camera or the weapon's looking vector. */
static int vr_script_head_valid(void)
{
	const unsigned int needed = HALO_XR_FRAME_SHOULD_RENDER | HALO_XR_FRAME_VIEWS_VALID | HALO_XR_FRAME_FOCUSED;
	return vr.active && vr.stereo_enabled && frame_begin() && (vr.frame.flags & needed) == needed;
}

int vr_script_head_view(const float position[3], float out_position[3], float out_forward[3])
{
	float offset[3], heading[3], up[3];
	if (!vr_script_head_valid() || !vr_heading_forward(heading)) return 0;
	head_offset(offset);
	view(offset, vr.frame.head.orientation, position, heading, out_position, out_forward, up);
	return 1;
}

static struct { int valid; float yaw, pitch; } tutorial_look;
void vr_head_look_reset(void) { tutorial_look.valid = 0; }

unsigned int vr_head_look_actions(void)
{
	float forward[3], yaw, pitch, delta;
	unsigned int actions = 0;
	if (!vr_script_head_valid()) { vr_head_look_reset(); return 0; }
	head_forward(forward);
	yaw = atan2f(forward[1], forward[0]);
	pitch = atan2f(forward[2], hypotf(forward[0], forward[1]));
	if (!isfinite(yaw) || !isfinite(pitch)) { vr_head_look_reset(); return 0; }
	if (!tutorial_look.valid || (vr.frame.flags & HALO_XR_FRAME_RECENTRED)) {
		tutorial_look.valid = 1; tutorial_look.yaw = yaw; tutorial_look.pitch = pitch; return 0;
	}
	/* Accumulate slow deliberate movement as well as fast turns. World turns,
	 * weapon movement and controller grip cannot pass the head-look test. */
	delta = wrap_angle(yaw - tutorial_look.yaw);
	if (fabsf(delta) > 0.02f) { actions |= delta > 0 ? 4u : 8u; tutorial_look.yaw = yaw; }
	delta = pitch - tutorial_look.pitch;
	if (fabsf(delta) > 0.02f) { actions |= delta > 0 ? 1u : 2u; tutorial_look.pitch = pitch; }
	return actions;
}

int vr_head_view(const float position[3], const float forward[3],
	float out_position[3], float out_forward[3], float out_up[3])
{
	float offset[3];

	if (!vr.stereo)
		return 0;
	head_offset(offset);
	view(offset, vr.frame.head.orientation, position, forward, out_position, out_forward, out_up);
	return 1;
}

void vr_hud_bounds(float aspect, float bounds[4])
{
	float x = vr.hud_distance > 0.0f ? vr.hud_width * 0.5f / vr.hud_distance : 1.0f;

	bounds[0] = -x / aspect;
	bounds[1] = x / aspect;
	bounds[2] = -x * 0.75f;
	bounds[3] = x * 0.75f;
}

void vr_resolve_eye(int eye, unsigned int source, int width, int height)
{
	if ((!vr.stereo && !vr.cinema) || eye < 0 || eye > 1)
		return;
	if (copy_to_swapchain((unsigned int)eye, source, width, height))
		vr.eyes_resolved |= 1u << eye;
}

/* writes a swapchain's acquired image (or the HUD's, with its alpha) as a
BMP into the data folder */
static void dump_image(unsigned int which, int index, const char *name)
{
	const char *directory = getenv("HALO_DATA_ROOT");
	int width = (int)vr.info.width[which], height = (int)vr.info.height[which];
	unsigned char header[54] = { 'B', 'M' };
	unsigned int size = (unsigned int)(width * height * 4);
	unsigned char *pixels = malloc(size);
	unsigned int pixel, transparent = 0;
	char path[512];
	FILE *file;

	if (!pixels)
		return;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vr.info.images[which][index], 0);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	for (pixel = 0; pixel < (unsigned int)(width * height); pixel++)
	{
		unsigned char red = pixels[pixel * 4];

		pixels[pixel * 4] = pixels[pixel * 4 + 2];
		pixels[pixel * 4 + 2] = red;
		if (pixels[pixel * 4 + 3] == 0)
			transparent++;
		if (which != HALO_XR_SWAPCHAIN_QUAD)
			pixels[pixel * 4 + 3] = 0xff;
	}
	snprintf(path, sizeof(path), "%s/%s", directory ? directory : ".", name);
	file = fopen(path, "wb");
	if (file)
	{
		*(unsigned int *)(header + 2) = 54 + size;
		*(unsigned int *)(header + 10) = 54;
		*(unsigned int *)(header + 14) = 40;
		*(int *)(header + 18) = width;
		*(int *)(header + 22) = height; /* bottom-up, as GL reads */
		*(unsigned short *)(header + 26) = 1;
		*(unsigned short *)(header + 28) = 32;
		*(unsigned int *)(header + 34) = size;
		fwrite(header, 1, sizeof(header), file);
		fwrite(pixels, 1, size, file);
		fclose(file);
		platform_log("vr: wrote %s (%dx%d, %.1f%% transparent)", path, width, height,
			100.0 * transparent / (width * height));
	}
	free(pixels);
}

/* The HUD pass draws on transparent black, but the game's blending leaves
the target's alpha as scratch (none of its HUD writes it). Its copy into the
layer's image makes each pixel as opaque as it is bright: premultiplied,
which is how OpenXR blends a layer by default. */
static const char hud_vertex_source[] =
	"#version 300 es\n"
	"out vec2 coordinate;\n"
	"void main()\n"
	"{\n"
	"	vec2 corner = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));\n"
	"	/* row 0 of the game's target is the top; of OpenXR's, the bottom */\n"
	"	coordinate = vec2(corner.x, 1.0 - corner.y);\n"
	"	gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);\n"
	"}\n";
static const char hud_fragment_source[] =
	"#version 300 es\n"
	"precision mediump float;\n"
	"uniform sampler2D hud;\n"
	"in vec2 coordinate;\n"
	"out vec4 colour;\n"
	"void main()\n"
	"{\n"
	"	vec3 rgb = texture(hud, coordinate).rgb;\n"
	"	colour = vec4(rgb, max(rgb.r, max(rgb.g, rgb.b)));\n"
	"}\n";

static GLuint hud_program, scope_program, hud_vertex_array;
static GLuint crosshair_program;
/* test26: the wrist HUD (vr.wrist_hud): the HUD's corners that tell the
player's state, as Halo lays its HUD out on the 640x480 screen (shield and
health top right; the weapon's ammunition and the grenades top left; the
motion tracker bottom left), drawn on a panel on the off hand's wrist and
left out of the HUD ahead (not while menus are up). In the HUD's image
coordinates (0 at the top): left, top, right, bottom */
static const float wrist_crops[3][4] =
{
	{ 0.60f, 0.00f, 1.00f, 0.17f },
	{ 0.00f, 0.00f, 0.40f, 0.27f },
	{ 0.00f, 0.66f, 0.30f, 1.00f },
};
static const char hud_masked_fragment_source[] =
	"#version 300 es\n"
	"precision mediump float;\n"
	"uniform sampler2D hud;\n"
	"uniform vec4 masks[3];\n"
	"in vec2 coordinate;\n"
	"out vec4 colour;\n"
	"bool inside(vec4 r) { return coordinate.x >= r.x && coordinate.x <= r.z && coordinate.y >= r.y && coordinate.y <= r.w; }\n"
	"void main()\n"
	"{\n"
	"	vec3 rgb = texture(hud, coordinate).rgb;\n"
	"	if (inside(masks[0]) || inside(masks[1]) || inside(masks[2])) rgb = vec3(0.0);\n"
	"	colour = vec4(rgb, max(rgb.r, max(rgb.g, rgb.b)));\n"
	"}\n";
static GLuint hud_masked_program;
static int hud_masked_failed;

/* whether the HUD's corners go to the wrist this frame: not in a seat, nor
with the off hand untracked (the panel cannot show then: the HUD ahead is
whole); with both hands on the gun they stay off it, the panel hidden */
static int wrist_hud_active(void)
{
	return vr.wrist_hud && !vr.menus_active && !vr.hud_hidden && !vr.seated &&
		(vr.frame.hand_valid[1 - vr.weapon_hand] & 1);
}

static const char crosshair_fragment_source[] =
	"#version 300 es\n"
	"precision mediump float;\n"
	"uniform sampler2D hud;\n"
	"uniform vec4 crop;\n"
	"uniform float opacity;\n"
	"in vec2 coordinate;\n"
	"out vec4 colour;\n"
	"void main() {\n"
	" vec3 rgb = texture(hud, mix(crop.xy, crop.zw, coordinate)).rgb;\n"
	" colour = vec4(rgb, max(rgb.r, max(rgb.g, rgb.b))) * opacity;\n"
	"}\n";

static GLuint compile_shader(GLenum type, const char *source)
{
	GLuint shader = glCreateShader(type);
	GLint ok = 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		char log[512];

		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		platform_log("vr: HUD shader: %s", log);
	}
	return shader;
}

/* a program drawing a whole target from the corners hud_vertex_source
makes */
static GLuint link_program(const char *fragment_source, const char *name)
{
	GLuint program = glCreateProgram();
	GLuint vertex = compile_shader(GL_VERTEX_SHADER, hud_vertex_source);
	GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
	GLint ok = 0;

	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	if (!ok)
	{
		char log[512];

		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		platform_log("vr: %s program: %s", name, log);
		glDeleteProgram(program);
		return 0;
	}
	if (!hud_vertex_array)
		glGenVertexArrays(1, &hud_vertex_array);
	return program;
}

/* test24b: the comfort vignette: black, clear within `aperture` and
whole past `aperture` + `feather`, both as tangents of the angle from the
eye's straight ahead (so round however the eye's view is skewed) */
static const char vignette_fragment_source[] =
	"#version 300 es\n"
	"precision mediump float;\n"
	"uniform vec4 tangents;\n"
	"uniform float aperture;\n"
	"uniform float feather;\n"
	"in vec2 coordinate;\n"
	"out vec4 colour;\n"
	"void main() {\n"
	" vec2 t = vec2(mix(tangents.x, tangents.y, coordinate.x), mix(tangents.z, tangents.w, coordinate.y));\n"
	" colour = vec4(0.0, 0.0, 0.0, smoothstep(aperture, aperture + feather, length(t)));\n"
	"}\n";
static GLuint vignette_program;
static int vignette_failed;

static void draw_vignette(unsigned int which)
{
	float tangents[4], aperture;
	int i;

	/* fov: left, right, up, down; the image's coordinate runs top to bottom */
	for (i = 0; i < 4; i++)
		tangents[i] = tanf(vr.frame.fov[which][i]);
	aperture = vr_vignette_aperture(vr.vignette_strength, vr.vignette_amount,
		sqrtf(fmaxf(tangents[0] * tangents[0], tangents[1] * tangents[1]) +
			fmaxf(tangents[2] * tangents[2], tangents[3] * tangents[3])));

	if (!vignette_program && !vignette_failed)
	{
		vignette_program = link_program(vignette_fragment_source, "vignette");
		vignette_failed = !vignette_program;
	}
	if (!vignette_program)
		return;
	glViewport(0, 0, (GLsizei)vr.info.width[which], (GLsizei)vr.info.height[which]);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	/* the eye darkened by the vignette's alpha, its own alpha kept */
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glUseProgram(vignette_program);
	glUniform4fv(glGetUniformLocation(vignette_program, "tangents"), 1, tangents);
	glUniform1f(glGetUniformLocation(vignette_program, "aperture"), aperture);
	glUniform1f(glGetUniformLocation(vignette_program, "feather"), VR_VIGNETTE_FEATHER);
	glBindVertexArray(hud_vertex_array);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_BLEND);
}

static int hud_program_ready(void)
{
	if (!hud_program)
		hud_program = link_program(hud_fragment_source, "HUD");
	return hud_program != 0;
}

/* the HUD from `texture` into the quad's image, its alpha made up */
static int copy_hud(GLuint texture)
{
	unsigned int which = HALO_XR_SWAPCHAIN_QUAD;
	int index;

	if (!hud_program_ready() || (index = host_xr_acquire(which)) < 0)
		return 0;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[which][index], 0);
	glViewport(0, 0, (GLsizei)vr.info.width[which], (GLsizei)vr.info.height[which]);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (vr.srgb_write_control)
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	/* test26: the wrist HUD's corners left out of the HUD ahead */
	if (wrist_hud_active() && !hud_masked_program && !hud_masked_failed)
	{
		hud_masked_program = link_program(hud_masked_fragment_source, "masked HUD");
		hud_masked_failed = !hud_masked_program;
	}
	if (wrist_hud_active() && hud_masked_program)
	{
		glUseProgram(hud_masked_program);
		glUniform1i(glGetUniformLocation(hud_masked_program, "hud"), 0);
		glUniform4fv(glGetUniformLocation(hud_masked_program, "masks"), 3, wrist_crops[0]);
	}
	else
	{
		glUseProgram(hud_program);
		glUniform1i(glGetUniformLocation(hud_program, "hud"), 0);
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindSampler(0, 0);
	glBindVertexArray(hud_vertex_array);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);
	if (vr.srgb_write_control)
		glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (dumping())
		dump_image(which, index, "vr-hud.bmp");
	host_xr_release(which);
	return 1;
}

/* test26: the wrist HUD's panel: the HUD's corners (wrist_crops) on a dark
ground, the shield and health across the top, the motion tracker under it
on the left and the ammunition beside it, each as wide as its height keeps
its shape (the panel's image row 0 is OpenXR's bottom) */
static int copy_wrist(GLuint texture)
{
	unsigned int which = HALO_XR_SWAPCHAIN_WRIST;
	int index, region, width = (int)vr.info.width[which], height = (int)vr.info.height[which];
	int place[3][4];

	if (!crosshair_program)
		crosshair_program = link_program(crosshair_fragment_source, "crosshair");
	if (!crosshair_program || width < 64 || height < 64 || (index = host_xr_acquire(which)) < 0)
		return 0;
	{
		/* (each crop's shape: its share of the 4:3 screen) */
		float aspect[3];
		int top, rest, tracker_width;

		for (region = 0; region < 3; region++)
			aspect[region] = (wrist_crops[region][2] - wrist_crops[region][0]) * 4.0f /
				((wrist_crops[region][3] - wrist_crops[region][1]) * 3.0f);
		top = (int)((float)width / aspect[0]);
		if (top > height / 2) top = height / 2;
		rest = height - top;
		tracker_width = (int)((float)rest * aspect[2]);
		if (tracker_width > width / 2) tracker_width = width / 2;
		/* x, y from the top, width, height */
		place[0][0] = 0; place[0][1] = 0; place[0][2] = width; place[0][3] = top;
		place[2][0] = 0; place[2][1] = top; place[2][2] = tracker_width; place[2][3] = rest;
		place[1][0] = tracker_width; place[1][1] = top; place[1][2] = width - tracker_width;
		place[1][3] = (int)((float)(width - tracker_width) / aspect[1]);
		if (place[1][3] > rest) place[1][3] = rest;
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[which][index], 0);
	glViewport(0, 0, width, height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (vr.srgb_write_control)
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	/* (a dark ground, premultiplied, so it reads against a bright world) */
	glClearColor(0.0f, 0.0f, 0.0f, 0.45f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(crosshair_program);
	glUniform1i(glGetUniformLocation(crosshair_program, "hud"), 0);
	glUniform1f(glGetUniformLocation(crosshair_program, "opacity"), 1.0f);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindSampler(0, 0);
	glBindVertexArray(hud_vertex_array);
	for (region = 0; region < 3; region++)
	{
		if (place[region][2] < 1 || place[region][3] < 1)
			continue;
		glViewport(place[region][0], height - place[region][1] - place[region][3], place[region][2], place[region][3]);
		glUniform4fv(glGetUniformLocation(crosshair_program, "crop"), 1, wrist_crops[region]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	if (vr.srgb_write_control)
		glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	host_xr_release(which);
	return 1;
}

/* test29: the wrist HUD's default place: behind the grip (along the arm)
and out of the back of the wrist (metres) */
#define WRIST_HUD_BACK 0.10f
#define WRIST_HUD_OUT 0.045f

/* test26: where the wrist HUD's panel goes: on the back of the off hand's
wrist (7.5 cm behind the grip, as the gun's wrist is, and 4 cm out of the
back of the hand), facing out of it, as a watch is read: its long side
along the arm, its top the little finger's side (the far side, the forearm
across the body). Shown while it faces the eyes, not while the hand holds
the gun or steers, and 11 by 8 cm. 0 when not shown */
static int place_wrist(struct halo_xr_layers *layers)
{
	static const float grip_down[3] = { 0.0f, -1.0f, 0.0f };
	/* test29: on the wrist itself (WRIST_HUD_BACK behind the grip; it was
	7.5 cm, on the back of the hand) and WRIST_HUD_OUT out of it, then as
	the HUD page moves it: along the arm (+ toward the elbow), across it
	(+ the thumb's side) and out */
	const float back[3] = { 0.0f, vr.wrist_across, WRIST_HUD_BACK + vr.wrist_along };
	int o = 1 - vr.weapon_hand, axis;
	const struct halo_xr_pose *grip = &vr.frame.grip[o];
	float offset[3], centre[3], outward[3], up[3], forward[3], to_eyes[3], length;
	/* (OpenXR's grip +x is out of a left palm and into a right one: the
	back of a left hand is -x, of a right one +x; +y the thumb's side) */
	const float out_axis[3] = { o == 0 ? -1.0f : 1.0f, 0.0f, 0.0f };

	if (!(vr.frame.hand_valid[o] & 1) || vr.two_hand_held || vr.seated)
		return 0;
	rotate(grip->orientation, back, offset);
	rotate(grip->orientation, out_axis, outward);
	rotate(grip->orientation, grip_down, up);
	for (axis = 0; axis < 3; axis++)
	{
		centre[axis] = grip->position[axis] + offset[axis] + outward[axis] * (WRIST_HUD_OUT + vr.wrist_out);
		to_eyes[axis] = vr.frame.head.position[axis] - centre[axis];
		forward[axis] = -outward[axis];
	}
	length = sqrtf(to_eyes[0] * to_eyes[0] + to_eyes[1] * to_eyes[1] + to_eyes[2] * to_eyes[2]);
	if (!(length > 0.05f) || (outward[0] * to_eyes[0] + outward[1] * to_eyes[1] + outward[2] * to_eyes[2]) < 0.45f * length)
		return 0;
	memcpy(layers->wrist_pose.position, centre, sizeof(centre));
	look_rotation(forward, up, layers->wrist_pose.orientation);
	layers->wrist_size[0] = 0.11f * vr.wrist_size;
	layers->wrist_size[1] = 0.11f * vr.wrist_size * (float)vr.info.height[HALO_XR_SWAPCHAIN_WRIST] /
		(float)(vr.info.width[HALO_XR_SWAPCHAIN_WRIST] ? vr.info.width[HALO_XR_SWAPCHAIN_WRIST] : 1);
	return 1;
}

/* Resolve only this frame's authored crosshair, including its native frame and
 * targeting colors. The game's HUD alpha is scratch, as with copy_hud above. */
static int copy_crosshair(void)
{
	unsigned int which = HALO_XR_SWAPCHAIN_RETICLE;
	int index;
	if (!vr.crosshair_texture || !vr_crosshair_enabled()) return 0;
	if (!vr_crosshair_ready() || (index = host_xr_acquire(which)) < 0) return 0;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[which][index], 0);
	glViewport(0, 0, (GLsizei)vr.info.width[which], (GLsizei)vr.info.height[which]);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (vr.srgb_write_control) glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	glUseProgram(crosshair_program);
	glUniform1i(glGetUniformLocation(crosshair_program, "hud"), 0);
	glUniform4fv(glGetUniformLocation(crosshair_program, "crop"), 1, vr.crosshair_uv);
	glUniform1f(glGetUniformLocation(crosshair_program, "opacity"), vr.crosshair_opacity);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, vr.crosshair_texture);
	glBindSampler(0, 0);
	glBindVertexArray(hud_vertex_array);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);
	if (vr.srgb_write_control) glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	host_xr_release(which);
	return 1;
}

int vr_crosshair_ready(void)
{
	static int failed;
	if (!crosshair_program && !failed)
	{
		crosshair_program = link_program(crosshair_fragment_source, "native crosshair");
		if (!crosshair_program)
		{
			failed = 1;
			platform_log("vr: native crosshair compositor unavailable; native HUD fallback");
		}
	}
	return crosshair_program != 0;
}

/* what a frame shows (vr.mode) */
enum
{
	VR_MODE_FLAT,
	VR_MODE_STEREO,
	VR_MODE_CINEMA,
};

/* ---------- the menus' laser pointer */

/* where the weapon hand's aim meets a quad of `width` x `height` metres
at `pose` (LOCAL, facing +z): 1 with the point (LOCAL) and its place on
the quad (0..1 from the top left) */
static int ray_hits_quad(const struct halo_xr_pose *pose, float width, float height,
	float hit[3], float *u, float *v, float *distance)
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	const struct halo_xr_pose *aim = &vr.frame.aim[vr.weapon_hand];
	float inverse[4], origin[3], direction[3], local_origin[3], local_direction[3], t;
	int axis;

	if (!(vr.frame.hand_valid[vr.weapon_hand] & 2) || width <= 0.0f || height <= 0.0f)
		return 0;
	rotate(aim->orientation, xr_forward, direction);
	for (axis = 0; axis < 3; axis++)
		origin[axis] = aim->position[axis] - pose->position[axis];
	inverse[0] = -pose->orientation[0];
	inverse[1] = -pose->orientation[1];
	inverse[2] = -pose->orientation[2];
	inverse[3] = pose->orientation[3];
	rotate(inverse, origin, local_origin);
	rotate(inverse, direction, local_direction);
	/* from in front of it, toward it */
	if (local_origin[2] <= 0.0f || local_direction[2] >= -1e-4f)
		return 0;
	t = -local_origin[2] / local_direction[2];
	*u = (local_origin[0] + local_direction[0] * t) / width + 0.5f;
	*v = 0.5f - (local_origin[1] + local_direction[1] * t) / height;
	if (*u < 0.0f || *u > 1.0f || *v < 0.0f || *v > 1.0f)
		return 0;
	for (axis = 0; axis < 3; axis++)
		hit[axis] = aim->position[axis] + direction[axis] * t;
	*distance = t;
	return 1;
}

int vr_ui_pointer(int menus_active, struct halo_ui_pointer *pointer)
{
	static const float trigger_on = 0.6f, trigger_off = 0.45f;
	struct halo_xr_pose screen;
	float width, hit[3], u, v, distance, trigger;
	int trigger_down, back_down;
	short x, y;

	vr.menus_active = menus_active != 0;
	if (!vr.active || !menus_active)
	{
		vr.pointer_age = 0;
		return 0;
	}
	/* the screen the menus are on: the flat screen, or in stereo (the
	pause menu) the HUD's panel ahead of the head */
	if (vr.mode == VR_MODE_STEREO)
	{
		static const float ahead[3] = { 0.0f, 0.0f, -1.0f };
		float offset[3];
		int axis;

		rotate(vr.frame.head.orientation, ahead, offset);
		memcpy(screen.orientation, vr.frame.head.orientation, sizeof(screen.orientation));
		for (axis = 0; axis < 3; axis++)
			screen.position[axis] = vr.frame.head.position[axis] + offset[axis] * vr.hud_distance;
		width = vr.hud_width;
	}
	else if (vr.mode == VR_MODE_FLAT)
	{
		screen = vr.screen_pose;
		width = vr.screen_width;
	}
	else
	{
		vr.pointer_age = 0;
		return 0;
	}
	if (!ray_hits_quad(&screen, width, width * 0.75f, hit, &u, &v, &distance))
	{
		vr.pointer_age = 0;
		return 0;
	}
	memset(pointer, 0, sizeof(*pointer));
	x = (short)(u * 640.0f);
	y = (short)(v * 480.0f);
	pointer->moved = x != vr.pointer_x || y != vr.pointer_y || !vr.pointer_age;
	pointer->x = pointer->click_x = x;
	pointer->y = pointer->click_y = y;
	/* the weapon hand's trigger clicks; the major hand's upper face button
	(right B; left-handed, left Y) goes back */
	trigger = vr.frame.trigger[vr.weapon_hand];
	trigger_down = vr.pointer_trigger ? trigger > trigger_off : trigger > trigger_on;
	if (trigger_down && !vr.pointer_trigger)
	{
		pointer->left_clicks = 1;
		vr_haptic(vr.weapon_hand, 0.3f, 0.02f);
	}
	back_down = (vr.frame.hand_buttons[vr.controls_mirrored ? 0 : 1] & HALO_XR_HAND_EAST) != 0;
	if (back_down && !vr.pointer_back)
		pointer->right_clicks = 1;
	if (!vr.pointer_age || pointer->left_clicks || pointer->right_clicks)
	{
		platform_log("vr: pointer at %d, %d on the %s%s", x, y, vr.mode == VR_MODE_STEREO ? "HUD" : "screen",
			pointer->left_clicks ? ", click" : pointer->right_clicks ? ", back" : "");
	}
	vr.pointer_trigger = trigger_down;
	vr.pointer_back = back_down;
	vr.pointer_x = x;
	vr.pointer_y = y;
	memcpy(vr.pointer_hit, hit, sizeof(hit));
	memcpy(vr.pointer_orientation, screen.orientation, sizeof(vr.pointer_orientation));
	vr.pointer_distance = distance;
	vr.pointer_age = 1;
	return 1;
}

/* the pointer's dot, on the reticle's layer, a few centimetres off the
screen toward the hand */
static void place_pointer(struct halo_xr_layers *layers)
{
	static const float toward[3] = { 0.0f, 0.0f, 1.0f };
	float out[3];
	int axis;

	draw_reticle();
	rotate(vr.pointer_orientation, toward, out);
	for (axis = 0; axis < 3; axis++)
		layers->reticle_pose.position[axis] = vr.pointer_hit[axis] + out[axis] * 0.02f;
	memcpy(layers->reticle_pose.orientation, vr.pointer_orientation, sizeof(layers->reticle_pose.orientation));
	/* a dot plain to see: its bright centre (12 of the image's 64) about
	2 cm across on the flat screen 2.5 m off */
	layers->reticle_size[0] = layers->reticle_size[1] = 0.04f * vr.pointer_distance;
	/* over the menus' screen, which is opaque (under it, as the hand's
	reticle is under the HUD, the screen hid it) */
	layers->flags |= HALO_XR_LAYER_RETICLE | HALO_XR_LAYER_RETICLE_ON_TOP;
}

/* ---------- the scope */

/* The scope's view, a square of the back buffer, through its sight: a
disc (or the sniper rifle's wide rounded rectangle) darkening toward its
rim, a thin cross with a dot at its centre, transparent outside it
(premultiplied). `coordinate` runs from the top left of the square. */
static const char scope_fragment_source[] =
	"#version 300 es\n"
	"precision mediump float;\n"
	"uniform sampler2D view;\n"
	"uniform vec4 rect;\n"
	"uniform int shape;\n"
	"in vec2 coordinate;\n"
	"out vec4 colour;\n"
	"void main()\n"
	"{\n"
	"	vec2 p = vec2(coordinate.x * 2.0 - 1.0, 1.0 - coordinate.y * 2.0);\n"
	"	float edge;\n"
	"	if (shape == 2)\n"
	"	{\n"
	"		vec2 q = abs(p) - vec2(1.0, 0.626) + 0.08;\n"
	"		edge = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - 0.08;\n"
	"	}\n"
	"	else\n"
	"		edge = length(p) - 1.0;\n"
	"	float soft = max(fwidth(edge), 0.004);\n"
	"	float inside = 1.0 - smoothstep(-soft, soft, edge);\n"
	"	vec3 rgb = texture(view, rect.xy + coordinate * rect.zw).rgb;\n"
	"	rgb *= mix(1.0, 0.45, smoothstep(-0.2, 0.0, edge));\n"
	"	vec2 a = abs(p);\n"
	"	float line = 0.012;\n"
	"	float mark = (a.x < line && a.y > 0.1 && a.y < 0.45) || (a.y < line && a.x > 0.1 && a.x < 0.45) ||\n"
	"		length(p) < 0.025 ? 1.0 : 0.0;\n"
	"	rgb = mix(rgb, vec3(0.85, 0.95, 1.0), mark * 0.85);\n"
	"	colour = vec4(rgb * inside, inside);\n"
	"}\n";

int vr_scope_view(const float position[3], float out_position[3], float out_forward[3], float out_up[3],
	int *out_pixels)
{
	static int previous_gate = -1;
	static int previous_camera = -1;
	int pixels = (int)vr.info.width[HALO_XR_SWAPCHAIN_SCOPE];
	int gate = !vr.scope_enabled ? 0 : !vr.stereo ? 1 : !vr_hand_aiming() ? 2 :
		!(vr.frame.hand_valid[vr.weapon_hand] & 2) ? 3 : 4;

	if (gate != previous_gate)
	{
		static const char *const reasons[] =
		{
			"disabled in VR settings", "waiting for stereo frame", "requires hand aim",
			"weapon-hand aim pose is not tracked", "camera pose check"
		};
		platform_log("vr: scope view gate: %s; enabled %d, stereo %d, hand aim %d, weapon hand %s, "
			"weapon-hand tracking 0x%x, tracking L/R 0x%x/0x%x, zoom %d, two-hand %s, weapon class %d",
			reasons[gate], vr.scope_enabled, vr.stereo, vr_hand_aiming(), vr.weapon_hand ? "right" : "left",
			(unsigned)vr.frame.hand_valid[vr.weapon_hand], (unsigned)vr.frame.hand_valid[0],
			(unsigned)vr.frame.hand_valid[1], vr.zoom_level, vr.two_handed ? "locked" : "one hand", vr.gun_class);
		previous_gate = gate;
	}
	if (gate < 4)
	{
		previous_camera = -1;
		return 0;
	}
	/* the gun's aim, rolled with it: the image keeps the world's way up on
	the layer, which rolls with the gun too */
	if (!hand_view(&vr.shot_pose, NULL, position, out_position, out_forward, out_up))
	{
		if (previous_camera != 0)
			platform_log("vr: scope view gate: shot-pose camera could not be built");
		previous_camera = 0;
		return 0;
	}
	if (previous_camera != 1)
		platform_log("vr: scope view gate: ready");
	previous_camera = 1;
	*out_pixels = pixels < vr.eye_size ? pixels : vr.eye_size;
	return 1;
}

static int scope_program_ready(void)
{
	if (!scope_program)
		scope_program = link_program(scope_fragment_source, "scope");
	return scope_program != 0;
}

void vr_resolve_scope(unsigned int texture, int width, int height, int x, int y, int size, int shape)
{
	unsigned int which = HALO_XR_SWAPCHAIN_SCOPE;
	int index;

	if (!vr.stereo || size <= 0 || width <= 0 || height <= 0 || !scope_program_ready() ||
		(index = host_xr_acquire(which)) < 0)
	{
		return;
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[which][index], 0);
	glViewport(0, 0, (GLsizei)vr.info.width[which], (GLsizei)vr.info.height[which]);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (vr.srgb_write_control)
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
	glUseProgram(scope_program);
	glUniform1i(glGetUniformLocation(scope_program, "view"), 0);
	{
		float rect[4] = { (float)x / (float)width, (float)y / (float)height,
			(float)size / (float)width, (float)size / (float)height };

		glUniform4fv(glGetUniformLocation(scope_program, "rect"), 1, rect);
	}
	glUniform1i(glGetUniformLocation(scope_program, "shape"), shape == VR_SCOPE_SNIPER ? 2 : 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindSampler(0, 0);
	glBindVertexArray(hud_vertex_array);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	glUseProgram(0);
	if (vr.srgb_write_control)
		glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (dumping())
		dump_image(which, index, "vr-scope.bmp");
	host_xr_release(which);
	vr.scope_resolved = 1;
	vr.scope_shape = shape;
}

/* the scope's layer, held at the gun: its place by the sight's shape
(the PC mod HaloCEVR's offsets: forward, left and up, metres, from the
aim), facing back along it, vr.scope_size across */
static void place_scope(struct halo_xr_layers *layers)
{
	static int logged_shape = -1, logged_zoom = -999, logged_weapon_hand = -1;
	static int logged_two_handed = -1, logged_gun_class = -999, logged_settings_generation = -1;
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	static const float offsets[][3] =
	{
		{ -0.10f, 0.00f, 0.15f },  /* VR_SCOPE_ROUND */
		{ -0.15f, 0.00f, 0.15f },  /* VR_SCOPE_SNIPER */
		{ 0.10f, 0.20f, 0.10f },   /* VR_SCOPE_ROCKET */
	};
	const float *offset = offsets[vr.scope_shape >= VR_SCOPE_ROUND && vr.scope_shape <= VR_SCOPE_ROCKET ?
		vr.scope_shape - VR_SCOPE_ROUND : 0];
	/* test22: the player's own place and size for the pistol's and the
	sniper rifle's (none set: exactly the usual) */
	int kind = vr.scope_shape == VR_SCOPE_SNIPER ? 1 : vr.scope_shape == VR_SCOPE_ROUND ? 0 : -1;
	const float *adjust = kind >= 0 ? vr.scope_adjust[kind] : NULL;
	/* OpenXR's x right, y up, z back; in the left hand, left is the other way */
	float local[3], turned[3], origin[3], shot_forward[3];
	int axis;

	local[0] = vr.weapon_hand ? -offset[1] : offset[1];
	local[1] = offset[2];
	local[2] = -offset[0];
	if (adjust)
	{
		/* (right is the player's right in either hand) */
		local[0] += adjust[2];
		local[1] += adjust[1];
		local[2] -= adjust[0];
	}
	tracked_hand_origin(&vr.aim_pose, origin);
	rotate(vr.aim_pose.orientation, local, turned);
	for (axis = 0; axis < 3; axis++)
		layers->scope_pose.position[axis] = origin[axis] + turned[axis];
	/* The layer's image is rendered from shot_pose (which includes this gun's
	 * aim calibration); keep its plane on that same direction. Its center and
	 * adjustable sight offset remain attached to the physical gun pose. */
	memcpy(layers->scope_pose.orientation, vr.shot_pose.orientation, sizeof(layers->scope_pose.orientation));
	layers->scope_size[0] = layers->scope_size[1] = vr.scope_size * (adjust ? adjust[3] : 1.0f);
	layers->flags |= HALO_XR_LAYER_SCOPE;
	if (logged_shape != vr.scope_shape || logged_zoom != vr.zoom_level || logged_weapon_hand != vr.weapon_hand ||
		logged_two_handed != vr.two_handed || logged_gun_class != vr.gun_class ||
		logged_settings_generation != vr.settings_generation)
	{
		rotate(vr.shot_pose.orientation, xr_forward, shot_forward);
		platform_log("vr: scope layer pose: shape %d, zoom %d, weapon hand %s, two-hand %s, weapon class %d, "
			"tracking L/R 0x%x/0x%x; center %.3f/%.3f/%.3f m, local offset %.3f/%.3f/%.3f m, "
			"shot forward %.3f/%.3f/%.3f, layer size %.3f m",
			vr.scope_shape, vr.zoom_level, vr.weapon_hand ? "right" : "left",
			vr.two_handed ? "locked" : "one hand", vr.gun_class,
			(unsigned)vr.frame.hand_valid[0], (unsigned)vr.frame.hand_valid[1],
			layers->scope_pose.position[0], layers->scope_pose.position[1], layers->scope_pose.position[2],
			local[0], local[1], local[2], shot_forward[0], shot_forward[1], shot_forward[2], layers->scope_size[0]);
		logged_shape = vr.scope_shape;
		logged_zoom = vr.zoom_level;
		logged_weapon_hand = vr.weapon_hand;
		logged_two_handed = vr.two_handed;
		logged_gun_class = vr.gun_class;
		logged_settings_generation = vr.settings_generation;
	}
}

/* seconds a change of mode takes to come in from black */
#define VR_FADE_SECONDS 0.35f
/* and how long it stays black before */
#define VR_FADE_HOLD_SECONDS 0.25f

/* the screen `distance` metres ahead of where the head faces, upright, at
the head's height */
static void place_screen(float distance)
{
	static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
	float forward[3], yaw;

	rotate(vr.frame.head.orientation, xr_forward, forward);
	yaw = (forward[0] * forward[0] + forward[2] * forward[2] > 1e-4f) ? atan2f(-forward[0], -forward[2]) : 0.0f;
	vr.screen_pose.orientation[0] = 0.0f;
	vr.screen_pose.orientation[1] = sinf(yaw * 0.5f);
	vr.screen_pose.orientation[2] = 0.0f;
	vr.screen_pose.orientation[3] = cosf(yaw * 0.5f);
	vr.screen_pose.position[0] = vr.frame.head.position[0] - sinf(yaw) * distance;
	vr.screen_pose.position[1] = vr.frame.head.position[1];
	vr.screen_pose.position[2] = vr.frame.head.position[2] - cosf(yaw) * distance;
}

/* the fade layer's image: black, `amount` opaque (premultiplied) */
static void draw_fade(float amount)
{
	int index = host_xr_acquire(HALO_XR_SWAPCHAIN_FADE);

	if (index < 0)
		return;
	if (amount > 1.0f)
		amount = 1.0f;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vr.framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		vr.info.images[HALO_XR_SWAPCHAIN_FADE][index], 0);
	glViewport(0, 0, (GLsizei)vr.info.width[HALO_XR_SWAPCHAIN_FADE], (GLsizei)vr.info.height[HALO_XR_SWAPCHAIN_FADE]);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, amount);
	glClear(GL_COLOR_BUFFER_BIT);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	host_xr_release(HALO_XR_SWAPCHAIN_FADE);
}

int vr_present(unsigned int source, unsigned int texture, int width, int height)
{
	static long frames_missed;
	struct halo_xr_layers layers;
	double start = 0.0, copied = 0.0;

	if (!vr.active)
		return 0;
	if (!frame_begin())
	{
		/* said once, a few seconds in: a short gap is a level loading */
		if (++frames_missed == 300)
		{
			platform_log("vr: the headset takes no frames (session state %u): the game is shown in the "
				"window; was the app opened in VR?", (unsigned)vr.frame.session_state);
		}
		return 0;
	}
	if (frames_missed >= 300)
		platform_log("vr: the headset takes frames again");
	frames_missed = 0;
	if (vr.timing)
	{
		start = now_ms();
		/* vr.timing_gpu: wait here for the GPU, so the time it takes
		shows apart from the game's */
		if (vr.gpu_finish)
		{
			glFinish();
			vr.gpu_ms += now_ms() - start;
		}
		vr.frame_ms += start - vr.frame_start;
		start = now_ms();
	}
	memset(&layers, 0, sizeof(layers));
	{
		int mode = vr.stereo ? VR_MODE_STEREO : vr.cinema ? VR_MODE_CINEMA : VR_MODE_FLAT;

		/* a change of what is shown comes in from black, and a screen comes
		up where the head faces */
		if (mode != vr.mode)
		{
			static const char *const names[] = { "a screen", "stereo", "a 3D screen" };

			platform_log("vr: showing %s", names[mode]);
			/* black a moment first: a level shows a few frames of play
			before its script starts the opening cutscene, and such a
			flash between two fades is hidden */
			vr.fade = 1.0f + VR_FADE_HOLD_SECONDS / VR_FADE_SECONDS;
			if (mode != VR_MODE_STEREO)
				place_screen(mode == VR_MODE_CINEMA ? vr.cinema_distance : vr.screen_distance);
			vr.mode = mode;
		}
	}
	if (vr.cinema)
	{
		/* each eye's image on the same screen: a 3D picture */
		if (vr.eyes_resolved == 3)
		{
			layers.flags |= HALO_XR_LAYER_STEREO_SCREEN;
			layers.quad_pose = vr.screen_pose;
			layers.quad_size[0] = vr.cinema_width;
			layers.quad_size[1] = vr.cinema_width * 0.75f;
		}
	}
	else if (vr.stereo)
	{
		/* the eyes are in their images; what is left in the back buffer is
		the HUD on a transparent ground, which rides ahead of the head */
		if (vr.eyes_resolved == 3)
		{
			layers.flags |= HALO_XR_LAYER_PROJECTION;
			/* the eyes were drawn over the glasses window only;
			the compositor shows them there, and nothing round it */
			if (vr.frame_glasses)
			{
				layers.flags |= HALO_XR_LAYER_EYE_FOV;
				memcpy(layers.eye_fov, vr.frame.fov, sizeof(layers.eye_fov));
			}
		}
		if (copy_hud(texture))
		{
			layers.flags |= HALO_XR_LAYER_QUAD | HALO_XR_LAYER_QUAD_HEAD_LOCKED | HALO_XR_LAYER_QUAD_ALPHA;
			layers.quad_pose.position[2] = -vr.hud_distance;
			layers.quad_pose.orientation[3] = 1.0f;
			layers.quad_size[0] = vr.hud_width;
			layers.quad_size[1] = vr.hud_width * 0.75f;
			/* test26: and the wrist HUD's panel, while it faces the eyes */
			if (wrist_hud_active() && place_wrist(&layers) && copy_wrist(texture))
				layers.flags |= HALO_XR_LAYER_WRIST;
		}
		/* (not while the hand points at a menu: its dot has the layer) */
		if ((!vr.hand_aiming || (vr.reticle_distance > 0.0f && (vr.frame.hand_valid[vr.weapon_hand] & 2))) &&
			!(vr.pointer_age > 0 && vr.pointer_age <= 8) && copy_crosshair())
		{
			static const float xr_forward[3] = { 0.0f, 0.0f, -1.0f };
			float direction[3];
			int world_reticle = vr.reticle_distance > 0.0f && (vr.hand_aiming || vr.seated);
			float distance = world_reticle ? vr.reticle_distance : vr.hud_distance;
			const struct halo_xr_pose *pose = vr.hand_aiming ? &vr.aim_pose : &vr.frame.head;
			int axis;

			/* The game supplied a collision point on its firing ray. Preserve
			 * the authored crosshair and angular size; head aim keeps its HUD plane. */
			rotate(pose->orientation, xr_forward, direction);
			for (axis = 0; axis < 3; axis++)
				layers.reticle_pose.position[axis] = world_reticle ? vr.reticle_position[axis] :
					pose->position[axis] + direction[axis] * distance;
			memcpy(layers.reticle_pose.orientation, vr.frame.head.orientation, sizeof(layers.reticle_pose.orientation));
			layers.reticle_size[0] = layers.reticle_size[1] =
				0.4f * vr.hud_width / fmaxf(0.1f, vr.hud_distance) * distance * vr.crosshair_size;
			layers.flags |= HALO_XR_LAYER_RETICLE;
			if (vr.dump_frame > 0 && vr.stereo_frames == vr.dump_frame)
				platform_log("vr: reticle %.2f m along the hand's aim", vr.reticle_distance);
		}
		if (vr.scope_resolved && (vr.frame.hand_valid[vr.weapon_hand] & 2) && vr.scope_size > 0.0f)
			place_scope(&layers);
	}
	else if ((vr.frame.flags & HALO_XR_FRAME_SHOULD_RENDER) &&
		copy_to_swapchain(HALO_XR_SWAPCHAIN_QUAD, source, width, height))
	{
		/* a screen floating where the head faced, opaque */
		layers.flags = HALO_XR_LAYER_QUAD;
		layers.quad_pose = vr.screen_pose;
		layers.quad_size[0] = vr.screen_width;
		layers.quad_size[1] = vr.screen_width * 0.75f;
	}
	/* the menus' pointer, while it meets their screen (the menus look at the
	pointer as they update, which is not every headset frame: the dot stays a
	few frames between) */
	if (vr.pointer_age > 0 && vr.pointer_age <= 8 && !(layers.flags & HALO_XR_LAYER_RETICLE) &&
		(layers.flags & (HALO_XR_LAYER_QUAD | HALO_XR_LAYER_PROJECTION)))
	{
		place_pointer(&layers);
	}
	if (vr.pointer_age > 0)
		vr.pointer_age++;
	if (vr.fade > 0.0f && (vr.frame.flags & HALO_XR_FRAME_SHOULD_RENDER))
	{
		draw_fade(vr.fade);
		layers.flags |= HALO_XR_LAYER_FADE;
		vr.fade -= (float)(vr.frame.predicted_display_period * 1e-9) / VR_FADE_SECONDS;
	}
	if (vr.timing)
		copied = now_ms();
	if (vr.stereo && vr.dump_frame > 0 && vr.stereo_frames == vr.dump_frame)
		platform_log("vr: aim at the dump: %s, %s%s, heading %.1f, head yaw %.1f, aim yaw %.1f",
			vr.seated ? "seated" : "on foot", vr.hand_aiming ? "hand aims" : "head aims",
			vr.two_handed ? " with both hands" : "",
			vr.heading * 57.29578f, vr.head_yaw * 57.29578f, vr.aim_yaw * 57.29578f);
	frame_end(&layers);
	if (vr.timing)
	{
		double ended = now_ms();

		vr.copy_ms += copied - start;
		vr.end_ms += ended - copied;
		if (++vr.timed == 300)
		{
			double n = (double)vr.timed;

			platform_log("[vr-frame] %s ms a frame: wait %.2f, game %.2f (left eye %.2f, right eye %.2f, HUD %.2f, "
				"scope %.2f; GPU finish %.2f), copies %.2f, end %.2f",
				vr.stereo ? "stereo" : "flat", vr.wait_ms / n, vr.frame_ms / n, vr.pass_ms[0] / n, vr.pass_ms[1] / n,
				vr.pass_ms[2] / n, vr.pass_ms[3] / n, vr.gpu_ms / n, vr.copy_ms / n, vr.end_ms / n);
			vr.timed = 0;
			vr.wait_ms = vr.frame_ms = vr.gpu_ms = vr.copy_ms = vr.end_ms = 0.0;
			vr.pass_ms[0] = vr.pass_ms[1] = vr.pass_ms[2] = vr.pass_ms[3] = 0.0;
		}
	}
	if (vr.stereo)
		vr.stereo_frames++;
	if (vr.cinema)
		vr.cinema_frames++;
	vr.stereo = 0;
	vr.cinema = 0;
	vr.eyes_resolved = 0;
	vr.scope_resolved = 0;
	vr.aiming_last_frame = vr.aiming;
	/* test24b: out of the player's control (a cutscene, a menu) the
	vignette is gone, and begins anew from nothing */
	if (!vr.aiming)
		vr.vignette_amount = vr.snap_pulse = 0.0f;
	vr.aiming = 0;
	vr.hand_aiming_last_frame = vr.hand_aiming;
	vr.hand_aiming = 0;
	vr.reticle_distance = 0.0f;
	return 1;
}

void vr_probe(void)
{
	double seconds = config_real("vr.probe_seconds");
	long long start = 0, now = 0, frames = 0, rendered = 0;
	struct halo_xr_frame frame;

	if (!vr.active || seconds <= 0.0)
		return;
	platform_log("vr: probe for %.0f s (dim red left eye, dim blue right eye, grey panel ahead)", seconds);
	while (!start || (now - start) < (long long)(seconds * 1e9))
	{
		struct halo_xr_layers layers;

		if (!host_xr_begin_frame(&frame))
		{
			if (frame.flags & HALO_XR_FRAME_EXIT)
				break;
			continue;
		}
		now = frame.predicted_display_time;
		if (!start)
			start = now;
		frames++;
		memset(&layers, 0, sizeof(layers));
		if (frame.flags & HALO_XR_FRAME_SHOULD_RENDER)
		{
			/* dim: full-intensity colours fill the view uncomfortably */
			clear_swapchain(HALO_XR_SWAPCHAIN_LEFT, 0.18f, 0.03f, 0.03f, 1.0f);
			clear_swapchain(HALO_XR_SWAPCHAIN_RIGHT, 0.03f, 0.03f, 0.18f, 1.0f);
			clear_swapchain(HALO_XR_SWAPCHAIN_QUAD, 0.15f, 0.15f, 0.15f, 1.0f);
			layers.flags = HALO_XR_LAYER_PROJECTION | HALO_XR_LAYER_QUAD;
			/* 1 m wide, 2 m ahead at eye height */
			layers.quad_pose.position[2] = -2.0f;
			layers.quad_pose.orientation[3] = 1.0f;
			layers.quad_size[0] = 1.0f;
			layers.quad_size[1] = 0.75f;
			rendered++;
		}
		if ((frames % 360) == 1)
		{
			platform_log("vr: probe frame %lld: state %u flags 0x%x head (%.2f %.2f %.2f) "
				"fov L %.3f %.3f %.3f %.3f buttons 0x%x thumbs %.2f %.2f %.2f %.2f hands %u %u",
				frames, frame.session_state, frame.flags, frame.head.position[0], frame.head.position[1],
				frame.head.position[2], frame.fov[0][0], frame.fov[0][1], frame.fov[0][2], frame.fov[0][3],
				frame.buttons, frame.thumb[0], frame.thumb[1], frame.thumb[2], frame.thumb[3],
				frame.hand_valid[0], frame.hand_valid[1]);
		}
		host_xr_end_frame(&layers);
	}
	platform_log("vr: probe done: %lld frames, %lld rendered, %.1f frames/s", frames, rendered,
		now > start ? frames / ((now - start) * 1e-9) : 0.0);
}

#endif /* HALO_VR */
