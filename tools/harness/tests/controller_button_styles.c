/*
CONTROLLER_BUTTON_STYLES.C (test)

The real HUD button styles of port/linux/src/controller_buttons.c (each
port's controller's family, and the style its viewport draws in) and the
real artwork lookup of port/linux/game/controller_glyphs.c, against a fake
SDL and fake settings. test_controller_button_styles.py takes the header
(config.inc), the code (buttons.inc, glyphs.inc), the artwork table
(controller_glyph_assets.h) and port_config.c's allowed values
(allowed.inc).
*/

#include "harness.h"

#define NUMBEROF(array) (sizeof(array) / sizeof((array)[0]))

#include "config.inc"

/* ---------- a fake SDL: a pad is its type and its instance */

typedef enum
{
	SDL_GAMEPAD_TYPE_UNKNOWN,
	SDL_GAMEPAD_TYPE_STANDARD,
	SDL_GAMEPAD_TYPE_XBOX360,
	SDL_GAMEPAD_TYPE_XBOXONE,
	SDL_GAMEPAD_TYPE_PS3,
	SDL_GAMEPAD_TYPE_PS4,
	SDL_GAMEPAD_TYPE_PS5,
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO,
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT,
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT,
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR,
	SDL_GAMEPAD_TYPE_GAMECUBE
} SDL_GamepadType;

typedef struct SDL_Gamepad
{
	SDL_GamepadType type;
	unsigned int id;
} SDL_Gamepad;

static SDL_GamepadType SDL_GetGamepadType(SDL_Gamepad *gamepad)
{
	return gamepad->type;
}

static unsigned int SDL_GetGamepadID(SDL_Gamepad *gamepad)
{
	return gamepad->id;
}

/* ---------- fake settings: config_changes counts the changes */

static struct
{
	char const *name;
	char const *value;
} settings[] =
{
	{ "display.hud_button_style", "auto" },
	{ "display.hud_button_style_xbox", "classic" },
	{ "display.hud_button_style_ps", "ps_classic" },
	{ "display.hud_button_style_switch", "switch_classic" },
};
static unsigned long changes;

static char const *config_string(char const *name)
{
	size_t index;

	for (index = 0; index < NUMBEROF(settings); index++)
	{
		if (!strcmp(settings[index].name, name))
			return settings[index].value;
	}
	CHECK(0, "an unknown setting was read: %s", name);
	return "";
}

static unsigned long config_changes(void)
{
	return changes;
}

/* a setting changed without telling config_changes (to see what is cached) */
static void set_quietly(char const *name, char const *value)
{
	size_t index;

	for (index = 0; index < NUMBEROF(settings); index++)
	{
		if (!strcmp(settings[index].name, name))
		{
			settings[index].value = value;
			return;
		}
	}
	CHECK(0, "an unknown setting was set: %s", name);
}

static void set(char const *name, char const *value)
{
	set_quietly(name, value);
	changes++;
}

#include "buttons.inc"
#include "glyphs.inc"
#include "allowed.inc"

static SDL_Gamepad pads[CONTROLLER_PORTS];

/* each port's controller: a type, or none (the type's NONE) */
static void plug(int type0, int type1, int type2, int type3)
{
	int const types[CONTROLLER_PORTS] = { type0, type1, type2, type3 };
	short port;

	for (port = 0; port < CONTROLLER_PORTS; port++)
	{
		pads[port].type = types[port] == NONE ? SDL_GAMEPAD_TYPE_UNKNOWN : (SDL_GamepadType)types[port];
		pads[port].id = 100 + port;
		controller_device_update(port, types[port] == NONE ? NULL : &pads[port]);
	}
}

static void expect(short port, int style)
{
	int drawn = controller_button_style_for_port(port);

	CHECK(drawn == style, "port %d draws %s (%d), not %s (%d)", port, controller_button_style_name(drawn), drawn,
		controller_button_style_name(style), style);
}

static void defaults(void)
{
	set("display.hud_button_style", "auto");
	set("display.hud_button_style_xbox", "classic");
	set("display.hud_button_style_ps", "ps_classic");
	set("display.hud_button_style_switch", "switch_classic");
}

/* a setting's value as the style it names, by its name ("menu_gems" and
"dark_gems" both the Xbox's dark gems), else NONE */
static int named(char const *name)
{
	int style;

	if (!strcmp(name, "dark_gems"))
		return _button_style_dark_gems;
	for (style = 0; style < NUMBER_OF_BUTTON_STYLES; style++)
	{
		if (!strcmp(name, controller_button_style_name(style)))
			return style;
	}
	return NONE;
}

