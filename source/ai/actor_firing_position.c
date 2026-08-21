/*
ACTOR_FIRING_POSITION.C

symbols in this file:
00012610 0050:
	_code_00012610 (0000)
00012660 0010:
	_code_00012660 (0000)
00012670 00d0:
	_code_00012670 (0000)
00012740 0240:
	_code_00012740 (0000)
00012980 00e0:
	_code_00012980 (0000)
00012A60 0180:
	_code_00012a60 (0000)
00012BE0 00e0:
	_code_00012be0 (0000)
00012CC0 00c0:
	_code_00012cc0 (0000)
00012D80 00e0:
	_code_00012d80 (0000)
00012E60 0040:
	_code_00012e60 (0000)
00012EA0 0070:
	_code_00012ea0 (0000)
00012F10 0050:
	_code_00012f10 (0000)
00012F60 0110:
	_code_00012f60 (0000)
00013070 0120:
	_actor_get_firing_position_group (0000)
00013190 0060:
	_actor_clear_discarded_firing_positions (0000)
000131F0 00c0:
	_actor_discard_firing_position (0000)
000132B0 0050:
	_actor_firing_position_discarded (0000)
00013300 0650:
	_code_00013300 (0000)
00013950 01d0:
	_code_00013950 (0000)
00013B20 0290:
	_code_00013b20 (0000)
00013DB0 01d0:
	_code_00013db0 (0000)
00013F80 0090:
	_code_00013f80 (0000)
00014010 0210:
	_actor_nearby_firing_positions (0000)
00014220 1480:
	_actor_select_firing_position (0000)
000156A0 0240:
	_actor_active_select_firing_position (0000)
000158E0 0140:
	_actor_change_firing_position (0000)
00015A20 0460:
	_code_00015a20 (0000)
00244794 0068:
	_global_pre_evaluator_table (0000)
	_global_post_evaluator_table (0038)
002447FC 002e:
	??_C@_0CO@CPIODGNI@?$CIevaluation?5?$DO?$DN?50?40f?$CJ?5?$CG?$CG?5?$CIevaluat@ (0000)
0024482C 002a:
	??_C@_0CK@HINGLJIA@c?3?2halo?2SOURCE?2ai?2actor_firing_p@ (0000)
00244858 0004:
	__real@447a0000 (0000)
0024485C 0004:
	__real@41080000 (0000)
00244860 0004:
	__real@41700000 (0000)
00244864 0004:
	__real@40a00000 (0000)
00244868 0004:
	__real@41400000 (0000)
0024486C 0004:
	__real@40f00000 (0000)
00244870 0004:
	__real@41a00000 (0000)
00244874 0020:
	??_C@_0CA@KIJFLGCC@firing_position?9?$DOvalid?5?$DN?$DN?5valid?$AA@ (0000)
00244898 0048:
	??_C@_0EI@IDDMBEDB@?$CIindex2?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIindex2?5?$DM?5globa@ (0000)
002448E0 0048:
	??_C@_0EI@BDJPJEGE@?$CIindex1?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIindex1?5?$DM?5globa@ (0000)
00244928 002c:
	??_C@_0CM@BJJOKNGI@global_temporary_sort_firing_pos@ (0000)
00244954 003b:
	??_C@_0DL@JAGLMMAN@?$CIindex?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIindex?5?$DM?5NUMBER_@ (0000)
00244990 0004:
	__real@42dfec4b (0000)
00244994 0004:
	__real@c1700000 (0000)
00244998 0004:
	__real@42800000 (0000)
0024499C 0004:
	__real@3be38e39 (0000)
002449A0 0004:
	__real@44610000 (0000)
002449A4 0004:
	__real@40200000 (0000)
002449A8 0060:
	??_C@_0GA@KIPPIIL@?$CIactor?9?$DOdanger_zone?4danger_type?5@ (0000)
00244A08 0004:
	__real@3fb504f3 (0000)
00244A0C 0004:
	__real@40555555 (0000)
00244A10 0004:
	__real@42440000 (0000)
00244A14 0004:
	__real@41800000 (0000)
00244A18 001f:
	??_C@_0BP@KIOMHHMP@evaluation_context?9?$DOhas_target?$AA@ (0000)
00244A38 0052:
	??_C@_0FC@JPOFKNDC@?$CItest_surface_index?5?$DO?$DN?50?$CJ?5?$CG?$CG?5?$CIte@ (0000)
00244A90 005b:
	??_C@_0FL@LHFKPKFO@fp?9eval?5?$CFs?3?5encounter?5?$CF3d?5consid@ (0000)
00244AEC 0007:
	??_C@_06GFDMOFFM@pursue?$AA@ (0000)
00244AF4 0006:
	??_C@_05MGHOCOML@cover?$AA@ (0000)
00244AFC 0015:
	??_C@_0BF@JGHAJKFH@?$CBexpected_to_discard?$AA@ (0000)
00244B14 001a:
	??_C@_0BK@FJCPIFLD@firing_position_count?5?$DO?50?$AA@ (0000)
00244B30 0004:
	__real@43c80000 (0000)
00244B34 0035:
	??_C@_0DF@NLMOLOFE@encounter?5?$CFs?5has?5too?5many?5firing@ (0000)
00244B6C 0016:
	??_C@_0BG@HHMJFCBB@area_path_state_valid?$AA@ (0000)
00244B84 0004:
	__real@42a00000 (0000)
00244B88 0020:
	??_C@_0CA@EEMPDNKJ@?$CBactor?9?$DOinput?4vehicle_passenger?$AA@ (0000)
00244BA8 0046:
	??_C@_0EG@KOFLIKBB@?$CItotal_groups?5?$CG?5currently_allowe@ (0000)
00244BF0 001e:
	??_C@_0BO@EAEPOCDN@actor_index?5?$CB?$DN?5previous_owner?$AA@ (0000)
00244C10 0024:
	??_C@_0CE@DHIELLBI@actor?9?$DOmeta?4encounter_index?5?$CB?$DN?5N@ (0000)
00244C34 0004:
	__real@41440000 (0000)
00244C38 0039:
	??_C@_0DJ@OIJCIDAH@?$CGevaluation_context?9?$DOattack_vect@ (0000)
00319CA0 0008:
	_global_temporary_sort_firing_position_count (0000)
	_global_temporary_sort_firing_position_array (0004)
*/

