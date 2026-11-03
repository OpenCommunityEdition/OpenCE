/*
FRAG_NATION.C

Frag Nation and Snowball Fight, modes of Slayer played with grenades
(game_modes.c). Everyone starts with no weapon, whose blow is a bare hand's
(bipeds.c, units.c: a gametype's loadout of none) and who throws grenades as
any player does, and nothing else is picked up.

- Frag Nation: two frag and two plasma grenades. A grenade kill gives the
  killer grenades back, of the kind that made it: a frag's explosion a frag,
  a plasma's a plasma, a plasma stuck to the one killed two plasmas, each up
  to the most a player carries. The map's grenades lie on it, unless the
  gametype's frag_nation_no_map_grenades leaves them off
  (game_modes_map_grenades_off).
- Snowball Fight: four plasma grenades, none on the map; a plasma kill gives
  plasmas back as Frag Nation's does.

A kill's damage (game_modes_killing_damage, from damage.c) is a grenade's if
it is the damage of the globals' grenade's projectile: its stuck damage, or a
damage of its detonation's effects. Only the host gives grenades: a player's
grenades go to every machine with their unit (network_objects.c).
*/

#include "cseries.h"
#include "effects/effect_definitions.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "items/projectile_definitions.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "tag_files/tag_groups.h"
#include "units/units.h"
#include "frag_nation.h"
#include "game_modes.h"

void platform_log(char const *format, ...);

/* the grenades at a spawn: Frag Nation's frags and plasmas, Snowball
Fight's plasmas */
enum
{
	FRAG_NATION_FRAGS = 2,
	FRAG_NATION_PLASMAS = 2,
	SNOWBALL_FIGHT_PLASMAS = 4
};

static struct
{
	struct game_engine engine;
	struct game_engine slayer;
} frag_nation;

void frag_nation_grenades_for_kill(
	short kill,
	boolean plasmas_only,
	short *frags,
	short *plasmas)
{
	*frags = 0;
	*plasmas = 0;
	switch (kill)
	{
	case _frag_nation_kill_frag:
		*frags = plasmas_only ? 0 : 1;
		break;
	case _frag_nation_kill_plasma:
		*plasmas = 1;
		break;
	case _frag_nation_kill_plasma_stuck:
		*plasmas = 2;
		break;
	}
}

short frag_nation_grenades_added(
	short count,
	short added,
	short maximum)
{
	if (added <= 0 || count >= maximum)
		return count;
	return (short)(count + added > maximum ? maximum : count + added);
}

/* the most grenades of a kind a unit carries (the globals'), 0 for none */
static short frag_nation_grenade_maximum(
	short grenade_type)
{
	struct game_globals *globals = scenario_get_game_globals();

	if (!globals || !VALID_INDEX(grenade_type, globals->grenades.count))
		return 0;
	return TAG_BLOCK_GET_ELEMENT(&globals->grenades, grenade_type, struct game_globals_grenade)->maximum_count;
}

