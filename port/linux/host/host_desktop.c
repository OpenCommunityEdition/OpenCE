/*
HOST_DESKTOP.C

The host's half of what the Linux desktop's code paths need beyond the
Android app's (port/android/host): windows that change size and go
fullscreen, the displays' modes, SDL's 2D renderer (the progress screens of
the first start's disc image copy and of the self-updater, sdl_platform.c
and updater.c), the system's dialogs, and the self-updater's download and
file replacement (port/linux/src/posix_update.c, compiled into the host).
The guest's half is guest_desktop.c; host_imports.list here lists them.

Only integers, floats, strings and flat structures cross, as in host_sdl.c.
*/

#include "host.h"
#include "update.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------- general */

void host_sdl_get_base_path(char *buffer, uint32_t size)
{
	const char *path = SDL_GetBasePath();

	SDL_strlcpy(buffer, path ? path : "", size);
}

int64_t host_sdl_ticks_ns(void)
{
	return (int64_t)SDL_GetTicksNS();
}

void host_sdl_pump_events(void)
{
	SDL_PumpEvents();
}

/* events that carry no pointer (the menus' Quit pushes SDL_EVENT_QUIT) */
int host_sdl_push_event(const void *event)
{
	SDL_Event copy;

	memcpy(&copy, event, sizeof(copy));
	return SDL_PushEvent(&copy) ? 1 : 0;
}

/* ---------- windows and displays */

static SDL_Window *window_of(uint32_t window)
{
	return host_sdl_handle_get(window, _handle_window);
}

int64_t host_sdl_get_window_flags(uint32_t window)
{
	SDL_Window *object = window_of(window);

	return object ? (int64_t)SDL_GetWindowFlags(object) : 0;
}

int host_sdl_set_window_fullscreen(uint32_t window, int fullscreen)
{
	SDL_Window *object = window_of(window);

	return object && SDL_SetWindowFullscreen(object, fullscreen != 0) ? 1 : 0;
}

/* exclusive fullscreen at the display's mode closest to width x height at
refresh_rate, or (width 0) the desktop's resolution */
int host_sdl_set_window_fullscreen_mode(uint32_t window, uint32_t display, int width, int height, float refresh_rate)
{
	SDL_Window *object = window_of(window);
	SDL_DisplayMode mode;

	if (!object)
		return 0;
	if (width <= 0)
		return SDL_SetWindowFullscreenMode(object, NULL) ? 1 : 0;
	if (!SDL_GetClosestFullscreenDisplayMode((SDL_DisplayID)display, width, height, refresh_rate, true, &mode))
		return 0;
	return SDL_SetWindowFullscreenMode(object, &mode) ? 1 : 0;
}

void host_sdl_get_window_size(uint32_t window, int *width, int *height)
{
	SDL_Window *object = window_of(window);

	*width = 0;
	*height = 0;
	if (object)
		SDL_GetWindowSize(object, width, height);
}

int host_sdl_set_window_size(uint32_t window, int width, int height)
{
	SDL_Window *object = window_of(window);

	return object && SDL_SetWindowSize(object, width, height) ? 1 : 0;
}

void host_sdl_destroy_window(uint32_t window)
{
	SDL_Window *object = window_of(window);

	if (!object)
		return;
	host_sdl_handle_free(window);
	SDL_DestroyWindow(object);
}

int host_sdl_warp_mouse_in_window(uint32_t window, float x, float y)
{
	SDL_WarpMouseInWindow(window_of(window), x, y);
	return 1;
}

int host_sdl_set_window_mouse_grab(uint32_t window, int grabbed)
{
	SDL_Window *object = window_of(window);

	return object && SDL_SetWindowMouseGrab(object, grabbed != 0) ? 1 : 0;
}

/* (the window's handle: the one it has, as the guest made it) */
uint32_t host_sdl_get_keyboard_focus(void)
{
	SDL_Window *focus = SDL_GetKeyboardFocus();

	return focus ? host_sdl_handle_new(_handle_window, focus) : 0;
}

int host_sdl_cursor(int shown)
{
	if (shown > 0)
		SDL_ShowCursor();
	else if (shown == 0)
		SDL_HideCursor();
	return SDL_CursorVisible() ? 1 : 0;
}

void host_sdl_get_current_video_driver(char *buffer, uint32_t size)
{
	const char *name = SDL_GetCurrentVideoDriver();

	SDL_strlcpy(buffer, name ? name : "", size);
}

/* a window's property by name (sdl_platform.c's HDR: its booleans and
floats) */
int host_sdl_get_window_property_boolean(uint32_t window, const char *name, int default_value)
{
	SDL_Window *object = window_of(window);
	SDL_PropertiesID properties = object ? SDL_GetWindowProperties(object) : 0;

	return (properties ? SDL_GetBooleanProperty(properties, name, default_value != 0) : default_value != 0) ? 1 : 0;
}

