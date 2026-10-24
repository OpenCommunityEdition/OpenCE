/*
BOTS_LEVELS.C (test)

The bots' skill levels: bots.c's table of them and its setting
(bots.skill). test_bots_levels.py takes the levels, the table and the
setting's code (skills.inc) from the sources.
*/

#include "harness.h"

#include <string.h>

/* the setting the code reads */
static char const *skill_setting = "normal";

static char const *config_string(char const *name)
{
	return !strcmp(name, "bots.skill") ? skill_setting : "";
}

#include "skills.inc"

static struct bots_skill_definition const *row(short skill)
{
	return &bots_skills[skill];
}

/* every number of a row, in order */
static int row_is(short skill, char const *name, real reaction, real sight, real view, real error, real settle,
	real turn, real cone, real lead, real grenade, real jump, short burst, short pause)
{
	struct bots_skill_definition const *definition = row(skill);

	return !strcmp(definition->name, name) && definition->reaction_seconds == reaction &&
		definition->sight_distance == sight && definition->field_of_view_degrees == view &&
		definition->aim_error_degrees == error && definition->aim_settle_seconds == settle &&
		definition->turn_degrees_per_tick == turn && definition->fire_cone == cone && definition->lead == lead &&
		definition->grenade_chance == grenade && definition->jump_chance == jump && definition->burst_ticks == burst &&
		definition->pause_ticks == pause;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short skill;

	/* the bots' levels as they were are kept number for number: easy,
	normal, heroic and legendary */
	CASE("levels-kept")
	{
		CHECK(row_is(_bots_skill_easy,
			"easy", 0.75f, 22.0f, 100.0f, 12.0f, 1.4f, 8.0f, 2.2f, 0.3f, 0.04f, 0.00f, 6, 14),
			"easy is not the bots' easy of before");
		CHECK(row_is(_bots_skill_normal,
			"normal", 0.45f, 32.0f, 120.0f, 7.0f, 0.9f, 13.0f, 1.8f, 0.7f, 0.10f, 0.05f, 9, 9),
			"normal is not the bots' normal of before");
		CHECK(row_is(_bots_skill_heroic,
			"heroic", 0.30f, 42.0f, 140.0f, 4.0f, 0.6f, 20.0f, 1.5f, 0.8f, 0.16f, 0.15f, 12, 6),
			"heroic is not the bots' heroic of before");
		CHECK(row_is(_bots_skill_legendary,
			"legendary", 0.18f, 55.0f, 160.0f, 2.2f, 0.35f, 30.0f, 1.25f, 1.0f, 0.22f, 0.25f, 16, 4),
			"legendary is not the bots' legendary of before");
		return 0;
	}
	/* six levels, each better than the one below in every number (or as
	good), the new veteran between heroic and legendary */
	CASE("levels-ordered")
	{
		static char const *const names[NUMBER_OF_BOTS_SKILLS] =
			{ "recruit", "easy", "normal", "heroic", "veteran", "legendary" };

		for (skill = 0; skill < NUMBER_OF_BOTS_SKILLS; skill++)
			CHECK(!strcmp(row(skill)->name, names[skill]),
				"level %d is %s, not %s", skill, row(skill)->name, names[skill]);
		for (skill = 1; skill < NUMBER_OF_BOTS_SKILLS; skill++)
		{
			struct bots_skill_definition const *lower = row(skill - 1), *level = row(skill);

			CHECK(level->reaction_seconds < lower->reaction_seconds,
				"%s reacts no sooner than %s", level->name, lower->name);
			CHECK(level->sight_distance > lower->sight_distance,
				"%s sees no further than %s", level->name, lower->name);
			CHECK(level->field_of_view_degrees > lower->field_of_view_degrees, "%s sees no wider", level->name);
			CHECK(level->aim_error_degrees < lower->aim_error_degrees,
				"%s aims no closer than %s", level->name, lower->name);
			CHECK(level->aim_settle_seconds < lower->aim_settle_seconds, "%s settles no sooner", level->name);
			CHECK(level->turn_degrees_per_tick > lower->turn_degrees_per_tick, "%s turns no faster", level->name);
			CHECK(level->fire_cone < lower->fire_cone, "%s fires no tighter", level->name);
			CHECK(level->lead >= lower->lead && level->grenade_chance >= lower->grenade_chance &&
				level->jump_chance >= lower->jump_chance, "%s leads, throws or jumps less", level->name);
			CHECK(level->burst_ticks > lower->burst_ticks && level->pause_ticks < lower->pause_ticks,
				"%s's bursts are no longer, or its pauses no shorter", level->name);
		}
		/* (recruit never throws nor jumps by the table, nor leads) */
		CHECK(row(_bots_skill_recruit)->grenade_chance == 0.0f && row(_bots_skill_recruit)->jump_chance == 0.0f &&
			row(_bots_skill_recruit)->lead == 0.0f, "recruit throws, jumps or leads");
		return 0;
	}
	/* bots.skill names a level (anything else is normal) */
	CASE("settings")
	{
		static char const *const names[NUMBER_OF_BOTS_SKILLS] =
			{ "recruit", "easy", "normal", "heroic", "veteran", "legendary" };

		for (skill = 0; skill < NUMBER_OF_BOTS_SKILLS; skill++)
		{
			skill_setting = names[skill];
			CHECK(bots_skill() == skill, "bots.skill \"%s\" is level %d", names[skill], bots_skill());
		}
		skill_setting = "";
		CHECK(bots_skill() == _bots_skill_normal, "no bots.skill is not normal");
		skill_setting = "impossible";
		CHECK(bots_skill() == _bots_skill_normal, "an unknown bots.skill is not normal");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
