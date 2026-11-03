/* bots_combat.h: the bots' fight by skill level, as rules (port/linux/game/bots_combat.c)

The skill levels of the bots (bots.skill), and what a bot of each level does
in a fight: rules of plain numbers, with no call into the engine, so that
the harness runs them as they are (tools/harness/tests/test_bots_levels.py).
bots.c reads the game and asks them.

The levels, from the weakest: RECRUIT, EASY, NORMAL, HEROIC, VETERAN,
LEGENDARY. Vitalities are fractions (1 full; a shield above 1 is an
overshield); distances are world units; times are ticks, 30 a second. A
`random` argument is a number from 0 to 1 (the bot's own random numbers, or
bots_combat_hash01's). */
#ifndef HALO_BOTS_COMBAT_H
#define HALO_BOTS_COMBAT_H

enum
{
	_bots_skill_recruit = 0,
	_bots_skill_easy,
	_bots_skill_normal,
	_bots_skill_heroic,
	_bots_skill_veteran,
	_bots_skill_legendary,
	NUMBER_OF_BOTS_SKILLS,
};

#define BOTS_COMBAT_TICKS_PER_SECOND 30

/* a number from 0 to 1 made from two others (the bot's player and a tick):
the same for the same pair, spread evenly; a decision's own random number,
which takes nothing from the bot's */
float bots_combat_hash01(unsigned long a, unsigned long b);

/* ---------- seeing and aiming */

/* the ticks from seeing a target to the first shot: the level's own time
0.85 to 1.35 of it, another for each sighting (RECRUIT's 1 to 1.5 of it:
never quicker than its own) */
long bots_combat_reaction_ticks(short skill, long skill_ticks, float random);
/* the ticks added to the look's three between looks (0, 1 or 2) */
long bots_combat_look_jitter(float random);
/* the ticks a bot's aim trails its target by (it aims where the target was
that long ago): RECRUIT 8, EASY 6, none above */
long bots_combat_track_delay_ticks(short skill);
/* RECRUIT's aim error (degrees, its yaw's or its pitch's with scale 0.6)
after the settling: a wander between 0.8 and 6 degrees off that never
settles; the error unchanged for the other levels */
float bots_combat_aim_wander(short skill, float error_degrees, float scale, float random_signed);
/* the least aim error (degrees) a bot keeps on a target this far away: none
within 10 units, then the level's degrees more every 10 units: RECRUIT 3,
EASY 2.6, NORMAL 2, HEROIC 1.6, VETERAN 1.2, LEGENDARY none */
float bots_combat_range_error_floor(short skill, float distance);
/* whether the bot aims at its target's head: NORMAL and up, the target's
shields down, with a weapon that kills with a shot to the head (the pistol,
the sniper rifle) */
int bots_combat_aim_head(short skill, float target_shield, int precision_weapon);
/* half the vertical view (degrees) about where the bot looks: RECRUIT 22,
EASY 25, NORMAL 28, HEROIC 32, VETERAN 36, LEGENDARY 40; and whether a target
elevation_degrees above (below: negative) the level of its eyes is in view
while it looks pitch_degrees up */
float bots_combat_vertical_half_view(short skill);
int bots_combat_in_vertical_view(short skill, float elevation_degrees, float pitch_degrees);

/* ---------- firing */

/* PACING (NORMAL and up): with a weapon whose error grows as it fires (the
pistol) at a target past 8 units, the shot is held while the trigger's error
(0 to 1) is above the level's limit (NORMAL 0.6, HEROIC 0.45, VETERAN 0.35,
LEGENDARY 0.25), for at most the level's ticks (6, 8, 10, 12). Nonzero: hold it */
int bots_combat_pace_holds(short skill, float distance, float trigger_error, long waited_ticks);
/* ZOOM (NORMAL and up): a weapon true only zoomed (the sniper rifle) is
zoomed at a target past 6 units; a hit knocks the zoom out, and the bot zooms
again after the level's moment (NORMAL 0.6 to 0.8 s, HEROIC 0.4 to 0.5,
VETERAN 0.25 to 0.35, LEGENDARY 0.15 to 0.22), holding its fire meanwhile past
10 units */
int bots_combat_zooms(short skill, float distance);
long bots_combat_rescope_ticks(short skill, float random);
int bots_combat_unzoomed_holds_fire(float distance);

/* ---------- blows */

/* whether the bot strikes now: 0 not now (out of reach, or still waiting
for the last blow's cooldown), 1 strike, 2 it chose not to (asked again a
second later). RECRUIT within 0.8 units, 4 times in 10; EASY within 1.1, 8
times in 10; NORMAL within 1.1; HEROIC and up within 1.1 at a target whose
back is turned, whose shields are low (HEROIC under 0.1, VETERAN and LEGENDARY
under 0.4) or that it charges, else only at point blank (0.6) */
int bots_combat_melee(short skill, float distance, int cooldown_over, int back_turned, float target_shield,
	int charging, float random);
/* the ticks it waits within reach before the blow: RECRUIT 1 to 3 s, EASY
0.6 to 1.5, NORMAL 0.35 to 0.7, HEROIC 0.25 to 0.5, VETERAN 0.18 to 0.35,
LEGENDARY 0.1 to 0.3 */
long bots_combat_melee_delay(short skill, float random);
/* the ticks before the next: RECRUIT 3 to 4.5 s, EASY 2 to 3, NORMAL 1.2 to
1.7, HEROIC 1 to 1.5, VETERAN 0.95 to 1.35, LEGENDARY 0.9 to 1.3 */
long bots_combat_melee_cooldown(short skill, float random);

