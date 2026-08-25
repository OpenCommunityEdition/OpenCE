/*
CINEMATICS.C

symbols in this file:
000813F0 0040:
	_cinematic_initialize (0000)
00081430 0010:
	_cinematic_dispose (0000)
00081440 0030:
	_cinematic_initialize_for_new_map (0000)
00081470 0020:
	_cinematic_dispose_from_old_map (0000)
00081490 0040:
	_cinematic_start (0000)
000814D0 0010:
	_cinematic_can_be_skipped (0000)
000814E0 0010:
	_cinematic_skip_start (0000)
000814F0 0010:
	_cinematic_skip_stop (0000)
00081500 0030:
	_cinematic_show_letterbox (0000)
00081530 0150:
	_draw_quad (0000)
00081680 0020:
	_cinematic_force_title (0000)
000816A0 0020:
	_cinematic_suppress_bsp_object_creation (0000)
000816C0 0050:
	_cinematic_stop (0000)
00081710 0010:
	_cinematic_in_progress (0000)
00081720 0090:
	_cinematic_set_title_delayed (0000)
000817B0 0500:
	_cinematic_render (0000)
00081CB0 0020:
	_cinematic_set_title (0000)
002589FC 0012:
	??_C@_0BC@NGABDHNK@cinematic_globals?$AA@ (0000)
00258A10 0025:
	??_C@_0CF@PIGNLDBH@c?3?2halo?2SOURCE?2cutscene?2cinemati@ (0000)
00258A38 0012:
	??_C@_0BC@PJBLDLNO@cinematic?5globals?$AA@ (0000)
00258A4C 0032:
	??_C@_0DC@GMMMKPOA@no?5free?5chapter?5title?5slots?5to?5d@ (0000)
00258A80 0004:
	__real@3e000000 (0000)
00435CA0 0004:
	_cinematic_globals (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cinematics.h"
#include "ai/ai.h"
#include "bitmaps/bitmap_group.h"
#include "game/game.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "interface/ui_widget.h"
#include "items/projectiles.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_cinematics.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void cinematic_set_title_delayed(
	short title_index,
	real delay);
void draw_quad(
	rectangle2d *rectangle,
	pixel32 color);

/* ---------- globals */

struct cinematic_global_data *cinematic_globals;

/* ---------- public code */

void cinematic_initialize(
	void)
{
	cinematic_globals = game_state_malloc(
		"cinematic globals",
		NULL,
		sizeof(*cinematic_globals));
	match_assert(
		"c:\\halo\\SOURCE\\cutscene\\cinematics.c",
		24,
		cinematic_globals);

	return;
}

void cinematic_dispose(
	void)
{
	return;
}

void cinematic_initialize_for_new_map(
	void)
{
	csmemset(cinematic_globals, 0, sizeof(*cinematic_globals));
	csmemset(
		cinematic_globals->queued_titles,
		NONE,
		sizeof(cinematic_globals->queued_titles));

	return;
}

void cinematic_skip_start(
	void)
{
	cinematic_globals->can_be_skipped = TRUE;

	return;
}

void cinematic_skip_stop(
	void)
{
	cinematic_globals->can_be_skipped = FALSE;

	return;
}

void cinematic_dispose_from_old_map(
	void)
{
	cinematic_globals->show_letterbox = FALSE;
	cinematic_globals->in_progress = FALSE;

	return;
}

void cinematic_start(
	void)
{
	player_input_enable(FALSE);
	ai_globals_dialogue_triggers_enabled(FALSE);
	cinematic_globals->show_letterbox = TRUE;
	cinematic_globals->start_tick = game_time_get();
	cinematic_globals->in_progress = TRUE;
	projectiles_delete_all();

	return;
}

boolean cinematic_can_be_skipped(
	void)
{
	return cinematic_globals->can_be_skipped;
}

void cinematic_show_letterbox(
	boolean show)
{
	cinematic_globals->show_letterbox = show;
	if (show)
	{
		cinematic_globals->start_tick = game_time_get();
	}

	return;
}

void cinematic_force_title(
	unsigned short title_index)
{
	cinematic_globals->queued_titles[0].title_index = title_index;
	cinematic_globals->queued_titles[0].time = 0;

	return;
}

void cinematic_suppress_bsp_object_creation(
	boolean suppress)
{
	cinematic_globals->suppress_bsp_object_creation = suppress;

	return;
}

boolean cinematic_in_progress(
	void)
{
	return cinematic_globals->in_progress;
}

void cinematic_stop(
	void)
{
	cinematic_globals->show_letterbox = FALSE;
	player_input_enable(TRUE);
	ai_globals_dialogue_triggers_enabled(TRUE);
	cinematic_globals->in_progress = FALSE;
	rasterizer_screen_effects_initialize_for_new_map();
	if (global_rasterizer_model_ambient_reflection_tint)
	{
		csmemset(
			global_rasterizer_model_ambient_reflection_tint,
			0,
			sizeof(*global_rasterizer_model_ambient_reflection_tint));
	}
	rasterizer_set_near_clip_distance(0.0f);
	display_errors_deferred_until_cinematic_stop();

	return;
}

void cinematic_set_title(
	unsigned short title_index)
{
	cinematic_set_title_delayed(title_index, 0.0f);

	return;
}

/* ---------- private code */

void draw_quad(
	rectangle2d *rectangle,
	pixel32 color)
{
	struct bitmap_data *map;
	real_point2d positions[4];
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	struct dynamic_screen_vertex vertices[4];
	real_point2d *position;
	struct dynamic_screen_vertex *vertex;
	short vertex_index;
	struct game_globals *game_globals;
	struct game_globals_rasterizer_data *rasterizer_data;

	global_scenario_get();
	game_globals = scenario_get_game_globals();
	rasterizer_data = game_globals->rasterizer_data.count
		? TAG_BLOCK_GET_ELEMENT(
			&game_globals->rasterizer_data,
			0,
			struct game_globals_rasterizer_data)
		: NULL;
	map = TAG_BLOCK_GET_ELEMENT(
		&bitmap_group_get(rasterizer_data->default_textures[0].index)->bitmap_data,
		1,
		struct bitmap_data);

	rasterizer_globals.current_lock_operation = _rasterizer_lock_cinematics;

	positions[0].x = (real)rectangle->x0;
	positions[0].y = (real)rectangle->y0;
	positions[1].x = (real)rectangle->x1;
	positions[1].y = (real)rectangle->y0;
	positions[2].x = (real)rectangle->x1;
	positions[2].y = (real)rectangle->y1;
	positions[3].x = (real)rectangle->x0;
	positions[3].y = (real)rectangle->y1;

	position = positions;
	vertex = vertices;
	for (vertex_index = 0; vertex_index < NUMBEROF(vertices); vertex_index++)
	{
		vertex->position.x = position->x;
		vertex->color = color;
		vertex->texture_coordinates.x = 0.0f;
		vertex->texture_coordinates.y = 0.0f;
		vertex->position.y = position->y;
		position++;
		vertex++;
	}

	csmemset(&parameters, 0, sizeof(parameters));
	parameters.framebuffer_blend_function = 0;
	parameters.map_texture_scale[0].j = 1.0f;
	parameters.map_texture_scale[0].i = 1.0f;
	parameters.map_scale[0].j = 1.0f;
	parameters.map_scale[0].i = 1.0f;
	parameters.meter_parameters = NULL;
	parameters.point_sampled = FALSE;
	parameters.map[0] = map;

	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);

	rasterizer_globals.current_lock_operation = _rasterizer_lock_unlocked;

	return;
}
