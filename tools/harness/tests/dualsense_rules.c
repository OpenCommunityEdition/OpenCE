/*
DUALSENSE_RULES.C (test)

A PS5 DualSense's output bytes (port/linux/src/dualsense_rules.c): the trigger effects, the weapon profiles by the
weapon's state and the trigger strength, the lightbar and player lights, the USB and Bluetooth reports with their CRC,
and the link that writes only what changed (one report a frame, one every 50 ms on Bluetooth).
test_dualsense_rules.py takes the whole file (under_test.inc) and its header.
*/

#include "harness.h"

#include <errno.h>

#include "under_test.inc"

static int same(unsigned char const *a, unsigned char const *b, size_t size)
{
	return memcmp(a, b, size) == 0;
}

/* a zone effect's (feedback, vibration) strength / amplitude 1-8 in its first active zone */
static int first_level(unsigned char const *effect)
{
	unsigned long active = effect[1] | (unsigned long)effect[2] << 8;
	unsigned long bits = effect[3] | (unsigned long)effect[4] << 8 | (unsigned long)effect[5] << 16 |
		(unsigned long)effect[6] << 24;
	int zone;

	for (zone = 0; zone < 10 && !(active & (1UL << zone)); zone++)
		;
	return zone < 10 ? (int)((bits >> (3 * zone)) & 7) + 1 : 0;
}

static int is_off(unsigned char const *effect)
{
	unsigned char off[DUALSENSE_EFFECT_SIZE];

	dualsense_effect_off(off);
	return same(effect, off, sizeof(off));
}

/* R2 by the state at a strength */
static void held_at(unsigned char effect[DUALSENSE_EFFECT_SIZE], struct dualsense_weapon_state const *state, int percent)
{
	dualsense_right_held(effect, state, percent);
}

static void effects(void)
{
	unsigned char effect[DUALSENSE_EFFECT_SIZE];

	dualsense_effect_off(effect);
	CHECK(effect[0] == 0x05 && effect[1] == 0 && effect[10] == 0, "off is mode 5 and zeros");
	/* feedback from zone 3, strength 4: zones 3-9 active, each level 3 */
	dualsense_effect_feedback(effect, 3, 4);
	CHECK(effect[0] == 0x21 && effect[1] == 0xf8 && effect[2] == 0x03, "feedback zones: %02x %02x", effect[1], effect[2]);
	CHECK(first_level(effect) == 4, "feedback level %d", first_level(effect));
	/* a strength of 0 is off, out of range numbers are clamped */
	dualsense_effect_feedback(effect, 3, 0);
	CHECK(is_off(effect), "strength 0 is off");
	dualsense_effect_feedback(effect, 99, 99);
	CHECK(effect[0] == 0x21 && effect[1] == 0x00 && effect[2] == 0x02 && first_level(effect) == 8,
		"clamped to zone 9, strength 8: %02x %02x", effect[1], effect[2]);
	/* the span: zones 1-2 only */
	dualsense_effect_feedback_span(effect, 1, 2, 2);
	CHECK(effect[0] == 0x21 && effect[1] == 0x06 && effect[2] == 0x00, "span zones: %02x %02x", effect[1], effect[2]);
	/* the click: zones 4 and 7, strength 5 stored as 4 */
	dualsense_effect_weapon(effect, 4, 7, 5);
	CHECK(effect[0] == 0x25 && effect[1] == 0x90 && effect[2] == 0x00 && effect[3] == 4, "weapon: %02x %02x %02x",
		effect[1], effect[2], effect[3]);
	dualsense_effect_weapon(effect, 0, 0, 3);
	CHECK(effect[1] == 0x0c && effect[3] == 2, "a click starts at zone 2 and ends after it: %02x", effect[1]);
	/* vibration: amplitude in the zones, frequency in byte 9 */
	dualsense_effect_vibration(effect, 3, 5, 30);
	CHECK(effect[0] == 0x26 && effect[1] == 0xf8 && effect[9] == 30 && first_level(effect) == 5, "vibration");
}

