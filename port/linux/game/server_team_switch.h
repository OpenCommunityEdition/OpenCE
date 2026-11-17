#ifndef SERVER_TEAM_SWITCH_H
#define SERVER_TEAM_SWITCH_H

/* (the host of a dedicated server; network_chat.c) a player's "!switch" or
"!team": TRUE when they were moved to the other team, notice (of
notice_size bytes) then saying so to every machine; FALSE with why not, for
them alone. name is the player's, as the chat writes it */
boolean server_team_switch(long player_index, char const *name, char *notice, long notice_size);

/* (players_initialize_for_new_map) forgets every cooldown and move: a new
map's players reuse the last map's player indices */
void server_team_switch_initialize_for_new_map(void);

/* (object_damage_aftermath, around its game_engine_player_killed) whether
the death of a player's unit is the no-statistics one a switch asked for */
void server_team_switch_damage(long player_index, long unit_index, boolean no_statistics);

/* (game_engine_player_killed, before the gametype hears of it) a switching
player's death is no one's kill */
void server_team_switch_player_killed(long *killing_player_index, long *killing_object_index,
	long dead_player_index, boolean *friendly_fire);

/* (game_engine_player_killed, where the auto team balance moves the dead) a
switching player is moved to their new team */
void server_team_switch_player_dead(long dead_player_index);

#endif
