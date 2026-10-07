/*
GUEST_SDL_DESKTOP.C

The SDL3 functions the platform layer's desktop code calls (sdl_platform.c,
port_config.c, menu_files.c), for the macOS guest; the ones it shares with
Android are in port/android/guest/runtime/guest_sdl.c.

Windows, renderers and displays go to the host (port/macos/host/host_sdl.c)
as small integer handles and fixed-width structures (halo_macos_abi.h),
since SDL's own have pointers in them. Files, globbing, atomics and delays
are done here with the guest's C library.
*/

/* before SDL: musl's alloca.h must come first */
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <SDL3/SDL.h>

#include "guest_host.h"
#include "guest_host_macos.h"

/* ---------- time */

Uint64 SDL_GetTicksNS(void)
{
	return (Uint64)host_sdl_ticks_ns();
}

void SDL_DelayPrecise(Uint64 nanoseconds)
{
	struct timespec duration, remaining;

	duration.tv_sec = (time_t)(nanoseconds / 1000000000ull);
	duration.tv_nsec = (long)(nanoseconds % 1000000000ull);
	while (nanosleep(&duration, &remaining) != 0 && errno == EINTR)
		duration = remaining;
}

/* ---------- atomics */

int SDL_GetAtomicInt(SDL_AtomicInt *atomic)
{
	return __atomic_load_n(&atomic->value, __ATOMIC_SEQ_CST);
}

int SDL_SetAtomicInt(SDL_AtomicInt *atomic, int value)
{
	return __atomic_exchange_n(&atomic->value, value, __ATOMIC_SEQ_CST);
}

/* ---------- events */

void SDL_PumpEvents(void)
{
	host_sdl_pump_events();
}

bool SDL_PushEvent(SDL_Event *event)
{
	return host_sdl_push_event(event) != 0;
}

/* ---------- windows */

void SDL_DestroyWindow(SDL_Window *window)
{
	host_sdl_destroy_window((unsigned int)window);
}

bool SDL_GetWindowSize(SDL_Window *window, int *width, int *height)
{
	int w = 0, h = 0;

	host_sdl_window_size((unsigned int)window, &w, &h);
	if (width)
		*width = w;
	if (height)
		*height = h;
	return true;
}

bool SDL_SetWindowSize(SDL_Window *window, int width, int height)
{
	return host_sdl_set_window_size((unsigned int)window, width, height) != 0;
}

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window *window)
{
	return (SDL_WindowFlags)host_sdl_window_flags((unsigned int)window);
}

bool SDL_SetWindowFullscreen(SDL_Window *window, bool fullscreen)
{
	return host_sdl_set_window_fullscreen((unsigned int)window, fullscreen) != 0;
}

void SDL_WarpMouseInWindow(SDL_Window *window, float x, float y)
{
	host_sdl_warp_mouse((unsigned int)window, x, y);
}

/* ---------- displays */

static void mode_from_host(SDL_DisplayMode *mode, const struct halo_macos_display_mode *host)
{
	memset(mode, 0, sizeof(*mode));
	mode->displayID = host->display;
	mode->format = (SDL_PixelFormat)host->format;
	mode->w = host->width;
	mode->h = host->height;
	mode->pixel_density = host->pixel_density;
	mode->refresh_rate = host->refresh_rate;
	mode->refresh_rate_numerator = host->refresh_numerator;
	mode->refresh_rate_denominator = host->refresh_denominator;
}

static void mode_to_host(const SDL_DisplayMode *mode, struct halo_macos_display_mode *host)
{
	host->display = mode->displayID;
	host->format = (unsigned int)mode->format;
	host->width = mode->w;
	host->height = mode->h;
	host->pixel_density = mode->pixel_density;
	host->refresh_rate = mode->refresh_rate;
	host->refresh_numerator = mode->refresh_rate_numerator;
	host->refresh_denominator = mode->refresh_rate_denominator;
}

SDL_DisplayID SDL_GetPrimaryDisplay(void)
{
	return host_sdl_primary_display();
}

SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window *window)
{
	return host_sdl_display_for_window((unsigned int)window);
}

/* SDL keeps the modes it returns for as long as the display lasts: these
are kept per display (and which), refreshed on each call */
#define CACHED_DISPLAYS 8

