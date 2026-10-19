/*
HOST_INPUT.C

The app's keyboard and mouse (port/macos/app's OpenCEWindow): the AppKit
events its windows' views pass on (halo_host_post_*) become the SDL events
the guest polls for (port/linux/src/sdl_platform.c), queued here until it
does. A poll takes this queue's events first, then SDL's own (the gamepads',
for now); without the app's windows the poll and the mouse capture are
SDL's, as the command-line host has them.

The views post on the main thread and the game polls on it (host_sdl_poll_event
is a main-thread import), but the queue has a lock all the same. Losing the
focus lets the mouse go at once, not when the game next polls: the game on
Android does not take it back on focus, so having the focus again does, if
the game still wants it.
*/

#include "host.h"
#include "halo_host.h"

#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* AppKit's virtual key codes (kVK_*, Carbon's Events.h): the key's place on
the keyboard, as SDL's scancodes are; 0 for those the game has no key for */
static const uint8_t key_code_scancodes[0x80] = {
	[0x00] = SDL_SCANCODE_A, [0x01] = SDL_SCANCODE_S, [0x02] = SDL_SCANCODE_D, [0x03] = SDL_SCANCODE_F,
	[0x04] = SDL_SCANCODE_H, [0x05] = SDL_SCANCODE_G, [0x06] = SDL_SCANCODE_Z, [0x07] = SDL_SCANCODE_X,
	[0x08] = SDL_SCANCODE_C, [0x09] = SDL_SCANCODE_V, [0x0a] = SDL_SCANCODE_NONUSBACKSLASH,
	[0x0b] = SDL_SCANCODE_B, [0x0c] = SDL_SCANCODE_Q, [0x0d] = SDL_SCANCODE_W, [0x0e] = SDL_SCANCODE_E,
	[0x0f] = SDL_SCANCODE_R, [0x10] = SDL_SCANCODE_Y, [0x11] = SDL_SCANCODE_T, [0x12] = SDL_SCANCODE_1,
	[0x13] = SDL_SCANCODE_2, [0x14] = SDL_SCANCODE_3, [0x15] = SDL_SCANCODE_4, [0x16] = SDL_SCANCODE_6,
	[0x17] = SDL_SCANCODE_5, [0x18] = SDL_SCANCODE_EQUALS, [0x19] = SDL_SCANCODE_9, [0x1a] = SDL_SCANCODE_7,
	[0x1b] = SDL_SCANCODE_MINUS, [0x1c] = SDL_SCANCODE_8, [0x1d] = SDL_SCANCODE_0,
	[0x1e] = SDL_SCANCODE_RIGHTBRACKET, [0x1f] = SDL_SCANCODE_O, [0x20] = SDL_SCANCODE_U,
	[0x21] = SDL_SCANCODE_LEFTBRACKET, [0x22] = SDL_SCANCODE_I, [0x23] = SDL_SCANCODE_P,
	[0x24] = SDL_SCANCODE_RETURN, [0x25] = SDL_SCANCODE_L, [0x26] = SDL_SCANCODE_J,
	[0x27] = SDL_SCANCODE_APOSTROPHE, [0x28] = SDL_SCANCODE_K, [0x29] = SDL_SCANCODE_SEMICOLON,
	[0x2a] = SDL_SCANCODE_BACKSLASH, [0x2b] = SDL_SCANCODE_COMMA, [0x2c] = SDL_SCANCODE_SLASH,
	[0x2d] = SDL_SCANCODE_N, [0x2e] = SDL_SCANCODE_M, [0x2f] = SDL_SCANCODE_PERIOD,
	[0x30] = SDL_SCANCODE_TAB, [0x31] = SDL_SCANCODE_SPACE, [0x32] = SDL_SCANCODE_GRAVE,
	[0x33] = SDL_SCANCODE_BACKSPACE, [0x35] = SDL_SCANCODE_ESCAPE,
	[0x36] = SDL_SCANCODE_RGUI, [0x37] = SDL_SCANCODE_LGUI, [0x38] = SDL_SCANCODE_LSHIFT,
	[0x39] = SDL_SCANCODE_CAPSLOCK, [0x3a] = SDL_SCANCODE_LALT, [0x3b] = SDL_SCANCODE_LCTRL,
	[0x3c] = SDL_SCANCODE_RSHIFT, [0x3d] = SDL_SCANCODE_RALT, [0x3e] = SDL_SCANCODE_RCTRL,
	[0x40] = SDL_SCANCODE_F17, [0x41] = SDL_SCANCODE_KP_PERIOD, [0x43] = SDL_SCANCODE_KP_MULTIPLY,
	[0x45] = SDL_SCANCODE_KP_PLUS, [0x47] = SDL_SCANCODE_NUMLOCKCLEAR, [0x48] = SDL_SCANCODE_VOLUMEUP,
	[0x49] = SDL_SCANCODE_VOLUMEDOWN, [0x4a] = SDL_SCANCODE_MUTE, [0x4b] = SDL_SCANCODE_KP_DIVIDE,
	[0x4c] = SDL_SCANCODE_KP_ENTER, [0x4e] = SDL_SCANCODE_KP_MINUS, [0x4f] = SDL_SCANCODE_F18,
	[0x50] = SDL_SCANCODE_F19, [0x51] = SDL_SCANCODE_KP_EQUALS, [0x52] = SDL_SCANCODE_KP_0,
	[0x53] = SDL_SCANCODE_KP_1, [0x54] = SDL_SCANCODE_KP_2, [0x55] = SDL_SCANCODE_KP_3,
	[0x56] = SDL_SCANCODE_KP_4, [0x57] = SDL_SCANCODE_KP_5, [0x58] = SDL_SCANCODE_KP_6,
	[0x59] = SDL_SCANCODE_KP_7, [0x5a] = SDL_SCANCODE_F20, [0x5b] = SDL_SCANCODE_KP_8,
	[0x5c] = SDL_SCANCODE_KP_9, [0x5d] = SDL_SCANCODE_INTERNATIONAL3, [0x5e] = SDL_SCANCODE_INTERNATIONAL1,
	[0x5f] = SDL_SCANCODE_KP_COMMA, [0x60] = SDL_SCANCODE_F5, [0x61] = SDL_SCANCODE_F6,
	[0x62] = SDL_SCANCODE_F7, [0x63] = SDL_SCANCODE_F3, [0x64] = SDL_SCANCODE_F8, [0x65] = SDL_SCANCODE_F9,
	[0x66] = SDL_SCANCODE_LANG2, [0x67] = SDL_SCANCODE_F11, [0x68] = SDL_SCANCODE_LANG1,
	[0x69] = SDL_SCANCODE_F13, [0x6a] = SDL_SCANCODE_F16, [0x6b] = SDL_SCANCODE_F14,
	[0x6d] = SDL_SCANCODE_F10, [0x6e] = SDL_SCANCODE_APPLICATION, [0x6f] = SDL_SCANCODE_F12,
	[0x71] = SDL_SCANCODE_F15, [0x72] = SDL_SCANCODE_INSERT, [0x73] = SDL_SCANCODE_HOME,
	[0x74] = SDL_SCANCODE_PAGEUP, [0x75] = SDL_SCANCODE_DELETE, [0x76] = SDL_SCANCODE_F4,
	[0x77] = SDL_SCANCODE_END, [0x78] = SDL_SCANCODE_F2, [0x79] = SDL_SCANCODE_PAGEDOWN,
	[0x7a] = SDL_SCANCODE_F1, [0x7b] = SDL_SCANCODE_LEFT, [0x7c] = SDL_SCANCODE_RIGHT,
	[0x7d] = SDL_SCANCODE_DOWN, [0x7e] = SDL_SCANCODE_UP,
};

