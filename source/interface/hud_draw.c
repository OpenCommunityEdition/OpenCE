/*
HUD_DRAW.C

symbols in this file:
000C08E0 0010:
	_get_return_eip (0000)
000C08F0 0030:
	_check_stack_buffer (0000)
000C0920 0110:
	_code_000c0920 (0000)
000C0A30 0010:
	_hud_globals_get_scale (0000)
000C0A40 01f0:
	_hud_retrieve_bitmap_and_bounding_rect (0000)
000C0C30 01e0:
	_code_000c0c30 (0000)
000C0E10 01e0:
	_code_000c0e10 (0000)
000C0FF0 0038:
	_fast_ftol_C (0000)
000C1030 0140:
	_real_argb_color_to_pixel32 (0000)
000C1170 00c0:
	_real_rgb_color_to_pixel32 (0000)
000C1230 00b0:
	_real_alpha_intensity_to_pixel32 (0000)
000C12E0 03c0:
	_hud_calculate_point (0000)
000C16A0 0020:
	_get_flash_duration (0000)
000C16C0 0260:
	_get_flash_color (0000)
000C1920 0220:
	_code_000c1920 (0000)
000C1B40 08e0:
	_code_000c1b40 (0000)
000C2420 0180:
	_code_000c2420 (0000)
000C25A0 0140:
	_hud_draw_bitmap_direct (0000)
000C26E0 0520:
	_hud_draw_meter (0000)
000C2C00 0740:
	_hud_draw_numbers (0000)
000C3340 0040:
	_hud_draw_bitmap (0000)
000C3380 0280:
	_hud_draw_static_element (0000)
000C3600 0210:
	_hud_draw_weapon_overlays (0000)
0026FD68 0015:
	??_C@_0BF@LPAPHBIL@corrupt?5stack?5at?5?$CFd?$CB?$AA@ (0000)
0026FD80 0018:
	??_C@_0BI@PIKMLAKB@corrupt?5return?5address?$CB?$AA@ (0000)
0026FD98 0024:
	??_C@_0CE@JPFFJICF@c?3?2halo?2SOURCE?2interface?2hud_dra@ (0000)
0026FDBC 0011:
	??_C@_0BB@HJJFJKIM@frame_index?5?$DO?$DN?50?$AA@ (0000)
0026FDD0 0005:
	??_C@_04POIGMEJN@clip?$AA@ (0000)
0026FDD8 0036:
	??_C@_0DG@HKMKFIHC@player?9?$DOlocal_player_index?$DN?$DNrend@ (0000)
0026FE10 0011:
	??_C@_0BB@NLHGFKHK@verify?5?$DN?$DN?5result?$AA@ (0000)
0026FE24 0023:
	??_C@_0CD@KHKIMBNF@intensity?$DO?$DN0?40f?5?$CG?$CG?5intensity?$DM?$DN1?4@ (0000)
0026FE48 000a:
	??_C@_09MKANOGAA@placement?$AA@ (0000)
0026FE54 0013:
	??_C@_0BD@GFIGJECD@absolute_placement?$AA@ (0000)
0026FE68 002e:
	??_C@_0CO@FKHKBGNC@render?4local_player_index?$DN?$DNlocal@ (0000)
0026FE98 0004:
	__real@40c90e56 (0000)
0026FE9C 0025:
	??_C@_0CF@FIKNBIIH@dest_value?$DO?$DN0?40f?5?$CG?$CG?5dest_value?$DM?$DN@ (0000)
0026FEC4 001d:
	??_C@_0BN@IFLKFMOJ@source_bitmap?$DN?$DNnumber_bitmap?$AA@ (0000)
00453AB4 0004:
	_bss_00453ab4 (0000)
*/

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_group.h"
#include "cache/texture_cache.h"
#include "effects/particles.h"
#include "game/game.h"
#include "game/players.h"
#include "interface/hud_definitions.h"
#include "interface/hud_draw.h"
#include "interface/unit_hud_interface_definition.h"
#include "rasterizer/rasterizer.h"
#include "render/render.h"

/* ---------- constants */

enum
{
	STACK_BUFFER_LENGTH = 0x80,
	STACK_BUFFER_FILL = 0x62626262,
};

enum
{
	_hud_dont_scale_offset_bit = 0,
};

