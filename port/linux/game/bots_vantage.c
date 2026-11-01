/* Source: thelinkin3000/halo-ce-universal, commit
 * d92cc93385edf03d0a4a21cfe4c58dd578a3e786, port/linux/game/bots_vantage.c.
 * Derived from BirchWoodGod/halo-ce-vita (GPL-3.0-only).
 * Used only by GulchCE's optional enhanced bots.
 */
/* Optional enhanced bots' vantage points: standing places with sight of
flag stands, hills, routes and item spawns. A bounded surface scan retains
30 candidates; two sight rays per tick score their coverage and openness.
Up to six separated spots survive. The temporary graph and height grid are
released after preparation; per-bot claims are released on death/unload.
Weapon and objective policy is in bots_roles.c and enhanced_bots.c. */

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "physics/collisions.h"
#include "tag_files/tag_groups.h"

#include "bots.h"
#include "bots_tactics.h"
#include "bots_objectives.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ---------- constants */

/* the spots looked at with rays, and how far apart */
#define BOTS_VANTAGE_CANDIDATES 30
#define BOTS_VANTAGE_CANDIDATE_SPACING 6.0f
/* the vantage points kept, how far apart */
#define BOTS_VANTAGE_SPACING 12.0f
/* the work of a tick: surfaces scored, rays cast */
#define BOTS_VANTAGE_NODES_A_TICK 200
#define BOTS_VANTAGE_RAYS_A_TICK 2
/* a bot's eyes above its feet, a place's middle above its ground */
#define BOTS_VANTAGE_EYE_HEIGHT 0.62f
#define BOTS_PLACE_HEIGHT 0.55f
/* how far a place is seen, and the openness rays' length */
#define BOTS_VANTAGE_SIGHT 90.0f
#define BOTS_VANTAGE_OPEN_RAYS 8
#define BOTS_VANTAGE_OPEN_LENGTH 15.0f
/* the height grid's cells (world units) */
#define BOTS_VANTAGE_CELL 8.0f
#define BOTS_VANTAGE_GRID_MAXIMUM 128

enum
{
	_vantage_phase_idle = 0,
	_vantage_phase_bounds,
	_vantage_phase_grid,
	_vantage_phase_candidates,
	_vantage_phase_rays,
	_vantage_phase_done,
};

/* ---------- structures */

struct bots_vantage_spot
{
	real_point3d point;
	long surface_index;
	real height;
	real first_score;
	real score;
	unsigned long sees;
	short sees_count;
	short open;
};

/* ---------- globals */

static struct
{
	short phase;
	long cursor;
	unsigned long long started_us;
	/* the places */
	long place_count;
	struct bots_place places[BOTS_PLACES_MAXIMUM];
	/* the height grid: its corner, size, and each cell's area and area-weighted
	height */
	real grid_x;
	real grid_y;
	real grid_x1;
	real grid_y1;
	real cell;
	long grid_width;
	long grid_height;
	real *grid_area;
	real *grid_height_sum;
	/* the spots scored */
	long candidate_count;
	struct bots_vantage_spot candidates[BOTS_VANTAGE_CANDIDATES];
	long rays;
	/* the vantage points, the bot going to each (NONE: none) */
	long count;
	struct bots_vantage_spot points[BOTS_VANTAGE_MAXIMUM];
	long claims[BOTS_VANTAGE_MAXIMUM];
} bots_vantage;

/* ---------- small helpers */

static real bots_vantage_distance3d(real_point3d const *a, real_point3d const *b)
{
	real dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;

	return (real)sqrt(dx * dx + dy * dy + dz * dz);
}

/* how much a place this far away counts: best from twelve to fifty units */
static real bots_vantage_range(real distance)
{
	if (distance < 5.0f)
		return 0.2f;
	if (distance < 12.0f)
		return 0.2f + 0.8f * (distance - 5.0f) / 7.0f;
	if (distance < 50.0f)
		return 1.0f;
	if (distance < BOTS_VANTAGE_SIGHT)
		return 1.0f - 0.7f * (distance - 50.0f) / (BOTS_VANTAGE_SIGHT - 50.0f);
	return 0.0f;
}

