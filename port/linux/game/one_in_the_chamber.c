/*
ONE_IN_THE_CHAMBER.C

One in the Chamber, a mode of free for all Slayer (game_modes.c). Everyone
starts with a pistol holding one round, with none carried and no grenades;
nothing else is picked up. A bullet of another player's kills at once, body
or head (game_modes_hit_kills, from damage.c), and a kill gives the killer
a round, into the pistol's magazine while it has room, else carried; or,
with the gametype's chamber_kill_resets, sets the pistol back to one round.
The built-in gametype has lives (3), so the last player standing wins, as
any game with lives ends (game_engine.c's multiple_teams_alive) and ranks
its players (adjust_score_for_ranking).

Only the host gives the pistol, its rounds and kills: a player's weapons go
to every machine with their inventory (network_objects.c).
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "objects/objects.h"
#include "units/units.h"
#include "game_modes.h"
#include "one_in_the_chamber.h"

void platform_log(char const *format, ...);

static struct
{
	struct game_engine engine;
	struct game_engine slayer;
	/* the pistol being given (one_in_the_chamber_allow_pick_up allows it) */
	boolean equipping;
	/* the missing pistol has been logged */
	boolean no_pistol_logged;
} one_in_the_chamber;

void one_in_the_chamber_round_for_kill(
	short *loaded,
	short *carried,
	short loaded_maximum,
	short carried_maximum,
	boolean resets)
{
	if (resets)
	{
		*loaded = 1;
		*carried = 0;
	}
	else if (*loaded < loaded_maximum)
		(*loaded)++;
	else if (*carried < carried_maximum)
		(*carried)++;
}

void one_in_the_chamber_postspawn(
	long player_index)
{
	struct unit_datum *unit = game_modes_host_unit(player_index);
	long definition_index = game_engine_loadout_weapon_definition(_loadout_weapon_pistol);
	struct object_placement_data placement;
	struct weapon_datum *weapon;
	long weapon_index;

	if (!unit)
		return;
	if (definition_index == NONE)
	{
		/* (logged once: the loadouts have no pistol to give) */
		if (!one_in_the_chamber.no_pistol_logged)
		{
			one_in_the_chamber.no_pistol_logged = TRUE;
			platform_log("one in the chamber: no pistol in this map's loadouts, players spawn unarmed");
		}
		return;
	}
	one_in_the_chamber.equipping = TRUE;
	game_modes_delete_weapons(player_get(player_index)->unit_index);
	object_placement_data_new(&placement, definition_index, NONE);
	weapon_index = object_new(&placement);
	if (weapon_index != NONE && !unit_add_weapon_to_inventory(player_get(player_index)->unit_index, weapon_index, 2))
	{
		object_delete(weapon_index);
		weapon_index = NONE;
	}
	one_in_the_chamber.equipping = FALSE;
	unit->unit.grenade_counts[_unit_grenade_human_fragmentation] = 0;
	unit->unit.grenade_counts[_unit_grenade_covenant_plasma] = 0;
	weapon = weapon_index != NONE ? weapon_try_and_get(weapon_index) : NULL;
	if (!weapon)
		return;
	weapon->weapon.magazines[0].rounds_loaded = 1;
	weapon->weapon.magazines[0].rounds_total = 0;
	platform_log("one in the chamber: player %ld spawns with a pistol, 1 round",
		(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index));
}

/* only the pistol given (and a weapon held, which readying it asks of) */
static boolean one_in_the_chamber_allow_pick_up(
	long unit_index,
	long weapon_index)
{
	struct unit_datum *unit = unit_try_and_get(unit_index);
	short slot;

	if (one_in_the_chamber.equipping)
		return TRUE;
	for (slot = 0; unit && slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		if (unit->unit.weapon_object_indices[slot] == weapon_index)
			return TRUE;
	}
	return FALSE;
}

static void one_in_the_chamber_player_killed_player(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire)
{
	struct unit_datum *unit;
	short slot;

	one_in_the_chamber.slayer.player_killed_player(killing_player_index, killing_object_index, dead_player_index,
		friendly_fire);
	if (friendly_fire || killing_player_index == dead_player_index)
		return;
	unit = game_modes_host_unit(killing_player_index);
	/* the killer's pistol (their first weapon with a magazine) */
	for (slot = 0; unit && slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[slot];
		struct weapon_datum *weapon = weapon_index != NONE ? weapon_try_and_get(weapon_index) : NULL;
		struct weapon_definition *definition;
		struct weapon_magazine_definition const *magazine;

		if (!weapon)
			continue;
		definition = weapon_definition_get(weapon->definition_index);
		if (definition->weapon.magazines.count <= 0)
			continue;
		magazine = TAG_BLOCK_GET_ELEMENT(&definition->weapon.magazines, 0, struct weapon_magazine_definition);
		one_in_the_chamber_round_for_kill(&weapon->weapon.magazines[0].rounds_loaded,
			&weapon->weapon.magazines[0].rounds_total, magazine->rounds_loaded_maximum,
			magazine->rounds_total_maximum, game_variant_options_get()->chamber_kill_resets);
		platform_log("one in the chamber: player %ld's kill: %d rounds loaded, %d carried",
			(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(killing_player_index), (int)weapon->weapon.magazines[0].rounds_loaded,
			(int)weapon->weapon.magazines[0].rounds_total);
		return;
	}
}

struct game_engine *one_in_the_chamber_engine_get(
	struct game_engine const *slayer)
{
	one_in_the_chamber.slayer = *slayer;
	one_in_the_chamber.engine = *slayer;
	one_in_the_chamber.engine.allow_pick_up = one_in_the_chamber_allow_pick_up;
	one_in_the_chamber.engine.player_killed_player = one_in_the_chamber_player_killed_player;
	return &one_in_the_chamber.engine;
}
