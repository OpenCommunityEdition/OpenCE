/*
DUALSENSE_RULES.C

A PS5 DualSense's output bytes (dualsense_rules.h). The zone-based trigger effects are packed as in John "Nielk1"
Klein's TriggerEffectGenerator (MIT licence, 2021-2022); the body layout, the player lights' patterns and the report
framing follow SDL3's PS5 driver (zlib licence). Pure: no SDL, no game.
*/

#include <errno.h>
#include <string.h>

#include "dualsense_rules.h"

static int clamp(int value, int low, int high)
{
	return value < low ? low : value > high ? high : value;
}

enum dualsense_weapon_kind dualsense_weapon_kind(int needler, float rate_of_fire, float charging_time,
	float heat_per_round, int zoom_levels, int no_projectile, int plasma_rifle_type)
{
	if (no_projectile)
		return DUALSENSE_WEAPON_CARRY;
	if (charging_time > 0.0f)
		return DUALSENSE_WEAPON_CHARGE;
	if (zoom_levels >= 2 && rate_of_fire < 3.0f)
		return DUALSENSE_WEAPON_SNIPER;
	if (needler)
		return DUALSENSE_WEAPON_NEEDLER;
	if (heat_per_round > 0.0f && rate_of_fire >= 4.0f)
		return plasma_rifle_type ? DUALSENSE_WEAPON_PLASMA_HEAT : DUALSENSE_WEAPON_FLAME;
	if (rate_of_fire >= 6.0f)
		return DUALSENSE_WEAPON_AUTOMATIC;
	if (rate_of_fire > 0.0f && rate_of_fire < 1.6f)
		return DUALSENSE_WEAPON_HEAVY;
	return DUALSENSE_WEAPON_SEMI;
}

char const *dualsense_weapon_kind_name(enum dualsense_weapon_kind kind)
{
	static char const *const names[DUALSENSE_WEAPON_KINDS] = {
		"none", "semi", "heavy", "automatic", "sniper", "charge", "needler", "plasma heat", "carry", "flame" };

	return kind >= 0 && kind < DUALSENSE_WEAPON_KINDS ? names[kind] : "?";
}

void dualsense_effect_off(unsigned char effect[DUALSENSE_EFFECT_SIZE])
{
	memset(effect, 0, DUALSENSE_EFFECT_SIZE);
	effect[0] = 0x05;
}

/* zones `position` to `last` active, each with the same 3-bit level (Feedback and Vibration share it) */
static void zones(unsigned char effect[DUALSENSE_EFFECT_SIZE], int position, int last, int level)
{
	unsigned long active = 0, levels = 0;
	int zone;

	for (zone = position; zone <= last && zone < 10; zone++)
	{
		active |= 1UL << zone;
		levels |= (unsigned long)(level & 7) << (3 * zone);
	}
	effect[1] = (unsigned char)(active & 0xff);
	effect[2] = (unsigned char)((active >> 8) & 0xff);
	effect[3] = (unsigned char)(levels & 0xff);
	effect[4] = (unsigned char)((levels >> 8) & 0xff);
	effect[5] = (unsigned char)((levels >> 16) & 0xff);
	effect[6] = (unsigned char)((levels >> 24) & 0xff);
}

void dualsense_effect_feedback(unsigned char effect[DUALSENSE_EFFECT_SIZE], int position, int strength)
{
	if (strength <= 0)
	{
		dualsense_effect_off(effect);
		return;
	}
	memset(effect, 0, DUALSENSE_EFFECT_SIZE);
	effect[0] = 0x21;
	zones(effect, clamp(position, 0, 9), 9, clamp(strength, 1, 8) - 1);
}

void dualsense_effect_feedback_span(unsigned char effect[DUALSENSE_EFFECT_SIZE], int first, int last, int strength)
{
	if (strength <= 0)
	{
		dualsense_effect_off(effect);
		return;
	}
	first = clamp(first, 0, 9);
	memset(effect, 0, DUALSENSE_EFFECT_SIZE);
	effect[0] = 0x21;
	zones(effect, first, clamp(last, first, 9), clamp(strength, 1, 8) - 1);
}

