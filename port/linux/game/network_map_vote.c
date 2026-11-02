/*
NETWORK_MAP_VOTE.C

A dedicated server's vote for the next game (server.map_vote;
docs/dedicated_server.md, "Map votes"; port/linux/NETCODE.md). As its lobby
opens after a game with players in it, the director picks
server.map_vote_choices (4) entries of its rotation at random and every
client's lobby opens its vote screen (menu_functions.c): the choices, their
maps' pictures and their votes. A player picks one (and may pick again);
the host counts a vote a machine, over its own connection alone, and once
server.map_vote_seconds (20) are up, or every machine has voted, the choice
with the most votes is played next (a tie, or no vote, chosen at random
among them). The rotation goes on after it.

The host tells every client the vote (_distributed_message_map_vote) once a
second and as it changes, and the result as it ends; a client tells the
host its choice (_distributed_message_map_vote_choice). Both go in the
lobby, reliably, as map sharing's do. A build without them drops them as
kinds it does not know: its lobby shows no vote, and its players do not
vote.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_map_vote.h"

#include <stdio.h>
#include <string.h>

/* the netcode's (network_game_globals.c, network_server_message_handler.c,
network_messages.c), the platform's */
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
short network_distributed_server_machines(long *machine_indices, short maximum);
unsigned long system_milliseconds(void);

/* ---------- constants */

enum
{
	MAXIMUM_MAP_VOTE_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	/* a client forgets the vote it has not heard of again this long */
	STATUS_TIMEOUT_MILLISECONDS = 3000,
	/* a machine's choices the host takes, at most one this often */
	CHOICE_INTERVAL_MILLISECONDS = 250,
};

/* the status's flags */
enum
{
	/* the vote is over: its winner chosen */
	_map_vote_over_bit,
};

/* ---------- structures */

struct distributed_map_vote_choice_entry
{
	char map_name[NETWORK_MAP_VOTE_MAP_SIZE];
	char gametype[NETWORK_MAP_VOTE_GAMETYPE_SIZE];
	byte votes;
	byte pad[3];
};

struct distributed_map_vote_status
{
	byte round;
	byte count;
	byte seconds_left;
	byte flags;
	/* the machine told's choice, NONE_BYTE for none */
	byte chosen;
	/* the vote over: the choice that won */
	byte winner;
	byte pad[2];
	struct distributed_map_vote_choice_entry choices[NETWORK_MAP_VOTE_MAXIMUM_CHOICES];
};

struct distributed_map_vote_choice
{
	byte round;
	byte choice;
	byte pad[2];
};

typedef char distributed_map_vote_choice_entry_size_assert[
	sizeof(struct distributed_map_vote_choice_entry) == 60 ? 1 : -1];
typedef char distributed_map_vote_status_size_assert[sizeof(struct distributed_map_vote_status) == 488 ? 1 : -1];
typedef char distributed_map_vote_choice_size_assert[sizeof(struct distributed_map_vote_choice) == 4 ? 1 : -1];

#define NONE_BYTE 0xFF

/* ---------- globals */

/* the host's vote */
static struct
{
	boolean running;
	byte round;
	short count;
	char map_names[NETWORK_MAP_VOTE_MAXIMUM_CHOICES][NETWORK_MAP_VOTE_MAP_SIZE];
	char gametypes[NETWORK_MAP_VOTE_MAXIMUM_CHOICES][NETWORK_MAP_VOTE_GAMETYPE_SIZE];
	short votes[NETWORK_MAP_VOTE_MAXIMUM_CHOICES];
	unsigned long started_at;
	unsigned long ends_at;
	unsigned long told_at;
	boolean changed;
	/* each machine's choice (NONE_BYTE: none), and when it last chose */
	byte choices[MAXIMUM_MAP_VOTE_MACHINES];
	unsigned long chosen_at[MAXIMUM_MAP_VOTE_MACHINES];
} map_vote;

static unsigned long map_vote_random_state;

