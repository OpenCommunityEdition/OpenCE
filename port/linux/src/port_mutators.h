/*
PORT_MUTATORS.H

The mutators (port_mutators.c): those built into the game and those in the
data root's mutators folder, each a mutator.toml naming effects the game
has built in (port/linux/game/mutators.c) and their parameters' values, and
which are on (game.mutators). port/linux/game/mutators.c and
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

/* ---------- the Mods menu's */

long port_mutators_count(void);
const char *port_mutators_name(long index);
int port_mutators_on(long index);
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
