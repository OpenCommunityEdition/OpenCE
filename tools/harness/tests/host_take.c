/* the host's real take path, bounds and riding helpers against salted objects */
#include "harness.h"
#include <math.h>
#include <limits.h>
#include <stddef.h>

typedef unsigned short word;
typedef struct { real i, j; } real_vector2d;
#define MAXIMUM_TRACKED_PLAYERS 128
#define TICKS_PER_SECOND 30
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define csmemset memset
#define _object_mask_biped 1
#define _object_mask_device 2
#define _object_mask_machine 2
#define _biped_airborne_bit 0
#define _game_connection_network_client 2
#include "types.inc"

enum { V = 0x10080, OTHER = 0x20081 };
struct fake_object
{
    long mask;
    long definition_index;
    struct { real_point3d position; real_vector3d translational_velocity; long parent_object_index; } object;
    struct { long elevator_object_index; long support_surface_index; char elevator_ticks; long flags; long airborne_ticks; } biped;
    struct { long player_index; } unit;
    struct { real position_velocity; } device;
};
#define object_datum fake_object
#define biped_datum fake_object
#define device_datum fake_object
static struct fake_object objects[MAXIMUM_TRACKED_PLAYERS];
static struct vehicle_riding_state riding[MAXIMUM_TRACKED_PLAYERS];
static struct vehicle_riding_air host_air;
static struct vehicle_riding_vehicle vehicles[2];
static struct player_datum { long unit_index; real speed_multiplier; } players[MAXIMUM_TRACKED_PLAYERS];
static struct game_globals_player_information
{
    real run_forward_speed, run_backward_speed, run_sideways_speed, double_speed_multiplier;
} information = { 2.25f, 2.0f, 1.75f, 2.0f };
static struct game_globals { struct { long count; } player_information; } fake_globals = { { 1 } };
static struct biped_definition
{
    struct { real jump_velocity, collision_height_standing, collision_height_crouching; } biped;
} fake_definition = { { 0.11f, 0.7f, 0.5f } };
static real global_gravity = 0.0036f;
static long game_time;
static long reconcile_calls, rebase_calls, phase;
static long apply_calls[MAXIMUM_TRACKED_PLAYERS], apply_phase[MAXIMUM_TRACKED_PLAYERS];
static boolean distributed_rider_pass[MAXIMUM_TRACKED_PLAYERS];
static boolean riding_applies = TRUE;
static struct { long corrections; } distributed_statistics;
static real vehicle_write;
static real pill_offset = -0.1f;
static boolean replay_frames;
static struct vehicle_riding_vehicle replay_body, replay_node;
real vehicle_riding_transition_leash(void) { return 0.25f; }
static real_point3d const body_place = { -0.5f, 0.2f, 0.0f };

#define object_get(i) (&objects[(i)])
#define unit_get(i) object_get(i)
#define game_time_get() game_time
#define game_connection() 1
#define game_players_are_double_speed() FALSE
#define scenario_get_game_globals() (&fake_globals)
#define biped_definition_get(i) (&fake_definition)
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)checked_information((index)))
static void *checked_information(long index)
{
    CHECK(index == 0 && fake_globals.player_information.count > index, "invalid tag element");
    return &information;
}
static void *object_try_and_get_and_verify_type(long index, long mask)
{
    return index >= 0 && index < MAXIMUM_TRACKED_PLAYERS && (objects[index].mask & mask) ? &objects[index] : NULL;
}
static struct player_datum *distributed_player(short p) { return &players[p]; }
static long distributed_living_unit(struct player_datum const *p) { return p->unit_index; }
static long distributed_player_machine(short p) { return p; }
static real rtt_ticks = 3.0f;
static real distributed_machine_round_trip_ticks(long machine) { return rtt_ticks; }
static boolean network_coop_player_has_structure_bsp(long p) { return TRUE; }
static boolean distributed_point_valid(real_point3d const *p, real bound)
{
    return valid_real_point3d(p) && fabsf(p->x) <= bound && fabsf(p->y) <= bound && fabsf(p->z) <= bound;
}
static boolean distributed_axes_make_valid(real_vector3d *f, real_vector3d *u)
{
    return valid_real_vector3d(f) && valid_real_vector3d(u) && normalize3d(f) > 0.0f && normalize3d(u) > 0.0f;
}
#define UNIT_WORLD_BOUND 100000.0f
static void distributed_vector_unpack(struct distributed_vector const *packed, real scale, real_vector3d *v)
{
    v->i = packed->i / scale; v->j = packed->j / scale; v->k = packed->k / scale;
}
static void biped_get_physics_pill(long unit, real_point3d *base, real *height, real *width)
{
    *base = objects[unit].object.position;
    base->z += pill_offset;
    *height = 0.7f; *width = 0.2f;
}
static boolean biped_riding_get_state(long unit, struct vehicle_riding_state *state)
{
    if (!riding_applies || unit < 0 || unit >= MAXIMUM_TRACKED_PLAYERS || !objects[unit].mask)
        return FALSE;
    *state = riding[unit];
    return TRUE;
}
static short biped_riding_vehicle_node_count(long v)
{
    return v == vehicles[0].object_index || v == vehicles[1].object_index ? 3 : NONE;
}
static boolean biped_riding_vehicle(long v, short node, struct vehicle_riding_vehicle *out)
{
    long slot;
    if (biped_riding_vehicle_node_count(v) == NONE || (node != NONE && (node < 0 || node >= 3)))
        return FALSE;
    if (replay_frames)
    {
        *out = node == NONE ? replay_body : replay_node;
        out->object_index = v; out->node_index = node; out->origin = out->position;
        return TRUE;
    }
    slot = v == vehicles[0].object_index ? 0 : 1;
    *out = vehicles[slot];
    out->origin = vehicles[slot].position;
    out->node_index = node;
    if (node == 2)
    {
        out->position.z += vehicles[slot].bounding_sphere_radius;
        out->forward.i = cosf(0.3f); out->forward.j = sinf(0.3f);
    }
    return TRUE;
}
static boolean biped_riding_rebase(long unit);
static struct vehicle_riding_cabin host_cabin;
static boolean biped_riding_frame_velocity(long unit, struct vehicle_riding_deck const *deck, real_point3d const *base,
    real_vector3d *velocity)
{
    if (riding[unit].ticks > 0 && riding[unit].object_index == deck->vehicle_index)
        *velocity = riding[unit].velocity;
    else
        memset(velocity, 0, sizeof(*velocity));
    return TRUE;
}
/* (what a fault that took the part into the host's own state would do) */
static boolean biped_riding_take_deck(long unit, struct vehicle_riding_deck const *deck)
{
    riding[unit].ticks = deck->ticks;
    riding[unit].falling = deck->flags & 1;
    riding[unit].velocity = deck->velocity;
    return TRUE;
}
enum { _network_vehicle_write_state };
static boolean network_objects_reconcile(long unit, real_point3d const *p, real_vector3d const *f,
    real_vector3d const *up, real_vector3d const *velocity, real_vector3d const *angular, real blend, short kind)
{
    boolean snap = distance_squared3d(&objects[unit].object.position, p) > blend * blend;
    reconcile_calls++;
    if (snap)
        objects[unit].object.position = *p;
    else
    {
        objects[unit].object.position.x += (p->x - objects[unit].object.position.x) * 0.5f;
        objects[unit].object.position.y += (p->y - objects[unit].object.position.y) * 0.5f;
        objects[unit].object.position.z += (p->z - objects[unit].object.position.z) * 0.5f;
    }
    if (velocity)
        objects[unit].object.translational_velocity = *velocity;
    return snap;
}
/* (MCC movement effects: none in this fixture) */
static boolean mcc_script_effects_host_movement(void) { return FALSE; }
#include "under_test.inc"

static boolean biped_riding_rebase(long unit)
{
    struct vehicle_riding_vehicle v;
    real_point3d base;
    real height, width;
    rebase_calls++;
    if (!biped_riding_vehicle(riding[unit].object_index, riding[unit].node_index, &v))
        return FALSE;
    biped_get_physics_pill(unit, &base, &height, &width);
    return vehicle_riding_rebase(&riding[unit], &v, &base);
}
static real_point3d to_world(real_point3d const *place)
{
    real_point3d p;
    vehicle_riding_point_to_world(&vehicles[0].position, &vehicles[0].forward, &vehicles[0].up, place, &p);
    return p;
}
static void host_tick_at(short p, real_point3d const *place)
{
    real_point3d base = to_world(place);
    objects[p].object.position = base;
    objects[p].object.position.z += 0.1f;
    objects[p].object.translational_velocity = riding[p].velocity;
    riding[p].body_position = vehicles[0].position;
    riding[p].body_forward = vehicles[0].forward;
    riding[p].body_up = vehicles[0].up;
    riding[p].vehicle_position = vehicles[0].position;
    riding[p].vehicle_forward = vehicles[0].forward;
    riding[p].vehicle_up = vehicles[0].up;
    riding[p].local_point = *place;
}
static void host_tick(short p) { host_tick_at(p, &body_place); }
static void prediction(short p, real_point3d place)
{
    struct distributed_unit_state *s = &distributed_predictions[p].state;
    distributed_predictions[p].valid = TRUE;
    distributed_predictions[p].time = game_time;
    distributed_predictions[p].taken_host_time = NONE;
    s->player_index = p; s->unit_index = p;
    s->deck_present = TRUE;
    s->deck.vehicle_index = V; s->deck.node_index = NONE; s->deck.ticks = RIDING_TICKS;
    s->deck.place = place;
    s->position = to_world(&place);
    s->position.x -= 1.0f;
    s->position.z += 0.1f;
    s->forward.i = (short)DISTRIBUTED_UNIT_SCALE;
    s->up.k = (short)DISTRIBUTED_UNIT_SCALE;
}
static void setup(void)
{
    long p;
    memset(objects, 0, sizeof(objects)); memset(riding, 0, sizeof(riding));
    memset(&host_air, 0, sizeof(host_air));
    memset(distributed_accepted, 0, sizeof(distributed_accepted));
    memset(distributed_predictions, 0, sizeof(distributed_predictions));
    memset(distributed_host_speeds, 0, sizeof(distributed_host_speeds));
    memset(apply_calls, 0, sizeof(apply_calls));
    memset(vehicles, 0, sizeof(vehicles));
    vehicles[0].object_index = V; vehicles[1].object_index = OTHER;
    vehicles[0].forward.i = vehicles[1].forward.i = 1.0f;
    vehicles[0].up.k = vehicles[1].up.k = 1.0f;
    vehicles[0].body_forward.i = vehicles[1].body_forward.i = 1.0f;
    vehicles[0].body_up.k = vehicles[1].body_up.k = 1.0f;
    vehicles[0].bounding_sphere_radius = 0.5f;
    game_time = 1; rebase_calls = reconcile_calls = 0; vehicle_write = 0.0f;
    rtt_ticks = 3.0f; pill_offset = -0.1f; replay_frames = FALSE;
    riding_applies = TRUE;
    memset(&host_cabin, 0, sizeof(host_cabin));
    for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
    {
        players[p].unit_index = NONE; players[p].speed_multiplier = 1.0f;
        distributed_host_speeds[p].unit_index = NONE;
        distributed_accepted[p].taken_air_unit_index = NONE; distributed_accepted[p].taken_air_vehicle = NONE;
        objects[p].object.parent_object_index = NONE;
        objects[p].biped.elevator_object_index = NONE;
    }
    objects[0].mask = _object_mask_biped; objects[0].unit.player_index = 0;
    players[0].unit_index = 0;
    riding[0].ticks = RIDING_TICKS; riding[0].object_index = V; riding[0].node_index = NONE;
    riding[0].velocity.i = 0.333f;
    host_tick(0); prediction(0, body_place);
}
static boolean take(struct distributed_on_foot_bound *bound)
{
    distributed_on_foot_bound(0, 0, &riding[0].velocity, bound);
    distributed_take_prediction(0, 0, bound);
    return distributed_predictions[0].taken_host_time == game_time;
}
static void stable_window(void)
{
    distributed_host_speeds[0].unit_index = 0;
    distributed_host_speeds[0].window = TRUE;
    distributed_host_speeds[0].vehicle_index = V;
    distributed_host_speeds[0].velocity = riding[0].velocity;
    distributed_host_speeds[0].frame_velocity = riding[0].velocity;
}
static void check_base(real_point3d expected)
{
    real_point3d base;
    real h, w;
    biped_get_physics_pill(0, &base, &h, &w);
    CHECK(distance_squared3d(&base, &expected) < 1e-8f, "base (%f,%f,%f) != (%f,%f,%f)",
        base.x, base.y, base.z, expected.x, expected.y, expected.z);
}
static void network_objects_apply_vehicle_predictions(void) { vehicles[0].position.x += vehicle_write; }
static void two_passes(void)
{
    short p;
    for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
        distributed_rider_pass[p] = distributed_prediction_rides(p, distributed_living_unit(distributed_player(p)));
    phase = 0; distributed_apply_predictions(FALSE);
    network_objects_apply_vehicle_predictions();
    phase = 1; distributed_apply_predictions(TRUE);
}
static void accepted_once(void)
{
    struct distributed_on_foot_bound b;
    CHECK(take(&b), "body word not taken");
    CHECK(rebase_calls == 1, "deck take did not rebase");
}

/* (the biped tick updates the falling frame before the network bound reads it) */
static void air_tick(real deck_k, boolean over_deck)
{
    struct vehicle_riding_step step;
    real_vector3d frame;
    memset(&step, 0, sizeof(step));
    step.player_ticks = 1; step.over_deck = over_deck;
    step.tracked_deck_velocity.k = deck_k;
    vehicle_riding_air_frame(&riding[0], &step, &host_air, TRUE,
        &objects[0].object.translational_velocity, &frame);
}

