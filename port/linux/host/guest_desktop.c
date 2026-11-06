/*
GUEST_DESKTOP.C

The guest's half of what the Linux desktop's code paths need beyond the
Android app's (port/android/guest/runtime/guest_sdl.c): the SDL3 functions
sdl_platform.c, menu_files.c, xiso.c and updater.c call, and update.h's for
the self-updater. host_desktop.c is the host's half.

What the guest can do itself it does here, on its own threads and its own C
library: SDL's threads, mutexes, conditions and atomics (a thread that runs guest code
must be a guest thread), its files (SDL_IOStream is a stdio FILE here), SDL_LoadFile,
SDL_GetPathInfo and SDL_GlobDirectory. The rest goes to the host, with SDL's objects as handles
and its structures copied field by field where they hold pointers.
*/

/* before SDL: musl's alloca.h must come first */
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <SDL3/SDL.h>

#include "update.h"

/* ---------- the host's half (host_desktop.c, host_imports.list) */

void host_sdl_get_base_path(char *buffer, unsigned int size);
long long host_sdl_ticks_ns(void);
void host_sdl_pump_events(void);
int host_sdl_push_event(const void *event);
long long host_sdl_get_window_flags(unsigned int window);
int host_sdl_set_window_fullscreen(unsigned int window, int fullscreen);
int host_sdl_set_window_fullscreen_mode(unsigned int window, unsigned int display, int width, int height,
	float refresh_rate);
void host_sdl_get_window_size(unsigned int window, int *width, int *height);
int host_sdl_set_window_size(unsigned int window, int width, int height);
void host_sdl_destroy_window(unsigned int window);
int host_sdl_warp_mouse_in_window(unsigned int window, float x, float y);
int host_sdl_has_screen_keyboard_support(void);
int host_sdl_get_hint_boolean(const char *name, int default_value);
int host_sdl_text_input_active(unsigned int window);
int host_sdl_start_text_input(unsigned int window, int multiline, int type);
int host_sdl_stop_text_input(unsigned int window);
unsigned int host_sdl_get_display_for_window(unsigned int window);
unsigned int host_sdl_get_primary_display(void);
int host_sdl_get_display_mode(unsigned int display, int current, void *result);
int host_sdl_get_fullscreen_display_modes(unsigned int display, void *result, int maximum);
int host_sdl_get_closest_fullscreen_display_mode(unsigned int display, int width, int height, float refresh_rate,
	int include_high_density_modes, void *result);
int host_sdl_get_display_usable_bounds(unsigned int display, void *rectangle);
int host_sdl_get_audio_devices(int recording, unsigned int *ids, int maximum);
int host_sdl_get_audio_device_name(unsigned int device, char *name, int size);
unsigned int host_sdl_create_renderer(unsigned int window, const char *name);
void host_sdl_destroy_renderer(unsigned int renderer);
int host_sdl_set_render_vsync(unsigned int renderer, int vsync);
int host_sdl_set_render_draw_color(unsigned int renderer, unsigned int red, unsigned int green, unsigned int blue,
	unsigned int alpha);
int host_sdl_set_render_scale(unsigned int renderer, float x, float y);
int host_sdl_render_clear(unsigned int renderer);
int host_sdl_render_fill_rect(unsigned int renderer, const void *rectangle);
int host_sdl_render_debug_text(unsigned int renderer, float x, float y, const char *text);
int host_sdl_render_present(unsigned int renderer);
int host_sdl_open_file_dialog(int filter_count, const char *filter_names, const char *filter_patterns,
	char *path, unsigned int size);
/* a window's properties (HDR: sdl_platform.c), by name */
int host_sdl_get_window_property_boolean(unsigned int window, const char *name, int default_value);
float host_sdl_get_window_property_float(unsigned int window, const char *name, float default_value);
void host_sdl_get_current_video_driver(char *buffer, unsigned int size);
/* the window with the keyboard's focus (its handle), else 0 */
unsigned int host_sdl_get_keyboard_focus(void);
int host_sdl_set_window_mouse_grab(unsigned int window, int grabbed);
/* the cursor: shown (1), hidden (0), or (-1) only asked; whether it is shown */
int host_sdl_cursor(int shown);
unsigned int host_sdl_get_key_from_scancode(int scancode, unsigned int modifiers, int key_event);
/* the automated test's virtual gamepads (xinput_sdl.c): the joystick's id,
its handle once open, its buttons and axes */
unsigned int host_sdl_attach_virtual_joystick(int type, int axes, int buttons, unsigned int axis_mask,
	unsigned int button_mask, const char *name);
