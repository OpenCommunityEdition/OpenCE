/*
INTERFACE.C

symbols in this file:
000CE180 0020:
	_interface_initialize (0000)
000CE1A0 0010:
	_interface_dispose_from_old_map (0000)
000CE1B0 0020:
	_interface_dispose (0000)
000CE1D0 0080:
	_interface_get_tag_index (0000)
000CE250 0080:
	_interface_get_real_argb_color (0000)
000CE2D0 0050:
	_interface_set_bitmap_text_draw_mode (0000)
000CE320 01c0:
	_code_000ce320 (0000)
000CE4E0 03a0:
	_interface_draw_screen (0000)
000CE880 0080:
	_profile_graph_toggle (0000)
000CE900 0110:
	_code_000ce900 (0000)
000CEA10 08e0:
	_code_000cea10 (0000)
000CF2F0 00e0:
	_interface_splitscreen_render (0000)
000CF3D0 0060:
	_interface_initialize_for_new_map (0000)
000CF430 0070:
	_interface_get_rgb_color (0000)
000CF4A0 0020:
	_interface_draw_fullscreen_overlays (0000)
000CF4C0 0180:
	_interface_draw_bitmap (0000)
000CF640 01c0:
	_interface_draw_bitmap_modulated (0000)
000CF800 0160:
	_interface_draw_bitmap_modulated_p32 (0000)
00270988 0047:
	??_C@_0EH@NDHILNFF@interface_tag_index?$DO?$DN0?5?$CG?$CG?5interf@ (0000)
002709D0 0025:
	??_C@_0CF@HPKANGD@c?3?2halo?2SOURCE?2interface?2interfa@ (0000)
002709F8 001f:
	??_C@_0BP@HBELBMEI@drawingbuf_counts?$FLindex?$FN?5?$DM?5512?$AA@ (0000)
00270A18 000d:
	??_C@_0N@KBDODLDN@?$HMn?$HMn?$HMn?$HMn?$HMn?$HMn?$AA@ (0000)
00270A28 0011:
	??_C@_0BB@EMOPPCDP@?$CF?55d?5particles?$HMn?$AA@ (0000)
00270A3C 002e:
	??_C@_0CO@GJNLBLAB@?$CF?55d?5active?5of?5?$CF?55d?5effects?5?$CI?$CF5d@ (0000)
00270A6C 002e:
	??_C@_0CO@ENADKIPF@?$CF?55d?5active?5of?5?$CF?55d?5objects?5?$CI?$CF?53@ (0000)
00270A9C 0020:
	??_C@_0CA@DMKIAGNI@?$CF?56?41fk?5free?5of?5?$CF?56?41fk?5total?$HMn?$AA@ (0000)
00270ABC 0004:
	__real@3a800000 (0000)
00270AC0 0010:
	??_C@_0BA@DAFKNDHA@window_count?$DN?$DN4?$AA@ (0000)
002E4C88 18918:
	_profile_game_value_count (0000)
	_profile_game_values (0008)
	_profile_frame_value_count (8308)
	_profile_frame_values (8310)
	_profile_graph_value_count (10610)
	_profile_graph_values (10618)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/color_table_group.h"
#include "cutscene/cinematics.h"
#include "game/game_globals.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "interface/first_person_weapons.h"
#include "interface/hud.h"
#include "interface/interface.h"
#include "interface/hud_messaging.h"
#include "interface/terminal.h"
#include "main/main.h"
#include "math/real_math.h"
#include "rasterizer/rasterizer.h"
#include "scenario/scenario.h"
#include "text/draw_string.h"

/* ---------- constants */

enum
{
	_shader_framebuffer_blend_function_alpha_blend = 0,
	_shader_framebuffer_blend_function_multiply,
	_shader_framebuffer_blend_function_double_multiply,
	_shader_framebuffer_blend_function_add,
	_shader_framebuffer_blend_function_subtract,
	_shader_framebuffer_blend_function_component_min,
	_shader_framebuffer_blend_function_component_max,
	_shader_framebuffer_blend_function_alpha_multiply_add,
	NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS
};

/* ---------- macros */

#define interface_tag_references_get() \
	(scenario_get_game_globals()->interface_tag_references.count ? \
		TAG_BLOCK_GET_ELEMENT(&scenario_get_game_globals()->interface_tag_references, 0, \
			struct interface_tag_references_definition) : \
		NULL)

