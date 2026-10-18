/*
ACHIEVEMENTS.C (test)

The rules of port/linux/game/achievements.c (the first part of the file,
ACHIEVEMENTS_RULES_ONLY) fed events as the game's hooks feed them: wins,
reverts, deaths, disqualification, counts and the profile's file.
test_achievements.py takes the file itself (under_test.inc).
*/

#include "harness.h"
#include <stdarg.h>

#include "under_test.inc"

static struct ach_profile saved_profile;
static int unlock_count[ACH_PLAYERS][NUMBER_OF_ACHIEVEMENTS];
static int saves;

void ach_platform_unlocked(int local_player, int achievement)
{
	unlock_count[local_player][achievement]++;
}

void ach_platform_save(int local_player)
{
	(void)local_player;
	saves++;
}

void ach_platform_log(char const *format, ...)
{
	(void)format;
}

ach_u32 ach_platform_time(void)
{
	return 1000;
}

static void begin(int level, int difficulty)
{
	memset(unlock_count, 0, sizeof(unlock_count));
	ach_rules_new_map(1, level, difficulty);
	ach_rules_player_begin(0, 1);
}

static int got(int achievement)
{
	return unlock_count[0][achievement];
}

static void ticks(int count)
{
	while (count-- > 0)
		ach_rules_tick();
}

static void case_birth(void)
{
	begin(_ach_level_a10, _ach_easy);
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 1, "a10 won");
	CHECK(got(_ach_feet_firmly_planted) == 1, "no jump");
	begin(_ach_level_a30, _ach_easy);
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 0, "only a10");
	ach_rules_won(-1);
	CHECK(unlock_count[0][_ach_feet_firmly_planted] == 1, "a second win is no second unlock");
}

static void case_off(void)
{
	memset(unlock_count, 0, sizeof(unlock_count));
	ach_rules_new_map(0, _ach_level_a10, _ach_normal);
	ach_rules_player_begin(0, 1);
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 0, "off");
	ach_rules_new_map(1, -1, _ach_normal);
	ach_rules_player_begin(0, 1);
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 0, "not a campaign level");
	begin(_ach_level_a10, _ach_normal);
	ach_rules_player_begin(0, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 0, "no profile to keep it in");
}

static void case_taint(void)
{
	begin(_ach_level_a10, _ach_normal);
	ach_rules_taint();
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 0, "a typed command");
	CHECK(!ach_rules_enabled(), "off for the level");
	begin(_ach_level_a10, _ach_normal);
	ach_rules_checkpoint_save();
	ach_rules_taint();
	ach_rules_checkpoint_load();
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 0, "a revert does not clear it");
}

static void case_difficulty(void)
{
	begin(_ach_level_a10, _ach_normal);
	ach_rules_won(-1);
	CHECK(got(_ach_walk_it_off) == 0, "Walk it Off is Heroic and higher");
	CHECK(got(_ach_standard_operating_procedure) == 1, "normal, no packs");
	begin(_ach_level_a10, _ach_heroic);
	ach_rules_won(-1);
	CHECK(got(_ach_walk_it_off) == 1, "heroic");
	CHECK(got(_ach_shielded) == 0, "Shielded is Legendary");
	begin(_ach_level_a10, _ach_legendary);
	ach_rules_won(-1);
	CHECK(got(_ach_shielded) == 1, "legendary, shield whole");
	begin(_ach_level_a10, _ach_legendary);
	ach_rules_player_damaged(0, 0, 1);
	ach_rules_won(-1);
	CHECK(got(_ach_shielded) == 0, "shield broke");
	begin(_ach_level_a10, _ach_easy);
	ach_rules_won(-1);
	CHECK(got(_ach_standard_operating_procedure) == 0, "not on Easy");
}

static void case_health_packs(void)
{
	begin(_ach_level_c40, _ach_heroic);
	ach_rules_powerup(0, _ach_powerup_health);
	ach_rules_won(-1);
	CHECK(got(_ach_cheap_thrills) == 0, "a pack");
	CHECK(got(_ach_standard_operating_procedure) == 0, "a pack");
	begin(_ach_level_c40, _ach_heroic);
	ach_rules_powerup(0, _ach_powerup_overshield);
	ach_rules_won(-1);
	CHECK(got(_ach_cheap_thrills) == 1, "an overshield is no pack");
}

