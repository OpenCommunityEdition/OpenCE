/*
CAMPAIGN_CARRY.H

A won campaign level's weapons carried into the next (game.carry_weapons).
*/

#ifndef __CAMPAIGN_CARRY_H
#define __CAMPAIGN_CARRY_H

/* the local players' weapons and grenades as a level is won (main.c) */
void campaign_carry_take(void);
/* a map begun (game.c): the level they were taken for, or not */
void campaign_carry_new_map(char const *map_name);
/* a local player's unit given the level's starting loadout (players.c) */
void campaign_carry_give(long unit_index, short local_player_index);

#endif
