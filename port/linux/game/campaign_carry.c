/*
CAMPAIGN_CARRY.C

A won campaign level's weapons carried into the next (game.carry_weapons,
off by default): each local player's weapons as the level is won, with
their rounds and battery, the one in hand, and the grenades, given at the
next level's first spawn in place of the loadout the level starts with.
The Xbox starts every level with its own loadout.

Every level of the campaign gives its loadout once, as the player spawns
in its first tick, before its first checkpoint (the scripts give none at
the start): a revert keeps what was carried. Restarting the level gives it
again; any other map forgets it. A weapon the next map does not have is
left out, and one of the level's own takes its place, so that the player
has as many weapons as were carried. A game of this machine only, alone or
in split screen: network co-op's players come and go between its rounds.
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "objects/objects.h"
#include "tag_files/tag_files.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"

#include "campaign_carry.h"
#include "mcc_grenades.h"

#include <string.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);

enum
{
	/* (the grenades' types: the Xbox's two, and MCC maps' two more,
	mcc_grenades.c) */
	CAMPAIGN_CARRY_GRENADE_TYPES = 4,
	CAMPAIGN_CARRY_NAME_SIZE = 256,
	CAMPAIGN_CARRY_MAGAZINES = 2,
};

enum
{
	_campaign_carry_none = 0,
	/* taken as a level was won, for the next map begun */
	_campaign_carry_pending,
	/* that map begun: given at each local player's first spawn in it */
	_campaign_carry_active,
};

/* a weapon carried: its tag's name (its index is the old map's), its
magazines' rounds and its battery (age, 1 spent) */
struct campaign_carry_weapon
{
	char name[CAMPAIGN_CARRY_NAME_SIZE];
	long definition_index;
	short rounds_total[CAMPAIGN_CARRY_MAGAZINES];
	short rounds_loaded[CAMPAIGN_CARRY_MAGAZINES];
	real age;
};

struct campaign_carry_player
{
	boolean taken;
	boolean given;
	short weapon_count;
	/* (the weapon in hand, NONE for none) */
	short weapon_in_hand;
	struct campaign_carry_weapon weapons[MAXIMUM_WEAPONS_PER_UNIT];
	short grenade_counts[CAMPAIGN_CARRY_GRENADE_TYPES];
};

static short campaign_carry_state = _campaign_carry_none;
static char campaign_carry_level[CAMPAIGN_CARRY_NAME_SIZE];
static struct campaign_carry_player campaign_carry_players[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];

/* a weapon's rounds and battery, kept */
static void campaign_carry_weapon_keep(
	struct campaign_carry_weapon *kept,
	long weapon_index)
{
	struct weapon_datum *weapon = weapon_get(weapon_index);
	short magazine;

	kept->definition_index = weapon->definition_index;
	for (magazine = 0; magazine < CAMPAIGN_CARRY_MAGAZINES; magazine++)
	{
		kept->rounds_total[magazine] = weapon->weapon.magazines[magazine].rounds_total;
		kept->rounds_loaded[magazine] = weapon->weapon.magazines[magazine].rounds_loaded;
	}
	kept->age = weapon->weapon.age;
}

/* a player's weapons and grenades as the level is won: none for one dead
or not spawned, who starts the next level as it starts anyone */
static void campaign_carry_take_player(
	struct campaign_carry_player *carried,
	long unit_index)
{
	struct unit_datum *unit;
	short in_hand;
	short slot;
	short type;

	csmemset(carried, 0, sizeof(*carried));
	carried->weapon_in_hand = NONE;
	if (unit_index == NONE)
		return;
	unit = unit_get(unit_index);
	if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;
	/* (the weapon being readied, if none is in hand yet) */
	in_hand = unit->unit.current_weapon_index != NONE ?
		unit->unit.current_weapon_index : unit->unit.desired_weapon_index;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[slot];
		struct campaign_carry_weapon *kept;

		if (weapon_index == NONE)
			continue;
		kept = &carried->weapons[carried->weapon_count];
		campaign_carry_weapon_keep(kept, weapon_index);
		csstrncpy(kept->name, tag_get_name(kept->definition_index), CAMPAIGN_CARRY_NAME_SIZE - 1);
		kept->name[CAMPAIGN_CARRY_NAME_SIZE - 1] = 0;
		if (slot == in_hand)
			carried->weapon_in_hand = carried->weapon_count;
		carried->weapon_count++;
	}
	for (type = 0; type < CAMPAIGN_CARRY_GRENADE_TYPES; type++)
	{
		carried->grenade_counts[type] = (short)(type < NUMBER_OF_UNIT_GRENADE_TYPES ?
			MAX(unit->unit.grenade_counts[type], 0) : mcc_grenades_get(unit_index, type));
	}
	carried->taken = TRUE;
}

void campaign_carry_take(
	void)
{
	short local_player_index;

	campaign_carry_state = _campaign_carry_none;
	if (!config_boolean("game.carry_weapons") ||
		game_connection() != _game_connection_local ||
		game_engine_running())
	{
		return;
	}
	for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
	{
		long player_index = local_player_get_player_index(local_player_index);

		campaign_carry_take_player(&campaign_carry_players[local_player_index],
			player_index == NONE ? NONE : player_get(player_index)->unit_index);
	}
	campaign_carry_state = _campaign_carry_pending;
}

