/*
GUN_GAME.C

Gun Game, a mode of free for all Slayer (game_modes.c). Everyone starts with
the first weapon of a list and nothing else; each kill gives the killer the
next, and a kill with the last wins. A player's level is their Slayer score
(the kills less the suicides), so the scores the host sends its clients
(game_engine_slayer_write_network_state) carry it, and the game ends as
Slayer's does, at the score to win, which is the list's length.

The list is those of the loadout weapons (game_engine.h's _loadout_weapon_*)
the map has, the strongest first (the rocket launcher, the shotgun, the
sniper rifle, the plasma rifle, the assault rifle, the needler, the plasma
pistol, the pistol), the weakest first, or in an order the host draws for
each game. Only the host gives weapons: a player's weapons go to every
machine with their inventory (network_objects.c). No one picks up a
weapon; a weapon emptied is replaced with a full one; and a melee kill can
take the victim back a weapon (gun_game_melee_demotes).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_engine_slayer.h"
#include "game/players.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "networking/network_game_globals.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game_modes.h"
#include "gun_game.h"

static byte const gun_game_strongest_first[] =
{
	_loadout_weapon_rocket_launcher,
	_loadout_weapon_shotgun,
	_loadout_weapon_sniper_rifle,
	_loadout_weapon_plasma_rifle,
	_loadout_weapon_assault_rifle,
	_loadout_weapon_needler,
	_loadout_weapon_plasma_pistol,
	_loadout_weapon_pistol,
};

#define GUN_GAME_MAXIMUM_LEVELS NUMBEROF(gun_game_strongest_first)

static struct
{
	struct game_engine engine;
	struct game_engine slayer;
	/* the map's list, its weapons as _loadout_weapon_* */
	byte levels[GUN_GAME_MAXIMUM_LEVELS];
	short level_count;
	/* the host's: each player's unit given a level's weapon, and that level
	(by absolute player index) */
	long equipped_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	short equipped_level[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	/* a weapon being given (gun_game_allow_pick_up allows it) */
	boolean equipping;
} gun_game;

short gun_game_level_count(
	void)
{
	return gun_game.level_count;
}

byte gun_game_level_weapon(
	short level)
{
	return VALID_INDEX(level, gun_game.level_count) ? gun_game.levels[level] : _loadout_weapon_none;
}

/* a player's level: their score, within the list */
static short gun_game_player_level(
	long player_index)
{
	long score = gun_game.slayer.get_player_score(player_index, _get_score_individual);

	return (short)PIN(score, 0, MAX(gun_game.level_count - 1, 0));
}

/* a weapon with no rounds left (none loaded nor carried), or a battery
spent */
static boolean gun_game_weapon_empty(
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_try_and_get(weapon_index);
	struct weapon_definition *definition;
	short magazine;

	if (!weapon)
		return FALSE;
	definition = weapon_definition_get(weapon->definition_index);
	if (definition->weapon.magazines.count <= 0)
		return weapon->weapon.age >= 1.0f;
	for (magazine = 0; magazine < MIN(definition->weapon.magazines.count, NUMBEROF(weapon->weapon.magazines));
		magazine++)
	{
		if (weapon->weapon.magazines[magazine].rounds_loaded > 0 || weapon->weapon.magazines[magazine].rounds_total > 0)
			return FALSE;
	}
	return TRUE;
}

/* every weapon of a unit gone, the one in its hand too (which
unit_delete_all_weapons keeps) */
static void gun_game_delete_weapons(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	short slot;
	short other;

	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[slot];

		if (weapon_index == NONE)
			continue;
		/* (each deleted once, should two slots hold it) */
		for (other = slot; other < MAXIMUM_WEAPONS_PER_UNIT; other++)
		{
			if (unit->unit.weapon_object_indices[other] == weapon_index)
			{
				unit->unit.weapon_object_indices[other] = NONE;
				if (unit->unit.current_weapon_index == other)
					unit->unit.current_weapon_index = NONE;
				if (unit->unit.desired_weapon_index == other)
					unit->unit.desired_weapon_index = NONE;
			}
		}
		if (object_try_and_get(weapon_index))
			object_delete(weapon_index);
	}
}

