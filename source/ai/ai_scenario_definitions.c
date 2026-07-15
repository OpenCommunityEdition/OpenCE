/*
AI_SCENARIO_DEFINITIONS.C

symbols in this file:
00042490 0060:
	_scenario_get_encounter_by_name (0000)
000424F0 0060:
	_encounter_definition_get_squad_by_name (0000)
00042550 0060:
	_encounter_definition_get_platoon_by_name (0000)
000425B0 00e0:
	_choose_random_array_element (0000)
0024BDF8 0012:
	??_C@_0BC@MAOKEIKD@guard_at_position?$AA@ (0000)
0024BE0C 000c:
	??_C@_0M@CJJPMLKE@move_random?$AA@ (0000)
0024BE18 0011:
	??_C@_0BB@OJDOAOPK@move_loop_random?$AA@ (0000)
0024BE2C 0019:
	??_C@_0BJ@IJBEFDLI@move_loop_back_and_forth?$AA@ (0000)
0024BE48 000a:
	??_C@_09KGOMJPLJ@move_loop?$AA@ (0000)
0024BE54 000c:
	??_C@_0M@EEPMEDCG@move_repeat?$AA@ (0000)
002B7544 0030:
	_global_ai_default_state_names (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "ai_scenario_definitions.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

long scenario_get_encounter_by_name(
	struct scenario *scenario,
	char const *name)
{
	long encounter_index;
	long result;

	result = NONE;
	for (encounter_index = 0; encounter_index < scenario->ai_encounters.count; encounter_index++)
	{
		struct encounter_definition *encounter;

		encounter = TAG_BLOCK_GET_ELEMENT(
			&scenario->ai_encounters,
			encounter_index,
			struct encounter_definition);
		if (_strnicmp(encounter->name, name, TAG_STRING_LENGTH+1) == 0)
		{
			result = encounter_index;
			break;
		}
	}

	return result;
}

long encounter_definition_get_squad_by_name(
	struct encounter_definition *encounter,
	char const *name)
{
	long result;
	long squad_index;

	result = NONE;
	for (squad_index = 0; squad_index < encounter->squads.count; squad_index++)
	{
		struct squad_definition *squad;

		squad = TAG_BLOCK_GET_ELEMENT(
			&encounter->squads,
			squad_index,
			struct squad_definition);
		if (_strnicmp(squad->name, name, TAG_STRING_LENGTH+1) == 0)
		{
			result = squad_index;
			break;
		}
	}

	return result;
}

long encounter_definition_get_platoon_by_name(
	struct encounter_definition *encounter,
	char const *name)
{
	long platoon_index;
	long result;

	result = NONE;
	for (platoon_index = 0; platoon_index < encounter->platoons.count; platoon_index++)
	{
		struct platoon_definition *platoon;

		platoon = TAG_BLOCK_GET_ELEMENT(
			&encounter->platoons,
			platoon_index,
			struct platoon_definition);
		if (_strnicmp(platoon->name, name, TAG_STRING_LENGTH+1) == 0)
		{
			result = platoon_index;
			break;
		}
	}

	return result;
}

/* ---------- private code */