/* ---------- structures */

union argb_color
{
	word n[4];
};
typedef union argb_color argb_color;

typedef char argb_color_size_assert[
	sizeof(argb_color) == 0x8 ? 1 : -1];

struct interface_tag_references_definition
{
	struct tag_reference tags[NUMBER_OF_INTERFACE_TAGS];
	byte unused[48];
};

typedef char interface_tag_references_definition_size_assert[
	sizeof(struct interface_tag_references_definition) == 0x130 ? 1 : -1];

/* ---------- prototypes */

void interface_splitscreen_render(
	void);
void code_000cea10(
	void);
/* ---------- globals */

/* ---------- public code */

void interface_initialize(
	void)
{
	terminal_initialize();
	hud_initialize();
	draw_string_initialize();
	first_person_weapons_initialize();

	return;
}

void interface_initialize_for_new_map(
	void)
{
	hud_initialize_for_new_map();
	draw_string_initialize_for_new_map();
	first_person_weapons_initialize_for_new_map();

	draw_string_set_draw_mode(
		interface_tag_references_get()->tags[_interface_font_terminal].index,
		NONE,
		0,
		0,
		global_real_argb_white);

	return;
}

void interface_dispose_from_old_map(
	void)
{
	draw_string_dispose_from_old_map();
	hud_dispose_from_old_map();
	first_person_weapons_dispose_from_old_map();

	return;
}

void interface_dispose(
	void)
{
	draw_string_dispose();
	terminal_dispose();
	hud_dispose();
	first_person_weapons_dispose();

	return;
}

long interface_get_tag_index(
	short interface_tag_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\interface\\interface.c",
		109,
		interface_tag_index>=0 && interface_tag_index<NUMBER_OF_INTERFACE_TAGS);

	return interface_tag_references_get()->tags[interface_tag_index].index;
}

real_argb_color *interface_get_real_argb_color(
	short interface_color_table_index,
	short color_index,
	real_argb_color *color)
{
	long color_table_tag_index = interface_get_tag_index(interface_color_table_index);

	color->alpha = color->red = color->green = color->blue = 1.0f;

	if (color_table_tag_index != NONE)
	{
		struct color_table_definition *color_table =
			color_table_definition_get(color_table_tag_index);

		if (color_table->colors.count)
		{
			color_index %= color_table->colors.count;
			*color = TAG_BLOCK_GET_ELEMENT(
				&color_table->colors,
				color_index,
				struct color_table_color)->real_color;
		}
	}

	return color;
}

void interface_set_bitmap_text_draw_mode(
	short interface_font_index,
	short style,
	short justification,
	unsigned long flags,
	short color_table_index,
	short color_index)
{
	long font_tag_index;
	real_argb_color color;

	font_tag_index = interface_get_tag_index(interface_font_index);
	interface_get_real_argb_color(color_table_index, color_index, &color);
	draw_string_set_draw_mode(font_tag_index, style, justification, flags, &color);

	return;
}

argb_color *interface_get_rgb_color(
	short interface_color_table_index,
	short color_index,
	argb_color *color)
{
	real_argb_color real_color;

	interface_get_real_argb_color(
		interface_color_table_index,
		color_index,
		&real_color);
	color->n[0] = (word)(real_color.n[0] * 65535.0f);
	color->n[1] = (word)(real_color.n[1] * 65535.0f);
	color->n[2] = (word)(real_color.n[2] * 65535.0f);
	color->n[3] = (word)(real_color.n[3] * 65535.0f);

	return color;
}

void interface_draw_fullscreen_overlays(
	void)
{
	cinematic_render();
	interface_splitscreen_render();
	hud_render_timer();
	terminal_draw();
	main_framerate_render();
	code_000cea10();

	return;
}

