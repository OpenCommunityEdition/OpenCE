/*
HOST_SDL.C

SDL3 on behalf of the macOS guest (port/android/guest/runtime/guest_sdl.c),
after the Android host's (port/android/host/host_sdl.c), plus the calls of
the platform layer's desktop branches, which the macOS guest takes
(port/macos/host_imports_macos.list). SDL objects are 64-bit pointers, which
the guest cannot hold; it gets small handles into the table here instead.

The guest runs on the process's main thread (host_main.c), with its stack
in guest memory, so these run there too, as Cocoa wants of windows, events
and dialogs. SDL's audio thread is the exception: it has no guest stack,
so the audio callback is handed to a thread that has one.
*/

#include "host.h"
#include "host_services.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <pthread/qos.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* The guest's SDL_Event is SDL's, with 4-byte pointers: the events
without pointers, which are all the platform layer reads, have the same
layout as here, and are copied whole (guest_sdl.c asserts the same). */
_Static_assert(sizeof(SDL_Event) == 128, "SDL_Event");
_Static_assert(offsetof(SDL_Event, key.scancode) == 24 && offsetof(SDL_Event, key.down) == 36 &&
	offsetof(SDL_Event, key.repeat) == 37, "SDL_KeyboardEvent");
_Static_assert(offsetof(SDL_Event, button.button) == 24 && offsetof(SDL_Event, button.down) == 25 &&
	offsetof(SDL_Event, button.x) == 28 && offsetof(SDL_Event, button.y) == 32, "SDL_MouseButtonEvent");
_Static_assert(offsetof(SDL_Event, motion.x) == 28 && offsetof(SDL_Event, motion.y) == 32 &&
	offsetof(SDL_Event, motion.xrel) == 36 && offsetof(SDL_Event, motion.yrel) == 40, "SDL_MouseMotionEvent");
_Static_assert(offsetof(SDL_Event, wheel.y) == 28 && offsetof(SDL_Event, gdevice.which) == 16 &&
	offsetof(SDL_Event, window.windowID) == 16, "SDL_Event");
/* SDL_AudioSpec is all 32-bit values: the guest's is the same */
_Static_assert(sizeof(SDL_AudioSpec) == 12, "SDL_AudioSpec");
_Static_assert(sizeof(SDL_Rect) == 16, "SDL_Rect");

#define HANDLE_COUNT 256

enum handle_type
{
	_handle_free,
	_handle_window,
	_handle_context,
	_handle_gamepad,
	_handle_audio,
};

struct handle
{
	int type;
	void *object;
};

static struct handle handles[HANDLE_COUNT];
static pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t handle_new(int type, void *object)
{
	uint32_t index;

	if (!object)
		return 0;
	pthread_mutex_lock(&handle_lock);
	/* an object that already has a handle keeps it */
	for (index = 1; index < HANDLE_COUNT; index++)
	{
		if (handles[index].type == type && handles[index].object == object)
		{
			pthread_mutex_unlock(&handle_lock);
			return index;
		}
	}
	for (index = 1; index < HANDLE_COUNT; index++)
	{
		if (handles[index].type == _handle_free)
		{
			handles[index].type = type;
			handles[index].object = object;
			pthread_mutex_unlock(&handle_lock);
			return index;
		}
	}
	pthread_mutex_unlock(&handle_lock);
	host_logf(HOST_LOG_ERROR, "out of SDL handles");
	return 0;
}

static void *handle_get(uint32_t handle, int type)
{
	void *object = NULL;

	if (handle == 0 || handle >= HANDLE_COUNT)
		return NULL;
	pthread_mutex_lock(&handle_lock);
	if (handles[handle].type == type)
		object = handles[handle].object;
	pthread_mutex_unlock(&handle_lock);
	return object;
}

/* ---------- general */

int host_sdl_init(uint32_t flags)
{
	return SDL_Init((SDL_InitFlags)flags);
}

