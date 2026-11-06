/*
MENDICANT_MOVE.C

MENDICANT AI's movement (PORT_SPEC.md section 8, module M3):

- free movement (actor_go): any actor, Marine or Covenant, in an encounter or
  not, walks to any point of the loaded structure BSP, not only to its
  encounter's firing positions. The engine's own raw destination
  (actor_move_to_point) is used, and the path search (path_state_find) knows
  nothing of encounters. Each action re-issues its own destination every
  tick, so the five actor_move_* entries ask mendicant_go_redirect first
  (actor_moving.c): for an ordered actor they become actor_move_to_point(our
  waypoint). A search holds 1024 surfaces, so a long order is planned over a
  coarse graph of the level's firing positions, and walked in legs.
- maneuver slots: every encounter of the loaded part of the level gets extra
  slots in a longer copy of its firing position list, in place only during
  the game tick, so that a drill can order an actor to ground outside its
  encounter's own area.
- the position services the drills use: firing positions of an encounter,
  leashed and ordered selection, cover scoring by ray tests, the drill's
  position search, holding and assigning positions.
- the Marines follow the Chief (absolute player 0's unit).
- the move test (ai.mendicant.move_test_log).

Ported from the ai_brain hook library (actor_go.inc, maneuver.inc and
ai_brain.c), with the same behaviour: every address became the source
function or global, every raw offset the struct member, every detour a call
from source/.
*/

#include "cseries.h"
#include "memory/data.h"
#include "game/game.h"
#include "objects/objects.h"
#include "ai/actors.h"
#include "ai/encounters.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/path.h"
#include "physics/collisions.h"
#include "physics/collision_usage.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mendicant.h"

/* the maneuver copies are calloc'd and freed with the C library's own free,
not cseries.h's debug_free */
#undef malloc
#undef free

/* ---------- the declarations between modules (PORT_SPEC.md section 5.2)

mendicant_internal.h carries the shared types once the modules are put
together; until then this unit defines the ones it uses, exactly as the
specification does. The functions and globals are declared here in either case
(a second identical declaration is allowed). */

#if defined(__has_include)
#if __has_include("mendicant_internal.h")
#include "mendicant_internal.h"
#define MENDICANT_MOVE_HAVE_INTERNAL_HEADER
#endif
#endif

#ifndef MENDICANT_MAX_ACTORS
#define MENDICANT_MAX_ACTORS 1024
#endif
#ifndef MENDICANT_MAX_ENCOUNTERS
#define MENDICANT_MAX_ENCOUNTERS 256
#endif
#ifndef MENDICANT_MAX_POSITIONS
#define MENDICANT_MAX_POSITIONS 512
#endif
#ifndef TEAM_HUMAN
#define TEAM_HUMAN 2
#endif
#ifndef TEAM_COVENANT
#define TEAM_COVENANT 3
#endif
#ifndef MODE_GAME
#define MODE_GAME 0
#endif

#ifndef MENDICANT_MOVE_HAVE_INTERNAL_HEADER
struct mendicant_member { long actor_index; struct actor_datum *actor; };
struct mendicant_geometry { real_point3d const *target; real axis_x, axis_y; };

enum { GO_STATUS_NONE = 0, GO_STATUS_MOVING, GO_STATUS_ARRIVED, GO_STATUS_YIELDING,
	GO_STATUS_NO_GROUND = -1, GO_STATUS_NO_PATH = -2, GO_STATUS_GONE = -3 };
#endif

struct model_bridge;

/* M5 platform */
extern FILE *mendicant_drill_file;
long mendicant_player_unit(void);

/* M2 command */
extern long order_actor[MENDICANT_MAX_ACTORS];
extern short order_position[MENDICANT_MAX_ACTORS];
extern real actor_leash[MENDICANT_MAX_ACTORS][4];
extern long leash_corrections;
int mendicant_team_mode(short team);
int mendicant_side_of(short team);
boolean mendicant_actor_can_drill(struct actor_datum *actor);
void mendicant_order(long actor_index, short position);
unsigned long mendicant_drill_rand(void);

/* M3 movement (this unit) */
int actor_go_to(long actor_index, real_point3d const *point, real tolerance, int urgency);
int actor_go_status(long actor_index);
void actor_go_cancel(long actor_index);
void mendicant_movement_reset(void);
void mendicant_movement_dispose(void);
void mendicant_movement_tick_begin(void);
void mendicant_movement_tick_end(void);
void mendicant_maneuver_check_map(void);
short mendicant_select_position(long actor_index,
	struct firing_position_evaluation_context *evaluation_context,
	struct firing_position *best_firing_position, long *current_owner,
	struct path_state *area_path_state, boolean *area_path_state_valid);
void mendicant_guard_change(long actor_index, short *firing_position_index,
	long *previous_owner, boolean *cached_path_available);
boolean mendicant_go_redirect(short kind, long actor_index, real_point3d const *point, boolean *result);
void mendicant_follow_chief(long time);
void mendicant_follow_cancel(long actor_index);
void mendicant_go_release_drill(long actor_index);
void mendicant_cover_want(int cover, real_point3d const *threat);
struct firing_position_definition *mendicant_encounter_firing_positions(long encounter_index, int *count);
boolean mendicant_firing_position_owned(long encounter_index, short position);
boolean mendicant_position_usable(long encounter_index, short position, struct firing_position_definition const *entry);
int mendicant_drill_position(struct actor_datum *actor, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);
boolean mendicant_drill_move(struct mendicant_member *member, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used);
void mendicant_drill_hold(struct mendicant_member *member, long *used);
boolean mendicant_assign_position(struct mendicant_member *member, short position);
boolean mendicant_go_fallback(struct mendicant_member *member, real_point3d const *point, real tolerance);
int mendicant_maneuver_slot(long encounter_index, real_point3d const *point);
int mendicant_maneuver_near(real_point3d const *point, real radius);
int mendicant_line_blocked(real_point3d const *from, real_point3d const *to);
void mendicant_move_test(long actor_index);
void mendicant_movement_report(struct model_bridge *bridge);

/* Beyond section 5.2: what command.inc's move_near, step orders and plan
entities read directly in the hook library (the cover enum, the reach, the
borrowed ground, the cover score), and the move test's log. */
#ifndef MENDICANT_MOVE_HAVE_INTERNAL_HEADER
enum { MENDICANT_COVER_NONE = 0, MENDICANT_COVER_FIGHTING, MENDICANT_COVER_CONCEALED };
#endif
extern real position_reach_centre[3], position_reach; /* M2: a Covenant defender's post */
extern FILE *mendicant_move_test_file;                /* M5: NULL unless ai.mendicant.move_test_log */
boolean mendicant_within_reach(real_point3d const *point);
int mendicant_cover_score(real_point3d const *position, real_point3d const *threat);
real mendicant_cover_bonus(real_point3d const *position);
real_point3d const *mendicant_maneuver_pool_point(int candidate);

/* not in a header (scenario.c) */
extern struct structure_bsp *global_structure_bsp;

/* ---------- constants */

#define MOVE_MINIMUM(a, b) ((a) < (b) ? (a) : (b))
#define MOVE_MAXIMUM(a, b) ((a) > (b) ? (a) : (b))

/* urgency: 0 yields to fight, charge, flee, avoid and vehicle actions (resumes after);
1 moves while fighting (still flees and dodges); 2 always moves */
#define GO_MAX_ORDERS 64
#define GO_REPLAN_TICKS 45
#define GO_STUCK_TICKS 300
#define GO_STUCK_REPLANS 4
#define GO_MAX_ROUTE 96
#define GO_MAX_NODES 1024
#define GO_NEIGHBOURS 12
#define GO_LINK_RANGE 30.0f /* graph links are searched up to this far (a search reaches ~25-40) */
#define GO_NODE_SPACING 5.0f
#define GO_PLAN_BUDGET 24 /* engine searches per tick for the planner (~0.1-0.25 ms each) */
#define GO_PLAN_EXPANSIONS 600
#define GO_FORCED_ENCOUNTERS 1024
#define GO_START GO_MAX_NODES
#define GO_GOAL (GO_MAX_NODES + 1)

/* front-facing surfaces of the structure: the ground under a point */
#define MOVE_GROUND_COLLISION_FLAGS \
	(FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_structure_bit))
/* both facings, no invisible surfaces (player clip), structure only: cover */
#define MOVE_COVER_COLLISION_FLAGS \
	(FLAG(_collision_test_front_facing_surfaces_bit) | FLAG(_collision_test_back_facing_surfaces_bit) | \
	FLAG(_collision_test_ignore_invisible_surfaces_bit) | FLAG(_collision_test_structure_bit))

#define MANEUVER_SLOTS 48
#define MANEUVER_POOL 6144
#define MANEUVER_MAXIMUM_SQUADS 64

#define COVER_CANDIDATES 6
#define FLANK_HEIGHT_MAXIMUM 8.0f
#define MOVE_RADIANS_TO_DEGREES 57.29578f

#define MOVE_TEST_TICKS 900 /* how long each order is watched: 30 seconds */
#define MOVE_TEST_REPORT_TICKS 30
#define MOVE_TEST_HOLD_TICKS 300 /* keeps the fight code from choosing a new position */
#define MOVE_ARRIVED_DISTANCE 1.5f
#define MOVE_MINIMUM_DISTANCE 6.0f
#define MOVE_MAXIMUM_DISTANCE 15.0f

/* Every Marine who has been with the Chief (within 30 wu) keeps up with him from
then on, over any ground: out of a fight he stays within 10 wu, in one within 28. */
#define FOLLOW_MET 30.0f
#define FOLLOW_CALM 10.0f
#define FOLLOW_FIGHTING 28.0f /* the unit bounds up to him first (unit_update); this is the backstop */

/* ---------- structures */

struct go_order
{
	long actor; /* datum index, NONE free */
	real_point3d goal;
	long goal_surface;
	real tolerance;
	int urgency;
	int status;
	real_point3d waypoint;
	long waypoint_surface;
	boolean waypoint_final;
	long planned_time, progress_time;
	real best_distance;
	int replans_without_progress;
	long encounter; /* encounter we hold force_active, NONE none */
	long issued_time;
	long legs, searches;
	/* the route over the waypoint graph (graph node indices), when the planner found one */
	short route[GO_MAX_ROUTE];
	int route_count, route_index, route_state; /* state: 0 not asked, 1 queued, 2 found, 3 none */
	long shortcut_time;
};