/* whether a ray from `from` to `to` meets nothing (the bots' line of sight
but for vehicles, which move) */
static boolean bots_vantage_clear(real_point3d const *from, real_point3d const *to)
{
	struct collision_result collision;
	real_vector3d vector;

	vector.i = to->x - from->x;
	vector.j = to->y - from->y;
	vector.k = to->z - from->z;
	bots_vantage.rays++;
	return !collision_test_vector(_collision_test_for_line_of_sight_flags & ~FLAG(_collision_test_objects_vehicles_bit),
		from, &vector, NONE, &collision);
}

static void bots_vantage_add_place(real_point3d const *point, real weight, char const *name)
{
	real_point3d ground;
	struct bots_place *place;

	if (bots_vantage.place_count >= BOTS_PLACES_MAXIMUM)
		return;
	place = &bots_vantage.places[bots_vantage.place_count];
	place->point = *point;
	if (bots_routes_ground(point, &ground) != NONE)
		place->point = ground;
	place->point.z += BOTS_PLACE_HEIGHT;
	place->weight = weight;
	place->name = name;
	bots_vantage.place_count++;
}

/* the places that matter in this game */
static void bots_vantage_find_places(void)
{
	struct scenario *scenario = global_scenario_get();
	long game = bots_tactics_game();
	long index, lane, waypoint;
	real_point3d point;

	bots_vantage.place_count = 0;
	if (!scenario)
		return;
	for (index = 0; index < scenario->netgame_flags.count; index++)
	{
		struct scenario_netgame_flag *flag = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index, struct scenario_netgame_flag);

		if (flag->type == _netgame_flag_ctf_flag && (flag->team_index == 0 || flag->team_index == 1))
			bots_vantage_add_place(&flag->position, game == game_engine_ctf ? 3.0f : 1.5f, flag->team_index ? "stand 1" : "stand 0");
	}
	if (game == game_engine_king)
	{
		/* (a hill's markers share its team index: their middle) */
		for (index = 0; index < 8; index++)
		{
			real_point3d sum = { 0.0f, 0.0f, 0.0f };
			long marker, count = 0;

			for (marker = 0; marker < scenario->netgame_flags.count; marker++)
			{
				struct scenario_netgame_flag *flag =
					TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, marker, struct scenario_netgame_flag);

				if (flag->type == _netgame_flag_hill && flag->team_index == index)
				{
					sum.x += flag->position.x;
					sum.y += flag->position.y;
					sum.z += flag->position.z;
					count++;
				}
			}
			if (count)
			{
				sum.x /= (real)count;
				sum.y /= (real)count;
				sum.z /= (real)count;
				bots_vantage_add_place(&sum, 3.0f, "hill");
			}
		}
	}
	if (game == game_engine_oddball)
	{
		long count = 0;

		for (index = 0; index < scenario->netgame_flags.count && count < 4; index++)
		{
			struct scenario_netgame_flag *flag =
				TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, index, struct scenario_netgame_flag);

			if (flag->type == _netgame_flag_oddball_ball_spawn)
			{
				bots_vantage_add_place(&flag->position, 2.0f, "ball spawn");
				count++;
			}
		}
	}
	if (bots_routes_map.route_point_count == BOTS_ROUTE_POINTS)
	{
		for (index = 4; index <= 12; index += 4)
			bots_vantage_add_place(&bots_routes_map.route_points[index], 2.0f, "route");
	}
	for (lane = 0; lane < BOTS_SIDE_LANES; lane++)
	{
		for (waypoint = 0; bots_routes_lane_waypoint(lane, waypoint, &point); waypoint++)
			bots_vantage_add_place(&point, 1.0f, "lane");
	}
	/* the weapon and item spawns, where the players go (Sidewinder's field,
	whose stands and direct route are in its bases and gullies), as many as
	there is room for, each the furthest from those taken */
	{
		long first = bots_vantage.place_count, count;

		for (count = 0; bots_vantage.place_count < BOTS_PLACES_MAXIMUM && count < 8; count++)
		{
			long best = NONE;
			real best_distance = -1.0f;

			for (index = 0; index < scenario->netgame_equipment.count; index++)
			{
				struct scenario_netgame_equipment *equipment =
					TAG_BLOCK_GET_ELEMENT(&scenario->netgame_equipment, index, struct scenario_netgame_equipment);
				real nearest = REAL_MAX;
				long place;

				for (place = 0; place < bots_vantage.place_count; place++)
				{
					real distance = bots_vantage_distance3d(&bots_vantage.places[place].point, &equipment->position);

					if (place >= first || distance < 2.0f)
						nearest = MIN(nearest, distance);
				}
				if (nearest > best_distance && nearest > 6.0f)
				{
					best_distance = nearest;
					best = index;
				}
			}
			if (best == NONE)
				break;
			bots_vantage_add_place(&TAG_BLOCK_GET_ELEMENT(&scenario->netgame_equipment, best,
				struct scenario_netgame_equipment)->position, 1.0f, "item");
		}
	}
}

