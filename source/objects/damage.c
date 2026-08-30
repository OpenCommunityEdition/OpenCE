/*
DAMAGE.C

symbols in this file:
00125DD0 0010:
	_damage_initialize (0000)
00125DE0 0010:
	_damage_dispose (0000)
00125DF0 0010:
	_damage_initialize_for_new_map (0000)
00125E00 0010:
	_damage_dispose_from_old_map (0000)
00125E10 0010:
	_damage_render_debug (0000)
00125E20 00e0:
	_object_initialize_vitality (0000)
00125F00 0050:
	_object_get_maximum_body_vitality (0000)
00125F50 0050:
	_object_get_maximum_shield_vitality (0000)
00125FA0 0040:
	_damage_data_new (0000)
00125FE0 0050:
	_object_restore_body (0000)
00126030 0060:
	_object_double_charge_shield (0000)
00126090 0050:
	_code_00126090 (0000)
001260E0 0050:
	_code_001260e0 (0000)
00126130 0050:
	_object_can_take_damage (0000)
00126180 0050:
	_object_cannot_take_damage (0000)
001261D0 0030:
	_object_set_ranged_attack_inhibited (0000)
00126200 0030:
	_object_set_melee_attack_inhibited (0000)
00126230 0020:
	_code_00126230 (0000)
00126250 0080:
	_code_00126250 (0000)
001262D0 0060:
	_object_get_actual_body_vitality (0000)
00126330 0060:
	_object_get_actual_shield_vitality (0000)
00126390 0080:
	_object_deplete_shield (0000)
00126410 0380:
	_code_00126410 (0000)
00126790 0230:
	_code_00126790 (0000)
001269C0 0200:
	_code_001269c0 (0000)
00126BC0 01d0:
	_render_debug_object_damage (0000)
00126D90 00e0:
	_object_deplete_body (0000)
00126E70 0070:
	_object_destroy (0000)
00126EE0 0140:
	_code_00126ee0 (0000)
00127020 0550:
	_code_00127020 (0000)
00127570 07c0:
	_object_cause_damage (0000)
00127D30 0420:
	_object_damage_update (0000)
00128150 0530:
	_code_00128150 (0000)
00128680 0080:
	_area_of_effect_cause_damage (0000)
00289410 006f:
	??_C@_0GP@MMLJIPGP@damage_resistance?9?$DOshield_materi@ (0000)
00289480 0020:
	??_C@_0CA@CANDACNM@c?3?2halo?2SOURCE?2objects?2damage?4c?$AA@ (0000)
002894A0 0029:
	??_C@_0CJ@DEGDFOIJ@collision?4type?$DN?$DN_collision_resul@ (0000)
002894D0 0061:
	??_C@_0GB@CBCJEGGG@?$CFs?$HMnbody?5?$CF0?43f?$HMn?5?5current?5?$CF0?43f?$HM@ (0000)
00289534 002c:
	??_C@_0CM@MBIMGLAN@no?5object?5to?5debug?$HMn?$CIpoint?5and?5p@ (0000)
00289560 0009:
	??_C@_08KBLDDAKN@?$HOdamaged?$AA@ (0000)
0028956C 003b:
	??_C@_0DL@NODEPHOL@region_index?$DO?$DN0?5?$CG?$CG?5region_index?$DM@ (0000)
002895A8 004b:
	??_C@_0EL@KFMBAJDI@damage_material?9?$DOtype?$DO?$DN0?5?$CG?$CG?5dama@ (0000)
002895F4 0028:
	??_C@_0CI@HMLKKOKN@?$CFs?3?5?$CC?$CFs?$CC?5?$CC?$CFs?$CC?5k?$DN?$CF0?42f?5S?$FL?$CF3?42f?$FN?5B@ (0000)
00289620 0041:
	??_C@_0EB@BEIFMKJC@damaged_object_count?$DMsizeof?$CIdama@ (0000)
00289668 0053:
	??_C@_0FD@MEFHNJOJ@region_index?$DN?$DNNONE?5?$HM?$HM?5?$CIregion_in@ (0000)
002896BC 0004:
	__real@3a422e45 (0000)
00456E48 004c:
	_bss_00456e48 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "damage.h"
#include "effects/effects.h"
#include "game/cheats.h"
#include "game/game_globals.h"
#include "hs/object_lists.h"
#include "object_definitions.h"
#include "object_types.h"
#include "physics/collision_model_definitions.h"
#include "units/units.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct damage_globals
{
	byte unknown[0x48];
	long debug_object_index;
};

typedef char object_deplete_body_definition_index_offset_assert[
	offsetof(struct object_datum, definition_index) == 0x00 ? 1 : -1];
typedef char object_deplete_body_type_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, type) == 0x64 ? 1 : -1];
typedef char object_deplete_body_damage_flags_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, damage_flags) == 0xB6 ? 1 : -1];
typedef char object_deplete_body_next_object_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, next_object_index) == 0xC4 ? 1 : -1];
typedef char object_deplete_body_first_child_offset_assert[
	offsetof(struct object_datum, object) + offsetof(struct _object_datum, first_child_object_index) == 0xC8 ? 1 : -1];
typedef char object_deplete_body_player_index_offset_assert[
	offsetof(struct unit_datum, unit) + offsetof(struct _unit_datum, player_index) == 0x1C8 ? 1 : -1];
typedef char object_deplete_body_parent_seat_offset_assert[
	offsetof(struct unit_datum, unit) + offsetof(struct _unit_datum, parent_seat_index) == 0x2A0 ? 1 : -1];
typedef char object_deplete_body_collision_model_offset_assert[
	offsetof(struct object_definition, object) + offsetof(struct _object_definition, collision_model) + offsetof(struct tag_reference, index) == 0x7C ? 1 : -1];
typedef char object_deplete_body_effect_offset_assert[
	offsetof(struct collision_model, resistance) + offsetof(struct damage_resistance, body_depleted_effect) + offsetof(struct tag_reference, index) == 0xB4 ? 1 : -1];
typedef char object_destroy_effect_offset_assert[
	offsetof(struct collision_model, resistance) + offsetof(struct damage_resistance, body_destroyed_effect) + offsetof(struct tag_reference, index) == 0xC8 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct damage_globals bss_00456e48;

/* ---------- public code */

