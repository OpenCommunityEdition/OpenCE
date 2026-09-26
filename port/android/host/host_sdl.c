/*
HOST_SDL.C

SDL3 on behalf of the guest (guest/runtime/guest_sdl.c). SDL objects are
64-bit pointers, which the guest cannot hold; it gets small handles into the
table here instead.

SDL may call into Java, and ART checks the stack pointer on every such call,
so each service runs on the calling thread's own stack rather than the guest
stack it is called on: the exported functions pass their arguments to a
*_native implementation through HOST_NATIVE (host_thread.c).
*/

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

static void *handle_get(uint64_t handle, int type)
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

#define POINTER(argument) ((void *)(uintptr_t)(argument))

/* ---------- general */

static uint64_t init_native(uint64_t flags, uint64_t b, uint64_t c, uint64_t d)
{
	return SDL_Init((SDL_InitFlags)flags);
}

int host_sdl_init(uint32_t flags)
{
	return (int)HOST_NATIVE(init_native, flags, 0, 0, 0);
}

static uint64_t set_hint_native(uint64_t name, uint64_t value, uint64_t c, uint64_t d)
{
	return SDL_SetHint(POINTER(name), POINTER(value));
}

int host_sdl_set_hint(const char *name, const char *value)
{
	return (int)HOST_NATIVE(set_hint_native, (uintptr_t)name, (uintptr_t)value, 0, 0);
}

static uint64_t get_error_native(uint64_t buffer, uint64_t size, uint64_t c, uint64_t d)
{
	SDL_strlcpy(POINTER(buffer), SDL_GetError(), (size_t)size);
	return 0;
}

void host_sdl_get_error(char *buffer, uint32_t size)
{
	HOST_NATIVE(get_error_native, (uintptr_t)buffer, size, 0, 0);
}

int64_t host_sdl_ticks(void)
{
	return (int64_t)SDL_GetTicks();
}

int64_t host_sdl_thread_id(void)
{
	return (int64_t)SDL_GetCurrentThreadID();
}

/* ---------- video */

static uint64_t create_window_native(uint64_t title, uint64_t width, uint64_t height, uint64_t flags)
{
	return handle_new(_handle_window, SDL_CreateWindow(POINTER(title), (int)width, (int)height, (SDL_WindowFlags)flags));
}

uint32_t host_sdl_create_window(const char *title, int width, int height, int64_t flags)
{
	return (uint32_t)HOST_NATIVE(create_window_native, (uintptr_t)title, width, height, flags);
}

static uint64_t window_size_native(uint64_t window, uint64_t width, uint64_t height, uint64_t d)
{
	SDL_Window *object = handle_get(window, _handle_window);
	int *w = POINTER(width), *h = POINTER(height);

	*w = 0;
	*h = 0;
	if (object)
		SDL_GetWindowSizeInPixels(object, w, h);
	return 0;
}

void host_sdl_window_size_in_pixels(uint32_t window, int *width, int *height)
{
	HOST_NATIVE(window_size_native, window, (uintptr_t)width, (uintptr_t)height, 0);
}

static uint64_t relative_mouse_native(uint64_t window, uint64_t enabled, uint64_t c, uint64_t d)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_SetWindowRelativeMouseMode(object, enabled != 0) : 0;
}

int host_sdl_set_relative_mouse(uint32_t window, int enabled)
{
	return (int)HOST_NATIVE(relative_mouse_native, window, enabled, 0, 0);
}

static uint64_t gl_set_attribute_native(uint64_t attribute, uint64_t value, uint64_t c, uint64_t d)
{
	return SDL_GL_SetAttribute((SDL_GLAttr)attribute, (int)value);
}

int host_sdl_gl_set_attribute(int attribute, int value)
{
	return (int)HOST_NATIVE(gl_set_attribute_native, (uint32_t)attribute, (uint32_t)value, 0, 0);
}

