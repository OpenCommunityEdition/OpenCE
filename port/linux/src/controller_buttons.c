/*
CONTROLLER_BUTTONS.C

Each port's controller and the HUD's button styles (controller_buttons.h).
xinput_sdl.c tells each port's controller as it reads the pads, on the
game's thread, which the HUD and the menus read it on too. The styles'
settings are read again only when a setting changes: the HUD asks for a
viewport's style at every button it draws.
*/

#include "controller_buttons.h"
#include "port_config.h"

#include <SDL3/SDL.h>
#include <string.h>

static struct controller_device controller_devices[CONTROLLER_PORTS];

static struct
{
	char const *name;
	int family;
} const button_styles[NUMBER_OF_BUTTON_STYLES] =
{
	{ "classic", _controller_xbox },
	{ "gems", _controller_xbox },
	{ "menu_gems", _controller_xbox },
	{ "xbox_modern", _controller_xbox },
	{ "ps_classic", _controller_playstation },
	{ "ps_gems", _controller_playstation },
	{ "ps_dark_gems", _controller_playstation },
	{ "ps_modern", _controller_playstation },
	{ "switch_classic", _controller_switch },
	{ "switch_gems", _controller_switch },
	{ "switch_modern", _controller_switch },
};

/* each family's setting, and its style where the setting names another
family's */
static char const *const family_settings[NUMBER_OF_CONTROLLER_FAMILIES] =
{
	NULL, "display.hud_button_style_xbox", "display.hud_button_style_ps", "display.hud_button_style_switch"
};
static int const family_defaults[NUMBER_OF_CONTROLLER_FAMILIES] =
{
	_button_style_classic, _button_style_classic, _button_style_ps_classic, _button_style_switch_classic
};

static int controller_family(SDL_GamepadType type)
{
	switch (type)
	{
	case SDL_GAMEPAD_TYPE_XBOX360:
	case SDL_GAMEPAD_TYPE_XBOXONE:
		return _controller_xbox;
	case SDL_GAMEPAD_TYPE_PS3:
	case SDL_GAMEPAD_TYPE_PS4:
	case SDL_GAMEPAD_TYPE_PS5:
		return _controller_playstation;
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
	case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
		return _controller_switch;
	default:
		return _controller_unknown;
	}
}

void controller_device_update(short port, void *gamepad)
{
	struct controller_device *device;

	if (port < 0 || port >= CONTROLLER_PORTS)
		return;
	device = &controller_devices[port];
	memset(device, 0, sizeof(*device));
	if (!gamepad)
		return;
	device->connected = 1;
	device->family = controller_family(SDL_GetGamepadType((SDL_Gamepad *)gamepad));
#ifndef HALO_ARM64_GUEST
	/* (the guest reaches SDL only through host_imports.list, which has no
	SDL_GetGamepadID: every controller there is instance 0) */
	device->instance = (unsigned int)SDL_GetGamepadID((SDL_Gamepad *)gamepad);
#endif
}

void controller_device_get(short port, struct controller_device *device)
{
	if (port >= 0 && port < CONTROLLER_PORTS)
		*device = controller_devices[port];
	else
		memset(device, 0, sizeof(*device));
}

char const *controller_button_style_name(int style)
{
	return style >= 0 && style < NUMBER_OF_BUTTON_STYLES ? button_styles[style].name : "";
}

int controller_button_style_family(int style)
{
	return style >= 0 && style < NUMBER_OF_BUTTON_STYLES ? button_styles[style].family : _controller_unknown;
}

/* the style a setting's value names, else -1 ("auto", or anything else);
"dark_gems" is "menu_gems", its earlier name */
static int button_style_named(char const *name)
{
	int style;

	if (!strcmp(name, "dark_gems"))
		return _button_style_dark_gems;
	for (style = 0; style < NUMBER_OF_BUTTON_STYLES; style++)
	{
		if (!strcmp(name, button_styles[style].name))
			return style;
	}
	return -1;
}

int controller_button_style_for_port(short port)
{
	static unsigned long read_at = (unsigned long)-1;
	static int chosen, family_styles[NUMBER_OF_CONTROLLER_FAMILIES];
	struct controller_device device;
	int family;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		chosen = button_style_named(config_string("display.hud_button_style"));
		family_styles[_controller_unknown] = family_defaults[_controller_unknown];
		for (family = _controller_xbox; family < NUMBER_OF_CONTROLLER_FAMILIES; family++)
		{
			int style = button_style_named(config_string(family_settings[family]));

			family_styles[family] = controller_button_style_family(style) == family ? style : family_defaults[family];
		}
	}
	if (chosen >= 0)
		return chosen;
	/* (auto: the controller's family's style; with none, or one of another
	kind, the Xbox's) */
	controller_device_get(port, &device);
	family = device.family > _controller_unknown && device.family < NUMBER_OF_CONTROLLER_FAMILIES ?
		device.family : _controller_xbox;
	return family_styles[family];
}
