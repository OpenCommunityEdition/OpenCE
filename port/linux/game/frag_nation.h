/* frag_nation.h: Frag Nation and Snowball Fight, modes of Slayer played
with grenades (port/linux/game/frag_nation.c) */
#ifndef HALO_FRAG_NATION_H
#define HALO_FRAG_NATION_H

struct game_engine;

/* the damage a kill was made with */
enum
{
	_frag_nation_kill_other = 0,
	/* a frag grenade's explosion */
	_frag_nation_kill_frag,
	/* a plasma grenade's explosion */
	_frag_nation_kill_plasma,
	/* a plasma grenade stuck to the one killed */
	_frag_nation_kill_plasma_stuck,
	NUMBER_OF_FRAG_NATION_KILLS
};

/* Slayer's engine with Frag Nation's (or Snowball Fight's) rules
(game_modes_engine) */
struct game_engine *frag_nation_engine_get(struct game_engine const *slayer);
/* a player spawned: no weapon, and the mode's grenades (game_modes_postspawn) */
void frag_nation_postspawn(long player_index);
/* the grenades a kill gives back (a frag's a frag, a plasma's a plasma, a
plasma stuck two; with plasmas only, Snowball Fight's, no frag) */
void frag_nation_grenades_for_kill(short kill, boolean plasmas_only, short *frags, short *plasmas);
/* a count of grenades given more, up to the most a unit carries (a count
already past it kept) */
short frag_nation_grenades_added(short count, short added, short maximum);

#endif
