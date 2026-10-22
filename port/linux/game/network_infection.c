#include "cseries/cseries.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "memory/data.h"
#include "networking/network_game_globals.h"
#include "infection.h"
#include "network_infection.h"

typedef char infection_state_fits[
	sizeof(struct distributed_infection_state) <= GAME_ENGINE_MAXIMUM_NETWORK_STATE_SIZE ? 1 : -1];

/* No machine-local datum handles on the wire. This identity also prevents a
   queued snapshot for a departed player being applied to a reused slot. */
unsigned long infection_player_identity(long player_index)
{
	struct player_datum *player = player_get(player_index);
	struct network_player const *net = &player->network_player_data;
	unsigned long hash = 2166136261UL;
	short index;
	for (index = 0; index < NUMBEROF(net->name); index++)
		hash = (hash ^ (word)net->name[index]) * 16777619UL;
	hash = (hash ^ (byte)net->machine_index) * 16777619UL;
	hash = (hash ^ (byte)net->controller_index) * 16777619UL;
	hash = (hash ^ (byte)net->player_list_index) * 16777619UL;
	return hash ? hash : 1;
}

boolean infection_snapshot_valid(struct distributed_infection_state const *state, long game_number, long revision)
{
	short index;
	if (state->game_number != game_number || state->revision < revision || state->revision < 0 ||
		state->phase < _distributed_infection_waiting || state->phase > _distributed_infection_finished ||
		state->phase_tick < 0 || state->winner < NONE || state->winner > 1 ||
		(state->phase != _distributed_infection_finished && state->winner != NONE))
		return FALSE;
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct distributed_infection_player const *player = &state->players[index];
		if (player->role > 2 || player->score < 0 || player->score > 1000000 ||
			(player->role && !player->identity) || player->reserved[0] || player->reserved[1] || player->reserved[2])
			return FALSE;
	}
	return TRUE;
}

long infection_write_network_state(byte *buffer, long size)
{
	if (size < (long)sizeof(struct distributed_infection_state))
		return 0;
	csmemcpy(buffer, &infection_state, sizeof(infection_state));
	return sizeof(infection_state);
}

boolean infection_read_network_state(byte const *buffer, long size, boolean first)
{
	struct distributed_infection_state state;
	struct data_iterator iterator;
	if (!network_game_distributed_client() || size != (long)sizeof(state))
		return FALSE;
	csmemcpy(&state, buffer, sizeof(state));
	if (!infection_snapshot_valid(&state, network_game_get_number_of_games_played(),
		first ? 0 : infection_state.revision))
		return FALSE;
	/* Validate everything before changing anything. Reliable ordered game
	   states and game_number fence previous rounds. */
	infection_state = state;
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
		infection_reconcile_player(iterator.datum_index);
	return TRUE;
}
