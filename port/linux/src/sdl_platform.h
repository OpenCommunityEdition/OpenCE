/*
SDL_PLATFORM.H

Window, OpenGL context and input state shared by the renderer and the
controller emulation (see sdl_platform.c).
*/

#ifndef __HALO_LINUX_SDL_PLATFORM_H
#define __HALO_LINUX_SDL_PLATFORM_H

#include <SDL3/SDL_scancode.h>

#define PLATFORM_MOUSE_BUTTON_COUNT 8

struct platform_input_state
{
	unsigned char keys[SDL_SCANCODE_COUNT];
	unsigned char mouse_buttons[PLATFORM_MOUSE_BUTTON_COUNT]; /* SDL_BUTTON_* */
	float mouse_dx, mouse_dy;
	float mouse_wheel;
	BOOL focused;
	BOOL mouse_released;
	/* the mouse drives the menus' pointer (platform_ui_pointer_set_active)
	instead of the controller */
	BOOL ui_pointer;
};

struct platform_keystroke
{
	BYTE virtual_key;
	CHAR ascii;
	BYTE flags;
};

BOOL platform_sdl_initialize(void);
/* creates the window and makes its OpenGL context current on this thread */
BOOL platform_video_initialize(unsigned long width, unsigned long height);
#ifndef HALO_ANDROID
BOOL platform_screen_mode(long *width, long *height);
#endif
void platform_video_drawable_size(int *width, int *height);
void platform_video_swap(void);
/* frames between the 30 Hz ticks at the display's refresh rate, unless
display.interpolation is false (port/linux/game/render_interpolation.c) */
int halo_interpolation_enabled(void);
void platform_mouse_capture(BOOL capture);

/**
 * @brief Handles the window's and the input devices' events. Main thread
 * only, a no-op elsewhere. On Android this is where the finger events reach
 * touch_input.c, which is why that module's state is only touched from the
 * main thread (see touch_input.h).
 */
void platform_pump_events(void);
/* a snapshot of the input state; consume_motion resets the mouse deltas */
void platform_input_read(struct platform_input_state *state, BOOL consume_motion);
/* the pointer in the menus (d3d8_gl.c, halo_ui_pointer_update) */
struct platform_ui_pointer
{
	/* in window coordinates, as SDL reports them (on Android the window's
	pixels: touch_input.c) */
	float x, y;
	float click_x, click_y;
	BOOL moved;
	int left_clicks, right_clicks;
	int wheel_steps;
	/* fingers down since the last read, and where the latest went down
	(the touchscreen only; the debug view of the menus' targets shows it) */
	int downs;
	float down_x, down_y;
	/* the pointer is the touchscreen (touch_input.c), not a mouse */
	BOOL touch;
};
/**
 * @brief Says whether a menu is up. The mouse is released for the menus
 * (desktop), or the touchscreen goes to them (Android). A change of mode
 * drops the gesture in progress.
 * @param active nonzero while a menu is up
 */
void platform_ui_pointer_set_active(BOOL active);

/**
 * @brief What the pointer did since the last call.
 * @param pointer receives the position, the clicks and the wheel steps
 * @return nonzero while a menu is up (platform_ui_pointer_set_active)
 */
BOOL platform_ui_pointer_read(struct platform_ui_pointer *pointer);

/**
 * @brief The window's size in the units that pointer positions come in,
 * which differ from the drawable's pixels on displays that scale.
 * @param width,height receive the size
 */
void platform_video_window_size(int *width, int *height);
BOOL platform_next_keystroke(struct platform_keystroke *keystroke);

#endif
