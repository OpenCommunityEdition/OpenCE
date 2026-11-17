/* The map's riding choice overrides the host's pregame switch for this map,
and the host's latched riding values replace its clients' own. The host sends
both reliably at every map start, when either changes, and when a client
becomes ready, with the choice none when the map makes none (the pregame
switch then still decides). */
#include "cseries.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "main/main.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_riding_map.h"
#include "vehicle_riding.h"
#include <string.h>

static short sent_map = NONE;
static boolean sent;
static struct distributed_riding_latched sent_values;

struct distributed_riding_map_message
{
	struct distributed_message_header header;
	word map;
	word reserved;
	struct distributed_riding_latched values;
};

static void fill(struct distributed_riding_map_message *message)
{
	memset(message, 0, sizeof(*message));
	message->map = (word)(vehicle_riding_map() + 1);
	vehicle_riding_latched_get(&message->values);
}

void network_riding_map_reset(void)
{
	vehicle_riding_map_received(NONE);
	vehicle_riding_latched_forget();
	sent_map = NONE;
	sent = FALSE;
}

void network_riding_map_send(long machine_index)
{
	struct distributed_riding_map_message message;

	if (game_connection() != _game_connection_network_server)
		return;
	fill(&message);
	distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_vehicle_riding_map, 1,
		sizeof(message));
}

void network_riding_map_update(void)
{
	struct distributed_riding_map_message message;

	if (!game_in_progress() || main_menu_is_active() || game_connection() != _game_connection_network_server)
		return;
	fill(&message);
	if (!sent || sent_map != vehicle_riding_map() || memcmp(&sent_values, &message.values, sizeof(sent_values)))
	{
		sent = TRUE;
		sent_map = vehicle_riding_map();
		sent_values = message.values;
		distributed_send(&message, _distributed_message_vehicle_riding_map, 1, sizeof(message),
			_distributed_to_clients_reliably);
	}
}

boolean network_riding_map_handles_message(word const *message, word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	memcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_vehicle_riding_map;
}

/* only the host's reliable stream, whole, while the game is running */
void network_riding_map_handle_message(long machine_index, word const *data, word size)
{
	struct distributed_riding_map_message message;

	if (machine_index != NONE || game_connection() != _game_connection_network_client ||
		!game_in_progress() || main_menu_is_active() || network_connection_last_read_was_unreliable() || size != sizeof(message))
		return;
	memcpy(&message, data, sizeof(message));
	if (GET_MESSAGE_TYPE(message.header.header) != 2 || message.header.type != _distributed_message_vehicle_riding_map ||
		message.header.count != 1 || message.map > 2 || message.reserved != 0 || message.values.look_carry > 1)
		return;
	vehicle_riding_map_received((short)message.map - 1);
	vehicle_riding_latched_received(&message.values);
}
