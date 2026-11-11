/*
RIDING_IDLE.C (test)

The biped hooks of vehicle riding with riding off (or a biped with no player),
taken from the sources by test_riding_idle.py: the tick's prepare and finish
(bipeds.c's biped_update_moving, with the native physics between them stubbed)
and biped_riding_seated, each run twice from the same world, once as the game
builds them and once with the fast path switched off by text. Everything the
tick may write must come out the same: the physics fields it fills, the
biped's stored riding fields, and the player's mark, contacts, air, cabin
and kept turn.
*/

#include "harness.h"
typedef unsigned short word;
#include <math.h>
typedef struct { real i, j; } real_vector2d;
#include "enums.inc"
#include "engine.inc"
#include "idle_types.inc"

typedef struct { real yaw, pitch; } real_euler_angles2d;
#define TICKS_PER_SECOND 30
#define PI 3.14159265f
#define csmemset memset

/* vehicle_riding.c's world: settings, connection, look, the host's switches */
static int config_riding = 1;
int config_boolean(const char *name)
{
	return (!strcmp(name, "network.vehicle_riding") && config_riding) || !strcmp(name, "debug.riding_look_carry");
}
double config_real(const char *name) { (void)name; return 0.0; }
short game_connection(void) { return _game_connection_local; }
boolean game_in_editor(void) { return FALSE; }
void player_control_permanent_impulse(short local_player_index, real_euler_angles2d const *delta)
{
	(void)local_player_index;
	(void)delta;
}
struct game_variant { short game_engine_index; struct { unsigned long flags; } universal_variant; };
struct network_game { struct game_variant variant; byte cooperative_flags; };
static struct network_game test_game;
struct network_game *network_game_get_game(void) { return &test_game; }
static struct game_variant engine_variant;
boolean game_engine_running(void) { return FALSE; }
struct game_variant *game_engine_get_variant(void) { return &engine_variant; }
int halo_server_mode(void) { return 0; }
static boolean game_players_are_double_speed(void) { return FALSE; }

/* the map's riding slot: a case flips it to switch riding on and off with no latch, as a map script or the host's
message does */
struct biped_datum;
static long map_slot_storage;
static void *game_state_malloc(char const *name, void *type, long size) { (void)name; (void)type; (void)size; return &map_slot_storage; }
static short hs_runtime_port_executing_script_type(void) { return NONE; }
static short hs_runtime_port_scenario_boolean(char const *name) { (void)name; return NONE; }
static void error(long priority, char const *format, ...) { (void)priority; (void)format; }
struct object_iterator { int next; };
enum { _object_mask_biped = 1 };
static void object_iterator_new(struct object_iterator *i, long mask, long flags) { (void)mask; (void)flags; i->next = 0; }
static void *object_iterator_next(struct object_iterator *i) { (void)i; return NULL; }
void biped_riding_clear(struct biped_datum *biped);
static long game_time_get(void) { return 1; }
#include "under_test.inc"

/* the map's decision as vehicle_riding_objects_reset makes it: a setting changed counts from the next map */
static void decide_now(void)
{
#ifndef FIXTURE_NO_DECIDE
	vehicle_riding_map_decide();
#endif
}

/* ---------- bipeds.c's world */

#define BIPED 0x20000005
#define SLOT 3
#define VEHICLE 0x10000001

struct biped_datum
{
	struct
	{
		unsigned long damage_flags;
		real_vector3d translational_velocity;
	} object;
	struct
	{
		long player_index;
		unsigned long flags;
	} unit;
	long definition_index;
	struct
	{
		unsigned long flags;
		long elevator_object_index;
		short elevator_ticks;
		char riding_ticks;
		char riding_falling;
		long riding_object_index;
		long riding_vehicle_updates;
		real_point3d riding_vehicle_position;
		real_vector3d riding_vehicle_forward;
		real_vector3d riding_vehicle_up;
		real_vector3d riding_velocity;
		real_point3d riding_local_point;
		short riding_node_index;
		short riding_pad;
		real_point3d riding_body_position;
		real_vector3d riding_body_forward;
		real_vector3d riding_body_up;
	} biped;
};

struct biped_definition { long unused; };
static struct biped_definition the_definition;
#define biped_definition_get(index) ((void)(index), &the_definition)

