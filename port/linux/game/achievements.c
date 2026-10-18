/*
ACHIEVEMENTS.C

Optional achievements (the game.achievements setting, off by default; see
port/linux/README.md, "Achievements"). The campaign's 38, kept per player
profile in the profile's own folder (achieve.bin), so deleting a profile
deletes them.

The file has two parts. The first, the rules, knows nothing of the game:
events with small integers in, unlocks out, which a test compiles by itself
(ACHIEVEMENTS_RULES_ONLY, tools/harness/tests/achievements.c). The second
reads the game's objects, turns what happens into those events, keeps the
profile files, and draws the notice of an unlock.

What the rules keep of a level is private to this file; it is never in the
game state, so saved games and the netcode do not change, and nothing here
is sent to another machine. Only a machine that decides the game (a
single-player game, or the host of a co-op one) records anything, and only
for its own local players. Every entry point returns first thing when
achievements are off.

A checkpoint saved copies the counts, and a checkpoint loaded (a revert)
puts them back, as the game puts back its own state: damage taken, health
packs, shots, and the kills counted since are forgotten with the segment
that is lost. What a revert does not put back: the deaths (a death is why
there was a revert), and whether the level is disqualified. The time is
strict: the ticks of a lost segment still count.

Disqualified for the level: any typed command (the console, telnet, a cheat
button, init.txt), a cheat flag, a debug core loaded. A level resumed from a
saved game has no history, so only its win counts, not its challenges.
*/

#include <stddef.h>
#include <string.h>

/* ---------- the rules */

#define ACH_PLAYERS 4
#define ACH_LEVELS 10

/* the order is the file's: new ones go at the end */
enum
{
	_ach_birth_of_a_spartan,
	_ach_believe_in_a_hero,
	_ach_living_legend,
	_ach_standard_operating_procedure,
	_ach_walk_it_off,
	_ach_shielded,
	_ach_scunt,
	_ach_how_can_i_hit_you,
	_ach_all_according_to_plan,
	_ach_close_quarters,
	_ach_beachhead,
	_ach_grenadier,
	_ach_speed_reader,
	_ach_ill_be_back,
	_ach_the_flood,
	_ach_bug_spray,
	_ach_that_just_happened,
	_ach_read_the_history_books,
	_ach_mind_the_gap,
	_ach_cheap_thrills,
	_ach_popcorn,
	_ach_what_back_already,
	_ach_leave_it_where_it_lay,
	_ach_never_tell_me_the_odds,
	_ach_get_me_out_of_here,
	_ach_you_are_the_weapon,
	_ach_feet_firmly_planted,
	_ach_pacifist,
	_ach_overshielded,
	_ach_precision_shooting,
	_ach_wrap_it_up,
	_ach_sticky_situation,
	_ach_heavy_artillery,
	_ach_bro_slayer,
	_ach_together_again,
	_ach_tying_up_loose_ends,
	_ach_airborne,
	_ach_a_day_to_remember,
	NUMBER_OF_ACHIEVEMENTS,
	ACH_FILE_CAPACITY = 40
};

/* levels */
enum
{
	_ach_level_a10, _ach_level_a30, _ach_level_a50, _ach_level_b30, _ach_level_b40,
	_ach_level_c10, _ach_level_c20, _ach_level_c40, _ach_level_d20, _ach_level_d40
};

/* difficulties (the game's) */
enum
{
	_ach_easy, _ach_normal, _ach_heroic, _ach_legendary
};

/* what a victim, a damage, a weapon or a vehicle is, by tag name (the game side
chooses) */
enum
{
	_ach_victim_other,
	_ach_victim_infection_form,
	_ach_victim_hunter
};

enum
{
	_ach_damage_other,
	_ach_damage_pistol,
	_ach_damage_sniper,
	_ach_damage_shotgun_pellet,
	_ach_damage_rocket_explosion,
	_ach_damage_plasma_explosion,
	_ach_damage_melee
};

enum
{
	_ach_weapon_other,
	_ach_weapon_shotgun
};

enum
{
	_ach_vehicle_other,
	_ach_vehicle_banshee,
	_ach_vehicle_pelican,
	_ach_vehicle_warthog
};

enum
{
	_ach_powerup_health,
	_ach_powerup_overshield
};

/* ach_rules_kill's flags */
enum
{
	_ach_kill_headshot = 1,
	_ach_kill_assassination = 2
};

/* the tick rate; the times of the speed achievements */
#define ACH_TICKS_PER_SECOND 30
#define ACH_PELICAN_TICKS (5 * ACH_TICKS_PER_SECOND)
#define ACH_AIRBORNE_TICKS (5 * ACH_TICKS_PER_SECOND)
#define ACH_AIRBORNE_GRACE_TICKS ACH_TICKS_PER_SECOND
#define ACH_STEALTH_MINIMUM_ENEMIES 3
#define ACH_DAMAGED_HUNTERS 16

#define ACH_FILE_MAGIC 0x4145434Fu /* "OCEA" read as bytes */
#define ACH_FILE_VERSION 1
#define ACH_FILE_SIZE 204

typedef unsigned int ach_u32;

struct ach_definition
{
	char const *name;
	char const *description;
	signed char level; /* -1: any */
	signed char minimum_difficulty;
	/* the rule is about the whole level, so a level resumed from a saved game
	cannot earn it */
	signed char whole_level;
	signed char counted_in_file; /* shown with a count */
};

/* the user's names and descriptions; the order is the enum's */
static struct ach_definition const ach_definitions[NUMBER_OF_ACHIEVEMENTS] =
{
	{ "Birth of a Spartan", "Complete the level Pillar of Autumn on any difficulty.", _ach_level_a10, _ach_easy, 0, 0 },
	{ "Believe in a Hero", "Complete the campaign on Heroic difficulty.", -1, _ach_heroic, 0, 0 },
	{ "Living Legend", "Complete the campaign on Legendary difficulty.", -1, _ach_legendary, 0, 0 },
	{ "Standard Operating Procedure", "Complete any level on Normal difficulty or higher without using health packs.", -1, _ach_normal, 1, 0 },
	{ "Walk it Off", "Complete Pillar of Autumn on Heroic difficulty or higher without picking up a health pack.", _ach_level_a10, _ach_heroic, 1, 0 },
	{ "Shielded", "Complete Pillar of Autumn on Legendary without losing your shield.", _ach_level_a10, _ach_legendary, 1, 0 },
	{ "Scunt", "Complete Halo on Heroic difficulty or higher without entering a vehicle.", _ach_level_a30, _ach_heroic, 1, 0 },
	{ "How Can I Hit You If You Won't Move?", "Complete Halo on Legendary difficulty without taking any health damage.", _ach_level_a30, _ach_legendary, 1, 0 },
	{ "All According to Plan", "On Truth and Reconciliation, kill all enemies in the boarding area without being detected on Heroic or higher.", _ach_level_a50, _ach_heroic, 1, 0 },
	{ "Close Quarters", "Complete Truth and Reconciliation on Heroic or higher without firing a scoped weapon.", _ach_level_a50, _ach_heroic, 1, 0 },
	{ "Beachhead", "Complete The Silent Cartographer on Heroic or higher without taking body damage during the beach landing.", _ach_level_b30, _ach_heroic, 1, 0 },
	{ "Grenadier", "Complete The Silent Cartographer on Heroic or higher using only grenades and melee attacks.", _ach_level_b30, _ach_heroic, 1, 0 },
	{ "Speed Reader", "Complete Assault on the Control Room in under 30 minutes on Heroic or higher.", _ach_level_b40, _ach_heroic, 0, 0 },
	{ "I'll Be Back", "Complete Assault on the Control Room on Heroic or higher without taking health damage.", _ach_level_b40, _ach_heroic, 1, 0 },
	{ "The Flood", "Complete 343 Guilty Spark on Heroic or higher without taking health damage.", _ach_level_c10, _ach_heroic, 1, 0 },
	{ "Bug Spray", "Complete 343 Guilty Spark on Heroic or higher using only the Shotgun.", _ach_level_c10, _ach_heroic, 1, 0 },
	{ "That Just Happened", "Complete The Library on Heroic or higher without dying.", _ach_level_c20, _ach_heroic, 1, 0 },
	{ "Read the History Books", "Complete The Library on Legendary difficulty in under 30 minutes.", _ach_level_c20, _ach_legendary, 0, 0 },
	{ "Mind the Gap", "Complete Two Betrayals on Heroic or higher without using a Banshee.", _ach_level_c40, _ach_heroic, 1, 0 },
	{ "Cheap Thrills", "Complete Two Betrayals on Heroic or higher without using health packs.", _ach_level_c40, _ach_heroic, 1, 0 },
	{ "Popcorn", "On Keyes, kill 50 Infection Forms with the Shotgun on Heroic or higher.", _ach_level_d20, _ach_heroic, 1, 0 },
	{ "What? Back Already?", "Complete Keyes on Heroic or higher in under 15 minutes.", _ach_level_d20, _ach_heroic, 0, 0 },
	{ "Leave It Where It Lay", "Complete The Maw on Heroic or higher without picking up any weapons.", _ach_level_d40, _ach_heroic, 1, 0 },
	{ "Never Tell Me the Odds", "Escape The Maw on Legendary with at least 1 minute remaining on the countdown.", _ach_level_d40, _ach_legendary, 0, 0 },
	{ "Get Me Out of Here!", "Ride in a Pelican for several seconds on 343 Guilty Spark.", _ach_level_c10, _ach_easy, 0, 0 },
	{ "You Are The Weapon", "Kill a Hunter with a single Pistol round.", -1, _ach_easy, 0, 0 },
	{ "Feet Firmly Planted", "Complete a level without jumping.", -1, _ach_easy, 1, 0 },
	{ "Pacifist", "Complete The Silent Cartographer without firing a weapon, throwing a grenade, using melee or dying.", _ach_level_b30, _ach_easy, 1, 0 },
	{ "Overshielded", "Pick up 5 Overshields in a single level.", -1, _ach_easy, 0, 0 },
	{ "Precision Shooting", "Get 50 headshots with the Pistol or Sniper Rifle.", -1, _ach_easy, 0, 1 },
	{ "Wrap It Up", "Perform 10 assassinations (melee from behind).", -1, _ach_easy, 0, 1 },
	{ "Sticky Situation", "Stick 20 enemies with Plasma Grenades.", -1, _ach_easy, 0, 1 },
	{ "Heavy Artillery", "Kill 20 enemies with the Rocket Launcher in a single level.", -1, _ach_easy, 0, 0 },
	{ "Bro Slayer", "Complete any level in co-op mode.", -1, _ach_easy, 0, 0 },
	{ "Together Again", "Complete the entire campaign in co-op mode.", -1, _ach_easy, 0, 0 },
	{ "Tying Up Loose Ends", "Kill 5 enemies with a single Plasma Grenade in co-op.", -1, _ach_easy, 0, 0 },
	{ "Airborne", "Launch a Warthog into the air for 5 seconds and splatter an enemy with it.", -1, _ach_easy, 0, 0 },
	{ "A Day To Remember", "Play the game on the 15th of a month.", -1, _ach_easy, 0, 0 }
};

