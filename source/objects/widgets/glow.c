/*
GLOW.C

symbols in this file:
00122790 0010:
	_glow_dispose (0000)
001227A0 0030:
	_glow_initialize_for_new_map (0000)
001227D0 0030:
	_glow_dispose_from_old_map (0000)
00122800 00f0:
	_glow_new (0000)
001228F0 0050:
	_glow_delete (0000)
00122940 0080:
	_code_00122940 (0000)
001229C0 0060:
	_code_001229c0 (0000)
00122A20 0090:
	_code_00122a20 (0000)
00122AB0 0040:
	_code_00122ab0 (0000)
00122AF0 0060:
	_code_00122af0 (0000)
00122B50 01a0:
	_code_00122b50 (0000)
00122CF0 0010:
	_code_00122cf0 (0000)
00122D00 0040:
	_code_00122d00 (0000)
00122D40 0030:
	_point_from_parametric_line (0000)
00122D70 00c0:
	_glow_render (0000)
00122E30 00c0:
	_nonuniform_cubic_spline (0000)
00122EF0 00b0:
	_nonuniform_cubic_spline_vector3d (0000)
00122FA0 0070:
	_glow_initialize (0000)
00123010 01e0:
	_code_00123010 (0000)
001231F0 06d0:
	_code_001231f0 (0000)
001238C0 0230:
	_code_001238c0 (0000)
00123AF0 00b0:
	_code_00123af0 (0000)
00123BA0 0260:
	_code_00123ba0 (0000)
00123E00 0530:
	_code_00123e00 (0000)
00124330 0070:
	_glow_submit (0000)
00289014 0012:
	??_C@_0BC@NKHKLAHF@t?$DO?$DN?5t0?5?$CG?$CG?5t?5?$DM?$DN?5t3?$AA@ (0000)
00289028 0023:
	??_C@_0CD@NAJLAIFJ@could?5not?5allocate?5glow?5data?5arr@ (0000)
0028904C 002c:
	??_C@_0CM@DGMFKJDA@could?5not?5allocate?5glow?5particle@ (0000)
00289078 000f:
	??_C@_0P@CJGCAKMC@glow?5particles?$AA@ (0000)
00289088 0005:
	??_C@_04EAMKIICI@glow?$AA@ (0000)
00289090 0026:
	??_C@_0CG@PCCALJDB@c?3?2halo?2SOURCE?2objects?2widgets?2g@ (0000)
002890B8 001c:
	??_C@_0BM@LFIJOPFP@glow?9?$DOnumber_of_markers?5?$DO?51?$AA@ (0000)
002890D4 0027:
	??_C@_0CH@HNGLDBKF@marker_index?$DMglow?9?$DOnumber_of_mar@ (0000)
002890FC 002d:
	??_C@_0CN@JJJNNAME@glow?5effect?5received?5illegal?5bou@ (0000)
0028912C 0028:
	??_C@_0CI@PJIOCDIO@unknown?5trailing?5particle?5distri@ (0000)
00289158 0047:
	??_C@_0EH@KHNBMDOO@the?5map?5limit?5for?5the?5number?5of?5@ (0000)
*/

/* ---------- headers */

#define nonuniform_cubic_spline glow_nonuniform_cubic_spline_inline
#define nonuniform_cubic_spline_vector3d glow_nonuniform_cubic_spline_vector3d_inline
#include "objects/widgets/glow.h"
#undef nonuniform_cubic_spline_vector3d
#undef nonuniform_cubic_spline

#include "bitmaps/bitmap_group.h"
#include "cseries/cseries.h"
#include "cseries/errors.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "saved games/game_state.h"

/* ---------- constants */

enum
{
	GLOW_TAG = 'glw!',
	_bitmap_group_type_sprites = 3,
};

/* ---------- macros */

/* ---------- structures */

struct glow_particle
{
	struct datum_header header;
	short parent_marker_index;
	long index;
	real initial_angle;
	real_argb_color color;
	real distance_to_object;
	real initial_size;
	real present_size;
	real t;
	real_point3d position;
	real_vector3d initial_velocity;
	real_vector3d present_velocity;
	short ticks_in_existence;
	short lifetime;
	unsigned long flags;
	real fade;
	struct glow_particle *next;
	struct glow_particle *previous;
};

struct glow_datum
{
	struct datum_header header;
	boolean initialized;
	byte pad3;
	short number_of_markers;
	short pad6;
	struct object_marker markers[5];
	long definition_index;
	short bitmap_dimension;
	short marker_order[5];
	real total_time;
	real marker_time_index[5];
	short number_of_particles;
	short pad24E;
	struct glow_particle *head_particle;
	struct glow_particle *tail_particle;
	short accumulated_trailing_particle_generation_ticks;
};

struct glow_definition
{
	char attachment_marker[32];
	short number_of_particles;
	byte _unknown022[0x122];
	struct tag_reference texture;
};

struct bitmap_group_sprite
{
	short bitmap_index;
	word pad02;
	long unknown004;
	real_rectangle2d bounds;
	real_point2d registration_point;
};

struct bitmap_group_sequence
{
	char name[32];
	short first_bitmap_index;
	short bitmap_count;
	long unknown024[4];
	struct tag_block sprites;
};

typedef char glow_datum_definition_index_offset_assert[
	offsetof(struct glow_datum, definition_index) == 0x224 ? 1 : -1];
typedef char glow_datum_bitmap_dimension_offset_assert[
	offsetof(struct glow_datum, bitmap_dimension) == 0x228 ? 1 : -1];
typedef char glow_datum_number_of_particles_offset_assert[
	offsetof(struct glow_datum, number_of_particles) == 0x24C ? 1 : -1];
typedef char glow_datum_size_assert[
	sizeof(struct glow_datum) == 0x25C ? 1 : -1];
