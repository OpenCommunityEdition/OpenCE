/*
MUTATORS.H

The mutators a game plays (mutators.c): their tag edits and poses and their
scripts, the same on every machine of a network game (network_mutators.c).
The mutators' files are port/linux/src/port_mutators.c's.
*/

#ifndef MUTATORS_H
#define MUTATORS_H

/* ---------- constants */

/* a key: a script's ("script." and its mutator's digest's first digits),
ended */
#define MUTATOR_KEY_LENGTH 32
/* the most keys a game plays */
#define MAXIMUM_MUTATOR_KEYS 32
/* the most bytes of tag edits a game plays, all its mutators' (mutator_tags.c
says what the bytes are) */
#define MAXIMUM_MUTATOR_TAG_EDIT_BYTES 32768

/* a tag edit's kind of value */
enum
{
	_mutator_tag_edit_number,
	/* a tag's name, an enum's, or flags' separated by commas */
	_mutator_tag_edit_text,
	/* elements of a block copied from another tag's */
	_mutator_tag_edit_copy,
	/* a node of objects scaled and moved (object_pose.c) */
	_mutator_tag_edit_pose,
	/* what a unit of a group does as it dies (object_death.c) */
	_mutator_tag_edit_death,
	/* the decal every explosion leaves (explosion_decals.c) */
	_mutator_tag_edit_explosion,
	NUMBER_OF_MUTATOR_TAG_EDIT_KINDS
};

/* ---------- prototypes/MUTATORS.C */

/* a new map (network_distributed_new_game): the mutators turned on, on the
host and in a game of one machine; none on a client until the host's come
(mutators_set_host) */
void mutators_new_game(void);
/* whether a script or a tag edit is played */
boolean mutators_active(void);

/* the keys the game plays and their values (1): how many */
short mutators_keys(char keys[][MUTATOR_KEY_LENGTH], real *values, short maximum);
/* (network_mutators.c) a client's: the host's keys, played instead of any
before; a key this build does not have is passed over and said on the
console. FALSE, and nothing played, when one is not a key's text or a
value is not a number */
boolean mutators_set_host(char const keys[][MUTATOR_KEY_LENGTH], real const *values, short count);

/* (port_mutators.c) whether a mutator.toml's effect's key and value are this
build's: none is, the game having no effects built in; why not into reason */
int mutators_key_check(char const *key, double value, char *reason, unsigned long size);

/* the tag edits the game plays (the host's, on a client): how many bytes */
long mutators_tag_edits(byte const **edits);
/* (network_mutators.c) a client's: the host's tag edits, played instead of
any before. FALSE, and none played, when the bytes are not edits or one is
not an edit this build makes */
boolean mutators_set_host_tag_edits(byte const *edits, long size);

/* ---------- prototypes/MUTATOR_TAGS.C */

/* (port_mutators.c) whether one edit's bytes are an edit this build makes,
in a tag of a group it edits, of a field it sets, with a value in range; if
not, why not into reason */
int mutators_tag_edit_check(byte const *edit, long size, char *reason, unsigned long reason_size);
/* (scenario_tags_load) a map's tags loaded: none of them edited */
void mutator_tags_loaded(void);
/* the edits made undone */
void mutator_tags_undo(void);
/* the last edits undone, then these made: an edit of a tag, element or tag
named that the map has not is passed over (and logged). FALSE, and none
made, when the bytes are not edits or one is not one this build makes */
boolean mutator_tags_play(byte const *edits, long size);

#endif
