/*
DEVICE_CONTROLS.C

symbols in this file:
00083D10 0010:
	_controls_initialize (0000)
00083D20 0010:
	_controls_dispose (0000)
00083D30 0010:
	_controls_initialize_for_new_map (0000)
00083D40 0010:
	_controls_dispose_from_old_map (0000)
00083D50 0070:
	_control_place (0000)
00083DC0 0030:
	_control_new (0000)
00083DF0 0010:
	_control_delete (0000)
00083E00 0030:
	_control_update (0000)
00083E30 0130:
	_code_00083e30 (0000)
00083F60 0040:
	_control_touched (0000)
00083FA0 0040:
	_control_destroyed (0000)
00259634 0029:
	??_C@_0CJ@LFHDKHDO@c?3?2halo?2SOURCE?2devices?2device_co@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "device_controls.h"
#include "scenario/scenario_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct scenario_control_datum
{
	struct scenario_object_datum object;
	struct scenario_device_datum device;
	word flags;
	short unused;
	short custom_name_index;
};

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void controls_initialize(
	void)
{
	return;
}

void controls_dispose(
	void)
{
	return;
}

void controls_initialize_for_new_map(
	void)
{
	return;
}

void controls_dispose_from_old_map(
	void)
{
	return;
}

boolean control_new(
	long control_index)
{
	struct control_datum *control = control_get(control_index);

	control_definition_get(control->definition_index);

	return TRUE;
}

void control_delete(
	long control_index)
{
	return;
}

boolean control_update(
	long control_index)
{
	struct control_datum *control = control_get(control_index);

	control_definition_get(control->definition_index);

	return TRUE;
}

void control_place(
	long control_index,
	struct scenario_control_datum *scenario_control)
{
	struct control_datum *control = control_get(control_index);

	control_definition_get(control->definition_index);
	device_add_scenario_information(control_index, &scenario_control->device);
	if (TEST_FLAG(scenario_control->flags, 0))
		control->control.flags |= FLAG(0);
	if (TEST_FLAG(scenario_control->flags, 4))
		control->control.flags |= FLAG(1);
	control->control.custom_name_index = scenario_control->custom_name_index - 1;

	return;
}

/* ---------- private code */
