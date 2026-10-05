/*
COOP_SCRIPTS.H

Campaign scripts made to work for every player in network co-op
(coop_scripts.c).
*/

#ifndef __COOP_SCRIPTS_H
#define __COOP_SCRIPTS_H

/* ---------- prototypes */

/* hs_library_external.c's volume_test_objects_all: whether to count the
list as inside once any of it is, because every object in it is a player */
boolean coop_scripts_any_player_will_do(
	long object_list_index);

/* The units of the players the scripts can't name, when `unit_index` is
player0's: they go along with whatever the scripts do to player0. Returns
how many, 0 outside a co-op host's game or for any other unit. */
short coop_scripts_players_following(
	long unit_index,
	long *unit_indices,
	short maximum_count);

/* units.c's vehicle_load_magic and unit_enter_vehicle: puts the players
following player0 into the vehicle as well, each in a free seat of the
same kind as player0's (`seat_name`) */
void coop_scripts_board_followers(
	long unit_index,
	long vehicle_index,
	char const *seat_name);

/* hs.c's player_add_equipment, for the scripts: gives the starting profile
to player0's followers too */
void coop_scripts_player_add_equipment(
	long unit_index,
	short starting_profile_index,
	boolean reset_equipment);

#endif
