/*
XGPU_DEVICE.H

The Xbox Direct3D 8 device, between the game and a renderer.

d3d8_device.c is the device the game drives: the state its calls set (render
and texture stage states, vertex shaders and their constants, streams,
targets, the viewport), the screen's size and the settings the renderers
share (anti-aliasing, the shadow maps' size), the render targets by their
addresses, the mirror of vertex data in the contiguous window, immediate
mode, the vertical blank and the frame traces. A renderer (struct
xgpu_backend) draws what the device holds: OpenGL (d3d8_gl.c) everywhere,
Direct3D 12 (port/windows/src/d3d12_device.c) on Windows. display.renderer
chooses one as the device is made.
*/

#ifndef __HALO_LINUX_XGPU_DEVICE_H
#define __HALO_LINUX_XGPU_DEVICE_H

#include "xgpu.h"

/* ---------- vertex shaders */

#define VERTEX_SHADER_SIGNATURE 0x76736864UL /* 'vshd' */
#define VERTEX_PROGRAM_SLOTS 136

struct vertex_element
{
	unsigned char reg;
	unsigned char stream;
	unsigned char type;
	unsigned char bytes;
	unsigned short offset;
};

struct vertex_shader_object
{
	unsigned long signature;
	unsigned long id;
	DWORD *instructions;
	unsigned long instruction_count;
	struct vertex_element elements[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long element_count;
	unsigned long packed_mask;
	/* the OpenGL renderer's shaders: [0] streams per the declaration, [1]
	immediate mode */
	unsigned int shader[2];
	/* one of the game's model lighting programs (halo_vertex_shader_lighting),
	whose draws can be lit for each pixel (display.per_pixel_lighting): where
	its lighting's normal and position are (lighting.lights is 0 for the
	others), and its shaders that hand them on, as shader[] */
	struct nv2a_vertex_lighting lighting;
	unsigned int lit_shader[2];
	/* a shader lit for each pixel failed to compile or link: lit as the
	vertex shader lights it from then on */
	BOOL lighting_failed;
};

/* ---------- the uniforms a draw sets besides the vertex constants */

struct draw_uniforms
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
};

/* the uniforms of the latest draws, from the state they were converted from
(xgpu_draw_uniforms_update); the serial counts the conversions */
extern struct draw_uniforms xgpu_draw_uniforms;
extern unsigned long xgpu_draw_uniforms_serial;

/* converts the state the uniforms come from when it differs from the last
draw's: texture_scale is the draw's (each stage's texel scale for linear
textures, bind_textures) */
void xgpu_draw_uniforms_update(const float texture_scale[4][4]);

/* ---------- render targets */

struct render_target_entry
{
	struct render_target_entry *next;
	/* the next with the same address bucket */
	struct render_target_entry *next_in_bucket;
	struct xgpu_render_target target;
	unsigned long last_rendered;
	/* the back buffer or its depth buffer, which the 3D view is drawn into:
	multisampled with multisampling */
	BOOL screen_buffer;
};

/* the entry of a surface's target, made with its renderer's textures the
first time; NULL for none */
struct render_target_entry *xgpu_render_target_get(const D3DSurface *surface);
void xgpu_surface_dimensions(const D3DSurface *surface, unsigned long *width, unsigned long *height, BOOL *depth);

/* ---------- the device */

#define VISIBILITY_TEST_SLOTS 4096

struct xgpu_device
{
	D3DPRESENT_PARAMETERS presentation;
	D3DSurface back_buffer;
	D3DSurface depth_buffer;
	D3DSurface *render_target;
	D3DSurface *depth_stencil;
	D3DVIEWPORT8 viewport;
	D3DMATRIX transforms[D3DTS_MAX];
	D3DBaseTexture *textures[D3DTSS_MAXSTAGES];
	D3DPalette *palettes[D3DTSS_MAXSTAGES];
	D3DSHADERCONSTANTMODE shader_constant_mode;

	struct vertex_shader_object *vertex_shader;
	struct vertex_shader_object *program_slots[VERTEX_PROGRAM_SLOTS];
	unsigned long program_address;
	float constants[XGPU_VERTEX_CONSTANT_COUNT][4];
	float viewport_scale[4];
	float viewport_offset[4];

	struct
	{
		DWORD data;
		UINT stride;
	} streams[16];
	/* SetIndices' base vertex: added to every index of an indexed draw (the
	dynamic vertex buffers keep each buffer's vertices at an offset into one
	vertex buffer, and their triangles count from 0: contrails, lightning) */
	UINT base_vertex_index;

	/* the current value of each input register (SetVertexData) */
	float attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	BOOL immediate_active;
	D3DPRIMITIVETYPE immediate_type;
	float *immediate_vertices;
	unsigned long immediate_count;
	unsigned long immediate_capacity;

	BOOL visibility_test_active;

	/* the renderer's: the most samples a pixel it draws with, and its largest
	target (texture and renderbuffer) each way */
	int maximum_samples;
	long maximum_target_size;

	unsigned long frame;
	unsigned long next_vertex_shader_id;
	/* a renderer draws (a window and its GPU device exist) */
	BOOL ready;
	BOOL created;
};

