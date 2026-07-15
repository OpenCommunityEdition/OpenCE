/*
DEVICE_MACHINES.C

symbols in this file:
00084100 0010:
	_machines_initialize (0000)
00084110 0010:
	_machines_dispose (0000)
00084120 0010:
	_machines_initialize_for_new_map (0000)
00084130 0010:
	_machines_dispose_from_old_map (0000)
00084140 0080:
	_machine_place (0000)
000841C0 0080:
	_machine_new (0000)
00084240 0010:
	_machine_delete (0000)
00084250 0030:
	_machine_bumped (0000)
00084280 0050:
	_machine_try_to_open_with_damage (0000)
000842D0 0400:
	_machine_update (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "device_machines.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

void machines_initialize(
	void)
{
	return;
}

void machines_dispose(
	void)
{
	return;
}

void machines_initialize_for_new_map(
	void)
{
	return;
}

void machines_dispose_from_old_map(
	void)
{
	return;
}

void machine_place(
	long machine_index,
	struct scenario_machine_datum *scenario_machine)
{
	struct machine_datum *machine = machine_get(machine_index);

	device_add_scenario_information(machine_index, &scenario_machine->device);
	if (TEST_FLAG(scenario_machine->flags, _machine_does_not_operate_automatically_bit))
	{
		machine->machine.flags |= FLAG(_machine_does_not_operate_automatically_bit);
	}
	if (TEST_FLAG(scenario_machine->flags, _machine_one_sided_bit))
	{
		machine->machine.flags |= FLAG(_machine_one_sided_bit);
	}
	if (TEST_FLAG(scenario_machine->flags, _machine_never_appears_locked_bit))
	{
		machine->machine.flags |= FLAG(_machine_never_appears_locked_bit);
	}
	if (TEST_FLAG(scenario_machine->flags, _machine_opened_by_melee_attack_bit))
	{
		machine->machine.flags |= FLAG(_machine_opened_by_melee_attack_bit);
	}

	return;
}

void machine_delete(
	long machine_index)
{
	return;
}

void machine_bumped(
	long machine_index,
	long biped_index)
{
	struct machine_datum *machine = machine_get(machine_index);

	machine_definition_get(machine->definition_index);

	return;
}

/* ---------- private code */