static void case_revert(void)
{
	begin(_ach_level_c40, _ach_heroic);
	ach_rules_checkpoint_save();
	ach_rules_powerup(0, _ach_powerup_health);
	ach_rules_vehicle_entered(0, _ach_vehicle_banshee);
	ach_rules_checkpoint_load();
	ach_rules_won(-1);
	CHECK(got(_ach_cheap_thrills) == 1, "the pack was lost with the segment");
	CHECK(got(_ach_mind_the_gap) == 1, "so was the banshee");
	begin(_ach_level_c40, _ach_heroic);
	ach_rules_powerup(0, _ach_powerup_health);
	ach_rules_checkpoint_save();
	ach_rules_checkpoint_load();
	ach_rules_won(-1);
	CHECK(got(_ach_cheap_thrills) == 0, "a pack before the checkpoint stays");
}

static void case_deaths_latched(void)
{
	begin(_ach_level_c20, _ach_heroic);
	ach_rules_checkpoint_save();
	ach_rules_player_died(0);
	ach_rules_checkpoint_load();
	ach_rules_won(-1);
	CHECK(got(_ach_that_just_happened) == 0, "a death is not undone by its revert");
	begin(_ach_level_c20, _ach_heroic);
	ach_rules_won(-1);
	CHECK(got(_ach_that_just_happened) == 1, "no death");
	begin(_ach_level_c20, _ach_heroic);
	ach_rules_checkpoint_save();
	ach_rules_player_died(0);
	ach_rules_new_map(1, _ach_level_c20, _ach_heroic);
	ach_rules_player_begin(0, 1);
	ach_rules_won(-1);
	CHECK(got(_ach_that_just_happened) == 1, "a restarted level starts again");
}

static void case_time_strict(void)
{
	begin(_ach_level_b40, _ach_heroic);
	ticks(30 * 60 * 30);
	ach_rules_won(-1);
	CHECK(got(_ach_speed_reader) == 1, "exactly 30 minutes");
	begin(_ach_level_b40, _ach_heroic);
	ticks(30 * 60 * 30 + 1);
	ach_rules_won(-1);
	CHECK(got(_ach_speed_reader) == 0, "a tick over");
	begin(_ach_level_b40, _ach_heroic);
	ach_rules_checkpoint_save();
	ticks(30 * 60 * 20);
	ach_rules_checkpoint_load();
	ticks(30 * 60 * 20);
	ach_rules_won(-1);
	CHECK(got(_ach_speed_reader) == 0, "the lost segment's time still counts");
	begin(_ach_level_c20, _ach_heroic);
	ach_rules_won(-1);
	begin(_ach_level_c20, _ach_legendary);
	ticks(54000);
	ach_rules_won(-1);
	CHECK(got(_ach_read_the_history_books) == 1, "c20 legendary in 30 minutes");
	begin(_ach_level_d20, _ach_heroic);
	ticks(27000);
	ach_rules_won(-1);
	CHECK(got(_ach_what_back_already) == 1, "d20 in 15 minutes");
	begin(_ach_level_d20, _ach_heroic);
	ticks(27001);
	ach_rules_won(-1);
	CHECK(got(_ach_what_back_already) == 0, "d20 too slow");
}

static void case_maw_timer(void)
{
	begin(_ach_level_d40, _ach_legendary);
	ach_rules_won(1800);
	CHECK(got(_ach_never_tell_me_the_odds) == 1, "a minute left");
	begin(_ach_level_d40, _ach_legendary);
	ach_rules_won(1799);
	CHECK(got(_ach_never_tell_me_the_odds) == 0, "59 seconds left");
	begin(_ach_level_d40, _ach_heroic);
	ach_rules_won(3000);
	CHECK(got(_ach_never_tell_me_the_odds) == 0, "Legendary only");
}

static void case_resumed(void)
{
	begin(_ach_level_a10, _ach_heroic);
	ach_rules_resumed();
	ach_rules_won(-1);
	CHECK(got(_ach_birth_of_a_spartan) == 1, "the win counts");
	CHECK(got(_ach_walk_it_off) == 0, "the challenge has no history");
	CHECK(got(_ach_standard_operating_procedure) == 0, "none either");
	CHECK(got(_ach_feet_firmly_planted) == 0, "none either");
	CHECK(got(_ach_speed_reader) == 0, "another level's");
}

