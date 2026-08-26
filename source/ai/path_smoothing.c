/*
PATH_SMOOTHING.C

symbols in this file:
00051190 0080:
	_code_00051190 (0000)
00051210 0150:
	_code_00051210 (0000)
00051360 0120:
	_code_00051360 (0000)
00051480 0220:
	_code_00051480 (0000)
000516A0 0380:
	_code_000516a0 (0000)
00051A20 0360:
	_path_smooth (0000)
0024EAB0 0046:
	??_C@_0EG@DGPBJLIB@TEST_FLAG?$CIcollision_surface?9?$DOfla@ (0000)
0024EAF8 0023:
	??_C@_0CD@IKIHDGOB@c?3?2halo?2SOURCE?2ai?2path_smoothing@ (0000)
0024EB20 006d:
	??_C@_0GN@DGMBNALI@collision_edge?9?$DOvertex_indices?$FL0@ (0000)
0024EB90 0024:
	??_C@_0CE@JGDDFPAJ@clockwise?$DN?$DNTRUE?5?$HM?$HM?5clockwise?$DN?$DNFA@ (0000)
0024EBB4 0012:
	??_C@_0BC@GBNEHDPD@steps_finish_path?$AA@ (0000)
0024EBC8 000f:
	??_C@_0P@LBJDKFJI@smoothed_steps?$AA@ (0000)
0024EBD8 0014:
	??_C@_0BE@FBKMNIEA@smoothed_step_count?$AA@ (0000)
0024EBEC 000a:
	??_C@_09LCGCKCI@raw_steps?$AA@ (0000)
0024EBF8 0013:
	??_C@_0BD@DLANHJEI@raw_step_count?5?$DO?50?$AA@ (0000)
*/

/* ---------- headers */

#include "cseries.h"

#include "ai/path.h"
#include "ai/path_structure_bsp.h"
#include "math/real_math.h"
#include "physics/breakable_surfaces.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "structures/structure_bsp_definitions.h"

/* ---------- constants */

enum
{
	_collision_surface_breakable_bit = 3,
	_pathfinding_surface_walkable_bit = 6,
	_pathfinding_surface_breakable_bit = 7,
	_path_test_pill_endpoint_near_wall_ok_bit = 0,
};

/* ---------- macros */

#define NORMALIZE_DIRECTION2D(vector, magnitude)                         \
	do                                                                    \
	{                                                                     \
		(magnitude) = (real)sqrt(                                          \
			(vector)->i*(vector)->i + (vector)->j*(vector)->j);              \
		if (!(_real_epsilon > fabs((magnitude) - 0.0f)))                   \
		{                                                                 \
			(vector)->i = (vector)->i*(1.0f / (magnitude));                  \
			(vector)->j = (vector)->j*(1.0f / (magnitude));                  \
		}                                                                 \
		else                                                              \
		{                                                                 \
			(magnitude) = 0.0f;                                             \
		}                                                                 \
	} while (FALSE)

/* ---------- structures */

/* ---------- prototypes */

boolean structure_test_pill2d(
	struct structure_bsp const *structure,
	boolean ignore_broken_surfaces,
	real const *p0,
	long p0_surface_index,
	real const *p1,
	long p1_surface_index,
	real radius,
	unsigned long flags,
	struct path_collision_result *result);

/* ---------- globals */

/* ---------- public code */

void path_smooth(
	struct path_state *state,
	short raw_step_count,
	struct path_step const *raw_steps,
	short *smoothed_step_count,
	struct path_step *smoothed_steps,
	boolean *steps_finish_path);

/* ---------- private code */

static boolean code_00051190(
	byte const *pathfinding_surfaces,
	struct collision_bsp const *bsp,
	byte const *breakable_surface_flags,
	long surface_index,
	boolean ignore_broken_surfaces)
{
	byte pathfinding_surface_flags;
	boolean walkable;
	struct collision_surface const *collision_surface;
	byte breakable_surface_index;

	pathfinding_surface_flags = pathfinding_surfaces[surface_index];
	walkable = (pathfinding_surface_flags >> _pathfinding_surface_walkable_bit) & 1;

	if (!ignore_broken_surfaces &&
		walkable &&
		TEST_FLAG(pathfinding_surface_flags, _pathfinding_surface_breakable_bit))
	{
		collision_surface = TAG_BLOCK_GET_ELEMENT(
			&bsp->surfaces,
			surface_index,
			struct collision_surface);
#line 482 "c:\\halo\\SOURCE\\ai\\path_smoothing.c"
		match_assert(__FILE__, __LINE__, TEST_FLAG(collision_surface->flags, _collision_surface_breakable_bit));
		breakable_surface_index = collision_surface->breakable_surface_index;
		walkable = BIT_VECTOR_TEST_FLAG(
			(long const *)breakable_surface_flags,
			breakable_surface_index);
	}

	return walkable;
}

