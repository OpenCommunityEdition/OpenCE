/*
VEHICLE_RIDING.C

A player standing on a moving vehicle is carried by it (network.vehicle_riding,
port/linux/README.md): its position and view move with the vehicle, a jump
lands back on it, stepping off keeps its motion, and the vehicle does not
hurt it. These are the decisions of each tick, over a plain copy of a
biped's riding fields, so the asset-free harness compiles this file whole
(tools/harness/tests/vehicle_riding.c); vehicle_riding_bipeds.c gathers what
the game knows, applies what comes back and updates riders after the
vehicles, and physics.c asks the collision decision. The map's choice
wins where it made one, else in a network game the host's switch decides
for every machine; film playback and the editor play
the original game.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "vehicle_riding.h"
#include "cseries/errors.h"
#include "editor/editor_stubs.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "hs/hs.h"
#include "hs/hs_scenario_definitions.h"
#include "saved games/game_state.h"

#include <math.h>

/* ---------- constants */

/* the least a vehicle's up keeps of world up to be a deck in the air (45
degrees off it) */
#define RIDING_UPRIGHT_K 0.70710678f

/* the most a ridden node's origin may shift (world units) and its up may
tilt (radians) on its vehicle's body a player tick and still be a deck: a
turret stays put and turns about its own up, a leg's stride moves both */
#define RIDING_NODE_SHIFT 0.01f
#define RIDING_NODE_TILT 0.01f

/* how far a followed vehicle's move may depart from its own velocity's path
over the elapsed ticks (world units) before it is the host's snap, carried
but never the deck's speed (a Longsword's 2.5 a tick along its velocity is
motion) */
#define RIDING_SNAP_STEP 1.0f

/* the air frame's speed below which it has no way of its own and goes the
deck's (world units a tick) */
#define RIDING_AIR_STILL 0.001f

/* how far back inside the biped's surface limit a vehicle's up must come
(a cosine) before the cabin's rays turn from its up to world up again: a
ship rocking about the limit keeps one axis */
#define RIDING_CABIN_AXIS_BAND 0.01f

/* one failure report per second of player simulation */
#define RIDING_CABIN_LOG_TICKS 30

/* ---------- prototypes */

/* (port_config.c, read as units.c reads them) */
int config_boolean(const char *name);
double config_real(const char *name);
/* (network_coop.c's: the host's switch, in a network game) */
boolean network_game_port_vehicle_riding(void);

/* ---------- globals */

/* the game's decision for the running map, made at its start (a revert keeps it; a setting changed now counts from the next map) */
static boolean riding_local_decision;
static boolean riding_network_decision;
/* game state so a revert or saved game keeps the map's choice */
static long *riding_map;
static short riding_host_map = NONE;
static boolean riding_map_refusal_logged;
static boolean riding_look_carry = TRUE;
static real riding_damage_speed;
static real riding_teleport_step = 4.f;
static real riding_moved_alone = 0.25f;
static real riding_transition_leash = 0.25f;
static real riding_air_grip = .045f;
static short riding_cabin_grace = RIDING_CABIN_GRACE;
/* the host's values a client holds for this game; a latch puts them back over
its own config */
static boolean riding_host_held;
static struct distributed_riding_latched riding_host_values;

/* each local player's view turn by its vehicle this tick, for the direct
camera (render_interpolation.c) */
static real riding_kept_dyaws[MAXIMUM_LOCAL_PLAYERS];