struct go_node
{
	real_point3d point;
	long surface;
	short neighbour[GO_NEIGHBOURS];
	unsigned char link[GO_NEIGHBOURS]; /* 0 unknown, 1 valid, 2 invalid */
	real link_radius[GO_NEIGHBOURS];   /* the pathfinding radius it was valid for */
};

/* ---------- globals */

static boolean move_reset_done;

/* free movement */
static struct go_order go_orders[GO_MAX_ORDERS];
static boolean go_inside;
static unsigned char go_forced_by_us[GO_FORCED_ENCOUNTERS]; /* encounter absolute index: we set force_active */
static struct path_state go_path_state;
static struct { long searches, partial, failed, redirects; } go_stats;

/* the waypoint graph */
static struct go_node go_nodes[GO_MAX_NODES];
static int go_node_count;
static boolean go_graph_built;
static struct scenario *go_graph_scenario;
static short go_graph_bsp = NONE;
static struct { long nodes, links_searched, links_valid, plans, plans_found, plan_searches; } go_graph_stats;

/* the planner: one order at a time, a budget of searches per tick; node ids are
graph nodes, then GO_START (the actor) and GO_GOAL */
static struct
{
	int order; /* -1 idle */
	real g[GO_MAX_NODES + 2], f[GO_MAX_NODES + 2];
	short parent[GO_MAX_NODES + 2];
	unsigned char open[GO_MAX_NODES + 2], closed[GO_MAX_NODES + 2];
	short start_link[GO_MAX_NODES]; /* 0 unknown, 1 valid, 2 invalid: from the actor */
	short goal_link[GO_MAX_NODES];  /* from a node to the goal */
	int expansions, budget;
	real radius;
} go_planner;
static int go_planner_candidates[GO_MAX_NODES + 1], go_planner_slots[GO_MAX_NODES + 1];

/* maneuver slots */
static struct scenario *maneuver_scenario; /* the scenario the slots were made for */
static short maneuver_bsp = NONE;
static long maneuver_original[MENDICANT_MAX_ENCOUNTERS]; /* each encounter's own count; 0: not extended */
static struct firing_position_definition *maneuver_block[MENDICANT_MAX_ENCOUNTERS];     /* our copy, freed with the map */
static struct firing_position_definition *maneuver_own_block[MENDICANT_MAX_ENCOUNTERS]; /* the level's own list */
static boolean maneuver_swapped; /* our copies are in place (only during a game tick) */
static short maneuver_next[MENDICANT_MAX_ENCOUNTERS];
static real_point3d maneuver_pool[MANEUVER_POOL]; /* every encounter's own positions, this part */
static int maneuver_pool_count;
static boolean maneuver_pool_built;

/* cover: what the next position search prefers, and from where (the enemy's eye) */
static int cover_wanted;
static real_point3d cover_threat;

/* actors under a free-movement order from a drill (no position fitted) or from
following the Chief: a position order ends it, so the drill has the actor again */
static unsigned char go_by_drill[MENDICANT_MAX_ACTORS];
static unsigned char follow_going[MENDICANT_MAX_ACTORS];
static long go_fallbacks;

/* following the Chief */
static short follow_identifier[MENDICANT_MAX_ACTORS];
static unsigned char follow_met[MENDICANT_MAX_ACTORS];
static long follow_time[MENDICANT_MAX_ACTORS];
static real_point3d follow_goal[MENDICANT_MAX_ACTORS];
static long marines_hops, marines_stuck;

/* the move test: one fighting enemy at a time ordered to a position we choose */
static long move_test_actor = NONE;
static long move_test_start_time;
static long move_test_report_time;
static real_point3d move_test_point;

/* ---------- lookups */

/* the game's own datum access, NULL for a stale index (actor_get, go_datum) */
static void *move_datum(struct data_array *data, long index)
{
	if (!data || !data->valid || index == NONE)
		return NULL;
	return datum_try_and_get(data, index);
}

static struct actor_datum *move_actor(long actor_index)
{
	return (struct actor_datum *)move_datum(actor_data, actor_index);
}

/* an object's datum (object_pointer) */
static struct object_datum *move_object(long object_index)
{
	struct object_header_datum *header = (struct object_header_datum *)move_datum(object_header_data, object_index);

	return header ? header->datum : NULL;
}

/* the actor at an absolute index of actor_data, or NULL when the slot is free */
static struct actor_datum *move_actor_at(short absolute_index)
{
	struct actor_datum *actor = (struct actor_datum *)((byte *)actor_data->data + actor_data->size * absolute_index);

	return actor->identifier ? actor : NULL;
}

static long move_actor_index(short identifier, short absolute_index)
{
	return (long)(((unsigned long)(unsigned short)identifier << 16) | (unsigned long)(unsigned short)absolute_index);
}

static short move_slot(long actor_index)
{
	return (short)actor_index;
}

static real move_distance(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

	return (real)sqrt(dx * dx + dy * dy + dz * dz);
}

/* from an actor's pathfinding point (distance_to) */
static real move_distance_to(struct actor_datum *actor, real_point3d const *point)
{
	return move_distance(&actor->input.pathfinding_point, point);
}

static real move_signed_angle(real from_x, real from_y, real to_x, real to_y)
{
	return (real)atan2(from_x * to_y - from_y * to_x, from_x * to_x + from_y * to_y) * MOVE_RADIANS_TO_DEGREES;
}

/* the ground under (or at) a point: its surface index, and the point on it */
static boolean move_ground_ray(real_point3d const *point, real depth, struct collision_result *collision)
{
	real_point3d from;
	real_vector3d down;

	if (!global_structure_bsp || global_current_collision_user_depth < 1)
		return FALSE;
	from.x = point->x;
	from.y = point->y;
	from.z = point->z + 1.0f;
	down.i = 0.0f;
	down.j = 0.0f;
	down.k = -depth;
	return collision_test_vector(MOVE_GROUND_COLLISION_FLAGS, &from, &down, NONE, collision);
}

/* ---------- free movement: calls into the path code */

static boolean go_ground(real_point3d const *point, real_point3d *ground, long *surface)
{
	struct collision_result collision;

	if (!move_ground_ray(point, 6.0f, &collision))
		return FALSE;
	*ground = collision.point;
	*surface = collision.surface_index;
	return *surface >= 0;
}

/* One A* search from the actor's own pathfinding location (the path input as
actor_path_refresh makes it: radius, vehicle, broken surfaces) with an accept
radius: the endpoint is the goal when the search reaches its surface, else the
closest point it found. 1 = goal, 2 = partial, 0 = none. */
static int go_search_from(long actor_index, real_point3d const *start, long start_surface,
	real_point3d const *goal, long goal_surface, real_point3d *endpoint, long *endpoint_surface)
{
	struct path_state *state = &go_path_state;
	struct path_result result;
	boolean found;

	memset(state, 0, sizeof(*state));
	actor_path_input_new(actor_index, &state->input);
	if (start)
	{
		/* the actor's radius and ignore rules, another start: a link of the graph */
		state->input.start_valid = TRUE;
		state->input.start_point = *start;
		state->input.start_surface_index = start_surface;
	}
	state->debug = NULL;
	state->destination_valid = TRUE;
	state->destination.point = *goal;
	state->destination.surface_index = goal_surface;
	state->destination.target_radius = 100000.0f; /* accept radius: the closest point */
	state->structure = global_structure_bsp;
	go_stats.searches++;
	memset(&result, 0, sizeof(result));
	found = path_state_find(state);
	if (!found || !path_state_build_path(state, &result) || !result.valid)
	{
		go_stats.failed++;
		return 0;
	}
	*endpoint = result.endpoint.point;
	*endpoint_surface = result.endpoint.surface_index;
	if (*endpoint_surface == goal_surface)
		return 1;
	go_stats.partial++;
	return 2;
}

static int go_search(long actor_index, real_point3d const *goal, long goal_surface,
	real_point3d *endpoint, long *endpoint_surface)
{
	return go_search_from(actor_index, NULL, NONE, goal, goal_surface, endpoint, endpoint_surface);
}

static struct go_order *go_find(long actor_index)
{
	int i;

	for (i = 0; i < GO_MAX_ORDERS; i++)
		if (go_orders[i].actor != NONE && go_orders[i].actor == actor_index)
			return &go_orders[i];
	return NULL;
}

/* ---------- the waypoint graph: a route past dead ends

One search reaches the goal's surface for most goals under 25 wu, half at
25-50, few past 75 (1024 nodes), and the closest point of a capped search is a
dead end whenever the way round goes away from the goal first. So long orders
are planned over a coarse graph: its nodes are every encounter's firing
positions of the loaded BSP, at least GO_NODE_SPACING apart, linked to their
nearest neighbours within GO_LINK_RANGE; a link is valid when one engine search
from one end reaches the other's surface. Links are searched lazily by an A*
over the graph (GO_PLAN_BUDGET searches per tick) and remembered for the map
(directed: drops are one way). */

static void go_graph_check(void)
{
	int i;

	if (global_scenario == go_graph_scenario && global_structure_bsp_index == go_graph_bsp)
		return;
	go_graph_scenario = global_scenario;
	go_graph_bsp = global_structure_bsp_index;
	go_graph_built = FALSE;
	go_node_count = 0;
	for (i = 0; i < GO_MAX_ORDERS; i++)
	{
		go_orders[i].route_state = go_orders[i].route_count = 0;
		/* goals and waypoints are surface indices of the old BSP: never hand them to the path code */
		if (go_orders[i].actor != NONE && go_orders[i].status > 0)
			go_orders[i].status = GO_STATUS_NO_PATH;
	}
}