static void hover(boolean stopped)
{
    long speed, t;
    for (speed = 1; speed <= 2; speed++)
    {
        real vk = -0.1f * speed, excess = 0.0f;
        long refused_at = 0, taken = 0;
        setup(); riding[0].velocity.i = 0.0f; riding[0].velocity.k = vk;
        host_tick(0); accepted_once();
        objects[0].biped.flags = 1 << _biped_airborne_bit; riding[0].ticks--;
        for (t = 1; t <= 30; t++)
        {
            real limit, elapsed = (real)MAX(t - 9, 0);
            /* (the host follows taken words, then advances by its own jump physics) */
            game_time = 1 + t; vehicles[0].position.z = stopped ? vk : vk * t;
            objects[0].object.position.z += vk + 0.11f - 0.0018f * (2*t - 1);
            objects[0].object.translational_velocity.k = vk + 0.11f - 0.0036f * t;
            objects[0].biped.airborne_ticks = t;
            riding[0].body_position = riding[0].vehicle_position = vehicles[0].position;
            air_tick(stopped ? 0.0f : vk, TRUE);
            prediction(0, body_place);
            distributed_predictions[0].state.deck_present = FALSE;
            distributed_predictions[0].state.position = objects[0].object.position;
            distributed_predictions[0].state.position.z = 0.1f + 0.11f*t - 0.0018f*t*t;
            /* (round up the wire velocity so quantization cannot fake a host throw) */
            distributed_predictions[0].state.velocity.k = (short)ceilf(
                (vk + 0.11f - 0.0036f*t) * DISTRIBUTED_VELOCITY_SCALE);
            two_passes();
            if (distributed_predictions[0].taken_host_time != game_time)
            {
                if (!refused_at) refused_at = t;
            }
            else
            {
                taken++;
                /* (literal fixture physics: apex plus crouch and tolerance;
                three round-trip ticks and six jitter ticks ease the falling ceiling) */
                limit = MIN(2.880556f, 1.2f + 0.11f*elapsed - 0.0018f*elapsed*elapsed);
                if (t <= 9) limit = 2.880556f;
                excess = MAX(excess, distributed_predictions[0].state.position.z - (0.1f + vk*t + limit));
                excess = MAX(excess, objects[0].object.position.z - (0.1f + vk*t + limit));
            }
        }
        printf("hover stopped=%d vk=%f: taken %ld/30, first refusal %ld, accepted/committed excess %f\n",
            stopped, vk, taken, refused_at, excess);
        CHECK(refused_at > 0 && refused_at <= 30, "still-frame hover never refused");
        CHECK(excess <= 0.0f, "hover escaped takeoff-frame envelope by %f", excess);
    }
}

/* the host copy follows what it takes while the client claims 3 wu a tick more, on a deck
that moves, with its deck part (or the host copy's riding) coming and going */
static void weave(boolean window, long period, real vehicle_speed)
{
    struct distributed_on_foot_bound b;
    real_point3d hp = body_place;
    long tick, taken = 0, refused_at = 0;
    setup();
    for (tick = 1; tick <= 30; tick++)
    {
        real_point3d place = body_place, base;
        boolean off = ((tick / period) & 1) != 0;
        game_time = tick; vehicles[0].position.x += vehicle_speed;
        riding[0].ticks = window && off ? RIDING_TICKS - 1 : RIDING_TICKS;
        host_tick_at(0, &hp);
        place.x += 3.0f * tick; prediction(0, place);
        if (off)
        {
            distributed_predictions[0].state.deck_present = FALSE;
            distributed_predictions[0].state.position = to_world(&place);
            distributed_predictions[0].state.position.z += 0.1f;
        }
        if (take(&b)) taken++; else if (!refused_at) refused_at = tick;
        base = objects[0].object.position; base.z -= 0.1f;
        vehicle_riding_point_to_local(&vehicles[0].position, &vehicles[0].forward, &vehicles[0].up, &base, &hp);
    }
    CHECK(refused_at > 0 && refused_at <= 30 && taken < 30, "deck part or window coming and going never refused");
    CHECK(hp.x - body_place.x <= b.speed * 30.0f, "walked %f along the deck in a second", hp.x - body_place.x);
}

static void world_word(real dx, real dz)
{
    prediction(0, body_place);
    distributed_predictions[0].state.deck_present = FALSE;
    distributed_predictions[0].state.position = objects[0].object.position;
    distributed_predictions[0].state.position.x += dx;
    distributed_predictions[0].state.position.z += dz;
}

/* the part of a word that names the vehicle in its air, its place the one the word's position has on the vehicle (the part
carries the base of the pill the position is of) */
static void air_part_at_word(long ticks)
{
    struct distributed_unit_state *s = &distributed_predictions[0].state;
    real_point3d base = s->position;
    base.z += pill_offset;
    s->deck_present = TRUE;
    s->deck.vehicle_index = V; s->deck.node_index = NONE; s->deck.ticks = (char)ticks;
    vehicle_riding_point_to_local(&vehicles[0].position, &vehicles[0].forward, &vehicles[0].up, &base, &s->deck.place);
}

/* a word of a part of the vehicle in its air is no deck word: it names a place on the vehicle (air_part_at_word), and the along path guards the height */
static void disagree(boolean pitched, boolean air_part)
{
    struct distributed_on_foot_bound b;
    long t;
    for (t = 1; t <= 20; t++)
    {
        real a = 0.05f * t;
        real_point3d before, after;
        real h, w;
        game_time = t; vehicles[0].position.z = 0.1f * t;
        if (pitched)
        {
            vehicles[0].forward.i = cosf(a) * 0.939692621f;
            vehicles[0].forward.j = sinf(a) * 0.939692621f;
            vehicles[0].forward.k = -0.342020143f;
            vehicles[0].up.i = cosf(a) * 0.342020143f;
            vehicles[0].up.j = sinf(a) * 0.342020143f;
            vehicles[0].up.k = 0.939692621f;
        }
        riding[0].velocity.i = 0; riding[0].velocity.k = 0.1f;
        host_tick(0); world_word(0.05f, -0.3f);
        if (air_part)
        {
            air_part_at_word(RIDING_TICKS - 1);
        }
        CHECK(take(&b), "disagreeing word refused at %ld", t);
        biped_get_physics_pill(0, &before, &h, &w);
        vehicle_riding_point_to_local(&vehicles[0].position, &vehicles[0].forward, &vehicles[0].up, &before, &after);
        CHECK(fabsf(after.z) < 1e-5f, "host height changed along deck up at %ld: %f", t, after.z);
        CHECK(after.z >= -1e-5f, "rider taken below deck");
        CHECK(after.x > body_place.x, "across motion discarded");
    }
}


/* the host copy on a vehicle's deck at a place, as its own tick leaves it */
static void host_on(long slot, real_point3d const *place)
{
    struct vehicle_riding_vehicle *v = &vehicles[slot];
    real_point3d base;
    vehicle_riding_point_to_world(&v->position, &v->forward, &v->up, place, &base);
    objects[0].object.position = base; objects[0].object.position.z += 0.1f;
    objects[0].object.translational_velocity = riding[0].velocity;
    riding[0].object_index = v->object_index; riding[0].node_index = NONE;
    riding[0].body_position = riding[0].vehicle_position = v->position;
    riding[0].body_forward = riding[0].vehicle_forward = v->forward;
    riding[0].body_up = riding[0].vehicle_up = v->up;
    riding[0].local_point = *place;
}
/* the client's word: a deck part of that vehicle at a place, or only a world position there */
static void word_on(long slot, real_point3d const *place, short ticks, boolean part)
{
    struct distributed_unit_state *s;
    real_point3d w;
    vehicle_riding_point_to_world(&vehicles[slot].position, &vehicles[slot].forward, &vehicles[slot].up, place, &w);
    prediction(0, body_place);
    s = &distributed_predictions[0].state;
    s->deck_present = part; s->deck.vehicle_index = vehicles[slot].object_index;
    s->deck.node_index = NONE; s->deck.ticks = (char)ticks; s->deck.place = *place;
    s->position = w; s->position.z += 0.1f;
}
/* (the anchor itself: the leash's bookkeeping after it follows the client's part as well) */
#define ANCHOR_BYTES offsetof(struct distributed_accepted_state, transition_sampled)
static boolean taken_now(void) { return distributed_predictions[0].taken_host_time == game_time; }
static real_point3d world_on(long slot, real_point3d const *place)
{
    real_point3d w;
    vehicle_riding_point_to_world(&vehicles[slot].position, &vehicles[slot].forward, &vehicles[slot].up, place, &w);
    return w;
}
static void off_deck(void)
{
    riding[0].ticks = 0; riding[0].falling = 0; riding[0].object_index = NONE;
    memset(&riding[0].velocity, 0, sizeof(riding[0].velocity));
}

/* the recorded rise 0 tick 2034 and brush 0 tick 2240 of the host copy of a client-owned rider: the vehicle's frames, the host copy,
the client's word and the transition memory of the tick before, as logged (the pill sits 0.2 under the unit there) */
static real_point3d pt(real x, real y, real z) { real_point3d p; p.x = x; p.y = y; p.z = z; return p; }
static real_vector3d vc(real i, real j, real k) { real_vector3d v; v.i = i; v.j = j; v.k = k; return v; }
static void transition_memory(short host_ticks, long host_vehicle, short client_ticks, long client_vehicle)
{
    struct distributed_accepted_state *a = &distributed_accepted[0];
    a->transition_sampled = TRUE; a->transition_unit_index = 0; a->transition_until = 0; a->transition_client_spent = FALSE;
    a->transition_host_ticks = host_ticks; a->transition_host_vehicle = host_vehicle;
    a->transition_client_ticks = client_ticks; a->transition_client_vehicle = client_vehicle;
    a->transition_leash_time = NONE; a->transition_board_vehicle = NONE; a->transition_board_until = 0;
}
static void client_word(short ticks, long vehicle, boolean part)
{
    struct distributed_unit_state *s = &distributed_predictions[0].state;
    s->deck_present = part; s->deck.vehicle_index = vehicle; s->deck.ticks = (char)ticks;
}
static void host_window(short ticks, long vehicle)
{
    struct vehicle_riding_state host = riding[0];
    host.ticks = ticks; host.object_index = vehicle;
    distributed_riding_transition(0, 0, &host);
}
static void load_rise_2034(void)
{
    struct distributed_unit_state *s;
    setup();
    game_time = 2034; rtt_ticks = 0.0f; pill_offset = 0.2f; replay_frames = TRUE;
    distributed_host_speeds[0].ground_place_height = 0.4f;
    objects[0].object.position = pt(30.01221085f, -90.62013245f, 0.80156642f);
    objects[0].object.translational_velocity = vc(0.00095343f, -0.00072971f, -0.00049056f);
    replay_body.position = pt(29.58844185f, -90.04680634f, 0.53777862f);
    replay_body.forward = vc(0.5724744f, -0.8163233f, 0.0767435f); replay_body.up = vc(0.0007455f, 0.0941166f, 0.9955609f);
    replay_node.position = pt(29.52730560f, -89.95027924f, 0.62735379f);
    replay_node.forward = replay_body.forward; replay_node.up = replay_body.up;
    memset(&riding[0], 0, sizeof(riding[0]));
    riding[0].ticks = RIDING_TICKS; riding[0].object_index = V; riding[0].node_index = 0;
    riding[0].velocity = vc(0.00009346f, 0.00021362f, -0.00002527f);
    riding[0].body_position = replay_body.position; riding[0].body_forward = replay_body.forward; riding[0].body_up = replay_body.up;
    riding[0].vehicle_position = replay_node.position; riding[0].vehicle_forward = replay_node.forward; riding[0].vehicle_up = replay_node.up;
    riding[0].local_point = pt(0.85313100f, -0.00453853f, 0.30986860f);
    distributed_predictions[0].valid = TRUE; distributed_predictions[0].time = game_time; distributed_predictions[0].taken_host_time = NONE;
    s = &distributed_predictions[0].state;
    memset(s, 0, sizeof(*s));
    s->player_index = 0; s->unit_index = 0;
    s->position = pt(29.95589066f, -90.72336578f, 0.82292825f);
    s->deck_present = TRUE;
    s->deck.vehicle_index = V; s->deck.node_index = 0; s->deck.ticks = RIDING_TICKS; s->deck.flags = 2;
    s->deck.place = pt(0.83487767f, 0.02975389f, 0.31906742f);
    s->forward.i = (short)DISTRIBUTED_UNIT_SCALE; s->up.k = (short)DISTRIBUTED_UNIT_SCALE;
    /* (the tick before: the host copy in the air of V with 59, the client riding it, the transition armed until 2032) */
    transition_memory(59, V, RIDING_TICKS, V);
    distributed_accepted[0].transition_until = 2032;
}
static void load_brush_2240(void)
{
    struct distributed_unit_state *s;
    setup();
    game_time = 2240; rtt_ticks = 0.0f; pill_offset = 0.2f; replay_frames = TRUE;
    distributed_host_speeds[0].ground_place_height = 0.4f;
    objects[0].object.position = pt(38.17061615f, -90.24746704f, 0.49638015f);
    objects[0].object.translational_velocity = vc(-0.01595429f, -0.00736767f, -0.01757029f);
    replay_body.position = pt(38.9912f, -90.7144f, 0.4095f);
    replay_body.forward = vc(1.0f, 0.0f, 0.0f); replay_body.up = vc(0.0f, 0.0f, 1.0f);
    replay_node = replay_body;
    memset(&riding[0], 0, sizeof(riding[0]));
    riding[0].ticks = RIDING_TICKS - 1; riding[0].object_index = V; riding[0].node_index = NONE;
    riding[0].body_position = replay_body.position; riding[0].body_forward = replay_body.forward; riding[0].body_up = replay_body.up;
    riding[0].vehicle_position = replay_body.position; riding[0].vehicle_forward = replay_body.forward; riding[0].vehicle_up = replay_body.up;
    distributed_predictions[0].valid = TRUE; distributed_predictions[0].time = game_time; distributed_predictions[0].taken_host_time = NONE;
    s = &distributed_predictions[0].state;
    memset(s, 0, sizeof(*s));
    s->player_index = 0; s->unit_index = 0;
    s->position = pt(38.31852341f, -90.20667267f, 0.50130570f);
    s->deck_present = TRUE;
    s->deck.vehicle_index = V; s->deck.node_index = NONE; s->deck.ticks = RIDING_TICKS - 1; s->deck.flags = 1;
    s->forward.i = (short)DISTRIBUTED_UNIT_SCALE; s->up.k = (short)DISTRIBUTED_UNIT_SCALE;
    /* (the tick before: both ends on V's deck, the transition armed until 1997) */
    transition_memory(RIDING_TICKS, V, RIDING_TICKS, V);
    distributed_accepted[0].transition_until = 1997;
}
static void bound_wide(struct distributed_on_foot_bound *b)
{
    memset(b, 0, sizeof(*b));
    b->speed = 100.0f; b->frame_speed = 100.0f; b->airborne_ticks = 0; b->rise = 10.0f;
}

