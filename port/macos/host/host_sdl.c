/*
HOST_SDL.C

SDL3 on behalf of the guest (port/android/guest/runtime/guest_sdl.c and
port/macos/guest/guest_sdl_desktop.c). SDL objects are 64-bit pointers,
which the guest cannot hold; it gets small handles into the table here
instead. Its pointers arrive as guest addresses, taken here as 32-bit
values (G2H).

The guest calls these on its own threads: its main thread, which is the
process's first (Cocoa's windows and events want it), and the others, all
with stacks in guest memory (host_thread.c). SDL's audio thread is the
exception: it has no guest stack, so the audio callback is handed to a
thread that has one.
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
	_handle_renderer,
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

static void handle_free(uint32_t handle)
{
	if (handle == 0 || handle >= HANDLE_COUNT)
		return;
	pthread_mutex_lock(&handle_lock);
	handles[handle].type = _handle_free;
	handles[handle].object = NULL;
	pthread_mutex_unlock(&handle_lock);
}

static void copy_out(uint32_t buffer, uint32_t size, const char *text)
{
	if (buffer && size)
		SDL_strlcpy(G2H(buffer), text ? text : "", size);
}

/* ---------- general */

int host_sdl_init(uint32_t flags)
{
	return SDL_Init((SDL_InitFlags)flags);
}

int host_sdl_set_hint(uint32_t name, uint32_t value)
{
	return SDL_SetHint(G2H(name), G2H(value));
}

void host_sdl_get_error(uint32_t buffer, uint32_t size)
{
	copy_out(buffer, size, SDL_GetError());
}

void host_sdl_scancode_name(int32_t scancode, uint32_t buffer, uint32_t size)
{
	copy_out(buffer, size, SDL_GetScancodeName((SDL_Scancode)scancode));
}

int32_t host_sdl_scancode_from_name(uint32_t name)
{
	return (int32_t)SDL_GetScancodeFromName(G2H(name));
}

int64_t host_sdl_ticks(void)
{
	return (int64_t)SDL_GetTicks();
}

int64_t host_sdl_ticks_ns(void)
{
	return (int64_t)SDL_GetTicksNS();
}

int64_t host_sdl_thread_id(void)
{
	return (int64_t)SDL_GetCurrentThreadID();
}

/* ---------- video */

uint32_t host_sdl_create_window(uint32_t title, int width, int height, int64_t flags)
{
	return handle_new(_handle_window, SDL_CreateWindow(G2H(title), width, height, (SDL_WindowFlags)flags));
}

void host_sdl_destroy_window(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	if (!object)
		return;
	handle_free(window);
	SDL_DestroyWindow(object);
}

void host_sdl_window_size_in_pixels(uint32_t window, uint32_t width, uint32_t height)
{
	SDL_Window *object = handle_get(window, _handle_window);
	int w = 0, h = 0;

	if (object)
		SDL_GetWindowSizeInPixels(object, &w, &h);
	*(int *)G2H(width) = w;
	*(int *)G2H(height) = h;
}

