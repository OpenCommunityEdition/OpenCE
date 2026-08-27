/*
HS_LIBRARY_EXTERNAL.C

symbols in this file:
000B8960 0010:
	_hs_not (0000)
000B8970 0020:
	_hs_print (0000)
000B8990 0060:
	_hs_players (0000)
000B89F0 00b0:
	_hs_trigger_volume_test_objects (0000)
000B8AA0 0070:
	_hs_unit_can_see_object (0000)
000B8B10 0080:
	_hs_objects_can_see_object (0000)
000B8B90 0050:
	_hs_unit_can_see_flag (0000)
000B8BE0 00a0:
	_hs_objects_can_see_flag (0000)
000B8C80 00b0:
	_code_000b8c80 (0000)
000B8D30 0050:
	_hs_object_create (0000)
000B8D80 0040:
	_hs_object_destroy (0000)
000B8DC0 0030:
	_hs_object_destroy_by_name (0000)
000B8DF0 00c0:
	_hs_object_destroy_all (0000)
000B8EB0 0080:
	_code_000b8eb0 (0000)
000B8F30 0020:
	_hs_object_create_containing (0000)
000B8F50 0020:
	_hs_object_destroy_containing (0000)
000B8F70 0040:
	_hs_object_list_get_element (0000)
000B8FB0 0070:
	_hs_object_set_shield (0000)
000B9020 00c0:
	_hs_object_set_permutation (0000)
000B90E0 0040:
	_hs_objects_predict (0000)
000B9120 0060:
	_hs_objects_delete_by_definition (0000)
000B9180 0070:
	_hs_effect_new (0000)
000B91F0 0070:
	_hs_effect_new_from_object_marker (0000)
000B9260 0070:
	_hs_damage_new (0000)
000B92D0 0060:
	_hs_damage_object (0000)
000B9330 0080:
	_code_000b9330 (0000)
000B93B0 0020:
	_hs_sound_get_gain (0000)
000B93D0 0020:
	_hs_sound_set_gain (0000)
000B93F0 00a0:
	_hs_trigger_volume_test_objects_all (0000)
000B9490 0020:
	_hs_trigger_volume_test_objects_any (0000)
000B94B0 0030:
	_hs_object_create_anew (0000)
000B94E0 0020:
	_hs_object_create_anew_containing (0000)
000B9500 0290:
	_code_000b9500 (0000)
000B9790 0020:
	_hs_object_teleport (0000)
000B97B0 0020:
	_hs_object_set_facing (0000)
000B97D0 0080:
	_hs_teleport_players_not_in_trigger_volume (0000)
0026E798 002d:
	??_C@_0CN@IIANAKMD@WARNING?3?5object_create?5?9?5?8?$CFs?8?5al@ (0000)
0026E7C8 005f:
	??_C@_0FP@BIAHJJOL@?$CD?$CD?$CD?5ERROR?5a?5script?5tried?5to?5dele@ (0000)
0026E828 0028:
	??_C@_0CI@GDKOMGNN@c?3?2halo?2SOURCE?2hs?2hs_library_ext@ (0000)
0026E850 001e:
	??_C@_0BO@NDHEDPHG@the?5sound?5?8?$CFs?8?5does?5not?5exist?$AA@ (0000)
0026E870 0010:
	??_C@_0BA@NMABAOHF@?$CGflag?9?$DOposition?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "objects/objects.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

long object_index_from_name_index(
	short name_index);
long object_list_get_first(
	long object_list_index,
	long *reference_index);
long object_list_get_next(
	long object_list_index,
	long *reference_index);
boolean hs_trigger_volume_test_objects(
	short trigger_volume_index,
	long object_list_index,
	boolean all);
void hs_object_destroy(
	long object_index);
void terminal_printf(
	union real_argb_color const *color,
	char const *format,
	...);

/* ---------- globals */

/* ---------- public code */

boolean hs_not(
	boolean value)
{
	return !value;
}

void hs_print(
	char const *message)
{
	terminal_printf(global_real_argb_green, message);

	return;
}

void hs_object_destroy_by_name(
	short object_name_index)
{
	if (object_name_index != NONE)
	{
		long object_index;

		object_index = object_index_from_name_index(object_name_index);
		if (object_index != NONE)
			hs_object_destroy(object_index);
	}

	return;
}

long hs_object_list_get_element(
	long object_list_index,
	short element_index)
{
	long reference_index;
	long object_index;

	object_index = object_list_get_first(object_list_index, &reference_index);
	while (element_index > 0 && object_index != NONE)
	{
		object_index = object_list_get_next(object_list_index, &reference_index);
		element_index--;
	}

	return object_index;
}

void hs_objects_predict(
	long object_list_index)
{
	long reference_index;
	long object_index;

	object_index = object_list_get_first(object_list_index, &reference_index);
	while (object_index != NONE)
	{
		object_predict(object_index);
		object_index = object_list_get_next(object_list_index, &reference_index);
	}

	return;
}

void hs_objects_delete_by_definition(
	long definition_index)
{
	struct object_iterator iterator;
	struct object_datum *object;

	object_iterator_new(&iterator, _object_mask_all, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		if (object->definition_index == definition_index)
			object_delete(iterator.index);
	}
	objects_memory_compact();

	return;
}

boolean hs_trigger_volume_test_objects_any(
	short trigger_volume_index,
	long object_list_index)
{
	return hs_trigger_volume_test_objects(
		trigger_volume_index,
		object_list_index,
		FALSE);
}

/* ---------- private code */
