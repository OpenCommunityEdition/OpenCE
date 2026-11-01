/* Source: thelinkin3000/halo-ce-universal, commit
 * d92cc93385edf03d0a4a21cfe4c58dd578a3e786, port/linux/game/bots_routes.c.
 * Derived from BirchWoodGod/halo-ce-vita (GPL-3.0-only).
 * Used only by GulchCE's optional enhanced bots.
 */
/* Optional enhanced bots' routes. Direct uses the original pathfinder's
goal. Lanes and Mixed distribute attackers over up to two side routes.
The graph joins validated walkable surfaces, excluding blocked machine
surfaces. Weighted shortest-path searches prefer opposite sides of the
direct route; a lane too close to that route or too long is rejected.

Preparation advances in bounded steps each tick. Only two waypoints per
lane are retained; the graph is released after vantage preparation. A bot
abandons an unreachable lane for the rest of its life. Vehicle paths stay
direct. HALO_BOT_ROUTES_DUMP writes diagnostic geometry in the data folder. */

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collisions.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"

#include "bots.h"
#include "bots_tactics.h"
#include "bots_objectives.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

/* a pathfinding surface's flag: walkable (path.c's) */
#define BOTS_ROUTES_WALKABLE_BIT 6
/* the most walkable surfaces a map's graph takes (Blood Gulch has 2807);
more, and the map has no lanes nor vantage points */
#define BOTS_GRAPH_MAXIMUM_NODES 32768
/* Bound the surface-to-node lookup too, including unwalkable surfaces. */
#define BOTS_GRAPH_MAXIMUM_SURFACES 262144
/* the corners of a surface looked at (its ring of edges) */
#define BOTS_ROUTES_CORNERS 16
/* the work of a tick while the lanes are made: surfaces looked at, the
searches' surfaces taken, the nodes placed across the direct route */
#define BOTS_ROUTES_SURFACES_A_TICK 300
#define BOTS_ROUTES_POPS_A_TICK 80
#define BOTS_ROUTES_NODES_A_TICK 200
/* the machines (doors, lifts) whose ground the graph leaves out */
#define BOTS_ROUTES_MACHINES 32
/* a goal nearer than this is gone to directly */
#define BOTS_LANE_LEAST_DISTANCE 25.0f
/* how near a waypoint a bot has come to it, and how long it has to */
#define BOTS_LANE_WAYPOINT_REACHED 3.0f
#define BOTS_LANE_WAYPOINT_SECONDS 40

enum
{
	_routes_phase_idle = 0,
	_routes_phase_machines,
	_routes_phase_nodes,
	_routes_phase_links,
	_routes_phase_bases,
	_routes_phase_direct,
	_routes_phase_place,
	_routes_phase_lanes,
	_routes_phase_done,
	_routes_phase_none,
};

/* ---------- structures */

struct bots_lane_waypoint
{
	real_point3d point;
	long surface_index;
	/* how far along the direct route (0 to 1) */
	real along;
};

struct bots_lane
{
	boolean valid;
	long waypoint_count;
	struct bots_lane_waypoint waypoints[BOTS_LANE_WAYPOINTS];
	real length;
	real offset;
};

/* ---------- globals */

struct bots_graph bots_graph;
struct bots_routes_map bots_routes_map;

static struct
{
	short phase;
	short mode;
	struct structure_bsp const *structure;
	long surface_count;
	long *node_of_surface;
	long cursor;
	long link_capacity;
	long link_write;
	/* the search (Dijkstra's, an indexed binary heap): its costs, the way
	back, the heap and each node's place in it (NONE: not in it, -2: done) */
	real *cost;
	long *previous;
	long *heap;
	long *heap_position;
	long heap_count;
	long origin;
	long target;
	/* the bases' nodes, and which bases they are (bots_routes_find_bases'
	sets: the stands, the teams' starting locations, the two furthest apart) */
	long base_nodes[2];
	long base_set;
	/* the nodes' costs from the first base made (what is reached: the
	vantage points' spots, lanes or none) */
	boolean costs_made;
	/* the machines' spheres, whose ground the graph leaves out */
	long machine_count;
	real_point3d machine_centers[BOTS_ROUTES_MACHINES];
	real machine_radii[BOTS_ROUTES_MACHINES];
	long machine_surfaces;
	/* the side lane being searched (0 or 1), its side (-1 or 1) */
	long lane_index;
	real lane_side;
	real penalty_radius;
	/* the direct route's polyline (bots_routes_map.route_points): its parts'
	lengths, and the length to each point */
	real segment_lengths[BOTS_ROUTE_POINTS];
	real point_lengths[BOTS_ROUTE_POINTS];
	struct bots_lane lanes[BOTS_SIDE_LANES];
} bots_routes;

/* ---------- the setting */

static short bots_routes_setting(void)
{
	char const *value = getenv("HALO_BOT_ROUTES");

	if (!value || !value[0])
		value = config_string("bots.routes");
	if (value && !strcmp(value, "lanes"))
		return _bots_routes_lanes;
	if (value && !strcmp(value, "mixed"))
		return _bots_routes_mixed;
	return _bots_routes_direct;
}

/* the setting, read once a second (the panel changes it at any time) */
short bots_routes_mode(void)
{
	return bots_routes.mode;
}

void bots_routes_read_setting(void)
{
	bots_routes.mode = bots_routes_setting();
}

/* ---------- small helpers */

static real bots_routes_distance2d(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y;

	return (real)sqrt(dx * dx + dy * dy);
}

static real bots_routes_distance3d(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

	return (real)sqrt(dx * dx + dy * dy + dz * dz);
}

long bots_routes_ground(real_point3d const *point, real_point3d *ground)
{
	struct collision_result collision;
	real_point3d start = *point;
	real_vector3d down = { 0.0f, 0.0f, -6.0f };

	start.z += 0.4f;
	if (collision_test_vector(FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit),
		&start, &down, NONE, &collision) && collision.type == _collision_result_structure)
	{
		if (ground)
			*ground = collision.point;
		return collision.surface_index;
	}
	return NONE;
}

/* the structure's collision bsp, NULL for none */
static struct collision_bsp const *bots_routes_bsp(void)
{
	struct structure_bsp const *structure = bots_routes.structure;

