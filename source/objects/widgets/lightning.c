/*
LIGHTNING.C

symbols in this file:
00124B70 0040:
	_lightnings_initialize (0000)
00124BB0 0010:
	_lightnings_dispose (0000)
00124BC0 0020:
	_lightnings_initialize_for_new_map (0000)
00124BE0 0020:
	_lightnings_dispose_from_old_map (0000)
00124C00 0040:
	_lightning_new (0000)
00124C40 0020:
	_lightning_delete (0000)
00124C60 0010:
	_lightning_render (0000)
00124C70 00f0:
	_code_00124c70 (0000)
00124D60 0a10:
	_lightning_submit (0000)
00289208 0021:
	??_C@_0CB@BNDAIAGI@lightning_globals?4lightning_data@ (0000)
0028922C 002b:
	??_C@_0CL@PEKNBFKF@c?3?2halo?2SOURCE?2objects?2widgets?2l@ (0000)
00289258 000b:
	??_C@_0L@PEMAMKLL@lightnings?$AA@ (0000)
00289264 0017:
	??_C@_0BH@PECODGGH@random_position_bounds?$AA@ (0000)
0028927C 001b:
	??_C@_0BL@JLLGKOIH@points?$FLvertex_index?$FN?4valid?$AA@ (0000)
00289298 002f:
	??_C@_0CP@DDEILNHB@?$CBpoints?$FLpoint_count?$CLsegment_poin@ (0000)
002892C8 001c:
	??_C@_0BM@GBNGDLFN@segment_point_start_index?$DO0?$AA@ (0000)
002892E4 002f:
	??_C@_0CP@ODBOIENA@?$CBpoints?$FLpoint_count?$CLsegment_poin@ (0000)
00456E44 0004:
	_bss_00456e44 (0000)
*/

/* ---------- headers */

#include "objects/widgets/lightning.h"

#include "bitmaps/bitmap_group.h"
#include "cache/texture_cache.h"
#include "cseries/cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "render/render.h"
#include "saved games/game_state.h"
#include "shaders/shader_definitions.h"
#include "shaders/shaders.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

enum
{
	LIGHTNING_TAG = 'elec',
	MAXIMUM_LIGHTNING_POINTS = 4097,
};

enum
{
	_lightning_marker_not_connected_to_next_marker_bit = 0,
};

enum
{
	_rasterizer_lock_none = 0,
	_rasterizer_lock_lightning = 12,
};

/* ---------- macros */

#define lightning_definition_get(index) \
	((struct lightning_definition *)tag_get(LIGHTNING_TAG, (index)))

/* ---------- structures */

struct lightning_marker_definition
{
	char attachment_marker[32];
	word flags;
	short type;
	short octaves_to_next_marker;
	word pad26;
	byte reserved28[0x4C];
	real_vector3d random_position_bounds;
	real random_jitter_offset;
	real thickness;
	real_argb_color tint;
	byte reserved98[0x4C];
};

struct lightning_definition
{
	word flags;
	short count;
	byte reserved04[0x28];
	short jitter_scale_source;
	short thickness_scale_source;
	short tint_modulation_source;
	short brightness_scale_source;
	struct tag_reference map;
	byte reserved44[0x54];
	struct tag_block markers;
	struct tag_block shaders;
	byte reservedB0[0x58];
};

struct intermediate_lightning_point
{
	real_point3d position;
	real width;
	real_argb_color color;
	boolean valid;
	byte pad21[3];
};

struct lightning_vertex
{
	real_point3d point;
	real_point2d texture;
	pixel32 color;
};

typedef char lightning_marker_definition_size_assert[
	sizeof(struct lightning_marker_definition) == 0xE4 ? 1 : -1];
typedef char lightning_marker_random_position_bounds_offset_assert[
	offsetof(struct lightning_marker_definition, random_position_bounds) == 0x74 ? 1 : -1];
typedef char lightning_marker_random_jitter_offset_offset_assert[
	offsetof(struct lightning_marker_definition, random_jitter_offset) == 0x80 ? 1 : -1];
typedef char lightning_marker_thickness_offset_assert[
	offsetof(struct lightning_marker_definition, thickness) == 0x84 ? 1 : -1];
