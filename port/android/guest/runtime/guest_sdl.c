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

#ifdef HALO_GUEST_VM
/* In the macOS host's VM the guest reads the generic timer's count itself,
at EL0 (the host sets CNTKCTL_EL1.EL0VCTEN, and gives every vCPU the same
count): only the first call asks the host, for SDL's milliseconds at a
count, and each later one is that plus the time since, with no exit. */
static unsigned long long guest_counter(void)
{
	unsigned long long value;

	__asm__ volatile("isb\n\tmrs %0, cntvct_el0" : "=r"(value) : : "memory");
	return value;
}

static unsigned long long guest_counter_frequency(void)
{
	unsigned long long value;

	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
	return value;
}

Uint64 SDL_GetTicks(void)
{
	/* 0, then 1 while a thread calibrates, then 2 */
	static int calibration;
	static unsigned long long base_count, base_ticks, frequency;
	unsigned long long count;
	int expected = 0;

	if (__atomic_load_n(&calibration, __ATOMIC_ACQUIRE) != 2)
	{
		if (__atomic_compare_exchange_n(&calibration, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		{
			base_ticks = (unsigned long long)host_sdl_ticks();
			base_count = guest_counter();
			frequency = guest_counter_frequency();
			if (!frequency)
				frequency = 24000000;
			__atomic_store_n(&calibration, 2, __ATOMIC_RELEASE);
		}
		else
		{
			/* (another thread is calibrating, a single exit to the host) */
			while (__atomic_load_n(&calibration, __ATOMIC_ACQUIRE) != 2)
				;
		}
	}
	count = guest_counter() - base_count;
	return (Uint64)(base_ticks + count / frequency * 1000 + count % frequency * 1000 / frequency);
}
#else
Uint64 SDL_GetTicks(void)
{
	return (Uint64)host_sdl_ticks();
}
#endif

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

/* (up to three buttons, their ids those of the guest's) */
bool SDL_ShowMessageBox(const SDL_MessageBoxData *data, int *buttonid)
{
	const char *labels[3] = { NULL, NULL, NULL };
	int default_index = -1;
	int index;
	int chosen;

	if (!data || data->numbuttons < 1 || data->numbuttons > 3)
		return false;
	for (index = 0; index < data->numbuttons; index++)
	{
		labels[index] = data->buttons[index].text;
		if (data->buttons[index].flags & SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT)
			default_index = index;
	}
	chosen = host_sdl_show_message_box((unsigned int)data->flags, data->title, data->message, default_index,
		labels[0], labels[1], labels[2]);
	if (chosen < 0 || chosen >= data->numbuttons)
		return false;
	if (buttonid)
		*buttonid = data->buttons[chosen].buttonID;
	return true;
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

void SDL_CloseGamepad(SDL_Gamepad *gamepad)
{
	host_sdl_close_gamepad((unsigned int)gamepad);
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

int SDL_GetAudioStreamData(SDL_AudioStream *stream, void *data, int length)
{
	return host_sdl_get_audio_stream_data((unsigned int)stream, data, length);
}

int SDL_GetAudioStreamAvailable(SDL_AudioStream *stream)
{
	return host_sdl_get_audio_stream_available((unsigned int)stream);
}

void SDL_DestroyAudioStream(SDL_AudioStream *stream)
{
	host_sdl_destroy_audio_stream((unsigned int)stream);
}