static uint64_t gl_create_context_native(uint64_t window, uint64_t b, uint64_t c, uint64_t d)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? handle_new(_handle_context, SDL_GL_CreateContext(object)) : 0;
}

uint32_t host_sdl_gl_create_context(uint32_t window)
{
	return (uint32_t)HOST_NATIVE(gl_create_context_native, window, 0, 0, 0);
}

static uint64_t gl_make_current_native(uint64_t window, uint64_t context, uint64_t c, uint64_t d)
{
	return SDL_GL_MakeCurrent(handle_get(window, _handle_window), handle_get(context, _handle_context));
}

int host_sdl_gl_make_current(uint32_t window, uint32_t context)
{
	return (int)HOST_NATIVE(gl_make_current_native, window, context, 0, 0);
}

static uint64_t gl_swap_interval_native(uint64_t interval, uint64_t b, uint64_t c, uint64_t d)
{
	return SDL_GL_SetSwapInterval((int)interval);
}

int host_sdl_gl_set_swap_interval(int interval)
{
	return (int)HOST_NATIVE(gl_swap_interval_native, (uint32_t)interval, 0, 0, 0);
}

static uint64_t gl_swap_native(uint64_t window, uint64_t b, uint64_t c, uint64_t d)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_GL_SwapWindow(object) : 0;
}

int host_sdl_gl_swap_window(uint32_t window)
{
	return (int)HOST_NATIVE(gl_swap_native, window, 0, 0, 0);
}

/* ---------- events */

static uint64_t poll_event_native(uint64_t event, uint64_t b, uint64_t c, uint64_t d)
{
	SDL_Event host_event;

	if (!SDL_PollEvent(&host_event))
		return 0;
	/* the layouts agree except for the pointers of text, drop and user
	events, which the guest does not read */
	memcpy(POINTER(event), &host_event, sizeof(host_event));
	return 1;
}

int host_sdl_poll_event(void *event)
{
	return (int)HOST_NATIVE(poll_event_native, (uintptr_t)event, 0, 0, 0);
}

/* ---------- gamepads */

static uint64_t get_gamepads_native(uint64_t ids, uint64_t capacity, uint64_t c, uint64_t d)
{
	int count = 0, index;
	SDL_JoystickID *list = SDL_GetGamepads(&count);
	uint32_t *result = POINTER(ids);

	if (!list)
		return 0;
	if (count > (int)capacity)
		count = (int)capacity;
	for (index = 0; index < count; index++)
		result[index] = list[index];
	SDL_free(list);
	return (uint64_t)count;
}

int host_sdl_get_gamepads(uint32_t *ids, int capacity)
{
	return (int)HOST_NATIVE(get_gamepads_native, (uintptr_t)ids, (uint32_t)capacity, 0, 0);
}

static uint64_t open_gamepad_native(uint64_t id, uint64_t b, uint64_t c, uint64_t d)
{
	SDL_Gamepad *gamepad = SDL_OpenGamepad((SDL_JoystickID)id);

	if (gamepad)
		host_logf(HOST_LOG_INFO, "gamepad %u: %s (type %d, %04x:%04x)", (unsigned)id, SDL_GetGamepadName(gamepad),
			(int)SDL_GetGamepadType(gamepad), SDL_GetGamepadVendor(gamepad), SDL_GetGamepadProduct(gamepad));
	return handle_new(_handle_gamepad, gamepad);
}

uint32_t host_sdl_open_gamepad(uint32_t id)
{
	return (uint32_t)HOST_NATIVE(open_gamepad_native, id, 0, 0, 0);
}

static uint64_t gamepad_from_id_native(uint64_t id, uint64_t b, uint64_t c, uint64_t d)
{
	return handle_new(_handle_gamepad, SDL_GetGamepadFromID((SDL_JoystickID)id));
}

uint32_t host_sdl_gamepad_from_id(uint32_t id)
{
	return (uint32_t)HOST_NATIVE(gamepad_from_id_native, id, 0, 0, 0);
}

