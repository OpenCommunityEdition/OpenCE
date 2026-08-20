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
#include "game/game_globals.h"
#include "hs/object_lists.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct damage_globals
{
	byte unknown[0x48];
	long debug_object_index;
};

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