static void kinds(void)
{
	/* (needler, rate, charging, heat, zoom levels, no projectile, plasma rifle type) */
	CHECK(dualsense_weapon_kind(0, 3.5f, 0, 0, 0, 0, 0) == DUALSENSE_WEAPON_SEMI, "pistol");
	CHECK(dualsense_weapon_kind(0, 1.0f, 0, 0, 0, 0, 0) == DUALSENSE_WEAPON_HEAVY, "shotgun");
	CHECK(dualsense_weapon_kind(0, 15.0f, 0, 0, 0, 0, 0) == DUALSENSE_WEAPON_AUTOMATIC, "assault rifle");
	CHECK(dualsense_weapon_kind(0, 2.0f, 0, 0, 2, 0, 0) == DUALSENSE_WEAPON_SNIPER, "sniper rifle");
	CHECK(dualsense_weapon_kind(0, 2.0f, 0, 0, 1, 0, 0) == DUALSENSE_WEAPON_SEMI, "one zoom level is no sniper rifle");
	CHECK(dualsense_weapon_kind(0, 3.0f, 0.4f, 0, 0, 0, 0) == DUALSENSE_WEAPON_CHARGE, "plasma pistol");
	CHECK(dualsense_weapon_kind(1, 8.0f, 0, 0.1f, 0, 0, 0) == DUALSENSE_WEAPON_NEEDLER, "needler");
	CHECK(dualsense_weapon_kind(0, 8.0f, 0, 0.01f, 0, 0, 1) == DUALSENSE_WEAPON_PLASMA_HEAT, "plasma rifle");
	CHECK(dualsense_weapon_kind(0, 8.0f, 0, 0.01f, 0, 0, 0) == DUALSENSE_WEAPON_FLAME, "flamethrower");
	CHECK(dualsense_weapon_kind(0, 0, 0, 0, 0, 1, 0) == DUALSENSE_WEAPON_CARRY, "ball and flag");
	CHECK(!strcmp(dualsense_weapon_kind_name(DUALSENSE_WEAPON_SNIPER), "sniper"), "name");
	/* only Sony's DualSense and DualSense Edge: other makers' PS5-type pads are left alone */
	CHECK(dualsense_is_dualsense(0x054c, 0x0ce6) && dualsense_is_dualsense(0x054c, 0x0df2), "DualSense, Edge");
	CHECK(!dualsense_is_dualsense(0x054c, 0x09cc) && !dualsense_is_dualsense(0x20d6, 0x0ce6) &&
		!dualsense_is_dualsense(0, 0), "DualShock 4, another maker's pad");
}

