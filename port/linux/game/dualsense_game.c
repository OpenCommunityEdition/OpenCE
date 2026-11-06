/*
DUALSENSE_GAME.C

Each local player's DualSense wish (dualsense_game.h), for a controller port whose pad is a DualSense the platform side
can drive (port/linux/src/dualsense.c):
- the lightbar in the player's colour (the team's in a team game, the armour's otherwise; a campaign's Chief green),
  and the player lights the split-screen number;
- the physical trigger mapped to fire by the weapon in hand: its profile (dualsense_rules.c) from the tag's own numbers, and the weapon's state each
  frame; each new round (the weapon's last-fired game time moved) is a shot the platform side turns into a kick. In a
  vehicle, a gunner's seat takes the vehicle's weapon; any other seat has none;
- the grenade/zoom trigger a light resistance while zoomed, or a hair trigger while there are grenades to throw.
Only L2 and R2 have actuators. Other mapped actions leave them free; non-playing contexts release both.
*/

#include <string.h>

#include "cseries.h"
#include "real_math.h"
#include "tag_files/tag_groups.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "camera/director.h"
#include "controller_remap.h"
#include "controller_buttons.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "items/weapon_datum_flags.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "../src/dualsense.h"
#include "dualsense_game.h"

static unsigned char channel(real value)
{
	if (value <= 0.0f)
		return 0;
	if (value >= 1.0f)
		return 255;
	return (unsigned char)(value * 255.0f + 0.5f);
}

/* a round in this many ticks counts as firing (the slowest automatic, 6 rounds a second, fires every 5) */
#define FIRING_TICKS 5

/* Game-thread history is separate from checkpoint data. Context changes discard old shots. */
static struct
{
	int valid;
	long player_index, unit_index, weapon_index, last_fired;
	unsigned int instance;
	unsigned long context, kicks;
	int percent;
	unsigned char layout[CONTROLLER_REMAP_ACTIONS];
	float last_charge;
	unsigned char kick[DUALSENSE_EFFECT_SIZE];
	int kick_ms;
} rounds[MAXIMUM_LOCAL_PLAYERS];
static unsigned long next_context;
static int tracking;

static void clear_round(short port)
{
	memset(&rounds[port], 0, sizeof(rounds[port]));
	rounds[port].context = ++next_context;
}

void dualsense_game_reset(void)
{
	short port;

	for (port = 0; port < MAXIMUM_LOCAL_PLAYERS; port++)
		clear_round(port);
	dualsense_clear();
	tracking = 0;
}

/* a weapon definition's kind from its tag's numbers (and its first trigger, NULL: none) */
static enum dualsense_weapon_kind definition_kind(long definition_index, float *rate_of_fire,
	struct weapon_trigger_definition **first_trigger)
{
	struct weapon_definition *definition = weapon_definition_get(definition_index);
	struct weapon_trigger_definition *first = NULL;
	float rate = 0.0f, charging = 0.0f;
	int projectiles = 0;
	long trigger;

	for (trigger = 0; trigger < definition->weapon.triggers.count; trigger++)
	{
		struct weapon_trigger_definition *trigger_definition =
			TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, trigger, struct weapon_trigger_definition);

		if (trigger == 0)
		{
			first = trigger_definition;
			rate = trigger_definition->initial_rate_of_fire > trigger_definition->final_rate_of_fire ?
				trigger_definition->initial_rate_of_fire : trigger_definition->final_rate_of_fire;
		}
		if (trigger_definition->charging_time > charging)
			charging = trigger_definition->charging_time;
		if (trigger_definition->projectile.index != NONE)
			projectiles++;
	}
	*rate_of_fire = rate;
	if (first_trigger)
		*first_trigger = first;
	return dualsense_weapon_kind(definition->weapon.weapon_type == _weapon_type_needler, rate, charging,
		first ? first->heat_generated_per_round : 0.0f, definition->weapon.zoom_level_count, projectiles == 0,
		definition->weapon.weapon_type == _weapon_type_plasma_rifle);
}

