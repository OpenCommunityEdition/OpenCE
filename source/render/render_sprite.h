/*
RENDER_SPRITE.H

header included in hcex build.
*/

#ifndef __RENDER_SPRITE_H
#define __RENDER_SPRITE_H
#pragma once

/* ---------- headers */

#include "math/real_math.h"
#include "shaders/shader_definitions.h"

/* ---------- structures */

struct build_sprite_data
{
	long bitmap_group_index;
	short maximum_sprite_count;
	short screen_space;
	struct shader_effect_definition const *shader;
	short sprite_count;
	word pad0E;
	unsigned long flags;
	real_point3d centroid;
	short group_count;
	word pad22;
	byte unknown024[0x80];
};

typedef char build_sprite_data_size_assert[
	sizeof(struct build_sprite_data) == 0xA4 ? 1 : -1];

/* ---------- prototypes/RENDER_SPRITE.C */

void build_sprites_begin(
	struct build_sprite_data *data,
	short maximum_sprite_count,
	long bitmap_group_index,
	struct shader_effect_definition const *shader,
	unsigned long flags);
void build_sprite(
	struct build_sprite_data *data,
	short mode,
	short sequence_index,
	short sprite_index,
	real_point3d const *untransformed_origin,
	real_vector3d const *untransformed_direction,
	real rotation,
	real scale,
	real_argb_color const *color,
	real fade,
	unsigned long flags);
void build_sprites_end(
	struct build_sprite_data *data);

#endif // __RENDER_SPRITE_H