	if (!structure || structure->collision_bsp.count <= 0)
		return NULL;
	return TAG_BLOCK_GET_ELEMENT(&structure->collision_bsp, 0, struct collision_bsp);
}

/* whether a surface is walkable (the structure's pathfinding flags) */
static boolean bots_routes_walkable(long surface_index)
{
	byte const *flags = (byte const *)bots_routes.structure->pathfinding_surfaces.address;

	return surface_index >= 0 && surface_index < bots_routes.surface_count &&
		TEST_FLAG(flags[surface_index], BOTS_ROUTES_WALKABLE_BIT);
}

/* the surface's corners (at most BOTS_ROUTES_CORNERS) from its ring of edges,
and the surface across each edge (NONE for none); 0 for one not the bsp's */
static long bots_routes_corners(struct collision_bsp const *bsp, long surface_index, real_point3d *corners,
	long *neighbours)
{
	struct collision_surface const *surface;
	long edge_index, count = 0;

	if (surface_index < 0 || surface_index >= bsp->surfaces.count)
		return 0;
	surface = TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface);
	edge_index = surface->first_edge_index;
	do
	{
		struct collision_edge const *edge;
		boolean right;

		if (edge_index < 0 || edge_index >= bsp->edges.count)
			return 0;
		edge = TAG_BLOCK_GET_ELEMENT(&bsp->edges, edge_index, struct collision_edge);
		right = edge->surface_indices[1] == surface_index;
		if (!VALID_INDEX(edge->vertex_indices[right], bsp->vertices.count))
			return 0;
		corners[count] = TAG_BLOCK_GET_ELEMENT(&bsp->vertices, edge->vertex_indices[right], struct collision_vertex)->point;
		if (neighbours)
			neighbours[count] = edge->surface_indices[!right];
		count++;
		edge_index = edge->edge_indices[right];
	} while (edge_index != surface->first_edge_index && count < BOTS_ROUTES_CORNERS);
	return count;
}

/* ---------- the walkable graph */

static void bots_routes_free(void)
{
	/* (the game's free, debug_memory.c's, takes no NULL) */
	if (bots_routes.node_of_surface)
		free(bots_routes.node_of_surface);
	if (bots_graph.nodes)
		free(bots_graph.nodes);
	if (bots_graph.link_first)
		free(bots_graph.link_first);
	if (bots_graph.links)
		free(bots_graph.links);
	if (bots_routes.cost)
		free(bots_routes.cost);
	if (bots_routes.previous)
		free(bots_routes.previous);
	if (bots_routes.heap)
		free(bots_routes.heap);
	if (bots_routes.heap_position)
		free(bots_routes.heap_position);
	bots_routes.node_of_surface = NULL;
	bots_graph.nodes = NULL;
	bots_graph.link_first = NULL;
	bots_graph.links = NULL;
	bots_routes.cost = NULL;
	bots_routes.previous = NULL;
	bots_routes.heap = NULL;
	bots_routes.heap_position = NULL;
	bots_graph.node_count = 0;
	bots_graph.link_count = 0;
	bots_routes.link_capacity = 0;
}

/* the graph's memory let go (its answers, the lanes and the direct route,
kept) */
void bots_routes_release_graph(void)
{
	bots_routes_free();
}

void bots_routes_reset(void)
{
	bots_routes_free();
	csmemset(&bots_routes_map, 0, sizeof(bots_routes_map));
	csmemset(bots_routes.lanes, 0, sizeof(bots_routes.lanes));
	bots_routes.phase = _routes_phase_idle;
	bots_routes.costs_made = FALSE;
	bots_routes.structure = NULL;
	bots_routes.cursor = 0;
}

/* the graph begun: the walkable surfaces counted, the memory for them; FALSE
for a map without (or with too many) */
static boolean bots_routes_begin(void)
{
	struct structure_bsp const *structure = global_structure_bsp_get();
	struct collision_bsp const *bsp;
	byte const *flags;
	long index, count = 0;

	if (!structure || structure->collision_bsp.count <= 0 || structure->pathfinding_surfaces.count <= 0)
	{
		platform_log("bots: routes: the level has no walkable surfaces: no lanes, no vantage points");
		return FALSE;
	}
	bots_routes.structure = structure;
	bsp = bots_routes_bsp();
	bots_routes.surface_count = MIN((long)bsp->surfaces.count, (long)structure->pathfinding_surfaces.count);
	if (bots_routes.surface_count > BOTS_GRAPH_MAXIMUM_SURFACES)
	{
		platform_log("bots: routes: %ld surfaces exceed the %d surface budget: direct paths only",
			bots_routes.surface_count, BOTS_GRAPH_MAXIMUM_SURFACES);
		return FALSE;
	}
	flags = (byte const *)structure->pathfinding_surfaces.address;
	for (index = 0; index < bots_routes.surface_count; index++)
	{
		if (TEST_FLAG(flags[index], BOTS_ROUTES_WALKABLE_BIT))
			count++;
	}
	if (count < 8 || count > BOTS_GRAPH_MAXIMUM_NODES)
	{
		platform_log("bots: routes: %ld walkable surfaces: no lanes, no vantage points", count);
		return FALSE;
	}
	bots_routes.node_of_surface = (long *)malloc(sizeof(long) * (size_t)bots_routes.surface_count);
	bots_graph.nodes = (struct bots_graph_node *)malloc(sizeof(struct bots_graph_node) * (size_t)count);
	bots_graph.link_first = (long *)malloc(sizeof(long) * (size_t)(count + 1));
	bots_routes.link_capacity = count * 6;
	bots_graph.links = (long *)malloc(sizeof(long) * (size_t)bots_routes.link_capacity);
	bots_routes.cost = (real *)malloc(sizeof(real) * (size_t)count);
	bots_routes.previous = (long *)malloc(sizeof(long) * (size_t)count);
	bots_routes.heap = (long *)malloc(sizeof(long) * (size_t)count);
	bots_routes.heap_position = (long *)malloc(sizeof(long) * (size_t)count);
	if (!bots_routes.node_of_surface || !bots_graph.nodes || !bots_graph.link_first || !bots_graph.links ||
		!bots_routes.cost || !bots_routes.previous || !bots_routes.heap || !bots_routes.heap_position)
	{
		platform_log("bots: routes: no memory for %ld walkable surfaces", count);
		bots_routes_free();
		return FALSE;
	}
	for (index = 0; index < bots_routes.surface_count; index++)
		bots_routes.node_of_surface[index] = NONE;
	bots_routes.base_set = 0;
	bots_routes.machine_surfaces = 0;
	bots_graph.node_count = 0;
	bots_graph.link_count = 0;
	bots_routes.cursor = 0;
	return TRUE;
}