/* the recorded straight 0 tick 252 of the host copy of a client-owned rider: the leash armed at 243 has run out (until 252), the
client's word has no deck part yet, and it was taken on the along path */
static void load_straight_252(void)
{
    struct distributed_unit_state *s;
    setup();
    game_time = 252; rtt_ticks = 0.0f; pill_offset = 0.2f; replay_frames = TRUE;
    distributed_host_speeds[0].ground_place_height = 0.4f;
    objects[0].object.position = pt(38.92505646f, -89.95860291f, 0.66301131f);
    objects[0].object.translational_velocity = vc(0.00005592f, 0.00008599f, -0.00121392f);
    replay_body.position = pt(39.21093369f, -90.65383911f, 0.42749006f);
    replay_body.forward = vc(0.3835937f, -0.9235006f, -0.0015883f); replay_body.up = vc(-0.0315861f, -0.0148387f, 0.9993909f);
    replay_node.position = pt(39.16681671f, -90.55655670f, 0.52581680f);
    replay_node.forward = replay_body.forward; replay_node.up = replay_body.up;
    memset(&riding[0], 0, sizeof(riding[0]));
    riding[0].ticks = RIDING_TICKS; riding[0].object_index = V; riding[0].node_index = 0;
    riding[0].velocity = vc(0.00005341f, 0.00008392f, -0.00121403f);
    riding[0].body_position = replay_body.position; riding[0].body_forward = replay_body.forward; riding[0].body_up = replay_body.up;
    riding[0].vehicle_position = replay_node.position; riding[0].vehicle_forward = replay_node.forward; riding[0].vehicle_up = replay_node.up;
    riding[0].local_point = pt(-0.64548397f, 0.01782141f, 0.33575249f);
    distributed_predictions[0].valid = TRUE; distributed_predictions[0].time = game_time; distributed_predictions[0].taken_host_time = NONE;
    s = &distributed_predictions[0].state;
    memset(s, 0, sizeof(*s));
    s->player_index = 0; s->unit_index = 0;
    s->position = pt(38.92392731f, -89.96109009f, 0.70503366f);
    s->deck_present = FALSE;
    s->velocity.k = (short)(-0.0078125f * DISTRIBUTED_VELOCITY_SCALE);
    s->forward.i = (short)DISTRIBUTED_UNIT_SCALE; s->up.k = (short)DISTRIBUTED_UNIT_SCALE;
    transition_memory(RIDING_TICKS, V, 0, NONE);
    distributed_accepted[0].transition_until = 252;
}
/* a world point in the recorded host node's frame, and the host copy's pill base there */
static real_point3d in_node(real_point3d const *world)
{
    real_point3d place;
    vehicle_riding_point_to_local(&replay_node.position, &replay_node.forward, &replay_node.up, world, &place);
    return place;
}
static real_point3d pill_in_node(void)
{
    real_point3d base;
    real height, width;
    biped_get_physics_pill(0, &base, &height, &width);
    return in_node(&base);
}
/* the word's own pill base: the word names the unit's position, the pill hangs the same way under it, here in the node's frame */
static real_point3d word_in_node(void)
{
    real_point3d base, word = distributed_predictions[0].state.position;
    real height, width;
    biped_get_physics_pill(0, &base, &height, &width);
    word.x += base.x - objects[0].object.position.x; word.y += base.y - objects[0].object.position.y;
    word.z += base.z - objects[0].object.position.z;
    return in_node(&word);
}
/* the recorded rise 2034 deck take with the leash armed until the given tick, for the pill base, velocity and place it leaves */
static void rise_take(long until, real_point3d *base, real_vector3d *velocity, real_point3d *place)
{
    struct distributed_on_foot_bound b;
    real height, width;
    load_rise_2034(); bound_wide(&b);
    distributed_accepted[0].transition_until = until;
    CHECK(distributed_take_riding(0, 0, &b, TRUE), "deck take refused with the leash until %ld", until);
    biped_get_physics_pill(0, base, &height, &width);
    *velocity = objects[0].object.translational_velocity;
    *place = in_node(base);
}
/* the gap is in the host's body frame, independent of the client's world copy: a word riding the deck
after a word airborne on it lands 0.12 off the host copy */
static real_point3d const landing_before = { 0.f, -0.20f, 0.42f };
static real_point3d const landing_word = { 0.f, -0.08f, 0.42f };
enum { landing_world, landing_leash, landing_along, landing_no_part, landing_other, landing_steady };
static void landing_seed(short route)
{
    setup();
    host_on(0, &landing_before);
    word_on(0, &landing_before, route == landing_along ? 59 : 60, TRUE);
    two_passes();
    CHECK(taken_now(), "landing seed refused");
    game_time++;
    riding[0].ticks = route == landing_world || route == landing_no_part || route == landing_other ? 59 : 60;
    host_on(0, &landing_before);
    word_on(route == landing_other ? 1 : 0, &landing_before, route == landing_steady ? 60 : 59,
        route != landing_no_part);
    if (route == landing_along)
        CHECK(distributed_accepted[0].transition_until <= game_time, "along fixture armed before take");
    two_passes();
    CHECK(taken_now(), "preceding word refused");
    if (route == landing_along)
        CHECK(distributed_accepted[0].transition_until <= game_time, "along fixture armed at take");
    if (route == landing_leash)
        CHECK(distributed_accepted[0].transition_leash_time == game_time, "preceding air word was not leashed");
    game_time++;
    riding[0].ticks = RIDING_TICKS;
    host_on(0, &landing_before);
    word_on(0, &landing_word, RIDING_TICKS, TRUE);
}
static void landing_base(real y)
{
    real_point3d base;
    real h, w;
    biped_get_physics_pill(0, &base, &h, &w);
    CHECK(fabsf(base.x) < 1e-5f && fabsf(base.y-y) < 1e-5f && fabsf(base.z-0.42f) < 1e-5f,
        "landing base (%f,%f,%f), expected (0,%f,.42)", base.x, base.y, base.z, y);
}
/* an air word of V taken, then the host copy still in V's window (59) while the client already rides (60): its word
is taken by the leash, and the deck word of the next tick follows. The word the leash takes is the riding word of
slot / ticks / part */
static void landing_riding_leash(long slot, short ticks, boolean part)
{
    landing_seed(landing_world);
    CHECK(distributed_accepted[0].taken_air_vehicle == V, "air word of V not recorded before the leash take");
    riding[0].ticks = 59;
    host_on(0, &landing_before);
    distributed_accepted[0].transition_until = game_time + 9;
    /* (a leash the host's own change armed, the word already on the deck: a client change landing in it would end the leash) */
    distributed_accepted[0].transition_client_ticks = RIDING_TICKS; distributed_accepted[0].transition_client_vehicle = V;
    word_on(slot, &landing_before, ticks, part);
    two_passes();
    CHECK(taken_now() && distributed_accepted[0].transition_leash_time == game_time, "the riding word was not leash-taken (taken %d leash time %ld at %ld until %ld)", taken_now(), distributed_accepted[0].transition_leash_time, game_time, distributed_accepted[0].transition_until);
    game_time++;
    riding[0].ticks = RIDING_TICKS;
    host_on(0, &landing_before);
    word_on(0, &landing_word, RIDING_TICKS, TRUE);
}
static int landing_case(char const *case_name)
{
    CASE("host-landing-deck-take-exact")
    {
        long rebases;
        landing_seed(landing_world); rebases = rebase_calls;
        distributed_predictions[0].state.deck.velocity = vc(0.01f, 0.02f, -0.03f);
        two_passes();
        CHECK(taken_now(), "landing deck word refused");
        landing_base(-0.08f);
        CHECK(rebase_calls == rebases+1 && fabsf(riding[0].local_point.y+0.08f) < 1e-5f,
            "landing deck take did not rebase at the word");
        CHECK(fabsf(objects[0].object.translational_velocity.i-0.343f) < 1e-5f &&
            fabsf(objects[0].object.translational_velocity.j-0.02f) < 1e-5f &&
            fabsf(objects[0].object.translational_velocity.k+0.03f) < 1e-5f, "landing velocity changed");
        return 0;
    }
    CASE("host-landing-after-leash-exact")
    {
        landing_seed(landing_leash); two_passes();
        CHECK(taken_now(), "landing after leash refused"); landing_base(-0.08f); return 0;
    }
    CASE("host-landing-after-along-exact")
    {
        landing_seed(landing_along); two_passes();
        CHECK(taken_now(), "landing after along refused"); landing_base(-0.08f); return 0;
    }
    CASE("host-landing-second-deck-take-blends")
    {
        real_point3d next = { 0.f, -0.06f, 0.42f };
        landing_seed(landing_world); two_passes(); CHECK(taken_now(), "first deck take refused");
        game_time++; host_on(0, &landing_word); word_on(0, &next, RIDING_TICKS, TRUE); two_passes();
        CHECK(taken_now(), "second deck take refused"); landing_base(-0.07f); return 0;
    }
    CASE("host-deck-take-steady-blends")
    {
        landing_seed(landing_steady); two_passes();
        CHECK(taken_now(), "steady deck word refused"); landing_base(-0.14f); return 0;
    }
    CASE("host-deck-after-no-part-blends")
    {
        landing_seed(landing_no_part); two_passes();
        CHECK(taken_now(), "deck after no part refused"); landing_base(-0.14f); return 0;
    }
    CASE("host-deck-after-other-vehicle-air-blends")
    {
        landing_seed(landing_other); two_passes();
        CHECK(taken_now(), "deck after other vehicle refused"); landing_base(-0.14f); return 0;
    }
    CASE("host-landing-word-refused")
    {
        real_point3d before, anchor;
        long record_unit, record_vehicle;
        landing_seed(landing_world);
        anchor = distributed_accepted[0].place;
        distributed_accepted[0].place.y -= 2.f;
        before = objects[0].object.position;
        record_unit = distributed_accepted[0].taken_air_unit_index;
        record_vehicle = distributed_accepted[0].taken_air_vehicle;
        two_passes();
        CHECK(!taken_now(), "landing bypassed anchor");
        CHECK(!memcmp(&before, &objects[0].object.position, sizeof(before)), "refused landing moved copy");
        CHECK(distributed_accepted[0].valid, "host-moved reset bypassed anchor refusal");
        CHECK(distributed_accepted[0].taken_air_unit_index == record_unit &&
            distributed_accepted[0].taken_air_vehicle == record_vehicle, "refusal changed air record");
        distributed_accepted[0].place = anchor;
        game_time++; word_on(0, &landing_word, RIDING_TICKS, TRUE); two_passes();
        CHECK(taken_now(), "valid landing after refusal refused"); landing_base(-0.08f); return 0;
    }
    CASE("host-landing-beyond-accept-refused")
    {
        real_point3d far = { 3.6f, -0.20f, 0.42f }, before;
        landing_seed(landing_world); before = objects[0].object.position;
        /* (only the immediate accept bound refuses this word, not the accumulated anchor) */
        distributed_accepted[0].place = far;
        word_on(0, &far, RIDING_TICKS, TRUE); two_passes();
        CHECK(!taken_now(), "landing bypassed accept tolerance");
        CHECK(!memcmp(&before, &objects[0].object.position, sizeof(before)), "far landing moved copy"); return 0;
    }
    CASE("host-landing-record-from-taken-only")
    {
        setup(); host_on(0, &landing_before); word_on(0, &landing_before, RIDING_TICKS, TRUE); two_passes();
        CHECK(taken_now() && distributed_accepted[0].taken_air_vehicle == NONE, "riding seed recorded as air");
        game_time++; riding[0].ticks = 59; objects[0].biped.flags = 1 << _biped_airborne_bit;
        /* (the copy is above its old takeoff ground; the close word passes the anchor but not the ceiling) */
        distributed_host_speeds[0].ground_height -= 4.f;
        word_on(0, &landing_before, 59, TRUE); two_passes();
        CHECK(!taken_now(), "air word above ceiling taken");
        CHECK(distributed_accepted[0].taken_air_vehicle == NONE, "refused air word changed record");
        game_time++; riding[0].ticks = 60; objects[0].biped.flags = 0;
        host_on(0, &landing_before); word_on(0, &landing_word, RIDING_TICKS, TRUE); two_passes();
        CHECK(taken_now(), "deck after refused air word refused"); landing_base(-0.14f); return 0;
    }
    CASE("host-landing-new-life-blends")
    {
        landing_seed(landing_world);
        objects[1] = objects[0]; riding[1] = riding[0]; players[0].unit_index = 1;
        distributed_predictions[0].state.unit_index = 1;
        two_passes();
        CHECK(taken_now(), "new life deck word refused");
        CHECK(fabsf(objects[1].object.position.y+0.14f) < 1e-5f, "new life inherited air record"); return 0;
    }
    CASE("host-landing-new-life-after-leash-blends")
    {
        /* (the state is synthetic and pins the conjunct: U1's air word of V is the record and U2's first take is the leash
        take of its riding word of V, not U1's landing. A new life whose first take is a window-route leash take is not
        reachable) */
        landing_seed(landing_world);
        riding[0].ticks = 59;
        host_on(0, &landing_before);
        distributed_accepted[0].transition_until = game_time + 9;
        /* (a leash the host's own change armed, the word already on the deck: a client change landing in it would end the leash) */
        distributed_accepted[0].transition_client_ticks = RIDING_TICKS; distributed_accepted[0].transition_client_vehicle = V;
        word_on(0, &landing_before, RIDING_TICKS, TRUE);
        objects[1] = objects[0]; riding[1] = riding[0]; players[0].unit_index = 1;
        distributed_predictions[0].state.unit_index = 1;
        /* (the new life is noted by the host's transition already, as one past its first tick: the leash stays armed) */
        distributed_host_speeds[0].unit_index = 1; distributed_accepted[0].transition_unit_index = 1; distributed_accepted[0].unit_index = 1;
        two_passes();
        CHECK(taken_now() && distributed_accepted[0].transition_leash_time == game_time, "the new life riding word was not leash-taken (taken %d leash %ld now %ld until %ld)", taken_now(), distributed_accepted[0].transition_leash_time, game_time, distributed_accepted[0].transition_until);
        game_time++;
        riding[0].ticks = RIDING_TICKS;
        host_on(0, &landing_before);
        objects[1] = objects[0]; riding[1] = riding[0];
        word_on(0, &landing_word, RIDING_TICKS, TRUE);
        distributed_predictions[0].state.unit_index = 1;
        two_passes();
        CHECK(taken_now(), "new life deck word refused");
        CHECK(fabsf(objects[1].object.position.y+0.14f) < 1e-5f, "new life's first deck take inherited the old life's air record: y %f",
            objects[1].object.position.y);
        return 0;
    }
    CASE("host-landing-switch-off-unchanged")
    {
        real before;
        landing_seed(landing_world); riding_applies = FALSE;
        before = objects[0].object.position.y;
        distributed_predictions[0].state.position = objects[0].object.position;
        distributed_predictions[0].state.position.y += 0.12f;
        two_passes();
        CHECK(taken_now(), "switch-off word refused");
        CHECK(fabsf(objects[0].object.position.y-before-0.06f) < 1e-5f, "switch-off world take changed"); return 0;
    }
    CASE("host-landing-after-riding-leash-exact")
    {
        landing_riding_leash(0, RIDING_TICKS, TRUE);
        CHECK(distributed_accepted[0].taken_air_unit_index == 0 && distributed_accepted[0].taken_air_vehicle == V,
            "the leash take of V's riding word consumed the air record");
        two_passes();
        CHECK(taken_now(), "deck word after the riding leash take refused"); landing_base(-0.08f); return 0;
    }
    CASE("host-landing-leash-other-word-consumes")
    {
        long variant;
        for (variant = 0; variant < 2; variant++)
        {
            /* (another vehicle's riding word, then a word with no part: neither is the landing's word) */
            landing_riding_leash(variant == 0 ? 1 : 0, RIDING_TICKS, variant == 0);
            CHECK(distributed_accepted[0].taken_air_vehicle == NONE, "leash take of a word that does not ride V kept the record (variant %ld)", variant);
            two_passes();
            CHECK(taken_now(), "deck word refused (variant %ld)", variant); landing_base(-0.14f);
        }
        return 0;
    }
    CASE("host-landing-riding-leash-consumed-once")
    {
        real_point3d next = { 0.f, -0.06f, 0.42f };
        landing_riding_leash(0, RIDING_TICKS, TRUE); two_passes();
        CHECK(taken_now(), "first deck take refused"); landing_base(-0.08f);
        CHECK(distributed_accepted[0].taken_air_vehicle == NONE, "the deck take kept the air record");
        game_time++; host_on(0, &landing_word); word_on(0, &next, RIDING_TICKS, TRUE); two_passes();
        CHECK(taken_now(), "second deck take refused"); landing_base(-0.07f); return 0;
    }
    CASE("host-landing-128")
    {
        long p;
        setup();
        for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
        {
            objects[p] = objects[0]; players[p].unit_index = p; riding[p] = riding[0]; riding[p].ticks = 59;
            host_tick_at(p, &landing_before); prediction(p, landing_before);
            distributed_predictions[p].state.deck.ticks = 59;
            distributed_predictions[p].state.position = objects[p].object.position;
        }
        two_passes(); game_time++;
        for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
        {
            riding[p].ticks = RIDING_TICKS; host_tick_at(p, &landing_before); prediction(p, landing_word);
        }
        two_passes();
        for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
        {
            CHECK(distributed_predictions[p].taken_host_time == game_time && apply_calls[p] == 2, "lost landing player %ld", p);
            CHECK(fabsf(objects[p].object.position.y+0.08f) < 1e-5f, "player %ld landing blended", p);
        }
        return 0;
    }
    return 1;
}
int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    struct distributed_on_foot_bound b;
    setup();
    CASE("host-along-rising") { disagree(FALSE, FALSE); return 0; }
    CASE("host-along-jump-takeoff")
    {
        long leads[] = {2, 3, 1};
        real across[] = {0.15f, 0.225f, 0.075f};
        real rise[] = {0.2128f, 0.3138f, 0.1082f};
        long sample;
        for (sample = 0; sample < 3; sample++)
        {
            real_point3d base;
            real height, width;
            setup(); accepted_once(); game_time = 2; host_tick(0);
            world_word(across[sample], rise[sample]);
            distributed_predictions[0].time = 1 + leads[sample];
            air_part_at_word(RIDING_TICKS - leads[sample]);
            distributed_predictions[0].state.velocity.i = (short)(0.075f * DISTRIBUTED_VELOCITY_SCALE);
            distributed_predictions[0].state.velocity.k = (short)((0.11f - 0.0036f*leads[sample]) * DISTRIBUTED_VELOCITY_SCALE);
            if (sample < 2)
                CHECK(distance_squared3d(&distributed_predictions[0].state.position, &objects[0].object.position) > 0.0625f,
                    "running takeoff must be outside default leash");
            CHECK(take(&b), "legitimate running takeoff refused with lead %ld", leads[sample]);
            biped_get_physics_pill(0, &base, &height, &width);
            CHECK(fabsf(base.z - rise[sample]) < 1e-5f, "running takeoff rise flattened: lead=%ld rise=%f expected=%f",
                leads[sample], base.z, rise[sample]);
            CHECK(fabsf(base.x - body_place.x - across[sample]) < 1e-5f, "running takeoff across changed: lead=%ld x=%f",
                leads[sample], base.x - body_place.x);
            CHECK(distributed_accepted[0].host_time == 1, "takeoff restarted deck anchor");
            printf("running takeoff lead=%ld: across=%.6f rise=%.6f\n", leads[sample], base.x - body_place.x, base.z);
        }

        /* (positive motion still belongs to the accumulated anchor and ceiling) */
        setup(); accepted_once(); game_time = 2; host_tick(0);
        world_word(0.15f, 0.2128f);
        distributed_predictions[0].time = 3;
        air_part_at_word(58);
        distributed_accepted[0].place.x -= 3.f;
        CHECK(!take(&b), "running takeoff bypassed accumulated anchor");
        check_base(to_world(&body_place));
        setup(); accepted_once(); game_time = 21; host_tick(0);
        world_word(0.15f, 3.1f);
        air_part_at_word(58);
        CHECK(!take(&b), "positive along rise bypassed deck ceiling");
        check_base(to_world(&body_place));
        return 0;
    }
    CASE("host-along-air-part")
    {
        short ticks[] = {59, 40, 1, 60};
        long t;
        real_point3d base;
        real height, width;
        accepted_once();
        for (t = 0; t < 4; t++)
        {
            game_time = 2 + t;
            vehicles[0].position.z = 0.1f * t;
            host_tick(0); world_word(0.05f, -0.3f);
            air_part_at_word(ticks[t]);
            if (t == 3)
                distributed_predictions[0].state.deck.vehicle_index = OTHER;
            CHECK(take(&b), "host-riding/client-air disagreement refused at %ld", t);
            biped_get_physics_pill(0, &base, &height, &width);
            CHECK(fabsf(base.z - 0.1f*t) < 1e-5f, "present part took host under rising deck: ticks=%d height=%f",
                ticks[t], base.z - 0.1f*t);
            CHECK(base.x > body_place.x, "present part discarded across motion");
            CHECK(distributed_accepted[0].host_time == 1, "disagreeing client part reset deck anchor");
        }
        return 0;
    }
    CASE("host-along-air-part-place")
    {
        struct distributed_on_foot_bound b;
        real_point3d base, expected;
        real height, width;
        accepted_once();
        game_time = 5;
        host_tick(0);
        prediction(0, body_place);
        distributed_predictions[0].state.deck.ticks = RIDING_TICKS - 1;
        distributed_predictions[0].state.deck.place.x += 0.001f;
        /* (the client's copy of the vehicle stood .27 behind the host's: its world word is that far off the deck place) */
        distributed_predictions[0].state.position = to_world(&body_place);
        distributed_predictions[0].state.position.z += 0.1f;
        distributed_predictions[0].state.position.x += 0.27f;
        expected = to_world(&distributed_predictions[0].state.deck.place);
        CHECK(take(&b), "a part whose place is on the deck refused");
        biped_get_physics_pill(0, &base, &height, &width);
        CHECK(distance_squared3d(&base, &expected) < 1e-4f, "the world word moved the rider off its place: (%f,%f,%f) not (%f,%f,%f)",
            base.x, base.y, base.z, expected.x, expected.y, expected.z);
        return 0;
    }
    CASE("host-along-air-part-limits")
    {
        struct distributed_on_foot_bound b;
        real_point3d base, before;
        real height, width;
        long variant;
        for (variant = 0; variant < 4; variant++)
        {
            setup(); accepted_once();
            game_time = 5;
            host_tick(0);
            prediction(0, body_place);
            distributed_predictions[0].state.position = to_world(&body_place);
            distributed_predictions[0].state.position.z += 0.1f;
            /* (off the host's by more than the leash, as the client's copy of the vehicle is) */
            distributed_predictions[0].state.position.x += 0.27f;
            distributed_predictions[0].state.deck.ticks = RIDING_TICKS - 1;
            biped_get_physics_pill(0, &before, &height, &width);
            if (variant == 0)
                distributed_predictions[0].state.deck.place.x += 3.6f;
            if (variant == 1)
                distributed_predictions[0].state.deck.place.x += 2.5f;
            if (variant == 2)
            {
                distributed_predictions[0].state.deck.vehicle_index = OTHER;
                distributed_predictions[0].state.deck.place.x += 3.6f;
            }
            if (variant == 3)
            {
                /* (a place in another node's frame is no place in the host's) */
                distributed_predictions[0].state.deck.node_index = 1;
                distributed_predictions[0].state.deck.place.x += 3.6f;
            }
            if (variant < 2)
            {
                CHECK(!take(&b), "a part's place %s refused: taken", variant ? "past the anchor's bound" : "3.6 wu off");
                biped_get_physics_pill(0, &base, &height, &width);
                CHECK(distance_squared3d(&base, &before) < 1e-8f, "a refused part moved the rider");
            }
            else
            {
                CHECK(take(&b), "a part of another vehicle or node read as a place on this one");
            }
        }
        return 0;
    }
    CASE("host-world-leash-velocity")
    {
        real_vector3d velocities[] = {{3, 0, 0}, {0, 0, 3}, {0, 0, -3}};
        real_vector3d zero = {0, 0, 0};
        long variant;
        for (variant = 0; variant < 3; variant++)
        {
            real_point3d target, before;
            setup(); accepted_once(); game_time = 2;
            riding[0].ticks = 0;
            objects[0].object.translational_velocity = velocities[variant];
            world_word(0.1f, 0); target = distributed_predictions[0].state.position;
            distributed_on_foot_bound(0, 0, &zero, &b);
            CHECK(b.speed < 3.f && distributed_on_foot_fall_speed(&b, target.z) < 3.f,
                "fixture must exceed the real ordinary bounds");
            distributed_take_prediction(0, 0, &b);
            CHECK(distributed_predictions[0].taken_host_time == game_time, "world leash word refused");
            CHECK(distributed_accepted[0].vehicle_index == NONE, "world leash kept a deck anchor");
            CHECK(distance_squared3d(&objects[0].object.position, &target) < 1e-10f, "world leash did not take position whole");
            CHECK(!memcmp(&objects[0].object.translational_velocity, &velocities[variant], sizeof(zero)),
                "world leash clamped host velocity variant %ld: (%f,%f,%f)", variant,
                objects[0].object.translational_velocity.i, objects[0].object.translational_velocity.j,
                objects[0].object.translational_velocity.k);

            /* (the leash is still subject to the ordinary world's accumulated check) */
            game_time++; world_word(0.1f, 0); before = objects[0].object.position;
            distributed_accepted[0].position.x -= 3.f;
            distributed_take_prediction(0, 0, &b);
            CHECK(distributed_predictions[0].taken_host_time != game_time, "world leash skipped anchor");
            CHECK(!memcmp(&objects[0].object.position, &before, sizeof(before)), "refused anchor word moved host");
            distributed_accepted[0].position = before;

            /* (a close world word cannot waive the height check either) */
            game_time++; world_word(0, 0.1f); b.rise = 0.f;
            distributed_take_prediction(0, 0, &b);
            CHECK(distributed_predictions[0].taken_host_time != game_time, "world leash skipped ceiling");
            CHECK(!memcmp(&objects[0].object.position, &before, sizeof(before)), "refused ceiling word moved host");
        }
        return 0;
    }
    CASE("host-riding-off")
    {
        long t;
        riding_applies = FALSE;
        riding[0].ticks = 0;
        world_word(0, 0); CHECK(take(&b), "off seed refused");
        for (t = 1; t <= 2; t++)
        {
            real before = objects[0].object.position.z;
            game_time++; world_word(0, -0.2f);
            distributed_predictions[0].state.deck_present = t == 1;
            CHECK(take(&b), "off word refused");
            CHECK(fabsf(objects[0].object.position.z - before + 0.1f) < 1e-5f, "riding off changed original world blend");
        }
        return 0;
    }
    CASE("host-transition-128")
    {
        long p, tick;
        for (p = 1; p < MAXIMUM_TRACKED_PLAYERS; p++)
        {
            objects[p] = objects[0]; objects[p].unit.player_index = 0x30000 | p;
            players[p].unit_index = p; riding[p] = riding[0]; prediction(p, body_place);
        }
        two_passes();
        for (tick = 2; tick <= 12; tick++)
        {
            game_time = tick;
            for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
            {
                host_tick(p); prediction(p, body_place);
                distributed_predictions[p].state.deck_present = FALSE;
                distributed_predictions[p].state.position = objects[p].object.position;
                distributed_predictions[p].state.position.z -= 0.2f;
            }
            two_passes();
            for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
            {
                CHECK(distributed_predictions[p].taken_host_time == tick, "lost player %ld at %ld", p, tick);
                CHECK(fabsf(objects[p].object.position.z - (tick <= 10 ? -0.1f : 0.1f)) < 1e-5f,
                    "wrong transition/along target for player %ld at %ld", p, tick);
            }
        }
        return 0;
    }
    CASE("host-transition-no-word")
    {
        accepted_once(); game_time = 2; world_word(0, -0.2f); CHECK(take(&b), "client episode refused");
        game_time = 12; host_tick(0); world_word(0, -0.2f); CHECK(take(&b), "expired episode refused");
        check_base(to_world(&body_place));
        distributed_predictions[0].valid = FALSE;
        game_time = 13; riding[0].ticks = 59; two_passes();
        game_time = 14; riding[0].ticks = 60; host_tick(0); two_passes();
        game_time = 25; host_tick(0); world_word(0, -0.2f);
        distributed_predictions[0].state.deck_present = TRUE;
        CHECK(take(&b), "client episode after quiet host transition refused");
        /* the entitlement is restored (the transition is armed again), but a deck word of the host's own vehicle is never leashed, so
        the take is the deck's, in the host's frame, not the client's world word below it */
        CHECK(distributed_accepted[0].transition_until == 25 + 9, "quiet host change did not restore client entitlement: until %ld",
            distributed_accepted[0].transition_until);
        check_base(to_world(&body_place));
        return 0;
    }
    CASE("host-along-every-axis") { disagree(TRUE, FALSE); return 0; }
    CASE("host-along-every-axis-air-part") { disagree(TRUE, TRUE); return 0; }
    CASE("host-world-client-rides")
    {
        real_point3d p = body_place, expected;
        riding[0].ticks = 0; riding[0].velocity.i = 0;
        p.x += 0.6f; prediction(0, p); expected = to_world(&p);
        CHECK(take(&b), "client-only riding word refused");
        check_base(expected);
        CHECK(distributed_accepted[0].vehicle_index == NONE, "world word created a deck anchor");
        game_time++; p.x += 2; prediction(0, p);
        CHECK(!take(&b), "client-only rider bypassed world anchor");
        vehicles[0].object_index += 0x10000; game_time++; world_word(0, 0);
        distributed_predictions[0].state.deck_present = TRUE;
        CHECK(take(&b), "gone client vehicle did not fall back to world");
        return 0;
    }
    CASE("host-air-bound")
    {
        long foot, t;
        for (foot = 0; foot <= 1; foot++)
        {
            long taken = 0;
            setup(); riding[0].velocity.i = 0.3f; host_tick(0); accepted_once();
            riding[0].ticks = foot ? 0 : 40;
            if (foot) riding[0].velocity.i = 0;
            objects[0].biped.flags = 1 << _biped_airborne_bit;
            for (t = 1; t <= 20; t++)
            {
                game_time = 1 + t; objects[0].biped.airborne_ticks = t;
                objects[0].object.translational_velocity.i = riding[0].velocity.i;
                world_word(0.45f, 0);
                distributed_predictions[0].state.position.x = body_place.x + 0.45f*t;
                if (take(&b)) taken++;
            }
            CHECK(foot ? taken < 20 : taken == 20, "air bound foot=%ld: %ld/20", foot, taken);
            if (!foot)
            {
                real_point3d from = {0,0,0}, to = {2.55f,0,0};
                b.speed = 2.3f;
                CHECK(distributed_on_foot_move_valid(&b, &from, &to, 1), "air reach recapped below frame allowance");
                CHECK(distributed_on_foot_fall_speed(&b, -1000) > 2.29f, "air fall cap not widened");
            }
        }
        return 0;
    }
    CASE("host-air-throw")
    {
        riding[0].ticks = 40; riding[0].velocity.i = 0.3f;
        stable_window(); objects[0].biped.flags = 1 << _biped_airborne_bit;
        objects[0].biped.airborne_ticks = 1;
        objects[0].object.translational_velocity = riding[0].velocity;
        distributed_host_speeds[0].flight_speed = 0.6f;
        distributed_on_foot_bound(0, 0, &riding[0].velocity, &b);
        CHECK(fabsf(b.speed - 0.9f) < 1e-6f, "throw lost or frame not added: %f", b.speed);
        distributed_host_speeds[0].flight_speed = 4.0f;
        distributed_on_foot_bound(0, 0, &riding[0].velocity, &b);
        CHECK(fabsf(b.speed - 2.3f) < 1e-6f, "throw cap must precede frame: %f", b.speed);
        return 0;
    }
    CASE("host-leash")
    {
        real_point3d target;
        real_vector3d velocity;
        long variant;
        for (variant = 0; variant < 3; variant++)
        {
            setup(); accepted_once(); game_time = 2;
            riding[0].ticks = 59; objects[0].biped.flags = 1 << _biped_airborne_bit;
            objects[0].biped.airborne_ticks = 1;
            world_word(0.0f, variant == 1 ? -0.3f : -0.2f);
            target = distributed_predictions[0].state.position;
            velocity = objects[0].object.translational_velocity;
            distributed_on_foot_bound(0, 0, &riding[0].velocity, &b);
            b.speed = 0.01f;
            distributed_accepted[0].place.z = 0.1f;
            if (variant == 2)
            {
                /* (close to the host is not close enough to its accumulated anchor) */
                distributed_accepted[0].place.x -= 3.0f;
                distributed_accepted[0].position.x -= 3.0f;
            }
            distributed_take_prediction(0, 0, &b);
            if (variant == 2)
                CHECK(distributed_predictions[0].taken_host_time != game_time, "leash bypassed accumulated anchor");
            else if (variant == 0)
            {
                CHECK(distance_squared3d(&target, &objects[0].object.position) < 1e-10f, "leash did not take whole position");
                CHECK(!memcmp(&velocity, &objects[0].object.translational_velocity, sizeof(velocity)), "leash changed host velocity");
            }
            else
            {
                /* (outside the leash the normal world accumulated check still applies) */
                CHECK(distance_squared3d(&target, &objects[0].object.position) > 1e-8f, "outside-leash word committed whole");
            }
        }
        setup(); accepted_once(); game_time = 2; riding[0].ticks = 59;
        world_word(0, 0); CHECK(take(&b), "takeoff seed refused");
        game_time = 3; riding[0].ticks = 60; host_tick(0); world_word(0, 0);
        CHECK(take(&b), "landing seed refused");
        game_time = 13; host_tick(0); world_word(0, -0.2f);
        CHECK(take(&b), "expired leash along word refused");
        check_base(to_world(&body_place));
        return 0;
    }
    CASE("host-leash-toggle")
    {
        long t, whole = 0, renewed = 0;
        accepted_once();
        for (t = 1; t <= 75; t++)
        {
            boolean part = t <= 30 ? (t & 1) == 0 : ((t - 31) / 9) % 2 == 0;
            game_time = 1 + t; host_tick(0); world_word(0, -0.2f);
            distributed_predictions[0].state.deck_present = part;
            CHECK(take(&b), "toggle refused legal word at %ld", t);
            if (objects[0].object.position.z < 0.09999f) whole++;
            if (t > 9)
                CHECK(fabsf(objects[0].object.position.z - 0.1f) < 1e-5f, "expired toggle took height at %ld", t);
        }
        CHECK(whole > 0 && whole <= 9, "client rearmed leash: %ld/75", whole);
        game_time++; riding[0].ticks = 59; world_word(0, 0); CHECK(take(&b), "takeoff refused");
        game_time++; riding[0].ticks = 60; host_tick(0); world_word(0, 0);
        distributed_predictions[0].state.deck_present = TRUE;
        CHECK(take(&b), "landing refused");
        game_time += 10; host_tick(0); prediction(0, body_place); CHECK(take(&b), "postlanding part refused");
        for (t = 1; t <= 12; t++)
        {
            game_time++; host_tick(0); world_word(0, -0.2f);
            CHECK(take(&b), "renewed episode refused");
            if (objects[0].object.position.z < 0.09999f) renewed++;
        }
        CHECK(renewed == 9, "host change did not allow one more episode: %ld", renewed);
        return 0;
    }
    CASE("host-deck-lagging")
    {
        CHECK(take(&b), "lagging word refused"); check_base(to_world(&body_place));
        CHECK(rebase_calls == 1 && distributed_accepted[0].vehicle_index == V, "missing deck transaction");
        game_time++; host_tick(0); prediction(0, body_place);
        CHECK(take(&b) && distributed_accepted[0].host_time == 1, "anchor reset after first word");
        return 0;
    }
    CASE("host-deck-relative-speed")
    {
        struct distributed_on_foot_bound rest, deck, foot;
        real_vector3d zero = { 0, 0, 0 }, frame = { 0.333f, 0, 0 };
        objects[0].object.translational_velocity = zero;
        distributed_on_foot_bound(0, 0, &zero, &rest);
        setup(); stable_window(); distributed_on_foot_bound(0, 0, &frame, &deck);
        setup(); riding[0].ticks = 0; distributed_on_foot_bound(0, 0, &zero, &foot);
        CHECK(fabsf(rest.speed - deck.speed) < 1e-6f, "carry inflates bound: %f vs %f", deck.speed, rest.speed);
        CHECK(foot.speed > deck.speed, "on-foot throw no longer counted"); return 0;
    }
    CASE("host-deck-advance-refused")
    {
        long tick, refusals = 0;
        for (tick = 1; tick < 30; tick++)
        {
            real_point3d place = body_place;
            game_time = tick; vehicles[0].position.x += 0.333f; host_tick(0);
            place.x += 0.333f * tick; prediction(0, place);
            distributed_predictions[0].state.deck.velocity.i = 0.333f;
            if (!take(&b)) refusals++;
            else CHECK(objects[0].object.translational_velocity.i - riding[0].velocity.i <= b.speed + 1e-6f,
                "unclamped deck velocity");
        }
        CHECK(refusals > 0, "double-speed deck advance never refused"); return 0;
    }
    CASE("host-deck-carried-anchor")
    {
        long tick, writes = 0, prior_time = NONE;
        for (tick = 1; tick <= 90; tick++)
        {
            game_time = tick; vehicles[0].position.x += 0.333f;
            /* (translation alone fits inside the one-tick jitter allowance;
            a turn carries the same body place beyond it, without a reset) */
            vehicles[0].forward.i = vehicles[0].body_forward.i = tick & 1 ? 1.0f : -1.0f;
            host_tick(0); prediction(0, body_place);
            CHECK(take(&b), "still rider refused at %ld", tick);
            if (distributed_accepted[0].host_time != prior_time)
            {
                CHECK(tick == 1 || tick == 31 || tick == 61, "unscheduled anchor at %ld", tick);
                writes++; prior_time = distributed_accepted[0].host_time;
            }
        }
        CHECK(writes == 3, "anchor writes %ld, expected 3", writes); return 0;
    }
    CASE("host-now-once")
    {
        accepted_once(); game_time = 2; host_tick(0); prediction(0, body_place);
        to_world_calls = 0;
        CHECK(take(&b), "still rider refused");
        /* (the host's own place, the anchor's host place and the anchor's place: one transform each) */
        CHECK(to_world_calls == 3, "%ld transforms to the world, expected 3", to_world_calls); return 0;
    }
    CASE("host-deck-vehicle-gone")
    {
        accepted_once(); rebase_calls = 0; game_time++; prediction(0, body_place);
        vehicles[0].object_index += 0x10000;
        distributed_predictions[0].state.position = objects[0].object.position;
        distributed_predictions[0].state.position.x += 0.5f;
        CHECK(take(&b), "gone vehicle did not use world path");
        CHECK(rebase_calls == 0, "rebased against stale salted vehicle");
        CHECK(objects[0].object.position.x == distributed_predictions[0].state.position.x, "world word not taken");
        return 0;
    }
    CASE("host-deck-after-vehicle-correction")
    {
        distributed_predictions[0].state.position.x -= 3.0f;
        vehicle_write = 3.0f; two_passes();
        CHECK(distributed_predictions[0].taken_host_time == game_time, "body-equivalent corrected word refused");
        check_base(to_world(&body_place)); CHECK(rebase_calls == 1, "rider was not rebased"); return 0;
    }
    CASE("host-pass-partition")
    {
        long p;
        riding[0].ticks = 0;
        objects[1] = objects[0]; objects[1].unit.player_index = 1; players[1].unit_index = 1;
        riding[1].object_index = NONE;
        prediction(1, body_place); distributed_predictions[1].state.deck_present = FALSE;
        distributed_predictions[1].state.position = objects[1].object.position;
        game_time = 200;
        two_passes();
        for (p = 0; p < 2; p++)
        {
            CHECK(apply_calls[p] == 1, "player %ld taken %ld times", p, apply_calls[p]);
            CHECK(distributed_host_speeds[p].time == 200, "player %ld bound not noted", p);
            CHECK(distributed_predictions[p].taken_host_time == 200, "player %ld word not taken", p);
        }
        CHECK(apply_phase[0] == 1 && apply_phase[1] == 0, "boarding and foot words in wrong passes"); return 0;
    }
    CASE("host-deck-node-equivalent")
    {
        struct vehicle_riding_vehicle node;
        real_point3d target;
        accepted_once();
        game_time++; prediction(0, body_place);
        target = to_world(&body_place); biped_riding_vehicle(V, 2, &node);
        vehicle_riding_point_to_local(&node.position, &node.forward, &node.up, &target,
            &distributed_predictions[0].state.deck.place);
        distributed_predictions[0].state.deck.node_index = 2;
        CHECK(take(&b), "equivalent node word refused"); check_base(target);
        CHECK(distributed_accepted[0].host_time == 1, "client-only node change resets anchor");
        riding[0].node_index = 2; biped_riding_rebase(0); game_time++;
        distributed_predictions[0].time = game_time; distributed_predictions[0].taken_host_time = NONE;
        CHECK(take(&b), "equivalent word after host node change refused"); check_base(target);
        CHECK(distributed_accepted[0].host_time == 1, "host node change resets anchor");
        game_time = 31; distributed_predictions[0].time = 31; distributed_predictions[0].taken_host_time = NONE;
        CHECK(take(&b), "equivalent word at scheduled renewal refused");
        CHECK(distance_squared3d(&distributed_accepted[0].place, &body_place) < 1e-10f,
            "renewed anchor is not in the body frame"); return 0;
    }
    CASE("host-deck-node-forgery")
    {
        accepted_once(); game_time = 21;
        vehicles[0].bounding_sphere_radius = 3.0f; prediction(0, body_place);
        distributed_predictions[0].state.deck.node_index = 2;
        CHECK(!take(&b), "node-local zero height evades body ceiling"); return 0;
    }
    CASE("host-deck-accelerating")
    {
        struct distributed_on_foot_bound rest;
        real_vector3d zero = { 0, 0, 0 };
        long tick;
        objects[0].object.translational_velocity = zero; riding[0].velocity = zero;
        distributed_on_foot_bound(0, 0, &zero, &rest);
        for (tick = 1; tick <= 30; tick++)
        {
            game_time = tick;
            riding[0].velocity.i = tick <= 10 ? 0.333f * tick / 10 : tick <= 20 ? 0.333f * (20-tick)/10 : 0.0f;
            riding[0].velocity.k = tick > 20 ? 0.1f * (tick-20)/10 : 0.0f;
            host_tick(0); prediction(0, body_place); two_passes();
            distributed_on_foot_bound(0, 0, &riding[0].velocity, &b);
            CHECK(fabsf(rest.speed-b.speed) < 1e-6f && fabsf(rest.rise-b.rise) < 1e-6f,
                "tick %ld carry acceleration inflates bound (%f,%f) vs (%f,%f)", tick,b.speed,b.rise,rest.speed,rest.rise);
        }
        /* (a nonzero relative motion must also survive acceleration: the
        taken excess is measured in its own tick's frame, not this one's) */
        setup(); stable_window();
        distributed_host_speeds[0].velocity.i += 0.3f;
        objects[0].object.translational_velocity.i = riding[0].velocity.i + 0.3f;
        distributed_on_foot_bound(0, 0, &riding[0].velocity, &rest);
        memset(distributed_host_speeds[0].speeds, 0, sizeof(distributed_host_speeds[0].speeds));
        riding[0].velocity.i += 0.0333f;
        objects[0].object.translational_velocity.i += 0.0333f;
        distributed_on_foot_bound(0, 0, &riding[0].velocity, &b);
        CHECK(fabsf(rest.speed-b.speed) < 1e-6f, "acceleration changes the taken relative excess");
        return 0;
    }
    CASE("apply-velocity-within-tolerance")
    {
        real_vector3d v = { 0.1f, 0, 0 };
        CHECK(distributed_apply_state(0, &distributed_predictions[0].state, &objects[0].object.position,
            &v, 0.1f, HOST_BLEND_DISTANCE, 0, 0), "valid override refused");
        CHECK(reconcile_calls == 0 && objects[0].object.translational_velocity.i == 0.1f,
            "velocity lost without position write");
        CHECK(distributed_apply_state(0, &distributed_predictions[0].state, &objects[0].object.position,
            NULL, 0.1f, HOST_BLEND_DISTANCE, 0, 0), "valid NULL override refused");
        CHECK(objects[0].object.translational_velocity.i == 0.1f && reconcile_calls == 0, "NULL override changes velocity");
        return 0;
    }
    CASE("host-deck-temporaries")
    {
        struct fake_object before;
        struct vehicle_riding_state before_riding;
        byte before_anchor[sizeof(distributed_accepted[0])];
        accepted_once(); game_time = 21;
        vehicles[0].position.x += 3.0f; host_tick(0); prediction(0, body_place);
        distributed_predictions[0].state.deck.place.z = 3.0f;
        before = objects[0]; before_riding = riding[0]; memcpy(before_anchor, &distributed_accepted[0], sizeof(before_anchor));
        CHECK(!take(&b), "ceiling violation taken");
        CHECK(!memcmp(&before, &objects[0], sizeof(before)), "rejected word changes unit");
        CHECK(!memcmp(&before_riding, &riding[0], sizeof(before_riding)), "rejected word changes riding");
        CHECK(!memcmp(before_anchor, &distributed_accepted[0], sizeof(before_anchor)), "rejected word changes anchor");
        return 0;
    }
    CASE("host-path-rules")
    {
        struct vehicle_riding_deck d = distributed_predictions[0].state.deck;
        CHECK(vehicle_riding_host_path(&riding[0], TRUE, &d, FALSE) == _vehicle_riding_host_deck, "same deck not selected");
        d.node_index = 2;
        CHECK(vehicle_riding_host_path(&riding[0], TRUE, &d, TRUE) == _vehicle_riding_host_deck, "node affects deck path");
        d.vehicle_index = OTHER;
        CHECK(vehicle_riding_host_path(&riding[0], TRUE, &d, FALSE) == _vehicle_riding_host_along, "other vehicle bypassed host-riding height");
        d.vehicle_index = V; d.ticks = RIDING_TICKS-1;
        CHECK(vehicle_riding_host_path(&riding[0], TRUE, &d, FALSE) == _vehicle_riding_host_along, "client air part bypassed host-riding height");
        riding[0].ticks--;
        CHECK(vehicle_riding_host_path(&riding[0], FALSE, &d, FALSE) == _vehicle_riding_host_world, "window air not world");
        return 0;
    }
    CASE("host-velocity-limits")
    {
        real_vector3d relative = { 3, 4, 12 }, frame = { 0.333f, 0, 0.1f }, world;
        vehicle_riding_host_velocity(&relative, &frame, 0.13f, 0.2f, &world);
        CHECK(fabsf(world.i-frame.i-0.03f) < 1e-6f && fabsf(world.j-0.04f) < 1e-6f &&
            fabsf(world.k-frame.k-0.12f) < 1e-6f, "relative up/across not clamped before carry");
        relative.k = -12;
        vehicle_riding_host_velocity(&relative, &frame, 0.1f, 0.2f, &world);
        CHECK(fabsf(world.i-frame.i-0.06f) < 1e-6f && fabsf(world.j-0.08f) < 1e-6f &&
            fabsf(world.k-frame.k+0.2f) < 1e-6f, "fall limit coupled to horizontal speed"); return 0;
    }
    CASE("host-world-new-life")
    {
        struct distributed_on_foot_bound fresh;
        real_vector3d zero = { 0, 0, 0 };
        riding[0].ticks = 0; objects[0].object.translational_velocity.k = 0.2f;
        distributed_on_foot_bound(0, 0, &zero, &fresh);
        setup(); riding[0].ticks = 0; objects[0].object.translational_velocity.k = 0.2f;
        distributed_host_speeds[0].unit_index = 1;
        distributed_host_speeds[0].velocity.k = 1.0f;
        distributed_host_speeds[0].frame_velocity.k = 0.7f;
        distributed_on_foot_bound(0, 0, &zero, &b);
        CHECK(fabsf(b.rise-fresh.rise) < 1e-6f && fabsf(b.speed-fresh.speed) < 1e-6f,
            "previous life's relative velocity changes a fresh world's bound"); return 0;
    }
    CASE("host-pass-128")
    {
        long p;
        for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
        {
            objects[p] = objects[0]; objects[p].unit.player_index = 0x30000 | p;
            players[p].unit_index = p; riding[p] = riding[0];
            prediction(p, body_place);
            if (p & 1) { riding[p].ticks = 0; distributed_predictions[p].state.deck_present = FALSE;
                distributed_predictions[p].state.position = objects[p].object.position; }
        }
        two_passes();
        for (p = 0; p < MAXIMUM_TRACKED_PLAYERS; p++)
            CHECK(apply_calls[p] == 1 && distributed_predictions[p].taken_host_time == game_time &&
                apply_phase[p] == !(p & 1), "128-player partition lost or repeated player %ld", p);
        return 0;
    }
    CASE("host-deck-anchor-walk")
    {
        /* (the host copy keeps where it was taken, so only the anchor can refuse) */
        real_point3d hp = body_place;
        long tick, taken = 0, refused_at = 0;
        for (tick = 1; tick <= 30; tick++)
        {
            real_point3d place = body_place, base;
            game_time = tick; host_tick_at(0, &hp);
            place.x += 0.6f * tick; prediction(0, place);
            if (take(&b)) taken++; else if (!refused_at) refused_at = tick;
            base = objects[0].object.position; base.z -= 0.1f;
            vehicle_riding_point_to_local(&vehicles[0].position, &vehicles[0].forward, &vehicles[0].up, &base, &hp);
        }
        CHECK(refused_at > 0 && refused_at <= 30 && taken < 30, "walking along the deck never refused");
        CHECK(hp.x - body_place.x <= b.speed * 30.0f, "walked %f along the deck in a second", hp.x - body_place.x);
        return 0;
    }
    CASE("host-deck-host-moved")
    {
        real_point3d moved = body_place;
        accepted_once(); game_time = 2;
        moved.x += 3.0f; host_tick_at(0, &moved); prediction(0, moved);
        CHECK(take(&b), "word on the host's own move refused");
        CHECK(distributed_accepted[0].host_time == 2, "host's own move kept the old anchor"); return 0;
    }
    CASE("host-deck-part-alternating")
    {
        weave(FALSE, 1, 0.0f); weave(FALSE, 1, 0.333f); return 0;
    }
    CASE("host-deck-window-switch")
    {
        weave(TRUE, 1, 0.333f); weave(TRUE, 2, 0.333f); weave(TRUE, 1, 0.0f); return 0;
    }
    CASE("host-deck-jump-descending")
    {
        /* (a jump from a deck that sinks under it: its height is the world's, not the deck's) */
        long t, taken = 0;
        riding[0].velocity.i = 0.0f; accepted_once();
        objects[0].biped.flags = 1 << _biped_airborne_bit; riding[0].ticks = RIDING_TICKS - 1;
        for (t = 1; t <= 20; t++)
        {
            game_time = 1 + t; vehicles[0].position.z = -0.1f * t;
            objects[0].object.position.z = 0.1f + 0.11f * t - 0.0018f * t * t;
            objects[0].object.translational_velocity.k = 0.11f - 0.0036f * t;
            objects[0].biped.airborne_ticks = t;
            riding[0].body_position = riding[0].vehicle_position = vehicles[0].position;
            air_tick(-0.1f, t <= 2);
            prediction(0, body_place);
            distributed_predictions[0].state.deck_present = FALSE;
            distributed_predictions[0].state.position = objects[0].object.position;
            if (take(&b)) taken++;
        }
        CHECK(taken == 20, "a legitimate jump taken %ld of 20 times", taken);
        printf("descending-after-takeoff jump: %ld/20\n", taken); return 0;
    }
    CASE("host-deck-jump-rising")
    {
        long speed, t, stopped;
        long taken[2] = { 0, 0 };
        for (stopped = 0; stopped <= 1; stopped++)
        {
            taken[0] = taken[1] = 0;
            for (speed = 1; speed <= 2; speed++)
            {
                real vk = 0.1f * speed;
                setup(); riding[0].velocity.i = 0.0f; riding[0].velocity.k = vk;
                host_tick(0); accepted_once();
                objects[0].biped.flags = 1 << _biped_airborne_bit; riding[0].ticks--;
                for (t = 1; t <= 30; t++)
                {
                    game_time = 1 + t; vehicles[0].position.z = stopped ? vk : vk * t;
                    objects[0].object.position.z = 0.1f + vk * t + 0.11f * t - 0.0018f * t * t;
                    objects[0].object.translational_velocity.k = vk + 0.11f - 0.0036f * t;
                    objects[0].biped.airborne_ticks = t;
                    riding[0].body_position = riding[0].vehicle_position = vehicles[0].position;
                    air_tick(stopped ? 0.0f : vk, TRUE);
                    prediction(0, body_place);
                    distributed_predictions[0].state.deck_present = FALSE;
                    distributed_predictions[0].state.position = objects[0].object.position;
                    two_passes();
                    if (distributed_predictions[0].taken_host_time == game_time) taken[speed-1]++;
                    CHECK(fabsf(objects[0].object.position.z - (0.1f + vk*t + 0.11f*t - 0.0018f*t*t)) < 1e-5f,
                        "legitimate rising jump moved off its path");
                }
            }
            CHECK(taken[0] == 30 && taken[1] == 30,
                "legitimate rising jumps taken vk=0.1: %ld/30, vk=0.2: %ld/30", taken[0], taken[1]);
            printf("rising jumps stopped=%ld: vk=0.1 %ld/30, vk=0.2 %ld/30\n", stopped, taken[0], taken[1]);
        }
        return 0;
    }
    CASE("host-deck-hover-descending")
    {
        hover(FALSE); hover(TRUE); return 0;
    }
    CASE("host-deck-hover-stopped")
    {
        hover(TRUE); return 0;
    }
    CASE("host-deck-window-ceiling")
    {
        struct fake_object before;
        riding[0].velocity.i = 0.0f; riding[0].velocity.k = -0.2f;
        host_tick(0); accepted_once();
        objects[0].biped.flags = 1 << _biped_airborne_bit; riding[0].ticks--;
        game_time = 16; vehicles[0].position.z = -3.0f;
        objects[0].object.position.z = 0.1f - 3.0f + 0.11f*15 - 0.0018f*15*15;
        objects[0].object.translational_velocity.k = -0.2f + 0.11f - 0.0036f*15;
        objects[0].biped.airborne_ticks = 15;
        riding[0].body_position = riding[0].vehicle_position = vehicles[0].position;
        prediction(0, body_place);
        distributed_predictions[0].state.deck_present = FALSE;
        distributed_predictions[0].state.position = objects[0].object.position;
        distributed_predictions[0].state.position.z = 0.1f - 3.0f + 3.1f;
        before = objects[0];
        CHECK(!take(&b), "window word above takeoff-frame jump apex taken");
        CHECK(!memcmp(&before, &objects[0], sizeof(before)), "refused window word changed unit");
        return 0;
    }
    CASE("host-anchor-omitted-parts")
    {
        /* (the host copy keeps where it was taken, so only the anchor can refuse a client that runs along the deck) */
        real_point3d hp = body_place;
        long tick, taken = 0, refused_at = 0, writes = 0, prior = NONE;
        setup();
        for (tick = 1; tick <= 30; tick++)
        {
            real_point3d place = body_place, base;
            game_time = tick; vehicles[0].position.x += 0.333f;
            host_tick_at(0, &hp);
            place.x += 0.4f * tick;
            /* (a valid part, none, an invalid one with no ticks) */
            word_on(0, &place, tick % 3 == 2 ? 0 : RIDING_TICKS, tick % 3 != 1);
            distributed_predictions[0].time = game_time;
            if (take(&b)) taken++; else if (!refused_at) refused_at = tick;
            if (distributed_accepted[0].host_time != prior)
            {
                writes++; prior = distributed_accepted[0].host_time;
            }
            base = objects[0].object.position; base.z -= 0.1f;
            vehicle_riding_point_to_local(&vehicles[0].position, &vehicles[0].forward, &vehicles[0].up, &base, &hp);
        }
        CHECK(writes == 1, "a missing or invalid part reset the anchor: %ld writes", writes);
        CHECK(refused_at > 0 && refused_at <= 30 && taken < 30, "a run along the deck was never refused");
        CHECK(hp.x - body_place.x <= b.speed * 30.0f, "walked %f along the deck in a second", hp.x - body_place.x);
        return 0;
    }
    CASE("host-anchor-named-resets")
    {
        /* (a deck that moves and turns every tick, so that a carry measured in the world would trip every tick) */
        static long const expected[] = { 1, 10, 20, 35, 55, 85 };
        real_point3d spot, ground, moved;
        long tick, writes = 0, prior = NONE;
        setup(); riding[0].velocity.i = 0.0f;
        vehicles[1].position.x = 10.0f;
        ground = world_on(0, &body_place); ground.y += 6.0f; ground.z = 0.1f;
        moved = body_place; moved.x += 2.0f;
        for (tick = 1; tick <= 90; tick++)
        {
            long slot = tick >= 35 && tick < 55 ? 1 : 0;
            game_time = tick; vehicles[0].position.x += 0.333f;
            vehicles[0].forward.i = vehicles[0].body_forward.i = tick & 1 ? 1.0f : -1.0f;
            spot = tick >= 20 ? moved : body_place;
            if (tick < 10 || tick >= 55)
            {
                /* (on foot: where the host left it, at first the ground it boards from) */
                if (tick == 1 || tick == 10)
                    objects[0].object.position = ground;
                off_deck();
                word_on(0, &spot, RIDING_TICKS, FALSE);
                distributed_predictions[0].state.position = objects[0].object.position;
            }
            else
            {
                riding[0].ticks = RIDING_TICKS;
                host_on(slot, &spot);
                word_on(slot, &spot, RIDING_TICKS, TRUE);
            }
            distributed_predictions[0].time = game_time;
            distributed_on_foot_bound(0, 0, &riding[0].velocity, &b);
            distributed_take_prediction(0, 0, &b);
            CHECK(taken_now(), "word refused at %ld", tick);
            if (distributed_accepted[0].host_time != prior)
            {
                CHECK(writes < 6 && expected[writes] == tick, "unnamed anchor write at %ld (write %ld)", tick, writes);
                writes++; prior = distributed_accepted[0].host_time;
            }
        }
        CHECK(writes == 6, "anchor writes %ld, expected 6", writes);
        return 0;
    }
    CASE("client-return-no-allowance")
    {
        /* (the client's word moves 1.5 wu toward its interior anchor, its own cabin's return: no reset, no allowance) */
        real_point3d returned = body_place;
        returned.x += 1.5f;
        accepted_once(); game_time = 2;
        host_tick_at(0, &body_place); prediction(0, returned);
        CHECK(!take(&b), "the client's own return of 1.5 wu taken");
        CHECK(distributed_accepted[0].host_time == 1, "the client's return reset the anchor");
        /* (the host's own recovery of 1.5 wu moves its copy: the anchor starts again, and the word from there is taken) */
        setup(); accepted_once(); game_time = 2;
        host_tick_at(0, &returned); prediction(0, returned);
        CHECK(take(&b), "the word from the host's own recovery refused");
        CHECK(distributed_accepted[0].host_time == 2, "the host's own recovery did not reset the anchor");
        return 0;
    }
    CASE("host-forged")
    {
        struct vehicle_riding_state before_riding;
        struct vehicle_riding_air before_air;
        struct vehicle_riding_cabin before_cabin;
        real_vector3d relative;
        long variant;
        for (variant = 0; variant < 2; variant++)
        {
            setup(); accepted_once(); game_time = 2; host_tick(0);
            host_air.peak = 1.0f; host_air.have_peak = 1;
            prediction(0, body_place);
            /* (the deck path, and the along path of a part in the air) */
            distributed_predictions[0].state.deck.ticks = variant ? RIDING_TICKS : 1;
            if (!variant)
                distributed_predictions[0].state.position = objects[0].object.position;
            distributed_predictions[0].state.deck.velocity.i = 5.0f;
            distributed_predictions[0].state.deck.velocity.k = 5.0f;
            distributed_predictions[0].state.deck.flags = 1 | (2 << 1);
            distributed_predictions[0].state.deck.peak = 30.0f;
            distributed_predictions[0].state.velocity.i = 0x7FFF;
            before_riding = riding[0]; before_air = host_air; before_cabin = host_cabin;
            CHECK(take(&b), "forged part word refused (variant %ld)", variant);
            CHECK(riding[0].ticks == before_riding.ticks && riding[0].falling == before_riding.falling &&
                !memcmp(&riding[0].velocity, &before_riding.velocity, sizeof(before_riding.velocity)),
                "host riding state taken from the part: ticks %d falling %d (variant %ld)", riding[0].ticks,
                riding[0].falling, variant);
            CHECK(!memcmp(&host_air, &before_air, sizeof(before_air)), "host air taken from the part");
            CHECK(!memcmp(&host_cabin, &before_cabin, sizeof(before_cabin)), "host cabin taken from the part");
            subtract_vectors3d(&objects[0].object.translational_velocity, &riding[0].velocity, &relative);
            CHECK(magnitude3d(&relative) <= b.speed + 1e-5f, "unit velocity relative to the deck %f above bound %f",
                magnitude3d(&relative), b.speed);
        }
        return 0;
    }
    CASE("host-parked-rub")
    {
        /* (the client's word moves a touch's push past what a player runs: the host made no such move) */
        real_point3d before;
        setup(); riding[0].ticks = 0; riding[0].velocity.i = 0.0f; vehicles[0].position.x = 2.0f;
        world_word(0, 0); CHECK(take(&b), "seed refused");
        before = objects[0].object.position;
        game_time = 2; world_word(0, 0);
        distributed_predictions[0].state.position.x += b.speed + PREDICTED_ON_FOOT_SPEED_MARGIN + HOST_BLEND_DISTANCE + 0.3f;
        CHECK(!take(&b), "a push past the run bound taken");
        CHECK(!memcmp(&before, &objects[0].object.position, sizeof(before)), "refused word moved the host copy");
        return 0;
    }
    CASE("host-push-not-made")
    {
        /* (the vehicle drove into the client's copy only: the host copy was not touched) */
        real_point3d before;
        setup(); riding[0].ticks = 0; riding[0].velocity.i = 0.0f;
        world_word(0, 0); CHECK(take(&b), "seed refused");
        before = objects[0].object.position;
        game_time = 2; vehicles[0].position.x += 1.0f; world_word(0, 0);
        distributed_predictions[0].state.position.x += 1.2f;
        CHECK(!take(&b), "a push the host did not make taken");
        CHECK(!memcmp(&before, &objects[0].object.position, sizeof(before)), "refused push moved the host copy");
        return 0;
    }
    CASE("host-window-braking-climb")
    {
        /* (the host copy in its window over a vehicle that brakes hard again and again, the client's words a little
        higher each tick than the host copy: the ground record resets only for the host's own reasons, never for the
        vehicle's motion, so the climb stays under the jump's ceiling) */
        static real const profiles[4] = { 0.5f, 1.0f, 1.5f, 3.0f };
        long profile, t, resets, taken;
        for (profile = 0; profile < 4; profile++)
        {
            real vx = 0.0f, vspeed = 0.5f, climbed = 0.0f, worst = 0.0f, z_free;
            long prior_ground = NONE;
            setup(); riding[0].velocity.i = 0.5f; vehicles[0].position.x = 0.0f; host_tick(0); accepted_once();
            objects[0].biped.flags = 1 << _biped_airborne_bit; riding[0].ticks = RIDING_TICKS - 1; riding[0].falling = 1;
            z_free = objects[0].object.position.z; resets = taken = 0;
            for (t = 1; t <= 40; t++)
            {
                real vz = 0.11f - 0.0036f * (real)t;
                real_vector3d velocity;
                game_time = 1 + t;
                /* (the vehicle's speed swings between two extremes each tick: a brake from the kept speed to the other way) */
                vspeed = t & 1 ? 0.5f : 0.5f - profiles[profile];
                vx += vspeed; vehicles[0].position.x = vx;
                /* (the host copy's own jump physics, its kept frame velocity across) */
                objects[0].object.position.x += 0.5f; objects[0].object.position.z += vz;
                objects[0].object.translational_velocity.i = 0.5f; objects[0].object.translational_velocity.k = vz;
                objects[0].biped.airborne_ticks = t;
                z_free += vz;
                riding[0].body_position = riding[0].vehicle_position = vehicles[0].position;
                prediction(0, body_place);
                distributed_predictions[0].state.deck_present = FALSE;
                distributed_predictions[0].state.position = objects[0].object.position;
                distributed_predictions[0].state.position.z += 0.15f;
                velocity = objects[0].object.translational_velocity;
                distributed_predictions[0].state.velocity.i = (short)(velocity.i * DISTRIBUTED_VELOCITY_SCALE);
                distributed_predictions[0].state.velocity.k = (short)(velocity.k * DISTRIBUTED_VELOCITY_SCALE);
                two_passes();
                if (taken_now()) taken++;
                if (distributed_host_speeds[0].ground_time != prior_ground)
                {
                    resets++; prior_ground = distributed_host_speeds[0].ground_time;
                }
                climbed = objects[0].object.position.z - z_free;
                if (climbed > worst) worst = climbed;
            }
            printf("braking profile %ld: taken %ld/40, ground resets %ld, climbed %f above its own jump\n",
                profile, taken, resets, worst);
            CHECK(resets <= 1, "the vehicle's motion reset the ground record %ld times (profile %ld)", resets, profile);
            CHECK(worst <= 1.5f, "climbed %f above its own jump by resets (profile %ld)", worst, profile);
        }
        return 0;
    }
    CASE("host-physics-board-rise-agreed")
    {
        real_point3d base, place;
        real height, width;
        load_rise_2034(); bound_wide(&b);
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 0, "the shared landing armed the transition until %ld", distributed_accepted[0].transition_until);
        CHECK(distributed_take_riding(0, 0, &b, TRUE), "deck take refused");
        biped_get_physics_pill(0, &base, &height, &width);
        vehicle_riding_point_to_local(&replay_node.position, &replay_node.forward, &replay_node.up, &base, &place);
        CHECK(fabsf(base.z - 1.00451f) < 1e-5f, "pill base z %f, not the deck's 1.00451", base.z);
        CHECK(fabsf(place.z - 0.314468f) < 1e-5f, "place in the node z %f, not 0.314468", place.z);
        return 0;
    }
    CASE("host-step-off-together-agreed")
    {
        real_point3d after = pt(38.24456787f, -90.22706604f, 0.49884292f);
        real_point3d position;
        load_brush_2240(); bound_wide(&b);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until == 0, "the shared step off armed the transition until %ld", distributed_accepted[0].transition_until);
        CHECK(distributed_take_riding(0, 0, &b, FALSE), "world take refused");
        position = objects[0].object.position;
        CHECK(fabsf(position.x - after.x) < 1e-5f && fabsf(position.y - after.y) < 1e-5f && fabsf(position.z - after.z) < 1e-5f,
            "position (%f,%f,%f) is not the half blend", position.x, position.y, position.z);
        CHECK(objects[0].object.translational_velocity.i == 0.0f && objects[0].object.translational_velocity.j == 0.0f &&
            objects[0].object.translational_velocity.k == 0.0f, "velocity (%f,%f,%f) is not the wire's",
            objects[0].object.translational_velocity.i, objects[0].object.translational_velocity.j, objects[0].object.translational_velocity.k);
        return 0;
    }
    CASE("host-physics-board-agreed")
    {
        long T = 100, L = 9, t;
        setup();
        /* (T: the copy on foot, a world-only word of the client's taken) */
        game_time = T; off_deck(); host_tick(0); world_word(0, 0);
        CHECK(take(&b), "world-only word refused at T");
        for (t = 1; t <= L + 11; t++)
        {
            /* (the host's physics boards the copy at T+1; the client's riding word comes with the end of the transition length) */
            game_time = T + t;
            riding[0].ticks = RIDING_TICKS; riding[0].object_index = V; riding[0].node_index = NONE;
            host_tick(0);
            if (t >= L + 1)
                word_on(0, &body_place, RIDING_TICKS, TRUE);
            else
                world_word(0, 0);
            CHECK(take(&b), "word refused at T+%ld", t);
            CHECK(distributed_accepted[0].transition_until <= game_time, "armed until %ld at T+%ld", distributed_accepted[0].transition_until, t);
            CHECK(!distributed_accepted[0].transition_client_spent, "entitlement spent at T+%ld", t);
            check_base(to_world(&body_place));
        }
        return 0;
    }
    CASE("host-physics-board-client-air-arms")
    {
        setup(); game_time = 500;
        transition_memory(0, NONE, RIDING_TICKS - 1, V); client_word(RIDING_TICKS - 1, V, TRUE);
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a boarding in the client's air armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-client-lands-in-host-air")
    {
        setup(); game_time = 500;
        /* (both copies in the jump over V; the client's word lands first, the host's physics lands the copy a few ticks on) */
        transition_memory(RIDING_TICKS - 1, V, RIDING_TICKS - 1, V); client_word(RIDING_TICKS, V, TRUE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until <= game_time, "a landing on the deck the host's copy jumps from armed until %ld",
            distributed_accepted[0].transition_until);
        CHECK(!distributed_accepted[0].transition_client_spent, "the landing spent the client's entitlement");
        return 0;
    }
    CASE("host-client-lands-ends-running-leash")
    {
        setup(); game_time = 500;
        /* (the client's own take-off arms its leash while the copy is in the air; a short hop later its word rides the deck again, the copy still in the air) */
        transition_memory(RIDING_TICKS - 1, V, RIDING_TICKS, V); client_word(RIDING_TICKS - 1, V, TRUE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "the take-off armed until %ld", distributed_accepted[0].transition_until);
        game_time = 503; client_word(RIDING_TICKS, V, TRUE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until <= game_time, "a landing left the take-off's leash running until %ld", distributed_accepted[0].transition_until);
        CHECK(distributed_accepted[0].transition_client_spent, "the landing gave the client's entitlement back");
        return 0;
    }
    CASE("host-client-lands-other-vehicle-arms")
    {
        setup(); game_time = 500;
        transition_memory(RIDING_TICKS - 1, V, RIDING_TICKS - 1, V); client_word(RIDING_TICKS, OTHER, TRUE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a landing on another vehicle armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-client-lands-host-riding-arms")
    {
        setup(); game_time = 500;
        transition_memory(RIDING_TICKS, V, RIDING_TICKS - 1, V); client_word(RIDING_TICKS, V, TRUE);
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a landing after the copy rides again armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-transition-real-drop")
    {
        setup(); game_time = 500;
        transition_memory(RIDING_TICKS, V, RIDING_TICKS, V); client_word(RIDING_TICKS, V, TRUE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a drop the client does not share armed until %ld", distributed_accepted[0].transition_until);
        transition_memory(RIDING_TICKS, V, 0, NONE); client_word(0, NONE, FALSE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a drop over a world-only word armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-step-off-other-vehicle-arms")
    {
        setup(); game_time = 500;
        transition_memory(RIDING_TICKS, V, RIDING_TICKS, V); client_word(RIDING_TICKS - 1, OTHER, TRUE);
        host_window(RIDING_TICKS - 1, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a step off with the part on another vehicle armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-board-world-word-stale-arms")
    {
        setup(); game_time = 500;
        transition_memory(0, NONE, 0, NONE); client_word(0, NONE, FALSE);
        distributed_accepted[0].taken_host_time = 500 - 9 - 1;
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 500 + 9, "a boarding long after the last word armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-board-ack-late-arms")
    {
        setup(); game_time = 500;
        transition_memory(0, NONE, 0, NONE); client_word(0, NONE, FALSE);
        distributed_accepted[0].taken_host_time = 499;
        host_window(RIDING_TICKS, V);
        game_time = 500 + 9 + 1; client_word(RIDING_TICKS, V, TRUE);
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == game_time + 9, "a riding word after the bound armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-board-ack-then-jump-arms")
    {
        setup(); game_time = 500;
        transition_memory(0, NONE, 0, NONE); client_word(0, NONE, FALSE);
        distributed_accepted[0].taken_host_time = 499;
        host_window(RIDING_TICKS, V);
        game_time = 503; client_word(RIDING_TICKS, V, TRUE);
        host_window(RIDING_TICKS, V);
        game_time = 505; client_word(RIDING_TICKS - 1, V, TRUE);
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 505 + 9, "a jump after the acknowledgement armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-board-ack-other-vehicle-arms")
    {
        setup(); game_time = 500;
        transition_memory(0, NONE, 0, NONE); client_word(0, NONE, FALSE);
        distributed_accepted[0].taken_host_time = 499;
        host_window(RIDING_TICKS, V);
        game_time = 503; client_word(RIDING_TICKS, OTHER, TRUE);
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 503 + 9, "a word riding another vehicle armed until %ld", distributed_accepted[0].transition_until);
        return 0;
    }
    CASE("host-flip-after-leash-write")
    {
        long run, until;
        for (run = 1; run <= 2; run++)
        {
            /* (T: a leash take of a world-only word at the host copy's place, its transition armed until T+9) */
            setup(); accepted_once(); game_time = 2; host_tick(0); world_word(0, 0);
            distributed_accepted[0].transition_until = 11; distributed_accepted[0].transition_client_spent = TRUE;
            CHECK(take(&b), "leash take refused");
            CHECK(distributed_accepted[0].transition_leash_time == 2, "the leash write at 2 was stamped %ld", distributed_accepted[0].transition_leash_time);
            until = distributed_accepted[0].transition_until;
            if (run == 1)
            {
                game_time = 3; host_tick(0); world_word(0, 0);
                host_window(RIDING_TICKS - 1, V);
                CHECK(distributed_accepted[0].transition_until == until, "the flip after the write armed until %ld", distributed_accepted[0].transition_until);
                CHECK(distributed_accepted[0].transition_client_spent, "the flip after the write reset the client's entitlement");
            }
            else
            {
                game_time = 3; host_tick(0); world_word(0, 0);
                host_window(RIDING_TICKS, V);
                CHECK(distributed_accepted[0].transition_leash_time == 2, "a no-take tick moved the stamp to %ld", distributed_accepted[0].transition_leash_time);
                game_time = 4; host_tick(0); world_word(0, 0);
                host_window(RIDING_TICKS - 1, V);
                CHECK(distributed_accepted[0].transition_until == 4 + 9, "the flip two ticks after the write armed until %ld", distributed_accepted[0].transition_until);
            }
        }
        return 0;
    }
    CASE("host-leash-world-write-stamped")
    {
        setup(); game_time = 500;
        off_deck(); host_tick(0); world_word(0, 0);
        transition_memory(0, NONE, 0, NONE); distributed_accepted[0].transition_until = 510;
        CHECK(take(&b), "world word refused");
        CHECK(distributed_accepted[0].transition_leash_time == 500, "the leash write on foot was stamped %ld", distributed_accepted[0].transition_leash_time);
        return 0;
    }
    CASE("host-agreement-touches-transition-only")
    {
        struct distributed_accepted_state before;
        struct vehicle_riding_state riding_before;
        struct distributed_host_speed speeds_before;
        size_t tail = offsetof(struct distributed_accepted_state, taken_air_unit_index);
        setup(); game_time = 500;
        distributed_accepted[0].position = pt(1, 2, 3); distributed_accepted[0].place = pt(4, 5, 6);
        distributed_accepted[0].taken_air_unit_index = 0; distributed_accepted[0].taken_air_vehicle = V;
        transition_memory(59, V, RIDING_TICKS, V); distributed_accepted[0].transition_until = 499;
        before = distributed_accepted[0]; riding_before = riding[0]; speeds_before = distributed_host_speeds[0];
        host_window(RIDING_TICKS, V);
        CHECK(distributed_accepted[0].transition_until == 0, "the shared landing was not agreed");
        CHECK(!memcmp(&before, &distributed_accepted[0], ANCHOR_BYTES), "the anchor changed");
        CHECK(!memcmp((char *)&before + tail, (char *)&distributed_accepted[0] + tail, sizeof(before) - tail), "the air record changed");
        CHECK(!memcmp(&riding_before, &riding[0], sizeof(riding_before)), "the riding state changed");
        CHECK(!memcmp(&speeds_before, &distributed_host_speeds[0], sizeof(speeds_before)), "the host's speeds changed");
        return 0;
    }
    CASE("host-leash-keeps-support")
    {
        real_point3d armed_base, free_base, armed_place, free_place, before, deck_place = pt(0.83487767f, 0.02975389f, 0.31906742f);
        real_vector3d armed_velocity, free_velocity;
        rise_take(2040, &armed_base, &armed_velocity, &armed_place);
        rise_take(0, &free_base, &free_velocity, &free_place);
        CHECK(fabsf(free_base.z - 1.00451f) < 1e-5f && fabsf(free_place.z - 0.314468f) < 1e-5f,
            "the unarmed take is not the recorded deck take: base z %f, place z %f", free_base.z, free_place.z);
        CHECK(distance_squared3d(&armed_base, &free_base) < 1e-10f, "the leash moved the pill base (%f,%f,%f), not the deck take's (%f,%f,%f)",
            armed_base.x, armed_base.y, armed_base.z, free_base.x, free_base.y, free_base.z);
        CHECK(distance_squared3d(&armed_place, &free_place) < 1e-10f, "the leash moved the place");
        CHECK(!memcmp(&armed_velocity, &free_velocity, sizeof(armed_velocity)), "the leash kept the velocity (%f,%f,%f), not the deck path's (%f,%f,%f)",
            armed_velocity.i, armed_velocity.j, armed_velocity.k, free_velocity.i, free_velocity.j, free_velocity.k);
        load_rise_2034();
        before = pill_in_node();
        CHECK(distance_squared3d(&armed_place, &deck_place) <= distance_squared3d(&before, &deck_place) + 1e-8f,
            "the take moved the host copy away from the client's deck place");
        return 0;
    }
    CASE("host-along-world-word-no-lift")
    {
        real_point3d before, after, word;
        load_straight_252(); bound_wide(&b);
        before = pill_in_node(); word = word_in_node();
        CHECK(word.z > before.z, "the recorded word is not above the host's place");
        CHECK(distributed_take_riding(0, 0, &b, FALSE), "world word refused on the along path");
        after = pill_in_node();
        CHECK(fabsf(after.z - before.z) < 1e-5f, "the world word lifted the pill %f above its place on the deck", after.z - before.z);
        CHECK(fabsf(after.x - (before.x + word.x) * 0.5f) < 1e-5f && fabsf(after.y - (before.y + word.y) * 0.5f) < 1e-5f,
            "across (%f,%f) is not halfway to the word's (%f,%f) from (%f,%f)", after.x, after.y, word.x, word.y, before.x, before.y);
        return 0;
    }
    CASE("host-leash-world-word-no-lift")
    {
        real_point3d before, after, word;
        real_vector3d velocity;
        load_straight_252(); bound_wide(&b);
        distributed_accepted[0].transition_until = 260;
        before = pill_in_node(); word = word_in_node();
        velocity = objects[0].object.translational_velocity;
        CHECK(distributed_take_riding(0, 0, &b, FALSE), "world word refused under the leash");
        after = pill_in_node();
        CHECK(fabsf(after.z - before.z) < 1e-5f, "the leashed world word lifted the pill %f above its place on the deck", after.z - before.z);
        CHECK(fabsf(after.x - word.x) < 1e-5f && fabsf(after.y - word.y) < 1e-5f, "across (%f,%f) is not the word's (%f,%f), whole", after.x, after.y, word.x, word.y);
        CHECK(!memcmp(&velocity, &objects[0].object.translational_velocity, sizeof(velocity)), "the leash changed the host's velocity");
        return 0;
    }
    CASE("host-leash-world-word-lowers")
    {
        real_point3d word;
        load_straight_252(); bound_wide(&b);
        distributed_accepted[0].transition_until = 260;
        distributed_predictions[0].state.position.z = objects[0].object.position.z - 0.2f;
        word = distributed_predictions[0].state.position;
        CHECK(distributed_take_riding(0, 0, &b, FALSE), "lower world word refused under the leash");
        CHECK(distance_squared3d(&objects[0].object.position, &word) < 1e-10f, "the lower word (%f,%f,%f) was not taken whole: (%f,%f,%f)",
            word.x, word.y, word.z, objects[0].object.position.x, objects[0].object.position.y, objects[0].object.position.z);
        return 0;
    }
    CASE("host-deck-word-other-vehicle-riding-follows")
    {
        struct distributed_unit_state *s;
        real_point3d before, after, word;
        load_straight_252(); bound_wide(&b);
        s = &distributed_predictions[0].state;
        s->deck_present = TRUE; s->deck.vehicle_index = OTHER; s->deck.node_index = NONE; s->deck.ticks = RIDING_TICKS;
        s->deck.place = pt(0.0f, 0.0f, 0.3f);
        s->position.z += 0.1f;
        before = pill_in_node(); word = word_in_node();
        CHECK(word.z > before.z, "the word is not above the host's place");
        CHECK(distributed_take_riding(0, 0, &b, FALSE), "word with another vehicle's riding part refused");
        after = pill_in_node();
        /* (the 0.25 blend applies as for any along take: halfway) */
        CHECK(fabsf(after.z - (before.z + word.z) * 0.5f) < 1e-5f, "the host copy did not follow a rider of another vehicle up: z %f from %f toward the word's %f", after.z, before.z, word.z);
        return 0;
    }
    CASE("host-deck-word-other-vehicle-air-no-lift")
    {
        struct distributed_unit_state *s;
        real_point3d before, after;
        load_straight_252(); bound_wide(&b);
        s = &distributed_predictions[0].state;
        s->deck_present = TRUE; s->deck.vehicle_index = OTHER; s->deck.node_index = NONE; s->deck.ticks = RIDING_TICKS - 1;
        s->deck.place = pt(0.0f, 0.0f, 0.3f);
        s->position.z += 0.1f;
        before = pill_in_node();
        CHECK(distributed_take_riding(0, 0, &b, FALSE), "word with another vehicle's air part refused");
        after = pill_in_node();
        CHECK(fabsf(after.z - before.z) < 1e-5f, "an air part of another vehicle lifted the pill %f above its place on the deck", after.z - before.z);
        return 0;
    }
    {
        int landing = landing_case(case_name);
        if (landing != 1)
            return landing;
    }
    fprintf(stderr, "no case %s\n", case_name); return 2;
}