static void held(void)
{
	struct dualsense_weapon_state state;
	unsigned char effect[DUALSENSE_EFFECT_SIZE];

	memset(&state, 0, sizeof(state));
	/* no weapon, a carried ball: off */
	held_at(effect, &state, 100);
	CHECK(is_off(effect), "no weapon is off");
	state.kind = DUALSENSE_WEAPON_CARRY;
	held_at(effect, &state, 100);
	CHECK(is_off(effect), "a carried ball is off");
	/* the pistol's click, strength 4 */
	state.kind = DUALSENSE_WEAPON_SEMI;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x25 && effect[1] == 0x90 && effect[3] == 3, "pistol click: %02x %02x %02x", effect[0], effect[1],
		effect[3]);
	/* the weapon's state beats the shape: readying and reloading free, empty the dry click, overheated locked */
	state.readying = 1;
	held_at(effect, &state, 100);
	CHECK(is_off(effect), "readying is free");
	state.readying = 0;
	state.reloading = 1;
	held_at(effect, &state, 100);
	CHECK(is_off(effect), "reloading is free");
	state.reloading = 0;
	state.empty = 1;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x25 && effect[1] == 0x80 && effect[2] == 0x01 && effect[3] == 1, "dry click: %02x %02x %02x",
		effect[0], effect[1], effect[3]);
	state.empty = 0;
	state.kind = DUALSENSE_WEAPON_PLASMA_HEAT;
	state.overheated = 1;
	state.ticks = 4;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x21 && first_level(effect) == 8, "overheated is locked: %02x", effect[0]);
	state.ticks = 0;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x26, "the throb: %02x", effect[0]);
	state.overheated = 0;
	/* heat raises the plasma rifle's hold from 2 to 6 */
	state.heat = 1.0f;
	held_at(effect, &state, 100);
	CHECK(first_level(effect) == 6, "hot: %d", first_level(effect));
	/* the assault rifle buzzes at its rate of fire while it fires, holds lightly otherwise */
	memset(&state, 0, sizeof(state));
	state.kind = DUALSENSE_WEAPON_AUTOMATIC;
	state.rate_of_fire = 15.0f;
	state.firing = 1;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x26 && effect[9] == 15, "automatic firing at 15 Hz: %02x %d", effect[0], effect[9]);
	/* the plasma pistol charging hums, faster once charged */
	memset(&state, 0, sizeof(state));
	state.kind = DUALSENSE_WEAPON_CHARGE;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x21, "uncharged hold");
	state.charge = 0.5f;
	held_at(effect, &state, 100);
	CHECK(effect[0] == 0x26 && effect[9] == 25 && first_level(effect) == 2, "charging hum: %d %d", effect[9],
		first_level(effect));
	state.charge = 1.0f;
	held_at(effect, &state, 100);
	CHECK(effect[9] == 40 && first_level(effect) == 3, "charged hum: %d %d", effect[9], first_level(effect));
}

static void strength(void)
{
	struct dualsense_weapon_state state;
	unsigned char effect[DUALSENSE_EFFECT_SIZE], hair[DUALSENSE_EFFECT_SIZE];
	int kind;

	CHECK(dualsense_strength_percent("light") == DUALSENSE_STRENGTH_LIGHT, "light");
	CHECK(dualsense_strength_percent("normal") == 100, "normal");
	CHECK(dualsense_strength_percent("strong") == DUALSENSE_STRENGTH_STRONG, "strong");
	CHECK(dualsense_strength_percent("") == 100 && dualsense_strength_percent("LIGHT") == 100, "anything else is normal");
	dualsense_effect_feedback_span(hair, 1, 2, 2);
	/* light: every weapon's hold or click is the hair trigger's tick */
	for (kind = DUALSENSE_WEAPON_SEMI; kind < DUALSENSE_WEAPON_KINDS; kind++)
	{
		if (kind == DUALSENSE_WEAPON_CARRY)
			continue;
		memset(&state, 0, sizeof(state));
		state.kind = (enum dualsense_weapon_kind)kind;
		state.rate_of_fire = 8.0f;
		held_at(effect, &state, DUALSENSE_STRENGTH_LIGHT);
		CHECK(same(effect, hair, sizeof(hair)), "kind %d on light is the hair trigger: %02x %02x", kind, effect[0],
			effect[1]);
	}
	/* strong: a hold two steps up, never above 6; a click two steps up, never above 7 */
	memset(&state, 0, sizeof(state));
	state.kind = DUALSENSE_WEAPON_SNIPER;
	held_at(effect, &state, DUALSENSE_STRENGTH_STRONG);
	CHECK(effect[0] == 0x21 && first_level(effect) == 6, "sniper hold on strong: %d", first_level(effect));
	state.kind = DUALSENSE_WEAPON_AUTOMATIC;
	held_at(effect, &state, DUALSENSE_STRENGTH_STRONG);
	CHECK(first_level(effect) == 4, "automatic hold on strong: %d", first_level(effect));
	state.kind = DUALSENSE_WEAPON_HEAVY;
	held_at(effect, &state, DUALSENSE_STRENGTH_STRONG);
	CHECK(effect[0] == 0x25 && effect[3] == 6, "shotgun click on strong: %d", effect[3]);
	/* the dry click: a step firmer on strong */
	state.empty = 1;
	held_at(effect, &state, DUALSENSE_STRENGTH_STRONG);
	CHECK(effect[3] == 2, "dry click on strong");
}