/* the machines, and the scenery that is a door, whose ground the graph
leaves out: the way under Sidewinder's blast door between the bases, scenery
that never opens and that bots.c's searches learn to be in the way, would be
the direct route */
static void bots_routes_find_machines(void)
{
	struct object_iterator iterator;

	bots_routes.machine_count = 0;
	object_iterator_new(&iterator, _object_mask_machine | _object_mask_scenery, 0);
	while (object_iterator_next(&iterator) && bots_routes.machine_count < BOTS_ROUTES_MACHINES)
	{
		struct object_datum *object = object_get(iterator.index);
		char const *name = tag_get_name(object->definition_index);

		if (!object_try_and_get_and_verify_type(iterator.index, _object_mask_machine) && (!name || !strstr(name, "door")))
			continue;
		bots_routes.machine_centers[bots_routes.machine_count] = object->object.bounding_sphere_center;
		bots_routes.machine_radii[bots_routes.machine_count] = object->object.bounding_sphere_radius;
		bots_routes.machine_count++;
	}
}

/* the nodes: a step of the walkable surfaces' middles, areas and slopes; TRUE
when all are made */
static boolean bots_routes_nodes_step(void)
{
	struct collision_bsp const *bsp = bots_routes_bsp();
	long last = MIN(bots_routes.cursor + BOTS_ROUTES_SURFACES_A_TICK, bots_routes.surface_count);

	for (; bots_routes.cursor < last; bots_routes.cursor++)
	{
		long surface_index = bots_routes.cursor;
		real_point3d corners[BOTS_ROUTES_CORNERS];
		long neighbours[BOTS_ROUTES_CORNERS];
		struct bots_graph_node *node;
		real_vector3d normal = { 0.0f, 0.0f, 0.0f };
		long count, corner;
		real reach;

		if (!bots_routes_walkable(surface_index))
			continue;
		count = bots_routes_corners(bsp, surface_index, corners, neighbours);
		if (count < 3)
			continue;
		node = &bots_graph.nodes[bots_graph.node_count];
		node->middle.x = node->middle.y = node->middle.z = 0.0f;
		for (corner = 0; corner < count; corner++)
		{
			node->middle.x += corners[corner].x / (real)count;
			node->middle.y += corners[corner].y / (real)count;
			node->middle.z += corners[corner].z / (real)count;
		}
		/* (the area and the normal: the fan of triangles from the first corner) */
		for (corner = 1; corner + 1 < count; corner++)
		{
			real_vector3d a, b, c;

			vector_from_points3d(&corners[0], &corners[corner], &a);
			vector_from_points3d(&corners[0], &corners[corner + 1], &b);
			cross_product3d(&a, &b, &c);
			normal.i += c.i;
			normal.j += c.j;
			normal.k += c.k;
		}
		/* (not under a machine: the surface reaching into its sphere) */
		reach = 0.0f;
		for (corner = 0; corner < count; corner++)
			reach = MAX(reach, bots_routes_distance2d(&node->middle, &corners[corner]));
		for (corner = 0; corner < bots_routes.machine_count; corner++)
		{
			real_point3d const *center = &bots_routes.machine_centers[corner];
			real radius = bots_routes.machine_radii[corner];

			if (bots_routes_distance2d(&node->middle, center) < radius * 0.8f + reach &&
				node->middle.z > center->z - radius - 1.0f && node->middle.z < center->z + radius)
			{
				break;
			}
		}
		if (corner < bots_routes.machine_count)
		{
			bots_routes.machine_surfaces++;
			continue;
		}
		node->area = 0.5f * (real)sqrt(normal.i * normal.i + normal.j * normal.j + normal.k * normal.k);
		node->normal_k = node->area > 0.0f ? (real)fabs(normal.k) * 0.5f / node->area : 0.0f;
		node->surface_index = surface_index;
		node->cost = REAL_MAX;
		node->route_distance = 0.0f;
		node->route_side = 0.0f;
		node->route_along = 0.0f;
		/* (its walkable neighbours' surfaces, made nodes' indices once all
		are made: bots_routes_links_step) */
		bots_graph.link_first[bots_graph.node_count] = bots_graph.link_count;
		for (corner = 0; corner < count; corner++)
		{
			if (!bots_routes_walkable(neighbours[corner]))
				continue;
			if (bots_graph.link_count >= bots_routes.link_capacity)
			{
				long capacity = bots_routes.link_capacity * 2;
				long *links = (long *)realloc(bots_graph.links, sizeof(long) * (size_t)capacity);

				if (!links)
					break;
				bots_graph.links = links;
				bots_routes.link_capacity = capacity;
			}
			bots_graph.links[bots_graph.link_count++] = neighbours[corner];
		}
		bots_routes.node_of_surface[surface_index] = bots_graph.node_count;
		bots_graph.node_count++;
	}
	if (bots_routes.cursor < bots_routes.surface_count)
		return FALSE;
	bots_graph.link_first[bots_graph.node_count] = bots_graph.link_count;
	return TRUE;
}

/* the links: a step of the nodes' neighbours' surfaces made nodes' indices
(those left out dropped, the links packed down in place); TRUE when all are */
static boolean bots_routes_links_step(void)
{
	long last = MIN(bots_routes.cursor + BOTS_ROUTES_SURFACES_A_TICK * 4, bots_graph.node_count);

	for (; bots_routes.cursor < last; bots_routes.cursor++)
	{
		long node_index = bots_routes.cursor;
		long first = bots_graph.link_first[node_index], end = bots_graph.link_first[node_index + 1], link;

		bots_graph.link_first[node_index] = bots_routes.link_write;
		for (link = first; link < end; link++)
		{
			long other = bots_routes.node_of_surface[bots_graph.links[link]];

			if (other != NONE)
				bots_graph.links[bots_routes.link_write++] = other;
		}
	}
	if (bots_routes.cursor < bots_graph.node_count)
		return FALSE;
	bots_graph.link_first[bots_graph.node_count] = bots_routes.link_write;
	bots_graph.link_count = bots_routes.link_write;
	return TRUE;
}

