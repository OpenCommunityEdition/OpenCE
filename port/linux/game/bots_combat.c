/*
BOTS_COMBAT.C

The bots' fight by skill level (bots_combat.h): plain rules, no engine. Each
level's numbers were tuned in bot games on the stock maps, against players
and against bots of the other levels, so that each level plays clearly
better than the one below it: RECRUIT for new players, EASY and NORMAL for
casual games, HEROIC about even with a good player, VETERAN and LEGENDARY
above. NORMAL keeps the numbers bots.c's normal always had, and adds the
habits of a player of that level (pacing, halts, a retreat when losing, a
blow when near).
*/

#include "bots_combat.h"

#define TICKS BOTS_COMBAT_TICKS_PER_SECOND

static float bots_combat_clamp(float value, float low, float high)
{
	return value < low ? low : value > high ? high : value;
}

static float bots_combat_clamp01(float value)
{
	return bots_combat_clamp(value, 0.0f, 1.0f);
}

/* a time between low and high seconds, in ticks (rounded) */
static long bots_combat_seconds(float low, float high, float random)
{
	return (long)((low + (high - low) * bots_combat_clamp01(random)) * TICKS + 0.5f);
}

float bots_combat_hash01(unsigned long a, unsigned long b)
{
	/* (a 32-bit mix: the same in 32- and 64-bit builds) */
	unsigned long x = (a * 0x9E3779B1UL + b * 0x85EBCA77UL + 0x165667B1UL) & 0xFFFFFFFFUL;

	x ^= x >> 15;
	x = (x * 0x2C1B3C6DUL) & 0xFFFFFFFFUL;
	x ^= x >> 12;
	x = (x * 0x297A2D39UL) & 0xFFFFFFFFUL;
	x ^= x >> 15;
	return (float)(x & 0xFFFFFFUL) / (float)0x1000000UL;
}

/* ---------- seeing and aiming */

long bots_combat_reaction_ticks(short skill, long skill_ticks, float random)
{
	float low = skill == _bots_skill_recruit ? 1.0f : 0.85f;

	return (long)((float)skill_ticks * (low + 0.5f * bots_combat_clamp01(random)) + 0.5f);
}

long bots_combat_look_jitter(float random)
{
	long jitter = (long)(bots_combat_clamp01(random) * 3.0f);

	return jitter > 2 ? 2 : jitter;
}

long bots_combat_track_delay_ticks(short skill)
{
	return skill == _bots_skill_recruit ? 8 : skill == _bots_skill_easy ? 6 : 0;
}

float bots_combat_aim_wander(short skill, float error_degrees, float scale, float random_signed)
{
	float const low = 0.8f * scale, high = 6.0f * scale, step = 0.6f * scale;
	float value, magnitude, sign;

	if (skill != _bots_skill_recruit)
		return error_degrees;
	value = error_degrees + bots_combat_clamp(random_signed, -1.0f, 1.0f) * step;
	magnitude = value < 0.0f ? -value : value;
	sign = value < 0.0f ? -1.0f : 1.0f;
	if (magnitude < low)
		value += sign * 0.5f * step;
	if (magnitude > high)
		value = sign * high;
	return value;
}

float bots_combat_range_error_floor(short skill, float distance)
{
	static float const slopes[NUMBER_OF_BOTS_SKILLS] = { 3.0f, 2.6f, 2.0f, 1.6f, 1.2f, 0.0f };

	if (skill < 0 || skill >= NUMBER_OF_BOTS_SKILLS || distance <= 10.0f)
		return 0.0f;
	return slopes[skill] * (distance - 10.0f) / 10.0f;
}

int bots_combat_aim_head(short skill, float target_shield, int precision_weapon)
{
	return skill >= _bots_skill_normal && target_shield <= 0.0f && precision_weapon;
}

float bots_combat_vertical_half_view(short skill)
{
	static float const halves[NUMBER_OF_BOTS_SKILLS] = { 22.0f, 25.0f, 28.0f, 32.0f, 36.0f, 40.0f };

	return skill >= 0 && skill < NUMBER_OF_BOTS_SKILLS ? halves[skill] : 40.0f;
}

int bots_combat_in_vertical_view(short skill, float elevation_degrees, float pitch_degrees)
{
	float off = elevation_degrees - pitch_degrees;

	if (off < 0.0f)
		off = -off;
	return off <= bots_combat_vertical_half_view(skill);
}