void damage_initialize(void)
{
	return;
}

void damage_dispose(void)
{
	return;
}

void damage_initialize_for_new_map(void)
{
	bss_00456e48.debug_object_index = NONE;
	return;
}

void damage_dispose_from_old_map(void)
{
	return;
}

void damage_render_debug(void)
{
	return;
}

void object_initialize_vitality(
	long object_index,
	real *custom_body_vitality,
	real *custom_shield_vitality)
{
	struct object_datum *object = object_get(object_index);
	struct object_definition *definition = object_definition_get(object->definition_index);
	real maximum_body_vitality = 0.f;
	real maximum_shield_vitality = 0.f;

	if (definition->object.collision_model.index != NONE)
	{
		struct collision_model *collision_model =
			collision_model_definition_get(definition->object.collision_model.index);

		if (collision_model)
		{
			maximum_body_vitality = collision_model->resistance.maximum_body_vitality;
			maximum_shield_vitality = collision_model->resistance.maximum_shield_vitality;
		}
	}

	if (custom_body_vitality)
		maximum_body_vitality = *custom_body_vitality;

	if (custom_shield_vitality)
		maximum_shield_vitality = *custom_shield_vitality;

	object->object.maximum_body_vitality = maximum_body_vitality;
	object->object.maximum_shield_vitality = maximum_shield_vitality;
	object->object.body_vitality = maximum_body_vitality > 0.f ? 1.f : 0.f;
	object->object.shield_vitality = maximum_shield_vitality > 0.f ? 1.f : 0.f;
	return;
}

real object_get_maximum_body_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	struct object_datum *object = object_get(object_index);
	real result = object->object.maximum_body_vitality;

	if (!ignore_difficulty)
		result *= game_difficulty_get_team_value(1, object->object.owner_team_index);

	return result;
}

real object_get_maximum_shield_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	struct object_datum *object = object_get(object_index);
	real result = object->object.maximum_shield_vitality;

	if (!ignore_difficulty)
		result *= game_difficulty_get_team_value(2, object->object.owner_team_index);

	return result;
}

real object_get_actual_body_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	real body_vitality = object_get(object_index)->object.body_vitality;
	real result = object_get_maximum_body_vitality(object_index, ignore_difficulty);

	result *= body_vitality;

	return result;
}

real object_get_actual_shield_vitality(
	long object_index,
	boolean ignore_difficulty)
{
	real shield_vitality = object_get(object_index)->object.shield_vitality;
	real result = object_get_maximum_shield_vitality(object_index, ignore_difficulty);

	result *= shield_vitality;

	return result;
}

