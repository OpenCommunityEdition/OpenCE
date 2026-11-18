/*
CRYO_STEAM.H

a10's cryo bay: the short cryo scene (fast_setup, off Normal and in co-op)
opening the Master Chief's tube without steam (cryo_steam.c).
*/

#ifndef __CRYO_STEAM_H
#define __CRYO_STEAM_H

struct data_array;
struct scenario;

/* the begin whose root expression is root_expression_index: its
(object_create cryotube_1_steam_1) and (object_create cryotube_1_steam_2),
and its (if (game_is_cooperative) (object_create cryotube_2_steam_1)) and
(... cryotube_2_steam_2), taken out. TRUE if they were; FALSE, and nothing
changed, unless the begin has each of the four once */
boolean cryo_steam_wake_without_steam(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index);

/* a10's fast_setup, as its scripts are made ready
(hs_scenario_postprocess) */
void cryo_steam_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax);

#endif