static void case_campaign(void)
{
	int level;

	memset(&saved_profile, 0, sizeof(saved_profile));
	for (level = 0; level < 10; level++)
	{
		begin(level, level == 9 ? _ach_legendary : _ach_heroic);
		*ach_rules_profile(0) = saved_profile;
		ach_rules_player_begin(0, 1);
		ach_rules_won(-1);
		CHECK(level == 9 || got(_ach_believe_in_a_hero) == 0, "not before the last");
		saved_profile = *ach_rules_profile(0);
	}
	CHECK(got(_ach_believe_in_a_hero) == 1, "ten heroic wins");
	CHECK(got(_ach_living_legend) == 0, "one legendary");
	for (level = 0; level < 10; level++)
	{
		begin(level, _ach_legendary);
		*ach_rules_profile(0) = saved_profile;
		ach_rules_player_begin(0, 1);
		ach_rules_won(-1);
		saved_profile = *ach_rules_profile(0);
		/* (all ten have a legendary win once the ninth in order is won, level 9's being from before) */
		CHECK(level < 8 ? !ach_test(saved_profile.unlocked, _ach_living_legend) : got(_ach_living_legend) || level == 9,
			"level %d", level);
	}
	CHECK(ach_test(saved_profile.unlocked, _ach_living_legend), "ten legendary wins");
}

static void case_coop(void)
{
	int level;

	begin(_ach_level_a10, _ach_normal);
	ach_rules_set_humans(2);
	ach_rules_won(-1);
	CHECK(got(_ach_bro_slayer) == 1, "two humans");
	begin(_ach_level_a10, _ach_normal);
	ach_rules_won(-1);
	CHECK(got(_ach_bro_slayer) == 0, "one human");
	for (level = 0; level < 10; level++)
	{
		struct ach_profile keep = ach.profile[0];

		begin(level, _ach_easy);
		ach.profile[0] = keep;
		ach_rules_player_begin(0, 1);
		ach_rules_set_humans(2);
		ach_rules_won(-1);
		CHECK(level == 9 || got(_ach_together_again) == 0, "not before the last");
	}
	CHECK(got(_ach_together_again) == 1, "all ten levels in co-op");
}

static void case_counts(void)
{
	int index;

	begin(_ach_level_d20, _ach_heroic);
	for (index = 0; index < 49; index++)
		ach_rules_kill(0, _ach_victim_infection_form, index, _ach_damage_shotgun_pellet, 0, 0);
	CHECK(got(_ach_popcorn) == 0, "49");
	ach_rules_kill(0, _ach_victim_infection_form, 99, _ach_damage_shotgun_pellet, 0, 0);
	CHECK(got(_ach_popcorn) == 1, "50");
	begin(_ach_level_d20, _ach_easy);
	for (index = 0; index < 60; index++)
		ach_rules_kill(0, _ach_victim_infection_form, index, _ach_damage_shotgun_pellet, 0, 0);
	CHECK(got(_ach_popcorn) == 0, "Heroic and higher");
	begin(_ach_level_d20, _ach_heroic);
	for (index = 0; index < 60; index++)
		ach_rules_kill(0, _ach_victim_infection_form, index, _ach_damage_other, 0, 0);
	CHECK(got(_ach_popcorn) == 0, "only the shotgun's");
	begin(_ach_level_b40, _ach_easy);
	for (index = 0; index < 19; index++)
		ach_rules_kill(0, _ach_victim_other, index, _ach_damage_rocket_explosion, 0, 0);
	CHECK(got(_ach_heavy_artillery) == 0, "19 rockets");
	ach_rules_kill(0, _ach_victim_other, 50, _ach_damage_rocket_explosion, 0, 0);
	CHECK(got(_ach_heavy_artillery) == 1, "20 rockets");
	begin(_ach_level_b40, _ach_easy);
	for (index = 0; index < 4; index++)
		ach_rules_powerup(0, _ach_powerup_overshield);
	CHECK(got(_ach_overshielded) == 0, "4 overshields");
	ach_rules_powerup(0, _ach_powerup_overshield);
	CHECK(got(_ach_overshielded) == 1, "5 overshields");
	begin(_ach_level_b40, _ach_easy);
	ach_rules_checkpoint_save();
	for (index = 0; index < 4; index++)
		ach_rules_powerup(0, _ach_powerup_overshield);
	ach_rules_checkpoint_load();
	ach_rules_powerup(0, _ach_powerup_overshield);
	CHECK(got(_ach_overshielded) == 0, "the four were lost with the segment");
}

