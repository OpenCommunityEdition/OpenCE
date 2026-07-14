/*
SHADER_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __SHADER_DEFINITIONS_H
#define __SHADER_DEFINITIONS_H
#pragma once

/* ---------- headers */

#include "real_math.h"
#include "tag_groups.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

struct shader_base
{
	unsigned short flags;
	short detail_level;
	real power;
	real_rgb_color color_of_emitted_light;
	real_rgb_color tint_color;
	unsigned short physics_flags;
	short material_type;
	short type;
	short pad;
};

struct shader
{
	struct shader_base base;
};

struct shader_effect_definition
{
	struct shader shader;
	byte numeric_counter_limit;
	byte flags;
	short framebuffer_blend_function;
	byte reserved_before_bitmap[32];
	struct tag_reference bitmap;
	byte reserved_runtime[88];
};

/* ---------- prototypes/SHADER_DEFINITIONS.C */

struct shader *shader_get_and_verify_type(struct shader *shader, short shader_type);

/* ---------- globals */

extern struct shader_effect_definition global_shader_effect_additive;
extern struct shader_effect_definition global_shader_effect_alpha_blended;

/* ---------- public code */

#endif // __SHADER_DEFINITIONS_H
