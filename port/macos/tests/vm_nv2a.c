/*
VM_NV2A.C

The NV2A shader translators (port/linux/src/nv2a_vsh.c, nv2a_psh.c), as the
guest build compiles them, run in the macOS host's VM on known programs
(tools/test_nv2a_msl.py): each prints the shader it writes.

	vm_nv2a vertex glsl|msl <packed mask> <present mask> lit|unlit <word>...
		the vertex program of these instruction words (hex, after the
		program's header), its attributes in the masks (hex); lit: with the
		lighting nv2a_vertex_shader_lighting finds (exit 3 if none)
	vm_nv2a pixel glsl|msl <case>
		one of the pixel shader keys below
	vm_nv2a pixel glsl|msl mode <stage> <mode> <sampler>
		texture stage <stage> in <mode> (PSTextureModes' value) with a
		sampler of <sampler> (_xgpu_sampler_*), the stages before it 2D
		textures and the combiners using every texture
*/

#include "xgpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* (the game's platform layer, which nv2a_psh.c and the GLES path read) */
struct xgpu_capabilities xgpu_capabilities = { 0, 0, 0, 0, 0, 0, "300 es" };

int config_boolean(const char *name)
{
	(void)name;
	return 0;
}

const char *config_string(const char *name)
{
	(void)name;
	return "";
}

/* ---------- pixel shader keys */

/* a combiner input byte: a register (nv2a_psh.c), its alpha, its mapping */
#define INPUT(reg, alpha, mapping) ((DWORD)(reg) | ((alpha) ? 0x10 : 0) | (mapping))
#define ZERO 0
#define C0 1
#define FOG 3
#define V0 4
#define V1 5
#define T0 8
#define R0 12
#define INVERT 0x20
#define SIGNED 0xc0

/* stage 0's combiners: r0 = A * B, both portions, and the final combiner
writing r0 */
static void multiply(struct nv2a_pixel_shader_key *key, int stage, DWORD a, DWORD b)
{
	DWORD *state = key->combiner_state;

	state[D3DRS_PSRGBINPUTS0 + stage] = (a << 24) | (b << 16) | (INPUT(ZERO, 0, 0) << 8) | INPUT(ZERO, 0, 0);
	state[D3DRS_PSALPHAINPUTS0 + stage] = ((a | 0x10) << 24) | ((b | 0x10) << 16);
	state[D3DRS_PSRGBOUTPUTS0 + stage] = R0 << 4;
	state[D3DRS_PSALPHAOUTPUTS0 + stage] = R0 << 4;
	if ((state[D3DRS_PSCOMBINERCOUNT] & 0xff) < (DWORD)stage + 1)
		state[D3DRS_PSCOMBINERCOUNT] = (state[D3DRS_PSCOMBINERCOUNT] & ~0xffUL) | (DWORD)(stage + 1);
	/* rgb = 0 * 0 + 1 * 0 + r0 (fog's D...): A B C zero, D r0; G r0's alpha */
	state[D3DRS_PSFINALCOMBINERINPUTSABCD] = INPUT(R0, 0, 0);
	state[D3DRS_PSFINALCOMBINERINPUTSEFG] = INPUT(R0, 1, 0) << 8;
}

static void set_mode(struct nv2a_pixel_shader_key *key, int stage, unsigned long mode, unsigned char sampler)
{
	key->texture_modes = (key->texture_modes & ~(0x1fUL << (5 * stage))) | (mode << (5 * stage));
	key->sampler_type[stage] = sampler;
}