static void events(void)
{
	unsigned char effect[DUALSENSE_EFFECT_SIZE];
	int ms;

	/* the pistol's shot: a snap from zone 0; the assault rifle's: none (its firing shape does it) */
	ms = dualsense_event(effect, DUALSENSE_WEAPON_SEMI, DUALSENSE_EVENT_SHOT, 100);
	CHECK(ms == 35 && effect[0] == 0x21 && first_level(effect) == 7, "pistol shot kick: %d ms", ms);
	ms = dualsense_event(effect, DUALSENSE_WEAPON_AUTOMATIC, DUALSENSE_EVENT_SHOT, 100);
	CHECK(ms == 0 && is_off(effect), "automatic shot has no kick");
	/* the plasma pistol's blast when charged, its ordinary pulse otherwise */
	ms = dualsense_event(effect, DUALSENSE_WEAPON_CHARGE, DUALSENSE_EVENT_SHOT_CHARGED, 100);
	CHECK(ms == 240 && effect[0] == 0x26 && effect[9] == 15, "charged blast: %d ms", ms);
	ms = dualsense_event(effect, DUALSENSE_WEAPON_CHARGE, DUALSENSE_EVENT_SHOT, 100);
	CHECK(ms == 70 && effect[9] == 40, "plasma pistol shot: %d ms", ms);
	ms = dualsense_event(effect, DUALSENSE_WEAPON_HEAVY, DUALSENSE_EVENT_SHOT, 100);
	CHECK(ms == 130 && effect[0] == 0x26 && effect[9] == 30, "shotgun shot kick: %d ms", ms);
	/* light: kicks at most 2; strong: two steps up, at most 8 */
	dualsense_event(effect, DUALSENSE_WEAPON_SEMI, DUALSENSE_EVENT_SHOT, DUALSENSE_STRENGTH_LIGHT);
	CHECK(first_level(effect) == 2, "light kick: %d", first_level(effect));
	dualsense_event(effect, DUALSENSE_WEAPON_SEMI, DUALSENSE_EVENT_SHOT, DUALSENSE_STRENGTH_STRONG);
	CHECK(first_level(effect) == 8, "strong kick: %d", first_level(effect));
}

static void left(void)
{
	unsigned char effect[DUALSENSE_EFFECT_SIZE], tick[DUALSENSE_EFFECT_SIZE];

	dualsense_effect_feedback_span(tick, 1, 2, 2);
	dualsense_left_effect(effect, 1, 0, 0, 100);
	CHECK(effect[0] == 0x21 && first_level(effect) == 2 && effect[1] == 0xfc, "zoomed hold");
	dualsense_left_effect(effect, 1, 3, 0, DUALSENSE_STRENGTH_STRONG);
	CHECK(first_level(effect) == 3, "zoomed hold on strong");
	dualsense_left_effect(effect, 1, 3, 0, DUALSENSE_STRENGTH_LIGHT);
	CHECK(is_off(effect), "zoomed on light is free");
	dualsense_left_effect(effect, 0, 3, 0, 100);
	CHECK(same(effect, tick, sizeof(tick)), "grenades in hand: the hair tick");
	dualsense_left_effect(effect, 0, 3, 1, 100);
	CHECK(is_off(effect), "a throw wound up is free");
	dualsense_left_effect(effect, 0, 0, 0, 100);
	CHECK(effect[0] == 0x25 && effect[3] == 1, "no grenades: the dry click");
}

