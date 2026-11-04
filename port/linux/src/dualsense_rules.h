/*
DUALSENSE_RULES.H

A PS5 DualSense's (and DualSense Edge's) output bytes: trigger effects, lightbar, player lights, the reports that carry
them, and the link that decides what to write when. Pure (no SDL, no game), so tools/harness/tests/dualsense_rules.c
checks it.

- The shared 47-byte body: the layout of SDL3's DS5EffectsState_t (src/joystick/hidapi/SDL_hidapi_ps5.c, zlib licence).
- A trigger effect's 11 bytes: the zone-based modes (feedback 0x21, weapon 0x25, vibration 0x26) packed as in
  John "Nielk1" Klein's TriggerEffectGenerator (MIT licence), and 0x05, the "off" SDL's testcontroller.c sends.
- The whole report for a hidraw write: USB report 0x02 (48 bytes) or Bluetooth report 0x31 (78 bytes: the sequence in
  the high nibble, tag 0x10, the body, a CRC-32 over 0xA2 and the first 74 bytes), as SDL's driver frames them.
*/

#ifndef __DUALSENSE_RULES_H
#define __DUALSENSE_RULES_H

#include <stddef.h>

#define DUALSENSE_EFFECT_SIZE 11
#define DUALSENSE_BODY_SIZE 47
#define DUALSENSE_USB_REPORT_SIZE 48
#define DUALSENSE_BLUETOOTH_REPORT_SIZE 78

/* the body's fields used here (byte offsets) */
#define DUALSENSE_BODY_ENABLE1 0 /* 0x04 right trigger effect, 0x08 left trigger effect */
#define DUALSENSE_BODY_ENABLE2 1 /* 0x04 lightbar colour, 0x10 player lights */
#define DUALSENSE_BODY_RIGHT_TRIGGER 10
#define DUALSENSE_BODY_LEFT_TRIGGER 21
#define DUALSENSE_BODY_PLAYER_LIGHTS 43
#define DUALSENSE_BODY_RED 44
#define DUALSENSE_BODY_GREEN 45
#define DUALSENSE_BODY_BLUE 46

#define DUALSENSE_ENABLE1_RIGHT_TRIGGER 0x04
#define DUALSENSE_ENABLE1_LEFT_TRIGGER 0x08
#define DUALSENSE_ENABLE2_LIGHTBAR 0x04
#define DUALSENSE_ENABLE2_PLAYER_LIGHTS 0x10

/* the weapon kinds, each with its own R2 profile */
enum dualsense_weapon_kind
{
	DUALSENSE_WEAPON_NONE = 0,
	DUALSENSE_WEAPON_SEMI,        /* the pistol: a short click */
	DUALSENSE_WEAPON_HEAVY,       /* slow single shots (shotgun, rocket launcher): a heavy click */
	DUALSENSE_WEAPON_AUTOMATIC,   /* assault rifle: buzz at the rate of fire */
	DUALSENSE_WEAPON_SNIPER,      /* two zoom levels and a slow trigger: firm resistance */
	DUALSENSE_WEAPON_CHARGE,      /* a trigger that charges (plasma pistol): resistance from mid pull */
	DUALSENSE_WEAPON_NEEDLER,     /* the needler: a fast light buzz */
	DUALSENSE_WEAPON_PLASMA_HEAT, /* rounds make heat (plasma rifle): rises with heat, locked when overheated */
	DUALSENSE_WEAPON_CARRY,       /* no trigger with a projectile (ball, flag): free */
	DUALSENSE_WEAPON_FLAME,       /* fast, with heat, not the plasma rifle (flamethrower): a roar while it burns */
	DUALSENSE_WEAPON_KINDS
};

/* the kind from the weapon's tag numbers: whether its weapon type is the needler, its first trigger's rate of fire
(rounds a second), the longest charging time of its triggers (seconds), its first trigger's heat a round, its zoom
level count, whether no trigger has a projectile (a carried ball or flag), whether its weapon type is the plasma rifle
(a fast weapon with heat is the plasma rifle's kind if so, else the flamethrower's) */
enum dualsense_weapon_kind dualsense_weapon_kind(int needler, float rate_of_fire, float charging_time,
	float heat_per_round, int zoom_levels, int no_projectile, int plasma_rifle_type);
char const *dualsense_weapon_kind_name(enum dualsense_weapon_kind kind);

