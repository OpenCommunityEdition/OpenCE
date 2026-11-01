#include "harness.h"
#include <math.h>
#include <float.h>
#define REAL_MAX FLT_MAX
#define BOTS_MAXIMUM 63
#define TICKS_PER_SECOND 30
#define BOTS_PATH_RADIUS 0.25f
#define PATH_NODE_LIST_SIZE 16
#define BOTS_PI 3.14159265f
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define csmemset memset
#define platform_log(...) ((void)0)
enum { _bots_goal_none, _bots_goal_item, _bots_goal_vehicle, _bots_goal_enhanced };
enum { _bots_role_attacker, _bots_role_defender };
enum { _unit_control_weapon_primary_trigger_bit, _unit_control_jump_bit };
struct structure_bsp { int unused; };
struct bots_bot { short goal_kind; long goal_surface_index, path_tick, climb_until_tick;
	long target_player_index, target_seen_tick;
    long stuck_tick, stuck_count, unstick_until_tick; real unstick_direction;
    real_point3d stuck_position;
    real_point3d goal_point, last_position; struct { boolean valid; } path; };
static struct { struct bots_bot bots[BOTS_MAXIMUM]; long stuck_events, stuck_giveups; } bots_globals;
static boolean bots_enhanced_running = TRUE;
struct object_datum { struct { long parent_object_index; } object; };
struct bots_tick { long now; boolean moving; real_point3d position; real move_direction;
    struct object_datum *object; unsigned long flags; };
static real bots_random_real(struct bots_bot *bot) { (void)bot; return 0.0f; }
static void bots_pick_roam_goal(struct bots_bot *bot, real_point3d const *position)
{ (void)bot; (void)position; }
struct bots_tactics_bot { short role, lane; boolean lane_given_up; };
static struct bots_tactics_bot bots_tactics_bots[BOTS_MAXIMUM];
struct path_node { real path_distance_from_origin; real_point3d entry_point; };
struct path_state { short node_count, closest_node_index, heap_count; real closest_distance;
    struct path_node node_list[16]; };
struct player_action { struct { real yaw, pitch; } desired_facing; struct { real i, j; } throttle;
    real primary_trigger; unsigned long control_flags; };
static long now = 100;
static long game_time_get(void) { return now; }
static short path_node_from_hash_table(struct path_state *state, long surface)
{ return surface >= 0 && surface < state->node_count ? (short)surface : NONE; }
static real bots_distance3d(real_point3d const *a, real_point3d const *b)
{ real x=a->x-b->x, y=a->y-b->y, z=a->z-b->z; return (real)sqrt(x*x+y*y+z*z); }
static real bots_distance2d(real_point3d const *a, real_point3d const *b)
{ real x=a->x-b->x, y=a->y-b->y; return (real)sqrt(x*x+y*y); }
/* Game allocations use debug_free, which rejects NULL unlike libc free. */
static void checked_free(void *pointer) { CHECK(pointer != NULL, "engine free received NULL"); free(pointer); }
#define free checked_free
#include "under_test.inc"