static int pixel_case(const char *name, struct nv2a_pixel_shader_key *key)
{
	memset(key, 0, sizeof(*key));
	if (!strcmp(name, "plain"))
	{
		/* no stages, no combiners: r0 */
	}
	else if (!strcmp(name, "diffuse"))
	{
		multiply(key, 0, INPUT(V0, 0, 0), INPUT(ZERO, 0, INVERT));
	}
	else if (!strcmp(name, "texture"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(V0, 0, 0));
	}
	else if (!strcmp(name, "fog"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(V0, 0, 0));
		/* rgb = fog.a * r0 + (1 - fog.a) * fog */
		key->combiner_state[D3DRS_PSFINALCOMBINERINPUTSABCD] =
			(INPUT(FOG, 1, 0) << 24) | (INPUT(R0, 0, 0) << 16) | (INPUT(FOG, 0, 0) << 8);
		key->fog_enable = 1;
		key->fog_table_mode = D3DFOG_LINEAR;
	}
	else if (!strcmp(name, "fog_exp2"))
	{
		multiply(key, 0, INPUT(V0, 0, 0), INPUT(C0, 0, SIGNED));
		key->fog_enable = 1;
		key->fog_table_mode = D3DFOG_EXP2;
	}
	else if (!strcmp(name, "mux"))
	{
		/* two stages, the second muxing on r0's alpha (its most significant
		bit), the first's AB a dot product; unique constants */
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0x40), INPUT(V1, 0, 0x40));
		key->combiner_state[D3DRS_PSRGBOUTPUTS0] |= 0x02UL << 12;
		multiply(key, 1, INPUT(R0, 0, 0), INPUT(C0, 0, 0));
		key->combiner_state[D3DRS_PSRGBOUTPUTS0 + 1] |= (0x04UL << 12) | (R0 << 8);
		key->combiner_state[D3DRS_PSCOMBINERCOUNT] |= 0x100 | 0x1000 | 0x10000;
	}
	else if (!strcmp(name, "mux_lsb"))
	{
		multiply(key, 0, INPUT(V0, 0, 0), INPUT(V1, 0, 0));
		key->combiner_state[D3DRS_PSRGBOUTPUTS0] |= (0x04UL << 12) | (R0 << 8) | (0x10UL << 12);
	}
	else if (!strcmp(name, "alpha_test"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(V0, 0, 0));
		key->alpha_test_function = D3DCMP_GREATER;
	}
	else if (!strcmp(name, "alpha_test_samples"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(V0, 0, 0));
		key->alpha_test_function = D3DCMP_GREATEREQUAL;
		key->alpha_test_samples = 4;
	}
	else if (!strcmp(name, "alpha_test_never"))
	{
		key->alpha_test_function = D3DCMP_NEVER;
	}
	else if (!strcmp(name, "alpha_kill"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(V0, 0, 0));
		key->alpha_kill[0] = 1;
		key->color_sign[0] = 0x0f;
	}
	else if (!strcmp(name, "lighting"))
	{
		multiply(key, 0, INPUT(V0, 0, 0), INPUT(ZERO, 0, INVERT));
		key->per_pixel_lighting = 1;
	}
	else if (!strcmp(name, "point_lighting"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(V0, 0, 0));
		key->per_pixel_lighting = 2;
	}
	else if (!strcmp(name, "meter"))
	{
		set_mode(key, 0, 0x01, _xgpu_sampler_2d);
		multiply(key, 0, INPUT(T0, 0, 0), INPUT(C0, 0, 0));
		key->point_threshold = 1;
		key->coverage_alpha = 1;
		key->source_blend = 2;
	}
	else if (!strcmp(name, "blend"))
	{
		multiply(key, 0, INPUT(V0, 0, 0), INPUT(ZERO, 0, INVERT));
		key->source_blend = 3;
	}
	else if (!strcmp(name, "count_samples"))
	{
		multiply(key, 0, INPUT(V0, 0, 0), INPUT(ZERO, 0, INVERT));
		key->count_samples = 1;
	}
	else
	{
		return 0;
	}
	return 1;
}

/* every stage before this one a 2D texture, this one in the mode, every
texture's color summed by the combiners (as the dot product and dependent
modes read the stages before them) */
static void mode_case(struct nv2a_pixel_shader_key *key, int stage, unsigned long mode, unsigned char sampler)
{
	int before;

	memset(key, 0, sizeof(*key));
	for (before = 0; before < stage; before++)
		set_mode(key, before, 0x01, _xgpu_sampler_2d);
	set_mode(key, stage, mode, sampler);
	for (before = 0; before <= stage; before++)
		multiply(key, before, INPUT(T0 + before, 0, 0), INPUT(R0, 0, before ? 0 : INVERT));
	/* stage 2 reads stage 1, stage 3 stage 2 (PSInputTexture) */
	key->combiner_state[D3DRS_PSINPUTTEXTURE] = (1UL << 16) | (2UL << 20);
	key->combiner_state[D3DRS_PSDOTMAPPING] = 0x1 | (0x2 << 4) | (0x3 << 8);
	key->combiner_state[D3DRS_PSCOMPAREMODE] = 0x5UL << (4 * stage);
}

/* ---------- main */

int main(int argc, char **argv)
{
	int msl;
	char *source;

	if (argc < 4 || (strcmp(argv[2], "glsl") && strcmp(argv[2], "msl")))
	{
		fprintf(stderr, "usage: vm_nv2a vertex|pixel glsl|msl ...\n");
		return 2;
	}
	msl = !strcmp(argv[2], "msl");
	if (!strcmp(argv[1], "vertex") && argc >= 6)
	{
		unsigned long packed = strtoul(argv[3], NULL, 16), present = strtoul(argv[4], NULL, 16);
		unsigned long count = (unsigned long)(argc - 6) / 4, index;
		DWORD *instructions = calloc(count * 4 + 4, sizeof(DWORD));
		struct nv2a_vertex_lighting lighting;
		BOOL lit = !strcmp(argv[5], "lit");

		for (index = 0; index < count * 4; index++)
			instructions[index] = (DWORD)strtoul(argv[6 + index], NULL, 16);
		if (lit && !nv2a_vertex_shader_lighting(instructions, count, &lighting))
		{
			fprintf(stderr, "the program does not light its models\n");
			return 3;
		}
		source = msl ?
			nv2a_vertex_shader_to_msl(instructions, count, packed, present, lit ? &lighting : NULL) :
			nv2a_vertex_shader_to_glsl(instructions, count, packed, lit ? &lighting : NULL);
	}
	else if (!strcmp(argv[1], "pixel"))
	{
		struct nv2a_pixel_shader_key key;

		if (!strcmp(argv[3], "mode") && argc == 7)
		{
			mode_case(&key, atoi(argv[4]), strtoul(argv[5], NULL, 0), (unsigned char)atoi(argv[6]));
		}
		else if (!pixel_case(argv[3], &key))
		{
			fprintf(stderr, "no pixel shader case %s\n", argv[3]);
			return 2;
		}
		source = msl ? nv2a_pixel_shader_to_msl(&key) : nv2a_pixel_shader_to_glsl(&key);
	}
	else
	{
		fprintf(stderr, "usage: vm_nv2a vertex|pixel glsl|msl ...\n");
		return 2;
	}
	if (!source)
		return 4;
	fputs(source, stdout);
	free(source);
	return 0;
}