/* ---------- grenades */

/* about to die: no shields and under 0.6 of its health */
int bots_combat_about_to_die(float own_shield, float own_body);
/* whether the bot throws now (asked each tick of a fight, a target 6 to 18
units off): RECRUIT never; EASY rarely, but often about to die; NORMAL at
the table's rate (its grenade_chance, three times a second's); HEROIC and up
that rate, more on first contact (the first 1.5 s of a fight) and about to
die */
int bots_combat_grenade(short skill, float distance, int has_grenade, int reacted, int first_contact,
	int about_to_die, float skill_chance, float random);
/* the ticks until it may throw again: RECRUIT 20 to 30 s, EASY 3 to 10.5,
NORMAL 1 to 6, HEROIC 0.5 to 5.5, VETERAN and LEGENDARY 0.1 to 5.1 */
long bots_combat_grenade_cooldown(short skill, float random);
/* how much of its target's path a throw leads: none up to NORMAL (at the
feet), all from HEROIC */
float bots_combat_grenade_lead(short skill);
/* the throw at the spot a target was last seen (VETERAN and up): lost from
sight 0.6 to 3 s ago, the spot 5 to 20 units off; VETERAN half the time,
LEGENDARY three times in four */
int bots_combat_prenade(short skill, float seconds_unseen, float distance, int has_grenade, float random);

/* ---------- moving in a fight */

/* how sure the bot is that it wins the fight, 0 to 1 (0.5 even): its own
shields and health against its target's (shields count more: they come
back), less with its magazine nearly empty or a target much further than its
weapon's distance, a little more from above */
float bots_combat_confidence(float own_shield, float own_body, float target_shield, float target_body,
	float distance, float weapon_range, float loaded, float height_above);

/* RETREAT (NORMAL and up): a confidence under 0.1, its shields under 0.3,
held for the bot's hesitation (6 to 18 ticks) starts it; VETERAN and up
also as its shields pop while it is losing (confidence under 0.35, after 3 to
9 ticks). It lasts at least 2 s (HEROIC and up 3 s), ends when its shields
are back (0.95) or its confidence is (0.6), after 7 s at most, and does not
start again for 6 s */
struct bots_combat_retreat
{
	int active;
	long low_since;
	long started;
	long next_allowed;
};
void bots_combat_retreat_reset(struct bots_combat_retreat *state);
/* the state's step at tick now: whether it retreats; *started and *ended
are set on the ticks it does */
int bots_combat_retreat_step(struct bots_combat_retreat *state, short skill, float confidence, float own_shield,
	long now, float hesitation_random, int *started, int *ended);
float bots_combat_retreat_threshold(short skill);

/* CLOSING IN (HEROIC and up): it runs in to finish a target it is sure of
(0.6), whose shields are down and health under half, 0.8 to 6 units off */
int bots_combat_charge(short skill, float confidence, float target_shield, float target_body, float distance);
/* STEPPING IN for a blow rather than backing off to its weapon's distance:
EASY within 2.5 units of a target whose shields are under 0.5, NORMAL within
3.5 (under 0.8), HEROIC and up within 4; RECRUIT never */
int bots_combat_step_in(short skill, float distance, float target_shield);

/* STRAFING: the ticks a strafe to one side lasts: RECRUIT 1 to 4 s, EASY 1.5
to 3, NORMAL 2 to 4, HEROIC 1 to 2, VETERAN and LEGENDARY 0.2 to 0.8 (a jiggle
from cover); and how hard it sidesteps (RECRUIT 0.15, the others fully) */
long bots_combat_strafe_ticks(short skill, float random);
float bots_combat_strafe_strength(short skill);
/* HALTS (NORMAL and up): the ticks it stands (keeping its weapon's
distance) at a new strafe, 0 for none: NORMAL 2 times in 10 for 1 to 2 s,
HEROIC 1 in 10 for 0.1 to 0.2 s, VETERAN and LEGENDARY 1 in 20 for 0.05 to 0.1
s; and at a fight's start, NORMAL and HEROIC up to 0.2 s, VETERAN and LEGENDARY
up to 0.1 s */
long bots_combat_halt_ticks(short skill, float chance_random, float length_random);
long bots_combat_initial_halt_ticks(short skill, float random);
/* JUMPS in a fight: the ticks to the next, 0 for a level that does not jump
in one: NORMAL every 3 to 5 s, HEROIC 1 to 4, VETERAN and LEGENDARY 1 to 3 */
long bots_combat_jump_delay(short skill, float random);
/* CROUCHING in a fight (NORMAL and up): the chance a second it starts a
crouch while it strafes (NORMAL 0.12, HEROIC 0.2, VETERAN and LEGENDARY 0.28);
a crouch lasts 0.3 to 0.6 s, then none for 2.5 s */
struct bots_combat_crouch
{
	long until;
	long next;
};
float bots_combat_crouch_chance(short skill);
int bots_combat_crouch_step(struct bots_combat_crouch *state, short skill, long now, float random);

/* ---------- roaming */

/* the chance a roam goes towards an enemy rather than anywhere: RECRUIT 0.2,
the others 0.65 (bots.c's) */
float bots_combat_hunt_chance(short skill);
/* HIGH GROUND (VETERAN and up): a roaming place's credit (units of
distance) for its height above the enemy hunted (or the bot): VETERAN 1.5 a
unit up to 4.5, LEGENDARY 2 a unit up to 6; none below */
float bots_combat_high_ground(short skill, float height_above);

#endif
