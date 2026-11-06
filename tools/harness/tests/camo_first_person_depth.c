/*
CAMO_FIRST_PERSON_DEPTH.C (test)

The real rasterizer_active_camouflage_draw
(rasterizer_xbox_active_camouflage.c), with its
rasterizer_active_camouflage_draw_depth and
rasterizer_active_camouflage_window_depth, over a device that keeps the
states and the viewport each of its draws is made with, and one pixel's
depth, stencil and color through them. Uncloaked, the first-person weapon and
arms mark the stencil and the world is not drawn over them; cloaked they are
drawn after the world, tested against its depth, and on the Xbox a floor or a
wall within reach cut them. Cloaked, fully or fading, each first-person group
first sets the depth under it to the far plane where no such group has been
drawn (a depth-only draw that always passes, the window's depth range pinned
to its far end, marking the stencil where it is unmarked), and is then drawn
as the Xbox drew it, with the window's whole depth range and the stencil not
tested. The pixel shows the nearest part of the weapon, in front of the world,
whatever order its parts are drawn in; another unit's cloak is drawn as
before. test_camo_first_person_depth.py takes the enumerations and the code
from the source (config.inc, under_test.inc).
*/

#include <string.h>
#include "harness.h"

typedef unsigned long DWORD;
typedef struct { real i, j; } real_vector2d;
typedef struct { real i, j, k, l; } real_vector4d;
typedef struct { real red, green, blue; } real_rgb_color;
#define csmemset memset
#define csmemcpy memcpy
#define NUMBEROF(a) (sizeof(a) / sizeof((a)[0]))

#include "config.inc"

/* the Direct3D names the draw uses: only the comparisons', the stencil
operations' and the render states' values matter here, as long as each is
its own */
enum
{
	D3DBLENDOP_ADD = 1, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA,
	D3DCMP_ALWAYS, D3DCMP_EQUAL, D3DCMP_GREATER, D3DCMP_LESSEQUAL, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE,
	D3DCOLORWRITEENABLE_BLUE, D3DCOLORWRITEENABLE_GREEN, D3DCOLORWRITEENABLE_RED, D3DCULL_CCW, D3DCULL_NONE,
	D3DRS_ALPHABLENDENABLE, D3DRS_ALPHAREF, D3DRS_ALPHATESTENABLE, D3DRS_BLENDOP, D3DRS_COLORWRITEENABLE,
	D3DRS_CULLMODE, D3DRS_DESTBLEND, D3DRS_SRCBLEND, D3DRS_ZBIAS, D3DRS_ZENABLE, D3DRS_ZFUNC, D3DRS_ZWRITEENABLE,
	D3DRS_STENCILENABLE, D3DRS_STENCILFAIL, D3DRS_STENCILZFAIL, D3DRS_STENCILPASS, D3DRS_STENCILFUNC,
	D3DRS_STENCILREF, D3DRS_STENCILMASK, D3DRS_STENCILWRITEMASK,
	D3DTADDRESS_CLAMP, D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTSS_ADDRESSU, D3DTSS_ADDRESSV,
	D3DTSS_MAGFILTER, D3DTSS_MINFILTER, D3DTSS_MIPFILTER,
	PS_CHANNEL_ALPHA, PS_DOTMAPPING_MINUS1_TO_1_D3D, PS_DOTMAPPING_ZERO_TO_ONE, PS_INPUTMAPPING_UNSIGNED_INVERT,
	PS_REGISTER_C0, PS_REGISTER_DISCARD, PS_REGISTER_ONE, PS_REGISTER_R0, PS_REGISTER_T0, PS_REGISTER_T2,
	PS_REGISTER_V0, PS_REGISTER_ZERO, PS_TEXTUREMODES_CUBEMAP, PS_TEXTUREMODES_DOTPRODUCT, PS_TEXTUREMODES_DOT_ST,
	PS_TEXTUREMODES_NONE, PS_TEXTUREMODES_PROJECT2D,
	D3DRS_MAX
};
#define PS_TEXTUREMODES(a, b, c, d) 0
#define PS_COMBINERCOUNT(a, b) 0
#define PS_COMBINERINPUTS(a, b, c, d) 0
#define PS_COMBINEROUTPUTS(a, b, c, d) 0
#define PS_INPUTTEXTURE(a, b, c, d) 0
#define PS_DOTMAPPING(a, b, c, d) 0
static struct
{
	DWORD texture_modes, input_texture, dot_mapping, combiner_count;
	DWORD rgb_inputs[2], rgb_outputs[2];
	DWORD final_combiner_constant_0, final_combiner_inputs_abcd, final_combiner_inputs_efg;
} pixel_shader;