extern struct xgpu_device xgpu_device;

/* how the current declaration feeds a program's input registers
(nv2a_vertex_inputs): immediate mode's every register as floats */
void xgpu_vertex_inputs(BOOL immediate, struct nv2a_vertex_inputs *inputs);

/* the program that runs: the one loaded at the selected address, else the
current shader's own */
static inline struct vertex_shader_object *current_program(void)
{
	struct vertex_shader_object *program = xgpu_device.program_slots[xgpu_device.program_address];

	return program ? program : xgpu_device.vertex_shader;
}

/* ---------- vertex constants

Each vertex constant register's serial is the value the serial took when the
register last changed; a program's registers are current up to the serial it
recorded when it last uploaded them (d3d8_device.c). */

#define XGPU_CONSTANT_LOG_SIZE 1024

struct xgpu_constant_serials
{
	unsigned long long registers[XGPU_VERTEX_CONSTANT_COUNT];
	unsigned long long serial;
	/* the register each of the latest serials changed */
	unsigned char log[XGPU_CONSTANT_LOG_SIZE];
	/* the registers changed since the last upload (to any program): the
	smallest and largest, and the serial that upload was current to */
	unsigned long long checkpoint_serial;
	unsigned long checkpoint_first, checkpoint_last;
};

extern struct xgpu_constant_serials xgpu_constant_serials;

/* the vertex constant register of each of the per-pixel lighting's
(XGPU_MODEL_LIGHT_COUNT) */
static inline unsigned long model_light_register(int light)
{
	return (unsigned long)(XGPU_VERTEX_CONSTANT_BIAS + (light ? -80 + light : -82));
}

/* display.per_pixel_lighting, read again when a setting changes */
BOOL xgpu_per_pixel_lighting(void);

/* ---------- the screen */

enum
{
	_anti_aliasing_off,
	_anti_aliasing_fxaa,
	_anti_aliasing_smaa,
	_anti_aliasing_ssaa,
	_anti_aliasing_msaa,
};

/* display.anti_aliasing's mode in effect, and multisampling's samples a
pixel (at most the renderer's) */
int xgpu_anti_aliasing(void);
int xgpu_anti_aliasing_samples(void);
/* the columns the menus shift by to center on a wide screen */
long xgpu_ui_offset(void);
/* the pixel edge of a coordinate in a target's units, at its scale */
long xgpu_scaled_pixel(float coordinate, float scale);

/* ---------- the mirror of the contiguous window

Vertex and index buffers live in the Xbox's contiguous memory, where most
never change once loaded. The mirror keeps a copy of that memory in the
renderer's buffers (one per segment, made when first needed) and uploads a
page only when it is first drawn from or after the game has written it:
pages are write-protected once uploaded, as cached textures are
(memory_watch.c). Pages the game rewrites frame after frame (dynamic
vertices) would fault on every write; after a few such rewrites a page
counts as volatile for a while, and draws that use it stream their data. */

#define MIRROR_SEGMENT_SIZE 0x400000UL
#define MIRROR_SEGMENT_COUNT (PLATFORM_CONTIGUOUS_SIZE / MIRROR_SEGMENT_SIZE)

/* makes [address, address + size) current in the mirror, giving the segment
that holds it, the range's offset in that segment and the newest upload
generation of its pages (which changes whenever its contents do); FALSE if
the range is outside the window, spans two segments or is volatile */
BOOL xgpu_mirror_range(unsigned long address, unsigned long size, unsigned long *segment, unsigned long *offset,
	unsigned long *generation);

/* the smallest and largest index of an index range; cached by its address,
count and generation when the mirror holds it */
void xgpu_index_extent(const WORD *indices, unsigned long count, unsigned long generation, BOOL cached,
	unsigned long *minimum, unsigned long *maximum);
/* quads as two triangles each; malloc'd */
WORD *xgpu_quad_indices(const WORD *indices, unsigned long count, unsigned long *out_count);
/* a triangle fan (or polygon) as a list of triangles; malloc'd */
WORD *xgpu_fan_indices(const WORD *indices, unsigned long count, unsigned long *out_count);

/* ---------- statistics and tracing */

/* debug.gpu_stats prints these once a second */
struct xgpu_statistics
{
	unsigned long draws, immediate_draws, clears, presents;
	unsigned long skipped_no_program, skipped_no_target, skipped_link;
	unsigned long target_changes;
	/* vertex and index bytes drawn from the mirror, and streamed */
	unsigned long mirrored_bytes, streamed_bytes;
};

extern struct xgpu_statistics xgpu_statistics;

/* debugging settings, read once as the renderer starts */
struct xgpu_debug_settings
{
	/* debug.gpu_skip_vertex_shaders "<id>,<id>..." drops draws by vertex
	shader, for finding which pass produces something (port_config.c) */
	const char *skip_vertex_shaders;
	const char *dump_shaders;
	BOOL statistics;
};

extern struct xgpu_debug_settings xgpu_debug_settings;

/* whether the draw of this program is dropped (skip_vertex_shaders) */
BOOL xgpu_skip_program(const struct vertex_shader_object *program);