static void bots_vantage_free(void)
{
	/* (the game's free, debug_memory.c's, takes no NULL) */
	if (bots_vantage.grid_area)
		free(bots_vantage.grid_area);
	if (bots_vantage.grid_height_sum)
		free(bots_vantage.grid_height_sum);
	bots_vantage.grid_area = NULL;
	bots_vantage.grid_height_sum = NULL;
}

/* ---------- making them */

void bots_vantage_dispose(void)
{
	long index;

	bots_vantage_free();
	bots_vantage.phase = _vantage_phase_idle;
	bots_vantage.count = 0;
	bots_vantage.candidate_count = 0;
	bots_vantage.place_count = 0;
	bots_vantage.rays = 0;
	for (index = 0; index < BOTS_VANTAGE_MAXIMUM; index++)
		bots_vantage.claims[index] = NONE;
}

static void bots_vantage_begin(void)
{
	bots_vantage_dispose();
	bots_vantage.phase = _vantage_phase_bounds;
	bots_vantage.cursor = 0;
	bots_vantage.grid_x = bots_vantage.grid_y = REAL_MAX;
	bots_vantage.grid_x1 = bots_vantage.grid_y1 = -REAL_MAX;
	bots_vantage.grid_width = bots_vantage.grid_height = 0;
	bots_vantage_find_places();
}

/* the height grid's cell of a point (NONE outside) */
static long bots_vantage_cell(real x, real y)
{
	long cx = (long)((x - bots_vantage.grid_x) / bots_vantage.cell);
	long cy = (long)((y - bots_vantage.grid_y) / bots_vantage.cell);

	if (cx < 0 || cy < 0 || cx >= bots_vantage.grid_width || cy >= bots_vantage.grid_height)
		return NONE;
	return cy * bots_vantage.grid_width + cx;
}

/* how high a spot stands above the walkable surfaces about it (the area's
mean height in the 3 by 3 cells about it) */
static real bots_vantage_height(real_point3d const *point)
{
	long cx = (long)((point->x - bots_vantage.grid_x) / bots_vantage.cell);
	long cy = (long)((point->y - bots_vantage.grid_y) / bots_vantage.cell);
	real area = 0.0f, sum = 0.0f;
	long x, y;

	for (y = cy - 1; y <= cy + 1; y++)
	{
		for (x = cx - 1; x <= cx + 1; x++)
		{
			if (x < 0 || y < 0 || x >= bots_vantage.grid_width || y >= bots_vantage.grid_height)
				continue;
			area += bots_vantage.grid_area[y * bots_vantage.grid_width + x];
			sum += bots_vantage.grid_height_sum[y * bots_vantage.grid_width + x];
		}
	}
	return area > 0.0f ? point->z - sum / area : 0.0f;
}