static struct biped_datum rider;
#define biped_get(index) ((void)(index), &rider)
static short local_player;
static short unit_get_local_player_index(long unit_index) { (void)unit_index; return local_player; }

static real_vector3d const zero_vector = { 0.f, 0.f, 0.f };
static real_vector3d const *const global_zero_vector3d = &zero_vector;
static real global_gravity = .005f;
#define SET_FLAG(flags, bit, value) ((value) ? ((flags) |= (unsigned)FLAG(bit)) : ((flags) &= ~(unsigned)FLAG(bit)))

/* what the hooks call on a biped that rides: none of it runs with riding off or no player */
#define UNREACHED(name) CHECK(FALSE, "%s reached with riding off", name)
static boolean biped_riding_vehicle(long index, short node, struct vehicle_riding_vehicle *vehicle)
{
	(void)index; (void)node; (void)vehicle;
	UNREACHED("biped_riding_vehicle");
	return FALSE;
}
static real biped_riding_damage_speed(struct biped_datum const *b) { (void)b; UNREACHED("biped_riding_damage_speed"); return 0.f; }
static void biped_riding_carry(long biped_index, struct biped_datum const *b, struct biped_definition const *definition,
	long vehicle_index, real_vector3d const *displacement, real height, real width, real_point3d *position)
{
	(void)biped_index; (void)b; (void)definition; (void)vehicle_index; (void)displacement; (void)height; (void)width;
	(void)position;
	UNREACHED("biped_riding_carry");
}
static boolean biped_riding_cabin_tick(struct biped_datum *b, struct biped_definition const *definition,
	struct vehicle_riding_cabin *cabin, struct vehicle_riding_state const *riding, struct vehicle_riding_step const *step,
	struct vehicle_riding_air const *air, real_point3d const *base, real height)
{
	(void)b; (void)definition; (void)cabin; (void)riding; (void)step; (void)air; (void)base; (void)height;
	UNREACHED("biped_riding_cabin_tick");
	return FALSE;
}
static boolean biped_riding_window_over_deck(struct biped_definition const *definition,
	struct vehicle_riding_state const *riding, struct vehicle_riding_vehicle const *vehicle, real_point3d const *base,
	real height, boolean exception)
{
	(void)definition; (void)riding; (void)vehicle; (void)base; (void)height; (void)exception;
	UNREACHED("biped_riding_window_over_deck");
	return FALSE;
}
static void biped_riding_turn(struct biped_datum *b, struct biped_physics *physics, real dyaw)
{
	(void)b; (void)physics; (void)dyaw;
	UNREACHED("biped_riding_turn");
}
static void biped_riding_enclosure(struct biped_definition const *definition, struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base, real height, struct vehicle_riding_air *air)
{
	(void)definition; (void)vehicle; (void)base; (void)height; (void)air;
	UNREACHED("biped_riding_enclosure");
}
static short biped_riding_cabin_settle(long biped_index, struct biped_datum const *b, struct biped_definition const *definition,
	struct vehicle_riding_cabin *cabin, struct vehicle_riding_vehicle const *vehicle, real_point3d const *entry,
	real_vector3d const *normal, real_vector3d const *ahead, boolean final, real height, real width, real_point3d *base)
{
	(void)biped_index; (void)b; (void)definition; (void)cabin; (void)vehicle; (void)entry; (void)normal; (void)ahead;
	(void)final; (void)height; (void)width; (void)base;
	UNREACHED("biped_riding_cabin_settle");
	return 0;
}
static boolean biped_riding_cabin_sample(struct biped_definition const *definition, struct vehicle_riding_cabin *cabin,
	struct vehicle_riding_vehicle const *vehicle, real_point3d const *base, real height)
{
	(void)definition; (void)cabin; (void)vehicle; (void)base; (void)height;
	UNREACHED("biped_riding_cabin_sample");
	return FALSE;
}

#include "bipeds.inc"

/* ---------- the world's setup and what a tick leaves behind */

static char log_text[2][16384];
static int log_length[2];

#define LOG(variant, ...) (log_length[variant] += snprintf(log_text[variant] + log_length[variant], \
	sizeof(log_text[variant]) - (size_t)log_length[variant], __VA_ARGS__))