void dualsense_effect_weapon(unsigned char effect[DUALSENSE_EFFECT_SIZE], int start, int end, int strength)
{
	unsigned long both;

	if (strength <= 0)
	{
		dualsense_effect_off(effect);
		return;
	}
	start = clamp(start, 2, 7);
	end = clamp(end, start + 1, 8);
	both = (1UL << start) | (1UL << end);
	memset(effect, 0, DUALSENSE_EFFECT_SIZE);
	effect[0] = 0x25;
	effect[1] = (unsigned char)(both & 0xff);
	effect[2] = (unsigned char)((both >> 8) & 0xff);
	effect[3] = (unsigned char)(clamp(strength, 1, 8) - 1);
}

void dualsense_effect_vibration(unsigned char effect[DUALSENSE_EFFECT_SIZE], int position, int amplitude,
	int frequency)
{
	if (amplitude <= 0 || frequency <= 0)
	{
		dualsense_effect_off(effect);
		return;
	}
	memset(effect, 0, DUALSENSE_EFFECT_SIZE);
	effect[0] = 0x26;
	zones(effect, clamp(position, 0, 9), 9, clamp(amplitude, 1, 8) - 1);
	effect[9] = (unsigned char)clamp(frequency, 1, 255);
}

/* a shape: OFF, FEEDBACK (a position, b strength), WEAPON (a start, b strength, c end), VIBRATION (a position,
b amplitude, c frequency; c 0: the gun's rate of fire times the profile's per_round) */
enum { OFF, FEEDBACK, WEAPON, VIBRATION, SAME };
struct shape
{
	unsigned char mode, a, b, c;
};

/* ONE profile a weapon kind (the trigger strength setting scales every b):
- held: the trigger as it rests and is pulled;
- firing: while a round went in the last ~170 ms (SAME: the held one), e.g. a vibration at the rate of fire, one
  pulse a round, timed by the pad itself (no report a round);
- kick: a short shape sent at each round, then back to held/firing after kick_ms (two reports a round);
- ramp: 1 heat: the held and firing shapes' b goes from its table value at 0 to ramp_to at 1; 2 charge: while
  charging, a hum from the held shape's zone whose amplitude goes so;
- charged_kick: a fully charged shot's instead (the plasma pistol's blast);
- the weapon's state comes first, for every kind (dualsense_right_held). */
#define NO { OFF, 0, 0, 0 }
#define SM { SAME, 0, 0, 0 }
#define F(position, strength) { FEEDBACK, position, strength, 0 }
#define W(start, end, strength) { WEAPON, start, strength, end }
#define V(position, amplitude, frequency) { VIBRATION, position, amplitude, frequency }
static struct profile
{
	struct shape held, firing, kick;
	int kick_ms;
	int per_round;
	int ramp, ramp_to;
	char const *feel;
	struct shape charged_kick;
	int charged_kick_ms;
} const profiles[DUALSENSE_WEAPON_KINDS] = {
	/* (strong has room above normal: a hold at most 4 here, a click 5, a kick 7) */
	/* none */ { NO, SM, NO, 0, 0, 0, 0, "off", NO, 0 },
	/* semi (pistol): its click, a sharp snap back each shot (35 ms of 285) */
	{ W(4, 7, 4), SM, F(0, 7), 35, 0, 0, 0, "click, snap a shot", NO, 0 },
	/* heavy (shotgun, rockets): a heavy click, a big kick each shot (a 30 Hz jolt, 130 ms of ~700) */
	{ W(3, 7, 5), SM, V(0, 7, 30), 130, 0, 0, 0, "heavy click, big kick", NO, 0 },
	/* automatic (assault rifle): light hold; firing, round 1's buzz at 1x the rate of fire = a pulse a round */
	{ F(3, 2), V(3, 5, 0), NO, 0, 1, 0, 0, "light hold, a pulse a round", NO, 0 },
	/* sniper: firm, a heavy thump each shot (90 ms of 500) */
	{ F(2, 4), SM, F(0, 7), 90, 0, 0, 0, "firm, heavy thump", NO, 0 },
	/* charge (plasma pistol): a hold from zone 2; while it charges a hum from there rising to full; each ordinary
	shot a 40 Hz jolt, 70 ms; the overcharged release a longer, lower one (15 Hz, 240 ms) */
	{ F(2, 3), SM, V(0, 7, 40), 70, 0, 2, 8, "hums as it charges, a pulse a shot, a blast charged", V(0, 8, 15), 240 },
	/* needler: barely there; firing, a fast light buzz (2x the rate of fire, amplitude 3) */
	{ F(3, 2), V(3, 3, 0), NO, 0, 2, 0, 0, "fast light buzz", NO, 0 },
	/* plasma heat (plasma rifle): resistance and hum rising with heat (locked while overheated, as every weapon) */
	{ F(2, 2), V(2, 3, 0), NO, 0, 1, 1, 6, "rises with heat, locked overheated", NO, 0 },
	/* carry (ball, flag: no projectile, R2 swings it): free, nothing to shoot */
	{ NO, SM, NO, 0, 0, 0, 0, "free (melee carry)", NO, 0 },
	/* flame (flamethrower): light hold; firing, a low 18 Hz roar rising with heat */
	{ F(3, 2), V(3, 3, 18), NO, 0, 0, 1, 6, "light hold, a roar while it burns", NO, 0 },
};
#define THROB_TICKS 8
#define THROB_ON_TICKS 2