static void lights(void)
{
	int number;

	/* the player lights: SDL's patterns with the "at once" bit, back to the player */
	CHECK(dualsense_player_lights(1) == 0x24 && dualsense_player_lights(2) == 0x2a &&
		dualsense_player_lights(3) == 0x35 && dualsense_player_lights(4) == 0x3b, "patterns");
	CHECK(dualsense_player_lights(0) == 0 && dualsense_player_lights(5) == 0, "no pattern for another number");
	for (number = 1; number <= 4; number++)
		CHECK(dualsense_player_index_of_lights(dualsense_player_lights(number)) == number - 1, "round trip %d", number);
}

static void reports(void)
{
	unsigned char body[DUALSENSE_BODY_SIZE], report[DUALSENSE_BLUETOOTH_REPORT_SIZE], again[DUALSENSE_BLUETOOTH_REPORT_SIZE];
	unsigned char hidp = 0xA2;
	unsigned long crc;
	size_t size;
	int i;

	for (i = 0; i < DUALSENSE_BODY_SIZE; i++)
		body[i] = (unsigned char)(i + 1);
	/* the CRC-32 check value */
	CHECK(dualsense_crc32(0, (unsigned char const *)"123456789", 9) == 0xCBF43926UL, "crc of 123456789");
	CHECK(dualsense_crc32(dualsense_crc32(0, (unsigned char const *)"1234", 4), (unsigned char const *)"56789", 5) ==
		0xCBF43926UL, "crc continued");
	/* USB: report 0x02, the body, padded to 48 */
	size = dualsense_output_report(report, sizeof(report), 0, 5, body);
	CHECK(size == DUALSENSE_USB_REPORT_SIZE && report[0] == 0x02 && same(report + 1, body, sizeof(body)), "usb");
	/* Bluetooth: 0x31, the sequence in the high nibble, 0x10, the body, the CRC of 0xA2 and the first 74 bytes */
	size = dualsense_output_report(report, sizeof(report), 1, 0x13, body);
	CHECK(size == DUALSENSE_BLUETOOTH_REPORT_SIZE && report[0] == 0x31 && report[1] == 0x30 && report[2] == 0x10 &&
		same(report + 3, body, sizeof(body)), "bluetooth framing: %02x %02x", report[0], report[1]);
	crc = dualsense_crc32(dualsense_crc32(0, &hidp, 1), report, 74);
	CHECK(report[74] == (crc & 0xff) && report[75] == ((crc >> 8) & 0xff) && report[76] == ((crc >> 16) & 0xff) &&
		report[77] == ((crc >> 24) & 0xff), "bluetooth crc");
	dualsense_output_report(again, sizeof(again), 1, 0x14, body);
	CHECK(!same(report + 74, again + 74, 4) && again[1] == 0x40, "the sequence number is in the crc");
	/* a write's errno as an outcome */
	CHECK(dualsense_write_result_of_errno(EAGAIN) == DUALSENSE_WRITE_BUSY &&
		dualsense_write_result_of_errno(ENODEV) == DUALSENSE_WRITE_GONE &&
		dualsense_write_result_of_errno(EACCES) == DUALSENSE_WRITE_REFUSED, "errno");
}