void host_sdl_window_size(uint32_t window, uint32_t width, uint32_t height)
{
	SDL_Window *object = handle_get(window, _handle_window);
	int w = 0, h = 0;

	if (object)
		SDL_GetWindowSize(object, &w, &h);
	*(int *)G2H(width) = w;
	*(int *)G2H(height) = h;
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

int host_sdl_set_window_fullscreen_mode(uint32_t window, uint32_t mode)
{
	SDL_Window *object = handle_get(window, _handle_window);
	const struct halo_macos_display_mode *wanted = G2H(mode);
	SDL_DisplayMode closest;

	if (!object)
		return 0;
	if (!wanted)
		return SDL_SetWindowFullscreenMode(object, NULL);
	/* (one of SDL's own modes, found again by its size and rate) */
	if (!SDL_GetClosestFullscreenDisplayMode(wanted->display, wanted->width, wanted->height, wanted->refresh_rate,
		wanted->pixel_density > 1.0f, &closest))
	{
		return 0;
	}
	return SDL_SetWindowFullscreenMode(object, &closest);
}

void host_sdl_warp_mouse(uint32_t window, float x, float y)
{
	SDL_Window *object = handle_get(window, _handle_window);

	if (object)
		SDL_WarpMouseInWindow(object, x, y);
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

/* ---------- displays */

static void mode_out(struct halo_macos_display_mode *result, const SDL_DisplayMode *mode)
{
	result->display = mode->displayID;
	result->format = (uint32_t)mode->format;
	result->width = mode->w;
	result->height = mode->h;
	result->pixel_density = mode->pixel_density;
	result->refresh_rate = mode->refresh_rate;
	result->refresh_numerator = mode->refresh_rate_numerator;
	result->refresh_denominator = mode->refresh_rate_denominator;
}

uint32_t host_sdl_primary_display(void)
{
	return SDL_GetPrimaryDisplay();
}

uint32_t host_sdl_display_for_window(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? SDL_GetDisplayForWindow(object) : 0;
}

int host_sdl_display_mode(uint32_t display, int which, uint32_t mode)
{
	const SDL_DisplayMode *found = which ? SDL_GetCurrentDisplayMode(display) : SDL_GetDesktopDisplayMode(display);

	if (!found)
		return 0;
	mode_out(G2H(mode), found);
	return 1;
}

int host_sdl_fullscreen_display_modes(uint32_t display, uint32_t modes, int capacity)
{
	struct halo_macos_display_mode *result = G2H(modes);
	int count = 0, index;
	SDL_DisplayMode **list = SDL_GetFullscreenDisplayModes(display, &count);

	if (!list)
		return 0;
	if (count > capacity)
		count = capacity;
	for (index = 0; index < count; index++)
		mode_out(&result[index], list[index]);
	SDL_free(list);
	return count;
}

int host_sdl_closest_display_mode(uint32_t display, int width, int height, float refresh_rate,
	int include_high_density, uint32_t mode)
{
	SDL_DisplayMode closest;

	if (!SDL_GetClosestFullscreenDisplayMode(display, width, height, refresh_rate, include_high_density != 0,
		&closest))
	{
		return 0;
	}
	mode_out(G2H(mode), &closest);
	return 1;
}

int host_sdl_display_usable_bounds(uint32_t display, uint32_t rectangle)
{
	SDL_Rect bounds;
	int *result = G2H(rectangle);

	if (!SDL_GetDisplayUsableBounds(display, &bounds))
		return 0;
	result[0] = bounds.x;
	result[1] = bounds.y;
	result[2] = bounds.w;
	result[3] = bounds.h;
	return 1;
}

/* ---------- a software renderer (the first start's data extraction) */

uint32_t host_sdl_create_renderer(uint32_t window)
{
	SDL_Window *object = handle_get(window, _handle_window);

	return object ? handle_new(_handle_renderer, SDL_CreateRenderer(object, NULL)) : 0;
}

void host_sdl_destroy_renderer(uint32_t renderer)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	if (!object)
		return;
	handle_free(renderer);
	SDL_DestroyRenderer(object);
}

int host_sdl_set_render_vsync(uint32_t renderer, int vsync)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	return object ? SDL_SetRenderVSync(object, vsync) : 0;
}

int host_sdl_set_render_draw_color(uint32_t renderer, int red, int green, int blue, int alpha)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	return object ? SDL_SetRenderDrawColor(object, (Uint8)red, (Uint8)green, (Uint8)blue, (Uint8)alpha) : 0;
}

int host_sdl_set_render_scale(uint32_t renderer, float x, float y)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	return object ? SDL_SetRenderScale(object, x, y) : 0;
}

int host_sdl_render_clear(uint32_t renderer)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	return object ? SDL_RenderClear(object) : 0;
}

int host_sdl_render_fill_rect(uint32_t renderer, float x, float y, float width, float height)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);
	SDL_FRect rectangle = { x, y, width, height };

	if (!object)
		return 0;
	return SDL_RenderFillRect(object, width < 0.0f ? NULL : &rectangle);
}

int host_sdl_render_debug_text(uint32_t renderer, float x, float y, uint32_t text)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	return object ? SDL_RenderDebugText(object, x, y, G2H(text)) : 0;
}

int host_sdl_render_present(uint32_t renderer)
{
	SDL_Renderer *object = handle_get(renderer, _handle_renderer);

	return object ? SDL_RenderPresent(object) : 0;
}

/* ---------- events */

void host_sdl_pump_events(void)
{
	SDL_PumpEvents();
}

int host_sdl_poll_event(uint32_t event)
{
	SDL_Event host_event;

	if (!SDL_PollEvent(&host_event))
		return 0;
	/* the layouts agree except for the pointers of text, drop and user
	events, which the guest does not read */
	memcpy(G2H(event), &host_event, sizeof(host_event));
	return 1;
}

int host_sdl_push_event(uint32_t event)
{
	SDL_Event host_event;

	memcpy(&host_event, G2H(event), sizeof(host_event));
	return SDL_PushEvent(&host_event) ? 1 : 0;
}

/* ---------- gamepads */

