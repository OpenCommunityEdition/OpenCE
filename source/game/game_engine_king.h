/*
GAME_ENGINE_KING.H
*/

#ifndef __GAME_ENGINE_KING_H
#define __GAME_ENGINE_KING_H
#pragma once

/* ---------- headers */

#include "game/game_engine.h"

/* ---------- constants */

enum king_message
{
	king_message_enemy_on_the_hill = 0x1E,
	king_message_ally_on_the_hill,
	king_message_you_are_on_the_hill,
};

enum king_hill_state
{
	king_hill_uncontrolled = 0,
	king_hill_controlled,
	king_hill_controlled_red,
	king_hill_controlled_blue,
	king_hill_contested,
};

/* ---------- structures */

struct model_vertex_uncompressed;
struct render_animation;
struct render_lighting;

struct king_globals
{
	long score[16];
	long score_tick[16];
	boolean on_the_hill[16];
	long hill_point_count;
	real_point3d hill_points[12];
	real_point2d convex_hull[12];
	real_point3d hill_center;
	long hill_state;
	long hill_controlled_count;
	long hill_previous_controller;
	real hill_top;
	real hill_bottom;
	long hill_id;
	long hill_timer;
};

typedef char verify_king_globals_size[
	sizeof(struct king_globals) == 0x1AC ? 1 : -1];
typedef char verify_king_globals_on_the_hill_offset[
	offsetof(struct king_globals, on_the_hill) == 0x80 ? 1 : -1];
typedef char verify_king_globals_convex_hull_offset[
	offsetof(struct king_globals, convex_hull) == 0x124 ? 1 : -1];
typedef char verify_king_globals_hill_id_offset[
	offsetof(struct king_globals, hill_id) == 0x1A4 ? 1 : -1];

/* ---------- prototypes/GAME_ENGINE_KING.C */

void render_dynamic_quad_initialize(
	void);

void render_dynamic_quad(
	struct model_vertex_uncompressed *vertices,
	long shader_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation,
	real u_scale,
	real v_scale);

/* ---------- globals */

extern struct game_engine king_engine;

#endif // __GAME_ENGINE_KING_H
