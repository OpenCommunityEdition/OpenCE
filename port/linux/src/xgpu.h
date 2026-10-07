/*
XGPU.H

Internals shared by the renderers of the Xbox Direct3D API (xgpu_device.h):
the NV2A shader translators (nv2a_vsh.c, nv2a_psh.c), texture decoding and
the texture cache (xbox_textures.c), guest memory write tracking
(memory_watch.c), the renderers' textures and render targets. The OpenGL
renderer's own are in xgpu_gl.h.
*/

#ifndef __HALO_LINUX_XGPU_H
#define __HALO_LINUX_XGPU_H

#include "platform.h"

#ifdef HALO_ANDROID
/* OpenGL ES features that are optional (d3d8_gl.c gl_initialize) */
struct xgpu_capabilities
{
	BOOL copy_image;
	BOOL border_clamp;
	BOOL anisotropy;
	BOOL s3tc;
	/* ES 3.2: glDrawElementsBaseVertex */
	BOOL base_vertex;
	/* ES 3.1 with fragment atomic counters: exact visibility test counts */
	BOOL atomic_counters;
	/* "300 es" or "310 es" */
	const char *shading_language;
};

extern struct xgpu_capabilities xgpu_capabilities;

/* port/android/guest/runtime/guest_host.h */
int host_gl_has_extension(const char *name);
unsigned int host_gl_read_buffer_word(unsigned int buffer, unsigned int offset);
void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data);
void host_gl_fence_frame(unsigned int slot);
void host_gl_wait_frame(unsigned int slot);
#endif

/* ---------- generated source text */

struct xgpu_text
{
	char *buffer;
	unsigned long length;
	unsigned long capacity;
};