unsigned int host_sdl_open_joystick(unsigned int id);
int host_sdl_set_joystick_virtual_button(unsigned int joystick, int button, int down);
int host_sdl_set_joystick_virtual_axis(unsigned int joystick, int axis, int value);
/* (struct download and struct https_request, below) */
int host_update_download(void *download);
int host_update_https_request(void *request);
int host_update_executable_path(char *path, int size);
int host_update_replace_file(const char *path, const char *new_path, const char *old_path);
void host_update_delete_file(const char *path);
int host_update_make_directory(const char *path);
int host_update_launch(const char *path);

/* ---------- general */

/* (posix_server.h's: the guest's standard output, whose buffer is the guest's
C library's; the rest of posix_server.h runs in the host, posix_server.c) */
void posix_server_stdout_line_buffered(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
}

const char *SDL_GetBasePath(void)
{
	static char path[1024];

	if (!path[0])
		host_sdl_get_base_path(path, sizeof(path));
	return path;
}

Uint64 SDL_GetTicksNS(void)
{
	return (Uint64)host_sdl_ticks_ns();
}

void SDL_DelayPrecise(Uint64 nanoseconds)
{
	struct timespec duration;

	duration.tv_sec = (time_t)(nanoseconds / 1000000000ULL);
	duration.tv_nsec = (long)(nanoseconds % 1000000000ULL);
	while (nanosleep(&duration, &duration) != 0 && errno == EINTR)
		;
}

void SDL_PumpEvents(void)
{
	host_sdl_pump_events();
}

bool SDL_PushEvent(SDL_Event *event)
{
	return host_sdl_push_event(event) != 0;
}

/* ---------- atomics, mutexes and threads, the guest's own */

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

SDL_Mutex *SDL_CreateMutex(void)
{
	pthread_mutex_t *mutex = malloc(sizeof(*mutex));

	if (mutex && pthread_mutex_init(mutex, NULL) != 0)
	{
		free(mutex);
		mutex = NULL;
	}
	return (SDL_Mutex *)mutex;
}

void SDL_LockMutex(SDL_Mutex *mutex)
{
	if (mutex)
		pthread_mutex_lock((pthread_mutex_t *)mutex);
}