static void case_lifetime(void)
{
	int index;

	begin(_ach_level_a10, _ach_easy);
	for (index = 0; index < 48; index++)
		ach_rules_kill(0, _ach_victim_other, index, _ach_damage_pistol, _ach_kill_headshot, 0);
	ach_rules_checkpoint_fold();
	ach_rules_checkpoint_save();
	ach_rules_kill(0, _ach_victim_other, 100, _ach_damage_sniper, _ach_kill_headshot, 0);
	ach_rules_checkpoint_load();
	ach_rules_kill(0, _ach_victim_other, 101, _ach_damage_sniper, _ach_kill_headshot, 0);
	CHECK(got(_ach_precision_shooting) == 0, "49: the one before the revert is not counted twice");
	ach_rules_kill(0, _ach_victim_other, 102, _ach_damage_pistol, _ach_kill_headshot, 0);
	CHECK(got(_ach_precision_shooting) == 1, "50");
	begin(_ach_level_a10, _ach_easy);
	for (index = 0; index < 60; index++)
		ach_rules_kill(0, _ach_victim_other, index, _ach_damage_other, _ach_kill_headshot, 0);
	CHECK(got(_ach_precision_shooting) == 0, "only the pistol's and the sniper's");
	begin(_ach_level_a10, _ach_easy);
	for (index = 0; index < 9; index++)
		ach_rules_kill(0, _ach_victim_other, index, _ach_damage_melee, _ach_kill_assassination, 0);
	CHECK(got(_ach_wrap_it_up) == 0, "9 assassinations");
	ach_rules_kill(0, _ach_victim_other, 9, _ach_damage_melee, _ach_kill_assassination, 0);
	CHECK(got(_ach_wrap_it_up) == 1, "10");
	begin(_ach_level_a10, _ach_easy);
	for (index = 0; index < 19; index++)
		ach_rules_stuck(0);
	CHECK(got(_ach_sticky_situation) == 0, "19 sticks");
	ach_rules_stuck(0);
	CHECK(got(_ach_sticky_situation) == 1, "20");
	/* the counts carry over in the profile, across two levels */
	begin(_ach_level_a10, _ach_easy);
	for (index = 0; index < 10; index++)
		ach_rules_stuck(0);
	ach_rules_won(-1);
	{
		struct ach_profile keep = *ach_rules_profile(0);

		ach_rules_new_map(1, _ach_level_a30, _ach_easy);
		*ach_rules_profile(0) = keep;
		ach_rules_player_begin(0, 1);
	}
	for (index = 0; index < 9; index++)
		ach_rules_stuck(0);
	CHECK(unlock_count[0][_ach_sticky_situation] == 0, "19 over two levels");
	ach_rules_stuck(0);
	CHECK(unlock_count[0][_ach_sticky_situation] == 1, "20 over two levels");
}

