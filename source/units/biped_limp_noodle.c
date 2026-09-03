/*
BIPED_LIMP_NOODLE.C

symbols in this file:
0018ED80 0010:
	_biped_limp_noodle_get_max_relaxation_iterations (0000)
0018ED90 04e0:
	_code_0018ed90 (0000)
0018F270 07b0:
	_code_0018f270 (0000)
0018FA20 01f0:
	_validate_real_vector3d_axes3 (0000)
0018FC10 02c0:
	_code_0018fc10 (0000)
0018FED0 0100:
	_biped_limp_noodle_relax_nodes_onto_environment (0000)
002A3030 0045:
	??_C@_0EF@HHBJMIJE@realcmp?$CIplane3d_distance_to_poin@ (0000)
002A3078 0029:
	??_C@_0CJ@FPDJGKOB@c?3?2halo?2SOURCE?2units?2biped_limp_@ (0000)
002A30A4 0004:
	__real@3d8f5c29 (0000)
002A30A8 0004:
	__real@3f83d70a (0000)
002A30AC 0004:
	__real@bd036d41 (0000)
004C1C08 af08:
	_bss_004c1c08 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "physics/collision_features.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "units/biped_definitions.h"
#include "units/biped_limp_noodle.h"
#include "units/bipeds.h"

/* ---------- constants */

enum
{
	_joint_type_ball_and_socket = 0,
	_joint_type_hinge,
	_joint_type_no_movement,
	NUMBER_OF_JOINT_TYPES,
};

/* ---------- macros */

#define realcmp(a, b) (fabs((a) - (b)) < _real_epsilon)

/* ---------- structures */

struct animation_graph_node
{
	char name[TAG_STRING_LENGTH + 1];
	short next_sibling_node_index;
	short first_child_node_index;
	short parent_node_index;
	word pad;
	unsigned long flags;
	real_vector3d base_vector;
	real range;
	long pad1;
};

struct biped_limp_noodle_globals
{
	struct collision_feature_list features;
	real_point3d last_positions[MAXIMUM_NODES_PER_ANIMATION];
};

/* ---------- prototypes */

static boolean biped_limp_noodle_valid_joint_rotation(
	long biped_index,
	long node_index,
	real_matrix4x3 *node_matrices,
	real_point3d *current_world_position,
	real_point3d const *new_world_position,
	unsigned long *moved_node_flags);
static void biped_limp_noodle_move_relax_and_constrain_positions(
	long biped_index,
	real_matrix4x3 *node_matrices);
static void biped_limp_noodle_adjust_orientations(
	long biped_index,
	real_matrix4x3 *node_matrices,
	long node_count,
	real_point3d const *last_positions);
/* ---------- globals */

static struct biped_limp_noodle_globals biped_limp_noodle_globals;

/* ---------- public code */

long biped_limp_noodle_get_max_relaxation_iterations(
	long biped_index)
{
	return 20;
}

/* ---------- private code */

