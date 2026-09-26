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
void platform_video_drawable_size(int *width, int *height);
void platform_video_swap(void);
/* frames between the 30 Hz ticks at the display's refresh rate, unless
HALO_INTERPOLATION=0 (port/linux/game/render_interpolation.c) */
int halo_interpolation_enabled(void);
void platform_mouse_capture(BOOL capture);

/* main thread only; a no-op elsewhere */
void platform_pump_events(void);
/* a snapshot of the input state; consume_motion resets the mouse deltas */
void platform_input_read(struct platform_input_state *state, BOOL consume_motion);
BOOL platform_next_keystroke(struct platform_keystroke *keystroke);

#endif