void SDL_UnlockMutex(SDL_Mutex *mutex)
{
	if (mutex)
		pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

void SDL_DestroyMutex(SDL_Mutex *mutex)
{
	if (!mutex)
		return;
	pthread_mutex_destroy((pthread_mutex_t *)mutex);
	free(mutex);
}

struct guest_sdl_thread
{
	pthread_t thread;
	SDL_ThreadFunction function;
	void *data;
	int status;
	/* the thread's and its handle's (until SDL_WaitThread or
	SDL_DetachThread): the last to let go frees it */
	int references;
};

static void guest_sdl_thread_release(struct guest_sdl_thread *thread)
{
	if (__atomic_sub_fetch(&thread->references, 1, __ATOMIC_ACQ_REL) == 0)
		free(thread);
}

static void *guest_sdl_thread_start(void *argument)
{
	struct guest_sdl_thread *thread = argument;

	thread->status = thread->function(thread->data);
	guest_sdl_thread_release(thread);
	return NULL;
}

/* (what SDL_CreateThread expands to) */
SDL_Thread *SDL_CreateThreadRuntime(SDL_ThreadFunction function, const char *name, void *data,
	SDL_FunctionPointer begin_thread, SDL_FunctionPointer end_thread)
{
	struct guest_sdl_thread *thread = calloc(1, sizeof(*thread));

	(void)name;
	(void)begin_thread;
	(void)end_thread;
	if (!thread)
		return NULL;
	thread->function = function;
	thread->data = data;
	thread->references = 2;
	if (pthread_create(&thread->thread, NULL, guest_sdl_thread_start, thread) != 0)
	{
		free(thread);
		return NULL;
	}
	return (SDL_Thread *)thread;
}

void SDL_WaitThread(SDL_Thread *handle, int *status)
{
	struct guest_sdl_thread *thread = (struct guest_sdl_thread *)handle;

	if (!thread)
		return;
	pthread_join(thread->thread, NULL);
	if (status)
		*status = thread->status;
	guest_sdl_thread_release(thread);
}

void SDL_DetachThread(SDL_Thread *handle)
{
	struct guest_sdl_thread *thread = (struct guest_sdl_thread *)handle;

	if (!thread)
		return;
	pthread_detach(thread->thread);
	guest_sdl_thread_release(thread);
}

/* (the dedicated server's master servers' and netlist's threads:
server_master.c, server_netlist.c) */
SDL_Condition *SDL_CreateCondition(void)
{
	pthread_cond_t *condition = malloc(sizeof(*condition));

	if (condition && pthread_cond_init(condition, NULL) != 0)
	{
		free(condition);
		condition = NULL;
	}
	return (SDL_Condition *)condition;
}

void SDL_DestroyCondition(SDL_Condition *condition)
{
	if (!condition)
		return;
	pthread_cond_destroy((pthread_cond_t *)condition);
	free(condition);
}

void SDL_SignalCondition(SDL_Condition *condition)
{
	if (condition)
		pthread_cond_signal((pthread_cond_t *)condition);
}

void SDL_BroadcastCondition(SDL_Condition *condition)
{
	if (condition)
		pthread_cond_broadcast((pthread_cond_t *)condition);
}

/* true if signalled, false on the timeout (SDL's; a negative one waits
for ever) */
bool SDL_WaitConditionTimeout(SDL_Condition *condition, SDL_Mutex *mutex, Sint32 milliseconds)
{
	struct timespec deadline;

	if (!condition || !mutex)
		return false;
	if (milliseconds < 0)
		return pthread_cond_wait((pthread_cond_t *)condition, (pthread_mutex_t *)mutex) == 0;
	clock_gettime(CLOCK_REALTIME, &deadline);
	deadline.tv_sec += milliseconds / 1000;
	deadline.tv_nsec += (long)(milliseconds % 1000) * 1000000L;
	if (deadline.tv_nsec >= 1000000000L)
	{
		deadline.tv_sec++;
		deadline.tv_nsec -= 1000000000L;
	}
	return pthread_cond_timedwait((pthread_cond_t *)condition, (pthread_mutex_t *)mutex, &deadline) == 0;
}

/* ---------- files: an SDL_IOStream is a stdio FILE */

SDL_IOStream *SDL_IOFromFile(const char *path, const char *mode)
{
	return (SDL_IOStream *)fopen(path, mode);
}

/* (the self-updater's downloaded archive, updater.c) */
SDL_IOStream *SDL_IOFromConstMem(const void *memory, size_t size)
{
	return (SDL_IOStream *)fmemopen((void *)memory, size, "rb");
}

/* the whole file, with a NUL after it (not counted), which SDL_free frees */
void *SDL_LoadFile(const char *path, size_t *size)
{
	FILE *file = fopen(path, "rb");
	struct stat information;
	char *data;
	size_t length;

	if (size)
		*size = 0;
	if (!file)
		return NULL;
	if (fstat(fileno(file), &information) != 0 || information.st_size < 0 ||
		(unsigned long long)information.st_size >= 0x7FFFFFFFull ||
		!(data = malloc((size_t)information.st_size + 1)))
	{
		fclose(file);
		return NULL;
	}
	length = fread(data, 1, (size_t)information.st_size, file);
	fclose(file);
	data[length] = 0;
	if (size)
		*size = length;
	return data;
}

/* (port_config.c: another config.toml beside it) */
bool SDL_GetPathInfo(const char *path, SDL_PathInfo *info)
{
	struct stat information;

	if (info)
		memset(info, 0, sizeof(*info));
	if (!path || stat(path, &information) != 0)
		return false;
	if (!info)
		return true;
	info->type = S_ISREG(information.st_mode) ? SDL_PATHTYPE_FILE :
		S_ISDIR(information.st_mode) ? SDL_PATHTYPE_DIRECTORY : SDL_PATHTYPE_OTHER;
	info->size = (Uint64)information.st_size;
	info->modify_time = (SDL_Time)information.st_mtim.tv_sec * 1000000000LL + information.st_mtim.tv_nsec;
	info->access_time = (SDL_Time)information.st_atim.tv_sec * 1000000000LL + information.st_atim.tv_nsec;
	info->create_time = (SDL_Time)information.st_ctim.tv_sec * 1000000000LL + information.st_ctim.tv_nsec;
	return true;
}

bool SDL_CloseIO(SDL_IOStream *stream)
{
	return stream && fclose((FILE *)stream) == 0;
}

size_t SDL_ReadIO(SDL_IOStream *stream, void *buffer, size_t size)
{
	return fread(buffer, 1, size, (FILE *)stream);
}

size_t SDL_WriteIO(SDL_IOStream *stream, const void *buffer, size_t size)
{
	return fwrite(buffer, 1, size, (FILE *)stream);
}

Sint64 SDL_SeekIO(SDL_IOStream *stream, Sint64 offset, SDL_IOWhence whence)
{
	int origin = whence == SDL_IO_SEEK_CUR ? SEEK_CUR : whence == SDL_IO_SEEK_END ? SEEK_END : SEEK_SET;

	if (fseeko((FILE *)stream, (off_t)offset, origin) != 0)
		return -1;
	return (Sint64)ftello((FILE *)stream);
}

Sint64 SDL_TellIO(SDL_IOStream *stream)
{
	return (Sint64)ftello((FILE *)stream);
}

Sint64 SDL_GetIOSize(SDL_IOStream *stream)
{
	struct stat information;

	if (fstat(fileno((FILE *)stream), &information) != 0)
		return -1;
	return (Sint64)information.st_size;
}

/* ---------- SDL_GlobDirectory: * and ? within a name, as SDL's */

static int glob_match(const char *pattern, const char *name)
{
	for (; *pattern; pattern++, name++)
	{
		if (*pattern == '*')
		{
			for (pattern++; ; name++)
			{
				if (glob_match(pattern, name))
					return 1;
				if (!*name)
					return 0;
			}
		}
		if (!*name || (*pattern != '?' && *pattern != *name))
			return 0;
	}
	return !*name;
}

struct glob_result
{
	char **names;
	int count, capacity;
	size_t text_size;
};

static void glob_add(struct glob_result *result, const char *name)
{
	if (result->count == result->capacity)
	{
		int capacity = result->capacity ? result->capacity * 2 : 16;
		char **names = realloc(result->names, (size_t)capacity * sizeof(char *));

		if (!names)
			return;
		result->names = names;
		result->capacity = capacity;
	}
	result->names[result->count] = strdup(name);
	if (result->names[result->count])
		result->text_size += strlen(name) + 1, result->count++;
}

/* the names under folder (relative to root, prefix) matching the pattern's
components from here on */
static void glob_folder(struct glob_result *result, const char *root, const char *prefix, const char *pattern)
{
	const char *slash = strchr(pattern, '/');
	size_t component_length = slash ? (size_t)(slash - pattern) : strlen(pattern);
	char component[256];
	char folder[1024];
	DIR *directory;
	struct dirent *entry;

	if (component_length >= sizeof(component))
		return;
	memcpy(component, pattern, component_length);
	component[component_length] = 0;
	snprintf(folder, sizeof(folder), "%s%s%s", root, *prefix ? "/" : "", prefix);
	directory = opendir(folder);
	if (!directory)
		return;
	while ((entry = readdir(directory)))
	{
		char name[1024];

		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") || !glob_match(component, entry->d_name))
			continue;
		snprintf(name, sizeof(name), "%s%s%s", prefix, *prefix ? "/" : "", entry->d_name);
		if (slash)
			glob_folder(result, root, name, slash + 1);
		else
			glob_add(result, name);
	}
	closedir(directory);
}

/* one allocation, the array then the names, which SDL_free frees */
char **SDL_GlobDirectory(const char *path, const char *pattern, SDL_GlobFlags flags, int *count)
{
	struct glob_result result = { NULL, 0, 0, 0 };
	char **names = NULL;
	int index;

	(void)flags;
	glob_folder(&result, path, "", pattern ? pattern : "*");
	names = malloc((size_t)(result.count + 1) * sizeof(char *) + result.text_size);
	if (names)
	{
		char *text = (char *)(names + result.count + 1);

		for (index = 0; index < result.count; index++)
		{
			size_t length = strlen(result.names[index]) + 1;

			memcpy(text, result.names[index], length);
			names[index] = text;
			text += length;
		}
		names[result.count] = NULL;
	}
	for (index = 0; index < result.count; index++)
		free(result.names[index]);
	free(result.names);
	if (count)
		*count = names ? result.count : 0;
	return names;
}

/* ---------- windows and displays */

#define WINDOW(window) ((unsigned int)(uintptr_t)(window))

SDL_WindowFlags SDL_GetWindowFlags(SDL_Window *window)
{
	return (SDL_WindowFlags)host_sdl_get_window_flags(WINDOW(window));
}

bool SDL_SetWindowFullscreen(SDL_Window *window, bool fullscreen)
{
	return host_sdl_set_window_fullscreen(WINDOW(window), fullscreen) != 0;
}

bool SDL_SetWindowFullscreenMode(SDL_Window *window, const SDL_DisplayMode *mode)
{
	if (!mode)
		return host_sdl_set_window_fullscreen_mode(WINDOW(window), 0, 0, 0, 0.0f) != 0;
	return host_sdl_set_window_fullscreen_mode(WINDOW(window), (unsigned int)mode->displayID, mode->w, mode->h,
		mode->refresh_rate) != 0;
}

bool SDL_GetWindowSize(SDL_Window *window, int *width, int *height)
{
	int w = 0, h = 0;

	host_sdl_get_window_size(WINDOW(window), &w, &h);
	if (width)
		*width = w;
	if (height)
		*height = h;
	return w > 0;
}

bool SDL_SetWindowSize(SDL_Window *window, int width, int height)
{
	return host_sdl_set_window_size(WINDOW(window), width, height) != 0;
}

void SDL_DestroyWindow(SDL_Window *window)
{
	host_sdl_destroy_window(WINDOW(window));
}

void SDL_WarpMouseInWindow(SDL_Window *window, float x, float y)
{
	host_sdl_warp_mouse_in_window(WINDOW(window), x, y);
}

bool SDL_SetWindowMouseGrab(SDL_Window *window, bool grabbed)
{
	return host_sdl_set_window_mouse_grab(WINDOW(window), grabbed) != 0;
}

SDL_Window *SDL_GetKeyboardFocus(void)
{
	return (SDL_Window *)(uintptr_t)host_sdl_get_keyboard_focus();
}

bool SDL_ShowCursor(void)
{
	host_sdl_cursor(1);
	return true;
}

bool SDL_HideCursor(void)
{
	host_sdl_cursor(0);
	return true;
}

bool SDL_CursorVisible(void)
{
	return host_sdl_cursor(-1) != 0;
}

const char *SDL_GetCurrentVideoDriver(void)
{
	static char name[64];

	host_sdl_get_current_video_driver(name, sizeof(name));
	return name[0] ? name : NULL;
}

/* A window's properties (sdl_platform.c's HDR): an id above the text
input's (below) that names the window, read from the host by name. */
#define WINDOW_PROPERTIES 0x10000u

SDL_PropertiesID SDL_GetWindowProperties(SDL_Window *window)
{
	return window ? (SDL_PropertiesID)(WINDOW_PROPERTIES + WINDOW(window)) : 0;
}

/* ---------- text input (Steam's on-screen keyboard, sdl_platform.c) */

bool SDL_HasScreenKeyboardSupport(void)
{
	return host_sdl_has_screen_keyboard_support() != 0;
}

bool SDL_GetHintBoolean(const char *name, bool default_value)
{
	return host_sdl_get_hint_boolean(name, default_value) != 0;
}

bool SDL_TextInputActive(SDL_Window *window)
{
	return host_sdl_text_input_active(WINDOW(window)) != 0;
}

bool SDL_StopTextInput(SDL_Window *window)
{
	return host_sdl_stop_text_input(WINDOW(window)) != 0;
}

/* The properties of text input, the only ones the guest sets: kept here and
handed to the host when text input starts (SDL_StartTextInputWithProperties).
Made and used on the window's thread, a few at a time. */
#define TEXT_INPUT_PROPERTIES_MAXIMUM 4

static struct
{
	bool used;
	bool multiline;
	Sint64 type;
} text_input_properties[TEXT_INPUT_PROPERTIES_MAXIMUM];

SDL_PropertiesID SDL_CreateProperties(void)
{
	int index;

	for (index = 0; index < TEXT_INPUT_PROPERTIES_MAXIMUM; index++)
	{
		if (!text_input_properties[index].used)
		{
			/* (SDL's defaults: more than one line, any text) */
			text_input_properties[index].used = true;
			text_input_properties[index].multiline = true;
			text_input_properties[index].type = SDL_TEXTINPUT_TYPE_TEXT;
			return (SDL_PropertiesID)(index + 1);
		}
	}
	return 0;
}

void SDL_DestroyProperties(SDL_PropertiesID properties)
{
	if (properties >= 1 && properties <= TEXT_INPUT_PROPERTIES_MAXIMUM)
		text_input_properties[properties - 1].used = false;
}

bool SDL_SetBooleanProperty(SDL_PropertiesID properties, const char *name, bool value)
{
	if (properties < 1 || properties > TEXT_INPUT_PROPERTIES_MAXIMUM || !name)
		return false;
	if (!strcmp(name, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN))
		text_input_properties[properties - 1].multiline = value;
	return true;
}

bool SDL_SetNumberProperty(SDL_PropertiesID properties, const char *name, Sint64 value)
{
	if (properties < 1 || properties > TEXT_INPUT_PROPERTIES_MAXIMUM || !name)
		return false;
	if (!strcmp(name, SDL_PROP_TEXTINPUT_TYPE_NUMBER))
		text_input_properties[properties - 1].type = value;
	return true;
}

bool SDL_GetBooleanProperty(SDL_PropertiesID properties, const char *name, bool default_value)
{
	if (properties > WINDOW_PROPERTIES && name)
		return host_sdl_get_window_property_boolean(properties - WINDOW_PROPERTIES, name, default_value) != 0;
	if (properties >= 1 && properties <= TEXT_INPUT_PROPERTIES_MAXIMUM && name &&
		!strcmp(name, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN))
		return text_input_properties[properties - 1].multiline;
	return default_value;
}

float SDL_GetFloatProperty(SDL_PropertiesID properties, const char *name, float default_value)
{
	if (properties > WINDOW_PROPERTIES && name)
		return host_sdl_get_window_property_float(properties - WINDOW_PROPERTIES, name, default_value);
	return default_value;
}

bool SDL_StartTextInputWithProperties(SDL_Window *window, SDL_PropertiesID properties)
{
	bool multiline = true;
	Sint64 type = SDL_TEXTINPUT_TYPE_TEXT;

	if (properties >= 1 && properties <= TEXT_INPUT_PROPERTIES_MAXIMUM)
	{
		multiline = text_input_properties[properties - 1].multiline;
		type = text_input_properties[properties - 1].type;
	}
	return host_sdl_start_text_input(WINDOW(window), multiline, (int)type) != 0;
}

SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window *window)
{
	return (SDL_DisplayID)host_sdl_get_display_for_window(WINDOW(window));
}