/* a client's: the vote as last heard (the time: 0, none) */
static struct distributed_map_vote_status map_vote_heard;
static unsigned long map_vote_heard_at;

/* ---------- private code */

static unsigned long map_vote_random(
	void)
{
	if (!map_vote_random_state)
		map_vote_random_state = system_milliseconds() | 1;
	map_vote_random_state ^= map_vote_random_state << 13;
	map_vote_random_state ^= map_vote_random_state >> 17;
	map_vote_random_state ^= map_vote_random_state << 5;
	return map_vote_random_state;
}

static void map_vote_fill_header(
	struct distributed_message_header *header,
	byte type,
	word size)
{
	header->header = 0;
	build_message_header(&header->header, size, 2, 0);
	header->type = type;
	header->count = 1;
	header->game_time = game_in_progress() ? game_time_get() : 0;
}

/* a text copied into a field, ended within it, printable ASCII */
static void map_vote_copy(
	char *destination,
	char const *source,
	long size)
{
	long index;

	for (index = 0; source[index] && index < size - 1; index++)
	{
		unsigned char character = (unsigned char)source[index];

		destination[index] = character >= 0x20 && character < 0x7F ? (char)character : '?';
	}
	for (; index < size; index++)
		destination[index] = 0;
}

/* the votes counted again, of the machines in the game; how many machines
there are, and how many voted */
static void map_vote_recount(
	short *machine_count,
	short *voted_count)
{
	long machine_indices[MAXIMUM_MAP_VOTE_MACHINES];
	short count = network_distributed_server_machines(machine_indices, MAXIMUM_MAP_VOTE_MACHINES);
	short votes[NETWORK_MAP_VOTE_MAXIMUM_CHOICES];
	boolean present[MAXIMUM_MAP_VOTE_MACHINES];
	short index;

	csmemset(votes, 0, sizeof(votes));
	csmemset(present, 0, sizeof(present));
	*machine_count = 0;
	*voted_count = 0;
	for (index = 0; index < count; index++)
	{
		long machine_index = machine_indices[index];

		if (machine_index < 0 || machine_index >= MAXIMUM_MAP_VOTE_MACHINES)
			continue;
		present[machine_index] = TRUE;
		(*machine_count)++;
		if (map_vote.choices[machine_index] < map_vote.count)
		{
			votes[map_vote.choices[machine_index]]++;
			(*voted_count)++;
		}
	}
	/* (a machine gone takes its vote with it, and its slot's next starts
	with none) */
	for (index = 0; index < MAXIMUM_MAP_VOTE_MACHINES; index++)
	{
		if (!present[index])
			map_vote.choices[index] = NONE_BYTE;
	}
	if (csmemcmp(votes, map_vote.votes, sizeof(votes)))
		map_vote.changed = TRUE;
	csmemcpy(map_vote.votes, votes, sizeof(votes));
}

/* the vote to every client (winner: NONE while it runs) */
static void map_vote_send(
	short winner)
{
	long machine_indices[MAXIMUM_MAP_VOTE_MACHINES];
	short count = network_distributed_server_machines(machine_indices, MAXIMUM_MAP_VOTE_MACHINES);
	unsigned long now = system_milliseconds();
	long left = (long)(map_vote.ends_at - now) > 0 ? (long)(map_vote.ends_at - now + 999) / 1000 : 0;
	short index;

	for (index = 0; index < count; index++)
	{
		struct
		{
			struct distributed_message_header header;
			struct distributed_map_vote_status status;
		} message;
		long machine_index = machine_indices[index];
		short choice;

		if (machine_index < 0 || machine_index >= MAXIMUM_MAP_VOTE_MACHINES)
			continue;
		csmemset(&message, 0, sizeof(message));
		map_vote_fill_header(&message.header, _distributed_message_map_vote, (word)sizeof(message));
		message.status.round = map_vote.round;
		message.status.count = (byte)map_vote.count;
		message.status.seconds_left = (byte)PIN(left, 0, 255);
		message.status.chosen = map_vote.choices[machine_index];
		message.status.winner = winner == NONE ? NONE_BYTE : (byte)winner;
		if (winner != NONE)
			message.status.flags |= FLAG(_map_vote_over_bit);
		for (choice = 0; choice < map_vote.count; choice++)
		{
			csmemcpy(message.status.choices[choice].map_name, map_vote.map_names[choice], NETWORK_MAP_VOTE_MAP_SIZE);
			csmemcpy(message.status.choices[choice].gametype, map_vote.gametypes[choice],
				NETWORK_MAP_VOTE_GAMETYPE_SIZE);
			message.status.choices[choice].votes = (byte)PIN(map_vote.votes[choice], 0, 255);
		}
		network_distributed_server_send_to_machine_reliably(machine_index, &message, (word)sizeof(message));
	}
	map_vote.told_at = now;
	map_vote.changed = FALSE;
}

