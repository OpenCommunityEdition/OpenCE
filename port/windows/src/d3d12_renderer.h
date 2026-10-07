/*
D3D12_RENDERER.H

The Direct3D 12 renderer, between its two halves. They cannot share a file:
the platform layer is compiled with the Xbox SDK's declarations
(port/include/xdk), and Direct3D 12 needs the Windows SDK's, which declare
many of the same names differently (port/windows/README.md). So:

- d3d12_device.c is the renderer of the device (struct xgpu_backend,
  xgpu_device.h), compiled as the platform layer is. It reads the device's
  state and describes each draw in the terms below: the shaders' HLSL (from
  nv2a_vsh.c and nv2a_psh.c), the pipeline state, the textures and targets by
  handle, the vertices in the mirror or in memory, the constants. It keeps
  what is the device's: the render targets by address, the shader caches,
  the visibility tests' areas, the screenshots' files.
- win32_d3d12_*.c (win32_d3d12.h) do the Direct3D 12 work, compiled with the
  Windows SDK: the device, the swap chain, the frames in flight, the
  resources, the pipeline states, the recording of the draws, the passes.

Between them, only C's own types. The values below that are Direct3D 12's
(D3D12R_*) are its enumerations' numbers, which the host half checks against
the Windows SDK's as it is compiled.

Every draw goes through d3d12r_draw, described whole and labelled with the
game's pass and window (xgpu_device.h), and each window's camera comes
through d3d12r_view_begin: what a later renderer of the same frames needs (a
raytracer's, which builds its scene from the opaque passes' geometry and
sees it from the window's camera) is there to be read in one place. A
raytracer runs in a 64-bit process of its own (DXR is not offered to 32-bit
ones); d3d12r_opaque_done is where its frame can begin.
*/

#ifndef __HALO_WINDOWS_D3D12_RENDERER_H
#define __HALO_WINDOWS_D3D12_RENDERER_H

/* (platform.h's, for the half that does not see it) */
void platform_log(const char *format, ...);

/* ---------- textures, as xgpu.h's (d3d12_device.c checks they agree) */

enum
{
	_d3d12r_texture_2d,
	_d3d12r_texture_cube,
	_d3d12r_texture_3d,
};

enum
{
	_d3d12r_format_bgra8,
	_d3d12r_format_rgba8,
	_d3d12r_format_dxt1,
	_d3d12r_format_dxt3,
	_d3d12r_format_dxt5,
	/* the renderer's own (SMAA's lookup textures) */
	_d3d12r_format_rg8,
	_d3d12r_format_r8,
};

enum
{
	_d3d12r_sampler_none,
	_d3d12r_sampler_2d,
	_d3d12r_sampler_3d,
	_d3d12r_sampler_cube,
};

/* the mirror of the contiguous window (xgpu_device.h) */
#define D3D12R_MIRROR_SEGMENT_SIZE 0x400000UL
#define D3D12R_MIRROR_SEGMENT_COUNT 32

#define D3D12R_STAGES 4
#define D3D12R_STREAMS 16
#define D3D12R_ATTRIBUTES 16
#define D3D12R_VISIBILITY_SLOTS 4096

/* ---------- Direct3D 12's numbers */

enum
{
	D3D12R_BLEND_ZERO = 1,
	D3D12R_BLEND_ONE = 2,
	D3D12R_BLEND_SRC_COLOR = 3,
	D3D12R_BLEND_INV_SRC_COLOR = 4,
	D3D12R_BLEND_SRC_ALPHA = 5,
	D3D12R_BLEND_INV_SRC_ALPHA = 6,
	D3D12R_BLEND_DEST_ALPHA = 7,
	D3D12R_BLEND_INV_DEST_ALPHA = 8,
	D3D12R_BLEND_DEST_COLOR = 9,
	D3D12R_BLEND_INV_DEST_COLOR = 10,
	D3D12R_BLEND_SRC_ALPHA_SAT = 11,
	D3D12R_BLEND_BLEND_FACTOR = 14,
	D3D12R_BLEND_INV_BLEND_FACTOR = 15,