static void go_graph_build(void)
{
	struct scenario *scenario = global_scenario;
	struct encounter_definition *definitions;
	long encounters;
	int e, k, n, m;

	go_graph_built = TRUE;
	go_node_count = 0;
	if (!scenario)
		return;
	encounters = scenario->ai_encounters.count;
	definitions = (struct encounter_definition *)scenario->ai_encounters.address;
	for (e = 0; e < encounters && e < 512; e++)
	{
		struct encounter_definition *definition = &definitions[e];
		struct firing_position_definition *positions;
		long count;

		if (definition->runtime_structure_bsp_reference_index != go_graph_bsp)
			continue;
		count = definition->firing_positions.count;
		positions = (struct firing_position_definition *)definition->firing_positions.address;
		for (k = 0; positions && k < count && k < 512 && go_node_count < GO_MAX_NODES; k++)
		{
			real_point3d const *p = &positions[k].position;
			boolean near = FALSE;
			struct go_node *node;

			if (positions[k].surface_index < 0)
				continue; /* flying positions and unused maneuver slots */
			for (n = 0; n < go_node_count && !near; n++)
				near = move_distance(&go_nodes[n].point, p) < GO_NODE_SPACING;
			if (near)
				continue;
			node = &go_nodes[go_node_count];
			if (!go_ground(p, &node->point, &node->surface))
				continue;
			go_node_count++;
		}
	}
	/* nearest neighbours within range */
	for (n = 0; n < go_node_count; n++)
	{
		struct go_node *node = &go_nodes[n];
		real best[GO_NEIGHBOURS];
		int found = 0;

		for (m = 0; m < go_node_count; m++)
		{
			real d = move_distance(&node->point, &go_nodes[m].point);
			int at;

			if (m == n || d > GO_LINK_RANGE)
				continue;
			at = found < GO_NEIGHBOURS ? found++ : GO_NEIGHBOURS;
			if (at == GO_NEIGHBOURS)
			{
				if (d >= best[GO_NEIGHBOURS - 1])
					continue;
				at = GO_NEIGHBOURS - 1;
			}
			while (at > 0 && best[at - 1] > d)
			{
				best[at] = best[at - 1];
				node->neighbour[at] = node->neighbour[at - 1];
				at--;
			}
			best[at] = d;
			node->neighbour[at] = (short)m;
		}
		for (k = found; k < GO_NEIGHBOURS; k++)
			node->neighbour[k] = NONE;
		memset(node->link, 0, sizeof(node->link));
	}
	go_graph_stats.nodes = go_node_count;
}

static real_point3d const *go_id_point(struct go_order *order, struct actor_datum *actor, int id)
{
	if (id == GO_START)
		return &actor->input.pathfinding_point;
	if (id == GO_GOAL)
		return &order->goal;
	return &go_nodes[id].point;
}

/* Is the link searched (and valid)? -1: out of budget for this tick. */
static int go_link(struct go_order *order, int from, int to, int slot)
{
	unsigned char *known = NULL;
	short *known16 = NULL;
	int state;
	real_point3d endpoint;
	real_point3d const *goal;
	long surface, goal_surface;
	int found;
	boolean valid;

	if (from == GO_START)
		known16 = &go_planner.start_link[to];
	else if (to == GO_GOAL)
		known16 = &go_planner.goal_link[from];
	else
		known = &go_nodes[from].link[slot];
	state = known16 ? *known16 : *known;
	if (!known16 && state == 1 && go_nodes[from].link_radius[slot] + 0.05f < go_planner.radius)
		state = 0; /* valid for a slimmer actor: search again */
	if (state)
		return state == 1;
	if (go_planner.budget <= 0)
		return -1;
	go_planner.budget--;
	go_graph_stats.plan_searches++;
	goal = to == GO_GOAL ? &order->goal : &go_nodes[to].point;
	goal_surface = to == GO_GOAL ? order->goal_surface : go_nodes[to].surface;
	found = from == GO_START ? go_search(order->actor, goal, goal_surface, &endpoint, &surface) :
		go_search_from(order->actor, &go_nodes[from].point, go_nodes[from].surface, goal, goal_surface,
			&endpoint, &surface);
	valid = found == 1 && move_distance(&endpoint, goal) < 1.0f;
	if (known16)
	{
		*known16 = valid ? 1 : 2;
	}
	else
	{
		go_graph_stats.links_searched++;
		go_graph_stats.links_valid += valid;
		*known = valid ? 1 : 2;
		go_nodes[from].link_radius[slot] = go_planner.radius;
	}
	return valid;
}

static void go_planner_begin(int index, struct actor_datum *actor)
{
	struct go_order *order = &go_orders[index];
	struct path_input input;
	int i;

	go_planner.order = index;
	go_planner.expansions = 0;
	for (i = 0; i < GO_MAX_NODES + 2; i++)
	{
		go_planner.g[i] = 1e30f;
		go_planner.open[i] = go_planner.closed[i] = 0;
		go_planner.parent[i] = NONE;
	}
	memset(go_planner.start_link, 0, sizeof(go_planner.start_link));
	memset(go_planner.goal_link, 0, sizeof(go_planner.goal_link));
	memset(&input, 0, sizeof(input));
	actor_path_input_new(order->actor, &input);
	go_planner.radius = input.pathfinding_radius;
	go_planner.g[GO_START] = 0.0f;
	go_planner.f[GO_START] = move_distance(go_id_point(order, actor, GO_START), &order->goal);
	go_planner.open[GO_START] = 1;
	go_graph_stats.plans++;
}

static void go_planner_finish(struct go_order *order, boolean found)
{
	order->route_count = 0;
	order->route_index = 0;
	if (found)
	{
		short reversed[GO_MAX_ROUTE];
		int count = 0, id, k;

		for (id = go_planner.parent[GO_GOAL]; id >= 0 && id != GO_START && count < GO_MAX_ROUTE; id = go_planner.parent[id])
			reversed[count++] = (short)id;
		for (k = 0; k < count; k++)
			order->route[k] = reversed[count - 1 - k];
		order->route_count = count;
		go_graph_stats.plans_found++;
	}
	order->route_state = found ? 2 : 3;
	order->best_distance = 1e30f; /* measured along the new route from now on */
	go_planner.order = -1;
}

/* A* with lazily searched links; returns when done or out of this tick's budget. */
static void go_planner_step(void)
{
	struct go_order *order;
	struct actor_datum *actor;
	int i;

	go_planner.budget = GO_PLAN_BUDGET;
	if (go_planner.order < 0)
	{
		for (i = 0; i < GO_MAX_ORDERS; i++)
		{
			if (go_orders[i].actor != NONE && go_orders[i].status > 0 && go_orders[i].route_state == 1)
			{
				actor = move_actor(go_orders[i].actor);
				if (actor)
				{
					go_planner_begin(i, actor);
					break;
				}
			}
		}
		if (go_planner.order < 0)
			return;
	}
	order = &go_orders[go_planner.order];
	actor = move_actor(order->actor);
	if (!actor || order->status <= 0 || order->route_state != 1)
	{
		go_planner.order = -1;
		return;
	}
	while (go_planner.budget > 0)
	{
		int u = -1, id, count = 0, c, k, n;
		real best = 1e30f;
		real_point3d const *at;
		boolean out_of_budget = FALSE;

		for (id = 0; id < GO_MAX_NODES + 2; id++)
		{
			if (go_planner.open[id] && go_planner.f[id] < best)
			{
				best = go_planner.f[id];
				u = id;
			}
		}
		if (u < 0 || go_planner.expansions >= GO_PLAN_EXPANSIONS)
		{
			go_planner_finish(order, FALSE);
			return;
		}
		if (u == GO_GOAL)
		{
			go_planner_finish(order, TRUE);
			return;
		}
		at = go_id_point(order, actor, u);
		/* the links of u: its neighbours (the actor: every node in range), and the goal */
		if (u == GO_START)
		{
			for (n = 0; n < go_node_count; n++)
			{
				if (move_distance(at, &go_nodes[n].point) < GO_LINK_RANGE)
				{
					go_planner_candidates[count] = n;
					go_planner_slots[count++] = -1;
				}
			}
		}
		else
		{
			for (k = 0; k < GO_NEIGHBOURS && go_nodes[u].neighbour[k] >= 0; k++)
			{
				go_planner_candidates[count] = go_nodes[u].neighbour[k];
				go_planner_slots[count++] = k;
			}
			if (move_distance(at, &order->goal) < GO_LINK_RANGE)
			{
				go_planner_candidates[count] = GO_GOAL;
				go_planner_slots[count++] = -1;
			}
		}
		for (c = 0; c < count; c++)
		{
			int v = go_planner_candidates[c], valid;
			real cost;

			if (go_planner.closed[v])
				continue;
			cost = go_planner.g[u] + move_distance(at, go_id_point(order, actor, v));
			if (cost >= go_planner.g[v])
				continue;
			valid = go_link(order, u, v, go_planner_slots[c]);
			if (valid < 0)
			{
				out_of_budget = TRUE;
				break;
			}
			if (!valid)
				continue;
			go_planner.g[v] = cost;
			go_planner.f[v] = cost + move_distance(go_id_point(order, actor, v), &order->goal);
			go_planner.parent[v] = (short)u;
			go_planner.open[v] = 1;
		}
		if (out_of_budget)
			return; /* u stays open: its links are finished next tick (the searched ones are known) */
		go_planner.open[u] = 0;
		go_planner.closed[u] = 1;
		go_planner.expansions++;
	}
}

/* A link the actor could not walk after all (stuck on it): never offered again on this map. */
static void go_forget_link(int from, int to)
{
	int k;

	if (from < 0 || from >= go_node_count)
		return;
	for (k = 0; k < GO_NEIGHBOURS; k++)
		if (go_nodes[from].neighbour[k] == to)
			go_nodes[from].link[k] = 2;
}

static boolean go_yields(struct go_order *order, struct actor_datum *actor)
{
	short action;

	if (actor->input.vehicle_passenger)
		return TRUE;
	action = actor->state.action;
	if (order->urgency >= 2)
		return FALSE;
	if (action == _actor_action_flee || action == _actor_action_avoid)
		return TRUE;
	return order->urgency == 0 &&
		(action == _actor_action_fight || action == _actor_action_charge || action == _actor_action_vehicle);
}

/* An encounter's actors only update while it is active: an ordered actor's
encounter is held force_active while it has orders. */
static void go_hold_encounter(struct go_order *order, struct actor_datum *actor)
{
	long encounter = actor->meta.encounter_index;
	struct encounter_datum *datum;
	int absolute = encounter & 0xffff;

	if (encounter == order->encounter)
		return;
	order->encounter = NONE;
	datum = (struct encounter_datum *)move_datum(encounter_data, encounter);
	if (!datum || absolute >= GO_FORCED_ENCOUNTERS)
		return;
	if (!datum->force_active)
	{
		datum->force_active = TRUE;
		go_forced_by_us[absolute] = 1;
	}
	order->encounter = encounter;
}

