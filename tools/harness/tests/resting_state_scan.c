/* a resting vehicle must not be starved by the resting-state budget */
#include "harness.h"
typedef unsigned short word;
#include "config.inc"
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAXIMUM_ENTRIES_PER_MESSAGE 4
#define DATAGRAM_ENTRIES(type) 4
#define OWN_VEHICLE_PERIOD_TICKS 1
#define VEHICLE_PREDICTION_REFERENCE_TICKS 60
#define MAXIMUM_TRACKED_PLAYERS 1
#define SET_FLAG(v,b,s) ((v)=(s)?((v)|FLAG(b)):((v)&~FLAG(b)))
enum { _object_at_rest_bit, _distributed_object_predicted_bit, _distributed_message_object_states };
struct distributed_object_state { long object_index; real_point3d position; unsigned long flags; short time; };
struct distributed_object_state_message { word header; struct distributed_object_state states[4]; };
static struct object_datum { struct { unsigned long flags; } object; } objects[MAXIMUM_TRACKED_OBJECTS];
static boolean present[MAXIMUM_TRACKED_OBJECTS], enabled;
static long visits[MAXIMUM_TRACKED_OBJECTS], order[MAXIMUM_TRACKED_OBJECTS + 4], order_count;
static long objects_host_told_count, objects_host_resting_cursor, clock_tick, inspected;
static boolean objects_host_state_moving[MAXIMUM_TRACKED_OBJECTS];
static long objects_host_rest_times[MAXIMUM_TRACKED_OBJECTS];
static struct {
    short count;
    long indices[1];
    struct { short vehicle_count; long vehicle_indices[1]; short vehicle_player_indices[1]; } machines[1];
} objects_host_viewers;
static struct { long accepted_vehicle_index, accepted_host_time, accepted_time; } objects_host_vehicle_predictions[1];
static long game_time_get(void) { return clock_tick; }
static boolean vehicle_riding_enabled(void) { return enabled; }
static long distributed_host_placed_object(long index)
{
    CHECK(index >= 0 && index < objects_host_told_count, "scan outside pool: %ld", index);
    inspected++;
    return present[index] ? index : NONE;
}
static struct object_datum *object_get(long index) { return &objects[index]; }
static void distributed_state_from_object(long index, struct distributed_object_state *state)
{
    memset(state, 0, sizeof(*state)); state->object_index = index;
}
static short distributed_host_object_period(short machine, real_point3d const *p) { return 1; }
static void distributed_send_to_machine(long machine, struct distributed_object_state_message const *message,
    short kind, short count, word size)
{
    short i;
    for (i = 0; i < count; i++)
    {
        long index = message->states[i].object_index;
        CHECK(index >= 0 && index < objects_host_told_count, "sent outside pool");
        visits[index]++;
        CHECK(order_count < MAXIMUM_TRACKED_OBJECTS + 4, "unbounded state count");
        order[order_count++] = index;
    }
}
#include "under_test.inc"

static void setup(long count, long cursor)
{
    long i;
    memset(present, 0, sizeof(present)); memset(visits, 0, sizeof(visits));
    memset(objects_host_state_moving, 0, sizeof(objects_host_state_moving));
    for (i = 0; i < MAXIMUM_TRACKED_OBJECTS; i++)
    {
        objects_host_rest_times[i] = NONE;
        objects[i].object.flags = FLAG(_object_at_rest_bit);
    }
    objects_host_told_count = count; objects_host_resting_cursor = cursor;
    objects_host_viewers.count = 1;
    order_count = inspected = 0; enabled = TRUE; clock_tick = 100;
}
static void send_tick(void)
{
    long before = order_count;
    distributed_host_send_states(); clock_tick++;
    CHECK(order_count - before <= RESTING_STATES_PER_TICK, "resting budget exceeded");
}
int main(int argc, char **argv)
{
    char const *case_name = argv[1];
    long i, tick, count;
    CASE("sparse-hulls")
    {
        setup(114, 71);
        present[0] = present[2] = present[113] = TRUE;
        for (i = 60; i < 96; i++) present[i] = TRUE;
        for (tick = 0; tick < 10; tick++) send_tick();
        CHECK(visits[0] && visits[2], "rested hulls starved: slot 0 sent %ld, slot 2 sent %ld", visits[0], visits[2]);
        for (i = 0; i < 114; i++) CHECK(!present[i] || visits[i], "rested object %ld starved", i);
        return 0;
    }
    CASE("all-rested")
    {
        setup(10, 2);
        for (i = 0; i < 10; i++) present[i] = TRUE;
        for (tick = 0; tick < 3; tick++) send_tick();
        for (i = 0; i < 10; i++) CHECK(visits[i], "rested object %ld starved", i);
        return 0;
    }
    CASE("maximum-pool")
    {
        count = MAXIMUM_TRACKED_OBJECTS; setup(count, count - 2);
        for (i = 0; i < count; i++) present[i] = TRUE;
        for (tick = 0; tick < (count + RESTING_STATES_PER_TICK - 1) / RESTING_STATES_PER_TICK; tick++) send_tick();
        for (i = 0; i < count; i++) CHECK(visits[i] == 1, "maximum pool object %ld sent %ld times", i, visits[i]);
        CHECK(inspected <= tick * count + count, "scan exceeded one pool per tick plus its selected objects");
        return 0;
    }
    CASE("sparse-maximum-pool")
    {
        setup(MAXIMUM_TRACKED_OBJECTS, 0); present[MAXIMUM_TRACKED_OBJECTS - 1] = TRUE;
        send_tick();
        CHECK(visits[MAXIMUM_TRACKED_OBJECTS - 1] == 1, "sparse hull repeated within one scan");
        CHECK(inspected == 2 * MAXIMUM_TRACKED_OBJECTS, "sparse scan must visit each slot once per pass: %ld", inspected);
        return 0;
    }
    CASE("empty-pool")
    {
        setup(0, 0); send_tick();
        CHECK(!order_count && !inspected, "empty pool visited or sent objects");
        return 0;
    }
    CASE("riding-off-original")
    {
        static long const original[] = { 2, 4, 7, 1, 2, 4, 7, 1 };
        setup(10, 2); enabled = FALSE;
        for (i = 0; i < 10; i++) present[i] = TRUE;
        send_tick(); send_tick();
        CHECK(order_count == 8, "legacy state count changed");
        for (i = 0; i < 8; i++) CHECK(order[i] == original[i], "riding off changed send %ld: %ld", i, order[i]);
        return 0;
    }
    return 2;
}