	D3D12R_BLEND_OP_ADD = 1,
	D3D12R_BLEND_OP_SUBTRACT = 2,
	D3D12R_BLEND_OP_REV_SUBTRACT = 3,
	D3D12R_BLEND_OP_MIN = 4,
	D3D12R_BLEND_OP_MAX = 5,

	D3D12R_COMPARISON_NEVER = 1,
	D3D12R_COMPARISON_ALWAYS = 8,

	D3D12R_STENCIL_OP_KEEP = 1,
	D3D12R_STENCIL_OP_ZERO = 2,
	D3D12R_STENCIL_OP_REPLACE = 3,
	D3D12R_STENCIL_OP_INCR_SAT = 4,
	D3D12R_STENCIL_OP_DECR_SAT = 5,
	D3D12R_STENCIL_OP_INVERT = 6,
	D3D12R_STENCIL_OP_INCR = 7,
	D3D12R_STENCIL_OP_DECR = 8,

	D3D12R_CULL_NONE = 1,
	D3D12R_CULL_FRONT = 2,
	D3D12R_CULL_BACK = 3,

	D3D12R_FILL_WIREFRAME = 2,
	D3D12R_FILL_SOLID = 3,

	D3D12R_TOPOLOGY_TYPE_POINT = 1,
	D3D12R_TOPOLOGY_TYPE_LINE = 2,
	D3D12R_TOPOLOGY_TYPE_TRIANGLE = 3,

	D3D12R_TOPOLOGY_POINTLIST = 1,
	D3D12R_TOPOLOGY_LINELIST = 2,
	D3D12R_TOPOLOGY_LINESTRIP = 3,
	D3D12R_TOPOLOGY_TRIANGLELIST = 4,
	D3D12R_TOPOLOGY_TRIANGLESTRIP = 5,

	D3D12R_COLOR_WRITE_RED = 1,
	D3D12R_COLOR_WRITE_GREEN = 2,
	D3D12R_COLOR_WRITE_BLUE = 4,
	D3D12R_COLOR_WRITE_ALPHA = 8,
	D3D12R_COLOR_WRITE_ALL = 15,

	D3D12R_FILTER_TYPE_POINT = 0,
	D3D12R_FILTER_TYPE_LINEAR = 1,
	D3D12R_FILTER_ANISOTROPIC = 0x55,

	D3D12R_ADDRESS_WRAP = 1,
	D3D12R_ADDRESS_MIRROR = 2,
	D3D12R_ADDRESS_CLAMP = 3,
	D3D12R_ADDRESS_BORDER = 4,
};

/* D3D12_ENCODE_BASIC_FILTER's, of the standard reduction */
#define D3D12R_BASIC_FILTER(minification, magnification, mip) \
	((((minification) & 3) << 4) | (((magnification) & 3) << 2) | ((mip) & 3))

/* DXGI's formats of the vertex elements */
enum
{
	D3D12R_FORMAT_R32G32B32A32_FLOAT = 2,
	D3D12R_FORMAT_R32G32B32_FLOAT = 6,
	D3D12R_FORMAT_R16G16B16A16_SNORM = 13,
	D3D12R_FORMAT_R16G16B16A16_SINT = 14,
	D3D12R_FORMAT_R32G32_FLOAT = 16,
	D3D12R_FORMAT_R8G8B8A8_UNORM = 28,
	D3D12R_FORMAT_R16G16_SNORM = 37,
	D3D12R_FORMAT_R16G16_SINT = 38,
	D3D12R_FORMAT_R32_FLOAT = 41,
	D3D12R_FORMAT_R32_UINT = 42,
	D3D12R_FORMAT_R8G8_UNORM = 49,
	D3D12R_FORMAT_R16_SNORM = 58,
	D3D12R_FORMAT_R16_SINT = 59,
	D3D12R_FORMAT_R8_UNORM = 61,
	D3D12R_FORMAT_B8G8R8A8_UNORM = 87,
};