enum hud_flash_flags
{
	_hud_flash_reverse_colors_bit = 0,
};

enum hud_weapon_overlay_flags
{
	_hud_overlay_flashes_bit = 0,
	_hud_overlay_runtime_invalid_bit,
};

enum
{
	_shader_framebuffer_blend_function_alpha_multiply_add = 7,
};

/* ---------- macros */

#define hud_draw_stack_buffer_check(line) \
{ \
	short corrupt_index = check_stack_buffer(stack_buffer); \
	match_vassert("c:\\halo\\SOURCE\\interface\\hud_draw.c", line, return_eip==get_return_eip(), "corrupt return address!"); \
	match_vassert("c:\\halo\\SOURCE\\interface\\hud_draw.c", line, corrupt_index==NONE, csprintf(temporary, "corrupt stack at %d!", corrupt_index)); \
}

#define _hud_anchor_right_bit 0
#define _hud_anchor_bottom_bit 1

/* ---------- structures */

struct weapon_hud_overlay_item
{
	struct hud_placement_definition placement;
	struct hud_color_definition colors;
	short frame_rate;
	short pad;
	short sequence_index;
	short type;
	long flags;
	long unused[14];
};

struct weapon_hud_overlay_definition
{
	struct tag_reference bitmap;
	struct tag_block items;
};

/* ---------- prototypes */

static real_rectangle2d const *get_sprite_clip_rect(
	long bitmap_group_index,
	short sequence_index,
	short frame_index);
static void hud_calculate_bitmap_bounds(
	struct bitmap_data const *bitmap,
	short placement_type,
	real_rectangle2d const *clip,
	real_rectangle2d *bounds,
	boolean is_interface_bitmap);
static void hud_draw_bitmap_internal(
	void *meter_parameters,
	struct bitmap_data const *bitmap,
	point2d const *point,
	real_rectangle2d const *clip,
	real_rectangle2d const *bounds,
	real_vector2d const *xy_scale,
	real theta,
	pixel32 color);
static void hud_draw_bitmap_with_meter(
	void *meter_parameters,
	struct bitmap_data const *bitmap,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct hud_placement_definition const *placement,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean in_multiplayer,
	boolean is_interface_bitmap,
	boolean is_crosshair_bitmap);

/* ---------- globals */

/* ---------- public code */

/* Inspect the guarded caller's frame, not the return site of this helper.
 * A normal prologue would replace EBP and defeat the paired stack check. */
__declspec(naked) long get_return_eip(
	void)
{
	__asm
	{
		mov eax, [ebp+4]
		ret
	}
}

short check_stack_buffer(
	long *buffer)
{
	short index;

	for (index = STACK_BUFFER_LENGTH-1; index>=0; index--)
	{
		if (buffer[index]!=STACK_BUFFER_FILL)
			return index;
	}

	return NONE;
}

real hud_globals_get_scale(
	boolean in_multiplayer)
{
	return 1.0f;
}

void hud_retrieve_bitmap_and_bounding_rect(
	long bitmap_group_index,
	short sequence_index,
	short frame_index,
	struct bitmap_data const **bitmap,
	real_rectangle2d const **clip)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	match_assert("c:\\halo\\SOURCE\\interface\\hud_draw.c", 193, bitmap);
	match_assert("c:\\halo\\SOURCE\\interface\\hud_draw.c", 194, clip);

	if (bitmap_group_index!=NONE)
	{
		struct bitmap_group *group = bitmap_group_get(bitmap_group_index);

		if (sequence_index<group->sequences.count)
		{
			struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
				&group->sequences, sequence_index, struct bitmap_group_sequence);
			long sprite_count;

			frame_index &= 0x7FFF;
			match_assert("c:\\halo\\SOURCE\\interface\\hud_draw.c", 205, frame_index >= 0);

			sprite_count = sequence->sprites.count;
			if (sprite_count)
			{
				struct bitmap_group_sprite *sprite = TAG_BLOCK_GET_ELEMENT(
					&sequence->sprites, frame_index%sprite_count, struct bitmap_group_sprite);

				*bitmap = TAG_BLOCK_GET_ELEMENT(
					&group->bitmap_data, sprite->bitmap_index, struct bitmap_data);
			}
			else
			{
				*bitmap = bitmap_group_get_bitmap_from_sequence(
					bitmap_group_index, sequence_index, frame_index);
			}
		}
	}

	if (*bitmap)
		*clip = get_sprite_clip_rect(bitmap_group_index, sequence_index, frame_index);
	else
		*clip = NULL;

	hud_draw_stack_buffer_check(228);

	return;
}