/* what a profile keeps for good */
struct ach_profile
{
	int usable; /* a profile folder to keep it in */
	int dirty;
	ach_u32 unlocked[2];
	ach_u32 unlock_time[ACH_FILE_CAPACITY];
	unsigned char won[ACH_LEVELS]; /* a bit a difficulty */
	unsigned short coop_won; /* a bit a level */
	ach_u32 headshots;
	ach_u32 assassinations;
	ach_u32 sticks;
};

/* what a player has done in the level, kept and put back with the checkpoint */
struct ach_run
{
	int health_damage; /* times the body took damage */
	int shield_broken;
	int health_packs;
	int overshields;
	int vehicles_entered;
	int banshees_entered;
	int shots;
	int scoped_shots;
	int shots_after_shotgun; /* of any weapon but the shotgun, once it was had */
	int shotgun_owned;
	int weapon_pickups;
	int jumped;
	int melee;
	int grenades;
	int infection_forms_shotgunned;
	int rocket_kills;
	/* since the last checkpoint, which put them in the profile's counts */
	int headshots;
	int assassinations;
	int sticks;
	int pelican_ticks;
	int airborne_ticks;
	int airborne_grace;
	/* one plasma grenade's kills (the tick and the place of its explosion) */
	ach_u32 explosion_tick;
	ach_u32 explosion_key;
	int explosion_kills;
};

/* what the players share, put back too */
struct ach_shared_run
{
	/* all according to plan: the group, once seen, until it is dead */
	int stealth_peak;
	int stealth_detected;
	int stealth_done;
	/* beachhead: the landing's group, the body damage since the level began */
	int beach_peak;
	int beach_damaged;
	int beach_done;
	int damaged_hunters[ACH_DAMAGED_HUNTERS];
	int damaged_hunter_next;
};

/* what a level keeps, the latched part not put back */
struct ach_player
{
	int live; /* a local player of a profile that keeps its file */
	int deaths;
	int previous_state; /* the last tick's (the game side's) */
	int day_checked;
};

static struct
{
	int enabled;
	int level; /* -1 not a campaign level */
	int difficulty;
	int tainted;
	int history_incomplete;
	int won;
	int humans;
	ach_u32 level_ticks;
	struct ach_player player[ACH_PLAYERS];
	struct ach_run run[ACH_PLAYERS];
	struct ach_run saved_run[ACH_PLAYERS];
	struct ach_shared_run shared;
	struct ach_shared_run saved_shared;
	int saved;
	struct ach_profile profile[ACH_PLAYERS];
} ach;

/* the game side (or a test) answers these */
void ach_platform_unlocked(int local_player, int achievement);
void ach_platform_save(int local_player);
void ach_platform_log(char const *format, ...);
ach_u32 ach_platform_time(void);

static int ach_test(ach_u32 const *bits, int index)
{
	return (bits[index >> 5] >> (index & 31)) & 1;
}

static int ach_unlocked(int local_player, int achievement)
{
	return ach_test(ach.profile[local_player].unlocked, achievement);
}

/* a player the rules count: achievements on, the level not disqualified, a
profile to keep the results in */
static int ach_counting(int local_player)
{
	return ach.enabled && !ach.tainted && local_player >= 0 && local_player < ACH_PLAYERS &&
		ach.player[local_player].live;
}

static void ach_unlock(int local_player, int achievement)
{
	struct ach_profile *profile = &ach.profile[local_player];

	if (ach_unlocked(local_player, achievement))
		return;
	profile->unlocked[achievement >> 5] |= 1u << (achievement & 31);
	profile->unlock_time[achievement] = ach_platform_time();
	profile->dirty = 1;
	ach_platform_log("achievements: player %d unlocked %s", local_player, ach_definitions[achievement].name);
	ach_platform_unlocked(local_player, achievement);
	ach_platform_save(local_player);
}

static int ach_difficulty_at_least(int difficulty)
{
	return ach.difficulty >= difficulty;
}

/* a rule of a level (-1 any) at a difficulty, and, if it is about the whole
level, with its history known */
static int ach_applies(int achievement)
{
	struct ach_definition const *definition = &ach_definitions[achievement];

	if (definition->level != -1 && definition->level != ach.level)
		return 0;
	if (!ach_difficulty_at_least(definition->minimum_difficulty))
		return 0;
	if (definition->whole_level && ach.history_incomplete)
		return 0;
	return 1;
}

static void ach_clear_run(void)
{
	memset(ach.run, 0, sizeof(ach.run));
	memset(&ach.shared, 0, sizeof(ach.shared));
	memset(ach.saved_run, 0, sizeof(ach.saved_run));
	memset(&ach.saved_shared, 0, sizeof(ach.saved_shared));
	ach.saved = 0;
}

/* a new map, the level a campaign level's index (or -1) and its difficulty;
the profiles are read again as the players are found */
void ach_rules_new_map(int enabled, int level, int difficulty)
{
	int local_player;

	ach.enabled = enabled && level >= 0 && level < ACH_LEVELS;
	ach.level = level;
	ach.difficulty = difficulty;
	ach.tainted = 0;
	ach.history_incomplete = 0;
	ach.won = 0;
	ach.humans = 1;
	ach.level_ticks = 0;
	memset(ach.player, 0, sizeof(ach.player));
	for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
		memset(&ach.profile[local_player], 0, sizeof(ach.profile[local_player]));
	ach_clear_run();
}

int ach_rules_enabled(void)
{
	return ach.enabled && !ach.tainted;
}

int ach_rules_level(void)
{
	return ach.level;
}

