/*
GPU.H

The interface between the Xbox Direct3D device (d3d8_device.c, with its
texture cache, xbox_textures.c, and the high-res HUD, text and menu art) and
the backend that drives the GPU for it (gpu_gl.c: OpenGL 4.5, or OpenGL ES 3
on Android). The device keeps the Xbox's state and says what it needs in the
neutral terms below, one self-contained packet per draw; the backend owns
the GPU's objects and makes every call to its API.

It includes only <stdint.h>, and its structs have fixed-width fields only, so
that a backend built for another ABI than the game's (a 64-bit host drawing
for the 32-bit game) can take them as they are.
*/

#ifndef __HALO_GPU_H
#define __HALO_GPU_H

#include <stdint.h>

/* a texture (render targets included), a buffer and a compiled shader; 0
is none */
typedef uint32_t gpu_texture, gpu_buffer, gpu_shader;

/* the Xbox's texture stages and vertex attributes (D3DTSS_MAXSTAGES,
XGPU_VERTEX_ATTRIBUTE_COUNT) */
enum { GPU_STAGE_COUNT = 4, GPU_ATTRIBUTE_COUNT = 16 };

/* ---------- the backend */

/* how visibility (occlusion) tests count */
enum
{
	/* the samples that passed */
	GPU_OCCLUSION_EXACT,
	/* only whether any passed (OpenGL ES's occlusion queries) */
	GPU_OCCLUSION_ANY_SAMPLE,
	/* the pixel shader counts the samples (nv2a_pixel_shader_key count_samples) */
	GPU_OCCLUSION_SHADER_COUNTER,
};

struct gpu_capabilities
{
	/* the GLSL version the shaders are written in: 450, or 300 or 310 with
	shading_language_es */
	uint16_t shading_language;
	uint8_t shading_language_es;
	/* samplers apply a LOD bias; otherwise each lookup in the pixel shader
	does (nv2a_dialect lookup_lod_bias) */
	uint8_t sampler_lod_bias;
	/* the vertex shader turns clip space into GL's, as glClipControl does
	on the desktop (nv2a_dialect clip_control) */
	uint8_t shader_clip_control;
	/* BC1, BC2 and BC3 textures; otherwise DXT textures are decoded */
	uint8_t s3tc;
	/* vertex attributes read D3DCOLOR's byte order (BGRA); otherwise the
	device swaps the bytes as it streams them */
	uint8_t vertex_bgra;
	/* indexed draws add a base vertex; otherwise the device rebases the
	indices */
	uint8_t base_vertex;
	/* GPU_OCCLUSION_* */
	uint8_t occlusion;
	/* a pixel shader can choose the samples it covers (gl_SampleMask:
	nv2a_pixel_shader_key alpha_test_samples) */
	uint8_t sample_mask;
	uint8_t pad[2];
	/* the most samples a pixel of a multisampled target can have, and the
	largest render target each way */
	uint32_t max_samples;
	uint32_t max_target_size;
};

/* sets up the backend for the context, which must be current, and reports
what it can do */
void gpu_initialize(struct gpu_capabilities *capabilities);

/* ---------- textures */

/* gpu_texture_description.type */
enum { GPU_TEXTURE_2D = 1, GPU_TEXTURE_3D, GPU_TEXTURE_CUBE };

/* gpu_texture_description.format. A BGRA8 texel is a 32-bit ARGB word (the
bytes blue, green, red, alpha); an RGBA8 texel the bytes red, green, blue,
alpha. BC1 to BC3 are DXT1, DXT3 and DXT5 blocks. */
enum
{
	GPU_FORMAT_BGRA8 = 1,
	GPU_FORMAT_RGBA8,
	GPU_FORMAT_BC1,
	GPU_FORMAT_BC2,
	GPU_FORMAT_BC3,
	GPU_FORMAT_DEPTH_STENCIL,
};

/* gpu_texture_description.usage */
enum
{
	/* its levels come from gpu_texture_upload */
	GPU_USAGE_UPLOAD = 1,
	/* drawn into, or given its levels by gpu_texture_copy_level: every
	level is made with the texture, its texels undefined */
	GPU_USAGE_RENDER_TARGET,
};

