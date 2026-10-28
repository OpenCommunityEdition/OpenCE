#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <SDL3/SDL.h>

long long host_sdl_ticks_ns(void);
int host_sdl_get_path_info(const char *path, SDL_PathInfo *info);
int host_sdl_glob_directory(const char *path, const char *pattern, unsigned int flags, char *buffer, int size);
void host_sdl_pump_events(void);
int host_sdl_push_event(void *event);
unsigned int host_sdl_key_from_scancode(int scancode, int modifiers, int key_event);
int host_sdl_audio_devices(int recording, unsigned int *ids, int capacity);
void host_sdl_audio_device_name(unsigned int id, char *buffer, unsigned int size);
long long host_sdl_window_flags(unsigned int window);
int host_sdl_set_window_fullscreen(unsigned int window, int fullscreen);
int host_sdl_set_window_size(unsigned int window, int width, int height);
int host_sdl_window_size(unsigned int window, int *width, int *height);
void host_sdl_destroy_window(unsigned int window);
unsigned int host_sdl_keyboard_focus(void);
int host_sdl_set_window_mouse_grab(unsigned int window, int grabbed);
void host_sdl_warp_mouse(unsigned int window, float x, float y);
int host_sdl_show_cursor(int shown);
unsigned int host_sdl_primary_display(void);
unsigned int host_sdl_display_for_window(unsigned int window);
int host_sdl_display_usable_bounds(unsigned int display, SDL_Rect *rect);
int host_sdl_display_mode(unsigned int display, int current, SDL_DisplayMode *mode);
int host_sdl_fullscreen_modes(unsigned int display, SDL_DisplayMode *modes, int capacity);
int host_sdl_closest_fullscreen_mode(unsigned int display, int width, int height, float refresh, int high_density,
	SDL_DisplayMode *mode);
int host_sdl_set_window_fullscreen_mode(unsigned int window, const SDL_DisplayMode *mode);
void host_sdl_open_file_dialog(const char *filters);
int host_sdl_file_dialog_result(char *path, unsigned int size);
unsigned int host_sdl_create_renderer(unsigned int window);
void host_sdl_destroy_renderer(unsigned int renderer);
int host_sdl_render(unsigned int renderer, int operation, float x, float y, const char *text, const SDL_FRect *rect);

Uint64 SDL_GetTicksNS(void)
{
	return (Uint64)host_sdl_ticks_ns();
}

void SDL_DelayPrecise(Uint64 nanoseconds)
{
	struct timespec duration = { (time_t)(nanoseconds / 1000000000u), (long)(nanoseconds % 1000000000u) };

	nanosleep(&duration, NULL);
}

int SDL_GetAtomicInt(SDL_AtomicInt *atomic)
{
	return __atomic_load_n(&atomic->value, __ATOMIC_SEQ_CST);
}

int SDL_SetAtomicInt(SDL_AtomicInt *atomic, int value)
{
	return __atomic_exchange_n(&atomic->value, value, __ATOMIC_SEQ_CST);
}

int SDL_AddAtomicInt(SDL_AtomicInt *atomic, int value)
{
	return __atomic_fetch_add(&atomic->value, value, __ATOMIC_SEQ_CST);
}

const char *SDL_GetBasePath(void)
{
	static char path[1024];
	ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
	char *slash;

	path[length > 0 ? length : 0] = 0;
	slash = strrchr(path, '/');
	if (slash)
		slash[1] = 0;
	return path;
}

bool SDL_GetPathInfo(const char *path, SDL_PathInfo *info)
{
	return host_sdl_get_path_info(path, info) != 0;
}

char **SDL_GlobDirectory(const char *path, const char *pattern, SDL_GlobFlags flags, int *count)
{
	static char text[16384];
	int found = host_sdl_glob_directory(path, pattern, flags, text, sizeof(text)), index;
	size_t length = 0;
	char **names;

	for (index = 0; index < found; index++)
		length += strlen(text + length) + 1;
	names = malloc((found + 1) * sizeof(char *) + length);
	if (!names)
		return NULL;
	memcpy(names + found + 1, text, length);
	for (index = 0, length = 0; index < found; index++)
	{
		names[index] = (char *)(names + found + 1) + length;
		length += strlen(names[index]) + 1;
	}
	names[found] = NULL;
	if (count)
		*count = found;
	return names;
}