void xgpu_text_append(struct xgpu_text *text, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* ---------- vertex shaders */

#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define XGPU_VERTEX_CONSTANT_COUNT 192
/* D3D constant register -96 is hardware register 0 */
#define XGPU_VERTEX_CONSTANT_BIAS 96

/* where one of the game's model lighting programs (d3d8_device.c
halo_vertex_shader_lighting) has the normal, and the world position, that it
lights the diffuse color by: the temporary register that holds each before
the instruction given */
struct nv2a_vertex_lighting
{
	/* 1 by the ambient and distant lights, 2 by the point lights too */
	int lights;
	unsigned long normal_instruction, normal_register;
	/* (with the point lights only) */
	unsigned long position_instruction, position_register;
};

/* finds them in a model lighting program, checking that it lights its
diffuse color as nv2a_psh.c does for each pixel; FALSE if it does not */
BOOL nv2a_vertex_shader_lighting(const DWORD *instructions, unsigned long instruction_count,
	struct nv2a_vertex_lighting *lighting);

/* GLSL for an NV2A vertex program (the instruction words after the program
header). Attributes whose bit is set in packed_attribute_mask are fed as
NORMPACKED3 32-bit integers and unpacked in the shader. With lighting (else
NULL), the normal and world position go to the pixel shader too, which
lights the diffuse color for each pixel (nv2a_pixel_shader_key
per_pixel_lighting). Returns a malloc'd string. */
char *nv2a_vertex_shader_to_glsl(const DWORD *instructions, unsigned long instruction_count,
	unsigned long packed_attribute_mask, const struct nv2a_vertex_lighting *lighting);

/* ---------- pixel shaders */

enum
{
	_xgpu_sampler_none = 0,
	_xgpu_sampler_2d,
	_xgpu_sampler_3d,
	_xgpu_sampler_cube,
};

/* everything a translated pixel shader depends on; the program caches are
keyed by these bytes */
struct nv2a_pixel_shader_key
{
	DWORD combiner_state[D3DRS_PS_MAX];
	/* D3DRS_PSTEXTUREMODES lies past D3DRS_PS_MAX */
	DWORD texture_modes;
	unsigned char sampler_type[4];
	unsigned char alpha_kill[4];
	/* D3DTSS_COLORSIGN: channels (bit 0 alpha ... bit 3 blue, as
	D3DTSIGN_*) that hold signed data in an unsigned texture format */
	unsigned char color_sign[4];
	/* D3DCMP_* function for the alpha test, or 0 when disabled */
	unsigned long alpha_test_function;
	unsigned char fog_enable;
	unsigned char fog_table_mode;
	/* inside a visibility test: count the samples that pass (Android) */
	unsigned char count_samples;
	/* a high-res HUD meter (hud_hires.h) drawn with the meter's blend (the
	destination kept by the source's alpha): that alpha is eased to 1 by the
	coverage texture 0's green holds, so that the meter darkens what is
	behind it only where it covers it (the Xbox's point-sampled meters stop
	at their texels' edges; filtered ones have a fringe of faint texels) */
	unsigned char coverage_alpha;
	/* a model lighting program's draw lit for each pixel
	(display.per_pixel_lighting): nv2a_vertex_lighting's lights, or 0 for
	the diffuse color the vertex shader computed */
	unsigned char per_pixel_lighting;
	/* drawn into a multisampled target (display.anti_aliasing's
	multisampling), its samples a pixel: the alpha test covers samples in
	proportion to how far alpha is past the reference, not all of the pixel
	or none of it, so that cut-out edges (foliage, grates) are smoothed too */
	unsigned char alpha_test_samples;
};

char *nv2a_pixel_shader_to_glsl(const struct nv2a_pixel_shader_key *key);

/* ---------- HLSL (the Direct3D 12 renderer; xgpu_hlsl.c)

The same programs in HLSL (shader model 5.0). Their constants are in two
constant buffers, b0 for the vertex shader and b1 for the pixel shader, laid
out as these structures; textures t0 to t3 with samplers s0 to s3. */

struct xgpu_hlsl_vertex_constants
{
	float c[XGPU_VERTEX_CONSTANT_COUNT][4];
	float viewport_scale[4];
	float viewport_offset[4];
	/* point_size, screen_offset */
	float misc[4];
	/* the input registers' current values (SetVertexData), for those no
	stream feeds */
	float attribute_values[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
};

struct xgpu_hlsl_pixel_constants
{
	float ps_c0[8][4];
	float ps_c1[8][4];
	float ps_final_c0[4];
	float ps_final_c1[4];
	float fog_color[4];
	float fog_parameters[4];
	/* alpha_reference */
	float misc[4];
	float bump_matrix[4][4];
	float bump_luminance[4][4];
	float texture_scale[4][4];
	/* (XGPU_MODEL_LIGHT_COUNT) */
	float model_lights[12][4];
};

/* how a vertex program's input registers are fed: from a stream or not
(then from attribute_values), as NORMPACKED3 words, as integers (SHORTn: no
format reads them as floats), and with w read through a format of four
components for three (made 1) */
struct nv2a_vertex_inputs
{
	unsigned long provided_mask;
	unsigned long packed_mask;
	unsigned long integer_mask;
	unsigned long w_one_mask;
};

extern const char xgpu_hlsl_prologue[];
/* GLSL's one-argument vector constructors made HLSL's casts; malloc'd */
char *xgpu_hlsl_from_glsl(const char *text);

char *nv2a_vertex_shader_to_hlsl(const DWORD *instructions, unsigned long instruction_count,
	const struct nv2a_vertex_inputs *inputs, const struct nv2a_vertex_lighting *lighting);
char *nv2a_pixel_shader_to_hlsl(const struct nv2a_pixel_shader_key *key);

#ifdef HALO_ANDROID
/* ES samplers have no LOD bias of their own */
#define XGPU_PIXEL_UNIFORMS_ES "uniform vec4 texture_lod_bias;\n"
#else
#define XGPU_PIXEL_UNIFORMS_ES ""
#endif

/* the combiner registers that live in uniforms rather than in the program:
C0/C1 of each stage and the final combiner, and texture constants */
#define XGPU_PIXEL_UNIFORMS \
	"uniform vec4 ps_c0[8];\n" \
	"uniform vec4 ps_c1[8];\n" \
	"uniform vec4 ps_final_c0;\n" \
	"uniform vec4 ps_final_c1;\n" \
	"uniform vec4 fog_color;\n" \
	"uniform vec4 fog_parameters;\n" \
	"uniform float alpha_reference;\n" \
	"uniform vec4 bump_matrix[4];\n" \
	"uniform vec4 bump_luminance[4];\n" \
	"uniform vec4 texture_scale[4];\n" \
	XGPU_PIXEL_UNIFORMS_ES

/* the vertex constants the per-pixel model lighting reads, in a uniform of
their own (the vertex shader's 192 would pass OpenGL ES's least fragment
uniform space): [0] c[-82] (the translucency in z), [1] to [11] c[-79] to
c[-69] (rasterizer_set_model_lighting's two point lights, two distant
lights and the ambient light) */
#define XGPU_MODEL_LIGHT_COUNT 12

/* ---------- the renderer's textures

Every renderer keeps its textures behind these handles, 0 being none: the
texture cache below, the high-res HUD and text and the menus' art make
theirs with them (xgpu_device.h's renderer does the work). */

enum
{
	_xgpu_texture_2d,
	_xgpu_texture_cube,
	_xgpu_texture_3d,
};

enum
{
	/* 32-bit words 0xAARRGGBB (bytes B, G, R, A), as the decoders write them */
	_xgpu_format_bgra8,
	/* bytes R, G, B, A, as PNGs and the text atlas have them */
	_xgpu_format_rgba8,
	_xgpu_format_dxt1,
	_xgpu_format_dxt3,
	_xgpu_format_dxt5,
};

/* a texture of type with levels mip levels; 0 if it cannot be made */
unsigned int xgpu_texture_new(int type, int format, unsigned long width, unsigned long height, unsigned long depth,
	unsigned long levels);
/* the whole of one level of a face (cube faces 0 to 5; a 3D texture's
slices one after another), its rows packed (DXT: rows of blocks) */
void xgpu_texture_write(unsigned int texture, unsigned long face, unsigned long level, const void *data);
/* rows of level 0 of a 2D texture, packed */
void xgpu_texture_write_rows(unsigned int texture, unsigned long first_row, unsigned long rows, const void *data);
/* levels 1 and up made from level 0, whose pixels (as written) are given */
void xgpu_texture_mipmaps(unsigned int texture, const void *level0);
void xgpu_texture_delete(unsigned int texture);
/* whether a DXT texture of this size is kept compressed; else its texels are
decoded to _xgpu_format_bgra8 */
BOOL xgpu_texture_compressed_supported(unsigned long width, unsigned long height);

struct xgpu_texture_description;
/* debug.texture_dump_directory: a texture's level 0 written out, as it was
uploaded (by the renderers that can read it back) */
void xgpu_texture_dump(unsigned int texture, const struct xgpu_texture_description *description);

/* ---------- textures of the game */

struct xgpu_texture_description
{
	DWORD format;       /* D3DFMT_* */
	unsigned long width, height, depth, levels;
	BOOL cube_map;
	BOOL linear;        /* not swizzled; addressed with texel coordinates */
	BOOL compressed;
	unsigned long pitch; /* linear textures */
	BOOL hires;         /* a high-res HUD texture drawn in the texture's place (hud_hires.h) */
	BOOL hires_coverage; /* ... whose green is its coverage (a meter's) */
};

void xgpu_texture_describe(DWORD format_word, DWORD size_word, struct xgpu_texture_description *description);
/* bytes of one face, mip levels included (cube faces are padded) */
unsigned long xgpu_texture_face_size(const struct xgpu_texture_description *description);
unsigned long xgpu_texture_level_offset(const struct xgpu_texture_description *description, unsigned long level);
unsigned long xgpu_texture_level_pitch(const struct xgpu_texture_description *description, unsigned long level);

/* the renderer's texture for an Xbox texture header, uploading or refreshing
it from guest memory as needed; *type receives _xgpu_texture_2d etc. */
unsigned int xgpu_texture_get(const DWORD *resource, const D3DCOLOR *palette, int *type,
	struct xgpu_texture_description *description);
void xgpu_texture_cache_begin_frame(void);

/* ---------- render targets */

struct xgpu_render_target
{
	unsigned long data;  /* physical address */
	unsigned long width, height;
	BOOL depth;
	/* the renderer's texture */
	unsigned int texture;
	/* pixels per unit of width and height: more than 1 for the screen's
	targets when the game draws at the display's resolution (d3d8_device.c) */
	float scale[2];
	unsigned long pixel_width, pixel_height;
	/* with multisampling, the multisampled storage draws go to, its samples a
	pixel (0 when it has none), and whether it has been drawn into since the
	texture last had its pixels (the renderer's) */
	unsigned int multisample;
	int samples;
	BOOL unresolved;
};

/* the render target with this physical address drawn into last, or NULL */
struct xgpu_render_target *xgpu_render_target_find(unsigned long data);

#endif