/* the charging hum's frequency (Hz), and the fully charged one's */
#define CHARGING_HUM_HZ 25
#define CHARGED_HUM_HZ 40
#define CHARGING_HUM_LOW 1
#define CHARGING_HUM_HIGH 3
/* L2's grenade: a hair trigger, the lightest tick that is still felt, early in the pull, nothing after it */
#define GRENADE_TICK_START 1
#define GRENADE_TICK_END 2
#define GRENADE_TICK_STRENGTH 2

static int ramped(int base, int to, float amount)
{
	if (amount <= 0.0f)
		return base;
	if (amount >= 1.0f)
		return to;
	return base + (int)((float)(to - base) * amount + 0.5f);
}

/* the table is for normal. Strong: a hold (resistance from a zone to the end of the pull) two steps up, at most
HOLD_STRONGEST so a full pull held through a fight does not tire; a click CLICK_STRONGEST; a kick or buzz two steps up
(at most 8). Light is a hair trigger on both triggers: every hold and click, whatever the weapon, a tiny tick early in
the pull (a resistance over zones HAIR_START to HAIR_END only, at HAIR_STRENGTH), so the pull is almost free; kicks
and buzzes at most LIGHT_PULSE_MOST */
#define STRONG_STEPS 2
#define HOLD_STRONGEST 6
#define CLICK_STRONGEST 7
#define HAIR_START 1
#define HAIR_END 2
#define HAIR_STRENGTH 2
#define LIGHT_PULSE_MOST 2

static int hair_trigger(int percent)
{
	return percent <= DUALSENSE_STRENGTH_LIGHT;
}

static int strong(int percent)
{
	return percent > 100;
}

/* a hold's or click's strength (normal's value) on strong, never above `strongest` (nor under the value) */
static int held_strong(int value, int strongest)
{
	int result = value + STRONG_STEPS;

	if (result > strongest)
		result = strongest > value ? strongest : value;
	return clamp(result, 1, 8);
}

/* light's tick: early in the pull, nothing after it */
static void hair_tick(unsigned char effect[DUALSENSE_EFFECT_SIZE])
{
	dualsense_effect_feedback_span(effect, HAIR_START, HAIR_END, HAIR_STRENGTH);
}

/* a kick's or buzz's strength / amplitude for the setting */
static int pulse_scaled(int value, int percent)
{
	if (value <= 0)
		return value;
	if (hair_trigger(percent))
		return value < LIGHT_PULSE_MOST ? value : LIGHT_PULSE_MOST;
	if (strong(percent))
		return clamp(value + STRONG_STEPS, 1, 8);
	return clamp(value, 1, 8);
}

