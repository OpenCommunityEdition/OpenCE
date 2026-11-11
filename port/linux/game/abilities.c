/*
ABILITIES.C

Sprint, which every player has, in every game (multiplayer, the campaign
and co-op), and armour lock, which a multiplayer gametype may give its
players (OPENCE OPTIONS' ARMOR LOCK: game_engine.h's
game_variant_options.armor_lock).

- Sprint (controls.sprint): held while moving forward on foot, standing, the
  player runs SPRINT_SPEED_SCALE times as fast with the weapon lowered (no
  trigger, grenade, reload or zoom). It lasts SPRINT_TICKS from full, and
  recharges in SPRINT_RECHARGE_TICKS after SPRINT_RECHARGE_DELAY_TICKS
  without it. Firing (or throwing a grenade) ends it, and the weapon comes up
  in SPRINT_RECOVERY_TICKS. Out of energy, or ended by firing, it starts
  again only once its control is let go and pressed again.
- Armour lock (controls.armor_lock): held on the ground, the player crouches
  locked in place, takes no damage (but the kill zones' and the scripts'
  instant kills) and can do nothing else. It lasts ARMOR_LOCK_TICKS from
  full and at least ARMOR_LOCK_MINIMUM_TICKS, and recharges in
  ARMOR_LOCK_RECHARGE_TICKS after ARMOR_LOCK_RECHARGE_DELAY_TICKS out of it.

The controls ride above the unit's own in the player's action (units.h), so
each machine runs a player's abilities from the input it runs their
movement with: a client its own players' at once, and the others' from
what the host relays (network_distributed.c sends them in a byte of their
own). Only the host deals damage (damage.c), so its copy of a player's
armour lock is the one that counts.
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/bipeds.h"
#include "units/units.h"
#include "abilities.h"

#define SPRINT_SPEED_SCALE 1.5f
/* the stick pushed forward at least this far */
#define SPRINT_MINIMUM_THROTTLE 0.5f
#define SPRINT_TICKS (5 * TICKS_PER_SECOND)
#define SPRINT_RECHARGE_TICKS (4 * TICKS_PER_SECOND)
#define SPRINT_RECHARGE_DELAY_TICKS (TICKS_PER_SECOND)
#define SPRINT_RECOVERY_TICKS (TICKS_PER_SECOND / 5)
/* (too little to be worth starting) */
#define SPRINT_MINIMUM_ENERGY 0.2f

#define ARMOR_LOCK_TICKS (4 * TICKS_PER_SECOND)
#define ARMOR_LOCK_MINIMUM_TICKS (TICKS_PER_SECOND / 2)
#define ARMOR_LOCK_RECHARGE_TICKS (8 * TICKS_PER_SECOND)
#define ARMOR_LOCK_RECHARGE_DELAY_TICKS (TICKS_PER_SECOND)
#define ARMOR_LOCK_MINIMUM_ENERGY 0.25f

/* the abilities a game gives, a bit each */
enum
{
	_ability_sprint_bit = 0,
	_ability_armor_lock_bit
};

/* what a sprint lowers the weapon for */
#define SPRINT_BLOCKED_CONTROL_FLAGS ( \
	FLAG(_unit_control_weapon_primary_trigger_bit) | \
	FLAG(_unit_control_weapon_secondary_trigger_bit) | \
	FLAG(_unit_control_throw_grenade_bit) | \
	FLAG(_unit_control_weapon_reload_bit))

struct ability_state
{
	/* the unit it is of (a new unit starts afresh), and the tick it was last
	run (a unit whose player has not been run since has no ability on) */
	long unit_index;
	long time;
	real sprint_energy;
	short sprint_recharge_delay;
	short sprint_recovery;
	boolean sprinting;
	boolean sprint_needs_release;
	real armor_lock_energy;
	short armor_lock_recharge_delay;
	short armor_lock_ticks;
	boolean armor_locked;
	boolean armor_lock_needs_release;
};

/* by absolute player index */
static struct ability_state abilities_states[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

/* the abilities the game gives (_ability_*_bit): sprint always, armour
lock in a gametype that gives it */
static byte abilities_enabled(
	void)
{
	byte enabled = FLAG(_ability_sprint_bit);

	if (game_engine_running() && game_variant_options_get()->armor_lock)
		enabled |= FLAG(_ability_armor_lock_bit);
	return enabled;
}

static struct ability_state *abilities_player_state(
	long player_index)
{
	long slot;

	if (player_index == NONE)
		return NULL;
	slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	return VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS) ? &abilities_states[slot] : NULL;
}

