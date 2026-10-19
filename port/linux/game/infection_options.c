#include "cseries/cseries.h"
#include "game/game_engine.h"
#include "infection.h"

/* Versioned marker within Slayer's existing variant; GPVO stays unchanged. */
#define INFECTION_SIGNATURE 0x31464E49UL /* INF1 */

static struct infection_options const defaults = { FALSE, 1, 180, 200, 5, 5, TRUE, { 0, 0, 0 } };

struct infection_options const *infection_options_get(struct game_variant const *variant)
{
	struct infection_options const *options;
	if (!variant || variant->game_engine_index != game_engine_slayer ||
		variant->game_engine_variant.slayer.infection_signature != INFECTION_SIGNATURE)
		return &defaults;
	options = &variant->game_engine_variant.slayer.infection;
	/* Saved and remote variants are untrusted. Invalid extensions stay off. */
	if (options->infection_enabled > TRUE || options->infection_initial_zombies < 1 ||
		options->infection_initial_zombies > 16 || options->infection_speed_percent < 100 ||
		options->infection_speed_percent > 200 || options->infection_damage_percent < 100 ||
		options->infection_damage_percent > 250 || options->infection_respawn_seconds < 1 ||
		options->infection_respawn_seconds > 10 || options->infection_countdown_seconds < 0 ||
		options->infection_countdown_seconds > 10 || options->infection_zombie_shields > TRUE ||
		options->reserved[0] || options->reserved[1] || options->reserved[2])
		return &defaults;
	return options;
}

struct infection_options *infection_options_edit(struct game_variant *variant)
{
	if (!variant || variant->game_engine_index != game_engine_slayer) return NULL;
	if (infection_options_get(variant) == &defaults)
	{
		variant->game_engine_variant.slayer.infection_signature = INFECTION_SIGNATURE;
		variant->game_engine_variant.slayer.infection = defaults;
	}
	return &variant->game_engine_variant.slayer.infection;
}
