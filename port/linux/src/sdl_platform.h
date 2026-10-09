/*
SDL_PLATFORM.H

Window, OpenGL context and input state shared by the renderer and the
controller emulation (see sdl_platform.c).
*/

#ifndef __HALO_LINUX_SDL_PLATFORM_H
#define __HALO_LINUX_SDL_PLATFORM_H

#include <SDL3/SDL_scancode.h>
#include <stddef.h>

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
	/* Keyboard/menu bindings are separate from gameplay actions. */
	BOOL menus;
};

#define INPUT_MOUSE SDL_SCANCODE_COUNT
#define INPUT_WHEEL (INPUT_MOUSE + PLATFORM_MOUSE_BUTTON_COUNT)
#define INPUT_WHEEL_UP (INPUT_WHEEL + 1)
#define INPUT_WHEEL_DOWN (INPUT_WHEEL + 2)
int halo_input_from_name(const char *name);
void halo_input_name(int input, char *name, size_t size);
void platform_menus_set_active(BOOL active);
void platform_binding_capture_begin(void);
/* 0 waiting, 1 captured, 2 clear (Delete), 3 cancel (Escape/focus lost). */
int platform_binding_capture_poll(int *input);
int platform_clipboard_get(char *text, int size);
void platform_clipboard_set(const char *text);
void platform_request_quit(void);

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
/* The current drawable only. Desktop display-mode rows are not offered by
the Android/Quest menus; these queries never resize/recreate a VR target. */
int platform_display_resolutions(long *widths, long *heights, int maximum);
int platform_window_sizes(long *widths, long *heights, int maximum);
/* Apply flat presentation VSync only; OpenXR retains its own pacing. */
void platform_display_apply(void);
void platform_video_swap(void);
/* frames between the 30 Hz ticks at the display's refresh rate, unless
display.interpolation is false (port/linux/game/render_interpolation.c) */
int halo_interpolation_enabled(void);
void platform_mouse_capture(BOOL capture);

/* main thread only; a no-op elsewhere */
void platform_pump_events(void);
/* Upstream scoreboard wheel/key paging plus logical controller/touch/VR paging. */
void platform_scoreboard_scroll(int open, long *notches, long *pages);
void platform_scoreboard_gamepad(unsigned short *buttons, short *right_y);
/* a snapshot of the input state; consume_motion resets the mouse deltas */
void platform_input_read(struct platform_input_state *state, BOOL consume_motion);
/* the pointer in the menus (d3d8_gl.c, halo_ui_pointer_update) */
struct platform_ui_pointer
{
	/* in window coordinates, as SDL reports them */
	float x, y;
	float click_x, click_y;
	BOOL moved;
	int left_clicks, right_clicks;
	int wheel_steps;
};
void platform_ui_pointer_set_active(BOOL active);
BOOL platform_ui_pointer_read(struct platform_ui_pointer *pointer);
void platform_video_window_size(int *width, int *height);
BOOL platform_next_keystroke(struct platform_keystroke *keystroke);

#endif
