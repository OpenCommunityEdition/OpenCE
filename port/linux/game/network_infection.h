#ifndef NETWORK_INFECTION_H
#define NETWORK_INFECTION_H

enum
{
	_distributed_infection_waiting,
	_distributed_infection_countdown,
	_distributed_infection_active,
	_distributed_infection_finished
};

struct distributed_infection_player
{
	unsigned long identity;
	long score;
	byte role; /* 0 absent, 1 human, 2 zombie */
	byte reserved[3];
};

struct distributed_infection_state
{
	long game_number;
	long revision;
	long phase;
	long phase_tick;
	long winner; /* team 0 human, 1 zombie, NONE cancelled */
	struct distributed_infection_player players[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
};

extern struct distributed_infection_state infection_state;
unsigned long infection_player_identity(long player_index);
boolean infection_snapshot_valid(struct distributed_infection_state const *state, long game_number, long revision);

#endif