static void go_release_encounters(void)
{
	int absolute, i;

	for (absolute = 0; absolute < GO_FORCED_ENCOUNTERS; absolute++)
	{
		struct encounter_datum *datum;
		boolean held = FALSE;

		if (!go_forced_by_us[absolute])
			continue;
		for (i = 0; i < GO_MAX_ORDERS; i++)
			if (go_orders[i].actor != NONE && go_orders[i].status > 0 && (go_orders[i].encounter & 0xffff) == absolute)
				held = TRUE;
		if (held)
			continue;
		go_forced_by_us[absolute] = 0;
		/* (the absolute index alone: any identifier) */
		datum = (struct encounter_datum *)move_datum(encounter_data, absolute);
		if (datum)
			datum->force_active = FALSE;
	}
}

static void go_plan(struct go_order *order, long time)
{
	real_point3d endpoint;
	long surface;
	int found;

	order->planned_time = time;
	order->searches++;
	found = go_search(order->actor, &order->goal, order->goal_surface, &endpoint, &surface);
	if (found == 1 || (found == 2 && move_distance(&endpoint, &order->goal) <= order->tolerance))
	{
		order->waypoint = order->goal;
		order->waypoint_surface = order->goal_surface;
		order->waypoint_final = TRUE;
	}
	else if (found == 2)
	{
		if (!order->waypoint_final && order->waypoint_surface == surface)
			return;
		order->waypoint = endpoint;
		order->waypoint_surface = surface;
		order->waypoint_final = FALSE;
		order->legs++;
	}
	else if (order->waypoint_surface < 0)
	{
		/* no search from here (the actor is off the ground): head for the goal as asked */
		order->waypoint = order->goal;
		order->waypoint_surface = order->goal_surface;
		order->waypoint_final = TRUE;
	}
}

/* Once per game tick, before the AI update. */
static void go_update(void)
{
	long time;
	int i, k;

	if (!actor_data)
		return;
	time = game_time_get();
	go_graph_check();
	for (i = 0; i < GO_MAX_ORDERS && !go_graph_built; i++)
		if (go_orders[i].actor != NONE && go_orders[i].status > 0 && global_current_collision_user_depth >= 1)
			go_graph_build();
	go_planner_step();
	for (i = 0; i < GO_MAX_ORDERS; i++)
	{
		struct go_order *order = &go_orders[i];
		struct actor_datum *actor;
		real_point3d const *at;
		real remaining, along;

		if (order->actor == NONE || order->status <= 0)
			continue;
		actor = move_actor(order->actor);
		if (!actor)
		{
			order->status = GO_STATUS_GONE;
			continue;
		}
		go_hold_encounter(order, actor);
		at = &actor->input.pathfinding_point;
		remaining = move_distance(at, &order->goal);
		if (remaining <= order->tolerance)
			order->status = GO_STATUS_ARRIVED;
		else if (order->status == GO_STATUS_ARRIVED && remaining > order->tolerance + 2.0f)
			order->status = GO_STATUS_MOVING; /* pushed off the point: go back */
		if (go_yields(order, actor))
		{
			if (order->status != GO_STATUS_ARRIVED)
				order->status = GO_STATUS_YIELDING;
			order->progress_time = time; /* fighting is not being stuck */
			continue;
		}
		if (order->status == GO_STATUS_YIELDING)
			order->status = GO_STATUS_MOVING;
		/* progress is along the route: a way round first goes away from the goal */
		along = remaining;
		if (order->route_state == 2 && order->route_index < order->route_count)
		{
			real_point3d const *from = at;

			along = 0.0f;
			for (k = order->route_index; k < order->route_count; k++)
			{
				along += move_distance(from, &go_nodes[order->route[k]].point);
				from = &go_nodes[order->route[k]].point;
			}
			along += move_distance(from, &order->goal);
		}
		if (along < order->best_distance - 1.0f)
		{
			order->best_distance = along;
			order->progress_time = time;
			order->replans_without_progress = 0;
		}
		if (order->status == GO_STATUS_MOVING)
		{
			if (order->waypoint_surface < 0)
			{
				/* first: one search straight there; past its reach, a route over the graph
				(queued for the planner) while the actor heads for the closest point */
				go_plan(order, time);
				order->route_state = order->waypoint_final ? 2 : 1;
				order->route_count = order->route_index = 0;
				order->shortcut_time = time;
			}
			if (order->route_state == 2 && order->route_index < order->route_count)
			{
				struct go_node *node = &go_nodes[order->route[order->route_index]];

				if (move_distance(at, &node->point) < 2.0f)
				{
					order->route_index++;
					order->legs++;
				}
				if (time - order->shortcut_time >= GO_REPLAN_TICKS)
				{
					/* the goal, or the node after next, may be in one search's reach now */
					real_point3d endpoint;
					long surface;

					order->shortcut_time = time;
					order->searches++;
					if (go_search(order->actor, &order->goal, order->goal_surface, &endpoint, &surface) == 1)
					{
						order->route_index = order->route_count;
					}
					else if (order->route_index + 1 < order->route_count)
					{
						struct go_node *after = &go_nodes[order->route[order->route_index + 1]];

						if (go_search(order->actor, &after->point, after->surface, &endpoint, &surface) == 1 &&
							move_distance(at, &after->point) < GO_LINK_RANGE)
							order->route_index++;
					}
				}
				if (order->route_index < order->route_count)
				{
					node = &go_nodes[order->route[order->route_index]];
					order->waypoint = node->point;
					order->waypoint_surface = node->surface;
					order->waypoint_final = FALSE;
				}
				else
				{
					order->waypoint = order->goal;
					order->waypoint_surface = order->goal_surface;
					order->waypoint_final = TRUE;
				}
			}
			else if (order->route_state == 2)
			{
				order->waypoint = order->goal;
				order->waypoint_surface = order->goal_surface;
				order->waypoint_final = TRUE;
			}
			else if (!order->waypoint_final && (time - order->planned_time >= GO_REPLAN_TICKS ||
				move_distance(at, &order->waypoint) < 1.5f))
			{
				go_plan(order, time); /* planning, or no route: the closest point it can reach */
			}
			if (time - order->progress_time > GO_STUCK_TICKS)
			{
				order->progress_time = time;
				if (++order->replans_without_progress > GO_STUCK_REPLANS)
				{
					order->status = GO_STATUS_NO_PATH;
					continue;
				}
				if (order->route_state == 2 && order->route_index < order->route_count && order->route_index > 0)
					go_forget_link(order->route[order->route_index - 1], order->route[order->route_index]);
				if (order->route_state != 1)
				{
					order->route_state = 1; /* plan again from here */
					order->route_count = order->route_index = 0;
					order->waypoint_final = FALSE;
					go_plan(order, time);
				}
			}
		}
		go_inside = TRUE;
		actor_move_to_point(order->actor, &order->waypoint, order->waypoint_surface, NONE);
		go_inside = FALSE;
	}
	go_release_encounters();
}

/* ---------- free movement: the redirect of the actor_move_* entries (actor_moving.c)

FALSE lets the game's function run; TRUE means the ordered actor went to its
waypoint instead, and *result is what the entry returns. */

boolean mendicant_go_redirect(short kind, long actor_index, real_point3d const *point, boolean *result)
{
	struct go_order *order;
	struct actor_datum *actor;

	if (go_inside)
		return FALSE;
	order = go_find(actor_index);
	if (!order || (order->status != GO_STATUS_MOVING && order->status != GO_STATUS_ARRIVED) ||
		order->waypoint_surface < 0)
		return FALSE;
	actor = move_actor(actor_index);
	if (!actor || go_yields(order, actor))
		return FALSE;
	if (kind == _mendicant_move_point && point && move_distance(point, &order->waypoint) < 0.1f)
		return FALSE;
	go_stats.redirects++;
	go_inside = TRUE;
	*result = actor_move_to_point(actor_index, &order->waypoint, order->waypoint_surface, NONE);
	go_inside = FALSE;
	return TRUE;
}

/* ---------- free movement: the API */

/* Orders an actor (datum index) to a point; tolerance in world units (min 0.5).
1 = accepted (the walk starts at the next tick), 0 = bad actor, -1 = no ground
under the point. A new order replaces the actor's old one. Call inside the
game tick. */
int actor_go_to(long actor_index, real_point3d const *point, real tolerance, int urgency)
{
	real_point3d ground;
	long surface, encounter;
	struct go_order *order;
	int i;

	if (!move_actor(actor_index))
		return 0;
	if (!go_ground(point, &ground, &surface))
		return -1;
	order = go_find(actor_index);
	for (i = 0; !order && i < GO_MAX_ORDERS; i++)
		if (go_orders[i].actor == NONE || go_orders[i].status <= 0)
			order = &go_orders[i];
	if (!order)
		return 0;
	encounter = order->actor == actor_index ? order->encounter : NONE;
	memset(order, 0, sizeof(*order));
	order->actor = actor_index;
	order->encounter = encounter;
	order->goal = ground;
	order->goal_surface = surface;
	order->tolerance = tolerance < 0.5f ? 0.5f : tolerance;
	order->urgency = urgency;
	order->status = GO_STATUS_MOVING;
	order->waypoint_surface = NONE; /* planned at the next update */
	order->best_distance = 1e30f;
	order->issued_time = order->progress_time = game_time_get();
	return 1;
}

int actor_go_status(long actor_index)
{
	struct go_order *order = go_find(actor_index);

	return order ? order->status : GO_STATUS_NONE;
}

/* Ends an order; the actor's own action takes it from there (next tick it picks its positions). */
void actor_go_cancel(long actor_index)
{
	struct go_order *order = go_find(actor_index);

	if (order)
	{
		order->actor = NONE;
		order->status = GO_STATUS_NONE;
	}
	go_release_encounters();
}

/* ---------- maneuver slots: ground beyond a unit's own area

An actor can only be ordered to a firing position of its own encounter, and an
encounter's positions end at its area. So every encounter of the loaded part
of the level gets extra slots in its own firing position list (a copy of the
list with room added; at most 512 in all), and a step that wants ground outside
the area borrows a spot from any encounter's positions there: the spot is
copied into a free slot of the actor's own encounter, with its ground, surface
and cluster from a ray down, and the actor is ordered to it. The slots carry a
position group no squad of the encounter uses, so the game never gives them to
anyone itself. Moving actors into other encounters instead changes their team,
wakes the encounter and confuses its scripts, so it is never done. */

