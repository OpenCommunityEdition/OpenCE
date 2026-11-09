/*
SCENARIO_PLACEMENT.H

An object a map places as it loads that the port holds back
(scenario_placement.c).
*/

#ifndef __SCENARIO_PLACEMENT_H
#define __SCENARIO_PLACEMENT_H

struct scenario_object_datum;
struct tag_block;

/* whether an object a map places as it loads is not placed: a10's Johnson,
in a local game (only its opening cinematic uses him, and creates him) */
boolean scenario_placement_withheld(struct scenario_object_datum const *scenario_object,
	struct tag_block const *palette);
/* (by the map's, the object's and its definition's names, in a game of the
connection given) */
boolean scenario_placement_withheld_by_name(short connection, char const *scenario_name,
	char const *object_name, char const *definition_name);

#endif