/* the choice with the most votes; a tie (or none voted) at random among them */
static short map_vote_winner(
	void)
{
	short best = 0;
	short tied = 0;
	short pick;
	short index;

	for (index = 0; index < map_vote.count; index++)
		best = MAX(best, map_vote.votes[index]);
	for (index = 0; index < map_vote.count; index++)
	{
		if (map_vote.votes[index] == best)
			tied++;
	}
	if (!tied)
		return NONE;
	pick = (short)(map_vote_random() % (unsigned long)tied);
	for (index = 0; index < map_vote.count; index++)
	{
		if (map_vote.votes[index] == best && pick-- == 0)
			return index;
	}
	return NONE;
}

/* ---------- the host */

void network_map_vote_begin(
	char const (*map_names)[NETWORK_MAP_VOTE_MAP_SIZE],
	char const (*gametypes)[NETWORK_MAP_VOTE_GAMETYPE_SIZE],
	short count,
	short seconds)
{
	byte round = (byte)(map_vote.round + 1);
	unsigned long now = system_milliseconds();
	short index;

	csmemset(&map_vote, 0, sizeof(map_vote));
	map_vote.round = round;
	map_vote.count = (short)PIN(count, 0, NETWORK_MAP_VOTE_MAXIMUM_CHOICES);
	for (index = 0; index < map_vote.count; index++)
	{
		map_vote_copy(map_vote.map_names[index], map_names[index], NETWORK_MAP_VOTE_MAP_SIZE);
		map_vote_copy(map_vote.gametypes[index], gametypes[index], NETWORK_MAP_VOTE_GAMETYPE_SIZE);
	}
	csmemset(map_vote.choices, NONE_BYTE, sizeof(map_vote.choices));
	map_vote.started_at = now;
	map_vote.ends_at = now + (unsigned long)PIN(seconds, 1, 255) * 1000;
	map_vote.running = map_vote.count > 0;
	if (map_vote.running)
		map_vote_send(NONE);
}

short network_map_vote_host_update(
	void)
{
	unsigned long now = system_milliseconds();
	short machines;
	short voted;
	short winner;

	if (!map_vote.running)
		return NONE;
	map_vote_recount(&machines, &voted);
	if ((long)(now - map_vote.ends_at) < 0 && (machines == 0 || voted < machines))
	{
		if (map_vote.changed || now - map_vote.told_at >= 1000)
			map_vote_send(NONE);
		return NONE;
	}
	winner = map_vote_winner();
	map_vote.running = FALSE;
	if (winner != NONE)
		map_vote_send(winner);
	return winner;
}

boolean network_map_vote_running(
	void)
{
	return map_vote.running;
}

short network_map_vote_votes(
	short choice)
{
	return choice >= 0 && choice < map_vote.count ? map_vote.votes[choice] : 0;
}

void network_map_vote_cancel(
	void)
{
	map_vote.running = FALSE;
}

/* ---------- a client */

boolean network_map_vote_status_get(
	struct network_map_vote_status *status)
{
	short index;