static void code_00051210(
	real const *point,
	real const *center,
	real radius,
	boolean clockwise,
	real *tangent_point)
{
	real_vector2d center_to_point;
	real distance_squared;
	real tangent_length;
	real inverse_distance_squared;
	real magnitude;
	real_point2d tangent_points[2];
	boolean cross_positive;
	long tangent_point_index;

	center_to_point.i = point[0] - center[0];
	center_to_point.j = point[1] - center[1];
	distance_squared = center_to_point.i*center_to_point.i + center_to_point.j*center_to_point.j;
	inverse_distance_squared = radius / distance_squared;
	tangent_length = distance_squared - radius*radius;

	if (tangent_length > 0.0f)
	{
		tangent_length = (real)sqrt(tangent_length);

		tangent_points[0].x =
			(center_to_point.i*radius + center_to_point.j*tangent_length)*inverse_distance_squared + center[0];
		tangent_points[0].y =
			(center_to_point.j*radius - center_to_point.i*tangent_length)*inverse_distance_squared + center[1];
		tangent_points[1].x =
			(center_to_point.i*radius - center_to_point.j*tangent_length)*inverse_distance_squared + center[0];
		tangent_points[1].y =
			(center_to_point.i*tangent_length + center_to_point.j*radius)*inverse_distance_squared + center[1];

		cross_positive =
			(tangent_points[1].y - point[1])*(tangent_points[0].x - point[0]) -
			(tangent_points[1].x - point[0])*(tangent_points[0].y - point[1]) > 0.0f;
		tangent_point_index = cross_positive != clockwise;
		tangent_point[0] = tangent_points[tangent_point_index].x;
		tangent_point[1] = tangent_points[tangent_point_index].y;
	}
	else
	{
		NORMALIZE_DIRECTION2D(&center_to_point, magnitude);
		if (magnitude == 0.0f)
			center_to_point = *global_left2d;

		tangent_point[0] = center_to_point.i*radius + center[0];
		tangent_point[1] = center_to_point.j*radius + center[1];
	}

	return;
}

static void code_00051360(
	real const *tangent_points,
	real const *center,
	real const *start_point,
	real radius,
	real *avoidance_point)
{
	real dx0;
	real dy0;
	real dx1;
	real dy1;
	real cross;
	real scale;
	real magnitude;
	real_vector2d direction;

	dx0 = tangent_points[0] - center[0];
	dy0 = tangent_points[1] - center[1];
	dx1 = tangent_points[2] - center[0];
	dy1 = tangent_points[3] - center[1];
	cross = dy1*dx0 - dy0*dx1;

	if (!(fabs(cross) < _real_epsilon))
	{
		scale = radius*radius / cross;
		avoidance_point[1] = (tangent_points[0] - tangent_points[2])*scale + center[1];
		avoidance_point[0] = center[0] - (dy0 - dy1)*scale;

		direction.i = avoidance_point[0] - center[0];
		direction.j = avoidance_point[1] - center[1];
		if (direction.i*direction.i + direction.j*direction.j <= radius*radius*4.0f)
			return;
	}

	direction.i = tangent_points[0] - start_point[0];
	direction.j = tangent_points[1] - start_point[1];
	NORMALIZE_DIRECTION2D(&direction, magnitude);
	if (magnitude == 0.0f)
		direction = *global_left2d;

	avoidance_point[0] = direction.i*radius + tangent_points[0];
	avoidance_point[1] = direction.j*radius + tangent_points[1];

	return;
}