int host_sdl_set_hint(const char *name, const char *value)
{
	return SDL_SetHint(name, value);
}

void host_sdl_get_error(char *buffer, uint32_t size)
{
	SDL_strlcpy(buffer, SDL_GetError(), size);
}

void host_sdl_scancode_name(int32_t scancode, char *buffer, uint32_t size)
{
	SDL_strlcpy(buffer, SDL_GetScancodeName((SDL_Scancode)scancode), size);
}

int32_t host_sdl_scancode_from_name(const char *name)
{
	return (int32_t)SDL_GetScancodeFromName(name);
}

int64_t host_sdl_ticks(void)
{
	return (int64_t)SDL_GetTicks();
}

/* (the frame pacing's clock, sdl_platform.c: SDL's own, as SDL_GetTicks
is, rather than the guest's clock_gettime) */
int64_t host_sdl_ticks_ns(void)
{
	return (int64_t)SDL_GetTicksNS();
}

void host_sdl_delay_precise(int64_t nanoseconds)
{
	if (nanoseconds > 0)
		SDL_DelayPrecise((Uint64)nanoseconds);
}

int64_t host_sdl_thread_id(void)
{
	return (int64_t)SDL_GetCurrentThreadID();
}

/* ---------- video */

uint32_t host_sdl_create_window(const char *title, int width, int height, int64_t flags)
{
	return handle_new(_handle_window, SDL_CreateWindow(title, width, height, (SDL_WindowFlags)flags));
}

void host_sdl_window_size_in_pixels(uint32_t window, int *width, int *height)
{
	SDL_Window *object = handle_get(window, _handle_window);

	*width = 0;
	*height = 0;
	if (object)
		SDL_GetWindowSizeInPixels(object, width, height);
}

int host_sdl_window_size(uint32_t window, int *width, int *height)
{
	SDL_Window *object = handle_get(window, _handle_window);

	*width = 0;
	*height = 0;
	return object ? SDL_GetWindowSize(object, width, height) : 0;
}

int host_sdl_set_window_size(uint32_t window, int width, int height)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_SetWindowSize(object, width, height) : 0;
}

int64_t host_sdl_window_flags(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? (int64_t)SDL_GetWindowFlags(object) : 0;
}

int host_sdl_set_window_fullscreen(uint32_t window, int fullscreen)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_SetWindowFullscreen(object, fullscreen != 0) : 0;
}

int host_sdl_set_relative_mouse(uint32_t window, int enabled)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_SetWindowRelativeMouseMode(object, enabled != 0) : 0;
}

void host_sdl_warp_mouse_in_window(uint32_t window, float x, float y)
{
	SDL_Window *object = handle_get(window, _handle_window);

	if (object)
		SDL_WarpMouseInWindow(object, x, y);
}

int host_sdl_gl_set_attribute(int attribute, int value)
{
	return SDL_GL_SetAttribute((SDL_GLAttr)attribute, value);
}

uint32_t host_sdl_gl_create_context(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? handle_new(_handle_context, SDL_GL_CreateContext(object)) : 0;
}

int host_sdl_gl_make_current(uint32_t window, uint32_t context)
{
	return SDL_GL_MakeCurrent(handle_get(window, _handle_window), handle_get(context, _handle_context));
}

int host_sdl_gl_set_swap_interval(int interval)
{
	return SDL_GL_SetSwapInterval(interval);
}

int host_sdl_gl_swap_window(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_GL_SwapWindow(object) : 0;
}

/* ---------- displays (the desktop's video settings, sdl_platform.c)

A mode crosses as SDL_DisplayMode without the driver's pointer that ends
it (guest_host.h's struct host_sdl_display_mode). SDL finds the mode a
window is to be set to by its other fields, so a mode the guest passes back
needs no pointer. */

struct host_sdl_display_mode
{
	uint32_t display;
	uint32_t format;
	int32_t w;
	int32_t h;
	float pixel_density;
	float refresh_rate;
	int32_t refresh_rate_numerator;
	int32_t refresh_rate_denominator;
};