/* the device: its render states and viewport, and how many times the
viewport was set */
typedef struct { DWORD X, Y, Width, Height; float MinZ, MaxZ; } D3DVIEWPORT8;
static DWORD render_states[D3DRS_MAX];
static D3DVIEWPORT8 device_viewport;
static long viewports_set;
static int device_object;
static int *global_d3d_device = &device_object;

static void IDirect3DDevice8_SetViewport(int *device, D3DVIEWPORT8 const *viewport)
{
	device_viewport = *viewport;
	viewports_set++;
}
static void IDirect3DDevice8_SetRenderState(int *device, long state, DWORD value)
{
	CHECK(state >= 0 && state < D3DRS_MAX, "render state %ld", state);
	render_states[state] = value;
}
static void IDirect3DDevice8_SetTextureStageState(int *device, long stage, long state, DWORD value) { }
static void IDirect3DDevice8_SetVertexShaderConstant(int *device, long index, void const *data, long count) { }
#define SetRenderStateSmart(state, value) IDirect3DDevice8_SetRenderState(global_d3d_device, (state), (value))

/* the stencil modes (rasterizer_xbox.c): none, and reject, which a group of
the first-person weapon's drawn before leaves (its second pass). A mode set
again is skipped, as the rasterizer's state cache does */
static long stencil_mode_cached;