pixel32 real_alpha_intensity_to_pixel32(
	real alpha,
	real intensity)
{
	real_argb_color color;

	match_assert(
		"..\\bitmaps\\bitmaps_inlines.h",
		0x13F,
		alpha>=0.0f && alpha<=1.0f);
	match_assert(
		"..\\bitmaps\\bitmaps_inlines.h",
		0x140,
		intensity>=0.0f && intensity<=1.0f);

	color.alpha = alpha;
	color.red = intensity;
	color.green = intensity;
	color.blue = intensity;

	return real_argb_color_to_pixel32(&color);
}

void hud_calculate_point(
	short local_player_index,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct hud_placement_definition const *placement,
	struct bitmap_data const *bitmap_data,
	boolean in_multiplayer,
	real override_scale,
	point2d *result)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	real_point2d point;
	real scale;
	short corner;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	if (in_multiplayer && override_scale != 0.0f)
	{
		scale = override_scale;
	}
	else
	{
		scale = hud_globals_get_scale(in_multiplayer);
	}

	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_draw.c",
		125,
		render.local_player_index==local_player_index);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_draw.c",
		126,
		absolute_placement);
	match_assert(
		"c:\\halo\\SOURCE\\interface\\hud_draw.c",
		127,
		placement);

	corner = absolute_placement->corner;
	if (corner < _hud_anchor_center)
	{
		point.x = placement->offset.x *
			(TEST_FLAG(corner, _hud_anchor_right_bit) ? -1 : 1) * scale +
			render.camera.window_bounds.v[
				((corner & FLAG(_hud_anchor_right_bit)) << 1) |
					FLAG(_hud_anchor_right_bit)] -
			render.camera.viewport_bounds.x0;
		point.y = placement->offset.y *
			(TEST_FLAG(corner, _hud_anchor_bottom_bit) ? -1 : 1) * scale +
			render.camera.window_bounds.v[corner & FLAG(_hud_anchor_bottom_bit)] -
			render.camera.viewport_bounds.y0;
	}
	else
	{
		point2d window_center;

		window_center.x = (short)(
			(render.camera.window_bounds.x1 + render.camera.window_bounds.x0) / 2);
		window_center.y = (short)(
			(render.camera.window_bounds.y1 + render.camera.window_bounds.y0) / 2);

		point.x = window_center.x - render.camera.viewport_bounds.x0 +
			placement->offset.x * scale;
		point.y = window_center.y - render.camera.viewport_bounds.y0 +
			placement->offset.y * scale;
	}

	if (bitmap_data)
	{
		switch (corner)
		{
		case _hud_anchor_top_left:
			point.x += bitmap_data->registration_point_x * scale;
			point.y += bitmap_data->registration_point_y * scale;
			break;

		case _hud_anchor_top_right:
			point.x += (bitmap_data->registration_point_x - bitmap_data->width) * scale;
			point.y += bitmap_data->registration_point_y * scale;
			break;

		case _hud_anchor_bottom_left:
			point.x += bitmap_data->registration_point_x * scale;
			point.y += (bitmap_data->registration_point_y - bitmap_data->height) * scale;
			break;

		case _hud_anchor_bottom_right:
			point.x += (bitmap_data->registration_point_x - bitmap_data->width) * scale;
			point.y += (bitmap_data->registration_point_y - bitmap_data->height) * scale;
			break;

		case _hud_anchor_center:
			point.x += (bitmap_data->registration_point_x + bitmap_data->width / 2) * scale;
			point.y += (bitmap_data->registration_point_y + bitmap_data->width / 2) * scale;
			break;

		default:
			match_assert(
				"c:\\halo\\SOURCE\\interface\\hud_draw.c",
				174,
				!"unreachable");
			break;
		}
	}

	result->x = (short)fast_ftol(point.x);
	result->y = (short)fast_ftol(point.y);

	hud_draw_stack_buffer_check(181);

	return;
}

long get_flash_duration(
	struct hud_color_definition const *hud_color)
{
	return fast_ftol(hud_color->flash_period * 30.0f);
}

