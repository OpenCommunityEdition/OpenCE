#include "host.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <string.h>

#define HANDLE_COUNT 256

enum handle_type
{
	_handle_free,
	_handle_window,
	_handle_context,
	_handle_gamepad,
	_handle_audio,
	_handle_renderer,
};

struct handle
{
	int type;
	void *object;
	void *binding;
};

static struct handle handles[HANDLE_COUNT];
static pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t handle_new(int type, void *object)
{
	uint32_t index;

	if (!object)
		return 0;
	pthread_mutex_lock(&handle_lock);
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
			handles[index].binding = NULL;
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

static void *handle_release(uint32_t handle, int type, void **binding)
{
	void *object = NULL;

	if (binding)
		*binding = NULL;
	if (handle == 0 || handle >= HANDLE_COUNT)
		return NULL;
	pthread_mutex_lock(&handle_lock);
	if (handles[handle].type == type)
	{
		object = handles[handle].object;
		if (binding)
			*binding = handles[handle].binding;
		handles[handle].type = _handle_free;
		handles[handle].object = NULL;
		handles[handle].binding = NULL;
	}
	pthread_mutex_unlock(&handle_lock);
	return object;
}

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

int64_t host_sdl_thread_id(void)
{
	return (int64_t)SDL_GetCurrentThreadID();
}

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

int host_sdl_set_relative_mouse(uint32_t window, int enabled)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_SetWindowRelativeMouseMode(object, enabled != 0) : 0;
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

int host_sdl_poll_event(void *event)
{
	SDL_Event host_event;

	if (!SDL_PollEvent(&host_event))
		return 0;
	memcpy(event, &host_event, sizeof(host_event));
	return 1;
}

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

void host_sdl_close_gamepad(uint32_t gamepad)
{
	SDL_Gamepad *object = handle_release(gamepad, _handle_gamepad, NULL);

	if (object)
		SDL_CloseGamepad(object);
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

struct audio_binding
{
	uint32_t handle;
	uint32_t callback;
	uint32_t userdata;
	pthread_mutex_t lock;
	pthread_cond_t requested;
	pthread_cond_t done;
	int pending;
	int quit;
	int additional;
	int total;
	unsigned char *buffer;
	int buffer_length;
	int buffer_size;
};

static __thread struct audio_binding *calling_back;

static void *audio_thread(void *context)
{
	struct audio_binding *binding = context;

	pthread_mutex_lock(&binding->lock);
	for (;;)
	{
		int additional, total;

		while (!binding->pending && !binding->quit)
			pthread_cond_wait(&binding->requested, &binding->lock);
		if (binding->quit)
			break;
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
	pthread_mutex_unlock(&binding->lock);
	pthread_cond_destroy(&binding->done);
	pthread_cond_destroy(&binding->requested);
	pthread_mutex_destroy(&binding->lock);
	SDL_free(binding->buffer);
	SDL_free(binding);
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
	struct audio_binding *binding;
	SDL_AudioStream *stream;

	if (!callback)
	{
		stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)device, spec, NULL, NULL);
		return stream ? handle_new(_handle_audio, stream) : 0;
	}
	binding = SDL_calloc(1, sizeof(*binding));
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
	binding->handle = handle_new(_handle_audio, stream);
	if (!binding->handle)
	{
		SDL_DestroyAudioStream(stream);
		pthread_cond_destroy(&binding->done);
		pthread_cond_destroy(&binding->requested);
		pthread_mutex_destroy(&binding->lock);
		SDL_free(binding);
		return 0;
	}
	pthread_mutex_lock(&handle_lock);
	handles[binding->handle].binding = binding;
	pthread_mutex_unlock(&handle_lock);
	if (host_native_thread_create(audio_thread, binding, 256 * 1024) != 0)
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

int host_sdl_get_audio_stream_data(uint32_t stream, void *data, int length)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	return object ? SDL_GetAudioStreamData(object, data, length) : -1;
}

int host_sdl_get_audio_stream_available(uint32_t stream)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	return object ? SDL_GetAudioStreamAvailable(object) : -1;
}

void host_sdl_destroy_audio_stream(uint32_t stream)
{
	void *context;
	SDL_AudioStream *object = handle_release(stream, _handle_audio, &context);
	struct audio_binding *binding = context;

	if (!object)
		return;
	SDL_DestroyAudioStream(object);
	if (binding)
	{
		pthread_mutex_lock(&binding->lock);
		binding->quit = 1;
		pthread_cond_signal(&binding->requested);
		pthread_mutex_unlock(&binding->lock);
	}
}

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

int host_sdl_show_simple_message_box(uint32_t flags, const char *title, const char *message)
{
	return SDL_ShowSimpleMessageBox((SDL_MessageBoxFlags)flags, title, message, NULL) ? 1 : 0;
}

