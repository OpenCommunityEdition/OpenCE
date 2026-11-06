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

Then the host's tag edits (mutator_tags.c), if it plays any: their bytes in
pieces, each a message of its own with the edits' size and where the piece
goes, in order on the same connection. A client takes a piece only from the
host, only reliably, only where the last ended, and plays the edits when it
has them all; edits that are not this build's leave none played.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_mutators.h"
#include "mutators.h"

#include <stddef.h>
#include <string.h>

/* ---------- constants */

/* the bytes of tag edits in a message */
#define MUTATOR_TAG_EDIT_PIECE_BYTES 4000

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

/* a piece of the host's tag edits: their size, where it goes, its bytes */
struct distributed_mutator_tag_edits_message
{
	struct distributed_message_header header;
	word size;
	word offset;
	byte bytes[MUTATOR_TAG_EDIT_PIECE_BYTES];
};

typedef char mutator_entry_size_assert[sizeof(struct distributed_mutator_entry) == MUTATOR_KEY_LENGTH + 4 ? 1 : -1];
typedef char mutators_message_entries_assert[MAXIMUM_MUTATOR_KEYS <= RELIABLE_ENTRIES(struct distributed_mutator_entry) ? 1 : -1];
typedef char mutator_tag_edits_message_size_assert[
	sizeof(struct distributed_mutator_tag_edits_message) <= MAXIMUM_MESSAGE_SIZE ? 1 : -1];
typedef char mutator_tag_edit_size_assert[MAXIMUM_MUTATOR_TAG_EDIT_BYTES <= 0xFFFF ? 1 : -1];

/* ---------- globals */

/* a client's: the host's tag edits as their pieces come */
static struct
{
	byte bytes[MAXIMUM_MUTATOR_TAG_EDIT_BYTES];
	long size;
	long received;
} network_mutator_tag_edits;

/* ---------- private code */

static void network_mutators_send_tag_edits(
	long machine_index)
{
	struct distributed_mutator_tag_edits_message message;
	byte const *edits;
	long size = mutators_tag_edits(&edits);
	long offset;

	for (offset = 0; offset < size; offset += MUTATOR_TAG_EDIT_PIECE_BYTES)
	{
		long piece = MIN(size - offset, MUTATOR_TAG_EDIT_PIECE_BYTES);

		memset(&message, 0, sizeof(message));
		message.size = (word)size;
		message.offset = (word)offset;
		memcpy(message.bytes, edits + offset, piece);
		distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_mutator_tag_edits, 1,
			(word)(offsetof(struct distributed_mutator_tag_edits_message, bytes) + piece));
	}
}

static void network_mutators_handle_tag_edits(
	word const *data,
	word size)
{
	struct distributed_mutator_tag_edits_message message;
	long piece;

	if (size <= offsetof(struct distributed_mutator_tag_edits_message, bytes) || size > sizeof(message))
		return;
	memset(&message, 0, sizeof(message));
	memcpy(&message, data, size);
	piece = size - (long)offsetof(struct distributed_mutator_tag_edits_message, bytes);
	if (GET_MESSAGE_TYPE(message.header.header) != 2 || message.header.count != 1 ||
		!message.size || message.size > MAXIMUM_MUTATOR_TAG_EDIT_BYTES || message.offset + piece > message.size ||
		(message.offset && (message.offset != network_mutator_tag_edits.received ||
			message.size != network_mutator_tag_edits.size)))
	{
		error(_error_log, "mutators: a piece of the host's tag edits is not the next: passed over");
		network_mutator_tag_edits.received = 0;
		network_mutator_tag_edits.size = 0;
		return;
	}
	if (!message.offset)
	{
		network_mutator_tag_edits.size = message.size;
		network_mutator_tag_edits.received = 0;
	}
	memcpy(network_mutator_tag_edits.bytes + message.offset, message.bytes, piece);
	network_mutator_tag_edits.received = message.offset + piece;
	if (network_mutator_tag_edits.received == network_mutator_tag_edits.size)
	{
		if (!mutators_set_host_tag_edits(network_mutator_tag_edits.bytes, network_mutator_tag_edits.size))
			error(_error_log, "mutators: the host's tag edits are not this build's: none played");
		network_mutator_tag_edits.received = 0;
		network_mutator_tag_edits.size = 0;
	}
}

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
	network_mutators_send_tag_edits(machine_index);
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
	return header.type == _distributed_message_mutators || header.type == _distributed_message_mutator_tag_edits;
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
		size < sizeof(message.header))
	{
		return;
	}
	memcpy(&message.header, data, sizeof(message.header));
	if (message.header.type == _distributed_message_mutator_tag_edits)
	{
		network_mutators_handle_tag_edits(data, size);
		return;
	}
	if (size > sizeof(message))
		return;
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
