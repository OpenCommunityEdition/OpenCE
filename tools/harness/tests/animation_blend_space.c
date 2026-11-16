/*
ANIMATION_BLEND_SPACE.C (test)

The real animation_blend_space.c: the space (the object's, or the world's
for a world-relative animation) of each pose computed is noted, a blend
takes the space of the pose it keeps as it starts, and a blend into an
animation of the other space is not drawn. In c10's flashback marine_2 and
Bisenti blended from their standing pose into world-relative animations,
and back, and flew across the room. An entry is an object's only while its
index is, and none is kept once forgotten (a map's start, a game state's
load).
test_animation_blend_space.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#include "config.inc"

#include "under_test.inc"

/* marine_2 and Bisenti, the objects at their absolute indices with another
salt, and one past the array */
#define MARINE_2 0xEA2C00A9L
#define BISENTI 0xEA2900A6L
#define MARINE_2_REUSED 0xEB2C00A9L
#define PAST_THE_ARRAY ((long)(0xEA000000UL | MAXIMUM_OBJECTS_PER_MAP))

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	animation_blend_space_forget();

	/* standing, then a world-relative custom animation with its blend:
	crossed, as it is drawn in the new space, until the blend ends */
	CASE("into the world's")
	{
		animation_blend_space_computed(MARINE_2, FALSE);
		animation_blend_space_started(MARINE_2);
		CHECK(animation_blend_space_crossed(MARINE_2, TRUE), "the blend into the world's space drawn");
		animation_blend_space_computed(MARINE_2, TRUE);
		CHECK(animation_blend_space_crossed(MARINE_2, TRUE), "drawn after its first tick");
		return 0;
	}
	/* back from the world-relative animation to the unit's own, the new
	animation set before the blend starts (unit_animation_set_state) */
	CASE("back to its own")
	{
		animation_blend_space_computed(BISENTI, TRUE);
		animation_blend_space_started(BISENTI);
		CHECK(animation_blend_space_crossed(BISENTI, FALSE), "the blend back drawn");
		return 0;
	}
	/* blends within one space, either: drawn */
	CASE("within one space")
	{
		animation_blend_space_computed(MARINE_2, FALSE);
		animation_blend_space_started(MARINE_2);
		CHECK(!animation_blend_space_crossed(MARINE_2, FALSE), "a blend in the object's space not drawn");
		animation_blend_space_computed(BISENTI, TRUE);
		animation_blend_space_started(BISENTI);
		CHECK(!animation_blend_space_crossed(BISENTI, TRUE), "a blend in the world's space not drawn");
		return 0;
	}
	/* the space of the pose last computed, and each object its own */
	CASE("the pose last computed")
	{
		animation_blend_space_computed(MARINE_2, TRUE);
		animation_blend_space_computed(BISENTI, TRUE);
		animation_blend_space_computed(MARINE_2, FALSE);
		animation_blend_space_started(MARINE_2);
		animation_blend_space_started(BISENTI);
		CHECK(animation_blend_space_crossed(MARINE_2, TRUE), "marine_2's earlier pose taken");
		CHECK(!animation_blend_space_crossed(BISENTI, TRUE), "marine_2's pose taken for Bisenti");
		return 0;
	}
	/* nothing known: drawn, as the original draws it */
	CASE("nothing known")
	{
		animation_blend_space_started(MARINE_2);
		CHECK(!animation_blend_space_crossed(MARINE_2, TRUE), "a blend with no pose computed not drawn");
		animation_blend_space_computed(MARINE_2, FALSE);
		animation_blend_space_started(MARINE_2);
		CHECK(!animation_blend_space_crossed(MARINE_2_REUSED, TRUE), "another object at the index taken for it");
		animation_blend_space_computed(MARINE_2_REUSED, FALSE);
		animation_blend_space_started(MARINE_2_REUSED);
		CHECK(!animation_blend_space_crossed(MARINE_2, TRUE), "the index's former object taken for it");
		CHECK(!animation_blend_space_crossed(NONE, TRUE), "no object crossed");
		animation_blend_space_computed(PAST_THE_ARRAY, FALSE);
		animation_blend_space_started(PAST_THE_ARRAY);
		CHECK(!animation_blend_space_crossed(PAST_THE_ARRAY, TRUE), "an index past the array crossed");
		return 0;
	}
	/* forgotten (a map's start, a game state's load): nothing known */
	CASE("forgotten")
	{
		animation_blend_space_computed(MARINE_2, FALSE);
		animation_blend_space_started(MARINE_2);
		animation_blend_space_forget();
		CHECK(!animation_blend_space_crossed(MARINE_2, TRUE), "a blend kept across a load");
		animation_blend_space_computed(BISENTI, TRUE);
		animation_blend_space_forget();
		animation_blend_space_started(BISENTI);
		CHECK(!animation_blend_space_crossed(BISENTI, FALSE), "a pose kept across a load");
		return 0;
	}

	return 2;
}
