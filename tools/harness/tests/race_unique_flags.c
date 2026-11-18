/*
RACE_UNIQUE_FLAGS.C (test)

The real race_flags_make_unique and new_rally_flag (game_engine_race.c) over a
scenario's netgame flags: a race track's flag whose number another flag has is
given a free number of its own, which no later flag then has too, so every
flag of the track has a number of its own, and flag rally can choose each of
them. The original marked the flag's own number, used already, rather than
the one it gave, and a later flag of the number given (the next of a track
numbered on, a third of one number) shared it: a lap had fewer flags than the
map, and flag rally could never choose some of them. test_race_unique_flags.py
takes the enums, structure, constant and code from the sources (config.inc,
under_test.inc).
*/

#include "harness.h"

#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)))

#include "config.inc"

/* the scenario: its netgame flags */
#define MAXIMUM_FLAGS 40
struct tag_block { long count; void *address; };
struct scenario { struct tag_block netgame_flags; };
#define TAG_BLOCK_GET_ELEMENT(block, element, type) (&((type *)(block)->address)[(long)(element)])
static struct scenario_netgame_flag flags[MAXIMUM_FLAGS];
static struct scenario scenario = { { 0, flags } };
static struct scenario *global_scenario_get(void) { return &scenario; }

/* the race: the flags of a lap (as race_engine_initialize_for_new_map
sets them) */
static struct { unsigned long lap_completed_value; } race_globals;

/* the random numbers: the one the test chooses */
static long chosen_random;
static long random_range(long lower, long upper)
{
	CHECK(chosen_random >= lower && chosen_random < upper, "a random number %ld outside %ld to %ld", chosen_random,
		lower, upper);
	return chosen_random;
}

#include "under_test.inc"

/* a scenario of race track flags of these numbers (a negative one a CTF
flag of team 0 or 1 between them) */
static void reset(short const *numbers, long count)
{
	long index;

	memset(flags, 0, sizeof(flags));
	for (index = 0; index < count; index++)
	{
		flags[index].type = numbers[index] < 0 ? _netgame_flag_ctf_flag : _netgame_flag_race_track;
		flags[index].team_index = numbers[index] < 0 ? (short)(-numbers[index] - 1) : numbers[index];
	}
	scenario.netgame_flags.count = count;
}

/* the track's numbers each one flag's, and the lap's flags as many */
static void check_unique(long track_count)
{
	unsigned long seen = 0;
	long index;

	race_globals.lap_completed_value = 0;
	for (index = 0; index < scenario.netgame_flags.count; index++)
	{
		if (flags[index].type != _netgame_flag_race_track)
			continue;
		CHECK(flags[index].team_index >= 0 && flags[index].team_index < MAXIMUM_RACE_FLAGS,
			"flag %ld numbered %d", index, flags[index].team_index);
		CHECK(!TEST_FLAG(seen, flags[index].team_index), "flag %ld's number %d is another flag's too", index,
			flags[index].team_index);
		SET_FLAG(seen, flags[index].team_index, TRUE);
		SET_FLAG(race_globals.lap_completed_value, flags[index].team_index, TRUE);
	}
	CHECK(count_bits_32(race_globals.lap_completed_value) == track_count, "a lap of %ld flags, not %ld",
		count_bits_32(race_globals.lap_completed_value), track_count);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a track numbered on past a number used twice: the second 2 is given 3,
	and the flag numbered 3 after it then 4 */
	CASE("duplicate-then-next")
	{
		static short const numbers[] = { 0, 1, 2, 2, 3 };

		reset(numbers, NUMBEROF_NUMBERS(numbers));
		race_flags_make_unique();
		check_unique(5);
		CHECK(flags[3].team_index == 3 && flags[4].team_index == 4, "the flags numbered %d and %d, not 3 and 4",
			flags[3].team_index, flags[4].team_index);
		return 0;
	}
	/* three flags of one number */
	CASE("three-of-one")
	{
		static short const numbers[] = { 1, 1, 1 };

		reset(numbers, NUMBEROF_NUMBERS(numbers));
		race_flags_make_unique();
		check_unique(3);
		return 0;
	}
	/* numbers of their own, other flags among them, and numbers past the
	track's (which the race refuses) are left as they are */
	CASE("unique-unchanged")
	{
		static short const numbers[] = { 3, -1, 0, -2, 2, 1, 40 };
		long index;

		reset(numbers, NUMBEROF_NUMBERS(numbers));
		race_flags_make_unique();
		for (index = 0; index < NUMBEROF_NUMBERS(numbers); index++)
		{
			CHECK(flags[index].team_index == (numbers[index] < 0 ? -numbers[index] - 1 : numbers[index]),
				"flag %ld numbered %d, not as the map has it", index, flags[index].team_index);
		}
		return 0;
	}
	/* flag rally chooses among the track's flags: each of them can be chosen */
	CASE("rally")
	{
		static short const numbers[] = { 0, 0, 1, 2 };
		unsigned long chosen = 0;
		long count;

		reset(numbers, NUMBEROF_NUMBERS(numbers));
		race_flags_make_unique();
		check_unique(4);
		count = count_bits_32(race_globals.lap_completed_value);
		for (chosen_random = 0; chosen_random < count; chosen_random++)
		{
			long flag = new_rally_flag(NONE);

			CHECK(flag >= 0 && flag < MAXIMUM_RACE_FLAGS, "flag rally chose flag %ld", flag);
			SET_FLAG(chosen, flag, TRUE);
		}
		CHECK(chosen == race_globals.lap_completed_value, "flag rally could choose the flags %lx of %lx", chosen,
			race_globals.lap_completed_value);
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
