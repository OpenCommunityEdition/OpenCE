/*
GUEST_HOST_MACOS.H

The desktop services the macOS host adds to the Android port's
(port/android/guest/runtime/guest_host.h), for the platform layer's desktop
code (port/linux/src/sdl_platform.c): the window's sizes and fullscreen
modes, the displays, a small software-drawn window for the first start's
data extraction, message boxes and the file dialog. Every name here is also
listed in port/macos/host_imports.list; the host defines them in
port/macos/host/host_sdl.c.

The parameter types follow guest_host.h's rules. SDL objects are small
integer handles here too.
*/

#ifndef __GUEST_HOST_MACOS_H
#define __GUEST_HOST_MACOS_H

#include "halo_macos_abi.h"

long long host_sdl_ticks_ns(void);
void host_sdl_pump_events(void);
/* pushes a copy of the 128-byte SDL_Event (one without pointers) */
int host_sdl_push_event(const void *event);

/* ---------- windows */

void host_sdl_destroy_window(unsigned int window);
void host_sdl_window_size(unsigned int window, int *width, int *height);
int host_sdl_set_window_size(unsigned int window, int width, int height);
long long host_sdl_window_flags(unsigned int window);
int host_sdl_set_window_fullscreen(unsigned int window, int fullscreen);
/* the mode, a fullscreen mode of its display; NULL for borderless */
int host_sdl_set_window_fullscreen_mode(unsigned int window, const struct halo_macos_display_mode *mode);
void host_sdl_warp_mouse(unsigned int window, float x, float y);

/* ---------- displays */

unsigned int host_sdl_primary_display(void);
unsigned int host_sdl_display_for_window(unsigned int window);
/* the display's desktop mode (which 0) or current mode (1); 1 on success */
int host_sdl_display_mode(unsigned int display, int which, struct halo_macos_display_mode *mode);
/* the display's fullscreen modes, at most capacity of them; how many */
int host_sdl_fullscreen_display_modes(unsigned int display, struct halo_macos_display_mode *modes, int capacity);
/* SDL_GetClosestFullscreenDisplayMode; 1 on success */
int host_sdl_closest_display_mode(unsigned int display, int width, int height, float refresh_rate,
	int include_high_density, struct halo_macos_display_mode *mode);
/* x, y, width and height; 1 on success */
int host_sdl_display_usable_bounds(unsigned int display, int *rectangle);

/* ---------- a software renderer (the data extraction's window) */

unsigned int host_sdl_create_renderer(unsigned int window);
void host_sdl_destroy_renderer(unsigned int renderer);
int host_sdl_set_render_vsync(unsigned int renderer, int vsync);
int host_sdl_set_render_draw_color(unsigned int renderer, int red, int green, int blue, int alpha);
int host_sdl_set_render_scale(unsigned int renderer, float x, float y);
int host_sdl_render_clear(unsigned int renderer);
int host_sdl_render_fill_rect(unsigned int renderer, float x, float y, float width, float height);
int host_sdl_render_debug_text(unsigned int renderer, float x, float y, const char *text);
int host_sdl_render_present(unsigned int renderer);

/* ---------- dialogs */

/* SDL_ShowMessageBox: the button chosen in answer; 1 on success */
int host_sdl_show_message_box(unsigned int flags, unsigned int window, const char *title, const char *message,
	int button_count, const struct halo_macos_message_box_button *buttons, int *answer);
/* the system's open panel, for one file matching the patterns
(semicolon-separated extensions, "*" for any) under the filter's name;
waits for the player's choice, copied into path. 1 if they chose a file */
int host_sdl_choose_file(const char *filter_name, const char *patterns, char *path, unsigned int size);

/* ---------- the process */

/* the folder the game treats as the executable's (SDL_GetBasePath: its
config.toml, and its data unless they are elsewhere), with a final '/' */
void host_macos_base_path(char *buffer, unsigned int size);

#endif