int main(int argc, char **argv)
{
    struct bots_bot *bot = &bots_globals.bots[0];
    struct bots_nav_route *route = &bots_navigation.routes[0];
    struct bots_nav_link *link = &bots_navigation.links[0];
    struct path_state search;
    real_point3d start = {0,0,0}, goal;
    long start_surface = 0, goal_surface = 1;
    real direction = 0;
    struct player_action action;
    CHECK(argc == 2, "case");
    memset(&search, 0, sizeof(search));
    memset(&action, 0, sizeof(action));
    bot->target_player_index = NONE;
    bots_navigation.count = 1;
    link->entrance.x = 10; link->entrance_surface = 0;
    link->exit.x = 80; link->exit_surface = 1;
    bot->goal_point.x = 100; bot->goal_surface_index = 1; bot->goal_kind = _bots_goal_enhanced;
    search.node_count = 2;
    search.node_list[0].entry_point = link->entrance;
    search.node_list[0].path_distance_from_origin = 10;
    search.node_list[1].entry_point = bot->goal_point;
    search.node_list[1].path_distance_from_origin = 100;
    if (!strcmp(argv[1], "measure") || !strcmp(argv[1], "reject"))
    {
        CHECK(!bots_navigation_answer(bot, &search, &start, TRUE, FALSE, 0.25f), "normal path lost");
        CHECK(route->phase == _bots_nav_exit, "shortcut did not request exit measurement");
        goal = bot->goal_point;
        CHECK(bots_navigation_query(bot, &start, &start_surface, &goal, &goal_surface), "no measuring query");
        CHECK(start.x == 80 && start_surface == 1 && goal.x == 100, "exit query used bot's origin");
        search.node_list[1].path_distance_from_origin = !strcmp(argv[1], "reject") ? 95 : 20;
        CHECK(bots_navigation_answer(bot, &search, &start, TRUE, FALSE, 0.25f), "measurement replaced path");
        if (!strcmp(argv[1], "reject")) CHECK(route->phase == _bots_nav_none, "expensive exit was accepted");
        else
        {
            CHECK(route->phase == _bots_nav_entry, "short exit refused");
            bots_navigation_query(bot, &start, &start_surface, &goal, &goal_surface);
            CHECK(goal.x == 10 && goal_surface == 0, "entrance was not measured");
            CHECK(!bots_navigation_answer(bot, &search, &start, TRUE, FALSE, 0.25f), "entrance path discarded");
            CHECK(route->phase == _bots_nav_going, "validated shortcut not followed");
            bot->last_position.x = 10; start.x = 80;
            bots_navigation_update(bot, &start, FALSE);
            CHECK(route->phase == _bots_nav_none && link->successes == 1, "landing retained route");
        }
    }
    else if (!strcmp(argv[1], "reservation"))
    {
        bots_navigation.routes[1].phase = bots_navigation.routes[2].phase = _bots_nav_going;
        bots_navigation_answer(bot, &search, &start, TRUE, FALSE, 0.25f);
        CHECK(route->phase == _bots_nav_none, "crowded teleporter selected");
        bots_navigation_reset_bot(1); now += 151;
        bots_navigation_answer(bot, &search, &start, TRUE, FALSE, 0.25f);
        CHECK(route->phase == _bots_nav_exit, "released reservation still blocked entrance");
    }
    else if (!strcmp(argv[1], "ladder"))
    {
        route->phase = _bots_nav_going; route->goal = bot->goal_point; route->deadline = 1000;
        link->ladder = TRUE; link->top = 5; link->climb_direction = 1.0f; link->exit = link->entrance; link->exit.z = 5;
        start = link->entrance;
        CHECK(bots_navigation_follow(bot, &start, &direction) && direction == 1.0f, "ladder facing");
        bots_navigation_action(bot, &action);
        CHECK(action.throttle.i == 1 && action.desired_facing.yaw == 1 && TEST_FLAG(action.control_flags, _unit_control_jump_bit), "ladder controls");
        start.z = 5; now += 30;
        bots_navigation_follow(bot, &start, &direction);
        CHECK(route->phase == _bots_nav_none && link->successes == 1, "ladder landing cleanup");
    }
    else if (!strcmp(argv[1], "timeout"))
    {
        route->phase = _bots_nav_going; route->goal = bot->goal_point; route->deadline = now-1;
        bots_navigation_update(bot, &start, FALSE);
        CHECK(route->phase == _bots_nav_none && link->failures == 1 && link->rest_until == now+900, "failure cooldown");
        now += 151;
        bots_navigation_answer(bot, &search, &start, TRUE, FALSE, 0.25f);
        CHECK(route->phase == _bots_nav_none, "failed link retried during cooldown");
    }
    else if (!strcmp(argv[1], "tall_ladder"))
    {
        route->phase = _bots_nav_going; route->goal = bot->goal_point; route->deadline = now + 240;
        link->ladder = TRUE; link->top = 5; start = link->entrance;
        CHECK(bots_navigation_follow(bot, &start, &direction), "climb not started");
        now += 10 * TICKS_PER_SECOND; start.z = 4;
        bots_navigation_update(bot, &start, FALSE);
        CHECK(bots_navigation_follow(bot, &start, &direction) && !link->failures,
            "tall ladder timed out during stock-speed progress");
        now += 6 * TICKS_PER_SECOND;
        CHECK(!bots_navigation_follow(bot, &start, &direction) && link->failures == 1 &&
            link->successes == 0, "stalled ladder has no bounded timeout");
    }
    else if (!strcmp(argv[1], "climb_recovery"))
    {
        struct object_datum unit;
        struct bots_tick tick;
        memset(&tick, 0, sizeof(tick));
        unit.object.parent_object_index = NONE;
        tick.object = &unit; tick.moving = TRUE; tick.now = now;
        tick.position.z = 3;
        route->phase = _bots_nav_going; route->climb_tick = now - 60;
        bot->stuck_count = 2; bot->unstick_until_tick = now + 10;
        bots_bot_unstick(bot, &tick);
        CHECK(!bots_globals.stuck_events && !bot->stuck_count && !bot->unstick_until_tick &&
            bot->stuck_position.z == 3, "climb mistaken for stuck movement");
        route->phase = _bots_nav_none; tick.now += 30;
        bots_bot_unstick(bot, &tick);
        CHECK(bots_globals.stuck_events == 1, "ordinary stuck recovery suppressed");
        bots_enhanced_running = FALSE; route->phase = _bots_nav_going; tick.now += 30;
        bots_bot_unstick(bot, &tick);
        CHECK(bots_globals.stuck_events == 2, "disabled behavior changed");
    }
    else if (!strcmp(argv[1], "lifecycle"))
    {
        route->phase = _bots_nav_going; route->goal = bot->goal_point; route->deadline = 1000;
        bots_navigation_update(bot, &start, TRUE);
        CHECK(!route->phase && !link->failures, "boarding counted as a broken link");
        bots_navigation.vehicle_walkable = (byte *)malloc(17) + 1;
        bots_navigation_reset(); bots_navigation_reset();
        CHECK(!bots_navigation.count && !bots_navigation.vehicle_walkable, "map state retained");
    }
    else CHECK(0, "unknown case");
    return 0;
}