static void case_weapons(void)
{
	begin(_ach_level_b30, _ach_heroic);
	ach_rules_won(-1);
	CHECK(got(_ach_grenadier) == 1, "no shot");
	CHECK(got(_ach_pacifist) == 1, "nothing at all");
	begin(_ach_level_b30, _ach_heroic);
	ach_rules_weapon_fired(0, _ach_weapon_other, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_grenadier) == 0, "a shot");
	CHECK(got(_ach_pacifist) == 0, "a shot");
	begin(_ach_level_b30, _ach_heroic);
	ach_rules_grenade_thrown(0);
	ach_rules_won(-1);
	CHECK(got(_ach_grenadier) == 1, "grenades are allowed");
	CHECK(got(_ach_pacifist) == 0, "but not Pacifist's");
	begin(_ach_level_a50, _ach_heroic);
	ach_rules_weapon_fired(0, _ach_weapon_other, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_close_quarters) == 1, "unscoped");
	begin(_ach_level_a50, _ach_heroic);
	ach_rules_weapon_fired(0, _ach_weapon_other, 1);
	ach_rules_won(-1);
	CHECK(got(_ach_close_quarters) == 0, "scoped");
	begin(_ach_level_c10, _ach_heroic);
	ach_rules_weapon_fired(0, _ach_weapon_other, 0);
	ach_rules_weapon_picked(0, _ach_weapon_shotgun);
	ach_rules_weapon_fired(0, _ach_weapon_shotgun, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_bug_spray) == 1, "shots before the shotgun do not count");
	begin(_ach_level_c10, _ach_heroic);
	ach_rules_weapon_picked(0, _ach_weapon_shotgun);
	ach_rules_weapon_fired(0, _ach_weapon_other, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_bug_spray) == 0, "another weapon after it");
	begin(_ach_level_c10, _ach_heroic);
	ach_rules_won(-1);
	CHECK(got(_ach_bug_spray) == 0, "the shotgun must have been had");
	begin(_ach_level_d40, _ach_heroic);
	ach_rules_won(-1);
	CHECK(got(_ach_leave_it_where_it_lay) == 1, "no pickups");
	begin(_ach_level_d40, _ach_heroic);
	ach_rules_weapon_picked(0, _ach_weapon_other);
	ach_rules_won(-1);
	CHECK(got(_ach_leave_it_where_it_lay) == 0, "a pickup");
	begin(_ach_level_a30, _ach_heroic);
	ach_rules_vehicle_entered(0, _ach_vehicle_warthog);
	ach_rules_won(-1);
	CHECK(got(_ach_scunt) == 0, "a vehicle");
	begin(_ach_level_a10, _ach_normal);
	ach_rules_jumped(0);
	ach_rules_won(-1);
	CHECK(got(_ach_feet_firmly_planted) == 0, "a jump");
}

static void case_damage(void)
{
	begin(_ach_level_a30, _ach_legendary);
	ach_rules_player_damaged(0, 0, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_how_can_i_hit_you) == 1, "shield damage only");
	begin(_ach_level_a30, _ach_legendary);
	ach_rules_player_damaged(0, 1, 1);
	ach_rules_won(-1);
	CHECK(got(_ach_how_can_i_hit_you) == 0, "body damage");
	begin(_ach_level_a30, _ach_heroic);
	ach_rules_player_damaged(0, 1, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_how_can_i_hit_you) == 0, "Legendary only");
	begin(_ach_level_b40, _ach_heroic);
	ach_rules_player_damaged(0, 1, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_ill_be_back) == 0, "b40 body damage");
	begin(_ach_level_c10, _ach_heroic);
	ach_rules_won(-1);
	CHECK(got(_ach_the_flood) == 1, "c10 undamaged");
}

static void case_stealth(void)
{
	begin(_ach_level_a50, _ach_heroic);
	ach_rules_stealth_poll(0, 0);
	ach_rules_stealth_poll(6, 1);
	ach_rules_stealth_poll(3, 1);
	ach_rules_stealth_poll(0, 0);
	CHECK(got(_ach_all_according_to_plan) == 1, "all dead, none alerted");
	begin(_ach_level_a50, _ach_heroic);
	ach_rules_stealth_poll(6, 1);
	ach_rules_stealth_poll(5, 2);
	ach_rules_stealth_poll(0, 0);
	CHECK(got(_ach_all_according_to_plan) == 0, "alerted");
	begin(_ach_level_a50, _ach_normal);
	ach_rules_stealth_poll(6, 1);
	ach_rules_stealth_poll(0, 0);
	CHECK(got(_ach_all_according_to_plan) == 0, "Heroic and higher");
	begin(_ach_level_a50, _ach_heroic);
	ach_rules_stealth_poll(2, 1);
	ach_rules_stealth_poll(0, 0);
	CHECK(got(_ach_all_according_to_plan) == 0, "too few ever seen to be the area");
	begin(_ach_level_a50, _ach_heroic);
	ach_rules_stealth_poll(6, 1);
	ach_rules_checkpoint_save();
	ach_rules_stealth_poll(6, 2);
	ach_rules_checkpoint_load();
	ach_rules_stealth_poll(0, 0);
	CHECK(got(_ach_all_according_to_plan) == 1, "the alert was lost with the segment");
}