typedef char lightning_marker_tint_offset_assert[
	offsetof(struct lightning_marker_definition, tint) == 0x88 ? 1 : -1];
typedef char lightning_definition_size_assert[
	sizeof(struct lightning_definition) == 0x108 ? 1 : -1];
typedef char lightning_definition_markers_offset_assert[
	offsetof(struct lightning_definition, markers) == 0x98 ? 1 : -1];
typedef char lightning_definition_shaders_offset_assert[
	offsetof(struct lightning_definition, shaders) == 0xA4 ? 1 : -1];
typedef char intermediate_lightning_point_size_assert[
	sizeof(struct intermediate_lightning_point) == 0x24 ? 1 : -1];
typedef char lightning_vertex_size_assert[
	sizeof(struct lightning_vertex) == 0x18 ? 1 : -1];

/* ---------- prototypes */

static void lightning_offset_marker_position(
	real_point3d *position,
	real_matrix4x3 const *matrix,
	real_vector3d const *random_position_bounds);

/* ---------- globals */

struct lightning_globals lightning_globals = {0};

/* ---------- public code */

void lightnings_initialize(
	void)
{
	lightning_globals.lightning_data = game_state_data_new("lightnings", 256, 8);
	match_assert("c:\\halo\\SOURCE\\objects\\widgets\\lightning.c", 50, lightning_globals.lightning_data);

	return;
}

void lightnings_dispose(
	void)
{
	return;
}

void lightnings_initialize_for_new_map(
	void)
{
	if (lightning_globals.lightning_data)
		data_make_valid(lightning_globals.lightning_data);

	return;
}

void lightnings_dispose_from_old_map(
	void)
{
	if (lightning_globals.lightning_data)
		data_make_invalid(lightning_globals.lightning_data);

	return;
}

long lightning_new(
	long definition_index)
{
	long lightning_index = datum_new(lightning_globals.lightning_data);

	if (lightning_index != NONE)
		lightning_get(lightning_index)->definition_index = definition_index;

	return lightning_index;
}

void lightning_delete(
	long lightning_index)
{
	if (lightning_index != NONE)
		datum_delete(lightning_globals.lightning_data, lightning_index);

	return;
}

void lightning_render(
	void)
{
	return;
}

/* ---------- private code */

static void lightning_offset_marker_position(
	real_point3d *position,
	real_matrix4x3 const *matrix,
	real_vector3d const *random_position_bounds)
{
	real random_z;
	real random_y;
	real random_x;
	real_vector3d offset;

	match_assert("c:\\halo\\SOURCE\\objects\\widgets\\lightning.c", 0x74, position);
	match_assert("c:\\halo\\SOURCE\\objects\\widgets\\lightning.c", 0x75, matrix);
	match_assert("c:\\halo\\SOURCE\\objects\\widgets\\lightning.c", 0x76, random_position_bounds);

	random_z = real_seed_random(get_global_local_random_seed_address());
	random_y = real_seed_random(get_global_local_random_seed_address());
	random_x = real_seed_random(get_global_local_random_seed_address());
	offset.i = (random_x + random_x - 1.0f) * random_position_bounds->i;
	offset.j = (random_y + random_y - 1.0f) * random_position_bounds->j;
	offset.k = (random_z + random_z - 1.0f) * random_position_bounds->k;
	matrix4x3_transform_vector(matrix, &offset, &offset);
	position->x = offset.i + position->x;
	position->y = offset.j + position->y;
	position->z = offset.k + position->z;

	return;
}

/* ---------- public code */