	csmemset(status, 0, sizeof(*status));
	if (global_network_game_server_get())
		return FALSE;
	if (!map_vote_heard_at || system_milliseconds() - map_vote_heard_at > STATUS_TIMEOUT_MILLISECONDS ||
		!map_vote_heard.count || map_vote_heard.count > NETWORK_MAP_VOTE_MAXIMUM_CHOICES)
	{
		return FALSE;
	}
	status->round = map_vote_heard.round;
	status->count = map_vote_heard.count;
	for (index = 0; index < status->count; index++)
	{
		map_vote_copy(status->map_names[index], map_vote_heard.choices[index].map_name, NETWORK_MAP_VOTE_MAP_SIZE);
		map_vote_copy(status->gametypes[index], map_vote_heard.choices[index].gametype, NETWORK_MAP_VOTE_GAMETYPE_SIZE);
		status->votes[index] = map_vote_heard.choices[index].votes;
	}
	status->seconds_left = map_vote_heard.seconds_left;
	status->chosen = map_vote_heard.chosen < status->count ? map_vote_heard.chosen : NONE;
	status->winner = TEST_FLAG(map_vote_heard.flags, _map_vote_over_bit) && map_vote_heard.winner < status->count ?
		map_vote_heard.winner : NONE;
	return TRUE;
}

boolean network_map_vote_choose(
	short choice)
{
	struct
	{
		struct distributed_message_header header;
		struct distributed_map_vote_choice choice;
	} message;
	struct network_map_vote_status status;

	if (!network_map_vote_status_get(&status) || status.winner != NONE || choice < 0 || choice >= status.count)
		return FALSE;
	csmemset(&message, 0, sizeof(message));
	map_vote_fill_header(&message.header, _distributed_message_map_vote_choice, (word)sizeof(message));
	message.choice.round = (byte)status.round;
	message.choice.choice = (byte)choice;
	/* (shown at once: the host's count follows) */
	map_vote_heard.chosen = (byte)choice;
	return network_distributed_client_send_reliably(&message, (word)sizeof(message));
}

void network_map_vote_reset(
	void)
{
	csmemset(&map_vote_heard, 0, sizeof(map_vote_heard));
	map_vote_heard_at = 0;
}

/* ---------- messages */

boolean network_map_vote_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_map_vote || header.type == _distributed_message_map_vote_choice;
}

void network_map_vote_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream)
{
	struct distributed_message_header header;
	byte const *entries = (byte const *)message + sizeof(header);
	long length = (long)size - (long)sizeof(header);

	if (!network_map_vote_handles_message(message, size) || !from_stream)
		return;
	csmemcpy(&header, message, sizeof(header));
	if (header.type == _distributed_message_map_vote_choice)
	{
		struct distributed_map_vote_choice choice;
		unsigned long now = system_milliseconds();

		if (machine_index < 0 || machine_index >= MAXIMUM_MAP_VOTE_MACHINES || !map_vote.running ||
			length != (long)sizeof(choice))
		{
			return;
		}
		csmemcpy(&choice, entries, sizeof(choice));
		if (choice.round != map_vote.round || choice.choice >= map_vote.count ||
			(map_vote.chosen_at[machine_index] && now - map_vote.chosen_at[machine_index] < CHOICE_INTERVAL_MILLISECONDS))
		{
			return;
		}
		map_vote.chosen_at[machine_index] = now ? now : 1;
		if (map_vote.choices[machine_index] != choice.choice)
		{
			map_vote.choices[machine_index] = choice.choice;
			map_vote.changed = TRUE;
		}
		return;
	}
	/* (the host's, to a client) */
	if (machine_index != NONE || global_network_game_server_get() || length != (long)sizeof(map_vote_heard))
	{
		return;
	}
	csmemcpy(&map_vote_heard, entries, sizeof(map_vote_heard));
	map_vote_heard_at = system_milliseconds();
	if (!map_vote_heard_at)
		map_vote_heard_at = 1;
}
