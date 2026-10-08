/*
GUEST_SDL.C

The SDL3 functions the platform layer calls (sdl_platform.c, xinput_sdl.c,
dsound_sdl.c), for the guest. SDL itself runs in the host; objects it
returns (windows, contexts, gamepads, audio streams) are 64-bit pointers
there, so the guest only ever sees small integer handles that the host maps
back (host_sdl.c). Events are written by SDL straight into the guest's
buffer: SDL_Event has the same 128-byte layout in both ABIs for every event
type that carries no pointer, which covers all the platform layer reads.
*/

/* before SDL: musl's alloca.h must come first */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(HALO_MACOS)
#include <stddef.h>
#endif
#include <SDL3/SDL.h>

#include "guest_host.h"

/* guest/runtime/guest_gl.c (generated) */
SDL_FunctionPointer guest_gl_get_proc_address(const char *name);

bool SDL_Init(SDL_InitFlags flags)
{
	return host_sdl_init(flags) != 0;
}

bool SDL_SetHint(const char *name, const char *value)
{
	return host_sdl_set_hint(name, value) != 0;
}

const char *SDL_GetError(void)
{
	static __thread char buffer[256];

	host_sdl_get_error(buffer, sizeof(buffer));
	return buffer;
}

const char *SDL_GetScancodeName(SDL_Scancode scancode)
{
	static __thread char buffer[64];

	host_sdl_scancode_name((int)scancode, buffer, sizeof(buffer));
	return buffer;
}

SDL_Scancode SDL_GetScancodeFromName(const char *name)
{
	return (SDL_Scancode)host_sdl_scancode_from_name(name);
}

Uint64 SDL_GetTicks(void)
{
	return (Uint64)host_sdl_ticks();
}

SDL_ThreadID SDL_GetCurrentThreadID(void)
{
	return (SDL_ThreadID)host_sdl_thread_id();
}

void SDL_free(void *memory)
{
	free(memory);
}

/* ---------- the clipboard (internet play's invite links, sdl_platform.c) */

bool SDL_SetClipboardText(const char *text)
{
	return host_sdl_set_clipboard_text(text) != 0;
}

char *SDL_GetClipboardText(void)
{
	char buffer[1024];

	host_sdl_get_clipboard_text(buffer, sizeof(buffer));
	return strdup(buffer);
}

bool SDL_ShowAndroidToast(const char *message, int duration, int gravity, int xoffset, int yoffset)
{
	return host_sdl_show_toast(message, duration, gravity, xoffset, yoffset) != 0;
}

/* ---------- a message for the player (sdl_platform.c): the host's own window */

bool SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags flags, const char *title, const char *message, SDL_Window *window)
{
	(void)window;
	return host_sdl_show_simple_message_box((unsigned int)flags, title, message) != 0;
}

void SDL_Delay(Uint32 milliseconds)
{
	struct timespec duration;

	duration.tv_sec = milliseconds / 1000;
	duration.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
	nanosleep(&duration, NULL);
}

/* ---------- video */

SDL_Window *SDL_CreateWindow(const char *title, int width, int height, SDL_WindowFlags flags)
{
	return (SDL_Window *)host_sdl_create_window(title, width, height, (long long)flags);
}

bool SDL_GetWindowSizeInPixels(SDL_Window *window, int *width, int *height)
{
	int w = 0, h = 0;

	host_sdl_window_size_in_pixels((unsigned int)window, &w, &h);
	if (width)
		*width = w;
	if (height)
		*height = h;
	return true;
}

bool SDL_SetWindowRelativeMouseMode(SDL_Window *window, bool enabled)
{
	return host_sdl_set_relative_mouse((unsigned int)window, enabled) != 0;
}

bool SDL_GL_SetAttribute(SDL_GLAttr attribute, int value)
{
	return host_sdl_gl_set_attribute((int)attribute, value) != 0;
}

SDL_GLContext SDL_GL_CreateContext(SDL_Window *window)
{
	return (SDL_GLContext)host_sdl_gl_create_context((unsigned int)window);
}

