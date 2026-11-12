#ifndef SERVER_TEAM_SWITCH_H
#define SERVER_TEAM_SWITCH_H

/* (the host of a dedicated server; network_chat.c) a player's "!switch" or
"!team": TRUE when they were moved to the other team, notice (of
notice_size bytes) then saying so to every machine; FALSE with why not, for
them alone. name is the player's, as the chat writes it */
boolean server_team_switch(long player_index, char const *name, char *notice, long notice_size);

#endif