/* the weapon in hand now: its kind from the tag's numbers, its heat, charge, firing and magazine */
static void weapon_state(long weapon_index, struct dualsense_weapon_state *state)
{
	struct weapon_datum *weapon;
	struct weapon_definition *definition;
	struct weapon_trigger_definition *first = NULL;
	float rate = 0.0f;
	long trigger;
	long now = game_time_get();

	memset(state, 0, sizeof(*state));
	if (weapon_index == NONE)
		return;
	weapon = weapon_get(weapon_index);
	definition = weapon_definition_get(weapon->definition_index);
	state->kind = definition_kind(weapon->definition_index, &rate, &first);
	state->rate_of_fire = rate;
	/* (the charge: a trigger charging counts its timer down from charging_time) */
	for (trigger = 0; trigger < definition->weapon.triggers.count && trigger < 2; trigger++)
	{
		struct weapon_trigger_definition *trigger_definition =
			TAG_BLOCK_GET_ELEMENT(&definition->weapon.triggers, trigger, struct weapon_trigger_definition);

		if (weapon->weapon.triggers[trigger].state == _trigger_charged)
			state->charge = 1.0f;
		else if (weapon->weapon.triggers[trigger].state == _trigger_charging && trigger_definition->charging_time > 0.0f &&
			state->charge < 1.0f)
		{
			float left = (float)weapon->weapon.triggers[trigger].state_timer /
				(trigger_definition->charging_time * (float)TICKS_PER_SECOND);
			float charge = 1.0f - (left < 0.0f ? 0.0f : left > 1.0f ? 1.0f : left);

			if (charge > state->charge)
				state->charge = charge;
		}
	}
	state->heat = weapon->weapon.heat;
	/* (the game's own overheated flag, which weapons.c clears as the heat falls under the recovery threshold) */
	state->overheated = TEST_FLAG(weapon->weapon.flags, _weapon_overheated_bit) && weapon->weapon.heat > 0.0f ? 1 : 0;
	state->firing = weapon->weapon.game_time_last_fired > 0 && now - weapon->weapon.game_time_last_fired <= FIRING_TICKS;
	/* (switching weapons: the readying and putting-away states, or the player asked to switch) */
	state->readying = weapon->weapon.state == _weapon_state_ready || weapon->weapon.state == _weapon_state_put_away ||
		TEST_FLAG(weapon->weapon.control_flags, _weapon_control_user_switching_weapons_bit);
	state->reloading = weapon->weapon.state == _weapon_state_primary_reload ||
		weapon->weapon.state == _weapon_state_secondary_reload;
	/* (empty: a battery spent, as weapon_useful reads it, or a magazine that holds rounds with none loaded) */
	state->empty = weapon->weapon.age >= 1.0f;
	if (first && first->magazine_index >= 0 && first->magazine_index < 2 &&
		first->magazine_index < definition->weapon.magazines.count)
	{
		struct weapon_magazine const *magazine = &weapon->weapon.magazines[first->magazine_index];
		struct weapon_magazine_definition const *magazine_definition = TAG_BLOCK_GET_ELEMENT(
			&definition->weapon.magazines, first->magazine_index, struct weapon_magazine_definition);

		if (magazine_definition->rounds_loaded_maximum > 0)
		{
			if (magazine->state == _magazine_reloading)
				state->reloading = 1;
			if (magazine->rounds_loaded <= 0)
				state->empty = 1;
		}
	}
}

/* the weapon the player fires: in a vehicle's seat, the vehicle's weapon when the vehicle says this unit is its
gunner (its gunner_object_index: the ghost's driver, the warthog's gunner), none in any other seat; on foot, the one
in hand */
static long seated_weapon(long unit_index, long weapon_index)
{
	struct unit_datum *unit;
	struct unit_datum *parent;

	if (unit_index == NONE)
		return weapon_index;
	unit = unit_get(unit_index);
	if (unit->object.parent_object_index == NONE || unit->unit.parent_seat_index == NONE)
		return weapon_index;
	parent = unit_try_and_get(unit->object.parent_object_index);
	if (!parent || parent->unit.gunner_object_index != unit_index)
		return NONE;
	return unit_inventory_get_weapon(unit->object.parent_object_index, parent->unit.current_weapon_index);
}

/* Only physical triggers can produce resistance. Other actions leave them free. */
static void mapped_effect(unsigned char *effect, int button, unsigned char const *layout,
	struct dualsense_weapon_state const *state, int zoomed, int grenades, int winding, int percent)
{
	if (layout[PAD_FIRE] == button)
		dualsense_right_held(effect, state, percent);
	else if (layout[PAD_GRENADE] == button)
		dualsense_left_effect(effect, zoomed, grenades, winding, percent);
	else if (layout[PAD_ZOOM] == button && zoomed)
		dualsense_left_effect(effect, 1, 0, 0, percent);
	else
		dualsense_effect_off(effect);
}