/* a spot kept among the candidates by its first score (a better one within
their spacing taking its place) */
static void bots_vantage_consider(struct bots_vantage_spot const *spot)
{
	long index, worst = NONE;

	for (index = 0; index < bots_vantage.candidate_count; index++)
	{
		struct bots_vantage_spot *other = &bots_vantage.candidates[index];

		if (bots_vantage_distance3d(&other->point, &spot->point) < BOTS_VANTAGE_CANDIDATE_SPACING)
		{
			if (other->first_score < spot->first_score)
				*other = *spot;
			return;
		}
	}
	if (bots_vantage.candidate_count < BOTS_VANTAGE_CANDIDATES)
	{
		bots_vantage.candidates[bots_vantage.candidate_count++] = *spot;
		return;
	}
	for (index = 0; index < bots_vantage.candidate_count; index++)
	{
		if (worst == NONE || bots_vantage.candidates[index].first_score < bots_vantage.candidates[worst].first_score)
			worst = index;
	}
	if (bots_vantage.candidates[worst].first_score < spot->first_score)
		bots_vantage.candidates[worst] = *spot;
}

/* the vantage points chosen from the spots */
static void bots_vantage_choose_points(void)
{
	long index, other;

	for (index = 0; index < bots_vantage.candidate_count; index++)
	{
		struct bots_vantage_spot *spot = &bots_vantage.candidates[index];
		long place;

		spot->score = 0.8f * PIN(spot->height, 0.0f, 6.0f) + 2.5f * (real)spot->open / (real)BOTS_VANTAGE_OPEN_RAYS;
		spot->sees_count = 0;
		for (place = 0; place < bots_vantage.place_count; place++)
		{
			if (TEST_FLAG(spot->sees, place))
			{
				spot->score += bots_vantage.places[place].weight *
					bots_vantage_range(bots_vantage_distance3d(&spot->point, &bots_vantage.places[place].point));
				spot->sees_count++;
			}
		}
	}
	/* (debug: HALO_BOT_TRACE, every spot scored) */
	for (index = 0; bots_trace_enabled() && index < bots_vantage.candidate_count; index++)
	{
		struct bots_vantage_spot const *spot = &bots_vantage.candidates[index];

		platform_log("bots: trace vantage spot (%.1f %.1f %.1f) height %.1f open %d sees %d (%lx) first %.1f score %.1f",
			spot->point.x, spot->point.y, spot->point.z, spot->height, (int)spot->open, (int)spot->sees_count, spot->sees,
			spot->first_score, spot->score);
	}
	for (index = 0; bots_trace_enabled() && index < bots_vantage.place_count; index++)
		platform_log("bots: trace vantage place %ld %s (%.1f %.1f %.1f) weight %.1f", index, bots_vantage.places[index].name,
			bots_vantage.places[index].point.x, bots_vantage.places[index].point.y, bots_vantage.places[index].point.z,
			bots_vantage.places[index].weight);
	bots_vantage.count = 0;
	while (bots_vantage.count < BOTS_VANTAGE_MAXIMUM)
	{
		long best = NONE;

		for (index = 0; index < bots_vantage.candidate_count; index++)
		{
			struct bots_vantage_spot const *spot = &bots_vantage.candidates[index];
			boolean near = FALSE;

			if (spot->sees_count < 2 || (best != NONE && spot->score <= bots_vantage.candidates[best].score))
				continue;
			for (other = 0; other < bots_vantage.count && !near; other++)
				near = bots_vantage_distance3d(&bots_vantage.points[other].point, &spot->point) < BOTS_VANTAGE_SPACING;
			if (!near)
				best = index;
		}
		if (best == NONE)
			break;
		bots_vantage.points[bots_vantage.count] = bots_vantage.candidates[best];
		bots_vantage.claims[bots_vantage.count] = NONE;
		bots_vantage.count++;
	}
	for (index = 0; index < bots_vantage.count; index++)
	{
		struct bots_vantage_spot const *spot = &bots_vantage.points[index];
		char seen[200];
		int length = 0;
		long place;

		seen[0] = 0;
		for (place = 0; place < bots_vantage.place_count && length < (int)sizeof(seen) - 24; place++)
		{
			if (TEST_FLAG(spot->sees, place))
				length += snprintf(seen + length, sizeof(seen) - (size_t)length, "%s%s %.0f", length ? ", " : "",
					bots_vantage.places[place].name, bots_vantage_distance3d(&spot->point, &bots_vantage.places[place].point));
		}
		platform_log("bots: vantage point %ld at (%.1f %.1f %.1f): %.1f above its surroundings, open %d/%d, sees %d of %ld places "
			"(%s); score %.1f", index + 1, spot->point.x, spot->point.y, spot->point.z, spot->height, (int)spot->open,
			BOTS_VANTAGE_OPEN_RAYS, (int)spot->sees_count, bots_vantage.place_count, seen, spot->score);
	}
	platform_log("bots: vantage: %ld points of %ld spots scored (%ld places, %ld rays)", bots_vantage.count,
		bots_vantage.candidate_count, bots_vantage.place_count, bots_vantage.rays);
}