static boolean biped_limp_noodle_valid_joint_rotation(
	long biped_index,
	long node_index,
	real_matrix4x3 *node_matrices,
	real_point3d *current_world_position,
	real_point3d const *new_world_position,
	unsigned long *moved_node_flags)
{
	boolean moved = FALSE;
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct animation_graph *animation_graph = animation_graph_definition_get(
		definition->object.animation_graph.index);
	struct tag_block *nodes = &animation_graph->nodes;
	struct animation_graph_node *node = TAG_BLOCK_GET_ELEMENT(
		nodes,
		node_index,
		struct animation_graph_node);
	struct animation_graph_node *parent_node = TAG_BLOCK_GET_ELEMENT(
		nodes,
		node->parent_node_index,
		struct animation_graph_node);
	real collision_radius = animation_graph->limp_body_node_collision_radius;

	if (fabs(collision_radius) < _real_epsilon ||
		collision_radius < 0.f ||
		collision_radius > 0.07f)
	{
		collision_radius = 0.03f;
	}

	if (node->parent_node_index &&
		!TEST_FLAG(parent_node->flags, _joint_type_no_movement))
	{
		real_matrix4x3 *parent_matrix = &node_matrices[node->parent_node_index];
		real_vector3d current_direction;
		real_vector3d desired_direction;
		real_vector3d rotation_axis;
		real rotation_cosine;

		vector_from_points3d(
			&parent_matrix->position,
			&node_matrices[node_index].position,
			&current_direction);
		vector_from_points3d(
			&parent_matrix->position,
			new_world_position,
			&desired_direction);
		normalize3d(&current_direction);
		normalize3d(&desired_direction);
		cross_product3d(&current_direction, &desired_direction, &rotation_axis);
		normalize3d(&rotation_axis);
		rotation_cosine = dot_product3d(&current_direction, &desired_direction);

		if (fabs(rotation_cosine - 1.f) >= _real_epsilon)
		{
			real rotation_angle = arccosine(rotation_cosine);
			real_matrix4x3 parent_inverse;
			real_matrix4x3 grandparent_inverse;
			real_vector3d local_rotation_axis;
			real_vector3d parent_axis;
			real_vector3d *constraint_axis = &parent_node->base_vector;

			matrix4x3_inverse(parent_matrix, &parent_inverse);
			matrix4x3_inverse(
				&node_matrices[parent_node->parent_node_index],
				&grandparent_inverse);
			matrix4x3_transform_vector(
				&parent_inverse,
				&rotation_axis,
				&local_rotation_axis);
			parent_axis = parent_matrix->forward;
			matrix4x3_transform_vector(
				&grandparent_inverse,
				&parent_axis,
				&parent_axis);

			if (TEST_FLAG(parent_node->flags, _joint_type_hinge))
			{
				real_plane3d plane;
				real_vector3d plane_normal = parent_matrix->up;
				real_point3d rotate_to_position;
				real_vector3d rotate_to_direction;
				real_vector3d local_rotate_to_direction;
				real constraint_cosine;
				real projection_distance;

				plane3d_from_point_and_normal(
					&plane,
					&parent_matrix->position,
					&plane_normal);
				projection_distance =
					plane3d_distance_to_point(&plane, new_world_position) * -1.f;
				rotate_to_position.x =
					plane.n.i * projection_distance + new_world_position->x;
				rotate_to_position.y =
					plane.n.j * projection_distance + new_world_position->y;
				rotate_to_position.z =
					plane.n.k * projection_distance + new_world_position->z;
				match_assert(
					"c:\\halo\\SOURCE\\units\\biped_limp_noodle.c",
					231,
					realcmp(plane3d_distance_to_point(&plane, &rotate_to_position), 0.f));

				vector_from_points3d(
					&parent_matrix->position,
					&rotate_to_position,
					&rotate_to_direction);
				normalize3d(&rotate_to_direction);
				matrix4x3_transform_vector(
					&parent_inverse,
					&rotate_to_direction,
					&local_rotate_to_direction);
				constraint_cosine = dot_product3d(
					constraint_axis,
					&local_rotate_to_direction);

				if (fabs(constraint_cosine - 1.f) < _real_epsilon)
					goto follow_parent;

				BIT_VECTOR_SET_FLAG(moved_node_flags, node_index, TRUE);
				if (current_world_position->z >= rotate_to_position.z &&
					!collision_test_sphere(
						&rotate_to_position,
						collision_radius,
						biped_index))
				{
					*current_world_position = rotate_to_position;
				}
			}
			else
			{
				real constraint_cosine;
				real constraint_angle;

				rotate_vector_about_axis(
					&parent_axis,
					&local_rotation_axis,
					sine(rotation_angle),
					rotation_cosine);
				constraint_cosine = dot_product3d(constraint_axis, &parent_axis);
				if (fabs(constraint_cosine - 1.f) < _real_epsilon)
					goto follow_parent;

				constraint_angle = arccosine(constraint_cosine);
				if (fabs(constraint_angle) >= parent_node->range ||
					current_world_position->z <= new_world_position->z)
				{
					goto follow_parent;
				}

				BIT_VECTOR_SET_FLAG(moved_node_flags, node_index, TRUE);
				*current_world_position = *new_world_position;
			}

			moved = TRUE;
		}
	}

follow_parent:
	if ((!TEST_FLAG(parent_node->flags, _joint_type_no_movement) && moved) ||
		!BIT_VECTOR_TEST_FLAG(moved_node_flags, node->parent_node_index) ||
		current_world_position->z <= new_world_position->z)
	{
		return moved;
	}

	BIT_VECTOR_SET_FLAG(moved_node_flags, node_index, TRUE);
	*current_world_position = *new_world_position;

	return TRUE;
}

