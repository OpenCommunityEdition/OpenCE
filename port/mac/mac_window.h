#ifndef HALO_MAC_WINDOW_H
#define HALO_MAC_WINDOW_H
#include "mac_input.h"

struct mac_window { void *state; };
typedef int (*mac_window_frame_proc)(void *,const struct mac_input_frame *,unsigned,unsigned);

/* Visible Carbon/AGL windows are opt-in. On modern 64-bit macOS only the
   deterministic CGL offscreen mode is available through this legacy host. */
int mac_window_open(struct mac_window *,unsigned width,unsigned height,int visible,const char *title);
/* GPU render targets have fixed dimensions; this variant disables resizing
   for a visible Carbon window while leaving other window callers unchanged. */
int mac_window_open_fixed(struct mac_window *,unsigned width,unsigned height,int visible,const char *title);
int mac_window_frame(struct mac_window *,mac_window_frame_proc,void *context);
int mac_window_run(struct mac_window *,mac_window_frame_proc,void *context);
/* Present the active visible AGL drawable on its owner thread. Hidden CGL
   surfaces are offscreen and cannot be presented to a display. */
int mac_window_present_active(void);
void mac_window_close(struct mac_window *);
int mac_window_is_visible(const struct mac_window *);
int mac_window_is_accelerated(const struct mac_window *);
/* Engine-facing queries/actions use the one active native window. A missing
   window reports dimensions zero; quit is consumed by its owning frame loop. */
void platform_video_window_size(int *width,int *height);
long halo_screen_width(void);
/* The width of the picture the game draws for the window (640 for the Xbox's
   shape, more for a wider window), taken up between frames; the hook is told
   of a change (engine_device_state.c resizes the back buffer). */
long mac_window_screen_commit(void);
void mac_window_screen_width_hook(void (*hook)(unsigned width));
void platform_request_quit(void);
/* True only when the active window was closed by the user or an explicit
   platform_request_quit call. Renderer and callback failures remain false. */
int mac_window_quit_requested_active(void);
/* the seconds the frame rate limiter has waited since this was last asked (a frame's work is its time less this) */
double mac_window_take_limiter_wait(void);
/* Resize the one active window/surface. Returns false where the requested
   dimensions cannot be applied by its native owner. */
int mac_window_resize_active(unsigned width,unsigned height);
int mac_window_set_vsync_active(int on);
/* port: whether the active window is the captured display (the mode the game
   started in); 0 for a window or none. A running game does not switch between
   the two (engine_display.c says so). */
int mac_window_fullscreen_active(void);
/* port: what display.mode asks of a game window: 0 a titled window, 1 a
   borderless window over the whole display, 2 the display captured at a mode
   of its own (display.fullscreen_exclusive, or a display.resolution that is
   not the desktop's) */
int mac_window_display_plan(void);
/* port: moves the one context between a titled window and a borderless one
   over the display, live (the Dock and menu bar hidden while it is in front);
   1 when it is in the mode asked for */
int mac_window_set_borderless_active(int borderless);
/* port: told every frame whether the menus want the pointer (the engine's
   halo_ui_pointer_update); a game window holds the mouse otherwise, until F12,
   or the window is not in front (input.mouse_capture) */
void mac_window_mouse_menus(int menus_active);

#endif