bool SDL_GL_MakeCurrent(SDL_Window *window, SDL_GLContext context)
{
	return host_sdl_gl_make_current((unsigned int)window, (unsigned int)context) != 0;
}

bool SDL_GL_SetSwapInterval(int interval)
{
	return host_sdl_gl_set_swap_interval(interval) != 0;
}

bool SDL_GL_SwapWindow(SDL_Window *window)
{
	return host_sdl_gl_swap_window((unsigned int)window) != 0;
}

SDL_FunctionPointer SDL_GL_GetProcAddress(const char *name)
{
	return guest_gl_get_proc_address(name);
}

/* ---------- events */

bool SDL_PollEvent(SDL_Event *event)
{
	SDL_Event scratch;

	return host_sdl_poll_event(event ? event : &scratch) != 0;
}

/* ---------- gamepads */

SDL_JoystickID *SDL_GetGamepads(int *count)
{
	unsigned int ids[16];
	int found = host_sdl_get_gamepads(ids, 16);
	SDL_JoystickID *result = malloc((found + 1) * sizeof(SDL_JoystickID));
	int index;

	if (!result)
		return NULL;
	for (index = 0; index < found; index++)
		result[index] = ids[index];
	result[found] = 0;
	if (count)
		*count = found;
	return result;
}

SDL_Gamepad *SDL_OpenGamepad(SDL_JoystickID id)
{
	return (SDL_Gamepad *)host_sdl_open_gamepad(id);
}

SDL_Gamepad *SDL_GetGamepadFromID(SDL_JoystickID id)
{
	return (SDL_Gamepad *)host_sdl_gamepad_from_id(id);
}

Sint16 SDL_GetGamepadAxis(SDL_Gamepad *gamepad, SDL_GamepadAxis axis)
{
	return (Sint16)host_sdl_gamepad_axis((unsigned int)gamepad, (int)axis);
}

bool SDL_GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button)
{
	return host_sdl_gamepad_button((unsigned int)gamepad, (int)button) != 0;
}

SDL_GamepadType SDL_GetGamepadType(SDL_Gamepad *gamepad)
{
	return (SDL_GamepadType)host_sdl_gamepad_type((unsigned int)gamepad);
}

bool SDL_RumbleGamepad(SDL_Gamepad *gamepad, Uint16 low, Uint16 high, Uint32 milliseconds)
{
	return host_sdl_rumble_gamepad((unsigned int)gamepad, low, high, milliseconds) != 0;
}

/* ---------- audio */

SDL_AudioStream *SDL_OpenAudioDeviceStream(SDL_AudioDeviceID device, const SDL_AudioSpec *spec,
	SDL_AudioStreamCallback callback, void *userdata)
{
	return (SDL_AudioStream *)host_sdl_open_audio_stream(device, spec, (unsigned int)callback,
		(unsigned int)userdata);
}

bool SDL_PutAudioStreamData(SDL_AudioStream *stream, const void *data, int length)
{
	return host_sdl_put_audio_stream_data((unsigned int)stream, data, length) != 0;
}

bool SDL_ResumeAudioStreamDevice(SDL_AudioStream *stream)
{
	return host_sdl_resume_audio_stream_device((unsigned int)stream) != 0;
}
#if defined(HALO_MACOS)

/* ---------- the desktop branches (macOS)

The macOS guest takes the Linux build's desktop branches of sdl_platform.c
(but for its data offer, which the host makes), which call more of SDL than
Android's: window, display and frame pacing calls, which go to the host
(port/macos/host/host_sdl.c). Its files are read with musl's stdio, as
Android's are (port_config.c, menu_files.c), not SDL's. */

/* the events the platform layer reads, where the host copies them from
(host_sdl.c asserts the same of its SDL_Event) */
_Static_assert(sizeof(SDL_Event) == 128, "SDL_Event");
_Static_assert(offsetof(SDL_Event, key.scancode) == 24 && offsetof(SDL_Event, key.down) == 36 &&
	offsetof(SDL_Event, key.repeat) == 37, "SDL_KeyboardEvent");