/* a local player's profile has been read (its file's contents are in
ach_rules_profile) and the player is now counted */
struct ach_profile *ach_rules_profile(int local_player)
{
	return &ach.profile[local_player];
}

void ach_rules_player_begin(int local_player, int usable)
{
	if (local_player < 0 || local_player >= ACH_PLAYERS)
		return;
	ach.profile[local_player].usable = usable;
	ach.player[local_player].live = usable;
}

void ach_rules_player_end(int local_player)
{
	if (local_player < 0 || local_player >= ACH_PLAYERS)
		return;
	ach.player[local_player].live = 0;
}

int ach_rules_player_live(int local_player)
{
	return local_player >= 0 && local_player < ACH_PLAYERS && ach.player[local_player].live;
}

/* the rest of the level cannot count */
void ach_rules_taint(void)
{
	if (ach.enabled && !ach.tainted)
	{
		ach.tainted = 1;
		ach_platform_log("achievements: this level no longer counts");
	}
}

void ach_rules_resumed(void)
{
	ach.history_incomplete = 1;
}

/* the checkpoint: counts copied, and put back by a revert, but for the
deaths and the disqualification */
void ach_rules_checkpoint_save(void)
{
	if (!ach.enabled)
		return;
	memcpy(ach.saved_run, ach.run, sizeof(ach.run));
	ach.saved_shared = ach.shared;
	ach.saved = 1;
}

void ach_rules_checkpoint_load(void)
{
	if (!ach.enabled || !ach.saved)
		return;
	memcpy(ach.run, ach.saved_run, sizeof(ach.run));
	ach.shared = ach.saved_shared;
}

/* the counts that go to the profile, with what was counted since the
checkpoint */
static void ach_check_counts(int local_player)
{
	struct ach_profile const *profile = &ach.profile[local_player];
	struct ach_run const *run = &ach.run[local_player];

	if (profile->headshots + (ach_u32)run->headshots >= 50)
		ach_unlock(local_player, _ach_precision_shooting);
	if (profile->assassinations + (ach_u32)run->assassinations >= 10)
		ach_unlock(local_player, _ach_wrap_it_up);
	if (profile->sticks + (ach_u32)run->sticks >= 20)
		ach_unlock(local_player, _ach_sticky_situation);
}

/* the counts of the profile take what the run counted since the checkpoint
(a checkpoint saved, or the win) */
static void ach_fold_counts(int local_player)
{
	struct ach_profile *profile = &ach.profile[local_player];
	struct ach_run *run = &ach.run[local_player];

	if (run->headshots || run->assassinations || run->sticks)
	{
		profile->headshots += (ach_u32)run->headshots;
		profile->assassinations += (ach_u32)run->assassinations;
		profile->sticks += (ach_u32)run->sticks;
		run->headshots = run->assassinations = run->sticks = 0;
		profile->dirty = 1;
	}
}

/* just before the checkpoint's copy: the counts are the profile's now */
void ach_rules_checkpoint_fold(void)
{
	int local_player;

	if (!ach.enabled)
		return;
	for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
	{
		if (ach_counting(local_player))
		{
			ach_fold_counts(local_player);
			if (ach.profile[local_player].dirty)
				ach_platform_save(local_player);
		}
	}
}

/* every tick: the time; the game side polls the rest */
void ach_rules_tick(void)
{
	if (ach.enabled)
		ach.level_ticks++;
}

ach_u32 ach_rules_ticks(void)
{
	return ach.level_ticks;
}

void ach_rules_set_humans(int humans)
{
	ach.humans = humans;
}

/* the player took damage: whether the body (not only the shield) did, and
whether the shield broke */
void ach_rules_player_damaged(int local_player, int body, int shield_broke)
{
	struct ach_run *run;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	if (body)
	{
		run->health_damage++;
		/* (the landing's group not yet gone) */
		if (ach.level == _ach_level_b30 && !ach.shared.beach_done)
			ach.shared.beach_damaged = 1;
	}
	if (shield_broke)
		run->shield_broken++;
}

void ach_rules_player_died(int local_player)
{
	if (!ach_counting(local_player))
		return;
	ach.player[local_player].deaths++;
}

void ach_rules_powerup(int local_player, int powerup)
{
	struct ach_run *run;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	if (powerup == _ach_powerup_health)
		run->health_packs++;
	else if (powerup == _ach_powerup_overshield)
	{
		run->overshields++;
		if (run->overshields >= 5)
			ach_unlock(local_player, _ach_overshielded);
	}
}

void ach_rules_vehicle_entered(int local_player, int vehicle)
{
	struct ach_run *run;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	run->vehicles_entered++;
	if (vehicle == _ach_vehicle_banshee)
		run->banshees_entered++;
}

void ach_rules_weapon_picked(int local_player, int weapon)
{
	struct ach_run *run;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	run->weapon_pickups++;
	if (weapon == _ach_weapon_shotgun)
		run->shotgun_owned = 1;
}

/* a shot of the player's, or of the vehicle's weapon they drive or man */
void ach_rules_weapon_fired(int local_player, int weapon, int scoped)
{
	struct ach_run *run;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	run->shots++;
	if (scoped)
		run->scoped_shots++;
	if (run->shotgun_owned && weapon != _ach_weapon_shotgun)
		run->shots_after_shotgun++;
}

void ach_rules_jumped(int local_player)
{
	if (ach_counting(local_player))
		ach.run[local_player].jumped = 1;
}

void ach_rules_melee(int local_player)
{
	if (ach_counting(local_player))
		ach.run[local_player].melee++;
}

void ach_rules_grenade_thrown(int local_player)
{
	if (ach_counting(local_player))
		ach.run[local_player].grenades++;
}

/* a plasma grenade stuck to an enemy */
void ach_rules_stuck(int local_player)
{
	if (!ach_counting(local_player))
		return;
	ach.run[local_player].sticks++;
	ach_check_counts(local_player);
}

/* an enemy of a kind took damage that did not kill it */
void ach_rules_enemy_damaged(int victim_class, int victim_key)
{
	int index;

	if (!ach.enabled || victim_class != _ach_victim_hunter)
		return;
	for (index = 0; index < ACH_DAMAGED_HUNTERS; index++)
	{
		if (ach.shared.damaged_hunters[index] == victim_key + 1)
			return;
	}
	ach.shared.damaged_hunters[ach.shared.damaged_hunter_next] = victim_key + 1;
	ach.shared.damaged_hunter_next = (ach.shared.damaged_hunter_next + 1) % ACH_DAMAGED_HUNTERS;
}

/* an enemy was killed: the player credited, what it was, what killed it,
and, for an explosion's kills, a key of the explosion (its tick and place) */
void ach_rules_kill(int local_player, int victim_class, int victim_key, int damage, int flags,
	ach_u32 explosion_key)
{
	struct ach_run *run;
	int index;
	int fresh = 1;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	if (victim_class == _ach_victim_hunter)
	{
		for (index = 0; index < ACH_DAMAGED_HUNTERS; index++)
		{
			if (ach.shared.damaged_hunters[index] == victim_key + 1)
			{
				ach.shared.damaged_hunters[index] = 0;
				fresh = 0;
			}
		}
		if (fresh && damage == _ach_damage_pistol)
			ach_unlock(local_player, _ach_you_are_the_weapon);
	}
	if (victim_class == _ach_victim_infection_form && damage == _ach_damage_shotgun_pellet &&
		ach_applies(_ach_popcorn))
	{
		run->infection_forms_shotgunned++;
		if (run->infection_forms_shotgunned >= 50)
			ach_unlock(local_player, _ach_popcorn);
	}
	if (damage == _ach_damage_rocket_explosion)
	{
		run->rocket_kills++;
		if (run->rocket_kills >= 20)
			ach_unlock(local_player, _ach_heavy_artillery);
	}
	if (flags & _ach_kill_headshot && (damage == _ach_damage_pistol || damage == _ach_damage_sniper))
		run->headshots++;
	if (flags & _ach_kill_assassination)
		run->assassinations++;
	if (damage == _ach_damage_plasma_explosion)
	{
		if (run->explosion_kills > 0 && run->explosion_tick == ach.level_ticks && run->explosion_key == explosion_key)
			run->explosion_kills++;
		else
		{
			run->explosion_tick = ach.level_ticks;
			run->explosion_key = explosion_key;
			run->explosion_kills = 1;
		}
		if (run->explosion_kills >= 5 && ach.humans >= 2)
			ach_unlock(local_player, _ach_tying_up_loose_ends);
	}
	ach_check_counts(local_player);
}

