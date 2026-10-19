/*
HOST_INPUT_TEST.C

A native test of port/macos/host/host_input.c (tools/test_macos_vm.py): the
app's keyboard and mouse, as AppKit reports them, become the SDL events the
guest polls for; losing the focus releases the mouse, and having it again
takes the mouse back if the game wanted it; the app's game has no gamepads.
SDL's own imports are stand-ins here.
*/

#include "host.h"
#include "halo_host.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* NSEvent.ModifierFlags: Shift, with the left Shift key's own bit */
#define FLAGS_SHIFT 0x20000u
#define FLAGS_LEFT_SHIFT 0x2u
#define FLAGS_CAPS_LOCK 0x10000u

/* ---------- SDL's, as host_sdl.c has them */

static int sdl_polls;
static int sdl_relative_calls;
/* the events SDL's poll gives, in turn */
static SDL_Event sdl_events[4];
static int sdl_event_count;

uint32_t host_sdl_create_window(const char *title, int width, int height, int64_t flags)
{
	(void)title;
	(void)width;
	(void)height;
	(void)flags;
	return 1;
}

void host_sdl_window_size_in_pixels(uint32_t window, int *width, int *height)
{
	(void)window;
	*width = 0;
	*height = 0;
}

uint32_t host_sdl_gl_create_context(uint32_t window)
{
	(void)window;
	return 0;
}

int host_sdl_gl_make_current(uint32_t window, uint32_t context)
{
	(void)window;
	(void)context;
	return 0;
}

int host_sdl_gl_set_swap_interval(int interval)
{
	(void)interval;
	return 0;
}

int host_sdl_gl_swap_window(uint32_t window)
{
	(void)window;
	return 0;
}

int host_sdl_poll_event(void *event)
{
	sdl_polls++;
	if (!sdl_event_count)
		return 0;
	memcpy(event, &sdl_events[0], sizeof(SDL_Event));
	memmove(&sdl_events[0], &sdl_events[1], sizeof(SDL_Event) * (size_t)--sdl_event_count);
	return 1;
}

static void sdl_event(Uint32 type)
{
	memset(&sdl_events[sdl_event_count], 0, sizeof(SDL_Event));
	sdl_events[sdl_event_count].type = type;
	sdl_events[sdl_event_count].gdevice.which = 99;
	sdl_event_count++;
}

int host_sdl_set_relative_mouse(uint32_t window, int enabled)
{
	(void)window;
	(void)enabled;
	sdl_relative_calls++;
	return 1;
}

/* ---------- the app's windows: one, whose mouse capture is recorded */

static int relative_calls;
static int relative_enabled = -1;

static uint32_t app_create(void *context, const char *title, int width, int height, int64_t flags)
{
	(void)context;
	(void)title;
	(void)width;
	(void)height;
	(void)flags;
	return 1;
}

static void app_size_in_pixels(void *context, uint32_t window, int *width, int *height)
{
	(void)context;
	(void)window;
	*width = 1280;
	*height = 960;
}

static int app_set_relative_mouse(void *context, uint32_t window, int enabled)
{
	(void)context;
	CHECK(window == 1);
	relative_calls++;
	relative_enabled = enabled;
	return 1;
}

static int poll(SDL_Event *event)
{
	memset(event, 0xa5, sizeof(*event));
	return host_input_poll_event(event);
}

/* ---------- the key codes' table */