/* the shape's bytes for the setting (the shape's numbers normal's) */
static void shape_effect(unsigned char effect[DUALSENSE_EFFECT_SIZE], struct shape shape, float rate_of_fire,
	int per_round, int percent)
{
	int frequency;

	switch (shape.mode)
	{
	case FEEDBACK:
		if (shape.a == 0)
			/* (from zone 0: a kick after the shot, or the overheat's lock) */
			dualsense_effect_feedback(effect, 0, pulse_scaled(shape.b, percent));
		else if (hair_trigger(percent))
			hair_tick(effect);
		else
			dualsense_effect_feedback(effect, shape.a,
				strong(percent) ? held_strong(shape.b, HOLD_STRONGEST) : clamp(shape.b, 1, 8));
		break;
	case WEAPON:
		if (hair_trigger(percent))
			hair_tick(effect);
		else
			dualsense_effect_weapon(effect, shape.a, shape.c,
				strong(percent) ? held_strong(shape.b, CLICK_STRONGEST) : clamp(shape.b, 1, 8));
		break;
	case VIBRATION:
		frequency = shape.c ? shape.c : clamp((int)(rate_of_fire * (float)(per_round > 0 ? per_round : 1) + 0.5f), 6, 60);
		dualsense_effect_vibration(effect, shape.a, pulse_scaled(shape.b, percent), frequency);
		break;
	default:
		dualsense_effect_off(effect);
		break;
	}
}

/* dry click (an empty weapon, L2 with no grenades): slack, then a weak hollow click at the bottom of the pull, on every
setting (the click is news, not resistance): light the faintest, strong a step firmer */
#define DRY_STRENGTH 2
static void dry_click(unsigned char effect[DUALSENSE_EFFECT_SIZE], int percent)
{
	dualsense_effect_weapon(effect, 7, 8, hair_trigger(percent) ? 1 : DRY_STRENGTH + (strong(percent) ? 1 : 0));
}

void dualsense_right_held(unsigned char effect[DUALSENSE_EFFECT_SIZE], struct dualsense_weapon_state const *state,
	int percent)
{
	struct profile const *profile =
		&profiles[state->kind >= 0 && state->kind < DUALSENSE_WEAPON_KINDS ? state->kind : 0];
	struct shape shape = profile->held;

	/* (the weapon's state first, for every kind: free while it comes up or reloads) */
	if (shape.mode == OFF || state->readying || (state->reloading && !state->overheated))
	{
		dualsense_effect_off(effect);
		return;
	}
	if (state->overheated)
	{
		/* locked solid, with a two-tick throb every eight, until the game lets it fire */
		struct shape locked = F(0, 8), throb = V(0, 8, 8);

		shape_effect(effect, state->ticks % THROB_TICKS < THROB_ON_TICKS ? throb : locked, 0.0f, 1, percent);
		return;
	}
	if (state->empty)
	{
		dry_click(effect, percent);
		return;
	}
	if (state->firing && profile->firing.mode != SAME)
		shape = profile->firing;
	if (profile->ramp == 1)
		shape.b = (unsigned char)ramped(shape.b, profile->ramp_to, state->heat);
	else if (profile->ramp == 2 && state->charge > 0.0f)
	{
		/* the trigger is held down while it charges, so a resistance building with the charge would not be felt: a
		vibration from the hold's zone, its amplitude rising with the charge, faster once fully charged */
		shape = (struct shape)V(profile->held.a, 0, 0);
		shape.b = (unsigned char)ramped(CHARGING_HUM_LOW, CHARGING_HUM_HIGH, state->charge);
		shape.c = (unsigned char)(state->charge >= 1.0f ? CHARGED_HUM_HZ : CHARGING_HUM_HZ);
	}
	shape_effect(effect, shape, state->rate_of_fire, profile->per_round, percent);
}

