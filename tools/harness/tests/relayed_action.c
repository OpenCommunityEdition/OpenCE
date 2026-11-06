/*
RELAYED_ACTION.C (test)

A relayed action's bytes on the wire (port/linux/game/network_distributed.c's
distributed_relayed_action_write and _read): an action with no script's
buttons set has no byte of them, as in every game without a mutator's script;
one with them set has it, flagged in its parts; each read back as it was
written, and one cut short refused. test_relayed_action.py takes the
constants (config.inc) and the code (under_test.inc).
*/

#include "harness.h"

typedef unsigned short word;
#define csmemcpy memcpy
#define csmemset memset
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))

#include "config.inc"
#include "under_test.inc"

static void action(struct distributed_relayed_action *relayed, byte buttons)
{
	short history;

	memset(relayed, 0, sizeof(*relayed));
	relayed->player_index = 3;
	relayed->desired_weapon_index = 1;
	relayed->yaw = 1234;
	relayed->pitch = -321;
	relayed->primary_trigger = 200;
	relayed->script_buttons = buttons;
	for (history = 0; history < DISTRIBUTED_INPUT_HISTORY; history++)
		relayed->control_flags[history] = (unsigned short)(history == 2 ? 0x10 : 0x01);
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];
	byte buffer[64];
	struct distributed_relayed_action written, read;
	word size;

	CHECK(argc == 2, "case");
	if (!strcmp(case_name, "none"))
	{
		action(&written, 0);
		size = distributed_relayed_action_write(&written, buffer);
		CHECK(size == DISTRIBUTED_RELAYED_ACTION_MINIMUM_SIZE + 2 * 2, "a byte of no buttons sent (%d)", size);
		CHECK(!TEST_FLAG(buffer[1], DISTRIBUTED_RELAYED_ACTION_SCRIPT_BUTTONS_BIT), "flagged");
		CHECK(distributed_relayed_action_read(buffer, buffer + size, &read) == size &&
			!memcmp(&read, &written, sizeof(read)), "not read back");
		CHECK(!distributed_relayed_action_read(buffer, buffer + size - 1, &read), "one cut short read");
	}
	else if (!strcmp(case_name, "buttons"))
	{
		action(&written, 0x05);
		size = distributed_relayed_action_write(&written, buffer);
		CHECK(size == DISTRIBUTED_RELAYED_ACTION_MINIMUM_SIZE + 1 + 2 * 2 &&
			size <= DISTRIBUTED_RELAYED_ACTION_MAXIMUM_SIZE, "the buttons' byte (%d)", size);
		CHECK(TEST_FLAG(buffer[1], DISTRIBUTED_RELAYED_ACTION_SCRIPT_BUTTONS_BIT), "not flagged");
		CHECK(distributed_relayed_action_read(buffer, buffer + size, &read) == size &&
			!memcmp(&read, &written, sizeof(read)) && read.script_buttons == 0x05, "not read back");
		CHECK(!distributed_relayed_action_read(buffer, buffer + size - 1, &read), "one cut short read");
		/* (flagged, and not there: refused) */
		action(&written, 0);
		size = distributed_relayed_action_write(&written, buffer);
		buffer[1] |= 1 << DISTRIBUTED_RELAYED_ACTION_SCRIPT_BUTTONS_BIT;
		CHECK(!distributed_relayed_action_read(buffer, buffer + size - 2 * 2, &read), "a missing byte read");
	}
	else
	{
		CHECK(0, "no case %s", case_name);
	}
	return 0;
}
