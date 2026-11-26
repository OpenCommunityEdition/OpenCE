/* ---------- headers */

#include "cseries.h"
#include "real_math.h"
#include "shader_definitions.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

struct shader_effect_definition global_shader_effect_additive =
{
	{
		{
			0,
			0,
			0.0f,
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			0,
			0,
			1,
			0
		}
	},
	0,
	3,
	0,
	0,
	{ 0 },
	{ 'bitm', "", 0, NONE },
	{ 0 }
};

struct shader_effect_definition global_shader_effect_alpha_blended =
{
	{
		{
			0,
			0,
			0.0f,
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			0,
			0,
			1,
			0
		}
	},
	0,
	0,
	0,
	0,
	{ 0 },
	{ 'bitm', "", 0, NONE },
	{ 0 }
};

/* ---------- public code */

struct shader *shader_get_and_verify_type(struct shader *shader, short shader_type)
{
	match_assert("c:\\halo\\SOURCE\\shaders\\shader_definitions.c", 2140, shader);
	match_assert("c:\\halo\\SOURCE\\shaders\\shader_definitions.c", 2141, shader->base.type==shader_type);
	return shader;
}

/* ---------- private code */