typedef char glow_definition_texture_offset_assert[
	offsetof(struct glow_definition, texture) == 0x144 ? 1 : -1];
typedef char glow_definition_size_assert[
	sizeof(struct glow_definition) == 0x154 ? 1 : -1];
typedef char bitmap_group_sequences_offset_assert[
	offsetof(struct bitmap_group, sequences) == 0x54 ? 1 : -1];
typedef char bitmap_group_sprite_size_assert[
	sizeof(struct bitmap_group_sprite) == 0x20 ? 1 : -1];
typedef char bitmap_group_sequence_size_assert[
	sizeof(struct bitmap_group_sequence) == 0x40 ? 1 : -1];

/* ---------- prototypes */

struct bitmap_data *bitmap_group_get_bitmap_from_sequence(
	long bitmap_group_index,
	short sequence_index,
	short frame_index);

/* ---------- globals */

/* ---------- public code */

void glow_initialize(
	void)
{
	if (!glow_globals.glow_data)
	{
		glow_globals.glow_data = game_state_data_new("glow", 8, 0x25C);
		if (glow_globals.glow_data)
		{
			if (!glow_globals.glow_particle_data)
			{
				glow_globals.glow_particle_data = game_state_data_new("glow particles", 512, 100);
				if (!glow_globals.glow_particle_data)
				{
					error(_error_silent, "could not allocate glow particle data array");
					return;
				}
			}
		}
		else
		{
			error(_error_silent, "could not allocate glow data array");
		}
	}

	return;
}

void glow_initialize_for_new_map(
	void)
{
	if (glow_globals.glow_data)
		data_make_valid(glow_globals.glow_data);
	if (glow_globals.glow_particle_data)
		data_make_valid(glow_globals.glow_particle_data);

	return;
}

void glow_dispose_from_old_map(
	void)
{
	if (glow_globals.glow_data)
		data_make_invalid(glow_globals.glow_data);
	if (glow_globals.glow_particle_data)
		data_make_invalid(glow_globals.glow_particle_data);

	return;
}

void glow_dispose(
	void)
{
	return;
}

long glow_new(
	long definition_index)
{
	long glow_index = NONE;

	if (definition_index != NONE)
	{
		glow_index = datum_new(glow_globals.glow_data);
		if (glow_index != NONE)
		{
			struct glow_datum *glow =
				(struct glow_datum *)datum_get(
					glow_globals.glow_data,
					glow_index);
			struct glow_definition *definition =
				(struct glow_definition *)tag_get(GLOW_TAG, definition_index);
			struct bitmap_group *sprite_group =
				bitmap_group_get(definition->texture.index);

			if (sprite_group->type == _bitmap_group_type_sprites)
			{
				struct bitmap_group_sprite *sprite = TAG_BLOCK_GET_ELEMENT(
					&TAG_BLOCK_GET_ELEMENT(
						&sprite_group->sequences,
						0,
						struct bitmap_group_sequence)->sprites,
					0,
					struct bitmap_group_sprite);
				struct bitmap_data *bitmap =
					bitmap_group_get_bitmap_from_sequence(
						definition->texture.index,
						0,
						sprite->bitmap_index);

				glow->definition_index = definition_index;
				glow->number_of_particles = definition->number_of_particles;
				glow->bitmap_dimension = (short)(long)(
					(sprite->bounds.x1 - sprite->bounds.x0) * bitmap->width);
			}
		}
	}

	return glow_index;
}

void glow_delete(
	long glow_index)
{
	struct glow_datum *glow = datum_get(glow_globals.glow_data, glow_index);
	struct glow_particle *particle = glow->head_particle;

	while (particle)
	{
		struct glow_particle *next = particle->next;

		datum_delete(glow_globals.glow_particle_data, particle->index);
		particle = next;
	}

	datum_delete(glow_globals.glow_data, glow_index);
	return;
}

/* ---------- private code */

void point_from_parametric_line(
	real_point3d const *point,
	real_vector3d const *forward,
	real t,
	real_point3d *result)
{
	result->x = forward->i * t + point->x;
	result->y = forward->j * t + point->y;
	result->z = forward->k * t + point->y;

	return;
}

real nonuniform_cubic_spline(
	real f0,
	real f1,
	real f2,
	real f3,
	real t0,
	real t1,
	real t2,
	real t3,
	real t)
{
	match_assert("..\\math\\real_math.h", 1530, t>= t0 && t <= t3);

	f3 = (f3 - f2) / (t3 - t2);
	f2 = (f2 - f1) / (t2 - t1);
	f1 = (f1 - f0) / (t1 - t0);
	f3 = (f3 - f2) / (t3 - t1);
	f2 = (f2 - f1) / (t2 - t0);
	f3 = (f3 - f2) / (t3 - t0);

	return f0 + (t - t0) * (f1 + (t - t1) * (f2 + (t - t2) * f3));
}

void nonuniform_cubic_spline_vector3d(
	real_vector3d *result,
	real_vector3d const *f0,
	real_vector3d const *f1,
	real_vector3d const *f2,
	real_vector3d const *f3,
	real t0,
	real t1,
	real t2,
	real t3,
	real t)
{
	result->i = nonuniform_cubic_spline(
		f0->i,
		f1->i,
		f2->i,
		f3->i,
		t0,
		t1,
		t2,
		t3,
		t);
	result->j = nonuniform_cubic_spline(
		f0->j,
		f1->j,
		f2->j,
		f3->j,
		t0,
		t1,
		t2,
		t3,
		t);
	result->k = nonuniform_cubic_spline(
		f0->k,
		f1->k,
		f2->k,
		f3->k,
		t0,
		t1,
		t2,
		t3,
		t);

	return;
}
