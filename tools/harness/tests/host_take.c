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
static void accepted_once(void)
{
    struct distributed_on_foot_bound b;
    CHECK(take(&b), "body word not taken");
    CHECK(rebase_calls == 1, "deck take did not rebase");
}
int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    struct distributed_on_foot_bound b;
    setup();
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
    CASE("host-deck-node-forgery")
    {
        accepted_once(); game_time = 21;
        vehicles[0].bounding_sphere_radius = 3.0f; prediction(0, body_place);
        distributed_predictions[0].state.deck.node_index = 2;
        CHECK(!take(&b), "node-local zero height evades body ceiling"); return 0;
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
    fprintf(stderr, "no case %s\n", case_name); return 2;
}