float host_sdl_get_window_property_float(uint32_t window, const char *name, float default_value)
{
	SDL_Window *object = window_of(window);
	SDL_PropertiesID properties = object ? SDL_GetWindowProperties(object) : 0;

	return properties ? SDL_GetFloatProperty(properties, name, default_value) : default_value;
}

/* ---------- the keyboard's layout (the controls' names, xinput_sdl.c) */

uint32_t host_sdl_get_key_from_scancode(int scancode, uint32_t modifiers, int key_event)
{
	if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT)
		return SDLK_UNKNOWN;
	return (uint32_t)SDL_GetKeyFromScancode((SDL_Scancode)scancode, (SDL_Keymod)modifiers, key_event != 0);
}

/* ---------- virtual gamepads (debug.virtual_gamepads, xinput_sdl.c) */

/* name is in guest memory, which lasts as long as the process (SDL keeps
the pointer) */
uint32_t host_sdl_attach_virtual_joystick(int type, int axes, int buttons, uint32_t axis_mask, uint32_t button_mask,
	const char *name)
{
	SDL_VirtualJoystickDesc desc;

	SDL_INIT_INTERFACE(&desc);
	desc.type = (Uint16)type;
	desc.naxes = (Uint16)axes;
	desc.nbuttons = (Uint16)buttons;
	desc.axis_mask = axis_mask;
	desc.button_mask = button_mask;
	desc.name = name;
	return (uint32_t)SDL_AttachVirtualJoystick(&desc);
}

uint32_t host_sdl_open_joystick(uint32_t id)
{
	return host_sdl_handle_new(_handle_joystick, SDL_OpenJoystick((SDL_JoystickID)id));
}

int host_sdl_set_joystick_virtual_button(uint32_t joystick, int button, int down)
{
	SDL_Joystick *object = host_sdl_handle_get(joystick, _handle_joystick);

	return object && SDL_SetJoystickVirtualButton(object, button, down != 0) ? 1 : 0;
}

int host_sdl_set_joystick_virtual_axis(uint32_t joystick, int axis, int value)
{
	SDL_Joystick *object = host_sdl_handle_get(joystick, _handle_joystick);

	return object && SDL_SetJoystickVirtualAxis(object, axis, (Sint16)value) ? 1 : 0;
}

/* ---------- text input (Steam's on-screen keyboard, sdl_platform.c) */

int host_sdl_has_screen_keyboard_support(void)
{
	return SDL_HasScreenKeyboardSupport() ? 1 : 0;
}

int host_sdl_get_hint_boolean(const char *name, int default_value)
{
	return SDL_GetHintBoolean(name, default_value != 0) ? 1 : 0;
}

int host_sdl_text_input_active(uint32_t window)
{
	return SDL_TextInputActive(window_of(window)) ? 1 : 0;
}

/* text input with the two properties the guest sets: one line or more, and
SDL_TEXTINPUT_TYPE_* */
int host_sdl_start_text_input(uint32_t window, int multiline, int type)
{
	SDL_PropertiesID properties = SDL_CreateProperties();
	bool started;

	SDL_SetBooleanProperty(properties, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN, multiline != 0);
	SDL_SetNumberProperty(properties, SDL_PROP_TEXTINPUT_TYPE_NUMBER, type);
	started = SDL_StartTextInputWithProperties(window_of(window), properties);
	SDL_DestroyProperties(properties);
	return started ? 1 : 0;
}

int host_sdl_stop_text_input(uint32_t window)
{
	return SDL_StopTextInput(window_of(window)) ? 1 : 0;
}

uint32_t host_sdl_get_display_for_window(uint32_t window)
{
	SDL_Window *object = window_of(window);

	return object ? (uint32_t)SDL_GetDisplayForWindow(object) : 0;
}

uint32_t host_sdl_get_primary_display(void)
{
	return (uint32_t)SDL_GetPrimaryDisplay();
}

/* a display's desktop (current 0) or current mode: the fields of
SDL_DisplayMode before its pointer, which the guest copies into its own
layout (guest_desktop.c) */
struct host_display_mode
{
	uint32_t display;
	uint32_t format;
	int32_t width, height;
	float pixel_density;
	float refresh_rate;
	int32_t refresh_rate_numerator, refresh_rate_denominator;
};

static void display_mode_copy(struct host_display_mode *out, const SDL_DisplayMode *mode)
{
	out->display = (uint32_t)mode->displayID;
	out->format = (uint32_t)mode->format;
	out->width = mode->w;
	out->height = mode->h;
	out->pixel_density = mode->pixel_density;
	out->refresh_rate = mode->refresh_rate;
	out->refresh_rate_numerator = mode->refresh_rate_numerator;
	out->refresh_rate_denominator = mode->refresh_rate_denominator;
}