static void dualsense_player(short port, short local, struct dualsense_settings const *settings)
{
	struct dualsense_wish wish;
	struct dualsense_weapon_state state;
	struct controller_device device;
	unsigned char layout[CONTROLLER_REMAP_ACTIONS];
	long player_index = local_player_get_player_index(local);
	long unit_index = player_get(player_index)->unit_index;
	long weapon_index = NONE;
	struct unit_datum *unit = unit_index != NONE ? unit_get(unit_index) : NULL;
	real_rgb_color color;
	int winding, grenades, zoomed;

	memset(&wish, 0, sizeof(wish));
	dualsense_effect_off(wish.right);
	dualsense_effect_off(wish.left);
	wish.player_lights = dualsense_player_lights(local + 1);
	if (game_engine_running())
		game_engine_player_get_change_color(&color, player_index);
	else
	{
		color.red = 0.25f;
		color.green = 0.45f;
		color.blue = 0.15f;
	}
	wish.red = channel(color.red);
	wish.green = channel(color.green);
	wish.blue = channel(color.blue);
	wish.playing = settings->triggers && unit && controller_remap_playing(port) &&
		!game_time_get_paused() && !director_inhibited_input(local);
	if (!wish.playing)
	{
		if (rounds[port].valid)
			clear_round(port);
		wish.context = rounds[port].context;
		dualsense_wish_set(port, &wish);
		return;
	}

	if (!controller_remap_read(port, layout))
		controller_remap_profile(port, layout);
	controller_device_get(port, &device);
	weapon_index = seated_weapon(unit_index, unit_inventory_get_weapon(unit_index, unit->unit.current_weapon_index));
	if (!rounds[port].valid || rounds[port].player_index != player_index || rounds[port].unit_index != unit_index ||
		rounds[port].weapon_index != weapon_index || rounds[port].instance != device.instance ||
		rounds[port].percent != settings->percent || memcmp(rounds[port].layout, layout, sizeof(layout)))
	{
		clear_round(port);
		rounds[port].player_index = player_index;
		rounds[port].unit_index = unit_index;
		rounds[port].weapon_index = weapon_index;
		rounds[port].instance = device.instance;
		rounds[port].percent = settings->percent;
		memcpy(rounds[port].layout, layout, sizeof(layout));
	}
	weapon_state(weapon_index, &state);
	state.ticks = game_time_get();
	zoomed = player_control_get_zoom_level(local) != NONE;
	winding = unit->unit.grenade_throw_state == _unit_grenade_throw_wind_up ||
		unit->unit.grenade_throw_state == _unit_grenade_throw_in_hand;
	grenades = unit->unit.current_grenade_index >= 0 ? unit_get_grenade_count(unit_index, unit->unit.current_grenade_index) : 0;
	mapped_effect(wish.right, 7, layout, &state, zoomed, grenades, winding, settings->percent);
	mapped_effect(wish.left, 6, layout, &state, zoomed, grenades, winding, settings->percent);
	if (!state.readying && !state.reloading && !state.overheated)
		wish.kick_trigger = layout[PAD_FIRE] == 7 ? 1 : layout[PAD_FIRE] == 6 ? 2 : 0;
	if (weapon_index != NONE)
	{
		long last_fired = weapon_get(weapon_index)->weapon.game_time_last_fired;

		if (rounds[port].valid && last_fired > rounds[port].last_fired && wish.kick_trigger)
		{
			rounds[port].kick_ms = dualsense_event(rounds[port].kick, state.kind,
				rounds[port].last_charge >= 0.95f ? DUALSENSE_EVENT_SHOT_CHARGED : DUALSENSE_EVENT_SHOT, settings->percent);
			if (rounds[port].kick_ms > 0)
				rounds[port].kicks++;
		}
		rounds[port].last_fired = last_fired;
	}
	rounds[port].valid = 1;
	rounds[port].last_charge = state.charge;
	wish.context = rounds[port].context;
	memcpy(wish.kick, rounds[port].kick, sizeof(wish.kick));
	wish.kick_ms = rounds[port].kick_ms;
	wish.kicks = rounds[port].kicks;
	dualsense_wish_set(port, &wish);
}

/* After every local player's input update, including dead players and menus. */
void dualsense_game_update(void)
{
	struct dualsense_settings settings = dualsense_settings_get();
	short port;

	if ((!settings.triggers && !settings.lights) || !game_time_initialized() || !game_in_progress() || main_menu_is_active())
	{
		if (tracking)
			dualsense_game_reset();
		return;
	}
	tracking = 1;
	for (port = 0; port < MAXIMUM_LOCAL_PLAYERS; port++)
	{
		short local = controller_remap_local_player(port);

		if (local != NONE && dualsense_port_active(port))
			dualsense_player(port, local, &settings);
		else
		{
			struct dualsense_wish wish;

			if (rounds[port].valid)
				clear_round(port);
			memset(&wish, 0, sizeof(wish));
			wish.context = rounds[port].context;
			dualsense_effect_off(wish.right);
			dualsense_effect_off(wish.left);
			dualsense_wish_set(port, &wish);
		}
	}
}
