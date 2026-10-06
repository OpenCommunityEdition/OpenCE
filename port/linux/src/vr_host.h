/*
VR_HOST.H

What the VR mode (HALO_VR, vr.c) asks of OpenXR, which the host does
(port/linux/arm64/host_vr.c): the session, the swapchains, the frame loop,
the poses and the controllers. On the Linux arm64 build the game is guest
code with 32-bit pointers that cannot call OpenXR, whose structures hold
64-bit pointers and handles, so these structures hold only 32-bit integers
and floats, laid out alike on both sides, and the swapchains' images are GL
texture names.

Poses are in the play space: OpenXR's (metres; x right, y up, z backwards),
the stage when standing, the local space when seated. Field of view angles
are tangents: left and down negative.
*/

#ifndef __HALO_VR_HOST_H
#define __HALO_VR_HOST_H

enum
{
	VR_SWAPCHAIN_EYES = 0, /* a 2-layer texture array: layer 0 the left eye */
	VR_SWAPCHAIN_HUD,      /* the HUD and the menus, a quad layer */
	VR_SWAPCHAIN_DEPTH,    /* the eyes' depth (XR_KHR_composition_layer_depth), 2 layers */
	VR_SWAPCHAIN_COUNT,
	VR_SWAPCHAIN_IMAGES = 4,
};

/* the controllers' buttons (struct vr_host_input) */
enum
{
	VR_BUTTON_A = 1 << 0,
	VR_BUTTON_B = 1 << 1,
	VR_BUTTON_X = 1 << 2,
	VR_BUTTON_Y = 1 << 3,
	VR_BUTTON_MENU = 1 << 4,
	VR_BUTTON_VIEW = 1 << 5,
	VR_BUTTON_DPAD_UP = 1 << 6,
	VR_BUTTON_DPAD_DOWN = 1 << 7,
	VR_BUTTON_DPAD_LEFT = 1 << 8,
	VR_BUTTON_DPAD_RIGHT = 1 << 9,
	VR_BUTTON_LEFT_STICK = 1 << 10,
	VR_BUTTON_RIGHT_STICK = 1 << 11,
	VR_BUTTON_LEFT_BUMPER = 1 << 12,
	VR_BUTTON_RIGHT_BUMPER = 1 << 13,
};

struct vr_host_info
{
	/* in: the eye images' size (0: the runtime's recommendation scaled by
	scale); out: the size made */
	int eye_width, eye_height;
	float scale;
	/* in */
	int hud_width, hud_height;
	int standing;
	float refresh_rate_wanted; /* 0: the runtime's choice */
	int depth; /* in: submit the eyes' depth; out: the runtime takes it */
	/* out */
	int recommended_width, recommended_height;
	int image_count[VR_SWAPCHAIN_COUNT];
	unsigned int images[VR_SWAPCHAIN_COUNT][VR_SWAPCHAIN_IMAGES];
	float refresh_rate;
	char system_name[64];
};

struct vr_host_pose
{
	float position[3];
	float orientation[4]; /* x, y, z, w */
	int valid;
};

struct vr_host_views
{
	/* the frame's (host_vr_frame_wait) */
	int should_render;
	int focused;
	float display_period; /* seconds */
	/* from the last frame's predicted display time to this one's; 0 on
	the first */
	float display_elapsed; /* seconds */
	/* host_vr_locate's, at the frame's predicted display time */
	struct vr_host_pose eye[2];
	float fov[2][4]; /* left, right, up, down */
	struct vr_host_pose head;
	struct vr_host_pose aim[2]; /* the controllers' pointing poses */
	struct vr_host_pose grip[2];
};

struct vr_host_input
{
	unsigned int buttons;
	float left_stick[2], right_stick[2];
	float left_trigger, right_trigger;
	float left_grip, right_grip;
	int active[2];
};

struct vr_host_layers
{
	/* the eye images, drawn from these views */
	int projection;
	struct vr_host_pose eye[2];
	float fov[2][4];
	/* with the eyes' depth (0 to 1 from near_z to far_z, metres) */
	int depth;
	float near_z, far_z;
	/* the HUD image on a quad: in front of the head (head_locked), or at
	pose in the play space; size in metres */
	int hud;
	int hud_head_locked;
	struct vr_host_pose hud_pose;
	float hud_size[2];
};

/* after the GL context is made and current (on its thread); 0 if there is
no OpenXR runtime, headset or GL binding */
int host_vr_initialize(struct vr_host_info *info);
/* waits for the runtime's next frame and begins it; 0 if the session is
not running (no frame is begun) */
int host_vr_frame_wait(struct vr_host_views *views);
/* the views, head and controllers at the frame's display time */
int host_vr_locate(struct vr_host_views *views);
int host_vr_input(struct vr_host_input *input);
/* the swapchain's next image (an index into vr_host_info's images), or -1 */
int host_vr_acquire(int swapchain);
void host_vr_release(int swapchain);
/* ends the frame host_vr_frame_wait began */
void host_vr_frame_end(const struct vr_host_layers *layers);
void host_vr_haptic(int hand, float amplitude, float seconds);

#endif
