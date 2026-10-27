/*
CONTROLLER_BUTTONS.H

Each port's controller, as SDL tells it (its family: Xbox, PlayStation,
Switch), which port/linux/src/xinput_sdl.c keeps as it reads the pads, and
the HUD's button styles: display.hud_button_style names one for every
viewport, or "auto" for each viewport's controller's family's
(display.hud_button_style_xbox, _ps and _switch). The styles draw the
buttons only: they change no input.
*/

#ifndef CONTROLLER_BUTTONS_H
#define CONTROLLER_BUTTONS_H

#define CONTROLLER_PORTS 4

enum controller_family
{
	_controller_unknown,
	_controller_xbox,
	_controller_playstation,
	_controller_switch,
	NUMBER_OF_CONTROLLER_FAMILIES
};

/* display.hud_button_style's values, in order (the Xbox's first three are
port/assets/buttons' redraws, hud_hires_tags.c's; the others are
port/assets/controller_buttons', controller_glyphs.c's) */
enum controller_button_style
{
	_button_style_classic,
	_button_style_gems,
	_button_style_dark_gems,
	_button_style_xbox_modern,
	_button_style_ps_classic,
	_button_style_ps_gems,
	_button_style_ps_dark_gems,
	_button_style_ps_modern,
	_button_style_switch_classic,
	_button_style_switch_gems,
	_button_style_switch_modern,
	NUMBER_OF_BUTTON_STYLES
};

struct controller_device
{
	int connected;
	int family;
	/* SDL's joystick instance: the same controller while it stays plugged in */
	unsigned int instance;
};

/* a port's controller (gamepad: its SDL_Gamepad, or NULL), and what it is */
void controller_device_update(short port, void *gamepad);
void controller_device_get(short port, struct controller_device *device);

/* a style's name (display.hud_button_style's value) and its family */
char const *controller_button_style_name(int style);
int controller_button_style_family(int style);
/* the style a local player's viewport draws its buttons in */
int controller_button_style_for_port(short port);

#endif