#define LOG_REAL(variant, name, value) LOG(variant, " %s=%a", name, (double)(value))
#define LOG_VECTOR(variant, name, vector) LOG(variant, " %s=(%a %a %a)", name, (double)(vector).i, (double)(vector).j, \
	(double)(vector).k)
#define LOG_POINT(variant, name, point) LOG(variant, " %s=(%a %a %a)", name, (double)(point).x, (double)(point).y, \
	(double)(point).z)
#define LOG_INT(variant, name, value) LOG(variant, " %s=%ld", name, (long)(value))

/* a cleared player: the world as the latch leaves it, riding on or off, a biped with a player or none */
static void world(boolean player, boolean riding)
{
	vehicle_riding_map_initialize();
	decide_now();
	vehicle_riding_latch();
	map_slot_storage = riding != FALSE;
	memset(&rider, 0, sizeof(rider));
	rider.unit.player_index = player ? SLOT : NONE;
	rider.object.translational_velocity.i = .25f;
	rider.object.translational_velocity.j = -.5f;
	rider.object.translational_velocity.k = .125f;
	biped_riding_clear(&rider);
	local_player = player ? 0 : NONE;
	get_calls = set_calls = air_begin_calls = cabin_begin_calls = 0;
}

static void log_physics(int variant, struct biped_physics const *physics)
{
	LOG_INT(variant, "applies", physics->riding_applies);
	LOG_INT(variant, "riding", physics->riding_object_index);
	LOG_INT(variant, "tracked", physics->tracked_object_index);
	LOG_INT(variant, "support", physics->support_object_index);
	LOG_INT(variant, "cabin", physics->cabin_object_index);
	LOG_INT(variant, "carrier", physics->carrier_object_index);
	LOG_INT(variant, "support_node", physics->support_node_index);
	LOG_INT(variant, "carrier_node", physics->carrier_node_index);
	LOG_VECTOR(variant, "tracked_velocity", physics->tracked_deck_velocity);
	LOG_VECTOR(variant, "support_velocity", physics->support_velocity);
	LOG_VECTOR(variant, "frame_velocity", physics->riding_frame_velocity);
	LOG_VECTOR(variant, "air_frame_velocity", physics->air_frame_velocity);
	LOG_VECTOR(variant, "velocity", physics->velocity);
	LOG_POINT(variant, "position", physics->position);
	LOG_POINT(variant, "new_position", physics->new_position);
}

static void log_biped(int variant)
{
	LOG_INT(variant, "ticks", rider.biped.riding_ticks);
	LOG_INT(variant, "falling", rider.biped.riding_falling);
	LOG_INT(variant, "object", rider.biped.riding_object_index);
	LOG_INT(variant, "updates", rider.biped.riding_vehicle_updates);
	LOG_POINT(variant, "vehicle_position", rider.biped.riding_vehicle_position);
	LOG_VECTOR(variant, "vehicle_forward", rider.biped.riding_vehicle_forward);
	LOG_VECTOR(variant, "vehicle_up", rider.biped.riding_vehicle_up);
	LOG_VECTOR(variant, "velocity_kept", rider.biped.riding_velocity);
	LOG_POINT(variant, "local_point", rider.biped.riding_local_point);
	LOG_INT(variant, "node", rider.biped.riding_node_index);
	LOG_INT(variant, "pad", rider.biped.riding_pad);
	LOG_POINT(variant, "body_position", rider.biped.riding_body_position);
	LOG_VECTOR(variant, "body_forward", rider.biped.riding_body_forward);
	LOG_VECTOR(variant, "body_up", rider.biped.riding_body_up);
}