struct gpu_texture_description
{
	uint8_t type;
	uint8_t format;
	uint8_t usage;
	uint8_t pad;
	uint32_t width, height, depth, levels;
};

gpu_texture gpu_texture_create(const struct gpu_texture_description *description);
/* the channel of the texels (0 red, 1 green, 2 blue, 3 alpha) each channel
is sampled from, from the next upload of face 0's level 0 on; red, green,
blue and alpha from their own until then. An RGBA8 texture's channels are
always its own. */
void gpu_texture_channels(gpu_texture texture, const uint8_t channels[4]);
/* one level of one face (0 but for a cube), size bytes in the texture's
format, or with no data the level with its texels undefined; face 0's
level 0 first */
void gpu_texture_upload(gpu_texture texture, uint32_t face, uint32_t level, const void *data, uint32_t size);
/* rows first_row to first_row + rows - 1 of a 2D texture's level 0, once
it has been uploaded */
void gpu_texture_upload_rows(gpu_texture texture, uint32_t first_row, uint32_t rows, const void *data);
/* level 0 of a color render target into level of another, the size that
level 0 is */
void gpu_texture_copy_level(gpu_texture source, gpu_texture destination, uint32_t level);
/* the levels after base_level, made from base_level */
void gpu_texture_generate_mipmaps(gpu_texture texture, uint32_t base_level);
void gpu_texture_destroy(gpu_texture texture);
/* level 0 of a 2D texture as BGRA8 rows from the top: its width times its
height times 4 bytes. Returns 0 and writes nothing if size is short of that
or the backend cannot read the texture (depth, and with OpenGL ES, anything
but a color render target). */
uint32_t gpu_texture_read(gpu_texture texture, void *pixels, uint32_t size);

/* ---------- samplers */

/* texture filters (D3DTSS_MINFILTER, MAGFILTER and MIPFILTER, which they
number as D3DTEXF_* does): GL samples all but POINT linearly, and an
ANISOTROPIC minification with anisotropy */
enum
{
	GPU_FILTER_NONE, GPU_FILTER_POINT, GPU_FILTER_LINEAR, GPU_FILTER_ANISOTROPIC,
	GPU_FILTER_QUINCUNX, GPU_FILTER_GAUSSIAN_CUBIC,
};

/* texture addressing (D3DTSS_ADDRESSU, V and W): CLAMP and CLAMP_TO_EDGE
sample alike; BORDER becomes CLAMP_TO_EDGE where the GPU has no border
color */
enum { GPU_ADDRESS_WRAP, GPU_ADDRESS_MIRROR, GPU_ADDRESS_CLAMP, GPU_ADDRESS_BORDER, GPU_ADDRESS_CLAMP_TO_EDGE };

struct gpu_sampler_state
{
	uint8_t min_filter, mag_filter, mip_filter;
	uint8_t address_u, address_v, address_w;
	uint8_t pad[2];
	/* the first level sampled (D3DTSS_MAXMIPLEVEL) */
	uint32_t max_mip_level;
	/* the anisotropy of an ANISOTROPIC minification */
	uint32_t max_anisotropy;
	/* added to the level of detail: by the sampler, or without
	sampler_lod_bias by the pixel shader (texture_lod_bias) */
	float lod_bias;
	/* ARGB, as D3DCOLOR */
	uint32_t border_color;
};

/* a texture stage: the texture a draw samples there and how */
struct gpu_stage
{
	gpu_texture texture;
	/* 0: nothing is sampled; else the texture's GPU_TEXTURE_* */
	uint8_t type;
	uint8_t pad[3];
	struct gpu_sampler_state sampler;
};

/* ---------- buffers

The device's copy of the Xbox's contiguous memory (vertex and index data
that stays put) lives in buffers it writes as the game changes it; what a
draw streams goes into the backend's own buffers for the frame. */

/* gpu_buffer_write flags: no draw queued so far reads the range (pages
written for the first time), so the write need not wait for one */
enum { GPU_WRITE_UNUSED = 1 };

gpu_buffer gpu_buffer_create(uint32_t size);
void gpu_buffer_write(gpu_buffer buffer, uint32_t offset, uint32_t size, const void *data, uint32_t flags);

