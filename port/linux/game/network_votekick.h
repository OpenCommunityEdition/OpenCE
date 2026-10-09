/*
NETWORK_VOTEKICK.H

The players' vote to kick a player (network_votekick.c): what the
scoreboard (game_engine.c), the console (hs.c) and the host's server
(network_server_manager.c) ask of it.
*/

#ifndef __NETWORK_VOTEKICK_H
#define __NETWORK_VOTEKICK_H
#pragma once

/* ---------- constants */

enum
{
	/* a player's name in ASCII, with its end */
	VOTEKICK_NAME_TEXT_SIZE = 16,
};

/* ---------- structures */

/* the vote running, as the host counts it */
struct network_votekick_status
{
	/* the player voted against (absolute index) */
	short player_index;
	short votes;
	short needed;
	short seconds_left;
	/* this machine has voted; may vote */
	boolean voted;
	boolean may_vote;
};

/* ---------- prototypes/NETWORK_VOTEKICK.C */

/* whether this machine is in a network game, where votes are */
boolean network_votekick_available(void);
/* the vote running; FALSE for none */
boolean network_votekick_get_status(struct network_votekick_status *status);
/* this machine votes against the player (absolute index): starts a vote,
or votes in the one running; the host answers on the console */
boolean network_votekick_request(short player_index);
/* whether this machine hosts the game (its scoreboard offers Kick and Ban) */
boolean network_votekick_host(void);
/* (the host) kicks, or bans, the machine of the player (absolute index):
its kick and ban commands' (network_server_manager.c) */
boolean network_votekick_host_kick(short player_index, boolean ban);
/* the console's "votekick <name>": the player of that name (or the one
whose name begins with it) voted against */
boolean network_votekick_player_named(char const *text);
/* the names of the game's players but this machine's that begin with the
text (the command's completion: console.c); how many */
short network_votekick_matching_player_names(char const *text, char (*names)[VOTEKICK_NAME_TEXT_SIZE],
	short maximum_count);
/* (the host's server) a machine joined at the slot */
void network_votekick_machine_joined(long machine_index);
/* (the host's server) whether a machine of the address (host byte order)
and hardware id is kept out, kicked by a vote */
boolean network_votekick_kept_out(unsigned long address, char const *hardware_id);

#endif // __NETWORK_VOTEKICK_H