pixel32 get_flash_color(
	struct hud_color_definition const *hud_color,
	long reference_value)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	real_argb_color result;
	real_argb_color base_color;
	real_argb_color flash_color;
	real flash_phase;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	flash_phase = (real)fmod(
		(game_time_get() - reference_value) * (1.0f / TICKS_PER_SECOND),
		hud_color->flash_period);
	pixel32_to_real_argb_color(hud_color->color, &base_color);
	pixel32_to_real_argb_color(hud_color->flash_color, &flash_color);

	if (flash_phase < hud_color->number_of_flashes *
		(hud_color->flash_delay + hud_color->flash_length))
	{
		flash_phase = (real)fmod(
			flash_phase,
			hud_color->flash_delay + hud_color->flash_length);

		if (!reference_value)
		{
			if (TEST_FLAG(
				hud_color->flash_flags,
				_hud_flash_reverse_colors_bit))
			{
				result = base_color;
			}
			else
			{
				result = flash_color;
			}
		}
		else if (flash_phase < hud_color->flash_length)
		{
			real fraction = square_root(
				PIN(
					(real)(1.0 -
						(cos(flash_phase / hud_color->flash_length * 6.283f) + 1.0) * 0.5),
					0.0f,
					1.0f));

			if (TEST_FLAG(
				hud_color->flash_flags,
				_hud_flash_reverse_colors_bit))
			{
				vectors_interpolate(
					(real_vector3d const *)&flash_color.rgb,
					(real_vector3d const *)&base_color.rgb,
					fraction,
					(real_vector3d *)&result.rgb);
				scalars_interpolate(
					flash_color.alpha,
					base_color.alpha,
					fraction,
					&result.alpha);
			}
			else
			{
				vectors_interpolate(
					(real_vector3d const *)&base_color.rgb,
					(real_vector3d const *)&flash_color.rgb,
					fraction,
					(real_vector3d *)&result.rgb);
				scalars_interpolate(
					base_color.alpha,
					flash_color.alpha,
					fraction,
					&result.alpha);
			}
		}
		else
		{
			if (TEST_FLAG(
				hud_color->flash_flags,
				_hud_flash_reverse_colors_bit))
			{
				result = flash_color;
			}
			else
			{
				result = base_color;
			}
		}
	}
	else
	{
		result = base_color;
	}

	hud_draw_stack_buffer_check(311);

	return real_argb_color_to_pixel32(&result);
}

void hud_draw_weapon_overlays(
	short local_player_index,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct weapon_hud_overlay_definition const *overlays,
	long type_flags,
	long reference_time,
	short draw_flags,
	boolean in_multiplayer)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	long item_index;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	for (item_index = 0; item_index < overlays->items.count; item_index++)
	{
		struct weapon_hud_overlay_item const *item = TAG_BLOCK_GET_ELEMENT(
			&overlays->items,
			item_index,
			struct weapon_hud_overlay_item);

		if (!TEST_FLAG(item->flags, _hud_overlay_runtime_invalid_bit) &&
			(item->type & type_flags))
		{
			struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
				&bitmap_group_get(overlays->bitmap.index)->sequences,
				item->sequence_index,
				struct bitmap_group_sequence);
			pixel32 color;
			short frame_index;

			if (TEST_FLAG(item->flags, _hud_overlay_flashes_bit) &&
				TEST_FLAG(draw_flags, _hud_draw_flashing_bit))
			{
				color = get_flash_color(&item->colors, reference_time);
			}
			else
			{
				color = item->colors.color;
			}

			if (TEST_FLAG(item->flags, _hud_overlay_flashes_bit) &&
				TEST_FLAG(draw_flags, _hud_draw_flashing_bit) &&
				item->frame_rate > 0)
			{
				frame_index = (short)(((game_time_get() - reference_time) /
					item->frame_rate / TICKS_PER_SECOND) % sequence->sprites.count);
			}
			else
			{
				frame_index = 0;
			}

			{
				struct bitmap_data const *bitmap = NULL;
				real_rectangle2d const *clip = NULL;

				hud_retrieve_bitmap_and_bounding_rect(
					overlays->bitmap.index,
					item->sequence_index,
					frame_index,
					&bitmap,
					&clip);

				if (bitmap && _texture_cache_bitmap_get_hardware_format(
					(struct bitmap_data *)bitmap,
					FALSE,
					TRUE))
				{
					hud_draw_bitmap_with_meter(
						NULL,
						bitmap,
						absolute_placement,
						&item->placement,
						clip,
						1.0f,
						0.0f,
						color,
						in_multiplayer,
						FALSE,
						FALSE);
				}
			}
		}
	}

	hud_draw_stack_buffer_check(748);

	return;
}

