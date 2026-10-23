/* gun_game.h: Gun Game, a mode of free for all Slayer (port/linux/game/gun_game.c) */
#ifndef HALO_GUN_GAME_H
#define HALO_GUN_GAME_H

struct game_engine;

/* Slayer's engine with Gun Game's rules (game_modes_engine) */
struct game_engine *gun_game_engine_get(struct game_engine const *slayer);
/* a player spawned: their weapon (game_modes_postspawn) */
void gun_game_postspawn(long player_index);
/* the weapons of the map's list (each machine's alike), and a level's (a
_loadout_weapon_*) */
short gun_game_level_count(void);
byte gun_game_level_weapon(short level);

#endif