static const SDL_DisplayMode *display_mode(SDL_DisplayID display, int which)
{
	static struct
	{
		SDL_DisplayID display;
		SDL_DisplayMode mode;
	} cache[2][CACHED_DISPLAYS];
	struct halo_macos_display_mode host;
	int slot;

	if (!host_sdl_display_mode(display, which, &host))
		return NULL;
	for (slot = 0; slot < CACHED_DISPLAYS - 1; slot++)
	{
		if (cache[which][slot].display == display || !cache[which][slot].display)
			break;
	}
	cache[which][slot].display = display;
	mode_from_host(&cache[which][slot].mode, &host);
	return &cache[which][slot].mode;
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
	static struct halo_macos_display_mode host[HALO_MACOS_DISPLAY_MODE_MAXIMUM];
	int found = host_sdl_fullscreen_display_modes(display, host, HALO_MACOS_DISPLAY_MODE_MAXIMUM);
	SDL_DisplayMode **result;
	SDL_DisplayMode *modes;
	int index;

	if (found < 0)
		found = 0;
	/* (one allocation, which SDL_free frees, as SDL's own) */
	result = malloc((found + 1) * sizeof(SDL_DisplayMode *) + found * sizeof(SDL_DisplayMode));
	if (!result)
		return NULL;
	modes = (SDL_DisplayMode *)(result + found + 1);
	for (index = 0; index < found; index++)
	{
		mode_from_host(&modes[index], &host[index]);
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
	struct halo_macos_display_mode host;

	if (!host_sdl_closest_display_mode(display, width, height, refresh_rate, include_high_density_modes, &host))
		return false;
	mode_from_host(closest, &host);
	return true;
}

bool SDL_SetWindowFullscreenMode(SDL_Window *window, const SDL_DisplayMode *mode)
{
	struct halo_macos_display_mode host;

	if (!mode)
		return host_sdl_set_window_fullscreen_mode((unsigned int)window, NULL) != 0;
	mode_to_host(mode, &host);
	return host_sdl_set_window_fullscreen_mode((unsigned int)window, &host) != 0;
}

bool SDL_GetDisplayUsableBounds(SDL_DisplayID display, SDL_Rect *rectangle)
{
	int bounds[4];

	if (!host_sdl_display_usable_bounds(display, bounds))
		return false;
	rectangle->x = bounds[0];
	rectangle->y = bounds[1];
	rectangle->w = bounds[2];
	rectangle->h = bounds[3];
	return true;
}

/* ---------- a software renderer (the first start's data extraction) */

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
	return host_sdl_set_render_vsync((unsigned int)renderer, vsync) != 0;
}

bool SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
	return host_sdl_set_render_draw_color((unsigned int)renderer, red, green, blue, alpha) != 0;
}

bool SDL_SetRenderScale(SDL_Renderer *renderer, float x, float y)
{
	return host_sdl_set_render_scale((unsigned int)renderer, x, y) != 0;
}

bool SDL_RenderClear(SDL_Renderer *renderer)
{
	return host_sdl_render_clear((unsigned int)renderer) != 0;
}

bool SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rectangle)
{
	/* (no rectangle: the whole target) */
	if (!rectangle)
		return host_sdl_render_fill_rect((unsigned int)renderer, 0.0f, 0.0f, -1.0f, -1.0f) != 0;
	return host_sdl_render_fill_rect((unsigned int)renderer, rectangle->x, rectangle->y, rectangle->w,
		rectangle->h) != 0;
}

bool SDL_RenderDebugText(SDL_Renderer *renderer, float x, float y, const char *text)
{
	return host_sdl_render_debug_text((unsigned int)renderer, x, y, text) != 0;
}

bool SDL_RenderPresent(SDL_Renderer *renderer)
{
	return host_sdl_render_present((unsigned int)renderer) != 0;
}

/* ---------- dialogs */

bool SDL_ShowMessageBox(const SDL_MessageBoxData *data, int *button)
{
	struct halo_macos_message_box_button buttons[8];
	int count = data->numbuttons < 8 ? data->numbuttons : 8;
	int answer = -1, index;
	int shown;

	for (index = 0; index < count; index++)
	{
		buttons[index].flags = data->buttons[index].flags;
		buttons[index].id = data->buttons[index].buttonID;
		buttons[index].text = (unsigned int)data->buttons[index].text;
	}
	shown = host_sdl_show_message_box(data->flags, (unsigned int)data->window, data->title, data->message,
		count, buttons, &answer);
	if (button)
		*button = answer;
	return shown != 0;
}

/* (answered at once: the host waits for the player) */
void SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void *userdata, SDL_Window *window,
	const SDL_DialogFileFilter *filters, int filter_count, const char *default_location, bool allow_many)
{
	static char path[1024];
	const char *files[2] = { NULL, NULL };

	(void)window;
	(void)default_location;
	(void)allow_many;
	if (host_sdl_choose_file(filter_count > 0 ? filters[0].name : "", filter_count > 0 ? filters[0].pattern : "*",
		path, sizeof(path)))
	{
		files[0] = path;
	}
	callback(userdata, files, files[0] ? 0 : -1);
}

/* ---------- files */