/* ---------- firing */

int bots_combat_pace_holds(short skill, float distance, float trigger_error, long waited_ticks)
{
	static float const limits[NUMBER_OF_BOTS_SKILLS] = { 2.0f, 2.0f, 0.6f, 0.45f, 0.35f, 0.25f };
	static long const waits[NUMBER_OF_BOTS_SKILLS] = { 0, 0, 6, 8, 10, 12 };

	if (skill < _bots_skill_normal || skill >= NUMBER_OF_BOTS_SKILLS || distance <= 8.0f)
		return 0;
	return trigger_error > limits[skill] && waited_ticks < waits[skill];
}

int bots_combat_zooms(short skill, float distance)
{
	return skill >= _bots_skill_normal && skill < NUMBER_OF_BOTS_SKILLS && distance > 6.0f;
}

long bots_combat_rescope_ticks(short skill, float random)
{
	switch (skill)
	{
	case _bots_skill_normal: return bots_combat_seconds(0.6f, 0.8f, random);
	case _bots_skill_heroic: return bots_combat_seconds(0.4f, 0.5f, random);
	case _bots_skill_veteran: return bots_combat_seconds(0.25f, 0.35f, random);
	case _bots_skill_legendary: return bots_combat_seconds(0.15f, 0.22f, random);
	default: return 0;
	}
}

int bots_combat_unzoomed_holds_fire(float distance)
{
	return distance > 10.0f;
}

/* ---------- blows */

int bots_combat_melee(short skill, float distance, int cooldown_over, int back_turned, float target_shield,
	int charging, float random)
{
	float reach = 1.1f, chance = 1.0f;

	if (!cooldown_over)
		return 0;
	switch (skill)
	{
	case _bots_skill_recruit:
		reach = 0.8f;
		chance = 0.4f;
		break;
	case _bots_skill_easy:
		chance = 0.8f;
		break;
	case _bots_skill_normal:
		break;
	default:
		/* (HEROIC and up: a blow when it tells, else only at point blank) */
		if (!back_turned && target_shield >= (skill >= _bots_skill_veteran ? 0.4f : 0.1f) && !charging)
			reach = 0.6f;
		break;
	}
	if (distance >= reach)
		return 0;
	return random < chance ? 1 : 2;
}

long bots_combat_melee_delay(short skill, float random)
{
	switch (skill)
	{
	case _bots_skill_recruit: return bots_combat_seconds(1.0f, 3.0f, random);
	case _bots_skill_easy: return bots_combat_seconds(0.6f, 1.5f, random);
	case _bots_skill_normal: return bots_combat_seconds(0.35f, 0.7f, random);
	case _bots_skill_heroic: return bots_combat_seconds(0.25f, 0.5f, random);
	case _bots_skill_veteran: return bots_combat_seconds(0.18f, 0.35f, random);
	default: return bots_combat_seconds(0.1f, 0.3f, random);
	}
}

long bots_combat_melee_cooldown(short skill, float random)
{
	float seconds;

	random = bots_combat_clamp01(random);
	switch (skill)
	{
	case _bots_skill_recruit: seconds = 3.0f + 1.5f * random; break;
	case _bots_skill_easy: seconds = 2.0f + random; break;
	case _bots_skill_normal: seconds = 1.2f + 0.5f * random; break;
	case _bots_skill_heroic: seconds = 1.0f + 0.5f * random; break;
	case _bots_skill_veteran: seconds = 0.95f + 0.4f * random; break;
	default: seconds = 0.9f + 0.4f * random; break;
	}
	return (long)(seconds * TICKS);
}

/* ---------- grenades */

int bots_combat_about_to_die(float own_shield, float own_body)
{
	return own_shield <= 0.02f && own_body < 0.6f;
}

int bots_combat_grenade(short skill, float distance, int has_grenade, int reacted, int first_contact,
	int about_to_die, float skill_chance, float random)
{
	float rate;

	if (!has_grenade || !reacted || distance <= 6.0f || distance >= 18.0f)
		return 0;
	switch (skill)
	{
	case _bots_skill_recruit:
		return 0;
	case _bots_skill_easy:
		rate = about_to_die ? 0.6f : 0.02f;
		break;
	case _bots_skill_normal:
		rate = skill_chance * 3.0f;
		break;
	default:
		rate = skill_chance * 3.0f + (first_contact ? 0.5f : 0.0f) + (about_to_die ? 0.6f : 0.0f);
		break;
	}
	return random < rate / (float)TICKS;
}

