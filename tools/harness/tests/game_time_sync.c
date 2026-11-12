/* The real frame clock and reliable host-update handler, with tick callbacks
 * recorded instead of a loaded world. No map or transport assets are needed. */
#include "harness.h"
#include <math.h>

/* The production clock's three booleans are bytes on the supported ILP32 ABI. */
#define boolean byte
#undef match_assert
#define match_assert(file, line, condition) CHECK(condition, "engine assertion")
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define network_event(...) ((void)0)
#define profile_scope(section, code) do { code } while (0);
#include "config.inc"

struct network_game_client { unsigned long next_update_number; boolean started; };
struct message_server_game_update { unsigned long update_number; long game_time; };
struct scenario { short type; };
static struct scenario scenario_storage, *global_scenario;
static struct network_game_client client;
static boolean network_game_client_late_join_clock_pending;
static struct game_time_globals_struct clock_storage, *game_time_globals = &clock_storage;
static short connection;
static boolean mcc;
static real remote_speed;
static long maximum_server_time, first_tick, previous_tick;
static int ticks, snapshots, sends, frames, speed_frames;
static int allocations, client_starts, server_starts;
static real frame_delta;

static short game_connection(void) { return connection; }
static boolean network_game_distributed_client(void) { return connection == _game_connection_network_client; }
static struct network_game_client *global_network_game_client_get(void) { return &client; }
static boolean network_game_client_server_has_started_game(struct network_game_client *value) { return value->started; }
static void mcc_game_speed_frame(void) { speed_frames++; }
static real mcc_game_speed_client(real original) { return mcc ? remote_speed : original; }
static boolean halo_fixed_step(void) { return FALSE; }
static void update_client_local_ticks(long count) { maximum_server_time += count; }
static void *global_network_game_server_get(void) { return NULL; }
static void network_game_server_update_ticks(void *server, short count) { (void)server; maximum_server_time += count; }
static long update_client_get_maximum_possible_server_time(void) { return maximum_server_time; }
static void update_client_start(void) { client_starts++; }
static void update_server_start(void) { server_starts++; }
static void *game_state_malloc(char const *name, void *unused, unsigned long size)
{
    (void)unused;
    CHECK(!strcmp(name, "game time globals") && size == 32, "saved clock allocation changed");
    allocations++;
    return &clock_storage;
}

static void game_tick(void)
{
    long now = game_time_globals->local_time;
    CHECK(!ticks || now == previous_tick + 1, "a simulation tick was skipped or repeated");
    CHECK(ticks == snapshots && sends == (global_scenario ? 0 : ticks), "tick callback ordering changed");
    if (!ticks) first_tick = now;
    previous_tick = now;
    ticks++;
}

static void render_interpolation_tick(void)
{
    CHECK(ticks == snapshots + 1 && game_time_globals->local_time == previous_tick,
          "snapshot did not follow the same simulation tick");
    snapshots++;
}

static void network_distributed_tick(void)
{
    CHECK(snapshots == sends + 1 && game_time_globals->local_time == previous_tick + 1,
          "state send did not follow clock advancement and snapshot");
    sends++;
}

static void render_interpolation_frame_begin(void) { }
static void render_interpolation_frame_end(void) { }
static void game_frame(real delta) { frames++; frame_delta = delta; }

long game_time_get(void);
boolean game_time_menu_scene(void);
#include "under_test.inc"

static void reset(long time)
{
    /* A real lifecycle reset, so cases do not borrow the prior target. */
    game_time_dispose_from_old_map();
    game_time_initialize_for_new_map();
    game_time_globals->active = TRUE;
    game_time_globals->local_time = time;
    game_time_globals->server_time = time;
    game_time_globals->speed = 1.0f;
    game_time_globals->leftover_dt = .005f;
    connection = _game_connection_network_client;
    memset(&client, 0, sizeof(client));
    client.started = TRUE;
    network_game_client_late_join_clock_pending = FALSE;
    global_scenario = NULL;
    mcc = FALSE;
    remote_speed = 1.0f;
    maximum_server_time = time;
    first_tick = previous_tick = NONE;
    ticks = snapshots = sends = frames = speed_frames = 0;
    frame_delta = 0.f;
}

static void host_update(long time)
{
    struct message_server_game_update update;
    update.update_number = client.next_update_number;
    update.game_time = time;
    CHECK(network_game_client_handle_game_update(&client, &update), "valid handler result");
}

static void frame(real dt)
{
    long before = game_time_get();
    int before_ticks = ticks;
    game_time_update(dt);
    CHECK(game_time_get() - before == ticks - before_ticks, "clock jumped without executing the simulation");
    CHECK(game_time_globals->server_time == game_time_get(), "server and local clocks separated");
    CHECK(ticks - before_ticks <= TICKS_PER_SECOND, "frame exceeded the existing tick work limit");
}

