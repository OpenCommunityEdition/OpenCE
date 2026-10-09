/*
HALO_ANDROID_ABI.H

The contract between the two halves of the Android port (see
port/android/README.md):

- the guest: the game, the platform layer shared with the Linux port and a
  small C runtime, compiled as ILP32 AArch64 code (32-bit pointers) and
  linked into a static image that runs in the low 4 GB of the process;
- the host: an ordinary 64-bit Android library (libmain.so) that loads the
  image, owns the process (SDL3, OpenGL ES, bionic) and serves the guest's
  requests.

The guest calls the host through import stubs that jump through a table of
64-bit function pointers the host fills in at load time. Only types whose
layout and register treatment agree between the two ABIs cross this
boundary: 32-bit integers, 64-bit integers, floats, and pointers (which
arm64_32 always passes zero-extended). Structures shared here are made of
fixed-width members only.

This header is included by both halves.
*/

#ifndef __HALO_ANDROID_ABI_H
#define __HALO_ANDROID_ABI_H

#include <stdint.h>

/* the guest image is linked to run here, just above the Xbox window: ART
keeps its heaps low in the address space and fills it upwards */
#define HALO_GUEST_IMAGE_BASE 0x88000000u

/* the Xbox contiguous memory window (port/linux/src/platform.h) */
#define HALO_GUEST_WINDOW_BASE 0x80000000u
#define HALO_GUEST_WINDOW_SIZE 0x08000000u

#define HALO_GUEST_MAGIC 0x4f4c4148u /* 'HALO' */
#define HALO_GUEST_ABI_VERSION 1

/* at HALO_GUEST_IMAGE_BASE */
struct halo_guest_header
{
	uint32_t magic;
	uint32_t abi_version;
	uint32_t image_end;          /* end of .bss */
	uint32_t import_table;       /* uint64_t[import_count], filled by the host */
	uint32_t import_names;       /* import_count NUL-terminated names */
	uint32_t import_count;       /* address of a uint32_t holding the count */
	uint32_t start;              /* void __guest_start(struct halo_guest_boot *) */
	uint32_t thread_start;       /* void __guest_thread_start(uint32_t thread) */
	uint32_t thread_attach;      /* uint32_t __guest_thread_attach(void) */
	uint32_t init_array_start;   /* void (*)(void) entries, 4 bytes each */
	uint32_t init_array_end;
};

/* the host's description of the process, handed to __guest_start */
struct halo_guest_boot
{
	uint32_t argc;
	uint32_t argv;               /* char ** in guest memory */
	uint32_t environment;        /* char ** in guest memory, NULL-terminated */
	uint32_t page_size;
};

/* ---------- OpenXR (HALO_VR builds; host/host_xr.c, port/linux/src/vr_*.c)

Poses are in the runtime's LOCAL space (metres; +y up, -z forward),
relative to the last recentre. Every member is fixed-width and 64-bit
members come first, so both ABIs lay these out the same; the offsets are
checked on both sides below. */

#define HALO_XR_SWAPCHAIN_LEFT 0
#define HALO_XR_SWAPCHAIN_RIGHT 1
#define HALO_XR_SWAPCHAIN_QUAD 2
#define HALO_XR_SWAPCHAIN_RETICLE 3
#define HALO_XR_SWAPCHAIN_FADE 4
#define HALO_XR_SWAPCHAIN_SCOPE 5
#define HALO_XR_SWAPCHAIN_WRIST 6          /* test26: the wrist HUD's panel */
#define HALO_XR_SWAPCHAIN_COUNT 7
#define HALO_XR_MAXIMUM_IMAGES 4

struct halo_xr_pose
{
	float position[3];
	float orientation[4];        /* x, y, z, w */
};

