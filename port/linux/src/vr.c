/*
VR.C

The VR mode's platform side (HALO_VR; vr.h, port/linux/README.md "VR"): the
OpenXR session through the host (vr_host.h, port/linux/arm64/host_vr.c),
the frame loop, the head's pose in the game's axes, the controllers as a
gamepad, turning and recentring.

Frames: the runtime paces the game. The present of one frame ends it and
waits for the next (halo_vr_present), whose head pose then aims the player
(halo_vr_aim) while the game updates; just before the frame's draws the
views are located again for the display time (halo_vr_view), and those are
the poses the frame is drawn and submitted with.

The head moves the view about the player's eye, which the game keeps (it
crouches and rides as before): from where the head was at the last
recentring (seated), or from vr.player_height above the floor (standing).
The controllers' right stick turns the player (snap or smooth), and the
aim on foot is where the head looks, so the reticle stays in the middle of
the view and the netcode sees an ordinary player's aim.

With vr.aim = "gamepad" a gamepad plays as on the flat screen: its stick
turns and aims the player (the game's own look code), and the head looks
about from the aim: the view is the facing, yaw and pitch, turned and moved
by the head (vr.gamepad_view = "camera"), or the facing's yaw alone, with
it or in steps ("level", "snap"). The controllers are a gamepad's halves.
*/

#ifdef HALO_VR
#include "platform.h"
#include "port_config.h"
#include "vr.h"
#include "vr_host.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* the HUD's and menus' image: the 640x480 screen at twice its size */
#define HUD_WIDTH 1280
#define HUD_HEIGHT 960
/* a world unit is 10 feet */
#define METRES_PER_WORLD_UNIT 3.048f
/* a melee gesture: the right hand faster than this along where it points
(metres a second), no sooner than this after the last */
#define MELEE_SPEED 2.5f
#define MELEE_INTERVAL_MS 600
/* holding both grips this long recentres, or a gamepad's Back (View) */
#define RECENTRE_SECONDS 0.75f
#define GAMEPAD_RECENTRE_SECONDS 1.0f
/* both hands on a long gun (vr.two_handed): the left hand takes the
foregrip when its grip is pressed this near the line ahead of the right hand
(metres), and lets it go when the grip is let go; the aim turns to the line
between the hands, smoothed over this time constant (seconds) */
#define FOREGRIP_NEAREST 0.08f
#define FOREGRIP_FARTHEST 0.70f
#define FOREGRIP_RADIUS 0.18f
#define FOREGRIP_PRESSED 0.6f
#define FOREGRIP_RELEASED 0.3f
#define TWO_HANDED_SMOOTHING 0.05f
#define TWO_HANDED_BLEND 0.15f
/* debug.vr_test_hands: the poses it gives the controllers, each held this
many frames */
#define TEST_HANDS_POSES 8
#define TEST_HANDS_FRAMES 288
/* and its buttons (vr_host.h's VR_BUTTON_*, with the triggers as these)
pressed for the first frames of the pose */
#define TEST_HANDS_PRESS_FRAMES 20
#define TEST_HANDS_RIGHT_TRIGGER (1 << 16)
#define TEST_HANDS_LEFT_TRIGGER (1 << 17)

static struct
{
	int initialized;
	int failed;
	struct vr_host_info info;
	int frame; /* a frame is begun */
	unsigned long presents;
	struct vr_host_views views;
	/* debug.vr_test_head: the head's place, and the time it turns by */
	int test_head_placed;
	float test_head_centre[3];
	double test_head_seconds;
	/* debug.vr_test_jitter: this frame's move and turn of the head */
	float jitter_offset[3];
	float jitter_turn[4];
	struct vr_host_input input;
	/* the recentred origin, in the play space */
	int origin_valid;
	/* whether the session had the focus last frame: the headset going on
	(the focus coming) recentres, as the pose before it may be the headset
	lying wherever it was */
	int was_focused;
	float origin_position[3];
	float origin_yaw;
	/* the turn, and whether the aim follows the head */
	float body_yaw;
	int aiming;
	int aimed;
	int aim_synced;
	int snap_latched;
	float recentre_time;
	/* a gamepad's Back held (halo_vr_gamepad_back), and for how long */
	int gamepad_back;
	float gamepad_recentre_time;
	unsigned long last_ticks;
	/* the image of each swapchain for this frame */
	int acquired[VR_SWAPCHAIN_COUNT];
	int force_render;
	int menus;
	/* the eyes' depth this frame, and its range */
	int depth_written;
	/* the melee gesture: the right hand's last position, and the frames
	left of the button it presses */
	int hand_valid;
	float hand_position[3];
	int melee_frames;
	unsigned long melee_ticks;
	/* both hands on a long gun: whether the left holds the foregrip, how
	far the aim has turned to the line between the hands (0 to 1), that
	line, smoothed (OpenXR axes), and whether the weapon is one */
	int two_handed;
	float two_handed_weight;
	float two_handed_line[3];
	int long_gun;
	float depth_near, depth_far;
	/* the presented frame, ended when the next begins (halo_vr_present) */
	struct vr_host_layers pending_layers;
	int pending;
	int attempted;
	/* debug.gpu_stats: the frames' timing */
	int statistics;
	Uint64 frame_start, wait_start;
	unsigned long timed_frames, late_frames;
	double frame_seconds, wait_seconds, worst_seconds;
} vr;

/* ---------- settings */

static struct
{
	unsigned long read_at;
	int smooth_turn;
	float snap_angle;
	float smooth_speed;
	int standing;
	float player_height;
	float world_scale;
	float hud_distance;
	float hud_size;
	float menu_distance;
	float menu_width;
	int controller_aim;
	int gamepad_aim;
	/* vr.gamepad_view: the view's heading in steps ("snap"), and pitched
	with the facing ("camera") */
	int gamepad_snap;
	int gamepad_pitch;
	float weapon_offset[3];
	int melee_gesture;
	int vehicle_first_person;
	float test_turn;
	float test_jitter;
	/* debug.vr_test_head: the head held still, turned left and right by
	this many degrees over this many seconds */
	int test_head;
	float test_head_yaw;
	float test_head_period;
	int two_handed;
	int floating_hands;
	int arms;
	float shoulder_offset[3];
	/* debug.vr_test_hands: each pose's left and right hands (metres right,
	up, forward from the recentred head; degrees of yaw left, pitch up, roll
	right), the left grip's pull, the buttons pressed, the right trigger's
	pull and the head's turn (degrees left) */
	int test_hand_poses;
	float test_hands[TEST_HANDS_POSES][16];
} settings = { (unsigned long)-1 };

