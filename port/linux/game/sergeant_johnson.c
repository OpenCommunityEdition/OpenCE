/*
SERGEANT_JOHNSON.C

Sergeant Johnson where the campaign's story has him (a10 and a50).

A squad of marines whose leader the game picks (unique leader "normal", as
every squad of a10 and a50 is) gets one once its encounter holds four
marines: Sergeant Johnson (the armored marine's variant 100) or Sergeant
Lehto (variant 101), at random (encounter_create, encounters.c), on
the Xbox too. So Johnson could lead the marines holding a10's last
lifepod bay as the Covenant break in, though he had left the Pillar of
Autumn's hangar long before, and a50's marines in the Truth and
Reconciliation's gravity lift room whether or not he had come that far,
while a50's opening, in which he speaks (the sergeant's lines), had no
Johnson.

- a10: a leader who would be Johnson is Lehto.
- a50: the last marine to step out of the Pelican (ini_marine_7) is
  Johnson. (The script starts the sergeant's last line as ini_marine_6
  steps out after him, but never creates ini_marine_6, nor ini_marine_3:
  the line comes as ini_marine_7 has stepped out.) Whether he is alive is
  taken when the marines are taken away for the gravity lift
  (ai_erase_all: the lift cinematic's, or the gravity room's when the
  cinematic is skipped) while he is there, and when he dies (unit_died;
  an erased actor's unit does not die). If he made it, the lift
  cinematic's first marine (lift_marine_1, whom its first shot is of) is
  Johnson, and so is the leader of the marines who hold the gravity lift
  room after it (gravity_pad_marines); if not, they are not, the room's
  leader is Lehto, and no other leader of a50 is Johnson.

Whether he made it is not game state (a saved game's layout is unchanged):
it is taken again at each ai_erase_all while he is there, and a revert or
a load to before his death, which brings him back, brings him back alive to
the next one. A variant is sent to the other machines with the object
(network_objects.c), so a network co-op game's clients see the host's.
*/

#include "cseries.h"
#include "tag_files/tag_groups.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "objects/objects.h"
#include "sergeant_johnson.h"

/* ---------- constants */

#define JOHNSON_VARIANT 100
#define LEHTO_VARIANT 101

/* ---------- globals */

/* a50's Johnson alive when last seen (the last ai_erase_all he was there
for, or his death) */
static boolean johnson_alive;

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
	char const *encounter_name,
	short variant,
	boolean alive)
{
	/* not a leader */
	if (variant != JOHNSON_VARIANT && variant != LEHTO_VARIANT)
		return variant;
	if (map == _sergeant_johnson_map_a50 &&
		encounter_name && !csstrcmp(encounter_name, "gravity_pad_marines"))
	{
		return alive ? JOHNSON_VARIANT : LEHTO_VARIANT;
	}
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
	short variant,
	boolean alive)
{
	if (map != _sergeant_johnson_map_a50 || !object_name || !definition_name ||
		csstrcmp(definition_name, "characters\\marine_armored\\marine_armored"))
	{
		return variant;
	}
	if (!csstrcmp(object_name, "ini_marine_7"))
		return JOHNSON_VARIANT;
	if (!csstrcmp(object_name, "lift_marine_1") && alive)
		return JOHNSON_VARIANT;
	return variant;
}

/* ---------- the game's */

static short sergeant_johnson_current_map(void)
{
	return global_scenario_index == NONE ?
		_sergeant_johnson_map_other :
		sergeant_johnson_map(tag_get_name(global_scenario_index));
}

/* the scenario's name index of an object name, or NONE */
static short sergeant_johnson_name_index(char const *name)
{
	struct scenario *scenario = global_scenario_get();
	long index;

	if (!scenario)
		return NONE;
	for (index = 0; index < scenario->object_names.count && index < SHORT_MAX; index++)
	{
		if (!csstrcmp(TAG_BLOCK_GET_ELEMENT(&scenario->object_names, index, struct scenario_object_name)->name, name))
			return (short)index;
	}
	return NONE;
}

void sergeant_johnson_reset(void)
{
	johnson_alive = FALSE;
}

short sergeant_johnson_leader_variant(char const *encounter_name, short variant)
{
	return sergeant_johnson_leader_variant_in(sergeant_johnson_current_map(), encounter_name, variant,
		johnson_alive);
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
		tag_get_name(definition_index), variant, johnson_alive);
}

void sergeant_johnson_erasing_all(void)
{
	long object_index;
	struct object_datum *object;

	if (sergeant_johnson_current_map() != _sergeant_johnson_map_a50)
		return;
	object_index = object_index_from_name_index(sergeant_johnson_name_index("ini_marine_7"));
	object = object_index == NONE ? NULL : object_try_and_get_and_verify_type(object_index, _object_mask_unit);
	if (object)
		johnson_alive = !TEST_FLAG(object->object.damage_flags, _object_dead_bit);
}

void sergeant_johnson_unit_died(long unit_index)
{
	struct object_datum *object = unit_index == NONE ? NULL :
		object_try_and_get_and_verify_type(unit_index, _object_mask_unit);

	if (object && object->object.variant_number == JOHNSON_VARIANT &&
		sergeant_johnson_current_map() == _sergeant_johnson_map_a50)
	{
		johnson_alive = FALSE;
	}
}