/* ---------- the device */

/* the device and the swap chain in the window (an HWND); FALSE if they
cannot be made. The most samples a pixel of the targets, and the largest
target */
int d3d12r_initialize(void *window, int width, int height, int debug, int gpu_validation, int *maximum_samples,
	long *maximum_target_size);
void d3d12r_shutdown(void);
/* a back buffer's texture shown, letterboxed, in the window's drawable
size, and the next frame begun */
void d3d12r_present(unsigned int back_buffer, int window_width, int window_height, int vsync);
/* a target's pixels, rows from the top of 4 bytes B, G, R, A, into pixels
(width x height); 1 on success */
int d3d12r_read_pixels(unsigned int texture, unsigned char *pixels, unsigned long width, unsigned long height);

/* HLSL compiled (a vertex or a pixel shader); NULL with the log written */
void *d3d12r_shader_compile(const char *source, int pixel, const char *name);

/* ---------- textures (xgpu.h's, and render targets) */

unsigned int d3d12r_texture_new(int type, int format, unsigned long width, unsigned long height, unsigned long depth,
	unsigned long levels);
void d3d12r_texture_write(unsigned int texture, unsigned long face, unsigned long level, const void *data);
void d3d12r_texture_write_rows(unsigned int texture, unsigned long first_row, unsigned long rows, const void *data);
void d3d12r_texture_mipmaps(unsigned int texture, const void *level0);
void d3d12r_texture_delete(unsigned int texture);
int d3d12r_texture_compressed_supported(unsigned long width, unsigned long height);

/* a render target's texture: B8G8R8A8, or depth and stencil (D24S8); with
samples (more than 1), multisampled storage that draws go to, cleared */
unsigned int d3d12r_target_create(int depth, unsigned long width, unsigned long height, int samples);
/* a multisampled color target's pixels resolved into its texture */
void d3d12r_target_resolve(unsigned int multisample, unsigned int texture);
/* a texture of levels mip levels that render targets are copied into
(B8G8R8A8), a level copied from a target, levels from first_level on made
from the ones before */
unsigned int d3d12r_composite_create(unsigned long width, unsigned long height, unsigned long levels);
void d3d12r_composite_copy_level(unsigned int composite, unsigned long level, unsigned int target);
void d3d12r_composite_mipmaps(unsigned int composite, unsigned long first_level);

/* the mirror's segment, made the first time, given bytes at offset */
void d3d12r_mirror_upload(unsigned long segment, unsigned long offset, unsigned long size, const void *data);

/* ---------- the draws */

/* a sampler's state, as D3D12_SAMPLER_DESC's */
struct d3d12r_sampler
{
	unsigned int filter;
	unsigned int address_u, address_v, address_w;
	unsigned int maximum_anisotropy;
	float mip_lod_bias, minimum_lod, maximum_lod;
	float border_color[4];
};

/* an input register read from a stream (DXGI's format) */
struct d3d12r_element
{
	unsigned char reg, stream;
	unsigned short offset;
	unsigned int format;
};

/* everything a pipeline state is made from; its bytes are its key, so it is
cleared before it is filled in */
struct d3d12r_pipeline
{
	/* the shaders' and the input layout's ids (d3d12_device.c's caches) */
	unsigned long vertex_shader, pixel_shader, input_layout;
	int depth_bias;
	float slope_scaled_depth_bias;
	unsigned char topology_type, samples, has_color, has_depth;
	unsigned char blend_enable, blend_source, blend_destination, blend_operation;
	unsigned char blend_source_alpha, blend_destination_alpha, write_mask, depth_enable;
	unsigned char depth_write, depth_function, stencil_enable, stencil_read_mask;
	unsigned char stencil_write_mask, stencil_function, stencil_fail, stencil_depth_fail;
	unsigned char stencil_pass, fill_mode, cull_mode, front_counter_clockwise;
};

