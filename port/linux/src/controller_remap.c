/*
CONTROLLER_REMAP.C

The controllers' own button layouts (controller_remap.h): a layout is the
button each of ten of the game's controls is on, its setting's text their
names in that order, by where the buttons are ("south" is the Xbox's A, the
PlayStation's cross and the Switch's B), so that the one layout means the
same hand on any controller. The button styles (controller_buttons.h) only
draw the buttons: they never change a layout.
*/

#include "controller_remap.h"
#include "controller_buttons.h"
#include "port_config.h"

#include <stdio.h>
#include <string.h>

/* the buttons a layout can use (not start, back or the d-pad's other three,
which the game's menus and controls keep), each by its setting's name and
its name on each family's controller */
static struct
{
	unsigned char button;
	char const *name;
	char const *labels[NUMBER_OF_CONTROLLER_FAMILIES];
} const remap_buttons[CONTROLLER_REMAP_BUTTONS] =
{
	{ _remap_button_a, "south", { "A", "A", "CROSS", "B" } },
	{ _remap_button_b, "east", { "B", "B", "CIRCLE", "A" } },
	{ _remap_button_x, "west", { "X", "X", "SQUARE", "Y" } },
	{ _remap_button_y, "north", { "Y", "Y", "TRIANGLE", "X" } },
	{ _remap_button_black, "rb", { "RB / BLACK", "RB / BLACK", "R1", "R" } },
	{ _remap_button_white, "lb", { "LB / WHITE", "LB / WHITE", "L1", "L" } },
	{ _remap_button_left_trigger, "lt", { "LT", "LT", "L2", "ZL" } },
	{ _remap_button_right_trigger, "rt", { "RT", "RT", "R2", "ZR" } },
	{ _remap_button_left_thumb, "ls", { "LEFT STICK", "LEFT STICK", "L3", "LEFT STICK" } },
	{ _remap_button_right_thumb, "rs", { "RIGHT STICK", "RIGHT STICK", "R3", "RIGHT STICK" } },
	{ _remap_button_dpad_left, "dpad_left", { "D-PAD LEFT", "D-PAD LEFT", "D-PAD LEFT", "D-PAD LEFT" } },
};

/* a button's place in remap_buttons, else -1 */
static int remap_button_index(int button)
{
	int index;

	for (index = 0; index < CONTROLLER_REMAP_BUTTONS; index++)
	{
		if (remap_buttons[index].button == button)
			return index;
	}
	return -1;
}

int controller_remap_bindable(int button)
{
	return remap_button_index(button) >= 0;
}

char const *controller_remap_setting(short port)
{
	static char const *const settings[CONTROLLER_PORTS] =
	{
		"input.controller_p1", "input.controller_p2", "input.controller_p3", "input.controller_p4"
	};

	return port >= 0 && port < CONTROLLER_PORTS ? settings[port] : NULL;
}

int controller_remap_parse(char const *text, unsigned char *buttons)
{
	unsigned char parsed[CONTROLLER_REMAP_ACTIONS];
	unsigned int used = 0;
	int action, index;

	if (!text || !*text)
		return 0;
	for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
	{
		size_t length = strcspn(text, ",");

		for (index = 0; index < CONTROLLER_REMAP_BUTTONS; index++)
		{
			if (strlen(remap_buttons[index].name) == length && !strncmp(text, remap_buttons[index].name, length))
				break;
		}
		/* (each button once) */
		if (index == CONTROLLER_REMAP_BUTTONS || (used & (1u << index)))
			return 0;
		used |= 1u << index;
		parsed[action] = remap_buttons[index].button;
		text += length;
		if (action < CONTROLLER_REMAP_ACTIONS - 1 ? *text++ != ',' : *text != 0)
			return 0;
	}
	memcpy(buttons, parsed, sizeof(parsed));
	return 1;
}

void controller_remap_format(unsigned char const *buttons, char *text, unsigned int size)
{
	unsigned int used = 0;
	int action;

	if (!size)
		return;
	*text = 0;
	for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
	{
		int index = remap_button_index(buttons[action]);

		if (index < 0 || used + strlen(remap_buttons[index].name) + 2 > size)
		{
			*text = 0;
			return;
		}
		used += (unsigned int)snprintf(text + used, size - used, "%s%s", action ? "," : "", remap_buttons[index].name);
	}
}

int controller_remap_read(short port, unsigned char *buttons)
{
	/* (read again only when a setting changes: the controller is read
	every tick) */
	static unsigned long read_at = (unsigned long)-1;
	static unsigned char layouts[CONTROLLER_PORTS][CONTROLLER_REMAP_ACTIONS];
	static int valid[CONTROLLER_PORTS];
	short each;

	if (port < 0 || port >= CONTROLLER_PORTS)
		return 0;
	if (read_at != config_changes())
	{
		read_at = config_changes();
		for (each = 0; each < CONTROLLER_PORTS; each++)
			valid[each] = controller_remap_parse(config_string(controller_remap_setting(each)), layouts[each]);
	}
	if (valid[port])
		memcpy(buttons, layouts[port], CONTROLLER_REMAP_ACTIONS);
	return valid[port];
}

void controller_remap_swap(unsigned char *buttons, int action, int button)
{
	int other;

	if (action < 0 || action >= CONTROLLER_REMAP_ACTIONS || remap_button_index(button) < 0)
		return;
	for (other = 0; other < CONTROLLER_REMAP_ACTIONS; other++)
	{
		if (buttons[other] == button)
		{
			buttons[other] = buttons[action];
			break;
		}
	}
	buttons[action] = (unsigned char)button;
}

int controller_remap_cycle(int button, int direction)
{
	int index = remap_button_index(button);

	if (index < 0)
		return remap_buttons[0].button;
	index = (index + (direction < 0 ? CONTROLLER_REMAP_BUTTONS - 1 : 1)) % CONTROLLER_REMAP_BUTTONS;
	return remap_buttons[index].button;
}

char const *controller_remap_button_name(int button, int family)
{
	int index = remap_button_index(button);

	if (family < 0 || family >= NUMBER_OF_CONTROLLER_FAMILIES)
		family = _controller_unknown;
	return index < 0 ? "-" : remap_buttons[index].labels[family];
}