long bots_combat_grenade_cooldown(short skill, float random)
{
	float seconds;

	random = bots_combat_clamp01(random);
	switch (skill)
	{
	case _bots_skill_recruit: seconds = 20.0f + 10.0f * random; break;
	case _bots_skill_easy: seconds = 3.0f + 7.5f * random; break;
	case _bots_skill_normal: seconds = 1.0f + 5.0f * random; break;
	case _bots_skill_heroic: seconds = 0.5f + 5.0f * random; break;
	default: seconds = 0.1f + 5.0f * random; break;
	}
	return (long)(seconds * TICKS);
}

float bots_combat_grenade_lead(short skill)
{
	return skill >= _bots_skill_heroic ? 1.0f : 0.0f;
}

int bots_combat_prenade(short skill, float seconds_unseen, float distance, int has_grenade, float random)
{
	float chance = skill == _bots_skill_legendary ? 0.75f : skill == _bots_skill_veteran ? 0.5f : 0.0f;

	return has_grenade && seconds_unseen >= 0.6f && seconds_unseen <= 3.0f && distance >= 5.0f && distance <= 20.0f &&
		random < chance;
}

/* ---------- moving in a fight */

float bots_combat_confidence(float own_shield, float own_body, float target_shield, float target_body,
	float distance, float weapon_range, float loaded, float height_above)
{
	float own = 0.6f * bots_combat_clamp(own_shield, 0.0f, 2.0f) + 0.4f * bots_combat_clamp01(own_body);
	float target = 0.6f * bots_combat_clamp(target_shield, 0.0f, 2.0f) + 0.4f * bots_combat_clamp01(target_body);
	float confidence = 0.5f + 0.5f * (own - target);

	if (loaded < 0.15f)
		confidence -= 0.15f;
	if (weapon_range > 0.0f && distance > weapon_range * 1.5f)
		confidence -= 0.05f;
	confidence += bots_combat_clamp(height_above / 5.0f, -0.1f, 0.1f);
	return bots_combat_clamp01(confidence);
}

float bots_combat_retreat_threshold(short skill)
{
	return skill >= _bots_skill_normal && skill < NUMBER_OF_BOTS_SKILLS ? 0.1f : 0.0f;
}

void bots_combat_retreat_reset(struct bots_combat_retreat *state)
{
	state->active = 0;
	state->low_since = -1;
	state->started = 0;
	state->next_allowed = 0;
}

int bots_combat_retreat_step(struct bots_combat_retreat *state, short skill, float confidence, float own_shield,
	long now, float hesitation_random, int *started, int *ended)
{
	float threshold = bots_combat_retreat_threshold(skill);
	long hesitation = 6 + (long)(12.0f * bots_combat_clamp01(hesitation_random));
	long shortest = skill >= _bots_skill_heroic ? 3 * TICKS : 2 * TICKS;
	int popped;

	*started = 0;
	*ended = 0;
	if (state->active)
	{
		long length = now - state->started;

		if (length >= 7 * TICKS || (length >= shortest && (own_shield >= 0.95f || confidence >= 0.6f)) ||
			threshold <= 0.0f)
		{
			state->active = 0;
			state->next_allowed = now + 6 * TICKS;
			state->low_since = -1;
			*ended = 1;
		}
		return state->active;
	}
	/* (VETERAN and up: also as its shields pop while it is losing, after a
	shorter hesitation) */
	popped = skill >= _bots_skill_veteran && own_shield <= 0.0f && confidence < 0.35f;
	if (threshold <= 0.0f || now < state->next_allowed ||
		(!popped && (confidence >= threshold || own_shield >= 0.3f)))
	{
		state->low_since = -1;
		return 0;
	}
	if (state->low_since < 0 || state->low_since > now)
		state->low_since = now;
	if (popped)
		hesitation = 3 + (long)(6.0f * bots_combat_clamp01(hesitation_random));
	if (now - state->low_since < hesitation)
		return 0;
	state->active = 1;
	state->started = now;
	*started = 1;
	return 1;
}

int bots_combat_charge(short skill, float confidence, float target_shield, float target_body, float distance)
{
	return skill >= _bots_skill_heroic && confidence >= 0.6f && target_shield <= 0.05f && target_body < 0.5f &&
		distance < 6.0f && distance > 0.8f;
}

