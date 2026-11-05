/*
MUTATOR_SHARE.C

The host's mutators' files sent to its clients (docs/mutators.md, "Network games"):
each mutator.toml, Lua script, .cevehicle and .ceweapon of the mutators it plays
that have an add-on or a script. An effect's and a tag edit's values go as they did
(network_mutators.c), as the game starts.

A client asks as it joins, and again for each game. The host lists them, by
their SHA-256s (message _mutator_share_entry), in the order it plays them. The
client plays those, in that order, in place of its own add-ons: those it has by their
SHA-256 (its own folders, or downloads from before), and those it lacks it fetches
from the host over a connection of map sharing's (map_share.c: the game's own
connection carries 90 KB/s of them at best, and cannot do that and stay up), at
network.map_share_upload at most. Its game does not load until it is done
(map_share.c's map_share_downloading, map_share_machine_needs_map).

The host is not trusted: each file's name, kind and size are checked against the
list's limits before it is asked for, its bytes against its SHA-256, and the files
together against the mutator's, and the mutator is then read as any mutator is. What
a host sends goes in a folder of the data root's of its own (mutator_downloads),
never the mutators folder, and is played only in that host's game.

A script runs code on this machine, so a player is asked before the first time one
is fetched (network.mutator_host_scripts: "ask"): A or Enter for yes, B or Esc for
no, and no after PROMPT_TIMEOUT. Asked once for a mutator, by its SHA-256: a yes is
kept beside its files. A script the player has already, in their own folders, is
not asked about. One that is refused is not fetched, if its mutator is only that; a
mutator with an add-on too needs it to be checked, so it is fetched, and not run.
*/

#include "cseries.h"
#include "cseries/errors.h"
#include "game/game.h"
#include "main/console.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"
#include "halo_keyboard.h"
#include "mutator_share.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* (the C library's allocator: port_mutators.c's frees and fills what it holds) */
#undef malloc
#undef free

/* ---------- constants */

enum
{
	MAXIMUM_SHARED_MUTATORS = 8,
	MAXIMUM_SHARED_FILES = 3,
	SHARED_NAME_BYTES = 64,
	SHARED_ID_BYTES = 64,
	SHARED_DIGEST_BYTES = 32,
	/* milliseconds: an answer's wait, a client's word that it is fetching, how often it
	says so, and for how long the host takes its word */
	ANSWER_TIMEOUT = 30000,
	FETCHING_INTERVAL = 5000,
	FETCHING_TIMEOUT = 20000,
	PROMPT_TIMEOUT = 30000,
	PROMPT_READ_TIME = 1500,
};

enum
{
	_share_none,
	_share_asking,
	_share_prompting,
	_share_fetching,
	_share_done,
};

/* ---------- structures */

/* (port_mutators.c's) */
struct port_mutator_share_file
{
	char name[SHARED_NAME_BYTES];
	unsigned long size;
	byte sha256[SHARED_DIGEST_BYTES];
};

struct port_mutator_share
{
	char id[SHARED_ID_BYTES];
	byte digest[SHARED_DIGEST_BYTES];
	long file_count;
	struct port_mutator_share_file files[MAXIMUM_SHARED_FILES];
};

struct mutator_share_entry_message
{
	struct distributed_message_header header;
	long total;
	long index;
	/* whether the host's game is under way: a client joining it must wait */
	long in_progress;
	struct port_mutator_share entry;
};

/* ---------- globals */