SDL_DisplayID SDL_GetPrimaryDisplay(void)
{
	return (SDL_DisplayID)host_sdl_get_primary_display();
}

/* host_desktop.c's struct host_display_mode */
struct display_mode
{
	unsigned int display;
	unsigned int format;
	int width, height;
	float pixel_density;
	float refresh_rate;
	int refresh_rate_numerator, refresh_rate_denominator;
};

static void display_mode_from_copy(SDL_DisplayMode *mode, const struct display_mode *copy)
{
	memset(mode, 0, sizeof(*mode));
	mode->displayID = (SDL_DisplayID)copy->display;
	mode->format = (SDL_PixelFormat)copy->format;
	mode->w = copy->width;
	mode->h = copy->height;
	mode->pixel_density = copy->pixel_density;
	mode->refresh_rate = copy->refresh_rate;
	mode->refresh_rate_numerator = copy->refresh_rate_numerator;
	mode->refresh_rate_denominator = copy->refresh_rate_denominator;
}

static const SDL_DisplayMode *display_mode(SDL_DisplayID display, int current)
{
	/* (SDL keeps the modes it returns; these last until the next call on
	this thread) */
	static __thread SDL_DisplayMode modes[2];
	SDL_DisplayMode *mode = &modes[current != 0];
	struct display_mode copy;

	if (!host_sdl_get_display_mode((unsigned int)display, current, &copy))
		return NULL;
	display_mode_from_copy(mode, &copy);
	return mode;
}