static void dialog_answer(void);

void SDL_PumpEvents(void)
{
	host_sdl_pump_events();
	dialog_answer();
}

bool SDL_PushEvent(SDL_Event *event)
{
	return host_sdl_push_event(event) != 0;
}

SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, SDL_Keymod modifiers, bool key_event)
{
	return host_sdl_key_from_scancode((int)scancode, (int)modifiers, key_event);
}

SDL_PropertiesID SDL_GetWindowProperties(SDL_Window *window)
{
	(void)window;
	return 0;
}

bool SDL_GetBooleanProperty(SDL_PropertiesID properties, const char *name, bool default_value)
{
	(void)properties;
	(void)name;
	return default_value;
}

float SDL_GetFloatProperty(SDL_PropertiesID properties, const char *name, float default_value)
{
	(void)properties;
	(void)name;
	return default_value;
}

static SDL_AudioDeviceID *audio_devices(int recording, int *count)
{
	unsigned int ids[64];
	int found = host_sdl_audio_devices(recording, ids, 64), index;
	SDL_AudioDeviceID *result = malloc((found + 1) * sizeof(SDL_AudioDeviceID));

	if (!result)
		return NULL;
	for (index = 0; index < found; index++)
		result[index] = ids[index];
	result[found] = 0;
	if (count)
		*count = found;
	return result;
}

SDL_AudioDeviceID *SDL_GetAudioPlaybackDevices(int *count)
{
	return audio_devices(0, count);
}

SDL_AudioDeviceID *SDL_GetAudioRecordingDevices(int *count)
{
	return audio_devices(1, count);
}

const char *SDL_GetAudioDeviceName(SDL_AudioDeviceID id)
{
	static __thread char name[256];

	host_sdl_audio_device_name(id, name, sizeof(name));
	return name[0] ? name : NULL;
}

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window *window)
{
	return (SDL_WindowFlags)host_sdl_window_flags((unsigned int)window);
}

bool SDL_SetWindowFullscreen(SDL_Window *window, bool fullscreen)
{
	return host_sdl_set_window_fullscreen((unsigned int)window, fullscreen) != 0;
}

bool SDL_SetWindowSize(SDL_Window *window, int width, int height)
{
	return host_sdl_set_window_size((unsigned int)window, width, height) != 0;
}

bool SDL_GetWindowSize(SDL_Window *window, int *width, int *height)
{
	return host_sdl_window_size((unsigned int)window, width, height) != 0;
}

void SDL_DestroyWindow(SDL_Window *window)
{
	host_sdl_destroy_window((unsigned int)window);
}

SDL_Window *SDL_GetKeyboardFocus(void)
{
	return (SDL_Window *)host_sdl_keyboard_focus();
}

bool SDL_SetWindowMouseGrab(SDL_Window *window, bool grabbed)
{
	return host_sdl_set_window_mouse_grab((unsigned int)window, grabbed) != 0;
}

void SDL_WarpMouseInWindow(SDL_Window *window, float x, float y)
{
	host_sdl_warp_mouse((unsigned int)window, x, y);
}

bool SDL_ShowCursor(void)
{
	return host_sdl_show_cursor(1) != 0;
}

bool SDL_HideCursor(void)
{
	return host_sdl_show_cursor(0) != 0;
}

SDL_DisplayID SDL_GetPrimaryDisplay(void)
{
	return host_sdl_primary_display();
}

SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window *window)
{
	return host_sdl_display_for_window((unsigned int)window);
}

bool SDL_GetDisplayUsableBounds(SDL_DisplayID display, SDL_Rect *rect)
{
	return host_sdl_display_usable_bounds(display, rect) != 0;
}

static const SDL_DisplayMode *display_mode(SDL_DisplayID display, int current)
{
	static __thread SDL_DisplayMode modes[2];

	return host_sdl_display_mode(display, current, &modes[current]) ? &modes[current] : NULL;
}

const SDL_DisplayMode *SDL_GetDesktopDisplayMode(SDL_DisplayID display)
{
	return display_mode(display, 0);
}

