/*
SCENARIO_PLACEMENT.C

An object a map places as it loads that the port holds back
(object_new_from_scenario, objects.c): one only a cinematic uses, which the
cinematic creates itself.

a10 (levels\a10\a10) places Sergeant Johnson (the biped named johnson, an
armored marine) as it loads, in a hallway by the cryo bay, though only its
opening cinematic (x10) uses him: the cinematic creates him anew for the
hangar (hangar_2, object_create_anew) and deletes him after it
(hangar_2_cleanup). Every other biped a10's cinematics use is placed not
automatically. Skipping the cinematic goes back to a save made just before
it, so that a player who skipped it found him standing in the hallway,
doing nothing, on the Xbox too.

He is not placed as a local game of a10 loads; the cinematic still creates
him (object_create_anew places an object that is not automatic). In a
network game (co-op) the objects placed as a map loads must be alike on
every machine (NETCODE.md, object identity), and there he is placed as
before.
*/

#include "cseries.h"
#include "game/game.h"
#include "tag_files/tag_groups.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "scenario_placement.h"

/* whether a map's object, by its map, name and definition, is held back in a
game of the connection given */
boolean scenario_placement_withheld_by_name(
	short connection,
	char const *scenario_name,
	char const *object_name,
	char const *definition_name)
{
	return connection == _game_connection_local &&
		scenario_name && !csstrcmp(scenario_name, "levels\\a10\\a10") &&
		object_name && !csstrcmp(object_name, "johnson") &&
		definition_name && !csstrcmp(definition_name, "characters\\marine_armored\\marine_armored");
}

boolean scenario_placement_withheld(
	struct scenario_object_datum const *scenario_object,
	struct tag_block const *palette)
{
	struct scenario *scenario = global_scenario_get();
	long definition_index;

	if (!scenario_object || !palette || !scenario || global_scenario_index == NONE ||
		!VALID_INDEX(scenario_object->name_index, scenario->object_names.count) ||
		!VALID_INDEX(scenario_object->palette_entry_index, palette->count))
	{
		return FALSE;
	}
	definition_index = TAG_BLOCK_GET_ELEMENT(palette, scenario_object->palette_entry_index,
		struct scenario_object_palette_entry)->reference.index;
	if (definition_index == NONE)
		return FALSE;
	return scenario_placement_withheld_by_name(
		game_connection(),
		tag_get_name(global_scenario_index),
		TAG_BLOCK_GET_ELEMENT(&scenario->object_names, scenario_object->name_index, struct scenario_object_name)->name,
		tag_get_name(definition_index));
}
