/*
ANTENNA.C

symbols in this file:
00120710 0030:
	_antennas_initialize (0000)
00120740 0010:
	_antennas_initialize_for_new_map (0000)
00120750 0010:
	_antennas_dispose_from_old_map (0000)
00120760 0020:
	_antennas_dispose (0000)
00120780 0200:
	_antenna_new (0000)
00120980 0020:
	_antenna_delete (0000)
001209A0 0130:
	_antenna_update_attachment (0000)
00120AD0 0170:
	_antenna_render_proper (0000)
00120C40 0310:
	_antenna_update (0000)
00120F50 0090:
	_antenna_render (0000)
00120FE0 00b0:
	_antennas_update (0000)
00288EEC 0022:
	??_C@_0CC@JBAGLFPJ@couldn?8t?5allocate?5antenna?5global@ (0000)
00288F10 0008:
	??_C@_07HJCCDMBN@antenna?$AA@ (0000)
*/

/* ---------- headers */

#include "objects/widgets/antenna.h"

#include "bitmaps/bitmap_group.h"
#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "physics/point_physics.h"
#include "render/render_sprite.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"
#include "shaders/shader_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* A chain's shape after each of the last two ticks, relative to the point
its object's marker carries the base by: a frame between the ticks is drawn
with the part of the way between those two shapes that it is, as an object is
drawn between its own (port/linux/game/render_interpolation.c). */
struct antenna_shape
{
	short count;
	boolean has_previous;
	real_point3d previous[MAXIMUM_ANTENNA_VERTICES + 1];
	real_point3d latest[MAXIMUM_ANTENNA_VERTICES + 1];
};

/* ---------- prototypes */

struct bitmap_data *bitmap_group_try_and_get_bitmap(
	long bitmap_group_index,
	short bitmap_index);

static void antenna_update_attachment(
	struct antenna_datum *antenna,
	struct antenna_definition *definition,
	struct location *attachment_location,
	real_point3d *attachment_point,
	real_vector3d *attachment_vector);
static void antenna_record_shape(
	long antenna_index,
	struct antenna_datum *antenna,
	short count);
static void antenna_render_proper(
	long antenna_index,
	struct antenna_datum *antenna,
	struct antenna_definition *definition);
static void antenna_update(
	struct antenna_datum *antenna,
	struct antenna_definition *definition,
	real delta);

/* ---------- globals */

struct data_array *antenna_data;
static struct antenna_shape antenna_shapes[MAXIMUM_ANTENNAS];

/* ---------- public code */

void antennas_initialize(
	void)
{
	antenna_data = game_state_data_new(
		"antenna",
		MAXIMUM_ANTENNAS,
		sizeof(struct antenna_datum));
	if (!antenna_data)
		error(_error_immediate, "couldn't allocate antenna globals");

	return;
}

void antennas_initialize_for_new_map(
	void)
{
	data_make_valid(antenna_data);

	return;
}

void antennas_dispose_from_old_map(
	void)
{
	data_make_invalid(antenna_data);

	return;
}

void antennas_dispose(
	void)
{
	if (antenna_data)
		antenna_data = NULL;

	return;
}

void antenna_delete(
	long antenna_index)
{
	datum_delete(antenna_data, antenna_index);

	return;
}