const SDL_DisplayMode *SDL_GetDesktopDisplayMode(SDL_DisplayID display)
{
	return display_mode(display, 0);
}

const SDL_DisplayMode *SDL_GetCurrentDisplayMode(SDL_DisplayID display)
{
	return display_mode(display, 1);
}

/* more than any display has */
#define FULLSCREEN_MODES_MAXIMUM 256

/* one allocation, the array then the modes, which SDL_free frees */
SDL_DisplayMode **SDL_GetFullscreenDisplayModes(SDL_DisplayID display, int *count)
{
	struct display_mode *copies = malloc(FULLSCREEN_MODES_MAXIMUM * sizeof(*copies));
	SDL_DisplayMode **result = NULL;
	SDL_DisplayMode *modes;
	int found = 0, index;

	if (copies)
	{
		found = host_sdl_get_fullscreen_display_modes((unsigned int)display, copies, FULLSCREEN_MODES_MAXIMUM);
		if (found > FULLSCREEN_MODES_MAXIMUM)
			found = FULLSCREEN_MODES_MAXIMUM;
		result = malloc((size_t)(found + 1) * sizeof(*result) + (size_t)found * sizeof(*modes));
	}
	if (result)
	{
		modes = (SDL_DisplayMode *)(result + found + 1);
		for (index = 0; index < found; index++)
		{
			display_mode_from_copy(&modes[index], &copies[index]);
			result[index] = &modes[index];
		}
		result[found] = NULL;
	}
	else
		found = 0;
	free(copies);
	if (count)
		*count = found;
	return result;
}

bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID display, int w, int h, float refresh_rate,
	bool include_high_density_modes, SDL_DisplayMode *closest)
{
	struct display_mode copy;

	if (!host_sdl_get_closest_fullscreen_display_mode((unsigned int)display, w, h, refresh_rate,
		include_high_density_modes, &copy))
	{
		return false;
	}
	display_mode_from_copy(closest, &copy);
	return true;
}

bool SDL_GetDisplayUsableBounds(SDL_DisplayID display, SDL_Rect *rect)
{
	return host_sdl_get_display_usable_bounds((unsigned int)display, rect) != 0;
}

/* ---------- the keyboard's layout (the controls' names, xinput_sdl.c) */

SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, SDL_Keymod modifiers, bool key_event)
{
	return (SDL_Keycode)host_sdl_get_key_from_scancode((int)scancode, (unsigned int)modifiers, key_event);
}

/* ---------- virtual gamepads (debug.virtual_gamepads, xinput_sdl.c): the
host's SDL joysticks, as handles. The name stays in guest memory, where SDL
keeps its pointer (the host reads guest memory at the same address). */

SDL_JoystickID SDL_AttachVirtualJoystick(const SDL_VirtualJoystickDesc *desc)
{
	if (!desc)
		return 0;
	return (SDL_JoystickID)host_sdl_attach_virtual_joystick((int)desc->type, desc->naxes, desc->nbuttons,
		desc->axis_mask, desc->button_mask, desc->name);
}