static void check_table(void)
{
	static const struct { uint16_t code; SDL_Scancode scancode; } known[] = {
		{0x00, SDL_SCANCODE_A}, {0x0d, SDL_SCANCODE_W}, {0x01, SDL_SCANCODE_S}, {0x02, SDL_SCANCODE_D},
		{0x12, SDL_SCANCODE_1}, {0x1d, SDL_SCANCODE_0}, {0x24, SDL_SCANCODE_RETURN},
		{0x30, SDL_SCANCODE_TAB}, {0x31, SDL_SCANCODE_SPACE}, {0x33, SDL_SCANCODE_BACKSPACE},
		{0x35, SDL_SCANCODE_ESCAPE}, {0x75, SDL_SCANCODE_DELETE}, {0x32, SDL_SCANCODE_GRAVE},
		{0x7a, SDL_SCANCODE_F1}, {0x60, SDL_SCANCODE_F5}, {0x67, SDL_SCANCODE_F11}, {0x6f, SDL_SCANCODE_F12},
		{0x7b, SDL_SCANCODE_LEFT}, {0x7c, SDL_SCANCODE_RIGHT}, {0x7d, SDL_SCANCODE_DOWN}, {0x7e, SDL_SCANCODE_UP},
		{0x52, SDL_SCANCODE_KP_0}, {0x5c, SDL_SCANCODE_KP_9}, {0x4c, SDL_SCANCODE_KP_ENTER},
		{0x38, SDL_SCANCODE_LSHIFT}, {0x3c, SDL_SCANCODE_RSHIFT}, {0x3b, SDL_SCANCODE_LCTRL},
		{0x3e, SDL_SCANCODE_RCTRL}, {0x3a, SDL_SCANCODE_LALT}, {0x3d, SDL_SCANCODE_RALT},
		{0x37, SDL_SCANCODE_LGUI}, {0x36, SDL_SCANCODE_RGUI}, {0x39, SDL_SCANCODE_CAPSLOCK},
	};
	unsigned char seen[SDL_SCANCODE_COUNT];
	int i;

	for (i = 0; i < (int)(sizeof(known) / sizeof(known[0])); i++)
	{
		if (host_input_scancode(known[i].code) != (int32_t)known[i].scancode)
			printf("key code 0x%02x: %d, not %d\n", known[i].code, host_input_scancode(known[i].code),
				known[i].scancode);
		CHECK(host_input_scancode(known[i].code) == (int32_t)known[i].scancode);
	}
	/* (the Fn key, and codes no Mac keyboard has) */
	CHECK(host_input_scancode(0x3f) == SDL_SCANCODE_UNKNOWN);
	CHECK(host_input_scancode(0x34) == SDL_SCANCODE_UNKNOWN);
	CHECK(host_input_scancode(0x80) == SDL_SCANCODE_UNKNOWN);
	CHECK(host_input_scancode(0xffff) == SDL_SCANCODE_UNKNOWN);
	/* no two keys are the same key to the game */
	memset(seen, 0, sizeof(seen));
	for (i = 0; i < 0x80; i++)
	{
		int32_t scancode = host_input_scancode((uint16_t)i);

		CHECK(scancode >= 0 && scancode < SDL_SCANCODE_COUNT);
		if (scancode == SDL_SCANCODE_UNKNOWN)
			continue;
		if (seen[scancode])
			printf("key code 0x%02x: scancode %d twice\n", i, scancode);
		CHECK(!seen[scancode]);
		seen[scancode] = 1;
	}
}