const SDL_DisplayMode *SDL_GetCurrentDisplayMode(SDL_DisplayID display)
{
	return display_mode(display, 1);
}

SDL_DisplayMode **SDL_GetFullscreenDisplayModes(SDL_DisplayID display, int *count)
{
	SDL_DisplayMode found[128];
	int number = host_sdl_fullscreen_modes(display, found, 128), index;
	SDL_DisplayMode **result = malloc((number + 1) * sizeof(SDL_DisplayMode *) + number * sizeof(SDL_DisplayMode));
	SDL_DisplayMode *modes = (SDL_DisplayMode *)(result + number + 1);

	if (!result)
		return NULL;
	for (index = 0; index < number; index++)
	{
		modes[index] = found[index];
		result[index] = &modes[index];
	}
	result[number] = NULL;
	if (count)
		*count = number;
	return result;
}

bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID display, int width, int height, float refresh,
	bool high_density, SDL_DisplayMode *closest)
{
	return host_sdl_closest_fullscreen_mode(display, width, height, refresh, high_density, closest) != 0;
}

bool SDL_SetWindowFullscreenMode(SDL_Window *window, const SDL_DisplayMode *mode)
{
	return host_sdl_set_window_fullscreen_mode((unsigned int)window, mode) != 0;
}

static SDL_DialogFileCallback dialog_callback;
static void *dialog_userdata;

void SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void *userdata, SDL_Window *window,
	const SDL_DialogFileFilter *filters, int filter_count, const char *default_location, bool allow_many)
{
	char text[512] = "";
	int index;

	(void)window;
	(void)default_location;
	(void)allow_many;
	for (index = 0; index < filter_count; index++)
	{
		snprintf(text + strlen(text), sizeof(text) - strlen(text), "%s\t%s\n", filters[index].name,
			filters[index].pattern);
	}
	dialog_callback = callback;
	dialog_userdata = userdata;
	host_sdl_open_file_dialog(text);
}

static void dialog_answer(void)
{
	char path[1024];
	const char *files[2] = { path, NULL };
	int state;

	if (!dialog_callback || (state = host_sdl_file_dialog_result(path, sizeof(path))) < 0)
		return;
	{
		SDL_DialogFileCallback callback = dialog_callback;

		dialog_callback = NULL;
		callback(dialog_userdata, state == 2 ? NULL : state ? files : files + 1, -1);
	}
}

enum
{
	_render_vsync,
	_render_color,
	_render_clear,
	_render_scale,
	_render_text,
	_render_fill,
	_render_present,
};

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, const char *name)
{
	(void)name;
	return (SDL_Renderer *)host_sdl_create_renderer((unsigned int)window);
}

void SDL_DestroyRenderer(SDL_Renderer *renderer)
{
	host_sdl_destroy_renderer((unsigned int)renderer);
}

bool SDL_SetRenderVSync(SDL_Renderer *renderer, int vsync)
{
	return host_sdl_render((unsigned int)renderer, _render_vsync, (float)vsync, 0.0f, NULL, NULL) != 0;
}

bool SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	SDL_FRect color = { r, g, b, a };

	return host_sdl_render((unsigned int)renderer, _render_color, 0.0f, 0.0f, NULL, &color) != 0;
}

bool SDL_RenderClear(SDL_Renderer *renderer)
{
	return host_sdl_render((unsigned int)renderer, _render_clear, 0.0f, 0.0f, NULL, NULL) != 0;
}

bool SDL_SetRenderScale(SDL_Renderer *renderer, float x, float y)
{
	return host_sdl_render((unsigned int)renderer, _render_scale, x, y, NULL, NULL) != 0;
}

bool SDL_RenderDebugText(SDL_Renderer *renderer, float x, float y, const char *text)
{
	return host_sdl_render((unsigned int)renderer, _render_text, x, y, text, NULL) != 0;
}

bool SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rect)
{
	return host_sdl_render((unsigned int)renderer, _render_fill, 0.0f, 0.0f, NULL, rect) != 0;
}

bool SDL_RenderPresent(SDL_Renderer *renderer)
{
	return host_sdl_render((unsigned int)renderer, _render_present, 0.0f, 0.0f, NULL, NULL) != 0;
}
