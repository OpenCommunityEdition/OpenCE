/*
COOP_ENEMIES.H

Network co-op's extra enemies, for its players (coop_enemies.c).
*/

#ifndef __COOP_ENEMIES_H
#define __COOP_ENEMIES_H

/* network_coop.c: a game begins (its EXTRA ENEMIES read), or the host's
game state went back (a cutscene skipped): the dropships' riders kept are
forgotten */
void coop_enemies_new_game(
	void);
void coop_enemies_reset(
	void);

/* encounters.c: how many enemies more a squad placed with `count` gets
(none but on a co-op host, for an encounter of the players' enemies) */
short coop_enemies_extra_count(
	long encounter_index,
	short count);

/* encounters.c: the place of a squad's `number`th extra enemy (from 1) at
its starting location `origin`: rings about it, on open ground; FALSE if
none is (the starting location itself then) */
boolean coop_enemies_spread_position(
	real_point3d const *origin,
	short number,
	real_point3d *position);

/* units.c's vehicle_scripting_load_magic: a rider seated, and one left
without a seat (the vehicle's seats taken: an extra enemy), which is kept,
off the map, until the vehicle's riders get out (TRUE: kept, the caller
erases it) */
void coop_enemies_rider_seated(
	long vehicle_index,
	long unit_index);
boolean coop_enemies_rider_unseated(
	long vehicle_index,
	long unit_index);

/* network_coop.c, after each of the host's ticks: the riders kept placed
at their vehicle's seats once its own riders get out */
void coop_enemies_update(
	void);

#endif