static struct encounter_definition *maneuver_encounter_definition(int index)
{
	struct scenario *scenario = global_scenario;

	if (!scenario || index < 0 || index >= scenario->ai_encounters.count || index >= MENDICANT_MAX_ENCOUNTERS)
		return NULL;
	return (struct encounter_definition *)scenario->ai_encounters.address + index;
}

/* Our longer lists are in the level's encounter definitions only while the game
ticks: a level load, a reset or a saved game must only ever see the level's own
(a reset re-reads them). */
static void maneuver_swap(boolean in)
{
	int index;

	if (!global_scenario || global_scenario != maneuver_scenario)
	{
		maneuver_swapped = FALSE;
		return;
	}
	for (index = 0; index < MENDICANT_MAX_ENCOUNTERS; index++)
	{
		struct encounter_definition *definition;

		if (!maneuver_original[index] || !(definition = maneuver_encounter_definition(index)))
			continue;
		definition->firing_positions.address = in ? maneuver_block[index] : maneuver_own_block[index];
		definition->firing_positions.count = maneuver_original[index] + (in ? MANEUVER_SLOTS : 0);
	}
	maneuver_swapped = in;
}

/* the copies are freed and every per-map value forgotten */
static void maneuver_forget(void)
{
	int index;

	for (index = 0; index < MENDICANT_MAX_ENCOUNTERS; index++)
	{
		free(maneuver_block[index]);
		maneuver_block[index] = NULL;
		maneuver_own_block[index] = NULL;
	}
	memset(maneuver_original, 0, sizeof(maneuver_original));
	memset(maneuver_next, 0, sizeof(maneuver_next));
}

/* A new level or part of it: the old copies go (the definitions they patched went with the map). */
void mendicant_maneuver_check_map(void)
{
	struct scenario *scenario = global_scenario;
	short bsp = global_structure_bsp_index;
	int index;
	long slot;

	if (scenario == maneuver_scenario && bsp == maneuver_bsp)
		return;
	if (scenario != maneuver_scenario)
	{
		maneuver_forget();
	}
	else
	{
		/* the same level, another part: the slots' ground is gone, they stay unusable */
		for (index = 0; index < MENDICANT_MAX_ENCOUNTERS; index++)
		{
			struct encounter_definition *definition = maneuver_encounter_definition(index);
			struct firing_position_definition *positions;

			if (!definition || !maneuver_original[index])
				continue;
			positions = (struct firing_position_definition *)definition->firing_positions.address;
			for (slot = maneuver_original[index]; slot < definition->firing_positions.count; slot++)
				positions[slot].surface_index = NONE;
		}
	}
	maneuver_scenario = scenario;
	maneuver_bsp = bsp;
	maneuver_pool_built = FALSE;
}

/* Every encounter's own positions in the loaded part: the ground a maneuver may borrow. */
static void maneuver_build_pool(void)
{
	struct scenario *scenario = global_scenario;
	long encounters;
	int index, position;

	maneuver_pool_count = 0;
	maneuver_pool_built = TRUE;
	if (!scenario)
		return;
	encounters = scenario->ai_encounters.count;
	for (index = 0; index < encounters && index < MENDICANT_MAX_ENCOUNTERS; index++)
	{
		struct encounter_definition *definition = maneuver_encounter_definition(index);
		struct firing_position_definition *positions;
		long own;

		if (!definition || definition->runtime_structure_bsp_reference_index != maneuver_bsp)
			continue;
		own = maneuver_original[index] ? maneuver_original[index] : definition->firing_positions.count;
		positions = (struct firing_position_definition *)definition->firing_positions.address;
		for (position = 0; positions && position < own && maneuver_pool_count < MANEUVER_POOL; position++)
			maneuver_pool[maneuver_pool_count++] = positions[position].position;
	}
}

/* The encounter's list with room for the slots, made once per map. */
static boolean maneuver_extend(int index)
{
	struct encounter_definition *definition;
	struct firing_position_definition *positions, *block;
	struct squad_definition *squads;
	unsigned long used = 0;
	long count, slot;
	int index_squad, mask, group;

	if (!maneuver_swapped)
		return FALSE; /* only inside the game tick */
	definition = maneuver_encounter_definition(index);
	if (!definition)
		return FALSE;
	if (maneuver_original[index])
		return TRUE;
	count = definition->firing_positions.count;
	if (count <= 0 || count + MANEUVER_SLOTS > MENDICANT_MAX_POSITIONS ||
		definition->runtime_structure_bsp_reference_index != maneuver_bsp)
		return FALSE;
	/* a group no squad of the encounter uses, so the game never picks a slot for anyone */
	squads = (struct squad_definition *)definition->squads.address;
	for (index_squad = 0; squads && index_squad < definition->squads.count && index_squad < MANEUVER_MAXIMUM_SQUADS; index_squad++)
		for (mask = 0; mask < MAXIMUM_NUMBER_OF_FIRING_POSITION_GROUPS; mask++)
			used |= squads[index_squad].firing_position_groups[mask];
	group = 31;
	while (group >= 0 && (used >> group & 1))
		group--;
	if (group < 0)
		return FALSE;
	block = (struct firing_position_definition *)calloc(count + MANEUVER_SLOTS, sizeof(struct firing_position_definition));
	if (!block)
		return FALSE;
	positions = (struct firing_position_definition *)definition->firing_positions.address;
	memcpy(block, positions, count * sizeof(struct firing_position_definition));
	for (slot = count; slot < count + MANEUVER_SLOTS; slot++)
	{
		struct firing_position_definition *entry = &block[slot];

		*entry = positions[0]; /* the unknown bytes, as the first's */
		entry->group_index = (short)group;
		/* the cluster stays the first position's: the game's activation indexes by it (NONE crashes) */
		entry->surface_index = NONE; /* no surface: unusable until set */
	}
	maneuver_original[index] = count;
	maneuver_block[index] = block;
	maneuver_own_block[index] = positions;
	maneuver_next[index] = 0;
	if (maneuver_swapped)
	{
		definition->firing_positions.address = block;
		definition->firing_positions.count = count + MANEUVER_SLOTS;
	}
	return TRUE;
}

/* Whether a position may be chosen: an extra slot only once a maneuver has set its ground. */
boolean mendicant_position_usable(long encounter_index, short position, struct firing_position_definition const *entry)
{
	int index = (short)encounter_index;

	if (index < 0 || index >= MENDICANT_MAX_ENCOUNTERS || !maneuver_original[index] || position < maneuver_original[index])
		return TRUE;
	return entry->surface_index != NONE;
}

/* A slot of the encounter set to a point (its ground, surface and cluster from a
ray down), or -1. A slot already at that point is reused; one an actor holds or
is ordered to is never taken. */
int mendicant_maneuver_slot(long encounter_index, real_point3d const *point)
{
	int index = (short)encounter_index;
	struct encounter_definition *definition;
	struct firing_position_definition *positions;
	struct collision_result collision;
	long first, slot;
	int tries, actor_slot;

	if (!maneuver_extend(index))
		return -1;
	definition = maneuver_encounter_definition(index);
	positions = (struct firing_position_definition *)definition->firing_positions.address;
	first = maneuver_original[index];
	for (slot = first; slot < first + MANEUVER_SLOTS; slot++)
	{
		struct firing_position_definition const *p = &positions[slot];

		if (p->surface_index != NONE && (real)fabs(p->position.x - point->x) < 0.5f &&
			(real)fabs(p->position.y - point->y) < 0.5f)
			return (int)slot;
	}
	if (!move_ground_ray(point, 5.0f, &collision))
		return -1;
	for (tries = 0; tries < MANEUVER_SLOTS; tries++)
	{
		struct firing_position_definition *entry;
		boolean ordered = FALSE;

		slot = first + maneuver_next[index];
		maneuver_next[index] = (short)((maneuver_next[index] + 1) % MANEUVER_SLOTS);
		if (mendicant_firing_position_owned(encounter_index, (short)slot))
			continue;
		for (actor_slot = 0; actor_slot < MENDICANT_MAX_ACTORS && !ordered; actor_slot++)
		{
			struct actor_datum *holder = order_actor[actor_slot] != NONE && order_position[actor_slot] == slot ?
				move_actor(order_actor[actor_slot]) : NULL;

			ordered = holder && (short)holder->meta.encounter_index == index;
		}
		if (ordered)
			continue;
		entry = &positions[slot];
		entry->position = collision.point; /* the ground under the point */
		entry->cluster_index = collision.location.cluster_index;
		entry->surface_index = collision.surface_index;
		if (mendicant_drill_file)
			fprintf(mendicant_drill_file, "%ld,borrowed,%d,slot=%ld,x=%.1f,y=%.1f\n",
				game_time_get(), index, slot, point->x, point->y);
		return (int)slot;
	}
	return -1;
}

/* an enemy's own encounter holds the positions it may take */
static struct firing_position_definition *move_member_positions(struct actor_datum *actor, int *count)
{
	struct firing_position_definition *positions = mendicant_encounter_firing_positions(actor->meta.encounter_index, count);

	if (*count > MENDICANT_MAX_POSITIONS)
		*count = MENDICANT_MAX_POSITIONS;
	return positions;
}

/* The best borrowed ground for a band around the enemy (as the drill's position
search scores it), or -1: only spots not already near one of the encounter's
own positions. */
static int maneuver_position(struct actor_datum *actor, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, real *score_out)
{
	real_point3d const *target = geometry->target;
	struct firing_position_definition *own;
	int own_count = 0, best = -1, candidate, position, encounter = (short)actor->meta.encounter_index;
	long limit;
	real angle_middle = (angle_low + angle_high) * 0.5f, range_middle = (range_low + range_high) * 0.5f;
	real best_score = 1e30f;

	if (!maneuver_pool_built)
		maneuver_build_pool();
	own = move_member_positions(actor, &own_count);
	limit = encounter >= 0 && encounter < MENDICANT_MAX_ENCOUNTERS ? maneuver_original[encounter] : 0;
	if (limit)
		own_count = (int)limit;
	for (candidate = 0; candidate < maneuver_pool_count; candidate++)
	{
		real_point3d const *point = &maneuver_pool[candidate];
		real x = point->x - target->x, y = point->y - target->y;
		real range = (real)sqrt(x * x + y * y);
		real angle, travel, score;
		boolean near_own = FALSE;

		if (range < range_low || range > range_high || (real)fabs(point->z - target->z) > FLANK_HEIGHT_MAXIMUM ||
			!mendicant_within_reach(point))
			continue;
		angle = move_signed_angle(geometry->axis_x, geometry->axis_y, x, y);
		angle = side ? angle * side : (real)fabs(angle);
		if (angle < angle_low || angle > angle_high)
			continue;
		travel = move_distance_to(actor, point);
		if (travel > 40.0f)
			continue;
		score = (real)fabs(angle - angle_middle) + 2.0f * (real)fabs(range - range_middle) + 0.3f * travel;
		if (score >= best_score)
			continue;
		for (position = 0; own && position < own_count && !near_own; position++)
		{
			real_point3d const *p = &own[position].position;

			near_own = (real)fabs(p->x - point->x) < 2.0f && (real)fabs(p->y - point->y) < 2.0f;
		}
		if (near_own)
			continue;
		best = candidate;
		best_score = score;
	}
	if (best < 0)
		return -1;
	*score_out = best_score - mendicant_cover_bonus(&maneuver_pool[best]);
	return best;
}

