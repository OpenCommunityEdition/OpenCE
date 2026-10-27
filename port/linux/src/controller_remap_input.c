/*
CONTROLLER_REMAP_INPUT.C

The controllers' own button layouts at the controller (controller_remap.h),
on the game's thread, where xinput_sdl.c reads each port's pad: a playing
player's pad moved to their layout, and the capture of a button for Button
Remapping's rows, which takes the port's pad (and every other port's in the
menus) until it ends, so that the button pressed neither presses the menu
nor plays.
*/

#include "platform.h"
#include "sdl_platform.h"
#include "controller_remap.h"
#include "controller_buttons.h"

#include <SDL3/SDL.h>
#include <string.h>

/* a button's pressure past which it is held */
#define REMAP_PRESSED 30
/* a capture the menu stops polling (its screen gone) ends after this long */
#define REMAP_CAPTURE_TIMEOUT_MS 1000

enum remap_capture_phase
{
	_capture_idle,
	/* the press that began it, until the controller's buttons are all up */
	_capture_releasing,
	/* the next button pressed */
	_capture_waiting,
	/* a button taken, or cancelled, until the menu polls it */
	_capture_ended,
	/* over: the port reads nothing until its buttons are all up */
	_capture_draining
};

/* the pad's buttons from the d-pad's up (_remap_button_dpad_up) on */
static WORD const digital_masks[NUMBER_OF_REMAP_BUTTONS - _remap_button_dpad_up] =
{
	XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
	XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB
};

static struct
{
	short port;
	int phase;
	int result;
	int button;
	/* the controller it is for: another one plugged into the port cancels it */
	unsigned int instance;
	Uint64 polled;
} capture = { -1, _capture_idle, _remap_capture_waiting, 0, 0, 0 };

/* each port's layout in use, and the buttons held when it began (or
changed), which do nothing until they are let go */
static struct
{
	int active;
	unsigned int blocked;
	unsigned char layout[CONTROLLER_REMAP_ACTIONS];
	unsigned char targets[CONTROLLER_REMAP_ACTIONS];
} remap_ports[CONTROLLER_PORTS];

/* each button's pressure (controller_remap_button's order) */
static void pad_pressures(XINPUT_GAMEPAD const *pad, unsigned char *pressures)
{
	int button;

	memcpy(pressures, pad->bAnalogButtons, _remap_button_dpad_up);
	for (button = _remap_button_dpad_up; button < NUMBER_OF_REMAP_BUTTONS; button++)
		pressures[button] = pad->wButtons & digital_masks[button - _remap_button_dpad_up] ? 0xff : 0;
}

static int capture_controller_present(void)
{
	struct controller_device device;

	controller_device_get(capture.port, &device);
	return device.connected && device.instance == capture.instance;
}

static void capture_finish(int result)
{
	capture.result = result;
	capture.phase = _capture_ended;
}

void controller_remap_capture_begin(short port)
{
	struct controller_device device;

	controller_remap_capture_end();
	if (port < 0 || port >= CONTROLLER_PORTS)
		return;
	controller_device_get(port, &device);
	capture.port = port;
	capture.instance = device.instance;
	capture.phase = _capture_releasing;
	capture.polled = SDL_GetTicks();
	/* (the keyboard's Escape cancels: it binds nothing) */
	platform_binding_capture_begin();
}

void controller_remap_capture_end(void)
{
	if (capture.phase == _capture_idle)
		return;
	/* (a controller gone may not be read again, to drain) */
	capture.phase = capture_controller_present() ? _capture_draining : _capture_idle;
	if (capture.phase == _capture_idle)
		capture.port = -1;
	capture.result = _remap_capture_waiting;
	platform_binding_capture_cancel();
}

int controller_remap_capture_poll(int *button)
{
	int key, result;

	if (capture.phase == _capture_idle || capture.phase == _capture_draining)
		return _remap_capture_cancelled;
	if (!capture_controller_present())
	{
		controller_remap_capture_end();
		return _remap_capture_cancelled;
	}
	capture.polled = SDL_GetTicks();
	result = platform_binding_capture_poll(&key);
	if (result == 3)
		capture_finish(_remap_capture_cancelled);
	else if (result)
		platform_binding_capture_begin();
	if (capture.phase != _capture_ended)
		return _remap_capture_waiting;
	result = capture.result;
	*button = capture.button;
	controller_remap_capture_end();
	return result;
}