static void convergence(long lag, boolean scaled)
{
    long host_time = 1000;
    int index;
    reset(host_time - lag);
    mcc = scaled;
    remote_speed = scaled ? .5f : 1.0f;
    for (index = 0; index < 300; index++)
    {
        int before = ticks;
        if (index % TICKS_PER_SECOND == 0) host_update(host_time);
        frame(1.f / (remote_speed * TICKS_PER_SECOND));
        CHECK(ticks - before >= 1 && ticks - before <= 2, "catch-up exceeded twice the normal tick rate");
        host_time++;
        CHECK(game_time_get() <= host_time, "catch-up advanced ahead of the known host");
    }
    CHECK(host_time - game_time_get() <= 2, "small clock lag remains after ten host updates: %ld", host_time-game_time_get());
    CHECK(ticks > 300 && ticks <= 300 + lag, "missing or excessive actual catch-up ticks");
    CHECK(sends == ticks && snapshots == ticks && previous_tick-first_tick+1 == ticks,
          "catch-up omitted simulation, rendering or network publication");
}

static void lifecycle(char const *which)
{
    reset(1000);
    host_update(1008);
    if (!strcmp(which, "new-map"))
    {
        game_time_globals->initialized = FALSE;
        game_time_initialize_for_new_map();
    }
    else if (!strcmp(which, "dispose")) game_time_dispose_from_old_map();
    else if (!strcmp(which, "end")) { game_time_end(); game_time_start(); }
    else if (!strcmp(which, "set")) game_time_set_distributed(1000);
    else if (!strcmp(which, "initialize")) game_time_initialize();
    else CHECK(FALSE, "unknown lifecycle case");
    /* Reactivate without another reset that could conceal the tested one. */
    game_time_globals->initialized = game_time_globals->active = TRUE;
    game_time_globals->local_time = game_time_globals->server_time = 1000;
    game_time_globals->speed = 1.f;
    frame(1.f / TICKS_PER_SECOND + .000001f);
    CHECK(ticks == 1, "prior session's target survived %s", which);
}

static long repeated_target(boolean repeat)
{
    int index;
    reset(1000);
    host_update(1008);
    for (index = 0; index < 30; index++)
    {
        if (repeat) host_update(1008);
        frame(1.f / TICKS_PER_SECOND);
    }
    CHECK(game_time_get() <= 1038, "stale target was extrapolated into a moving target");
    return game_time_get();
}

static void setter_context(char const *which)
{
    reset(1000);
    if (!strcmp(which, "local")) connection = _game_connection_local;
    else if (!strcmp(which, "host")) connection = _game_connection_network_server;
    else if (!strcmp(which, "menu"))
    {
        scenario_storage.type = _scenario_type_main_menu;
        global_scenario = &scenario_storage;
    }
    else if (!strcmp(which, "inactive")) game_time_globals->active = FALSE;
    else if (!strcmp(which, "uninitialized")) game_time_globals->initialized = FALSE;
    else CHECK(FALSE, "unknown setter context");
    game_time_synchronize_distributed(1008);
    connection = _game_connection_network_client;
    global_scenario = NULL;
    game_time_globals->initialized = game_time_globals->active = TRUE;
    frame(1.f / TICKS_PER_SECOND);
    CHECK(ticks == 1, "irrelevant %s timestamp reached a later client frame", which);
}