void hud_draw_bitmap(
	struct bitmap_data const *bitmap,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct hud_placement_definition const *placement,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean in_multiplayer,
	boolean is_interface_bitmap,
	boolean is_crosshair_bitmap)
{
	hud_draw_bitmap_with_meter(
		NULL,
		bitmap,
		absolute_placement,
		placement,
		clip,
		scale,
		theta,
		color,
		in_multiplayer,
		is_interface_bitmap,
		is_crosshair_bitmap);

	return;
}

static void hud_draw_bitmap_internal(
	void *meter_parameters,
	struct bitmap_data const *bitmap,
	point2d const *point,
	real_rectangle2d const *clip,
	real_rectangle2d const *bounds,
	real_vector2d const *xy_scale,
	real theta,
	pixel32 color)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	real sin_theta;
	real cos_theta;
	struct dynamic_screen_vertex vertices[4];
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	short vertex_index;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	sin_theta = (real)sin(theta);
	cos_theta = (real)cos(theta);

	for (vertex_index = 0; vertex_index < 4; vertex_index++)
	{
		long use_x1 = (vertex_index+1)&2;
		real texture_x = use_x1 ? clip->x1 : clip->x0;
		real texture_y = vertex_index>1 ? clip->y1 : clip->y0;
		real bound_x = use_x1 ? bounds->x1 : bounds->x0;
		real bound_y = vertex_index>1 ? bounds->y1 : bounds->y0;

		vertices[vertex_index].position.x = (real)(point->x + fast_ftol(
			(bound_x*cos_theta-bound_y*sin_theta)*xy_scale->i));
		vertices[vertex_index].position.y = (real)(point->y + fast_ftol(
			(bound_y*cos_theta+bound_x*sin_theta)*xy_scale->j));
		vertices[vertex_index].texture_coordinates.x = texture_x;
		vertices[vertex_index].texture_coordinates.y = texture_y;
		vertices[vertex_index].color = color;
	}

	csmemset(&parameters, 0, sizeof(parameters));
	parameters.map_texture_scale[0].j = 1.0f;
	parameters.map_texture_scale[0].i = 1.0f;
	parameters.map_scale[0].j = 1.0f;
	parameters.map_scale[0].i = 1.0f;
	parameters.meter_parameters = meter_parameters;
	parameters.point_sampled = meter_parameters && local_player_count()==1;
	parameters.map[0] = (struct bitmap_data *)bitmap;
	parameters.framebuffer_blend_function =
		_shader_framebuffer_blend_function_alpha_multiply_add;

	rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);

	hud_draw_stack_buffer_check(985);

	return;
}

static void hud_draw_bitmap_with_meter(
	void *meter_parameters,
	struct bitmap_data const *bitmap,
	struct hud_absolute_placement_definition const *absolute_placement,
	struct hud_placement_definition const *placement,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean in_multiplayer,
	boolean is_interface_bitmap,
	boolean is_crosshair_bitmap)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	real_rectangle2d default_clip;
	real_vector2d xy_scale;
	point2d point;
	real_rectangle2d bounds;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	default_clip.x0 = 0.0f;
	default_clip.x1 = 1.0f;
	default_clip.y0 = 0.0f;
	default_clip.y1 = 1.0f;

	if (is_interface_bitmap)
	{
		default_clip.x1 = (real)bitmap->width;
		default_clip.y1 = (real)bitmap->height;
	}

	if (!clip)
	{
		clip = &default_clip;
	}

	xy_scale.i = placement->scale.i*scale;
	xy_scale.j = placement->scale.j*scale;

	hud_calculate_point(
		render.local_player_index,
		absolute_placement,
		placement,
		NULL,
		in_multiplayer && !TEST_FLAG(
			placement->multiplayer_scaling_flags,
			_hud_dont_scale_offset_bit),
		0.0f,
		&point);
	hud_calculate_bitmap_bounds(
		bitmap,
		absolute_placement->corner,
		clip,
		&bounds,
		is_interface_bitmap);
	hud_draw_bitmap_internal(
		meter_parameters,
		bitmap,
		&point,
		clip,
		&bounds,
		&xy_scale,
		theta,
		color);

	hud_draw_stack_buffer_check(814);

	return;
}