/* the capture's port's pad: the button pressed taken, start or back
cancelling */
static void capture_input(short port, unsigned int held, int connected)
{
	struct controller_device device;
	int button;

	controller_device_get(port, &device);
	if ((capture.phase == _capture_releasing || capture.phase == _capture_waiting) &&
		(!connected || device.instance != capture.instance))
	{
		capture_finish(_remap_capture_cancelled);
	}
	if (capture.phase == _capture_releasing && !held)
	{
		capture.phase = _capture_waiting;
	}
	else if (capture.phase == _capture_waiting && held)
	{
		if (held & ((1u << _remap_button_start) | (1u << _remap_button_back)))
		{
			capture_finish(_remap_capture_cancelled);
		}
		else
		{
			for (button = 0; button < NUMBER_OF_REMAP_BUTTONS; button++)
			{
				if ((held & (1u << button)) && controller_remap_bindable(button))
				{
					capture.button = button;
					capture_finish(_remap_capture_bound);
					break;
				}
			}
		}
	}
	else if (capture.phase == _capture_draining && !held)
	{
		capture.phase = _capture_idle;
		capture.port = -1;
	}
}

int controller_remap_input(short port, XINPUT_GAMEPAD *pad, int connected)
{
	unsigned char pressures[NUMBER_OF_REMAP_BUTTONS];
	unsigned char layout[CONTROLLER_REMAP_ACTIONS], targets[CONTROLLER_REMAP_ACTIONS];
	unsigned int held = 0;
	int button, action;

	if (port < 0 || port >= CONTROLLER_PORTS)
		return 0;
	/* (checked from any port read, the capture's own may no longer be) */
	if (capture.phase != _capture_idle && (!capture_controller_present() ||
		(capture.phase != _capture_draining && SDL_GetTicks() - capture.polled > REMAP_CAPTURE_TIMEOUT_MS)))
	{
		controller_remap_capture_end();
	}
	/* (while one controller binds a button, the others cannot move through
	the menu; players playing keep theirs) */
	if (capture.phase != _capture_idle && capture.phase != _capture_draining && capture.port != port &&
		!controller_remap_playing(port))
	{
		memset(pad, 0, sizeof(*pad));
		return 1;
	}
	pad_pressures(pad, pressures);
	for (button = 0; button < NUMBER_OF_REMAP_BUTTONS; button++)
	{
		if (pressures[button] > REMAP_PRESSED)
			held |= 1u << button;
	}
	if (capture.phase != _capture_idle && capture.port == port)
	{
		capture_input(port, held, connected);
		memset(pad, 0, sizeof(*pad));
		remap_ports[port].active = 0;
		return 1;
	}
	if (!connected || !controller_remap_read(port, layout) || !controller_remap_playing(port))
	{
		memset(&remap_ports[port], 0, sizeof(remap_ports[port]));
		return 0;
	}
	/* (the layout's buttons go to the controls' buttons in the profile's
	layout, which the game reads them on) */
	controller_remap_profile(port, targets);
	if (!remap_ports[port].active || memcmp(layout, remap_ports[port].layout, sizeof(layout)) ||
		memcmp(targets, remap_ports[port].targets, sizeof(targets)))
	{
		remap_ports[port].blocked = held;
		memcpy(remap_ports[port].layout, layout, sizeof(layout));
		memcpy(remap_ports[port].targets, targets, sizeof(targets));
	}
	remap_ports[port].active = 1;
	remap_ports[port].blocked &= held;
	memset(pad->bAnalogButtons, 0, sizeof(pad->bAnalogButtons));
	pad->wButtons &= ~(XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB);
	/* (the d-pad's left is a control's only where one of the layouts puts
	one there; else it is still the d-pad's) */
	for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
	{
		if (layout[action] == _remap_button_dpad_left || targets[action] == _remap_button_dpad_left)
			pad->wButtons &= ~XINPUT_GAMEPAD_DPAD_LEFT;
	}
	for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
	{
		unsigned char pressure = remap_ports[port].blocked & (1u << layout[action]) ? 0 : pressures[layout[action]];

		if (targets[action] < _remap_button_dpad_up)
			pad->bAnalogButtons[targets[action]] = pressure;
		else if (pressure > REMAP_PRESSED)
			pad->wButtons |= digital_masks[targets[action] - _remap_button_dpad_up];
	}
	return 0;
}