static uint64_t gamepad_axis_native(uint64_t gamepad, uint64_t axis, uint64_t c, uint64_t d)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return (uint64_t)(int64_t)(object ? SDL_GetGamepadAxis(object, (SDL_GamepadAxis)axis) : 0);
}

int host_sdl_gamepad_axis(uint32_t gamepad, int axis)
{
	return (int)HOST_NATIVE(gamepad_axis_native, gamepad, (uint32_t)axis, 0, 0);
}

static uint64_t gamepad_button_native(uint64_t gamepad, uint64_t button, uint64_t c, uint64_t d)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return object ? SDL_GetGamepadButton(object, (SDL_GamepadButton)button) : 0;
}

int host_sdl_gamepad_button(uint32_t gamepad, int button)
{
	return (int)HOST_NATIVE(gamepad_button_native, gamepad, (uint32_t)button, 0, 0);
}

static uint64_t gamepad_type_native(uint64_t gamepad, uint64_t b, uint64_t c, uint64_t d)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return (uint64_t)(object ? SDL_GetGamepadType(object) : SDL_GAMEPAD_TYPE_UNKNOWN);
}

int host_sdl_gamepad_type(uint32_t gamepad)
{
	return (int)HOST_NATIVE(gamepad_type_native, gamepad, 0, 0, 0);
}

static uint64_t rumble_native(uint64_t gamepad, uint64_t low, uint64_t high, uint64_t milliseconds)
{
	SDL_Gamepad *object = handle_get(gamepad, _handle_gamepad);

	return object ? SDL_RumbleGamepad(object, (Uint16)low, (Uint16)high, (Uint32)milliseconds) : 0;
}

int host_sdl_rumble_gamepad(uint32_t gamepad, uint32_t low, uint32_t high, uint32_t milliseconds)
{
	return (int)HOST_NATIVE(rumble_native, gamepad, low, high, milliseconds);
}

/* ---------- audio */

struct audio_binding
{
	uint32_t handle;
	uint32_t callback;
	uint32_t userdata;
};

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional, int total)
{
	struct audio_binding *binding = userdata;

	(void)stream;
	host_call_guest(binding->callback, binding->userdata, binding->handle, (uint32_t)additional, (uint32_t)total);
}

static uint64_t open_audio_native(uint64_t device, uint64_t spec, uint64_t callback, uint64_t userdata)
{
	struct audio_binding *binding = SDL_calloc(1, sizeof(*binding));
	SDL_AudioStream *stream;

	binding->callback = (uint32_t)callback;
	binding->userdata = (uint32_t)userdata;
	stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)device, POINTER(spec),
		callback ? audio_callback : NULL, binding);
	if (!stream)
	{
		SDL_free(binding);
		return 0;
	}
	/* the device starts paused, so no callback can run before this */
	binding->handle = handle_new(_handle_audio, stream);
	return binding->handle;
}

uint32_t host_sdl_open_audio_stream(uint32_t device, const void *spec, uint32_t callback, uint32_t userdata)
{
	return (uint32_t)HOST_NATIVE(open_audio_native, device, (uintptr_t)spec, callback, userdata);
}

static uint64_t put_audio_native(uint64_t stream, uint64_t data, uint64_t length, uint64_t d)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	return object ? SDL_PutAudioStreamData(object, POINTER(data), (int)length) : 0;
}

int host_sdl_put_audio_stream_data(uint32_t stream, const void *data, int length)
{
	return (int)HOST_NATIVE(put_audio_native, stream, (uintptr_t)data, (uint32_t)length, 0);
}

static uint64_t resume_audio_native(uint64_t stream, uint64_t b, uint64_t c, uint64_t d)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	return object ? SDL_ResumeAudioStreamDevice(object) : 0;
}

int host_sdl_resume_audio_stream_device(uint32_t stream)
{
	return (int)HOST_NATIVE(resume_audio_native, stream, 0, 0, 0);
}