SDL_Joystick *SDL_OpenJoystick(SDL_JoystickID id)
{
	return (SDL_Joystick *)(uintptr_t)host_sdl_open_joystick((unsigned int)id);
}

bool SDL_SetJoystickVirtualButton(SDL_Joystick *joystick, int button, bool down)
{
	return host_sdl_set_joystick_virtual_button((unsigned int)(uintptr_t)joystick, button, down) != 0;
}

bool SDL_SetJoystickVirtualAxis(SDL_Joystick *joystick, int axis, Sint16 value)
{
	return host_sdl_set_joystick_virtual_axis((unsigned int)(uintptr_t)joystick, axis, value) != 0;
}

/* ---------- audio devices (Settings > Audio, sdl_platform.c) */

#define AUDIO_DEVICES_MAXIMUM 64
#define AUDIO_DEVICE_NAME_SIZE 256

/* an array of the devices' ids ending in 0, which SDL_free frees */
static SDL_AudioDeviceID *audio_devices(int recording, int *count)
{
	SDL_AudioDeviceID *result = malloc((AUDIO_DEVICES_MAXIMUM + 1) * sizeof(*result));
	int found;

	if (count)
		*count = 0;
	if (!result)
		return NULL;
	found = host_sdl_get_audio_devices(recording, (unsigned int *)result, AUDIO_DEVICES_MAXIMUM);
	if (found > AUDIO_DEVICES_MAXIMUM)
		found = AUDIO_DEVICES_MAXIMUM;
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

/* SDL keeps a device's name as long as the device: each id's is kept here
for good, so that the menus and the audio code can hold one while the other
asks for another */
const char *SDL_GetAudioDeviceName(SDL_AudioDeviceID device)
{
	static struct
	{
		SDL_AudioDeviceID device;
		char name[AUDIO_DEVICE_NAME_SIZE];
	} names[AUDIO_DEVICES_MAXIMUM];
	static int name_count;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	const char *result = NULL;
	int index;

	pthread_mutex_lock(&lock);
	for (index = 0; index < name_count && !result; index++)
	{
		if (names[index].device == device)
			result = names[index].name;
	}
	if (!result && name_count < AUDIO_DEVICES_MAXIMUM &&
		host_sdl_get_audio_device_name((unsigned int)device, names[name_count].name, AUDIO_DEVICE_NAME_SIZE))
	{
		names[name_count].device = device;
		result = names[name_count++].name;
	}
	pthread_mutex_unlock(&lock);
	return result;
}

/* ---------- the 2D renderer */

#define RENDERER(renderer) ((unsigned int)(uintptr_t)(renderer))

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, const char *name)
{
	return (SDL_Renderer *)(uintptr_t)host_sdl_create_renderer(WINDOW(window), name);
}

void SDL_DestroyRenderer(SDL_Renderer *renderer)
{
	host_sdl_destroy_renderer(RENDERER(renderer));
}

bool SDL_SetRenderVSync(SDL_Renderer *renderer, int vsync)
{
	return host_sdl_set_render_vsync(RENDERER(renderer), vsync) != 0;
}

bool SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
	return host_sdl_set_render_draw_color(RENDERER(renderer), red, green, blue, alpha) != 0;
}

bool SDL_SetRenderScale(SDL_Renderer *renderer, float x, float y)
{
	return host_sdl_set_render_scale(RENDERER(renderer), x, y) != 0;
}

bool SDL_RenderClear(SDL_Renderer *renderer)
{
	return host_sdl_render_clear(RENDERER(renderer)) != 0;
}

bool SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rectangle)
{
	return host_sdl_render_fill_rect(RENDERER(renderer), rectangle) != 0;
}

bool SDL_RenderDebugText(SDL_Renderer *renderer, float x, float y, const char *text)
{
	return host_sdl_render_debug_text(RENDERER(renderer), x, y, text) != 0;
}

bool SDL_RenderPresent(SDL_Renderer *renderer)
{
	return host_sdl_render_present(RENDERER(renderer)) != 0;
}

/* ---------- dialogs */

/* the host waits for the dialog, then the callback runs here, before this
returns (sdl_platform.c waits for it either way) */
void SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void *userdata, SDL_Window *window,
	const SDL_DialogFileFilter *filters, int filter_count, const char *default_location, bool allow_many)
{
	char names[512], patterns[512];
	size_t names_used = 0, patterns_used = 0;
	char path[1024];
	const char *files[2] = { path, NULL };
	int count = 0;
	int index;

	(void)window;
	(void)default_location;
	(void)allow_many;
	for (index = 0; index < filter_count && index < 8; index++)
	{
		size_t name_length = strlen(filters[index].name) + 1;
		size_t pattern_length = strlen(filters[index].pattern) + 1;

		if (names_used + name_length > sizeof(names) || patterns_used + pattern_length > sizeof(patterns))
			break;
		memcpy(names + names_used, filters[index].name, name_length);
		memcpy(patterns + patterns_used, filters[index].pattern, pattern_length);
		names_used += name_length;
		patterns_used += pattern_length;
		count++;
	}
	if (host_sdl_open_file_dialog(count, names, patterns, path, sizeof(path)))
		callback(userdata, files, -1);
	else
		callback(userdata, files + 1, -1);
}