_Static_assert(sizeof(struct host_sdl_display_mode) == 32, "host_sdl_display_mode");

static void display_mode_to_guest(struct host_sdl_display_mode *guest, const SDL_DisplayMode *mode)
{
	guest->display = mode->displayID;
	guest->format = (uint32_t)mode->format;
	guest->w = mode->w;
	guest->h = mode->h;
	guest->pixel_density = mode->pixel_density;
	guest->refresh_rate = mode->refresh_rate;
	guest->refresh_rate_numerator = mode->refresh_rate_numerator;
	guest->refresh_rate_denominator = mode->refresh_rate_denominator;
}

static void display_mode_from_guest(SDL_DisplayMode *mode, const struct host_sdl_display_mode *guest)
{
	memset(mode, 0, sizeof(*mode));
	mode->displayID = guest->display;
	mode->format = (SDL_PixelFormat)guest->format;
	mode->w = guest->w;
	mode->h = guest->h;
	mode->pixel_density = guest->pixel_density;
	mode->refresh_rate = guest->refresh_rate;
	mode->refresh_rate_numerator = guest->refresh_rate_numerator;
	mode->refresh_rate_denominator = guest->refresh_rate_denominator;
}

uint32_t host_sdl_primary_display(void)
{
	return (uint32_t)SDL_GetPrimaryDisplay();
}

uint32_t host_sdl_display_for_window(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? (uint32_t)SDL_GetDisplayForWindow(object) : 0;
}

int host_sdl_display_usable_bounds(uint32_t display, SDL_Rect *rect)
{
	return SDL_GetDisplayUsableBounds((SDL_DisplayID)display, rect);
}

int host_sdl_display_mode(uint32_t display, int which, struct host_sdl_display_mode *mode)
{
	const SDL_DisplayMode *found = which ? SDL_GetCurrentDisplayMode((SDL_DisplayID)display) :
		SDL_GetDesktopDisplayMode((SDL_DisplayID)display);

	if (!found)
		return 0;
	display_mode_to_guest(mode, found);
	return 1;
}

int host_sdl_fullscreen_display_modes(uint32_t display, struct host_sdl_display_mode *modes, int capacity)
{
	int count = 0, index;
	SDL_DisplayMode **found = SDL_GetFullscreenDisplayModes((SDL_DisplayID)display, &count);

	if (!found)
		return -1;
	for (index = 0; index < count && index < capacity; index++)
		display_mode_to_guest(&modes[index], found[index]);
	SDL_free(found);
	return count;
}

int host_sdl_closest_fullscreen_display_mode(uint32_t display, int width, int height, float refresh_rate,
	int include_high_density, struct host_sdl_display_mode *mode)
{
	SDL_DisplayMode closest;

	if (!SDL_GetClosestFullscreenDisplayMode((SDL_DisplayID)display, width, height, refresh_rate,
		include_high_density != 0, &closest))
	{
		return 0;
	}
	display_mode_to_guest(mode, &closest);
	return 1;
}

int host_sdl_set_window_fullscreen_mode(uint32_t window, const struct host_sdl_display_mode *mode)
{
	SDL_Window *object = handle_get(window, _handle_window);
	SDL_DisplayMode host_mode;

	if (!object)
		return 0;
	if (!mode)
		return SDL_SetWindowFullscreenMode(object, NULL);
	display_mode_from_guest(&host_mode, mode);
	return SDL_SetWindowFullscreenMode(object, &host_mode);
}

/* ---------- events */