/* The nearest borrowed ground to a point within a radius (for rally points, posts, sectors), or -1. */
int mendicant_maneuver_near(real_point3d const *point, real radius)
{
	int best = -1, candidate;
	real best_distance = radius * radius;

	if (!maneuver_pool_built)
		maneuver_build_pool();
	for (candidate = 0; candidate < maneuver_pool_count; candidate++)
	{
		real_point3d const *p = &maneuver_pool[candidate];
		real dx = p->x - point->x, dy = p->y - point->y, dz = p->z - point->z;
		real distance = dx * dx + dy * dy + 4.0f * dz * dz;

		if (distance < best_distance && mendicant_within_reach(p))
		{
			best = candidate;
			best_distance = distance;
		}
	}
	return best;
}

/* a candidate of mendicant_maneuver_near (maneuver_pool[candidate]) */
real_point3d const *mendicant_maneuver_pool_point(int candidate)
{
	if (candidate < 0 || candidate >= maneuver_pool_count)
		return NULL;
	return &maneuver_pool[candidate];
}

/* ---------- firing positions */

struct firing_position_definition *mendicant_encounter_firing_positions(long encounter_index, int *count)
{
	struct scenario *scenario = global_scenario;
	int index = (short)encounter_index;
	struct encounter_definition *encounter;

	if (!scenario || index < 0 || index >= scenario->ai_encounters.count)
		return NULL;
	encounter = (struct encounter_definition *)scenario->ai_encounters.address + index;
	*count = (int)encounter->firing_positions.count;
	return (struct firing_position_definition *)encounter->firing_positions.address;
}

boolean mendicant_firing_position_owned(long encounter_index, short position)
{
	short index;

	if (!actor_data)
		return FALSE;
	for (index = 0; index < actor_data->count; index++)
	{
		struct actor_datum *actor = move_actor_at(index);

		if (actor && actor->meta.encounter_index == encounter_index &&
			actor->firing_positions.current_position_index == position)
			return TRUE;
	}
	return FALSE;
}

/* A member of a unit fights where the game wants, but never far from its team: a
choice beyond the leash is replaced by the free position nearest to it within
the leash. */
static short move_leashed_position(long actor_index, short chosen, long *owner, boolean *path_valid)
{
	int index = move_slot(actor_index), count = 0, best = -1, position;
	struct actor_datum *actor = move_actor(actor_index);
	struct firing_position_definition *positions;
	real_point3d centre;
	real_point3d const *wanted;
	real radius_squared, dx, dy, best_distance = 1e30f;
	long encounter_index;

	if (index < 0 || index >= MENDICANT_MAX_ACTORS || !actor || actor_leash[index][3] <= 0.0f || chosen < 0)
		return chosen;
	encounter_index = actor->meta.encounter_index;
	positions = mendicant_encounter_firing_positions(encounter_index, &count);
	if (!positions || chosen >= count)
		return chosen;
	centre.x = actor_leash[index][0];
	centre.y = actor_leash[index][1];
	centre.z = actor_leash[index][2];
	radius_squared = actor_leash[index][3] * actor_leash[index][3];
	wanted = &positions[chosen].position;
	dx = wanted->x - centre.x;
	dy = wanted->y - centre.y;
	if (dx * dx + dy * dy <= radius_squared)
		return chosen;
	for (position = 0; position < count; position++)
	{
		real_point3d const *point = &positions[position].position;
		real cx = point->x - centre.x, cy = point->y - centre.y, wx, wy;

		if (cx * cx + cy * cy > radius_squared || (real)fabs(point->z - centre.z) > 4.0f ||
			!mendicant_position_usable(encounter_index, (short)position, &positions[position]) ||
			mendicant_firing_position_owned(encounter_index, (short)position))
			continue;
		wx = point->x - wanted->x;
		wy = point->y - wanted->y;
		if (wx * wx + wy * wy < best_distance)
		{
			best_distance = wx * wx + wy * wy;
			best = position;
		}
	}
	if (best < 0) /* stays put */
		return actor->firing_positions.current_position_index >= 0 ? actor->firing_positions.current_position_index : chosen;
	leash_corrections++;
	*owner = NONE;
	*path_valid = FALSE;
	return (short)best;
}

/* The fight and uncover code ask this each time the actor may move (in place of
actor_active_select_firing_position). An enemy under orders keeps its ordered
position; everyone else gets the game's own choice, leashed. */
short mendicant_select_position(long actor_index,
	struct firing_position_evaluation_context *evaluation_context,
	struct firing_position *best_firing_position, long *current_owner,
	struct path_state *area_path_state, boolean *area_path_state_valid)
{
	int index = move_slot(actor_index);
	short chosen;

	if (index >= 0 && index < MENDICANT_MAX_ACTORS && order_actor[index] == actor_index)
	{
		/* (the hook library left the candidate unset; action_uncover reads it) */
		memset(best_firing_position, 0, sizeof(*best_firing_position));
		*current_owner = NONE;
		*area_path_state_valid = FALSE;
		return order_position[index];
	}
	chosen = actor_active_select_firing_position(actor_index, evaluation_context, best_firing_position,
		current_owner, area_path_state, area_path_state_valid);
	return move_leashed_position(actor_index, chosen, current_owner, area_path_state_valid);
}

/* action_guard_perform applies the guard post it chose with
actor_change_firing_position: an actor under orders (a posture's post) gets its
ordered post instead. */
void mendicant_guard_change(long actor_index, short *firing_position_index,
	long *previous_owner, boolean *cached_path_available)
{
	int slot = move_slot(actor_index);

	if (slot >= 0 && slot < MENDICANT_MAX_ACTORS && order_actor[slot] == actor_index && order_position[slot] >= 0 &&
		order_position[slot] != *firing_position_index)
	{
		/* the game's path led to its own choice: for ours, no owner and no path, so it finds one */
		*firing_position_index = order_position[slot];
		*previous_owner = NONE;
		*cached_path_available = FALSE;
	}
}

/* ---------- cover: the game's own ray test, structure only, from the enemy's eye
to a position at crouching and at standing chest height */

/* a reach the position search keeps to (a Covenant defender's post), set around a step's orders */
boolean mendicant_within_reach(real_point3d const *point)
{
	real dx, dy;

	if (position_reach <= 0.0f)
		return TRUE;
	dx = point->x - position_reach_centre[0];
	dy = point->y - position_reach_centre[1];
	return dx * dx + dy * dy <= position_reach * position_reach;
}

/* -1: no structure loaded, 0: clear, 1: blocked */
int mendicant_line_blocked(real_point3d const *from, real_point3d const *to)
{
	real_vector3d vector;
	struct collision_result collision;

	if (!global_structure_bsp || global_current_collision_user_depth < 1)
		return -1;
	vector.i = to->x - from->x;
	vector.j = to->y - from->y;
	vector.k = to->z - from->z;
	return collision_test_vector(MOVE_COVER_COLLISION_FLAGS, from, &vector, NONE, &collision) != FALSE;
}

/* 2: cover to fight from (blocked crouched, clear standing), 1: concealed (blocked both), 0: exposed */
int mendicant_cover_score(real_point3d const *position, real_point3d const *threat)
{
	real_point3d crouched, standing;

	crouched = *position;
	crouched.z += 0.6f;
	standing = *position;
	standing.z += 1.2f;
	if (mendicant_line_blocked(threat, &crouched) <= 0)
		return 0;
	return mendicant_line_blocked(threat, &standing) == 0 ? 2 : 1;
}

/* How much a position's cover is worth to the search now: a bonus subtracted from its score. */
real mendicant_cover_bonus(real_point3d const *position)
{
	int cover;

	if (cover_wanted == MENDICANT_COVER_NONE)
		return 0.0f;
	cover = mendicant_cover_score(position, &cover_threat);
	if (cover_wanted == MENDICANT_COVER_FIGHTING)
		return cover == 2 ? 25.0f : cover == 1 ? 8.0f : 0.0f;
	return cover == 1 ? 25.0f : cover == 2 ? 15.0f : 0.0f;
}

/* what the next position searches prefer; a NULL threat keeps the last one */
void mendicant_cover_want(int cover, real_point3d const *threat)
{
	cover_wanted = cover;
	if (threat)
		cover_threat = *threat;
}

/* ---------- the drill's positions */