/* a unit's state, if its player's abilities were run this tick or the last */
static struct ability_state *abilities_current_unit_state(
	long unit_index)
{
	struct unit_datum *unit = unit_index != NONE ? unit_try_and_get(unit_index) : NULL;
	struct ability_state *state;
	long now;

	if (!unit || unit->unit.player_index == NONE || !abilities_enabled())
		return NULL;
	state = abilities_player_state(unit->unit.player_index);
	if (!state || state->unit_index != unit_index)
		return NULL;
	now = game_time_get();
	return now - state->time <= 1 && now >= state->time ? state : NULL;
}

static void abilities_state_new(
	struct ability_state *state,
	long unit_index)
{
	csmemset(state, 0, sizeof(*state));
	state->unit_index = unit_index;
	state->time = NONE;
	state->sprint_energy = 1.0f;
	state->armor_lock_energy = 1.0f;
}

static void abilities_sprint_end(
	struct ability_state *state,
	boolean held)
{
	state->sprinting = FALSE;
	state->sprint_recovery = SPRINT_RECOVERY_TICKS;
	state->sprint_recharge_delay = SPRINT_RECHARGE_DELAY_TICKS;
	state->sprint_needs_release = held;
}

static void abilities_armor_lock_end(
	struct ability_state *state,
	boolean held)
{
	state->armor_locked = FALSE;
	state->armor_lock_ticks = 0;
	state->armor_lock_recharge_delay = ARMOR_LOCK_RECHARGE_DELAY_TICKS;
	state->armor_lock_needs_release = held;
}

static void abilities_recharge(
	real *energy,
	short *delay,
	long recharge_ticks)
{
	if (*delay > 0)
		(*delay)--;
	else
		*energy = MIN(*energy + 1.0f / (real)recharge_ticks, 1.0f);
}

/* a tick of the state's armour lock and sprint, from the player's controls */
static void abilities_update(
	struct ability_state *state,
	byte enabled,
	struct unit_datum *unit,
	struct biped_datum *biped,
	struct player_action const *action)
{
	boolean sprint_held = TEST_FLAG(enabled, _ability_sprint_bit) &&
		TEST_FLAG(action->control_flags, UNIT_CONTROL_PORT_SPRINT_BIT);
	boolean armor_lock_held = TEST_FLAG(enabled, _ability_armor_lock_bit) &&
		TEST_FLAG(action->control_flags, UNIT_CONTROL_PORT_ARMOR_LOCK_BIT);
	boolean crouching = TEST_FLAG(action->control_flags, _unit_control_crouch_modifier_bit);
	boolean attacking = (action->control_flags & (FLAG(_unit_control_weapon_primary_trigger_bit) |
		FLAG(_unit_control_weapon_secondary_trigger_bit) | FLAG(_unit_control_throw_grenade_bit))) != 0;
	boolean on_foot = biped && unit->object.parent_object_index == NONE &&
		!TEST_FLAG(unit->object.damage_flags, _object_dead_bit);
	boolean grounded = on_foot && !TEST_FLAG(biped->biped.flags, _biped_airborne_bit);

	if (!sprint_held)
		state->sprint_needs_release = FALSE;
	if (!armor_lock_held)
		state->armor_lock_needs_release = FALSE;

	/* armour lock: held, for at least its least, while its energy lasts */
	if (state->armor_locked)
	{
		state->armor_lock_ticks++;
		state->armor_lock_energy = MAX(state->armor_lock_energy - 1.0f / (real)ARMOR_LOCK_TICKS, 0.0f);
		if (!on_foot || state->armor_lock_energy <= 0.0f ||
			(!armor_lock_held && state->armor_lock_ticks >= ARMOR_LOCK_MINIMUM_TICKS))
		{
			abilities_armor_lock_end(state, armor_lock_held);
		}
	}
	else if (armor_lock_held && grounded && !state->armor_lock_needs_release &&
		state->armor_lock_energy >= ARMOR_LOCK_MINIMUM_ENERGY)
	{
		state->armor_locked = TRUE;
		state->armor_lock_ticks = 0;
		if (state->sprinting)
			abilities_sprint_end(state, sprint_held);
	}
	else
	{
		abilities_recharge(&state->armor_lock_energy, &state->armor_lock_recharge_delay, ARMOR_LOCK_RECHARGE_TICKS);
	}