/* writes a link macOS opened the app with where the game picks it up
(p2p.c's poll_invite_file, as the Android app's launcher does): whole under
another name first, so that the game never reads half of it */
static void write_join_link(const char *link)
{
	const char *root = host_launcher_data_root();
	char partial[1100], path[1100];
	FILE *file;
	int written;

	if (!*root)
		return;
	snprintf(partial, sizeof(partial), "%s/join_link.txt.tmp", root);
	snprintf(path, sizeof(path), "%s/join_link.txt", root);
	file = fopen(partial, "wb");
	if (!file)
	{
		host_logf(HOST_LOG_ERROR, "cannot write %s", partial);
		return;
	}
	written = fputs(link, file) >= 0;
	if (fclose(file) != 0 || !written || rename(partial, path) != 0)
	{
		host_logf(HOST_LOG_ERROR, "cannot write %s", path);
		remove(partial);
		return;
	}
	host_logf(HOST_LOG_INFO, "passed on a link the app was opened with");
}

int host_sdl_take_event(const SDL_Event *event)
{
	switch (event->type)
	{
	/* SDL turns the link macOS opens the app with (an Apple event) into a
	dropped file; a link's text dropped on the window counts too. Only an
	invite (halo://) is passed on: Discord's (discord-<application id>://)
	only starts the game, which then hears of the invite from Discord
	itself (p2p_discord.c) */
	case SDL_EVENT_DROP_FILE:
	case SDL_EVENT_DROP_TEXT:
		if (event->drop.data && !SDL_strncasecmp(event->drop.data, "halo://", 7))
			write_join_link(event->drop.data);
		return 1;
	/* (the guest reads no drops: their pointers are the host's) */
	case SDL_EVENT_DROP_BEGIN:
	case SDL_EVENT_DROP_COMPLETE:
	case SDL_EVENT_DROP_POSITION:
		return 1;
	default:
		return 0;
	}
}

int host_sdl_poll_event(void *event)
{
	SDL_Event host_event;

	do
	{
		if (!SDL_PollEvent(&host_event))
			return 0;
	} while (host_sdl_take_event(&host_event));
	/* the layouts agree except for the pointers of text, drop and user
	events, which the guest does not read */
	memcpy(event, &host_event, sizeof(host_event));
	return 1;
}

void host_sdl_pump_events(void)
{
	SDL_PumpEvents();
}

/* the guest pushes a quit (platform_request_quit, the menus' Quit); no
event with pointers in it, which would be the guest's */
int host_sdl_push_event(const void *event)
{
	SDL_Event host_event;

	memcpy(&host_event, event, sizeof(host_event));
	switch (host_event.type)
	{
	case SDL_EVENT_TEXT_EDITING:
	case SDL_EVENT_TEXT_INPUT:
	case SDL_EVENT_TEXT_EDITING_CANDIDATES:
	case SDL_EVENT_DROP_FILE:
	case SDL_EVENT_DROP_TEXT:
	case SDL_EVENT_DROP_BEGIN:
	case SDL_EVENT_DROP_COMPLETE:
	case SDL_EVENT_DROP_POSITION:
	case SDL_EVENT_CLIPBOARD_UPDATE:
		return SDL_SetError("the guest's event %u has pointers", (unsigned)host_event.type);
	default:
		if (host_event.type >= SDL_EVENT_USER)
			return SDL_SetError("the guest's event %u has pointers", (unsigned)host_event.type);
		return SDL_PushEvent(&host_event);
	}
}

/* ---------- gamepads */

int host_sdl_get_gamepads(uint32_t *ids, int capacity)
{
	int count = 0, index;
	SDL_JoystickID *list = SDL_GetGamepads(&count);

	if (!list)
		return 0;
	if (count > capacity)
		count = capacity;
	for (index = 0; index < count; index++)
		ids[index] = list[index];
	SDL_free(list);
	return count;
}

uint32_t host_sdl_open_gamepad(uint32_t id)
{
	SDL_Gamepad *gamepad = SDL_OpenGamepad((SDL_JoystickID)id);

	if (gamepad)
		host_logf(HOST_LOG_INFO, "gamepad %u: %s (type %d, %04x:%04x)", (unsigned)id, SDL_GetGamepadName(gamepad),
			(int)SDL_GetGamepadType(gamepad), SDL_GetGamepadVendor(gamepad), SDL_GetGamepadProduct(gamepad));
	return handle_new(_handle_gamepad, gamepad);
}

