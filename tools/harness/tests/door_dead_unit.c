/*
DOOR_DEAD_UNIT.C (test)

machine_update (device_machines.c) lets a door refuse a unit that is dead (bit 2 of its damage flags, a word at offset
0xb6). The original read the word's first byte: the low byte on the x86, the high byte on the Mac's PowerPC. The
condition is taken from the source; the big-endian run reads a byte at that offset as a big-endian machine would. The
negative control is the original's byte read.
*/

#include "harness.h"

typedef unsigned short word;
struct object_part { char pad[0xB6 - 4]; word damage_flags; };
struct unit_datum { long definition_index; struct object_part object; };
#include "under_test.inc"

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	struct unit_datum unit;

	memset(&unit, 0, sizeof(unit));

	CASE("dead-unit-refused")
	{
		unit.object.damage_flags = 1 << 2;
		CHECK(!unit_can_open(&unit), "a dead unit opened the door");
	}

	CASE("living-unit-opens")
	{
		/* bits of the high byte are set (shield over charging and the like) but not the dead bit */
		unit.object.damage_flags = (word)((1 << 9) | (1 << 12) | (1 << 3));
		CHECK(unit_can_open(&unit), "a living unit was refused");
		unit.object.damage_flags = 0;
		CHECK(unit_can_open(&unit), "a unit without flags was refused");
	}

	return 0;
}