int host_sdl_get_display_mode(uint32_t display, int current, void *result)
{
	const SDL_DisplayMode *mode = current ? SDL_GetCurrentDisplayMode((SDL_DisplayID)display) :
		SDL_GetDesktopDisplayMode((SDL_DisplayID)display);

	if (!mode)
		return 0;
	display_mode_copy(result, mode);
	return 1;
}

/* up to maximum of a display's fullscreen modes into result, as
host_display_modes; the number there are */
int host_sdl_get_fullscreen_display_modes(uint32_t display, void *result, int maximum)
{
	struct host_display_mode *out = result;
	SDL_DisplayMode **modes;
	int count = 0, index;

	modes = SDL_GetFullscreenDisplayModes((SDL_DisplayID)display, &count);
	if (!modes)
		return 0;
	for (index = 0; index < count && index < maximum; index++)
		display_mode_copy(&out[index], modes[index]);
	SDL_free(modes);
	return count;
}

int host_sdl_get_closest_fullscreen_display_mode(uint32_t display, int width, int height, float refresh_rate,
	int include_high_density_modes, void *result)
{
	SDL_DisplayMode mode;

	if (!SDL_GetClosestFullscreenDisplayMode((SDL_DisplayID)display, width, height, refresh_rate,
		include_high_density_modes != 0, &mode))
	{
		return 0;
	}
	display_mode_copy(result, &mode);
	return 1;
}

/* (SDL_Rect is four ints for the guest too) */
int host_sdl_get_display_usable_bounds(uint32_t display, void *rectangle)
{
	return SDL_GetDisplayUsableBounds((SDL_DisplayID)display, rectangle) ? 1 : 0;
}

/* up to maximum of the playback (recording 0) or recording devices' ids into
ids; how many there are */
int host_sdl_get_audio_devices(int recording, uint32_t *ids, int maximum)
{
	SDL_AudioDeviceID *devices;
	int count = 0, index;

	devices = recording ? SDL_GetAudioRecordingDevices(&count) : SDL_GetAudioPlaybackDevices(&count);
	if (!devices)
		return 0;
	for (index = 0; index < count && index < maximum; index++)
		ids[index] = (uint32_t)devices[index];
	SDL_free(devices);
	return count;
}

/* a device's name into name (size bytes, cut short); 0 when it has none */
int host_sdl_get_audio_device_name(uint32_t device, char *name, int size)
{
	const char *found = SDL_GetAudioDeviceName((SDL_AudioDeviceID)device);

	if (!found || size < 1)
		return 0;
	SDL_strlcpy(name, found, (size_t)size);
	return 1;
}

/* ---------- the 2D renderer */

static SDL_Renderer *renderer_of(uint32_t renderer)
{
	return host_sdl_handle_get(renderer, _handle_renderer);
}

uint32_t host_sdl_create_renderer(uint32_t window, const char *name)
{
	SDL_Window *object = window_of(window);

	return object ? host_sdl_handle_new(_handle_renderer, SDL_CreateRenderer(object, name)) : 0;
}

void host_sdl_destroy_renderer(uint32_t renderer)
{
	SDL_Renderer *object = renderer_of(renderer);

	if (!object)
		return;
	host_sdl_handle_free(renderer);
	SDL_DestroyRenderer(object);
}

int host_sdl_set_render_vsync(uint32_t renderer, int vsync)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_SetRenderVSync(object, vsync) ? 1 : 0;
}

int host_sdl_set_render_draw_color(uint32_t renderer, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_SetRenderDrawColor(object, (Uint8)red, (Uint8)green, (Uint8)blue, (Uint8)alpha) ? 1 : 0;
}

int host_sdl_set_render_scale(uint32_t renderer, float x, float y)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_SetRenderScale(object, x, y) ? 1 : 0;
}

int host_sdl_render_clear(uint32_t renderer)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_RenderClear(object) ? 1 : 0;
}

/* rectangle: NULL for the whole target, else an SDL_FRect (four floats in
either layout) */
int host_sdl_render_fill_rect(uint32_t renderer, const void *rectangle)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_RenderFillRect(object, (const SDL_FRect *)rectangle) ? 1 : 0;
}

int host_sdl_render_debug_text(uint32_t renderer, float x, float y, const char *text)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_RenderDebugText(object, x, y, text) ? 1 : 0;
}

int host_sdl_render_present(uint32_t renderer)
{
	SDL_Renderer *object = renderer_of(renderer);

	return object && SDL_RenderPresent(object) ? 1 : 0;
}

/* ---------- dialogs */

struct file_dialog
{
	SDL_AtomicInt done;
	char *path;
	uint32_t size;
};