static void case_beach(void)
{
	begin(_ach_level_b30, _ach_heroic);
	ach_rules_beach_poll(8);
	ach_rules_beach_poll(0);
	CHECK(got(_ach_beachhead) == 1, "landing cleared undamaged");
	begin(_ach_level_b30, _ach_heroic);
	ach_rules_beach_poll(8);
	ach_rules_player_damaged(0, 1, 0);
	ach_rules_beach_poll(0);
	CHECK(got(_ach_beachhead) == 0, "body damage on the beach");
	begin(_ach_level_b30, _ach_heroic);
	ach_rules_beach_poll(8);
	ach_rules_beach_poll(0);
	ach_rules_player_damaged(0, 1, 0);
	ach_rules_won(-1);
	CHECK(got(_ach_beachhead) == 1, "damage after the beach is no matter");
}

static void case_kills(void)
{
	begin(_ach_level_a10, _ach_easy);
	ach_rules_kill(0, _ach_victim_hunter, 7, _ach_damage_pistol, 0, 0);
	CHECK(got(_ach_you_are_the_weapon) == 1, "one round");
	begin(_ach_level_a10, _ach_easy);
	ach_rules_enemy_damaged(_ach_victim_hunter, 7);
	ach_rules_kill(0, _ach_victim_hunter, 7, _ach_damage_pistol, 0, 0);
	CHECK(got(_ach_you_are_the_weapon) == 0, "hurt before");
	ach_rules_kill(0, _ach_victim_hunter, 8, _ach_damage_other, 0, 0);
	CHECK(got(_ach_you_are_the_weapon) == 0, "another weapon");
	begin(_ach_level_a10, _ach_easy);
	ach_rules_set_humans(2);
	ach_rules_kill(0, _ach_victim_other, 1, _ach_damage_plasma_explosion, 0, 77);
	ticks(1);
	ach_rules_kill(0, _ach_victim_other, 2, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 3, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 4, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 5, _ach_damage_plasma_explosion, 0, 77);
	CHECK(got(_ach_tying_up_loose_ends) == 0, "the first was another tick's");
	ach_rules_kill(0, _ach_victim_other, 6, _ach_damage_plasma_explosion, 0, 77);
	CHECK(got(_ach_tying_up_loose_ends) == 1, "five of one explosion");
	begin(_ach_level_a10, _ach_easy);
	ach_rules_kill(0, _ach_victim_other, 1, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 2, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 3, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 4, _ach_damage_plasma_explosion, 0, 77);
	ach_rules_kill(0, _ach_victim_other, 5, _ach_damage_plasma_explosion, 0, 77);
	CHECK(got(_ach_tying_up_loose_ends) == 0, "alone");
}

static void case_riding(void)
{
	int index;

	begin(_ach_level_c10, _ach_easy);
	for (index = 0; index < ACH_PELICAN_TICKS - 1; index++)
		ach_rules_riding(0, 1, 0);
	CHECK(got(_ach_get_me_out_of_here) == 0, "4.97 seconds");
	ach_rules_riding(0, 1, 0);
	CHECK(got(_ach_get_me_out_of_here) == 1, "5 seconds");
	begin(_ach_level_b30, _ach_easy);
	for (index = 0; index < 200; index++)
		ach_rules_riding(0, 1, 0);
	CHECK(got(_ach_get_me_out_of_here) == 0, "only on 343 Guilty Spark");
	begin(_ach_level_c10, _ach_easy);
	for (index = 0; index < 200; index++)
		ach_rules_riding(0, index % 100 != 99, 0);
	CHECK(got(_ach_get_me_out_of_here) == 0, "the ride must not be broken");
	begin(_ach_level_b30, _ach_easy);
	for (index = 0; index < ACH_AIRBORNE_TICKS; index++)
		ach_rules_riding(0, 0, 1);
	ach_rules_splatter(0);
	CHECK(got(_ach_airborne) == 1, "5 seconds in the air");
	begin(_ach_level_b30, _ach_easy);
	for (index = 0; index < ACH_AIRBORNE_TICKS - 1; index++)
		ach_rules_riding(0, 0, 1);
	ach_rules_splatter(0);
	CHECK(got(_ach_airborne) == 0, "too short");
	begin(_ach_level_b30, _ach_easy);
	for (index = 0; index < ACH_AIRBORNE_TICKS; index++)
		ach_rules_riding(0, 0, 1);
	for (index = 0; index < ACH_AIRBORNE_GRACE_TICKS; index++)
		ach_rules_riding(0, 0, 0);
	ach_rules_splatter(0);
	CHECK(got(_ach_airborne) == 1, "just landed");
	for (index = 0; index < 2; index++)
		ach_rules_riding(0, 0, 0);
	begin(_ach_level_b30, _ach_easy);
	for (index = 0; index < ACH_AIRBORNE_TICKS; index++)
		ach_rules_riding(0, 0, 1);
	for (index = 0; index < ACH_AIRBORNE_GRACE_TICKS + 1; index++)
		ach_rules_riding(0, 0, 0);
	ach_rules_splatter(0);
	CHECK(got(_ach_airborne) == 0, "landed long ago");
}

