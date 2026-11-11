/* remote words against this machine's vehicle and a pill offset from its object */
#include "harness.h"
#include <math.h>

typedef unsigned short word;
typedef struct { real i, j; } real_vector2d;
#include "types.inc"

enum { V = 0x10080 };
enum { _network_vehicle_write_state, _game_connection_network_client };
static struct biped_datum
{
    struct { real_point3d position, bounding_sphere_center; real_vector3d translational_velocity; long parent_object_index; } object;
} unit;
#define object_datum biped_datum
#define object_get(i) (&unit)
#define biped_get(i) (&unit)
#define game_connection() _game_connection_network_client
#define game_players_are_double_speed() FALSE
#define UNIT_WORLD_BOUND 100000.0f
static struct vehicle_riding_state riding;
static struct vehicle_riding_claim riding_claims[128];
static struct distributed_own_position distributed_own_positions[4][64];
static long distributed_own_windows[4], distributed_own_air_episodes[4];
static real distributed_own_round_trip;
static void csmemset(void *p, int c, size_t n) { memset(p, c, n); }
static long clock_tick, world_comparisons;
static boolean dead;
static struct vehicle_riding_cabin cabin;
struct player_datum { short local_player_index; };
static struct player_datum player;
static struct vehicle_riding_vehicle vehicle;
static struct distributed_unit_state state;
static struct { long corrections; } distributed_statistics;
static long writes;
static real riding_moved_alone = 0.25f, riding_teleport_step = 4.0f;
static real riding_air_grip = 0.045f;
#include "riding.inc"

static struct biped_datum *biped_try_and_get(long i) { return i == 0 ? &unit : NULL; }
static void biped_riding_get(struct biped_datum const *b, struct vehicle_riding_state *out) { *out = riding; }
static void biped_riding_set(struct biped_datum *b, struct vehicle_riding_state const *s) { riding = *s; }
static boolean biped_riding_get_state(long i, struct vehicle_riding_state *out)
{
    if (i != 0 || dead || unit.object.parent_object_index != NONE)
        return FALSE;
    *out = riding;
    return TRUE;
}
static boolean biped_riding_vehicle(long i, short node, struct vehicle_riding_vehicle *out)
{
    if (i != vehicle.object_index || node != vehicle.node_index)
        return FALSE;
    *out = vehicle;
    return TRUE;
}
static boolean second_vehicle;
static short biped_riding_vehicle_node_count(long i)
{
    return i == vehicle.object_index || (second_vehicle && i == V + 1) ? 4 : NONE;
}
static struct vehicle_riding_air air_state;
static short biped_riding_player(struct biped_datum const *b) { return 0; }
static boolean biped_riding_applies(struct biped_datum const *b) { return !dead; }
struct vehicle_riding_air *vehicle_riding_player_air(short p) { return p == 0 ? &air_state : NULL; }
struct vehicle_riding_cabin *vehicle_riding_player_cabin(short p) { return &cabin; }
static void biped_get_physics_pill(long i, real_point3d *base, real *h, real *w)
{
    *base = unit.object.position;
    base->z -= 0.1f;
    *h = 0.7f; *w = 0.2f;
}
#include "bipeds.inc"

static long game_time_get(void) { return clock_tick; }
static long local_player_get_player_index(short p) { return p == 0 ? 0 : NONE; }
static struct player_datum *player_try_and_get(long p) { return p == 0 ? &player : NULL; }
static long distributed_living_unit(struct player_datum const *p) { return p && !dead ? 0 : NONE; }
static boolean network_coop_active(void) { return FALSE; }
/* (upstream's elevator tolerance and latency meter: not under test here) */
static long distributed_supported_elevator(long unit_index) { return NONE; }
static void distributed_note_own_round_trip(long time) {}
static long scenario_leaf_index_from_point(real_point3d const *p) { return 0; }

static boolean distributed_point_valid(real_point3d const *p, real bound)
{
    return valid_real_point3d(p) && fabsf(p->x) <= bound && fabsf(p->y) <= bound && fabsf(p->z) <= bound;
}
static boolean distributed_axes_make_valid(real_vector3d *f, real_vector3d *u)
{
    return valid_real_vector3d(f) && valid_real_vector3d(u) && normalize3d(f) > 0.0f && normalize3d(u) > 0.0f;
}
static void distributed_vector_unpack(struct distributed_vector const *p, real scale, real_vector3d *v)
{
    v->i = p->i / scale; v->j = p->j / scale; v->k = p->k / scale;
}
static boolean network_objects_reconcile(long i, real_point3d const *p, real_vector3d const *f,
    real_vector3d const *up, real_vector3d const *velocity, real_vector3d const *angular, real blend, short kind)
{
    boolean snap = distance_squared3d(&unit.object.position, p) > blend * blend;
    real fraction = snap ? 1.0f : 0.5f;
    writes++;
    unit.object.position.x += fraction * (p->x - unit.object.position.x);
    unit.object.position.y += fraction * (p->y - unit.object.position.y);
    unit.object.position.z += fraction * (p->z - unit.object.position.z);
    if (velocity)
        unit.object.translational_velocity = *velocity;
    return snap;
}
#include "under_test.inc"