static void biped_limp_noodle_move_relax_and_constrain_positions(
	long biped_index,
	real_matrix4x3 *node_matrices)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct animation_graph *animation_graph = animation_graph_definition_get(
		definition->object.animation_graph.index);
	real collision_radius = animation_graph->limp_body_node_collision_radius;
	byte total_iterations;
	byte current_iteration;
	real relaxation_fraction;
	unsigned long moved_node_flags[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_NODES_PER_ANIMATION)];
	struct collision_plane collision_planes[10];
	long iteration;

	match_assert(
		"c:\\halo\\SOURCE\\units\\biped_limp_noodle.c",
		319,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] =
		_collision_user_limp_body_physics;

	if (fabs(collision_radius) < _real_epsilon ||
		collision_radius < 0.f ||
		collision_radius > 0.07f)
	{
		collision_radius = 0.03f;
	}

	total_iterations = biped->biped.limp_body_max_relaxation_iterations;
	if (total_iterations <= 0 || total_iterations >= 30)
		return;

	current_iteration = biped->biped.limp_body_current_relaxation_iterations;
	relaxation_fraction =
		(real)(current_iteration + 1) /
		(real)total_iterations;
	if (fabs(relaxation_fraction) < _real_epsilon)
		return;
	if (current_iteration >= total_iterations)
		goto collision_user_end;

	collision_get_features_in_sphere(
		_collision_test_for_bipeds_dead_flags |
			FLAG(_collision_test_ignore_invisible_surfaces_bit),
		&biped->object.position,
		biped->object.bounding_sphere_radius + 0.0625f,
		0.f,
		collision_radius,
		biped_index,
		&biped_limp_noodle_globals.features);
	csmemset(moved_node_flags, 0, sizeof(moved_node_flags));

	for (iteration = 0; iteration < 4; iteration++)
	{
		short node_queue[MAXIMUM_NODES_PER_ANIMATION];
		short queue_head = 0;
		short queue_tail = 1;

		node_queue[0] = 0;
		do
		{
			short node_index = node_queue[queue_head++];
			struct animation_graph_node *node = TAG_BLOCK_GET_ELEMENT(
				&animation_graph->nodes,
				node_index,
				struct animation_graph_node);

			if (node_index)
			{
				real_point3d *position = &node_matrices[node_index].position;
				real_point3d *parent_position =
					&node_matrices[node->parent_node_index].position;
				real_vector3d velocity;
				real_vector3d segment;

				set_real_vector3d(
					&velocity,
					0.f,
					0.f,
					relaxation_fraction * -0.032086615f);
				vector_from_points3d(parent_position, position, &segment);

				if (!iteration &&
					!collision_test_sphere(
						position,
						collision_radius,
						biped_index))
				{
					real_point3d moved_position;
					real_vector3d moved_velocity;

					collision_move_point(
						position,
						&velocity,
						&biped_limp_noodle_globals.features,
						&moved_position,
						&moved_velocity,
						3,
						collision_planes);
					if (fabs(moved_velocity.i) < _real_epsilon &&
						fabs(moved_velocity.j) < _real_epsilon)
					{
						biped_limp_noodle_valid_joint_rotation(
							biped_index,
							node_index,
							node_matrices,
							position,
							&moved_position,
							moved_node_flags);
					}
				}

				{
					real_point3d ray_origin;
					struct collision_result collision;

					vector_from_points3d(parent_position, position, &segment);
					ray_origin.x = segment.i * -0.015f + parent_position->x;
					ray_origin.y = segment.j * -0.015f + parent_position->y;
					ray_origin.z = segment.k * -0.015f + parent_position->z;
					scale_vector3d(&segment, 1.03f, &segment);

					if (collision_test_vector(
						_collision_test_for_bipeds_dead_flags |
							FLAG(_collision_test_ignore_invisible_surfaces_bit),
						&ray_origin,
						&segment,
						biped_index,
						&collision))
					{
						boolean embedded[2];
						real push_distance[2];
						long endpoint_index;

						embedded[0] = collision_test_sphere(
							position,
							0.03f,
							biped_index);
						embedded[1] = collision_test_sphere(
							parent_position,
							0.03f,
							biped_index);

						if (embedded[0] || embedded[1])
						{
							if (embedded[0] + embedded[1] == 2)
							{
								for (endpoint_index = 0; endpoint_index < 2; endpoint_index++)
								{
									real_point3d *endpoint =
										endpoint_index ? parent_position : position;

									push_distance[endpoint_index] =
										-plane3d_distance_to_point(
											&collision.plane,
											endpoint) /
										dot_product3d(
											&collision.plane.n,
											&collision.plane.n);
									if (push_distance[endpoint_index] != 0.f)
									{
										push_distance[endpoint_index] +=
											collision_radius * 2.5f;
									}
								}
							}
							else
							{
								for (endpoint_index = 0; endpoint_index < 2; endpoint_index++)
								{
									if (embedded[endpoint_index])
									{
										real_point3d *endpoint =
											endpoint_index ? parent_position : position;

										push_distance[endpoint_index] =
											-plane3d_distance_to_point(
												&collision.plane,
												endpoint) /
											dot_product3d(
												&collision.plane.n,
												&collision.plane.n);
										if (push_distance[endpoint_index] != 0.f)
										{
											push_distance[endpoint_index] +=
												collision_radius * 2.5f;
										}
									}
									else
									{
										push_distance[endpoint_index] = 0.f;
									}
								}
							}

							for (endpoint_index = 0; endpoint_index < 2; endpoint_index++)
							{
								if (!(_real_epsilon > fabs(push_distance[endpoint_index])))
								{
									real_point3d *endpoint =
										endpoint_index ? parent_position : position;
									real push =
										push_distance[endpoint_index] * relaxation_fraction;

									endpoint->x = collision.plane.n.i * push + endpoint->x;
									endpoint->y = collision.plane.n.j * push + endpoint->y;
									endpoint->z = collision.plane.n.k * push + endpoint->z;
								}
							}
						}
					}
				}

				vector_from_points3d(parent_position, position, &segment);
				{
					struct model *model = model_definition_get(
						definition->object.model.index);
					struct model_node *model_node = TAG_BLOCK_GET_ELEMENT(
						&model->nodes,
						node_index,
						struct model_node);
					real rest_distance = model_node->node_distance_from_parent;
					real distance = distance3d(position, parent_position);

					if (rest_distance > 0.f &&
						rest_distance <= 10.f &&
						distance >= 0.f &&
						distance < 20.f &&
						fabs(distance) >= _real_epsilon &&
						fabs(rest_distance) >= _real_epsilon &&
						rest_distance != distance &&
						fabs(distance) >= _real_epsilon)
					{
						real correction = (rest_distance - distance) / distance;

						if (node->parent_node_index)
						{
							real half_correction = correction * 0.5f;

							scale_vector3d(&segment, -half_correction, &velocity);
							collision_move_point(
								parent_position,
								&velocity,
								&biped_limp_noodle_globals.features,
								parent_position,
								&velocity,
								3,
								collision_planes);
							scale_vector3d(&segment, half_correction, &velocity);
							collision_move_point(
								position,
								&velocity,
								&biped_limp_noodle_globals.features,
								position,
								&velocity,
								3,
								collision_planes);
						}
						else
						{
							scale_vector3d(&segment, correction, &velocity);
							if (collision_test_sphere(
								position,
								collision_radius,
								biped_index))
							{
								goto next_node;
							}
							collision_move_point(
								position,
								&velocity,
								&biped_limp_noodle_globals.features,
								position,
								&velocity,
								3,
								collision_planes);
						}
					}
				}
			}

next_node:
			if (node->next_sibling_node_index != NONE)
				node_queue[queue_tail++] = node->next_sibling_node_index;
			if (node->first_child_node_index != NONE)
				node_queue[queue_tail++] = node->first_child_node_index;
		}
		while (queue_head != queue_tail);
	}

