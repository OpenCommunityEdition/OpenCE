#ifndef HALO_MAC_ENGINE_INPUT_H
#define HALO_MAC_ENGINE_INPUT_H

#include "cseries.h"
#include "math/real_math.h"

struct mac_input_frame;
struct player_control;
struct player_action;

/* Pure translation helpers retained for callers which separate native input
   sampling from engine-resolved player control. They never mutate control. */
struct mac_engine_input
{
	real_vector2d throttle;
	real_euler_angles2d facing_delta;
	real primary_trigger;
	unsigned long unit_control_flags;
	boolean pause_pressed;
};

void mac_engine_input_prepare(
	struct mac_input_frame const *frame,
	unsigned int allowed_actions,
	int movement_enabled,
	int look_enabled,
	struct mac_engine_input *prepared);
void mac_engine_input_action(
	struct mac_engine_input const *prepared,
	struct player_control const *resolved_control,
	struct player_action *action);

/* Publish one native keyboard/mouse snapshot before the engine's input_update
   call. This exposes a virtual keyboard/mouse controller at port 0, matching
   the shared SDL backend; no physical gamepad is advertised. */
void mac_engine_input_publish(const struct mac_input_frame *frame);
/* The keyboard standing in for the first game pad's buttons (the menus'):
   masks of its analog buttons (A, B, X, Y, black, white, triggers) and of
   its binary ones (up, down, left, right, start, back, thumbs). */
/* port: the on-screen keyboard or a text field is up (platform_text_typing): the keys type */
int mac_engine_input_text_typing(void);
void mac_engine_input_keyboard_pad(unsigned analog_mask, unsigned binary_mask);
/* diagnostic: the first pad's left stick pushed to x for the next update */
void mac_engine_input_script_pad(short left_x);
void platform_text_typing(int typing);
void platform_text_field(int typing, int password);
void platform_binding_capture_begin(void);
int platform_binding_capture_poll(int *input);
int halo_input_from_name(const char *name);
void halo_input_name(int input,char *name,size_t size);
/* port: whether a keyboard action's binding (controls.<action>, a
   halo_keyboard_action) is held in this native snapshot: the menus' pause
   key. Keys and mouse buttons; the wheel is not held. */
int mac_engine_input_bound_held(unsigned action,const struct mac_input_frame *frame);
/* port: the screenshot key (controls.screenshot) was pressed since this was
   last asked; 1 once for each press */
int mac_engine_input_screenshot_pressed(void);
/* port: the rumble of the game pad at a port (left: the large motor, right: the small one, 0 to 65535);
   1 when a pad took it, 0 for none (only a Sony pad on USB has rumble) */
int mac_engine_input_rumble(unsigned port, unsigned short left, unsigned short right);

#endif
