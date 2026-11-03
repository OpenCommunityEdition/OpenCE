/*
NETWORK_VOTEKICK.H

The players' votes to kick a player and to end the game
(network_votekick.c): what the scoreboard (game_engine.c), the vote's box
and its keys (hud.c, F1 and F2), the chat's "!endgame" (network_chat.c),
the console (hs.c) and the host's server (network_server_manager.c) ask of
it.
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

/* a vote's kind (its value on the wire: NETCODE.md) */
enum
{
	_network_vote_none,
	_network_vote_kick,
	_network_vote_end_game,
	NUMBER_OF_NETWORK_VOTES
};

/* ---------- structures */

/* the vote running, as the host counts it */
struct network_votekick_status
{
	short kind;
	/* the player voted against (absolute index; NONE: not a kick) */
	short player_index;
	/* yes, no, the yes needed */
	short votes;
	short no_votes;
	short needed;
	short seconds_left;
	/* this machine has voted yes; no; may vote */
	boolean voted;
	boolean voted_no;
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
/* this machine starts a vote to end the game (the chat's "!endgame") */
boolean network_vote_end_game(void);
/* this machine votes yes or no in the vote running (F1, F2); FALSE if it
cannot (no vote, voted, too new) */
boolean network_vote_choose(boolean yes);
/* the vote running's box, in the view of the local player (hud.c: drawn for
the first) */
void network_votekick_draw(short local_player_index);
/* whether the first controller's D-pad left and right vote yes and no now
(xinput_sdl.c keeps them from the game meanwhile) */
unsigned char network_vote_pad_open(void);
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
