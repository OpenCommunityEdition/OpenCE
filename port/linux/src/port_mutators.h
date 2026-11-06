/*
PORT_MUTATORS.H

The mutators (port_mutators.c): the stock mutators (in the data root's
stock_mutators folder) and those in its mutators folder, each a mutator.toml naming effects the game
has built in (port/linux/game/mutators.c) and their parameters' values, and
the tags' values it sets (port/linux/game/mutator_tags.c), and which are on
(game.mutators). port/linux/game/mutators.c and
menu_functions.c declare these for themselves: keep them the same.
*/

#ifndef PORT_MUTATORS_H
#define PORT_MUTATORS_H

/* a key's length, ended (mutators.h's MUTATOR_KEY_LENGTH) */
#define PORT_MUTATOR_KEY_LENGTH 32

/* what is wrong with a mutator (port_mutators_status): the Mods menu's
values for a mod (port_mods.h) */
enum
{
	_port_mutator_fine = 0,
	/* it cannot be played: a key or value this build does not have, a later
	format, or another mutator's id */
	_port_mutator_unplayable = 3,
};

/* finds the mutators again, and which are on (game.mutators): how many */
long port_mutators_scan(void);
/* the keys and values of the mutators on that can be played, in their order
(those found first first): how many, at most `maximum` */
long port_mutators_keys(char keys[][PORT_MUTATOR_KEY_LENGTH], double *values, long maximum);
/* the tag edits of the mutators on that can be played, in their order: their
bytes (port/linux/game/mutator_tags.c's) into edits, at most `maximum`; how
many */
long port_mutators_tag_edits(unsigned char *edits, long maximum);

/* where a mutator places an add-on's vehicle (docs/addons.md): on the map of
the file's name, the vehicle of the tag's name, at a position in world units
facing a direction in degrees */
struct port_mutator_place
{
	char map[64];
	/* the vehicle's or (weapon) the weapon's tag */
	char tag[256];
	int weapon;
	float position[3];
	float facing;
};

/* an add-on's kind, by its file's name (addon_file.h's) */
enum
{
	_port_mutator_addon_vehicle = 1,
	_port_mutator_addon_weapon,
};

/* the mutators on that can be played that add an add-on, in their order:
how many */
long port_mutators_addon_count(void);
/* the size of the add-on's file (0 if it cannot be read), its mutator's id
into name */
unsigned long port_mutators_addon_size(long index, char *name, unsigned long name_size);
/* its file's size bytes into buffer: 0 if they cannot be read */
int port_mutators_addon_read(long index, void *buffer, unsigned long size);
/* its kind (_port_mutator_addon_*), by its file's name, and its mutator's
name (what the menus call its vehicle or weapon); 0 and NULL past them */
int port_mutators_addon_kind(long index);
const char *port_mutators_addon_name(long index);
/* the placements of the mutators on that can be played, in their order: how
many, at most `maximum` */
long port_mutators_places(struct port_mutator_place *places, long maximum);

/* ---------- the Mods menu's */

long port_mutators_count(void);
const char *port_mutators_name(long index);
int port_mutators_on(long index);
/* the options ([[option]]) of the mutators on, in their order, each a percent the host sets (MUTATOR
SETTINGS): its name, value, least, most and step; and one set (game.mutator_options) */
long port_mutators_option_count(void);
int port_mutators_option(long index, char *name, unsigned long size, long *value, long *minimum, long *maximum,
	long *step);
int port_mutators_option_set(long index, long value);
/* what is wrong with a mutator, and a line saying so into text (else empty) */
int port_mutators_status(long index, char *text, unsigned long size);
/* what its mutator.toml says of a mutator, a line each (its version,
authors, folder and effects), then its description; 0 if there is none */
int port_mutators_details(long index, char *text, unsigned long size);
void port_mutators_turn(long index, int on);
/* keeps the mutators on in game.mutators; 0 if it cannot be written */
int port_mutators_save(void);
/* the mutators on as game.mutators has them */
void port_mutators_revert(void);

#endif