/* a step of the map's vantage points; TRUE once made (or none) */
boolean bots_vantage_step(void)
{
	switch (bots_vantage.phase)
	{
	case _vantage_phase_idle:
		bots_vantage_begin();
		break;
	case _vantage_phase_bounds:
	{
		long last;

		if (!bots_routes_graph_made() || bots_vantage.place_count < 2)
		{
			platform_log("bots: vantage: none (%s)", bots_routes_graph_made() ? "fewer than two places" : "no walkable graph");
			bots_vantage.phase = _vantage_phase_done;
			break;
		}
		last = MIN(bots_vantage.cursor + BOTS_VANTAGE_NODES_A_TICK * 4, bots_graph.node_count);
		for (; bots_vantage.cursor < last; bots_vantage.cursor++)
		{
			struct bots_graph_node const *node = &bots_graph.nodes[bots_vantage.cursor];

			bots_vantage.grid_x = MIN(bots_vantage.grid_x, node->middle.x);
			bots_vantage.grid_y = MIN(bots_vantage.grid_y, node->middle.y);
			bots_vantage.grid_x1 = MAX(bots_vantage.grid_x1, node->middle.x);
			bots_vantage.grid_y1 = MAX(bots_vantage.grid_y1, node->middle.y);
		}
		if (bots_vantage.cursor >= bots_graph.node_count)
		{
			real width = bots_vantage.grid_x1 - bots_vantage.grid_x;
			real height = bots_vantage.grid_y1 - bots_vantage.grid_y;
			long cells;

			bots_vantage.cell = MAX(BOTS_VANTAGE_CELL, MAX(width, height) / (real)BOTS_VANTAGE_GRID_MAXIMUM);
			bots_vantage.grid_width = (long)(width / bots_vantage.cell) + 1;
			bots_vantage.grid_height = (long)(height / bots_vantage.cell) + 1;
			cells = bots_vantage.grid_width * bots_vantage.grid_height;
			bots_vantage.grid_area = (real *)malloc((size_t)cells * sizeof(real));
			bots_vantage.grid_height_sum = (real *)malloc((size_t)cells * sizeof(real));
			if (bots_vantage.grid_area)
				csmemset(bots_vantage.grid_area, 0, (size_t)cells * sizeof(real));
			if (bots_vantage.grid_height_sum)
				csmemset(bots_vantage.grid_height_sum, 0, (size_t)cells * sizeof(real));
			if (!bots_vantage.grid_area || !bots_vantage.grid_height_sum)
			{
				bots_vantage_free();
				platform_log("bots: vantage: none (no memory for the height grid)");
				bots_vantage.phase = _vantage_phase_done;
				break;
			}
			bots_vantage.cursor = 0;
			bots_vantage.phase = _vantage_phase_grid;
		}
		break;
	}
	case _vantage_phase_grid:
	{
		long last = MIN(bots_vantage.cursor + BOTS_VANTAGE_NODES_A_TICK * 4, bots_graph.node_count);

		for (; bots_vantage.cursor < last; bots_vantage.cursor++)
		{
			struct bots_graph_node const *node = &bots_graph.nodes[bots_vantage.cursor];
			long cell = bots_vantage_cell(node->middle.x, node->middle.y);

			if (cell == NONE || node->cost >= REAL_MAX)
				continue;
			bots_vantage.grid_area[cell] += node->area;
			bots_vantage.grid_height_sum[cell] += node->area * node->middle.z;
		}
		if (bots_vantage.cursor >= bots_graph.node_count)
		{
			bots_vantage.cursor = 0;
			bots_vantage.candidate_count = 0;
			bots_vantage.phase = _vantage_phase_candidates;
		}
		break;
	}
	case _vantage_phase_candidates:
	{
		long last = MIN(bots_vantage.cursor + BOTS_VANTAGE_NODES_A_TICK, bots_graph.node_count);

		for (; bots_vantage.cursor < last; bots_vantage.cursor++)
		{
			struct bots_graph_node const *node = &bots_graph.nodes[bots_vantage.cursor];
			struct bots_vantage_spot spot;
			long place;

			/* (reached from the bases, flat, room to stand) */
			if (node->cost >= REAL_MAX || node->normal_k < 0.8f || node->area < 0.25f)
				continue;
			csmemset(&spot, 0, sizeof(spot));
			spot.point = node->middle;
			spot.surface_index = node->surface_index;
			spot.height = bots_vantage_height(&node->middle);
			spot.first_score = PIN(spot.height, -2.0f, 8.0f);
			for (place = 0; place < bots_vantage.place_count; place++)
			{
				spot.first_score += 0.25f * bots_vantage.places[place].weight *
					bots_vantage_range(bots_vantage_distance3d(&node->middle, &bots_vantage.places[place].point));
			}
			bots_vantage_consider(&spot);
		}
		if (bots_vantage.cursor >= bots_graph.node_count)
		{
			bots_vantage_free();
			bots_vantage.cursor = 0;
			bots_vantage.phase = _vantage_phase_rays;
		}
		break;
	}
	case _vantage_phase_rays:
	{
		long rays_a_spot = bots_vantage.place_count + BOTS_VANTAGE_OPEN_RAYS;
		long total = rays_a_spot * bots_vantage.candidate_count;
		long rays;

		for (rays = 0; rays < BOTS_VANTAGE_RAYS_A_TICK && bots_vantage.cursor < total; bots_vantage.cursor++)
		{
			struct bots_vantage_spot *spot = &bots_vantage.candidates[bots_vantage.cursor / rays_a_spot];
			long ray = bots_vantage.cursor % rays_a_spot;
			real_point3d eye = spot->point, to;

			eye.z += BOTS_VANTAGE_EYE_HEIGHT;
			if (ray < bots_vantage.place_count)
			{
				to = bots_vantage.places[ray].point;
				if (bots_vantage_distance3d(&eye, &to) > BOTS_VANTAGE_SIGHT || bots_vantage_distance3d(&eye, &to) < 1.0f)
					continue;
				rays++;
				if (bots_vantage_clear(&eye, &to))
					SET_FLAG(spot->sees, ray, TRUE);
			}
			else
			{
				real angle = (real)(ray - bots_vantage.place_count) * 2.0f * 3.14159265f / (real)BOTS_VANTAGE_OPEN_RAYS;

				to = eye;
				to.x += (real)cos(angle) * BOTS_VANTAGE_OPEN_LENGTH;
				to.y += (real)sin(angle) * BOTS_VANTAGE_OPEN_LENGTH;
				rays++;
				if (bots_vantage_clear(&eye, &to))
					spot->open++;
			}
		}
		if (bots_vantage.cursor >= total)
		{
			bots_vantage_choose_points();
			bots_vantage.phase = _vantage_phase_done;
		}
		break;
	}
	case _vantage_phase_done:
		return TRUE;
	}
	return FALSE;
}