static void setup(void)
{
    memset(distributed_own_positions, 0, sizeof(distributed_own_positions));
    memset(distributed_own_windows, 0, sizeof(distributed_own_windows));
    memset(distributed_own_air_episodes, 0, sizeof(distributed_own_air_episodes));
    memset(riding_claims, 0, sizeof(riding_claims));
    memset(&air_state, 0, sizeof(air_state));
    memset(&cabin, 0, sizeof(cabin)); cabin.vehicle_index = NONE;
    clock_tick = world_comparisons = 0; dead = FALSE; distributed_own_round_trip = 0;
    second_vehicle = FALSE;
    memset(&unit, 0, sizeof(unit)); memset(&vehicle, 0, sizeof(vehicle)); memset(&state, 0, sizeof(state));
    vehicle_riding_clear(&riding);
    vehicle.object_index = V; vehicle.node_index = NONE;
    vehicle.forward.i = vehicle.body_forward.i = 1.0f;
    vehicle.up.k = vehicle.body_up.k = 1.0f;
    vehicle.translational_velocity.i = 0.333f;
    vehicle.bounding_sphere_radius = 3.0f;
    vehicle_riding_correction_identity(&vehicle.correction, V);
    unit.object.parent_object_index = NONE;
    unit.object.position.x = 0.6f; unit.object.position.z = 0.1f;
    riding.object_index = V; riding.node_index = NONE; riding.ticks = RIDING_TICKS;
    riding.velocity.i = 0.333f; riding.local_point.x = 0.6f;
    riding_snapshot(&riding, &vehicle);
    state.deck_present = TRUE; state.deck.vehicle_index = V; state.deck.node_index = NONE;
    state.deck.ticks = RIDING_TICKS; state.deck.place.x = 0.6f;
    state.position.x = 1.6f; state.position.z = 0.1f;
    state.forward.i = (short)DISTRIBUTED_UNIT_SCALE; state.up.k = (short)DISTRIBUTED_UNIT_SCALE;
    state.velocity.i = (short)(0.666f * DISTRIBUTED_VELOCITY_SCALE);
    writes = distributed_statistics.corrections = 0;
}
static void check_place(real x, real local)
{
    CHECK(fabsf(unit.object.position.x - x) < 1e-4f && fabsf(unit.object.position.z - 0.1f) < 1e-5f,
        "wrong base: object (%f,%f), expected (%f,0.1)", unit.object.position.x, unit.object.position.z, x);
    CHECK(riding.object_index == V && riding.ticks == RIDING_TICKS && riding.node_index == NONE,
        "remote deck not taken");
    CHECK(fabsf(riding.local_point.x-local) < 1e-5f && fabsf(riding.local_point.y) < 1e-5f && fabsf(riding.local_point.z) < 1e-5f,
        "anchor not at post-blend base: %f expected %f", riding.local_point.x, local);
}
static void next_tick(void)
{
    real_point3d base;
    real h, w;
    struct vehicle_riding_step step;
    vehicle.position.x += 0.333f; vehicle.origin = vehicle.position;
    vehicle.updates++; vehicle.time++;
    biped_get_physics_pill(0, &base, &h, &w);
    CHECK(vehicle_riding_snapshot_step(&riding, &vehicle, &base, h, w, FALSE, 100.0f, &step),
        "next snapshot refused");
    unit.object.position.x += step.displacement.i;
    riding.velocity = step.velocity_now;
    biped_get_physics_pill(0, &base, &h, &w);
    vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &base, &riding.local_point);
    riding_snapshot(&riding, &vehicle);
}
static int receive_before_carry(real speed, real expected_carry)
{
    long tick;
    real carried = 0.0f;
    real first_elapsed = -1.0f;
    real first_move = -1.0f;

    vehicle.role = _vehicle_riding_role_followed;
    vehicle.translational_velocity.i = speed;
    riding.velocity.i = speed;
    riding_snapshot(&riding, &vehicle);
    for (tick = 1; tick <= 128; tick++)
    {
        struct vehicle_riding_step step;
        real_point3d base;
        real h, w;

        /* (the followed vehicle's packet arrives before its rider updates) */
        vehicle.position.x += speed;
        vehicle.origin = vehicle.position;
        vehicle.time = clock_tick = tick;
        state.position.x = vehicle.position.x + 0.6f;
        distributed_apply_remote_unit(0, &state);
        biped_get_physics_pill(0, &base, &h, &w);
        CHECK(vehicle_riding_snapshot_step(&riding, &vehicle, &base, h, w, FALSE, 100.0f, &step),
            "followed rider lost its window at %ld", tick);
        if (tick == 1)
        {
            first_elapsed = step.player_ticks;
            first_move = step.displacement.i;
        }
        carried += step.displacement.i;
        unit.object.position.x += step.displacement.i;
        riding.velocity = step.velocity_now;
        biped_get_physics_pill(0, &base, &h, &w);
        vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &base, &riding.local_point);
        riding_snapshot(&riding, &vehicle);
    }
    CHECK(writes == 0,
        "standing packet consumed carry: writes=%ld first_elapsed=%f first_move=%f carried=%f",
        writes, first_elapsed, first_move, carried);
    CHECK(fabsf(carried - expected_carry) < 1e-4f && fabsf(riding.velocity.i - speed) < 1e-5f,
        "followed carry or frame velocity was consumed: carry=%f velocity=%f", carried, riding.velocity.i);
    check_place(expected_carry + 0.6f, 0.6f);
    return 0;
}
static void note(long tick)
{
    clock_tick = tick;
    distributed_note_own_positions();
}
static int opposing_motion(boolean repeated)
{
    long tick;
    real maximum_error = 0.0f;

    vehicle.role = _vehicle_riding_role_followed;
    vehicle.translational_velocity.i = riding.velocity.i = 0.06f;
    unit.object.position.x = riding.local_point.x = 0.68f;
    riding_snapshot(&riding, &vehicle);
    for (tick = 1; tick <= (repeated ? 128 : 1); tick++)
    {
        struct vehicle_riding_step step;
        real_point3d base;
        real h, w, error;

        vehicle.position.x += 0.06f;
        vehicle.origin = vehicle.position;
        vehicle.time = clock_tick = tick;
        distributed_apply_remote_unit(0, &state);
        if (!repeated)
        {
            CHECK(writes == 1,
                "world tolerance vetoed deck correction: writes=%ld x=%f snapshot=%f",
                writes, unit.object.position.x, riding.vehicle_position.x);
            check_place(0.67f, 0.61f);
            CHECK(riding.mark.time == 1 && fabsf(riding.vehicle_position.x - 0.06f) < 1e-6f,
                "opposing correction did not re-snapshot");
        }
        biped_get_physics_pill(0, &base, &h, &w);
        CHECK(vehicle_riding_snapshot_step(&riding, &vehicle, &base, h, w, FALSE, 100.0f, &step),
            "opposing-motion carry refused at %ld", tick);
        unit.object.position.x += step.displacement.i;
        riding.velocity = step.velocity_now;
        biped_get_physics_pill(0, &base, &h, &w);
        vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &base, &riding.local_point);
        riding_snapshot(&riding, &vehicle);
        error = fabsf(riding.local_point.x - state.deck.place.x);
        if (error > maximum_error)
            maximum_error = error;
    }
    CHECK(maximum_error <= REMOTE_CORRECTION_TOLERANCE && writes == 1,
        "opposing error did not converge and stay: ticks=%ld writes=%ld error=%f maximum=%f",
        tick - 1, writes, riding.local_point.x - state.deck.place.x, maximum_error);
    return 0;
}
static void echo(long tick)
{
    state.flags = FLAG(_distributed_unit_predicted_bit);
    state.predicted_time = (word)tick;
    distributed_correct_own_unit(&player, 0, &state);
}
static int legacy_case(char const *name)
{
    struct vehicle_riding_step step;
    struct vehicle_riding_deck made;
    real_point3d base;
    real h, w;
    long variant;
    boolean remote = !strcmp(name, "remote-legacy-verdict-held");
    short verdict = !strcmp(name, "own-legacy-verdict-open-equal-world") || strstr(name, "off-deck") ?
        _vehicle_riding_enclosure_open : _vehicle_riding_enclosure_unknown;

    memset(&step, 0, sizeof(step)); step.over_deck = TRUE;
    riding.ticks = 59;
    if (!strcmp(name, "own-legacy-verdict-off-deck-local-resets"))
    {
        vehicle_riding_air_enclosure(&air_state, FALSE, 0.f, 1.f, FALSE);
        step.over_deck = FALSE;
        CHECK(!vehicle_riding_air_begin(&air_state, &riding, &step, TRUE) &&
            air_state.enclosure == _vehicle_riding_enclosure_unknown && !air_state.host_verdict,
            "local-only off-deck verdict did not reset to unknown");
        return 0;
    }
    cabin.unit_index = 0; cabin.vehicle_index = V;
    cabin.jump = _vehicle_riding_enclosure_enclosed;
    air_state.enclosure = _vehicle_riding_enclosure_open;
    note(0); clock_tick = 3;
    state.position = unit.object.position; state.deck.ticks = 59;
    state.deck.flags = verdict << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
    if (remote) distributed_apply_remote_unit(0, &state); else echo(0);
    CHECK(air_state.enclosure == verdict, "equal-world legacy verdict not adopted");
    CHECK(air_state.host_verdict && cabin.host_legacy && !cabin.host, "host legacy provenance not held");
    CHECK(!vehicle_riding_air_begin(&air_state, &riding, &step, TRUE), "host legacy verdict resampled");
    CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, &air_state, FALSE) == verdict,
        "local cabin jump replaced host legacy verdict");
    CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, &air_state, TRUE) == _vehicle_riding_enclosure_unknown,
        "legacy verdict granted cabin protection");
    biped_get_physics_pill(0, &base, &h, &w);
    CHECK(vehicle_riding_deck_make(&riding, &vehicle, &base, &unit.object.translational_velocity,
        &air_state, &cabin, &made), "legacy deck refused");
    CHECK(made.flags == (verdict << VEHICLE_RIDING_DECK_VERDICT_SHIFT), "legacy deck flags %d", made.flags);
    if (!strcmp(name, "own-legacy-verdict-off-deck-host-held"))
    {
        step.over_deck = FALSE;
        CHECK(!vehicle_riding_air_begin(&air_state, &riding, &step, TRUE) &&
            air_state.enclosure == _vehicle_riding_enclosure_open && air_state.host_verdict,
            "off-deck copy reset or resampled the host's episode verdict");
    }
    if (!strcmp(name, "own-legacy-verdict-history-no-deck") || !strcmp(name, "own-legacy-verdict-stale-episode"))
    {
        boolean no_deck = !strcmp(name, "own-legacy-verdict-history-no-deck");
        /* (a real pre-acquisition capture, or an earlier jump in the same window) */
        setup(); riding.ticks = no_deck ? 0 : 59; note(0);
        if (!no_deck) { riding.ticks = 60; note(1); }
        riding.ticks = 59; note(2); clock_tick = 3;
        cabin.unit_index = 0; cabin.vehicle_index = V; cabin.jump = _vehicle_riding_enclosure_enclosed;
        air_state.enclosure = _vehicle_riding_enclosure_open;
        state.position = unit.object.position; state.deck.ticks = 59; state.deck.flags = 0;
        echo(0);
        CHECK(air_state.enclosure == _vehicle_riding_enclosure_open &&
            vehicle_riding_cabin_verdict(&cabin, &riding, &air_state, TRUE) == _vehicle_riding_enclosure_enclosed,
            "non-current legacy echo took verdict or disabled cabin");
    }
    if (!strcmp(name, "own-legacy-verdict-resolved"))
    {
        state.deck.flags = _vehicle_riding_enclosure_enclosed << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        echo(0);
        CHECK(air_state.enclosure == _vehicle_riding_enclosure_enclosed, "later legacy verdict not adopted");
        CHECK(!vehicle_riding_air_begin(&air_state, &riding, &step, TRUE), "resolved host verdict resampled");
    }
    if (!strcmp(name, "own-legacy-over-local-cabin-jump") || !strcmp(name, "own-cabin-verdict-unchanged"))
    {
        for (variant = 1; variant <= 2; variant++)
        {
            state.deck.flags = FLAG(_vehicle_riding_deck_cabin_bit) | (variant << VEHICLE_RIDING_DECK_VERDICT_SHIFT);
            echo(0);
            CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, &air_state, TRUE) == variant,
                "bit-3 host verdict path changed");
            CHECK(vehicle_riding_deck_make(&riding, &vehicle, &base, &unit.object.translational_velocity,
                &air_state, &cabin, &made) && made.flags == state.deck.flags, "bit-3 verdict not serialised");
        }
    }
    if (strstr(name, "-clears"))
    {
        boolean changed_vehicle = strstr(name, "vehicle-change") != NULL;
        if (changed_vehicle)
        {
            riding.object_index = vehicle.object_index = V + 1;
            CHECK(!vehicle_riding_air_begin(&air_state, &riding, NULL, TRUE), "new vehicle sampled before its window");
            vehicle_riding_cabin_begin(&cabin, 0, &riding, TRUE);
        }
        else
        {
            riding.ticks = RIDING_TICKS;
            CHECK(!vehicle_riding_air_begin(&air_state, &riding, &step, FALSE), "landing sampled air");
            vehicle_riding_cabin_standing(&cabin, TRUE, 4);
        }
        CHECK(!air_state.host_verdict && !cabin.host_legacy, "legacy provenance survived boundary");
        vehicle_riding_cabin_take_off(&cabin, &riding, 0);
        riding.ticks = 59;
        CHECK(vehicle_riding_air_begin(&air_state, &riding, &step, TRUE), "next episode did not sample locally");
        vehicle_riding_air_enclosure(&air_state, FALSE, 0.f, 1.f, FALSE);
        cabin.jump = _vehicle_riding_enclosure_enclosed;
        CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, &air_state, TRUE) == _vehicle_riding_enclosure_enclosed,
            "legacy cabin hold survived episode boundary");
    }
    return 0;
}
static int air_claim_case(boolean stale)
{
    long n, window, episode;
    real_point3d before;
    if (stale) { air_claim_case(FALSE); setup(); }
    for (n = 0; n < 64; n++) note(n);
    window = distributed_own_windows[0]; episode = distributed_own_air_episodes[0];
    if (stale) { riding.ticks = 0; note(64); riding.ticks = 60; note(65); }
    clock_tick = 66; state.position = unit.object.position;
    state.deck.ticks = 58; state.deck.node_index = NONE;
    state.deck.velocity.i = 0.125f; state.deck.peak = 0.25f;
    before = unit.object.position;
    for (n = 0; n < SEAT_DISAGREEMENT_TICKS; n++)
    {
        echo(63);
        CHECK(riding.ticks == 60 && writes == 0, "host air claim bypassed grace at %ld", n);
    }
    echo(63);
    if (stale)
        CHECK(riding.ticks == 60 && writes == 0, "stale air claim took newer standing window");
    else
    {
        CHECK(riding.ticks == 58 && riding.node_index == NONE && riding.object_index == V,
            "expired air claim did not take host ticks/node");
        CHECK(fabsf(riding.velocity.i - 0.125f) < 1e-6f && fabsf(air_state.peak - 0.25f) < 1e-6f,
            "expired air claim did not take frame/peak");
        CHECK(!memcmp(&before, &unit.object.position, sizeof(before)), "equal-world air claim moved position");
        for (n = 0; n < 64; n++)
            CHECK(!distributed_own_positions[0][n].air_baseline_valid, "standing history slot %ld not superseded", n);
        note(67);
        CHECK(distributed_own_windows[0] != window && distributed_own_air_episodes[0] != episode,
            "air claim did not drop standing window/start air episode");
        CHECK(distributed_own_positions[0][0].position.x == before.x &&
            !distributed_own_positions[0][0].air_baseline_valid, "air claim history not rebased");
    }
    return 0;
}
static int own_case(char const *name)
{
    long n, old_window;
    struct vehicle_riding_claim *claim = vehicle_riding_player_claim(0);
    if (strstr(name, "-corrects") && !strncmp(name, "own-host-air-", 13))
    {
        note(0);
        if (!strcmp(name, "own-host-air-stale-episode-corrects"))
        {
            riding.ticks = 59; note(1);
            riding.ticks = 60; note(2);
            riding.ticks = 59; note(3);
            riding.ticks = 60;
        }
        clock_tick = 4;
        state.position = unit.object.position; state.position.x += 4.0f;
        state.deck.ticks = 58;
        if (!strcmp(name, "own-host-air-other-vehicle-corrects"))
        {
            second_vehicle = TRUE;
            state.deck.vehicle_index = V + 1;
        }
        if (!strcmp(name, "own-host-air-taken-corrects"))
        {
            claim->vehicle_index = V; claim->node_index = NONE; claim->taken = TRUE;
        }
        echo(0);
        CHECK(writes == 1 && fabsf(unit.object.position.x - 4.6f) < 1e-5f,
            "host positional correction suppressed outside pending current claim: writes=%ld x=%f", writes, unit.object.position.x);
        CHECK(riding.ticks == 60 && claim->disagreement == 0, "non-matching pair started or took claim grace");
        CHECK(fabsf(distributed_own_positions[0][0].position.x - 4.6f) < 1e-5f,
            "host positional correction did not rebase history");
        return 0;
    }
    if (strstr(name, "legacy-") || !strcmp(name, "own-cabin-verdict-unchanged")) return legacy_case(name);
    if (!strcmp(name, "own-host-air-claim-after-grace")) return air_claim_case(FALSE);
    if (!strcmp(name, "own-host-air-claim-stale")) return air_claim_case(TRUE);
    if (!strncmp(name, "own-air-", 8))
    {
        struct vehicle_riding_step step;
        real_vector3d frame;
        boolean whole_sequence = !strcmp(name, "own-air-whole-down") || !strcmp(name, "own-air-whole-up");
        boolean upward = !strcmp(name, "own-air-whole-up");
        boolean previous_window = !strcmp(name, "own-air-other-window") || whole_sequence;
        boolean two_jumps = !strcmp(name, "own-air-two-jumps");
        boolean riding_echo = !strcmp(name, "own-air-from-riding-echo");
        boolean twice = !strcmp(name, "own-air-twice");
        real expected = upward ? 0.45f : previous_window || two_jumps ? 0.25f : riding_echo ? 0.3f : 0.35f;

        riding.ticks = riding_echo ? 60 : 59;
        riding.velocity.i = 0.2f;
        unit.object.translational_velocity.i = riding_echo ? 0.2f : 0.5f;
        note(0); note(1);
        if (previous_window) { riding.ticks = 0; note(2); }
        if (two_jumps) { riding.ticks = 60; note(2); }
        if (!previous_window && !two_jumps) note(2);
        if (twice) for (n = 3; n < 64; n++) note(n);
        riding.ticks = 58;
        riding.velocity.i = 0.3f;
        unit.object.translational_velocity.i = 0.6f;
        air_state.have_peak = TRUE; air_state.peak = 0.4f;
        air_state.enclosure = _vehicle_riding_enclosure_enclosed;
        /* (a take-off can precede its first airborne history capture) */
        if (!riding_echo && !twice) { note(3); note(4); }
        clock_tick = twice ? 66 : 7;
        state.deck.ticks = 59; state.deck.velocity.i = upward ? 0.45f : riding_echo ? 0.3f : 0.25f;
        state.deck.peak = 0.3f;
        state.deck.flags = _vehicle_riding_enclosure_open << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        state.position = unit.object.position; state.position.x += 3.5f;
        echo(twice ? 62 : 0);
        CHECK(fabsf(riding.velocity.i - expected) < 1e-6f,
            "air frame %f expected %f", riding.velocity.i, expected);
        CHECK(fabsf(unit.object.translational_velocity.i - (0.6f + expected - 0.3f)) < 1e-6f,
            "world velocity did not move by air difference: %f", unit.object.translational_velocity.i);
        CHECK(air_state.have_peak && fabsf(air_state.peak - 0.3f) < 1e-6f &&
            air_state.enclosure == _vehicle_riding_enclosure_open, "host peak/verdict not taken");
        if (twice)
        {
            for (n = 0; n < 64; n++)
                CHECK(fabsf(distributed_own_positions[0][n].deck.velocity.i - 0.25f) < 1e-6f,
                    "full ring slot %ld g not rebased", n);
            state.position.x += 3.5f;
            echo(63);
            CHECK(fabsf(riding.velocity.i - 0.35f) < 1e-6f, "air difference applied twice");
        }
        if (previous_window || two_jumps)
        {
            state.position.x += 3.5f;
            state.deck.velocity.i = 0.4f;
            echo(3);
            CHECK(fabsf(riding.velocity.i - 0.4f) < 1e-6f,
                "whole then current echo: g=%f expected 0.4", riding.velocity.i);
            CHECK(fabsf(unit.object.translational_velocity.i - 0.7f) < 1e-6f,
                "whole then current echo: world velocity=%f expected 0.7", unit.object.translational_velocity.i);
            /* (a new capture establishes a baseline; obsolete windows and standing entries still stay untouched) */
            note(5); clock_tick = 8;
            state.position.x += 3.5f; state.deck.velocity.i = 0.45f;
            echo(5);
            CHECK(fabsf(riding.velocity.i - 0.45f) < 1e-6f &&
                fabsf(unit.object.translational_velocity.i - 0.75f) < 1e-6f,
                "new capture did not restore the difference baseline");
            CHECK(fabsf(distributed_own_positions[0][0].deck.velocity.i - 0.2f) < 1e-6f,
                "old window/episode history rebased");
            CHECK(distributed_own_positions[0][0].air_baseline_valid,
                "old window/episode baseline invalidated");
            if (two_jumps)
            {
                CHECK(fabsf(distributed_own_positions[0][2].deck.velocity.i - 0.3f) < 1e-6f,
                    "riding history velocity rebased");
                CHECK(distributed_own_positions[0][2].air_baseline_valid,
                    "standing history baseline invalidated");
            }
        }
        if (!strcmp(name, "own-air-difference") || !strcmp(name, "own-air-peak-held"))
        {
            memset(&step, 0, sizeof(step));
            step.over_deck = TRUE; step.player_ticks = 1.0f;
            if (!strcmp(name, "own-air-peak-held"))
            {
                step.tracked_deck_velocity.i = 0.5f;
                vehicle_riding_air_frame(&riding, &step, &air_state, TRUE, &unit.object.translational_velocity, &frame);
                CHECK(fabsf(riding.velocity.i - 0.35f) < 1e-6f,
                    "accepted difference was clamped before braking");
            }
            step.tracked_deck_velocity.i = 0.2f;
            vehicle_riding_air_frame(&riding, &step, &air_state, TRUE, &unit.object.translational_velocity, &frame);
            step.tracked_deck_velocity.i = 0.5f;
            for (n = 0; n < 10; n++)
                vehicle_riding_air_frame(&riding, &step, &air_state, TRUE, &unit.object.translational_velocity, &frame);
            CHECK(fabsf(riding.velocity.i - 0.3f) < 1e-6f, "open speed-up exceeded host peak: %f", riding.velocity.i);
            if (!strcmp(name, "own-air-peak-held"))
            {
                real_vector3d zero = {0.0f, 0.0f, 0.0f};
                for (n = 0; n < 3; n++)
                {
                    vehicle_riding_air_correct(&riding, &air_state, &zero, 0.3f,
                        _vehicle_riding_enclosure_open, &unit.object.translational_velocity);
                    riding.velocity.i = 0.4f;
                    if (n == 0) riding.ticks = RIDING_TICKS;
                    vehicle_riding_air_begin(&air_state, &riding, n == 1 ? NULL : &step, n != 2);
                    riding.ticks = 58;
                    air_state.enclosure = _vehicle_riding_enclosure_open;
                    vehicle_riding_air_frame(&riding, &step, &air_state, TRUE, &unit.object.translational_velocity, &frame);
                    CHECK(fabsf(air_state.peak - 0.4f) < 1e-6f,
                        "boundary %ld kept the previous jump's host ceiling: %f", n, air_state.peak);
                }
            }
        }
        return 0;
    }
    if (!strcmp(name, "own-verdict-within-tolerance"))
    {
        struct vehicle_riding_state before;
        note(0); clock_tick = 3;
        state.position = unit.object.position;
        state.deck.flags = _vehicle_riding_enclosure_enclosed << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        before = riding;
        echo(0);
        CHECK(writes == 0 && !memcmp(&before, &riding, sizeof(before)), "an echo within tolerance wrote: %ld", writes);
        CHECK(cabin.host && cabin.host_verdict == _vehicle_riding_enclosure_enclosed &&
            cabin.host_phase == _vehicle_riding_cabin_sample_standing, "the host's verdict not taken");
        CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, NULL, TRUE) == _vehicle_riding_enclosure_enclosed,
            "the cabin's verdict is not the host's");
        return 0;
    }
    if (!strcmp(name, "transition-predict"))
    {
        /* (the local sample says enclosed at the protection transition; the host's part says open) */
        note(0); clock_tick = 3;
        cabin.unit_index = 0; cabin.vehicle_index = V; cabin.standing = _vehicle_riding_enclosure_enclosed;
        state.position = unit.object.position;
        state.deck.flags = _vehicle_riding_enclosure_open << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, NULL, TRUE) == _vehicle_riding_enclosure_enclosed, "no local verdict");
        echo(0);
        CHECK(vehicle_riding_cabin_verdict(&cabin, &riding, NULL, TRUE) == _vehicle_riding_enclosure_open,
            "the client kept its own verdict past the host's part");
        return 0;
    }
    if (!strcmp(name, "invalid-part-drops-host"))
    {
        long variant;
        for (variant = 0; variant < 4; variant++)
        {
            setup();
            note(0); clock_tick = 3;
            state.position = unit.object.position;
            state.deck.flags = _vehicle_riding_enclosure_enclosed << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
            if (variant < 2)
                echo(0);
            else
                distributed_apply_remote_unit(0, &state);
            CHECK(cabin.host, "variant %ld: no host verdict to drop", variant);
            if (variant & 1)
                state.deck.node_index = 5;
            else
                state.deck_present = FALSE;
            if (variant < 2)
                echo(0);
            else
                distributed_apply_remote_unit(0, &state);
            CHECK(!cabin.host, "variant %ld: an absent or invalid part kept the host's verdict", variant);
        }
        return 0;
    }
    if (!strcmp(name, "host-return-transaction"))
    {
        /* (the host's copy came back through the hull: its state's position and its part's place name the same spot) */
        for (n = 0; n < 64; n++) note(n);
        clock_tick = 64;
        state.deck.place.x = 4.1f; state.deck.place.y = 0.5f; state.deck.place.z = 0.2f;
        state.position.x = 4.1f; state.position.y = 0.5f; state.position.z = 0.3f;
        echo(62);
        CHECK(writes == 1 && riding.object_index == V && riding.ticks == RIDING_TICKS, "no single correction: writes %ld", writes);
        CHECK(fabsf(unit.object.position.x - 4.1f) < 1e-5f && fabsf(unit.object.position.y - 0.5f) < 1e-5f &&
            fabsf(unit.object.position.z - 0.3f) < 1e-5f, "position (%f,%f,%f) is not the part's place",
            unit.object.position.x, unit.object.position.y, unit.object.position.z);
        CHECK(fabsf(riding.local_point.x - 4.1f) < 1e-5f && fabsf(riding.local_point.y - 0.5f) < 1e-5f &&
            fabsf(riding.local_point.z - 0.2f) < 1e-5f, "the window's place (%f,%f,%f) is not the position's",
            riding.local_point.x, riding.local_point.y, riding.local_point.z);
        return 0;
    }
    if (!strcmp(name, "own-episode-echo"))
    {
        /* (jump 1's host verdict arrives late, in jump 2: an older air episode seeds nothing) */
        note(0);
        riding.ticks = 59; note(1);
        riding.ticks = RIDING_TICKS; note(2);
        riding.ticks = 59; note(3);
        CHECK(distributed_own_positions[0][1].air_episode == 1 && distributed_own_positions[0][3].air_episode == 2,
            "episodes %ld, %ld", distributed_own_positions[0][1].air_episode, distributed_own_positions[0][3].air_episode);
        clock_tick = 6;
        state.deck.ticks = 59;
        state.deck.flags = (byte)((_vehicle_riding_enclosure_open << VEHICLE_RIDING_DECK_VERDICT_SHIFT) |
            FLAG(_vehicle_riding_deck_cabin_bit));
        state.position = unit.object.position;
        echo(1);
        CHECK(!cabin.host, "a late echo of jump 1 seeded jump 2");
        echo(3);
        CHECK(cabin.host && cabin.host_verdict == _vehicle_riding_enclosure_open && cabin.host_phase == _vehicle_riding_cabin_sample_jump,
            "the current jump's echo did not seed it");
        return 0;
    }
    if (!strcmp(name, "own-host-drop-reboard"))
    {
        long local_drop;
        for (local_drop = 0; local_drop < 2; local_drop++)
        {
            setup();
            riding.ticks = 0; unit.object.position.x = -0.6f;
            note(0); clock_tick = 3;
            for (n = 0; n < 16; n++) echo(0);
            CHECK(claim->taken && riding.ticks == 60 && writes == 1, "first boarding missing");
            if (local_drop) biped_riding_drop(0);
            note(1); clock_tick = 4;
            state.deck_present = FALSE; state.position = unit.object.position;
            echo(1);
            CHECK(riding.ticks == 0 && !claim->taken && claim->disagreement == 0,
                "authoritative absence kept obsolete taken claim (local drop %ld)", local_drop);
            note(2); clock_tick = 5;
            state.deck_present = TRUE; state.deck.place.x = 1.6f;
            for (n = 0; n < 15; n++)
            {
                echo(2);
                CHECK(riding.ticks == 0 && writes == 1, "same-V reboard skipped its new grace");
            }
            echo(2);
            CHECK(riding.ticks == 60 && claim->taken && writes == 2 &&
                fabsf(unit.object.position.x - 1.6f) < 1e-5f,
                "same-V host claim did not board again after drop");
        }
        return 0;
    }
    if (!strcmp(name, "own-boarding-retry"))
    {
        riding.ticks = 0; unit.object.position.x = -0.6f;
        note(0); clock_tick = 3;
        for (n = 0; n < 15; n++) echo(0);
        state.forward.i = 0;
        echo(0);
        CHECK(writes == 0 && riding.ticks == 0 && !claim->taken,
            "rejected boarding consumed taken latch");
        state.forward.i = (short)DISTRIBUTED_UNIT_SCALE;
        echo(0);
        CHECK(writes == 1 && riding.ticks == 60 && claim->taken &&
            fabsf(unit.object.position.x - 0.6f) < 1e-5f,
            "expired boarding did not retry on next valid host state");
        return 0;
    }
    if (!strcmp(name, "own-stale-host-current"))
    {
        note(0);
        vehicle.object_index++; riding.object_index++;
        unit.object.position.x = riding.local_point.x = 5.6f;
        riding_snapshot(&riding, &vehicle);
        note(1); clock_tick = 3;
        state.deck.vehicle_index = vehicle.object_index;
        state.position.x = 100.0f;
        claim->disagreement = 5;
        for (n = 0; n < 45; n++)
        {
            echo(0);
            CHECK(writes == 0 && fabsf(unit.object.position.x - 5.6f) < 1e-5f &&
                fabsf(riding.local_point.x - 5.6f) < 1e-5f,
                "stale V1 echo rewound already-held V2 claim at disagreement %ld", n + 1);
        }
        CHECK(claim->disagreement == 0, "matching current claim counted stale disagreements");
        return 0;
    }
    if (!strcmp(name, "own-history-deck"))
    {
        note(0);
        CHECK(distributed_own_positions[0][0].deck_present && distributed_own_positions[0][0].window == 1,
            "deck/history window missing");
        CHECK(fabsf(distributed_own_positions[0][0].deck.place.x - 0.6f) < 1e-5f, "history place missing");
        vehicle.object_index++; riding.object_index++;
        note(1);
        CHECK(distributed_own_positions[0][1].window == 2, "vehicle crossing did not open window");
        riding.ticks = 59; note(2);
        CHECK(distributed_own_positions[0][2].air_episode == 1, "takeoff episode missing");
        riding.ticks = 0; note(3); riding.ticks = 59; note(4);
        CHECK(distributed_own_positions[0][4].window == 3 && distributed_own_positions[0][4].air_episode == 2,
            "air window did not start an episode");
        return 0;
    }
    if (!strcmp(name, "own-deck-lagging") || !strcmp(name, "own-deck-lagging-far"))
    {
        note(0); clock_tick = 3;
        state.position.x = !strcmp(name, "own-deck-lagging") ? 1.6f : 4.6f;
        echo(0);
        CHECK(writes == 0, "world lag falsely corrected a still deck rider: %ld", writes);
        return 0;
    }
    if (!strcmp(name, "own-deck-correct-once") || !strcmp(name, "own-deck-twice"))
    {
        for (n = 0; n < 64; n++) note(n);
        clock_tick = 64;
        state.deck.place.x = 4.1f;
        state.position = unit.object.position;
        state.deck.velocity.j = 0.07f;
        echo(62);
        CHECK(writes == 1 && fabsf(unit.object.position.x - 4.1f) < 1e-5f, "deck error not applied once");
        CHECK(fabsf(unit.object.translational_velocity.j - 0.07f) < 1e-5f, "relative velocity not taken");
        if (!strcmp(name, "own-deck-twice"))
        {
            for (n = 0; n < 64; n++)
                CHECK(fabsf(distributed_own_positions[0][n].deck.place.x - 4.1f) < 1e-5f,
                    "ring history slot %ld not rebased", n);
            echo(63);
            CHECK(writes == 1, "pre-correction deck history corrected twice");
        }
        else
        {
            next_tick();
            CHECK(fabsf(unit.object.position.x - 4.433f) < 1e-5f && fabsf(riding.local_point.x - 4.1f) < 1e-5f,
                "snapshot applied correction again or lost anchor");
        }
        return 0;
    }
    if (!strcmp(name, "own-unechoed-deck-lagging") || !strcmp(name, "own-unechoed-deck-error") ||
        !strcmp(name, "own-unechoed-other-vehicle") || !strcmp(name, "own-unechoed-other-node"))
    {
        /* (a state with no echoed tick: the predicted bit clear) */
        boolean other = !strcmp(name, "own-unechoed-other-vehicle") || !strcmp(name, "own-unechoed-other-node");
        boolean error = !strcmp(name, "own-unechoed-deck-error");

        cabin.unit_index = 0; cabin.vehicle_index = V; cabin.standing = _vehicle_riding_enclosure_enclosed;
        second_vehicle = other;
        state.flags = 0;
        state.position.x = 4.1f;
        if (!strcmp(name, "own-unechoed-other-vehicle"))
            state.deck.vehicle_index = V + 1;
        if (!strcmp(name, "own-unechoed-other-node"))
            state.deck.node_index = 3;
        if (error)
        {
            state.deck.place.x = 4.6f;
            state.position.x = 20.0f;
        }
        distributed_correct_own_unit(&player, 0, &state);
        if (other)
        {
            CHECK(writes == 1, "another vehicle's state was not left to the world correction: %ld", writes);
            return 0;
        }
        if (error)
        {
            CHECK(writes == 1 && fabsf(unit.object.position.x - 4.6f) < 1e-4f,
                "a deck error was not corrected on the deck: %ld writes, x %f", writes, unit.object.position.x);
            CHECK(fabsf(riding.local_point.x - 4.6f) < 1e-4f, "the correction left the deck place at %f", riding.local_point.x);
        }
        else
        {
            CHECK(writes == 0 && fabsf(unit.object.position.x - 0.6f) < 1e-5f,
                "world lag falsely corrected an unechoed deck rider: %ld writes, x %f", writes, unit.object.position.x);
        }
        CHECK(riding.ticks == RIDING_TICKS && riding.object_index == V && cabin.vehicle_index == V &&
            cabin.standing == _vehicle_riding_enclosure_enclosed, "the unechoed rider lost its deck or cabin");
        return 0;
    }
    if (!strcmp(name, "own-host-boards"))
    {
        riding.ticks = 0; unit.object.position.x = -0.6f; note(0); clock_tick = 3;
        state.position.x = 100.0f;
        for (n = 1; n <= 15; n++)
        {
            echo(0);
            CHECK(writes == 0 && riding.ticks == 0, "host boarding applied before grace");
        }
        echo(0);
        CHECK(writes == 1 && riding.ticks == 60 && fabsf(unit.object.position.x - 0.6f) < 1e-5f,
            "host boarding not one deck-frame transaction");
        return 0;
    }
    if (!strcmp(name, "own-host-drops"))
    {
        vehicle.position.z = vehicle.origin.z = 0.1f; unit.object.position.z = 0.2f;
        note(0); vehicle.position.z = vehicle.origin.z = 0.3f;
        unit.object.position.z = 0.4f; clock_tick = 3;
        state.deck_present = FALSE; state.position = unit.object.position; state.position.z -= 0.3f;
        echo(0);
        CHECK(riding.ticks != 60, "host drop retained standing ride");
        CHECK(unit.object.position.z >= 0.4f - 1e-5f, "host drop put base below rising deck");
        return 0;
    }
    if (!strcmp(name, "own-boarding-ahead"))
    {
        riding.ticks = 0;
        note(0); note(1); note(2);
        riding.ticks = 60; note(3);
        state.deck_present = FALSE; state.position = unit.object.position;
        for (n = 0; n < 3; n++)
        {
            clock_tick = n + 3; echo(n);
            CHECK(riding.ticks == 60 && claim->disagreement == 0 && writes == 0,
                "pre-boarding echo dropped/counts disagreement for current rider");
        }
        state.deck_present = TRUE; state.deck.place.x += 3.5f; echo(3);
        CHECK(writes == 1, "boarding echo did not compare deck error");
        return 0;
    }
    if (!strcmp(name, "claim-node-kept"))
    {
        struct vehicle_riding_step step;
        real_point3d base;
        real h, w;
        struct vehicle_riding_vehicle kept;
        note(0); clock_tick = 3; state.deck.node_index = 3;
        vehicle.node_index = 3; vehicle.bounding_sphere_radius = 20.0f;
        for (n = 0; n < 16; n++) echo(0);
        CHECK(claim->taken && riding.node_index == 3, "claim never taken");
        for (n = 0; n < 45; n++)
        {
            CHECK(!vehicle_riding_claim_grace(claim, V, NONE, V, 3, 15), "taken claim given another grace");
            biped_get_physics_pill(0, &base, &h, &w);
            vehicle.position.x += 0.1f; vehicle.updates++; vehicle.time++;
            kept = own_vehicle_for_step();
            CHECK(vehicle_riding_snapshot_step(&riding, &kept, &base, h, w, FALSE, 0.001f, &step), "kept node refused");
            CHECK(!step.node_rejected && riding.node_index == 3, "taken node judged again");
            unit.object.position.x += step.displacement.i;
            riding.velocity = step.velocity_now;
            riding_snapshot(&riding, &vehicle);
        }
        return 0;
    }
    if (!strcmp(name, "claim-crossing"))
    {
        note(0); clock_tick = 3; state.deck.vehicle_index++;
        state.position.x += 100.0f;
        echo(0);
        CHECK(writes == 0 && world_comparisons == 0, "deck crossing measured in world");
        return 0;
    }
    if (!strcmp(name, "own-echo-after-crossing") || !strcmp(name, "own-echo-after-reacquire"))
    {
        note(0); old_window = distributed_own_positions[0][0].window;
        if (!strcmp(name, "own-echo-after-crossing"))
        {
            vehicle.object_index++; riding.object_index++;
        }
        else
        {
            riding.ticks = 0; note(1); riding.ticks = 60;
        }
        note(2); clock_tick = 3;
        CHECK(distributed_own_windows[0] != old_window, "new acquisition reused window");
        state.deck.place.x += 3.5f;
        state.deck.flags = _vehicle_riding_enclosure_enclosed << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        echo(0);
        CHECK(writes == 0 && fabsf(riding.local_point.x - 0.6f) < 1e-5f && cabin.standing == _vehicle_riding_enclosure_unknown,
            "stale window applied error or seeded cabin");
        if (!strcmp(name, "own-echo-after-reacquire"))
        {
            echo(2);
            CHECK(writes == 1, "current window error ignored");
        }
        return 0;
    }
    return -1;
}
static void unpacked_velocity(struct distributed_unit_state const *s, real_vector3d *v)
{
    distributed_vector_unpack(&s->velocity, DISTRIBUTED_VELOCITY_SCALE, v);
}
static void air_state_make(void)
{
    state.deck.ticks = 30;
    state.deck.flags = (byte)(1 | (_vehicle_riding_enclosure_open << VEHICLE_RIDING_DECK_VERDICT_SHIFT));
    state.deck.velocity.i = 0.2f; state.deck.velocity.j = 0.01f; state.deck.velocity.k = 0.03f;
    state.deck.peak = 0.25f;
    state.velocity.i = (short)(0.02379f * DISTRIBUTED_VELOCITY_SCALE);
    state.velocity.j = (short)(0.03105f * DISTRIBUTED_VELOCITY_SCALE);
    state.velocity.k = (short)(0.06354f * DISTRIBUTED_VELOCITY_SCALE);
    unit.object.translational_velocity.i = 0.01839f;
    unit.object.translational_velocity.j = 0.01834f;
    unit.object.translational_velocity.k = 0.04757f;
}
static void velocity_is(real_vector3d const *want, real limit, char const *what)
{
    CHECK(fabsf(unit.object.translational_velocity.i - want->i) <= limit &&
        fabsf(unit.object.translational_velocity.j - want->j) <= limit &&
        fabsf(unit.object.translational_velocity.k - want->k) <= limit,
        "%s: world velocity (%.6f,%.6f,%.6f), expected (%.6f,%.6f,%.6f)", what,
        unit.object.translational_velocity.i, unit.object.translational_velocity.j,
        unit.object.translational_velocity.k, want->i, want->j, want->k);
}
/* the gun r1 takeoff (client tick 1140): the vehicle copy's own update, the rider's saved frame, then the host's takeoff word */
static int takeoff_frame(boolean host_velocities)
{
    real_point3d base, new_base;
    real h, w;
    struct vehicle_riding_step step;
    real_vector3d new_velocity = {0.0f, 0.0f, 0.0f}, saved, host_frame = {0.02435f, 0.03062f, -0.00290f};
    real_vector3d normal = {0.0f, 0.0f, 1.0f}, zero = {0.0f, 0.0f, 0.0f}, relative = {0.0f, 0.0f, 0.0703f};
    real_vector3d turn, want;
    real_point3d vehicle_origin = {35.2589f, -90.8963f, 0.5708f};
    real_point3d pill_base = {35.87329865f, -90.43139648f, 0.91992658f};

    vehicle.position = vehicle.origin = vehicle_origin;
    vehicle.translational_velocity.i = host_velocities ? 0.0286f : 0.0279f;
    vehicle.translational_velocity.j = host_velocities ? 0.0250f : 0.0232f;
    vehicle.translational_velocity.k = host_velocities ? -0.0029f : -0.0041f;
    vehicle.angular_velocity.i = host_velocities ? 0.0005f : 0.0006f;
    vehicle.angular_velocity.j = host_velocities ? 0.0003f : 0.0009f;
    vehicle.angular_velocity.k = host_velocities ? 0.0093f : 0.0086f;
    unit.object.position.x = pill_base.x; unit.object.position.y = pill_base.y; unit.object.position.z = pill_base.z + 0.1f;
    biped_get_physics_pill(0, &base, &h, &w);
    riding.ticks = RIDING_TICKS; riding.object_index = V; riding.node_index = NONE;
    riding_snapshot(&riding, &vehicle);
    vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &base, &riding.local_point);
    /* (the stated model boundary: one update of the vehicle's physics on its velocities) */
    vehicle.position.x += vehicle.translational_velocity.i;
    vehicle.position.y += vehicle.translational_velocity.j;
    vehicle.position.z += vehicle.translational_velocity.k;
    vehicle.origin = vehicle.position;
    cross_product3d(&vehicle.angular_velocity, &vehicle.forward, &turn);
    add_vectors3d(&vehicle.forward, &turn, &vehicle.forward);
    cross_product3d(&vehicle.angular_velocity, &vehicle.up, &turn);
    add_vectors3d(&vehicle.up, &turn, &vehicle.up);
    normalize3d(&vehicle.forward); normalize3d(&vehicle.up);
    vehicle.body_forward = vehicle.forward; vehicle.body_up = vehicle.up;
    vehicle.updates++; vehicle.time++;
    CHECK(vehicle_riding_snapshot_step(&riding, &vehicle, &base, h, w, FALSE, 100.0f, &step), "snapshot step refused");
    unit.object.position.x += step.displacement.i;
    unit.object.position.y += step.displacement.j;
    unit.object.position.z += step.displacement.k;
    biped_get_physics_pill(0, &new_base, &h, &w);
    CHECK(vehicle_riding_record(&riding, &step, &vehicle, TRUE, &zero, &normal, &new_base, FALSE, &new_velocity),
        "record did not keep the window");
    saved = riding.velocity;
    /* (a refresh that lands after the update does not change the frame the window saved) */
    vehicle.translational_velocity.i += 0.05f;
    state.deck_present = TRUE; state.deck.vehicle_index = V; state.deck.node_index = NONE;
    state.deck.ticks = RIDING_TICKS; state.deck.velocity = relative;
    vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &new_base, &state.deck.place);
    state.position = unit.object.position;
    distributed_apply_remote_unit(0, &state);
    want.i = relative.i + saved.i; want.j = relative.j + saved.j; want.k = relative.k + saved.k;
    velocity_is(&want, 1e-6f, "the riding branch adds the saved frame");
    want.i = relative.i + host_frame.i; want.j = relative.j + host_frame.j; want.k = relative.k + host_frame.k;
    return fabsf(unit.object.translational_velocity.i - want.i) <= 1.0f / 1024.0f &&
        fabsf(unit.object.translational_velocity.j - want.j) <= 1.0f / 1024.0f &&
        fabsf(unit.object.translational_velocity.k - want.k) <= 1.0f / 1024.0f;
}
static real_point3d pt3(real x, real y, real z)
{
    real_point3d p; p.x = x; p.y = y; p.z = z; return p;
}
static real_vector3d vc3(real i, real j, real k)
{
    real_vector3d v; v.i = i; v.j = j; v.k = k; return v;
}
/* the host's rider with the live hull at one pose and its snapshot at another, its deck part made as the wire does */
static void deck_made_in(real_point3d live_node, real_vector3d live_f, real_vector3d live_u,
    real_point3d snap_node, real_vector3d snap_f, real_vector3d snap_u, real_point3d base, short ticks,
    struct vehicle_riding_deck *deck)
{
    vehicle.position = vehicle.origin = live_node; vehicle.forward = vehicle.body_forward = live_f;
    vehicle.up = vehicle.body_up = live_u;
    riding.ticks = ticks; riding.falling = ticks < RIDING_TICKS;
    riding.vehicle_position = snap_node; riding.vehicle_forward = snap_f; riding.vehicle_up = snap_u;
    unit.object.position = base; unit.object.position.z += 0.1f;
    CHECK(biped_riding_deck(0, deck), "the rider's deck part refused");
}
static void place_is(struct vehicle_riding_deck const *deck, real x, real y, real z, real limit, char const *what)
{
    CHECK(fabsf(deck->place.x - x) <= limit && fabsf(deck->place.y - y) <= limit && fabsf(deck->place.z - z) <= limit,
        "%s: place (%.8f,%.8f,%.8f), expected (%.8f,%.8f,%.8f)", what, deck->place.x, deck->place.y, deck->place.z,
        x, y, z);
}
struct side { struct biped_datum unit; struct vehicle_riding_state riding; struct vehicle_riding_vehicle vehicle; };
static void side_swap(struct side *s)
{
    struct side t;
    t.unit = unit; t.riding = riding; t.vehicle = vehicle;
    unit = s->unit; riding = s->riding; vehicle = s->vehicle;
    *s = t;
}
/* the vehicle's own tick and the rider's biped tick after it: carried, its own motion on the deck, snapshot retaken */
static void rider_tick(real own_motion)
{
    real_point3d base;
    real h, w;
    struct vehicle_riding_step step;

    vehicle.position.x += 0.0335f; vehicle.origin = vehicle.position; vehicle.updates++; vehicle.time++;
    biped_get_physics_pill(0, &base, &h, &w);
    CHECK(vehicle_riding_snapshot_step(&riding, &vehicle, &base, h, w, FALSE, 100.0f, &step), "carry refused");
    unit.object.position.x += step.displacement.i; unit.object.position.y += step.displacement.j;
    unit.object.position.z += step.displacement.k;
    riding.velocity = step.velocity_now;
    unit.object.position.x += own_motion * vehicle.forward.i; unit.object.position.y += own_motion * vehicle.forward.j;
    unit.object.position.z += own_motion * vehicle.forward.k;
    biped_get_physics_pill(0, &base, &h, &w);
    vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &base, &riding.local_point);
    riding_snapshot(&riding, &vehicle);
}
/* a write to the vehicle (any kind): it moves by dy across and turns by yaw */
static void vehicle_written(real dy, real yaw)
{
    real_vector3d f = vehicle.forward;
    vehicle.position.y += dy; vehicle.origin = vehicle.position;
    vehicle.forward.i = f.i * cosf(yaw) - f.j * sinf(yaw);
    vehicle.forward.j = f.i * sinf(yaw) + f.j * cosf(yaw);
    vehicle.body_forward = vehicle.forward;
}
int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    setup();
    if (!strcmp(case_name, "remote-legacy-verdict-held")) return legacy_case(case_name);
    if (!strncmp(case_name, "own-", 4) || !strncmp(case_name, "claim-", 6) || !strcmp(case_name, "transition-predict") ||
        !strcmp(case_name, "invalid-part-drops-host") || !strcmp(case_name, "host-return-transaction"))
        return own_case(case_name);
    CASE("deck-place-carried-frame")
    {
        /* M3, driven 0 ms r2, host tick 373: the client's prediction wrote the vehicle before the rider's capture */
        struct vehicle_riding_deck deck;
        deck_made_in(pt3(32.83244324f, -91.15576172f, 0.76716369f), vc3(0.8119490f, 0.5826012f, -0.0362562f),
            vc3(-0.0066817f, 0.0713834f, 0.9974266f),
            pt3(32.89699936f, -91.14324188f, 0.76651883f), vc3(0.7827733f, 0.6210900f, -0.0389030f),
            vc3(-0.0039187f, 0.0674324f, 0.9977161f),
            pt3(32.41756439f, -91.58296204f, 1.13087583f), RIDING_TICKS, &deck);
        place_is(&deck, -0.66256917f, -0.06511759f, 0.33575228f, 1e-6f, "the place in the snapshot's node");
        return 0;
    }
    CASE("deck-place-driven-1140")
    {
        /* M2b, driven r1 1140: the hull before and after the kind-3 write, the base after the rider's step */
        struct vehicle_riding_deck deck;
        real_point3d base = pt3(36.77272797f, -88.41619110f, 0.75126523f), pre_node = pt3(36.00705719f, -88.32742310f, 0.46902356f);
        real_vector3d bf_pre = vc3(-0.967160f, 0.247639f, 0.057234f), bu_pre = vc3(0.085339f, 0.104286f, 0.990879f);
        real_point3d post_node = pt3(35.99680328f, -88.27513123f, 0.46608797f), snap_place;
        real_vector3d bf_post = vc3(-0.941321f, 0.333662f, 0.050844f), bu_post = vc3(0.089472f, 0.101435f, 0.990811f);
        deck_made_in(post_node, bf_post, bu_post, pre_node, bf_pre, bu_pre, base, RIDING_TICKS, &deck);
        vehicle_riding_point_to_local(&pre_node, &bf_pre, &bu_pre, &base, &snap_place);
        place_is(&deck, snap_place.x, snap_place.y, snap_place.z, 1e-6f, "the place in the snapshot's node");
        place_is(&deck, -0.74636f, -0.06338f, 0.33575f, 1e-3f, "the stored place");
        return 0;
    }
    CASE("deck-place-unwritten-unchanged")
    {
        struct vehicle_riding_deck made, today;
        real_point3d base = pt3(32.41756439f, -91.58296204f, 1.13087583f), node = pt3(32.83244324f, -91.15576172f, 0.76716369f);
        real_vector3d f = vc3(0.8119490f, 0.5826012f, -0.0362562f), u = vc3(-0.0066817f, 0.0713834f, 0.9974266f);
        real_point3d base_now;
        real h, w;
        memset(&made, 0, sizeof(made)); memset(&today, 0, sizeof(today));
        deck_made_in(node, f, u, node, f, u, base, RIDING_TICKS, &made);
        biped_get_physics_pill(0, &base_now, &h, &w);
        CHECK(vehicle_riding_deck_make(&riding, &vehicle, &base_now, &unit.object.translational_velocity,
            vehicle_riding_player_air(0), vehicle_riding_player_cabin(0), &today), "today's part refused");
        CHECK(!memcmp(&made, &today, sizeof(made)), "an unwritten vehicle's part differs from the live frame's");
        return 0;
    }
    CASE("deck-place-air-live-frame")
    {
        struct vehicle_riding_deck deck;
        real_point3d base = pt3(32.41756439f, -91.58296204f, 1.13087583f), node = pt3(32.83244324f, -91.15576172f, 0.76716369f);
        real_point3d live_place;
        real_vector3d f = vc3(0.8119490f, 0.5826012f, -0.0362562f), u = vc3(-0.0066817f, 0.0713834f, 0.9974266f);
        deck_made_in(node, f, u, pt3(32.89699936f, -91.14324188f, 0.76651883f), vc3(0.7827733f, 0.6210900f, -0.0389030f),
            vc3(-0.0039187f, 0.0674324f, 0.9977161f), base, RIDING_TICKS - 1, &deck);
        vehicle_riding_point_to_local(&node, &f, &u, &base, &live_place);
        place_is(&deck, live_place.x, live_place.y, live_place.z, 1e-6f, "a jumper's place in the live pose");
        return 0;
    }
    CASE("remote-standing-same-snapshot")
    {
        /* a host rider standing on a vehicle that is written to before each of its captures (M3's 0.046 wu and 0.035 rad,
        then a small one), its words applied to the client's copy of the same rider and vehicle that gets no writes */
        struct side host;
        long tick;
        real large = 0.0f;

        unit.object.position = pt3(0.6f, 0.5f, 0.1f);
        riding.local_point = pt3(0.6f, 0.5f, 0.0f);
        riding_snapshot(&riding, &vehicle);
        host.unit = unit; host.riding = riding; host.vehicle = vehicle;
        for (tick = 1; tick <= 60; tick++)
        {
            struct vehicle_riding_deck made;
            real_point3d base, place_live, place_snapshot;
            real h, w;
            boolean big = (tick & 1) != 0;

            clock_tick = tick;
            side_swap(&host);
            if (big)
                vehicle_written((tick & 2) ? 0.046f : -0.046f, (tick & 2) ? 0.035f : -0.035f);
            else
                vehicle_written(0.002f, 0.0f);
            biped_get_physics_pill(0, &base, &h, &w);
            vehicle_riding_point_to_local(&vehicle.position, &vehicle.forward, &vehicle.up, &base, &place_live);
            vehicle_riding_point_to_local(&riding.vehicle_position, &riding.vehicle_forward, &riding.vehicle_up, &base,
                &place_snapshot);
            CHECK(biped_riding_deck(0, &made), "the host rider's part refused at %ld", tick);
            state.deck = made; state.deck_present = TRUE; state.position = unit.object.position;
            if (big)
            {
                real moved = sqrtf(distance_squared3d(&place_live, &place_snapshot));
                CHECK(moved > REMOTE_CORRECTION_TOLERANCE, "the write at %ld is too small to matter: %f", tick, moved);
                large += 1.0f;
            }
            rider_tick(0.03f);
            side_swap(&host);
            distributed_apply_remote_unit(0, &state);
            CHECK(writes == 0, "the client corrected the host's word at %ld: writes=%ld",
                tick, writes);
            rider_tick(0.03f);
        }
        CHECK(large == 30.0f, "writes not made");
        return 0;
    }
    CASE("remote-receive-before-carry")
        return receive_before_carry(0.0335f, 4.288f);
    CASE("remote-receive-above-tolerance")
        return receive_before_carry(0.06f, 7.68f);
    CASE("remote-opposing-motion")
        return opposing_motion(FALSE);
    CASE("remote-opposing-converges")
        return opposing_motion(TRUE);
    CASE("remote-opposing-exact-target")
    {
        vehicle.role = _vehicle_riding_role_followed;
        unit.object.position.x = riding.local_point.x = 0.68f;
        riding_snapshot(&riding, &vehicle);
        vehicle.position.x = vehicle.origin.x = 0.08f;
        vehicle.time = clock_tick = 1;
        distributed_apply_remote_unit(0, &state);
        CHECK(writes == 1, "exact world target vetoed deck-decided write");
        check_place(0.68f, 0.6f);
        CHECK(riding.mark.time == 1, "exact target left its carry pending");
        return 0;
    }
    CASE("remote-written-retakes")
    {
        struct vehicle_riding_step step;
        real_point3d base;
        real h, w;

        vehicle.role = _vehicle_riding_role_followed;
        riding_snapshot(&riding, &vehicle);
        vehicle.position.x = vehicle.origin.x = 0.333f;
        vehicle.time = clock_tick = 1;
        unit.object.position.x = 0.4f;
        distributed_apply_remote_unit(0, &state);
        CHECK(writes == 1, "written correction not applied");
        check_place(0.6665f, 0.3335f);
        CHECK(riding.mark.time == 1 && fabsf(riding.vehicle_position.x - 0.333f) < 1e-6f,
            "written correction did not take the current snapshot");
        biped_get_physics_pill(0, &base, &h, &w);
        CHECK(vehicle_riding_snapshot_step(&riding, &vehicle, &base, h, w, FALSE, 100.0f, &step),
            "post-correction snapshot refused");
        CHECK(fabsf(step.displacement.i) < 1e-6f && step.player_ticks == 0.0f,
            "written correction left a second carry pending: %f", step.displacement.i);
        return 0;
    }
    CASE("remote-deck-lagging")
    {
        unit.object.position.x = -0.6f;
        distributed_apply_remote_unit(0, &state);
        check_place(0.6f, 0.6f);
        CHECK(writes == 1, "large deck error not written once");
        return 0;
    }
    CASE("remote-deck-blend")
    {
        unit.object.position.x = -0.2f;
        distributed_apply_remote_unit(0, &state);
        check_place(0.2f, 0.2f);
        CHECK(writes == 1 && distributed_statistics.corrections == 0, "deck blend snapped");
        next_tick();
        return 0;
    }
    CASE("remote-deck-still")
    {
        long tick;
        for (tick = 0; tick < 128; tick++)
        {
            next_tick();
            state.position.x = vehicle.position.x + 1.6f;
            distributed_apply_remote_unit(0, &state);
            check_place(vehicle.position.x + 0.6f, 0.6f);
        }
        CHECK(writes == 0 && distributed_statistics.corrections == 0,
            "still rider writes %ld, corrections %ld", writes, distributed_statistics.corrections);
        return 0;
    }
    CASE("remote-deck-velocity")
    {
        long exact;
        for (exact = 0; exact < 2; exact++)
        {
            setup();
            unit.object.position.x = exact ? 0.6f : -0.2f;
            state.deck.velocity.i = 0.07f; state.deck.velocity.j = -0.02f; state.deck.velocity.k = 0.03f;
            distributed_apply_remote_unit(0, &state);
            CHECK(fabsf(unit.object.translational_velocity.i - 0.403f) < 1e-6f &&
                fabsf(unit.object.translational_velocity.j + 0.02f) < 1e-6f &&
                fabsf(unit.object.translational_velocity.k - 0.03f) < 1e-6f,
                "world velocity taken instead of relative + client frame: %f", unit.object.translational_velocity.i);
            CHECK(writes == !exact, "exact position should only take velocity");
        }
        return 0;
    }
    CASE("remote-air")
    {
        state.deck.ticks = 30;
        state.deck.flags = (byte)(1 | (_vehicle_riding_enclosure_open << VEHICLE_RIDING_DECK_VERDICT_SHIFT));
        state.deck.velocity.i = 0.2f;
        state.deck.peak = 0.25f;
        state.position.x = 2.6f;
        distributed_apply_remote_unit(0, &state);
        CHECK(fabsf(unit.object.position.x - 2.6f) < 1e-4f,
            "air rider not placed by the host's world position: %f", unit.object.position.x);
        CHECK(riding.object_index == V && riding.ticks == 30 && riding.falling == 1 && riding.node_index == NONE,
            "air window not taken: ticks %d falling %d", riding.ticks, riding.falling);
        CHECK(fabsf(riding.velocity.i - 0.2f) < 1e-6f && fabsf(riding.velocity.j) < 1e-6f,
            "air frame not the part's: %f", riding.velocity.i);
        CHECK(fabsf(riding.local_point.x - 2.6f) < 1e-4f, "air window not anchored at the pill: %f", riding.local_point.x);
        CHECK(air_state.have_peak && fabsf(air_state.peak - 0.25f) < 1e-6f &&
            air_state.enclosure == _vehicle_riding_enclosure_open, "air peak %f or verdict %d not taken",
            air_state.peak, air_state.enclosure);
        CHECK(fabsf(unit.object.translational_velocity.i - 0.666f) < 1e-3f,
            "air world velocity not the host's: %f", unit.object.translational_velocity.i);
        return 0;
    }
    CASE("remote-air-one-frame-step")
    {
        real_vector3d wire;
        real_point3d before;

        air_state_make();
        state.position = unit.object.position; state.position.x += 0.01f;
        unpacked_velocity(&state, &wire);
        before = unit.object.position;
        distributed_apply_remote_unit(0, &state);
        CHECK(writes == 0 && unit.object.position.x == before.x, "a copy within tolerance was written: %ld", writes);
        velocity_is(&wire, 0.0f, "air copy within tolerance");
        unit.object.position.x += unit.object.translational_velocity.i;
        unit.object.position.y += unit.object.translational_velocity.j;
        unit.object.position.z += unit.object.translational_velocity.k;
        CHECK(fabsf(unit.object.position.x - before.x - wire.i) < 1e-6f &&
            fabsf(unit.object.position.y - before.y - wire.j) < 1e-6f &&
            fabsf(unit.object.position.z - before.z - wire.k) < 1e-6f, "one step did not advance by the host's velocity");
        return 0;
    }
    CASE("remote-air-genuine-residual")
    {
        real_vector3d wire;

        air_state_make();
        state.position = unit.object.position; state.position.x += 0.06f;
        unpacked_velocity(&state, &wire);
        distributed_apply_remote_unit(0, &state);
        CHECK(writes == 1, "a residual past the tolerance was hidden: writes %ld", writes);
        velocity_is(&wire, 0.0f, "air copy past tolerance");
        return 0;
    }
    CASE("remote-air-frame-is-hosts")
    {
        air_state_make();
        state.position = unit.object.position; state.position.x += 0.01f;
        distributed_apply_remote_unit(0, &state);
        CHECK(riding.object_index == V && riding.ticks == 30 && riding.falling == 1, "air window not taken");
        CHECK(riding.velocity.i == state.deck.velocity.i && riding.velocity.j == state.deck.velocity.j &&
            riding.velocity.k == state.deck.velocity.k, "air frame not the host's: (%f,%f,%f)",
            riding.velocity.i, riding.velocity.j, riding.velocity.k);
        return 0;
    }
    CASE("remote-takeoff-frame-after-refresh")
    {
        CHECK(takeoff_frame(TRUE), "takeoff frame not the host's after the vehicle took its velocities");
        setup();
        CHECK(!takeoff_frame(FALSE), "the stale vehicle velocities gave the host's frame: the oracle is blind");
        return 0;
    }
    CASE("remote-verdict-within-tolerance")
    {
        struct vehicle_riding_state before = riding;
        state.deck.flags = _vehicle_riding_enclosure_enclosed << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        unit.object.position.x = 0.64f;
        riding.local_point.x = 0.64f;
        before = riding;
        distributed_apply_remote_unit(0, &state);
        CHECK(writes == 0 && !memcmp(&before, &riding, sizeof(before)), "a remote within tolerance wrote: %ld", writes);
        CHECK(cabin.host && cabin.host_verdict == _vehicle_riding_enclosure_enclosed &&
            vehicle_riding_cabin_verdict(&cabin, &riding, NULL, TRUE) == _vehicle_riding_enclosure_enclosed,
            "the host's verdict not taken for a remote rider");
        return 0;
    }
    CASE("late-join-verdict")
    {
        vehicle_riding_clear(&riding);
        state.deck.flags = _vehicle_riding_enclosure_enclosed << VEHICLE_RIDING_DECK_VERDICT_SHIFT;
        distributed_apply_remote_unit(0, &state);
        CHECK(riding.ticks == RIDING_TICKS && cabin.unit_index == 0 && cabin.vehicle_index == V &&
            cabin.host && cabin.host_verdict == _vehicle_riding_enclosure_enclosed,
            "a late joiner's first verdict did not seed the cabin");
        return 0;
    }
    CASE("remote-invalid-clears")
    {
        long variant;
        for (variant = 0; variant < 3; variant++)
        {
            setup();
            state.position.x = 2.6f;
            if (variant == 0)
                state.deck_present = FALSE;
            else if (variant == 1)
                state.deck.node_index = 5;
            else
            {
                riding.ticks = 30; riding.falling = 1; riding.velocity.i = 0.2f;
                state.deck_present = FALSE;
            }
            distributed_apply_remote_unit(0, &state);
            CHECK(fabsf(unit.object.position.x - 2.6f) < 1e-4f, "variant %ld: not placed by the world position: %f",
                variant, unit.object.position.x);
            CHECK(riding.object_index == NONE && riding.ticks == 0, "variant %ld: riding kept: object %ld ticks %d",
                variant, riding.object_index, riding.ticks);
            if (variant == 2)
                CHECK(riding.falling == 1 && fabsf(riding.velocity.i - 0.2f) < 1e-6f, "dropped air lost its frame");
        }
        return 0;
    }
    CASE("remote-late-join")
    {
        vehicle_riding_clear(&riding);
        vehicle.angular_velocity.k = 0.5f;
        state.deck.flags = 1;
        distributed_apply_remote_unit(0, &state);
        check_place(0.6f, 0.6f);
        CHECK(riding.falling == 1, "falling not taken from the part");
        CHECK(fabsf(riding.velocity.i - 0.333f) < 1e-5f && fabsf(riding.velocity.j - 0.3f) < 1e-5f,
            "frame not measured from this machine's vehicle: (%f, %f)", riding.velocity.i, riding.velocity.j);
        CHECK(fabsf(unit.object.translational_velocity.j - 0.3f) < 1e-5f, "unit not given the measured frame");
        return 0;
    }
    fprintf(stderr, "no case %s\n", case_name); return 2;
}