/* the node of the walkable surface under a point, else the nearest node's */
static long bots_routes_node_at(real_point3d const *point)
{
	long surface_index = bots_routes_ground(point, NULL);
	long index, best = NONE;
	real best_distance = REAL_MAX;

	if (surface_index >= 0 && surface_index < bots_routes.surface_count &&
		bots_routes.node_of_surface[surface_index] != NONE)
	{
		return bots_routes.node_of_surface[surface_index];
	}
	for (index = 0; index < bots_graph.node_count; index++)
	{
		real distance = bots_routes_distance3d(&bots_graph.nodes[index].middle, point);

		if (distance < best_distance)
		{
			best_distance = distance;
			best = index;
		}
	}
	return best;
}

/* the map's two bases: the CTF flag stands (every stock map has them, in
every gametype), else (set 1, or no way between those: Prisoner's on its
upper floor) the teams' starting locations' middles, else (set 2) the two
starting locations furthest apart; FALSE without */
static boolean bots_routes_find_bases(long set, real_point3d *bases)
{
	struct scenario *scenario = global_scenario_get();
	boolean found[2] = { FALSE, FALSE };
	real_point3d sums[2];
	long counts[2] = { 0, 0 };
	long index, other;
	real best = 0.0f;

	if (!scenario)
		return FALSE;
	for (index = 0; index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index, struct scenario_netgame_flag);

		if (flag->type == _netgame_flag_ctf_flag && (flag->team_index == 0 || flag->team_index == 1) &&
			!found[flag->team_index])
		{
			bases[flag->team_index] = flag->position;
			found[flag->team_index] = TRUE;
		}
	}
	if (found[0] && found[1] && set <= 0)
		return TRUE;
	csmemset(sums, 0, sizeof(sums));
	for (index = 0; index < scenario->players.count; index++)
	{
		struct player_starting_location *location =
			TAG_BLOCK_GET_ELEMENT(&scenario->players, index, struct player_starting_location);

		if (location->team_index == 0 || location->team_index == 1)
		{
			sums[location->team_index].x += location->position.x;
			sums[location->team_index].y += location->position.y;
			sums[location->team_index].z += location->position.z;
			counts[location->team_index]++;
		}
	}
	if (counts[0] > 0 && counts[1] > 0)
	{
		for (index = 0; index < 2; index++)
		{
			bases[index].x = sums[index].x / (real)counts[index];
			bases[index].y = sums[index].y / (real)counts[index];
			bases[index].z = sums[index].z / (real)counts[index];
		}
		if (bots_routes_distance2d(&bases[0], &bases[1]) > 10.0f && set <= 1)
			return TRUE;
	}
	for (index = 0; index < scenario->players.count; index++)
	{
		struct player_starting_location *a = TAG_BLOCK_GET_ELEMENT(&scenario->players, index, struct player_starting_location);

		for (other = index + 1; other < scenario->players.count; other++)
		{
			struct player_starting_location *b = TAG_BLOCK_GET_ELEMENT(&scenario->players, other, struct player_starting_location);
			real distance = bots_routes_distance2d(&a->position, &b->position);

			if (distance > best)
			{
				best = distance;
				bases[0] = a->position;
				bases[1] = b->position;
			}
		}
	}
	return best > 10.0f;
}

/* ---------- the searches (Dijkstra's, a step of surfaces a tick) */

/* what entering a node costs a lane's search: the nearer the direct route
the dearer, and the other side of it dearer still; neither near the bases
(the first and last sixth of the way, full from a third on), which every
way leaves and comes to */
static real bots_routes_weight(long node_index)
{
	struct bots_graph_node const *node = &bots_graph.nodes[node_index];
	real weight = 1.0f, middle;

	if (bots_routes.lane_side == 0.0f)
		return 1.0f;
	middle = MIN(node->route_along, 1.0f - node->route_along);
	middle = PIN((middle - 0.15f) / 0.15f, 0.0f, 1.0f);
	if (middle <= 0.0f)
		return 1.0f;
	if (node->route_distance < bots_routes.penalty_radius)
		weight += 4.0f * middle * (1.0f - node->route_distance / bots_routes.penalty_radius);
	if (node->route_side * bots_routes.lane_side < 0.0f && node->route_distance > 2.0f)
		weight += 10.0f * middle;
	return weight;
}

static boolean bots_routes_heap_less(long a, long b)
{
	return bots_routes.cost[bots_routes.heap[a]] < bots_routes.cost[bots_routes.heap[b]];
}

static void bots_routes_heap_swap(long a, long b)
{
	long node = bots_routes.heap[a];

	bots_routes.heap[a] = bots_routes.heap[b];
	bots_routes.heap[b] = node;
	bots_routes.heap_position[bots_routes.heap[a]] = a;
	bots_routes.heap_position[bots_routes.heap[b]] = b;
}

static void bots_routes_heap_up(long position)
{
	while (position > 0 && bots_routes_heap_less(position, (position - 1) / 2))
	{
		bots_routes_heap_swap(position, (position - 1) / 2);
		position = (position - 1) / 2;
	}
}

static void bots_routes_heap_down(long position)
{
	for (;;)
	{
		long child = position * 2 + 1, best = position;

		if (child < bots_routes.heap_count && bots_routes_heap_less(child, best))
			best = child;
		if (child + 1 < bots_routes.heap_count && bots_routes_heap_less(child + 1, best))
			best = child + 1;
		if (best == position)
			return;
		bots_routes_heap_swap(position, best);
		position = best;
	}
}

static void bots_routes_search_start(long origin, long target, real lane_side)
{
	long index;

	for (index = 0; index < bots_graph.node_count; index++)
	{
		bots_routes.cost[index] = REAL_MAX;
		bots_routes.previous[index] = NONE;
		bots_routes.heap_position[index] = NONE;
	}
	bots_routes.origin = origin;
	bots_routes.target = target;
	bots_routes.lane_side = lane_side;
	bots_routes.cost[origin] = 0.0f;
	bots_routes.heap[0] = origin;
	bots_routes.heap_position[origin] = 0;
	bots_routes.heap_count = 1;
}