long antenna_new(
	long definition_index)
{
	long antenna_index = NONE;

	if (definition_index != NONE)
	{
		struct antenna_definition *definition = antenna_definition_get(definition_index);

		antenna_index = datum_new(antenna_data);
		if (antenna_index != NONE)
		{
			struct antenna_datum *antenna = antenna_get(antenna_index);
			real_point3d position;
			short vertex_index;

			antenna->initialized = FALSE;
			/* port: and no more vertices than the antenna holds, with the tip
			past the last (a map's count; retail has up to 7) */
			antenna->disabled = definition->vertices.count < 2 ||
				definition->vertices.count > MAXIMUM_ANTENNA_VERTICES;
			if (definition->vertices.count > MAXIMUM_ANTENNA_VERTICES)
			{
				static boolean vertex_count_reported = FALSE;

				if (!vertex_count_reported)
				{
					vertex_count_reported = TRUE;
					error(
						_error_silent,
						"### ERROR an antenna has %ld vertices; it isn't drawn",
						definition->vertices.count);
				}
			}
			antenna->definition_index = definition_index;
			antenna->object_index = NONE;
			antenna->updates_since_last_render = 0;
			/* (a new chain blends from no shape: the slot's record may be a
			deleted antenna's, or an earlier map's) */
			antenna_shapes[DATUM_INDEX_TO_ABSOLUTE_INDEX(antenna_index)].count = 0;
			antenna->last_attachment_location.z = 0.0f;
			antenna->last_attachment_location.y = 0.0f;
			antenna->last_attachment_location.x = 0.0f;
			position = antenna->last_attachment_location;

			vertex_index = 0;
			if (definition->vertices.count > 0)
			{
				do
				{
					struct antenna_vertex_datum *vertex = &antenna->vertices[vertex_index];
					struct antenna_vertex_definition *definition_vertex = TAG_BLOCK_GET_ELEMENT(
						&definition->vertices,
						vertex_index,
						struct antenna_vertex_definition);

					vertex->velocity.k = 0.0f;
					vertex->velocity.j = 0.0f;
					vertex->velocity.i = 0.0f;
					vertex->sprite_index = 0;
					vertex->sprite_scale = 0.0f;
					vertex->position = position;

					if (definition->texture.index != NONE)
					{
						struct bitmap_group *bitmap_group = bitmap_group_get(definition->texture.index);

						if (definition_vertex->sequence_index >= 0 &&
							definition_vertex->sequence_index < bitmap_group->sequences.count)
						{
							struct bitmap_group_sequence *sequence = TAG_BLOCK_GET_ELEMENT(
								&bitmap_group->sequences,
								definition_vertex->sequence_index,
								struct bitmap_group_sequence);

							if (sequence->sprites.count)
							{
								struct bitmap_group_sprite *sprite = TAG_BLOCK_GET_ELEMENT(
									&sequence->sprites,
									0,
									struct bitmap_group_sprite);
								struct bitmap_data *bitmap = bitmap_group_try_and_get_bitmap(
									definition->texture.index,
									sprite->bitmap_index);

								if (bitmap)
								{
									real denominator =
										(sprite->bounds.x1 - sprite->bounds.x0) * bitmap->width -
										2.0f * bitmap_group->sprite_spacing - 1.0f;

									vertex->sprite_scale = definition_vertex->length_to_next / denominator;
								}
							}
						}
					}

					position.x += definition_vertex->vector_to_next.i;
					position.y += definition_vertex->vector_to_next.j;
					position.z += definition_vertex->vector_to_next.k;
					vertex_index++;
				}
				/* port: the vertices the antenna holds (a map's count; it is
				disabled past them) */
				while (vertex_index < MIN(definition->vertices.count, MAXIMUM_ANTENNA_VERTICES));
			}

			{
				struct antenna_vertex_datum *vertex = &antenna->vertices[vertex_index];

				vertex->position = position;
				vertex->velocity.k = 0.0f;
				vertex->velocity.j = 0.0f;
				vertex->velocity.i = 0.0f;
			}
		}
	}

	return antenna_index;
}

void antenna_render(
	long object_index,
	long antenna_index,
	struct render_lighting const *lighting,
	struct render_animation const *animation)
{
	struct antenna_datum *antenna;
	struct antenna_definition *definition;

	object_get(object_index);
	antenna = antenna_get(antenna_index);
	definition = antenna_definition_get(antenna->definition_index);

	if (!antenna->disabled)
	{
		antenna->object_index = object_index;
		if (antenna->updates_since_last_render > 5)
		{
			antenna_update(antenna, definition, 0.05f);
			antenna_update(antenna, definition, 0.05f);
			antenna_update(antenna, definition, 0.05f);
		}

		antenna->updates_since_last_render = 0;
		antenna_render_proper(antenna_index, antenna, definition);
	}

