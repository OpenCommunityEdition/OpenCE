/* network_infection.h: Infection's state, which the host sends its clients
with the game type's (port/linux/game/infection.c) */
#ifndef HALO_NETWORK_INFECTION_H
#define HALO_NETWORK_INFECTION_H

enum
{
	/* fewer than two players alive: no countdown yet */
	_distributed_infection_waiting = 0,
	_distributed_infection_countdown,
	_distributed_infection_active,
	_distributed_infection_finished
};

enum
{
	_distributed_infection_role_none = 0,
	_distributed_infection_role_human,
	_distributed_infection_role_zombie
};

struct distributed_infection_player
{
	/* the player it is (infection_player_identity: no machine's datum
	index), 0 for none */
	unsigned long identity;
	/* the kills of the other side */
	long score;
	byte role;
	byte reserved[3];
};

struct distributed_infection_state
{
	/* the game it is of (network_game_get_number_of_games_played), and its
	revision, raised with each change */
	long game_number;
	long revision;
	long phase;
	/* the game time the phase began at */
	long phase_tick;
	/* the side that won (0 the humans, 1 the zombies), NONE */
	long winner;
	/* by absolute player index */
	struct distributed_infection_player players[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
};

/* (sent in the game type's state: game_engine.h) */
typedef char verify_distributed_infection_state_size[
	sizeof(struct distributed_infection_state) <= GAME_ENGINE_MAXIMUM_NETWORK_STATE_SIZE ? 1 : -1];

#endif