/* a step of the search; TRUE when done (every node reached taken, or the
target, when there is one) */
static boolean bots_routes_search_step(void)
{
	long pops;

	for (pops = 0; pops < BOTS_ROUTES_POPS_A_TICK && bots_routes.heap_count > 0; pops++)
	{
		long node_index = bots_routes.heap[0];
		real node_weight = bots_routes_weight(node_index);
		long link;

		bots_routes.heap_count--;
		if (bots_routes.heap_count > 0)
		{
			bots_routes.heap[0] = bots_routes.heap[bots_routes.heap_count];
			bots_routes.heap_position[bots_routes.heap[0]] = 0;
			bots_routes_heap_down(0);
		}
		bots_routes.heap_position[node_index] = -2;
		if (node_index == bots_routes.target)
			return TRUE;
		for (link = bots_graph.link_first[node_index]; link < bots_graph.link_first[node_index + 1]; link++)
		{
			long other = bots_graph.links[link];
			real cost;

			if (bots_routes.heap_position[other] == -2)
				continue;
			cost = bots_routes.cost[node_index] + bots_routes_distance3d(&bots_graph.nodes[node_index].middle,
				&bots_graph.nodes[other].middle) * 0.5f * (node_weight + bots_routes_weight(other));
			if (cost >= bots_routes.cost[other])
				continue;
			bots_routes.cost[other] = cost;
			bots_routes.previous[other] = node_index;
			if (bots_routes.heap_position[other] == NONE)
			{
				bots_routes.heap[bots_routes.heap_count] = other;
				bots_routes.heap_position[other] = bots_routes.heap_count;
				bots_routes.heap_count++;
			}
			bots_routes_heap_up(bots_routes.heap_position[other]);
		}
	}
	return bots_routes.heap_count <= 0;
}

/* ---------- the direct route */

void bots_routes_place_on_route(real_point3d const *point, real *along, real *offset)
{
	long index, best = 0;
	real best_distance = REAL_MAX, best_u = 0.0f, best_side = 1.0f;

	*along = 0.0f;
	*offset = 0.0f;
	if (bots_routes_map.route_point_count < 2 || bots_routes_map.route_length <= 0.0f)
		return;
	for (index = 0; index + 1 < bots_routes_map.route_point_count; index++)
	{
		real_point3d const *a = &bots_routes_map.route_points[index], *b = &bots_routes_map.route_points[index + 1];
		real dx = b->x - a->x, dy = b->y - a->y, px = point->x - a->x, py = point->y - a->y;
		real length2 = dx * dx + dy * dy;
		real u = length2 > 0.0001f ? (px * dx + py * dy) / length2 : 0.0f;
		real cx, cy, distance;

		u = PIN(u, 0.0f, 1.0f);
		cx = a->x + dx * u - point->x;
		cy = a->y + dy * u - point->y;
		distance = (real)sqrt(cx * cx + cy * cy);
		if (distance < best_distance)
		{
			best_distance = distance;
			best = index;
			best_u = u;
			best_side = dx * py - dy * px >= 0.0f ? 1.0f : -1.0f;
		}
	}
	*along = (bots_routes.point_lengths[best] + bots_routes.segment_lengths[best] * best_u) / bots_routes_map.route_length;
	*offset = best_distance * best_side;
}

/* the direct route from the search's way back from the second base: its
points every sixteenth of its length (2D), FALSE when the second base was
not reached */
static boolean bots_routes_make_direct(void)
{
	long node_index, count = 0, point;
	real total = 0.0f, walked = 0.0f;
	long *route;

	if (bots_routes.cost[bots_routes.target] >= REAL_MAX)
		return FALSE;
	for (node_index = bots_routes.target; node_index != NONE; node_index = bots_routes.previous[node_index])
		count++;
	route = (long *)malloc(sizeof(long) * (size_t)count);
	if (!route)
		return FALSE;
	point = count;
	for (node_index = bots_routes.target; node_index != NONE; node_index = bots_routes.previous[node_index])
		route[--point] = node_index;
	for (point = 0; point + 1 < count; point++)
		total += bots_routes_distance2d(&bots_graph.nodes[route[point]].middle, &bots_graph.nodes[route[point + 1]].middle);
	if (total < 1.0f || count < 2)
	{
		free(route);
		return FALSE;
	}
	bots_routes_map.route_points[0] = bots_graph.nodes[route[0]].middle;
	bots_routes_map.route_point_count = 1;
	for (point = 0; point + 1 < count && bots_routes_map.route_point_count < BOTS_ROUTE_POINTS; point++)
	{
		real_point3d const *a = &bots_graph.nodes[route[point]].middle, *b = &bots_graph.nodes[route[point + 1]].middle;
		real step = bots_routes_distance2d(a, b);

		while (step > 0.0f && bots_routes_map.route_point_count < BOTS_ROUTE_POINTS - 1 &&
			walked + step >= total * (real)bots_routes_map.route_point_count / (real)(BOTS_ROUTE_POINTS - 1))
		{
			real u = (total * (real)bots_routes_map.route_point_count / (real)(BOTS_ROUTE_POINTS - 1) - walked) / step;
			real_point3d *p = &bots_routes_map.route_points[bots_routes_map.route_point_count++];

			p->x = a->x + (b->x - a->x) * u;
			p->y = a->y + (b->y - a->y) * u;
			p->z = a->z + (b->z - a->z) * u;
		}
		walked += step;
	}
	bots_routes_map.route_points[bots_routes_map.route_point_count++] = bots_graph.nodes[route[count - 1]].middle;
	free(route);
	bots_routes_map.route_length = 0.0f;
	for (point = 0; point < bots_routes_map.route_point_count; point++)
	{
		bots_routes.point_lengths[point] = bots_routes_map.route_length;
		bots_routes.segment_lengths[point] = point + 1 < bots_routes_map.route_point_count ?
			bots_routes_distance2d(&bots_routes_map.route_points[point], &bots_routes_map.route_points[point + 1]) : 0.0f;
		bots_routes_map.route_length += bots_routes.segment_lengths[point];
	}
	return bots_routes_map.route_length > 1.0f;
}

/* a step of the nodes placed across the direct route; TRUE when done */
static boolean bots_routes_place_step(void)
{
	long last = MIN(bots_routes.cursor + BOTS_ROUTES_NODES_A_TICK, bots_graph.node_count);

	for (; bots_routes.cursor < last; bots_routes.cursor++)
	{
		struct bots_graph_node *node = &bots_graph.nodes[bots_routes.cursor];
		real offset;

		bots_routes_place_on_route(&node->middle, &node->route_along, &offset);
		node->route_distance = (real)fabs(offset);
		node->route_side = offset >= 0.0f ? 1.0f : -1.0f;
	}
	return bots_routes.cursor >= bots_graph.node_count;
}