	return;
}

void antennas_update(
	real delta)
{
	long antenna_index;

	for (antenna_index = data_next_index(antenna_data, NONE);
		antenna_index != NONE;
		antenna_index = data_next_index(antenna_data, antenna_index))
	{
		struct antenna_datum *antenna = antenna_get(antenna_index);
		struct antenna_definition *definition = antenna_definition_get(antenna->definition_index);

		if (!antenna->disabled)
		{
			antenna->updates_since_last_render++;
			if (antenna->object_index != NONE && antenna->updates_since_last_render < 5)
			{
				/* The chain is a simulation of the Xbox's own step, an update
				a tick: its springs, its carry and its points' physics are
				per-update quantities (game.c steps it in the tick, not in
				game_frame with the frames' own movement). */
				antenna_update(antenna, definition, MIN(delta, 1.0f / 15.0f));
				/* what this tick leaves a frame between ticks to be drawn in
				(antenna_render_proper) */
				antenna_record_shape(
					antenna_index,
					antenna,
					(short)(definition->vertices.count + 1));
			}
		}
	}

	return;
}

/* ---------- private code */

static void antenna_update_attachment(
	struct antenna_datum *antenna,
	struct antenna_definition *definition,
	struct location *attachment_location,
	real_point3d *attachment_point,
	real_vector3d *attachment_vector)
{
	struct object_marker marker;
	real_vector3d delta;

	object_get_marker_by_name(antenna->object_index, definition->attachment_marker, &marker, 1);
	*attachment_point = marker.matrix.position;
	*attachment_vector = marker.matrix.forward;
	scenario_location_from_point(attachment_location, &marker.matrix.position);

	delta.i = attachment_point->x - antenna->last_attachment_location.x;
	delta.j = attachment_point->y - antenna->last_attachment_location.y;
	delta.k = attachment_point->z - antenna->last_attachment_location.z;
	/* port: the movement compared as it is (it was truncated to whole units
	first, so it took two units of movement to move the vertices along) */
	if (fabsf(delta.i) > 1.0f ||
		fabsf(delta.j) > 1.0f ||
		fabsf(delta.k) > 1.0f)
	{
		short vertex_index;

		for (vertex_index = 0;
			vertex_index < definition->vertices.count + 1;
			vertex_index++)
		{
			real_point3d *vertex_position = &antenna->vertices[vertex_index].position;

			vertex_position->x += delta.i;
			vertex_position->y += delta.j;
			vertex_position->z += delta.k;
		}
	}

	antenna->last_attachment_location = *attachment_point;

	return;
}

/* the shape this tick leaves the chain in, relative to the point the base
was put at (antenna_update_attachment): a tick on from the last one, so a
frame between the two is drawn from both (antenna_render_proper) */
static void antenna_record_shape(
	long antenna_index,
	struct antenna_datum *antenna,
	short count)
{
	long absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(antenna_index);
	struct antenna_shape *shape;
	real_point3d const *base = &antenna->vertices[0].position;
	short vertex_index;

	if (count <= 0 || count > MAXIMUM_ANTENNA_VERTICES + 1 ||
		absolute_index < 0 || absolute_index >= MAXIMUM_ANTENNAS)
	{
		return;
	}

	shape = &antenna_shapes[absolute_index];
	if (shape->count == count)
	{
		csmemcpy(shape->previous, shape->latest, sizeof(real_point3d) * count);
		shape->has_previous = TRUE;
	}
	else
	{
		shape->count = count;
		shape->has_previous = FALSE;
	}

	for (vertex_index = 0; vertex_index < count; vertex_index++)
	{
		real_point3d const *position = &antenna->vertices[vertex_index].position;

		shape->latest[vertex_index].x = position->x - base->x;
		shape->latest[vertex_index].y = position->y - base->y;
		shape->latest[vertex_index].z = position->z - base->z;
	}

	return;
}