int main(
	int argc,
	char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("auto-by-family")
	{
		struct controller_device device;

		defaults();
		plug(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, SDL_GAMEPAD_TYPE_XBOXONE, NONE);
		expect(0, _button_style_ps_classic);
		expect(1, _button_style_switch_classic);
		expect(2, _button_style_classic);
		expect(3, _button_style_classic);
		controller_device_get(0, &device);
		CHECK(device.connected && device.family == _controller_playstation && device.instance == 100,
			"port 0's controller is %d, family %d, instance %u", device.connected, device.family, device.instance);
		controller_device_get(3, &device);
		CHECK(!device.connected, "port 3 has a controller");
		/* (the other types of each family; a kind with no family of its own
		is drawn as an Xbox's) */
		plug(SDL_GAMEPAD_TYPE_PS4, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR, SDL_GAMEPAD_TYPE_XBOX360,
			SDL_GAMEPAD_TYPE_GAMECUBE);
		expect(0, _button_style_ps_classic);
		expect(1, _button_style_switch_classic);
		expect(2, _button_style_classic);
		expect(3, _button_style_classic);
		plug(SDL_GAMEPAD_TYPE_PS3, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT,
			SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT, SDL_GAMEPAD_TYPE_STANDARD);
		expect(0, _button_style_ps_classic);
		expect(1, _button_style_switch_classic);
		expect(2, _button_style_switch_classic);
		expect(3, _button_style_classic);
		/* (each family's own setting) */
		set("display.hud_button_style_xbox", "xbox_modern");
		set("display.hud_button_style_ps", "ps_modern");
		set("display.hud_button_style_switch", "switch_gems");
		plug(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, SDL_GAMEPAD_TYPE_XBOXONE, NONE);
		expect(0, _button_style_ps_modern);
		expect(1, _button_style_switch_gems);
		expect(2, _button_style_xbox_modern);
		expect(3, _button_style_xbox_modern);
		/* (a port out of range has no controller) */
		controller_device_get(CONTROLLER_PORTS, &device);
		CHECK(!device.connected, "a port past the last has a controller");
		return 0;
	}
	CASE("wrong-family-setting")
	{
		/* (a family's setting naming another family's style, or nothing, is
		that family's default) */
		defaults();
		set("display.hud_button_style_xbox", "ps_gems");
		set("display.hud_button_style_ps", "switch_modern");
		set("display.hud_button_style_switch", "nonsense");
		plug(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, SDL_GAMEPAD_TYPE_XBOXONE, NONE);
		expect(0, _button_style_ps_classic);
		expect(1, _button_style_switch_classic);
		expect(2, _button_style_classic);
		expect(3, _button_style_classic);
		return 0;
	}
	CASE("one-style")
	{
		short port;
		int style;

		defaults();
		plug(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, SDL_GAMEPAD_TYPE_XBOXONE, NONE);
		for (style = 0; style < NUMBER_OF_BUTTON_STYLES; style++)
		{
			set("display.hud_button_style", controller_button_style_name(style));
			for (port = 0; port < CONTROLLER_PORTS; port++)
				expect(port, style);
		}
		/* ("dark_gems", the dark gems' other name) */
		set("display.hud_button_style", "dark_gems");
		for (port = 0; port < CONTROLLER_PORTS; port++)
			expect(port, _button_style_dark_gems);
		/* (a value of no style's is auto) */
		set("display.hud_button_style", "nonsense");
		expect(0, _button_style_ps_classic);
		expect(1, _button_style_switch_classic);
		return 0;
	}
	CASE("settings-cached")
	{
		defaults();
		plug(SDL_GAMEPAD_TYPE_PS5, NONE, NONE, NONE);
		set("display.hud_button_style", "ps_gems");
		expect(0, _button_style_ps_gems);
		/* (read again only once a setting changes) */
		set_quietly("display.hud_button_style", "switch_modern");
		expect(0, _button_style_ps_gems);
		changes++;
		expect(0, _button_style_switch_modern);
		/* (but the controller is not cached: a new one is drawn at once) */
		set("display.hud_button_style", "auto");
		expect(0, _button_style_ps_classic);
		plug(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, NONE, NONE, NONE);
		expect(0, _button_style_switch_classic);
		return 0;
	}
	CASE("settings-table")
	{
		size_t index;
		int style;

		defaults();
		plug(SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, SDL_GAMEPAD_TYPE_XBOXONE, NONE);
		/* (each value port_config.c allows draws its style, "auto" each
		controller's family's) */
		for (index = 0; index < NUMBEROF(allowed_styles); index++)
		{
			set("display.hud_button_style", allowed_styles[index]);
			if (!strcmp(allowed_styles[index], "auto"))
			{
				expect(0, _button_style_ps_classic);
				continue;
			}
			CHECK(named(allowed_styles[index]) != NONE, "display.hud_button_style allows %s, no style",
				allowed_styles[index]);
			expect(0, named(allowed_styles[index]));
		}
		/* (and every style can be chosen) */
		for (style = 0; style < NUMBER_OF_BUTTON_STYLES; style++)
		{
			for (index = 0; index < NUMBEROF(allowed_styles); index++)
			{
				if (!strcmp(allowed_styles[index], controller_button_style_name(style)))
					break;
			}
			CHECK(index < NUMBEROF(allowed_styles), "display.hud_button_style does not allow %s",
				controller_button_style_name(style));
		}
		/* (each family's setting allows its family's styles only, each drawn) */
		set("display.hud_button_style", "auto");
		for (index = 0; index < NUMBEROF(allowed_xbox); index++)
		{
			set("display.hud_button_style_xbox", allowed_xbox[index]);
			expect(2, named(allowed_xbox[index]));
			CHECK(controller_button_style_family(named(allowed_xbox[index])) == _controller_xbox,
				"the Xbox's setting allows %s", allowed_xbox[index]);
		}
		for (index = 0; index < NUMBEROF(allowed_ps); index++)
		{
			set("display.hud_button_style_ps", allowed_ps[index]);
			expect(0, named(allowed_ps[index]));
			CHECK(controller_button_style_family(named(allowed_ps[index])) == _controller_playstation,
				"the PlayStation's setting allows %s", allowed_ps[index]);
		}
		for (index = 0; index < NUMBEROF(allowed_switch); index++)
		{
			set("display.hud_button_style_switch", allowed_switch[index]);
			expect(1, named(allowed_switch[index]));
			CHECK(controller_button_style_family(named(allowed_switch[index])) == _controller_switch,
				"the Switch's setting allows %s", allowed_switch[index]);
		}
		return 0;
	}
	CASE("glyph-artwork")
	{
		static boolean used[NUMBEROF(controller_glyph_assets)];
		char expected[64];
		size_t asset;
		short slot;
		int style;

		for (style = 0; style < NUMBER_OF_BUTTON_STYLES; style++)
		{
			boolean redrawn = style == _button_style_classic || style == _button_style_gems ||
				style == _button_style_dark_gems;

			for (slot = 0; slot < CONTROLLER_GLYPH_SLOTS; slot++)
			{
				short key = controller_glyph_asset(style, slot);

				/* (the Xbox's first three are hud_hires_tags.c's, every button) */
				if (redrawn)
				{
					CHECK(key == NONE, "%s's slot %d has artwork here", controller_button_style_name(style), slot);
					continue;
				}
				CHECK(key != NONE, "%s has no artwork for slot %d", controller_button_style_name(style), slot);
				if (slot < CONTROLLER_GLYPH_FACES)
					snprintf(expected, sizeof(expected), "%s_%d", controller_button_style_name(style), slot);
				else
					snprintf(expected, sizeof(expected), "%s_control_%d",
						family_names[controller_button_style_family(style)], slot);
				CHECK(!strcmp(strrchr(controller_glyph_assets[key].tag, '\\') + 1, expected),
					"%s's slot %d is %s", controller_button_style_name(style), slot, controller_glyph_assets[key].tag);
				CHECK(controller_glyph_assets[key].bounds[2] > controller_glyph_assets[key].bounds[0] &&
					controller_glyph_assets[key].bounds[3] > controller_glyph_assets[key].bounds[1],
					"%s has no visible part", controller_glyph_assets[key].tag);
				used[key] = TRUE;
			}
		}
		CHECK(controller_glyph_asset(NUMBER_OF_BUTTON_STYLES, 0) == NONE &&
			controller_glyph_asset(_button_style_ps_classic, CONTROLLER_GLYPH_SLOTS) == NONE,
			"a style or slot out of range has artwork");
		/* (and no artwork is left unused) */
		for (asset = 0; asset < NUMBEROF(controller_glyph_assets); asset++)
			CHECK(used[asset], "no style draws %s", controller_glyph_assets[asset].tag);
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