/* the link: only what changed is written, in one report, enable bits for those fields only */
static void link_usb(void)
{
	struct dualsense_link link;
	unsigned char body[DUALSENSE_BODY_SIZE], held[DUALSENSE_EFFECT_SIZE], off[DUALSENSE_EFFECT_SIZE];
	int fields;

	dualsense_effect_weapon(held, 4, 7, 6);
	dualsense_effect_off(off);
	dualsense_link_init(&link, 0);
	CHECK(dualsense_link_due(&link, 0, body) == 0, "nothing wanted, nothing written");
	dualsense_link_want_triggers(&link, held, off);
	fields = dualsense_link_due(&link, 0, body);
	CHECK(fields == DUALSENSE_FIELD_TRIGGERS, "triggers due: %d", fields);
	CHECK(body[DUALSENSE_BODY_ENABLE1] == (DUALSENSE_ENABLE1_RIGHT_TRIGGER | DUALSENSE_ENABLE1_LEFT_TRIGGER) &&
		body[DUALSENSE_BODY_ENABLE2] == 0, "only the trigger bits: %02x %02x", body[0], body[1]);
	CHECK(same(body + DUALSENSE_BODY_RIGHT_TRIGGER, held, sizeof(held)) &&
		same(body + DUALSENSE_BODY_LEFT_TRIGGER, off, sizeof(off)), "the bytes");
	dualsense_link_result(&link, fields, DUALSENSE_WRITE_SENT, 0);
	/* the same again is no write; a changed trigger is one; a lightbar joins as its own field */
	dualsense_link_want_triggers(&link, held, off);
	CHECK(dualsense_link_due(&link, 1, body) == 0 && dualsense_link_due(&link, 2, body) == 0, "unchanged: no write");
	dualsense_link_want_triggers(&link, off, off);
	dualsense_link_want_lightbar(&link, 1, 2, 3);
	dualsense_link_want_player_lights(&link, 0x24);
	fields = dualsense_link_due(&link, 3, body);
	CHECK(fields == DUALSENSE_FIELDS, "all three fields: %d", fields);
	CHECK(body[DUALSENSE_BODY_ENABLE2] == (DUALSENSE_ENABLE2_LIGHTBAR | DUALSENSE_ENABLE2_PLAYER_LIGHTS) &&
		body[DUALSENSE_BODY_RED] == 1 && body[DUALSENSE_BODY_GREEN] == 2 && body[DUALSENSE_BODY_BLUE] == 3 &&
		body[DUALSENSE_BODY_PLAYER_LIGHTS] == 0x24, "lightbar bytes");
	dualsense_link_result(&link, fields, DUALSENSE_WRITE_SENT, 3);
	/* only the lightbar changes: only its bit */
	dualsense_link_want_lightbar(&link, 9, 2, 3);
	fields = dualsense_link_due(&link, 4, body);
	CHECK(fields == DUALSENSE_FIELD_LIGHTBAR && body[DUALSENSE_BODY_ENABLE1] == 0 &&
		body[DUALSENSE_BODY_ENABLE2] == DUALSENSE_ENABLE2_LIGHTBAR, "only the lightbar: %d", fields);
	dualsense_link_result(&link, fields, DUALSENSE_WRITE_SENT, 4);
	/* forced: sent though unchanged, once */
	dualsense_link_force(&link, DUALSENSE_FIELD_TRIGGERS);
	fields = dualsense_link_due(&link, 5, body);
	CHECK(fields == DUALSENSE_FIELD_TRIGGERS, "forced: %d", fields);
	dualsense_link_result(&link, fields, DUALSENSE_WRITE_SENT, 5);
	CHECK(dualsense_link_due(&link, 6, body) == 0, "a refresh is sent once");
	dualsense_link_want_lightbar(&link, 0, 0, 0);
	dualsense_link_drop(&link, DUALSENSE_FIELD_LIGHTBAR);
	CHECK(dualsense_link_due(&link, 7, body) == 0, "dropped fields are not written");
}

