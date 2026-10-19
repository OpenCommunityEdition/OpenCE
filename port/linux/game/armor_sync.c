/*
ARMOR_SYNC.C

Each machine's Armor choices in network games, so that the others draw its
players' Spartans so: the armor's color, its lights', its flashlight's
(spartan_colors.c), its shield's (shield_color.c) and its visor's
(visor_color.c): display.spartan_armor_color, display.spartan_lights_color,
display.flashlight_color, display.shield_color and display.visor_color, as
an index each of the armor colors (blackout after them for the armor), or
none for the game's own.

Each machine sends its choices to the host, reliably, whenever they are not
the ones the host has for it (every three seconds at most); the host keeps
every machine's and sends the whole table to every machine, reliably, when it
changes (half a second apart at the soonest), to a machine as soon as it is
in the game and has not had it (one that joined, or finished loading the
map), and every ten seconds; in the lobby and in the game. So a choice
changed in Extra Effects' Spartan Armor reaches the others within seconds. A
machine that leaves has its row cleared, so another taking its slot does not
show its choices. Both kinds are new messages, so the network version is
raised for them (33, HALO_PORT_NETWORK_VERSION): a build without them does
not play with one that has them. The host takes a machine's choices only from that
machine's own stream, and only indices in range or none; a client checks
the host's table's as it reads them.
*/

#include "cseries.h"
#include "game/players.h"
#include "game/game.h"
#include "bungie_net/common/message_header.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"
#include "armor_sync.h"

#include <string.h>

/* port/linux/src/port_config.c, network_distributed.c,
network_server_message_handler.c, cseries_windows.c */
const char *config_string(const char *name);
unsigned long config_changes(void);
boolean network_distributed_client_send_reliably(void *message, word size);
boolean network_distributed_server_send_to_machine_reliably(long machine_index, void *message, word size);
short network_distributed_server_machines(long *machine_indices, short maximum);
unsigned long system_milliseconds(void);

/* the armor colors, in the profile's order (player_profile.c), blackout
after them (the armor's only) */
static char const *const armor_sync_names[] =
{
	"white", "black", "red", "blue", "gray", "yellow", "green", "pink", "purple",
	"cyan", "cobalt", "orange", "teal", "sage", "brown", "tan", "maroon", "salmon",
	"blackout",
};
enum
{
	ARMOR_SYNC_BLACKOUT = 18,
	ARMOR_SYNC_MACHINES = HALO_PORT_MAXIMUM_NETWORK_MACHINES,
	ARMOR_SYNC_NONE = 0xFF,
	/* how often a machine says its choices while the host has others, the
	host sends its table unchanged, sends it again changed at the soonest,
	and looks for machines that have not had it (ms) */
	ARMOR_SYNC_SAY_MS = 3000,
	ARMOR_SYNC_TABLE_MS = 10000,
	ARMOR_SYNC_TABLE_SOONEST_MS = 500,
	ARMOR_SYNC_NEWCOMERS_MS = 250,
	/* (the host) a machine's choices changed this often at most: one that
	says them faster has the rest dropped (its next say, three seconds on,
	is taken) */
	ARMOR_SYNC_HEARD_MS = 1000,
};

/* a machine's choices, to the host */
struct distributed_armor
{
	byte choices[NUMBER_OF_ARMOR_SYNC_CHOICES]; /* an index each, or ARMOR_SYNC_NONE */
	byte pad[3];
};

/* every machine's, from the host: the rows up to the last machine in the
game's, the message as long as they are (those after, none) */
struct distributed_armors
{
	byte choices[ARMOR_SYNC_MACHINES][NUMBER_OF_ARMOR_SYNC_CHOICES];
};

struct distributed_armor_message
{
	struct distributed_message_header header;
	struct distributed_armor entry;
};

struct distributed_armors_message
{
	struct distributed_message_header header;
	struct distributed_armors entry;
};

static char const *const armor_sync_settings[NUMBER_OF_ARMOR_SYNC_CHOICES] =
{
	"display.spartan_armor_color", "display.spartan_lights_color", "display.flashlight_color",
	"display.shield_color", "display.visor_color",
};

/* each machine's choices as the host has them (heard: a client has had the
host's table) */
static byte armor_sync_table[ARMOR_SYNC_MACHINES][NUMBER_OF_ARMOR_SYNC_CHOICES];
static boolean armor_sync_table_heard = FALSE;
static boolean armor_sync_table_changed = FALSE;
static unsigned long armor_sync_table_sent_at = 0;
static unsigned long armor_sync_newcomers_at = 0;
static unsigned long armor_sync_said_at = 0;
static boolean armor_sync_in_game = FALSE;
/* (the host) the machines that have had the table as it is now */
static boolean armor_sync_sent_to[ARMOR_SYNC_MACHINES];
/* (the host) when each machine's choices last changed */
static unsigned long armor_sync_heard_at[ARMOR_SYNC_MACHINES];
/* (the host) the rows the table is sent with: the last machine's in the
game, and those before it */
static short armor_sync_rows = 1;
static unsigned long armor_sync_changes = (unsigned long)-1;
static byte armor_sync_own[NUMBER_OF_ARMOR_SYNC_CHOICES];

