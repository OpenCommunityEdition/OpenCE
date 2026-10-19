#include "harness.h"

#include "config.inc"

struct player_action { unsigned long control_flags; struct { real i, j; } throttle; };
struct unit_datum { struct { long parent_object_index; } object; };
static boolean rules_have_sprint;
static boolean network_game_unlimited_sprint(void) { return rules_have_sprint; }
#include "under_test.inc"

#define SPRINT FLAG(UNIT_CONTROL_PORT_SPRINT_BIT)

static real throttle_for(unsigned long flags, real forward, long parent, boolean rules)
{
	struct player_action action = { flags, { forward, 0.f } };
	struct unit_datum unit = { { parent } };

	rules_have_sprint = rules;
	return player_forward_throttle(&action, &unit);
}

int main(int argc, char **argv)
{
	CHECK(argc == 2, "a case");
	if (!strcmp(argv[1], "sprints"))
	{
		/* half as fast again: exactly, so that every machine has the same bits */
		CHECK(throttle_for(SPRINT, 1.f, NONE, TRUE) == 1.5f, "full stick");
		CHECK(throttle_for(SPRINT, 0.75f, NONE, TRUE) == 0.75f * 1.5f, "three quarters");
	}
	else if (!strcmp(argv[1], "rules_off"))
	{
		/* the game's rules without sprint (a client of a host without it, or the setting off): the action's */
		CHECK(throttle_for(SPRINT, 1.f, NONE, FALSE) == 1.f, "sprinted though the rules do not have it");
	}
	else if (!strcmp(argv[1], "not_held"))
	{
		CHECK(throttle_for(0, 1.f, NONE, TRUE) == 1.f, "sprinted without the control");
		CHECK(throttle_for(FLAG(_unit_control_jump_bit), 1.f, NONE, TRUE) == 1.f, "sprinted with another control");
	}
	else if (!strcmp(argv[1], "crouching"))
	{
		CHECK(throttle_for(SPRINT | FLAG(_unit_control_crouch_modifier_bit), 1.f, NONE, TRUE) == 1.f, "sprinted crouching");
	}
	else if (!strcmp(argv[1], "forward_only"))
	{
		CHECK(throttle_for(SPRINT, 0.5f, NONE, TRUE) == 0.5f, "sprinted on a light push");
		CHECK(throttle_for(SPRINT, 0.f, NONE, TRUE) == 0.f, "sprinted standing");
		CHECK(throttle_for(SPRINT, -1.f, NONE, TRUE) == -1.f, "sprinted backward");
	}
	else if (!strcmp(argv[1], "on_foot"))
	{
		CHECK(throttle_for(SPRINT, 1.f, 0x1234L, TRUE) == 1.f, "sprinted in a vehicle");
	}
	else
	{
		CHECK(0, "no case %s", argv[1]);
	}
	return 0;
}