static struct
{
	short state;
	long games_played;
	unsigned long heard_at;
	long total;
	long received;
	boolean in_progress;
	struct port_mutator_share entries[MAXIMUM_SHARED_MUTATORS];
	/* whether each mutator's script may run, and the one being asked about, when it
	was first asked, and the buttons held then, which are no answer */
	boolean script_allowed[MAXIMUM_SHARED_MUTATORS];
	long prompt;
	unsigned long prompt_at;
	unsigned long prompt_held;
	/* the file being fetched: which, its bytes, what has come of them, whether the
	host has been asked for the rest, and when it was last said to be */
	long mutator;
	long file;
	byte *bytes;
	unsigned long arrived;
	boolean asked;
	unsigned long said_at;
	/* (the host) when each machine last said it was fetching, 0 once it is done */
	unsigned long machine_asked_at[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
} mutator_share;

/* ---------- prototypes */

int config_boolean(const char *name);
const char *config_string(const char *name);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
unsigned long system_milliseconds(void);

long port_mutators_share_count(void);
int port_mutators_share_entry(long index, struct port_mutator_share *entry);
int port_mutators_share_valid(struct port_mutator_share const *entry);
int port_mutators_share_has(struct port_mutator_share const *entry);
int port_mutators_share_file_has(struct port_mutator_share const *entry, long file);
boolean map_share_mutator_connected(void);
boolean map_share_host_in_game(void);
boolean map_share_mutator_request(long mutator, long file, unsigned long offset);
int port_mutators_share_store(struct port_mutator_share const *entry, long file, void const *bytes, unsigned long size);
void port_mutators_host_begin(void);
int port_mutators_host_add(struct port_mutator_share const *entry, int script_allowed);
int port_mutators_share_has_script(struct port_mutator_share const *entry);
int port_mutators_share_script_known(struct port_mutator_share const *entry);
void port_mutators_share_script_allow(struct port_mutator_share const *entry);
void port_mutators_host_clear(void);

/* ---------- private code */

/* a client's network game, or NULL on a host and outside one */
static struct network_game *share_client_game(
	void)
{
	if (!global_network_game_client_get() || global_network_game_server_get())
		return NULL;
	return network_game_get_game();
}

static void share_fill_header(
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

static void client_send(
	byte type)
{
	struct distributed_message_header header;

	share_fill_header(&header, type, (word)sizeof(header));
	network_distributed_client_send_reliably(&header, (word)sizeof(header));
}

/* the mutators listed played, as far as this machine has them */
static void client_finish(
	void)
{
	long index;
	long played = 0;

	port_mutators_host_begin();
	for (index = 0; index < mutator_share.received && index < MAXIMUM_SHARED_MUTATORS; index++)
	{
		if (port_mutators_host_add(&mutator_share.entries[index], mutator_share.script_allowed[index]))
		{
			played++;
		}
		else
		{
			error(_error_log, "mutators: the host's %s is not played here: this game cannot play as the host's does",
				mutator_share.entries[index].id);
			console_warning("The host's mutator %s is not played here", mutator_share.entries[index].id);
		}
	}
	if (mutator_share.bytes)
	{
		free(mutator_share.bytes);
		mutator_share.bytes = NULL;
	}
	if (mutator_share.total)
		error(_error_silent, "mutators: %ld of the host's %ld mutators with files played", played, mutator_share.total);
	mutator_share.state = _share_done;
	client_send(_distributed_message_mutator_share_done);
}

/* whether a mutator has only a script to it, beside its mutator.toml */
static boolean entry_script_only(
	struct port_mutator_share const *entry)
{
	return entry->file_count == 2 && port_mutators_share_has_script(entry);
}

/* the next script to ask about, each decided by the setting or what is known: FALSE
(asking) until none is left */
static boolean client_scripts_decided(
	void)
{
	char const *mode = config_string("network.mutator_host_scripts");

	for (; mutator_share.prompt < mutator_share.received; mutator_share.prompt++)
	{
		struct port_mutator_share const *entry = &mutator_share.entries[mutator_share.prompt];

		if (!port_mutators_share_has_script(entry) || port_mutators_share_script_known(entry) ||
			(mode && !strcmp(mode, "always")))
		{
			mutator_share.script_allowed[mutator_share.prompt] = TRUE;
		}
		else if (!(mode && !strcmp(mode, "never")) && port_mutators_share_valid(entry))
		{
			mutator_share.state = _share_prompting;
			mutator_share.prompt_at = system_milliseconds();
			mutator_share.prompt_held = halo_chat_pad_buttons();
			/* (no tap from before it was asked) */
			{
				float x, y;

				halo_chat_taps(&x, &y);
			}
			return FALSE;
		}
	}
	return TRUE;
}

/* the next file this machine lacks begun, or finished if none is left */
static void client_next_file(
	void)
{
	if (!client_scripts_decided())
		return;
	for (; mutator_share.mutator < mutator_share.received; mutator_share.mutator++, mutator_share.file = 0)
	{
		struct port_mutator_share const *entry = &mutator_share.entries[mutator_share.mutator];

		if (port_mutators_share_has(entry) || !port_mutators_share_valid(entry))
			continue;
		/* (a script refused, which is all there is to fetch) */
		if (!mutator_share.script_allowed[mutator_share.mutator] && entry_script_only(entry))
			continue;
		for (; mutator_share.file < entry->file_count; mutator_share.file++)
		{
			if (port_mutators_share_file_has(entry, mutator_share.file))
				continue;
			mutator_share.bytes = malloc(entry->files[mutator_share.file].size + 1);
			if (!mutator_share.bytes)
			{
				error(_error_log, "mutators: no memory for the host's %s", entry->files[mutator_share.file].name);
				continue;
			}
			mutator_share.arrived = 0;
			mutator_share.asked = FALSE;
			mutator_share.state = _share_fetching;
			mutator_share.heard_at = system_milliseconds();
			error(_error_silent, "mutators: fetching %s of the host's %s (%lu bytes)", entry->files[mutator_share.file].name,
				entry->id, entry->files[mutator_share.file].size);
			return;
		}
	}
	client_finish();
}

static void client_reset(
	void)
{
	if (mutator_share.bytes)
		free(mutator_share.bytes);
	mutator_share.bytes = NULL;
	mutator_share.state = _share_none;
	mutator_share.total = 0;
	mutator_share.received = 0;
	mutator_share.mutator = 0;
	mutator_share.file = 0;
	mutator_share.prompt = 0;
	csmemset(mutator_share.script_allowed, 0, sizeof(mutator_share.script_allowed));
}

/* the player's answer to the question asked, or none yet */
static void client_prompt_update(
	void)
{
	struct port_mutator_share const *entry = &mutator_share.entries[mutator_share.prompt];
	unsigned long buttons = halo_chat_pad_buttons();
	/* (a press in its first moments is no answer: a player going through the menus with
	A has not read it) */
	boolean reading = system_milliseconds() - mutator_share.prompt_at < PROMPT_READ_TIME;
	unsigned long pressed = reading ? 0 : buttons & ~mutator_share.prompt_held;
	/* (and on a touchscreen, without a controller, a tap: on the left half yes, on
	the right no; one while it is read is no answer either) */
	float tap_x, tap_y;
	boolean tapped = halo_chat_taps(&tap_x, &tap_y) > 0 && !reading;
	boolean yes = (pressed & (1UL << HALO_CHAT_PAD_A)) != 0 || (tapped && tap_x < 0.5f);
	boolean no = (pressed & (1UL << HALO_CHAT_PAD_B)) != 0 || (tapped && tap_x >= 0.5f) ||
		system_milliseconds() - mutator_share.prompt_at > PROMPT_TIMEOUT;

	if (system_milliseconds() - mutator_share.prompt_at < PROMPT_READ_TIME)
		mutator_share.prompt_held = buttons;
	else
		mutator_share.prompt_held &= buttons;
	mutator_share.heard_at = system_milliseconds();
	if (!yes && !no)
		return;
	error(_error_log, "mutators: the player %s the script of the host's %s", yes ? "allowed" : "refused", entry->id);
	mutator_share.script_allowed[mutator_share.prompt] = yes;
	if (yes)
		port_mutators_share_script_allow(entry);
	mutator_share.prompt++;
	mutator_share.state = _share_asking;
	client_next_file();
}

/* the client's state, each game's asked for anew */
static void client_update(
	void)
{
	struct network_game *game = share_client_game();

	if (!game || !game->map.name[0])
	{
		if (!game && mutator_share.state != _share_none)
		{
			client_reset();
			port_mutators_host_clear();
		}
		return;
	}
	if (mutator_share.state == _share_none || mutator_share.games_played != game->number_of_games_played)
	{
		client_reset();
		if (!config_boolean("network.mutator_download"))
		{
			mutator_share.state = _share_done;
			mutator_share.games_played = game->number_of_games_played;
			return;
		}
		mutator_share.games_played = game->number_of_games_played;
		mutator_share.state = _share_asking;
		mutator_share.heard_at = system_milliseconds();
		client_send(_distributed_message_mutator_share_ask);
		return;
	}
	if (mutator_share.state == _share_prompting || mutator_share.state == _share_fetching)
	{
		/* (the host holds the game for this machine while it says so) */
		if (system_milliseconds() - mutator_share.said_at > FETCHING_INTERVAL)
		{
			client_send(_distributed_message_mutator_share_fetching);
			mutator_share.said_at = system_milliseconds();
		}
	}
	if (mutator_share.state == _share_prompting)
		client_prompt_update();
	if (mutator_share.state == _share_fetching)
	{
		/* (the rest of the file asked for, the connection lost or not: from where it is) */
		if (!map_share_mutator_connected())
			mutator_share.asked = FALSE;
		if (!mutator_share.asked)
			mutator_share.asked = map_share_mutator_request(mutator_share.mutator, mutator_share.file, mutator_share.arrived);
	}
	if ((mutator_share.state == _share_asking || mutator_share.state == _share_fetching) &&
		system_milliseconds() - mutator_share.heard_at > ANSWER_TIMEOUT)
	{
		error(_error_log, "mutators: the host stopped answering for its mutators' files");
		mutator_share.received = MIN(mutator_share.received, mutator_share.total);
		client_finish();
	}
}

static void host_answer(
	long machine_index)
{
	struct mutator_share_entry_message message;
	long count = port_mutators_share_count();
	long index;

	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return;
	if (count > MAXIMUM_SHARED_MUTATORS)
		count = MAXIMUM_SHARED_MUTATORS;
	csmemset(&message, 0, sizeof(message));
	share_fill_header(&message.header, _distributed_message_mutator_share_entry, (word)sizeof(message));
	message.total = count;
	message.index = NONE;
	message.in_progress = map_share_host_in_game();
	/* (none, or each: a client knows at once how many to wait for) */
	if (!count)
		network_distributed_server_send_to_machine_reliably(machine_index, &message, (word)sizeof(message));
	for (index = 0; index < count; index++)
	{
		message.index = index;
		if (port_mutators_share_entry(index, &message.entry))
			network_distributed_server_send_to_machine_reliably(machine_index, &message, (word)sizeof(message));
	}
}

/* ---------- public code */

boolean mutator_share_busy(
	void)
{
	struct network_game *game = share_client_game();

	if (!game || !game->map.name[0])
		return FALSE;
	/* (a game this client has not asked about: asked now) */
	if (mutator_share.state == _share_none || mutator_share.games_played != game->number_of_games_played)
		client_update();
	return mutator_share.state == _share_asking || mutator_share.state == _share_prompting ||
		mutator_share.state == _share_fetching;
}

boolean mutator_share_holds_players(
	void)
{
	/* (as a map's download: a lobby's machine with no players for long is refused, and
	its game does not load before it is done anyway. A game under way would start it
	at once) */
	return mutator_share_busy() && (mutator_share.state == _share_asking || mutator_share.in_progress);
}

boolean mutator_share_prompt_open(
	void)
{
	return mutator_share.state == _share_prompting;
}

void mutator_share_prompt_text(
	char *question,
	char *answers,
	unsigned long size)
{
	unsigned long waited = system_milliseconds() - mutator_share.prompt_at;

	snprintf(question, size, "The host's mutator %s runs a Lua script.", mutator_share.entries[mutator_share.prompt].id);
#if defined(HALO_ANDROID) || defined(HALO_WEB)
	snprintf(answers, size, "Download and run it?  Tap left or A: yes   Tap right or B: no (%lu)",
		waited < PROMPT_TIMEOUT ? (PROMPT_TIMEOUT - waited) / 1000 : 0UL);
#else
	snprintf(answers, size, "Download and run it?  A or Enter: yes   B or Esc: no (%lu)", waited < PROMPT_TIMEOUT ? (PROMPT_TIMEOUT - waited) / 1000 : 0UL);
#endif
}

void mutator_share_block(
	long mutator,
	long file,
	unsigned long offset,
	byte const *bytes,
	long size)
{
	struct port_mutator_share const *entry;
	unsigned long file_size;

	if (mutator_share.state != _share_fetching || mutator != mutator_share.mutator || file != mutator_share.file ||
		offset != mutator_share.arrived)
	{
		return;
	}
	entry = &mutator_share.entries[mutator_share.mutator];
	file_size = entry->files[mutator_share.file].size;
	mutator_share.heard_at = system_milliseconds();
	if (size <= 0 || (unsigned long)size > file_size - mutator_share.arrived)
	{
		error(_error_log, "mutators: the host's block of %s is not of it", entry->id);
		mutator_share.arrived = file_size;
	}
	else
	{
		csmemcpy(mutator_share.bytes + mutator_share.arrived, bytes, size);
		mutator_share.arrived += size;
	}
	if (mutator_share.arrived < file_size)
		return;
	/* (kept only if it is what the host's list said: port_mutators.c's check) */
	if (!port_mutators_share_store(entry, mutator_share.file, mutator_share.bytes, file_size))
		error(_error_log, "mutators: %s of the host's %s is not what its list said", entry->files[mutator_share.file].name,
			entry->id);
	else
		error(_error_silent, "mutators: fetched %s of the host's %s", entry->files[mutator_share.file].name, entry->id);
	free(mutator_share.bytes);
	mutator_share.bytes = NULL;
	mutator_share.file++;
	client_next_file();
}

boolean mutator_share_machine_fetching(
	long machine_index)
{
	unsigned long asked_at;

	if (machine_index < 0 || machine_index >= HALO_PORT_MAXIMUM_NETWORK_MACHINES)
		return FALSE;
	asked_at = mutator_share.machine_asked_at[machine_index];
	return asked_at && system_milliseconds() - asked_at < FETCHING_TIMEOUT;
}

void mutator_share_update(
	void)
{
	client_update();
}

boolean mutator_share_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type >= _distributed_message_mutator_share_ask && header.type <= _distributed_message_mutator_share_done;
}

void mutator_share_handle_message(
	long machine_index,
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (!mutator_share_handles_message(message, size))
		return;
	csmemcpy(&header, message, sizeof(header));
	/* (a request only from a client, of a host; an answer only from the host) */
	if (header.type == _distributed_message_mutator_share_ask)
	{
		if (machine_index != NONE && game_connection() == _game_connection_network_server)
			host_answer(machine_index);
	}
	else if (header.type == _distributed_message_mutator_share_fetching)
	{
		if (machine_index >= 0 && machine_index < HALO_PORT_MAXIMUM_NETWORK_MACHINES &&
			game_connection() == _game_connection_network_server)
		{
			mutator_share.machine_asked_at[machine_index] = system_milliseconds() | 1;
		}
	}
	else if (header.type == _distributed_message_mutator_share_done)
	{
		if (machine_index >= 0 && machine_index < HALO_PORT_MAXIMUM_NETWORK_MACHINES)
			mutator_share.machine_asked_at[machine_index] = 0;
	}
	else if (header.type == _distributed_message_mutator_share_entry)
	{
		static struct mutator_share_entry_message entry;

		if (machine_index != NONE || mutator_share.state != _share_asking || size != sizeof(entry))
			return;
		csmemcpy(&entry, message, sizeof(entry));
		if (entry.total < 0 || entry.total > MAXIMUM_SHARED_MUTATORS)
			return;
		mutator_share.total = entry.total;
		mutator_share.in_progress = entry.in_progress != 0;
		mutator_share.heard_at = system_milliseconds();
		if (entry.total && entry.index == mutator_share.received && entry.index < MAXIMUM_SHARED_MUTATORS)
		{
			entry.entry.id[SHARED_ID_BYTES - 1] = 0;
			/* (shown to the player: as text, whatever the host's held) */
			{
				short character;

				for (character = 0; entry.entry.id[character]; character++)
				{
					if (entry.entry.id[character] < 0x20 || entry.entry.id[character] > 0x7E)
						entry.entry.id[character] = '?';
				}
			}
			mutator_share.entries[mutator_share.received++] = entry.entry;
		}
		if (mutator_share.received == mutator_share.total)
			client_next_file();
	}
}