/* a player seated in a Pelican, or driving a Warthog that is in the air, this
tick; the game side polls it */
void ach_rules_riding(int local_player, int in_pelican, int warthog_airborne)
{
	struct ach_run *run;

	if (!ach_counting(local_player))
		return;
	run = &ach.run[local_player];
	if (in_pelican)
	{
		run->pelican_ticks++;
		if (run->pelican_ticks >= ACH_PELICAN_TICKS && ach_applies(_ach_get_me_out_of_here))
			ach_unlock(local_player, _ach_get_me_out_of_here);
	}
	else
		run->pelican_ticks = 0;
	if (warthog_airborne)
	{
		run->airborne_ticks++;
		run->airborne_grace = ACH_AIRBORNE_GRACE_TICKS;
	}
	else
	{
		if (run->airborne_grace > 0)
			run->airborne_grace--;
		else
			run->airborne_ticks = 0;
	}
}

/* a player's driven Warthog splattered an enemy */
void ach_rules_splatter(int local_player)
{
	if (ach_counting(local_player) && ach.run[local_player].airborne_ticks >= ACH_AIRBORNE_TICKS)
		ach_unlock(local_player, _ach_airborne);
}

/* all according to plan: the boarding area's group this tick, how many are
alive and the highest state of any (the AI's: idle 1, alerted into combat 2
and more); beachhead: the landing's group */
void ach_rules_stealth_poll(int living, int status)
{
	int local_player;

	if (!ach.enabled || ach.tainted || ach.level != _ach_level_a50 || ach.shared.stealth_done)
		return;
	if (living > ach.shared.stealth_peak)
		ach.shared.stealth_peak = living;
	if (living > 0 && status >= 2)
		ach.shared.stealth_detected = 1;
	if (living == 0 && ach.shared.stealth_peak >= ACH_STEALTH_MINIMUM_ENEMIES)
	{
		ach.shared.stealth_done = 1;
		if (!ach.shared.stealth_detected && ach_applies(_ach_all_according_to_plan))
		{
			for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
			{
				if (ach_counting(local_player))
					ach_unlock(local_player, _ach_all_according_to_plan);
			}
		}
	}
}

void ach_rules_beach_poll(int living)
{
	int local_player;

	if (!ach.enabled || ach.tainted || ach.level != _ach_level_b30 || ach.shared.beach_done)
		return;
	if (living > ach.shared.beach_peak)
		ach.shared.beach_peak = living;
	if (living == 0 && ach.shared.beach_peak >= ACH_STEALTH_MINIMUM_ENEMIES)
	{
		ach.shared.beach_done = 1;
		if (!ach.shared.beach_damaged && ach_applies(_ach_beachhead))
		{
			for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
			{
				if (ach_counting(local_player) && ach.run[local_player].health_damage == 0)
					ach_unlock(local_player, _ach_beachhead);
			}
		}
	}
}

/* the day of the month, once for each player counted in a level */
void ach_rules_day(int local_player, int day_of_month)
{
	if (ach.player[local_player].day_checked || !ach_counting(local_player))
		return;
	ach.player[local_player].day_checked = 1;
	if (day_of_month == 15)
		ach_unlock(local_player, _ach_a_day_to_remember);
}

/* the achievements the level's end earns a player, for the time left on the
countdown (The Maw's) in ticks */
static void ach_win_player(int local_player, int timer_ticks)
{
	struct ach_run const *run = &ach.run[local_player];
	struct ach_profile *profile = &ach.profile[local_player];
	int level;

	if (ach.level >= 0 && ach.level < ACH_LEVELS)
	{
		profile->won[ach.level] = (unsigned char)(profile->won[ach.level] | (1 << ach.difficulty));
		if (ach.humans >= 2)
			profile->coop_won = (unsigned short)(profile->coop_won | (1 << ach.level));
		profile->dirty = 1;
	}
	if (ach_applies(_ach_birth_of_a_spartan))
		ach_unlock(local_player, _ach_birth_of_a_spartan);
	if (ach.humans >= 2)
	{
		ach_unlock(local_player, _ach_bro_slayer);
		if (profile->coop_won == (1 << ACH_LEVELS) - 1)
			ach_unlock(local_player, _ach_together_again);
	}
	{
		int heroic = 1;
		int legendary = 1;

		for (level = 0; level < ACH_LEVELS; level++)
		{
			if (!(profile->won[level] & 12))
				heroic = 0;
			if (!(profile->won[level] & 8))
				legendary = 0;
		}
		if (heroic)
			ach_unlock(local_player, _ach_believe_in_a_hero);
		if (legendary)
			ach_unlock(local_player, _ach_living_legend);
	}
	if (ach_applies(_ach_standard_operating_procedure) && run->health_packs == 0)
		ach_unlock(local_player, _ach_standard_operating_procedure);
	if (ach_applies(_ach_walk_it_off) && run->health_packs == 0)
		ach_unlock(local_player, _ach_walk_it_off);
	if (ach_applies(_ach_shielded) && run->shield_broken == 0)
		ach_unlock(local_player, _ach_shielded);
	if (ach_applies(_ach_scunt) && run->vehicles_entered == 0)
		ach_unlock(local_player, _ach_scunt);
	if (ach_applies(_ach_how_can_i_hit_you) && run->health_damage == 0)
		ach_unlock(local_player, _ach_how_can_i_hit_you);
	if (ach_applies(_ach_close_quarters) && run->scoped_shots == 0)
		ach_unlock(local_player, _ach_close_quarters);
	if (ach_applies(_ach_grenadier) && run->shots == 0)
		ach_unlock(local_player, _ach_grenadier);
	if (ach_applies(_ach_speed_reader) && ach.level_ticks <= 30 * 60 * ACH_TICKS_PER_SECOND)
		ach_unlock(local_player, _ach_speed_reader);
	if (ach_applies(_ach_ill_be_back) && run->health_damage == 0)
		ach_unlock(local_player, _ach_ill_be_back);
	if (ach_applies(_ach_the_flood) && run->health_damage == 0)
		ach_unlock(local_player, _ach_the_flood);
	if (ach_applies(_ach_bug_spray) && run->shotgun_owned && run->shots_after_shotgun == 0)
		ach_unlock(local_player, _ach_bug_spray);
	if (ach_applies(_ach_that_just_happened) && ach.player[local_player].deaths == 0)
		ach_unlock(local_player, _ach_that_just_happened);
	if (ach_applies(_ach_read_the_history_books) && ach.level_ticks <= 30 * 60 * ACH_TICKS_PER_SECOND)
		ach_unlock(local_player, _ach_read_the_history_books);
	if (ach_applies(_ach_mind_the_gap) && run->banshees_entered == 0)
		ach_unlock(local_player, _ach_mind_the_gap);
	if (ach_applies(_ach_cheap_thrills) && run->health_packs == 0)
		ach_unlock(local_player, _ach_cheap_thrills);
	if (ach_applies(_ach_what_back_already) && ach.level_ticks <= 15 * 60 * ACH_TICKS_PER_SECOND)
		ach_unlock(local_player, _ach_what_back_already);
	if (ach_applies(_ach_leave_it_where_it_lay) && run->weapon_pickups == 0)
		ach_unlock(local_player, _ach_leave_it_where_it_lay);
	if (ach_applies(_ach_never_tell_me_the_odds) && timer_ticks >= 60 * ACH_TICKS_PER_SECOND)
		ach_unlock(local_player, _ach_never_tell_me_the_odds);
	if (ach_applies(_ach_feet_firmly_planted) && !run->jumped)
		ach_unlock(local_player, _ach_feet_firmly_planted);
	if (ach_applies(_ach_pacifist) && run->shots == 0 && run->grenades == 0 && run->melee == 0 &&
		ach.player[local_player].deaths == 0)
		ach_unlock(local_player, _ach_pacifist);
}

/* the level is won: the countdown's ticks left (The Maw's), or -1 */
void ach_rules_won(int timer_ticks)
{
	int local_player;

	if (!ach.enabled || ach.tainted || ach.won)
		return;
	ach.won = 1;
	for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
	{
		if (ach_counting(local_player))
		{
			ach_win_player(local_player, timer_ticks);
			ach_fold_counts(local_player);
			ach_check_counts(local_player);
			ach_platform_save(local_player);
		}
	}
}