	/* sprint: held, forward, standing, on foot, the weapon not used, and not
	locked */
	if (state->sprinting)
	{
		state->sprint_energy = MAX(state->sprint_energy - 1.0f / (real)SPRINT_TICKS, 0.0f);
		if (!sprint_held || !on_foot || attacking || crouching || action->throttle.i < SPRINT_MINIMUM_THROTTLE ||
			state->sprint_energy <= 0.0f)
		{
			abilities_sprint_end(state, sprint_held && (attacking || state->sprint_energy <= 0.0f));
		}
	}
	else if (sprint_held && grounded && !state->armor_locked && !attacking && !crouching &&
		!state->sprint_needs_release && action->throttle.i >= SPRINT_MINIMUM_THROTTLE &&
		state->sprint_energy >= SPRINT_MINIMUM_ENERGY)
	{
		state->sprinting = TRUE;
		state->sprint_recovery = 0;
	}
	else
	{
		abilities_recharge(&state->sprint_energy, &state->sprint_recharge_delay, SPRINT_RECHARGE_TICKS);
		if (state->sprint_recovery > 0)
			state->sprint_recovery--;
	}
}

boolean abilities_player_action(
	long player_index,
	long unit_index,
	struct player_action *action)
{
	byte enabled = abilities_enabled();
	struct ability_state *state = abilities_player_state(player_index);
	struct unit_datum *unit = unit_try_and_get(unit_index);
	long now;

	if (enabled && state && unit)
	{
		now = game_time_get();
		/* (a new unit, or a new game, whose clock starts again and whose
		units may have the last one's indices) */
		if (state->unit_index != unit_index || (state->time != NONE && now < state->time))
			abilities_state_new(state, unit_index);
		/* (not run for a while, the player's input held off: what was on is
		off) */
		if (state->time != NONE && now - state->time > 1)
		{
			if (state->sprinting)
				abilities_sprint_end(state, FALSE);
			if (state->armor_locked)
				abilities_armor_lock_end(state, FALSE);
		}
		/* (once a tick, should a tick run the players twice) */
		if (state->time != now)
		{
			state->time = now;
			abilities_update(state, enabled, unit, biped_try_and_get(unit_index), action);
		}
	}
	/* (the controls never reach the unit: players.c gives it the low word) */
	action->control_flags &= ~(UNIT_CONTROL_PORT_ABILITIES_MASK << UNIT_CONTROL_PORT_ABILITIES_SHIFT);
	if (!enabled || !state || !unit || state->unit_index != unit_index)
		return FALSE;
	if (state->armor_locked)
	{
		/* (still, crouched, the weapon held as it is, nothing else) */
		action->control_flags = FLAG(_unit_control_crouch_modifier_bit);
		action->throttle.i = 0.0f;
		action->throttle.j = 0.0f;
		action->primary_trigger = 0.0f;
		action->desired_weapon_index = unit->unit.current_weapon_index;
		action->desired_zoom_level = NONE;
		return TRUE;
	}
	/* (the weapon lowered while sprinting, and on its way up after) */
	if (state->sprinting || state->sprint_recovery > 0)
	{
		action->control_flags &= ~SPRINT_BLOCKED_CONTROL_FLAGS;
		action->primary_trigger = 0.0f;
		action->desired_zoom_level = NONE;
	}
	return FALSE;
}

real abilities_speed_scale(
	long unit_index)
{
	struct ability_state *state = abilities_current_unit_state(unit_index);

	return state && state->sprinting ? SPRINT_SPEED_SCALE : 1.0f;
}

real abilities_maximum_speed_scale(
	void)
{
	return SPRINT_SPEED_SCALE;
}

boolean abilities_blocks_damage(
	long object_index)
{
	struct ability_state *state = abilities_current_unit_state(object_index);

	return state && state->armor_locked;
}

boolean abilities_unit_state(
	long unit_index,
	real *sprint_energy,
	boolean *sprinting,
	real *armor_lock_energy,
	boolean *armor_locked)
{
	struct ability_state *state = abilities_current_unit_state(unit_index);

	if (!state)
		return FALSE;
	*sprint_energy = state->sprint_energy;
	*sprinting = state->sprinting;
	*armor_lock_energy = state->armor_lock_energy;
	*armor_locked = state->armor_locked;
	return TRUE;
}
