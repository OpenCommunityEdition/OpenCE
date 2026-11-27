/*
NETWORK_MAP_VOTE.H

A dedicated server's vote for the next game in its lobby
(network_map_vote.c): what its director (server_director.c), the lobby's
vote screen (menu_functions.c) and the netcode's lobby messages
(network_distributed.c, network_server_message_handler.c) ask of it.
*/

#ifndef __NETWORK_MAP_VOTE_H
#define __NETWORK_MAP_VOTE_H
#pragma once

/* ---------- constants */

enum
{
	NETWORK_MAP_VOTE_MAXIMUM_CHOICES = 8,
	/* a choice's map (its own name: bloodgulch) and gametype, ASCII, with
	their ends */
	NETWORK_MAP_VOTE_MAP_SIZE = 32,
	NETWORK_MAP_VOTE_GAMETYPE_SIZE = 24,
};

/* ---------- structures */

/* the vote, as this machine last heard it (the host: as it counts it) */
struct network_map_vote_status
{
	/* which vote (each new one's is one more) */
	short round;
	short count;
	char map_names[NETWORK_MAP_VOTE_MAXIMUM_CHOICES][NETWORK_MAP_VOTE_MAP_SIZE];
	char gametypes[NETWORK_MAP_VOTE_MAXIMUM_CHOICES][NETWORK_MAP_VOTE_GAMETYPE_SIZE];
	short votes[NETWORK_MAP_VOTE_MAXIMUM_CHOICES];
	short seconds_left;
	/* this machine's choice, NONE for none */
	short chosen;
	/* the vote over: the choice that won (NONE while it runs) */
	short winner;
};

/* ---------- prototypes/NETWORK_MAP_VOTE.C */

/* (the host's director) a vote among the choices for so many seconds */
void network_map_vote_begin(char const (*map_names)[NETWORK_MAP_VOTE_MAP_SIZE],
	char const (*gametypes)[NETWORK_MAP_VOTE_GAMETYPE_SIZE], short count, short seconds);
/* ... each frame while it runs: its count told every client once a second
(and as it changes); the choice that won once it is over (its time up, or
every machine voted), else NONE */
short network_map_vote_host_update(void);
/* ... whether one runs */
boolean network_map_vote_running(void);
/* ... the votes a choice had, as last counted */
short network_map_vote_votes(short choice);
/* ... the vote dropped (the server's lobby left, or its rotation changed) */
void network_map_vote_cancel(void);

/* (a client) the vote running, or just over; FALSE for none */
boolean network_map_vote_status_get(struct network_map_vote_status *status);
/* ... this machine's vote for a choice (it may choose again while the vote
runs) */
boolean network_map_vote_choose(short choice);
/* ... no vote heard any more (a game left) */
void network_map_vote_reset(void);

/* the netcode's: whether a message is one of the vote's, which go in the
lobby too, and its handling (machine_index: the sender's on the host, NONE
on a client; from_stream: over the sender's own connection) */
boolean network_map_vote_handles_message(word const *message, word size);
void network_map_vote_handle_message(long machine_index, word const *message, word size, boolean from_stream);

#endif // __NETWORK_MAP_VOTE_H