/* each player's air frame beyond its biped's fields, by the player's
absolute index: the host rides every player's unit and a client its remote
ones, which have no local index (never saved: a load starts the peak again
from the frame's speed) */
static struct vehicle_riding_air riding_airs[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

/* each player's touches by vehicles this tick, by the same index, recorded
during the vehicles' updates and taken by its own tick (never saved) */
static struct vehicle_riding_contact riding_contacts[HALO_PORT_MAXIMUM_NETWORK_PLAYERS][RIDING_CONTACT_RECORDS];
static short riding_contact_counts[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

/* each player's cabin, by the same index (never saved: a load samples it
again) */
static struct vehicle_riding_cabin riding_cabins[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

/* each player's snapshot of the vehicle's writes, never saved */
static struct vehicle_riding_mark riding_marks[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static struct vehicle_riding_claim riding_claims[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static struct vehicle_riding_correction riding_corrections[HALO_PORT_MAXIMUM_OBJECTS_PER_MAP];

/* ---------- private code */

/* (a vehicle without a player updates every other tick in double speed, so
its velocities and moves are for two player ticks) */
static real riding_period(
	struct vehicle_riding_vehicle const *vehicle,
	boolean double_speed)
{
	return double_speed && !vehicle->has_player ? 2.f : 1.f;
}

/* the part of a velocity into a support's plane, measured against the
support's own motion, taken off: the landing on it is counted once */
static void riding_clip(
	real_vector3d *velocity,
	real_vector3d const *support_velocity,
	real_vector3d const *normal)
{
	real_vector3d relative;
	real into;

	subtract_vectors3d(velocity, support_velocity, &relative);
	into = dot_product3d(&relative, normal);
	if (into < 0.f)
	{
		scale_vector3d(normal, into, &relative);
		subtract_vectors3d(velocity, &relative, velocity);
	}
}

/* the vehicle's pose and update count as they are now */
static void riding_snapshot(
	struct vehicle_riding_state *state,
	struct vehicle_riding_vehicle const *vehicle)
{
	state->vehicle_position = vehicle->position;
	state->vehicle_forward = vehicle->forward;
	state->vehicle_up = vehicle->up;
	state->vehicle_updates = vehicle->updates;
	state->body_position = vehicle->origin;
	state->body_forward = vehicle->body_forward;
	state->body_up = vehicle->body_up;
	state->mark.role = vehicle->role;
	state->mark.time = vehicle->time;
	state->mark.correction = vehicle->correction;
}

boolean vehicle_riding_rebase(
	struct vehicle_riding_state *state,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base)
{
	if (!vehicle || state->object_index != vehicle->object_index || state->node_index != vehicle->node_index)
		return FALSE;
	riding_snapshot(state, vehicle);
	vehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, base, &state->local_point);
	return TRUE;
}

/* a vehicle posed as its own body, with no node */
static void riding_body(
	struct vehicle_riding_vehicle const *vehicle,
	struct vehicle_riding_vehicle *body)
{
	*body = *vehicle;
	body->node_index = NONE;
	body->position = vehicle->origin;
	body->forward = vehicle->body_forward;
	body->up = vehicle->body_up;
}

/* the window anchored to the vehicle's body at a point: the frame of a
biped that left the floor or was moved off it, never a leg's or a turret's */
static void riding_anchor_body(
	struct vehicle_riding_state *state,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *point)
{
	struct vehicle_riding_vehicle body;

	riding_body(vehicle, &body);
	riding_snapshot(state, &body);
	state->node_index = NONE;
	vehicle_riding_point_to_local(&body.position, &body.forward, &body.up, point, &state->local_point);
}

/* a rider moved off its spot by something else (a script, a fix, a
teleport): in the air from the vehicle's body where it is now, its frame
velocity kept */
static void riding_open_window(
	struct vehicle_riding_state *state,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base)
{
	riding_anchor_body(state, vehicle, base);
	state->ticks = RIDING_TICKS - 1;
	state->falling = 1;
}

boolean vehicle_riding_take_deck(
	struct vehicle_riding_state *state,
	struct vehicle_riding_vehicle const *vehicle,
	struct vehicle_riding_deck const *deck,
	real_point3d const *base)
{
	if (!vehicle || vehicle->object_index != deck->vehicle_index || vehicle->node_index != deck->node_index)
		return FALSE;
	/* (in the air the window is the body's, as the host's own, and its frame is the host's air frame) */
	if (deck->ticks < RIDING_TICKS)
	{
		riding_anchor_body(state, vehicle, base);
		state->object_index = deck->vehicle_index;
		state->ticks = deck->ticks;
		state->falling = TEST_FLAG(deck->flags, _vehicle_riding_deck_falling_bit);
		state->velocity = deck->velocity;
		return TRUE;
	}
	/* (a correction changes the place, not an existing window's frame speed) */
	if (!(state->ticks > 0 && state->object_index == deck->vehicle_index && state->node_index == deck->node_index))
		vehicle_riding_deck_point_velocity(vehicle, base, game_players_are_double_speed(), &state->velocity);
	state->object_index = deck->vehicle_index;
	state->node_index = deck->node_index;
	state->ticks = deck->ticks;
	state->falling = TEST_FLAG(deck->flags, _vehicle_riding_deck_falling_bit);
	riding_snapshot(state, vehicle);
	/* (the blend may not have reached the part's place) */
	vehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, base, &state->local_point);
	return TRUE;
}

/* a new jump's air: no peak yet, no verdict */
static void riding_air_clear(
	struct vehicle_riding_air *air)
{
	air->have_peak = FALSE;
	air->host_peak = FALSE;
	air->host_verdict = FALSE;
	air->peak = 0.f;
	air->enclosure = _vehicle_riding_enclosure_unknown;
}

/* how far a node moved on its vehicle's body since the snapshot: the shift
of its origin and the tilt of its up, both in the body's frame */
static void riding_node_motion(
	struct vehicle_riding_state const *state,
	struct vehicle_riding_vehicle const *vehicle,
	real *shift,
	real *tilt)
{
	real_point3d tip;
	real_point3d then_origin;
	real_point3d then_tip;
	real_point3d now_origin;
	real_point3d now_tip;
	real_vector3d moved;
	real_vector3d then_up;
	real_vector3d now_up;
	real_vector3d turned;

	vehicle_riding_point_to_local(&state->body_position, &state->body_forward, &state->body_up,
		&state->vehicle_position, &then_origin);
	tip.x = state->vehicle_position.x + state->vehicle_up.i;
	tip.y = state->vehicle_position.y + state->vehicle_up.j;
	tip.z = state->vehicle_position.z + state->vehicle_up.k;
	vehicle_riding_point_to_local(&state->body_position, &state->body_forward, &state->body_up, &tip, &then_tip);
	vehicle_riding_point_to_local(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, &vehicle->position,
		&now_origin);
	tip.x = vehicle->position.x + vehicle->up.i;
	tip.y = vehicle->position.y + vehicle->up.j;
	tip.z = vehicle->position.z + vehicle->up.k;
	vehicle_riding_point_to_local(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, &tip, &now_tip);
	vector_from_points3d(&then_origin, &now_origin, &moved);
	*shift = magnitude3d(&moved);
	vector_from_points3d(&then_origin, &then_tip, &then_up);
	vector_from_points3d(&now_origin, &now_tip, &now_up);
	cross_product3d(&then_up, &now_up, &turned);
	*tilt = arctangent(magnitude3d(&turned), dot_product3d(&then_up, &now_up));
}

/* ---------- public code */

void vehicle_riding_correction_identity(
	struct vehicle_riding_correction *correction,
	long vehicle_index)
{
	correction->vehicle_index = vehicle_index;
	correction->rotation.v.i = correction->rotation.v.j = correction->rotation.v.k = 0.f;
	correction->rotation.w = 1.f;
	correction->translation.i = correction->translation.j = correction->translation.k = 0.f;
}

static void riding_correction_matrix(
	struct vehicle_riding_correction const *correction,
	real_matrix4x3 *matrix)
{
	matrix4x3_rotation_from_quaternion(matrix, &correction->rotation);
	matrix->position.x = correction->translation.i;
	matrix->position.y = correction->translation.j;
	matrix->position.z = correction->translation.k;
}

void vehicle_riding_correction_compose(
	struct vehicle_riding_correction *correction,
	real_point3d const *before_position,
	real_vector3d const *before_forward,
	real_vector3d const *before_up,
	real_point3d const *after_position,
	real_vector3d const *after_forward,
	real_vector3d const *after_up)
{
	real_matrix4x3 before, after, inverse, write, accumulated, composed;

	matrix4x3_from_point_and_vectors(&before, before_position, before_forward, before_up);
	matrix4x3_from_point_and_vectors(&after, after_position, after_forward, after_up);
	matrix4x3_inverse(&before, &inverse);
	matrix4x3_multiply(&after, &inverse, &write);
	riding_correction_matrix(correction, &accumulated);
	/* (newest writes act on the world place the earlier writes produced) */
	matrix4x3_multiply(&write, &accumulated, &composed);
	matrix4x3_rotation_to_quaternion(&composed, &correction->rotation);
	quaternion_normalize(&correction->rotation);
	correction->translation.i = composed.position.x;
	correction->translation.j = composed.position.y;
	correction->translation.k = composed.position.z;
}

void vehicle_riding_correction_since(
	struct vehicle_riding_correction const *now,
	struct vehicle_riding_correction const *then,
	real_matrix4x3 *since)
{
	real_matrix4x3 current, previous, inverse;
	struct vehicle_riding_correction identity;

	if (now->vehicle_index != then->vehicle_index)
	{
		vehicle_riding_correction_identity(&identity, NONE);
		riding_correction_matrix(&identity, since);
		return;
	}
	/* (the same correction twice is no motion, to the bit: M times its inverse
	is not exactly the identity in single precision, and a double-speed skipped
	tick has no update between the two) */
	if (now->rotation.v.i == then->rotation.v.i && now->rotation.v.j == then->rotation.v.j &&
		now->rotation.v.k == then->rotation.v.k && now->rotation.w == then->rotation.w &&
		now->translation.i == then->translation.i && now->translation.j == then->translation.j &&
		now->translation.k == then->translation.k)
	{
		vehicle_riding_correction_identity(&identity, NONE);
		riding_correction_matrix(&identity, since);
		return;
	}
	riding_correction_matrix(now, &current);
	riding_correction_matrix(then, &previous);
	matrix4x3_inverse(&previous, &inverse);
	matrix4x3_multiply(&current, &inverse, since);
}

struct vehicle_riding_correction const *vehicle_riding_vehicle_correction(
	long vehicle_index)
{
	static struct vehicle_riding_correction const identity = { NONE, { { 0.f, 0.f, 0.f }, 1.f }, { 0.f, 0.f, 0.f } };
	long index = DATUM_INDEX_TO_ABSOLUTE_INDEX(vehicle_index);
	struct vehicle_riding_correction *correction;

	if (vehicle_index == NONE || index >= HALO_PORT_MAXIMUM_OBJECTS_PER_MAP)
		return &identity;
	correction = &riding_corrections[index];
	/* (a reused slot must not inherit the old vehicle's writes) */
	if (correction->vehicle_index != vehicle_index)
		vehicle_riding_correction_identity(correction, vehicle_index);
	return correction;
}

void vehicle_riding_vehicle_corrected(
	long vehicle_index,
	real_point3d const *before_position,
	real_vector3d const *before_forward,
	real_vector3d const *before_up,
	real_point3d const *after_position,
	real_vector3d const *after_forward,
	real_vector3d const *after_up)
{
	long index = DATUM_INDEX_TO_ABSOLUTE_INDEX(vehicle_index);

	if (vehicle_index == NONE || index >= HALO_PORT_MAXIMUM_OBJECTS_PER_MAP)
		return;
	vehicle_riding_vehicle_correction(vehicle_index);
	vehicle_riding_correction_compose(&riding_corrections[index], before_position, before_forward, before_up,
		after_position, after_forward, after_up);
}

struct vehicle_riding_mark *vehicle_riding_player_mark(
	short player_index)
{
	if (!(player_index >= 0 && player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return NULL;
	return &riding_marks[player_index];
}

struct vehicle_riding_claim *vehicle_riding_player_claim(
	short player_index)
{
	if (!(player_index >= 0 && player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return NULL;
	return &riding_claims[player_index];
}

boolean vehicle_riding_claim_grace(
	struct vehicle_riding_claim *claim,
	long own_vehicle,
	short own_node,
	long host_vehicle,
	short host_node,
	short grace)
{
	if (!claim)
		return FALSE;
	if (claim->vehicle_index != host_vehicle || claim->node_index != host_node)
		claim->taken = FALSE;
	if (own_vehicle == host_vehicle && own_node == host_node)
	{
		claim->disagreement = 0;
		return FALSE;
	}
	/* (a taken host node is not put through another local grace) */
	if (claim->taken)
		return FALSE;
	if (++claim->disagreement <= grace)
		return FALSE;
	claim->vehicle_index = host_vehicle;
	claim->node_index = host_node;
	claim->taken = TRUE;
	claim->disagreement = 0;
	return TRUE;
}

static void riding_latch_grace(double grace)
{
	/* (player ticks: not a number is the default, a fraction cut off, then
	held to the range) */
	/* (not isfinite: the Linux build's math.h has no such macro, so it would
	be an undefined function) */
	if (!(grace == grace && grace - grace == 0.))
		riding_cabin_grace = RIDING_CABIN_GRACE;
	else if (grace <= 0.)
		riding_cabin_grace = 0;
	else if (grace >= (double)RIDING_CABIN_GRACE_MAXIMUM)
		riding_cabin_grace = RIDING_CABIN_GRACE_MAXIMUM;
	else
		riding_cabin_grace = (short)grace;
}

void vehicle_riding_latched_get(
	struct distributed_riding_latched *latched)
{
	latched->damage_speed = riding_damage_speed;
	latched->teleport_step = riding_teleport_step;
	latched->moved_alone = riding_moved_alone;
	latched->air_grip = riding_air_grip;
	latched->look_carry = riding_look_carry != FALSE;
	latched->cabin_grace = (word)riding_cabin_grace;
}

/* a calibration value as the client holds it, so the host's latch and a client taking the host's agree */
static real riding_checked(
	real value,
	real fallback)
{
	return valid_real(value) && value >= 0.f ? value : fallback;
}

static void riding_take_host_values(
	struct distributed_riding_latched const *host)
{
	/* (a value off the wire is held to what its config would be: no number,
	infinite or negative falls back to the default, and the grip to none) */
	riding_damage_speed = riding_checked(host->damage_speed, 0.f);
	riding_teleport_step = riding_checked(host->teleport_step, 4.f);
	riding_moved_alone = riding_checked(host->moved_alone, 0.25f);
	riding_air_grip = 0.f;
	if (valid_real(host->air_grip) && host->air_grip >= 0.f)
		riding_air_grip = host->air_grip;
	riding_look_carry = host->look_carry != 0;
	riding_latch_grace((double)host->cabin_grace);
}

void vehicle_riding_latched_received(
	struct distributed_riding_latched const *host)
{
	if (game_connection() != _game_connection_network_client)
		return;
	riding_host_values = *host;
	riding_host_held = TRUE;
	riding_take_host_values(host);
	/* (the one line a run shows the client rides by the host's calibration) */
	error(_error_silent, "vehicle riding: values from the host: air grip %f, damage speed %f, teleport step %f, moved alone %f, look carry %d, enclosure grace %d",
		riding_air_grip, riding_damage_speed, riding_teleport_step, riding_moved_alone, riding_look_carry != FALSE, (int)riding_cabin_grace);
}

void vehicle_riding_latched_forget(void)
{
	riding_host_held = FALSE;
}

boolean vehicle_riding_latch(
	void)
{
	short local_player_index;
	short player_index;
	long object_index;
	double grace;

	riding_look_carry = config_boolean("debug.riding_look_carry") != 0;
	riding_damage_speed = riding_checked((real)config_real("debug.riding_damage_speed"), 0.f);
	riding_teleport_step = riding_checked((real)config_real("debug.riding_teleport_step"), 4.f);
	riding_moved_alone = riding_checked((real)config_real("debug.riding_moved_alone"), 0.25f);
	riding_transition_leash = (real)config_real("debug.riding_transition_leash");
	/* (the leash is a distance, never an unbounded or negative allowance) */
	if (!valid_real(riding_transition_leash) || riding_transition_leash < 0.f)
		riding_transition_leash = 0.f;
	riding_air_grip = (real)config_real("debug.riding_air_grip");
	/* (world units a tick, each tick: 0 keeps the take-off's motion, and a
	large value follows the deck at once, so it needs no clamp above; below
	0 or not a number is none) */
	if (!(riding_air_grip >= 0.f))
		riding_air_grip = 0.f;
	grace = config_real("debug.riding_enclosure_grace_ticks");
	riding_latch_grace(grace);
	/* (a client's latch after the host's message keeps the host's values) */
	if (riding_host_held && game_connection() == _game_connection_network_client)
		riding_take_host_values(&riding_host_values);
	for (local_player_index = 0; local_player_index < MAXIMUM_LOCAL_PLAYERS; local_player_index++)
	{
		riding_kept_dyaws[local_player_index] = 0.f;
	}
	for (player_index = 0; player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; player_index++)
	{
		riding_air_clear(&riding_airs[player_index]);
		vehicle_riding_cabin_clear(&riding_cabins[player_index]);
		riding_marks[player_index].role = _vehicle_riding_role_simulated;
		riding_marks[player_index].time = 0;
		vehicle_riding_correction_identity(&riding_marks[player_index].correction, NONE);
		riding_claims[player_index].vehicle_index = NONE;
		riding_claims[player_index].node_index = NONE;
		riding_claims[player_index].disagreement = 0;
		riding_claims[player_index].taken = FALSE;
	}
	for (object_index = 0; object_index < HALO_PORT_MAXIMUM_OBJECTS_PER_MAP; object_index++)
		vehicle_riding_correction_identity(&riding_corrections[object_index], NONE);
	vehicle_riding_contacts_clear();

	return vehicle_riding_enabled();
}

boolean vehicle_riding_enabled(
	void)
{
	short connection = game_connection();
	short map;

	/* (the connection is still asked on every call: a map's decisions are
	stored per kind of game, and which one counts is this machine's
	connection now, so a client takes the host's, never its own option) */
	if (game_in_editor())
		return FALSE;
	if (connection != _game_connection_local && connection != _game_connection_network_server && connection != _game_connection_network_client)
		return FALSE;
	map = vehicle_riding_map();
	if (map != NONE)
		return map != FALSE;
	return connection == _game_connection_local ? riding_local_decision : riding_network_decision;
}

/* every machine's decision for the map that starts: a local game's from its own inputs, a network game's the host's, as
received (bit 3 of the settings, fixed when the host started the game) */
void vehicle_riding_map_decide(void)
{
	boolean multiplayer = game_engine_running();
	boolean decision;

	riding_local_decision = vehicle_riding_decide(multiplayer,
		multiplayer && TEST_FLAG(game_engine_get_variant()->universal_variant.flags, _game_variant_vehicle_riding_bit),
		config_boolean("network.vehicle_riding") != 0, NONE, FALSE, FALSE);
	riding_network_decision = network_game_port_vehicle_riding();
	decision = game_connection() == _game_connection_local ? riding_local_decision : riding_network_decision;
	error(_error_silent, "vehicle riding: %s for this map unless the map chooses", decision ? "on" : "off");
}

/* the game's rule below a map's own choice, decided on the host (or locally) at a map's start */
boolean vehicle_riding_decide(
	boolean multiplayer,
	boolean gametype_rule,
	boolean host_option,
	short entry_rule,
	boolean dedicated,
	boolean override_gametype)
{
	if (!multiplayer)
		return entry_rule != NONE ? entry_rule != FALSE : host_option;
	return dedicated && override_gametype ? host_option : gametype_rule;
}

void vehicle_riding_map_initialize(void)
{
	riding_map = (long *)game_state_malloc("vehicle riding map", NULL, sizeof(long));
}

void vehicle_riding_map_reset(void)
{
	if (riding_map)
		*riding_map = NONE;
	riding_map_refusal_logged = FALSE;
}

short vehicle_riding_map(void)
{
	/* a client's own scripts never decide: only the host's received value */
	if (game_connection()==_game_connection_network_client)
		return riding_host_map;
	return riding_map ? (short)*riding_map : NONE;
}

void vehicle_riding_map_received(short map)
{
	riding_host_map = map;
}

void vehicle_riding_map_script(boolean on)
{
	if (game_in_editor() || game_connection()==_game_connection_network_client || !riding_map)
		return;
	/* the first tick only, so riding never changes under a rider */
	if (hs_runtime_port_executing_script_type()!=_hs_script_startup || game_time_get()!=0)
	{
		if (!riding_map_refusal_logged)
		{
			error(_error_silent, "vehicle_riding: a map sets riding only from a startup script, on its first tick; this call does nothing");
			riding_map_refusal_logged = TRUE;
		}
		return;
	}
	*riding_map = on != FALSE;
}

void vehicle_riding_map_start(void)
{
	short map;

	if (game_in_editor() || game_connection()==_game_connection_network_client || !riding_map)
		return;
	map = hs_runtime_port_scenario_boolean("vehicle_riding");
	if (map != NONE)
		*riding_map = map;
}

boolean vehicle_riding_applies(
	boolean has_player,
	boolean dead)
{
	return has_player && !dead && vehicle_riding_enabled();
}

boolean vehicle_riding_deferred(
	struct vehicle_riding_state const *state,
	boolean applies)
{
	/* (every biped in a window, not only those riding: one that left the
	deck last tick and lands back this tick must still update after the
	vehicle, or its carry spans none or two of the vehicle's updates) */
	return applies && state->ticks > 0;
}

void vehicle_riding_clear(
	struct vehicle_riding_state *state)
{
	struct vehicle_riding_state cleared = { 0 };

	*state = cleared;
	state->object_index = NONE;
	state->node_index = NONE;
	vehicle_riding_correction_identity(&state->mark.correction, NONE);
}

void vehicle_riding_drop(
	struct vehicle_riding_state *state)
{
	char falling = state->falling;
	real_vector3d velocity = state->velocity;

	vehicle_riding_clear(state);
	/* (a biped still in the air keeps the frame it fell from: its fall is
	judged against the deck until it lands) */
	if (falling)
	{
		state->falling = falling;
		state->velocity = velocity;
	}
}

boolean vehicle_riding_is_riding(
	struct vehicle_riding_state const *state)
{
	return state->ticks == RIDING_TICKS;
}

void vehicle_riding_point_to_local(
	real_point3d const *origin,
	real_vector3d const *forward,
	real_vector3d const *up,
	real_point3d const *point,
	real_point3d *local)
{
	real_matrix4x3 frame;

	matrix4x3_from_point_and_vectors(&frame, origin, forward, up);
	matrix4x3_inverse_transform_point(&frame, point, local);
}

void vehicle_riding_point_to_world(
	real_point3d const *origin,
	real_vector3d const *forward,
	real_vector3d const *up,
	real_point3d const *local,
	real_point3d *point)
{
	real_matrix4x3 frame;

	matrix4x3_from_point_and_vectors(&frame, origin, forward, up);
	matrix4x3_transform_point(&frame, local, point);
}

real vehicle_riding_yaw(
	real_vector3d const *from_forward,
	real_vector3d const *from_up,
	real_vector3d const *to_forward,
	real_vector3d const *to_up)
{
	static real_point3d const origin = { 0.f, 0.f, 0.f };
	real_matrix4x3 from;
	real_matrix4x3 to;
	real_vector3d rotation;

	matrix4x3_from_point_and_vectors(&from, &origin, from_forward, from_up);
	matrix4x3_from_point_and_vectors(&to, &origin, to_forward, to_up);
	vector_from_matrices4x3(&from, &to, &rotation);
	/* (the rotation's axis times its angle, its k: the vehicle's yaw
	alone, whatever its pitch) */
	return rotation.k;
}

void vehicle_riding_turn_vector(
	real_vector3d *vector,
	real angle)
{
	real cosine = (real)cos(angle);
	real sine = (real)sin(angle);
	real i = vector->i;

	vector->i = i*cosine - vector->j*sine;
	vector->j = i*sine + vector->j*cosine;
}

void vehicle_riding_turn_vectors(
	real_vector3d *const *vectors,
	short count,
	real dyaw)
{
	real cosine = (real)cos(dyaw);
	real sine = (real)sin(dyaw);
	short index;

	for (index = 0; index < count; index++)
	{
		real i = vectors[index]->i;

		vectors[index]->i = i*cosine - vectors[index]->j*sine;
		vectors[index]->j = i*sine + vectors[index]->j*cosine;
	}
}

void vehicle_riding_deck_point_velocity(
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *point,
	boolean double_speed,
	real_vector3d *velocity)
{
	real_vector3d offset;
	real_vector3d turning;

	/* (about the vehicle's own origin: its velocities are the body's, not a node's) */
	vector_from_points3d(&vehicle->origin, point, &offset);
	cross_product3d(&vehicle->angular_velocity, &offset, &turning);
	add_vectors3d(&vehicle->translational_velocity, &turning, velocity);
	scale_vector3d(velocity, 1.f/riding_period(vehicle, double_speed), velocity);
}

/* whether the node's motion is a deck's, and the body's carry if not */
static boolean riding_node_judged(
	struct vehicle_riding_state const *state,
	struct vehicle_riding_vehicle const *vehicle,
	struct vehicle_riding_step *step,
	real_point3d const *last,
	real_point3d *carried,
	real_point3d *local_base,
	real_point3d *moved_base,
	real_point3d const *base,
	real node_speed_limit)
{
	real_point3d body_local;
	real_point3d carried_body;

	vehicle_riding_point_to_local(&state->body_position, &state->body_forward, &state->body_up, last, &body_local);
	vehicle_riding_point_to_world(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, &body_local,
		&carried_body);
	/* (a turret stays put and turns about its own up; a leg's stride does neither) */
	riding_node_motion(state, vehicle, &step->node_shift, &step->node_tilt);
	step->node_shift /= step->player_ticks;
	step->node_tilt /= step->player_ticks;
	if (step->node_shift > RIDING_NODE_SHIFT || step->node_tilt > RIDING_NODE_TILT ||
		distance3d(carried, &carried_body)/step->player_ticks > node_speed_limit)
	{
		*carried = carried_body;
		vehicle_riding_point_to_local(&state->body_position, &state->body_forward, &state->body_up, base,
			local_base);
		vehicle_riding_point_to_world(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, local_base,
			moved_base);
		return TRUE;
	}
	return FALSE;
}

boolean vehicle_riding_snapshot_step(
	struct vehicle_riding_state *state,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real pill_height,
	real pill_width,
	boolean double_speed,
	real node_speed_limit,
	struct vehicle_riding_step *step)
{
	boolean riding = vehicle_riding_is_riding(state);
	real_point3d last;
	real_point3d carried;
	real_point3d local_base;
	real_point3d moved_base;
	real_point3d body_local_base;
	real_point3d body_moved_base;
	real_point3d corrected_last;
	real_point3d corrected_base;
	real_matrix4x3 correction;
	real_vector3d motion;
	real_vector3d path;
	real_vector3d off_path;
	real distance;
	real per_tick;
	long elapsed = 0;
	boolean followed;
	boolean role_changed;
	boolean refused;
	boolean held = FALSE;

	if (!vehicle || vehicle->object_index != state->object_index)
	{
		/* (a rider whose vehicle is gone falls from the frame it rode) */
		if (riding)
			state->falling = 1;
		vehicle_riding_drop(state);
		return FALSE;
	}
	/* (where it ended its last tick, from its place on the deck as the deck was then) */
	vehicle_riding_point_to_world(&state->vehicle_position, &state->vehicle_forward, &state->vehicle_up, &state->local_point, &last);
	/* (moved by something else since its last tick: a script, a cutscene, a teleport) */
	if (riding && distance3d(base, &last) > riding_moved_alone)
	{
		riding_open_window(state, vehicle, base);
		return FALSE;
	}
	/* (as an elevator only moves bipeds in its own sphere, device_machines.c) */
	if (!riding &&
		distance3d(base, &vehicle->bounding_sphere_center) > vehicle->bounding_sphere_radius + pill_height + pill_width)
	{
		vehicle_riding_drop(state);
		return FALSE;
	}

	step->vehicle = *vehicle;
	step->node_rejected = FALSE;
	step->node_shift = step->node_tilt = 0.f;
	step->over_deck = FALSE;
	step->updates = vehicle->updates - state->vehicle_updates;
	step->player_ticks = step->updates*riding_period(vehicle, double_speed);
	role_changed = vehicle->role != state->mark.role;
	followed = vehicle->role == _vehicle_riding_role_followed && !role_changed;
	if (followed)
	{
		/* (the host's word is no update of the vehicle's: only the time since the snapshot counts) */
		elapsed = vehicle->time - state->mark.time;
		step->player_ticks = (real)elapsed;
	}
	/* (the same place on the deck as the deck is now: the carry is the
	deck's motion there, about the ridden node, whatever the base did) */
	vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, &vehicle->up, &state->local_point, &carried);
	vehicle_riding_point_to_local(&state->vehicle_position, &state->vehicle_forward, &state->vehicle_up, base,
		&local_base);
	vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, &vehicle->up, &local_base, &moved_base);
	/* (a node counts as a deck only while it moves with the vehicle's body: an
	animated leg or a spinning wheel is a node too, and standing on its stride
	would fling the rider off it with the leg's speed. The judgement is the
	node's motion of the rider's spot beyond what the body alone would give
	it, so a turret turned the way the vehicle drives is judged on its turn,
	never on the vehicle's speed; the body itself is never judged) */
	if (!vehicle->node_kept && state->node_index != NONE && !role_changed && (followed ? elapsed > 0 : step->updates > 0))
		step->node_rejected = riding_node_judged(state, vehicle, step, &last, &carried, &local_base,
			&moved_base, base, node_speed_limit);
	vector_from_points3d(&last, &carried, &step->displacement);
	if (followed || role_changed)
	{
		/* (no correction is recorded for a vehicle the client follows, and none carries over a change of role) */
		corrected_last = last;
		corrected_base = *base;
	}
	else
	{
		vehicle_riding_correction_since(&vehicle->correction, &state->mark.correction, &correction);
		matrix4x3_transform_point(&correction, &last, &corrected_last);
		matrix4x3_transform_point(&correction, base, &corrected_base);
	}
	vector_from_points3d(&corrected_last, &carried, &motion);
	distance = magnitude3d(&motion);
	if (role_changed)
	{
		/* (unscaled: an unrecorded move across a change of role is a jump as stage 1 sees one) */
		refused = distance > riding_teleport_step;
		held = !refused;
	}
	else if (followed)
	{
		refused = elapsed < 0;
		if (!refused)
		{
			scale_vector3d(&vehicle->translational_velocity, (real)elapsed, &path);
			/* (a move off the vehicle's own path is the host's snap, carried with the velocities held) */
			subtract_vectors3d(&motion, &path, &off_path);
			held = elapsed == 0 || magnitude3d(&off_path) > RIDING_SNAP_STEP;
		}
	}
	else
		/* (netcode writes carry the rider but are not the deck's own motion) */
		refused = step->updates < 0 ||
			(step->updates == 0 && distance > 0.000001f) ||
			(step->updates > 0 && distance > riding_teleport_step*step->updates);
	if (refused)
	{
		if (riding)
			riding_open_window(state, vehicle, base);
		else
			vehicle_riding_drop(state);
		return FALSE;
	}

	if (step->node_rejected)
		step->dyaw = vehicle_riding_yaw(&state->body_forward, &state->body_up, &vehicle->body_forward, &vehicle->body_up);
	else
		step->dyaw = vehicle_riding_yaw(&state->vehicle_forward, &state->vehicle_up, &vehicle->forward, &vehicle->up);
	if (held || (!followed && !role_changed && step->updates == 0))
	{
		/* (double speed's skipped tick, a followed vehicle's no time or its
		snap, a change of role: the vehicle did not update or its move is
		no motion of the deck, and the frame velocities hold. A rider
		leaving a node on such a tick keeps the node's last frame, not the
		body's: accepted, a rare tick's difference) */
		step->velocity_now = state->velocity;
		step->tracked_deck_velocity = state->velocity;
		step->body_deck_velocity = state->velocity;
		return TRUE;
	}

	per_tick = 1.f/step->player_ticks;
	scale_vector3d(&motion, per_tick, &step->velocity_now);
	vector_from_points3d(&corrected_base, &moved_base, &step->tracked_deck_velocity);
	scale_vector3d(&step->tracked_deck_velocity, per_tick, &step->tracked_deck_velocity);
	/* (the body's own motion of the base, whatever node it stands on: the
	frame it keeps when it leaves the floor) */
	vehicle_riding_point_to_local(&state->body_position, &state->body_forward, &state->body_up, base,
		&body_local_base);
	vehicle_riding_point_to_world(&vehicle->origin, &vehicle->body_forward, &vehicle->body_up, &body_local_base,
		&body_moved_base);
	vector_from_points3d(&corrected_base, &body_moved_base, &step->body_deck_velocity);
	scale_vector3d(&step->body_deck_velocity, per_tick, &step->body_deck_velocity);
	return TRUE;
}

void vehicle_riding_window_base(
	struct vehicle_riding_state const *state,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real_point3d *origin)
{
	real_point3d last;
	real_point3d carried;
	real_point3d local_base;
	real_point3d moved_base;
	real_point3d corrected_base;
	real_matrix4x3 correction;
	real_vector3d off_path;
	long elapsed;

	*origin = *base;
	if (!vehicle || vehicle->object_index != state->object_index)
		return;
	/* (where the step holds the frame the vehicle's move is no motion of the
	deck, a host's snap above all: the base stays, as the frame does) */
	if (vehicle->role != state->mark.role)
		return;
	if (vehicle->role == _vehicle_riding_role_followed)
	{
		elapsed = vehicle->time - state->mark.time;
		if (elapsed <= 0)
			return;
		vehicle_riding_point_to_world(&state->vehicle_position, &state->vehicle_forward, &state->vehicle_up,
			&state->local_point, &last);
		vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, &vehicle->up, &state->local_point,
			&carried);
		vector_from_points3d(&last, &carried, &off_path);
		off_path.i -= vehicle->translational_velocity.i*(real)elapsed;
		off_path.j -= vehicle->translational_velocity.j*(real)elapsed;
		off_path.k -= vehicle->translational_velocity.k*(real)elapsed;
		if (magnitude3d(&off_path) > RIDING_SNAP_STEP)
			return;
	}
	/* (the vehicle has made this tick's move and the biped's physics has
	not: from the base itself a ray looks a tick of the deck's travel behind
	the rider's place on it, and misses the deck's last stretch at speed) */
	vehicle_riding_point_to_local(&state->vehicle_position, &state->vehicle_forward, &state->vehicle_up, base,
		&local_base);
	vehicle_riding_point_to_world(&vehicle->position, &vehicle->forward, &vehicle->up, &local_base, &moved_base);
	corrected_base = *base;
	/* (a write moves the vehicle, never a biped in the air: left out as the
	step leaves it out of the deck's motion) */
	if (vehicle->role != _vehicle_riding_role_followed)
	{
		vehicle_riding_correction_since(&vehicle->correction, &state->mark.correction, &correction);
		matrix4x3_transform_point(&correction, base, &corrected_base);
	}
	origin->x = base->x + moved_base.x - corrected_base.x;
	origin->y = base->y + moved_base.y - corrected_base.y;
	origin->z = base->z + moved_base.z - corrected_base.z;
}

void vehicle_riding_seed(
	struct vehicle_riding_state const *state,
	real_vector3d const *world_velocity,
	real_vector3d *velocity,
	real_vector3d *frame_velocity)
{
	frame_velocity->i = frame_velocity->j = frame_velocity->k = 0.f;
	if (vehicle_riding_is_riding(state))
		*frame_velocity = state->velocity;
	subtract_vectors3d(world_velocity, frame_velocity, velocity);
}

void vehicle_riding_support_velocity(
	long support_object_index,
	long riding_object_index,
	real_vector3d const *riding_frame_velocity,
	long tracked_object_index,
	real_vector3d const *tracked_deck_velocity,
	real_vector3d const *other_velocity,
	real_vector3d *support_velocity)
{
	if (support_object_index == NONE)
		support_velocity->i = support_velocity->j = support_velocity->k = 0.f;
	else if (support_object_index == riding_object_index)
		*support_velocity = *riding_frame_velocity;
	else if (support_object_index == tracked_object_index)
		*support_velocity = *tracked_deck_velocity;
	else
		*support_velocity = *other_velocity;
}

real vehicle_riding_landing_velocity(
	real_vector3d const *velocity,
	real_vector3d const *frame_velocity,
	real_vector3d const *support_velocity,
	real_vector3d const *normal)
{
	real_vector3d relative;

	/* (the physics' velocity is in the ridden deck's frame: back to the
	world's, then into the frame of what it lands on) */
	add_vectors3d(velocity, frame_velocity, &relative);
	subtract_vectors3d(&relative, support_velocity, &relative);
	return -dot_product3d(&relative, normal);
}

boolean vehicle_riding_record(
	struct vehicle_riding_state *state,
	struct vehicle_riding_step const *step,
	struct vehicle_riding_vehicle const *support,
	boolean supported,
	real_vector3d const *support_velocity,
	real_vector3d const *support_normal,
	real_point3d const *new_base,
	boolean suspended,
	real_vector3d *new_velocity)
{
	boolean started_riding = vehicle_riding_is_riding(state);
	struct vehicle_riding_vehicle body;

	if (started_riding)
	{
		/* (the deck's motion, taken off at the seed, added back: the world
		velocity. Off the vehicle it is the body's motion at its spot, never a
		leg's or a turret's) */
		real_vector3d const *frame = support && support->object_index == state->object_index ?
			&step->velocity_now : &step->body_deck_velocity;

		if (suspended)
			*new_velocity = *frame;
		else
			add_vectors3d(new_velocity, frame, new_velocity);
		state->velocity = *frame;
		if (supported && (!support || support->object_index != state->object_index))
			riding_clip(new_velocity, support_velocity, support_normal);
	}

	if (support)
	{
		/* (the node the step just rejected is no deck: it anchors to the body) */
		if (step && step->node_rejected && support->object_index == state->object_index &&
			support->node_index == step->vehicle.node_index)
		{
			riding_body(support, &body);
			support = &body;
		}
		if (step && support->object_index == state->object_index)
			state->velocity = started_riding ? step->velocity_now : step->tracked_deck_velocity;
		else
			state->velocity = *support_velocity;
		state->ticks = RIDING_TICKS;
		state->falling = 0;
		state->object_index = support->object_index;
		state->node_index = support->node_index;
		riding_snapshot(state, support);
		vehicle_riding_point_to_local(&support->position, &support->forward, &support->up, new_base,
			&state->local_point);
		return TRUE;
	}

	/* (falling from the deck it rode, or from something it stood on over the
	deck of a held window) */
	if (supported)
		state->falling = 0;
	else if (started_riding || (step && step->over_deck))
		state->falling = 1;
	if (step)
	{
		/* (its place on the deck is kept and the snapshot follows the deck:
		a landing back then has the deck's motion of that tick only. Leaving
		the floor, the place and the snapshot are the body's) */
		if (started_riding)
			riding_anchor_body(state, &step->vehicle, new_base);
		else
			riding_snapshot(state, &step->vehicle);
		/* (counted down the tick it leaves the floor, so that it rides no more,
		and on the ground off the deck; held in the air, over the deck or not,
		and on anything over the deck, so the vehicle it left stays the vehicle
		left for a fall or a stand of any length. A biped far from the vehicle
		leaves the window at the step's sphere instead) */
		if ((started_riding || (supported && !step->over_deck)) && --state->ticks <= 0)
			vehicle_riding_drop(state);
	}

	return FALSE;
}

real vehicle_riding_falling_velocity(
	struct vehicle_riding_state const *state,
	real world_velocity_k)
{
	real relative;

	if (vehicle_riding_is_riding(state) || state->falling)
	{
		relative = world_velocity_k - state->velocity.k;
		/* (the more lenient of the two: a descending deck is judged
		relative to it, a rising one never makes a fall deadlier than in
		the original game) */
		return relative > world_velocity_k ? relative : world_velocity_k;
	}
	return world_velocity_k;
}

void vehicle_riding_jump_frame(
	struct vehicle_riding_state const *state,
	real_vector3d *jump_frame)
{
	jump_frame->i = jump_frame->j = jump_frame->k = 0.f;
	if (vehicle_riding_is_riding(state))
	{
		*jump_frame = state->velocity;
	}
}

real vehicle_riding_reach(
	real jump_velocity,
	real gravity,
	real height)
{
	/* (no gravity, no jump height to speak of: the pill height alone) */
	if (!(gravity > 0.f))
		return height;
	return jump_velocity*jump_velocity/(2.f*gravity) + height;
}

boolean vehicle_riding_over_deck(
	boolean hit,
	real distance,
	real reach,
	real_vector3d const *body_up)
{
	/* (a rolling or flipping vehicle is no deck to come down on) */
	return hit && distance <= reach && body_up->k >= RIDING_UPRIGHT_K;
}

void vehicle_riding_turn_relative(
	struct vehicle_riding_state const *state,
	struct vehicle_riding_step const *step,
	real_vector3d *velocity)
{
	/* (every carried tick, whoever the rider: a run on a turning deck keeps
	its course on it, and a running jump leaves the way it ran) */
	if (step && vehicle_riding_is_riding(state))
		vehicle_riding_turn_vector(velocity, step->dyaw);
}

void vehicle_riding_air_frame(
	struct vehicle_riding_state *state,
	struct vehicle_riding_step const *step,
	struct vehicle_riding_air *air,
	boolean airborne,
	real_vector3d *velocity,
	real_vector3d *air_frame)
{
	real_vector3d change;
	real_vector3d way;
	real_vector3d slowed;
	real_vector3d beyond;
	real limit;
	real length;
	real speed;
	real peak;
	real along;
	real back;

	air_frame->i = air_frame->j = air_frame->k = 0.f;
	if (!air || !step || vehicle_riding_is_riding(state) || !step->over_deck)
		return;
	/* (the fall is judged against the deck as it is now) */
	state->velocity.k = step->tracked_deck_velocity.k;
	/* (standing on something inside the vehicle: the ground's physics is the world's) */
	if (!airborne)
		return;
	change.i = step->tracked_deck_velocity.i - state->velocity.i;
	change.j = step->tracked_deck_velocity.j - state->velocity.j;
	change.k = 0.f;
	slowed.i = slowed.j = slowed.k = 0.f;
	/* (the way the frame goes; a frame almost at rest has none of its own and
	goes the deck's, and with both almost at rest the change is limited
	whole) */
	way.i = state->velocity.i;
	way.j = state->velocity.j;
	way.k = 0.f;
	speed = normalize3d(&way);
	if (!(speed >= RIDING_AIR_STILL))
	{
		way.i = step->tracked_deck_velocity.i;
		way.j = step->tracked_deck_velocity.j;
		way.k = 0.f;
		if (!(normalize3d(&way) >= RIDING_AIR_STILL))
			way.i = way.j = 0.f;
	}
	peak = air->have_peak ? air->peak : speed;
	along = dot_product3d(&change, &way);
	if (along < 0.f)
	{
		/* (a deck slower along the way the biped goes, a brake, a stop, a
		reversal or a bounce: followed at once, so the frame never carries the
		biped ahead of the vehicle) */
		scale_vector3d(&way, along, &slowed);
		subtract_vectors3d(&change, &slowed, &change);
	}
	else if (air->enclosure != _vehicle_riding_enclosure_enclosed)
	{
		/* (a faster one on an open deck: followed only back up to the most the
		frame went since the take-off, so a deck's jitter does not ratchet it
		down; beyond that the vehicle pulls away and leaves the biped behind) */
		back = peak - dot_product3d(&state->velocity, &way);
		if (back < 0.f)
			back = 0.f;
		if (back < along)
		{
			scale_vector3d(&way, along - back, &beyond);
			subtract_vectors3d(&change, &beyond, &change);
		}
	}
	/* (the turn, and the speed-up followed, up to the grip a player tick over
	the ticks the vehicle's update spans: a small vehicle's turns and bumps
	stay under it, a large one's start goes over it. Shortened along the
	change, never axis by axis, so a diagonal change keeps its direction) */
	limit = riding_air_grip*step->player_ticks;
	length = magnitude3d(&change);
	if (length > limit)
	{
		scale_vector3d(&change, limit/length, &change);
	}
	add_vectors3d(&change, &slowed, &change);
	state->velocity.i += change.i;
	state->velocity.j += change.j;
	/* (the frame moves, not the biped's own motion relative to it) */
	velocity->i += change.i;
	velocity->j += change.j;
	air_frame->i = state->velocity.i;
	air_frame->j = state->velocity.j;
	way.i = state->velocity.i;
	way.j = state->velocity.j;
	way.k = 0.f;
	speed = magnitude3d(&way);
	/* (post-echo speed is kept, but cannot raise the host's ceiling for this jump) */
	if (!air->host_peak)
		air->peak = speed > peak ? speed : peak;
	air->have_peak = TRUE;
}

void vehicle_riding_air_correct(
	struct vehicle_riding_state *state,
	struct vehicle_riding_air *air,
	real_vector3d const *difference,
	real peak,
	short verdict,
	real_vector3d *world_velocity)
{
	add_vectors3d(&state->velocity, difference, &state->velocity);
	add_vectors3d(world_velocity, difference, world_velocity);
	if (air)
	{
		air->have_peak = TRUE;
		air->peak = peak;
		air->host_peak = TRUE;
		air->enclosure = verdict;
	}
}

void vehicle_riding_air_take(
	struct vehicle_riding_air *air,
	struct vehicle_riding_deck const *deck)
{
	if (!air)
		return;
	if (deck->ticks == RIDING_TICKS)
	{
		riding_air_clear(air);
		return;
	}
	air->have_peak = TRUE;
	air->peak = deck->peak;
	air->enclosure = (short)((deck->flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) & VEHICLE_RIDING_DECK_VERDICT_MASK);
	air->host_verdict = TRUE;
}

boolean vehicle_riding_air_take_verdict(
	struct vehicle_riding_air *air,
	struct vehicle_riding_deck const *deck,
	boolean current_episode)
{
	if (!air || !deck || !current_episode || deck->ticks <= 0 || deck->ticks >= RIDING_TICKS ||
		TEST_FLAG(deck->flags, _vehicle_riding_deck_cabin_bit))
		return FALSE;
	/* (unknown is the host's word too, never a request for a local sample) */
	air->enclosure = (short)((deck->flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) & VEHICLE_RIDING_DECK_VERDICT_MASK);
	air->host_verdict = TRUE;
	return TRUE;
}

struct vehicle_riding_air *vehicle_riding_player_air(
	short player_index)
{
	if (player_index < 0 || player_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
		return NULL;
	return &riding_airs[player_index];
}

boolean vehicle_riding_air_begin(
	struct vehicle_riding_air *air,
	struct vehicle_riding_state const *state,
	struct vehicle_riding_step const *step,
	boolean airborne)
{
	if (!air)
		return FALSE;
	/* (a window starts at a take-off or a drop: its peak and its verdict are
	that jump's) */
	if (!step || vehicle_riding_is_riding(state))
	{
		riding_air_clear(air);
		return FALSE;
	}
	/* (the host owns this episode's verdict; an airborne copy can drift past its own deck test) */
	if (air->host_verdict && airborne)
		return FALSE;
	/* (a local verdict holds until it lands or leaves the deck, so a biped
	bobbing under the edge of a roof does not flicker between the two) */
	if (!step->over_deck || !airborne)
	{
		/* (standing on something inside the vehicle: its next jump takes off
		at its own speed, not the last one's) */
		if (!airborne)
		{
			air->have_peak = FALSE;
			air->host_peak = FALSE;
			air->host_verdict = FALSE;
		}
		air->enclosure = _vehicle_riding_enclosure_unknown;
		return FALSE;
	}
	return air->enclosure == _vehicle_riding_enclosure_unknown;
}

void vehicle_riding_air_enclosure(
	struct vehicle_riding_air *air,
	boolean hit,
	real distance,
	real jump_height,
	boolean inside)
{
	/* (a roof within a jump encloses; a gun or a turret high over an open
	deck does not) */
	air->enclosure = inside || (hit && distance <= jump_height) ?
		_vehicle_riding_enclosure_enclosed : _vehicle_riding_enclosure_open;
}

void vehicle_riding_deck_frame(
	struct vehicle_riding_state const *state,
	struct vehicle_riding_vehicle const *vehicle,
	struct vehicle_riding_vehicle *frame)
{
	*frame = *vehicle;
	/* (a write to the vehicle since the rider's snapshot moved its node, but the rider's next carry keeps it where the
	snapshot's node had it; in the air the place is the take-off's, in the live pose) */
	if (vehicle_riding_is_riding(state) && state->object_index == vehicle->object_index &&
		state->node_index == vehicle->node_index)
	{
		frame->position = state->vehicle_position;
		frame->forward = state->vehicle_forward;
		frame->up = state->vehicle_up;
	}
}

boolean vehicle_riding_deck_make(
	struct vehicle_riding_state const *state,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real_vector3d const *world_velocity,
	struct vehicle_riding_air const *air,
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_deck *deck)
{
	struct vehicle_riding_deck made;
	real_vector3d way;

	if (!vehicle || vehicle->object_index != state->object_index || vehicle->node_index != state->node_index)
		return FALSE;
	made.vehicle_index = state->object_index;
	made.node_index = state->node_index;
	made.ticks = state->ticks;
	made.flags = state->falling ? FLAG(_vehicle_riding_deck_falling_bit) : 0;
	/* (the verdict the rider goes by, riding as well as in the air: the
	standing cabin's on the deck, a protected jump's raw sample, else the
	air verdict, a cabin of another vehicle or none included; a rider on the
	deck without a standing verdict sends unknown only because its air is
	cleared every riding tick, vehicle_riding_air_begin) */
	made.flags |= (byte)(vehicle_riding_cabin_verdict(cabin, state, air, FALSE) << VEHICLE_RIDING_DECK_VERDICT_SHIFT);
	/* (in the window in the air a verdict that came from the cabin keyed to
	this vehicle, a protected jump's sample or the host's word for it, says
	so: the legacy air verdict beside it is never cabin evidence, and a
	rider on the deck sends none, its verdict being the standing one's) */
	if (state->ticks < RIDING_TICKS && cabin && !cabin->host_legacy && cabin->vehicle_index == state->object_index &&
		((cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_jump) ||
		cabin->jump != _vehicle_riding_enclosure_unknown))
		made.flags |= FLAG(_vehicle_riding_deck_cabin_bit);
	/* (where the pill is now, in the node's frame as it is now: in the air
	the place it is kept at is the take-off's) */
	vehicle_riding_point_to_local(&vehicle->position, &vehicle->forward, &vehicle->up, base, &made.place);
	made.peak = 0.f;
	if (vehicle_riding_is_riding(state))
	{
		/* (relative to the deck: the frame velocity is each machine's own,
		from its snapshot step) */
		subtract_vectors3d(world_velocity, &state->velocity, &made.velocity);
	}
	else
	{
		made.velocity = state->velocity;
		/* (no peak yet: the frame's own speed across, as the air frame takes it) */
		way.i = state->velocity.i;
		way.j = state->velocity.j;
		way.k = 0.f;
		made.peak = magnitude3d(&way);
		if (air && air->have_peak)
		{
			made.peak = air->peak;
		}
	}
	if (!vehicle_riding_deck_values_valid(&made))
		return FALSE;
	*deck = made;
	return TRUE;
}

/* the host's own standing window decides which frame may take a word */
short vehicle_riding_host_path(
	struct vehicle_riding_state const *host,
	boolean deck_present,
	struct vehicle_riding_deck const *deck,
	boolean transition)
{
	if (vehicle_riding_is_riding(host) && deck_present && deck->ticks == RIDING_TICKS &&
		deck->vehicle_index == host->object_index)
	{
		return _vehicle_riding_host_deck;
	}
	if (vehicle_riding_is_riding(host) && !transition)
		return _vehicle_riding_host_along;
	return _vehicle_riding_host_world;
}

real vehicle_riding_transition_leash(
	void)
{
	return riding_transition_leash;
}

boolean vehicle_riding_host_leash(
	real_point3d const *client_base,
	real_point3d const *host_base,
	real leash)
{
	return valid_real(leash) && leash >= 0.f && distance_squared3d(client_base, host_base) <= leash * leash;
}

/* whether the host copy's change of window is one the client's word makes as well, in which case the host's leash (which puts the copy
where the client's world word is) has nothing to ease */
boolean vehicle_riding_transition_agreed(
	short host_before,
	long host_vehicle_before,
	short host_now,
	long host_vehicle,
	short client_ticks,
	long client_vehicle,
	boolean client_part,
	boolean word_taken)
{
	/* (boarding where the client rides the same deck. From another vehicle's window it is a vehicle change) */
	if (host_now == RIDING_TICKS && host_before != RIDING_TICKS &&
		client_ticks == RIDING_TICKS && client_vehicle == host_vehicle &&
		(host_before <= 0 || host_vehicle_before == host_vehicle))
		return TRUE;
	/* (boarding by the physics that follows a world-only word the host took within the transition length) */
	if (host_now == RIDING_TICKS && host_before <= 0 && !client_part && word_taken)
		return TRUE;
	/* (stepping off the same vehicle's deck together) */
	if (host_before == RIDING_TICKS && host_now > 0 && host_now < RIDING_TICKS && host_vehicle == host_vehicle_before &&
		client_ticks > 0 && client_ticks < RIDING_TICKS && client_vehicle == host_vehicle)
		return TRUE;
	return FALSE;
}

/* whether the client's word rides the deck the host copy is in the air over, however the word came to be there: the copy's take puts it
on its own deck and its fall lands it, where a leash would hold it at the word's world place, off its deck by the two copies of the
vehicle's offset */
boolean vehicle_riding_word_on_air_deck(
	short host_ticks,
	long host_vehicle,
	short client_ticks,
	long client_vehicle)
{
	return host_ticks > 0 && host_ticks < RIDING_TICKS && client_ticks == RIDING_TICKS && client_vehicle == host_vehicle;
}

void vehicle_riding_host_target(
	short path,
	struct vehicle_riding_vehicle const *client_node,
	struct vehicle_riding_vehicle const *host_node,
	real_point3d const *host_base,
	struct vehicle_riding_deck const *deck,
	real_point3d const *client_base,
	real_point3d *base)
{
	real_point3d place, host_place;

	*base = *client_base;
	if (path == _vehicle_riding_host_along)
	{
		/* (a part of this vehicle says where on it: the word's world place is where the client's copy of the vehicle
		stood, off by the two copies' offset) */
		if (client_node && deck->vehicle_index == host_node->object_index && client_node->node_index == host_node->node_index)
			place = deck->place;
		else
			vehicle_riding_point_to_local(&host_node->position, &host_node->forward, &host_node->up, client_base, &place);
		vehicle_riding_point_to_local(&host_node->position, &host_node->forward, &host_node->up, host_base, &host_place);
		if (place.z < host_place.z)
			place.z = host_place.z;
		vehicle_riding_point_to_world(&host_node->position, &host_node->forward, &host_node->up, &place, base);
	}
	else if (client_node && deck->ticks == RIDING_TICKS)
	{
		/* (a claimed deck place is input even when only the client rides it) */
		vehicle_riding_point_to_world(&client_node->position, &client_node->forward, &client_node->up, &deck->place, base);
	}
}

/* a word that names no riding part never lifts the copy above its place in the vehicle's frame, whatever it says across or
below */
void vehicle_riding_host_unlifted(
	struct vehicle_riding_vehicle const *host_node,
	real_point3d const *host_base,
	real_point3d *base)
{
	real_point3d place, host_place, clamped;

	vehicle_riding_point_to_local(&host_node->position, &host_node->forward, &host_node->up, base, &place);
	vehicle_riding_point_to_local(&host_node->position, &host_node->forward, &host_node->up, host_base, &host_place);
	if (place.z > host_place.z)
	{
		place.z = host_place.z;
		vehicle_riding_point_to_world(&host_node->position, &host_node->forward, &host_node->up, &place, &clamped);
		*base = clamped;
	}
}

/* clamp the player's motion before adding the host's carry */
void vehicle_riding_host_velocity(
	real_vector3d const *relative,
	real_vector3d const *frame_velocity,
	real speed,
	real fall_speed,
	real_vector3d *world)
{
	real_vector3d clamped = *relative;

	if (speed > 0.0f)
	{
		real rise = clamped.k > 0.0f ? clamped.k : 0.0f;
		real magnitude = (real)sqrt(clamped.i * clamped.i + clamped.j * clamped.j + rise * rise);

		if (magnitude > speed)
		{
			clamped.i *= speed / magnitude;
			clamped.j *= speed / magnitude;
			if (clamped.k > 0.0f)
				clamped.k *= speed / magnitude;
		}
		if (fall_speed > 0.0f && clamped.k < -fall_speed)
			clamped.k = -fall_speed;
	}
	add_vectors3d(&clamped, frame_velocity, world);
}

boolean vehicle_riding_deck_vehicle_named(
	long vehicle_index)
{
	return vehicle_index != NONE && ((unsigned long)vehicle_index >> 16) != 0;
}

boolean vehicle_riding_deck_values_valid(
	struct vehicle_riding_deck const *deck)
{
	short verdict = (short)((deck->flags >> VEHICLE_RIDING_DECK_VERDICT_SHIFT) & VEHICLE_RIDING_DECK_VERDICT_MASK);

	return vehicle_riding_deck_vehicle_named(deck->vehicle_index) &&
		deck->ticks >= 1 && deck->ticks <= RIDING_TICKS &&
		/* (a verdict of 3 is none; the cabin bit is the air's, with a verdict beside it; no bit above it) */
		verdict <= _vehicle_riding_enclosure_enclosed &&
		(!TEST_FLAG(deck->flags, _vehicle_riding_deck_cabin_bit) || deck->ticks != RIDING_TICKS) &&
		(!TEST_FLAG(deck->flags, _vehicle_riding_deck_cabin_bit) || verdict != _vehicle_riding_enclosure_unknown) &&
		(deck->flags >> (_vehicle_riding_deck_cabin_bit + 1)) == 0 &&
		valid_real_point3d(&deck->place) && valid_real_vector3d(&deck->velocity) &&
		valid_real(deck->peak) && deck->peak >= 0.f;
}

boolean vehicle_riding_deck_valid(
	struct vehicle_riding_deck const *deck,
	short node_count)
{
	return node_count != NONE && vehicle_riding_deck_values_valid(deck) &&
		(deck->node_index == NONE || (deck->node_index >= 0 && deck->node_index < node_count));
}

short vehicle_riding_collision_window(
	struct vehicle_riding_state const *state,
	boolean applies,
	long vehicle_index)
{
	if (!applies || state->ticks <= 0)
		return _vehicle_riding_collision_original;
	if (state->object_index != vehicle_index)
	{
		/* (a vehicle beside the one it rides, in a convoy: judged on the
		closing speed, never as a pedestrian) */
		return vehicle_riding_is_riding(state) ? _vehicle_riding_collision_judge : _vehicle_riding_collision_original;
	}
	if (vehicle_riding_is_riding(state))
		return _vehicle_riding_collision_ignore;
	return _vehicle_riding_collision_judge;
}

short vehicle_riding_collision_contact(
	struct vehicle_riding_state const *state,
	long vehicle_index,
	boolean over_deck,
	real_point3d const *vehicle_center,
	real_point3d const *biped_center,
	real_vector3d const *contact_normal,
	real_vector3d const *deck_velocity,
	real damage_speed,
	real_vector3d const *biped_velocity,
	real_vector3d *normal)
{
	real_vector3d direction;
	real_vector3d across;
	real closing;
	real normal_closing;

	/* (the way out of the vehicle where it touches: the contact plane's
	normal, else from the vehicle's centre to the biped's, for every
	vehicle) */
	vector_from_points3d(vehicle_center, biped_center, &direction);
	normalize3d(&direction);
	*normal = contact_normal ? *contact_normal : direction;
	/* (the vehicle under a biped in its window never hits it: its walls and
	floor move it along in its own tick) */
	if (!vehicle_riding_is_riding(state) && state->object_index == vehicle_index && over_deck)
		return _vehicle_riding_collision_resolved;
	/* (closing along the original push, from the vehicle's centre to the
	biped's, and along the touch's normal: the vehicle's motion in 3d, so one
	dropping onto a biped or a deck rising into one closes on it, but the
	biped's own only across, so its own fall never counts) */
	across.i = direction.i;
	across.j = direction.j;
	across.k = 0.f;
	closing = dot_product3d(deck_velocity, &direction) - dot_product3d(biped_velocity, &across);
	across.i = normal->i;
	across.j = normal->j;
	normal_closing = dot_product3d(deck_velocity, normal) - dot_product3d(biped_velocity, &across);
	if (closing > damage_speed || normal_closing > damage_speed)
		return _vehicle_riding_collision_original;
	return _vehicle_riding_collision_resolved;
}

boolean vehicle_riding_contact_record(
	short player_index,
	long vehicle_index,
	real_vector3d const *normal)
{
	short index;

	if (player_index < 0 || player_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
		return FALSE;
	/* (one record a vehicle: its second touch in the same update is the later one) */
	for (index = 0; index < riding_contact_counts[player_index]; index++)
	{
		if (riding_contacts[player_index][index].vehicle_index == vehicle_index)
		{
			riding_contacts[player_index][index].normal = *normal;
			return TRUE;
		}
	}
	/* (a further vehicle replaces nothing: it waits for its next touch) */
	if (riding_contact_counts[player_index] >= RIDING_CONTACT_RECORDS)
		return FALSE;
	index = riding_contact_counts[player_index]++;
	riding_contacts[player_index][index].vehicle_index = vehicle_index;
	riding_contacts[player_index][index].normal = *normal;
	riding_contacts[player_index][index].deferred = FALSE;
	return TRUE;
}

boolean vehicle_riding_contact_defer(
	short player_index,
	long vehicle_index)
{
	short index;

	if (HALO_PORT_MAXIMUM_NETWORK_PLAYERS <= (unsigned short)player_index)
		return FALSE;
	index = 0;
	while (index < riding_contact_counts[player_index] && riding_contacts[player_index][index].vehicle_index != vehicle_index)
		index++;
	if (index >= riding_contact_counts[player_index])
		return FALSE;
	riding_contacts[player_index][index].deferred = TRUE;
	return TRUE;
}

short vehicle_riding_contact_take(
	short player_index,
	struct vehicle_riding_contact *contacts)
{
	short count;
	short index;

	if (player_index < 0 || player_index >= HALO_PORT_MAXIMUM_NETWORK_PLAYERS)
		return 0;
	count = riding_contact_counts[player_index];
	for (index = 0; index < count; index++)
	{
		contacts[index] = riding_contacts[player_index][index];
	}
	riding_contact_counts[player_index] = 0;
	return count;
}

void vehicle_riding_contacts_clear(
	void)
{
	short player;

	for (player = 0; player < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; player++)
	{
		riding_contact_counts[player] = 0;
	}
}

real vehicle_riding_contact_move(
	struct vehicle_riding_step const *step,
	boolean carried,
	struct vehicle_riding_vehicle const *vehicle,
	real_point3d const *base,
	real_vector3d const *biped_velocity,
	real_vector3d const *normal,
	boolean double_speed,
	real_vector3d *move,
	real_vector3d *ahead)
{
	real_vector3d motion;
	real_vector3d own;
	real ticks;
	real into;

	/* (the vehicle's motion at the base over its update: for the vehicle the
	step tracks, its pose's, as the air frame reads it; for another, its
	velocities over the ticks its own update spans) */
	if (step && vehicle->object_index == step->vehicle.object_index)
	{
		ticks = step->player_ticks;
		scale_vector3d(&step->body_deck_velocity, ticks, &motion);
	}
	else
	{
		ticks = riding_period(vehicle, double_speed);
		vehicle_riding_deck_point_velocity(vehicle, base, double_speed, &motion);
		scale_vector3d(&motion, ticks, &motion);
	}
	/* (the biped's own motion over those ticks, so a biped moving with the
	vehicle is not moved again: a rider's carry, already made; else its
	velocity, which its physics makes after this, without its fall, so a
	deck rising into a falling biped still lifts it) */
	ahead->i = ahead->j = ahead->k = 0.f;
	if (step && carried)
	{
		own = step->displacement;
	}
	else
	{
		own.i = biped_velocity->i*ticks;
		own.j = biped_velocity->j*ticks;
		own.k = 0.f;
		*ahead = own;
	}
	subtract_vectors3d(&motion, &own, &motion);
	into = dot_product3d(&motion, normal);
	/* (a vehicle moving away, or slower than the biped, never pulls it) */
	if (into < 0.f)
		into = 0.f;
	scale_vector3d(normal, into, move);
	return into;
}

short vehicle_riding_contact_settle(
	boolean touching,
	boolean candidate_touching,
	boolean fixed)
{
	/* (no overlap of that vehicle left: with the relative move, an overlap
	still there is a real one, and only then does anything fix it) */
	if (!touching)
		return _vehicle_riding_settle_moved;
	/* (out along the touch first, before the fix's fixed world offsets) */
	if (!candidate_touching)
		return _vehicle_riding_settle_candidate;
	return fixed ? _vehicle_riding_settle_fixed : _vehicle_riding_settle_crushed;
}

boolean vehicle_riding_write_carries(
	struct vehicle_riding_state const *state,
	long vehicle_index,
	boolean touched_before,
	boolean touching,
	real distance)
{
	boolean in_window = state->ticks > 0 && state->ticks < RIDING_TICKS && state->object_index == vehicle_index;

	/* (a riding rider's own step takes every write, and a touch from before it is
	the original's to resolve, as is any move further than a continuous one) */
	return (state->ticks <= 0 || in_window) && !touched_before && touching && distance <= riding_moved_alone;
}

real vehicle_riding_run_jump_speed(
	boolean have_information,
	real run_forward,
	real run_backward,
	real run_sideways,
	boolean double_speed,
	real double_speed_multiplier,
	real player_speed_multiplier,
	real jump_velocity)
{
	real run = 0.f;
	real speed;

	/* (as bipeds.c moves a player: forward or back and sideways at once) */
	if (have_information)
	{
		real forward = run_forward > run_backward ? run_forward : run_backward;

		run = (real)sqrt(forward*forward + run_sideways*run_sideways)/TICKS_PER_SECOND;
		if (double_speed && double_speed_multiplier > 1.f)
			run *= double_speed_multiplier;
	}
	if (player_speed_multiplier > 1.f)
		run *= player_speed_multiplier;
	speed = (real)sqrt(run*run + jump_velocity*jump_velocity);
	/* (so written that a tag's speed not a number is none) */
	if (!(speed >= 0.f))
		speed = 0.f;

	return speed;
}

real vehicle_riding_damage_speed(
	boolean have_information,
	real run_forward,
	real run_backward,
	real run_sideways,
	boolean double_speed,
	real double_speed_multiplier,
	real player_speed_multiplier,
	real jump_velocity)
{
	if (riding_damage_speed != 0.f)
		return riding_damage_speed;
	return 2.f * vehicle_riding_run_jump_speed(have_information, run_forward, run_backward, run_sideways,
		double_speed, double_speed_multiplier, player_speed_multiplier, jump_velocity);
}

real vehicle_riding_look(
	short local_player_index,
	boolean carried,
	real dyaw)
{
	real_euler_angles2d impulse;

	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS)
		return 0.f;
	if (!carried || !riding_look_carry)
		dyaw = 0.f;
	if (dyaw != 0.f)
	{
		impulse.yaw = dyaw;
		impulse.pitch = 0.f;
		player_control_permanent_impulse(local_player_index, &impulse);
	}
	riding_kept_dyaws[local_player_index] = dyaw;

	return dyaw;
}

real vehicle_riding_kept_dyaw(
	short local_player_index)
{
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS)
		return 0.f;
	return riding_kept_dyaws[local_player_index];
}

short vehicle_riding_claim(
	long riding_object_index,
	short count,
	long const *objects,
	real const *normal_ks,
	real minimum_normal_k,
	short best)
{
	short index;

	if (riding_object_index == NONE || best < 0 || objects[best] == riding_object_index)
		return best;
	/* (two decks side by side: the one it rides keeps it, so the carry does
	not swap between them from tick to tick) */
	for (index = 0; index < count; index++)
	{
		if (objects[index] == riding_object_index && normal_ks[index] >= minimum_normal_k)
			return index;
	}
	return best;
}

struct vehicle_riding_cabin *vehicle_riding_player_cabin(
	short player_index)
{
	if (!(player_index >= 0 && player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return NULL;
	return &riding_cabins[player_index];
}

void vehicle_riding_cabin_clear(
	struct vehicle_riding_cabin *cabin)
{
	struct vehicle_riding_cabin cleared = { 0 };

	*cabin = cleared;
	cabin->unit_index = NONE;
	cabin->vehicle_index = NONE;
}

boolean vehicle_riding_real_same(
	real a,
	real b)
{
	union { real value; unsigned int bits; } left, right;

	left.value = a;
	right.value = b;
	return left.bits == right.bits;
}

/* the tables as a tick of a biped riding cannot apply to leaves them (vehicle_riding_player_idle) */
static boolean riding_mark_cleared(
	struct vehicle_riding_mark const *mark)
{
	struct vehicle_riding_state state;
	struct vehicle_riding_mark const *cleared = &state.mark;

	vehicle_riding_clear(&state);
	return mark->role == cleared->role && mark->time == cleared->time &&
		mark->correction.vehicle_index == cleared->correction.vehicle_index &&
		vehicle_riding_real_same(mark->correction.rotation.v.i, cleared->correction.rotation.v.i) &&
		vehicle_riding_real_same(mark->correction.rotation.v.j, cleared->correction.rotation.v.j) &&
		vehicle_riding_real_same(mark->correction.rotation.v.k, cleared->correction.rotation.v.k) &&
		vehicle_riding_real_same(mark->correction.rotation.w, cleared->correction.rotation.w) &&
		vehicle_riding_real_same(mark->correction.translation.i, cleared->correction.translation.i) &&
		vehicle_riding_real_same(mark->correction.translation.j, cleared->correction.translation.j) &&
		vehicle_riding_real_same(mark->correction.translation.k, cleared->correction.translation.k);
}

static boolean riding_air_cleared(
	struct vehicle_riding_air const *air)
{
	struct vehicle_riding_air cleared;

	riding_air_clear(&cleared);
	return air->have_peak == cleared.have_peak && air->host_peak == cleared.host_peak &&
		air->host_verdict == cleared.host_verdict && vehicle_riding_real_same(air->peak, cleared.peak) && air->enclosure == cleared.enclosure;
}

/* whether a cabin holds nothing, field by field (a struct's padding is not defined) */
static boolean riding_cabin_cleared(
	struct vehicle_riding_cabin const *cabin)
{
	struct vehicle_riding_cabin cleared;

	vehicle_riding_cabin_clear(&cleared);
	return cabin->unit_index == cleared.unit_index && cabin->vehicle_index == cleared.vehicle_index &&
		cabin->standing == cleared.standing && cabin->misses == cleared.misses && cabin->jump == cleared.jump &&
		cabin->host == cleared.host && cabin->host_legacy == cleared.host_legacy &&
		cabin->host_verdict == cleared.host_verdict && cabin->host_phase == cleared.host_phase &&
		cabin->body_axis == cleared.body_axis && cabin->take_off == cleared.take_off &&
		cabin->have_anchor == cleared.have_anchor && vehicle_riding_real_same(cabin->anchor.x, cleared.anchor.x) &&
		vehicle_riding_real_same(cabin->anchor.y, cleared.anchor.y) && vehicle_riding_real_same(cabin->anchor.z, cleared.anchor.z) &&
		cabin->exception == cleared.exception &&
		cabin->failure_logged == cleared.failure_logged && cabin->failure_log_tick == cleared.failure_log_tick &&
		cabin->failure_seen_tick == cleared.failure_seen_tick;
}

boolean vehicle_riding_player_idle(
	short player_index)
{
	if (!(player_index >= 0 && player_index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return TRUE;
	return riding_mark_cleared(&riding_marks[player_index]) &&
		riding_contact_counts[player_index] == 0 &&
		riding_air_cleared(&riding_airs[player_index]) &&
		riding_cabin_cleared(&riding_cabins[player_index]);
}

void vehicle_riding_cabin_begin(
	struct vehicle_riding_cabin *cabin,
	long unit_index,
	struct vehicle_riding_state const *state,
	boolean valid_window)
{
	boolean window = valid_window && state->ticks > 0;

	/* (one unit's, in one unbroken window of one vehicle: a window lost, a
	new body or another vehicle starts it again, so no verdict, grace or
	anchor outlives the guards) */
	if (!window || cabin->unit_index != unit_index || cabin->vehicle_index != state->object_index)
	{
		vehicle_riding_cabin_clear(cabin);
		if (window)
		{
			cabin->unit_index = unit_index;
			cabin->vehicle_index = state->object_index;
		}
	}
	cabin->exception = FALSE;
}

boolean vehicle_riding_cabin_acquire(
	struct vehicle_riding_cabin *cabin,
	long unit_index,
	struct vehicle_riding_state const *state)
{
	if (!cabin || !vehicle_riding_is_riding(state))
		return FALSE;
	if (cabin->unit_index != unit_index || cabin->vehicle_index != state->object_index)
	{
		vehicle_riding_cabin_clear(cabin);
		cabin->unit_index = unit_index;
		cabin->vehicle_index = state->object_index;
	}
	/* (a first claim samples once where it ended, for its next tick: the
	discovery never waits on the protection it decides) */
	return cabin->standing == _vehicle_riding_enclosure_unknown;
}

void vehicle_riding_cabin_take_off(
	struct vehicle_riding_cabin *cabin,
	struct vehicle_riding_state const *state,
	long unit_index)
{
	if (cabin && vehicle_riding_is_riding(state) && cabin->unit_index == unit_index &&
		cabin->vehicle_index == state->object_index)
	{
		cabin->take_off = TRUE;
		/* (the host's verdict ends with the phase it was for: a jump's starts unknown, until the host's next part) */
		cabin->host = FALSE;
	}
}

short vehicle_riding_cabin_grace(
	void)
{
	return riding_cabin_grace;
}

real_vector3d const *vehicle_riding_cabin_up(
	struct vehicle_riding_step const *step)
{
	/* (the body's: a turret's tilt, or a rider sliding from floor to wall,
	never changes the measure) */
	return &step->vehicle.body_up;
}

real vehicle_riding_cabin_cosine(
	real_vector3d const *body_up)
{
	real length = magnitude3d(body_up);

	return length > 0.f ? body_up->k/length : 1.f;
}

boolean vehicle_riding_cabin_tilted(
	real_vector3d const *body_up,
	real minimum_normal_k)
{
	/* (past the biped's own surface limit, or the window ray's 45 degrees
	where that comes first: both gates protected at once) */
	real limit = minimum_normal_k > RIDING_UPRIGHT_K ? minimum_normal_k : RIDING_UPRIGHT_K;

	return vehicle_riding_cabin_cosine(body_up) < limit;
}

void vehicle_riding_cabin_axis(
	struct vehicle_riding_cabin *cabin,
	real_vector3d const *body_up,
	real minimum_normal_k,
	real_vector3d *axis)
{
	real k = vehicle_riding_cabin_cosine(body_up);
	real back = minimum_normal_k + RIDING_CABIN_AXIS_BAND < 1.f ? minimum_normal_k + RIDING_CABIN_AXIS_BAND : 1.f;

	if (k < minimum_normal_k)
		cabin->body_axis = TRUE;
	else if (cabin->body_axis && k >= back)
		cabin->body_axis = FALSE;
	vehicle_riding_cabin_current_axis(cabin, body_up, axis);
}

void vehicle_riding_cabin_current_axis(
	struct vehicle_riding_cabin const *cabin,
	real_vector3d const *body_up,
	real_vector3d *axis)
{
	axis->i = axis->j = 0.f;
	axis->k = 1.f;
	if (cabin->body_axis && magnitude3d(body_up) > 0.f)
	{
		*axis = *body_up;
		normalize3d(axis);
	}
}

void vehicle_riding_cabin_ends(
	real_point3d const *base,
	real height,
	real_vector3d const *axis,
	real_point3d *ceiling_start,
	real_point3d *floor_start)
{
	real_point3d top = *base;

	top.z += height;
	/* (upside down the vehicle's ceiling lies below the pill: each ray
	starts from the pill's end nearer its surface, never through the pill) */
	if (axis->k < 0.f)
	{
		*ceiling_start = *base;
		*floor_start = top;
	}
	else
	{
		*ceiling_start = top;
		*floor_start = *base;
	}
}

boolean vehicle_riding_cabin_hit(
	struct vehicle_riding_cabin_rays const *rays,
	real jump_height,
	real height)
{
	boolean ceiling;

	if (!rays->valid)
		return FALSE;
	/* (a roof within a jump, or the pill's end already inside the vehicle;
	with no jump to rise by, the inside test alone) */
	ceiling = rays->inside || (jump_height > 0.f && rays->ceiling && rays->ceiling_distance <= jump_height);
	/* (and the vehicle's own floor under it: a belly overhead, or a gun over
	an open deck, is no cabin) */
	return ceiling && rays->floor && rays->floor_distance <= jump_height + height;
}

short vehicle_riding_cabin_phase(
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state,
	struct vehicle_riding_step const *step,
	boolean airborne,
	boolean tilted)
{
	if (!step || state->ticks <= 0 || cabin->vehicle_index != state->object_index)
		return _vehicle_riding_cabin_sample_none;
	/* (on the deck, sliding in the air too: once a player tick, whatever the
	vehicle's update did) */
	if (vehicle_riding_is_riding(state) && !cabin->take_off)
		return _vehicle_riding_cabin_sample_standing;
	/* (a jump: one raw sample at its first protected tick, kept for the
	jump, hit or miss) */
	if (tilted && cabin->jump == _vehicle_riding_enclosure_unknown && (cabin->take_off || airborne))
		return _vehicle_riding_cabin_sample_jump;
	return _vehicle_riding_cabin_sample_none;
}

void vehicle_riding_cabin_standing(
	struct vehicle_riding_cabin *cabin,
	boolean hit,
	short grace)
{
	/* (back on the deck: the jump's own sample is over) */
	cabin->jump = _vehicle_riding_enclosure_unknown;
	cabin->host_legacy = FALSE;
	if (hit)
	{
		cabin->standing = _vehicle_riding_enclosure_enclosed;
		cabin->misses = 0;
	}
	else if (cabin->standing == _vehicle_riding_enclosure_enclosed && cabin->misses < grace)
	{
		/* (a brief gap while sliding keeps it; an initial miss never had it) */
		cabin->misses++;
	}
	else
	{
		cabin->standing = _vehicle_riding_enclosure_open;
	}
}

void vehicle_riding_cabin_jump(
	struct vehicle_riding_cabin *cabin,
	boolean hit)
{
	/* (raw: standing grace never becomes a jump-long verdict; the standing
	misses wait for the landing) */
	cabin->jump = hit ? _vehicle_riding_enclosure_enclosed : _vehicle_riding_enclosure_open;
}

short vehicle_riding_cabin_verdict(
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state,
	struct vehicle_riding_air const *air,
	boolean cabin_evidence)
{
	if (state->ticks <= 0)
		return _vehicle_riding_enclosure_unknown;
	if (cabin && cabin->vehicle_index == state->object_index)
	{
		/* (the host's verdict for the phase the rider is in, whatever this machine's samples say) */
		if (cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_standing &&
			vehicle_riding_is_riding(state) && !cabin->take_off)
			return cabin->host_verdict;
		if (vehicle_riding_is_riding(state) && !cabin->take_off)
			return cabin->standing;
		if (cabin->host && cabin->host_phase == _vehicle_riding_cabin_sample_jump)
			return cabin->host_verdict;
		if (!(cabin->host_legacy && state->ticks < RIDING_TICKS) &&
			cabin->jump != _vehicle_riding_enclosure_unknown)
			return cabin->jump;
	}
	/* (a shallow jump's single ray up is no cabin evidence) */
	if (cabin_evidence || !air)
		return _vehicle_riding_enclosure_unknown;
	return air->enclosure;
}

boolean vehicle_riding_cabin_take_host(
	struct vehicle_riding_cabin *cabin,
	long unit_index,
	struct vehicle_riding_state const *state,
	long vehicle_index,
	short verdict,
	short phase,
	boolean cabin_evidence,
	boolean current_window)
{
	if (!cabin || !current_window || state->ticks <= 0 || state->object_index != vehicle_index)
		return FALSE;
	/* (a legacy jump has no cabin evidence, even when a local jump sample does) */
	if (phase == _vehicle_riding_cabin_sample_jump && !cabin_evidence)
	{
		if (cabin->unit_index != unit_index || cabin->vehicle_index != vehicle_index)
			vehicle_riding_cabin_clear(cabin);
		cabin->unit_index = unit_index;
		cabin->vehicle_index = vehicle_index;
		cabin->host_legacy = TRUE;
		cabin->host = FALSE;
		return FALSE;
	}
	cabin->host_legacy = FALSE;
	if (verdict == _vehicle_riding_enclosure_unknown)
	{
		if (cabin->unit_index == unit_index && cabin->vehicle_index == vehicle_index)
			cabin->host = FALSE;
		return FALSE;
	}
	/* (a window seen first by its host's word is keyed to it, as an acquisition does) */
	if (cabin->unit_index != unit_index || cabin->vehicle_index != vehicle_index)
	{
		vehicle_riding_cabin_clear(cabin);
		cabin->unit_index = unit_index;
		cabin->vehicle_index = vehicle_index;
	}
	cabin->host_verdict = verdict;
	cabin->host_phase = phase;
	cabin->host = TRUE;
	return TRUE;
}

void vehicle_riding_cabin_drop_host(
	struct vehicle_riding_cabin *cabin)
{
	if (cabin)
		cabin->host = FALSE;
}

boolean vehicle_riding_cabin_exception(
	boolean eligible,
	boolean valid_window,
	boolean enclosed,
	real_vector3d const *body_up,
	real minimum_normal_k)
{
	return eligible && valid_window && enclosed && vehicle_riding_cabin_tilted(body_up, minimum_normal_k);
}

boolean vehicle_riding_cabin_crossing(
	boolean hit,
	real t)
{
	/* (a pill resting against a wall touches it at an end; only a surface
	between the two places is a hull crossed) */
	return hit && t > 0.f && t < 1.f;
}

long vehicle_riding_cabin_carrier_vehicle(
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state,
	boolean exception)
{
	/* (a jump leaves the deck at its next tick, touched or held) */
	if (!exception || !cabin || cabin->take_off)
		return NONE;
	return state->object_index;
}

/* whether an enclosed rider stays on its vehicle this tick with no touch
of it: one the deck carried into the tick and its exception names. (A
rider in the window waits for a real touch: a jump would end at its next
tick) */
boolean vehicle_riding_cabin_holds(
	long cabin_vehicle,
	boolean carried)
{
	return cabin_vehicle != NONE && carried;
}

/* whether a rider's cabin protects it: the verdict it goes by, or on the
deck this machine's own standing sample (a host's open is a round trip old
and says nothing of a tilt reached since; a miss here never takes back the
host's enclosed, and no word the host reads changes) */
boolean vehicle_riding_cabin_encloses(
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state,
	struct vehicle_riding_air const *air)
{
	if (vehicle_riding_cabin_verdict(cabin, state, air, TRUE) == _vehicle_riding_enclosure_enclosed)
		return TRUE;
	return cabin && cabin->vehicle_index == state->object_index && vehicle_riding_is_riding(state) && !cabin->take_off &&
		cabin->standing == _vehicle_riding_enclosure_enclosed;
}

short vehicle_riding_cabin_carrier(
	long vehicle_index,
	short count,
	long const *objects)
{
	short index;

	if (vehicle_index == NONE)
		return NONE;
	/* (a real touch of that vehicle, a wall or a ceiling too: never the
	ground, another vehicle or a touch made up; a rider the deck carried in
	needs none, vehicle_riding_cabin_holds) */
	for (index = 0; index < count; index++)
	{
		if (objects[index] == vehicle_index)
			return index;
	}
	return NONE;
}

boolean vehicle_riding_cabin_candidate(
	boolean interior,
	boolean free,
	boolean crosses)
{
	return interior && free && !crosses;
}

short vehicle_riding_cabin_recover(
	struct vehicle_riding_cabin_check const *check)
{
	/* (no interior history: no cabin recovery; an overlap is left as it is,
	and logged) */
	if (!check->have_anchor)
		return check->touching ? _vehicle_riding_cabin_settle_failed : _vehicle_riding_cabin_settle_none;
	if (check->interior)
	{
		if (!check->touching)
			return _vehicle_riding_cabin_settle_none;
	}
	else if (check->crosses)
	{
		/* (out through the hull: back to the anchor however far it is, through
		the vehicle only, or a place about it) */
		if (check->anchor_valid)
			return _vehicle_riding_cabin_settle_anchor;
		return check->around_valid ? _vehicle_riding_cabin_settle_around : _vehicle_riding_cabin_settle_failed;
	}
	else if (!check->touching)
	{
		/* (out through an opening and clear of the hull: a touching raw miss
		is still an overlap, not a reason to forget its interior history) */
		return _vehicle_riding_cabin_settle_door;
	}
	/* (an overlap inside: the anchor if it is near, the touch's way out, the
	fix's offset) */
	if (check->anchor_valid && check->anchor_distance <= 2.f*check->width)
		return _vehicle_riding_cabin_settle_anchor;
	if (check->normal_valid)
		return _vehicle_riding_cabin_settle_candidate;
	if (check->fixed_valid)
		return _vehicle_riding_cabin_settle_fixed;
	if (check->around_valid)
		return _vehicle_riding_cabin_settle_around;
	return _vehicle_riding_cabin_settle_failed;
}

short vehicle_riding_cabin_log(
	struct vehicle_riding_cabin *cabin,
	short settle,
	long tick,
	boolean final)
{
	if (settle == _vehicle_riding_cabin_settle_failed)
	{
		cabin->failure_seen_tick = tick;
		if (!cabin->failure_logged || (unsigned long)tick - (unsigned long)cabin->failure_log_tick >= RIDING_CABIN_LOG_TICKS)
		{
			cabin->failure_logged = TRUE;
			cabin->failure_log_tick = tick;
			return 1;
		}
	}
	else if (final && cabin->failure_logged && cabin->failure_seen_tick != tick)
	{
		/* (a tick that failed anywhere is still in the episode: the staged check may fail and the final one clear, every tick) */
		cabin->failure_logged = FALSE;
		return -1;
	}
	return 0;
}

void vehicle_riding_cabin_settled(
	struct vehicle_riding_cabin *cabin,
	short settle,
	boolean interior,
	real_point3d const *local)
{
	/* (left through an opening: a later walk outside is no hull escape) */
	if (settle == _vehicle_riding_cabin_settle_door)
	{
		cabin->have_anchor = FALSE;
		return;
	}
	if (settle == _vehicle_riding_cabin_settle_failed)
		return;
	if (interior)
	{
		cabin->have_anchor = TRUE;
		cabin->anchor = *local;
	}
}

boolean vehicle_riding_cabin_watched(
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state,
	long unit_index)
{
	/* (no stock vehicle has cabin evidence: their riders cast nothing more) */
	return cabin && state->ticks > 0 && cabin->unit_index == unit_index && cabin->vehicle_index == state->object_index &&
		(cabin->exception || cabin->have_anchor || cabin->standing == _vehicle_riding_enclosure_enclosed ||
		cabin->jump == _vehicle_riding_enclosure_enclosed);
}

boolean vehicle_riding_cabin_defers(
	struct vehicle_riding_cabin const *cabin,
	struct vehicle_riding_state const *state,
	long unit_index,
	long vehicle_index,
	boolean exception)
{
	if (!cabin || state->ticks <= 0 || state->object_index != vehicle_index || cabin->unit_index != unit_index ||
		cabin->vehicle_index != vehicle_index)
	{
		return FALSE;
	}
	/* (protected, or with interior history its own tick must settle first:
	a hull escape's return, a blocked interior's no-op; a level vehicle too) */
	return exception || cabin->have_anchor;
}

short vehicle_riding_cabin_collision(
	short decision,
	boolean defers)
{
	/* (never the original push or hurt merely because the vehicle is steep) */
	return defers && decision != _vehicle_riding_collision_original ? _vehicle_riding_collision_resolved : decision;
}