int dualsense_event(unsigned char effect[DUALSENSE_EFFECT_SIZE], enum dualsense_weapon_kind kind,
	enum dualsense_event event, int percent)
{
	struct profile const *profile = &profiles[kind >= 0 && kind < DUALSENSE_WEAPON_KINDS ? kind : 0];
	struct shape shape = NO;
	int ms = 0;

	switch (event)
	{
	case DUALSENSE_EVENT_SHOT_CHARGED:
		if (profile->charged_kick.mode != OFF && profile->charged_kick_ms > 0)
		{
			shape = profile->charged_kick;
			ms = profile->charged_kick_ms;
			break;
		}
		/* (no charged kick: an ordinary shot) */
		/* fall through */
	case DUALSENSE_EVENT_SHOT:
		shape = profile->kick;
		ms = profile->kick_ms;
		break;
	}
	if (shape.mode == OFF || ms <= 0)
	{
		dualsense_effect_off(effect);
		return 0;
	}
	shape_effect(effect, shape, 0.0f, 1, percent);
	return ms;
}

void dualsense_left_effect(unsigned char effect[DUALSENSE_EFFECT_SIZE], int zoomed, int grenades, int winding,
	int percent)
{
	/* the zoom's hold: light none, normal feedback 2, strong 3; grenades in hand the hair tick (zones 1-2,
	strength 2) on every setting; wound up, free */
	if (zoomed)
	{
		if (hair_trigger(percent))
			dualsense_effect_off(effect);
		else
			dualsense_effect_feedback(effect, 2, strong(percent) ? 3 : 2);
	}
	else if (winding)
		dualsense_effect_off(effect);
	else if (grenades > 0)
		dualsense_effect_feedback_span(effect, GRENADE_TICK_START, GRENADE_TICK_END, GRENADE_TICK_STRENGTH);
	else
		dry_click(effect, percent);
}

int dualsense_strength_percent(char const *name)
{
	if (!strcmp(name, "light"))
		return DUALSENSE_STRENGTH_LIGHT;
	if (!strcmp(name, "strong"))
		return DUALSENSE_STRENGTH_STRONG;
	return 100;
}

int dualsense_is_dualsense(int vendor, int product)
{
	return vendor == 0x054c && (product == 0x0ce6 || product == 0x0df2);
}

/* (SDL's PS5 driver's pattern for each player, zlib licence: 1 the middle light, 2 the two beside it, 3 the middle
and the two outer ones, 4 all but the middle; 0x20 changes them at once instead of fading) */
static unsigned char const player_lights[4] = { 0x04, 0x0a, 0x15, 0x1b };

unsigned char dualsense_player_lights(int player_number)
{
	if (player_number < 1 || player_number > 4)
		return 0;
	return (unsigned char)(player_lights[player_number - 1] | 0x20);
}

int dualsense_player_index_of_lights(unsigned char lights)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		if ((lights & 0x1f) == player_lights[i])
			return i;
	}
	return -1;
}

unsigned long dualsense_crc32(unsigned long crc, unsigned char const *data, size_t size)
{
	size_t i;
	int bit;

	crc = ~crc & 0xffffffffUL;
	for (i = 0; i < size; i++)
	{
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
	}
	return ~crc & 0xffffffffUL;
}

size_t dualsense_output_report(unsigned char *report, size_t size, int bluetooth, int sequence,
	unsigned char const body[DUALSENSE_BODY_SIZE])
{
	if (!bluetooth)
	{
		if (size < DUALSENSE_USB_REPORT_SIZE)
			return 0;
		memset(report, 0, DUALSENSE_USB_REPORT_SIZE);
		report[0] = 0x02;
		memcpy(report + 1, body, DUALSENSE_BODY_SIZE);
		return DUALSENSE_USB_REPORT_SIZE;
	}
	else
	{
		static unsigned char const hidp_header = 0xA2; /* the Bluetooth HID "data, output" header, part of the CRC */
		unsigned long crc;

		if (size < DUALSENSE_BLUETOOTH_REPORT_SIZE)
			return 0;
		memset(report, 0, DUALSENSE_BLUETOOTH_REPORT_SIZE);
		report[0] = 0x31;
		report[1] = (unsigned char)((sequence & 0x0f) << 4);
		report[2] = 0x10;
		memcpy(report + 3, body, DUALSENSE_BODY_SIZE);
		crc = dualsense_crc32(0, &hidp_header, 1);
		crc = dualsense_crc32(crc, report, DUALSENSE_BLUETOOTH_REPORT_SIZE - 4);
		report[74] = (unsigned char)(crc & 0xff);
		report[75] = (unsigned char)((crc >> 8) & 0xff);
		report[76] = (unsigned char)((crc >> 16) & 0xff);
		report[77] = (unsigned char)((crc >> 24) & 0xff);
		return DUALSENSE_BLUETOOTH_REPORT_SIZE;
	}
}

