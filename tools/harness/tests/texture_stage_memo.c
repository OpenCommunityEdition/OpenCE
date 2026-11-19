#include "harness.h"

/* The real bind_textures and its stage memo (d3d8_gl.c) against a texture
cache and sampler objects of its own: a stage bound again as it was is given
what its lookup gave without one; any change to what the lookup or the
sampler is chosen by looks them up again. */

typedef unsigned int DWORD, GLuint, GLenum, D3DCOLOR;
typedef int GLsizei, BOOL;
#define GL_TEXTURE_2D 0x0de1
#define GL_TEXTURE_3D 0x806f
#define GL_TEXTURE_CUBE_MAP 0x8513
#define PLATFORM_PHYSICAL_TO_VIRTUAL(address) ((void *)(unsigned long)(address))

enum
{
	D3DTSS_ADDRESSU = 10, D3DTSS_ADDRESSV = 11, D3DTSS_ADDRESSW = 12, D3DTSS_MAGFILTER = 13,
	D3DTSS_MINFILTER = 14, D3DTSS_MIPFILTER = 15, D3DTSS_MIPMAPLODBIAS = 16, D3DTSS_MAXMIPLEVEL = 17,
	D3DTSS_MAXANISOTROPY = 18, D3DTSS_BORDERCOLOR = 29, D3DTSS_MAX = 32
};
#define D3DTSS_MAXSTAGES 4
enum { D3DRS_PSTEXTUREMODES, D3DRS_MAX };
DWORD D3D__RenderState[D3DRS_MAX];
DWORD D3D__TextureState[D3DTSS_MAXSTAGES][D3DTSS_MAX];

typedef struct { DWORD Common, Data, Lock, Format, Size; } D3DBaseTexture;
typedef struct { DWORD Common, Data, Lock; } D3DPalette;
static struct
{
	D3DBaseTexture *textures[D3DTSS_MAXSTAGES];
	D3DPalette *palettes[D3DTSS_MAXSTAGES];
} device;

enum { _xgpu_sampler_none, _xgpu_sampler_2d, _xgpu_sampler_3d, _xgpu_sampler_cube };
struct nv2a_pixel_shader_key
{
	unsigned char sampler_type[4];
	unsigned char coverage_alpha, point_threshold, ce_meter_channels;
};
struct xgpu_texture_description
{
	DWORD format;
	unsigned long width, height, depth, levels;
	BOOL cube_map, linear, hires, hires_coverage, hires_point_threshold, ce_meter_channels, direct;
};
struct xgpu_render_target { GLuint texture; unsigned long width, height; };

/* what the lookups and the bindings saw */
static unsigned long watch_serial = 1, lookup_serial = 1, config_serial = 1;
static unsigned long lookups, sampler_choices;
static BOOL texture_direct = TRUE;
static struct xgpu_render_target *render_target;
static GLuint bound_samplers[D3DTSS_MAXSTAGES];
static struct { GLuint textures[D3DTSS_MAXSTAGES][3]; } gl_state;

static unsigned long memory_watch_serial(void) { return watch_serial; }
static unsigned long xgpu_texture_lookup_serial(void) { return lookup_serial; }
static unsigned long config_changes(void) { return config_serial; }
static struct xgpu_render_target *xgpu_render_target_find(unsigned long data) { return render_target; }
static void render_target_resolve(struct xgpu_render_target *target) { }
static GLuint mip_composite_get(const struct xgpu_texture_description *description, DWORD data) { return 0; }
static int texture_slot(GLenum target)
{
	return target == GL_TEXTURE_CUBE_MAP ? 1 : target == GL_TEXTURE_3D ? 2 : 0;
}
static void glBindTextures(GLuint first, GLsizei count, const GLuint *textures) { }
static void state_sampler(int unit, GLuint sampler) { bound_samplers[unit] = sampler; }

/* the description of a header: Format 1 linear (64 by 32), 2 a cube map */
static void xgpu_texture_describe(DWORD format, DWORD size, struct xgpu_texture_description *description)
{
	memset(description, 0, sizeof(*description));
	description->width = 64;
	description->height = 32;
	description->depth = description->levels = 1;
	description->linear = format == 1;
	description->cube_map = format == 2;
	/* (as a lookup that could be remembered: render targets never are) */
	description->direct = TRUE;
}