collision_user_end:
	match_assert(
		"c:\\halo\\SOURCE\\units\\biped_limp_noodle.c",
		530,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return;
}

void validate_real_vector3d_axes3(
	real_vector3d *forward,
	real_vector3d *left,
	real_vector3d *up)
{
	match_assert("c:\\halo\\SOURCE\\units\\biped_limp_noodle.c", 535, forward);
	match_assert("c:\\halo\\SOURCE\\units\\biped_limp_noodle.c", 535, left);
	match_assert("c:\\halo\\SOURCE\\units\\biped_limp_noodle.c", 535, up);

	if (normalize3d(forward) == 0.f)
		*forward = *global_forward3d;
	if (normalize3d(up) == 0.f)
		*up = *global_up3d;

	cross_product3d(up, forward, left);
	if (normalize3d(left) == 0.f)
		*left = *global_left3d;

	cross_product3d(forward, left, up);
	if (normalize3d(up) == 0.f)
		*up = *global_up3d;

	cross_product3d(up, forward, left);
	if (normalize3d(left) == 0.f)
		*left = *global_left3d;

	return;
}

static void biped_limp_noodle_adjust_orientations(
	long biped_index,
	real_matrix4x3 *node_matrices,
	long node_count,
	real_point3d const *last_positions)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct animation_graph *animation_graph = animation_graph_definition_get(
		definition->object.animation_graph.index);
	long node_index;

	for (node_index = 0; node_index < animation_graph->nodes.count; node_index++)
	{
		struct animation_graph_node *node;
		struct animation_graph_node *parent_node;
		real_vector3d last_direction;
		real_vector3d current_direction;
		real_vector3d rotation_axis;
		real rotation_cosine;
		real rotation_angle;

		if (!node_index)
			continue;

		node = TAG_BLOCK_GET_ELEMENT(
			&animation_graph->nodes,
			node_index,
			struct animation_graph_node);
		parent_node = TAG_BLOCK_GET_ELEMENT(
			&animation_graph->nodes,
			node->parent_node_index,
			struct animation_graph_node);
		if (TEST_FLAG(parent_node->flags, _joint_type_no_movement))
			continue;

		vector_from_points3d(
			&last_positions[node->parent_node_index],
			&last_positions[node_index],
			&last_direction);
		vector_from_points3d(
			&node_matrices[node->parent_node_index].position,
			&node_matrices[node_index].position,
			&current_direction);
		normalize3d(&last_direction);
		normalize3d(&current_direction);
		cross_product3d(&last_direction, &current_direction, &rotation_axis);
		normalize3d(&rotation_axis);
		rotation_cosine = dot_product3d(&current_direction, &last_direction);
		if (fabs(rotation_cosine - 1.f) < _real_epsilon)
			continue;

		rotation_angle = arccosine(rotation_cosine);
		if (fabs(rotation_angle) < _real_epsilon ||
			!(fabs(rotation_angle) < 0.78539819f))
		{
			continue;
		}

		if (!valid_real_vector3d_axes3(
			&node_matrices[node->parent_node_index].forward,
			&node_matrices[node->parent_node_index].left,
			&node_matrices[node->parent_node_index].up))
		{
			validate_real_vector3d_axes3(
				&node_matrices[node->parent_node_index].forward,
				&node_matrices[node->parent_node_index].left,
				&node_matrices[node->parent_node_index].up);
		}

		rotate_vector_about_axis(
			&node_matrices[node->parent_node_index].forward,
			&rotation_axis,
			sine(rotation_angle),
			rotation_cosine);
		rotate_vector_about_axis(
			&node_matrices[node->parent_node_index].up,
			&rotation_axis,
			sine(rotation_angle),
			rotation_cosine);
		normalize3d(&node_matrices[node->parent_node_index].forward);
		normalize3d(&node_matrices[node->parent_node_index].up);
		cross_product3d(
			&node_matrices[node->parent_node_index].up,
			&node_matrices[node->parent_node_index].forward,
			&node_matrices[node->parent_node_index].left);
		normalize3d(&node_matrices[node->parent_node_index].left);

		if (!valid_real_vector3d_axes3(
			&node_matrices[node->parent_node_index].forward,
			&node_matrices[node->parent_node_index].left,
			&node_matrices[node->parent_node_index].up))
		{
			validate_real_vector3d_axes3(
				&node_matrices[node->parent_node_index].forward,
				&node_matrices[node->parent_node_index].left,
				&node_matrices[node->parent_node_index].up);
		}
	}

	return;
}