/* ---------- the profile's file (the bytes, which are little-endian whatever the machine) */

static void ach_put(unsigned char *bytes, int offset, ach_u32 value, int size)
{
	int index;

	for (index = 0; index < size; index++)
		bytes[offset + index] = (unsigned char)(value >> (8 * index));
}

static ach_u32 ach_get(unsigned char const *bytes, int offset, int size)
{
	ach_u32 value = 0;
	int index;

	for (index = 0; index < size; index++)
		value |= (ach_u32)bytes[offset + index] << (8 * index);
	return value;
}

static ach_u32 ach_crc32(unsigned char const *bytes, int size)
{
	ach_u32 crc = 0xFFFFFFFFu;
	int index;
	int bit;

	for (index = 0; index < size; index++)
	{
		crc ^= bytes[index];
		for (bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
	}
	return ~crc;
}

/* the file's bytes (ACH_FILE_SIZE of them) */
void ach_rules_profile_write(struct ach_profile const *profile, unsigned char *bytes)
{
	int index;

	memset(bytes, 0, ACH_FILE_SIZE);
	ach_put(bytes, 0, ACH_FILE_MAGIC, 4);
	ach_put(bytes, 4, ACH_FILE_VERSION, 2);
	ach_put(bytes, 6, ACH_FILE_SIZE, 2);
	ach_put(bytes, 8, profile->unlocked[0], 4);
	ach_put(bytes, 12, profile->unlocked[1], 4);
	for (index = 0; index < ACH_FILE_CAPACITY; index++)
		ach_put(bytes, 16 + 4 * index, profile->unlock_time[index], 4);
	for (index = 0; index < ACH_LEVELS; index++)
		bytes[176 + index] = profile->won[index];
	ach_put(bytes, 186, profile->coop_won, 2);
	ach_put(bytes, 188, profile->headshots, 4);
	ach_put(bytes, 192, profile->assassinations, 4);
	ach_put(bytes, 196, profile->sticks, 4);
	ach_put(bytes, 200, ach_crc32(bytes, 200), 4);
}

/* whether the bytes are a file of this version, and its contents if so (a
bad one is ignored, and replaced at the next write) */
int ach_rules_profile_read(struct ach_profile *profile, unsigned char const *bytes, long size)
{
	int index;

	if (size != ACH_FILE_SIZE || ach_get(bytes, 0, 4) != ACH_FILE_MAGIC || ach_get(bytes, 4, 2) != ACH_FILE_VERSION ||
		ach_get(bytes, 6, 2) != ACH_FILE_SIZE || ach_get(bytes, 200, 4) != ach_crc32(bytes, 200))
		return 0;
	profile->unlocked[0] = ach_get(bytes, 8, 4);
	profile->unlocked[1] = ach_get(bytes, 12, 4);
	/* (only the achievements there are) */
	profile->unlocked[0] &= NUMBER_OF_ACHIEVEMENTS >= 32 ? 0xFFFFFFFFu : (1u << NUMBER_OF_ACHIEVEMENTS) - 1;
	profile->unlocked[1] &= NUMBER_OF_ACHIEVEMENTS > 32 ? (1u << (NUMBER_OF_ACHIEVEMENTS - 32)) - 1 : 0;
	for (index = 0; index < ACH_FILE_CAPACITY; index++)
		profile->unlock_time[index] = ach_get(bytes, 16 + 4 * index, 4);
	for (index = 0; index < ACH_LEVELS; index++)
		profile->won[index] = (unsigned char)(bytes[176 + index] & 15);
	profile->coop_won = (unsigned short)(ach_get(bytes, 186, 2) & ((1u << ACH_LEVELS) - 1));
	profile->headshots = ach_get(bytes, 188, 4);
	profile->assassinations = ach_get(bytes, 192, 4);
	profile->sticks = ach_get(bytes, 196, 4);
	return 1;
}

/* ---------- the game */

#ifndef ACHIEVEMENTS_RULES_ONLY

#include "cseries/cseries.h"
#include "cseries/cseries_windows.h"
#include "ai/ai_script.h"
#include "ai/actors.h"
#include "game/cheats.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "cutscene/cinematics.h"
#include "game/game_allegiance.h"
#include "sound/game_sound.h"
#include "game/players.h"
#include "interface/hud.h"
#include "interface/hud_definitions.h"
#include "interface/interface.h"
#include "interface/hud_messaging.h"
#include "interface/player_ui.h"
#include "items/items.h"
#include "items/projectiles.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "main/main.h"
#include "math/real_math.h"
#include "objects/damage.h"
#include "game/game_globals.h"
#include "object_definitions.h"
#include "objects/damage_effect_definitions.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"
#include "scenario/scenario.h"
#include "tag_files/tag_files.h"
#include "text/draw_string.h"
#include "units/units.h"
#include "units/vehicles.h"
#include "networking/network_game_globals.h"
#include "achievements.h"
#include "network_coop.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

int config_boolean(char const *name);
int config_integer(char const *name);
short main_get_solo_level_from_name(char const *name);
void platform_log(char const *format, ...);

enum
{
	/* the unlock notice: how long it shows, and how long the next waits */
	ACHIEVEMENTS_PATH_SIZE = 256,
	NOTICE_MILLISECONDS = 4500,
	NOTICE_QUEUE = 8,
	NOTICE_TEXT_BOUNDS_HEIGHT = 18,
	_notice_text_centered = 2,
	/* units.c's damage side effect that kills the unsuspecting */
	DAMAGE_SIDE_EFFECT_LETHAL_TO_THE_UNSUSPECTING = 2
};

static struct
{
	int profile_index[ACH_PLAYERS];
	/* the AI groups the polls watch (-1 not in this level) */
	long stealth_group;
	long beach_group;
	int queue[NOTICE_QUEUE];
	int queued;
	unsigned long notice_start;
	int logging;
} achievements_globals;

static void ach_platform_unlocked_sound(void);

static int achievements_enabled(void)
{
	return ach_rules_enabled();
}

void ach_platform_log(char const *format, ...)
{
	if (achievements_globals.logging)
	{
		char text[256];
		va_list arguments;

		va_start(arguments, format);
		vsnprintf(text, sizeof(text), format, arguments);
		va_end(arguments);
		platform_log("%s", text);
	}
}

ach_u32 ach_platform_time(void)
{
	return (ach_u32)time(NULL);
}

/* the unlock notice: the name waits its turn on the screen, with the sound
(a placeholder: port/assets/achievements/README.md has what is to replace
them) */
void ach_platform_unlocked(int local_player, int achievement)
{
	(void)local_player;
	if (achievements_globals.queued < NOTICE_QUEUE)
		achievements_globals.queue[achievements_globals.queued++] = achievement;
	ach_platform_unlocked_sound();
}

/* ---------- the profiles' files */

static boolean achievements_profile_path(int local_player, char *path, long size)
{
	char directory[ACHIEVEMENTS_PATH_SIZE];
	boolean found = FALSE;

	directory[0] = 0;
	if (player_ui_get_path_to_local_player_profile_directory((short)local_player, directory) &&
		directory[0] && (long)strlen(directory) + (long)sizeof("achieve.bin") < size)
	{
		snprintf(path, (size_t)size, "%sachieve.bin", directory);
		found = TRUE;
	}
	else if (player_ui_get_active_player_profile_index((short)local_player) == NONE)
	{
		/* (a player with no profile, a game started by other means than the
		menus: one file in the save root for them all) */
		snprintf(path, (size_t)size, "u:\\achievements_default.bin");
		found = TRUE;
	}
	return found;
}

static void achievements_profile_load(int local_player)
{
	char path[ACHIEVEMENTS_PATH_SIZE + 16];
	unsigned char bytes[ACH_FILE_SIZE + 1];
	struct ach_profile *profile = ach_rules_profile(local_player);
	HANDLE file;
	unsigned long read = 0;
	int usable = 0;

	if (achievements_profile_path(local_player, path, sizeof(path)))
	{
		usable = 1;
		file = CreateFileA(path, GENERIC_READ, 1, NULL, OPEN_EXISTING, 0, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			if (ReadFile(file, bytes, sizeof(bytes), &read, NULL))
			{
				struct ach_profile fresh;

				memset(&fresh, 0, sizeof(fresh));
				if (ach_rules_profile_read(&fresh, bytes, (long)read))
				{
					fresh.usable = 1;
					*profile = fresh;
				}
			}
			CloseHandle(file);
		}
	}
	ach_rules_player_begin(local_player, usable);
}

/* written beside the file and moved over it, so a crash leaves the old or
the new one */
void ach_platform_save(int local_player)
{
	char path[ACHIEVEMENTS_PATH_SIZE + 16];
	char temporary[ACHIEVEMENTS_PATH_SIZE + 24];
	unsigned char bytes[ACH_FILE_SIZE];
	struct ach_profile *profile = ach_rules_profile(local_player);
	HANDLE file;
	unsigned long written = 0;
	boolean success = FALSE;

	if (!profile->usable || !profile->dirty || !achievements_profile_path(local_player, path, sizeof(path)))
		return;
	snprintf(temporary, sizeof(temporary), "%s.tmp", path);
	ach_rules_profile_write(profile, bytes);
	file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		success = WriteFile(file, bytes, sizeof(bytes), &written, NULL) && written == sizeof(bytes);
		CloseHandle(file);
	}
	if (success)
	{
		DeleteFileA(path);
		success = MoveFileA(temporary, path) != 0;
	}
	if (success)
		profile->dirty = 0;
	else
	{
		DeleteFileA(temporary);
		ach_platform_log("achievements: could not write %s", path);
	}
}