/* a texture each of the header's words gives one of its own */
static GLuint xgpu_texture_get(const DWORD *resource, const void *palette, GLenum *target,
	struct xgpu_texture_description *description)
{
	lookups++;
	xgpu_texture_describe(resource[3], resource[4], description);
	description->direct = texture_direct;
	*target = description->cube_map ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	return 100 + resource[1] * 7 + resource[3] * 3 + resource[4];
}

static const int sampler_states[] = { D3DTSS_MINFILTER, D3DTSS_MIPFILTER, D3DTSS_MAGFILTER, D3DTSS_ADDRESSU,
	D3DTSS_ADDRESSV, D3DTSS_ADDRESSW, D3DTSS_MIPMAPLODBIAS, D3DTSS_MAXMIPLEVEL, D3DTSS_MAXANISOTROPY,
	D3DTSS_BORDERCOLOR };
#define SAMPLER_STATES (sizeof(sampler_states) / sizeof(sampler_states[0]))

/* a sampler object each of the states it is chosen by gives one of its own */
static GLuint sampler_for(int stage)
{
	GLuint sampler = 1000 + (GLuint)config_serial;
	unsigned long index;

	for (index = 0; index < SAMPLER_STATES; index++)
		sampler = sampler * 31 + D3D__TextureState[stage][sampler_states[index]];
	return sampler;
}

static GLuint configure_sampler(int stage, BOOL mipmapped, BOOL hires, BOOL rendered)
{
	sampler_choices++;
	state_sampler(stage, sampler_for(stage));
	return sampler_for(stage);
}

#include "under_test.inc"

static D3DBaseTexture textures[D3DTSS_MAXSTAGES];

/* every stage textured (mode 1), a texture of its own */
static void setup(void)
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		textures[stage].Data = 0x1000 + stage;
		device.textures[stage] = &textures[stage];
	}
	D3D__RenderState[D3DRS_PSTEXTUREMODES] = 1 | 1 << 5 | 1 << 10 | 1 << 15;
}

static struct nv2a_pixel_shader_key key;
static float scale[4][4];

static void draw(void)
{
	memset(&key, 0, sizeof(key));
	bind_textures(&key, scale);
}

/* what a stage was given is what its lookup gives */
static void check_stage(int stage)
{
	struct xgpu_texture_description description;
	GLenum target;
	unsigned long counted = lookups;
	GLuint texture = xgpu_texture_get((const DWORD *)device.textures[stage], NULL, &target, &description);

	lookups = counted;
	CHECK(gl_state.textures[stage][texture_slot(target)] == texture, "stage %d bound %u, not %u", stage,
		gl_state.textures[stage][texture_slot(target)], texture);
	CHECK(bound_samplers[stage] == sampler_for(stage), "stage %d's sampler %u, not %u", stage, bound_samplers[stage],
		sampler_for(stage));
	CHECK(key.sampler_type[stage] == (target == GL_TEXTURE_CUBE_MAP ? _xgpu_sampler_cube : _xgpu_sampler_2d),
		"stage %d's sampler type %d", stage, key.sampler_type[stage]);
	CHECK(scale[stage][0] == (description.linear ? 1.0f / 64 : 1.0f) &&
		scale[stage][1] == (description.linear ? 1.0f / 32 : 1.0f), "stage %d's scale %g %g", stage, scale[stage][0],
		scale[stage][1]);
}

static void check_all(void)
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
		check_stage(stage);
}

/* a draw after which the stages were looked up this many times more */
static void draw_looking_up(unsigned long expected, const char *what)
{
	unsigned long before = lookups;

	draw();
	CHECK(lookups - before == expected, "%s: %lu lookups, not %lu", what, lookups - before, expected);
	check_all();
}