/* the largest index a choice may be (the armor's may be blackout) */
static byte armor_sync_limit(
	short choice)
{
	return (byte)(choice == _armor_sync_armor ? ARMOR_SYNC_BLACKOUT : ARMOR_SYNC_BLACKOUT - 1);
}

static void armor_sync_own_update(
	void)
{
	short choice;

	if (armor_sync_changes == config_changes())
		return;
	armor_sync_changes = config_changes();
	for (choice = 0; choice < NUMBER_OF_ARMOR_SYNC_CHOICES; choice++)
	{
		char const *value = config_string(armor_sync_settings[choice]);
		short index;

		armor_sync_own[choice] = ARMOR_SYNC_NONE;
		for (index = 0; value && index <= armor_sync_limit(choice); index++)
		{
			if (!strcmp(value, armor_sync_names[index]))
				armor_sync_own[choice] = (byte)index;
		}
	}
}

static struct network_game *armor_sync_network_game(
	void)
{
	struct network_game_client *client = global_network_game_client_get();

	return client ? network_game_client_get_game(client) : NULL;
}

static boolean armor_sync_host(
	void)
{
	return global_network_game_server_get() != NULL;
}

static void armor_sync_fill_header(
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

static void armor_sync_clear(
	void)
{
	csmemset(armor_sync_table, ARMOR_SYNC_NONE, sizeof(armor_sync_table));
	csmemset(armor_sync_sent_to, 0, sizeof(armor_sync_sent_to));
	csmemset(armor_sync_heard_at, 0, sizeof(armor_sync_heard_at));
	armor_sync_table_heard = FALSE;
}

/* (the host) the rows of the machines no longer in the game cleared */
static void armor_sync_forget_departed(
	struct network_game const *game)
{
	long machine_index;

	armor_sync_rows = 1;
	for (machine_index = 0; machine_index < ARMOR_SYNC_MACHINES; machine_index++)
	{
		short choice;

		if (game->machines[machine_index].machine_index == machine_index)
		{
			armor_sync_rows = (short)(machine_index + 1);
			continue;
		}
		armor_sync_sent_to[machine_index] = FALSE;
		for (choice = 0; choice < NUMBER_OF_ARMOR_SYNC_CHOICES; choice++)
		{
			if (armor_sync_table[machine_index][choice] != ARMOR_SYNC_NONE)
			{
				csmemset(armor_sync_table[machine_index], ARMOR_SYNC_NONE, sizeof(armor_sync_table[machine_index]));
				armor_sync_table_changed = TRUE;
				break;
			}
		}
	}
}

/* (the host) the table to the machines in the game: every one, or only
those that have not had it as it is (newcomers) */
static void armor_sync_send_table(
	long local_machine,
	boolean newcomers)
{
	long machines[ARMOR_SYNC_MACHINES];
	short count;
	short index;
	struct distributed_armors_message message;
	word size = (word)(sizeof(message.header) + armor_sync_rows * sizeof(message.entry.choices[0]));
	boolean prepared = FALSE;

	/* (a machine not in the list now, loading the map, has it when it is) */
	if (!newcomers)
		csmemset(armor_sync_sent_to, 0, sizeof(armor_sync_sent_to));
	count = network_distributed_server_machines(machines, ARMOR_SYNC_MACHINES);
	for (index = 0; index < count; index++)
	{
		long machine_index = machines[index];

		if (machine_index == local_machine || machine_index < 0 || machine_index >= ARMOR_SYNC_MACHINES ||
			armor_sync_sent_to[machine_index])
		{
			continue;
		}
		if (!prepared)
		{
			armor_sync_fill_header(&message.header, _distributed_message_armors, size);
			csmemcpy(message.entry.choices, armor_sync_table, armor_sync_rows * sizeof(message.entry.choices[0]));
			prepared = TRUE;
		}
		armor_sync_sent_to[machine_index] =
			network_distributed_server_send_to_machine_reliably(machine_index, &message, size);
	}
}

/* ---------- public code */

boolean armor_sync_network_active(
	void)
{
	return armor_sync_network_game() != NULL;
}

boolean armor_sync_of_machine(
	long machine_index,
	short choices[NUMBER_OF_ARMOR_SYNC_CHOICES])
{
	short choice;

	if (!armor_sync_table_heard || machine_index < 0 || machine_index >= ARMOR_SYNC_MACHINES)
		return FALSE;
	for (choice = 0; choice < NUMBER_OF_ARMOR_SYNC_CHOICES; choice++)
	{
		byte value = armor_sync_table[machine_index][choice];

		choices[choice] = value <= armor_sync_limit(choice) ? (short)value : NONE;
	}
	return TRUE;
}

void armor_sync_network_update(
	void)
{
	struct network_game *game = armor_sync_network_game();
	unsigned long now;
	long local_machine;

	if (!game)
	{
		if (armor_sync_in_game)
		{
			armor_sync_clear();
			armor_sync_in_game = FALSE;
		}
		return;
	}
	now = system_milliseconds();
	if (!armor_sync_in_game)
	{
		armor_sync_clear();
		armor_sync_in_game = TRUE;
		armor_sync_said_at = now - ARMOR_SYNC_SAY_MS;
		armor_sync_table_changed = TRUE;
	}
	armor_sync_own_update();
	local_machine = network_game_client_get_local_machine_index();
	if (armor_sync_host())
	{
		struct network_game *hosted = network_game_get_game();

		armor_sync_table_heard = TRUE;
		armor_sync_forget_departed(hosted ? hosted : game);
		if (local_machine >= 0 && local_machine < ARMOR_SYNC_MACHINES &&
			memcmp(armor_sync_table[local_machine], armor_sync_own, sizeof(armor_sync_own)))
		{
			csmemcpy(armor_sync_table[local_machine], armor_sync_own, sizeof(armor_sync_own));
			armor_sync_table_changed = TRUE;
		}
		if ((armor_sync_table_changed && now - armor_sync_table_sent_at >= ARMOR_SYNC_TABLE_SOONEST_MS) ||
			now - armor_sync_table_sent_at >= ARMOR_SYNC_TABLE_MS)
		{
			armor_sync_table_changed = FALSE;
			armor_sync_table_sent_at = now;
			armor_sync_newcomers_at = now;
			armor_sync_send_table(local_machine, FALSE);
		}
		else if (now - armor_sync_newcomers_at >= ARMOR_SYNC_NEWCOMERS_MS)
		{
			armor_sync_newcomers_at = now;
			armor_sync_send_table(local_machine, TRUE);
		}
		return;
	}
	/* (a client: its choices, while the host has others for it) */
	if (local_machine < 0 || local_machine >= ARMOR_SYNC_MACHINES ||
		(armor_sync_table_heard && !memcmp(armor_sync_table[local_machine], armor_sync_own, sizeof(armor_sync_own))) ||
		now - armor_sync_said_at < ARMOR_SYNC_SAY_MS)
	{
		return;
	}
	armor_sync_said_at = now;
	{
		struct distributed_armor_message message;

		csmemset(&message, 0, sizeof(message));
		armor_sync_fill_header(&message.header, _distributed_message_armor, (word)sizeof(message));
		csmemcpy(message.entry.choices, armor_sync_own, sizeof(armor_sync_own));
		network_distributed_client_send_reliably(&message, (word)sizeof(message));
	}
}

boolean armor_sync_handles_message(
	word const *message,
	word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	csmemcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_armor || header.type == _distributed_message_armors;
}

void armor_sync_handle_message(
	long machine_index,
	word const *message,
	word size,
	boolean from_stream)
{
	struct distributed_message_header header;
	byte const *entry = (byte const *)message + sizeof(header);

	if (!armor_sync_handles_message(message, size))
		return;
	csmemcpy(&header, message, sizeof(header));
	if (header.type == _distributed_message_armor)
	{
		struct distributed_armor armor;
		short choice;

		/* (the host, from that machine's own stream: indices in range or
		none) */
		if (machine_index == NONE || !from_stream || !armor_sync_host() || machine_index < 0 ||
			machine_index >= ARMOR_SYNC_MACHINES || size < sizeof(header) + sizeof(armor))
		{
			return;
		}
		csmemcpy(&armor, entry, sizeof(armor));
		for (choice = 0; choice < NUMBER_OF_ARMOR_SYNC_CHOICES; choice++)
		{
			if (armor.choices[choice] != ARMOR_SYNC_NONE && armor.choices[choice] > armor_sync_limit(choice))
				return;
		}
		if (memcmp(armor_sync_table[machine_index], armor.choices, sizeof(armor.choices)))
		{
			unsigned long now = system_milliseconds();

			if (now - armor_sync_heard_at[machine_index] < ARMOR_SYNC_HEARD_MS)
				return;
			armor_sync_heard_at[machine_index] = now;
			csmemcpy(armor_sync_table[machine_index], armor.choices, sizeof(armor.choices));
			armor_sync_table_changed = TRUE;
		}
	}
	else if (machine_index == NONE && !armor_sync_host() && size > sizeof(header))
	{
		/* (whole rows, as many as were sent; the rest none) */
		long rows = (long)(size - sizeof(header)) / (long)sizeof(armor_sync_table[0]);

		if (rows < 1)
			return;
		if (rows > ARMOR_SYNC_MACHINES)
			rows = ARMOR_SYNC_MACHINES;
		csmemcpy(armor_sync_table, entry, rows * sizeof(armor_sync_table[0]));
		csmemset((byte *)armor_sync_table + rows * sizeof(armor_sync_table[0]), ARMOR_SYNC_NONE,
			(ARMOR_SYNC_MACHINES - rows) * sizeof(armor_sync_table[0]));
		armor_sync_table_heard = TRUE;
	}
}
