/*
PATH.H

file has inline function assertions.
*/

#ifndef __PATH_H
#define __PATH_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"

/* ---------- constants */

enum
{
	PATH_NODE_LIST_SIZE = 0x400,
	PATH_HASH_TABLE_SIZE = 0x1000,
};

enum
{
	PATH_HASH_TABLE_SIZE_PER_KEY = 8,
	PATH_HASH_KEY_MASK = 511,
	PATH_HASH_TABLE_MASK = 4095,
	MAXIMUM_PATH_EDGES_PER_COLLISION_SURFACE = 64
};

enum
{
	_path_traverse_result_none = 0,
	_path_traverse_result_initial_not_pathfindable,
	_path_traverse_result_never_close_enough,
	_path_traverse_result_exhausted_search,
	_path_traverse_result_overflowed_nodes,
	_path_traverse_result_success,
	NUMBER_OF_PATH_TRAVERSE_RESULTS,
};

enum
{
	_path_build_result_none = 0,
	_path_build_result_no_destination,
	_path_build_result_cached_node_missing,
	_path_build_result_not_close_enough_to_destination,
	_path_build_result_obstacle_avoidance_failed,
	_path_build_result_success,
	NUMBER_OF_PATH_BUILD_RESULTS,
};


/* ---------- macros */

/* ---------- structures */

struct path_destination
{
	real_point3d point;
	long surface_index;
	real target_radius;
};

struct path_step
{
	long surface_index;
	real_point3d point;
};

struct path_result
{
	boolean valid;
	struct path_destination endpoint;
	boolean steps_finish_path;
	char step_count;
	char step_index;
	struct path_step steps[4];
};

struct path_input
{
	real pathfinding_radius;
	boolean ignore_broken_surfaces;
	long ignore_source_object_index;
	long ignore_target_object_index;
	boolean start_valid;
	real_point3d start_point;
	long start_surface_index;
	boolean attractor_valid;
	real_point3d attractor_point;
	long attractor_object_index;
	real attractor_radius;
	real attractor_weight;
	boolean search_bounded;
	real search_maximum_distance;
};

struct path_node
{
	short child_node_index;
	short parent_node_index;
	long parent_node_surface_index;
	long surface_index;
	real_point3d entry_point;
	real linear_distance_to_entry_point;
	real closest_approach_to_attractor;
	real path_distance_from_origin;
	real cumulative_cost;
	real total_cost_estimate;
	short quantized_cost_estimate;
	short depth;
	short heap_location;
	short last_render_id;
	real closest_distance_to_attractor;
	real_point3d closest_point_to_attractor;
};

typedef char path_node_size_assert[
	sizeof(struct path_node) == 0x44 ? 1 : -1];

struct path_heap_element
{
	short node_index;
	short quantized_cost_estimate;
};

struct path_state
{
	struct path_input input;
	struct path_debug_storage *debug;
	boolean destination_valid;
	struct path_destination destination;
	struct structure_bsp const *structure;
	short closest_node_index;
	real closest_distance;
	real closest_cost_estimate;
	real_point3d closest_point;
	short node_count;
	struct path_node node_list[PATH_NODE_LIST_SIZE];
	short heap_count;
	struct path_heap_element heap[1025];
	short hash_table[PATH_HASH_TABLE_SIZE];
};

struct path_avoidance_obstacles
{
	char __unknown00[0xC08];
};

struct path_avoidance_path
{
	long field_00;
	char __unknown04[0x1530];
};

struct path_debug_storage
{
	long actor_index;
	long path_time;
	long last_render_id;
	boolean valid;
	boolean failure;
	short structure_bsp_index;
	short path_traverse_result;
	short path_build_result;
	struct path_state path_state;
	char __unknown140A0[0x5C];
	short raw_step_count;
	word pad_140FE;
	struct path_step raw_steps[64];
	short smoothed_step_count;
	word pad_14502;
	struct path_step smoothed_steps[4];
	short avoided_step_count;
	word pad_14546;
	struct path_step avoided_steps[4];
	word pad_14588;
	short avoidance_path_count;
	struct path_avoidance_obstacles avoidance_obstacles[4];
	struct path_avoidance_path avoidance_paths[4];
};

typedef char path_state_node_count_offset_assert[
	offsetof(struct path_state, node_count) == 0x80 ? 1 : -1];
typedef char path_state_node_list_offset_assert[
	offsetof(struct path_state, node_list) == 0x84 ? 1 : -1];
typedef char path_state_hash_table_offset_assert[
	offsetof(struct path_state, hash_table) == 0x1208A ? 1 : -1];
typedef char path_state_size_assert[
	sizeof(struct path_state) == 0x1408C ? 1 : -1];

typedef char path_debug_storage_size_assert[
	sizeof(struct path_debug_storage) == 0x1CA7C ? 1 : -1];
typedef char path_debug_storage_raw_steps_offset_assert[
	offsetof(struct path_debug_storage, raw_steps) == 0x14100 ? 1 : -1];
typedef char path_debug_storage_avoided_steps_offset_assert[
	offsetof(struct path_debug_storage, avoided_steps) == 0x14548 ? 1 : -1];

/* ---------- prototypes/PATH.C */

// argument order is read off the call sites in ai_debug_update; every parameter
// is dword-sized, so the middle names of path_input_new are not pinned by it
void path_input_new(
	struct path_input *input,
	real pathfinding_radius,
	boolean ignore_broken_surfaces,
	long ignore_source_object_index);
void path_input_set_start(
	struct path_input *input,
	real_point3d const *point,
	long surface_index);
void path_input_set_search_bounds(
	struct path_input *input,
	real maximum_distance);
void path_input_set_attractor(
	struct path_input *input,
	real_point3d const *point,
	real radius,
	long object_index,
	real weight);

void path_state_new(
	struct path_input const *input,
	struct path_state *state,
	struct path_debug_storage *debug);
void path_state_destination(
	struct path_state *state,
	real_point3d const *point,
	long surface_index,
	real target_radius);
void path_state_find(
	struct path_state *state);
void path_state_build_path(
	struct path_state *state,
	boolean *complete);
void path_smooth(
	struct path_state *state,
	short raw_step_count,
	struct path_step const *raw_steps,
	short *smoothed_step_count,
	struct path_step *smoothed_steps,
	boolean *steps_finish_path);

struct path_node *path_get_node(
	struct path_state *state,
	short node_index);
short path_node_from_hash_table(
	struct path_state *state,
	long surface_index);
real path_attractor_weight(
	struct path_state *state,
	real_point3d const *point,
	real_point3d const *previous_point,
	real *closest_distance);

void paths_initialize(
	void);
void paths_dispose(
	void);
void paths_initialize_for_new_map(
	void);
void paths_dispose_from_old_map(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __PATH_H
