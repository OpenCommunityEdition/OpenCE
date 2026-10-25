#ifndef NETWORK_SHIELD_EFFECTS_H
#define NETWORK_SHIELD_EFFECTS_H
#include "cseries.h"
/* the shield effects a network game draws (network.shield_effects): the
host's, for every machine, or (players) each player's own */
enum
{
	_shield_effects_stock,
	_shield_effects_glow,
	_shield_effects_light_show,
	_shield_effects_overshield,
	_shield_effects_glow_overshield,
	_shield_effects_light_show_overshield,
	/* not synced: each machine draws its own settings' */
	_shield_effects_players,
	NUMBER_OF_SHIELD_EFFECTS
};
void network_shield_effects_reset(void);
void network_shield_effects_update(void);
void network_shield_effects_send(long machine_index);
short network_shield_effects(void);
boolean network_shield_effects_handles_message(word const *message, word size);
void network_shield_effects_handle_message(long machine_index, word const *message, word size);
#endif
