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

#define MESSAGE_BOX_BUTTONS 8

/* button_texts: the buttons' texts one after another, each NUL terminated;
returns 1 and the button chosen in *chosen, else 0 */
int host_sdl_show_message_box(uint32_t flags, const char *title, const char *message, int button_count,
	const int32_t *button_flags, const int32_t *button_ids, const char *button_texts, int32_t *chosen)
{
	SDL_MessageBoxButtonData buttons[MESSAGE_BOX_BUTTONS];
	SDL_MessageBoxData data;
	int index;
	int result = -1;

	if (button_count < 0 || button_count > MESSAGE_BOX_BUTTONS)
		return 0;
	for (index = 0; index < button_count; index++)
	{
		buttons[index].flags = (SDL_MessageBoxButtonFlags)button_flags[index];
		buttons[index].buttonID = button_ids[index];
		buttons[index].text = button_texts;
		button_texts += strlen(button_texts) + 1;
	}
	memset(&data, 0, sizeof(data));
	data.flags = (SDL_MessageBoxFlags)flags;
	data.title = title;
	data.message = message;
	data.numbuttons = button_count;
	data.buttons = buttons;
	if (!SDL_ShowMessageBox(&data, &result))
		return 0;
	*chosen = result;
	return 1;
}

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

/* progress: the guest's void (*)(unsigned int context, unsigned int received,
unsigned int total), called on this thread */
int host_update_download(const char *url, const char *path, uint32_t progress, uint32_t context,
	char *error, int error_size)
{
	struct download_progress binding = { progress, context };

	return update_download(url, path, progress ? download_progress : NULL, &binding, error, error_size);
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