static boolean code_00051480(
	real const *start_point,
	real const *clockwise_turning_point,
	real const *counterclockwise_turning_point,
	real const *unobstructed_path_point,
	real const *obstructed_path_point,
	real *result)
{
	real_vector2d start_to_clockwise;
	real_vector2d clockwise_to_unobstructed;
	real_vector2d clockwise_to_obstructed;
	real_vector2d start_to_counterclockwise;
	real_vector2d counterclockwise_to_unobstructed;
	real_vector2d counterclockwise_to_obstructed;
	real clockwise_turn;
	real counterclockwise_turn;
	real magnitude;

	start_to_clockwise.i = start_point[0] - clockwise_turning_point[0];
	start_to_clockwise.j = start_point[1] - clockwise_turning_point[1];
	NORMALIZE_DIRECTION2D(&start_to_clockwise, magnitude);

	clockwise_to_unobstructed.i = unobstructed_path_point[0] - clockwise_turning_point[0];
	clockwise_to_unobstructed.j = unobstructed_path_point[1] - clockwise_turning_point[1];
	NORMALIZE_DIRECTION2D(&clockwise_to_unobstructed, magnitude);

	clockwise_to_obstructed.i = obstructed_path_point[0] - clockwise_turning_point[0];
	clockwise_to_obstructed.j = obstructed_path_point[1] - clockwise_turning_point[1];
	NORMALIZE_DIRECTION2D(&clockwise_to_obstructed, magnitude);

	start_to_counterclockwise.i = start_point[0] - counterclockwise_turning_point[0];
	start_to_counterclockwise.j = start_point[1] - counterclockwise_turning_point[1];
	NORMALIZE_DIRECTION2D(&start_to_counterclockwise, magnitude);

	counterclockwise_to_unobstructed.i = unobstructed_path_point[0] - counterclockwise_turning_point[0];
	counterclockwise_to_unobstructed.j = unobstructed_path_point[1] - counterclockwise_turning_point[1];
	NORMALIZE_DIRECTION2D(&counterclockwise_to_unobstructed, magnitude);

	counterclockwise_to_obstructed.i = obstructed_path_point[0] - counterclockwise_turning_point[0];
	counterclockwise_to_obstructed.j = obstructed_path_point[1] - counterclockwise_turning_point[1];
	NORMALIZE_DIRECTION2D(&counterclockwise_to_obstructed, magnitude);

	clockwise_turn =
		signed_angle_between_vectors2d(&clockwise_to_unobstructed, &clockwise_to_obstructed) +
		signed_angle_between_vectors2d(&start_to_clockwise, &clockwise_to_unobstructed);
	counterclockwise_turn =
		signed_angle_between_vectors2d(&counterclockwise_to_unobstructed, &counterclockwise_to_obstructed);

	if (clockwise_turn <=
		-(counterclockwise_turn +
			signed_angle_between_vectors2d(
				&start_to_counterclockwise,
				&counterclockwise_to_unobstructed)))
	{
		result[0] = counterclockwise_turning_point[0];
		result[1] = counterclockwise_turning_point[1];
		return FALSE;
	}

	result[0] = clockwise_turning_point[0];
	result[1] = clockwise_turning_point[1];

	return TRUE;
}