static void log_player(int variant)
{
	struct vehicle_riding_mark const *mark = vehicle_riding_player_mark(SLOT);
	struct vehicle_riding_air const *air = vehicle_riding_player_air(SLOT);
	struct vehicle_riding_cabin const *cabin = vehicle_riding_player_cabin(SLOT);
	struct vehicle_riding_contact contacts[RIDING_CONTACT_RECORDS];

	LOG_INT(variant, "mark_role", mark->role);
	LOG_INT(variant, "mark_time", mark->time);
	LOG_INT(variant, "mark_vehicle", mark->correction.vehicle_index);
	LOG_REAL(variant, "mark_rotation_i", mark->correction.rotation.v.i);
	LOG_REAL(variant, "mark_rotation_j", mark->correction.rotation.v.j);
	LOG_REAL(variant, "mark_rotation_k", mark->correction.rotation.v.k);
	LOG_REAL(variant, "mark_rotation_w", mark->correction.rotation.w);
	LOG_VECTOR(variant, "mark_translation", mark->correction.translation);
	LOG_INT(variant, "have_peak", air->have_peak);
	LOG_INT(variant, "host_peak", air->host_peak);
	LOG_INT(variant, "host_verdict", air->host_verdict);
	LOG_REAL(variant, "peak", air->peak);
	LOG_INT(variant, "enclosure", air->enclosure);
	LOG_INT(variant, "cabin_unit", cabin->unit_index);
	LOG_INT(variant, "cabin_vehicle", cabin->vehicle_index);
	LOG_INT(variant, "standing", cabin->standing);
	LOG_INT(variant, "misses", cabin->misses);
	LOG_INT(variant, "jump", cabin->jump);
	LOG_INT(variant, "host", cabin->host);
	LOG_INT(variant, "host_legacy", cabin->host_legacy);
	LOG_INT(variant, "cabin_host_verdict", cabin->host_verdict);
	LOG_INT(variant, "host_phase", cabin->host_phase);
	LOG_INT(variant, "body_axis", cabin->body_axis);
	LOG_INT(variant, "take_off", cabin->take_off);
	LOG_INT(variant, "have_anchor", cabin->have_anchor);
	LOG_POINT(variant, "anchor", cabin->anchor);
	LOG_INT(variant, "exception", cabin->exception);
	LOG_INT(variant, "failure_logged", cabin->failure_logged);
	LOG_INT(variant, "failure_log_tick", cabin->failure_log_tick);
	LOG_INT(variant, "failure_seen_tick", cabin->failure_seen_tick);
	LOG_INT(variant, "contacts", vehicle_riding_contact_take(SLOT, contacts));
	LOG_REAL(variant, "kept_dyaw", vehicle_riding_kept_dyaw(0));
}

/* a biped everything of whose riding state is stale, as after a ride */
static void stale_everything(void)
{
	real_vector3d const normal = { 0.f, 0.f, 1.f };
	struct vehicle_riding_mark *mark = vehicle_riding_player_mark(SLOT);
	struct vehicle_riding_air *air = vehicle_riding_player_air(SLOT);
	struct vehicle_riding_cabin *cabin = vehicle_riding_player_cabin(SLOT);

	rider.biped.riding_ticks = RIDING_TICKS;
	rider.biped.riding_falling = 1;
	rider.biped.riding_object_index = VEHICLE;
	rider.biped.riding_vehicle_updates = 7;
	rider.biped.riding_vehicle_position.x = 1.f;
	rider.biped.riding_vehicle_forward.i = 1.f;
	rider.biped.riding_vehicle_up.k = 1.f;
	rider.biped.riding_velocity.i = .5f;
	rider.biped.riding_local_point.z = 2.f;
	rider.biped.riding_node_index = 4;
	rider.biped.riding_pad = 1;
	rider.biped.riding_body_position.y = 3.f;
	rider.biped.riding_body_forward.j = 1.f;
	rider.biped.riding_body_up.k = 1.f;
	mark->role = _vehicle_riding_role_followed;
	mark->time = 9;
	air->have_peak = TRUE;
	air->peak = 2.f;
	air->enclosure = _vehicle_riding_enclosure_enclosed;
	cabin->unit_index = BIPED;
	cabin->vehicle_index = VEHICLE;
	cabin->standing = _vehicle_riding_enclosure_enclosed;
	cabin->have_anchor = TRUE;
	vehicle_riding_look(0, TRUE, .3f);
	CHECK(vehicle_riding_contact_record(SLOT, VEHICLE, &normal), "no contact recorded");
}