/* ---------- reading the game */

static boolean achievements_name_has(long tag_index, char const *text)
{
	char const *name = tag_index != NONE ? tag_get_name(tag_index) : NULL;

	return name && strstr(name, text) != NULL;
}

static boolean achievements_name_is(long tag_index, char const *text)
{
	char const *name = tag_index != NONE ? tag_get_name(tag_index) : NULL;

	return name && strcmp(name, text) == 0;
}

static int achievements_weapon_class(long weapon_definition_index)
{
	return achievements_name_is(weapon_definition_index, "weapons\\shotgun\\shotgun") ?
		_ach_weapon_shotgun : _ach_weapon_other;
}

static int achievements_vehicle_class(long vehicle_definition_index)
{
	if (achievements_name_has(vehicle_definition_index, "banshee"))
		return _ach_vehicle_banshee;
	if (achievements_name_has(vehicle_definition_index, "pelican"))
		return _ach_vehicle_pelican;
	if (achievements_name_has(vehicle_definition_index, "warthog"))
		return _ach_vehicle_warthog;
	return _ach_vehicle_other;
}

static int achievements_damage_class(long damage_definition_index)
{
	char const *name = damage_definition_index != NONE ? tag_get_name(damage_definition_index) : NULL;
	size_t length;

	if (!name)
		return _ach_damage_other;
	if (strcmp(name, "weapons\\pistol\\bullet") == 0)
		return _ach_damage_pistol;
	if (strcmp(name, "weapons\\sniper rifle\\sniper bullet") == 0)
		return _ach_damage_sniper;
	if (strcmp(name, "weapons\\shotgun\\pellet") == 0)
		return _ach_damage_shotgun_pellet;
	if (strcmp(name, "weapons\\rocket launcher\\explosion") == 0)
		return _ach_damage_rocket_explosion;
	if (strcmp(name, "weapons\\plasma grenade\\explosion") == 0)
		return _ach_damage_plasma_explosion;
	length = strlen(name);
	if (length >= 6 && strcmp(name + length - 6, "\\melee") == 0)
		return _ach_damage_melee;
	return _ach_damage_other;
}

static int achievements_victim_class(long object_definition_index)
{
	if (achievements_name_has(object_definition_index, "flood_infection"))
		return _ach_victim_infection_form;
	if (achievements_name_is(object_definition_index, "characters\\hunter\\hunter"))
		return _ach_victim_hunter;
	return _ach_victim_other;
}

/* the local player of a player datum, or -1 */
static int achievements_local_player(long player_index)
{
	struct player_datum *player = player_index != NONE ? player_try_and_get(player_index) : NULL;

	return player && player->local_player_index >= 0 && player->local_player_index < ACH_PLAYERS ?
		player->local_player_index : -1;
}

/* the local player of a unit, or -1 (a unit of a player of another machine
is not one) */
static int achievements_local_player_of_unit(long unit_index)
{
	struct unit_datum *unit = unit_index != NONE ? unit_try_and_get(unit_index) : NULL;

	return unit ? achievements_local_player(unit->unit.player_index) : -1;
}

/* the local player a vehicle's weapon is the shot of: the one it carries as
its driver or its gunner */
static int achievements_local_player_of_vehicle(long vehicle_index)
{
	struct unit_datum *vehicle = unit_try_and_get(vehicle_index);
	int local_player = -1;

	if (vehicle)
	{
		local_player = achievements_local_player_of_unit(vehicle->unit.driver_object_index);
		if (local_player < 0)
			local_player = achievements_local_player_of_unit(vehicle->unit.gunner_object_index);
	}
	return local_player;
}

/* the profile of a local player is read the first time the level needs it,
and the player is dropped if their profile changes during the level */
static int achievements_player(int local_player)
{
	int profile_index;

	if (local_player < 0 || local_player >= ACH_PLAYERS)
		return 0;
	profile_index = (int)player_ui_get_active_player_profile_index((short)local_player);
	if (!ach_rules_profile(local_player)->usable && achievements_globals.profile_index[local_player] == 0)
	{
		achievements_globals.profile_index[local_player] = profile_index + 2;
		achievements_profile_load(local_player);
	}
	else if (achievements_globals.profile_index[local_player] != profile_index + 2)
	{
		ach_rules_player_end(local_player);
		return 0;
	}
	return ach_rules_player_live(local_player);
}

static int achievements_humans(void)
{
	struct data_iterator iterator;
	int humans = 0;

	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
		humans++;
	return humans;
}

/* a machine that decides the game: not a client of another's */
static int achievements_decides(void)
{
	return !network_game_distributed_client();
}

void achievements_new_map(void)
{
	char const *name = global_scenario_index != NONE ? tag_get_name(global_scenario_index) : NULL;
	int level = name ? (int)main_get_solo_level_from_name(name) : -1;
	int enabled = 0;
	/* (flush what the last map left unwritten) */
	{
		int local_player;

		for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
		{
			if (ach_rules_profile(local_player)->usable)
				ach_platform_save(local_player);
		}
	}
	achievements_globals.logging = config_boolean("debug.achievements_log") != 0;
	if (config_boolean("game.achievements") && !game_engine_running() && achievements_decides())
		enabled = 1;
	ach_rules_new_map(enabled, level, (int)game_difficulty_level_get());
	memset(achievements_globals.profile_index, 0, sizeof(achievements_globals.profile_index));
	achievements_globals.stealth_group = NONE;
	achievements_globals.beach_group = NONE;
	if (!ach_rules_enabled())
		return;
	/* (the groups the polls watch: a50's boarding area, b30's landing) */
	if (level == _ach_level_a50)
	{
		if (!ai_index_from_string(global_scenario_get(), "covenant_area2", &achievements_globals.stealth_group))
			achievements_globals.stealth_group = NONE;
	}
	else if (level == _ach_level_b30)
	{
		if (!ai_index_from_string(global_scenario_get(), "beach_lz", &achievements_globals.beach_group))
			achievements_globals.beach_group = NONE;
	}
	ach_platform_log("achievements: level %d, difficulty %d, groups %ld %ld", level, (int)game_difficulty_level_get(),
		achievements_globals.stealth_group, achievements_globals.beach_group);
}

/* a local player's biped unit, or NULL */
static struct unit_datum *achievements_unit_of(int local_player)
{
	long player_index = players_globals->local_players[local_player];
	struct player_datum *player = player_index != NONE ? player_try_and_get(player_index) : NULL;

	return player && player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
}