/* gpu_stream kinds */
enum { GPU_STREAM_VERTEX = 1, GPU_STREAM_INDEX };

/* room for bytes of vertices before a draw's first gpu_stream of them:
starting a new buffer between two of a draw's streams would leave the
earlier ones in the old one */
void gpu_stream_reserve(uint32_t bytes);
/* copies size bytes (rounded up to 16) into the frame's vertex or index
buffer; returns their offset there, and the buffer in *buffer */
uint32_t gpu_stream(uint32_t kind, const void *data, uint32_t size, gpu_buffer *buffer);

/* ---------- shaders */

enum { GPU_SHADER_VERTEX = 1, GPU_SHADER_PIXEL };

/* source is GLSL in the dialect the capabilities name; 0 if it does not
compile (the log says why) */
gpu_shader gpu_shader_create(uint32_t stage, const char *source);

/* ---------- what a draw's shaders read

The vertex constants: the device's copy of the 192 registers. Each register's
serial is the value serial had when the register last changed, and log
holds the register each of the latest serials changed (modulo its size), so
a backend that uploaded a program's registers at serial s finds what has
changed since without looking at them all. The checkpoint is what changed
since the backend last uploaded registers to any program, which the backend
resets as it does. The serials are 64-bit: a skinned model's draw changes up
to 132 registers, and 32 bits wrapped within minutes at a high frame rate. */

enum { GPU_CONSTANT_COUNT = 192, GPU_CONSTANT_LOG_SIZE = 1024 };

struct gpu_constant_store
{
	float c[GPU_CONSTANT_COUNT][4];
	uint64_t serials[GPU_CONSTANT_COUNT];
	uint64_t serial;
	uint8_t log[GPU_CONSTANT_LOG_SIZE];
	uint64_t checkpoint_serial;
	uint32_t checkpoint_first, checkpoint_last;
};

/* the per-pixel model lighting's registers (nv2a_psh.c model_lighting):
c[-82] and c[-79] to c[-69], at 96 + those */
enum { GPU_MODEL_LIGHT_COUNT = 12 };

/* the other uniforms (nv2a_vsh.c, nv2a_psh.c), and a serial that changes
whenever any of them does */
struct gpu_uniforms
{
	float viewport_scale[4];
	float viewport_offset[4];
	float point_size;
	float ps_c0[8][4];
	float ps_c1[8][4];
	float ps_final_c0[4];
	float ps_final_c1[4];
	float fog_color[4];
	float fog_parameters[4];
	float alpha_reference;
	float bump_matrix[4][4];
	float bump_luminance[4][4];
	float texture_scale[4][4];
	float screen_offset;
	float texture_lod_bias[4];
	uint32_t serial;
};

/* ---------- draw state */

/* depth and stencil tests' comparisons, as D3DCMP_* orders them */
enum
{
	GPU_COMPARE_NEVER, GPU_COMPARE_LESS, GPU_COMPARE_EQUAL, GPU_COMPARE_LESS_EQUAL,
	GPU_COMPARE_GREATER, GPU_COMPARE_NOT_EQUAL, GPU_COMPARE_GREATER_EQUAL, GPU_COMPARE_ALWAYS,
};

enum
{
	GPU_STENCIL_KEEP, GPU_STENCIL_ZERO, GPU_STENCIL_REPLACE, GPU_STENCIL_INCREMENT_CLAMP,
	GPU_STENCIL_DECREMENT_CLAMP, GPU_STENCIL_INVERT, GPU_STENCIL_INCREMENT_WRAP, GPU_STENCIL_DECREMENT_WRAP,
};

enum
{
	GPU_BLEND_ZERO, GPU_BLEND_ONE,
	GPU_BLEND_SOURCE_COLOR, GPU_BLEND_ONE_MINUS_SOURCE_COLOR,
	GPU_BLEND_SOURCE_ALPHA, GPU_BLEND_ONE_MINUS_SOURCE_ALPHA,
	GPU_BLEND_DESTINATION_ALPHA, GPU_BLEND_ONE_MINUS_DESTINATION_ALPHA,
	GPU_BLEND_DESTINATION_COLOR, GPU_BLEND_ONE_MINUS_DESTINATION_COLOR,
	GPU_BLEND_SOURCE_ALPHA_SATURATE,
	GPU_BLEND_CONSTANT_COLOR, GPU_BLEND_ONE_MINUS_CONSTANT_COLOR,
	GPU_BLEND_CONSTANT_ALPHA, GPU_BLEND_ONE_MINUS_CONSTANT_ALPHA,
};

