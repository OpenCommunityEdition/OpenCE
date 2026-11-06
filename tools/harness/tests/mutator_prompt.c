/*
MUTATOR_PROMPT.C (test)

The question about a host's script (port/linux/game/mutator_share.c's
client_prompt_update), answered with a controller or, on a touchscreen with
none, by a tap: on the left half yes, on the right no. Neither counts in its
first moments, when the player has not read it. test_mutator_prompt.py takes
the code (under_test.inc).
*/

#include "harness.h"

#define _error_log 0
#define HALO_PORT_MAXIMUM_NETWORK_MACHINES 16

static unsigned long now, buttons;
static int taps, allowed, next_files;
static float tap_x, tap_y;

static unsigned long system_milliseconds(void) { return now; }
static unsigned long halo_chat_pad_buttons(void) { return buttons; }
static int halo_chat_taps(float *x, float *y)
{
	int given = taps;

	*x = tap_x;
	*y = tap_y;
	taps = 0;
	return given;
}
#define error(level, ...) ((void)(level))

#include "under_test.inc"

static void port_mutators_share_script_allow(struct port_mutator_share const *entry) { (void)entry; allowed++; }
static void client_next_file(void) { next_files++; }

/* a question asked at 1000 milliseconds, and a tap at the time, where */
static int answer(unsigned long at, float x, int tapped)
{
	memset(&mutator_share, 0, sizeof(mutator_share));
	mutator_share.state = _share_prompting;
	mutator_share.prompt_at = 1000;
	now = at;
	buttons = 0;
	taps = tapped;
	tap_x = x;
	tap_y = 0.5f;
	client_prompt_update();
	return mutator_share.state;
}

int main(int argc, char **argv)
{
	char const *case_name = argv[1];

	CHECK(argc == 2, "case");
	if (!strcmp(case_name, "taps"))
	{
		CHECK(answer(4000, 0.25f, 1) == _share_asking && mutator_share.script_allowed[0] && allowed == 1 &&
			next_files == 1, "a tap on the left: yes");
		CHECK(answer(4000, 0.75f, 1) == _share_asking && !mutator_share.script_allowed[0] && allowed == 1 &&
			next_files == 2, "a tap on the right: no");
		CHECK(answer(4000, 0.25f, 0) == _share_prompting, "no tap, no answer");
		/* (one while it is read is no answer, and is taken: no answer later from it) */
		CHECK(answer(1200, 0.25f, 1) == _share_prompting && !taps, "a tap while it is read");
	}
	else if (!strcmp(case_name, "buttons"))
	{
		CHECK(answer(4000, 0.f, 0) == _share_prompting, "nothing pressed");
		memset(&mutator_share, 0, sizeof(mutator_share));
		mutator_share.state = _share_prompting;
		mutator_share.prompt_at = 1000;
		now = 4000;
		buttons = 1UL << HALO_CHAT_PAD_A;
		client_prompt_update();
		CHECK(mutator_share.state == _share_asking && mutator_share.script_allowed[0], "A: yes");
		CHECK(answer(1000 + 31000, 0.f, 0) == _share_asking && !mutator_share.script_allowed[0], "no after the wait");
	}
	else
	{
		CHECK(0, "no case %s", case_name);
	}
	return 0;
}
