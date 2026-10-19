/*
GAMEPAD_LAYOUTS.H

The game pad's button layouts of the later Halo games (input.gamepad_layout),
as a table, applied over the player's profile's BUTTON LAYOUT. Included by
source/input/input_abstraction.c, which applies it; it holds data and no
code that needs another file, so every build that compiles the input
abstraction has it.

Each layout gives the button every control of Halo: Combat Evolved is on,
from the layout that game has in its default settings (Halopedia's control
scheme pages for each game), and the button its sprint is on (unlimited
sprint: gameplay.unlimited_sprint). What Combat Evolved has no control for
(thruster pack, armor ability, equipment, ground pound) is left unbound;
what it has but the later game put on the d-pad (grenade switching, the
flashlight) is on the d-pad too, which then no longer moves the player.
"profile" is no layout: the buttons are those of the profile's BUTTON LAYOUT
(the Xbox's, and Halo 2's, which are the same).

A button is a number of input.h's gamepad buttons; GAMEPAD_LAYOUT_NONE is
none. The controls are in input_abstraction.c's order.
*/

#ifndef GAMEPAD_LAYOUTS_H
#define GAMEPAD_LAYOUTS_H

/* no button */
#define GAMEPAD_LAYOUT_NONE 0xFF

enum
{
	_gamepad_layout_profile = 0,
	_gamepad_layout_halo2,
	_gamepad_layout_halo3,
	_gamepad_layout_halo4,
	_gamepad_layout_halo5,
	_gamepad_layout_infinite,
	NUMBER_OF_GAMEPAD_LAYOUTS
};

struct gamepad_layout
{
	/* config.toml's value */
	char const *name;
	/* the controls' buttons: jump, switch grenades, action (and reload),
	switch weapons, melee, flashlight, throw grenade, fire, (start and back:
	not changed), crouch, zoom */
	byte buttons[12];
	byte sprint;
};

/* A = 0, B, X, Y, black (right bumper), white (left bumper), left trigger,
right trigger, d-pad up, down, left, right, start, back, left stick,
right stick (input.h) */
static struct gamepad_layout const gamepad_layouts[NUMBER_OF_GAMEPAD_LAYOUTS] =
{
	/* the profile's: nothing here is used */
	{ "profile", { 0 }, GAMEPAD_LAYOUT_NONE },
	/* the Xbox's own, which Halo 2 has: A jump, B melee, X action and reload,
	Y switch weapons, black switch grenades, white flashlight, left trigger
	grenade, right trigger fire, left stick crouch, right stick zoom */
	{ "halo2", { 0, 4, 2, 3, 1, 5, 6, 7, 12, 13, 14, 15 }, GAMEPAD_LAYOUT_NONE },
	/* Halo 3: right bumper action and reload, left bumper switch grenades,
	d-pad up flashlight (X, its equipment, is unbound) */
	{ "halo3", { 0, 5, 4, 3, 1, 8, 6, 7, 12, 13, 14, 15 }, GAMEPAD_LAYOUT_NONE },
	/* Halo 4: B crouch, left stick sprint, right bumper melee, left bumper
	flashlight (its armor ability's place), d-pad right next grenade */
	{ "halo4", { 0, 11, 2, 3, 4, 5, 6, 7, 12, 13, 1, 15 }, 14 },
	/* Halo 5: left stick sprint, right stick crouch, left bumper grenade,
	right bumper melee, left trigger zoom (smart-link), d-pad right next
	grenade, d-pad up flashlight (orders) (B, its thruster pack, is unbound) */
	{ "halo5", { 0, 11, 2, 3, 4, 8, 5, 7, 12, 13, 15, 6 }, 14 },
	/* Halo Infinite: B crouch, left bumper grenade, right stick melee, left
	trigger zoom, left stick sprint, d-pad up flashlight, d-pad left grenade
	switching (right bumper, its equipment, is unbound) */
	{ "infinite", { 0, 10, 2, 3, 15, 8, 5, 7, 12, 13, 1, 6 }, 14 },
};

#endif