enum { GPU_BLEND_OP_ADD, GPU_BLEND_OP_SUBTRACT, GPU_BLEND_OP_REVERSE_SUBTRACT, GPU_BLEND_OP_MIN, GPU_BLEND_OP_MAX };

/* the faces discarded; the winding of the front ones; how polygons fill */
enum { GPU_CULL_NONE, GPU_CULL_FRONT, GPU_CULL_BACK };
enum { GPU_FRONT_CLOCKWISE, GPU_FRONT_COUNTER_CLOCKWISE };
enum { GPU_FILL_SOLID, GPU_FILL_LINE, GPU_FILL_POINT };

/* a rectangle of the targets' pixels, rows from the top; the scissor test
is off when width or height is not above 0 */
struct gpu_rect
{
	int32_t x, y, width, height;
};

struct gpu_viewport
{
	struct gpu_rect rect;
	float min_z, max_z;
};

/* each comparison and operation is used only while its test is on */
struct gpu_depth_stencil_state
{
	uint8_t depth_test, depth_write, depth_function;
	uint8_t stencil_test, stencil_function;
	uint8_t stencil_fail, stencil_depth_fail, stencil_pass;
	uint32_t stencil_reference, stencil_read_mask, stencil_write_mask;
};

/* the factors, operation and color are used only while enable is on */
struct gpu_blend_state
{
	uint8_t enable;
	uint8_t source, destination, operation;
	/* ARGB, as D3DCOLOR */
	uint32_t color;
	/* bit 0 red, 1 green, 2 blue, 3 alpha */
	uint8_t color_write_mask;
	uint8_t pad[3];
};

struct gpu_raster_state
{
	uint8_t cull_mode, front_face, fill_mode;
	/* the depth bias, with a slope term, while enabled */
	uint8_t depth_bias;
	float depth_bias_slope, depth_bias_constant;
};

/* vertex attribute formats: BGRA8 is D3DCOLOR (with vertex_bgra; RGBA8 the
same swapped), SHORT a plain integer read as a float, NORMSHORT and UBYTE
normalized, and NORMPACKED3 one unsigned integer the vertex shader unpacks */
enum
{
	GPU_ATTRIBUTE_FLOAT1 = 1, GPU_ATTRIBUTE_FLOAT2, GPU_ATTRIBUTE_FLOAT3, GPU_ATTRIBUTE_FLOAT4,
	GPU_ATTRIBUTE_BGRA8, GPU_ATTRIBUTE_RGBA8,
	GPU_ATTRIBUTE_SHORT1, GPU_ATTRIBUTE_SHORT2, GPU_ATTRIBUTE_SHORT3, GPU_ATTRIBUTE_SHORT4,
	GPU_ATTRIBUTE_NORMSHORT1, GPU_ATTRIBUTE_NORMSHORT2, GPU_ATTRIBUTE_NORMSHORT3, GPU_ATTRIBUTE_NORMSHORT4,
	GPU_ATTRIBUTE_UBYTE1, GPU_ATTRIBUTE_UBYTE2, GPU_ATTRIBUTE_UBYTE3, GPU_ATTRIBUTE_UBYTE4,
	GPU_ATTRIBUTE_NORMPACKED3,
};

/* an attribute's source other than streams 0 to 15: the value in
constant_values, or the integer zero */
enum { GPU_STREAM_COUNT = 16, GPU_STREAM_CONSTANT = 16, GPU_STREAM_ZERO = 255 };

struct gpu_vertex_stream
{
	gpu_buffer buffer;
	uint32_t offset;
	uint32_t stride;
};

struct gpu_vertex_attribute
{
	uint8_t format;
	uint8_t stream;
	/* bytes from the start of the stream's vertex */
	uint16_t offset;
};