/* debug.vr_test_hands: "lx ly lz lyaw lpitch lroll, rx ry rz ryaw rpitch
rroll[, grip[, buttons[, trigger[, head yaw]]]]", poses separated by ';',
each held TEST_HANDS_FRAMES frames */
static void test_hands_read(const char *text)
{
	settings.test_hand_poses = 0;
	while (text && *text && settings.test_hand_poses < TEST_HANDS_POSES)
	{
		float *pose = settings.test_hands[settings.test_hand_poses];
		char numbers[256];
		const char *end = strchr(text, ';');
		size_t length = end ? (size_t)(end - text) : strlen(text);
		char *c;
		int count;

		if (length >= sizeof(numbers))
			length = sizeof(numbers) - 1;
		memcpy(numbers, text, length);
		numbers[length] = 0;
		for (c = numbers; *c; c++)
		{
			if (*c == ',')
				*c = ' ';
		}
		pose[12] = pose[13] = pose[14] = pose[15] = 0.0f;
		count = sscanf(numbers, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &pose[0], &pose[1], &pose[2],
			&pose[3], &pose[4], &pose[5], &pose[6], &pose[7], &pose[8], &pose[9], &pose[10], &pose[11], &pose[12],
			&pose[13], &pose[14], &pose[15]);
		if (count >= 12)
			settings.test_hand_poses++;
		text = end ? end + 1 : NULL;
	}
}

static void settings_read(void)
{
	if (settings.read_at == config_changes())
		return;
	settings.read_at = config_changes();
	settings.smooth_turn = !strcmp(config_string("vr.turn"), "smooth");
	settings.snap_angle = (float)config_real("vr.snap_turn_angle") * (float)M_PI / 180.0f;
	settings.smooth_speed = (float)config_real("vr.smooth_turn_speed") * (float)M_PI / 180.0f;
	settings.standing = !strcmp(config_string("vr.height"), "standing");
	settings.player_height = (float)config_real("vr.player_height");
	settings.world_scale = (float)config_real("vr.world_scale");
	if (settings.world_scale <= 0.0f)
		settings.world_scale = 1.0f;
	settings.hud_distance = (float)config_real("vr.hud_distance");
	if (settings.hud_distance < 0.3f)
		settings.hud_distance = 0.3f;
	settings.hud_size = (float)config_real("vr.hud_size");
	if (settings.hud_size < 20.0f || settings.hud_size > 120.0f)
		settings.hud_size = 60.0f;
	settings.menu_distance = (float)config_real("vr.menu_distance");
	settings.menu_width = (float)config_real("vr.menu_width");
	settings.gamepad_aim = !strcmp(config_string("vr.aim"), "gamepad");
	settings.controller_aim = !settings.gamepad_aim && strcmp(config_string("vr.aim"), "head") != 0;
	settings.gamepad_snap = !strcmp(config_string("vr.gamepad_view"), "snap");
	settings.gamepad_pitch = !settings.gamepad_snap && strcmp(config_string("vr.gamepad_view"), "level") != 0;
	settings.melee_gesture = config_boolean("vr.melee_gesture");
	settings.vehicle_first_person = strcmp(config_string("vr.vehicle_view"), "third_person") != 0;
	settings.test_turn = (float)config_real("debug.vr_test_turn") * (float)M_PI / 180.0f;
	settings.test_jitter = (float)config_real("debug.vr_test_jitter") * 0.001f;
	settings.test_head_yaw = 0.0f;
	settings.test_head_period = 0.0f;
	settings.test_head = sscanf(config_string("debug.vr_test_head"), "%f %f", &settings.test_head_yaw,
		&settings.test_head_period) >= 1;
	settings.two_handed = config_boolean("vr.two_handed");
	settings.floating_hands = strcmp(config_string("vr.hands"), "game") != 0;
	settings.arms = settings.floating_hands && strcmp(config_string("vr.hands"), "floating") != 0;
	settings.shoulder_offset[0] = 0.19f;
	settings.shoulder_offset[1] = -0.20f;
	settings.shoulder_offset[2] = -0.07f;
	sscanf(config_string("vr.shoulders"), "%f , %f , %f", &settings.shoulder_offset[0],
		&settings.shoulder_offset[1], &settings.shoulder_offset[2]);
	test_hands_read(config_string("debug.vr_test_hands"));
	settings.weapon_offset[0] = 0.15f;
	settings.weapon_offset[1] = -0.22f;
	settings.weapon_offset[2] = 0.30f;
	sscanf(config_string("vr.weapon_offset"), "%f , %f , %f", &settings.weapon_offset[0],
		&settings.weapon_offset[1], &settings.weapon_offset[2]);
}

int halo_vr_enabled(void)
{
	return config_boolean("vr.enabled");
}

int halo_vr_running(void)
{
	return vr.initialized;
}

/* ---------- maths: OpenXR's axes (x right, y up, z backwards) */

static void quaternion_rotate(const float q[4], const float v[3], float out[3])
{
	/* v + 2w(q x v) + 2 q x (q x v) */
	float tx = 2.0f * (q[1] * v[2] - q[2] * v[1]);
	float ty = 2.0f * (q[2] * v[0] - q[0] * v[2]);
	float tz = 2.0f * (q[0] * v[1] - q[1] * v[0]);

	out[0] = v[0] + q[3] * tx + (q[1] * tz - q[2] * ty);
	out[1] = v[1] + q[3] * ty + (q[2] * tx - q[0] * tz);
	out[2] = v[2] + q[3] * tz + (q[0] * ty - q[1] * tx);
}

/* the yaw (about y) the orientation looks along */
static float quaternion_yaw(const float q[4])
{
	static const float backwards[3] = { 0.0f, 0.0f, -1.0f };
	float forward[3];

	quaternion_rotate(q, backwards, forward);
	return atan2f(-forward[0], -forward[2]);
}

