/*
VOTEKICK_COUNT.C (test)

The real count of port/linux/game/network_votekick.c, which stands between
the game's players and a kick: who may vote, whose votes are one person's,
and how many kick. test_votekick_count.py takes the constants and the
voter's structure (config.inc) and the code (under_test.inc).
*/

#include "harness.h"

#define csstrcmp strcmp
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include "config.inc"
#include "under_test.inc"

static struct votekick_voter voters[16];
static short voter_count;

static struct votekick_voter *add(
	unsigned long address,
	char const *hardware_id,
	boolean eligible,
	boolean voted)
{
	struct votekick_voter *voter = &voters[voter_count++];

	memset(voter, 0, sizeof(*voter));
	voter->address = address;
	snprintf(voter->hardware_id, sizeof(voter->hardware_id), "%s", hardware_id);
	voter->eligible = eligible;
	voter->voted = voted;
	return voter;
}

static void target(
	unsigned long address,
	char const *hardware_id)
{
	add(address, hardware_id, TRUE, FALSE)->target = TRUE;
}

/* the count: votes, those who may vote, and the votes needed */
static void count(
	short *votes,
	short *electorate,
	short *needed)
{
	votekick_count(voters, voter_count, votes, electorate);
	*needed = votekick_votes_needed(*electorate, 0);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short votes, electorate, needed;

	/* more than half of everyone, the target counted */
	CASE("majority")
	{
		target(0x0A000001, "t");
		add(0x0A000002, "a", TRUE, TRUE);
		add(0x0A000003, "b", TRUE, TRUE);
		add(0x0A000004, "c", TRUE, TRUE);
		add(0x0A000005, "d", TRUE, FALSE);
		add(0x0A000006, "e", TRUE, FALSE);
		count(&votes, &electorate, &needed);
		CHECK(electorate == 6, "%d may vote, not 6", electorate);
		CHECK(needed == 4, "%d needed of 6, not 4", needed);
		CHECK(votes == 3, "%d votes, not 3", votes);
		return 0;
	}
	/* one team of an even game kicks nobody from the other */
	CASE("even-teams")
	{
		unsigned long index;

		for (index = 0; index < 4; index++)
			add(0x0A000010 + index, "", TRUE, TRUE);
		target(0x0A000020, "");
		for (index = 1; index < 4; index++)
			add(0x0A000020 + index, "", TRUE, FALSE);
		count(&votes, &electorate, &needed);
		CHECK(votes == 4 && electorate == 8, "%d votes of %d", votes, electorate);
		CHECK(votes < needed, "one team of four kicked from the other (%d needed)", needed);
		return 0;
	}
	/* two people never kick each other, nor one alone */
	CASE("two-never-kick")
	{
		target(0x0A000001, "");
		add(0x0A000002, "", TRUE, TRUE);
		count(&votes, &electorate, &needed);
		CHECK(votes < needed, "one of two kicked the other (%d of %d)", votes, needed);
		CHECK(votekick_votes_needed(0, 0) >= 2 && votekick_votes_needed(1, 0) >= 2, "one vote kicked");
		return 0;
	}
	/* machines of one address: one vote, and counted once */
	CASE("same-address-one-vote")
	{
		target(0x0A000001, "");
		add(0x0A000002, "a", TRUE, TRUE);
		add(0x0A000002, "b", TRUE, TRUE);
		add(0x0A000002, "c", TRUE, TRUE);
		add(0x0A000003, "d", TRUE, FALSE);
		count(&votes, &electorate, &needed);
		CHECK(votes == 1, "one address voted %d times", votes);
		CHECK(electorate == 3, "%d may vote, not 3 addresses", electorate);
		return 0;
	}
	/* machines of one hardware id: one vote */
	CASE("same-hardware-id-one-vote")
	{
		target(0x0A000001, "");
		add(0x0A000002, "abc", TRUE, TRUE);
		add(0x0A000003, "abc", TRUE, TRUE);
		count(&votes, &electorate, &needed);
		CHECK(votes == 1, "one hardware id voted %d times", votes);
		return 0;
	}
	/* a hardware id copied from another (anyone may tell any) never lowers
	the votes needed: those who may vote are counted by address */
	CASE("copied-hardware-id-lowers-nothing")
	{
		target(0x0A000001, "t");
		add(0x0A000002, "a", TRUE, TRUE);
		add(0x0A000003, "b", TRUE, FALSE);
		add(0x0A000004, "c", TRUE, FALSE);
		add(0x0A000005, "a", TRUE, FALSE);
		add(0x0A000006, "b", TRUE, FALSE);
		count(&votes, &electorate, &needed);
		CHECK(electorate == 6, "copied hardware ids made %d who may vote, not 6", electorate);
		CHECK(needed == 4, "%d needed, not 4", needed);
		return 0;
	}
	/* the target's person never votes against it, by address or id */
	CASE("target-never-votes")
	{
		target(0x0A000001, "t");
		add(0x0A000001, "x", TRUE, TRUE);
		add(0x0A000002, "t", TRUE, TRUE);
		add(0x0A000003, "y", TRUE, TRUE);
		count(&votes, &electorate, &needed);
		CHECK(votes == 1, "the target's address or id voted (%d votes)", votes);
		return 0;
	}
	/* those who have not played long enough neither vote nor count; an
	address not known neither; the host's machine does */
	CASE("eligibility")
	{
		target(0x0A000001, "");
		add(0x0A000002, "", FALSE, TRUE);
		add(0x0A000003, "", FALSE, TRUE);
		add(0, "z", TRUE, TRUE);
		add(0, "", TRUE, TRUE)->host = TRUE;
		add(0x0A000004, "", TRUE, TRUE);
		count(&votes, &electorate, &needed);
		CHECK(votes == 2, "%d votes, not the host's and one other", votes);
		CHECK(electorate == 3, "%d may vote, not 3", electorate);
		return 0;
	}
	/* the yes needed scale with the players who may vote: a majority up to
	8, 40% (at least 5) up to 32, 30% (at least 13) beyond */
	CASE("scales")
	{
		static short const expected[][2] =
		{
			{ 2, 2 }, { 3, 2 }, { 4, 3 }, { 8, 5 }, { 9, 5 }, { 12, 5 }, { 13, 6 }, { 16, 7 }, { 20, 8 },
			{ 32, 13 }, { 33, 13 }, { 40, 13 }, { 44, 14 }, { 64, 20 }, { 100, 30 },
		};
		short index;

		for (index = 0; index < (short)(sizeof(expected) / sizeof(expected[0])); index++)
		{
			needed = votekick_votes_needed(expected[index][0], 0);
			CHECK(needed == expected[index][1], "%d needed of %d, not %d", needed, expected[index][0],
				expected[index][1]);
		}
		for (index = 1; index < 128; index++)
			CHECK(votekick_votes_needed(index, 0) >= votekick_votes_needed((short)(index - 1), 0), "fewer needed of %d", index);
		return 0;
	}
	/* a static percentage of those who may vote, rounded up, at least 2 */
	CASE("static")
	{
		static short const expected[][3] =
		{
			{ 10, 50, 5 }, { 11, 50, 6 }, { 6, 50, 3 }, { 3, 50, 2 }, { 2, 50, 2 }, { 20, 75, 15 }, { 16, 100, 16 },
			{ 64, 30, 20 }, { 7, 1, 2 }, { 9, 67, 7 },
		};
		short index;

		for (index = 0; index < (short)(sizeof(expected) / sizeof(expected[0])); index++)
		{
			needed = votekick_votes_needed(expected[index][0], expected[index][1]);
			CHECK(needed == expected[index][2], "%d needed of %d at %d%%, not %d", needed, expected[index][0],
				expected[index][1], expected[index][2]);
		}
		return 0;
	}
	/* a vote passes with enough yes, and more yes than no */
	CASE("passes")
	{
		CHECK(votekick_passes(5, 0, 5), "5 yes of 5 needed did not pass");
		CHECK(!votekick_passes(4, 0, 5), "4 yes of 5 needed passed");
		CHECK(!votekick_passes(7, 7, 7), "as many no as yes passed");
		CHECK(votekick_passes(8, 7, 7), "more yes than no did not pass");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