/* ---------- the self-updater (update.h): the host's posix_update.c */

struct download_progress
{
	update_progress_proc progress;
	void *context;
};

/* called by the host on this thread (host_desktop.c, download_progress) */
static void download_progress(unsigned int context, unsigned int received, unsigned int total)
{
	struct download_progress *binding = (struct download_progress *)(uintptr_t)context;

	binding->progress(binding->context, received, total);
}

/* host_desktop.c's struct host_download: 32-bit members only, and one
pointer to it, so that every argument of the call has a register */
struct download
{
	unsigned int url;
	unsigned int path;
	/* the most bytes taken (a release is far below 4 GB) */
	unsigned int maximum;
	unsigned int progress;
	unsigned int context;
	unsigned int error;
	int error_size;
};

typedef char download_size_check[sizeof(struct download) == 28 ? 1 : -1];

int update_download(const char *url, const char *path, unsigned long long maximum, update_progress_proc progress,
	void *context, char *error, int error_size)
{
	struct download_progress binding = { progress, context };
	struct download download;

	download.url = (unsigned int)(uintptr_t)url;
	download.path = (unsigned int)(uintptr_t)path;
	download.maximum = maximum > 0xFFFFFFFFull ? 0xFFFFFFFFu : (unsigned int)maximum;
	download.progress = progress ? (unsigned int)(uintptr_t)download_progress : 0;
	download.context = (unsigned int)(uintptr_t)&binding;
	download.error = (unsigned int)(uintptr_t)error;
	download.error_size = error_size;
	return host_update_download(&download);
}

/* host_desktop.c's struct host_https_request: the request, and the guest's
buffers the host writes the answer into (the guest frees what it malloc'd,
which the host's malloc'd body could not be) */
struct https_request
{
	unsigned int method;
	unsigned int url;
	unsigned int headers;
	unsigned int body;
	unsigned int body_size;
	unsigned int maximum;
	/* the answer's headers (size bytes, NUL terminated) and body (maximum
	bytes, then a NUL) */
	unsigned int answer_headers;
	unsigned int answer_headers_size;
	unsigned int answer_body;
	int status;
	unsigned int answer_body_size;
	unsigned int error;
	int error_size;
};

typedef char https_request_size_check[sizeof(struct https_request) == 52 ? 1 : -1];

int update_https_request(const char *method, const char *url, const char *headers, const void *body,
	unsigned long body_size, unsigned long maximum, struct update_response *response, char *error, int error_size)
{
	struct https_request request;
	char *answer;

	memset(response, 0, sizeof(*response));
	if (maximum >= 0x7FFFFFFFul || !(answer = malloc(maximum + 1)))
	{
		if (error_size > 0)
			snprintf(error, (size_t)error_size, "no memory for an answer of %lu bytes", maximum);
		return 0;
	}
	request.method = (unsigned int)(uintptr_t)method;
	request.url = (unsigned int)(uintptr_t)url;
	request.headers = (unsigned int)(uintptr_t)headers;
	request.body = (unsigned int)(uintptr_t)body;
	request.body_size = (unsigned int)body_size;
	request.maximum = (unsigned int)maximum;
	request.answer_headers = (unsigned int)(uintptr_t)response->headers;
	request.answer_headers_size = (unsigned int)sizeof(response->headers);
	request.answer_body = (unsigned int)(uintptr_t)answer;
	request.status = 0;
	request.answer_body_size = 0;
	request.error = (unsigned int)(uintptr_t)error;
	request.error_size = error_size;
	if (!host_update_https_request(&request) || request.answer_body_size > maximum)
	{
		free(answer);
		return 0;
	}
	answer[request.answer_body_size] = 0;
	response->status = request.status;
	response->body = answer;
	response->body_size = request.answer_body_size;
	return 1;
}

int update_executable_path(char *path, int size)
{
	return host_update_executable_path(path, size);
}

int update_replace_file(const char *path, const char *new_path, const char *old_path)
{
	return host_update_replace_file(path, new_path, old_path);
}

void update_delete_file(const char *path)
{
	host_update_delete_file(path);
}

int update_make_directory(const char *path)
{
	return host_update_make_directory(path);
}

int update_launch(const char *path)
{
	return host_update_launch(path);
}