static void SDLCALL file_dialog_chosen(void *userdata, const char * const *files, int filter)
{
	struct file_dialog *dialog = userdata;

	(void)filter;
	if (files && files[0])
		SDL_strlcpy(dialog->path, files[0], dialog->size);
	SDL_SetAtomicInt(&dialog->done, 1);
}

/* the file the player picks (filter_names and filter_patterns: the
filters' names and patterns one after another, each NUL terminated), once
the dialog closes; 1 with its path, 0 if none */
int host_sdl_open_file_dialog(int filter_count, const char *filter_names, const char *filter_patterns,
	char *path, uint32_t size)
{
	SDL_DialogFileFilter filters[8];
	struct file_dialog dialog;
	int index;

	if (filter_count < 0 || filter_count > 8 || !size)
		return 0;
	for (index = 0; index < filter_count; index++)
	{
		filters[index].name = filter_names;
		filters[index].pattern = filter_patterns;
		filter_names += strlen(filter_names) + 1;
		filter_patterns += strlen(filter_patterns) + 1;
	}
	path[0] = 0;
	memset(&dialog, 0, sizeof(dialog));
	dialog.path = path;
	dialog.size = size;
	SDL_ShowOpenFileDialog(file_dialog_chosen, &dialog, NULL, filters, filter_count, NULL, false);
	/* the dialog answers through events (and on some systems another
	thread), as in sdl_platform.c */
	while (!SDL_GetAtomicInt(&dialog.done))
	{
		SDL_PumpEvents();
		SDL_Delay(50);
	}
	return path[0] != 0;
}

/* ---------- the self-updater (update.h, posix_update.c) */

struct download_progress
{
	uint32_t function;
	uint32_t context;
};

static void download_progress(void *context, unsigned long long received, unsigned long long total)
{
	struct download_progress *progress = context;

	/* (a release is far below 4 GB) */
	host_call_guest(progress->function, progress->context, (uint32_t)received, (uint32_t)total, 0);
}

/* guest_desktop.c's struct download; url, path and error are guest
addresses, progress the guest's void (*)(unsigned int context, unsigned int
received, unsigned int total), called on this thread */
struct host_download
{
	uint32_t url;
	uint32_t path;
	uint32_t maximum;
	uint32_t progress;
	uint32_t context;
	uint32_t error;
	int32_t error_size;
};

typedef char host_download_size_check[sizeof(struct host_download) == 28 ? 1 : -1];

int host_update_download(struct host_download *download)
{
	struct download_progress binding = { download->progress, download->context };

	return update_download((const char *)(uintptr_t)download->url, (const char *)(uintptr_t)download->path,
		download->maximum, download->progress ? download_progress : NULL, &binding,
		(char *)(uintptr_t)download->error, download->error_size);
}

/* guest_desktop.c's struct https_request: every pointer a guest address */
struct host_https_request
{
	uint32_t method;
	uint32_t url;
	uint32_t headers;
	uint32_t body;
	uint32_t body_size;
	uint32_t maximum;
	uint32_t answer_headers;
	uint32_t answer_headers_size;
	uint32_t answer_body;
	int32_t status;
	uint32_t answer_body_size;
	uint32_t error;
	int32_t error_size;
};

typedef char host_https_request_size_check[sizeof(struct host_https_request) == 52 ? 1 : -1];

/* update_https_request for the guest (master_list.c, server_master.c,
server_netlist.c): the answer copied into the guest's buffers; 1 if one came */
int host_update_https_request(struct host_https_request *request)
{
	struct update_response *response = malloc(sizeof(*response));
	char *headers = (char *)(uintptr_t)request->answer_headers;

	if (!response)
		return 0;
	if (!update_https_request((const char *)(uintptr_t)request->method, (const char *)(uintptr_t)request->url,
		request->headers ? (const char *)(uintptr_t)request->headers : NULL,
		request->body ? (const void *)(uintptr_t)request->body : NULL, request->body_size, request->maximum,
		response, (char *)(uintptr_t)request->error, request->error_size))
	{
		free(response);
		return 0;
	}
	if (request->answer_headers_size)
		snprintf(headers, request->answer_headers_size, "%s", response->headers);
	if (response->body_size > request->maximum)
		response->body_size = request->maximum;
	if (response->body_size)
		memcpy((void *)(uintptr_t)request->answer_body, response->body, response->body_size);
	request->status = response->status;
	request->answer_body_size = (uint32_t)response->body_size;
	free(response->body);
	free(response);
	return 1;
}

int host_update_executable_path(char *path, int size)
{
	return update_executable_path(path, size);
}

int host_update_replace_file(const char *path, const char *new_path, const char *old_path)
{
	return update_replace_file(path, new_path, old_path);
}

void host_update_delete_file(const char *path)
{
	update_delete_file(path);
}

int host_update_make_directory(const char *path)
{
	return update_make_directory(path);
}

int host_update_launch(const char *path)
{
	return update_launch(path);
}
