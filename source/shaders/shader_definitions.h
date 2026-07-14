/*
SHADER_DEFINITIONS.H

header included in hcex build.
*/

#ifndef __SHADER_DEFINITIONS_H
#define __SHADER_DEFINITIONS_H
#pragma once

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

/* ---------- prototypes/SHADER_DEFINITIONS.C */

struct shader *shader_get_and_verify_type(struct shader *shader, short shader_type);

/* ---------- globals */

/* ---------- public code */

#endif // __SHADER_DEFINITIONS_H
