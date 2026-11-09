/* one_in_the_chamber.h: One in the Chamber, a mode of free for all Slayer
(port/linux/game/one_in_the_chamber.c) */
#ifndef HALO_ONE_IN_THE_CHAMBER_H
#define HALO_ONE_IN_THE_CHAMBER_H

struct game_engine;

/* Slayer's engine with One in the Chamber's rules (game_modes_engine) */
struct game_engine *one_in_the_chamber_engine_get(struct game_engine const *slayer);
/* a player spawned: their pistol, with one round (game_modes_postspawn) */
void one_in_the_chamber_postspawn(long player_index);
/* a kill's round, into a magazine's rounds loaded and carried, at most its
most of each: one more, or (resets) one loaded and none carried */
void one_in_the_chamber_round_for_kill(short *loaded, short *carried, short loaded_maximum, short carried_maximum,
	boolean resets);

#endif