static void rasterizer_set_stencil_mode(long mode)
{
	if (mode == stencil_mode_cached)
		return;
	stencil_mode_cached = mode;
	render_states[D3DRS_STENCILENABLE] = mode != RASTERIZER_STENCIL_MODE_NONE;
	if (mode == RASTERIZER_STENCIL_MODE_REJECT)
	{
		render_states[D3DRS_STENCILFUNC] = D3DCMP_EQUAL;
		render_states[D3DRS_STENCILREF] = 0;
		render_states[D3DRS_STENCILMASK] = 1;
		render_states[D3DRS_STENCILWRITEMASK] = 0;
		render_states[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
	}
}

/* the cloaked group and its shader */
struct render_lighting { long unused; };
struct render_animation { long unused; };
struct shader { struct { short type; } base; };
struct shader_model_definition
{
	struct
	{
		struct { long index; } base_map;
		unsigned long flags;
		real detail_map_scale, detail_map_v_scale, map_u_scale, map_v_scale;
		long texture_animation;
	} model;
};
struct transparent_geometry_group
{
	struct shader *shader;
	short shader_permutation_index;
	unsigned long geometry_flags;
	struct { short type; real intensity, parameter; } effect;
	void *node_matrices;
	short node_matrix_count;
	real_point3d centroid;
	real_vector2d model_base_map_scale;
	struct render_lighting *lighting;
	struct render_animation *animation;
	void *triangle_buffer;
	long dynamic_triangle_buffer_index;
	long triangle_count;
	void *vertex_buffer;
	long dynamic_vertex_buffer_index;
	struct transparent_geometry_group *next; /* (the test's: the next drawn) */
};
struct rasterizer_model_begin_parameters
{
	unsigned long geometry_flags;
	struct { void *node_matrices; short node_matrix_count; } skinning;
	struct render_lighting lighting;
	struct render_animation animation;
	struct { short type; real intensity, parameter; } effect;
	real_point3d centroid;
	real_vector2d base_map_scale;
};
static struct shader_model_definition cloak;
static void *shader_get_and_verify_type(struct shader *shader, short type) { return &cloak; }
static void shader_texture_animation_evaluate(long const *animation, struct render_animation const *render_animation,
	real u_scale, real v_scale, real u_offset, real v_offset, real rotation, real time,
	real_vector4d *u, real_vector4d *v)
{
}
static DWORD real_alpha_to_pixel32(real alpha) { return 0; }

/* the rasterizer */
static struct { boolean active_camouflage_enabled; } rasterizer_debug_options = { TRUE };
static struct
{
	short rasterizer_target;
	struct { struct { short x0, y0, x1, y1; } viewport_bounds; } camera;
} global_window_parameters;
static struct { real game_time_sec; } global_frame_parameters;
static struct
{
	struct { long index; } active_camouflage_distortion;
	unsigned long active_camouflage_flags;
	real active_camouflage_refraction_amount, active_camouflage_hyper_stealth_refraction_amount;
	real active_camouflage_distance_falloff, active_camouflage_hyper_stealth_distance_falloff;
	real_rgb_color active_camouflage_tint_color, active_camouflage_hyper_stealth_tint_color;
} rasterizer_data, *global_rasterizer_data = &rasterizer_data;
static struct { real first_person_weapon_near_clip_distance, first_person_weapon_far_clip_distance; } rasterizer_globals =
	{ 0.01171875f, 1024.0f };

static void rasterizer_set_frustum_z(real z_near, real z_far) { }
/* the transparent groups after one, and the screen copies the cloak made */
static struct transparent_geometry_group *rasterizer_transparent_geometry_next_group(
	struct transparent_geometry_group const *group)
{
	return group->next;
}
static long screen_copies;
static void rasterizer_active_camouflage_cache_primary_render_target(void) { screen_copies++; }
static void rasterizer_set_vertex_shader_permutation(short shader, short vertex_type, short permutation) { }
static short rasterizer_transparent_geometry_get_primary_vertex_type(struct transparent_geometry_group const *group)
{
	return 0;
}
static void rasterizer_set_pixel_shader(void const *shader) { }
static void rasterizer_set_texture(short stage, short type, short usage, long bitmap, short index) { }
static void rasterizer_set_texture_direct(short stage, long bitmap, short index) { }
static void rasterizer_set_target_as_texture(short stage, short target, short index) { }
static void rasterizer_profile_enable(boolean enable) { }
static void rasterizer_models_begin(boolean enable) { }
static void rasterizer_models_end(void) { }
static void rasterizer_model_begin(struct rasterizer_model_begin_parameters const *parameters, boolean keep) { }
static void rasterizer_model_end(void) { }

/* the draws: each one's states and viewport, and a pixel of the window: its
depth (0 near, 1 far), its stencil and the part it shows. A group's depth at
the pixel is its triangle count in thousandths, and so is the part it shows
(the world's is NONE). A model drawn under a fading cloak sets its own depth
test and writes (rasterizer_xbox_models.c) */
struct draw
{
	boolean model;
	DWORD z_function, z_write, color_write, stencil_enable, stencil_function, stencil_pass;
	D3DVIEWPORT8 viewport;
};
enum { MAXIMUM_DRAWS = 16 };
static struct draw draws[MAXIMUM_DRAWS];
static long draw_count;
static real pixel_depth;
static DWORD pixel_stencil;
static long pixel_part;

static boolean depth_passed(DWORD function, real fragment)
{
	switch (function)
	{
	case D3DCMP_ALWAYS: return TRUE;
	case D3DCMP_LESSEQUAL: return fragment <= pixel_depth;
	case D3DCMP_EQUAL: return fragment == pixel_depth;
	}
	CHECK(FALSE, "depth function %lu", function);
	return FALSE;
}

static boolean stencil_passed(void)
{
	DWORD reference = render_states[D3DRS_STENCILREF] & render_states[D3DRS_STENCILMASK];
	DWORD value = pixel_stencil & render_states[D3DRS_STENCILMASK];

	if (!render_states[D3DRS_STENCILENABLE])
		return TRUE;
	switch (render_states[D3DRS_STENCILFUNC])
	{
	case D3DCMP_ALWAYS: return TRUE;
	case D3DCMP_EQUAL: return reference == value;
	case D3DCMP_GREATER: return reference > value;
	}
	CHECK(FALSE, "stencil function %lu", render_states[D3DRS_STENCILFUNC]);
	return FALSE;
}

static void drawn(boolean model, long triangle_count)
{
	struct draw *draw = &draws[draw_count];
	real depth = (real)triangle_count / 1000.0f;
	real fragment;

	CHECK(draw_count < MAXIMUM_DRAWS, "too many draws");
	draw->model = model;
	draw->z_function = model ? D3DCMP_LESSEQUAL : render_states[D3DRS_ZFUNC];
	draw->z_write = model ? TRUE : render_states[D3DRS_ZWRITEENABLE];
	draw->color_write = model ? D3DCOLORWRITEENABLE_RED : render_states[D3DRS_COLORWRITEENABLE];
	draw->stencil_enable = render_states[D3DRS_STENCILENABLE];
	draw->stencil_function = render_states[D3DRS_STENCILFUNC];
	draw->stencil_pass = render_states[D3DRS_STENCILPASS];
	draw->viewport = device_viewport;
	draw_count++;

	fragment = device_viewport.MinZ + (device_viewport.MaxZ - device_viewport.MinZ) * depth;
	if (stencil_passed() && depth_passed(draw->z_function, fragment))
	{
		if (draw->z_write)
			pixel_depth = fragment;
		if (render_states[D3DRS_STENCILENABLE] && render_states[D3DRS_STENCILPASS] == D3DSTENCILOP_REPLACE)
		{
			pixel_stencil = (pixel_stencil & ~render_states[D3DRS_STENCILWRITEMASK]) |
				(render_states[D3DRS_STENCILREF] & render_states[D3DRS_STENCILWRITEMASK]);
		}
		if (draw->color_write)
			pixel_part = triangle_count;
	}
}
static void rasterizer_transparent_geometry_group_draw__internal(struct transparent_geometry_group *group, boolean flag)
{
	drawn(FALSE, group->triangle_count);
}
static void rasterizer_model_draw(struct shader *shader, short permutation, void *triangle_buffer,
	long dynamic_triangle_buffer_index, long triangle_count, void *vertex_buffer, long dynamic_vertex_buffer_index)
{
	drawn(TRUE, triangle_count);
}

#include "under_test.inc"

/* a frame: the window's viewport, as rasterizer_set_target leaves it, the
world's depth at the pixel, and the stencil left in reject by the
first-person weapon's groups drawn before (their second pass); then each
cloaked group, at its depth, in the order given */
static struct render_lighting lighting;
static struct render_animation animation;
static struct shader shader;
static short window[4];

static void frame_begin(short x0, short y0, short x1, short y1, real world_depth)
{
	global_window_parameters.rasterizer_target = _rasterizer_target_render_primary;
	global_window_parameters.camera.viewport_bounds.x0 = window[0] = x0;
	global_window_parameters.camera.viewport_bounds.y0 = window[1] = y0;
	global_window_parameters.camera.viewport_bounds.x1 = window[2] = x1;
	global_window_parameters.camera.viewport_bounds.y1 = window[3] = y1;
	device_viewport.X = (DWORD)x0;
	device_viewport.Y = (DWORD)y0;
	device_viewport.Width = (DWORD)(x1 - x0);
	device_viewport.Height = (DWORD)(y1 - y0);
	device_viewport.MinZ = 0.0f;
	device_viewport.MaxZ = 1.0f;
	viewports_set = 0;
	draw_count = 0;
	pixel_depth = world_depth;
	pixel_stencil = 0;
	pixel_part = NONE;
	stencil_mode_cached = NONE;
	rasterizer_set_stencil_mode(RASTERIZER_STENCIL_MODE_REJECT);
}

static void cloaked(boolean first_person, real intensity, real depth)
{
	struct transparent_geometry_group group;
	static int node_matrices[4];

	memset(&group, 0, sizeof(group));
	group.shader = &shader;
	shader.base.type = _shader_type_model;
	group.geometry_flags = first_person ? FLAG(_rasterizer_geometry_first_person_bit) : 0;
	group.effect.type = _render_model_effect_type_active_camouflage;
	group.effect.intensity = intensity;
	group.node_matrices = node_matrices;
	group.node_matrix_count = 4;
	group.lighting = &lighting;
	group.animation = &animation;
	group.triangle_count = (long)(depth * 1000.0f + 0.5f);
	rasterizer_active_camouflage_draw(&group);
}

/* the window's viewport, with the depth range from min_z to max_z */
static void window_with(D3DVIEWPORT8 const *viewport, float min_z, float max_z, const char *what)
{
	CHECK(viewport->X == (DWORD)window[0] && viewport->Y == (DWORD)window[1] &&
		viewport->Width == (DWORD)(window[2] - window[0]) && viewport->Height == (DWORD)(window[3] - window[1]),
		"%s: the viewport is %lu,%lu %lux%lu, not the window's %d,%d %dx%d", what,
		viewport->X, viewport->Y, viewport->Width, viewport->Height,
		window[0], window[1], window[2] - window[0], window[3] - window[1]);
	CHECK(viewport->MinZ == min_z && viewport->MaxZ == max_z, "%s: depth from %g to %g, not %g to %g", what,
		viewport->MinZ, viewport->MaxZ, min_z, max_z);
}

/* one group's draws: first a depth-only draw that always passes, at the far
end of the window's depth range, marking the stencil where it is unmarked;
then the Xbox's, with the window's whole depth range and the stencil not
tested, which they are after it too */
static void cleared_then_as_the_xbox(long draws_after, const char *what)
{
	long index;

	CHECK(draw_count == 1 + draws_after, "%s: %ld draws, not %ld", what, draw_count, 1 + draws_after);
	CHECK(!draws[0].model && draws[0].z_function == D3DCMP_ALWAYS && draws[0].z_write == TRUE &&
		draws[0].color_write == 0, "%s: the first draw is not a depth-only draw that always passes", what);
	CHECK(draws[0].stencil_enable && draws[0].stencil_function == D3DCMP_GREATER &&
		draws[0].stencil_pass == D3DSTENCILOP_REPLACE,
		"%s: the first draw does not mark the stencil where it is unmarked", what);
	window_with(&draws[0].viewport, 1.0f, 1.0f, what);
	for (index = 1; index < draw_count; index++)
	{
		window_with(&draws[index].viewport, 0.0f, 1.0f, what);
		CHECK(!draws[index].stencil_enable, "%s: draw %ld tests the stencil", what, index);
	}
	window_with(&device_viewport, 0.0f, 1.0f, what);
	CHECK(!render_states[D3DRS_STENCILENABLE], "%s: the stencil is left tested", what);
	/* (a first-person group drawn after it sets its passes' stencil modes) */
	rasterizer_set_stencil_mode(RASTERIZER_STENCIL_MODE_REJECT);
	CHECK(render_states[D3DRS_STENCILENABLE], "%s: the stencil mode set after it is not set", what);
}

static void shows(long part, const char *what)
{
	CHECK(pixel_part == part, "%s: the pixel shows %s %ld, not %s %ld", what,
		pixel_part == NONE ? "the world" : "part", pixel_part, part == NONE ? "the world" : "part", part);
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";

	/* the first-person weapon fully cloaked, a wall within reach in front
	of it as deep: cleared, then its depth where it is nearest, then its
	distortion where the depth is its own */
	CASE("cloaked-first-person")
	{
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(TRUE, 1.0f, 0.4f);
		cleared_then_as_the_xbox(2, "fully cloaked in first person");
		CHECK(draws[1].z_function == D3DCMP_LESSEQUAL && draws[1].z_write == TRUE && draws[1].color_write == 0,
			"fully cloaked in first person: its depth is not laid down where it is nearest");
		CHECK(draws[2].z_function == D3DCMP_EQUAL,
			"fully cloaked in first person: its distortion is not drawn where the depth is its own");
		shows(400, "fully cloaked in first person, behind a wall's depth");
		return 0;
	}
	/* fading in or out (firing): cleared, then the model under its distortion */
	CASE("fading-first-person")
	{
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(TRUE, 0.5f, 0.4f);
		cleared_then_as_the_xbox(2, "fading in first person");
		CHECK(draws[1].model, "fading in first person: the model is not drawn after the depth is cleared");
		shows(400, "fading in first person, behind a wall's depth");
		return 0;
	}
	/* two parts at the pixel, the nearer drawn first, then the other way
	round, fully cloaked and fading: the nearer is shown */
	CASE("parts-in-order")
	{
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(TRUE, 1.0f, 0.4f);
		cloaked(TRUE, 1.0f, 0.6f);
		shows(400, "fully cloaked, the nearer part drawn first");
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(TRUE, 0.5f, 0.4f);
		cloaked(TRUE, 0.5f, 0.6f);
		shows(400, "fading, the nearer part drawn first");
		return 0;
	}
	CASE("parts-out-of-order")
	{
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(TRUE, 1.0f, 0.6f);
		cloaked(TRUE, 1.0f, 0.4f);
		shows(400, "fully cloaked, the farther part drawn first");
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(TRUE, 0.5f, 0.6f);
		cloaked(TRUE, 0.5f, 0.4f);
		shows(400, "fading, the farther part drawn first");
		return 0;
	}
	/* another unit's cloak: as the Xbox drew it, behind the wall's depth */
	CASE("another-unit")
	{
		frame_begin(0, 0, 640, 480, 0.3f);
		cloaked(FALSE, 1.0f, 0.4f);
		CHECK(draw_count == 2, "another unit's cloak: %ld draws, not 2", draw_count);
		CHECK(viewports_set == 0, "another unit's cloak set the viewport %ld times", viewports_set);
		CHECK(draws[0].z_function == D3DCMP_LESSEQUAL && draws[1].z_function == D3DCMP_EQUAL,
			"another unit's cloak is not drawn as the Xbox drew it");
		shows(NONE, "another unit's cloak behind a wall");
		return 0;
	}
	/* the first-person weapon's transparent geometry: its muzzle flash (an
	effect shader) as it fires, then its cloaked gun and arms. The screen
	they distort is copied once, before any of it is drawn */
	CASE("first-person-copy")
	{
		struct transparent_geometry_group groups[3];
		struct shader flash = { { _shader_type_effect } }, model = { { _shader_type_model } };
		short index;

		memset(groups, 0, sizeof(groups));
		for (index = 0; index < 3; index++)
		{
			groups[index].geometry_flags = FLAG(_rasterizer_geometry_first_person_bit);
			groups[index].shader = index ? &model : &flash;
			groups[index].effect.type = index ? _render_model_effect_type_active_camouflage : 0;
			groups[index].next = index < 2 ? &groups[index + 1] : NULL;
		}
		screen_copies = 0;
		rasterizer_active_camouflage_cache_for_first_person(&groups[0]);
		CHECK(screen_copies == 1, "the screen is copied %ld times before the first-person weapon, not once",
			screen_copies);
		/* the weapon uncloaked: no copy */
		groups[1].effect.type = groups[2].effect.type = 0;
		screen_copies = 0;
		rasterizer_active_camouflage_cache_for_first_person(&groups[0]);
		CHECK(screen_copies == 0, "the screen is copied %ld times for an uncloaked first-person weapon",
			screen_copies);
		return 0;
	}
	/* a split screen's lower right window */
	CASE("split-screen")
	{
		frame_begin(320, 240, 640, 480, 0.3f);
		cloaked(TRUE, 1.0f, 0.4f);
		cleared_then_as_the_xbox(2, "in a split screen's window");
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
