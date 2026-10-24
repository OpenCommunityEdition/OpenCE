/*
BOTS_LEVELS.C (test)

The bots' skill levels (bots.c's table and its setting, bots.skill) and
their fight by level (bots_combat.c's rules, compiled as
they are). test_bots_levels.py takes the table and the settings' code
(skills.inc) and the rules (bots_combat.h, under_test.inc) from the sources.
*/

#include "harness.h"

#include <string.h>

#include "under_test.inc"

/* the setting the code reads */
static char const *skill_setting = "normal";

static char const *config_string(char const *name)
{
	return !strcmp(name, "bots.skill") ? skill_setting : "";
}

#include "skills.inc"

#define TICKS BOTS_COMBAT_TICKS_PER_SECOND

static struct bots_skill_definition const *row(short skill)
{
	return &bots_skills[skill];
}

/* every number of a row, in order */
static int row_is(short skill, char const *name, real reaction, real sight, real view, real error, real settle,
	real turn, real cone, real lead, real grenade, short burst, short pause)
{
	struct bots_skill_definition const *definition = row(skill);

	return !strcmp(definition->name, name) && definition->reaction_seconds == reaction &&
		definition->sight_distance == sight && definition->field_of_view_degrees == view &&
		definition->aim_error_degrees == error && definition->aim_settle_seconds == settle &&
		definition->turn_degrees_per_tick == turn && definition->fire_cone == cone && definition->lead == lead &&
		definition->grenade_chance == grenade && definition->burst_ticks == burst &&
		definition->pause_ticks == pause;
}