/* ---- one report at a time a pad (dualsense_rules.h: the link) ---- */

/* a field's bytes in the body */
struct field_span
{
	int field;
	int at, size;
};
static struct field_span const field_spans[] =
{
	{ DUALSENSE_FIELD_TRIGGERS, DUALSENSE_BODY_RIGHT_TRIGGER, 2 * DUALSENSE_EFFECT_SIZE },
	{ DUALSENSE_FIELD_LIGHTBAR, DUALSENSE_BODY_RED, 3 },
	{ DUALSENSE_FIELD_PLAYER_LIGHTS, DUALSENSE_BODY_PLAYER_LIGHTS, 1 }
};
#define FIELD_SPANS ((int)(sizeof(field_spans) / sizeof(field_spans[0])))

enum dualsense_write_result dualsense_write_result_of_errno(int error)
{
	switch (error)
	{
	/* the queue full, or a moment's shortage: the next window */
	case EAGAIN:
	case EINTR:
	case ENOMEM:
#ifdef ENOBUFS
	case ENOBUFS:
#endif
		return DUALSENSE_WRITE_BUSY;
	/* the device went: a Bluetooth link lost, the cable pulled */
	case ENODEV:
	case EIO:
	case ENXIO:
	case EPIPE:
		return DUALSENSE_WRITE_GONE;
	default:
		return DUALSENSE_WRITE_REFUSED;
	}
}

void dualsense_link_init(struct dualsense_link *link, int bluetooth)
{
	memset(link, 0, sizeof(*link));
	link->bluetooth = bluetooth != 0;
}

void dualsense_link_forget(struct dualsense_link *link)
{
	link->known = 0;
	link->open_ms = 0;
}

static void link_want(struct dualsense_link *link, int field, int at, unsigned char const *bytes, int size)
{
	memcpy(link->want + at, bytes, (size_t)size);
	link->wanted |= field;
}

void dualsense_link_want_triggers(struct dualsense_link *link, unsigned char const right[DUALSENSE_EFFECT_SIZE],
	unsigned char const left[DUALSENSE_EFFECT_SIZE])
{
	link_want(link, DUALSENSE_FIELD_TRIGGERS, DUALSENSE_BODY_RIGHT_TRIGGER, right, DUALSENSE_EFFECT_SIZE);
	link_want(link, DUALSENSE_FIELD_TRIGGERS, DUALSENSE_BODY_LEFT_TRIGGER, left, DUALSENSE_EFFECT_SIZE);
}

void dualsense_link_want_lightbar(struct dualsense_link *link, int red, int green, int blue)
{
	unsigned char rgb[3];

	rgb[0] = (unsigned char)clamp(red, 0, 255);
	rgb[1] = (unsigned char)clamp(green, 0, 255);
	rgb[2] = (unsigned char)clamp(blue, 0, 255);
	link_want(link, DUALSENSE_FIELD_LIGHTBAR, DUALSENSE_BODY_RED, rgb, 3);
}

void dualsense_link_want_player_lights(struct dualsense_link *link, unsigned char lights)
{
	link_want(link, DUALSENSE_FIELD_PLAYER_LIGHTS, DUALSENSE_BODY_PLAYER_LIGHTS, &lights, 1);
}