/* (debug) HALO_BOT_ROUTES_DUMP=<file>: the walkable graph's nodes (where,
how far off the direct route, reached) and each lane's surfaces, for a
picture of them */
static void bots_routes_dump(char const *what, long const *route, long count)
{
	char const *name = getenv("HALO_BOT_ROUTES_DUMP");
	FILE *file;
	long index;

	if (!name || !name[0] || !(file = fopen(name, route ? "a" : "w")))
		return;
	if (!route)
	{
		for (index = 0; index < bots_graph.node_count; index++)
		{
			struct bots_graph_node const *node = &bots_graph.nodes[index];

			fprintf(file, "node %.2f %.2f %.2f %.2f %.0f %d\n", node->middle.x, node->middle.y, node->middle.z,
				node->route_distance, node->route_side, node->cost < REAL_MAX);
		}
		for (index = 0; index < bots_routes_map.route_point_count; index++)
			fprintf(file, "route %.2f %.2f %.2f\n", bots_routes_map.route_points[index].x, bots_routes_map.route_points[index].y,
				bots_routes_map.route_points[index].z);
	}
	for (index = 0; route && index < count; index++)
	{
		struct bots_graph_node const *node = &bots_graph.nodes[route[index]];

		fprintf(file, "%s %.2f %.2f %.2f\n", what, node->middle.x, node->middle.y, node->middle.z);
	}
	fclose(file);
}

/* ---------- the side lanes */

/* the lane's search done: its way back made the lane, if it is one */
static void bots_routes_make_lane(long lane_index)
{
	struct bots_lane *lane = &bots_routes.lanes[lane_index];
	long node_index, count = 0, point, waypoint, peak = NONE;
	real length = 0.0f, offset = 0.0f, walked;
	long *route;
	char const *why = NULL;

	csmemset(lane, 0, sizeof(*lane));
	if (bots_routes.cost[bots_routes.target] >= REAL_MAX)
	{
		platform_log("bots: routes: lane %ld: none (the second base not reached)", lane_index + 1);
		return;
	}
	for (node_index = bots_routes.target; node_index != NONE; node_index = bots_routes.previous[node_index])
		count++;
	route = (long *)malloc(sizeof(long) * (size_t)count);
	if (!route)
		return;
	point = count;
	for (node_index = bots_routes.target; node_index != NONE; node_index = bots_routes.previous[node_index])
		route[--point] = node_index;
	for (point = 0; point < count; point++)
	{
		struct bots_graph_node const *node = &bots_graph.nodes[route[point]];

		if (point > 0)
			length += bots_routes_distance3d(&bots_graph.nodes[route[point - 1]].middle, &node->middle);
		if (node->route_along > 0.15f && node->route_along < 0.85f && node->route_distance > offset)
		{
			offset = node->route_distance;
			peak = point;
		}
	}
	lane->length = length;
	lane->offset = offset;
	bots_routes_dump(lane_index ? "lane2" : "lane1", route, count);
	if (peak == NONE || offset < MAX(5.0f, bots_routes_map.route_length * 0.08f))
		why = "it keeps near the direct route";
	else if (length > bots_routes_map.route_length * 2.2f)
		why = "too long";
	/* the waypoints: its surfaces a third and two thirds of the way along
	it, those well off the direct route; else the one furthest off */
	if (!why)
	{
		static real const fractions[BOTS_LANE_WAYPOINTS] = { 0.33f, 0.67f };

		for (waypoint = 0; waypoint < BOTS_LANE_WAYPOINTS; waypoint++)
		{
			long best = NONE;
			real best_difference = REAL_MAX;

			walked = 0.0f;
			for (point = 0; point < count; point++)
			{
				struct bots_graph_node const *node = &bots_graph.nodes[route[point]];

				if (point > 0)
					walked += bots_routes_distance3d(&bots_graph.nodes[route[point - 1]].middle, &node->middle);
				if (node->route_distance >= offset * 0.5f && node->route_along > 0.1f && node->route_along < 0.9f &&
					fabs(walked / length - fractions[waypoint]) < best_difference)
				{
					best_difference = (real)fabs(walked / length - fractions[waypoint]);
					best = route[point];
				}
			}
			if (best == NONE || (lane->waypoint_count > 0 &&
				bots_routes_distance2d(&lane->waypoints[lane->waypoint_count - 1].point, &bots_graph.nodes[best].middle) < 8.0f))
			{
				continue;
			}
			lane->waypoints[lane->waypoint_count].point = bots_graph.nodes[best].middle;
			lane->waypoints[lane->waypoint_count].surface_index = bots_graph.nodes[best].surface_index;
			lane->waypoints[lane->waypoint_count].along = bots_graph.nodes[best].route_along;
			lane->waypoint_count++;
		}
		if (!lane->waypoint_count)
		{
			lane->waypoints[0].point = bots_graph.nodes[route[peak]].middle;
			lane->waypoints[0].surface_index = bots_graph.nodes[route[peak]].surface_index;
			lane->waypoints[0].along = bots_graph.nodes[route[peak]].route_along;
			lane->waypoint_count = 1;
		}
		/* (in order along the direct route) */
		if (lane->waypoint_count == 2 && lane->waypoints[0].along > lane->waypoints[1].along)
		{
			struct bots_lane_waypoint first = lane->waypoints[0];

			lane->waypoints[0] = lane->waypoints[1];
			lane->waypoints[1] = first;
		}
	}
	/* (the second lane by the first's waypoints: the same lane) */
	if (!why && lane_index == 1 && bots_routes.lanes[0].valid)
	{
		long a, b;

		for (a = 0; a < bots_routes.lanes[0].waypoint_count; a++)
		{
			for (b = 0; b < lane->waypoint_count; b++)
			{
				if (bots_routes_distance2d(&bots_routes.lanes[0].waypoints[a].point, &lane->waypoints[b].point) < 10.0f)
					why = "the first lane's";
			}
		}
	}
	free(route);
	if (why)
	{
		lane->waypoint_count = 0;
		platform_log("bots: routes: lane %ld: none (%s: %.0f units, %.0f off the direct route at most)", lane_index + 1, why,
			length, offset);
		return;
	}
	lane->valid = TRUE;
	bots_routes_map.lane_count++;
	if (lane->waypoint_count == 2)
	{
		platform_log("bots: routes: lane %ld by (%.1f %.1f %.1f) (%.1f %.1f %.1f): %.0f units (the direct route %.0f), %.0f off it at most",
			lane_index + 1, lane->waypoints[0].point.x, lane->waypoints[0].point.y, lane->waypoints[0].point.z,
			lane->waypoints[1].point.x, lane->waypoints[1].point.y, lane->waypoints[1].point.z, length,
			bots_routes_map.route_length, offset);
	}
	else
	{
		platform_log("bots: routes: lane %ld by (%.1f %.1f %.1f): %.0f units (the direct route %.0f), %.0f off it at most",
			lane_index + 1, lane->waypoints[0].point.x, lane->waypoints[0].point.y, lane->waypoints[0].point.z, length,
			bots_routes_map.route_length, offset);
	}
}