static boolean code_000516a0(
	struct structure_bsp const *structure,
	real const *point,
	real radius,
	long first_edge_index,
	boolean clockwise,
	boolean ignore_broken_surfaces,
	real *result)
{
	struct collision_bsp const *bsp;
	byte const *pathfinding_surfaces;
	byte const *breakable_surface_flags;
	struct collision_edge const *current_edge;
	struct collision_vertex const *vertex_a;
	struct collision_vertex const *vertex_b;
	real edge_dx;
	real edge_dy;
	real magnitude;
	real_vector2d edge_direction;
	real_point2d positive_point;
	real_point2d negative_point;
	boolean side_flag;
	boolean valid;
	boolean side_test;
	boolean xor_flag;
	boolean matches_end;
	boolean refined_side;
	long starting_vertex_index;
	long loop_reference_vertex_index;
	long vertex_a_index;
	long vertex_b_index;
	long next_vertex_index;
	long chain_start_edge_index;
	long candidate_surface_index;
	long next_edge_index;

	bsp = TAG_BLOCK_GET_ELEMENT(
		&structure->collision_bsp,
		0,
		struct collision_bsp);
	pathfinding_surfaces = structure->pathfinding_surfaces.address;
	breakable_surface_flags = breakable_surface_flags_get();
#line 511 "c:\\halo\\SOURCE\\ai\\path_smoothing.c"
	assert(clockwise==TRUE || clockwise==FALSE);

	starting_vertex_index = NONE;
	loop_reference_vertex_index = NONE;
	current_edge = TAG_BLOCK_GET_ELEMENT(
		&bsp->edges,
		first_edge_index,
		struct collision_edge);

	while (TRUE)
	{
		side_flag = code_00051190(
			pathfinding_surfaces,
			bsp,
			breakable_surface_flags,
			current_edge->surface_indices[0],
			ignore_broken_surfaces);

		vertex_a_index = current_edge->vertex_indices[side_flag];
		vertex_b_index = current_edge->vertex_indices[!side_flag];
		vertex_a = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			vertex_a_index,
			struct collision_vertex);
		vertex_b = TAG_BLOCK_GET_ELEMENT(
			&bsp->vertices,
			vertex_b_index,
			struct collision_vertex);

		edge_dx = vertex_b->point.x - vertex_a->point.x;
		edge_dy = vertex_b->point.y - vertex_a->point.y;
		edge_direction.i = -edge_dx;
		edge_direction.j = edge_dy;
		NORMALIZE_DIRECTION2D(&edge_direction, magnitude);

		positive_point.x = point[0] + edge_direction.j*radius;
		positive_point.y = point[1] + edge_direction.i*radius;
		negative_point.x = point[0] - edge_direction.j*radius;
		negative_point.y = point[1] - edge_direction.i*radius;

		valid = FALSE;
		side_test =
			(vertex_a->point.y - positive_point.y)*edge_dy +
			(vertex_a->point.x - positive_point.x)*edge_dx < 0.0f;
		if (side_test == clockwise)
		{
			if ((vertex_a->point.x - positive_point.x)*edge_dy -
				(vertex_a->point.y - positive_point.y)*edge_dx < 0.0f)
			{
				valid = TRUE;
			}
		}

		if ((vertex_a->point.x - negative_point.x)*edge_dy -
			(vertex_a->point.y - negative_point.y)*edge_dx < 0.0f)
		{
			valid = TRUE;
		}

		if (starting_vertex_index == NONE)
			valid = TRUE;

		xor_flag = side_flag != valid;
		next_vertex_index =
			clockwise == xor_flag ?
			current_edge->vertex_indices[1] :
			current_edge->vertex_indices[0];

		if (next_vertex_index == loop_reference_vertex_index)
		{
			vertex_a = TAG_BLOCK_GET_ELEMENT(
				&bsp->vertices,
				next_vertex_index,
				struct collision_vertex);
			result[0] = vertex_a->point.x;
			result[1] = vertex_a->point.y;
			return TRUE;
		}

		if (next_vertex_index == starting_vertex_index)
			return FALSE;

		if (starting_vertex_index == NONE)
			starting_vertex_index = next_vertex_index;

		chain_start_edge_index = first_edge_index;
		while (TRUE)
		{
			matches_end = next_vertex_index == current_edge->vertex_indices[1];
			candidate_surface_index = current_edge->surface_indices[!matches_end];
			refined_side = code_00051190(
				pathfinding_surfaces,
				bsp,
				breakable_surface_flags,
				candidate_surface_index,
				ignore_broken_surfaces);

			if (refined_side == clockwise)
				break;

			next_edge_index = current_edge->edge_indices[!matches_end];
			if (next_edge_index == chain_start_edge_index)
				return FALSE;

			current_edge = TAG_BLOCK_GET_ELEMENT(
				&bsp->edges,
				next_edge_index,
				struct collision_edge);
#line 631 "c:\\halo\\SOURCE\\ai\\path_smoothing.c"
			assert(current_edge->vertex_indices[0] == next_vertex_index || current_edge->vertex_indices[1] == next_vertex_index);
		}

		loop_reference_vertex_index = next_vertex_index;
	}

	return FALSE;
}