void dualsense_link_force(struct dualsense_link *link, int fields)
{
	link->forced |= fields & DUALSENSE_FIELDS;
}

void dualsense_link_drop(struct dualsense_link *link, int fields)
{
	link->wanted &= ~fields;
	link->forced &= ~fields;
}

int dualsense_link_dirty(struct dualsense_link const *link)
{
	int dirty = link->wanted & (~link->known | link->forced);
	int i;

	for (i = 0; i < FIELD_SPANS; i++)
	{
		struct field_span const *span = &field_spans[i];

		if ((link->wanted & span->field) && !(dirty & span->field) &&
			memcmp(link->want + span->at, link->sent + span->at, (size_t)span->size))
			dirty |= span->field;
	}
	return dirty & DUALSENSE_FIELDS;
}

/* the body for these fields: their enable bits, the wanted bytes */
static void link_body(struct dualsense_link const *link, int fields, unsigned char body[DUALSENSE_BODY_SIZE])
{
	memcpy(body, link->want, DUALSENSE_BODY_SIZE);
	body[DUALSENSE_BODY_ENABLE1] = 0;
	body[DUALSENSE_BODY_ENABLE2] = 0;
	if (fields & DUALSENSE_FIELD_TRIGGERS)
		body[DUALSENSE_BODY_ENABLE1] |= DUALSENSE_ENABLE1_RIGHT_TRIGGER | DUALSENSE_ENABLE1_LEFT_TRIGGER;
	if (fields & DUALSENSE_FIELD_LIGHTBAR)
		body[DUALSENSE_BODY_ENABLE2] |= DUALSENSE_ENABLE2_LIGHTBAR;
	if (fields & DUALSENSE_FIELD_PLAYER_LIGHTS)
		body[DUALSENSE_BODY_ENABLE2] |= DUALSENSE_ENABLE2_PLAYER_LIGHTS;
}

static int link_due(struct dualsense_link *link, unsigned long long now, unsigned char body[DUALSENSE_BODY_SIZE],
	int window)
{
	int dirty = dualsense_link_dirty(link);

	if (!dirty || (window && now < link->open_ms))
		return 0;
	link_body(link, dirty, body);
	return dirty;
}

int dualsense_link_due(struct dualsense_link *link, unsigned long long now, unsigned char body[DUALSENSE_BODY_SIZE])
{
	return link_due(link, now, body, 1);
}

int dualsense_link_due_now(struct dualsense_link *link, unsigned long long now,
	unsigned char body[DUALSENSE_BODY_SIZE])
{
	return link_due(link, now, body, 0);
}

void dualsense_link_result(struct dualsense_link *link, int fields, enum dualsense_write_result result,
	unsigned long long now)
{
	int i;

	fields &= DUALSENSE_FIELDS;
	if (result == DUALSENSE_WRITE_SENT)
	{
		for (i = 0; i < FIELD_SPANS; i++)
		{
			struct field_span const *span = &field_spans[i];

			if (fields & span->field)
				memcpy(link->sent + span->at, link->want + span->at, (size_t)span->size);
		}
		link->known |= fields;
		link->forced &= ~fields;
		link->open_ms = now + (link->bluetooth ? DUALSENSE_BLUETOOTH_INTERVAL_MS : 0);
		return;
	}
	if (result == DUALSENSE_WRITE_BUSY)
	{
		/* (kept: tried again as the next window opens, never at once) */
		link->open_ms = now + (link->bluetooth ? DUALSENSE_BLUETOOTH_INTERVAL_MS : DUALSENSE_BUSY_RETRY_MS);
		return;
	}
	if (result == DUALSENSE_WRITE_GONE)
		dualsense_link_forget(link);
}

void dualsense_bind(unsigned int *bound, int ports, int port, unsigned int id)
{
	int other;

	if (port < 0 || port >= ports)
		return;
	if (id)
	{
		for (other = 0; other < ports; other++)
		{
			if (other != port && bound[other] == id)
				bound[other] = 0;
		}
	}
	bound[port] = id;
}