/* filled by host_xr_init */
struct halo_xr_info
{
	int64_t display_period;      /* nanoseconds */
	int64_t color_format;        /* GL internal format of every swapchain */
	uint32_t width[HALO_XR_SWAPCHAIN_COUNT];
	uint32_t height[HALO_XR_SWAPCHAIN_COUNT];
	uint32_t image_count[HALO_XR_SWAPCHAIN_COUNT];
	uint32_t images[HALO_XR_SWAPCHAIN_COUNT][HALO_XR_MAXIMUM_IMAGES]; /* GL texture names */
	char runtime[64];
	char system[64];
};

/* frame flags */
#define HALO_XR_FRAME_BEGUN 0x1u          /* end it with host_xr_end_frame */
#define HALO_XR_FRAME_SHOULD_RENDER 0x2u
#define HALO_XR_FRAME_FOCUSED 0x4u        /* input is ours */
#define HALO_XR_FRAME_VIEWS_VALID 0x8u
#define HALO_XR_FRAME_EXIT 0x10u          /* the runtime ends the session for good */
#define HALO_XR_FRAME_RECENTRED 0x20u     /* the reference frame moved this frame */

/* buttons: the Xbox pad's digital bits (XINPUT_GAMEPAD_DPAD_UP .. RIGHT_THUMB,
port/include/xdk/xdk_xbox.h) and one bit for each of its analog buttons */
#define HALO_XR_BUTTON_DPAD_UP 0x0001u
#define HALO_XR_BUTTON_DPAD_DOWN 0x0002u
#define HALO_XR_BUTTON_DPAD_LEFT 0x0004u
#define HALO_XR_BUTTON_DPAD_RIGHT 0x0008u
#define HALO_XR_BUTTON_START 0x0010u
#define HALO_XR_BUTTON_BACK 0x0020u
#define HALO_XR_BUTTON_LEFT_THUMB 0x0040u
#define HALO_XR_BUTTON_RIGHT_THUMB 0x0080u
#define HALO_XR_BUTTON_WHITE 0x0100u       /* left shoulder */
#define HALO_XR_BUTTON_BLACK 0x0200u       /* right shoulder */
#define HALO_XR_BUTTON_A 0x1000u
#define HALO_XR_BUTTON_B 0x2000u
#define HALO_XR_BUTTON_X 0x4000u
#define HALO_XR_BUTTON_Y 0x8000u

/* filled by host_xr_begin_frame */
struct halo_xr_frame
{
	int64_t predicted_display_time; /* nanoseconds, the runtime's clock */
	int64_t predicted_display_period;
	uint32_t flags;
	uint32_t session_state;      /* XrSessionState */
	struct halo_xr_pose head;
	struct halo_xr_pose eye[2];
	float fov[2][4];             /* radians: left, right, up, down */
	uint32_t hand_valid[2];      /* bit 0 grip, bit 1 aim (0 left, 1 right) */
	struct halo_xr_pose grip[2];
	struct halo_xr_pose aim[2];
	uint32_t buttons;            /* HALO_XR_BUTTON_* */
	float trigger[2];            /* 0..1 */
	float thumb[4];              /* left x, y, right x, y: -1..1 */
	float squeeze[2];
	uint32_t hand_buttons[2];    /* HALO_XR_HAND_*, each hand's own (0 left, 1 right) */
};

/* hand_buttons: each controller's buttons as they are, for layouts that do
not follow the Xbox pad (vr.controls). Face buttons by place: south the
lower (Frame/Index/Touch right A, Touch left X), east the upper (B, Touch
left Y), west and north the Frame's right X and Y. */
#define HALO_XR_HAND_SOUTH 0x0001u
#define HALO_XR_HAND_EAST 0x0002u
#define HALO_XR_HAND_WEST 0x0004u
#define HALO_XR_HAND_NORTH 0x0008u
#define HALO_XR_HAND_BUMPER 0x0010u
#define HALO_XR_HAND_STICK 0x0020u         /* the stick pressed in */
#define HALO_XR_HAND_MENU 0x0040u
#define HALO_XR_HAND_VIEW 0x0080u
#define HALO_XR_HAND_DPAD_UP 0x0100u
#define HALO_XR_HAND_DPAD_DOWN 0x0200u
#define HALO_XR_HAND_DPAD_LEFT 0x0400u
#define HALO_XR_HAND_DPAD_RIGHT 0x0800u
#define HALO_XR_HAND_THUMB_TOUCH 0x1000u   /* the thumb resting (Touch: a button, the stick, the thumbrest) */
#define HALO_XR_HAND_INDEX_TOUCH 0x2000u   /* the index finger on the trigger */