/* ---------- the bots at them */

short bots_vantage_choose(long bot_index, real_point3d const *position, real_point3d const *objective,
	real maximum_distance)
{
	struct bots_tactics_bot *state = &bots_tactics_bots[bot_index];
	long index, best = NONE, objective_place = NONE;
	real best_score = -REAL_MAX;
	long now = game_time_get();

	/* (over the hill: the place nearest it, which the point must see) */
	if (objective)
	{
		real nearest = 15.0f;

		for (index = 0; index < bots_vantage.place_count; index++)
		{
			real distance = bots_vantage_distance3d(&bots_vantage.places[index].point, objective);

			if (distance < nearest)
			{
				nearest = distance;
				objective_place = index;
			}
		}
		if (objective_place == NONE)
			return NONE;
	}
	for (index = 0; index < bots_vantage.count; index++)
	{
		struct bots_vantage_spot const *spot = &bots_vantage.points[index];
		real score;

		if ((bots_vantage.claims[index] != NONE && bots_vantage.claims[index] != bot_index) ||
			(TEST_FLAG(state->failed_vantages, index) && now < state->failed_until_tick) ||
			(index == state->last_vantage && bots_vantage.count > 1) ||
			(objective_place != NONE && (!TEST_FLAG(spot->sees, objective_place) ||
				bots_vantage_distance3d(&spot->point, &bots_vantage.places[objective_place].point) > 40.0f)) ||
			bots_vantage_distance3d(&spot->point, position) > maximum_distance)
		{
			continue;
		}
		score = spot->score - bots_vantage_distance3d(&spot->point, position) / 10.0f +
			(real)(bots_tactics_random(state) & 0xFF) / 128.0f;
		if (score > best_score)
		{
			best_score = score;
			best = index;
		}
	}
	if (best != NONE)
		bots_vantage.claims[best] = bot_index;
	return (short)best;
}