void achievements_tick(void)
{
	int local_player;

	if (!achievements_enabled())
		return;
	ach_rules_tick();
	/* (a cheat flag set) */
	if (cheat.deathless_player || cheat.jetpack || cheat.infinite_ammo || cheat.bump_possession ||
		cheat.super_jump || cheat.reflexive_damage_effects || cheat.medusa || cheat.omnipotent ||
		cheat.bottomless_clip)
		ach_rules_taint();
	if ((ach_rules_ticks() & 31) == 1)
		ach_rules_set_humans(achievements_humans());
	for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
	{
		struct unit_datum *unit;
		short state;
		int in_pelican = 0;
		int airborne = 0;
		long player_unit_index;

		if (players_globals->local_players[local_player] == NONE || !achievements_player(local_player))
			continue;
		if (!ach.player[local_player].day_checked)
		{
			time_t now = time(NULL);
			struct tm *local = localtime(&now);

			int day = config_integer("debug.achievements_day");

			ach_rules_day(local_player, day > 0 ? day : local ? local->tm_mday : 0);
		}
		unit = achievements_unit_of(local_player);
		player_unit_index = player_get(players_globals->local_players[local_player])->unit_index;
		if (!unit)
		{
			ach_rules_riding(local_player, 0, 0);
			continue;
		}
		/* (the start of a grenade throw, and of a melee attack) */
		state = unit->unit.animation.state;
		if (state != ach.player[local_player].previous_state)
		{
			if (state == _unit_state_throw_grenade)
				ach_rules_grenade_thrown(local_player);
			else if (state == _unit_state_melee_attack || state == _unit_state_melee_airborne ||
				state == _unit_state_melee_continuous)
				ach_rules_melee(local_player);
			ach.player[local_player].previous_state = state;
		}
		if (unit->object.parent_object_index != NONE)
		{
			struct unit_datum *vehicle = unit_try_and_get(unit->object.parent_object_index);

			if (vehicle)
			{
				int kind = achievements_vehicle_class(vehicle->definition_index);

				in_pelican = kind == _ach_vehicle_pelican;
				airborne = kind == _ach_vehicle_warthog &&
					vehicle->unit.driver_object_index == player_unit_index &&
					!TEST_FLAG(vehicle->object.flags, _object_on_ground_bit);
			}
		}
		ach_rules_riding(local_player, in_pelican, airborne);
	}
	if (achievements_globals.stealth_group != NONE)
		ach_rules_stealth_poll(ai_scripting_living_count(achievements_globals.stealth_group),
			ai_scripting_status(achievements_globals.stealth_group));
	if (achievements_globals.beach_group != NONE)
		ach_rules_beach_poll(ai_scripting_living_count(achievements_globals.beach_group));
}

void achievements_won(void)
{
	int local_player;
	int timer_ticks = -1;

	if (!achievements_enabled())
		return;
	for (local_player = 0; local_player < ACH_PLAYERS; local_player++)
	{
		if (players_globals->local_players[local_player] != NONE)
			achievements_player(local_player);
	}
	ach_rules_set_humans(achievements_humans());
	ach_platform_log("achievements: won, %d humans, %d spawned, local players %ld %ld %ld %ld", achievements_humans(),
		(int)player_spawn_count, players_globals->local_players[0], players_globals->local_players[1],
		players_globals->local_players[2], players_globals->local_players[3]);
	if (ach_rules_level() == _ach_level_d40)
		timer_ticks = scripted_hud_get_timer_ticks();
	ach_rules_won(timer_ticks);
}

void achievements_checkpoint_save(void)
{
	ach_platform_log("achievements: checkpoint saved");
	ach_rules_checkpoint_fold();
	ach_rules_checkpoint_save();
}

void achievements_checkpoint_load(void)
{
	ach_platform_log("achievements: checkpoint loaded");
	ach_rules_checkpoint_load();
}

void achievements_note_resumed(void)
{
	ach_rules_resumed();
}

void achievements_note_core_loaded(void)
{
	ach_rules_taint();
}

/* what the host's own commands are (not changes of the game) */
static boolean achievements_harmless_command(char const *expression)
{
	static char const *const words[] = { "ban", "kick", "votekick", "bringto" };
	char const *text = expression;
	long word;

	while (*text == ' ' || *text == '\t' || *text == '(')
		text++;
	for (word = 0; word < (long)NUMBEROF(words); word++)
	{
		size_t length = strlen(words[word]);

		if (strncmp(text, words[word], length) == 0 && (text[length] == ' ' || text[length] == '\t' || text[length] == 0))
			return TRUE;
	}
	return FALSE;
}

void achievements_note_command(char const *expression)
{
	if (!ach_rules_enabled())
		return;
	/* (the setting for tests, which drive the game with commands) */
	if (achievements_harmless_command(expression) || config_boolean("debug.achievements_allow_commands"))
		return;
	ach_rules_taint();
}

/* a victim that is an enemy of the team that damaged it: a unit with an actor
of another side, or a swarm's form (the Flood's) */
static boolean achievements_enemy_unit(struct unit_datum const *unit, short team_index)
{
	if (unit->unit.actor_index != NONE)
	{
		struct actor_datum *actor = actor_try_and_get(unit->unit.actor_index);

		return actor && game_team_is_enemy(team_index, actor->meta.team_index);
	}
	return unit->unit.swarm_actor_index != NONE;
}

void achievements_note_damage(long object_index, struct damage_data const *damage,
	unsigned long being_damaged_flags, float shield_damage, float body_damage)
{
	struct unit_datum *unit;
	int victim_player;
	int owner;
	boolean killed;

	if (!achievements_enabled() || network_game_distributed_client())
		return;
	unit = unit_try_and_get(object_index);
	if (!unit)
		return;
	victim_player = achievements_local_player(unit->unit.player_index);
	if (victim_player >= 0)
	{
		if (achievements_globals.logging)
			ach_platform_log("achievements: player %d damaged, body %.3f, shield broke %d", victim_player,
				(double)body_damage, TEST_FLAG(being_damaged_flags, _object_being_damaged_shield_depleted_bit) != 0);
		if (achievements_player(victim_player))
			ach_rules_player_damaged(victim_player, body_damage > 0.0f,
				TEST_FLAG(being_damaged_flags, _object_being_damaged_shield_depleted_bit) != 0);
		return;
	}
	(void)shield_damage;
	killed = TEST_FLAG(being_damaged_flags, _object_being_damaged_body_depleted_bit) != 0;
	if (unit->unit.player_index != NONE)
		return;
	owner = achievements_local_player(damage->owner_player_index);
	if (owner < 0 || !achievements_player(owner) || !achievements_enemy_unit(unit, damage->owner_team_index))
		return;
	{
		int victim = achievements_victim_class(unit->definition_index);

		if (achievements_globals.logging)
			ach_platform_log("achievements: player %d %s %s with %s (class %d, damage class %d, flags %lx, body %.3f)",
				owner, killed ? "killed" : "hurt", tag_get_name(unit->definition_index),
				tag_get_name(damage->definition_index), victim, achievements_damage_class(damage->definition_index),
				being_damaged_flags, (double)body_damage);
		if (!killed)
			ach_rules_enemy_damaged(victim, (int)object_index);
		else
		{
			struct damage_effect_definition *effect = damage_effect_definition_get(damage->definition_index);
			boolean instant = TEST_FLAG(being_damaged_flags, _object_being_damaged_killed_instantly_bit) != 0;
			boolean unsuspecting = effect->damage.side_effect == DAMAGE_SIDE_EFFECT_LETHAL_TO_THE_UNSUSPECTING;
			int flags = 0;

			if (instant && unsuspecting)
				flags = _ach_kill_assassination;
			else if (instant)
				flags = _ach_kill_headshot;
			ach_rules_kill(owner, victim, (int)object_index, achievements_damage_class(damage->definition_index),
				flags,
				(ach_u32)((long)(damage->epicenter.x * 64.0f) * 31 + (long)(damage->epicenter.y * 64.0f)) * 31 +
					(ach_u32)(long)(damage->epicenter.z * 64.0f));
			/* (a Warthog's splatter) */
			if (achievements_name_has(damage->definition_index, "vehicle_killed_unit"))
				ach_rules_splatter(owner);
		}
	}
}

void achievements_note_player_died(long player_index)
{
	int local_player;

	if (!achievements_enabled())
		return;
	local_player = achievements_local_player(player_index);
	if (achievements_globals.logging && local_player >= 0)
		ach_platform_log("achievements: player %d died", local_player);
	if (local_player >= 0 && achievements_player(local_player))
		ach_rules_player_died(local_player);
}

/* kind: players.c's _network_pickup_ enum: 0 a weapon, 4 a power-up */
void achievements_note_pickup(long player_index, short kind, long definition_index)
{
	int local_player;

	if (!achievements_enabled() || (kind != 0 && kind != 4))
		return;
	local_player = achievements_local_player(player_index);
	if (local_player < 0 || !achievements_player(local_player))
		return;
	if (achievements_globals.logging)
		ach_platform_log("achievements: player %d picked up %s (kind %d)", local_player,
			tag_get_name(definition_index), (int)kind);
	if (kind == 0)
		ach_rules_weapon_picked(local_player, achievements_weapon_class(definition_index));
	else if (achievements_name_is(definition_index, "powerups\\health pack"))
		ach_rules_powerup(local_player, _ach_powerup_health);
	else if (achievements_name_is(definition_index, "powerups\\over shield"))
		ach_rules_powerup(local_player, _ach_powerup_overshield);
}