/* the weapon in hand now (the game side reads it each frame) */
struct dualsense_weapon_state
{
	enum dualsense_weapon_kind kind;
	float rate_of_fire;
	float heat;   /* 0-1 */
	int overheated;
	float charge; /* 0-1 while charging or charged, else 0 */
	int firing;   /* a round in the last few ticks */
	int reloading;
	int empty;    /* the magazine has no round loaded, or a battery weapon's battery is spent */
	int readying; /* switching weapons: readying or putting away */
	long ticks;   /* the game time, for the overheat's throb */
};

/* the events R2 feels once (a kick, then the held shape again) */
enum dualsense_event
{
	DUALSENSE_EVENT_SHOT,
	DUALSENSE_EVENT_SHOT_CHARGED /* a fully charged shot (the plasma pistol's blast) */
};

/* the 11 bytes of a trigger effect; out-of-range numbers are clamped, a strength or amplitude of 0 is "off" */
void dualsense_effect_off(unsigned char effect[DUALSENSE_EFFECT_SIZE]);
/* resistance from zone `position` (0-9) to the end, strength 1-8 */
void dualsense_effect_feedback(unsigned char effect[DUALSENSE_EFFECT_SIZE], int position, int strength);
/* resistance between zone `start` (2-7) and `end` (start+1 to 8) that gives way: a click; strength 1-8 */
void dualsense_effect_weapon(unsigned char effect[DUALSENSE_EFFECT_SIZE], int start, int end, int strength);
/* resistance over zones `first` to `last` (0-9) only, free after it, strength 1-8 */
void dualsense_effect_feedback_span(unsigned char effect[DUALSENSE_EFFECT_SIZE], int first, int last, int strength);
/* vibration from zone `position` (0-9) on, amplitude 1-8, frequency 1-255 (Hz) */
void dualsense_effect_vibration(unsigned char effect[DUALSENSE_EFFECT_SIZE], int position, int amplitude,
	int frequency);

/* R2 as the trigger rests / is pulled now, first rule that applies: no weapon or a carried ball/flag: off; readying:
free; overheated: locked with a throb; reloading: free; empty: a dry click; firing: the profile's firing shape; else
its held shape. Heat or charge ramps the strength where the profile says. `percent` is the trigger strength
(dualsense_strength_percent) */
void dualsense_right_held(unsigned char effect[DUALSENSE_EFFECT_SIZE], struct dualsense_weapon_state const *state,
	int percent);
/* an event's kick on R2 and how long it lasts (ms; 0: no kick, e.g. an automatic's shot, which its firing shape does) */
int dualsense_event(unsigned char effect[DUALSENSE_EFFECT_SIZE], enum dualsense_weapon_kind kind,
	enum dualsense_event event, int percent);
/* L2: light while zoomed (free on LIGHT); grenades in hand a hair trigger (the lightest tick still felt, zones 1-2,
nothing after it); a grenade wound up free; none, the dry click */
void dualsense_left_effect(unsigned char effect[DUALSENSE_EFFECT_SIZE], int zoomed, int grenades, int winding,
	int percent);

/* input.trigger_strength (light, normal, strong) as a percent (DUALSENSE_STRENGTH_LIGHT, 100,
DUALSENSE_STRENGTH_STRONG; anything else 100). The effects are written for normal; strong makes a hold, a click, a
kick and a buzz two steps firmer (a hold at most 6, a click 7, a kick 8); light is a hair trigger on both triggers: a
tiny tick early in the pull (zones 1-2, strength 2) and nothing after it, kicks and buzzes at most 2. */
#define DUALSENSE_STRENGTH_LIGHT 75
#define DUALSENSE_STRENGTH_STRONG 175
int dualsense_strength_percent(char const *name);

/* whether a USB vendor and product id are Sony's DualSense (054C:0CE6) or DualSense Edge (054C:0DF2): SDL's PS5 type
and LED also match other makers' pads, which are never written to */
int dualsense_is_dualsense(int vendor, int product);
/* the player lights' byte for split-screen player 1-4 (0 for another number), and back: the player index (0-3) a
byte shows, -1 for none */
unsigned char dualsense_player_lights(int player_number);
int dualsense_player_index_of_lights(unsigned char lights);

