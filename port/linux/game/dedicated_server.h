/*
DEDICATED_SERVER.H

The dedicated server (dedicated_server.c; port/linux/README.md, "Dedicated
server"): the game hosting with no window, sound or player of its own
(server.dedicated). What the game's own code asks it.
*/

#ifndef __DEDICATED_SERVER_H
#define __DEDICATED_SERVER_H

/* whether this machine is a dedicated server (server.dedicated; never on
Android or the web) */
boolean dedicated_server_active(void);
/* the server's work each frame (main.c, beside network_test_update) */
void dedicated_server_update(boolean main_menu_loaded, real seconds);
/* (network_server_manager.c) the lobby may count down to its game: the
server has set its map and gametype */
boolean dedicated_server_lobby_ready(void);
/* ... the lobby's countdown, in milliseconds (sv_start_delay) */
long dedicated_server_countdown_milliseconds(void);
/* ... the players a game needs to start (sv_minplayers) */
long dedicated_server_minimum_players(void);
/* ... whether a connection from this address (host byte order; an internet
play peer's stand-in counts as its real address) may join: so many a minute
from one address (counted) */
boolean dedicated_server_join_allowed(unsigned long address);

#endif