int host_sdl_show_message_box(uint32_t flags, const char *title, const char *message, int default_index,
	const char *button0, const char *button1, const char *button2)
{
	const char *labels[3] = { button0, button1, button2 };
	SDL_MessageBoxButtonData buttons[3];
	SDL_MessageBoxData question;
	int count = 0;
	int answer = -1;
	int index;

	for (index = 0; index < 3; index++)
	{
		if (!labels[index])
			continue;
		buttons[count].flags = index == default_index ?
			SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT : 0;
		buttons[count].buttonID = index;
		buttons[count].text = labels[index];
		count++;
	}
	question.flags = (SDL_MessageBoxFlags)flags;
	question.window = NULL;
	question.title = title;
	question.message = message;
	question.numbuttons = count;
	question.buttons = buttons;
	question.colorScheme = NULL;
	return SDL_ShowMessageBox(&question, &answer) ? answer : -1;
}

int64_t host_sdl_ticks_ns(void)
{
	return (int64_t)SDL_GetTicksNS();
}

void host_sdl_pump_events(void)
{
	SDL_PumpEvents();
}

int host_sdl_push_event(void *event)
{
	SDL_Event host_event;

	memcpy(&host_event, event, sizeof(host_event));
	return SDL_PushEvent(&host_event);
}

uint32_t host_sdl_key_from_scancode(int scancode, int modifiers, int key_event)
{
	return SDL_GetKeyFromScancode((SDL_Scancode)scancode, (SDL_Keymod)modifiers, key_event != 0);
}

int host_sdl_get_path_info(const char *path, SDL_PathInfo *info)
{
	return SDL_GetPathInfo(path, info);
}

int host_sdl_glob_directory(const char *path, const char *pattern, uint32_t flags, char *buffer, int size)
{
	int count = 0, index, used = 0;
	char **names = SDL_GlobDirectory(path, pattern, (SDL_GlobFlags)flags, &count);

	for (index = 0; names && index < count; index++)
	{
		int length = (int)strlen(names[index]) + 1;

		if (used + length > size)
			break;
		memcpy(buffer + used, names[index], (size_t)length);
		used += length;
	}
	SDL_free(names);
	return index;
}

int host_sdl_audio_devices(int recording, uint32_t *ids, int capacity)
{
	int count = 0, index;
	SDL_AudioDeviceID *list = recording ? SDL_GetAudioRecordingDevices(&count) : SDL_GetAudioPlaybackDevices(&count);

	if (!list)
		return 0;
	for (index = 0; index < count && index < capacity; index++)
		ids[index] = list[index];
	SDL_free(list);
	return index;
}

void host_sdl_audio_device_name(uint32_t id, char *buffer, uint32_t size)
{
	const char *name = SDL_GetAudioDeviceName((SDL_AudioDeviceID)id);

	SDL_strlcpy(buffer, name ? name : "", size);
}

int64_t host_sdl_window_flags(uint32_t window)
{
	return (int64_t)SDL_GetWindowFlags(handle_get(window, _handle_window));
}

int host_sdl_set_window_fullscreen(uint32_t window, int fullscreen)
{
	return SDL_SetWindowFullscreen(handle_get(window, _handle_window), fullscreen != 0);
}

int host_sdl_set_window_size(uint32_t window, int width, int height)
{
	return SDL_SetWindowSize(handle_get(window, _handle_window), width, height);
}

int host_sdl_window_size(uint32_t window, int *width, int *height)
{
	return SDL_GetWindowSize(handle_get(window, _handle_window), width, height);
}

void host_sdl_destroy_window(uint32_t window)
{
	SDL_Window *object = handle_release(window, _handle_window, NULL);

	if (object)
		SDL_DestroyWindow(object);
}

uint32_t host_sdl_keyboard_focus(void)
{
	return handle_new(_handle_window, SDL_GetKeyboardFocus());
}

int host_sdl_set_window_mouse_grab(uint32_t window, int grabbed)
{
	return SDL_SetWindowMouseGrab(handle_get(window, _handle_window), grabbed != 0);
}

void host_sdl_warp_mouse(uint32_t window, float x, float y)
{
	SDL_Window *object = handle_get(window, _handle_window);

	if (object)
		SDL_WarpMouseInWindow(object, x, y);
}

int host_sdl_show_cursor(int shown)
{
	return shown ? SDL_ShowCursor() : SDL_HideCursor();
}

#define DISPLAY_MODE_SIZE offsetof(SDL_DisplayMode, internal)

uint32_t host_sdl_primary_display(void)
{
	return SDL_GetPrimaryDisplay();
}

uint32_t host_sdl_display_for_window(uint32_t window)
{
	return SDL_GetDisplayForWindow(handle_get(window, _handle_window));
}

int host_sdl_display_usable_bounds(uint32_t display, SDL_Rect *rect)
{
	return SDL_GetDisplayUsableBounds((SDL_DisplayID)display, rect);
}