uint32_t host_sdl_gamepad_from_id(uint32_t id)
{
	return handle_new(_handle_gamepad, SDL_GetGamepadFromID((SDL_JoystickID)id));
}

int host_sdl_gamepad_axis(uint32_t gamepad, int axis)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return object ? SDL_GetGamepadAxis(object, (SDL_GamepadAxis)axis) : 0;
}

int host_sdl_gamepad_button(uint32_t gamepad, int button)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return object ? SDL_GetGamepadButton(object, (SDL_GamepadButton)button) : 0;
}

int host_sdl_gamepad_type(uint32_t gamepad)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return object ? SDL_GetGamepadType(object) : SDL_GAMEPAD_TYPE_UNKNOWN;
}

int host_sdl_rumble_gamepad(uint32_t gamepad, uint32_t low, uint32_t high, uint32_t milliseconds)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return object ? SDL_RumbleGamepad(object, (Uint16)low, (Uint16)high, milliseconds) : 0;
}

/* ---------- audio */

/* SDL calls audio_callback on its own audio thread (CoreAudio's), which
cannot run guest code; it passes each request to the stream's thread
(audio_thread), which can, and waits for it to be done. SDL holds the
stream's lock throughout, so the audio the guest puts into the stream
meanwhile (from audio_thread) is kept in the binding's buffer instead, and
put in by audio_callback once the guest is done: audio_thread putting it in
itself would wait for the lock forever */
struct audio_binding
{
	uint32_t handle;
	uint32_t callback;
	uint32_t userdata;
	pthread_mutex_t lock;
	pthread_cond_t requested;
	pthread_cond_t done;
	int pending;
	int additional;
	int total;
	unsigned char *buffer;
	int buffer_length;
	int buffer_size;
};

/* the binding whose callback this thread is running, if any */
static __thread struct audio_binding *calling_back;

static void *audio_thread(void *context)
{
	struct audio_binding *binding = context;

	/* CoreAudio's thread is a real-time one, and waits on this one for each
	buffer: at a lower priority, other work would make it late (a glitch) */
	pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
	pthread_mutex_lock(&binding->lock);
	for (;;)
	{
		int additional, total;

		while (!binding->pending)
			pthread_cond_wait(&binding->requested, &binding->lock);
		additional = binding->additional;
		total = binding->total;
		pthread_mutex_unlock(&binding->lock);
		calling_back = binding;
		host_call_guest(binding->callback, binding->userdata, binding->handle, (uint32_t)additional, (uint32_t)total);
		calling_back = NULL;
		pthread_mutex_lock(&binding->lock);
		binding->pending = 0;
		pthread_cond_signal(&binding->done);
	}
	return NULL;
}

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional, int total)
{
	struct audio_binding *binding = userdata;

	pthread_mutex_lock(&binding->lock);
	binding->additional = additional;
	binding->total = total;
	binding->pending = 1;
	pthread_cond_signal(&binding->requested);
	while (binding->pending)
		pthread_cond_wait(&binding->done, &binding->lock);
	pthread_mutex_unlock(&binding->lock);
	if (binding->buffer_length)
	{
		SDL_PutAudioStreamData(stream, binding->buffer, binding->buffer_length);
		binding->buffer_length = 0;
	}
}

/* audio the guest puts into its stream during the stream's callback, for
audio_callback to put in; 1 on success */
static int audio_keep(struct audio_binding *binding, const void *data, int length)
{
	if (length < 0)
		return 0;
	if (binding->buffer_length + length > binding->buffer_size)
	{
		int size = (binding->buffer_length + length) * 2;
		unsigned char *buffer = SDL_realloc(binding->buffer, (size_t)size);

		if (!buffer)
			return 0;
		binding->buffer = buffer;
		binding->buffer_size = size;
	}
	memcpy(binding->buffer + binding->buffer_length, data, (size_t)length);
	binding->buffer_length += length;
	return 1;
}