void bots_vantage_release(long bot_index)
{
	struct bots_tactics_bot *state = &bots_tactics_bots[bot_index];

	if (state->vantage >= 0 && state->vantage < BOTS_VANTAGE_MAXIMUM && bots_vantage.claims[state->vantage] == bot_index)
		bots_vantage.claims[state->vantage] = NONE;
	if (state->vantage != NONE)
		state->last_vantage = state->vantage;
	state->vantage = NONE;
	state->vantage_weapon = _bots_precision_none;
	state->hold_tick = 0;
}

void bots_vantage_point(short vantage, real_point3d *point, long *surface_index)
{
	*point = bots_vantage.points[vantage].point;
	*surface_index = bots_vantage.points[vantage].surface_index;
}

boolean bots_vantage_stand(long team, real_point3d *point)
{
	long index;

	for (index = 0; index < bots_vantage.place_count; index++)
	{
		if (!strcmp(bots_vantage.places[index].name, team ? "stand 1" : "stand 0"))
		{
			*point = bots_vantage.places[index].point;
			return team == 0 || team == 1;
		}
	}
	return FALSE;
}

unsigned long bots_vantage_sees(short vantage)
{
	return vantage >= 0 && vantage < bots_vantage.count ? bots_vantage.points[vantage].sees : 0;
}

long bots_vantage_count(void)
{
	return bots_vantage.count;
}

long bots_places_count(void)
{
	return bots_vantage.place_count;
}

struct bots_place const *bots_place_get(long index)
{
	return &bots_vantage.places[index];
}