/* NSEvent.ModifierFlags: caps lock, and the device-dependent bit of each
modifier key (IOKit's NX_DEVICE*KEYMASK), which says which of a pair is down */
#define FLAGS_CAPS_LOCK 0x10000u
static const struct { SDL_Scancode scancode; uint32_t flag; SDL_Keymod mod; } modifier_keys[] = {
	{SDL_SCANCODE_LCTRL, 0x1, SDL_KMOD_LCTRL}, {SDL_SCANCODE_LSHIFT, 0x2, SDL_KMOD_LSHIFT},
	{SDL_SCANCODE_RSHIFT, 0x4, SDL_KMOD_RSHIFT}, {SDL_SCANCODE_LGUI, 0x8, SDL_KMOD_LGUI},
	{SDL_SCANCODE_RGUI, 0x10, SDL_KMOD_RGUI}, {SDL_SCANCODE_LALT, 0x20, SDL_KMOD_LALT},
	{SDL_SCANCODE_RALT, 0x40, SDL_KMOD_RALT}, {SDL_SCANCODE_RCTRL, 0x2000, SDL_KMOD_RCTRL},
};
#define MODIFIER_KEY_COUNT (int)(sizeof(modifier_keys) / sizeof(modifier_keys[0]))

static pthread_mutex_t input_lock = PTHREAD_MUTEX_INITIALIZER;
static SDL_Event queue[HOST_INPUT_QUEUE_SIZE];
static int queue_first;
static int queue_count;
/* the modifiers held, as the keys' events say */
static SDL_Keymod modifiers;
/* the mouse capture the game asked for, and whether the window has the focus */
static int relative_wanted;
static int focused = 1;