enum
{
	GPU_PRIMITIVE_POINTS, GPU_PRIMITIVE_LINES, GPU_PRIMITIVE_LINE_LOOP, GPU_PRIMITIVE_LINE_STRIP,
	GPU_PRIMITIVE_TRIANGLES, GPU_PRIMITIVE_TRIANGLE_STRIP, GPU_PRIMITIVE_TRIANGLE_FAN,
};

/* one draw, complete: the backend applies it without reference to the draws
before it */
struct gpu_draw
{
	/* at least one; a depth target is a depth-stencil texture */
	gpu_texture color_target, depth_target;
	/* 0: the targets' textures are drawn into; else their multisampled
	storage, with this many samples a pixel */
	uint32_t samples;
	gpu_shader vertex_shader, pixel_shader;
	struct gpu_viewport viewport;
	struct gpu_rect scissor;
	struct gpu_depth_stencil_state depth_stencil;
	struct gpu_blend_state blend;
	struct gpu_raster_state raster;
	struct gpu_stage stages[GPU_STAGE_COUNT];
	struct gpu_vertex_stream streams[GPU_STREAM_COUNT];
	struct gpu_vertex_attribute attributes[GPU_ATTRIBUTE_COUNT];
	float constant_values[GPU_ATTRIBUTE_COUNT][4];
	/* 0: the vertices are drawn in order; else their 16-bit indices */
	gpu_buffer index_buffer;
	uint32_t index_offset;
	uint32_t primitive;
	uint32_t count;
	/* added to each index */
	int32_t base_vertex;
};

/* draws; returns 0 if nothing was drawn because the shaders do not link */
uint32_t gpu_draw(const struct gpu_draw *draw, struct gpu_constant_store *constants,
	const struct gpu_uniforms *uniforms);

/* ---------- clears */

/* gpu_clear.flags: what is cleared */
enum { GPU_CLEAR_COLOR = 1, GPU_CLEAR_DEPTH = 2, GPU_CLEAR_STENCIL = 4 };

struct gpu_clear
{
	/* as a draw's (struct gpu_draw); depth and stencil are cleared only with
	a depth target */
	gpu_texture color_target, depth_target;
	uint32_t samples;
	uint32_t flags;
	/* the color channels cleared: bit 0 red, 1 green, 2 blue, 3 alpha */
	uint8_t channel_mask;
	uint8_t pad[3];
	/* ARGB, as D3DCOLOR */
	uint32_t color;
	float depth;
	uint32_t stencil;
};

/* clears each of the rectangles of the targets' pixels (rows from the top),
whatever the draws' masks are; with no flags, it only makes the targets the
current ones */
void gpu_clear(const struct gpu_clear *clear, const struct gpu_rect *rectangles, uint32_t count);

/* ---------- visibility (occlusion) tests */

/* the slots a test's count is kept in; slot 0 is the backend's own */
enum { GPU_VISIBILITY_SLOTS = 4096 };

/* the draws until gpu_visibility_end count their samples */
void gpu_visibility_begin(void);
/* ... into slot (1 to GPU_VISIBILITY_SLOTS - 1) */
void gpu_visibility_end(uint32_t slot);
/* 1, and in *samples the slot's latest count the GPU has finished (as the
capabilities' occlusion counts), if there is one not given before or the
backend keeps them all; else 0 */
uint32_t gpu_visibility_result(uint32_t slot, uint32_t *samples);

/* ---------- frames */

/* display.anti_aliasing's passes over a window's 3D view (gpu_gl_post.c) */
enum { GPU_ANTI_ALIAS_FXAA = 1, GPU_ANTI_ALIAS_SMAA };

/* makes the pass's programs and textures now, rather than in the middle of a
frame; 0 if they cannot be made (once 0, always 0) */
uint32_t gpu_anti_alias_prepare(uint32_t pass);
/* the pass, in place, on the corners x0, y0 to x1, y1 (rows from the top) of
a color render target, which stops being multisampled */
void gpu_anti_alias(uint32_t pass, gpu_texture target, const int32_t corners[4]);

/* sends the commands so far to the GPU */
void gpu_flush(void);
/* shows the back buffer, letterboxed in the window, and starts the next
frame */
void gpu_present(gpu_texture back_buffer);

#endif