static void antenna_render_proper(
	long antenna_index,
	struct antenna_datum *antenna,
	struct antenna_definition *definition)
{
	struct tag_block *vertices = &definition->vertices;

	if (vertices->count)
	{
		struct build_sprite_data sprite_data;
		real falloff_scale =
			(100.0f - definition->cutoff_pixels) /
			(definition->falloff_pixels - definition->cutoff_pixels);
		struct antenna_shape const *shape =
			&antenna_shapes[DATUM_INDEX_TO_ABSOLUTE_INDEX(antenna_index)];
		real_point3d chain[MAXIMUM_ANTENNA_VERTICES + 1];
		real fraction = render_interpolation_fraction();
		real_point3d base = antenna->vertices[0].position;
		short chain_count = (short)(vertices->count + 1);
		short vertex_index;

		if (falloff_scale < 0.0f)
			falloff_scale = 0.0f;
		else if (falloff_scale > 1.0f)
			falloff_scale = 1.0f;

		if (chain_count > MAXIMUM_ANTENNA_VERTICES + 1)
			chain_count = MAXIMUM_ANTENNA_VERTICES + 1;

		/* The frames between the ticks (port/linux/game/render_interpolation.c):
		the ticks left the chain's shape, a frame is drawn the part of the way
		between the last two of them that it is, and the point its vehicle's
		marker is at now carries the whole of it. The ticks' own base is what
		the shape was recorded against, so with no such record the shape the
		last tick left is drawn instead. */
		if (antenna->object_index != NONE)
		{
			struct object_marker marker;

			object_get_marker_by_name(
				antenna->object_index,
				definition->attachment_marker,
				&marker,
				1);
			base = marker.matrix.position;
		}

		for (vertex_index = 0; vertex_index < chain_count; vertex_index++)
		{
			real_point3d offset;

			if (shape->count == chain_count && shape->has_previous)
			{
				offset.x = shape->previous[vertex_index].x +
					(shape->latest[vertex_index].x - shape->previous[vertex_index].x) * fraction;
				offset.y = shape->previous[vertex_index].y +
					(shape->latest[vertex_index].y - shape->previous[vertex_index].y) * fraction;
				offset.z = shape->previous[vertex_index].z +
					(shape->latest[vertex_index].z - shape->previous[vertex_index].z) * fraction;
			}
			else
			{
				offset.x = antenna->vertices[vertex_index].position.x - antenna->vertices[0].position.x;
				offset.y = antenna->vertices[vertex_index].position.y - antenna->vertices[0].position.y;
				offset.z = antenna->vertices[vertex_index].position.z - antenna->vertices[0].position.z;
			}

			chain[vertex_index].x = base.x + offset.x;
			chain[vertex_index].y = base.y + offset.y;
			chain[vertex_index].z = base.z + offset.z;
		}

		build_sprites_begin(
			&sprite_data,
			(short)vertices->count,
			definition->texture.index,
			&global_shader_effect_alpha_blended,
			0);

		for (vertex_index = 0;
			vertex_index < chain_count - 1;
			vertex_index = (short)(vertex_index + 1))
		{
			struct antenna_vertex_datum *vertex = &antenna->vertices[vertex_index];
			struct antenna_vertex_definition *definition_vertex = TAG_BLOCK_GET_ELEMENT(
				vertices,
				vertex_index,
				struct antenna_vertex_definition);
			real_vector3d direction;
			real_argb_color color;

			direction.i = chain[vertex_index + 1].x - chain[vertex_index].x;
			direction.j = chain[vertex_index + 1].y - chain[vertex_index].y;
			direction.k = chain[vertex_index + 1].z - chain[vertex_index].z;
			color = definition_vertex->color;

			if (vertex->sprite_scale != 0.0f && falloff_scale > 0.0f)
			{
				build_sprite(
					&sprite_data,
					1,
					definition_vertex->sequence_index,
					0,
					&chain[vertex_index],
					&direction,
					0.0f,
					vertex->sprite_scale,
					&color,
					falloff_scale,
					0);
			}
		}

		build_sprites_end(&sprite_data);
	}