static void display_mode_out(const SDL_DisplayMode *mode, void *guest_mode, int index)
{
	unsigned char *out = (unsigned char *)guest_mode + index * (DISPLAY_MODE_SIZE + 4);

	memcpy(out, mode, DISPLAY_MODE_SIZE);
	memset(out + DISPLAY_MODE_SIZE, 0, 4);
}

int host_sdl_display_mode(uint32_t display, int current, void *mode)
{
	const SDL_DisplayMode *found = current ? SDL_GetCurrentDisplayMode((SDL_DisplayID)display) :
		SDL_GetDesktopDisplayMode((SDL_DisplayID)display);

	if (found)
		display_mode_out(found, mode, 0);
	return found != NULL;
}

int host_sdl_fullscreen_modes(uint32_t display, void *modes, int capacity)
{
	int count = 0, index;
	SDL_DisplayMode **list = SDL_GetFullscreenDisplayModes((SDL_DisplayID)display, &count);

	if (!list)
		return 0;
	for (index = 0; index < count && index < capacity; index++)
		display_mode_out(list[index], modes, index);
	SDL_free(list);
	return index;
}

int host_sdl_closest_fullscreen_mode(uint32_t display, int width, int height, float refresh, int high_density,
	void *mode)
{
	SDL_DisplayMode closest;

	if (!SDL_GetClosestFullscreenDisplayMode((SDL_DisplayID)display, width, height, refresh, high_density != 0,
		&closest))
	{
		return 0;
	}
	display_mode_out(&closest, mode, 0);
	return 1;
}

int host_sdl_set_window_fullscreen_mode(uint32_t window, const void *mode)
{
	SDL_Window *object = handle_get(window, _handle_window);
	SDL_DisplayMode wanted, closest;

	if (!object)
		return 0;
	if (!mode)
		return SDL_SetWindowFullscreenMode(object, NULL);
	memcpy(&wanted, mode, DISPLAY_MODE_SIZE);
	if (!SDL_GetClosestFullscreenDisplayMode(wanted.displayID, wanted.w, wanted.h, wanted.refresh_rate,
		wanted.pixel_density > 1.0f, &closest))
	{
		return 0;
	}
	return SDL_SetWindowFullscreenMode(object, &closest);
}

static struct
{
	SDL_DialogFileFilter filters[4];
	char text[512];
	char path[1024];
	SDL_AtomicInt state;
} dialog;

static void SDLCALL dialog_chosen(void *userdata, const char *const *files, int filter)
{
	(void)userdata;
	(void)filter;
	if (files && files[0])
		SDL_strlcpy(dialog.path, files[0], sizeof(dialog.path));
	SDL_SetAtomicInt(&dialog.state, !files ? 2 : files[0] ? 1 : 0);
}

void host_sdl_open_file_dialog(const char *filters)
{
	char *line;
	int count = 0;

	SDL_strlcpy(dialog.text, filters, sizeof(dialog.text));
	for (line = dialog.text; *line && count < 4; count++)
	{
		char *tab = strchr(line, '\t'), *end = strchr(line, '\n');

		if (!tab || !end)
			break;
		*tab = *end = 0;
		dialog.filters[count].name = line;
		dialog.filters[count].pattern = tab + 1;
		line = end + 1;
	}
	SDL_SetAtomicInt(&dialog.state, -1);
	SDL_ShowOpenFileDialog(dialog_chosen, NULL, NULL, dialog.filters, count, NULL, false);
}

int host_sdl_file_dialog_result(char *path, uint32_t size)
{
	int state = SDL_GetAtomicInt(&dialog.state);

	if (state == 1)
		SDL_strlcpy(path, dialog.path, size);
	return state;
}

uint32_t host_sdl_create_renderer(uint32_t window)
{
	return handle_new(_handle_renderer, SDL_CreateRenderer(handle_get(window, _handle_window), NULL));
}

void host_sdl_destroy_renderer(uint32_t renderer)
{
	SDL_Renderer *object = handle_release(renderer, _handle_renderer, NULL);

	if (object)
		SDL_DestroyRenderer(object);
}

int host_sdl_render(uint32_t renderer, int operation, float x, float y, const char *text, const SDL_FRect *rect)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	if (!object)
		return 0;
	switch (operation)
	{
	case 0:
		return SDL_SetRenderVSync(object, (int)x);
	case 1:
		return SDL_SetRenderDrawColor(object, (Uint8)rect->x, (Uint8)rect->y, (Uint8)rect->w, (Uint8)rect->h);
	case 2:
		return SDL_RenderClear(object);
	case 3:
		return SDL_SetRenderScale(object, x, y);
	case 4:
		return SDL_RenderDebugText(object, x, y, text);
	case 5:
		return SDL_RenderFillRect(object, rect);
	default:
		return SDL_RenderPresent(object);
	}
}
