/*
SERGEANT_JOHNSON.C

Sergeant Johnson where the campaign's story has him (a10 and a50).

A squad of marines whose leader the game picks (unique leader "normal", as
every squad of a10 and a50 is) gets one once its encounter holds four
marines: Sergeant Johnson (the armored marine's variant 100) or Sergeant
Lehto (variant 101), at random (encounter_create, encounters.c), on the
Xbox too. So Johnson could lead the marines holding a10's last lifepod bay
as the Covenant break in, though he had left the Pillar of Autumn's hangar
long before, and a50's marines in the Truth and Reconciliation's gravity
lift room, while a50's opening, in which he speaks (the sergeant's lines),
had no Johnson.

- a10 and a50: a leader who would be Johnson is Lehto: Johnson is not in
  a10's lifepod bay, nor in the Truth and Reconciliation.
- a50: the last marine to step out of the opening's Pelican (ini_marine_7)
  is Johnson. (The script starts the sergeant's last line as ini_marine_6
  steps out after him, but never creates ini_marine_6, nor ini_marine_3:
  the line comes as ini_marine_7 has stepped out.) The gravity lift
  cinematic's marines (lift_marine_*) are as the scenario places them, none
  of them Johnson.

A variant is sent to the other machines with the object (network_objects.c),
so a network co-op game's clients see the host's.
*/

#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "sergeant_johnson.h"

/* ---------- constants */

#define JOHNSON_VARIANT 100
#define LEHTO_VARIANT 101

/* ---------- decisions */

short sergeant_johnson_map(char const *scenario_name)
{
	if (scenario_name && !csstrcmp(scenario_name, "levels\\a10\\a10"))
		return _sergeant_johnson_map_a10;
	if (scenario_name && !csstrcmp(scenario_name, "levels\\a50\\a50"))
		return _sergeant_johnson_map_a50;
	return _sergeant_johnson_map_other;
}

short sergeant_johnson_leader_variant_in(
	short map,
	short variant)
{
	if ((map == _sergeant_johnson_map_a10 || map == _sergeant_johnson_map_a50) &&
		variant == JOHNSON_VARIANT)
	{
		return LEHTO_VARIANT;
	}
	return variant;
}

short sergeant_johnson_scenario_variant_in(
	short map,
	char const *object_name,
	char const *definition_name,
	short variant)
{
	if (map == _sergeant_johnson_map_a50 && object_name && definition_name &&
		!csstrcmp(definition_name, "characters\\marine_armored\\marine_armored") &&
		!csstrcmp(object_name, "ini_marine_7"))
	{
		return JOHNSON_VARIANT;
	}
	return variant;
}

/* ---------- the game's */

static short sergeant_johnson_current_map(void)
{
	return global_scenario_index == NONE ?
		_sergeant_johnson_map_other :
		sergeant_johnson_map(tag_get_name(global_scenario_index));
}

short sergeant_johnson_leader_variant(short variant)
{
	return sergeant_johnson_leader_variant_in(sergeant_johnson_current_map(), variant);
}

short sergeant_johnson_scenario_variant(
	struct scenario_object_datum const *scenario_object,
	long definition_index,
	short variant)
{
	struct scenario *scenario = global_scenario_get();
	short map = sergeant_johnson_current_map();

	if (map != _sergeant_johnson_map_a50 || !scenario_object || !scenario || definition_index == NONE ||
		!VALID_INDEX(scenario_object->name_index, scenario->object_names.count))
	{
		return variant;
	}
	return sergeant_johnson_scenario_variant_in(map,
		TAG_BLOCK_GET_ELEMENT(&scenario->object_names, scenario_object->name_index, struct scenario_object_name)->name,
		tag_get_name(definition_index), variant);
}