/* The free position that best fits a place relative to the target: an angle from
the unit's front (side 0: either side) and a range. Wider limits are tried once
if nothing fits. */
int mendicant_drill_position(struct actor_datum *actor, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used)
{
	int count = 0, attempt, slot, position;
	long encounter_index = actor->meta.encounter_index;
	struct firing_position_definition *positions = move_member_positions(actor, &count);
	real_point3d const *target = geometry->target;

	if (!positions)
		return -1;
	for (attempt = 0; attempt < 2; attempt++)
	{
		/* the best few places by the drill's geometry, re-ranked by cover from the enemy;
		then one of the three best, at random, so a drill never runs the same way twice */
		int best[COVER_CANDIDATES], borrowed;
		real best_score[COVER_CANDIDATES], borrowed_score, own_best;
		real angle_middle = (angle_low + angle_high) * 0.5f, range_middle = (range_low + range_high) * 0.5f;

		for (slot = 0; slot < COVER_CANDIDATES; slot++)
		{
			best[slot] = -1;
			best_score[slot] = 1e30f;
		}
		for (position = 0; position < count; position++)
		{
			real_point3d const *point = &positions[position].position;
			real x = point->x - target->x, y = point->y - target->y;
			real range = (real)sqrt(x * x + y * y);
			real angle = move_signed_angle(geometry->axis_x, geometry->axis_y, x, y);
			real travel, score;

			angle = side ? angle * side : (real)fabs(angle);
			travel = move_distance_to(actor, point);
			if (used[position] == encounter_index ||
				!mendicant_position_usable(encounter_index, (short)position, &positions[position]) ||
				!mendicant_within_reach(point) || angle < angle_low || angle > angle_high ||
				range < range_low || range > range_high || travel > 40.0f ||
				(real)fabs(point->z - target->z) > FLANK_HEIGHT_MAXIMUM)
				continue;
			score = (real)fabs(angle - angle_middle) + 2.0f * (real)fabs(range - range_middle) + 0.3f * travel;
			if (score < best_score[COVER_CANDIDATES - 1] && !mendicant_firing_position_owned(encounter_index, (short)position))
			{
				slot = COVER_CANDIDATES - 1;
				while (slot > 0 && best_score[slot - 1] > score)
				{
					best[slot] = best[slot - 1];
					best_score[slot] = best_score[slot - 1];
					slot--;
				}
				best[slot] = position;
				best_score[slot] = score;
			}
		}
		/* ground beyond the encounter's own area, when it fits the band better */
		borrowed_score = 1e30f;
		borrowed = maneuver_position(actor, geometry, side, angle_low, angle_high, range_low, range_high,
			&borrowed_score);
		own_best = best[0] >= 0 ? best_score[0] - mendicant_cover_bonus(&positions[best[0]].position) : 1e30f;
		if (borrowed >= 0 && borrowed_score < own_best)
		{
			int borrowed_slot = mendicant_maneuver_slot(encounter_index, &maneuver_pool[borrowed]);

			if (borrowed_slot >= 0 && used[borrowed_slot] != encounter_index)
				return borrowed_slot;
		}
		if (best[0] >= 0)
		{
			int found = 0, a, b;

			for (slot = 0; slot < COVER_CANDIDATES && best[slot] >= 0; slot++)
			{
				best_score[slot] -= mendicant_cover_bonus(&positions[best[slot]].position);
				found++;
			}
			for (a = 1; a < found; a++)
			{
				for (b = a; b > 0 && best_score[b - 1] > best_score[b]; b--)
				{
					real score = best_score[b];
					int swapped = best[b];

					best_score[b] = best_score[b - 1];
					best[b] = best[b - 1];
					best_score[b - 1] = score;
					best[b - 1] = swapped;
				}
			}
			return best[mendicant_drill_rand() % (unsigned long)(found < 3 ? found : 3)];
		}
		angle_low -= 30.0f;
		angle_high += 30.0f;
		range_low = range_low > 5.0f ? range_low - 5.0f : 0.0f;
		range_high += 5.0f;
	}
	return -1;
}

/* Holds its position: it keeps firing from where it is. */
void mendicant_drill_hold(struct mendicant_member *member, long *used)
{
	short current = member->actor->firing_positions.current_position_index;

	if (current >= 0 && current < MENDICANT_MAX_POSITIONS)
	{
		used[current] = member->actor->meta.encounter_index;
		mendicant_order(member->actor_index, current);
	}
}

/* Gives a member its position. A fighting actor is moved there now; a guarding
one has it as its order, and the guard hook (mendicant_guard_change) applies it
when it next picks its post. */
boolean mendicant_assign_position(struct mendicant_member *member, short position)
{
	short action = member->actor->state.action;

	if (action != _actor_action_fight && action != _actor_action_uncover)
	{
		mendicant_order(member->actor_index, position);
		return TRUE;
	}
	if (actor_change_firing_position(member->actor_index, position, NULL, NONE, NULL, FALSE) == position)
	{
		mendicant_order(member->actor_index, position);
		return TRUE;
	}
	return FALSE;
}

/* No position fits (none of its encounter's, no known ground near): free movement
to the point itself, over the whole level, for any actor of either side. It
lasts until the member's next position order. */
boolean mendicant_go_fallback(struct mendicant_member *member, real_point3d const *point, real tolerance)
{
	int slot;

	if (!mendicant_actor_can_drill(member->actor) || move_distance_to(member->actor, point) > 80.0f ||
		actor_go_to(member->actor_index, point, tolerance, 1) != 1)
		return FALSE;
	slot = move_slot(member->actor_index);
	if (slot >= 0 && slot < MENDICANT_MAX_ACTORS)
		go_by_drill[slot] = 1;
	go_fallbacks++;
	return TRUE;
}

boolean mendicant_drill_move(struct mendicant_member *member, struct mendicant_geometry const *geometry, int side,
	real angle_low, real angle_high, real range_low, real range_high, long *used)
{
	int position = mendicant_drill_position(member->actor, geometry, side, angle_low, angle_high,
		range_low, range_high, used);

	if (position < 0 && geometry->target)
	{
		/* nowhere of its own in the band: the band's middle itself, by free movement */
		real_point3d const *target = geometry->target;
		real length = (real)sqrt(geometry->axis_x * geometry->axis_x + geometry->axis_y * geometry->axis_y) + 0.001f;
		real ux = geometry->axis_x / length, uy = geometry->axis_y / length;
		real sign = (real)side, angle, range;
		real_point3d point;

		if (!side)
		{
			real x = member->actor->input.pathfinding_point.x - target->x;
			real y = member->actor->input.pathfinding_point.y - target->y;

			sign = move_signed_angle(ux, uy, x, y) < 0.0f ? -1.0f : 1.0f;
		}
		angle = sign * (angle_low + angle_high) * 0.5f * 0.0174533f;
		range = (range_low + range_high) * 0.5f;
		point.x = target->x + (ux * (real)cos(angle) - uy * (real)sin(angle)) * range;
		point.y = target->y + (ux * (real)sin(angle) + uy * (real)cos(angle)) * range;
		point.z = member->actor->input.pathfinding_point.z;
		if (mendicant_go_fallback(member, &point, MOVE_MAXIMUM((range_high - range_low) * 0.5f, 2.0f)))
			return TRUE;
	}
	if (position < 0)
	{
		mendicant_drill_hold(member, used);
		return FALSE;
	}
	used[position] = member->actor->meta.encounter_index;
	return mendicant_assign_position(member, (short)position);
}

/* A position order ends a free-movement order from a drill or from following the
Chief, so the drill has the actor again (the start of the hook library's order()). */
void mendicant_go_release_drill(long actor_index)
{
	int slot = move_slot(actor_index);

	if (slot >= 0 && slot < MENDICANT_MAX_ACTORS && (go_by_drill[slot] || follow_going[slot]))
	{
		actor_go_cancel(actor_index);
		go_by_drill[slot] = follow_going[slot] = 0;
	}
}

/* ---------- the Marines go where the Chief goes

Every Marine who has been with him (within 30 wu) keeps up with him from then
on, over any ground: out of a fight he stays within 10 wu, in one within 28 (he
fights, but never lets the Chief go on alone). A Marine further than that from
him moves toward him each second; one who has never met him (Marines waiting
further on in the level) stays where the level put him until the Chief arrives.
"The Chief" is absolute player 0's unit. */

/* The next hop: of the known ground 4-16 wu from him, the spot that brings him
closest to the target (at least 2 wu closer), so there is always a way on while
any closer ground is near. */
static int follow_step_ground(real_point3d const *at, real_point3d const *target, real distance)
{
	int best = -1, candidate;
	real best_remaining = distance - 2.0f;

	if (!maneuver_pool_built)
		maneuver_build_pool();
	for (candidate = 0; candidate < maneuver_pool_count; candidate++)
	{
		real_point3d const *p = &maneuver_pool[candidate];
		real dx = p->x - at->x, dy = p->y - at->y, hop = dx * dx + dy * dy, tx, ty, remaining;

		if (hop < 4.0f * 4.0f || hop > 16.0f * 16.0f || (real)fabs(p->z - at->z) > 4.0f)
			continue;
		tx = target->x - p->x;
		ty = target->y - p->y;
		remaining = (real)sqrt(tx * tx + ty * ty);
		if (remaining < best_remaining)
		{
			best_remaining = remaining;
			best = candidate;
		}
	}
	return best;
}

static boolean follow_hop(long actor_index, struct actor_datum *actor, real_point3d const *at,
	real_point3d const *target, real distance)
{
	int borrowed = follow_step_ground(at, target, distance), slot, count = 0;
	long encounter;
	struct firing_position_definition *positions;
	short action;

	if (borrowed < 0)
		return FALSE;
	encounter = actor->meta.encounter_index;
	if (encounter == NONE)
	{
		struct collision_result collision;

		if (!move_ground_ray(&maneuver_pool[borrowed], 5.0f, &collision))
			return FALSE;
		actor_move_to_point(actor_index, &collision.point, collision.surface_index, NONE);
		return TRUE;
	}
	slot = mendicant_maneuver_slot(encounter, &maneuver_pool[borrowed]);
	positions = mendicant_encounter_firing_positions(encounter, &count);
	if (slot < 0 || !positions || slot >= count)
		return FALSE;
	mendicant_order(actor_index, (short)slot);
	action = actor->state.action;
	if (action == _actor_action_fight || action == _actor_action_uncover)
		actor_change_firing_position(actor_index, (short)slot, NULL, NONE, NULL, FALSE);
	else
		actor_move_to_point(actor_index, &positions[slot].position, positions[slot].surface_index, NONE);
	return TRUE;
}

