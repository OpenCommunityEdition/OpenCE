/* Gun Race Pro rule core. Compile with -DGUN_RACE_RULES_ONLY. */

#ifndef GUN_RACE_RULES_ONLY
#define GUN_RACE_RULES_ONLY
#endif

#include "gun_race.h"

static int failures = 0;

static void expect(int condition, const char *message)
{
	(void)message;
	if (!condition)
		failures++;
}

static void add_all(struct gr_match *match)
{
	int weapon;

	for (weapon = 0; weapon < GR_WEAPON_COUNT; weapon++)
		gr_match_add_stage(match, weapon);
}

static void add_without_heavy(struct gr_match *match)
{
	gr_match_add_stage(match, GR_ROCKET_LAUNCHER);
	gr_match_add_stage(match, GR_SNIPER);
	gr_match_add_stage(match, GR_SHOTGUN);
	gr_match_add_stage(match, GR_NEEDLER);
	gr_match_add_stage(match, GR_ASSAULT_RIFLE);
	gr_match_add_stage(match, GR_PLASMA_RIFLE);
	gr_match_add_stage(match, GR_PISTOL);
	gr_match_add_stage(match, GR_PLASMA_PISTOL);
	gr_match_add_stage(match, GR_UNARMED);
}

static int kill(struct gr_match *match, int weapon, int melee)
{
	return gr_host_kill(match, 0, 1, weapon, melee);
}

static void test_full_ladder(void)
{
	struct gr_match match;
	int kill_index;
	int stage;

	gr_match_init(&match);
	match.enabled = 1;
	add_all(&match);
	gr_player_join(&match, 0);
	gr_player_join(&match, 1);

	expect(gr_kills_to_win(&match) == 60, "full ladder is 60 kills");
	expect(gr_current_weapon(&match, 0) == GR_FLAMETHROWER, "start on flamethrower");
	expect(!gr_allow_grenade_use(&match), "grenades refused");
	expect(!gr_allow_world_weapon_pickup(&match), "pickups refused");
	expect(kill(&match, GR_FLAMETHROWER, 1) == GR_KILL_IGNORED, "melee on a gun does not count");
	expect(kill(&match, GR_PISTOL, 0) == GR_KILL_IGNORED, "wrong gun does not count");
	expect(gr_host_kill(&match, 0, 0, GR_FLAMETHROWER, 0) == GR_KILL_IGNORED, "suicide does not count");

	for (stage = 0; stage < GR_UNARMED; stage++)
	{
		for (kill_index = 0; kill_index < GR_KILLS_PER_GUN; kill_index++)
		{
			int result;

			result = kill(&match, stage, 0);
			if (kill_index + 1 < GR_KILLS_PER_GUN)
				expect(result == GR_KILL_COUNTED, "kill counted");
			else
				expect(result == GR_STAGE_ADVANCED, "fifth kill advances");
		}
		expect(gr_current_weapon(&match, 0) == stage + 1, "advanced in order");
		expect(match.players[0].stage_kills == 0, "stage kills reset");
	}

	expect(gr_current_weapon(&match, 0) == GR_UNARMED, "final stage is unarmed");
	expect(!gr_unarmed_melee_should_be_lethal(&match, 0, 0), "a shot is not the finishing melee");
	expect(gr_unarmed_melee_should_be_lethal(&match, 0, 1), "unarmed melee is lethal");
	expect(kill(&match, GR_PISTOL, 0) == GR_KILL_IGNORED, "a gun does not finish unarmed");

	for (kill_index = 0; kill_index < GR_FINAL_MELEE_KILLS; kill_index++)
	{
		int result;

		result = kill(&match, GR_UNARMED, 1);
		if (kill_index + 1 < GR_FINAL_MELEE_KILLS)
			expect(result == GR_KILL_COUNTED, "melee counted");
		else
			expect(result == GR_MATCH_WON, "tenth melee wins");
	}
	expect(match.winner_slot == 0, "player 0 won");
	expect(match.players[0].valid_kills == 60, "sixty valid kills");
	expect(kill(&match, GR_UNARMED, 1) == GR_KILL_IGNORED, "nothing counts after the win");
}

static void test_skipped_weapons(void)
{
	struct gr_match match;
	int kill_index;

	gr_match_init(&match);
	match.enabled = 1;
	add_without_heavy(&match);
	gr_player_join(&match, 0);
	gr_player_join(&match, 1);

	expect(match.stage_count == 9, "two missing guns leave nine stages");
	expect(gr_current_weapon(&match, 0) == GR_ROCKET_LAUNCHER, "ladder starts at the rocket launcher");
	expect(gr_kills_to_win(&match) == 50, "skipped guns lower the win score");
	expect(kill(&match, GR_FLAMETHROWER, 0) == GR_KILL_IGNORED, "a skipped gun does not count");

	for (kill_index = 0; kill_index < GR_KILLS_PER_GUN - 1; kill_index++)
		expect(kill(&match, GR_ROCKET_LAUNCHER, 0) == GR_KILL_COUNTED, "rocket kill counted");
	expect(match.players[0].stage_kills == 4, "four rocket kills");
	expect(kill(&match, GR_ROCKET_LAUNCHER, 0) == GR_STAGE_ADVANCED, "fifth rocket kill advances");
	expect(gr_current_weapon(&match, 0) == GR_SNIPER, "next weapon is the sniper rifle");
}

__declspec(dllimport) void __stdcall ExitProcess(unsigned int code);

void mainCRTStartup(void)
{
	test_full_ladder();
	test_skipped_weapons();
	ExitProcess((unsigned int)failures);
}