/* the hooks, as the game builds them (variant 1) and with the fast path off (variant 0); whether it was taken */
static boolean tick(int variant, struct biped_physics *physics)
{
	boolean idle;

	memset(physics, 0xA5, sizeof(*physics));
	idle = variant ? idle_tick(BIPED, &rider, NULL, physics) : reference_tick(BIPED, &rider, NULL, physics);
	log_physics(variant, physics);
	log_biped(variant);
	log_player(variant);
	return idle;
}

/* one scenario twice from the same world; each stale function puts one thing out of its cleared place, `index`
the field of several */
typedef void (*scenario)(int index);

static boolean ran_fast[2][4];

static void scenario_ticks(boolean player, boolean riding, scenario stale, int index, int ticks)
{
	int variant;

	for (variant = 0; variant < 2; variant++)
	{
		struct biped_physics physics;
		int at;

		world(player, riding);
		log_length[variant] = 0;
		log_text[variant][0] = 0;
		if (stale)
			stale(index);
		for (at = 0; at < ticks; at++)
			ran_fast[variant][at] = tick(variant, &physics);
	}
	CHECK(!strcmp(log_text[0], log_text[1]), "the fast path left another world (field %d):\n  full: %s\n  fast: %s", index,
		log_text[0], log_text[1]);
}

static void stale_none(int index) { (void)index; }

static void stale_state(int index)
{
	switch (index)
	{
	case 0: rider.biped.riding_ticks = 3; break;
	case 1: rider.biped.riding_falling = 1; break;
	case 2: rider.biped.riding_object_index = VEHICLE; break;
	case 3: rider.biped.riding_vehicle_updates = 7; break;
	case 4: rider.biped.riding_vehicle_position.x = 1.f; break;
	case 5: rider.biped.riding_vehicle_forward.i = 1.f; break;
	case 6: rider.biped.riding_vehicle_up.k = 1.f; break;
	case 7: rider.biped.riding_velocity.j = .5f; break;
	case 8: rider.biped.riding_local_point.z = 2.f; break;
	case 9: rider.biped.riding_node_index = 4; break;
	case 10: rider.biped.riding_body_position.y = 3.f; break;
	case 11: rider.biped.riding_body_forward.j = 1.f; break;
	case 12: rider.biped.riding_body_up.k = 1.f; break;
	}
}
enum { STALE_STATE_FIELDS = 13 };

static void stale_pad(int index) { (void)index; rider.biped.riding_pad = 1; }

static void stale_mark(int index)
{
	struct vehicle_riding_mark *mark = vehicle_riding_player_mark(SLOT);

	switch (index)
	{
	case 0: mark->role = _vehicle_riding_role_followed; break;
	case 1: mark->time = 9; break;
	case 2: mark->correction.vehicle_index = VEHICLE; break;
	case 3: mark->correction.rotation.v.i = .5f; break;
	case 4: mark->correction.rotation.w = .5f; break;
	case 5: mark->correction.translation.k = 1.f; break;
	}
}
enum { STALE_MARK_FIELDS = 6 };

static void stale_contact(int index)
{
	real_vector3d const normal = { 0.f, 0.f, 1.f };

	(void)index;
	CHECK(vehicle_riding_contact_record(SLOT, VEHICLE, &normal), "no contact recorded");
}

static void stale_cabin(int index)
{
	struct vehicle_riding_cabin *cabin = vehicle_riding_player_cabin(SLOT);

	switch (index)
	{
	case 0: cabin->unit_index = BIPED; break;
	case 1: cabin->vehicle_index = VEHICLE; break;
	case 2: cabin->standing = _vehicle_riding_enclosure_enclosed; break;
	case 3: cabin->misses = 1; break;
	case 4: cabin->jump = _vehicle_riding_enclosure_enclosed; break;
	case 5: cabin->host = TRUE; break;
	case 6: cabin->host_legacy = TRUE; break;
	case 7: cabin->host_verdict = _vehicle_riding_enclosure_enclosed; break;
	case 8: cabin->host_phase = _vehicle_riding_cabin_sample_jump; break;
	case 9: cabin->body_axis = TRUE; break;
	case 10: cabin->take_off = TRUE; break;
	case 11: cabin->have_anchor = TRUE; break;
	case 12: cabin->anchor.z = 1.f; break;
	case 13: cabin->exception = TRUE; break;
	case 14: cabin->failure_logged = TRUE; break;
	case 15: cabin->failure_log_tick = 5; break;
	case 16: cabin->failure_seen_tick = 5; break;
	}
}
enum { STALE_CABIN_FIELDS = 17 };