/* layer flags for host_xr_end_frame */
#define HALO_XR_LAYER_PROJECTION 0x1u     /* both eye swapchains, as posed this frame */
#define HALO_XR_LAYER_QUAD 0x2u
#define HALO_XR_LAYER_QUAD_HEAD_LOCKED 0x4u /* the quad's pose is in VIEW space, else LOCAL */
#define HALO_XR_LAYER_QUAD_ALPHA 0x8u       /* blend the quad by its alpha, else opaque */
#define HALO_XR_LAYER_RETICLE 0x10u         /* the reticle swapchain at reticle_pose (LOCAL), blended */
#define HALO_XR_LAYER_STEREO_SCREEN 0x20u   /* the eye swapchains as a quad each eye sees its own of,
                                               at quad_pose and quad_size: a 3D screen */
#define HALO_XR_LAYER_FADE 0x40u            /* the fade swapchain over everything, head-locked */
#define HALO_XR_LAYER_SCOPE 0x80u           /* the scope swapchain at scope_pose (LOCAL), blended */
#define HALO_XR_LAYER_RETICLE_ON_TOP 0x100u /* the reticle over the quad (the menus' pointer on their
                                               screen), else under it (the hand's aim, under the HUD) */
#define HALO_XR_LAYER_WRIST 0x200u          /* test26: the wrist swapchain at wrist_pose (LOCAL), blended */
#define HALO_XR_LAYER_EYE_FOV 0x400u        /* the projection's eyes span eye_fov, not the runtime's
                                               own field of view (vr.fov_mode "glasses"; PR #1) */

struct halo_xr_layers
{
	uint32_t flags;
	struct halo_xr_pose quad_pose;
	float quad_size[2];          /* metres */
	struct halo_xr_pose reticle_pose;
	float reticle_size[2];
	struct halo_xr_pose scope_pose;
	float scope_size[2];
	struct halo_xr_pose wrist_pose;
	float wrist_size[2];
	float eye_fov[2][4];         /* radians: left, right, up, down (HALO_XR_LAYER_EYE_FOV) */
};

#ifdef __cplusplus
#define HALO_XR_ASSERT static_assert
#else
#define HALO_XR_ASSERT _Static_assert
#endif
HALO_XR_ASSERT(sizeof(struct halo_xr_pose) == 28, "halo_xr_pose layout");
HALO_XR_ASSERT(sizeof(struct halo_xr_info) == 344, "halo_xr_info layout");
HALO_XR_ASSERT(sizeof(struct halo_xr_frame) == 304, "halo_xr_frame layout");
HALO_XR_ASSERT(__builtin_offsetof(struct halo_xr_frame, head) == 24, "halo_xr_frame.head");
HALO_XR_ASSERT(__builtin_offsetof(struct halo_xr_frame, buttons) == 260, "halo_xr_frame.buttons");
HALO_XR_ASSERT(sizeof(struct halo_xr_layers) == 180, "halo_xr_layers layout");
HALO_XR_ASSERT(__builtin_offsetof(struct halo_xr_layers, wrist_pose) == 112, "halo_xr_layers.wrist_pose");
HALO_XR_ASSERT(__builtin_offsetof(struct halo_xr_layers, eye_fov) == 148, "halo_xr_layers.eye_fov");

#endif