void interface_draw_bitmap(
	struct bitmap_data const *bitmap,
	point2d const *point,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	real fade)
{
	real_rectangle2d entire_bitmap = { 0.0f, 1.0f, 0.0f, 1.0f };
	real sine_theta = sine(theta);
	real cosine_theta = cosine(theta);
	pixel32 color;
	struct dynamic_screen_vertex vertices[NUMBER_OF_POINTS_PER_RECTANGLE];
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	short vertex_index;

	if (!clip)
		clip = &entire_bitmap;

	color = (((pixel32)(long)(fade*255.0f))<<24) | 0x00FFFFFF;

	for (vertex_index = 0; vertex_index < NUMBER_OF_POINTS_PER_RECTANGLE; vertex_index++)
	{
		real u = ((vertex_index+1)&2) ? clip->x1 : clip->x0;
		real v = (vertex_index>1) ? clip->y1 : clip->y0;
		real local_x = (bitmap->width*u - bitmap->registration_point_x)*scale;
		real local_y = (bitmap->height*v - bitmap->registration_point_y)*scale;

		vertices[vertex_index].position.x = point->x + local_x*cosine_theta - local_y*sine_theta;
		vertices[vertex_index].position.y = point->y + local_x*sine_theta + local_y*cosine_theta;
		vertices[vertex_index].texture_coordinates.u = u;
		vertices[vertex_index].texture_coordinates.v = v;
		vertices[vertex_index].color = color;
	}

	csmemset(&parameters, 0, sizeof(parameters));
	parameters.map_scale[0].i = parameters.map_scale[0].j =
		parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
	parameters.meter_parameters = NULL;
	parameters.point_sampled = FALSE;
	parameters.framebuffer_blend_function = _shader_framebuffer_blend_function_alpha_multiply_add;
	parameters.map[0] = (struct bitmap_data *)bitmap;

	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);

	return;
}

void interface_draw_bitmap_modulated(
	struct bitmap_data const *bitmap,
	point2d const *point,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	real_argb_color const *modulated_color,
	short shader_type)
{
	real_rectangle2d entire_bitmap =
		{ 0.0f, (real)bitmap->width, 0.0f, (real)bitmap->height };
	real sine_theta = sine(theta);
	real cosine_theta = cosine(theta);
	pixel32 color;
	struct dynamic_screen_vertex vertices[NUMBER_OF_POINTS_PER_RECTANGLE];
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	short vertex_index;

	if (!clip)
		clip = &entire_bitmap;

	color =
		(((pixel32)(long)(modulated_color->alpha*255.0f))<<24) |
		(((pixel32)(long)(modulated_color->red*255.0f))<<16) |
		(((pixel32)(long)(modulated_color->green*255.0f))<<8) |
		((pixel32)(long)(modulated_color->blue*255.0f));

	for (vertex_index = 0; vertex_index < NUMBER_OF_POINTS_PER_RECTANGLE; vertex_index++)
	{
		real u = ((vertex_index+1)&2) ? clip->x1 : clip->x0;
		real v = (vertex_index>1) ? clip->y1 : clip->y0;
		real local_x = (u - bitmap->registration_point_x)*scale;
		real local_y = (v - bitmap->registration_point_y)*scale;

		vertices[vertex_index].position.x = point->x + local_x*cosine_theta - local_y*sine_theta;
		vertices[vertex_index].position.y = point->y + local_x*sine_theta + local_y*cosine_theta;
		vertices[vertex_index].texture_coordinates.u = u;
		vertices[vertex_index].texture_coordinates.v = v;
		vertices[vertex_index].color = color;
	}

	csmemset(&parameters, 0, sizeof(parameters));
	parameters.map_scale[0].i = parameters.map_scale[0].j =
		parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
	parameters.meter_parameters = NULL;
	parameters.point_sampled = FALSE;
	parameters.framebuffer_blend_function = shader_type;
	parameters.map[0] = (struct bitmap_data *)bitmap;

	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);

	return;
}

/* ---------- private code */

void interface_splitscreen_render(
	void)
{
	rectangle2d bounds;
	short window_count;

	if (game_engine_force_single_screen() || cinematic_in_progress())
		return;

	window_count = local_player_count();

	if (window_count <= 1)
		return;

	bounds.y0 = 239;
	bounds.x0 = 0;
	bounds.y1 = 241;
	bounds.x1 = 640;
	draw_quad(&bounds, 0xFF000000);

	if (window_count <= 2)
		return;

	if (window_count == 3)
	{
		bounds.y0 = 240;
		bounds.x0 = 319;
		bounds.y1 = 480;
		bounds.x1 = 321;
		draw_quad(&bounds, 0xFF000000);

		return;
	}

	bounds.y0 = 0;
	bounds.x0 = 319;
	bounds.y1 = 480;
	bounds.x1 = 321;

	match_assert(
		"c:\\halo\\SOURCE\\interface\\interface.c",
		884,
		window_count==4);

	draw_quad(&bounds, 0xFF000000);

	return;
}