/* ---------- making them: a step a tick */

static void bots_routes_log_direct(void)
{
	char line[600];
	int length = 0;
	long point;

	for (point = 0; point < bots_routes_map.route_point_count && length < (int)sizeof(line) - 30; point++)
	{
		int written = snprintf(line + length, sizeof(line) - (size_t)length, " (%.1f %.1f)", bots_routes_map.route_points[point].x,
			bots_routes_map.route_points[point].y);
		/* snprintf returns the required length, even for extreme map coordinates. */
		if (written < 0 || written >= (int)sizeof(line) - length)
			break;
		length += written;
	}
	line[length] = 0;
	platform_log("bots: routes: walkable graph of %ld surfaces (%ld links; %ld under %ld machines left out), bases (%.1f %.1f %.1f) "
		"and (%.1f %.1f %.1f); the direct route %.0f units:%s", bots_graph.node_count, bots_graph.link_count,
		bots_routes.machine_surfaces, bots_routes.machine_count, bots_routes_map.bases[0].x,
		bots_routes_map.bases[0].y, bots_routes_map.bases[0].z, bots_routes_map.bases[1].x, bots_routes_map.bases[1].y,
		bots_routes_map.bases[1].z, bots_routes_map.route_length, line);
}

/* a step of the map's routes; TRUE once they are made (or the map has none) */
boolean bots_routes_step(void)
{
	switch (bots_routes.phase)
	{
	case _routes_phase_idle:
		bots_routes.phase = bots_routes_begin() ? _routes_phase_machines : _routes_phase_none;
		break;
	case _routes_phase_machines:
		bots_routes_find_machines();
		bots_routes.phase = _routes_phase_nodes;
		break;
	case _routes_phase_nodes:
		if (bots_routes_nodes_step())
		{
			bots_routes.cursor = 0;
			bots_routes.link_write = 0;
			bots_routes.phase = _routes_phase_links;
		}
		break;
	case _routes_phase_links:
		if (bots_routes_links_step())
			bots_routes.phase = _routes_phase_bases;
		break;
	case _routes_phase_bases:
	{
		real_point3d bases[2];
		long a, b;

		if (bots_graph.node_count < 8 || !bots_routes_find_bases(bots_routes.base_set, bases))
		{
			platform_log("bots: routes: no bases found: no lanes");
			bots_routes.phase = _routes_phase_none;
			break;
		}
		a = bots_routes_node_at(&bases[0]);
		b = bots_routes_node_at(&bases[1]);
		if (a == NONE || b == NONE || a == b)
		{
			platform_log("bots: routes: the bases are on no walkable surface: no lanes");
			bots_routes.phase = _routes_phase_none;
			break;
		}
		bots_routes_map.bases[0] = bots_graph.nodes[a].middle;
		bots_routes_map.bases[1] = bots_graph.nodes[b].middle;
		bots_routes_map.base_surfaces[0] = bots_graph.nodes[a].surface_index;
		bots_routes_map.base_surfaces[1] = bots_graph.nodes[b].surface_index;
		bots_routes_map.has_bases = TRUE;
		bots_routes.base_nodes[0] = a;
		bots_routes.base_nodes[1] = b;
		/* (the whole graph from the first base: its costs say what is reached) */
		bots_routes_search_start(a, NONE, 0.0f);
		bots_routes.phase = _routes_phase_direct;
		break;
	}
	case _routes_phase_direct:
		if (bots_routes_search_step())
		{
			long index;

			for (index = 0; index < bots_graph.node_count; index++)
				bots_graph.nodes[index].cost = bots_routes.cost[index];
			bots_routes.costs_made = TRUE;
			bots_routes.target = bots_routes.base_nodes[1];
			/* (no way between them: the next bases) */
			if (bots_routes.cost[bots_routes.target] >= REAL_MAX && bots_routes.base_set < 2)
			{
				platform_log("bots: routes: no way between the bases (%.1f %.1f %.1f) and (%.1f %.1f %.1f): others",
					bots_routes_map.bases[0].x, bots_routes_map.bases[0].y, bots_routes_map.bases[0].z,
					bots_routes_map.bases[1].x, bots_routes_map.bases[1].y, bots_routes_map.bases[1].z);
				bots_routes.base_set++;
				bots_routes_map.has_bases = FALSE;
				bots_routes.phase = _routes_phase_bases;
				break;
			}
			if (!bots_routes_make_direct())
			{
				platform_log("bots: routes: no way between the bases: no lanes");
				bots_routes_map.has_bases = FALSE;
				bots_routes.phase = _routes_phase_none;
				break;
			}
			bots_routes_log_direct();
			bots_routes.cursor = 0;
			bots_routes.phase = _routes_phase_place;
		}
		break;
	case _routes_phase_place:
		if (bots_routes_place_step())
		{
			bots_routes_dump(NULL, NULL, 0);
			bots_routes.penalty_radius = PIN(bots_routes_map.route_length * 0.15f, 6.0f, 18.0f);
			bots_routes.lane_index = 0;
			bots_routes_search_start(bots_routes.base_nodes[0], bots_routes.base_nodes[1], 1.0f);
			bots_routes.phase = _routes_phase_lanes;
		}
		break;
	case _routes_phase_lanes:
		if (bots_routes_search_step())
		{
			bots_routes_make_lane(bots_routes.lane_index);
			if (++bots_routes.lane_index < BOTS_SIDE_LANES)
				bots_routes_search_start(bots_routes.base_nodes[0], bots_routes.base_nodes[1], -1.0f);
			else
			{
				/* (the second lane but not the first: it is the first) */
				if (!bots_routes.lanes[0].valid && bots_routes.lanes[1].valid)
				{
					bots_routes.lanes[0] = bots_routes.lanes[1];
					bots_routes.lanes[1].valid = FALSE;
				}
				bots_routes.phase = _routes_phase_done;
			}
		}
		break;
	case _routes_phase_done:
	case _routes_phase_none:
		bots_routes_map.ready = TRUE;
		return TRUE;
	}
	return FALSE;
}