/* the host gives a living player their level's weapon (their others gone),
again when it is spent */
static void gun_game_equip(
	long player_index)
{
	struct player_datum *player = player_get(player_index);
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	struct unit_datum *unit;
	short level;
	long definition_index;
	long weapon_index;
	struct object_placement_data placement;

	if (network_game_distributed_client() || !VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS) ||
		player->quit_out_of_game || player->unit_index == NONE || !gun_game.level_count)
	{
		return;
	}
	unit = unit_try_and_get(player->unit_index);
	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;
	level = gun_game_player_level(player_index);
	if (gun_game.equipped_unit[slot] == player->unit_index && gun_game.equipped_level[slot] == level)
	{
		long current = VALID_INDEX(unit->unit.current_weapon_index, MAXIMUM_WEAPONS_PER_UNIT) ?
			unit->unit.weapon_object_indices[unit->unit.current_weapon_index] : NONE;

		if (current != NONE && !gun_game_weapon_empty(current))
			return;
	}
	/* (no weapon in the hand: a grenade being thrown has it, so wait) */
	if (unit->unit.grenade_throw_state != _unit_grenade_throw_idle)
		return;
	definition_index = game_engine_loadout_weapon_definition(gun_game.levels[level]);
	if (definition_index == NONE)
		return;
	gun_game.equipping = TRUE;
	gun_game_delete_weapons(player->unit_index);
	object_placement_data_new(&placement, definition_index, NONE);
	weapon_index = object_new(&placement);
	if (weapon_index != NONE && !unit_add_weapon_to_inventory(player->unit_index, weapon_index, 2))
	{
		object_delete(weapon_index);
		weapon_index = NONE;
	}
	gun_game.equipping = FALSE;
	gun_game.equipped_unit[slot] = player->unit_index;
	gun_game.equipped_level[slot] = level;
}

void gun_game_postspawn(
	long player_index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);

	if (VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		gun_game.equipped_unit[slot] = NONE;
	gun_game_equip(player_index);
}

/* the map's list, in the gametype's order */
static void gun_game_levels_build(
	void)
{
	short order = game_variant_options_get()->gun_game_order;
	short index;

	gun_game.level_count = 0;
	for (index = 0; index < (short)GUN_GAME_MAXIMUM_LEVELS; index++)
	{
		byte weapon = gun_game_strongest_first[order == _gun_game_order_weakest_first ?
			GUN_GAME_MAXIMUM_LEVELS - 1 - index : index];

		if (game_engine_loadout_weapon_definition(weapon) != NONE)
			gun_game.levels[gun_game.level_count++] = weapon;
	}
	/* (the host's draw: only the host gives weapons) */
	if (order == _gun_game_order_random && !network_game_distributed_client())
	{
		for (index = gun_game.level_count - 1; index > 0; index--)
		{
			short other = (short)local_random_range(0, index + 1);
			byte weapon = gun_game.levels[index];

			gun_game.levels[index] = gun_game.levels[other];
			gun_game.levels[other] = weapon;
		}
	}
}

static boolean gun_game_initialize_for_new_map(
	void)
{
	gun_game_levels_build();
	csmemset(gun_game.equipped_unit, 0xFF, sizeof(gun_game.equipped_unit));
	csmemset(gun_game.equipped_level, 0xFF, sizeof(gun_game.equipped_level));
	gun_game.equipping = FALSE;
	/* (the game is won with a kill with the last weapon: every machine's
	list is as long) */
	if (gun_game.level_count)
		game_engine_get_variant()->universal_variant.score_to_win = gun_game.level_count;
	return gun_game.slayer.initialize_for_new_map ? gun_game.slayer.initialize_for_new_map() : TRUE;
}

static void gun_game_player_update_each_tick(
	long player_index)
{
	if (gun_game.slayer.player_update_each_tick)
		gun_game.slayer.player_update_each_tick(player_index);
	gun_game_equip(player_index);
}

/* only the weapons given (and a weapon held, which readying it asks of) */
static boolean gun_game_allow_pick_up(
	long unit_index,
	long weapon_index)
{
	struct unit_datum *unit = unit_try_and_get(unit_index);
	short slot;

	if (gun_game.equipping)
		return TRUE;
	for (slot = 0; unit && slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		if (unit->unit.weapon_object_indices[slot] == weapon_index)
			return TRUE;
	}
	return FALSE;
}

static void gun_game_player_killed_player(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire)
{
	gun_game.slayer.player_killed_player(killing_player_index, killing_object_index, dead_player_index, friendly_fire);
	if (network_game_distributed_client() || player_get(dead_player_index)->quit_out_of_game)
		return;
	/* a melee kill takes the victim back a weapon */
	if (game_variant_options_get()->gun_game_melee_demotes && game_modes_killed_by_melee() && !friendly_fire &&
		killing_player_index != NONE && killing_player_index != dead_player_index &&
		gun_game.slayer.get_player_score(dead_player_index, _get_score_individual) > 0)
	{
		game_engine_slayer_adjust_score(dead_player_index, -1);
	}
	/* (a suicide at the first weapon leaves the score at 0) */
	if (gun_game.slayer.get_player_score(dead_player_index, _get_score_individual) < 0)
		game_engine_slayer_adjust_score(dead_player_index, 1);
}

struct game_engine *gun_game_engine_get(
	struct game_engine const *slayer)
{
	gun_game.slayer = *slayer;
	gun_game.engine = *slayer;
	gun_game.engine.initialize_for_new_map = gun_game_initialize_for_new_map;
	gun_game.engine.player_update_each_tick = gun_game_player_update_each_tick;
	gun_game.engine.allow_pick_up = gun_game_allow_pick_up;
	gun_game.engine.player_killed_player = gun_game_player_killed_player;
	return &gun_game.engine;
}