int host_sdl_get_gamepads(uint32_t ids, int capacity)
{
	uint32_t *result = G2H(ids);
	int count = 0, index;
	SDL_JoystickID *list = SDL_GetGamepads(&count);

	if (!list)
		return 0;
	if (count > capacity)
		count = capacity;
	for (index = 0; index < count; index++)
		result[index] = list[index];
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

/* SDL calls audio_callback on its own audio thread, which cannot run guest
code; it passes each request to the stream's thread (audio_thread), which
can, and waits for it to be done. SDL holds the stream's lock throughout, so
the audio the guest puts into the stream meanwhile (from audio_thread) is
kept in the binding's buffer instead, and put in by audio_callback once the
guest is done: audio_thread putting it in itself would wait for the lock
forever */
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

uint32_t host_sdl_open_audio_stream(uint32_t device, uint32_t spec, uint32_t callback, uint32_t userdata)
{
	struct audio_binding *binding = SDL_calloc(1, sizeof(*binding));
	SDL_AudioStream *stream;

	binding->callback = callback;
	binding->userdata = userdata;
	pthread_mutex_init(&binding->lock, NULL);
	pthread_cond_init(&binding->requested, NULL);
	pthread_cond_init(&binding->done, NULL);
	/* (SDL_AudioSpec is three 32-bit values in both ABIs) */
	stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)device, G2H(spec),
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

int host_sdl_put_audio_stream_data(uint32_t stream, uint32_t data, int length)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	if (!object)
		return 0;
	if (calling_back && calling_back->handle == stream)
		return audio_keep(calling_back, G2H(data), length);
	return SDL_PutAudioStreamData(object, G2H(data), length);
}

int host_sdl_resume_audio_stream_device(uint32_t stream)
{
	SDL_AudioStream *object = handle_get(stream, _handle_audio);

	return object ? SDL_ResumeAudioStreamDevice(object) : 0;
}

/* ---------- the clipboard (internet play's invite links) */

int host_sdl_set_clipboard_text(uint32_t text)
{
	return SDL_SetClipboardText(G2H(text)) ? 1 : 0;
}

void host_sdl_get_clipboard_text(uint32_t buffer, uint32_t size)
{
	char *text = SDL_GetClipboardText();

	copy_out(buffer, size, text);
	SDL_free(text);
}

/* ---------- dialogs */

int host_sdl_show_simple_message_box(uint32_t flags, uint32_t title, uint32_t message)
{
	return SDL_ShowSimpleMessageBox((SDL_MessageBoxFlags)flags, G2H(title), G2H(message), NULL) ? 1 : 0;
}

int host_sdl_show_message_box(uint32_t flags, uint32_t window, uint32_t title, uint32_t message, int button_count,
	uint32_t buttons, uint32_t answer)
{
	const struct halo_macos_message_box_button *guest_buttons = G2H(buttons);
	SDL_MessageBoxButtonData host_buttons[8];
	SDL_MessageBoxData data;
	int index, chosen = -1, shown;

	if (button_count > 8)
		button_count = 8;
	for (index = 0; index < button_count; index++)
	{
		host_buttons[index].flags = guest_buttons[index].flags;
		host_buttons[index].buttonID = guest_buttons[index].id;
		host_buttons[index].text = G2H(guest_buttons[index].text);
	}
	memset(&data, 0, sizeof(data));
	data.flags = (SDL_MessageBoxFlags)flags;
	data.window = handle_get(window, _handle_window);
	data.title = G2H(title);
	data.message = G2H(message);
	data.numbuttons = button_count;
	data.buttons = host_buttons;
	shown = SDL_ShowMessageBox(&data, &chosen);
	if (answer)
		*(int *)G2H(answer) = chosen;
	return shown ? 1 : 0;
}

struct file_choice
{
	SDL_AtomicInt done;
	char path[1024];
};

static void SDLCALL file_chosen(void *userdata, const char * const *files, int filter)
{
	struct file_choice *choice = userdata;

	(void)filter;
	if (files && files[0])
		SDL_strlcpy(choice->path, files[0], sizeof(choice->path));
	SDL_SetAtomicInt(&choice->done, 1);
}

int host_sdl_choose_file(uint32_t filter_name, uint32_t patterns, uint32_t path, uint32_t size)
{
	static struct file_choice choice;
	SDL_DialogFileFilter filters[2];

	memset(&choice, 0, sizeof(choice));
	filters[0].name = G2H(filter_name);
	filters[0].pattern = G2H(patterns);
	filters[1].name = "All files";
	filters[1].pattern = "*";
	SDL_ShowOpenFileDialog(file_chosen, &choice, NULL, filters, 2, NULL, false);
	/* the panel answers through the event loop */
	while (!SDL_GetAtomicInt(&choice.done))
	{
		SDL_PumpEvents();
		SDL_Delay(20);
	}
	if (!choice.path[0])
		return 0;
	copy_out(path, size, choice.path);
	return 1;
}

/* ---------- the process */

void host_macos_base_path(uint32_t buffer, uint32_t size)
{
	copy_out(buffer, size, host_base_path);
}