int main(void)
{
	struct halo_host_window_backend backend;
	SDL_Event event;
	int i;

	check_table();

	/* SDL's, while the app has given no windows */
	CHECK(poll(&event) == 0 && sdl_polls == 1);
	CHECK(host_input_set_relative_mouse(1, 1) == 1 && sdl_relative_calls == 1);
	/* (the command-line host's gamepads are SDL's) */
	sdl_event(SDL_EVENT_GAMEPAD_ADDED);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_GAMEPAD_ADDED && event.gdevice.which == 99);
	sdl_polls = 1;

	memset(&backend, 0, sizeof(backend));
	backend.create = app_create;
	backend.size_in_pixels = app_size_in_pixels;
	backend.set_relative_mouse = app_set_relative_mouse;
	halo_host_set_window_backend(&backend);
	CHECK(host_window_create("Halo", 640, 480, 0) == 1);

	/* keys */
	halo_host_post_key(1, 0x00, 1, 0);
	halo_host_post_key(1, 0x00, 1, 1);
	halo_host_post_key(1, 0x00, 0, 0);
	CHECK(poll(&event) == 1);
	CHECK(event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == 1 && event.key.scancode == SDL_SCANCODE_A);
	CHECK(event.key.key == SDLK_A && event.key.down && !event.key.repeat && event.key.mod == SDL_KMOD_NONE);
	CHECK(event.key.raw == 0x00 && event.key.which == 0);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_DOWN && event.key.repeat);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_UP && !event.key.down);
	/* (then SDL's own) */
	CHECK(poll(&event) == 0 && sdl_polls == 2);
	/* the app's game is played with the keyboard and mouse: SDL's joystick
	and gamepad events are left out, so it opens no pad; the others are not */
	sdl_event(SDL_EVENT_GAMEPAD_ADDED);
	sdl_event(SDL_EVENT_JOYSTICK_AXIS_MOTION);
	sdl_event(SDL_EVENT_GAMEPAD_SENSOR_UPDATE);
	sdl_event(SDL_EVENT_QUIT);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_QUIT);
	CHECK(poll(&event) == 0 && sdl_event_count == 0);
	sdl_polls = 2;
	/* a key the game has no scancode for is not sent */
	halo_host_post_key(1, 0x3f, 1, 0);
	CHECK(poll(&event) == 0);

	/* modifiers: a key of its own, and held for the keys after it */
	halo_host_post_modifiers(1, 0x38, FLAGS_SHIFT | FLAGS_LEFT_SHIFT);
	halo_host_post_key(1, 0x00, 1, 0);
	halo_host_post_modifiers(1, 0x38, 0);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_LSHIFT);
	CHECK(event.key.mod == SDL_KMOD_LSHIFT);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_A);
	CHECK(event.key.mod == SDL_KMOD_LSHIFT);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_UP && event.key.scancode == SDL_SCANCODE_LSHIFT);
	CHECK(event.key.mod == SDL_KMOD_NONE);
	/* caps lock is a press each time it changes, and a modifier while on */
	halo_host_post_modifiers(1, 0x39, FLAGS_CAPS_LOCK);
	halo_host_post_key(1, 0x00, 1, 0);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_CAPSLOCK);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_UP && event.key.scancode == SDL_SCANCODE_CAPSLOCK);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_DOWN && (event.key.mod & SDL_KMOD_CAPS));
	halo_host_post_modifiers(1, 0x39, 0);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_CAPSLOCK);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_KEY_UP && !(event.key.mod & SDL_KMOD_CAPS));

	/* mouse buttons: AppKit's numbers (0 left, 1 right, 2 middle) are SDL's */
	halo_host_post_mouse_button(1, 0, 1, 10.0f, 20.0f);
	halo_host_post_mouse_button(1, 1, 1, 10.0f, 20.0f);
	halo_host_post_mouse_button(1, 2, 0, 10.0f, 20.0f);
	halo_host_post_mouse_button(1, 3, 1, 10.0f, 20.0f);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT);
	CHECK(event.button.down && event.button.x == 10.0f && event.button.y == 20.0f && event.button.clicks == 1);
	CHECK(event.button.windowID == 1);
	CHECK(poll(&event) == 1 && event.button.button == SDL_BUTTON_RIGHT);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE);
	CHECK(!event.button.down);
	CHECK(poll(&event) == 1 && event.button.button == SDL_BUTTON_X1);

	/* motion: relative, and one event for the moves since the last poll */
	halo_host_post_mouse_motion(1, 100.0f, 50.0f, 3.0f, -2.0f);
	halo_host_post_mouse_motion(1, 104.0f, 49.0f, 4.0f, -1.0f);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_MOTION && event.motion.windowID == 1);
	CHECK(event.motion.xrel == 7.0f && event.motion.yrel == -3.0f);
	CHECK(event.motion.x == 104.0f && event.motion.y == 49.0f);
	CHECK(poll(&event) == 0);
	/* (but not across a click: the moves before it, then those after) */
	halo_host_post_mouse_motion(1, 0.0f, 0.0f, 1.0f, 0.0f);
	halo_host_post_mouse_button(1, 0, 0, 0.0f, 0.0f);
	halo_host_post_mouse_motion(1, 0.0f, 0.0f, 2.0f, 0.0f);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_MOTION && event.motion.xrel == 1.0f);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_BUTTON_UP);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_MOTION && event.motion.xrel == 2.0f);

	/* the wheel: a notch is 1; a trackpad's points are a tenth */
	halo_host_post_wheel(1, 0.0f, 1.0f, 0);
	halo_host_post_wheel(1, 0.0f, -10.0f, 1);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.y == 1.0f);
	CHECK(event.wheel.windowID == 1 && event.wheel.direction == SDL_MOUSEWHEEL_NORMAL);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.y == -1.0f);

	/* the capture: the game's, while the window has the focus */
	CHECK(host_input_set_relative_mouse(1, 1) == 1);
	CHECK(relative_calls == 1 && relative_enabled == 1 && sdl_relative_calls == 1);
	halo_host_post_modifiers(1, 0x38, FLAGS_SHIFT | FLAGS_LEFT_SHIFT);
	halo_host_post_focus(1, 0);
	/* (released at once, not when the game next polls) */
	CHECK(relative_calls == 2 && relative_enabled == 0);
	CHECK(poll(&event) == 1 && event.key.scancode == SDL_SCANCODE_LSHIFT);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_WINDOW_FOCUS_LOST && event.window.windowID == 1);
	/* the keys held are let go: the Shift held as the focus went is up */
	halo_host_post_key(1, 0x00, 1, 0);
	CHECK(poll(&event) == 1 && event.key.mod == SDL_KMOD_NONE);
	/* asked for while the window is not focused: kept for later */
	CHECK(host_input_set_relative_mouse(1, 1) == 1 && relative_calls == 2);
	halo_host_post_focus(1, 1);
	CHECK(relative_calls == 3 && relative_enabled == 1);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_WINDOW_FOCUS_GAINED);
	/* given up by the game: not taken back */
	CHECK(host_input_set_relative_mouse(1, 0) == 1 && relative_calls == 4 && relative_enabled == 0);
	halo_host_post_focus(1, 0);
	halo_host_post_focus(1, 1);
	CHECK(relative_calls == 4);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_WINDOW_FOCUS_LOST);
	CHECK(poll(&event) == 1 && event.type == SDL_EVENT_WINDOW_FOCUS_GAINED);

	/* a full queue keeps what it has (the game polls each frame) */
	for (i = 0; i < 1000; i++)
		halo_host_post_key(1, 0x00, i % 2 == 0, 0);
	for (i = 0; poll(&event) == 1; i++)
		CHECK(event.type == (i % 2 == 0 ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP));
	CHECK(i == HOST_INPUT_QUEUE_SIZE);

	/* and SDL's again once the app takes its windows back */
	halo_host_set_window_backend(NULL);
	halo_host_post_key(1, 0x00, 1, 0);
	sdl_polls = 0;
	CHECK(poll(&event) == 0 && sdl_polls == 1);
	printf("ok\n");
	return 0;
}
