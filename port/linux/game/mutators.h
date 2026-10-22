/*
MUTATORS.H

The mutators a game plays (mutators.c): effects built into the game that
change its rules, each set by the mutators' parameters, the same on every
machine of a network game (network_mutators.c). The mutators' files are
port/linux/src/port_mutators.c's.
*/

#ifndef MUTATORS_H
#define MUTATORS_H

/* ---------- constants */

/* a parameter's key: its effect's name ("big_head"), which turns the effect
on, or that and the parameter's ("big_head.scale"), ended */
#define MUTATOR_KEY_LENGTH 32
/* the most keys a game plays (every effect and every parameter fit) */
#define MAXIMUM_MUTATOR_KEYS 32

enum
{
	_mutator_effect_big_head,
	NUMBER_OF_MUTATOR_EFFECTS
};

enum
{
	_mutator_big_head_scale,
	NUMBER_OF_MUTATOR_PARAMETERS
};

/* ---------- prototypes/MUTATORS.C */

/* a new map (network_distributed_new_game): the mutators turned on, on the
host and in a game of one machine; none on a client until the host's come
(mutators_set_host) */
void mutators_new_game(void);
boolean mutators_effect_active(short effect);
/* a parameter of an effect that is on, else its default */
real mutators_parameter(short parameter);
/* whether any effect is on */
boolean mutators_active(void);

/* the keys the game plays and their values (a parameter's, or 1 for an
effect's): how many */
short mutators_keys(char keys[][MUTATOR_KEY_LENGTH], real *values, short maximum);
/* (network_mutators.c) a client's: the host's keys, played instead of any
before; a key this build does not have is passed over and said on the
console. FALSE, and nothing played, when one is not a key's text or a
value of a key this build has is out of its range */
boolean mutators_set_host(char const keys[][MUTATOR_KEY_LENGTH], real const *values, short count);

/* (port_mutators.c) whether a mutator.toml's key and value are this build's
and in range; if not, why not into reason */
int mutators_key_check(char const *key, double value, char *reason, unsigned long size);

/* ---------- prototypes/MUTATOR_BIG_HEAD.C */

/* (biped_preprocess_node_orientations) a biped's head node scaled */
void mutator_big_head_node_orientations(long biped_index, struct real_orientation *node_orientations);
/* (object_compute_node_matrices_non_recursive) an object's bounding radius,
made to hold its head as scaled */
real mutator_big_head_bounding_radius(long object_index, real bounding_radius);
/* (object_get_render_bounding_sphere) the radius an object is culled by,
made to hold its head as scaled */
real mutator_big_head_render_radius(long object_index, real render_radius);
/* a new map: the heads' sizes the radius is found with forgotten */
void mutator_big_head_new_game(void);

#endif
