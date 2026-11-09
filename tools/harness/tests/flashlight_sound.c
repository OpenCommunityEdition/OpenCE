/*
FLASHLIGHT_SOUND.C (test)

The real flashlight_sound_toggle_effect (flashlight_sound.c): a unit's
flashlight plays its definition's toggle effect as it turns on or off; the
multiplayer cyborg's names none, so that a multiplayer flashlight made no
sound. A player's unit whose definition names none plays the campaign
cyborg's (weapons\assault rifle\effects\flashlight), when the map has it.
test_flashlight_sound.py takes the code from the sources (under_test.inc).
*/

#include "harness.h"

#define EFFECT_DEFINITION_TAG 0x65666665 /* 'effe' */
#define _object_mask_unit 3

/* (what the fallback reads of a unit) */
struct unit_datum
{
	struct
	{
		long player_index;
	} unit;
};

static struct unit_datum units[2] = { { { 0 } }, { { NONE } } }; /* a player's, an AI's */
static long flashlight_effect_index = 42; /* the map's campaign flashlight effect; NONE for none */

static void *object_try_and_get_and_verify_type(long object_index, unsigned long mask)
{
	(void)mask;
	return object_index >= 0 && object_index < 2 ? &units[object_index] : NULL;
}

static long tag_loaded(long group_tag, char const *name)
{
	return group_tag == EFFECT_DEFINITION_TAG && !strcmp(name, "weapons\\assault rifle\\effects\\flashlight") ?
		flashlight_effect_index : NONE;
}

#include "under_test.inc"

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* a definition's own toggle effect (the campaign cyborg's, any unit's) */
	CASE("own effect")
	{
		CHECK(flashlight_sound_toggle_effect(0, 7) == 7, "a player's: %ld", flashlight_sound_toggle_effect(0, 7));
		CHECK(flashlight_sound_toggle_effect(1, 7) == 7, "an AI's: %ld", flashlight_sound_toggle_effect(1, 7));
		return 0;
	}
	/* the multiplayer cyborg (none of its own) a player plays: the campaign
	cyborg's */
	CASE("multiplayer player")
	{
		CHECK(flashlight_sound_toggle_effect(0, NONE) == 42, "%ld", flashlight_sound_toggle_effect(0, NONE));
		return 0;
	}
	/* a unit no player plays, no unit, a map without the campaign's effect:
	none, as before */
	CASE("none")
	{
		CHECK(flashlight_sound_toggle_effect(1, NONE) == NONE, "an AI's: %ld", flashlight_sound_toggle_effect(1, NONE));
		CHECK(flashlight_sound_toggle_effect(5, NONE) == NONE, "no unit: %ld", flashlight_sound_toggle_effect(5, NONE));
		flashlight_effect_index = NONE;
		CHECK(flashlight_sound_toggle_effect(0, NONE) == NONE, "no effect in the map: %ld",
			flashlight_sound_toggle_effect(0, NONE));
		return 0;
	}

	fprintf(stderr, "no such case: %s\n", case_name);
	return 2;
}