/* ---------- HALO_HOST_TRACE_INPUT: each event, as it is queued and polled */

static void input_trace(const char *what, const SDL_Event *event)
{
	static int tracing = -1;

	if (tracing < 0)
		tracing = getenv("HALO_HOST_TRACE_INPUT") != NULL;
	if (!tracing)
		return;
	if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
		fprintf(stderr, "halo_host: input %s: key %s, scancode %d, mod 0x%x\n", what,
			event->key.down ? "down" : "up", event->key.scancode, event->key.mod);
	else
		fprintf(stderr, "halo_host: input %s: event 0x%x\n", what, event->type);
}

int32_t host_input_scancode(uint16_t key_code)
{
	return key_code < sizeof(key_code_scancodes) ? key_code_scancodes[key_code] : SDL_SCANCODE_UNKNOWN;
}

/* the last event queued, or NULL */
static SDL_Event *queue_last(void)
{
	return queue_count ? &queue[(queue_first + queue_count - 1) % HOST_INPUT_QUEUE_SIZE] : NULL;
}

/* with the lock held: a full queue keeps the events it has */
static void queue_event(const SDL_Event *event)
{
	if (queue_count == HOST_INPUT_QUEUE_SIZE)
		return;
	input_trace("queued", event);
	queue[(queue_first + queue_count) % HOST_INPUT_QUEUE_SIZE] = *event;
	queue_count++;
}

static void queue_key(uint32_t window, uint16_t key_code, SDL_Scancode scancode, int down, int repeat)
{
	SDL_Event event;

	memset(&event, 0, sizeof(event));
	event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
	event.key.timestamp = SDL_GetTicksNS();
	event.key.windowID = window;
	event.key.scancode = scancode;
	event.key.key = SDL_GetKeyFromScancode(scancode, modifiers, true);
	event.key.mod = modifiers;
	event.key.raw = key_code;
	event.key.down = down != 0;
	event.key.repeat = repeat != 0;
	queue_event(&event);
}

static int app_windows(void)
{
	return host_window_backend() != NULL;
}

void halo_host_post_key(uint32_t window, uint16_t key_code, int down, int repeat)
{
	SDL_Scancode scancode = (SDL_Scancode)host_input_scancode(key_code);

	if (scancode == SDL_SCANCODE_UNKNOWN)
		return;
	pthread_mutex_lock(&input_lock);
	queue_key(window, key_code, scancode, down, repeat);
	pthread_mutex_unlock(&input_lock);
}

void halo_host_post_modifiers(uint32_t window, uint16_t key_code, uint64_t flags)
{
	SDL_Scancode scancode = (SDL_Scancode)host_input_scancode(key_code);
	int i;

	pthread_mutex_lock(&input_lock);
	if (scancode == SDL_SCANCODE_CAPSLOCK)
	{
		/* (a press each time it changes: AppKit sends no key up for it) */
		int on = (flags & FLAGS_CAPS_LOCK) != 0;

		modifiers = on ? (modifiers | SDL_KMOD_CAPS) : (modifiers & ~SDL_KMOD_CAPS);
		queue_key(window, key_code, scancode, 1, 0);
		queue_key(window, key_code, scancode, 0, 0);
	}
	for (i = 0; i < MODIFIER_KEY_COUNT; i++)
	{
		int down;

		if (modifier_keys[i].scancode != scancode)
			continue;
		down = (flags & modifier_keys[i].flag) != 0;
		modifiers = down ? (modifiers | modifier_keys[i].mod) : (modifiers & ~modifier_keys[i].mod);
		queue_key(window, key_code, scancode, down, 0);
	}
	pthread_mutex_unlock(&input_lock);
}