/* whether an effect's parts deal that damage */
static boolean frag_nation_effect_deals(
	long effect_definition_index,
	long damage_definition_index)
{
	struct effect_definition *effect;
	short event_index;
	short part_index;

	if (effect_definition_index == NONE)
		return FALSE;
	effect = (struct effect_definition *)tag_get(EFFECT_DEFINITION_TAG, effect_definition_index);
	for (event_index = 0; event_index < effect->events.count; event_index++)
	{
		struct effect_event_definition *event =
			TAG_BLOCK_GET_ELEMENT(&effect->events, event_index, struct effect_event_definition);

		for (part_index = 0; part_index < event->parts.count; part_index++)
		{
			if (TAG_BLOCK_GET_ELEMENT(&event->parts, part_index, struct effect_part_definition)->reference.index ==
				damage_definition_index)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* the kind of a kill's damage: a grenade's, by the globals' grenades'
projectiles */
static short frag_nation_kill_kind(
	long damage_definition_index)
{
	struct game_globals *globals = scenario_get_game_globals();
	short grenade_type;

	if (!globals || damage_definition_index == NONE)
		return _frag_nation_kill_other;
	for (grenade_type = 0; grenade_type < MIN(globals->grenades.count, NUMBER_OF_UNIT_GRENADE_TYPES); grenade_type++)
	{
		long projectile_index =
			TAG_BLOCK_GET_ELEMENT(&globals->grenades, grenade_type, struct game_globals_grenade)->projectile.index;
		boolean plasma = grenade_type == _unit_grenade_covenant_plasma;
		struct projectile_definition *projectile;
		short response;

		if (projectile_index == NONE)
			continue;
		projectile = projectile_definition_get(projectile_index);
		if (projectile->projectile.attached_detonation_damage.index == damage_definition_index)
			return plasma ? _frag_nation_kill_plasma_stuck : _frag_nation_kill_frag;
		if (frag_nation_effect_deals(projectile->projectile.effect.index, damage_definition_index) ||
			frag_nation_effect_deals(projectile->projectile.super_detonation.index, damage_definition_index))
		{
			return plasma ? _frag_nation_kill_plasma : _frag_nation_kill_frag;
		}
		for (response = 0; response < projectile->projectile.material_responses.count; response++)
		{
			struct projectile_material_response_definition *material_response = TAG_BLOCK_GET_ELEMENT(
				&projectile->projectile.material_responses, response, struct projectile_material_response_definition);

			if (frag_nation_effect_deals(material_response->detonation_effect.index, damage_definition_index))
				return plasma ? _frag_nation_kill_plasma : _frag_nation_kill_frag;
		}
	}
	return _frag_nation_kill_other;
}

void frag_nation_postspawn(
	long player_index)
{
	struct unit_datum *unit = game_modes_host_unit(player_index);
	boolean snowball = game_modes_mode() == _game_mode_snowball_fight;

	if (!unit)
		return;
	game_modes_delete_weapons(player_get(player_index)->unit_index);
	unit->unit.grenade_counts[_unit_grenade_human_fragmentation] = (char)frag_nation_grenades_added(0,
		snowball ? 0 : FRAG_NATION_FRAGS, frag_nation_grenade_maximum(_unit_grenade_human_fragmentation));
	unit->unit.grenade_counts[_unit_grenade_covenant_plasma] = (char)frag_nation_grenades_added(0,
		snowball ? SNOWBALL_FIGHT_PLASMAS : FRAG_NATION_PLASMAS,
		frag_nation_grenade_maximum(_unit_grenade_covenant_plasma));
	platform_log("%s: player %ld spawns with no weapon, %d frag, %d plasma",
		snowball ? "snowball fight" : "frag nation", (long)DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index),
		(int)unit->unit.grenade_counts[_unit_grenade_human_fragmentation],
		(int)unit->unit.grenade_counts[_unit_grenade_covenant_plasma]);
}

/* no weapon picked up (but one held, which readying it asks of) */
static boolean frag_nation_allow_pick_up(
	long unit_index,
	long weapon_index)
{
	struct unit_datum *unit = unit_try_and_get(unit_index);
	short slot;

	for (slot = 0; unit && slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		if (unit->unit.weapon_object_indices[slot] == weapon_index)
			return TRUE;
	}
	return FALSE;
}

static void frag_nation_player_killed_player(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire)
{
	struct unit_datum *unit;
	short kill;
	short frags;
	short plasmas;

	frag_nation.slayer.player_killed_player(killing_player_index, killing_object_index, dead_player_index,
		friendly_fire);
	if (friendly_fire || killing_player_index == dead_player_index)
		return;
	unit = game_modes_host_unit(killing_player_index);
	if (!unit)
		return;
	kill = frag_nation_kill_kind(game_modes_killing_damage());
	frag_nation_grenades_for_kill(kill, game_modes_mode() == _game_mode_snowball_fight, &frags, &plasmas);
	unit->unit.grenade_counts[_unit_grenade_human_fragmentation] = (char)frag_nation_grenades_added(
		unit->unit.grenade_counts[_unit_grenade_human_fragmentation], frags,
		frag_nation_grenade_maximum(_unit_grenade_human_fragmentation));
	unit->unit.grenade_counts[_unit_grenade_covenant_plasma] = (char)frag_nation_grenades_added(
		unit->unit.grenade_counts[_unit_grenade_covenant_plasma], plasmas,
		frag_nation_grenade_maximum(_unit_grenade_covenant_plasma));
	platform_log("%s: player %ld's kill (%s) gives %d frag, %d plasma: now %d frag, %d plasma",
		game_modes_mode() == _game_mode_snowball_fight ? "snowball fight" : "frag nation",
		(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(killing_player_index),
		kill == _frag_nation_kill_frag ? "frag" : kill == _frag_nation_kill_plasma ? "plasma" :
			kill == _frag_nation_kill_plasma_stuck ? "plasma stuck" : game_modes_killed_by_melee() ? "a blow" : "other",
		(int)frags, (int)plasmas, (int)unit->unit.grenade_counts[_unit_grenade_human_fragmentation],
		(int)unit->unit.grenade_counts[_unit_grenade_covenant_plasma]);
}

struct game_engine *frag_nation_engine_get(
	struct game_engine const *slayer)
{
	frag_nation.slayer = *slayer;
	frag_nation.engine = *slayer;
	frag_nation.engine.allow_pick_up = frag_nation_allow_pick_up;
	frag_nation.engine.player_killed_player = frag_nation_player_killed_player;
	return &frag_nation.engine;
}