/* ---------- headers */

#include "cseries.h"

#include "actors.h"
#include "ai_scenario_definitions.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

enum
{
	_firing_point_evaluation_mode_fight = 0,
	_firing_point_evaluation_mode_panic,
	_firing_point_evaluation_mode_cover,
	_firing_point_evaluation_mode_uncover,
	_firing_point_evaluation_mode_guard,
	_firing_point_evaluation_mode_pursue,
	_firing_point_evaluation_mode_avoid,
	NUMBER_OF_FIRING_POINT_EVALUATION_MODES,
};

enum
{
	_firing_position_group_normal = 0,
	_firing_position_group_when_searching,
	_firing_position_group_when_not_searching,
};

/* ---------- macros */

/* ---------- structures */

struct firing_position_definition
{
	real_point3d position;
	byte unresolved[2];
	short cluster_index;
	byte unresolved2[4];
	long surface_index;
};

typedef char actor_firing_position_definition_size_assert[
	sizeof(struct firing_position_definition) == 0x18 ? 1 : -1];
typedef char actor_firing_position_scenario_encounters_offset_assert[
	offsetof(struct scenario, ai_encounters) == 0x42C ? 1 : -1];
typedef char actor_firing_position_encounter_squads_offset_assert[
	offsetof(struct encounter_definition, squads) == 0x80 ? 1 : -1];
typedef char actor_firing_position_encounter_firing_positions_offset_assert[
	offsetof(struct encounter_definition, firing_positions) == 0x98 ? 1 : -1];
typedef char actor_firing_position_encounter_size_assert[
	sizeof(struct encounter_definition) == 0xB0 ? 1 : -1];
typedef char actor_firing_position_squad_groups_offset_assert[
	offsetof(struct squad_definition, firing_position_groups) == 0x54 ? 1 : -1];
typedef char actor_firing_position_squad_size_assert[
	sizeof(struct squad_definition) == 0xE8 ? 1 : -1];
typedef char actor_firing_position_actor_encounter_offset_assert[
	offsetof(struct actor_datum, meta.encounter_index) == 0x34 ? 1 : -1];
typedef char actor_firing_position_actor_squad_offset_assert[
	offsetof(struct actor_datum, meta.squad_index) == 0x3A ? 1 : -1];
typedef char actor_firing_position_actor_searching_offset_assert[
	offsetof(struct actor_datum, state.searching) == 0x98 ? 1 : -1];
typedef char actor_firing_position_actor_defending_offset_assert[
	offsetof(struct actor_datum, emotions.currently_defending) == 0x374 ? 1 : -1];
typedef char actor_discarded_firing_position_size_assert[
	sizeof(struct actor_discarded_firing_position) == 4 ? 1 : -1];
typedef char actor_firing_position_discard_cursor_offset_assert[
	offsetof(struct actor_datum, firing_positions.next_discarded_firing_positions_entry) == 0x3C6 ? 1 : -1];
typedef char actor_firing_position_discard_ring_offset_assert[
	offsetof(struct actor_datum, firing_positions.discarded_firing_positions) == 0x3C8 ? 1 : -1];
typedef char actor_firing_position_discard_index_offset_assert[
	offsetof(struct actor_datum, firing_positions.discarded_firing_positions) +
		offsetof(struct actor_discarded_firing_position, index) == 0x3CA ? 1 : -1];
typedef char actor_firing_position_last_discard_valid_offset_assert[
	offsetof(struct actor_datum, firing_positions.last_discarded_firing_position_valid) == 0x3D8 ? 1 : -1];
