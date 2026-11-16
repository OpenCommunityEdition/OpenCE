/*
CRYO_STEAM.H

a10's cryo bay: the opening cinematic's four vents of the Master Chief's
tube destroyed as its fade out begins, its white held until their steam
is gone, and the short cryo scene (fast_setup, off Normal and in co-op)
opening the tube without steam (cryo_steam.c).
*/

#ifndef __CRYO_STEAM_H
#define __CRYO_STEAM_H

struct data_array;
struct scenario;

/* the begin whose root expression is root_expression_index, given the
script nodes and the strings they name: its (object_destroy_containing
"x10_cryo_steam") moved to right before the fade_out before it, after its
(object_create_anew_containing "x10_cryo_steam"), and the (sleep 15) right
after the fade_out made a sleep of 45. TRUE if they were; FALSE, and
nothing changed, if the begin is not laid out so (or was changed already) */
boolean cryo_steam_vents_end_before_fade(
	struct data_array *syntax,
	char const *strings,
	long string_bytes,
	long root_expression_index);

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

/* a10's cryo and fast_setup, as its scripts are made ready
(hs_scenario_postprocess) */
void cryo_steam_scripts_loaded(
	struct scenario *scenario,
	struct data_array *syntax);

#endif