static void stale_air(int index)
{
	struct vehicle_riding_air *air = vehicle_riding_player_air(SLOT);

	switch (index)
	{
	case 0: air->have_peak = TRUE; break;
	case 1: air->host_peak = TRUE; break;
	case 2: air->host_verdict = TRUE; break;
	case 3: air->peak = 2.f; break;
	case 4: air->enclosure = _vehicle_riding_enclosure_enclosed; break;
	}
}
enum { STALE_AIR_FIELDS = 5 };

static void stale_dyaw(int index)
{
	(void)index;
	vehicle_riding_look(0, TRUE, .3f);
}

/* (riding was on, and the switch goes off: a map script, or the host's message) */
/* a cleared float that holds -0 instead: the full path writes +0, so the fast path may not be taken */
static real const negative_zero = -0.f;

static void stale_negative_fields(int index)
{
	real *fields[] = {
		&rider.biped.riding_vehicle_position.x, &rider.biped.riding_vehicle_position.y, &rider.biped.riding_vehicle_position.z,
		&rider.biped.riding_vehicle_forward.i, &rider.biped.riding_vehicle_forward.j, &rider.biped.riding_vehicle_forward.k,
		&rider.biped.riding_vehicle_up.i, &rider.biped.riding_vehicle_up.j, &rider.biped.riding_vehicle_up.k,
		&rider.biped.riding_velocity.i, &rider.biped.riding_velocity.j, &rider.biped.riding_velocity.k,
		&rider.biped.riding_local_point.x, &rider.biped.riding_local_point.y, &rider.biped.riding_local_point.z,
		&rider.biped.riding_body_position.x, &rider.biped.riding_body_position.y, &rider.biped.riding_body_position.z,
		&rider.biped.riding_body_forward.i, &rider.biped.riding_body_forward.j, &rider.biped.riding_body_forward.k,
		&rider.biped.riding_body_up.i, &rider.biped.riding_body_up.j, &rider.biped.riding_body_up.k,
	};

	*fields[index] = negative_zero;
}
enum { STALE_NEGATIVE_FIELDS = 24 };

static void stale_negative_tables(int index)
{
	struct vehicle_riding_mark *mark = vehicle_riding_player_mark(SLOT);
	struct vehicle_riding_air *air = vehicle_riding_player_air(SLOT);
	struct vehicle_riding_cabin *cabin = vehicle_riding_player_cabin(SLOT);
	real *fields[] = {
		&mark->correction.rotation.v.i, &mark->correction.rotation.v.j, &mark->correction.rotation.v.k,
		&mark->correction.translation.i, &mark->correction.translation.j, &mark->correction.translation.k,
		&air->peak, &cabin->anchor.x, &cabin->anchor.y, &cabin->anchor.z,
	};

	*fields[index] = negative_zero;
}
enum { STALE_NEGATIVE_TABLES = 10 };

static void stale_negative_dyaw(int index)
{
	(void)index;
	vehicle_riding_look(0, TRUE, negative_zero);
}

static void stale_all(int index)
{
	(void)index;
	stale_everything();
	map_slot_storage = FALSE;
}

/* a case: one stale thing at a time, off, and the fast path not taken for it */
static void only(scenario stale, int fields)
{
	int index;

	for (index = 0; index < fields; index++)
	{
		scenario_ticks(TRUE, FALSE, stale, index, 1);
		CHECK(!ran_fast[1][0], "the fast path taken with field %d stale", index);
	}
}