int bots_combat_step_in(short skill, float distance, float target_shield)
{
	switch (skill)
	{
	case _bots_skill_easy: return distance < 2.5f && target_shield < 0.5f;
	case _bots_skill_normal: return distance < 3.5f && target_shield < 0.8f;
	case _bots_skill_heroic:
	case _bots_skill_veteran:
	case _bots_skill_legendary: return distance < 4.0f;
	default: return 0;
	}
}

long bots_combat_strafe_ticks(short skill, float random)
{
	switch (skill)
	{
	case _bots_skill_recruit: return bots_combat_seconds(1.0f, 4.0f, random);
	case _bots_skill_easy: return bots_combat_seconds(1.5f, 3.0f, random);
	case _bots_skill_normal: return bots_combat_seconds(2.0f, 4.0f, random);
	case _bots_skill_heroic: return bots_combat_seconds(1.0f, 2.0f, random);
	default: return bots_combat_seconds(0.2f, 0.8f, random);
	}
}

float bots_combat_strafe_strength(short skill)
{
	return skill == _bots_skill_recruit ? 0.15f : 1.0f;
}

long bots_combat_halt_ticks(short skill, float chance_random, float length_random)
{
	float chance, low, high;

	switch (skill)
	{
	/* (RECRUIT and EASY: none: a bot that stands aims far better, and these
	levels must stay easy to beat) */
	case _bots_skill_recruit:
	case _bots_skill_easy:
		return 0;
	case _bots_skill_normal: chance = 0.2f; low = 1.0f; high = 2.0f; break;
	case _bots_skill_heroic: chance = 0.1f; low = 0.1f; high = 0.2f; break;
	default: chance = 0.05f; low = 0.05f; high = 0.1f; break;
	}
	if (chance_random >= chance)
		return 0;
	return bots_combat_seconds(low, high, length_random);
}

long bots_combat_initial_halt_ticks(short skill, float random)
{
	switch (skill)
	{
	case _bots_skill_recruit:
	case _bots_skill_easy:
		return 0;
	case _bots_skill_veteran:
	case _bots_skill_legendary:
		return bots_combat_seconds(0.0f, 0.1f, random);
	default:
		return bots_combat_seconds(0.0f, 0.2f, random);
	}
}

long bots_combat_jump_delay(short skill, float random)
{
	switch (skill)
	{
	case _bots_skill_normal: return bots_combat_seconds(3.0f, 5.0f, random);
	case _bots_skill_heroic: return bots_combat_seconds(1.0f, 4.0f, random);
	case _bots_skill_veteran:
	case _bots_skill_legendary: return bots_combat_seconds(1.0f, 3.0f, random);
	default: return 0;
	}
}

float bots_combat_crouch_chance(short skill)
{
	switch (skill)
	{
	case _bots_skill_normal: return 0.12f;
	case _bots_skill_heroic: return 0.20f;
	case _bots_skill_veteran:
	case _bots_skill_legendary: return 0.28f;
	default: return 0.0f;
	}
}

int bots_combat_crouch_step(struct bots_combat_crouch *state, short skill, long now, float random)
{
	float chance = bots_combat_crouch_chance(skill);

	if (chance <= 0.0f)
		return 0;
	/* (times further ahead than a crouch and its wait are a game before's:
	the game's ticks started over) */
	if (state->until > now + TICKS || state->next > now + TICKS * 10)
	{
		state->until = 0;
		state->next = 0;
	}
	if (now < state->until)
		return 1;
	if (now < state->next || random >= chance / TICKS)
		return 0;
	/* (random is under chance / TICKS here: spread over 0 to 1 again for the
	crouch's length) */
	state->until = now + (long)(TICKS * (0.3f + 0.3f * random * TICKS / chance));
	state->next = state->until + TICKS * 5 / 2;
	return 1;
}

/* ---------- roaming */

float bots_combat_hunt_chance(short skill)
{
	return skill == _bots_skill_recruit ? 0.2f : 0.65f;
}

float bots_combat_high_ground(short skill, float height_above)
{
	float credit;

	if (height_above <= 0.0f)
		return 0.0f;
	if (skill == _bots_skill_veteran)
	{
		credit = 1.5f * height_above;
		return credit < 4.5f ? credit : 4.5f;
	}
	if (skill == _bots_skill_legendary)
	{
		credit = 2.0f * height_above;
		return credit < 6.0f ? credit : 6.0f;
	}
	return 0.0f;
}