void mendicant_follow_chief(long time)
{
	long chief_unit = mendicant_player_unit();
	struct object_datum *chief = move_object(chief_unit);
	real_point3d const *at;
	short index;

	if (!chief || chief->object.body_vitality <= 0.0f || !actor_data || !actor_data->valid)
		return;
	at = &chief->object.position;
	for (index = 0; index < actor_data->count && index < MENDICANT_MAX_ACTORS; index++)
	{
		struct actor_datum *actor = move_actor_at(index);
		struct object_datum *unit;
		real_point3d const *from;
		real_point3d target;
		real dx, dy, distance;
		boolean fighting;
		long actor_index;

		if (!actor || mendicant_side_of(actor->meta.team_index) != TEAM_HUMAN ||
			mendicant_team_mode(actor->meta.team_index) == MODE_GAME)
			continue;
		unit = move_object(actor->meta.unit_index);
		if (!unit || actor->meta.unit_index == chief_unit || unit->object.body_vitality <= 0.0f ||
			actor->input.vehicle_index != NONE)
			continue; /* the Chief himself, the dead, and riders (the vehicles have their own rules) */
		if (follow_identifier[index] != actor->identifier)
		{
			follow_identifier[index] = actor->identifier;
			follow_met[index] = 0;
			follow_time[index] = 0;
			follow_going[index] = 0;
		}
		actor_index = move_actor_index(actor->identifier, index);
		from = &unit->object.position;
		dx = at->x - from->x;
		dy = at->y - from->y;
		distance = (real)sqrt(dx * dx + dy * dy);
		if (distance < FOLLOW_MET)
			follow_met[index] = 1;
		fighting = actor->state.action == _actor_action_fight;
		if (follow_going[index] && distance <= FOLLOW_CALM * 0.7f)
		{
			/* caught up: back to the unit's own positions and drills */
			actor_go_cancel(actor_index);
			follow_going[index] = 0;
		}
		if (!follow_met[index] || distance <= (fighting ? FOLLOW_FIGHTING : FOLLOW_CALM) ||
			time - follow_time[index] < 30)
			continue;
		follow_time[index] = time;
		/* to his side, 4 wu short of him on this Marine's side */
		target.x = at->x - dx / distance * 4.0f;
		target.y = at->y - dy / distance * 4.0f;
		target.z = at->z;
		{
			/* free movement: a route over the level to him, through any fight on the way
			(urgency 1: he fires as he goes, but still dodges); the order is renewed as
			the Chief moves on */
			int status = actor_go_status(actor_index);
			real gx = follow_goal[index].x - target.x, gy = follow_goal[index].y - target.y;

			if (status != GO_STATUS_MOVING || gx * gx + gy * gy > 16.0f)
			{
				if (actor_go_to(actor_index, &target, 3.0f, 1) == 1)
				{
					follow_goal[index] = target;
					follow_going[index] = 1;
					marines_hops++;
				}
				else if (follow_hop(actor_index, actor, from, &target, distance))
				{
					marines_hops++;
				}
				else
				{
					marines_stuck++;
				}
			}
		}
		if (mendicant_drill_file && (time % 150) < 30)
			fprintf(mendicant_drill_file, "%ld,follow,%d,distance=%.0f,fighting=%d,hops=%ld,stuck=%ld\n", time,
				(int)index, distance, (int)fighting, marines_hops, marines_stuck);
	}
}

/* a vehicle took over (vehicles.inc): no walking order holds him off a seat */
void mendicant_follow_cancel(long actor_index)
{
	int slot = move_slot(actor_index);

	actor_go_cancel(actor_index);
	if (slot >= 0 && slot < MENDICANT_MAX_ACTORS)
		follow_going[slot] = 0;
}

/* ---------- the move test (ai.mendicant.move_test_log): one fighting Covenant at a
time is ordered to a free position 6-15 wu away and watched for 30 seconds */

static void move_test_watch(long time)
{
	struct actor_datum *actor = move_actor(move_test_actor);
	FILE *file = mendicant_move_test_file;

	if (!actor)
	{
		fprintf(file, "%ld,gone,%d\n", time, (int)move_slot(move_test_actor));
		fflush(file);
		order_actor[move_slot(move_test_actor)] = NONE;
		move_test_actor = NONE;
		return;
	}
	if (time - move_test_report_time < MOVE_TEST_REPORT_TICKS)
		return;
	move_test_report_time = time;
	fprintf(file, "%ld,watch,%d,distance=%.2f,position=%d,action=%d\n",
		time, (int)move_slot(move_test_actor), move_distance_to(actor, &move_test_point),
		(int)actor->firing_positions.current_position_index, (int)actor->state.action);
	fflush(file);
	if (time - move_test_start_time >= MOVE_TEST_TICKS ||
		move_distance_to(actor, &move_test_point) < MOVE_ARRIVED_DISTANCE)
	{
		fprintf(file, "%ld,end,%d,distance=%.2f,ticks=%ld\n", time, (int)move_slot(move_test_actor),
			move_distance_to(actor, &move_test_point), time - move_test_start_time);
		order_actor[move_slot(move_test_actor)] = NONE;
		move_test_actor = NONE;
	}
}

static void move_test_start(long actor_index, struct actor_datum *actor, long time)
{
	long encounter_index = actor->meta.encounter_index;
	short current = actor->firing_positions.current_position_index, result;
	int count = 0, best = -1, position;
	real best_distance = 0.0f;
	struct firing_position_definition *positions = mendicant_encounter_firing_positions(encounter_index, &count);
	FILE *file = mendicant_move_test_file;

	if (!positions)
		return;
	for (position = 0; position < count; position++)
	{
		real distance = move_distance_to(actor, &positions[position].position);

		if (position != current && distance >= MOVE_MINIMUM_DISTANCE &&
			distance <= MOVE_MAXIMUM_DISTANCE && distance > best_distance &&
			!mendicant_firing_position_owned(encounter_index, (short)position))
		{
			best = position;
			best_distance = distance;
		}
	}
	if (best < 0)
		return;
	move_test_actor = actor_index;
	/* (set and cleared directly, as the hook library did: no free-movement order is cancelled) */
	order_actor[move_slot(actor_index)] = actor_index;
	order_position[move_slot(actor_index)] = (short)best;
	move_test_point = positions[best].position;
	move_test_start_time = move_test_report_time = time;
	result = actor_change_firing_position(actor_index, (short)best, NULL, NONE, NULL, FALSE);
	if (result == best)
	{
		actor->state.action_data.fight.firing_position_timer = MOVE_TEST_HOLD_TICKS;
	}
	else
	{
		order_actor[move_slot(actor_index)] = NONE;
		move_test_actor = NONE;
	}
	fprintf(file, "%ld,order,%d,from=%d,to=%d,result=%d,distance=%.2f,of=%d\n",
		time, (int)move_slot(actor_index), (int)current, best, (int)result, best_distance, count);
	fflush(file);
}

/* after each decision, when the move test log is open */
void mendicant_move_test(long actor_index)
{
	long time;
	struct actor_datum *actor;

	if (!mendicant_move_test_file)
		return;
	time = game_time_get();
	if (move_test_actor != NONE)
	{
		move_test_watch(time);
		return;
	}
	actor = move_actor(actor_index);
	if (actor && actor->meta.team_index == TEAM_COVENANT && actor->meta.active && !actor->meta.swarm &&
		actor->state.action == _actor_action_fight && actor->meta.encounter_index != NONE &&
		actor->input.vehicle_index == NONE)
		move_test_start(actor_index, actor, time);
}

/* ---------- the episode's report, the map and the tick */

/* episode_end: the movement counters, in the drill log (the bridge has no field for them) */
void mendicant_movement_report(struct model_bridge *bridge)
{
	(void)bridge;
	if (mendicant_drill_file)
		fprintf(mendicant_drill_file, "%ld,movement,drill_go=%ld,follow_moves=%ld,follow_stuck=%ld\n",
			game_time_get(), go_fallbacks, marines_hops, marines_stuck);
}

/* Each new map: no go orders, no graph, no follow state, no maneuver slots.
The counters (go_fallbacks, the hops) run on, as the hook library's did. */
void mendicant_movement_reset(void)
{
	int i;

	for (i = 0; i < GO_MAX_ORDERS; i++)
	{
		memset(&go_orders[i], 0, sizeof(go_orders[i]));
		go_orders[i].actor = NONE;
		go_orders[i].encounter = NONE;
	}
	go_inside = FALSE;
	memset(go_forced_by_us, 0, sizeof(go_forced_by_us));
	go_node_count = 0;
	go_graph_built = FALSE;
	go_graph_scenario = NULL;
	go_graph_bsp = NONE;
	go_planner.order = -1;
	if (maneuver_scenario)
		maneuver_forget();
	maneuver_scenario = NULL;
	maneuver_bsp = NONE;
	maneuver_swapped = FALSE;
	maneuver_pool_count = 0;
	maneuver_pool_built = FALSE;
	cover_wanted = MENDICANT_COVER_NONE;
	memset(&cover_threat, 0, sizeof(cover_threat));
	memset(go_by_drill, 0, sizeof(go_by_drill));
	memset(follow_going, 0, sizeof(follow_going));
	memset(follow_identifier, 0, sizeof(follow_identifier));
	memset(follow_met, 0, sizeof(follow_met));
	memset(follow_time, 0, sizeof(follow_time));
	memset(follow_goal, 0, sizeof(follow_goal));
	move_test_actor = NONE;
	move_reset_done = TRUE;
}

/* The map goes (the start of ai_dispose_from_old_map): the level's own firing
position lists go back before the tags do, the copies are freed, the go orders
end and every force_active this module set is cleared. */
void mendicant_movement_dispose(void)
{
	int index;

	if (global_scenario && global_scenario == maneuver_scenario)
	{
		for (index = 0; index < MENDICANT_MAX_ENCOUNTERS; index++)
		{
			struct encounter_definition *definition;

			if (!maneuver_original[index] || !(definition = maneuver_encounter_definition(index)))
				continue;
			definition->firing_positions.address = maneuver_own_block[index];
			definition->firing_positions.count = maneuver_original[index];
		}
	}
	maneuver_swapped = FALSE;
	maneuver_forget();
	maneuver_scenario = NULL;
	maneuver_bsp = NONE;
	maneuver_pool_built = FALSE;
	for (index = 0; index < GO_MAX_ORDERS; index++)
	{
		go_orders[index].actor = NONE;
		go_orders[index].status = GO_STATUS_NONE;
	}
	go_release_encounters();
	go_planner.order = -1;
	go_graph_scenario = NULL;
	go_graph_bsp = NONE;
	go_graph_built = FALSE;
	go_node_count = 0;
}

/* game_tick runs with the longer position lists: maneuver_check_map, the swap
in, then go_update (the hook library's wrapper, then game_tick's detoured entry) */
void mendicant_movement_tick_begin(void)
{
	if (!move_reset_done)
		mendicant_movement_reset();
	mendicant_maneuver_check_map();
	maneuver_swap(TRUE);
	go_update();
}

void mendicant_movement_tick_end(void)
{
	maneuver_swap(FALSE);
}
