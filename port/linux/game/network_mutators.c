/*
NETWORK_MUTATORS.C

The host's mutators (mutators.c) sent to each client as it is ready for the
host's game (its client_ready, before the host's objects and the game's
state, on the same reliable connection, so that the client plays them from
the first of the host's objects it makes): each key's text and its value,
which the client plays instead of its own. A client takes them only from
the host, only reliably, and only whole: a count the size does not match, a
key that is not a key's text or a value out of the range of a key it has
leaves the message unplayed.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_mutators.h"
#include "mutators.h"

#include <string.h>

/* ---------- structures */

struct distributed_mutator_entry
{
	char key[MUTATOR_KEY_LENGTH];
	real value;
};

/* (as many entries as the header's count) */
struct distributed_mutators_message
{
	struct distributed_message_header header;
	struct distributed_mutator_entry entries[MAXIMUM_MUTATOR_KEYS];
};

typedef char mutator_entry_size_assert[sizeof(struct distributed_mutator_entry) == MUTATOR_KEY_LENGTH + 4 ? 1 : -1];
typedef char mutators_message_entries_assert[MAXIMUM_MUTATOR_KEYS <= RELIABLE_ENTRIES(struct distributed_mutator_entry) ? 1 : -1];

/* ---------- public code */

void network_mutators_send(
	long machine_index)
{
	struct distributed_mutators_message message;
	char keys[MAXIMUM_MUTATOR_KEYS][MUTATOR_KEY_LENGTH];
	real values[MAXIMUM_MUTATOR_KEYS];
	short count;
	short index;

	if (game_connection() != _game_connection_network_server)
		return;
	count = mutators_keys(keys, values, MAXIMUM_MUTATOR_KEYS);
	/* (none: the client plays none already) */
	if (!count)
		return;
	memset(&message, 0, sizeof(message));
	for (index = 0; index < count; index++)
	{
		memcpy(message.entries[index].key, keys[index], MUTATOR_KEY_LENGTH);
		message.entries[index].value = values[index];
	}
	distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_mutators, count,
		(word)(sizeof(message.header) + count * sizeof(struct distributed_mutator_entry)));
}

boolean network_mutators_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	memcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_mutators;
}

void network_mutators_handle_message(
	long machine_index,
	word const *data,
	word size)
{
	struct distributed_mutators_message message;
	char keys[MAXIMUM_MUTATOR_KEYS][MUTATOR_KEY_LENGTH];
	real values[MAXIMUM_MUTATOR_KEYS];
	short index;

	if (machine_index != NONE || game_connection() != _game_connection_network_client ||
		!game_in_progress() || main_menu_is_active() || network_connection_last_read_was_unreliable() ||
		size < sizeof(message.header) || size > sizeof(message))
	{
		return;
	}
	memset(&message, 0, sizeof(message));
	memcpy(&message, data, size);
	if (GET_MESSAGE_TYPE(message.header.header) != 2 || message.header.type != _distributed_message_mutators ||
		message.header.count < 1 || message.header.count > MAXIMUM_MUTATOR_KEYS ||
		size != sizeof(message.header) + message.header.count * sizeof(struct distributed_mutator_entry))
	{
		error(_error_log, "mutators: the host's message is not one of mutators: passed over");
		return;
	}
	for (index = 0; index < message.header.count; index++)
	{
		memcpy(keys[index], message.entries[index].key, MUTATOR_KEY_LENGTH);
		values[index] = message.entries[index].value;
	}
	if (!mutators_set_host(keys, values, message.header.count))
		error(_error_log, "mutators: the host's keys or values are not mutators': passed over");
}