static void case_day(void)
{
	begin(_ach_level_a10, _ach_easy);
	ach_rules_day(0, 14);
	CHECK(got(_ach_a_day_to_remember) == 0, "the 14th");
	begin(_ach_level_a10, _ach_easy);
	ach_rules_day(0, 15);
	CHECK(got(_ach_a_day_to_remember) == 1, "the 15th");
}

static void case_file(void)
{
	struct ach_profile profile;
	struct ach_profile read;
	unsigned char bytes[ACH_FILE_SIZE];
	int index;

	memset(&profile, 0, sizeof(profile));
	profile.unlocked[0] = 0x80000005u;
	profile.unlocked[1] = 0x3F;
	profile.unlock_time[0] = 123456;
	profile.unlock_time[37] = 77;
	profile.won[3] = 0xD;
	profile.coop_won = 0x2A5;
	profile.headshots = 49;
	profile.assassinations = 3;
	profile.sticks = 19;
	ach_rules_profile_write(&profile, bytes);
	CHECK(bytes[0] == 'O' && bytes[1] == 'C' && bytes[2] == 'E' && bytes[3] == 'A', "the magic is bytes");
	memset(&read, 0, sizeof(read));
	CHECK(ach_rules_profile_read(&read, bytes, ACH_FILE_SIZE), "reads back");
	CHECK(read.unlocked[0] == profile.unlocked[0] && read.unlocked[1] == profile.unlocked[1], "unlocked");
	CHECK(read.unlock_time[0] == 123456 && read.unlock_time[37] == 77, "times");
	CHECK(read.won[3] == 0xD && read.coop_won == 0x2A5, "wins");
	CHECK(read.headshots == 49 && read.assassinations == 3 && read.sticks == 19, "counts");
	for (index = 0; index < ACH_FILE_SIZE; index++)
	{
		unsigned char copy[ACH_FILE_SIZE];

		memcpy(copy, bytes, sizeof(copy));
		copy[index] ^= 0x10;
		CHECK(!ach_rules_profile_read(&read, copy, ACH_FILE_SIZE), "a flipped byte %d is refused", index);
	}
	CHECK(!ach_rules_profile_read(&read, bytes, ACH_FILE_SIZE - 1), "short");
	CHECK(!ach_rules_profile_read(&read, bytes, ACH_FILE_SIZE + 1), "long");
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("birth") { case_birth(); return 0; }
	CASE("off") { case_off(); return 0; }
	CASE("taint") { case_taint(); return 0; }
	CASE("difficulty") { case_difficulty(); return 0; }
	CASE("health-packs") { case_health_packs(); return 0; }
	CASE("revert") { case_revert(); return 0; }
	CASE("deaths-latched") { case_deaths_latched(); return 0; }
	CASE("time-strict") { case_time_strict(); return 0; }
	CASE("maw-timer") { case_maw_timer(); return 0; }
	CASE("resumed") { case_resumed(); return 0; }
	CASE("campaign") { case_campaign(); return 0; }
	CASE("coop") { case_coop(); return 0; }
	CASE("counts") { case_counts(); return 0; }
	CASE("lifetime") { case_lifetime(); return 0; }
	CASE("weapons") { case_weapons(); return 0; }
	CASE("damage") { case_damage(); return 0; }
	CASE("stealth") { case_stealth(); return 0; }
	CASE("beach") { case_beach(); return 0; }
	CASE("kills") { case_kills(); return 0; }
	CASE("riding") { case_riding(); return 0; }
	CASE("day") { case_day(); return 0; }
	CASE("file") { case_file(); return 0; }
	fprintf(stderr, "unknown case %s\n", case_name);
	return 2;
}