struct d3d12r_stream
{
	/* where the mirror holds it: its segment (-1: nowhere) and offset */
	long segment;
	unsigned long offset;
	/* else its bytes from data, streamed */
	const void *data;
	unsigned long size, stride;
};

struct d3d12r_draw
{
	/* the shaders (d3d12r_shader_compile) and the input layout's elements */
	void *vertex_shader, *pixel_shader;
	const struct d3d12r_element *elements;
	unsigned long element_count;
	struct d3d12r_pipeline pipeline;
	unsigned int topology;

	/* each stage's texture (0: none, its sampler type's null texture) and
	sampler; a stage sampling the color target samples a copy of it */
	unsigned int textures[D3D12R_STAGES];
	int sampler_types[D3D12R_STAGES];
	struct d3d12r_sampler samplers[D3D12R_STAGES];

	/* the targets' textures (0: none), in the targets' pixels */
	unsigned int color, depth;
	float viewport[6]; /* x, y, width, height, minimum and maximum depth */
	long scissor[4]; /* left, top, right, bottom */
	unsigned int stencil_reference;
	float blend_factor[4];

	/* the constant buffers (struct xgpu_hlsl_vertex_constants and
	xgpu_hlsl_pixel_constants), uploaded again only when their serial changes */
	const void *vertex_constants, *pixel_constants;
	unsigned long vertex_constants_size, pixel_constants_size;
	unsigned long vertex_constants_serial, pixel_constants_serial;

	/* the vertices of each stream from vertex 0 of the draw */
	struct d3d12r_stream streams[D3D12R_STREAMS];
	/* without indices: the vertices drawn. With them, 16-bit indices from the
	mirror (index_segment, index_offset) or from memory, each the vertex
	index + base_vertex */
	unsigned long vertex_count;
	unsigned long index_count;
	long index_segment;
	unsigned long index_offset;
	const unsigned short *indices;
	long base_vertex;

	/* what the game is drawing (xgpu_device.h's xgpu_draw_label): its pass
	(-1: none) and the pass's name, the window (-1: none) */
	short pass, window;
	const char *pass_name;
};

void d3d12r_draw(const struct d3d12r_draw *draw);

/* a window's camera, as the window begins (xgpu_device.h's xgpu_view) */
struct d3d12r_view
{
	short window;
	int mirrored;
	float position[3], forward[3], up[3];
	float vertical_field_of_view, z_near, z_far;
	short viewport[4];
};

void d3d12r_view_begin(const struct d3d12r_view *view);
/* the window's solid surfaces all drawn: the frame's work so far is handed
to the GPU (the point a raytracer's scene and depth are complete at) */
void d3d12r_opaque_done(void);

/* channels (D3D12R_COLOR_WRITE_*) of the color target, its depth or its
stencil cleared over rectangles of its pixels (left, top, right, bottom) */
void d3d12r_clear(unsigned int color, unsigned int depth, unsigned int channels, const float rgba[4], int clear_depth,
	float z, int clear_stencil, unsigned int stencil, const long (*rectangles)[4], unsigned int count);

/* visibility tests: samples passed between a begin and an end, into the
slot (0 to D3D12R_VISIBILITY_SLOTS - 1); the latest count the GPU has
written there, or with wait the slot's last test's, waited for */
void d3d12r_visibility_begin(void);
void d3d12r_visibility_end(unsigned long slot);
unsigned long long d3d12r_visibility_samples(unsigned long slot, int wait);

/* ---------- the passes */

/* FXAA's (smaa 0) or SMAA's programs made; for SMAA, its source
(port/third_party/smaa) and its lookup textures' texels (area 160 x 560
of R8G8, search 64 x 16 of R8). 1 if they could be */
int d3d12r_anti_aliasing_prepare(int smaa, const char *smaa_source, const void *area, const void *search);
/* FXAA or SMAA over the corners (x0, y0, x1, y1) of a color target */
void d3d12r_anti_alias(int smaa, unsigned int target, const long corners[4]);

#endif