/* the chance a second of a throw, from many ticks' random numbers */
static float grenade_rate(short skill, float distance, int first_contact, int about_to_die, float skill_chance)
{
	int tick, throws = 0;

	for (tick = 0; tick < 30000; tick++)
		throws += bots_combat_grenade(skill, distance, 1, 1, first_contact, about_to_die, skill_chance,
			(float)tick / 30000.0f);
	return (float)throws / 1000.0f;
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";
	short skill;

	/* the bots' levels as they were are kept number for number: easy,
	normal, heroic and legendary (but their jump chance: the jumps in a
	fight are bots_combat_jump_delay's) */
	CASE("levels-kept")
	{
		CHECK(row_is(_bots_skill_easy,
			"easy", 0.75f, 22.0f, 100.0f, 12.0f, 1.4f, 8.0f, 2.2f, 0.3f, 0.04f, 6, 14),
			"easy is not the bots' easy of before");
		CHECK(row_is(_bots_skill_normal,
			"normal", 0.45f, 32.0f, 120.0f, 7.0f, 0.9f, 13.0f, 1.8f, 0.7f, 0.10f, 9, 9),
			"normal is not the bots' normal of before");
		CHECK(row_is(_bots_skill_heroic,
			"heroic", 0.30f, 42.0f, 140.0f, 4.0f, 0.6f, 20.0f, 1.5f, 0.8f, 0.16f, 12, 6),
			"heroic is not the bots' heroic of before");
		CHECK(row_is(_bots_skill_legendary,
			"legendary", 0.18f, 55.0f, 160.0f, 2.2f, 0.35f, 30.0f, 1.25f, 1.0f, 0.22f, 16, 4),
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
			CHECK(level->lead >= lower->lead && level->grenade_chance >= lower->grenade_chance,
				"%s leads or throws less", level->name);
			CHECK(level->burst_ticks > lower->burst_ticks && level->pause_ticks < lower->pause_ticks,
				"%s's bursts are no longer, or its pauses no shorter", level->name);
		}
		/* (recruit never throws by the table, nor leads) */
		CHECK(row(_bots_skill_recruit)->grenade_chance == 0.0f && row(_bots_skill_recruit)->lead == 0.0f,
			"recruit throws or leads");
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
	/* the reaction: 0.85 to 1.35 of the level's (RECRUIT's never quicker
	than its own); the look 3 to 5 ticks apart; the hash's numbers spread
	and repeatable */
	CASE("reaction")
	{
		unsigned long a;
		int low = 0;

		CHECK(bots_combat_reaction_ticks(_bots_skill_normal, 42, 0.0f) == 36, "normal's quickest is not 36");
		CHECK(bots_combat_reaction_ticks(_bots_skill_normal, 42, 0.999f) == 57, "normal's slowest is not 57");
		CHECK(bots_combat_reaction_ticks(_bots_skill_normal, 42, -3.0f) == 36, "a random under 0 is not 0");
		CHECK(bots_combat_reaction_ticks(_bots_skill_recruit, 42, 0.0f) == 42,
			"recruit reacts quicker than its own time");
		CHECK(bots_combat_reaction_ticks(_bots_skill_recruit, 42, 0.999f) == 63, "recruit's slowest is not 63");
		CHECK(bots_combat_look_jitter(0.0f) == 0 && bots_combat_look_jitter(0.5f) == 1 &&
			bots_combat_look_jitter(1.0f) == 2, "the look's jitter is not 0 to 2");
		for (a = 0; a < 2000; a++)
		{
			float h = bots_combat_hash01(a, 77);

			CHECK(h >= 0.0f && h < 1.0f && h == bots_combat_hash01(a, 77), "hash %lu out of 0..1 or not repeatable", a);
			low += h < 0.5f;
		}
		CHECK(low > 800 && low < 1200, "%d of 2000 hashes under a half", low);
		return 0;
	}
	/* aiming: the trail of the lowest levels, RECRUIT's wander, the far
	target's floor, headshots, the vertical view */
	CASE("aim")
	{
		float error = 0.0f, total = 0.0f;
		int step, near = 0;

		CHECK(bots_combat_track_delay_ticks(_bots_skill_recruit) == 8 &&
			bots_combat_track_delay_ticks(_bots_skill_easy) == 6,
			"recruit's and easy's trails are not 8 and 6 ticks");
		for (skill = _bots_skill_normal; skill < NUMBER_OF_BOTS_SKILLS; skill++)
			CHECK(bots_combat_track_delay_ticks(skill) == 0, "level %d's aim trails its target", skill);
		/* (RECRUIT: never settled, never wild: a few degrees off, seldom under
		0.8, never past 6) */
		for (step = 0; step < 5000; step++)
		{
			error = bots_combat_aim_wander(_bots_skill_recruit, error, 1.0f,
				(float)((step * 7919) % 2001) / 1000.0f - 1.0f);
			CHECK(error >= -6.0f && error <= 6.0f, "recruit's error %.2f past 6 degrees", error);
			near += error > -0.8f && error < 0.8f;
			total += error < 0.0f ? -error : error;
		}
		CHECK(near < 100 && total / 5000.0f > 2.0f && total / 5000.0f < 5.0f,
			"recruit's error settled: %d of 5000 steps under 0.8 degrees, %.2f on average", near, total / 5000.0f);
		for (skill = _bots_skill_easy; skill < NUMBER_OF_BOTS_SKILLS; skill++)
			CHECK(bots_combat_aim_wander(skill, 0.1f, 1.0f, 1.0f) == 0.1f, "level %d's error wanders", skill);
		CHECK(bots_combat_range_error_floor(_bots_skill_normal, 9.0f) == 0.0f, "a floor within 10 units");
		CHECK(bots_combat_range_error_floor(_bots_skill_veteran, 20.0f) > 1.1f &&
			bots_combat_range_error_floor(_bots_skill_veteran, 20.0f) < 1.3f,
				"veteran's floor at 20 units is not 1.2");
		CHECK(bots_combat_range_error_floor(_bots_skill_legendary, 40.0f) == 0.0f, "legendary keeps an error at range");
		for (skill = _bots_skill_easy; skill < _bots_skill_legendary; skill++)
			CHECK(bots_combat_range_error_floor(skill, 20.0f) < bots_combat_range_error_floor(skill - 1, 20.0f),
				"level %d's floor is no smaller than the one below's", skill);
		CHECK(!bots_combat_aim_head(_bots_skill_easy, 0.0f, 1), "easy aims at heads");
		CHECK(bots_combat_aim_head(_bots_skill_normal, 0.0f, 1), "normal does not aim at a head");
		CHECK(!bots_combat_aim_head(_bots_skill_legendary, 0.1f, 1) && !bots_combat_aim_head(_bots_skill_legendary, 0.0f, 0),
			"a head aimed at with its shields up or an automatic weapon");
		for (skill = 1; skill < NUMBER_OF_BOTS_SKILLS; skill++)
			CHECK(bots_combat_vertical_half_view(skill) > bots_combat_vertical_half_view(skill - 1),
				"level %d sees no further up than the one below", skill);
		CHECK(bots_combat_in_vertical_view(_bots_skill_heroic, 30.0f, 0.0f), "heroic misses 30 degrees up");
		CHECK(!bots_combat_in_vertical_view(_bots_skill_heroic, 45.0f, 0.0f),
			"heroic sees 45 degrees up looking ahead");
		CHECK(bots_combat_in_vertical_view(_bots_skill_heroic, 45.0f, 20.0f), "heroic misses 45 degrees up looking up");
		CHECK(!bots_combat_in_vertical_view(_bots_skill_heroic, -40.0f, 0.0f), "heroic sees 40 degrees down");
		CHECK(!bots_combat_in_vertical_view(_bots_skill_recruit, 23.0f, 0.0f), "recruit sees 23 degrees up");
		return 0;
	}
	/* firing: the pistol paced at range from NORMAL, the sniper rifle zoomed
	and zoomed again quicker at each level */
	CASE("firing")
	{
		CHECK(!bots_combat_pace_holds(_bots_skill_easy, 20.0f, 1.0f, 0), "easy paces its shots");
		CHECK(!bots_combat_pace_holds(_bots_skill_normal, 8.0f, 1.0f, 0), "normal paces its shots close up");
		CHECK(bots_combat_pace_holds(_bots_skill_normal, 12.0f, 0.7f, 0), "normal does not pace at range");
		CHECK(!bots_combat_pace_holds(_bots_skill_normal, 12.0f, 0.5f, 0), "normal holds a settled shot");
		CHECK(!bots_combat_pace_holds(_bots_skill_normal, 12.0f, 0.7f, 6), "normal holds past its 6 ticks");
		CHECK(bots_combat_pace_holds(_bots_skill_legendary, 12.0f, 0.3f, 11) &&
			!bots_combat_pace_holds(_bots_skill_legendary, 12.0f, 0.3f, 12),
			"legendary's pacing is not to 0.25 for 12 ticks");
		CHECK(!bots_combat_zooms(_bots_skill_easy, 30.0f) && !bots_combat_zooms(_bots_skill_normal, 6.0f) &&
			bots_combat_zooms(_bots_skill_normal, 6.5f), "the zoom is not from normal past 6 units");
		CHECK(bots_combat_rescope_ticks(_bots_skill_easy, 0.5f) == 0, "easy zooms again");
		CHECK(bots_combat_rescope_ticks(_bots_skill_normal, 0.0f) == 18 &&
			bots_combat_rescope_ticks(_bots_skill_normal, 1.0f) == 24,
			"normal's zoom again is not 0.6 to 0.8 s");
		for (skill = _bots_skill_normal; skill < _bots_skill_legendary; skill++)
			CHECK(bots_combat_rescope_ticks(skill + 1, 1.0f) < bots_combat_rescope_ticks(skill, 0.0f),
				"level %d zooms again no sooner than the one below", skill + 1);
		CHECK(!bots_combat_unzoomed_holds_fire(10.0f) && bots_combat_unzoomed_holds_fire(10.5f),
			"unzoomed, the fire is not held past 10 units");
		return 0;
	}
	/* blows: RECRUIT at point blank, now and then; HEROIC and up when it
	tells; the wait in reach and between blows shorter at each level */
	CASE("melee")
	{
		for (skill = 0; skill < NUMBER_OF_BOTS_SKILLS; skill++)
		{
			CHECK(bots_combat_melee(skill, 0.3f, 0, 1, 0, 1, 0.0f) == 0,
				"level %d strikes before its wait is over", skill);
			CHECK(bots_combat_melee(skill, 1.2f, 1, 1, 0, 1, 0.0f) == 0, "level %d strikes out of reach", skill);
		}
		CHECK(bots_combat_melee(_bots_skill_recruit, 0.9f, 1, 1, 0, 0, 0.0f) == 0, "recruit strikes from 0.9 units");
		CHECK(bots_combat_melee(_bots_skill_recruit, 0.5f, 1, 0, 1, 0, 0.1f) == 1 &&
			bots_combat_melee(_bots_skill_recruit, 0.5f, 1, 0, 1, 0, 0.5f) == 2, "recruit's blows are not 4 in 10");
		CHECK(bots_combat_melee(_bots_skill_normal, 1.0f, 1, 0, 1, 0, 0.99f) == 1, "normal does not strike in reach");
		for (skill = _bots_skill_heroic; skill < NUMBER_OF_BOTS_SKILLS; skill++)
		{
			CHECK(bots_combat_melee(skill, 1.0f, 1, 0, 1, 0, 0.0f) == 0,
				"level %d strikes a full shield from 1 unit", skill);
			CHECK(bots_combat_melee(skill, 1.0f, 1, 1, 1, 0, 0.0f) == 1, "level %d spares a turned back", skill);
			CHECK(bots_combat_melee(skill, 1.0f, 1, 0, 0.05f, 0, 0.0f) == 1, "level %d spares a popped shield", skill);
			CHECK(bots_combat_melee(skill, 1.0f, 1, 0, 1, 1, 0.0f) == 1, "level %d's charge does not strike", skill);
			CHECK(bots_combat_melee(skill, 0.5f, 1, 0, 1, 0, 0.99f) == 1,
				"level %d spares a point blank target", skill);
		}
		CHECK(bots_combat_melee(_bots_skill_heroic, 1.0f, 1, 0, 0.3f, 0, 0.0f) == 0 &&
			bots_combat_melee(_bots_skill_veteran, 1.0f, 1, 0, 0.3f, 0, 0.0f) == 1,
				"the low shields are not 0.1 and 0.4");
		CHECK(bots_combat_melee_delay(_bots_skill_recruit, 0.0f) == 30 &&
			bots_combat_melee_delay(_bots_skill_recruit, 1.0f) == 90,
			"recruit's wait in reach is not 1 to 3 s");
		CHECK(bots_combat_melee_delay(_bots_skill_legendary, 0.0f) == 3 &&
			bots_combat_melee_delay(_bots_skill_legendary, 1.0f) == 9,
			"legendary's wait in reach is not 0.1 to 0.3 s");
		CHECK(bots_combat_melee_cooldown(_bots_skill_normal, 0.0f) == 36, "normal's wait between blows is not 1.2 s");
		for (skill = 1; skill < NUMBER_OF_BOTS_SKILLS; skill++)
		{
			CHECK(bots_combat_melee_delay(skill, 0.0f) < bots_combat_melee_delay(skill - 1, 0.0f) &&
				bots_combat_melee_delay(skill, 1.0f) < bots_combat_melee_delay(skill - 1, 1.0f),
				"level %d waits in reach no less than the one below", skill);
			CHECK(bots_combat_melee_cooldown(skill, 0.0f) < bots_combat_melee_cooldown(skill - 1, 0.0f),
				"level %d waits between blows no less than the one below", skill);
		}
		return 0;
	}
	/* grenades: RECRUIT never, EASY about to die, NORMAL the table's rate,
	HEROIC and up more on first contact and about to die, leading from
	HEROIC; at the last-seen spot from VETERAN */
	CASE("grenades")
	{
		for (skill = 0; skill < NUMBER_OF_BOTS_SKILLS; skill++)
		{
			CHECK(!bots_combat_grenade(skill, 12, 0, 1, 1, 1, 1, 0.0f) &&
				!bots_combat_grenade(skill, 12, 1, 0, 1, 1, 1, 0.0f),
				"level %d throws with no grenade or before it reacts", skill);
			CHECK(!bots_combat_grenade(skill, 5, 1, 1, 1, 1, 1, 0.0f) &&
				!bots_combat_grenade(skill, 19, 1, 1, 1, 1, 1, 0.0f),
				"level %d throws nearer than 6 or further than 18 units", skill);
		}
		CHECK(grenade_rate(_bots_skill_recruit, 12, 1, 1, 0.5f) == 0.0f, "recruit throws");
		CHECK(grenade_rate(_bots_skill_easy, 12, 1, 0, 0.04f) < 0.03f &&
			grenade_rate(_bots_skill_easy, 12, 0, 1, 0.04f) > 0.5f,
			"easy's throws are not rare, but often about to die");
		CHECK(grenade_rate(_bots_skill_normal, 12, 0, 0, 0.10f) > 0.29f &&
			grenade_rate(_bots_skill_normal, 12, 1, 1, 0.10f) < 0.31f,
			"normal's rate is not the table's 0.3 a second");
		CHECK(grenade_rate(_bots_skill_heroic, 12, 1, 0, 0.16f) >
			grenade_rate(_bots_skill_heroic, 12, 0, 0, 0.16f) + 0.4f,
			"heroic throws no more on first contact");
		CHECK(bots_combat_grenade_cooldown(_bots_skill_recruit, 0.0f) == 600 &&
			bots_combat_grenade_cooldown(_bots_skill_normal, 0.0f) == 30,
			"the waits between throws are not recruit's 20 s and normal's 1 s");
		CHECK(bots_combat_grenade_lead(_bots_skill_normal) == 0.0f &&
			bots_combat_grenade_lead(_bots_skill_heroic) == 1.0f,
			"the throws lead below heroic, or not from it");
		CHECK(bots_combat_about_to_die(0, 0.5f) && !bots_combat_about_to_die(0.1f, 0.5f) &&
			!bots_combat_about_to_die(0, 0.7f),
			"about to die is not no shields and under 0.6");
		CHECK(!bots_combat_prenade(_bots_skill_heroic, 1.0f, 10.0f, 1, 0.0f), "heroic throws at a last-seen spot");
		CHECK(bots_combat_prenade(_bots_skill_veteran, 1.0f, 10.0f, 1, 0.4f) &&
			!bots_combat_prenade(_bots_skill_veteran, 1.0f, 10.0f, 1, 0.6f),
			"veteran's throws at a last-seen spot are not half the time");
		CHECK(bots_combat_prenade(_bots_skill_legendary, 1.0f, 10.0f, 1, 0.6f), "legendary's are not three in four");
		CHECK(!bots_combat_prenade(_bots_skill_legendary, 0.5f, 10.0f, 1, 0.0f) &&
			!bots_combat_prenade(_bots_skill_legendary, 3.5f, 10.0f, 1, 0.0f) &&
			!bots_combat_prenade(_bots_skill_legendary, 1.0f, 4.0f, 1, 0.0f) &&
			!bots_combat_prenade(_bots_skill_legendary, 1.0f, 21.0f, 1, 0.0f) &&
			!bots_combat_prenade(_bots_skill_legendary, 1.0f, 10.0f, 0, 0.0f),
			"a last-seen throw out of its time, distance or grenades");
		return 0;
	}
	/* the confidence; the retreat: never for RECRUIT and EASY, after the
	hesitation, 2 s at least (HEROIC 3), 7 at most, 6 before the next; as
	the shields pop from VETERAN */
	CASE("retreat")
	{
		struct bots_combat_retreat state;
		int started, ended;
		long now;
		float even = bots_combat_confidence(1, 1, 1, 1, 5, 9, 1, 0);

		CHECK(even > 0.49f && even < 0.51f, "an even fight's confidence is %.2f", even);
		CHECK(bots_combat_confidence(1, 1, 0, 0.2f, 5, 9, 1, 0) > 0.9f &&
			bots_combat_confidence(0, 0.3f, 1, 1, 5, 9, 1, 0) < 0.2f,
			"winning and losing are not sure and unsure");
		CHECK(bots_combat_confidence(1, 1, 1, 1, 5, 9, 0.1f, 0) < even - 0.1f &&
			bots_combat_confidence(1, 1, 1, 1, 5, 9, 1, 3) > even,
			"an empty magazine or the high ground count nothing");
		CHECK(bots_combat_confidence(3, 1, 0, 0, 1, 9, 1, 9) <= 1.0f &&
			bots_combat_confidence(0, 0, 3, 1, 50, 9, 0, -9) >= 0.0f,
			"the confidence leaves 0..1");
		for (skill = _bots_skill_recruit; skill <= _bots_skill_easy; skill++)
		{
			bots_combat_retreat_reset(&state);
			for (now = 0; now < 300; now++)
				CHECK(!bots_combat_retreat_step(&state, skill, 0.0f, 0.0f, now, 0.5f, &started, &ended),
					"level %d retreats", skill);
		}
		bots_combat_retreat_reset(&state);
		for (now = 100; now < 112; now++)
			CHECK(!bots_combat_retreat_step(&state, _bots_skill_normal, 0.05f, 0.1f, now, 0.5f, &started, &ended),
				"normal retreats before its hesitation of 12 ticks");
		CHECK(bots_combat_retreat_step(&state, _bots_skill_normal, 0.05f, 0.1f, 112, 0.5f, &started, &ended) && started,
			"normal does not retreat after its hesitation");
		CHECK(bots_combat_retreat_step(&state, _bots_skill_normal, 0.9f, 1.0f, 171, 0.5f, &started, &ended),
			"normal's retreat ends before 2 s");
		CHECK(!bots_combat_retreat_step(&state, _bots_skill_normal, 0.3f, 1.0f, 172, 0.5f, &started, &ended) && ended,
			"normal's retreat does not end with its shields back");
		for (now = 173; now < 358; now++)
			CHECK(!bots_combat_retreat_step(&state, _bots_skill_normal, 0.0f, 0.0f, now, 0.0f, &started, &ended),
				"normal retreats again within 6 s");
		CHECK(bots_combat_retreat_step(&state, _bots_skill_normal, 0.0f, 0.0f, 358, 0.0f, &started, &ended),
			"normal does not retreat again after 6 s");
		CHECK(!bots_combat_retreat_step(&state, _bots_skill_normal, 0.0f, 0.0f, 358 + 210, 0.0f, &started, &ended) &&
			ended,
			"a retreat lasts past 7 s");
		/* (shields up: none; as the shields pop, VETERAN after 3 ticks,
		HEROIC not) */
		bots_combat_retreat_reset(&state);
		for (now = 0; now < 100; now++)
			CHECK(!bots_combat_retreat_step(&state, _bots_skill_heroic, 0.0f, 0.5f, now, 0.0f, &started, &ended),
				"a retreat with shields at a half");
		bots_combat_retreat_reset(&state);
		for (now = 100; now < 103; now++)
			CHECK(!bots_combat_retreat_step(&state, _bots_skill_veteran, 0.3f, 0.0f, now, 0.0f, &started, &ended),
				"veteran retreats before 3 ticks");
		CHECK(bots_combat_retreat_step(&state, _bots_skill_veteran, 0.3f, 0.0f, 103, 0.0f, &started, &ended) &&
			started,
			"veteran does not retreat as its shields pop");
		bots_combat_retreat_reset(&state);
		for (now = 100; now < 140; now++)
			CHECK(!bots_combat_retreat_step(&state, _bots_skill_heroic, 0.3f, 0.0f, now, 0.0f, &started, &ended),
				"heroic retreats as its shields pop");
		return 0;
	}
	/* moving: the charge, the step in, the strafes (jiggles at the top),
	halts, jumps and crouches by level, the hunt and the high ground */
	CASE("moves")
	{
		struct bots_combat_crouch crouch = { 0, 0 };
		long now, crouched = 0, first = -1;

		CHECK(!bots_combat_charge(_bots_skill_normal, 0.9f, 0, 0.3f, 3) &&
			bots_combat_charge(_bots_skill_heroic, 0.9f, 0, 0.3f, 3),
			"the charge is not from heroic");
		CHECK(!bots_combat_charge(_bots_skill_heroic, 0.5f, 0, 0.3f, 3) &&
			!bots_combat_charge(_bots_skill_heroic, 0.9f, 0.2f, 0.3f, 3) &&
			!bots_combat_charge(_bots_skill_heroic, 0.9f, 0, 0.8f, 3) &&
			!bots_combat_charge(_bots_skill_legendary, 0.9f, 0, 0.3f, 7),
			"a charge unsure, at shields, at health or far");
		CHECK(!bots_combat_step_in(_bots_skill_recruit, 1.0f, 0.0f), "recruit steps in");
		CHECK(bots_combat_step_in(_bots_skill_easy, 2.4f, 0.4f) && !bots_combat_step_in(_bots_skill_easy, 1.0f, 0.6f),
			"easy's step in is not to a weak target within 2.5");
		CHECK(bots_combat_step_in(_bots_skill_normal, 3.4f, 0.7f) &&
			!bots_combat_step_in(_bots_skill_normal, 3.4f, 0.9f),
			"normal's step in is not under 0.8 within 3.5");
		CHECK(bots_combat_step_in(_bots_skill_heroic, 3.9f, 1.0f) &&
			!bots_combat_step_in(_bots_skill_legendary, 4.1f, 0.0f),
			"heroic's step in is not within 4");
		CHECK(bots_combat_strafe_ticks(_bots_skill_recruit, 0.0f) == 30 &&
			bots_combat_strafe_ticks(_bots_skill_recruit, 1.0f) == 120,
			"recruit's strafes are not 1 to 4 s");
		CHECK(bots_combat_strafe_ticks(_bots_skill_normal, 0.0f) == 60 &&
			bots_combat_strafe_ticks(_bots_skill_normal, 1.0f) == 120,
			"normal's strafes are not 2 to 4 s");
		CHECK(bots_combat_strafe_ticks(_bots_skill_veteran, 0.0f) == 6 &&
			bots_combat_strafe_ticks(_bots_skill_legendary, 1.0f) == 24,
			"the top's jiggles are not 0.2 to 0.8 s");
		CHECK(bots_combat_strafe_strength(_bots_skill_recruit) == 0.15f &&
			bots_combat_strafe_strength(_bots_skill_easy) == 1.0f,
			"recruit sidesteps hard, or easy not");
		for (skill = _bots_skill_recruit; skill <= _bots_skill_easy; skill++)
		{
			CHECK(bots_combat_halt_ticks(skill, 0.0f, 1.0f) == 0 && bots_combat_initial_halt_ticks(skill, 1.0f) == 0,
				"level %d halts", skill);
			CHECK(bots_combat_jump_delay(skill, 0.5f) == 0, "level %d jumps in a fight", skill);
		}
		CHECK(bots_combat_halt_ticks(_bots_skill_normal, 0.1f, 1.0f) == 60 &&
			bots_combat_halt_ticks(_bots_skill_normal, 0.3f, 1.0f) == 0,
			"normal's halts are not 2 in 10, up to 2 s");
		CHECK(bots_combat_halt_ticks(_bots_skill_veteran, 0.04f, 1.0f) == 3 &&
			bots_combat_halt_ticks(_bots_skill_veteran, 0.06f, 1.0f) == 0,
			"veteran's halts are not 1 in 20, up to 0.1 s");
		CHECK(bots_combat_initial_halt_ticks(_bots_skill_normal, 1.0f) == 6, "normal's first halt is not up to 0.2 s");
		CHECK(bots_combat_jump_delay(_bots_skill_normal, 0.0f) == 90 &&
			bots_combat_jump_delay(_bots_skill_veteran, 0.0f) == 30,
			"the jumps are not normal's 3 s, veteran's 1 s");
		/* (crouches: none below NORMAL; one of 0.3 to 0.6 s, then none for
		2.5 s, however often the random numbers ask: 4 of 9 ticks in 10 s) */
		CHECK(!bots_combat_crouch_step(&crouch, _bots_skill_easy, 100, 0.0f), "easy crouches");
		for (now = 100; now < 400; now++)
		{
			if (bots_combat_crouch_step(&crouch, _bots_skill_normal, now, 0.0f))
			{
				crouched++;
				if (first < 0)
					first = now;
			}
		}
		CHECK(first == 100 && crouched == 36, "normal crouched first at %ld, %ld ticks of 300", first, crouched);
		CHECK(bots_combat_hunt_chance(_bots_skill_recruit) == 0.2f &&
			bots_combat_hunt_chance(_bots_skill_normal) == 0.65f,
			"the hunts are not recruit's 0.2, the others' 0.65");
		CHECK(bots_combat_high_ground(_bots_skill_heroic, 3.0f) == 0.0f, "heroic seeks high ground");
		CHECK(bots_combat_high_ground(_bots_skill_veteran, -2.0f) == 0.0f, "veteran credits low ground");
		CHECK(bots_combat_high_ground(_bots_skill_veteran, 2.0f) == 3.0f &&
			bots_combat_high_ground(_bots_skill_veteran, 10.0f) == 4.5f,
			"veteran's credit is not 1.5 a unit up to 4.5");
		CHECK(bots_combat_high_ground(_bots_skill_legendary, 2.0f) == 4.0f &&
			bots_combat_high_ground(_bots_skill_legendary, 10.0f) == 6.0f,
			"legendary's credit is not 2 a unit up to 6");
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