_Static_assert(offsetof(SDL_Event, button.button) == 24 && offsetof(SDL_Event, button.down) == 25 &&
	offsetof(SDL_Event, button.x) == 28 && offsetof(SDL_Event, button.y) == 32, "SDL_MouseButtonEvent");
_Static_assert(offsetof(SDL_Event, motion.x) == 28 && offsetof(SDL_Event, motion.y) == 32 &&
	offsetof(SDL_Event, motion.xrel) == 36 && offsetof(SDL_Event, motion.yrel) == 40, "SDL_MouseMotionEvent");
_Static_assert(offsetof(SDL_Event, wheel.y) == 28 && offsetof(SDL_Event, gdevice.which) == 16 &&
	offsetof(SDL_Event, window.windowID) == 16, "SDL_Event");

void *SDL_malloc(size_t size)
{
	return malloc(size);
}

void *SDL_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}

void *SDL_realloc(void *memory, size_t size)
{
	return realloc(memory, size);
}

Uint64 SDL_GetTicksNS(void)
{
	return (Uint64)host_sdl_ticks_ns();
}

void SDL_DelayPrecise(Uint64 nanoseconds)
{
	host_sdl_delay_precise((long long)nanoseconds);
}

void SDL_PumpEvents(void)
{
	host_sdl_pump_events();
}

bool SDL_PushEvent(SDL_Event *event)
{
	return event && host_sdl_push_event(event) != 0;
}

bool SDL_ShowMessageBox(const SDL_MessageBoxData *data, int *button)
{
	struct host_sdl_message_box_button buttons[16];
	int count = data && data->numbuttons > 0 ? data->numbuttons : 0;
	int answer = -1;
	int index;

	if (!data)
		return false;
	if (count > (int)(sizeof(buttons) / sizeof(*buttons)))
		count = (int)(sizeof(buttons) / sizeof(*buttons));
	for (index = 0; index < count; index++)
	{
		buttons[index].flags = data->buttons[index].flags;
		buttons[index].id = data->buttons[index].buttonID;
		buttons[index].text = (unsigned int)data->buttons[index].text;
	}
	/* (in the host's own window: data->window is a handle, and Cocoa puts
	the box in front anyway) */
	if (!host_sdl_show_message_box(data->flags, data->title ? data->title : "",
		data->message ? data->message : "", count, buttons, &answer))
	{
		return false;
	}
	if (button)
		*button = answer;
	return true;
}

/* ---------- windows */

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window *window)
{
	return (SDL_WindowFlags)host_sdl_window_flags((unsigned int)window);
}

bool SDL_GetWindowSize(SDL_Window *window, int *width, int *height)
{
	int w = 0, h = 0;
	int result = host_sdl_window_size((unsigned int)window, &w, &h);

	if (width)
		*width = w;
	if (height)
		*height = h;
	return result != 0;
}

bool SDL_SetWindowSize(SDL_Window *window, int width, int height)
{
	return host_sdl_set_window_size((unsigned int)window, width, height) != 0;
}

bool SDL_SetWindowFullscreen(SDL_Window *window, bool fullscreen)
{
	return host_sdl_set_window_fullscreen((unsigned int)window, fullscreen) != 0;
}

void SDL_WarpMouseInWindow(SDL_Window *window, float x, float y)
{
	host_sdl_warp_mouse_in_window((unsigned int)window, x, y);
}

/* ---------- displays and their modes */

static void display_mode_from_host(SDL_DisplayMode *mode, const struct host_sdl_display_mode *host)
{
	memset(mode, 0, sizeof(*mode));
	mode->displayID = host->display;
	mode->format = (SDL_PixelFormat)host->format;
	mode->w = host->w;
	mode->h = host->h;
	mode->pixel_density = host->pixel_density;
	mode->refresh_rate = host->refresh_rate;
	mode->refresh_rate_numerator = host->refresh_rate_numerator;
	mode->refresh_rate_denominator = host->refresh_rate_denominator;
}

static void display_mode_to_host(struct host_sdl_display_mode *host, const SDL_DisplayMode *mode)
{
	host->display = mode->displayID;
	host->format = (unsigned int)mode->format;
	host->w = mode->w;
	host->h = mode->h;
	host->pixel_density = mode->pixel_density;
	host->refresh_rate = mode->refresh_rate;
	host->refresh_rate_numerator = mode->refresh_rate_numerator;
	host->refresh_rate_denominator = mode->refresh_rate_denominator;
}