static void quaternion_multiply(const float a[4], const float b[4], float out[4])
{
	float r[4];

	r[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
	r[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
	r[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
	r[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
	memcpy(out, r, sizeof(r));
}

/* an OpenXR vector turned by -yaw about y, in game axes */
static void game_axes(const float v[3], float yaw, float out[3])
{
	float c = cosf(-yaw), s = sinf(-yaw);
	float x = v[0] * c + v[2] * s;
	float z = -v[0] * s + v[2] * c;

	out[0] = -z;
	out[1] = -x;
	out[2] = v[1];
}

/* ---------- the session */

int halo_vr_initialize(void)
{
	if (vr.initialized || vr.failed)
		return vr.initialized;
	if (!halo_vr_enabled())
	{
		vr.failed = 1;
		return 0;
	}
	settings_read();
	memset(&vr.info, 0, sizeof(vr.info));
	vr.info.scale = (float)config_real("vr.resolution_scale");
	if (vr.info.scale < 0.25f || vr.info.scale > 2.0f)
		vr.info.scale = 1.0f;
	vr.info.hud_width = HUD_WIDTH;
	vr.info.hud_height = HUD_HEIGHT;
	vr.info.standing = settings.standing;
	vr.info.refresh_rate_wanted = (float)config_real("vr.refresh_rate");
	vr.info.depth = config_boolean("vr.depth");
	if (!host_vr_initialize(&vr.info))
	{
		platform_log("vr: OpenXR is not available; playing flat");
		vr.failed = 1;
		return 0;
	}
	vr.force_render = config_boolean("debug.vr_force_render");
	vr.acquired[0] = vr.acquired[1] = vr.acquired[2] = -1;
	vr.initialized = 1;
	vr.statistics = config_boolean("debug.gpu_stats");
	platform_log("vr: %s, eyes %dx%d, %.0f Hz", vr.info.system_name, vr.info.eye_width, vr.info.eye_height,
		vr.info.refresh_rate);
	return 1;
}

void halo_vr_eye_size(int *width, int *height)
{
	*width = vr.info.eye_width;
	*height = vr.info.eye_height;
}

void halo_vr_hud_size(int *width, int *height)
{
	*width = vr.info.hud_width;
	*height = vr.info.hud_height;
}

unsigned int halo_vr_image(int swapchain)
{
	int index;

	if (!vr.frame || swapchain < 0 || swapchain >= VR_SWAPCHAIN_COUNT)
		return 0;
	index = host_vr_acquire(swapchain);
	if (index < 0 || index >= vr.info.image_count[swapchain])
		return 0;
	vr.acquired[swapchain] = index;
	return vr.info.images[swapchain][index];
}

static void recentre(void)
{
	const struct vr_host_pose *head = &vr.views.head;
	float yaw;

	if (!head->valid)
		return;
	yaw = quaternion_yaw(head->orientation);
	/* the aim keeps pointing where it did; the gamepad's is where the head
	looks now (halo_vr_aim) */
	if (settings.gamepad_aim)
		vr.aim_synced = 0;
	else if (vr.origin_valid)
		vr.body_yaw += yaw - vr.origin_yaw;
	vr.origin_yaw = yaw;
	vr.origin_position[0] = head->position[0];
	vr.origin_position[1] = settings.standing ? settings.player_height : head->position[1];
	vr.origin_position[2] = head->position[2];
	vr.origin_valid = 1;
	platform_log("vr: recentred (%s)", settings.standing ? "standing" : "seated");
}

static void normalize(float v[3])
{
	float length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	if (length > 1e-6f)
	{
		v[0] /= length;
		v[1] /= length;
		v[2] /= length;
	}
}

/* both hands on a long gun, once a frame: the left hand takes the foregrip
(its grip pressed near the line ahead of the right hand) and lets it go, and
the line between the hands is smoothed */
static void two_handed_update(float seconds)
{
	static const float backwards[3] = { 0.0f, 0.0f, -1.0f };
	const struct vr_host_pose *right = &vr.views.grip[1], *left = &vr.views.grip[0];
	int able = settings.two_handed && settings.controller_aim && vr.long_gun && vr.aimed &&
		right->valid && left->valid && vr.views.aim[1].valid;
	float line[3], pointing[3], along = 0.0f, across = 0.0f, length;
	int axis;

	if (able)
	{
		quaternion_rotate(vr.views.aim[1].orientation, backwards, pointing);
		for (axis = 0; axis < 3; axis++)
			line[axis] = left->position[axis] - right->position[axis];
		for (axis = 0; axis < 3; axis++)
			along += line[axis] * pointing[axis];
		for (axis = 0; axis < 3; axis++)
		{
			float off = line[axis] - pointing[axis] * along;

			across += off * off;
		}
		across = sqrtf(across);
		if (!vr.two_handed && vr.input.left_grip > FOREGRIP_PRESSED && along > FOREGRIP_NEAREST &&
			along < FOREGRIP_FARTHEST && across < FOREGRIP_RADIUS)
		{
			vr.two_handed = 1;
			if (vr.two_handed_weight <= 0.0f)
				memcpy(vr.two_handed_line, pointing, sizeof(vr.two_handed_line));
			host_vr_haptic(0, 0.25f, 0.03f);
		}
		else if (vr.two_handed && vr.input.left_grip < FOREGRIP_RELEASED)
		{
			vr.two_handed = 0;
		}
	}
	else
	{
		vr.two_handed = 0;
	}
	if (vr.two_handed)
	{
		/* (the hands too near each other point nowhere: the line stays) */
		length = sqrtf(line[0] * line[0] + line[1] * line[1] + line[2] * line[2]);
		if (length > FOREGRIP_NEAREST * 0.5f)
		{
			float t = seconds > 0.0f ? 1.0f - expf(-seconds / TWO_HANDED_SMOOTHING) : 1.0f;

			for (axis = 0; axis < 3; axis++)
				vr.two_handed_line[axis] += (line[axis] / length - vr.two_handed_line[axis]) * t;
			normalize(vr.two_handed_line);
		}
		vr.two_handed_weight += seconds / TWO_HANDED_BLEND;
		if (vr.two_handed_weight > 1.0f)
			vr.two_handed_weight = 1.0f;
	}
	else
	{
		vr.two_handed_weight -= seconds / TWO_HANDED_BLEND;
		if (vr.two_handed_weight < 0.0f || !able)
			vr.two_handed_weight = 0.0f;
	}
}

/* where the weapon in the right hand points, and its up (OpenXR axes): the
right controller's pointing pose, or with both hands on a long gun, the line
between them, the right hand's roll kept */
static void weapon_pointing(float forward[3], float up[3])
{
	static const float backwards[3] = { 0.0f, 0.0f, -1.0f };
	static const float upwards[3] = { 0.0f, 1.0f, 0.0f };
	const float *orientation = vr.views.aim[1].orientation;
	int axis;

	quaternion_rotate(orientation, backwards, forward);
	quaternion_rotate(orientation, upwards, up);
	if (vr.two_handed_weight > 0.0f)
	{
		float w = vr.two_handed_weight, along = 0.0f;

		/* (smoothstep: no jolt as the hand takes the foregrip) */
		w = w * w * (3.0f - 2.0f * w);
		for (axis = 0; axis < 3; axis++)
			forward[axis] += (vr.two_handed_line[axis] - forward[axis]) * w;
		normalize(forward);
		for (axis = 0; axis < 3; axis++)
			along += up[axis] * forward[axis];
		for (axis = 0; axis < 3; axis++)
			up[axis] -= forward[axis] * along;
		normalize(up);
	}
}

/* the controllers' turning and recentring, once a frame */
static void input_update(void)
{
	unsigned long now = GetTickCount();
	float seconds = vr.last_ticks ? (float)(now - vr.last_ticks) * 0.001f : 0.0f;
	/* (vr.aim = "gamepad": the gamepad's right stick turns, as on the flat
	screen, and the view after it, halo_vr_aim; the controllers' does not) */
	float x = settings.gamepad_aim ? 0.0f : vr.input.right_stick[0];

	vr.last_ticks = now;
	if (seconds > 0.1f)
		seconds = 0.1f;
	/* a punch of the right controller melees: its hand moving fast along
	where it points, for a few frames (vr.melee_gesture) */
	if (vr.melee_frames > 0)
		vr.melee_frames--;
	if (settings.melee_gesture && !settings.gamepad_aim && vr.views.grip[1].valid && vr.hand_valid &&
		seconds > 0.0f)
	{
		static const float backwards[3] = { 0.0f, 0.0f, -1.0f };
		float pointing[3], speed = 0.0f;
		int axis;

		quaternion_rotate(vr.views.grip[1].orientation, backwards, pointing);
		for (axis = 0; axis < 3; axis++)
			speed += (vr.views.grip[1].position[axis] - vr.hand_position[axis]) * pointing[axis];
		speed /= seconds;
		if (speed > MELEE_SPEED && !vr.melee_frames && now - vr.melee_ticks > MELEE_INTERVAL_MS)
		{
			vr.melee_frames = 3;
			vr.melee_ticks = now;
		}
	}
	two_handed_update(seconds);
	vr.hand_valid = vr.views.grip[1].valid;
	memcpy(vr.hand_position, vr.views.grip[1].position, sizeof(vr.hand_position));
	/* debug.vr_test_turn: turning without hands, for automated tests (the
	gamepad's aim turns the body itself) */
	if (!settings.gamepad_aim)
		vr.body_yaw += settings.test_turn * seconds;
	/* a gamepad's Back held recentres (the controllers may be off) */
	if (vr.gamepad_back)
	{
		vr.gamepad_recentre_time += seconds;
		if (vr.gamepad_recentre_time >= GAMEPAD_RECENTRE_SECONDS &&
			vr.gamepad_recentre_time - seconds < GAMEPAD_RECENTRE_SECONDS)
		{
			recentre();
		}
	}
	else
	{
		vr.gamepad_recentre_time = 0.0f;
	}
	if (settings.smooth_turn)
	{
		if (fabsf(x) > 0.2f)
			vr.body_yaw -= (x - (x > 0.0f ? 0.2f : -0.2f)) / 0.8f * settings.smooth_speed * seconds;
	}
	else if (!vr.snap_latched && fabsf(x) > 0.7f)
	{
		vr.body_yaw -= x > 0.0f ? settings.snap_angle : -settings.snap_angle;
		vr.snap_latched = 1;
	}
	else if (vr.snap_latched && fabsf(x) < 0.3f)
	{
		vr.snap_latched = 0;
	}
	/* (not while the left hand holds a long gun's foregrip) */
	if (vr.input.left_grip > 0.8f && vr.input.right_grip > 0.8f && !vr.two_handed)
	{
		vr.recentre_time += seconds;
		if (vr.recentre_time >= RECENTRE_SECONDS && vr.recentre_time - seconds < RECENTRE_SECONDS)
			recentre();
	}
	else
	{
		vr.recentre_time = 0.0f;
	}
}

/* debug.gpu_stats: each frame's time from one wait's end to the next's (as
the game sees it), the part of it spent waiting for the runtime (the rest is
the game's), and the frames that took more than one display period */
static void timing_update(void)
{
	Uint64 now = SDL_GetTicksNS();

	if (vr.frame_start)
	{
		double frame = (double)(now - vr.frame_start) * 1e-9;

		vr.frame_seconds += frame;
		vr.wait_seconds += (double)(now - vr.wait_start) * 1e-9;
		if (frame > vr.worst_seconds)
			vr.worst_seconds = frame;
		if (vr.views.display_period > 0.0f && frame > vr.views.display_period * 1.5f)
			vr.late_frames++;
		if (++vr.timed_frames == 300)
		{
			platform_log("vr: %.2f ms a frame (period %.2f ms), %.2f ms of it the game's, worst %.2f ms, %lu of %lu late",
				vr.frame_seconds * 1000.0 / vr.timed_frames, vr.views.display_period * 1000.0f,
				(vr.frame_seconds - vr.wait_seconds) * 1000.0 / vr.timed_frames, vr.worst_seconds * 1000.0,
				vr.late_frames, vr.timed_frames);
			vr.timed_frames = vr.late_frames = 0;
			vr.frame_seconds = vr.wait_seconds = vr.worst_seconds = 0.0;
		}
	}
	vr.frame_start = now;
}

/* debug.vr_test_jitter: the head moved and turned a little, differently
each frame, as a worn headset's is (by up to this many millimetres, and as
many hundredths of a degree about each axis): for automated tests of what
flickers when the view moves by so little, with the headset still */
static float jitter_random(void)
{
	/* (xorshift: from -1 to 1) */
	static unsigned int state = 0x9e3779b9u;

	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return (float)(state >> 8) / (float)(1u << 23) - 1.0f;
}

static void jitter_new(void)
{
	float half_angles[3];
	int axis;

	for (axis = 0; axis < 3; axis++)
	{
		vr.jitter_offset[axis] = settings.test_jitter * jitter_random();
		/* (0.01 degree a millimetre, halved for the quaternion) */
		half_angles[axis] = settings.test_jitter * 1000.0f * 0.01f * (float)M_PI / 180.0f * 0.5f *
			jitter_random();
	}
	vr.jitter_turn[0] = half_angles[0];
	vr.jitter_turn[1] = half_angles[1];
	vr.jitter_turn[2] = half_angles[2];
	vr.jitter_turn[3] = 1.0f;
	{
		float length = sqrtf(vr.jitter_turn[0] * vr.jitter_turn[0] + vr.jitter_turn[1] * vr.jitter_turn[1] +
			vr.jitter_turn[2] * vr.jitter_turn[2] + 1.0f);

		for (axis = 0; axis < 4; axis++)
			vr.jitter_turn[axis] /= length;
	}
}

/* debug.vr_test_head: the head where it was first located, level, its yaw
a sine of the frames' display times (a simulated headset's head wobbles) */
static void test_head_apply(void)
{
	struct vr_host_pose *eyes = vr.views.eye;
	float half_ipd[3], v[3], turn[4], yaw = 0.0f;
	int eye, axis;

	if (!settings.test_head || !eyes[0].valid || !eyes[1].valid)
		return;
	for (axis = 0; axis < 3; axis++)
		v[axis] = (eyes[1].position[axis] - eyes[0].position[axis]) * 0.5f;
	if (!vr.test_head_placed)
	{
		for (axis = 0; axis < 3; axis++)
			vr.test_head_centre[axis] = (eyes[0].position[axis] + eyes[1].position[axis]) * 0.5f;
		vr.test_head_placed = 1;
	}
	if (settings.test_head_period > 0.0f)
		yaw = settings.test_head_yaw * (float)M_PI / 180.0f *
			(float)sin(2.0 * M_PI * vr.test_head_seconds / settings.test_head_period);
	turn[0] = turn[2] = 0.0f;
	turn[1] = sinf(yaw * 0.5f);
	turn[3] = cosf(yaw * 0.5f);
	v[0] = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[1] = v[2] = 0.0f;
	quaternion_rotate(turn, v, half_ipd);
	for (eye = 0; eye < 2; eye++)
	{
		for (axis = 0; axis < 3; axis++)
			eyes[eye].position[axis] = vr.test_head_centre[axis] + (eye ? half_ipd[axis] : -half_ipd[axis]);
		memcpy(eyes[eye].orientation, turn, sizeof(turn));
	}
	memcpy(vr.views.head.position, vr.test_head_centre, sizeof(vr.test_head_centre));
	memcpy(vr.views.head.orientation, turn, sizeof(turn));
}

/* the located views with the frame's jitter: the eyes turned about the
head's centre, and all of it moved */
static void jitter_apply(void)
{
	struct vr_host_pose *poses[3] = { &vr.views.eye[0], &vr.views.eye[1], &vr.views.head };
	float centre[3], v[3], turned[3];
	int pose, axis;

	if (settings.test_jitter <= 0.0f)
		return;
	for (axis = 0; axis < 3; axis++)
		centre[axis] = (vr.views.eye[0].position[axis] + vr.views.eye[1].position[axis]) * 0.5f;
	for (pose = 0; pose < 3; pose++)
	{
		for (axis = 0; axis < 3; axis++)
			v[axis] = poses[pose]->position[axis] - centre[axis];
		quaternion_rotate(vr.jitter_turn, v, turned);
		for (axis = 0; axis < 3; axis++)
			poses[pose]->position[axis] = centre[axis] + turned[axis] + vr.jitter_offset[axis];
		quaternion_multiply(vr.jitter_turn, poses[pose]->orientation, poses[pose]->orientation);
	}
}

/* debug.vr_test_hands: the controllers held where the setting says, from
the recentred head, each of its poses in turn (for automated tests of the
hands: a simulated headset has none) */
static void test_hands_apply(void)
{
	static int last_index = -1;
	const float *pose;
	int hand, index;

	if (!settings.test_hand_poses || !vr.origin_valid)
		return;
	index = (int)(vr.presents / TEST_HANDS_FRAMES % (unsigned long)settings.test_hand_poses);
	if (index != last_index)
	{
		platform_log("vr: test hands: pose %d from frame %lu", index, vr.presents);
		last_index = index;
	}
	pose = settings.test_hands[index];
	for (hand = 0; hand < 2; hand++)
	{
		const float *p = pose + hand * 6;
		float radians = (float)M_PI / 180.0f;
		float local[3] = { p[0], p[1], -p[2] };
		float yaw[4] = { 0.0f, sinf((vr.origin_yaw + p[3] * radians) * 0.5f), 0.0f,
			cosf((vr.origin_yaw + p[3] * radians) * 0.5f) };
		float pitch[4] = { sinf(p[4] * radians * 0.5f), 0.0f, 0.0f, cosf(p[4] * radians * 0.5f) };
		float roll[4] = { 0.0f, 0.0f, -sinf(p[5] * radians * 0.5f), cosf(p[5] * radians * 0.5f) };
		float heading[4] = { 0.0f, sinf(vr.origin_yaw * 0.5f), 0.0f, cosf(vr.origin_yaw * 0.5f) };
		struct vr_host_pose held;
		float offset[3];
		int axis;

		quaternion_rotate(heading, local, offset);
		for (axis = 0; axis < 3; axis++)
			held.position[axis] = vr.origin_position[axis] + offset[axis];
		quaternion_multiply(yaw, pitch, held.orientation);
		quaternion_multiply(held.orientation, roll, held.orientation);
		/* (a hand 10 metres or more out to the side is not tracked) */
		held.valid = fabsf(p[0]) < 10.0f;
		vr.views.aim[hand] = held;
		vr.views.grip[hand] = held;
	}
	vr.input.left_grip = pose[12];
	vr.input.right_trigger = pose[14];
	vr.input.active[0] = vr.input.active[1] = 1;
	if (pose[15] != 0.0f)
	{
		/* the head turned about its middle, by the pose's yaw */
		struct vr_host_pose *poses[3] = { &vr.views.eye[0], &vr.views.eye[1], &vr.views.head };
		float half = pose[15] * (float)M_PI / 180.0f * 0.5f;
		float turn[4] = { 0.0f, sinf(half), 0.0f, cosf(half) };
		float centre[3], v[3], turned[3];
		int which, axis;

		for (axis = 0; axis < 3; axis++)
			centre[axis] = (vr.views.eye[0].position[axis] + vr.views.eye[1].position[axis]) * 0.5f;
		for (which = 0; which < 3; which++)
		{
			for (axis = 0; axis < 3; axis++)
				v[axis] = poses[which]->position[axis] - centre[axis];
			quaternion_rotate(turn, v, turned);
			for (axis = 0; axis < 3; axis++)
				poses[which]->position[axis] = centre[axis] + turned[axis];
			quaternion_multiply(turn, poses[which]->orientation, poses[which]->orientation);
		}
	}
	if (vr.presents % TEST_HANDS_FRAMES < TEST_HANDS_PRESS_FRAMES)
	{
		unsigned int buttons = (unsigned int)pose[13];

		vr.input.buttons |= buttons & 0xffff;
		if (buttons & TEST_HANDS_RIGHT_TRIGGER)
			vr.input.right_trigger = 1.0f;
		if (buttons & TEST_HANDS_LEFT_TRIGGER)
			vr.input.left_trigger = 1.0f;
	}
}

int halo_vr_test_hands(void)
{
	return settings.test_hand_poses > 0;
}

/* waits for the runtime's next frame and begins it; whether it did now */
static int frame_begin(void)
{
	if (!vr.initialized || vr.frame || vr.attempted)
		return 0;
	/* (once a present: until the session runs, each try waits a little) */
	vr.attempted = 1;
	if (vr.pending)
	{
		host_vr_frame_end(&vr.pending_layers);
		vr.acquired[0] = vr.acquired[1] = vr.acquired[2] = -1;
		vr.pending = 0;
	}
	settings_read();
	vr.wait_start = SDL_GetTicksNS();
	if (!host_vr_frame_wait(&vr.views))
		return 0;
	vr.frame = 1;
	if (vr.statistics)
		timing_update();
	if (vr.force_render)
		vr.views.should_render = 1;
	jitter_new();
	vr.test_head_seconds += vr.views.display_elapsed > 0.0f ? vr.views.display_elapsed : vr.views.display_period;
	if (host_vr_locate(&vr.views))
	{
		test_head_apply();
		jitter_apply();
		if (!vr.origin_valid || (vr.views.focused && !vr.was_focused))
			recentre();
	}
	if (vr.views.head.valid)
		vr.was_focused = vr.views.focused;
	host_vr_input(&vr.input);
	test_hands_apply();
	input_update();
	return 1;
}

/* The frame is begun (the runtime's wait) before the game reads its clock
(main.c), and the time from the last frame's display to this one's is the
frame's: the world moves on by as much as the headset shows it moving.
Read after a draw that ended wherever the game's work did, the clock
stepped the frames unevenly, 9 to 19 ms at 72 Hz, which the headset's even
frames showed as judder (the Pelicans' flights in). */
int halo_vr_frame_clock(float *seconds)
{
	if (!frame_begin() || vr.views.display_elapsed <= 0.0f)
		return 0;
	*seconds = vr.views.display_elapsed;
	return 1;
}

int halo_vr_frame_active(void)
{
	frame_begin();
	return vr.frame && vr.views.should_render;
}

/* The frame is ended (xrEndFrame) only when the next begins, as the game
starts drawing it: SteamVR's xrEndFrame waits for the GPU to finish the
frame, and in the meantime the game updates (input, ticks, sounds). */
void halo_vr_present(int eyes_drawn, int hud_drawn)
{
	struct vr_host_layers *pending = &vr.pending_layers;

	vr.attempted = 0;
	vr.presents++;
	/* (the aim follows the head again only if the next update says so; the
	gamepad goes by this frame's) */
	vr.aimed = vr.aiming;
	vr.aiming = 0;
	if (!vr.frame)
		return;
	memset(pending, 0, sizeof(*pending));
	if (vr.views.should_render)
	{
		pending->projection = eyes_drawn && vr.acquired[VR_SWAPCHAIN_EYES] >= 0;
		memcpy(pending->eye, vr.views.eye, sizeof(pending->eye));
		memcpy(pending->fov, vr.views.fov, sizeof(pending->fov));
		pending->hud = hud_drawn && vr.acquired[VR_SWAPCHAIN_HUD] >= 0;
		pending->depth = pending->projection && vr.depth_written && vr.acquired[VR_SWAPCHAIN_DEPTH] >= 0 &&
			vr.depth_far > vr.depth_near;
		pending->near_z = vr.depth_near;
		pending->far_z = vr.depth_far;
		if (pending->projection && !vr.menus)
		{
			/* the HUD before the eyes, where the HUD's camera looks */
			float width = 2.0f * settings.hud_distance * tanf(settings.hud_size * 0.5f * (float)M_PI / 180.0f);

			pending->hud_head_locked = 1;
			pending->hud_pose.orientation[3] = 1.0f;
			pending->hud_pose.position[2] = -settings.hud_distance;
			pending->hud_size[0] = width;
			pending->hud_size[1] = width * (float)HUD_HEIGHT / (float)HUD_WIDTH;
		}
		else
		{
			/* the menus, in front of where the head was at the recentring */
			float s = sinf(vr.origin_yaw * 0.5f), c = cosf(vr.origin_yaw * 0.5f);

			pending->hud_pose.orientation[1] = s;
			pending->hud_pose.orientation[3] = c;
			pending->hud_pose.position[0] = vr.origin_position[0] - sinf(vr.origin_yaw) * settings.menu_distance;
			pending->hud_pose.position[1] = settings.standing ? settings.player_height : vr.origin_position[1];
			pending->hud_pose.position[2] = vr.origin_position[2] - cosf(vr.origin_yaw) * settings.menu_distance;
			pending->hud_size[0] = settings.menu_width;
			pending->hud_size[1] = settings.menu_width * (float)HUD_HEIGHT / (float)HUD_WIDTH;
		}
	}
	vr.pending = 1;
	vr.frame = 0;
	vr.depth_written = 0;
}

void halo_vr_depth_range(float near_metres, float far_metres)
{
	vr.depth_near = near_metres;
	vr.depth_far = far_metres;
}

int halo_vr_depth_wanted(void)
{
	return vr.initialized && vr.info.depth && vr.frame;
}

void halo_vr_depth_written(void)
{
	vr.depth_written = 1;
}

/* ---------- the views */

int halo_vr_view(struct halo_vr_view *view)
{
	const struct vr_host_pose *head;
	float centre[3], offset[3];
	static const float forward[3] = { 0.0f, 0.0f, -1.0f };
	static const float up[3] = { 0.0f, 1.0f, 0.0f };
	float v[3];
	float inverse[4];
	int eye;

	if (!halo_vr_frame_active())
		return 0;
	/* late: the poses the frame is drawn and submitted with */
	if (host_vr_locate(&vr.views))
	{
		test_head_apply();
		jitter_apply();
		test_hands_apply();
	}
	if (!vr.views.eye[0].valid)
		return 0;
	head = &vr.views.head;
	quaternion_rotate(vr.views.eye[0].orientation, forward, v);
	game_axes(v, vr.origin_yaw, view->head_forward);
	quaternion_rotate(vr.views.eye[0].orientation, up, v);
	game_axes(v, vr.origin_yaw, view->head_up);
	/* the head's centre, between the eyes */
	for (eye = 0; eye < 3; eye++)
	{
		centre[eye] = (vr.views.eye[0].position[eye] + vr.views.eye[1].position[eye]) * 0.5f;
		offset[eye] = centre[eye] - vr.origin_position[eye];
	}
	game_axes(offset, vr.origin_yaw, view->head_position);
	/* each eye from the centre, in the head's axes */
	inverse[0] = -vr.views.eye[0].orientation[0];
	inverse[1] = -vr.views.eye[0].orientation[1];
	inverse[2] = -vr.views.eye[0].orientation[2];
	inverse[3] = vr.views.eye[0].orientation[3];
	for (eye = 0; eye < 2; eye++)
	{
		float local[3];
		int axis;

		for (axis = 0; axis < 3; axis++)
			v[axis] = vr.views.eye[eye].position[axis] - centre[axis];
		quaternion_rotate(inverse, v, local);
		game_axes(local, 0.0f, view->eye_offset[eye]);
		memcpy(view->fov[eye], vr.views.fov[eye], sizeof(view->fov[eye]));
	}
	(void)head;
	view->body_yaw = vr.body_yaw;
	view->hud_tangent = tanf(settings.hud_size * 0.5f * (float)M_PI / 180.0f) * (float)HUD_HEIGHT / (float)HUD_WIDTH;
	view->world_units_per_metre = settings.world_scale / METRES_PER_WORLD_UNIT;
	/* the right controller */
	view->controller_aim = settings.controller_aim && vr.views.aim[1].valid;
	if (view->controller_aim)
	{
		const struct vr_host_pose *hand = &vr.views.aim[1];
		float pointing_up[3];

		weapon_pointing(v, pointing_up);
		game_axes(v, vr.origin_yaw, view->hand_forward);
		game_axes(pointing_up, vr.origin_yaw, view->hand_up);
		for (eye = 0; eye < 3; eye++)
			offset[eye] = hand->position[eye] - vr.origin_position[eye];
		game_axes(offset, vr.origin_yaw, view->hand_position);
	}
	memcpy(view->weapon_offset, settings.weapon_offset, sizeof(view->weapon_offset));
	/* the hands (vr.hands): where the controllers' grips are (else where
	they point from), and where the left one points */
	view->floating_hands = settings.floating_hands && settings.controller_aim;
	view->arms = settings.arms;
	view->left_hand_valid = vr.views.aim[0].valid;
	if (view->floating_hands && view->controller_aim)
	{
		const struct vr_host_pose *grip = vr.views.grip[1].valid ? &vr.views.grip[1] : &vr.views.aim[1];

		for (eye = 0; eye < 3; eye++)
			offset[eye] = grip->position[eye] - vr.origin_position[eye];
		game_axes(offset, vr.origin_yaw, view->right_grip_position);
	}
	if (view->floating_hands && view->left_hand_valid)
	{
		const struct vr_host_pose *hand = &vr.views.aim[0];
		const struct vr_host_pose *grip = vr.views.grip[0].valid ? &vr.views.grip[0] : hand;

		quaternion_rotate(hand->orientation, forward, v);
		game_axes(v, vr.origin_yaw, view->left_hand_forward);
		quaternion_rotate(hand->orientation, up, v);
		game_axes(v, vr.origin_yaw, view->left_hand_up);
		for (eye = 0; eye < 3; eye++)
			offset[eye] = grip->position[eye] - vr.origin_position[eye];
		game_axes(offset, vr.origin_yaw, view->left_grip_position);
	}
	view->two_handed = vr.two_handed_weight * vr.two_handed_weight * (3.0f - 2.0f * vr.two_handed_weight);
	view->right_trigger = vr.input.right_trigger;
	view->left_grip = vr.input.left_grip;
	memcpy(view->shoulder_offset, settings.shoulder_offset, sizeof(view->shoulder_offset));
	view->seconds = vr.views.display_elapsed > 0.0f ? vr.views.display_elapsed : vr.views.display_period;
	view->vehicle_first_person = settings.vehicle_first_person;
	view->gamepad_aim = settings.gamepad_aim;
	view->gamepad_pitch = settings.gamepad_aim && settings.gamepad_pitch;
	return 1;
}

/* ---------- aiming */

int halo_vr_aim(float current_yaw, int long_gun, float *yaw, float *pitch)
{
	static const float backwards[3] = { 0.0f, 0.0f, -1.0f };
	float v[3], f[3], head_yaw;

	if (!(vr.frame || vr.pending) || !vr.views.should_render || !vr.views.head.valid)
	{
		vr.aiming = 0;
		return 0;
	}
	if (settings.gamepad_aim)
	{
		/* the gamepad aims, as on the flat screen: the facing stays the
		game's, and the body (the view's heading before the head's turn)
		follows it: with it (vr.gamepad_view = "camera", "level"), or a step
		of vr.snap_turn_angle each time the aim is more than that from it
		("snap"), so the reticle and the weapon move about the view and the
		view turns in steps. (The view's pitch: vr_render.c.) */
		float off = remainderf(current_yaw - vr.body_yaw, 2.0f * (float)M_PI);
		float step = settings.snap_angle;

		if (!vr.aim_synced || !settings.gamepad_snap || step <= 0.0f || fabsf(off) > 2.0f * step)
			vr.body_yaw = current_yaw;
		else if (fabsf(off) > step)
			vr.body_yaw += off > 0.0f ? step : -step;
		vr.aim_synced = 1;
		*yaw = current_yaw;
		*pitch = 0.0f;
		vr.aiming = 1;
		return 2;
	}
	quaternion_rotate(vr.views.head.orientation, backwards, v);
	game_axes(v, vr.origin_yaw, f);
	head_yaw = atan2f(f[1], f[0]);
	if (!vr.aim_synced)
	{
		/* the turn that keeps the view where the player faced */
		vr.body_yaw = current_yaw - head_yaw;
		vr.aim_synced = 1;
	}
	vr.long_gun = long_gun;
	/* where the right controller points (vr.aim), or a long gun held in
	both hands, else where the head looks */
	if (settings.controller_aim && vr.views.aim[1].valid)
	{
		float up[3];

		weapon_pointing(v, up);
		game_axes(v, vr.origin_yaw, f);
		head_yaw = atan2f(f[1], f[0]);
	}
	*yaw = vr.body_yaw + head_yaw;
	*pitch = asinf(f[2] < -1.0f ? -1.0f : f[2] > 1.0f ? 1.0f : f[2]);
	vr.aiming = 1;
	return 1;
}

void halo_vr_aim_release(void)
{
	vr.aim_synced = 0;
	vr.aiming = 0;
}

void halo_vr_menus(int active)
{
	vr.menus = active;
}

int halo_vr_aiming(void)
{
	return vr.aiming;
}

int halo_vr_gamepad_aiming(void)
{
	return vr.aiming && settings.gamepad_aim;
}

void halo_vr_gamepad_back(int held)
{
	vr.gamepad_back = held;
}

/* ---------- the controllers as a gamepad */

static void analog(XINPUT_GAMEPAD *pad, int index, int down)
{
	if (down)
		pad->bAnalogButtons[index] = 0xff;
}

static void stick_merge(SHORT *axis, float value)
{
	long scaled = (long)(value * 32767.0f);

	if (scaled > 32767)
		scaled = 32767;
	if (scaled < -32767)
		scaled = -32767;
	if (labs(scaled) > labs((long)*axis))
		*axis = (SHORT)scaled;
}

void halo_vr_gamepad(void *gamepad)
{
	XINPUT_GAMEPAD *pad = gamepad;
	unsigned int buttons = vr.input.buttons;
	BYTE trigger;

	/* (vr.aim = "gamepad": the controllers play as the halves of an Xbox
	controller, as any gamepad does, their poses aiming nothing) */
	if (!vr.initialized || !(vr.frame || vr.pending))
		return;
	analog(pad, XINPUT_GAMEPAD_A, buttons & VR_BUTTON_A);
	analog(pad, XINPUT_GAMEPAD_B, (buttons & VR_BUTTON_B) || vr.melee_frames > 0);
	analog(pad, XINPUT_GAMEPAD_X, buttons & VR_BUTTON_X);
	analog(pad, XINPUT_GAMEPAD_Y, buttons & VR_BUTTON_Y);
	/* the bumpers where the Xbox's white and black buttons were: the
	flashlight and the grenade switch */
	analog(pad, XINPUT_GAMEPAD_WHITE, buttons & VR_BUTTON_LEFT_BUMPER);
	analog(pad, XINPUT_GAMEPAD_BLACK, buttons & VR_BUTTON_RIGHT_BUMPER);
	trigger = (BYTE)(vr.input.left_trigger * 255.0f);
	if (trigger > pad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER])
		pad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] = trigger;
	trigger = (BYTE)(vr.input.right_trigger * 255.0f);
	if (trigger > pad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER])
		pad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] = trigger;
	if (buttons & VR_BUTTON_DPAD_UP) pad->wButtons |= XINPUT_GAMEPAD_DPAD_UP;
	if (buttons & VR_BUTTON_DPAD_DOWN) pad->wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
	if (buttons & VR_BUTTON_DPAD_LEFT) pad->wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
	if (buttons & VR_BUTTON_DPAD_RIGHT) pad->wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
	if (buttons & VR_BUTTON_MENU) pad->wButtons |= XINPUT_GAMEPAD_START;
	if (buttons & VR_BUTTON_VIEW) pad->wButtons |= XINPUT_GAMEPAD_BACK;
	if (buttons & VR_BUTTON_LEFT_STICK) pad->wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
	if (buttons & VR_BUTTON_RIGHT_STICK) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
	stick_merge(&pad->sThumbLX, vr.input.left_stick[0]);
	stick_merge(&pad->sThumbLY, vr.input.left_stick[1]);
	/* the right stick turns (input_update); while the aim does not follow
	the head (menus, cinematics, a vehicle's turret without it), and always
	with vr.aim = "gamepad", it looks as a gamepad's would */
	if (!vr.aimed || settings.gamepad_aim)
	{
		stick_merge(&pad->sThumbRX, vr.input.right_stick[0]);
		stick_merge(&pad->sThumbRY, vr.input.right_stick[1]);
	}
}

void halo_vr_rumble(float left, float right)
{
	if (!vr.initialized)
		return;
	if (left > 0.01f)
		host_vr_haptic(0, left, 0.1f);
	if (right > 0.01f)
		host_vr_haptic(1, right, 0.1f);
}
#endif