boolean biped_limp_noodle_relax_nodes_onto_environment(
	long biped_index)
{
	struct biped_datum *biped = biped_get(biped_index);
	struct biped_definition *definition = biped_definition_get(biped->definition_index);
	struct animation_graph *animation_graph = animation_graph_definition_get(
		definition->object.animation_graph.index);
	real_matrix4x3 *node_matrices = (real_matrix4x3 *)object_header_block_get(
		biped_index,
		&object_get(biped_index)->object.node_matrices);
	boolean relaxation_complete =
		biped->biped.limp_body_current_relaxation_iterations >=
		biped->biped.limp_body_max_relaxation_iterations;

	if (!relaxation_complete)
	{
		long node_index;

		for (node_index = 0; node_index < animation_graph->nodes.count; node_index++)
		{
			biped_limp_noodle_globals.last_positions[node_index] =
				node_matrices[node_index].position;
		}

		biped_limp_noodle_move_relax_and_constrain_positions(biped_index, node_matrices);
		biped_limp_noodle_adjust_orientations(
			biped_index,
			node_matrices,
			animation_graph->nodes.count,
			biped_limp_noodle_globals.last_positions);

		if (biped->biped.limp_body_current_relaxation_iterations < CHAR_MAX)
			biped->biped.limp_body_current_relaxation_iterations++;
	}

	return relaxation_complete;
}