SDL_DisplayID SDL_GetPrimaryDisplay(void)
{
	return (SDL_DisplayID)host_sdl_primary_display();
}

SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window *window)
{
	return (SDL_DisplayID)host_sdl_display_for_window((unsigned int)window);
}

bool SDL_GetDisplayUsableBounds(SDL_DisplayID display, SDL_Rect *rect)
{
	SDL_Rect bounds = { 0, 0, 0, 0 };
	int result = host_sdl_display_usable_bounds(display, &bounds);

	if (rect)
		*rect = bounds;
	return result != 0;
}

/* SDL's own copies of a display's desktop and current modes stay put while
the modes do; these are kept the same way, one per display and kind (the
video calls are the main thread's) */
#define DISPLAY_MODE_SLOTS 16

static struct
{
	SDL_DisplayID display;
	int which;
	SDL_DisplayMode mode;
} display_modes[DISPLAY_MODE_SLOTS];

static const SDL_DisplayMode *display_mode(SDL_DisplayID display, int which)
{
	struct host_sdl_display_mode host;
	static int next;
	int slot;

	if (!host_sdl_display_mode(display, which, &host))
		return NULL;
	for (slot = 0; slot < DISPLAY_MODE_SLOTS; slot++)
	{
		if (display_modes[slot].display == display && display_modes[slot].which == which)
			break;
	}
	if (slot == DISPLAY_MODE_SLOTS)
	{
		slot = next;
		next = (next + 1) % DISPLAY_MODE_SLOTS;
		display_modes[slot].display = display;
		display_modes[slot].which = which;
	}
	display_mode_from_host(&display_modes[slot].mode, &host);
	return &display_modes[slot].mode;
}

const SDL_DisplayMode *SDL_GetDesktopDisplayMode(SDL_DisplayID display)
{
	return display_mode(display, 0);
}

const SDL_DisplayMode *SDL_GetCurrentDisplayMode(SDL_DisplayID display)
{
	return display_mode(display, 1);
}

/* one allocation, as SDL's: the pointers, then the modes they point to */
SDL_DisplayMode **SDL_GetFullscreenDisplayModes(SDL_DisplayID display, int *count)
{
	struct host_sdl_display_mode host[128];
	int found = host_sdl_fullscreen_display_modes(display, host, (int)(sizeof(host) / sizeof(*host)));
	SDL_DisplayMode **result;
	SDL_DisplayMode *modes;
	int index;

	if (count)
		*count = 0;
	if (found < 0)
		return NULL;
	if (found > (int)(sizeof(host) / sizeof(*host)))
		found = (int)(sizeof(host) / sizeof(*host));
	result = malloc((size_t)(found + 1) * sizeof(*result) + (size_t)found * sizeof(*modes));
	if (!result)
		return NULL;
	modes = (SDL_DisplayMode *)(result + found + 1);
	for (index = 0; index < found; index++)
	{
		display_mode_from_host(&modes[index], &host[index]);
		result[index] = &modes[index];
	}
	result[found] = NULL;
	if (count)
		*count = found;
	return result;
}

bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID display, int width, int height, float refresh_rate,
	bool include_high_density_modes, SDL_DisplayMode *closest)
{
	struct host_sdl_display_mode host;

	if (!closest || !host_sdl_closest_fullscreen_display_mode(display, width, height, refresh_rate,
		include_high_density_modes, &host))
	{
		return false;
	}
	display_mode_from_host(closest, &host);
	return true;
}

bool SDL_SetWindowFullscreenMode(SDL_Window *window, const SDL_DisplayMode *mode)
{
	struct host_sdl_display_mode host;

	if (!mode)
		return host_sdl_set_window_fullscreen_mode((unsigned int)window, NULL) != 0;
	display_mode_to_host(&host, mode);
	return host_sdl_set_window_fullscreen_mode((unsigned int)window, &host) != 0;
}
#endif