void damage_data_new(
	struct damage_data *damage_data,
	long definition_index)
{
	csmemset(damage_data, 0, sizeof(*damage_data));
	damage_data->definition_index = definition_index;
	damage_data->material_type = NONE;
	damage_data->owner_player_index = NONE;
	damage_data->owner_object_index = NONE;
	damage_data->owner_team_index = NONE;
	damage_data->location.cluster_index = NONE;
	damage_data->scale = 1.f;
	damage_data->multiplier = 1.f;
	return;
}

boolean object_restore_body(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	boolean restored = FALSE;

	if (!TEST_FLAG(object->object.damage_flags, _object_dead_bit) &&
		object->object.body_vitality < 1.f)
	{
		object->object.body_vitality = 1.f;
		restored = TRUE;
	}

	return restored;
}

void object_deplete_body(
	long object_index)
{
	struct object_datum *object;
	word damage_flags;

	object = object_get(object_index);
	damage_flags = object->object.damage_flags;
	if (!TEST_FLAG(damage_flags, _object_dead_bit))
	{
		struct object_definition *definition;
		long collision_model_index;

		SET_FLAG(damage_flags, _object_dead_bit, TRUE);
		object->object.damage_flags = damage_flags;

		definition = object_definition_get(object->definition_index);
		collision_model_index = definition->object.collision_model.index;
		if (collision_model_index != NONE)
		{
			struct collision_model *collision_model;

			collision_model = collision_model_definition_get(collision_model_index);
			effect_new_from_object(
				collision_model->resistance.body_depleted_effect.index,
				object_index,
				object_index,
				NONE,
				0.f,
				0.f,
				NULL,
				NULL);
		}

		if (object->object.type == _object_type_vehicle)
		{
			long unit_index;

			unit_index = object->object.first_child_object_index;
			while (unit_index != NONE)
			{
				struct unit_datum *unit;

				unit = (struct unit_datum *)object_get(unit_index);
				if (unit->object.type == _object_type_biped &&
					(unit->unit.player_index == NONE || !cheat.deathless_player) &&
					unit->unit.parent_seat_index != NONE)
				{
					unit_kill(unit_index);
				}

				unit_index = unit->object.next_object_index;
			}
		}

		object_deplete_shield(object_index);
	}

	return;
}

void code_00126090(
	long object_index)
{
	struct object_datum *object = object_get(object_index);
	long child_object_index = object->object.first_child_object_index;

	while (child_object_index != NONE)
	{
		long next_object_index = object_get(child_object_index)->object.next_object_index;

		if (!object_type_handle_parent_destroyed(child_object_index))
			code_00126090(child_object_index);

		child_object_index = next_object_index;
	}

	return;
}

void object_destroy(
	long object_index)
{
	struct object_datum *object;
	struct object_definition *definition;
	long collision_model_index;

	object = object_get(object_index);
	definition = object_definition_get(object->definition_index);

	object_deplete_body(object_index);

	collision_model_index = definition->object.collision_model.index;
	if (collision_model_index != NONE)
	{
		struct collision_model *collision_model;

		collision_model = collision_model_definition_get(collision_model_index);
		effect_new_from_object(
			collision_model->resistance.body_destroyed_effect.index,
			object_index,
			object_index,
			NONE,
			0.f,
			0.f,
			NULL,
			NULL);
	}

	code_00126090(object_index);
	object_delete(object_index);

	return;
}

void object_can_take_damage(
	long object_list_index)
{
	long reference_index;
	long object_index = object_list_get_first(object_list_index, &reference_index);

	while (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_cannot_take_damage_bit, FALSE);
		object_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void object_cannot_take_damage(
	long object_list_index)
{
	long reference_index;
	long object_index = object_list_get_first(object_list_index, &reference_index);

	while (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_cannot_take_damage_bit, TRUE);
		object_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void object_set_ranged_attack_inhibited(
	long object_index,
	boolean inhibited)
{
	if (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_ranged_attack_inhibited_bit, inhibited);
	}

	return;
}

void object_set_melee_attack_inhibited(
	long object_index,
	boolean inhibited)
{
	if (object_index != NONE)
	{
		struct object_datum *object = object_get(object_index);
		SET_FLAG(object->object.damage_flags, _object_melee_attack_inhibited_bit, inhibited);
	}

	return;
}

/* ---------- private code */