void halo_host_post_mouse_motion(uint32_t window, float x, float y, float dx, float dy)
{
	SDL_Event *last;

	pthread_mutex_lock(&input_lock);
	last = queue_last();
	if (last && last->type == SDL_EVENT_MOUSE_MOTION && last->motion.windowID == window)
	{
		/* (one event for the moves since the last poll) */
		last->motion.x = x;
		last->motion.y = y;
		last->motion.xrel += dx;
		last->motion.yrel += dy;
	}
	else
	{
		SDL_Event event;

		memset(&event, 0, sizeof(event));
		event.type = SDL_EVENT_MOUSE_MOTION;
		event.motion.timestamp = SDL_GetTicksNS();
		event.motion.windowID = window;
		event.motion.x = x;
		event.motion.y = y;
		event.motion.xrel = dx;
		event.motion.yrel = dy;
		queue_event(&event);
	}
	pthread_mutex_unlock(&input_lock);
}

void halo_host_post_mouse_button(uint32_t window, int number, int down, float x, float y)
{
	/* AppKit's numbers: 0 left, 1 right, 2 middle, then the others in order */
	static const uint8_t buttons[] = {SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE, SDL_BUTTON_X1, SDL_BUTTON_X2};
	SDL_Event event;

	if (number < 0 || number > 254)
		return;
	memset(&event, 0, sizeof(event));
	event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
	event.button.timestamp = SDL_GetTicksNS();
	event.button.windowID = window;
	event.button.button = number < (int)sizeof(buttons) ? buttons[number] : (uint8_t)(number + 1);
	event.button.down = down != 0;
	event.button.clicks = 1;
	event.button.x = x;
	event.button.y = y;
	pthread_mutex_lock(&input_lock);
	queue_event(&event);
	pthread_mutex_unlock(&input_lock);
}

void halo_host_post_wheel(uint32_t window, float dx, float dy, int precise)
{
	/* a trackpad's (precise) scrolling is in points: about ten to a notch */
	float scale = precise ? 0.1f : 1.0f;
	SDL_Event event;

	memset(&event, 0, sizeof(event));
	event.type = SDL_EVENT_MOUSE_WHEEL;
	event.wheel.timestamp = SDL_GetTicksNS();
	event.wheel.windowID = window;
	/* (AppKit's x is positive to the left, SDL's to the right) */
	event.wheel.x = -dx * scale;
	event.wheel.y = dy * scale;
	event.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
	pthread_mutex_lock(&input_lock);
	queue_event(&event);
	pthread_mutex_unlock(&input_lock);
}

void halo_host_post_focus(uint32_t window, int has_focus)
{
	const struct halo_host_window_backend *backend = host_window_backend();
	SDL_Event event;
	int capture;

	pthread_mutex_lock(&input_lock);
	focused = has_focus != 0;
	capture = relative_wanted;
	if (!focused)
		modifiers &= SDL_KMOD_CAPS;
	memset(&event, 0, sizeof(event));
	event.type = focused ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
	event.window.timestamp = SDL_GetTicksNS();
	event.window.windowID = window;
	queue_event(&event);
	pthread_mutex_unlock(&input_lock);
	/* (let go now, or taken back if the game still wants it) */
	if (backend && backend->set_relative_mouse && capture)
		backend->set_relative_mouse(backend->context, window, focused);
}

int host_input_poll_event(void *event)
{
	pthread_mutex_lock(&input_lock);
	if (app_windows() && queue_count)
	{
		input_trace("polled", &queue[queue_first]);
		memcpy(event, &queue[queue_first], sizeof(SDL_Event));
		queue_first = (queue_first + 1) % HOST_INPUT_QUEUE_SIZE;
		queue_count--;
		pthread_mutex_unlock(&input_lock);
		return 1;
	}
	pthread_mutex_unlock(&input_lock);
	return host_sdl_poll_event(event);
}

int host_input_set_relative_mouse(uint32_t window, int enabled)
{
	const struct halo_host_window_backend *backend = host_window_backend();
	int call;

	if (!backend || !backend->set_relative_mouse)
		return host_sdl_set_relative_mouse(window, enabled);
	pthread_mutex_lock(&input_lock);
	relative_wanted = enabled != 0;
	call = focused;
	pthread_mutex_unlock(&input_lock);
	/* (asked for while the window is not focused: taken when it is) */
	if (call)
		backend->set_relative_mouse(backend->context, window, enabled != 0);
	return 1;
}