typedef char actor_firing_position_last_discard_temporary_offset_assert[
	offsetof(struct actor_datum, firing_positions.last_discarded_firing_position_temporary) == 0x3D9 ? 1 : -1];
typedef char actor_firing_position_last_discard_point_offset_assert[
	offsetof(struct actor_datum, firing_positions.last_discarded_firing_position) == 0x3DC ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

long actor_get_firing_position_group(
	long actor_index,
	short evaluation_mode,
	short group_selection_mode)
{
	struct actor_datum *actor = actor_get(actor_index);
	long result = 0;

	if (actor->meta.encounter_index != NONE)
	{
		struct encounter_definition *encounter = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters,
			DATUM_INDEX_TO_ABSOLUTE_INDEX(actor->meta.encounter_index),
			struct encounter_definition);
		struct squad_definition *squad = TAG_BLOCK_GET_ELEMENT(
			&encounter->squads,
			actor->meta.squad_index,
			struct squad_definition);
		boolean searching = actor->state.searching;
		short index;

		switch (group_selection_mode)
		{
		case _firing_position_group_when_searching:
			searching = TRUE;
			break;
		case _firing_position_group_when_not_searching:
			searching = FALSE;
			break;
		}

		if (evaluation_mode == _firing_point_evaluation_mode_panic)
			index = _firing_position_group_defending_guard;
		else if (evaluation_mode == _firing_point_evaluation_mode_guard)
			index = actor->emotions.currently_defending
				? _firing_position_group_defending_guard
				: _firing_position_group_attacking_guard;
		else if (evaluation_mode == _firing_point_evaluation_mode_pursue)
			index = _firing_position_group_pursuing;
		else if (actor->emotions.currently_defending)
			index = searching
				? _firing_position_group_defending_search
				: _firing_position_group_defending;
		else
			index = searching
				? _firing_position_group_attacking_search
				: _firing_position_group_attacking;

		match_assert(
			"c:\\halo\\SOURCE\\ai\\actor_firing_position.c",
			1412,
			(index >= 0) && (index < NUMBER_OF_FIRING_POSITION_GROUPS));
		result = squad->firing_position_groups[index];
	}

	return result;
}

void actor_clear_discarded_firing_positions(
	long actor_index,
	boolean clear_temporary_only)
{
	struct actor_datum *actor = actor_get(actor_index);
	long index;

	actor->firing_positions.next_discarded_firing_positions_entry = 0;
	for (index = 0; index < NUMBER_OF_DISCARDED_FIRING_POSITIONS_PER_ACTOR; index++)
		actor->firing_positions.discarded_firing_positions[index].index = NONE;

	if (actor->firing_positions.last_discarded_firing_position_valid &&
		(!clear_temporary_only || actor->firing_positions.last_discarded_firing_position_temporary))
	{
		actor->firing_positions.last_discarded_firing_position_valid = FALSE;
	}

	return;
}

void actor_discard_firing_position(
	long actor_index,
	short firing_position_index,
	boolean temporary)
{
	if (firing_position_index != NONE)
	{
		struct actor_datum *actor = actor_get(actor_index);
		struct encounter_definition *encounter;
		struct firing_position_definition *firing_position;

		actor->firing_positions.discarded_firing_positions[
			actor->firing_positions.next_discarded_firing_positions_entry].temporary = temporary;
		actor->firing_positions.discarded_firing_positions[
			actor->firing_positions.next_discarded_firing_positions_entry].index = firing_position_index;
		actor->firing_positions.next_discarded_firing_positions_entry =
			(actor->firing_positions.next_discarded_firing_positions_entry + 1) %
			NUMBER_OF_DISCARDED_FIRING_POSITIONS_PER_ACTOR;

		encounter = TAG_BLOCK_GET_ELEMENT(
			&global_scenario_get()->ai_encounters,
			DATUM_INDEX_TO_ABSOLUTE_INDEX(actor->meta.encounter_index),
			struct encounter_definition);
		firing_position = TAG_BLOCK_GET_ELEMENT(
			&encounter->firing_positions,
			firing_position_index,
			struct firing_position_definition);

		actor->firing_positions.last_discarded_firing_position_temporary = temporary;
		actor->firing_positions.last_discarded_firing_position_valid = TRUE;
		actor->firing_positions.last_discarded_firing_position = firing_position->position;
	}

	return;
}

boolean actor_firing_position_discarded(
	long actor_index,
	short firing_position_index)
{
	struct actor_datum *actor = actor_get(actor_index);
	boolean result = FALSE;

	if (firing_position_index != NONE)
	{
		short index;

		for (index = 0; index < NUMBER_OF_DISCARDED_FIRING_POSITIONS_PER_ACTOR; index++)
		{
			if (firing_position_index == actor->firing_positions.discarded_firing_positions[index].index)
			{
				result = TRUE;
				break;
			}
		}
	}

	return result;
}

/* ---------- private code */