static void link_bluetooth(void)
{
	struct dualsense_link link;
	unsigned char body[DUALSENSE_BODY_SIZE], one[DUALSENSE_EFFECT_SIZE], two[DUALSENSE_EFFECT_SIZE];
	int fields;

	dualsense_effect_feedback(one, 2, 3);
	dualsense_effect_feedback(two, 2, 6);
	dualsense_link_init(&link, 1);
	dualsense_link_want_triggers(&link, one, one);
	fields = dualsense_link_due(&link, 1000, body);
	CHECK(fields == DUALSENSE_FIELD_TRIGGERS, "the first report goes at once");
	dualsense_link_result(&link, fields, DUALSENSE_WRITE_SENT, 1000);
	/* a change inside the window waits; a later one replaces it; the latest goes as the window opens */
	dualsense_link_want_triggers(&link, two, two);
	CHECK(dualsense_link_due(&link, 1010, body) == 0, "waits for the window");
	dualsense_link_want_triggers(&link, one, two);
	CHECK(dualsense_link_due(&link, 1049, body) == 0, "still waits");
	dualsense_link_want_triggers(&link, two, one);
	fields = dualsense_link_due(&link, 1050, body);
	CHECK(fields == DUALSENSE_FIELD_TRIGGERS && same(body + DUALSENSE_BODY_RIGHT_TRIGGER, two, sizeof(two)) &&
		same(body + DUALSENSE_BODY_LEFT_TRIGGER, one, sizeof(one)), "the latest goes as the window opens");
	/* the window ignored (at exit) */
	dualsense_link_result(&link, fields, DUALSENSE_WRITE_SENT, 1050);
	dualsense_link_want_triggers(&link, one, one);
	CHECK(dualsense_link_due(&link, 1051, body) == 0, "inside the window");
	CHECK(dualsense_link_due_now(&link, 1051, body) == DUALSENSE_FIELD_TRIGGERS, "due now ignores the window");
	/* busy: kept, tried again a window later */
	dualsense_link_result(&link, DUALSENSE_FIELD_TRIGGERS, DUALSENSE_WRITE_BUSY, 1051);
	CHECK(dualsense_link_due(&link, 1060, body) == 0 && dualsense_link_due(&link, 1100, body) == 0,
		"busy: not at once");
	CHECK(dualsense_link_due(&link, 1101, body) == DUALSENSE_FIELD_TRIGGERS, "busy: tried again");
	/* gone: the pad has nothing from us, everything is sent again */
	dualsense_link_result(&link, DUALSENSE_FIELD_TRIGGERS, DUALSENSE_WRITE_SENT, 1101);
	CHECK(dualsense_link_due(&link, 1200, body) == 0, "nothing to write");
	dualsense_link_result(&link, DUALSENSE_FIELD_TRIGGERS, DUALSENSE_WRITE_GONE, 1200);
	CHECK(dualsense_link_due(&link, 1200, body) == DUALSENSE_FIELD_TRIGGERS, "gone: everything again");
}

static void bind(void)
{
	unsigned int bound[4] = { 0, 0, 0, 0 };

	dualsense_bind(bound, 4, 0, 7);
	dualsense_bind(bound, 4, 1, 9);
	CHECK(bound[0] == 7 && bound[1] == 9, "two pads");
	/* pad 7 unplugged: port 1's handle is gone and never set again; pad 9 moves to port 0 */
	dualsense_bind(bound, 4, 0, 9);
	CHECK(bound[0] == 9 && bound[1] == 0, "a pad that moved is bound to its new port only");
	dualsense_bind(bound, 4, 4, 9);
	dualsense_bind(bound, 4, -1, 9);
	CHECK(bound[0] == 9 && bound[1] == 0 && bound[2] == 0 && bound[3] == 0, "out of range: nothing");
}

int main(int argc, char **argv)
{
	const char *name = argc > 1 ? argv[1] : "";

	if (!strcmp(name, "effects"))
		effects();
	else if (!strcmp(name, "kinds"))
		kinds();
	else if (!strcmp(name, "held"))
		held();
	else if (!strcmp(name, "strength"))
		strength();
	else if (!strcmp(name, "events"))
		events();
	else if (!strcmp(name, "left"))
		left();
	else if (!strcmp(name, "lights"))
		lights();
	else if (!strcmp(name, "reports"))
		reports();
	else if (!strcmp(name, "link-usb"))
		link_usb();
	else if (!strcmp(name, "link-bluetooth"))
		link_bluetooth();
	else if (!strcmp(name, "bind"))
		bind();
	else
	{
		fprintf(stderr, "unknown case %s\n", name);
		return 2;
	}
	return 0;
}
