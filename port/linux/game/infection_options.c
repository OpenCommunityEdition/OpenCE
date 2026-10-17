#include "cseries/cseries.h"
#include "game/game_engine.h"
#include "infection.h"

void infection_options_default(struct game_variant_options *options)
{
	options->infection_enabled = FALSE;
	options->infection_initial_zombies = 1;
	options->infection_speed_percent = 180;
	options->infection_damage_percent = 200;
	options->infection_respawn_seconds = 5;
	options->infection_countdown_seconds = 5;
	options->infection_zombie_shields = TRUE;
	options->infection_reserved[0] = options->infection_reserved[1] = options->infection_reserved[2] = 0;
}

void infection_options_validate(struct game_variant const *variant, struct game_variant_options *options)
{
	/* port: validate stored reserves even while their Infection rule is off. */
	variant_ammo_options_validate(options);
	options->infection_enabled = options->infection_enabled == TRUE && variant &&
		variant->game_engine_index == game_engine_slayer && variant->universal_variant.teams;
	options->infection_initial_zombies = (byte)PIN(options->infection_initial_zombies, 1, 16);
	options->infection_speed_percent = (byte)PIN(options->infection_speed_percent, 100, 200);
	options->infection_damage_percent = (byte)PIN(options->infection_damage_percent, 100, 250);
	options->infection_respawn_seconds = (short)PIN(options->infection_respawn_seconds, 1, 10);
	options->infection_countdown_seconds = (short)PIN(options->infection_countdown_seconds, 0, 10);
	options->infection_zombie_shields = options->infection_zombie_shields != FALSE;
	options->infection_reserved[0] = options->infection_reserved[1] = options->infection_reserved[2] = 0;
	if (options->infection_enabled)
	{
		options->auto_team_balance = FALSE;
		/* Team Options own friendly fire and its penalty, as in Team Slayer. */
		options->time_limit = (short)PIN(options->time_limit, 0, 120);
	}
}