int main(int argc, char **argv)
{
	const char *test = argc > 1 ? argv[1] : "";
	int stage;
	unsigned long index;

	setup();
	draw_looking_up(4, "the first draw");
	if (!strcmp(test, "remembered"))
	{
		/* bound again as they were, and again after their bindings were
		lost (xgpu_gl_state_invalidate) */
		draw_looking_up(0, "the same draw again");
		memset(bound_samplers, 0, sizeof(bound_samplers));
		memset(&gl_state, 0, sizeof(gl_state));
		draw_looking_up(0, "the same draw after its bindings were lost");
		/* a linear texture's scale and a cube map's type kept too */
		textures[1].Format = 1;
		textures[2].Format = 2;
		draw_looking_up(2, "two stages' formats changed");
		draw_looking_up(0, "the same draw again");
	}
	else if (!strcmp(test, "serials"))
	{
		watch_serial++;
		draw_looking_up(4, "a watched page written");
		lookup_serial++;
		draw_looking_up(4, "a lookup changed");
		config_serial++;
		draw_looking_up(4, "a setting changed");
		draw_looking_up(0, "nothing changed");
	}
	else if (!strcmp(test, "header"))
	{
		/* a header rewritten where it is: each word, a stage each */
		textures[0].Data += 0x100;
		textures[1].Format = 1;
		textures[2].Size = 5;
		draw_looking_up(3, "three headers rewritten");
		device.textures[3] = &textures[0];
		draw_looking_up(1, "another header");
	}
	else if (!strcmp(test, "sampler-states"))
	{
		for (index = 0; index < SAMPLER_STATES; index++)
		{
			stage = (int)(index % D3DTSS_MAXSTAGES);
			D3D__TextureState[stage][sampler_states[index]] += 3;
			draw_looking_up(1, "a sampler state changed");
		}
		/* a state the sampler is not chosen by */
		D3D__TextureState[0][D3DTSS_MAX - 1]++;
		draw_looking_up(0, "another state changed");
	}
	else if (!strcmp(test, "not-direct"))
	{
		/* (a palettized texture, a replacement, the meter's channels) */
		texture_direct = FALSE;
		watch_serial++;
		draw_looking_up(4, "lookups not to be remembered");
		draw_looking_up(4, "lookups not to be remembered, again");
	}
	else if (!strcmp(test, "render-target"))
	{
		static struct xgpu_render_target target = { 77, 64, 32 };
		unsigned long before = sampler_choices;

		render_target = &target;
		watch_serial++;
		draw();
		draw();
		CHECK(sampler_choices - before == 8, "%lu samplers chosen for render targets, not 8", sampler_choices - before);
		for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
			CHECK(gl_state.textures[stage][0] == 77, "stage %d bound %u, not the render target", stage,
				gl_state.textures[stage][0]);
	}
	else if (!strcmp(test, "key"))
	{
		/* stage 0's own pixels: no high-res meter's, however the key was */
		memset(&key, 1, sizeof(key));
		bind_textures(&key, scale);
		CHECK(!key.coverage_alpha && !key.point_threshold && !key.ce_meter_channels,
			"stage 0 remembered with a meter's channels");
		check_all();
	}
	else if (!strcmp(test, "ways"))
	{
		/* a stage taking turns between as many textures as it remembers */
		static D3DBaseTexture pool[TEXTURE_STAGE_WAYS + 1];
		int turn;

		for (index = 0; index <= TEXTURE_STAGE_WAYS; index++)
			pool[index].Data = 0x2000 + (DWORD)index;
		for (index = 0; index < TEXTURE_STAGE_WAYS; index++)
		{
			device.textures[1] = &pool[index];
			draw_looking_up(1, "a texture the stage has not had");
		}
		for (turn = 0; turn < 3; turn++)
		{
			for (index = 0; index < TEXTURE_STAGE_WAYS; index++)
			{
				device.textures[1] = &pool[index];
				draw_looking_up(0, "a texture the stage remembers");
			}
		}
		/* one of them rewritten: remembered again in its own way, the
		others kept */
		pool[3].Format = 1;
		device.textures[1] = &pool[3];
		draw_looking_up(1, "a texture rewritten");
		for (index = 0; index < TEXTURE_STAGE_WAYS; index++)
		{
			device.textures[1] = &pool[index];
			draw_looking_up(0, "a texture the stage remembers, after another was rewritten");
		}
		/* one more than it remembers: looked up, and given what the
		lookup gives, as each is forgotten in turn */
		for (turn = 0; turn < 3; turn++)
		{
			for (index = 0; index <= TEXTURE_STAGE_WAYS; index++)
			{
				unsigned long before = lookups;

				device.textures[1] = &pool[index];
				draw();
				CHECK(lookups - before <= 1, "%lu lookups", lookups - before);
				check_all();
			}
		}
	}
	else
		return 2;
	return 0;
}