/* whether the graph is there to read (the vantage points' making) */
boolean bots_routes_graph_made(void)
{
	return bots_routes.costs_made && bots_graph.nodes != NULL;
}

/* ---------- a bot's lane */

static boolean bots_routes_lane_valid(short lane)
{
	return lane >= 1 && lane <= BOTS_SIDE_LANES && bots_routes.lanes[lane - 1].valid;
}

/* the lane of the bot of this rank among its team's attackers (0 the direct
route): Lanes the direct, the first and the second in turn; Mixed every
third a side lane, the first and the second in turn */
short bots_routes_lane_for_rank(short rank)
{
	short lane = 0;

	if (bots_routes.mode == _bots_routes_lanes)
		lane = (short)(rank % 3);
	else if (bots_routes.mode == _bots_routes_mixed)
		lane = rank % 3 == 1 ? (short)((rank / 3) % 2 + 1) : 0;
	if (lane && !bots_routes_lane_valid(lane))
		lane = bots_routes_lane_valid(1) ? 1 : 0;
	return lane;
}

boolean bots_routes_search_goal(long bot_index, real_point3d const *start, real_point3d *goal, long *surface_index)
{
	struct bots_tactics_bot *state;
	struct bots_lane const *lane;
	real start_along, goal_along, offset, travel;
	long index, best = NONE;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return FALSE;
	state = &bots_tactics_bots[bot_index];
	if (bots_routes.mode == _bots_routes_direct || !bots_routes_map.ready || !bots_routes_lane_valid(state->lane) ||
		state->lane_given_up || state->vantage != NONE || bots_routes_distance2d(start, goal) < BOTS_LANE_LEAST_DISTANCE)
	{
		state->waypoint = NONE;
		return FALSE;
	}
	lane = &bots_routes.lanes[state->lane - 1];
	bots_routes_place_on_route(start, &start_along, &offset);
	bots_routes_place_on_route(goal, &goal_along, &offset);
	travel = goal_along - start_along;
	if (fabs(travel) >= 0.2f)
	{
		/* (the first waypoint on the way, in the way the bot goes) */
		for (index = 0; index < lane->waypoint_count && best == NONE; index++)
		{
			long waypoint = travel > 0.0f ? index : lane->waypoint_count - 1 - index;
			real along = lane->waypoints[waypoint].along;

			if (state->passed[waypoint])
				continue;
			if (travel > 0.0f ? along > start_along + 0.03f && along < goal_along - 0.03f :
				along < start_along - 0.03f && along > goal_along + 0.03f)
			{
				best = waypoint;
			}
		}
	}
	if (best == NONE)
	{
		state->waypoint = NONE;
		return FALSE;
	}
	if (state->waypoint != best)
	{
		state->waypoint = (short)best;
		state->waypoint_tick = game_time_get();
		bots_tactics_counts.lanes_taken++;
	}
	*goal = lane->waypoints[best].point;
	*surface_index = lane->waypoints[best].surface_index;
	return TRUE;
}

boolean bots_routes_waypoint_reached(long bot_index, real_point3d const *position)
{
	struct bots_tactics_bot *state;
	real_point3d const *point;

	if (bot_index < 0 || bot_index >= BOTS_MAXIMUM)
		return FALSE;
	state = &bots_tactics_bots[bot_index];
	if (state->waypoint == NONE || !bots_routes_lane_valid(state->lane))
		return FALSE;
	point = &bots_routes.lanes[state->lane - 1].waypoints[state->waypoint].point;
	if (bots_routes_distance2d(point, position) < BOTS_LANE_WAYPOINT_REACHED && fabs(point->z - position->z) < 2.5f)
	{
		state->passed[state->waypoint] = TRUE;
		state->waypoint = NONE;
		bots_tactics_counts.waypoints_passed++;
		return TRUE;
	}
	if (game_time_get() - state->waypoint_tick > TICKS_PER_SECOND * BOTS_LANE_WAYPOINT_SECONDS)
	{
		state->lane_given_up = TRUE;
		state->waypoint = NONE;
		bots_tactics_counts.lanes_given_up++;
		return TRUE;
	}
	return FALSE;
}

/* whether the goal a search went to is the bot's waypoint, given up then */
boolean bots_routes_search_failed(long bot_index, real_point3d const *goal)
{
	struct bots_tactics_bot *state = &bots_tactics_bots[bot_index];

	if (state->waypoint == NONE || !bots_routes_lane_valid(state->lane) ||
		bots_routes_distance3d(goal, &bots_routes.lanes[state->lane - 1].waypoints[state->waypoint].point) > 0.5f)
	{
		return FALSE;
	}
	state->lane_given_up = TRUE;
	state->waypoint = NONE;
	bots_tactics_counts.lanes_given_up++;
	return TRUE;
}

/* the lane's name for halo.log */
char const *bots_routes_lane_name(short lane)
{
	return lane == 1 ? "lane 1" : lane == 2 ? "lane 2" : "direct";
}

/* a lane's waypoint (for the vantage points' places): FALSE past the last */
boolean bots_routes_lane_waypoint(long lane_index, long waypoint, real_point3d *point)
{
	if (lane_index < 0 || lane_index >= BOTS_SIDE_LANES || !bots_routes.lanes[lane_index].valid ||
		waypoint < 0 || waypoint >= bots_routes.lanes[lane_index].waypoint_count)
	{
		return FALSE;
	}
	*point = bots_routes.lanes[lane_index].waypoints[waypoint].point;
	return TRUE;
}