#line 27 "c:\\halo\\SOURCE\\ai\\path_smoothing.c"
void path_smooth(
	struct path_state *state,
	short raw_step_count,
	struct path_step const *raw_steps,
	short *smoothed_step_count,
	struct path_step *smoothed_steps,
	boolean *steps_finish_path)
{
	real_point2d current_position;
	long current_surface_index;
	short smoothed_count;
	short step_index;
	short collision_step_index;
	long collision_edge_index;
	boolean collision_active;
	boolean found_clockwise;
	boolean found_counterclockwise;
	boolean chose_clockwise;
	boolean smoothed_path_finishes;
	short i;
	struct path_collision_result collision_result;
	struct path_step const *collision_step;
	real_point2d clockwise_turning_point;
	real_point2d counterclockwise_turning_point;
	real_point2d chosen_center;
	real_point2d tangent_points[2];
	real_point2d avoidance_point;
	real_point2d known_point;

#line 33 "c:\\halo\\SOURCE\\ai\\path_smoothing.c"
	assert(raw_step_count > 0);
	assert(raw_steps);
	assert(smoothed_step_count);
	assert(smoothed_steps);
	assert(steps_finish_path);

	if (raw_step_count <= 1)
	{
		*smoothed_step_count = 1;
		*smoothed_steps = *raw_steps;
		return;
	}

	current_position.x = state->input.start_point.x;
	current_position.y = state->input.start_point.y;
	current_surface_index = state->input.start_surface_index;
	smoothed_count = 0;
	step_index = 1;
	smoothed_path_finishes = FALSE;

	while (TRUE)
	{
		collision_step_index = NONE;
		collision_edge_index = NONE;
		collision_active = FALSE;

		for (i = step_index; i < raw_step_count; i++)
		{
			if (structure_test_pill2d(
				state->structure,
				state->input.ignore_broken_surfaces,
				current_position.n,
				current_surface_index,
				raw_steps[i].point.n,
				raw_steps[i].surface_index,
				0.3f,
				FLAG(_path_test_pill_endpoint_near_wall_ok_bit),
				&collision_result))
			{
				if (!collision_active)
				{
					collision_edge_index = collision_result.edge_index;
					collision_step_index = i;
					collision_active = TRUE;
				}
			}
			else if (collision_active)
			{
				collision_step_index = NONE;
				collision_edge_index = NONE;
				collision_active = FALSE;
			}
		}

		if (!collision_active || collision_edge_index == NONE)
			break;

		found_clockwise = code_000516a0(
			state->structure,
			current_position.n,
			0.3f,
			collision_edge_index,
			TRUE,
			state->input.ignore_broken_surfaces,
			clockwise_turning_point.n);
		found_counterclockwise = code_000516a0(
			state->structure,
			current_position.n,
			0.3f,
			collision_edge_index,
			FALSE,
			state->input.ignore_broken_surfaces,
			counterclockwise_turning_point.n);
		if (!found_clockwise || !found_counterclockwise)
			goto bail_out;

		collision_step = &raw_steps[collision_step_index];
		chose_clockwise = code_00051480(
			current_position.n,
			clockwise_turning_point.n,
			counterclockwise_turning_point.n,
			collision_step[-1].point.n,
			collision_step->point.n,
			chosen_center.n);

		code_00051210(
			current_position.n,
			chosen_center.n,
			0.35f,
			chose_clockwise,
			tangent_points[0].n);
		code_00051210(
			collision_step->point.n,
			chosen_center.n,
			0.35f,
			!chose_clockwise,
			tangent_points[1].n);
		code_00051360(
			tangent_points[0].n,
			chosen_center.n,
			current_position.n,
			0.35f,
			avoidance_point.n);

		known_point = current_position;
		current_position = avoidance_point;
		current_surface_index = structure_surface_index_from_point(
			state->structure,
			state->input.ignore_broken_surfaces,
			&known_point,
			current_surface_index,
			&current_position);

		collision_surface_project_point2d(
			TAG_BLOCK_GET_ELEMENT(
				&state->structure->collision_bsp,
				0,
				struct collision_bsp),
			current_surface_index,
			_z,
			TRUE,
			&current_position,
			&smoothed_steps[smoothed_count].point);
		smoothed_steps[smoothed_count].surface_index = current_surface_index;
		smoothed_count++;

		if (smoothed_count >= 4)
			goto bail_out;

		step_index = collision_step_index;
	}

	smoothed_steps[smoothed_count] = raw_steps[raw_step_count - 1];
	smoothed_count++;
	smoothed_path_finishes = TRUE;

bail_out:
	*smoothed_step_count = smoothed_count;
	if (!smoothed_path_finishes)
		*steps_finish_path = FALSE;

	return;
}