void achievements_note_vehicle_entered(long player_index, long vehicle_index)
{
	int local_player;
	struct unit_datum *vehicle;

	if (!achievements_enabled())
		return;
	local_player = achievements_local_player(player_index);
	vehicle = unit_try_and_get(vehicle_index);
	if (achievements_globals.logging && local_player >= 0 && vehicle)
		ach_platform_log("achievements: player %d entered %s", local_player, tag_get_name(vehicle->definition_index));
	if (local_player >= 0 && vehicle && achievements_player(local_player))
		ach_rules_vehicle_entered(local_player, achievements_vehicle_class(vehicle->definition_index));
}

void achievements_note_weapon_fired(long weapon_index, long owner_object_index)
{
	int local_player;
	int scoped = 0;

	if (!achievements_enabled() || owner_object_index == NONE)
		return;
	local_player = achievements_local_player_of_unit(owner_object_index);
	if (local_player >= 0)
		scoped = unit_get_zoom_level(owner_object_index) != NONE;
	else
		local_player = achievements_local_player_of_vehicle(owner_object_index);
	if (achievements_globals.logging && local_player >= 0)
		ach_platform_log("achievements: player %d fired %s%s", local_player,
			tag_get_name(weapon_get(weapon_index)->definition_index), scoped ? " (scoped)" : "");
	if (local_player >= 0 && achievements_player(local_player))
		ach_rules_weapon_fired(local_player, achievements_weapon_class(weapon_get(weapon_index)->definition_index),
			scoped);
}

void achievements_note_projectile_attached(long projectile_index, long target_object_index)
{
	struct projectile_datum *projectile;
	struct unit_datum *target;
	int local_player;

	if (!achievements_enabled() || target_object_index == NONE)
		return;
	projectile = (struct projectile_datum *)object_try_and_get(projectile_index);
	target = unit_try_and_get(target_object_index);
	if (!projectile || !target || target->unit.player_index != NONE ||
		!achievements_name_is(projectile->definition_index, "weapons\\plasma grenade\\plasma grenade"))
		return;
	local_player = achievements_local_player(projectile->object.owner_player_index);
	if (achievements_globals.logging && local_player >= 0)
		ach_platform_log("achievements: player %d stuck %s", local_player, tag_get_name(target->definition_index));
	if (local_player >= 0 && achievements_player(local_player) &&
		achievements_enemy_unit(target, projectile->object.owner_team_index))
		ach_rules_stuck(local_player);
}

void achievements_note_jump(long biped_index)
{
	int local_player;

	if (!achievements_enabled())
		return;
	local_player = achievements_local_player_of_unit(biped_index);
	if (achievements_globals.logging && local_player >= 0)
		ach_platform_log("achievements: player %d jumped", local_player);
	if (local_player >= 0 && achievements_player(local_player))
		ach_rules_jumped(local_player);
}

/* ---------- the unlock notice */

/* The notice: a dark pill near the bottom centre, a round slot at its left
for the achievement's icon, and two lines of the game's own text (small,
"Achievement unlocked", then the name), over a fade out. It is drawn in code
from rectangles, in the 480-line space of the menus and HUD, so it scales
with them, and costs about thirty rectangles while it shows.

ASSET SLOT: this is the placeholder. The animation and the sound the user
supplies replace achievements_notice_draw_custom() (drawn in its place when
it returns TRUE) and the sound played in ach_platform_unlocked_sound();
port/assets/achievements/README.md is their specification. */

static boolean achievements_notice_draw_custom(int achievement, unsigned long elapsed, rectangle2d const *window)
{
	(void)achievement;
	(void)elapsed;
	(void)window;
	return FALSE;
}

/* the square root of a whole number, rounded down */
static long achievements_isqrt(long value)
{
	long root = 0;

	while ((root + 1) * (root + 1) <= value)
		root++;
	return root;
}

/* a stadium (a circle if it is as wide as it is tall) filled in strips of rows */
static void achievements_fill_stadium(rectangle2d const *area, pixel32 color)
{
	long radius = (area->y1 - area->y0) / 2;
	long row;
	long step = radius >= 12 ? 2 : 1;

	for (row = 0; row < area->y1 - area->y0; row += step)
	{
		long offset = row + step / 2 - radius;
		long inset = radius - achievements_isqrt(radius * radius - offset * offset);
		rectangle2d strip;

		strip.x0 = (short)(area->x0 + inset);
		strip.x1 = (short)(area->x1 - inset);
		strip.y0 = (short)(area->y0 + row);
		strip.y1 = (short)MIN(area->y0 + row + step, area->y1);
		draw_quad(&strip, color);
	}
}

static void achievements_notice_text(rectangle2d const *bounds, char const *text, real alpha, real grey)
{
	long font_tag_index = interface_get_tag_index(_interface_font_system);
	real_argb_color color;

	if (font_tag_index == NONE)
		return;
	color.alpha = alpha;
	color.red = color.green = color.blue = grey;
	draw_string_set_draw_mode(font_tag_index, NONE, 0, 0, &color);
	rasterizer_draw_string(bounds, NULL, NULL, 0, text);
}

void achievements_render(void)
{
	unsigned long now;
	unsigned long elapsed;
	rectangle2d window;
	rectangle2d pill;
	rectangle2d icon;
	rectangle2d text;
	long alpha;
	real fade;
	long width = 300;
	long height = 48;

	if (!achievements_globals.queued)
		return;
	now = system_milliseconds();
	if (!achievements_globals.notice_start)
		achievements_globals.notice_start = now ? now : 1;
	elapsed = now - achievements_globals.notice_start;
	if (elapsed >= NOTICE_MILLISECONDS)
	{
		long index;

		for (index = 1; index < achievements_globals.queued; index++)
			achievements_globals.queue[index - 1] = achievements_globals.queue[index];
		achievements_globals.queued--;
		achievements_globals.notice_start = 0;
		return;
	}
	window = render.camera.window_bounds;
	offset_rectangle2d(&window, -render.camera.viewport_bounds.x0, -render.camera.viewport_bounds.y0);
	if (achievements_notice_draw_custom(achievements_globals.queue[0], elapsed, &window))
		return;
	/* (a fade over the last half second) */
	fade = elapsed > NOTICE_MILLISECONDS - 500 ? (real)(NOTICE_MILLISECONDS - elapsed) / 500.0f : 1.0f;
	alpha = (long)(fade * 224.0f);
	pill.x0 = (short)((window.x0 + window.x1 - width) / 2);
	pill.x1 = (short)(pill.x0 + width);
	pill.y1 = (short)(window.y1 - 40);
	pill.y0 = (short)(pill.y1 - height);
	achievements_fill_stadium(&pill, ((pixel32)alpha << 24) | 0x00141414);
	icon.x0 = (short)(pill.x0 + 5);
	icon.x1 = (short)(icon.x0 + height - 10);
	icon.y0 = (short)(pill.y0 + 5);
	icon.y1 = (short)(pill.y1 - 5);
	achievements_fill_stadium(&icon, ((pixel32)(alpha * 255 / 224) << 24) | 0x0058B848);
	text.x0 = (short)(icon.x1 + 8);
	text.x1 = (short)(pill.x1 - height / 2);
	text.y0 = (short)(pill.y0 + 3);
	text.y1 = (short)(text.y0 + 20);
	achievements_notice_text(&text, "Achievement unlocked", fade, 0.65f);
	text.y0 = (short)(text.y1 - 2);
	text.y1 = (short)(text.y0 + 22);
	achievements_notice_text(&text, ach_definitions[achievements_globals.queue[0]].name, fade, 1.0f);
}

/* the placeholder sound: the game's own checkpoint sound, played at the unlock
(the user's replaces it) */
static void ach_platform_unlocked_sound(void)
{
	if (game_in_progress() && hud_globals && hud_globals->checkpoint_sound.index != NONE)
		unspatialized_impulse_sound_new(hud_globals->checkpoint_sound.index, 1.0f);
}

#endif