const char *SDL_GetBasePath(void)
{
	static char path[1024];

	if (!path[0])
		host_macos_base_path(path, sizeof(path));
	return path;
}

void *SDL_LoadFile(const char *file, size_t *size)
{
	FILE *stream = fopen(file, "rb");
	unsigned char *data = NULL;
	long length;

	if (!stream)
		return NULL;
	if (fseek(stream, 0, SEEK_END) == 0 && (length = ftell(stream)) >= 0 && fseek(stream, 0, SEEK_SET) == 0)
	{
		/* (SDL's ends the data with a NUL, not counted) */
		data = malloc((size_t)length + 1);
		if (data && fread(data, 1, (size_t)length, stream) == (size_t)length)
		{
			data[length] = 0;
			if (size)
				*size = (size_t)length;
		}
		else
		{
			free(data);
			data = NULL;
		}
	}
	fclose(stream);
	return data;
}

bool SDL_SaveFile(const char *file, const void *data, size_t size)
{
	FILE *stream = fopen(file, "wb");
	bool written;

	if (!stream)
		return false;
	written = fwrite(data, 1, size, stream) == size;
	return fclose(stream) == 0 && written;
}

/* SDL's glob: * and ? match within one path component */
static int glob_match(const char *pattern, const char *name, int fold)
{
	for (;; pattern++, name++)
	{
		if (*pattern == '*')
		{
			while (*pattern == '*')
				pattern++;
			for (;; name++)
			{
				if (glob_match(pattern, name, fold))
					return 1;
				if (!*name || *name == '/')
					return 0;
			}
		}
		if (!*pattern)
			return !*name;
		if (!*name || *name == '/' && *pattern != '/')
			return 0;
		if (*pattern != '?')
		{
			int a = (unsigned char)*pattern, b = (unsigned char)*name;

			if (fold)
			{
				a = a >= 'A' && a <= 'Z' ? a + 32 : a;
				b = b >= 'A' && b <= 'Z' ? b + 32 : b;
			}
			if (a != b)
				return 0;
		}
	}
}

struct glob_list
{
	char **names;
	int count;
	int capacity;
	size_t text;
};

static void glob_walk(const char *root, const char *relative, const char *pattern, int fold, int depth,
	struct glob_list *list)
{
	char path[1200];
	DIR *directory;
	struct dirent *entry;

	snprintf(path, sizeof(path), "%s%s%s", root, *relative ? "/" : "", relative);
	directory = opendir(path);
	if (!directory)
		return;
	while ((entry = readdir(directory)) != NULL)
	{
		char name[1200];
		struct stat information;

		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		snprintf(name, sizeof(name), "%s%s%s", relative, *relative ? "/" : "", entry->d_name);
		if (!pattern || glob_match(pattern, name, fold))
		{
			if (list->count == list->capacity)
			{
				int capacity = list->capacity ? list->capacity * 2 : 16;
				char **names = realloc(list->names, capacity * sizeof(char *));

				if (!names)
					break;
				list->names = names;
				list->capacity = capacity;
			}
			list->names[list->count] = strdup(name);
			if (list->names[list->count])
				list->text += strlen(name) + 1, list->count++;
		}
		snprintf(path, sizeof(path), "%s/%s", root, name);
		if (depth > 0 && stat(path, &information) == 0 && S_ISDIR(information.st_mode))
			glob_walk(root, name, pattern, fold, depth - 1, list);
	}
	closedir(directory);
}

char **SDL_GlobDirectory(const char *path, const char *pattern, SDL_GlobFlags flags, int *count)
{
	struct glob_list list = { NULL, 0, 0, 0 };
	char root[1024];
	char **result;
	char *text;
	int depth = 16, index;

	if (count)
		*count = 0;
	snprintf(root, sizeof(root), "%s", path);
	while (strlen(root) > 1 && root[strlen(root) - 1] == '/')
		root[strlen(root) - 1] = '\0';
	/* (no deeper than the pattern's components reach) */
	if (pattern)
	{
		const char *character;

		for (depth = 0, character = pattern; *character; character++)
			depth += *character == '/';
	}
	glob_walk(root, "", pattern, (flags & SDL_GLOB_CASEINSENSITIVE) != 0, depth, &list);
	/* (one allocation, which SDL_free frees, as SDL's own) */
	result = malloc((list.count + 1) * sizeof(char *) + list.text);
	if (result)
	{
		text = (char *)(result + list.count + 1);
		for (index = 0; index < list.count; index++)
		{
			strcpy(text, list.names[index]);
			result[index] = text;
			text += strlen(text) + 1;
		}
		result[list.count] = NULL;
		if (count)
			*count = list.count;
	}
	for (index = 0; index < list.count; index++)
		free(list.names[index]);
	free(list.names);
	return result;
}