int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    CHECK(sizeof(long) == 4 && sizeof(clock_storage) == 32, "clock fixture must use the ILP32 layout");
    if (!strncmp(case_name, "lag-", 4))
    {
        convergence(strtol(case_name + 4, NULL, 10), FALSE);
        return 0;
    }
    if (!strncmp(case_name, "lifecycle-", 10))
    {
        lifecycle(case_name + 10);
        return 0;
    }
    if (!strncmp(case_name, "setter-", 7))
    {
        setter_context(case_name + 7);
        return 0;
    }
    CASE("scaled-convergence") { convergence(8, TRUE); return 0; }
    CASE("nominal")
    {
        reset(1000);
        host_update(1000);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1 && fabsf(game_time_globals->leftover_dt-.005f) < .000001f, "nominal frame changed");
        CHECK(game_time_globals->last_local_time_elapsed == 1 && frames == 1 && speed_frames == 1,
              "nominal frame accounting changed");
        return 0;
    }
    CASE("fraction")
    {
        real ordinary;
        reset(1000);
        frame(.04f);
        ordinary = game_time_globals->leftover_dt;
        reset(1000);
        host_update(1008);
        CHECK(game_time_globals->leftover_dt == .005f, "host time reset the partial tick");
        frame(.04f);
        CHECK(ticks == 2 && game_time_globals->leftover_dt == ordinary, "extra ticks changed the frame's fraction");
        CHECK(game_time_globals->last_local_time_elapsed == 2 && frame_delta == .04f,
              "tick elapsed or frame duration accounting changed");
        return 0;
    }
    CASE("work-budget")
    {
        reset(1000);
        game_time_synchronize_distributed(1100);
        frame(.5f);
        CHECK(ticks == 30, "expected bounded two-times catch-up");
        reset(1000);
        game_time_synchronize_distributed(1100);
        frame(.9f);
        CHECK(ticks == 30, "catch-up must retain thirty-tick frame cap");
        return 0;
    }
    CASE("target-bound")
    {
        reset(1000);
        host_update(1003);
        frame(.067f);
        CHECK(game_time_get() == 1003 && ticks == 3, "catch-up overshot the host's actual timestamp");
        return 0;
    }
    CASE("repeat-target")
    {
        long once = repeated_target(FALSE), repeated = repeated_target(TRUE);
        CHECK(once == repeated, "duplicate target changed total simulation time");
        return 0;
    }
    CASE("stale-target")
    {
        reset(1000);
        host_update(1008);
        host_update(999);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 2, "older time erased newer catch-up evidence");
        return 0;
    }
    CASE("sub-tick")
    {
        reset(1000);
        host_update(1008);
        frame(.001f);
        CHECK(!ticks && game_time_globals->last_local_time_elapsed == 0, "catch-up created a tick without normal time");
        return 0;
    }
    CASE("no-target")
    {
        int index;
        reset(1000);
        for (index = 0; index < 30; index++) frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 30, "client without host time changed speed");
        return 0;
    }
    CASE("client-ahead")
    {
        reset(1000);
        host_update(990);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(game_time_get() == 1001 && ticks == 1, "ahead client was rewound or accelerated");
        return 0;
    }
    CASE("held")
    {
        reset(1000);
        host_update(1008);
        client.started = FALSE;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(!ticks && game_time_globals->last_local_time_elapsed == 0, "waiting client simulated");
        client.started = TRUE;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 2, "ready client lost its valid target");
        return 0;
    }
    CASE("pause")
    {
        reset(1000);
        host_update(1008);
        game_time_set_paused(TRUE);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(!ticks && !frames && game_time_get_paused(), "normal pause no longer stops the frame clock");
        game_time_set_paused(FALSE);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 2, "resumed client did not process bounded catch-up");
        return 0;
    }
    CASE("mcc-rate-changes")
    {
        reset(1000);
        mcc = TRUE;
        host_update(1008);
        remote_speed = 0.f;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(!ticks && frame_delta == 0.f, "MCC zero speed was bypassed");
        remote_speed = .5f;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(!ticks, "MCC half speed produced a premature tick");
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 2, "MCC half-speed catch-up budget differs");
        remote_speed = 2.f;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 6, "MCC double-speed catch-up did not use the actual normal budget");
        return 0;
    }
    CASE("menu")
    {
        reset(1000);
        host_update(1008);
        scenario_storage.type = _scenario_type_main_menu;
        global_scenario = &scenario_storage;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1 && !sends, "menu scene used gameplay catch-up or network publication");
        return 0;
    }
    CASE("local")
    {
        reset(1000);
        host_update(1008);
        connection = _game_connection_local;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1, "local clock used a network target");
        return 0;
    }
    CASE("host")
    {
        reset(1000);
        host_update(1008);
        connection = _game_connection_network_server;
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1, "host clock used a client target");
        return 0;
    }
    CASE("initial-sync")
    {
        reset(993);
        network_game_client_late_join_clock_pending = TRUE;
        host_update(1000);
        CHECK(game_time_get() == 1000 && !network_game_client_late_join_clock_pending &&
              game_time_globals->leftover_dt == 0.f, "first reliable update no longer sets the clock");
        frame(1.f / TICKS_PER_SECOND + .000001f);
        CHECK(ticks == 1, "initial synchronization retained a catch-up target");
        return 0;
    }
    CASE("large-stall")
    {
        reset(1000);
        host_update(1008);
        host_update(1031);
        CHECK(game_time_get() == 1031 && game_time_globals->leftover_dt == 0.f,
              "existing long-stall recovery changed");
        frame(1.f / TICKS_PER_SECOND + .000001f);
        CHECK(ticks == 1, "long-stall recovery retained a catch-up target");
        return 0;
    }
    CASE("invalid-times")
    {
        reset(1000);
        host_update(-1);
        host_update(0x40000000L);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1, "invalid reliable timestamps affected the clock");
        return 0;
    }
    CASE("invalid-targets")
    {
        reset(1000);
        game_time_synchronize_distributed(-1);
        game_time_synchronize_distributed(0x40000000L);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1, "invalid direct synchronization target affected the clock");
        return 0;
    }
    CASE("negative-clock")
    {
        reset(-1);
        game_time_synchronize_distributed(1000);
        frame(1.f / TICKS_PER_SECOND);
        CHECK(ticks == 1 && game_time_get() == 0, "negative clock entered catch-up subtraction");
        return 0;
    }
    CASE("initialize-start")
    {
        reset(1000);
        host_update(1008);
        game_time_globals = NULL;
        game_time_synchronize_distributed(2000);
        game_time_initialize();
        CHECK(allocations == 1 && !game_time_globals->initialized && !game_time_globals->active,
              "actual engine initialization did not reset clock storage");
        game_time_initialize_for_new_map();
        game_time_start();
        CHECK(client_starts == 1 && !server_starts && game_time_globals->active,
              "client start did not use its normal queue");
        frame(1.f / TICKS_PER_SECOND + .000001f);
        CHECK(ticks == 1, "old session target leaked through actual initialization and start");
        return 0;
    }
    CHECK(FALSE, "unknown case %s", case_name);
    return 2;
}