void campaign_carry_new_map(
	char const *map_name)
{
	short local_player_index;

	/* (the map begun after the level won, or that map begun again: a
	restart; any other forgets what was carried) */
	if (campaign_carry_state == _campaign_carry_pending ||
		(campaign_carry_state == _campaign_carry_active && !strcmp(map_name, campaign_carry_level)))
	{
		campaign_carry_state = _campaign_carry_active;
		csstrncpy(campaign_carry_level, map_name, CAMPAIGN_CARRY_NAME_SIZE - 1);
		campaign_carry_level[CAMPAIGN_CARRY_NAME_SIZE - 1] = 0;
		for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
			campaign_carry_players[local_player_index].given = FALSE;
	}
	else
	{
		campaign_carry_state = _campaign_carry_none;
	}
}

/* whether a loadout has a weapon of that tag */
static boolean campaign_carry_holds(
	struct campaign_carry_weapon const *loadout,
	short count,
	long definition_index)
{
	short index;

	for (index = 0; index < count; index++)
	{
		if (loadout[index].definition_index == definition_index)
			return TRUE;
	}
	return FALSE;
}

/* a weapon made in the unit's hands (the first given) or inventory, its
rounds and battery as carried, within what this map's tag holds */
static void campaign_carry_add(
	long unit_index,
	struct campaign_carry_weapon const *carried,
	boolean in_hand)
{
	struct object_placement_data placement;
	struct weapon_definition *definition;
	struct weapon_datum *weapon;
	long weapon_index;
	short magazine;

	object_placement_data_new(&placement, carried->definition_index, unit_index);
	weapon_index = object_new(&placement);
	if (weapon_index == NONE)
		return;
	if (object_get(weapon_index)->object.type != _object_type_weapon)
	{
		object_delete(weapon_index);
		return;
	}
	weapon = weapon_get(weapon_index);
	definition = weapon_definition_get(weapon->definition_index);
	for (magazine = 0; magazine < CAMPAIGN_CARRY_MAGAZINES && magazine < definition->weapon.magazines.count; magazine++)
	{
		struct weapon_magazine_definition *magazine_definition = TAG_BLOCK_GET_ELEMENT(
			&definition->weapon.magazines, magazine, struct weapon_magazine_definition);

		weapon->weapon.magazines[magazine].rounds_loaded =
			(short)PIN(carried->rounds_loaded[magazine], 0, magazine_definition->rounds_loaded_maximum);
		weapon->weapon.magazines[magazine].rounds_total =
			(short)PIN(carried->rounds_total[magazine], 0, magazine_definition->rounds_total_maximum);
	}
	weapon->weapon.age = PIN(carried->age, 0.f, 1.f);
	if (!unit_add_weapon_to_inventory(unit_index, weapon_index,
		in_hand ? _unit_add_weapon_starting : _unit_add_weapon_normal))
	{
		object_delete(weapon_index);
	}
}

/* the carried weapons and grenades in place of the level's loadout, just
given to the unit */
static void campaign_carry_give_player(
	struct campaign_carry_player const *carried,
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);
	struct campaign_carry_weapon starting[MAXIMUM_WEAPONS_PER_UNIT];
	struct campaign_carry_weapon loadout[MAXIMUM_WEAPONS_PER_UNIT];
	short starting_count = 0;
	short count = 0;
	short index;
	short slot;
	short type;

	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		if (unit->unit.weapon_object_indices[slot] != NONE)
			campaign_carry_weapon_keep(&starting[starting_count++], unit->unit.weapon_object_indices[slot]);
	}
	/* the carried weapons this map has, the one in hand first */
	for (index = -1; index < carried->weapon_count; index++)
	{
		short weapon = index < 0 ? carried->weapon_in_hand : index;
		long definition_index;

		if (weapon == NONE || (index >= 0 && index == carried->weapon_in_hand))
			continue;
		definition_index = tag_loaded(WEAPON_DEFINITION_TAG, carried->weapons[weapon].name);
		if (definition_index == NONE || campaign_carry_holds(loadout, count, definition_index))
			continue;
		loadout[count] = carried->weapons[weapon];
		loadout[count].definition_index = definition_index;
		count++;
	}
	/* (none: the level's own loadout is kept) */
	if (count > 0)
	{
		/* a place left by a weapon this map lacks, taken by one of the level's */
		for (index = 0; index < starting_count && count < carried->weapon_count; index++)
		{
			if (!campaign_carry_holds(loadout, count, starting[index].definition_index))
				loadout[count++] = starting[index];
		}
		unit_delete_all_weapons(unit_index);
		for (index = 0; index < count; index++)
			campaign_carry_add(unit_index, &loadout[index], index == 0);
	}
	for (type = 0; type < CAMPAIGN_CARRY_GRENADE_TYPES; type++)
	{
		if (type < NUMBER_OF_UNIT_GRENADE_TYPES)
			unit->unit.grenade_counts[type] = (char)PIN(carried->grenade_counts[type], 0, 127);
		else
			mcc_grenades_set(unit_index, type, carried->grenade_counts[type]);
	}
}

void campaign_carry_give(
	long unit_index,
	short local_player_index)
{
	struct campaign_carry_player *carried;

	if (campaign_carry_state != _campaign_carry_active || unit_index == NONE ||
		local_player_index < 0 || local_player_index >= MAXIMUM_NUMBER_OF_LOCAL_PLAYERS)
	{
		return;
	}
	carried = &campaign_carry_players[local_player_index];
	/* (once a level: a player spawning again, in split screen, starts as
	the level starts anyone) */
	if (!carried->taken || carried->given)
		return;
	carried->given = TRUE;
	campaign_carry_give_player(carried, unit_index);
}
