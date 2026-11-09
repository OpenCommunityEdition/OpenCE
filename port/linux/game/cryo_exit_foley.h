/*
CRYO_EXIT_FOLEY.H

a10's short cryo scene (fast_setup, off Normal and in co-op) starts the
foley of the Master Chief climbing out of his cryo tube as far into his
climb as the tutorial on Normal does (cryo_exit_foley.c).
*/

#ifndef __CRYO_EXIT_FOLEY_H
#define __CRYO_EXIT_FOLEY_H

struct scenario;
struct data_array;

/* the begin whose root expression is root_expression_index, given the
script nodes and the strings they name: the foley's start (a
sound_looping_start of it, followed by the tube's unit_open) moved to 45
ticks after the begin's first unit_exit_vehicle after it, the sleep it
falls in split in two. TRUE if it was moved; FALSE, and nothing changed,
if the begin is not laid out so or the nodes have no room for the split */
boolean cryo_exit_foley_move(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index);

/* a10's fast_setup, as its scripts are made ready (hs_scenario_postprocess) */
void cryo_exit_foley_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax);

#endif