/* the seated hook from the same world, with the fast path off (variant 0) and as built (1) */
static void seated_ticks(scenario stale, int index)
{
	int variant;

	for (variant = 0; variant < 2; variant++)
	{
		struct biped_physics physics;

		world(TRUE, TRUE);
		log_length[variant] = 0;
		log_text[variant][0] = 0;
		if (stale)
			stale(index);
		memset(&physics, 0, sizeof(physics));
		if (variant)
			biped_riding_seated(BIPED, &rider);
		else
			reference_seated(BIPED, &rider);
		log_biped(variant);
		log_player(variant);
	}
	CHECK(!strcmp(log_text[0], log_text[1]), "the seated fast path left another world (field %d):\n  full: %s\n  fast: %s",
		index, log_text[0], log_text[1]);
}

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	CASE("riding-on-reached")
	{
		/* (the map's slot cleared, so riding follows the stored decision of the setting) */
		world(TRUE, TRUE);
		map_slot_storage = NONE;
		CHECK(vehicle_riding_enabled(), "riding is not on under the fixture's own on setup");
		return 0;
	}
	CASE("riding-off-reached")
	{
		world(TRUE, FALSE);
		map_slot_storage = NONE;
		CHECK(vehicle_riding_enabled(), "riding is not on before the setting goes off");
		config_riding = 0;
#ifndef FIXTURE_NO_DECIDE_OFF
		vehicle_riding_map_decide();
#endif
		CHECK(!vehicle_riding_enabled(), "riding is still on after the setting went off and the map decided again");
		return 0;
	}
	CASE("off-clean-player")
	{
		scenario_ticks(TRUE, FALSE, stale_none, 0, 1);
		CHECK(ran_fast[1][0] && !ran_fast[0][0], "the fast path not taken on a clean player (or taken in the reference)");
		return 0;
	}
	CASE("off-ai-biped")
	{
		scenario_ticks(FALSE, TRUE, stale_none, 0, 1);
		CHECK(ran_fast[1][0] && !ran_fast[0][0], "the fast path not taken on an AI biped");
		return 0;
	}
	CASE("off-seated")
	{
		/* (a clean seat; then the same with the mark stale, which the full path clears) */
		seated_ticks(stale_none, 0);
		seated_ticks(stale_mark, 1);
		seated_ticks(stale_state, 0);
		seated_ticks(stale_cabin, 2);
		seated_ticks(stale_dyaw, 0);
		return 0;
	}
	CASE("off-after-ride")
	{
		/* tick 1 clears everything (the full path), tick 2 finds it clear (the fast path); both as the reference */
		scenario_ticks(TRUE, TRUE, stale_all, 0, 2);
		CHECK(!ran_fast[1][0] && ran_fast[1][1], "ticks %d, %d: the first must be full and the second fast",
			ran_fast[1][0], ran_fast[1][1]);
		return 0;
	}
	CASE("off-stale-state-only") { only(stale_state, STALE_STATE_FIELDS); return 0; }
	CASE("off-stale-pad-only") { only(stale_pad, 1); return 0; }
	CASE("off-stale-mark-only") { only(stale_mark, STALE_MARK_FIELDS); return 0; }
	CASE("off-stale-contact-only") { only(stale_contact, 1); return 0; }
	CASE("off-stale-cabin-only") { only(stale_cabin, STALE_CABIN_FIELDS); return 0; }
	CASE("off-stale-air-only") { only(stale_air, STALE_AIR_FIELDS); return 0; }
	CASE("off-stale-dyaw-only") { only(stale_dyaw, 1); return 0; }
	CASE("off-negative-zero-fields") { only(stale_negative_fields, STALE_NEGATIVE_FIELDS); return 0; }
	CASE("off-negative-zero-tables") { only(stale_negative_tables, STALE_NEGATIVE_TABLES); return 0; }
	CASE("off-negative-zero-dyaw") { only(stale_negative_dyaw, 1); return 0; }
	CASE("off-negative-zero-seated")
	{
		/* (the seat's full path writes +0 over each of them too) */
		int index;

		for (index = 0; index < STALE_NEGATIVE_FIELDS; index++)
			seated_ticks(stale_negative_fields, index);
		for (index = 0; index < STALE_NEGATIVE_TABLES; index++)
			seated_ticks(stale_negative_tables, index);
		seated_ticks(stale_negative_dyaw, 0);
		return 0;
	}
	CASE("off-work-skipped")
	{
		struct biped_physics physics;

		world(TRUE, FALSE);
		tick(0, &physics);
		CHECK(get_calls > 0 && set_calls > 0 && air_begin_calls > 0 && cabin_begin_calls > 0,
			"the full path does not do the work this counts (get %ld, set %ld, air %ld, cabin %ld)", get_calls, set_calls,
			air_begin_calls, cabin_begin_calls);
		world(TRUE, FALSE);
		tick(1, &physics);
		CHECK(get_calls == 0, "biped_riding_get called %ld times", get_calls);
		CHECK(set_calls == 0, "biped_riding_set called %ld times", set_calls);
		CHECK(air_begin_calls == 0, "vehicle_riding_air_begin called %ld times", air_begin_calls);
		CHECK(cabin_begin_calls == 0, "vehicle_riding_cabin_begin called %ld times", cabin_begin_calls);
		return 0;
	}
	CASE("deferred-reads-fields")
	{
		static char const ticks[] = { -1, 0, 1, RIDING_TICKS };
		int tick_at;
		int applies;

		for (tick_at = 0; tick_at < 4; tick_at++)
		{
			for (applies = 0; applies < 2; applies++)
			{
				struct vehicle_riding_state state;

				world(TRUE, TRUE);
				rider.biped.riding_ticks = ticks[tick_at];
				if (!applies)
					rider.object.damage_flags |= (unsigned)FLAG(_object_dead_bit);
				vehicle_riding_clear(&state);
				state.ticks = ticks[tick_at];
				get_calls = 0;
				CHECK(biped_riding_deferred(BIPED) == vehicle_riding_deferred(&state, applies), "ticks %d, applies %d",
					ticks[tick_at], applies);
				CHECK(get_calls == 0, "biped_riding_get called %ld times", get_calls);
			}
		}
		return 0;
	}
	CASE("collision-reads-fields")
	{
		static char const ticks[] = { -1, 0, 1, RIDING_TICKS };
		static long const ridden[] = { VEHICLE, VEHICLE + 1 };
		real_vector3d const normal = { 0.f, 0.f, 1.f };
		real_point3d const base = { 0.f, 0.f, 0.f };
		int tick_at;
		int applies;
		int object;

		for (tick_at = 0; tick_at < 4; tick_at++)
		{
			for (applies = 0; applies < 2; applies++)
			{
				for (object = 0; object < 2; object++)
				{
					struct vehicle_riding_state state;

					world(TRUE, TRUE);
					rider.biped.riding_ticks = ticks[tick_at];
					rider.biped.riding_object_index = ridden[object];
					if (!applies)
						rider.object.damage_flags |= (unsigned)FLAG(_object_dead_bit);
					vehicle_riding_clear(&state);
					state.ticks = ticks[tick_at];
					state.object_index = ridden[object];
					get_calls = 0;
					CHECK(biped_riding_collision_head(BIPED, VEHICLE, &normal, &base, 1.f) ==
						vehicle_riding_collision_window(&state, applies, VEHICLE), "ticks %d, applies %d, object %d",
						ticks[tick_at], applies, object);
					if (!applies || ticks[tick_at] <= 0)
						CHECK(get_calls == 0, "biped_riding_get called %ld times (ticks %d, applies %d)", get_calls,
							ticks[tick_at], applies);
				}
			}
		}
		return 0;
	}
	CASE("falling-reads-fields")
	{
		static char const ticks[] = { 0, 1, RIDING_TICKS };
		int tick_at;
		int falling;

		for (tick_at = 0; tick_at < 3; tick_at++)
		{
			for (falling = 0; falling < 2; falling++)
			{
				struct vehicle_riding_state state;

				world(TRUE, TRUE);
				rider.object.translational_velocity.k = -.3f;
				rider.biped.riding_ticks = ticks[tick_at];
				rider.biped.riding_falling = (char)falling;
				rider.biped.riding_velocity.k = -.5f;
				vehicle_riding_clear(&state);
				state.ticks = ticks[tick_at];
				state.falling = (char)falling;
				state.velocity.k = -.5f;
				get_calls = 0;
				CHECK(biped_riding_falling_velocity(&rider) == vehicle_riding_falling_velocity(&state, -.3f),
					"ticks %d, falling %d", ticks[tick_at], falling);
				if (ticks[tick_at] != RIDING_TICKS && !falling)
					CHECK(get_calls == 0, "biped_riding_get called %ld times", get_calls);
			}
		}
		return 0;
	}
	fprintf(stderr, "unknown case: %s\n", case_name);
	return 2;
}
