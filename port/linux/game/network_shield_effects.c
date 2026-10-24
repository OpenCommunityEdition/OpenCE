/* The shield effects of a network game (network.shield_effects): the shield
glow, as far as it reaches, and the overshield effect are the host's choice,
for every machine, so that no player sees what another does not. Offline,
each player's own (display.shield_glow, display.shield_glow_intensity,
display.overshield_effect), and online too when the host chooses "players"
(each player's own, not synced). The host sends its choice to each client
reliably when it is ready and when it changes; until it arrives, a client
draws the stock game's. */
#include "cseries.h"
#include "game/game.h"
#include "interface/ui_widget.h"
#include "main/main.h"
#include "networking/network_game_globals.h"
#include "network_distributed.h"
#include "network_shield_effects.h"
#include <string.h>

char const *config_string(char const *name);
unsigned long config_changes(void);

static char const *const shield_effects_names[NUMBER_OF_SHIELD_EFFECTS] =
{
	"stock", "glow", "light_show", "overshield", "glow_overshield", "light_show_overshield", "players"
};
static short client_effects = _shield_effects_stock;
static short sent_effects = NONE;

struct shield_effects_message
{
	struct distributed_message_header header;
	word effects;
	word reserved;
};

/* the host's choice, read again when a setting changes (asked for each
unit drawn) */
static short host_effects(void)
{
	static unsigned long read_at = (unsigned long)-1;
	static short effects = _shield_effects_stock;

	if (read_at != config_changes())
	{
		char const *name = config_string("network.shield_effects");
		short index;

		read_at = config_changes();
		effects = _shield_effects_stock;
		for (index = 0; name && index < NUMBER_OF_SHIELD_EFFECTS; index++)
		{
			if (!strcmp(name, shield_effects_names[index]))
				effects = index;
		}
	}
	return effects;
}

void network_shield_effects_reset(void)
{
	client_effects = _shield_effects_stock;
	sent_effects = NONE;
}

/* the effects drawn: the host's in a network game, NONE (each player's own
settings) otherwise, or when the host leaves them to each player */
short network_shield_effects(void)
{
	short effects = NONE;

	if (game_connection() == _game_connection_network_server)
		effects = host_effects();
	else if (game_connection() == _game_connection_network_client)
		effects = client_effects;
	return effects == _shield_effects_players ? NONE : effects;
}

static void shield_effects_message_make(struct shield_effects_message *message, short effects)
{
	memset(message, 0, sizeof(*message));
	message->effects = (word)effects;
}

void network_shield_effects_send(long machine_index)
{
	struct shield_effects_message message;

	if (game_connection() != _game_connection_network_server)
		return;
	shield_effects_message_make(&message, host_effects());
	distributed_send_to_machine_reliably(machine_index, &message, _distributed_message_shield_effects, 1,
		sizeof(message));
}

void network_shield_effects_update(void)
{
	struct shield_effects_message message;
	short effects;

	if (!game_in_progress() || main_menu_is_active() || game_connection() != _game_connection_network_server)
		return;
	effects = host_effects();
	if (sent_effects == effects)
		return;
	sent_effects = effects;
	shield_effects_message_make(&message, effects);
	distributed_send(&message, _distributed_message_shield_effects, 1, sizeof(message),
		_distributed_to_clients_reliably);
}

boolean network_shield_effects_handles_message(word const *message, word size)
{
	struct distributed_message_header header;

	if (size < sizeof(header))
		return FALSE;
	memcpy(&header, message, sizeof(header));
	return header.type == _distributed_message_shield_effects;
}

/* (the host's alone, over its stream, whole) */
void network_shield_effects_handle_message(long machine_index, word const *data, word size)
{
	struct shield_effects_message message;

	if (machine_index != NONE || game_connection() != _game_connection_network_client ||
		network_connection_last_read_was_unreliable() || size != sizeof(message))
		return;
	memcpy(&message, data, sizeof(message));
	if (GET_MESSAGE_TYPE(message.header.header) != 2 || message.header.type != _distributed_message_shield_effects ||
		message.header.count != 1 || message.effects >= NUMBER_OF_SHIELD_EFFECTS || message.reserved != 0)
		return;
	client_effects = (short)message.effects;
}