	return;
}

static void antenna_update(
	struct antenna_datum *antenna,
	struct antenna_definition *definition,
	real delta)
{
	struct location attachment_location;
	real_point3d attachment_point;
	real_vector3d attachment_vector;

	antenna_update_attachment(
		antenna,
		definition,
		&attachment_location,
		&attachment_point,
		&attachment_vector);

	if (!antenna->disabled && delta > 0.0f)
	{
		real_point3d previous_position;
		real_vector3d carried_tip;
		short vertex_index;

		for (vertex_index = 0;
			vertex_index < definition->vertices.count + 1;
			vertex_index++)
		{
			struct antenna_vertex_datum *vertex = &antenna->vertices[vertex_index];
			long definition_vertex_index =
				vertex_index == definition->vertices.count ?
					definition->vertices.count - 1 : vertex_index;
			struct antenna_vertex_definition *definition_vertex = TAG_BLOCK_GET_ELEMENT(
				&definition->vertices,
				definition_vertex_index,
				struct antenna_vertex_definition);
			real spring = definition->spring_coefficient * definition_vertex->spring_coefficient;
			real inverse_delta = 1.0f / delta;
			/* The spring pulls the same fraction of the way each update, an
			update a tick on the Xbox; the native builds update every frame,
			several a tick (port/linux/game/render_interpolation.c), so pull
			as far as a tick's worth would. */
			if (spring > 0.0f)
			{
				spring = (real)pow(spring, delta * TICKS_PER_SECOND);
			}
			real_point3d position;
			real_vector3d segment;
			real_vector3d up_axis;
			real_vector3d perpendicular;

			vertex->sprite_index++;
			if (vertex_index == 0)
			{
				position = attachment_point;
				segment = attachment_vector;
			}
			else
			{
				real_vector3d offset;
				real scale;
				real_point3d target;

				position = vertex->position;
				point_physics_update(
					0,
					point_physics_definition_get(definition->physics.index),
					&attachment_location,
					NONE,
					&position,
					&vertex->velocity,
					NULL,
					NULL,
					NULL,
					0.02f,
					delta);

				offset.i = position.x - previous_position.x;
				offset.j = position.y - previous_position.y;
				offset.k = position.z - previous_position.z;
				scale = definition_vertex->length_to_next / magnitude3d(&offset);
				target.x = previous_position.x + scale * offset.i;
				target.y = previous_position.y + scale * offset.j;
				target.z = previous_position.z + scale * offset.k;
				position.x = (1.0f - spring) * target.x + spring * carried_tip.i;
				position.y = (1.0f - spring) * target.y + spring * carried_tip.j;
				position.z = (1.0f - spring) * target.z + spring * carried_tip.k;
				segment.i = position.x - previous_position.x;
				segment.j = position.y - previous_position.y;
				segment.k = position.z - previous_position.z;
			}

			up_axis.i = 0.0f;
			up_axis.j = 0.0f;
			up_axis.k = 1.0f;
			cross_product3d(&up_axis, &segment, &perpendicular);
			if (normalize3d(&perpendicular) == 0.0f)
				perpendicular = *global_left3d;

			{
				real_vector3d sprite_vector = definition_vertex->vector_to_next;

				{
					real angle = angle_between_vectors3d(&up_axis, &segment);

					rotate_vector_about_axis(
						&sprite_vector,
						&perpendicular,
						sine(angle),
						cosine(angle));
				}

				carried_tip.i = position.x + sprite_vector.i;
				carried_tip.j = position.y + sprite_vector.j;
				carried_tip.k = position.z + sprite_vector.k;
			}
			previous_position = position;
			vertex->velocity.i = (position.x - vertex->position.x) * inverse_delta;
			vertex->velocity.j = (position.y - vertex->position.y) * inverse_delta;
			vertex->velocity.k = (position.z - vertex->position.z) * inverse_delta;
			vertex->position = position;
		}
	}

	return;
}