/* debug.gpu_trace_frame: whether this frame is traced, and a draw's line
(the same for every renderer: the device's state, not the renderer's) */
BOOL xgpu_trace_frame(void);
void xgpu_trace_draw(const char *kind, D3DPRIMITIVETYPE type, unsigned long count, const float *first_vertex);

/* ---------- pixel shader keys */

/* the parts of a draw's pixel shader key its textures do not decide: the
combiner states (constants left out), the texture modes; then, after the
textures and targets are bound, the per-stage and test states.
target_samples is the bound targets' samples a pixel (1 if not
multisampled) */
void xgpu_pixel_shader_key_begin(struct nv2a_pixel_shader_key *key);
void xgpu_pixel_shader_key_finish(struct nv2a_pixel_shader_key *key, int target_samples);

/* ---------- helpers */

static inline float dword_to_float(DWORD value)
{
	union { DWORD d; float f; } u;

	u.d = value;
	return u.f;
}

static inline void color_to_vec4(D3DCOLOR color, float *out)
{
	out[0] = ((color >> 16) & 0xff) / 255.0f;
	out[1] = ((color >> 8) & 0xff) / 255.0f;
	out[2] = (color & 0xff) / 255.0f;
	out[3] = ((color >> 24) & 0xff) / 255.0f;
}

/* size is a multiple of 4 */
static inline unsigned long hash_words(const void *data, unsigned long size)
{
	const DWORD *words = data;
	unsigned long hash = 2166136261UL;

	for (size /= 4; size; size--)
		hash = (hash ^ *words++) * 16777619UL;
	return hash;
}

static inline unsigned long stage_texture_mode(int stage)
{
	return (D3D__RenderState[D3DRS_PSTEXTUREMODES] >> (5 * stage)) & 0x1f;
}

/* ---------- renderers */

struct xgpu_backend
{
	const char *name;
	/* makes the window and the GPU device, filling in xgpu_device's
	maximum_samples and maximum_target_size; FALSE if it cannot (no window is
	left behind) */
	BOOL (*initialize)(unsigned long width, unsigned long height);
	/* what display.anti_aliasing's mode needs (FXAA's or SMAA's programs),
	made as the setting is chosen rather than in the middle of a frame */
	void (*anti_aliasing_prepare)(int mode);

	/* a render target's textures, for target's size, depth and scale */
	void (*render_target_create)(struct xgpu_render_target *target);

	/* the draws (the device's state as it is now) */
	void (*draw_vertices)(D3DPRIMITIVETYPE type, UINT start_vertex, UINT vertex_count);
	void (*draw_indexed_vertices)(D3DPRIMITIVETYPE type, UINT vertex_count, const WORD *indices);
	/* count vertices of XGPU_VERTEX_ATTRIBUTE_COUNT float[4] each */
	void (*draw_immediate)(D3DPRIMITIVETYPE type, const float *vertices, unsigned long count);
	void (*clear)(DWORD count, const D3DRECT *rectangles, DWORD flags, D3DCOLOR color, float z, DWORD stencil);

	/* visibility tests; index is the result slot, 1 to VISIBILITY_TEST_SLOTS - 1 */
	void (*visibility_begin)(void);
	void (*visibility_end)(DWORD index);
	HRESULT (*visibility_result)(DWORD index, UINT *result);

	/* display.anti_aliasing's pass over a window's 3D view (FXAA, SMAA) */
	void (*anti_alias)(short x0, short y0, short x1, short y1);
	/* the back buffer's pixels saved as a BMP file (screenshot.h); 1 on success */
	int (*save_screenshot)(struct render_target_entry *back_buffer, const char *path);
	/* the back buffer shown in the window */
	void (*present)(struct render_target_entry *back_buffer);
	/* the work recorded so far handed to the GPU */
	void (*flush)(void);

	/* the mirror's segment, made the first time, given bytes at offset;
	unused: no draw queued so far can read them (they are uploaded for the
	first time) */
	void (*mirror_upload)(unsigned long segment, unsigned long offset, unsigned long size, const void *data,
		BOOL unused);

	/* textures (xgpu.h) */
	unsigned int (*texture_new)(int type, int format, unsigned long width, unsigned long height, unsigned long depth,
		unsigned long levels);
	void (*texture_write)(unsigned int texture, unsigned long face, unsigned long level, const void *data);
	void (*texture_write_rows)(unsigned int texture, unsigned long first_row, unsigned long rows, const void *data);
	void (*texture_mipmaps)(unsigned int texture, const void *level0);
	void (*texture_delete)(unsigned int texture);
	BOOL (*texture_compressed_supported)(unsigned long width, unsigned long height);
	/* debug.texture_dump_directory: level 0 written out (NULL: none) */
	void (*texture_dump)(unsigned int texture, const struct xgpu_texture_description *description);
};

extern const struct xgpu_backend xgpu_backend_gl;
#ifdef _WIN32
extern const struct xgpu_backend xgpu_backend_d3d12;
#endif

/* the renderer in use, NULL until the device is made (or without one:
debug.null_renderer) */
extern const struct xgpu_backend *xgpu_backend;

#endif