/* ---------- private code */

static real_rectangle2d const *get_sprite_clip_rect(
	long bitmap_group_index,
	short sequence_index,
	short frame_index)
{
	real_rectangle2d const *result = NULL;
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	if (bitmap_group_index!=NONE && sequence_index!=NONE && frame_index!=NONE)
	{
		struct bitmap_group *group = bitmap_group_get(bitmap_group_index);

		if (sequence_index<group->sequences.count)
		{
			struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
				&group->sequences, sequence_index, struct bitmap_group_sequence);
			long sprite_count = sequence->sprites.count;

			if (sprite_count)
			{
				struct bitmap_group_sprite *sprite = TAG_BLOCK_GET_ELEMENT(
					&sequence->sprites, frame_index%sprite_count, struct bitmap_group_sprite);

				result = &sprite->bounds;
			}
		}
	}

	hud_draw_stack_buffer_check(100);

	return result;
}

static void hud_calculate_bitmap_bounds(
	struct bitmap_data const *bitmap,
	short placement_type,
	real_rectangle2d const *clip,
	real_rectangle2d *bounds,
	boolean is_interface_bitmap)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	real width;
	real height;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	width = (clip->x1-clip->x0)*(is_interface_bitmap ? 1 : bitmap->width);
	height = (clip->y1-clip->y0)*(is_interface_bitmap ? 1 : bitmap->height);

	switch (placement_type)
	{
	case _hud_anchor_top_left:
		bounds->x0 = 0.0f;
		bounds->x1 = width;
		bounds->y0 = 0.0f;
		bounds->y1 = height;
		break;

	case _hud_anchor_top_right:
		bounds->x0 = -width;
		bounds->x1 = 0.0f;
		bounds->y0 = 0.0f;
		bounds->y1 = height;
		break;

	case _hud_anchor_bottom_left:
		bounds->x0 = 0.0f;
		bounds->x1 = width;
		bounds->y0 = -height;
		bounds->y1 = 0.0f;
		break;

	case _hud_anchor_bottom_right:
		bounds->x0 = -width;
		bounds->x1 = 0.0f;
		bounds->y0 = -height;
		bounds->y1 = 0.0f;
		break;

	case _hud_anchor_center:
		bounds->x0 = width*-0.5f;
		bounds->x1 = width*0.5f;
		bounds->y0 = height*-0.5f;
		bounds->y1 = height*0.5f;
		break;

	default:
		match_assert("c:\\halo\\SOURCE\\interface\\hud_draw.c", 904, !"unreachable");
		break;
	}

	hud_draw_stack_buffer_check(907);

	return;
}

void hud_draw_bitmap_direct(
	struct bitmap_data const *bitmap,
	short placement,
	point2d const *point,
	real_rectangle2d const *clip,
	real scale,
	real theta,
	pixel32 color,
	boolean is_interface_bitmap)
{
	long return_eip = get_return_eip();
	long stack_buffer[STACK_BUFFER_LENGTH];
	real_rectangle2d default_clip;
	real_vector2d xy_scale;
	real_rectangle2d bounds;

	csmemset(stack_buffer, 0x62, sizeof(stack_buffer));

	default_clip.x0 = 0.0f;
	default_clip.x1 = 1.0f;
	default_clip.y0 = 0.0f;
	default_clip.y1 = 1.0f;

	if (is_interface_bitmap)
	{
		default_clip.x1 = (real)bitmap->width;
		default_clip.y1 = (real)bitmap->height;
	}

	if (!clip)
	{
		clip = &default_clip;
	}

	xy_scale.i = scale;
	xy_scale.j = scale;

	hud_calculate_bitmap_bounds(
		bitmap,
		placement,
		clip,
		&bounds,
		is_interface_bitmap);
	hud_draw_bitmap_internal(
		NULL,
		bitmap,
		point,
		clip,
		&bounds,
		&xy_scale,
		theta,
		color);

	hud_draw_stack_buffer_check(856);

	return;
}
