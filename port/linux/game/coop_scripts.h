/*
COOP_SCRIPTS.H

Campaign scripts made to work for every player in network co-op
(coop_scripts.c).
*/

#ifndef __COOP_SCRIPTS_H
#define __COOP_SCRIPTS_H

#include "math/real_math.h"

/* ---------- prototypes */

/* hs_library_external.c's volume_test_objects_all: whether to count the
list as inside once any of it is, because every object in it is a player */
boolean coop_scripts_any_player_will_do(
	long object_list_index);
/* An empty (players) result or failed list allocation cannot advance a
co-op gate. Does not change valid empty AI lists or non-host behavior. */
boolean coop_scripts_empty_player_list(
	long object_list_index);
/* and when a script waits on it and one player is inside, the players of
the list outside come beside that one, so a gate (the Maw's lift, the run
to its bridge, a30's Pelicans) leaves no one behind. The caller must test
the volume again: a failed placement leaves the script waiting. */
void coop_scripts_gather_in_volume(
	short trigger_volume_index,
	long object_list_index);

/* What the scripts do to player0, done to the players they can't name too
(the "followers"). Each does nothing unless `unit_index` is player0's in a
co-op host's game. */

/* How far from a script's teleport the other players may stand. Farther
than this, an open doorway would walk the group into the next room. */
#define COOP_SCRIPT_TELEPORT_RADIUS 8.f

/* hs_library_external.c's object_teleport: player0's followers go to
`destination` (the cutscene flag), spreading around it so a large group
fits. `destination` is the flag even when player0 did not reach it. */
void coop_scripts_teleport_followers(
	long unit_index,
	real_point3d const *destination);
/* TRUE if the unit is within COOP_SCRIPT_TELEPORT_RADIUS of `destination` */
boolean coop_scripts_unit_near(
	long unit_index,
	real_point3d const *destination);
/* Place this player at `destination`, or on the floor around players
already listed in `arrived_unit_indices`. Returns whether they are there
afterwards. Nothing happens unless this machine is the co-op host. */
boolean coop_scripts_arrive_at(
	long player_index,
	real_point3d const *destination,
	long const *arrived_unit_indices,
	short arrived_count);
/* units.c's unit_suspended */
void coop_scripts_suspend_followers(
	long unit_index,
	boolean suspended);
/* units.c's unit_exit_vehicle: those riding player0's vehicle get out */
void coop_scripts_exit_followers(
	long unit_index,
	long vehicle_index);
/* units.c's vehicle_load_magic and unit_enter_vehicle: into the vehicle,
each in a free seat of the same kind as player0's (`seat_name`) */
void coop_scripts_board_followers(
	long unit_index,
	long vehicle_index,
	char const *seat_name);

/* hs.c's game_safe_to_save, for the scripts: the game's own test, or in
co-op, any one player alive, on the ground and unseen by enemies. The
scripts wait on it before a cutscene, and with players spread out a fight
somewhere else would hold up the one a player has reached. Checkpoints
keep the game's own test. */
boolean coop_scripts_safe_to_save(
	void);

/* hs.c's player_add_equipment, for the scripts: gives the starting profile
to player0's followers too */
void coop_scripts_player_add_equipment(
	long unit_index,
	short starting_profile_index,
	boolean reset_equipment);

#endif