uint32_t host_sdl_open_audio_stream(uint32_t device, const void *spec, uint32_t callback, uint32_t userdata)
{
	struct audio_binding *binding = SDL_calloc(1, sizeof(*binding));
	SDL_AudioStream *stream;

	if (!binding)
		return 0;
	binding->callback = callback;
	binding->userdata = userdata;
	pthread_mutex_init(&binding->lock, NULL);
	pthread_cond_init(&binding->requested, NULL);
	pthread_cond_init(&binding->done, NULL);
	stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)device, spec,
		callback ? audio_callback : NULL, binding);
	if (!stream)
	{
		SDL_free(binding);
		return 0;
	}
	/* the device starts paused, so no callback can run before this */
	binding->handle = handle_new(_handle_audio, stream);
	if (callback && host_native_thread_create(audio_thread, binding, 256 * 1024) != 0)
		host_fatal("cannot start the audio thread");
	return binding->handle;
}

int host_sdl_put_audio_stream_data(uint32_t stream, const void *data, int length)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	if (!object)
		return 0;
	if (calling_back && calling_back->handle == stream)
		return audio_keep(calling_back, data, length);
	return SDL_PutAudioStreamData(object, data, length);
}

int host_sdl_resume_audio_stream_device(uint32_t stream)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	return object ? SDL_ResumeAudioStreamDevice(object) : 0;
}

/* ---------- the clipboard (internet play's invite links) */

int host_sdl_set_clipboard_text(const char *text)
{
	return SDL_SetClipboardText(text) ? 1 : 0;
}

void host_sdl_get_clipboard_text(char *buffer, uint32_t size)
{
	char *text = SDL_GetClipboardText();

	SDL_strlcpy(buffer, text ? text : "", size);
	SDL_free(text);
}

/* (an Android import, which the macOS guest has too but never calls: macOS
has no toasts) */
int host_sdl_show_toast(const char *message, int duration, int gravity, int x, int y)
{
	(void)message;
	(void)duration;
	(void)gravity;
	(void)x;
	(void)y;
	return 0;
}

/* ---------- messages for the player */

int host_sdl_show_simple_message_box(uint32_t flags, const char *title, const char *message)
{
	/* (none in a run nobody watches: it would wait for ever) */
	if (host_unattended(host_launcher_data_root()))
		return 0;
	return SDL_ShowSimpleMessageBox((SDL_MessageBoxFlags)flags, title, message, NULL) ? 1 : 0;
}

/* guest_host.h's: a button, whose text is a guest address */
struct host_sdl_message_box_button
{
	uint32_t flags;
	int32_t id;
	uint32_t text;
};

int host_sdl_show_message_box(uint32_t flags, const char *title, const char *message, int button_count,
	const struct host_sdl_message_box_button *buttons, int *answer)
{
	SDL_MessageBoxButtonData host_buttons[16];
	SDL_MessageBoxData data;
	int chosen = -1;
	int index;

	if (host_unattended(host_launcher_data_root()))
		return 0;
	if (button_count < 0)
		button_count = 0;
	if (button_count > (int)(sizeof(host_buttons) / sizeof(*host_buttons)))
		button_count = (int)(sizeof(host_buttons) / sizeof(*host_buttons));
	for (index = 0; index < button_count; index++)
	{
		host_buttons[index].flags = (SDL_MessageBoxButtonFlags)buttons[index].flags;
		host_buttons[index].buttonID = buttons[index].id;
		host_buttons[index].text = (const char *)(uintptr_t)buttons[index].text;
	}
	memset(&data, 0, sizeof(data));
	data.flags = (SDL_MessageBoxFlags)flags;
	data.title = title;
	data.message = message;
	data.numbuttons = button_count;
	data.buttons = host_buttons;
	if (!SDL_ShowMessageBox(&data, &chosen))
		return 0;
	if (answer)
		*answer = chosen;
	return 1;
}
