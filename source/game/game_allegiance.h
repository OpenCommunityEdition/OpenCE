#ifndef __GAME_ALLEGIANCE_H
#define __GAME_ALLEGIANCE_H
#pragma once

void game_allegiance_dispose(
	void);
void game_allegiance_dispose_from_old_map(
	void);
boolean game_team_is_enemy(
	short team_index0,
	short team_index1);
boolean game_allegiance_remove(
	short team1_index,
	short team2_index);

#endif // __GAME_ALLEGIANCE_H