void lightning_submit(
	long object_index,
	long lightning_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation)
{
	struct lightning_datum *lightning;
	struct lightning_definition *definition;
	struct lightning_marker_definition *first_marker_definition;
	struct bitmap_group *bitmap_group;
	struct bitmap_data *bitmap;
	struct object_marker object_marker;
	struct intermediate_lightning_point points[MAXIMUM_LIGHTNING_POINTS];
	short instance_index;

	if (object_index == NONE || lightning_index == NONE)
		return;

	lightning = lightning_get(lightning_index);
	definition = lightning_definition_get(lightning->definition_index);
	if (definition->markers.count <= 0)
		return;

	first_marker_definition = TAG_BLOCK_GET_ELEMENT(
		&definition->markers,
		0,
		struct lightning_marker_definition);
	if (object_get_marker_by_name(
		object_index,
		first_marker_definition->attachment_marker,
		&object_marker,
		1) <= 0)
	{
		return;
	}

	bitmap_group = bitmap_group_get(definition->map.index);
	bitmap = TAG_BLOCK_GET_ELEMENT(
		&bitmap_group->bitmap_data,
		0,
		struct bitmap_data);
	if (!_texture_cache_bitmap_get_hardware_format(bitmap, FALSE, TRUE))
		return;

	if (definition->count <= 0)
		return;

	for (instance_index = 0;
		instance_index < definition->count;
		instance_index = (short)(instance_index + 1))
	{
		short point_count = 0;
		real jitter_scale = 1.0f;
		boolean first_marker = TRUE;
		short marker_index;

		if (animation && animation->values &&
			VALID_INDEX(definition->jitter_scale_source - 1, 4))
		{
			jitter_scale = animation->values[definition->jitter_scale_source - 1];
		}

		for (marker_index = 0;
			marker_index < definition->markers.count;
			marker_index = (short)(marker_index + 1))
		{
			struct lightning_marker_definition *marker_definition = TAG_BLOCK_GET_ELEMENT(
				&definition->markers,
				marker_index,
				struct lightning_marker_definition);

			if (first_marker)
			{
				point_count = 0;
				csmemset(points, 0, sizeof(points));
				object_get_marker_by_name(
					object_index,
					marker_definition->attachment_marker,
					&object_marker,
					1);
				points[0].position = object_marker.matrix.position;
				lightning_offset_marker_position(
					&points[0].position,
					&object_marker.matrix,
					&marker_definition->random_position_bounds);
				points[0].width = marker_definition->thickness;
				points[0].color = marker_definition->tint;
				points[0].valid = TRUE;
				first_marker = FALSE;
			}

			if (!TEST_FLAG(
				marker_definition->flags,
				_lightning_marker_not_connected_to_next_marker_bit) &&
				marker_index != definition->markers.count - 1)
			{
				struct lightning_marker_definition *next_marker_definition = TAG_BLOCK_GET_ELEMENT(
					&definition->markers,
					marker_index + 1,
					struct lightning_marker_definition);
				short octave_count = marker_definition->octaves_to_next_marker;
				short segment_point_count = (short)(1 << octave_count);
				short segment_start_index = point_count;
				short segment_end_index = (short)(segment_start_index + segment_point_count);
				struct intermediate_lightning_point *segment_end = &points[segment_end_index];
				real_vector3d segment_delta;
				real_vector3d perpendicular;
				real level_scale = 1.0f;

				match_assert(
					"c:\\halo\\SOURCE\\objects\\widgets\\lightning.c",
					0x17F,
					!points[point_count+segment_point_count].valid);
				object_get_marker_by_name(
					object_index,
					next_marker_definition->attachment_marker,
					&object_marker,
					1);
				segment_end->position = object_marker.matrix.position;
				lightning_offset_marker_position(
					&segment_end->position,
					&object_marker.matrix,
					&next_marker_definition->random_position_bounds);
				segment_end->width = next_marker_definition->thickness;
				segment_end->color = next_marker_definition->tint;
				segment_end->valid = TRUE;

				vector_from_points3d(
					&points[segment_start_index].position,
					&segment_end->position,
					&segment_delta);
				cross_product3d(&segment_delta, &render.camera.forward, &perpendicular);
				if (normalize3d(&perpendicular) == 0.0f)
				{
					perpendicular.i = global_z_axis3d->x;
					perpendicular.j = global_z_axis3d->y;
					perpendicular.k = global_z_axis3d->z;
				}

				if (octave_count > 0)
				{
					short octave_index = (short)(octave_count - 1);
					short remaining_octaves = octave_count;

					do
					{
						short segment_point_start_index = (short)(1 << octave_index);

						match_assert(
							"c:\\halo\\SOURCE\\objects\\widgets\\lightning.c",
							0x198,
							segment_point_start_index>0);
						if (segment_point_start_index < segment_point_count)
						{
							real segment_point_count_real = (real)segment_point_count;
							short segment_point_index = segment_point_start_index;

							do
							{
								struct intermediate_lightning_point *left_point =
									&points[segment_start_index + segment_point_index - segment_point_start_index];
								struct intermediate_lightning_point *right_point =
									&points[segment_start_index + segment_point_index + segment_point_start_index];
								struct intermediate_lightning_point *middle_point =
									&points[segment_start_index + segment_point_index];
								real marker_fraction = (real)segment_point_index / segment_point_count_real;
								real jitter =
									(marker_fraction *
										(next_marker_definition->random_jitter_offset -
										marker_definition->random_jitter_offset) +
									marker_definition->random_jitter_offset) *
									level_scale * jitter_scale;
								real displacement;

								match_assert(
									"c:\\halo\\SOURCE\\objects\\widgets\\lightning.c",
									0x1AA,
									!points[point_count+segment_point_index].valid);
								displacement = real_seed_random_range(
									get_global_local_random_seed_address(),
									-1.0f,
									1.0f) * jitter;
								middle_point->position.x =
									(right_point->position.x + left_point->position.x) * 0.5f +
									perpendicular.i * displacement;
								middle_point->position.y =
									(right_point->position.y + left_point->position.y) * 0.5f +
									perpendicular.j * displacement;
								middle_point->position.z =
									(right_point->position.z + left_point->position.z) * 0.5f +
									perpendicular.k * displacement;
								middle_point->width =
									(right_point->width + left_point->width) * 0.5f;
								middle_point->color.alpha =
									(right_point->color.alpha + left_point->color.alpha) * 0.5f;
								middle_point->color.red =
									(right_point->color.red + left_point->color.red) * 0.5f;
								middle_point->color.green =
									(right_point->color.green + left_point->color.green) * 0.5f;
								middle_point->color.blue =
									(right_point->color.blue + left_point->color.blue) * 0.5f;
								middle_point->valid = TRUE;
								segment_point_index = (short)(
									segment_point_index + segment_point_start_index * 2);
							}
							while (segment_point_index < segment_point_count);
						}

						level_scale *= 0.5f;
						octave_index = (short)(octave_index - 1);
						remaining_octaves = (short)(remaining_octaves - 1);
					}
					while (remaining_octaves > 0);
				}

				point_count = (short)(point_count +
					(1 << marker_definition->octaves_to_next_marker));
			}
			else
			{
				rasterizer_globals.current_lock_operation = _rasterizer_lock_lightning;
				if (point_count > 2)
				{
					long dynamic_vertex_buffer_index;

					point_count = (short)(point_count + 1);
					dynamic_vertex_buffer_index = rasterizer_dynamic_vertices_new(
						_rasterizer_vertex_type_dynamic_unlit,
						point_count * 2);
					if (dynamic_vertex_buffer_index != NONE)
					{
						struct lightning_vertex *vertices = rasterizer_dynamic_vertices_lock(
							dynamic_vertex_buffer_index);
						real inverse_point_count = 1.0f / (real)point_count;
						real texture_offset = real_seed_random(
							get_global_local_random_seed_address());
						real thickness_scale = 1.0f;
						real_rgb_color const *tint_modulation = global_real_rgb_white;
						real brightness_scale = 1.0f;
						short vertex_index;
						real_rectangle3d bounds;

						match_assert(
							"c:\\halo\\SOURCE\\objects\\widgets\\lightning.c",
							0xEF,
							vertices);
						if (animation)
						{
							if (animation->values &&
								VALID_INDEX(definition->thickness_scale_source - 1, 4))
							{
								thickness_scale =
									animation->values[definition->thickness_scale_source - 1];
							}
							if (animation->colors &&
								VALID_INDEX(definition->tint_modulation_source - 1, 4))
							{
								tint_modulation =
									&animation->colors[definition->tint_modulation_source - 1];
							}
							if (animation->values &&
								VALID_INDEX(definition->brightness_scale_source - 1, 4))
							{
								brightness_scale =
									animation->values[definition->brightness_scale_source - 1];
							}
						}

						for (vertex_index = 0;
							vertex_index < point_count;
							vertex_index = (short)(vertex_index + 1))
						{
							struct intermediate_lightning_point *point = &points[vertex_index];
							struct intermediate_lightning_point *previous_point =
								vertex_index > 0 ? &points[vertex_index - 1] : point;
							struct intermediate_lightning_point *next_point =
								vertex_index < point_count - 1 ? &points[vertex_index + 1] : point;
							real width = thickness_scale * point->width;
							real texture_coordinate = (real)vertex_index * inverse_point_count;
							real_vector3d segment_delta;
							real_vector3d perpendicular;
							real_argb_color color;
							pixel32 pixel_color;
							struct lightning_vertex *vertex = &vertices[vertex_index * 2];

							match_assert(
								"c:\\halo\\SOURCE\\objects\\widgets\\lightning.c",
								0x118,
								points[vertex_index].valid);
							vector_from_points3d(
								&previous_point->position,
								&next_point->position,
								&segment_delta);
							cross_product3d(
								&segment_delta,
								&render.camera.forward,
								&perpendicular);
							fast_normalize3d(&perpendicular);
							color.alpha = brightness_scale * point->color.alpha;
							color.red = point->color.red * tint_modulation->red;
							color.green = point->color.green * tint_modulation->green;
							color.blue = point->color.blue * tint_modulation->blue;
							pixel_color = real_argb_color_to_pixel32(&color);
							vertex[0].point.x = perpendicular.i * width + point->position.x;
							vertex[0].point.y = perpendicular.j * width + point->position.y;
							vertex[0].point.z = perpendicular.k * width + point->position.z;
							vertex[0].texture.x = texture_coordinate + texture_offset;
							vertex[0].texture.y = 0.0f;
							vertex[0].color = pixel_color;
							vertex[1].point.x = perpendicular.i * -width + point->position.x;
							vertex[1].point.y = perpendicular.j * -width + point->position.y;
							vertex[1].point.z = perpendicular.k * -width + point->position.z;
							vertex[1].texture.x = texture_coordinate + texture_offset;
							vertex[1].texture.y = 1.0f;
							vertex[1].color = pixel_color;

							if (vertex_index == 0)
							{
								bounds.x0 = bounds.x1 = point->position.x;
								bounds.y0 = bounds.y1 = point->position.y;
								bounds.z0 = bounds.z1 = point->position.z;
							}
							else
							{
								bounds.x0 = MIN(bounds.x0, point->position.x);
								bounds.y0 = MIN(bounds.y0, point->position.y);
								bounds.z0 = MIN(bounds.z0, point->position.z);
								bounds.x1 = MAX(bounds.x1, point->position.x);
								bounds.y1 = MAX(bounds.y1, point->position.y);
								bounds.z1 = MAX(bounds.z1, point->position.z);
							}
						}

						{
							real_point3d centroid;
							struct shader const *shader;

							centroid.x = (bounds.x0 + bounds.x1) * 0.5f;
							centroid.y = (bounds.y0 + bounds.y1) * 0.5f;
							centroid.z = (bounds.z0 + bounds.z1) * 0.5f;
							if (definition->shaders.count > 0)
							{
								struct shader_effect_definition *effect = TAG_BLOCK_GET_ELEMENT(
									&definition->shaders,
									0,
									struct shader_effect_definition);

								shader = &effect->shader;
							}
							else
							{
								shader = &global_shader_effect_additive.shader;
							}

							rasterizer_dynamic_vertices_unlock(dynamic_vertex_buffer_index);
							rasterizer_dynamic_unlit_geometry_draw(
								shader,
								bitmap,
								animation,
								-point_count * 2,
								dynamic_vertex_buffer_index,
								point_count * 2 - 2,
								&centroid,
								0);
							rasterizer_dynamic_vertices_delete(dynamic_vertex_buffer_index);
						}
					}
					first_marker = TRUE;
				}
				rasterizer_globals.current_lock_operation = _rasterizer_lock_none;
			}
		}
	}

	return;
}