/* CRC-32 (the zlib / IEEE one, reflected, polynomial 0xEDB88320), continued from `crc` (0 to start) */
unsigned long dualsense_crc32(unsigned long crc, unsigned char const *data, size_t size);

/* the whole output report for a hidraw write: the body framed for USB or Bluetooth (sequence 0-15); returns its
size, or 0 when `size` is too small */
size_t dualsense_output_report(unsigned char *report, size_t size, int bluetooth, int sequence,
	unsigned char const body[DUALSENSE_BODY_SIZE]);

/* A pad's link keeps the state it has from us and the state wanted, by field (the triggers, the lightbar, the player
lights). Each frame the wanted fields are set, then dualsense_link_due says what to write now: only the fields whose
bytes differ from the pad's (or not sent yet, or asked again), all in one report. On Bluetooth at most one report every
DUALSENSE_BLUETOOTH_INTERVAL_MS: a change inside the window waits for it, later changes fold into the waiting one (the
latest wins). USB: one report a frame at most. A report the pad's queue had no room for (busy) waits a window */
#define DUALSENSE_BLUETOOTH_INTERVAL_MS 50
/* (busy on USB: the next try this long after) */
#define DUALSENSE_BUSY_RETRY_MS 16

enum dualsense_field
{
	DUALSENSE_FIELD_TRIGGERS = 1,
	DUALSENSE_FIELD_LIGHTBAR = 2,
	DUALSENSE_FIELD_PLAYER_LIGHTS = 4,
	DUALSENSE_FIELDS = 7
};

/* a write's outcome: sent; busy (the queue full, try the next window); gone (the pad left: its node is closed, opened
again when it is back, and everything sent again); refused (the pad is left alone) */
enum dualsense_write_result
{
	DUALSENSE_WRITE_SENT,
	DUALSENSE_WRITE_BUSY,
	DUALSENSE_WRITE_GONE,
	DUALSENSE_WRITE_REFUSED
};
/* a failed write's errno as an outcome */
enum dualsense_write_result dualsense_write_result_of_errno(int error);

struct dualsense_link
{
	int bluetooth;
	/* the wanted state (every field's bytes in a body), the fields wanted, the ones to send though unchanged */
	unsigned char want[DUALSENSE_BODY_SIZE];
	int wanted;
	int forced;
	/* what the pad has from us, and which fields it has */
	unsigned char sent[DUALSENSE_BODY_SIZE];
	int known;
	/* when the next write may be made (ms) */
	unsigned long long open_ms;
};

void dualsense_link_init(struct dualsense_link *link, int bluetooth);
/* the pad has nothing from us (opened again after it left): everything wanted is sent again */
void dualsense_link_forget(struct dualsense_link *link);
/* this frame's wanted fields */
void dualsense_link_want_triggers(struct dualsense_link *link, unsigned char const right[DUALSENSE_EFFECT_SIZE],
	unsigned char const left[DUALSENSE_EFFECT_SIZE]);
void dualsense_link_want_lightbar(struct dualsense_link *link, int red, int green, int blue);
void dualsense_link_want_player_lights(struct dualsense_link *link, unsigned char lights);
/* the fields sent next time though unchanged (a refresh) */
void dualsense_link_force(struct dualsense_link *link, int fields);
/* the fields no longer driven (the pad keeps what it has) */
void dualsense_link_drop(struct dualsense_link *link, int fields);
/* the fields that differ from the pad's */
int dualsense_link_dirty(struct dualsense_link const *link);
/* once a frame: the fields to write now (0: none) with the body to send (their enable bits set); `now` in ms */
int dualsense_link_due(struct dualsense_link *link, unsigned long long now, unsigned char body[DUALSENSE_BODY_SIZE]);
/* as dualsense_link_due, the window ignored (at exit, a reset) */
int dualsense_link_due_now(struct dualsense_link *link, unsigned long long now,
	unsigned char body[DUALSENSE_BODY_SIZE]);
/* the outcome of writing what dualsense_link_due gave */
void dualsense_link_result(struct dualsense_link *link, int fields, enum dualsense_write_result result,
	unsigned long long now);

/* ONE PAD, ONE PORT: `port` is now pad `id`'s (0: no pad), and no other port is. A pad moves port mid-game when the
one before it is unplugged; the old port's handle went with that pad, so nothing sets the old port again */
void dualsense_bind(unsigned int *bound, int ports, int port, unsigned int id);

#endif
