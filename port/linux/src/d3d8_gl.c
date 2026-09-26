/*
D3D8_GL.C

The Xbox Direct3D 8 device, implemented with OpenGL 4.5.

The game drives the device through the XDK's inline functions, which keep
the "simple" render states in D3D__RenderState and call into this file for
everything else. At each draw the full state is read back from there and
translated: the vertex program into GLSL once per shader (nv2a_vsh.c), the
pixel shader - texture stages and register combiners, 57 render states -
into GLSL once per combination (nv2a_psh.c), and the rest into GL state.

Conventions carried over from the Xbox:
- Clip space is D3D's (depth 0..1, y down in window space). glClipControl
  (GL_UPPER_LEFT, GL_ZERO_TO_ONE) makes GL agree, so viewports, scissors and
  texture rows line up with D3D's top-left origin; the window blit at
  Present flips the image back for display.
- Render targets and textures are identified by the physical address in
  their Data field. A texture whose data is a render target samples the GL
  render target directly (render-to-texture).
- Vertex data is read from guest memory at draw time.
*/

#include "xgpu.h"
#include "sdl_platform.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void d3d8_surface_initialize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);

#ifdef HALO_ANDROID
/* OpenGL ES 3 (port/android/README.md): the desktop formats, enumerants
and entry points used below that ES lacks */
#define GL_BGRA GL_RGBA
#define glDepthRange glDepthRangef
#define glClearDepth glClearDepthf
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84fe
#endif
#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812d
#endif

/* what the context supports (gl_initialize) */
struct xgpu_capabilities xgpu_capabilities;
#endif

/* ---------- the screen's width

The Xbox screen is 640x480. On Android the game renders at the device's
aspect ratio instead: 480 lines, and as many columns as the display's shape
gives (HALO_SCREEN_WIDTH, set by the host; 640 keeps 4:3). The game's
camera derives its horizontal field of view from the viewport, so the 3D
view simply widens. The menus and full-screen overlays are laid out for 640
columns; while they draw (halo_android_ui_offset), everything shifts right
to center them. */

#ifdef HALO_ANDROID
static long ui_offset;
#define UI_OFFSET ((GLint)ui_offset)

long halo_android_screen_width(void)
{
	static long width;

	if (!width)
	{
		const char *text = getenv("HALO_SCREEN_WIDTH");

		width = text ? atol(text) : 640;
		if (width < 640)
			width = 640;
		if (width > 1600)
			width = 1600;
		width &= ~1L;
	}
	return width;
}

void halo_android_ui_offset(unsigned char centered)
{
	ui_offset = centered ? (halo_android_screen_width() - 640) / 2 : 0;
}
#else
#define UI_OFFSET 0
#endif

/* ---------- state the XDK header's inline functions read and write */

DWORD D3D__RenderState[D3DRS_MAX];
DWORD D3D__TextureState[D3DTSS_MAXSTAGES][D3DTSS_MAX];
WORD *D3D__IndexData;
BYTE D3D__StateBlockDirty[1024];

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
	/* [0] streams per the declaration, [1] immediate mode (all floats) */
	GLuint shader[2];
};

/* ---------- programs */

struct fragment_entry
{
	struct fragment_entry *next;
	unsigned long hash;
	struct nv2a_pixel_shader_key key;
	GLuint shader;
};

struct program_entry
{
	struct program_entry *next;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLint constants;
	GLint viewport_scale;
	GLint viewport_offset;
	GLint point_size;
	GLint ps_c0, ps_c1, ps_final_c0, ps_final_c1;
	GLint fog_color, fog_parameters, alpha_reference;
	GLint bump_matrix, bump_luminance, texture_scale;
	GLint texture_lod_bias;
	GLint screen_offset;
};

#define FRAGMENT_BUCKETS 1024
#define PROGRAM_BUCKETS 1024

static struct fragment_entry *fragment_buckets[FRAGMENT_BUCKETS];
static struct program_entry *program_buckets[PROGRAM_BUCKETS];

/* ---------- render targets */

struct render_target_entry
{
	struct render_target_entry *next;
	struct xgpu_render_target target;
	unsigned long last_rendered;
};

struct framebuffer_entry
{
	struct framebuffer_entry *next;
	GLuint color;
	GLuint depth;
	GLuint framebuffer;
};

static struct render_target_entry *render_targets;
static struct framebuffer_entry *framebuffers;

/* ---------- the device */

#ifdef HALO_ANDROID
/* Mobile drivers (Mali) keep every orphaned copy of a buffer until the GPU
is done with it, so a large buffer orphaned each frame costs its size per
frame in flight and more. Instead each frame streams into the next of a few
smaller buffers, reusing one only once the GPU has finished the frame that
last used it (host_gl_wait_frame). A busy frame streams about 5 MB of
vertices. */
#define STREAM_BUFFER_SIZE (16 * 1024 * 1024)
#define INDEX_BUFFER_SIZE (2 * 1024 * 1024)
#define STREAM_BUFFER_RING 3
#else
#define STREAM_BUFFER_SIZE (32 * 1024 * 1024)
#define INDEX_BUFFER_SIZE (8 * 1024 * 1024)
#endif
#define VISIBILITY_TEST_SLOTS 4096
#ifdef HALO_ANDROID
#define VISIBILITY_QUERY GL_ANY_SAMPLES_PASSED
#define VISIBILITY_ALL_SAMPLES 1000000
#else
#define VISIBILITY_QUERY GL_SAMPLES_PASSED
#endif

struct gl_device
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

	/* the current value of each input register (SetVertexData) */
	float attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	BOOL immediate_active;
	D3DPRIMITIVETYPE immediate_type;
	float *immediate_vertices;
	unsigned long immediate_count;
	unsigned long immediate_capacity;

	GLuint vertex_array;
	GLuint stream_buffer;
#ifdef HALO_ANDROID
	GLuint stream_buffers[STREAM_BUFFER_RING];
	GLuint index_buffers[STREAM_BUFFER_RING];
	unsigned long buffer_ring;
#endif
	unsigned long stream_offset;
	GLuint index_buffer;
	unsigned long index_offset;
	GLuint samplers[D3DTSS_MAXSTAGES];

	GLuint queries[VISIBILITY_TEST_SLOTS];
	BOOL query_pending[VISIBILITY_TEST_SLOTS];
	GLuint active_query;
	BOOL visibility_test_active;
#ifdef HALO_ANDROID
	/* with atomic counters: one counter per test, used as a ring; the
	counter a test ended in, per result slot */
	GLuint visibility_counters;
	unsigned long counter_next;
	unsigned long counter_active;
	unsigned long counter_of_slot[VISIBILITY_TEST_SLOTS];
#endif

	unsigned long frame;
	unsigned long next_vertex_shader_id;
	BOOL gl_ready;
	BOOL created;
};

static struct gl_device device;

/* HALO_GPU_STATS prints these once a second */
static struct
{
	unsigned long draws, immediate_draws, clears, presents;
	unsigned long skipped_no_program, skipped_no_target, skipped_link;
	unsigned long target_changes;
} stats;

static D3DDevice *device_pointer(void)
{
	return (D3DDevice *)&device;
}

static float dword_to_float(DWORD value)
{
	union { DWORD d; float f; } u;

	u.d = value;
	return u.f;
}

static void color_to_vec4(D3DCOLOR color, float *out)
{
	out[0] = ((color >> 16) & 0xff) / 255.0f;
	out[1] = ((color >> 8) & 0xff) / 255.0f;
	out[2] = (color & 0xff) / 255.0f;
	out[3] = ((color >> 24) & 0xff) / 255.0f;
}

/* ---------- vertical blank emulation */

#define VERTICAL_BLANK_NANOSECONDS (1000000000L / 60)

static pthread_mutex_t vertical_blank_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t vertical_blank_condition = PTHREAD_COND_INITIALIZER;
static D3DCALLBACK vertical_blank_callback;
static unsigned long vertical_blank_count;
static volatile unsigned int flip_count;
static unsigned long pending_flips;
static BOOL vertical_blank_thread_started = FALSE;

static void *vertical_blank_thread(void *unused)
{
	struct timespec next;

	(void)unused;
	clock_gettime(CLOCK_MONOTONIC, &next);
	for (;;)
	{
		D3DCALLBACK callback;

		next.tv_nsec += VERTICAL_BLANK_NANOSECONDS;
		if (next.tv_nsec >= 1000000000L)
		{
			next.tv_nsec -= 1000000000L;
			next.tv_sec++;
		}
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

		pthread_mutex_lock(&vertical_blank_lock);
		vertical_blank_count++;
		/* a presented frame becomes visible at the next vertical blank */
		if (pending_flips)
		{
			pending_flips--;
			flip_count++;
		}
		callback = vertical_blank_callback;
		pthread_cond_broadcast(&vertical_blank_condition);
		pthread_mutex_unlock(&vertical_blank_lock);

		if (callback)
			callback(0);
	}
	return NULL;
}

static void vertical_blank_start(void)
{
	pthread_mutex_lock(&vertical_blank_lock);
	if (!vertical_blank_thread_started)
	{
		pthread_t thread;

		if (pthread_create(&thread, NULL, vertical_blank_thread, NULL) == 0)
		{
			pthread_detach(thread);
			vertical_blank_thread_started = TRUE;
		}
		else
		{
			platform_log("cannot start the vertical blank thread");
		}
	}
	pthread_mutex_unlock(&vertical_blank_lock);
}

/* replaces main/d3d_intimacy.cpp, which reads the counter out of the Xbox
Direct3D runtime's private device structure */
volatile unsigned int *d3d_find_flipcount(void)
{
	return &flip_count;
}

void WINAPI D3DDevice_SetVerticalBlankCallback(D3DCALLBACK callback)
{
	pthread_mutex_lock(&vertical_blank_lock);
	vertical_blank_callback = callback;
	pthread_mutex_unlock(&vertical_blank_lock);
	vertical_blank_start();
}

void WINAPI D3DDevice_BlockUntilVerticalBlank(void)
{
	unsigned long count;

	vertical_blank_start();
	pthread_mutex_lock(&vertical_blank_lock);
	count = vertical_blank_count;
	while (vertical_blank_count == count)
		pthread_cond_wait(&vertical_blank_condition, &vertical_blank_lock);
	pthread_mutex_unlock(&vertical_blank_lock);
}

/* ---------- GL helpers */

static GLuint compile_shader(GLenum type, const char *source, const char *what)
{
	GLuint shader = glCreateShader(type);
	GLint status = 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		platform_log("cannot compile the %s shader:\n%s\n%s", what, log, source);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

#ifndef HALO_ANDROID
static void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar *message, const void *user)
{
	(void)source; (void)id; (void)length; (void)user;
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		platform_log("GL %s: %s", type == GL_DEBUG_TYPE_ERROR ? "error" : "debug", message);
}
#endif

/* ---------- render targets */

static void surface_dimensions(const D3DSurface *surface, unsigned long *width, unsigned long *height, BOOL *depth)
{
	struct xgpu_texture_description description;
	DWORD format;

	xgpu_texture_describe(surface->Format, surface->Size, &description);
	*width = description.width;
	*height = description.height;
	format = description.format;
	*depth = format == D3DFMT_D24S8 || format == D3DFMT_F24S8 || format == D3DFMT_D16 || format == D3DFMT_F16 ||
		format == D3DFMT_LIN_D24S8 || format == D3DFMT_LIN_F24S8 || format == D3DFMT_LIN_D16 || format == D3DFMT_LIN_F16;
}

static struct render_target_entry *render_target_get(const D3DSurface *surface)
{
	struct render_target_entry *entry;
	unsigned long width, height;
	BOOL depth;

	if (!surface || !surface->Data)
		return NULL;
	surface_dimensions(surface, &width, &height, &depth);
	for (entry = render_targets; entry; entry = entry->next)
	{
		if (entry->target.data == surface->Data && entry->target.width == width &&
			entry->target.height == height && entry->target.depth == depth)
		{
			return entry;
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->target.data = surface->Data;
	entry->target.width = width;
	entry->target.height = height;
	entry->target.depth = depth;
	glGenTextures(1, &entry->target.texture);
	glBindTexture(GL_TEXTURE_2D, entry->target.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	if (depth)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height, 0,
			GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	entry->next = render_targets;
	render_targets = entry;
	return entry;
}

struct xgpu_render_target *xgpu_render_target_find(unsigned long data)
{
	struct render_target_entry *entry, *best = NULL;

	for (entry = render_targets; entry; entry = entry->next)
	{
		if (entry->target.data == data && !entry->target.depth && (!best || entry->last_rendered > best->last_rendered))
			best = entry;
	}
	return best ? &best->target : NULL;
}

static GLuint framebuffer_get(GLuint color, GLuint depth)
{
	struct framebuffer_entry *entry;
	GLenum draw_buffer = color ? GL_COLOR_ATTACHMENT0 : GL_NONE;

	for (entry = framebuffers; entry; entry = entry->next)
	{
		if (entry->color == color && entry->depth == depth)
			return entry->framebuffer;
	}
	entry = calloc(1, sizeof(*entry));
	entry->color = color;
	entry->depth = depth;
	glGenFramebuffers(1, &entry->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, entry->framebuffer);
	if (color)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
	if (depth)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	glDrawBuffers(1, &draw_buffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		platform_log("framebuffer %u/%u is incomplete", color, depth);
	entry->next = framebuffers;
	framebuffers = entry;
	return entry->framebuffer;
}

/* binds the framebuffer for the current targets; returns FALSE if there is
nothing to draw into */
static BOOL bind_targets(BOOL *has_depth)
{
	struct render_target_entry *color = render_target_get(device.render_target);
	struct render_target_entry *depth = render_target_get(device.depth_stencil);

	if (depth && !depth->target.depth)
		depth = NULL;
	if (!color && !depth)
		return FALSE;
	if (color)
		color->last_rendered = device.frame + 1;
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_get(color ? color->target.texture : 0, depth ? depth->target.texture : 0));
	*has_depth = depth != NULL;
	return TRUE;
}

/* ---------- device creation */

static void gl_initialize(void)
{
	GLint major = 0, minor = 0;
	int index;

	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
#ifdef HALO_ANDROID
	{
		BOOL es32 = major > 3 || (major == 3 && minor >= 2);

		/* clip control is emulated in the vertex shader (nv2a_vsh.c) */
		xgpu_capabilities.copy_image = es32 || host_gl_has_extension("GL_EXT_copy_image") ||
			host_gl_has_extension("GL_OES_copy_image");
		xgpu_capabilities.border_clamp = es32 || host_gl_has_extension("GL_EXT_texture_border_clamp") ||
			host_gl_has_extension("GL_OES_texture_border_clamp");
		xgpu_capabilities.anisotropy = host_gl_has_extension("GL_EXT_texture_filter_anisotropic");
		xgpu_capabilities.shading_language = major > 3 || (major == 3 && minor >= 1) ? "310 es" : "300 es";
		if (major > 3 || (major == 3 && minor >= 1))
		{
			GLint counters = 0;

			glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &counters);
			xgpu_capabilities.atomic_counters = counters > 0;
		}
		xgpu_capabilities.s3tc = host_gl_has_extension("GL_EXT_texture_compression_s3tc") ||
			(host_gl_has_extension("GL_EXT_texture_compression_dxt1") &&
			host_gl_has_extension("GL_ANGLE_texture_compression_dxt3") &&
			host_gl_has_extension("GL_ANGLE_texture_compression_dxt5"));
		platform_log("OpenGL ES %d.%d: copy image %d, border clamp %d, anisotropy %d, S3TC %d, sample counting %d",
			(int)major, (int)minor, xgpu_capabilities.copy_image, xgpu_capabilities.border_clamp,
			xgpu_capabilities.anisotropy, xgpu_capabilities.s3tc, xgpu_capabilities.atomic_counters);
	}
#else
	if (getenv("HALO_GL_DEBUG"))
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(gl_debug_callback, NULL);
	}
	glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
	glEnable(GL_PROGRAM_POINT_SIZE);
#endif
	glGenVertexArrays(1, &device.vertex_array);
	glBindVertexArray(device.vertex_array);
#ifdef HALO_ANDROID
	{
		int ring;

		glGenBuffers(STREAM_BUFFER_RING, device.stream_buffers);
		glGenBuffers(STREAM_BUFFER_RING, device.index_buffers);
		for (ring = 0; ring < STREAM_BUFFER_RING; ring++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, device.stream_buffers[ring]);
			glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.index_buffers[ring]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		}
		device.stream_buffer = device.stream_buffers[0];
		device.index_buffer = device.index_buffers[0];
	}
#endif
#ifndef HALO_ANDROID
	glGenBuffers(1, &device.stream_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, device.stream_buffer);
	glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
	glGenBuffers(1, &device.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
#endif
	glGenSamplers(D3DTSS_MAXSTAGES, device.samplers);
	glGenQueries(VISIBILITY_TEST_SLOTS, device.queries);
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		glGenBuffers(1, &device.visibility_counters);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, device.visibility_counters);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, VISIBILITY_TEST_SLOTS * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}
#endif
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		device.attributes[index][3] = 1.0f;
		glVertexAttrib4fv(index, device.attributes[index]);
	}
	memory_watch_initialize();
	device.gl_ready = TRUE;
}

Direct3D *WINAPI Direct3DCreate8(UINT sdk_version)
{
	(void)sdk_version;
	return (Direct3D *)1;
}

void WINAPI Direct3D_SetPushBufferSize(DWORD push_buffer_size, DWORD segment_count)
{
	(void)push_buffer_size;
	(void)segment_count;
}

static void viewport_update_constants(void)
{
	/* Direct3D's reserved constants c[-38] and c[-37] map clip space to
	the screen; zscale is the depth buffer's range */
	float zscale = 16777215.0f;
	unsigned long width, height;
	BOOL depth;

	if (device.depth_stencil)
	{
		struct xgpu_texture_description description;

		xgpu_texture_describe(device.depth_stencil->Format, device.depth_stencil->Size, &description);
		if (description.format == D3DFMT_D16 || description.format == D3DFMT_LIN_D16 ||
			description.format == D3DFMT_F16 || description.format == D3DFMT_LIN_F16)
		{
			zscale = 65535.0f;
		}
	}
	(void)width; (void)height; (void)depth;
	device.viewport_scale[0] = device.viewport.Width * 0.5f;
	device.viewport_scale[1] = -(float)device.viewport.Height * 0.5f;
	device.viewport_scale[2] = zscale * (device.viewport.MaxZ - device.viewport.MinZ);
	device.viewport_scale[3] = 0.0f;
	device.viewport_offset[0] = device.viewport.X + device.viewport.Width * 0.5f;
	device.viewport_offset[1] = device.viewport.Y + device.viewport.Height * 0.5f;
	device.viewport_offset[2] = zscale * device.viewport.MinZ;
	device.viewport_offset[3] = 0.0f;
	if (!(device.shader_constant_mode & D3DSCM_NORESERVEDCONSTANTS))
	{
		memcpy(device.constants[XGPU_VERTEX_CONSTANT_BIAS - 38], device.viewport_scale, sizeof(device.viewport_scale));
		memcpy(device.constants[XGPU_VERTEX_CONSTANT_BIAS - 37], device.viewport_offset, sizeof(device.viewport_offset));
	}
}

HRESULT WINAPI Direct3D_CreateDevice(UINT adapter, D3DDEVTYPE device_type, void *unused, DWORD behavior_flags,
	D3DPRESENT_PARAMETERS *presentation_parameters, D3DDevice **returned_device)
{
	unsigned long width, height;
	int index;

	(void)adapter;
	(void)device_type;
	(void)unused;
	(void)behavior_flags;
	if (!device.created)
	{
		memset(&device, 0, sizeof(device));
		if (presentation_parameters)
			device.presentation = *presentation_parameters;
		width = device.presentation.BackBufferWidth ? device.presentation.BackBufferWidth : 640;
		height = device.presentation.BackBufferHeight ? device.presentation.BackBufferHeight : 480;
		d3d8_surface_initialize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, width, height);
		d3d8_surface_initialize(&device.depth_buffer, D3DFMT_LIN_D24S8, width, height);
		device.render_target = &device.back_buffer;
		device.depth_stencil = &device.depth_buffer;
		for (index = 0; index < D3DTS_MAX; index++)
		{
			device.transforms[index]._11 = 1.0f;
			device.transforms[index]._22 = 1.0f;
			device.transforms[index]._33 = 1.0f;
			device.transforms[index]._44 = 1.0f;
		}
		device.viewport.Width = width;
		device.viewport.Height = height;
		device.viewport.MaxZ = 1.0f;
		device.next_vertex_shader_id = 1;
		D3D__RenderState[D3DRS_ZENABLE] = TRUE;
		D3D__RenderState[D3DRS_ZWRITEENABLE] = TRUE;
		D3D__RenderState[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
		D3D__RenderState[D3DRS_COLORWRITEENABLE] = D3DCOLORWRITEENABLE_ALL;
		D3D__RenderState[D3DRS_SRCBLEND] = D3DBLEND_ONE;
		D3D__RenderState[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
		D3D__RenderState[D3DRS_BLENDOP] = D3DBLENDOP_ADD;
		D3D__RenderState[D3DRS_CULLMODE] = D3DCULL_CCW;
		D3D__RenderState[D3DRS_FRONTFACE] = D3DFRONT_CW;
		D3D__RenderState[D3DRS_FILLMODE] = D3DFILL_SOLID;
		D3D__RenderState[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
		D3D__RenderState[D3DRS_STENCILFUNC] = D3DCMP_ALWAYS;
		D3D__RenderState[D3DRS_STENCILMASK] = 0xff;
		D3D__RenderState[D3DRS_STENCILWRITEMASK] = 0xff;
		D3D__RenderState[D3DRS_STENCILFAIL] = D3DSTENCILOP_KEEP;
		D3D__RenderState[D3DRS_STENCILZFAIL] = D3DSTENCILOP_KEEP;
		D3D__RenderState[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
		for (index = 0; index < D3DTSS_MAXSTAGES; index++)
		{
			D3D__TextureState[index][D3DTSS_ADDRESSU] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_ADDRESSW] = D3DTADDRESS_WRAP;
			D3D__TextureState[index][D3DTSS_MAGFILTER] = D3DTEXF_POINT;
			D3D__TextureState[index][D3DTSS_MINFILTER] = D3DTEXF_POINT;
			D3D__TextureState[index][D3DTSS_MAXANISOTROPY] = 1;
		}
		viewport_update_constants();

		if (!getenv("HALO_NULL_RENDERER") && platform_video_initialize(width, height))
			gl_initialize();
		else
			platform_log("Direct3D: running without a window (nothing is displayed)");
		device.created = TRUE;
	}
	*returned_device = device_pointer();
	return S_OK;
}

ULONG WINAPI D3DDevice_Release(void)
{
	return 1;
}

void WINAPI D3DDevice_GetDeviceCaps(D3DCAPS8 *caps)
{
	memset(caps, 0, sizeof(*caps));
	caps->DeviceType = D3DDEVTYPE_HAL;
	caps->MaxTextureWidth = 4096;
	caps->MaxTextureHeight = 4096;
	caps->MaxVolumeExtent = 512;
	caps->MaxTextureRepeat = 8192;
	caps->MaxTextureAspectRatio = 4096;
	caps->MaxAnisotropy = 4;
	caps->MaxTextureBlendStages = 4;
	caps->MaxSimultaneousTextures = 4;
	caps->MaxActiveLights = 8;
	caps->MaxVertexBlendMatrices = 4;
	caps->MaxPointSize = 64.0f;
	caps->MaxPrimitiveCount = 0xfffff;
	caps->MaxVertexIndex = 0xffff;
	caps->MaxStreams = 16;
	caps->MaxStreamStride = 255;
	caps->VertexShaderVersion = D3DVS_VERSION(1, 1);
	caps->MaxVertexShaderConst = 192;
	caps->PixelShaderVersion = D3DPS_VERSION(1, 1);
	caps->MaxPixelShaderValue = 1.0f;
}

void WINAPI D3DDevice_GetBackBuffer(INT back_buffer, D3DBACKBUFFER_TYPE type, D3DSurface **result)
{
	(void)back_buffer;
	(void)type;
	/* like Direct3D, the caller gets a reference it must release */
	device.back_buffer.Common++;
	*result = &device.back_buffer;
}

HRESULT WINAPI D3DDevice_GetDepthStencilSurface(D3DSurface **result)
{
	*result = device.depth_stencil;
	if (!*result)
		return D3DERR_NOTFOUND;
	(*result)->Common++;
	return S_OK;
}

static BOOL trace_frame(void);

void WINAPI D3DDevice_SetRenderTarget(D3DSurface *render_target, D3DSurface *depth_stencil)
{
	if (trace_frame())
		platform_log("set render target %08lx depth %08lx", render_target ? (unsigned long)render_target->Data : 0,
			depth_stencil ? (unsigned long)depth_stencil->Data : 0);
	stats.target_changes++;
	if (render_target)
		device.render_target = render_target;
	device.depth_stencil = depth_stencil;
	/* like Direct3D, reset the viewport to the whole new target */
	if (device.render_target)
	{
		unsigned long width, height;
		BOOL depth;

		surface_dimensions(device.render_target, &width, &height, &depth);
		device.viewport.X = 0;
		device.viewport.Y = 0;
		device.viewport.Width = width;
		device.viewport.Height = height;
		device.viewport.MinZ = 0.0f;
		device.viewport.MaxZ = 1.0f;
	}
	viewport_update_constants();
}

void WINAPI D3DDevice_SetViewport(CONST D3DVIEWPORT8 *viewport)
{
	device.viewport = *viewport;
	viewport_update_constants();
}

void WINAPI D3DDevice_SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX *matrix)
{
	if ((unsigned long)state < D3DTS_MAX)
		device.transforms[state] = *matrix;
}

void WINAPI D3DDevice_GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
	if ((unsigned long)state < D3DTS_MAX)
		*matrix = device.transforms[state];
}

void WINAPI D3DDevice_SetFlickerFilter(DWORD filter) { (void)filter; }
void WINAPI D3DDevice_SetSoftDisplayFilter(BOOL enable) { (void)enable; }

void WINAPI D3DDevice_SetShaderConstantMode(D3DSHADERCONSTANTMODE mode)
{
	device.shader_constant_mode = mode;
	viewport_update_constants();
}

/* ---------- GPU synchronisation: GL keeps its own ordering */

BOOL WINAPI D3DDevice_IsBusy(void)
{
	return FALSE;
}

void WINAPI D3DDevice_KickPushBuffer(void)
{
	if (device.gl_ready)
		glFlush();
}

void WINAPI D3DDevice_InsertCallback(D3DCALLBACKTYPE type, D3DCALLBACK callback, DWORD context)
{
	(void)type;
	/* the "GPU" reaches the callback immediately */
	if (callback)
		callback(context);
}

/* ---------- visibility (occlusion) tests */

void WINAPI D3DDevice_BeginVisibilityTest(void)
{
	if (!device.gl_ready || device.visibility_test_active)
		return;
	/* the query object is chosen when the test ends; use a scratch one */
	device.visibility_test_active = TRUE;
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		const GLuint zero = 0;

		device.counter_next = (device.counter_next + 1) % VISIBILITY_TEST_SLOTS;
		device.counter_active = device.counter_next;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, device.visibility_counters);
		host_gl_buffer_write(GL_ATOMIC_COUNTER_BUFFER, (unsigned int)(device.counter_active * sizeof(GLuint)),
			sizeof(zero), &zero);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		return;
	}
#endif
	glBeginQuery(VISIBILITY_QUERY, device.queries[0]);
}

HRESULT WINAPI D3DDevice_EndVisibilityTest(DWORD index)
{
	GLuint scratch;

	if (!device.gl_ready || !device.visibility_test_active)
		return S_OK;
	device.visibility_test_active = FALSE;
	index %= VISIBILITY_TEST_SLOTS;
	if (!index)
		index = 1;
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		device.counter_of_slot[index] = device.counter_active;
		device.query_pending[index] = TRUE;
		return S_OK;
	}
#endif
	glEndQuery(VISIBILITY_QUERY);
	/* swap the scratch query into the requested slot */
	scratch = device.queries[0];
	device.queries[0] = device.queries[index];
	device.queries[index] = scratch;
	device.query_pending[index] = TRUE;
	return S_OK;
}

HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD index, UINT *result, ULONGLONG *time_stamp)
{
	GLuint available = 0, samples = 0;

	if (time_stamp)
		*time_stamp = 0;
	index %= VISIBILITY_TEST_SLOTS;
	if (!index)
		index = 1;
	if (!device.gl_ready || !device.query_pending[index])
	{
		if (result)
			*result = 0;
		return S_OK;
	}
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		/* reading the buffer waits for the draws that counted */
		samples = host_gl_read_buffer_word(device.visibility_counters,
			(unsigned int)(device.counter_of_slot[index] * sizeof(GLuint)));
		if (result)
			*result = samples;
		return S_OK;
	}
#endif
	glGetQueryObjectuiv(device.queries[index], GL_QUERY_RESULT_AVAILABLE, &available);
	if (!available)
		return D3DERR_TESTINCOMPLETE;
	glGetQueryObjectuiv(device.queries[index], GL_QUERY_RESULT, &samples);
#ifdef HALO_ANDROID
	/* ES only says whether any sample passed. The game divides the count by
	the test's area (lens flare brightness, rasterizer_lights.c): report
	more than any test covers, well below what would overflow there. */
	if (samples)
		samples = VISIBILITY_ALL_SAMPLES;
#endif
	if (result)
		*result = samples;
	return S_OK;
}

/* ---------- render and texture stage state */

void D3DFASTCALL D3DDevice_SetRenderState_Simple(DWORD method, DWORD value)
{
	/* callers also store the value in D3D__RenderState themselves */
	(void)method;
	(void)value;
}

void D3DFASTCALL D3DDevice_SetRenderState_Deferred(D3DRENDERSTATETYPE state, DWORD value)
{
	if ((unsigned long)state < D3DRS_MAX)
		D3D__RenderState[state] = value;
}

void WINAPI D3DDevice_SetRenderState_ZBias(DWORD value);

void WINAPI D3DDevice_SetRenderStateNotInline(D3DRENDERSTATETYPE state, DWORD value)
{
	if (state == D3DRS_ZBIAS)
		D3DDevice_SetRenderState_ZBias(value);
	else if ((unsigned long)state < D3DRS_MAX)
		D3D__RenderState[state] = value;
}

/* As the Xbox's D3D8 does it: a z bias is a polygon offset of -bias depth
units plus -bias/4 times the polygon's depth slope, enabled for every fill
mode. Without the slope term, decals (biased by 8) fight with the surface
under them wherever it is seen at an angle. */
void WINAPI D3DDevice_SetRenderState_ZBias(DWORD value)
{
	float offset = -(float)value;
	float slope = offset * 0.25f;
	DWORD enable = value != 0;

	memcpy(&D3D__RenderState[D3DRS_POLYGONOFFSETZSLOPESCALE], &slope, sizeof(slope));
	memcpy(&D3D__RenderState[D3DRS_POLYGONOFFSETZOFFSET], &offset, sizeof(offset));
	D3D__RenderState[D3DRS_POINTOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_WIREFRAMEOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_SOLIDOFFSETENABLE] = enable;
	D3D__RenderState[D3DRS_ZBIAS] = value;
}

#define COMPLEX_RENDER_STATE(name, state) \
	void WINAPI D3DDevice_SetRenderState_##name(DWORD value) { D3D__RenderState[state] = value; }

COMPLEX_RENDER_STATE(PSTextureModes, D3DRS_PSTEXTUREMODES)
COMPLEX_RENDER_STATE(VertexBlend, D3DRS_VERTEXBLEND)
COMPLEX_RENDER_STATE(FogColor, D3DRS_FOGCOLOR)
COMPLEX_RENDER_STATE(FillMode, D3DRS_FILLMODE)
COMPLEX_RENDER_STATE(BackFillMode, D3DRS_BACKFILLMODE)
COMPLEX_RENDER_STATE(TwoSidedLighting, D3DRS_TWOSIDEDLIGHTING)
COMPLEX_RENDER_STATE(NormalizeNormals, D3DRS_NORMALIZENORMALS)
COMPLEX_RENDER_STATE(ZEnable, D3DRS_ZENABLE)
COMPLEX_RENDER_STATE(StencilEnable, D3DRS_STENCILENABLE)
COMPLEX_RENDER_STATE(StencilFail, D3DRS_STENCILFAIL)
COMPLEX_RENDER_STATE(FrontFace, D3DRS_FRONTFACE)
COMPLEX_RENDER_STATE(CullMode, D3DRS_CULLMODE)
COMPLEX_RENDER_STATE(TextureFactor, D3DRS_TEXTUREFACTOR)
COMPLEX_RENDER_STATE(LogicOp, D3DRS_LOGICOP)
COMPLEX_RENDER_STATE(EdgeAntiAlias, D3DRS_EDGEANTIALIAS)
COMPLEX_RENDER_STATE(MultiSampleAntiAlias, D3DRS_MULTISAMPLEANTIALIAS)
COMPLEX_RENDER_STATE(MultiSampleMask, D3DRS_MULTISAMPLEMASK)
COMPLEX_RENDER_STATE(MultiSampleType, D3DRS_MULTISAMPLETYPE)
COMPLEX_RENDER_STATE(ShadowFunc, D3DRS_SHADOWFUNC)
COMPLEX_RENDER_STATE(LineWidth, D3DRS_LINEWIDTH)
COMPLEX_RENDER_STATE(Dxt1NoiseEnable, D3DRS_DXT1NOISEENABLE)
COMPLEX_RENDER_STATE(YuvEnable, D3DRS_YUVENABLE)
COMPLEX_RENDER_STATE(OcclusionCullEnable, D3DRS_OCCLUSIONCULLENABLE)
COMPLEX_RENDER_STATE(StencilCullEnable, D3DRS_STENCILCULLENABLE)
COMPLEX_RENDER_STATE(RopZCmpAlwaysRead, D3DRS_ROPZCMPALWAYSREAD)
COMPLEX_RENDER_STATE(RopZRead, D3DRS_ROPZREAD)
COMPLEX_RENDER_STATE(DoNotCullUncompressed, D3DRS_DONOTCULLUNCOMPRESSED)

void D3DFASTCALL D3DDevice_SetTextureState_Deferred(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES && (unsigned long)type < D3DTSS_MAX)
		D3D__TextureState[stage][type] = value;
}

void WINAPI D3DDevice_SetTextureState_TexCoordIndex(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_TEXCOORDINDEX] = value;
}

void WINAPI D3DDevice_SetTextureState_BorderColor(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_BORDERCOLOR] = value;
}

void WINAPI D3DDevice_SetTextureState_ColorKeyColor(DWORD stage, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES)
		D3D__TextureState[stage][D3DTSS_COLORKEYCOLOR] = value;
}

void WINAPI D3DDevice_SetTextureState_BumpEnv(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	if (stage < D3DTSS_MAXSTAGES && (unsigned long)type < D3DTSS_MAX)
		D3D__TextureState[stage][type] = value;
}

void WINAPI D3DDevice_SetTexture(DWORD stage, D3DBaseTexture *texture)
{
	if (stage < D3DTSS_MAXSTAGES)
		device.textures[stage] = texture;
}

void WINAPI D3DDevice_SetPalette(DWORD stage, D3DPalette *palette)
{
	if (stage < D3DTSS_MAXSTAGES)
		device.palettes[stage] = palette;
}

void WINAPI D3DDevice_SetPixelShaderProgram(D3DPIXELSHADERDEF *definition)
{
	/* the definition's members are the pixel shader render states */
	if (!definition)
		return;
	memcpy(&D3D__RenderState[D3DRS_PSALPHAINPUTS0], definition->PSAlphaInputs, sizeof(definition->PSAlphaInputs));
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSABCD] = definition->PSFinalCombinerInputsABCD;
	D3D__RenderState[D3DRS_PSFINALCOMBINERINPUTSEFG] = definition->PSFinalCombinerInputsEFG;
	memcpy(&D3D__RenderState[D3DRS_PSCONSTANT0_0], definition->PSConstant0, sizeof(definition->PSConstant0));
	memcpy(&D3D__RenderState[D3DRS_PSCONSTANT1_0], definition->PSConstant1, sizeof(definition->PSConstant1));
	memcpy(&D3D__RenderState[D3DRS_PSALPHAOUTPUTS0], definition->PSAlphaOutputs, sizeof(definition->PSAlphaOutputs));
	memcpy(&D3D__RenderState[D3DRS_PSRGBINPUTS0], definition->PSRGBInputs, sizeof(definition->PSRGBInputs));
	D3D__RenderState[D3DRS_PSCOMPAREMODE] = definition->PSCompareMode;
	D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0] = definition->PSFinalCombinerConstant0;
	D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1] = definition->PSFinalCombinerConstant1;
	memcpy(&D3D__RenderState[D3DRS_PSRGBOUTPUTS0], definition->PSRGBOutputs, sizeof(definition->PSRGBOutputs));
	D3D__RenderState[D3DRS_PSCOMBINERCOUNT] = definition->PSCombinerCount;
	D3D__RenderState[D3DRS_PSTEXTUREMODES] = definition->PSTextureModes;
	D3D__RenderState[D3DRS_PSDOTMAPPING] = definition->PSDotMapping;
	D3D__RenderState[D3DRS_PSINPUTTEXTURE] = definition->PSInputTexture;
}

/* ---------- vertex shaders */

static unsigned long vertex_type_bytes(unsigned long type)
{
	switch (type)
	{
	case D3DVSDT_FLOAT1: return 4;
	case D3DVSDT_FLOAT2: return 8;
	case D3DVSDT_FLOAT3: return 12;
	case D3DVSDT_FLOAT4: return 16;
	case D3DVSDT_D3DCOLOR: return 4;
	case D3DVSDT_SHORT1: return 2;
	case D3DVSDT_SHORT2: return 4;
	case D3DVSDT_SHORT3: return 6;
	case D3DVSDT_SHORT4: return 8;
	case D3DVSDT_NORMSHORT1: return 2;
	case D3DVSDT_NORMSHORT2: return 4;
	case D3DVSDT_NORMSHORT3: return 6;
	case D3DVSDT_NORMSHORT4: return 8;
	case D3DVSDT_NORMPACKED3: return 4;
	case D3DVSDT_PBYTE1: return 1;
	case D3DVSDT_PBYTE2: return 2;
	case D3DVSDT_PBYTE3: return 3;
	case D3DVSDT_PBYTE4: return 4;
	case D3DVSDT_FLOAT2H: return 12;
	default: return 0;
	}
}

static void parse_declaration(struct vertex_shader_object *object, const DWORD *declaration)
{
	unsigned long stream = 0;
	unsigned long offsets[16] = { 0 };

	for (; declaration && *declaration != D3DVSD_END(); declaration++)
	{
		DWORD token = *declaration;
		unsigned long token_type = (token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT;

		switch (token_type)
		{
		case D3DVSD_TOKEN_STREAM:
			stream = token & D3DVSD_STREAMNUMBERMASK;
			break;
		case D3DVSD_TOKEN_STREAMDATA:
			if (token & D3DVSD_DATALOADTYPEMASK)
			{
				/* skip: the count is in dwords, or in bytes with bit 27 */
				unsigned long count = (token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT;

				offsets[stream] += (token & 0x08000000) ? count : count * 4;
			}
			else if (object->element_count < XGPU_VERTEX_ATTRIBUTE_COUNT)
			{
				struct vertex_element *element = &object->elements[object->element_count++];

				element->reg = (unsigned char)(token & D3DVSD_VERTEXREGMASK);
				element->stream = (unsigned char)stream;
				element->type = (unsigned char)((token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT);
				element->bytes = (unsigned char)vertex_type_bytes(element->type);
				element->offset = (unsigned short)offsets[stream];
				offsets[stream] += element->bytes;
				if (element->type == D3DVSDT_NORMPACKED3)
					object->packed_mask |= 1UL << element->reg;
			}
			break;
		case D3DVSD_TOKEN_CONSTMEM:
			declaration += ((token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT) * 4;
			break;
		case D3DVSD_TOKEN_EXT:
			declaration += (token & D3DVSD_EXTCOUNTMASK) >> D3DVSD_EXTCOUNTSHIFT;
			break;
		default:
			break;
		}
	}
}

HRESULT WINAPI D3DDevice_CreateVertexShader(CONST DWORD *declaration, CONST DWORD *function, DWORD *handle, DWORD usage)
{
	struct vertex_shader_object *object = calloc(1, sizeof(*object));

	(void)usage;
	if (!object)
		return E_OUTOFMEMORY;
	object->signature = VERTEX_SHADER_SIGNATURE;
	object->id = device.next_vertex_shader_id++;
	if (function)
	{
		/* header: program type in the low word, instruction count in the high */
		object->instruction_count = function[0] >> 16;
		object->instructions = malloc(object->instruction_count * 4 * sizeof(DWORD));
		memcpy(object->instructions, function + 1, object->instruction_count * 4 * sizeof(DWORD));
	}
	parse_declaration(object, declaration);
	/* odd values are FVF codes; programmable shader handles are even */
	*handle = (DWORD)object;
	return S_OK;
}

static struct vertex_shader_object *vertex_shader_from_handle(DWORD handle)
{
	struct vertex_shader_object *object = (struct vertex_shader_object *)handle;

	if (!handle || (handle & 1) || object->signature != VERTEX_SHADER_SIGNATURE)
		return NULL;
	return object;
}

void WINAPI D3DDevice_DeleteVertexShader(DWORD handle)
{
	/* programs stay cached; the object is small */
	(void)handle;
}

void WINAPI D3DDevice_SetVertexShader(DWORD handle)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	if (object)
	{
		device.vertex_shader = object;
		device.program_address = 0;
		device.program_slots[0] = object;
	}
}

void WINAPI D3DDevice_LoadVertexShader(DWORD handle, DWORD address)
{
	if (address < VERTEX_PROGRAM_SLOTS)
		device.program_slots[address] = vertex_shader_from_handle(handle);
}

void WINAPI D3DDevice_SelectVertexShader(DWORD handle, DWORD address)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	if (object)
		device.vertex_shader = object;
	if (address < VERTEX_PROGRAM_SLOTS)
		device.program_address = address;
}

void WINAPI D3DDevice_GetVertexShaderSize(DWORD handle, UINT *size)
{
	struct vertex_shader_object *object = vertex_shader_from_handle(handle);

	*size = object ? object->instruction_count : 0;
}

void WINAPI D3DDevice_SetVertexShaderConstant(INT reg, CONST void *constant_data, DWORD constant_count)
{
	long first = reg + XGPU_VERTEX_CONSTANT_BIAS;

	if (first < 0 || first >= XGPU_VERTEX_CONSTANT_COUNT)
		return;
	if (first + (long)constant_count > XGPU_VERTEX_CONSTANT_COUNT)
		constant_count = XGPU_VERTEX_CONSTANT_COUNT - first;
	memcpy(device.constants[first], constant_data, constant_count * 4 * sizeof(float));
}

/* the program that runs: the one loaded at the selected address, else the
current shader's own */
static struct vertex_shader_object *current_program(void)
{
	struct vertex_shader_object *program = device.program_slots[device.program_address];

	return program ? program : device.vertex_shader;
}

/* ---------- program cache */

static unsigned long hash_bytes(const void *data, unsigned long size)
{
	const unsigned char *bytes = data;
	unsigned long hash = 2166136261UL;

	while (size--)
		hash = (hash ^ *bytes++) * 16777619UL;
	return hash;
}

static GLuint vertex_shader_get(struct vertex_shader_object *program, BOOL immediate)
{
	int variant = immediate ? 1 : 0;

	if (!program->shader[variant])
	{
		char *source = nv2a_vertex_shader_to_glsl(program->instructions, program->instruction_count,
			immediate ? 0 : device.vertex_shader->packed_mask);

		program->shader[variant] = compile_shader(GL_VERTEX_SHADER, source, "vertex");
		if (getenv("HALO_GPU_DUMP_SHADERS"))
		{
			char path[512];
			FILE *file;

			snprintf(path, sizeof(path), "%s/vs%03lu_%d.glsl", getenv("HALO_GPU_DUMP_SHADERS"), program->id, variant);
			if ((file = fopen(path, "w")) != NULL)
			{
				fputs(source, file);
				fclose(file);
			}
		}
		free(source);
	}
	return program->shader[variant];
}

static GLuint fragment_shader_get(const struct nv2a_pixel_shader_key *key)
{
	unsigned long hash = hash_bytes(key, sizeof(*key));
	struct fragment_entry **bucket = &fragment_buckets[hash % FRAGMENT_BUCKETS];
	struct fragment_entry *entry;
	char *source;

	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash && !memcmp(&entry->key, key, sizeof(*key)))
			return entry->shader;
	}
	entry = calloc(1, sizeof(*entry));
	entry->hash = hash;
	entry->key = *key;
	source = nv2a_pixel_shader_to_glsl(key);
	entry->shader = compile_shader(GL_FRAGMENT_SHADER, source, "pixel");
	if (getenv("HALO_GPU_DUMP_SHADERS"))
	{
		char path[512];
		FILE *file;

		snprintf(path, sizeof(path), "%s/ps_%08lx.glsl", getenv("HALO_GPU_DUMP_SHADERS"), hash);
		if ((file = fopen(path, "w")) != NULL)
		{
			fputs(source, file);
			fclose(file);
		}
	}
	free(source);
	entry->next = *bucket;
	*bucket = entry;
	return entry->shader;
}

static struct program_entry *program_get(GLuint vertex_shader, GLuint fragment_shader)
{
	unsigned long hash = (vertex_shader * 2654435761UL) ^ fragment_shader;
	struct program_entry **bucket = &program_buckets[hash % PROGRAM_BUCKETS];
	struct program_entry *entry;
	GLint status = 0;
	int stage;

	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->vertex_shader == vertex_shader && entry->fragment_shader == fragment_shader)
			return entry->program ? entry : NULL;
	}
	entry = calloc(1, sizeof(*entry));
	entry->vertex_shader = vertex_shader;
	entry->fragment_shader = fragment_shader;
	entry->next = *bucket;
	*bucket = entry;
	if (!vertex_shader || !fragment_shader)
		return NULL;

	entry->program = glCreateProgram();
	glAttachShader(entry->program, vertex_shader);
	glAttachShader(entry->program, fragment_shader);
	glLinkProgram(entry->program);
	glGetProgramiv(entry->program, GL_LINK_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetProgramInfoLog(entry->program, sizeof(log), NULL, log);
		platform_log("cannot link a shader program: %s", log);
		entry->program = 0;
		return NULL;
	}
	glUseProgram(entry->program);
	entry->constants = glGetUniformLocation(entry->program, "c");
	entry->viewport_scale = glGetUniformLocation(entry->program, "viewport_scale");
	entry->viewport_offset = glGetUniformLocation(entry->program, "viewport_offset");
	entry->point_size = glGetUniformLocation(entry->program, "point_size");
	entry->ps_c0 = glGetUniformLocation(entry->program, "ps_c0");
	entry->ps_c1 = glGetUniformLocation(entry->program, "ps_c1");
	entry->ps_final_c0 = glGetUniformLocation(entry->program, "ps_final_c0");
	entry->ps_final_c1 = glGetUniformLocation(entry->program, "ps_final_c1");
	entry->fog_color = glGetUniformLocation(entry->program, "fog_color");
	entry->fog_parameters = glGetUniformLocation(entry->program, "fog_parameters");
	entry->alpha_reference = glGetUniformLocation(entry->program, "alpha_reference");
	entry->bump_matrix = glGetUniformLocation(entry->program, "bump_matrix");
	entry->bump_luminance = glGetUniformLocation(entry->program, "bump_luminance");
	entry->texture_scale = glGetUniformLocation(entry->program, "texture_scale");
	entry->texture_lod_bias = glGetUniformLocation(entry->program, "texture_lod_bias");
	entry->screen_offset = glGetUniformLocation(entry->program, "screen_offset");
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		char name[8];

		snprintf(name, sizeof(name), "tex%d", stage);
		glUniform1i(glGetUniformLocation(entry->program, name), stage);
	}
	return entry;
}

/* ---------- per-draw state */

static unsigned long stage_texture_mode(int stage)
{
	return (D3D__RenderState[D3DRS_PSTEXTUREMODES] >> (5 * stage)) & 0x1f;
}

static GLenum address_mode(DWORD mode)
{
	switch (mode)
	{
	case D3DTADDRESS_MIRROR: return GL_MIRRORED_REPEAT;
	case D3DTADDRESS_CLAMP: return GL_CLAMP_TO_EDGE;
#ifdef HALO_ANDROID
	case D3DTADDRESS_BORDER: return xgpu_capabilities.border_clamp ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
#else
	case D3DTADDRESS_BORDER: return GL_CLAMP_TO_BORDER;
#endif
	case D3DTADDRESS_CLAMPTOEDGE: return GL_CLAMP_TO_EDGE;
	default: return GL_REPEAT;
	}
}

static void configure_sampler(int stage, BOOL mipmapped)
{
	GLuint sampler = device.samplers[stage];
	DWORD *state = D3D__TextureState[stage];
	DWORD min_filter = state[D3DTSS_MINFILTER];
	DWORD mip_filter = mipmapped ? state[D3DTSS_MIPFILTER] : D3DTEXF_NONE;
	GLenum minification;
	float border[4];

	if (min_filter == D3DTEXF_POINT)
		minification = mip_filter == D3DTEXF_NONE ? GL_NEAREST :
			mip_filter == D3DTEXF_POINT ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
	else
		minification = mip_filter == D3DTEXF_NONE ? GL_LINEAR :
			mip_filter == D3DTEXF_POINT ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, (GLint)minification);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, state[D3DTSS_MAGFILTER] == D3DTEXF_POINT ? GL_NEAREST : GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, (GLint)address_mode(state[D3DTSS_ADDRESSU]));
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, (GLint)address_mode(state[D3DTSS_ADDRESSV]));
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, (GLint)address_mode(state[D3DTSS_ADDRESSW]));
#ifdef HALO_ANDROID
	/* ES has no sampler LOD bias; the pixel shader applies it
	(texture_lod_bias) */
	glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, (float)state[D3DTSS_MAXMIPLEVEL]);
	if (xgpu_capabilities.anisotropy)
		glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			(min_filter == D3DTEXF_ANISOTROPIC && state[D3DTSS_MAXANISOTROPY] > 1) ? (float)state[D3DTSS_MAXANISOTROPY] : 1.0f);
	if (xgpu_capabilities.border_clamp)
	{
		color_to_vec4(state[D3DTSS_BORDERCOLOR], border);
		glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border);
	}
#else
	glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, dword_to_float(state[D3DTSS_MIPMAPLODBIAS]));
	glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, (float)state[D3DTSS_MAXMIPLEVEL]);
	glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY,
		(min_filter == D3DTEXF_ANISOTROPIC && state[D3DTSS_MAXANISOTROPY] > 1) ? (float)state[D3DTSS_MAXANISOTROPY] : 1.0f);
	color_to_vec4(state[D3DTSS_BORDERCOLOR], border);
	glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border);
#endif
}


/* ---------- render targets sampled with their mip chain

The game renders some textures one mip level at a time, each level being a
surface of its own (the water's ripple map). Sampling such a texture needs
every level in one GL texture, so the levels' render targets are copied into
a mipmapped composite whenever it is bound. */

struct mip_composite
{
	struct mip_composite *next;
	unsigned long data, width, height, levels;
	GLuint texture;
};

static struct mip_composite *mip_composites;

#ifdef HALO_ANDROID
static GLuint framebuffer_get(GLuint color, GLuint depth);

/* glCopyImageSubData for ES 3.0/3.1 contexts without the extension */
static void copy_level_by_blit(GLuint source, GLuint destination, GLint level, GLsizei width, GLsizei height)
{
	static GLuint draw_framebuffer;

	if (!draw_framebuffer)
		glGenFramebuffers(1, &draw_framebuffer);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_get(source, 0));
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destination, level);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif

static GLuint mip_composite_get(const struct xgpu_texture_description *description, unsigned long data)
{
	struct mip_composite *composite;
	unsigned long level, rendered_levels = 0;

	for (composite = mip_composites; composite; composite = composite->next)
	{
		if (composite->data == data && composite->width == description->width &&
			composite->height == description->height && composite->levels == description->levels)
		{
			break;
		}
	}
	if (!composite)
	{
		composite = calloc(1, sizeof(*composite));
		composite->data = data;
		composite->width = description->width;
		composite->height = description->height;
		composite->levels = description->levels;
		glGenTextures(1, &composite->texture);
		glBindTexture(GL_TEXTURE_2D, composite->texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)description->levels - 1);
		for (level = 0; level < description->levels; level++)
		{
			GLsizei width = (GLsizei)(description->width >> level ? description->width >> level : 1);
			GLsizei height = (GLsizei)(description->height >> level ? description->height >> level : 1);

			glTexImage2D(GL_TEXTURE_2D, (GLint)level, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		}
		composite->next = mip_composites;
		mip_composites = composite;
	}
	for (level = 0; level < description->levels; level++)
	{
		unsigned long width = description->width >> level ? description->width >> level : 1;
		unsigned long height = description->height >> level ? description->height >> level : 1;
		struct xgpu_render_target *target =
			xgpu_render_target_find(data + xgpu_texture_level_offset(description, level));

		if (!target || target->width != width || target->height != height)
			break;
#ifdef HALO_ANDROID
		if (!xgpu_capabilities.copy_image)
		{
			copy_level_by_blit(target->texture, composite->texture, (GLint)level, (GLsizei)width, (GLsizei)height);
		}
		else
#endif
		glCopyImageSubData(target->texture, GL_TEXTURE_2D, 0, 0, 0, 0,
			composite->texture, GL_TEXTURE_2D, (GLint)level, 0, 0, 0, (GLsizei)width, (GLsizei)height, 1);
		rendered_levels++;
	}
	glBindTexture(GL_TEXTURE_2D, composite->texture);
	/* levels the game did not render come from the ones it did */
	if (rendered_levels < description->levels)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, rendered_levels ? (GLint)rendered_levels - 1 : 0);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	}
	return composite->texture;
}

static void bind_textures(struct nv2a_pixel_shader_key *key, float texture_scale[4][4])
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = device.textures[stage];
		unsigned long mode = stage_texture_mode(stage);

		texture_scale[stage][0] = texture_scale[stage][1] = 1.0f;
		texture_scale[stage][2] = texture_scale[stage][3] = 1.0f;
		glActiveTexture(GL_TEXTURE0 + stage);
		if (!texture || !texture->Data || mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x11)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			key->sampler_type[stage] = mode == 0x11 ? _xgpu_sampler_2d : _xgpu_sampler_none;
			continue;
		}
		{
			struct xgpu_render_target *target = xgpu_render_target_find(texture->Data);
			struct xgpu_texture_description description;
			GLenum gl_target;
			GLuint gl_texture;

			if (target)
			{
				xgpu_texture_describe(texture->Format, texture->Size, &description);
				gl_texture = target->texture;
				gl_target = GL_TEXTURE_2D;
				if (description.linear)
				{
					texture_scale[stage][0] = 1.0f / (float)target->width;
					texture_scale[stage][1] = 1.0f / (float)target->height;
				}
				if (!description.linear && !description.cube_map && description.levels > 1 &&
					target->width == description.width && target->height == description.height)
					gl_texture = mip_composite_get(&description, texture->Data);
				else
					description.levels = 1;
			}
			else
			{
				const D3DCOLOR *palette = device.palettes[stage] && device.palettes[stage]->Data ?
					(const D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(device.palettes[stage]->Data) : NULL;

				gl_texture = xgpu_texture_get((const DWORD *)texture, palette, &gl_target, &description);
				if (description.linear)
				{
					texture_scale[stage][0] = 1.0f / (float)description.width;
					texture_scale[stage][1] = 1.0f / (float)description.height;
				}
			}
			glBindTexture(gl_target, gl_texture);
			glBindSampler(stage, device.samplers[stage]);
			configure_sampler(stage, description.levels > 1);
			key->sampler_type[stage] = gl_target == GL_TEXTURE_CUBE_MAP ? _xgpu_sampler_cube :
				gl_target == GL_TEXTURE_3D ? _xgpu_sampler_3d : _xgpu_sampler_2d;
		}
	}
}

static GLenum stencil_operation(DWORD operation)
{
	/* Xbox stencil operations are the GL enumerants, plus 0 for ZERO */
	return operation ? (GLenum)operation : GL_ZERO;
}

static GLenum blend_equation(DWORD operation)
{
	switch (operation)
	{
	case D3DBLENDOP_SUBTRACT: return GL_FUNC_SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT:
	case D3DBLENDOP_REVSUBTRACTSIGNED: return GL_FUNC_REVERSE_SUBTRACT;
	case D3DBLENDOP_MIN: return GL_MIN;
	case D3DBLENDOP_MAX: return GL_MAX;
	default: return GL_FUNC_ADD;
	}
}

static void apply_raster_state(BOOL has_depth)
{
	DWORD *rs = D3D__RenderState;
	DWORD write = rs[D3DRS_COLORWRITEENABLE];
	float blend_color[4];

	glViewport((GLint)device.viewport.X, (GLint)device.viewport.Y,
		(GLsizei)device.viewport.Width, (GLsizei)device.viewport.Height);
	glDepthRange(device.viewport.MinZ, device.viewport.MaxZ);

	if (has_depth && rs[D3DRS_ZENABLE])
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(rs[D3DRS_ZFUNC] ? (GLenum)rs[D3DRS_ZFUNC] : GL_NEVER);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
	glDepthMask(has_depth && rs[D3DRS_ZENABLE] && rs[D3DRS_ZWRITEENABLE] ? GL_TRUE : GL_FALSE);

	if (has_depth && rs[D3DRS_STENCILENABLE])
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(rs[D3DRS_STENCILFUNC] ? (GLenum)rs[D3DRS_STENCILFUNC] : GL_NEVER,
			(GLint)rs[D3DRS_STENCILREF], rs[D3DRS_STENCILMASK]);
		glStencilOp(stencil_operation(rs[D3DRS_STENCILFAIL]), stencil_operation(rs[D3DRS_STENCILZFAIL]),
			stencil_operation(rs[D3DRS_STENCILPASS]));
		glStencilMask(rs[D3DRS_STENCILWRITEMASK]);
	}
	else
	{
		glDisable(GL_STENCIL_TEST);
	}

	if (rs[D3DRS_ALPHABLENDENABLE])
	{
		glEnable(GL_BLEND);
		glBlendFunc((GLenum)rs[D3DRS_SRCBLEND], (GLenum)rs[D3DRS_DESTBLEND]);
		glBlendEquation(blend_equation(rs[D3DRS_BLENDOP]));
		color_to_vec4(rs[D3DRS_BLENDCOLOR], blend_color);
		glBlendColor(blend_color[0], blend_color[1], blend_color[2], blend_color[3]);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	glColorMask((write & D3DCOLORWRITEENABLE_RED) != 0, (write & D3DCOLORWRITEENABLE_GREEN) != 0,
		(write & D3DCOLORWRITEENABLE_BLUE) != 0, (write & D3DCOLORWRITEENABLE_ALPHA) != 0);

	/* the cull mode names the winding to discard; FRONTFACE names the
	front winding */
	if (rs[D3DRS_CULLMODE] == D3DCULL_NONE)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);
#ifdef HALO_ANDROID
		/* the vertex shader flips y in clip space, which (unlike desktop
		GL's upper-left clip origin) also flips the winding */
		glFrontFace(rs[D3DRS_FRONTFACE] == D3DFRONT_CCW ? GL_CW : GL_CCW);
#else
		glFrontFace(rs[D3DRS_FRONTFACE] == D3DFRONT_CCW ? GL_CCW : GL_CW);
#endif
		glCullFace(rs[D3DRS_CULLMODE] == rs[D3DRS_FRONTFACE] ? GL_FRONT : GL_BACK);
	}
#ifndef HALO_ANDROID
	/* ES draws filled polygons only (wireframe is a debug mode) */
	glPolygonMode(GL_FRONT_AND_BACK, rs[D3DRS_FILLMODE] == D3DFILL_WIREFRAME ? GL_LINE :
		rs[D3DRS_FILLMODE] == D3DFILL_POINT ? GL_POINT : GL_FILL);
#endif

	/* D3DRS_ZBIAS is expressed in these states (D3DDevice_SetRenderState_ZBias) */
	if (rs[D3DRS_SOLIDOFFSETENABLE])
	{
		float slope = dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]);
		float offset = dword_to_float(rs[D3DRS_POLYGONOFFSETZOFFSET]);

		glEnable(GL_POLYGON_OFFSET_FILL);
#ifndef HALO_ANDROID
		glEnable(GL_POLYGON_OFFSET_LINE);
#endif
		glPolygonOffset(slope, offset);
	}
	else
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
#ifndef HALO_ANDROID
		glDisable(GL_POLYGON_OFFSET_LINE);
#endif
	}
}

#ifdef HALO_ANDROID
/* ES has no debug callback in 3.0; HALO_GL_DEBUG polls glGetError around
each draw instead, reporting each distinct error a few times */
static void gl_check_errors(const char *where)
{
	static int enabled = -1;
	static unsigned long reports;
	GLenum error;

	if (enabled < 0)
		enabled = getenv("HALO_GL_DEBUG") != NULL;
	if (!enabled)
		return;
	while ((error = glGetError()) != GL_NO_ERROR)
	{
		if (reports++ < 200)
			platform_log("GL error %04x at %s (frame %lu)", (unsigned)error, where, device.frame);
	}
}
#else
#define gl_check_errors(where) ((void)0)
#endif

static struct program_entry *prepare_draw(BOOL immediate)
{
	struct vertex_shader_object *program = current_program();
	struct nv2a_pixel_shader_key key;
	struct program_entry *entry;
	float texture_scale[4][4];
	float ps_c0[8][4], ps_c1[8][4], final_c0[4], final_c1[4], fog_color[4];
	float fog_parameters[4], bump_matrix[4][4], bump_luminance[4][4];
	BOOL has_depth = FALSE;
	int stage;

	if (!device.gl_ready || !program || !device.vertex_shader || !program->instructions)
	{
		stats.skipped_no_program++;
		return NULL;
	}
	{
		/* HALO_GPU_SKIP_VS=<id>,<id>... drops draws by vertex shader, for
		finding which pass produces something */
		const char *skip = getenv("HALO_GPU_SKIP_VS");

		while (skip && *skip)
		{
			if ((unsigned long)atol(skip) == program->id)
				return NULL;
			skip = strchr(skip, ',');
			if (skip)
				skip++;
		}
	}
	if (!bind_targets(&has_depth))
	{
		stats.skipped_no_target++;
		return NULL;
	}
	apply_raster_state(has_depth);

	memset(&key, 0, sizeof(key));
	memcpy(key.combiner_state, D3D__RenderState, sizeof(key.combiner_state));
	/* constants are uniforms, not part of the program */
	memset(&key.combiner_state[D3DRS_PSCONSTANT0_0], 0, 16 * sizeof(DWORD));
	key.combiner_state[D3DRS_PSFINALCOMBINERCONSTANT0] = 0;
	key.combiner_state[D3DRS_PSFINALCOMBINERCONSTANT1] = 0;
	key.texture_modes = D3D__RenderState[D3DRS_PSTEXTUREMODES];
	bind_textures(&key, texture_scale);
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		key.alpha_kill[stage] = D3D__TextureState[stage][D3DTSS_ALPHAKILL] == D3DTALPHAKILL_ENABLE;
		key.color_sign[stage] = (unsigned char)((D3D__TextureState[stage][D3DTSS_COLORSIGN] >> 28) & 0xf);
	}
	key.alpha_test_function = D3D__RenderState[D3DRS_ALPHATESTENABLE] ? D3D__RenderState[D3DRS_ALPHAFUNC] : 0;
	key.fog_enable = D3D__RenderState[D3DRS_FOGENABLE] != 0;
	key.fog_table_mode = (unsigned char)D3D__RenderState[D3DRS_FOGTABLEMODE];
#ifdef HALO_ANDROID
	key.count_samples = device.visibility_test_active && xgpu_capabilities.atomic_counters;
#endif

	entry = program_get(vertex_shader_get(program, immediate), fragment_shader_get(&key));
	if (!entry)
	{
		stats.skipped_link++;
		gl_check_errors("program");
		return NULL;
	}
	gl_check_errors("state");
	if (immediate)
		stats.immediate_draws++;
	else
		stats.draws++;
	glUseProgram(entry->program);
#ifdef HALO_ANDROID
	if (key.count_samples)
		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, device.visibility_counters,
			(GLintptr)(device.counter_active * sizeof(GLuint)), sizeof(GLuint));
#endif

	glUniform4fv(entry->constants, XGPU_VERTEX_CONSTANT_COUNT, &device.constants[0][0]);
	glUniform4fv(entry->viewport_scale, 1, device.viewport_scale);
	glUniform4fv(entry->viewport_offset, 1, device.viewport_offset);
	glUniform1f(entry->point_size, D3D__RenderState[D3DRS_POINTSIZE] ? dword_to_float(D3D__RenderState[D3DRS_POINTSIZE]) : 1.0f);
	for (stage = 0; stage < 8; stage++)
	{
		color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0 + stage], ps_c0[stage]);
		color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0 + stage], ps_c1[stage]);
	}
	color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0], final_c0);
	color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1], final_c1);
	color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR], fog_color);
	fog_parameters[0] = dword_to_float(D3D__RenderState[D3DRS_FOGSTART]);
	fog_parameters[1] = dword_to_float(D3D__RenderState[D3DRS_FOGEND]);
	fog_parameters[2] = dword_to_float(D3D__RenderState[D3DRS_FOGDENSITY]);
	fog_parameters[3] = 0.0f;
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		DWORD *state = D3D__TextureState[stage];

		bump_matrix[stage][0] = dword_to_float(state[D3DTSS_BUMPENVMAT00]);
		bump_matrix[stage][1] = dword_to_float(state[D3DTSS_BUMPENVMAT01]);
		bump_matrix[stage][2] = dword_to_float(state[D3DTSS_BUMPENVMAT10]);
		bump_matrix[stage][3] = dword_to_float(state[D3DTSS_BUMPENVMAT11]);
		bump_luminance[stage][0] = dword_to_float(state[D3DTSS_BUMPENVLSCALE]);
		bump_luminance[stage][1] = dword_to_float(state[D3DTSS_BUMPENVLOFFSET]);
		bump_luminance[stage][2] = bump_luminance[stage][3] = 0.0f;
	}
	glUniform4fv(entry->ps_c0, 8, &ps_c0[0][0]);
	glUniform4fv(entry->ps_c1, 8, &ps_c1[0][0]);
	glUniform4fv(entry->ps_final_c0, 1, final_c0);
	glUniform4fv(entry->ps_final_c1, 1, final_c1);
	glUniform4fv(entry->fog_color, 1, fog_color);
	glUniform4fv(entry->fog_parameters, 1, fog_parameters);
	glUniform1f(entry->alpha_reference, (float)(D3D__RenderState[D3DRS_ALPHAREF] & 0xff));
	glUniform4fv(entry->bump_matrix, 4, &bump_matrix[0][0]);
	glUniform4fv(entry->bump_luminance, 4, &bump_luminance[0][0]);
	glUniform4fv(entry->texture_scale, 4, &texture_scale[0][0]);
	if (entry->screen_offset >= 0)
		glUniform1f(entry->screen_offset, (float)UI_OFFSET);
	if (entry->texture_lod_bias >= 0)
	{
		float bias[4];

		for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
			bias[stage] = dword_to_float(D3D__TextureState[stage][D3DTSS_MIPMAPLODBIAS]);
		glUniform4fv(entry->texture_lod_bias, 1, bias);
	}
	return entry;
}

/* ---------- tracing (HALO_GPU_TRACE=<frame>) */

static BOOL trace_frame(void)
{
	static long frame = -2;

	if (frame == -2)
		frame = getenv("HALO_GPU_TRACE") ? atol(getenv("HALO_GPU_TRACE")) : -1;
	return frame >= 0 && device.frame == (unsigned long)frame;
}

static void trace_draw(const char *kind, D3DPRIMITIVETYPE type, unsigned long count, const float *first_vertex)
{
	struct vertex_shader_object *program = current_program();
	DWORD *rs = D3D__RenderState;

	if (!trace_frame())
		return;
	platform_log("%s type %d count %lu vs %lu (decl %lu) vp %lu,%lu %lux%lu z%.2f-%.2f zen %lu zw %lu zf %lx blend %lu %lx/%lx cull %lx cw %08lx tm %05lx cc %lx fin %08lx/%08lx at %lu/%lx",
		kind, type, count, program ? program->id : 0, device.vertex_shader ? device.vertex_shader->id : 0,
		device.viewport.X, device.viewport.Y, device.viewport.Width, device.viewport.Height,
		device.viewport.MinZ, device.viewport.MaxZ, rs[D3DRS_ZENABLE], rs[D3DRS_ZWRITEENABLE], rs[D3DRS_ZFUNC],
		rs[D3DRS_ALPHABLENDENABLE], rs[D3DRS_SRCBLEND], rs[D3DRS_DESTBLEND], rs[D3DRS_CULLMODE],
		rs[D3DRS_COLORWRITEENABLE], rs[D3DRS_PSTEXTUREMODES], rs[D3DRS_PSCOMBINERCOUNT],
		rs[D3DRS_PSFINALCOMBINERINPUTSABCD], rs[D3DRS_PSFINALCOMBINERINPUTSEFG],
		rs[D3DRS_ALPHATESTENABLE], rs[D3DRS_ALPHAFUNC]);
	{
		int stage;

		for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
		{
			D3DBaseTexture *texture = device.textures[stage];
			struct xgpu_texture_description description;

			if (!texture || !((D3D__RenderState[D3DRS_PSTEXTUREMODES] >> (5 * stage)) & 0x1f))
				continue;
			xgpu_texture_describe(texture->Format, texture->Size, &description);
			platform_log("    t%d: data %08lx format %08lx size %08lx -> fmt %02lx %lux%lux%lu levels %lu linear %d cube %d rt %d min %lu mip %lu bias %g maxmip %lu",
				stage, texture->Data, texture->Format, texture->Size, description.format, description.width,
				description.height, description.depth, description.levels, description.linear, description.cube_map,
				xgpu_render_target_find(texture->Data) != NULL, D3D__TextureState[stage][D3DTSS_MINFILTER],
				D3D__TextureState[stage][D3DTSS_MIPFILTER], dword_to_float(D3D__TextureState[stage][D3DTSS_MIPMAPLODBIAS]),
				D3D__TextureState[stage][D3DTSS_MAXMIPLEVEL]);
		}
	}
	platform_log("    offset enable %lu slope %g offset %g zbias %ld stencil %lu func %lx ref %lx mask %lx write %lx ops %lx/%lx/%lx",
		rs[D3DRS_SOLIDOFFSETENABLE], dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]),
		dword_to_float(rs[D3DRS_POLYGONOFFSETZOFFSET]), (long)rs[D3DRS_ZBIAS], rs[D3DRS_STENCILENABLE],
		rs[D3DRS_STENCILFUNC], rs[D3DRS_STENCILREF], rs[D3DRS_STENCILMASK], rs[D3DRS_STENCILWRITEMASK],
		rs[D3DRS_STENCILFAIL], rs[D3DRS_STENCILZFAIL], rs[D3DRS_STENCILPASS]);
	if (getenv("HALO_GPU_TRACE_CONSTANTS"))
	{
		int constant;

		for (constant = 0; constant < XGPU_VERTEX_CONSTANT_COUNT; constant++)
		{
			const float *value = device.constants[constant];

			if (value[0] || value[1] || value[2] || value[3])
				platform_log("    c[%d] = %g %g %g %g", constant, value[0], value[1], value[2], value[3]);
		}
	}
	if (device.vertex_shader)
	{
		unsigned long index;

		for (index = 0; index < device.vertex_shader->element_count; index++)
		{
			const struct vertex_element *element = &device.vertex_shader->elements[index];

			platform_log("    decl v%lu: stream %lu offset %lu type %02lx", (unsigned long)element->reg,
				(unsigned long)element->stream, (unsigned long)element->offset, (unsigned long)element->type);
		}
		for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
		{
			const float *value = device.attributes[index];

			if (value[0] || value[1] || value[2] || value[3] != 1.0f)
				platform_log("    current v%lu = %g %g %g %g", index, value[0], value[1], value[2], value[3]);
		}
	}
	if (first_vertex)
	{
		int reg;

		for (reg = 0; reg < XGPU_VERTEX_ATTRIBUTE_COUNT; reg++)
		{
			const float *v = first_vertex + reg * 4;

			if (v[0] || v[1] || v[2] || v[3] != 1.0f)
				platform_log("    v%d = %g %g %g %g", reg, v[0], v[1], v[2], v[3]);
		}
	}
}


/* ---------- vertex data */

/* makes room for size bytes of uploads, orphaning the stream buffer if it
is full. A draw reserves room for all of its streams at once: orphaning
between two of them would leave the attributes already pointed at the
buffer reading its new, empty storage. */
static void stream_reserve(unsigned long size)
{
	if (device.stream_offset + size > STREAM_BUFFER_SIZE)
	{
		/* orphan the buffer and start again */
		glBindBuffer(GL_ARRAY_BUFFER, device.stream_buffer);
		glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		device.stream_offset = 0;
	}
}

static unsigned long stream_upload(const void *data, unsigned long size)
{
	unsigned long offset;

	size = (size + 15) & ~15UL;
	stream_reserve(size);
	offset = device.stream_offset;
	glBindBuffer(GL_ARRAY_BUFFER, device.stream_buffer);
#ifdef HALO_ANDROID
	host_gl_buffer_write(GL_ARRAY_BUFFER, (unsigned int)offset, (unsigned int)size, data);
#else
	glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
#endif
	device.stream_offset += size;
	return offset;
}

#ifdef HALO_ANDROID
/* stream_upload, with the D3DCOLOR elements of the stream turned from BGRA
into the RGBA byte order ES reads */
static unsigned long stream_upload_swizzled(const struct vertex_shader_object *declaration, unsigned long stream,
	const unsigned char *data, unsigned long size, unsigned long stride)
{
	static unsigned char *scratch;
	static unsigned long scratch_size;
	unsigned long offsets[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long count = 0, index, vertex;

	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];

		if (element->stream == stream && element->type == D3DVSDT_D3DCOLOR)
			offsets[count++] = element->offset;
	}
	if (!count || !stride)
		return stream_upload(data, size);
	if (scratch_size < size)
	{
		free(scratch);
		scratch_size = size + 65536;
		scratch = malloc(scratch_size);
	}
	memcpy(scratch, data, size);
	for (vertex = 0; vertex + stride <= size; vertex += stride)
	{
		for (index = 0; index < count; index++)
		{
			unsigned char *color = scratch + vertex + offsets[index];
			unsigned char blue = color[0];

			color[0] = color[2];
			color[2] = blue;
		}
	}
	return stream_upload(scratch, size);
}
#endif

static unsigned long index_upload(const void *data, unsigned long size)
{
	unsigned long offset;

	size = (size + 15) & ~15UL;
	if (device.index_offset + size > INDEX_BUFFER_SIZE)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		device.index_offset = 0;
	}
	offset = device.index_offset;
#ifdef HALO_ANDROID
	host_gl_buffer_write(GL_ELEMENT_ARRAY_BUFFER, (unsigned int)offset, (unsigned int)size, data);
#else
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
#endif
	device.index_offset += size;
	return offset;
}

static void attribute_format(const struct vertex_element *element, GLint *size, GLenum *type, GLboolean *normalized)
{
	*normalized = GL_FALSE;
	switch (element->type)
	{
	case D3DVSDT_FLOAT1: *size = 1; *type = GL_FLOAT; break;
	case D3DVSDT_FLOAT2: *size = 2; *type = GL_FLOAT; break;
	case D3DVSDT_FLOAT3: case D3DVSDT_FLOAT2H: *size = 3; *type = GL_FLOAT; break;
	case D3DVSDT_FLOAT4: *size = 4; *type = GL_FLOAT; break;
#ifdef HALO_ANDROID
	/* ES has no BGRA attributes: stream_upload_swizzled swaps the bytes */
	case D3DVSDT_D3DCOLOR: *size = 4; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
#else
	case D3DVSDT_D3DCOLOR: *size = GL_BGRA; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
#endif
	case D3DVSDT_SHORT1: *size = 1; *type = GL_SHORT; break;
	case D3DVSDT_SHORT2: *size = 2; *type = GL_SHORT; break;
	case D3DVSDT_SHORT3: *size = 3; *type = GL_SHORT; break;
	case D3DVSDT_SHORT4: *size = 4; *type = GL_SHORT; break;
	case D3DVSDT_NORMSHORT1: *size = 1; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case D3DVSDT_NORMSHORT2: *size = 2; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case D3DVSDT_NORMSHORT3: *size = 3; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case D3DVSDT_NORMSHORT4: *size = 4; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case D3DVSDT_PBYTE1: *size = 1; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case D3DVSDT_PBYTE2: *size = 2; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case D3DVSDT_PBYTE3: *size = 3; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case D3DVSDT_PBYTE4: *size = 4; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	default: *size = 4; *type = GL_FLOAT; break;
	}
}

/* upload vertices [first, first + count) of every stream the declaration
uses and point the attributes at them; attribute data then starts at
vertex 0 of the uploaded range */
static void setup_streams(unsigned long first, unsigned long count)
{
	struct vertex_shader_object *declaration = device.vertex_shader;
	unsigned long stream_offsets[16];
	BOOL uploaded[16] = { FALSE };
	BOOL enabled[XGPU_VERTEX_ATTRIBUTE_COUNT] = { FALSE };
	unsigned long index, total = 0;

	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];
		unsigned long stream = element->stream;
		unsigned long stride = device.streams[stream].stride;

		if (!device.streams[stream].data || element->type == D3DVSDT_NONE || uploaded[stream])
			continue;
		total += ((stride ? stride * count : 64) + 15) & ~15UL;
		uploaded[stream] = TRUE;
	}
	stream_reserve(total);
	memset(uploaded, 0, sizeof(uploaded));
	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];
		unsigned long stream = element->stream;
		unsigned long stride = device.streams[stream].stride;
		GLint size;
		GLenum type;
		GLboolean normalized;

		if (!device.streams[stream].data || element->type == D3DVSDT_NONE)
			continue;
		if (!uploaded[stream])
		{
			const unsigned char *base = PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data);
			unsigned long bytes = stride ? stride * count : 64;

#ifdef HALO_ANDROID
			stream_offsets[stream] = stream_upload_swizzled(declaration, stream, base + first * stride, bytes, stride);
#else
			stream_offsets[stream] = stream_upload(base + first * stride, bytes);
#endif
			uploaded[stream] = TRUE;
		}
		glBindBuffer(GL_ARRAY_BUFFER, device.stream_buffer);
		glEnableVertexAttribArray(element->reg);
		if (element->type == D3DVSDT_NORMPACKED3)
		{
			glVertexAttribIPointer(element->reg, 1, GL_UNSIGNED_INT, (GLsizei)stride,
				(const void *)(stream_offsets[stream] + element->offset));
		}
		else
		{
			attribute_format(element, &size, &type, &normalized);
			glVertexAttribPointer(element->reg, size, type, normalized, (GLsizei)stride,
				(const void *)(stream_offsets[stream] + element->offset));
		}
		enabled[element->reg] = TRUE;
	}
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		if (!enabled[index])
		{
			glDisableVertexAttribArray(index);
			if (declaration->packed_mask & (1UL << index))
				glVertexAttribI4ui(index, 0, 0, 0, 0);
			else
				glVertexAttrib4fv(index, device.attributes[index]);
		}
	}
}

static GLenum primitive_mode(D3DPRIMITIVETYPE type)
{
	switch (type)
	{
	case D3DPT_POINTLIST: return GL_POINTS;
	case D3DPT_LINELIST: return GL_LINES;
	case D3DPT_LINELOOP: return GL_LINE_LOOP;
	case D3DPT_LINESTRIP: return GL_LINE_STRIP;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_QUADSTRIP: return GL_TRIANGLE_STRIP;
	case D3DPT_TRIANGLEFAN:
	case D3DPT_POLYGON: return GL_TRIANGLE_FAN;
	default: return GL_TRIANGLES;
	}
}

/* quads become two triangles each */
static WORD *quad_indices(const WORD *indices, unsigned long count, unsigned long *out_count)
{
	unsigned long quads = count / 4;
	WORD *result = malloc(quads * 6 * sizeof(WORD) + 2);
	unsigned long quad;

	for (quad = 0; quad < quads; quad++)
	{
		WORD v0 = indices ? indices[quad * 4] : (WORD)(quad * 4);
		WORD v1 = indices ? indices[quad * 4 + 1] : (WORD)(quad * 4 + 1);
		WORD v2 = indices ? indices[quad * 4 + 2] : (WORD)(quad * 4 + 2);
		WORD v3 = indices ? indices[quad * 4 + 3] : (WORD)(quad * 4 + 3);

		result[quad * 6 + 0] = v0;
		result[quad * 6 + 1] = v1;
		result[quad * 6 + 2] = v2;
		result[quad * 6 + 3] = v0;
		result[quad * 6 + 4] = v2;
		result[quad * 6 + 5] = v3;
	}
	*out_count = quads * 6;
	return result;
}

void WINAPI D3DDevice_SetStreamSource(UINT stream_number, D3DVertexBuffer *stream_data, UINT stride)
{
	if (stream_number >= 16)
		return;
	device.streams[stream_number].data = stream_data ? stream_data->Data : 0;
	device.streams[stream_number].stride = stride;
}

void WINAPI D3DDevice_SetIndices(D3DIndexBuffer *index_data, UINT base_vertex_index)
{
	(void)base_vertex_index;
	D3D__IndexData = index_data ? (WORD *)index_data->Data : NULL;
}

void WINAPI D3DDevice_DrawVertices(D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT vertex_count)
{
	if (!vertex_count || !prepare_draw(FALSE))
		return;
	trace_draw("draw", primitive_type, vertex_count, NULL);
	setup_streams(start_vertex, vertex_count);
	if (primitive_type == D3DPT_QUADLIST)
	{
		unsigned long count;
		WORD *indices = quad_indices(NULL, vertex_count, &count);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.index_buffer);
		glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_SHORT,
			(const void *)index_upload(indices, count * sizeof(WORD)));
		free(indices);
	}
	else
	{
		glDrawArrays(primitive_mode(primitive_type), 0, (GLsizei)vertex_count);
	}
	gl_check_errors("draw");
}

void WINAPI D3DDevice_DrawIndexedVertices(D3DPRIMITIVETYPE primitive_type, UINT vertex_count, CONST WORD *index_data)
{
	unsigned long minimum = 0xffff, maximum = 0, index, count;
	WORD *indices = NULL;
	const WORD *source = index_data;

	if (!vertex_count || !index_data || !prepare_draw(FALSE))
		return;
	for (index = 0; index < vertex_count; index++)
	{
		if (index_data[index] < minimum)
			minimum = index_data[index];
		if (index_data[index] > maximum)
			maximum = index_data[index];
	}
	trace_draw("indexed", primitive_type, vertex_count, NULL);
	setup_streams(minimum, maximum - minimum + 1);
	count = vertex_count;
	if (primitive_type == D3DPT_QUADLIST)
	{
		indices = quad_indices(index_data, vertex_count, &count);
		source = indices;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.index_buffer);
#ifdef HALO_ANDROID
	{
		/* the indices are copied anyway: rebase them rather than rely on
		ES 3.2's glDrawElementsBaseVertex */
		WORD *rebased = malloc(count * sizeof(WORD) + 2);

		for (index = 0; index < count; index++)
			rebased[index] = (WORD)(source[index] - minimum);
		glDrawElements(primitive_mode(primitive_type), (GLsizei)count, GL_UNSIGNED_SHORT,
			(const void *)index_upload(rebased, count * sizeof(WORD)));
		free(rebased);
	}
#else
	glDrawElementsBaseVertex(primitive_mode(primitive_type), (GLsizei)count, GL_UNSIGNED_SHORT,
		(const void *)index_upload(source, count * sizeof(WORD)), -(GLint)minimum);
#endif
	free(indices);
}

/* ---------- immediate mode */

void WINAPI D3DDevice_Begin(D3DPRIMITIVETYPE primitive_type)
{
	device.immediate_active = TRUE;
	device.immediate_type = primitive_type;
	device.immediate_count = 0;
}

static void immediate_emit(void)
{
	unsigned long floats = XGPU_VERTEX_ATTRIBUTE_COUNT * 4;

	if (device.immediate_count == device.immediate_capacity)
	{
		device.immediate_capacity = device.immediate_capacity ? device.immediate_capacity * 2 : 256;
		device.immediate_vertices = realloc(device.immediate_vertices,
			device.immediate_capacity * floats * sizeof(float));
	}
	memcpy(device.immediate_vertices + device.immediate_count * floats, device.attributes, floats * sizeof(float));
	device.immediate_count++;
}

void WINAPI D3DDevice_End(void)
{
	unsigned long stride = XGPU_VERTEX_ATTRIBUTE_COUNT * 4 * sizeof(float);
	unsigned long offset, index, count = device.immediate_count;
	D3DPRIMITIVETYPE type = device.immediate_type;

	device.immediate_active = FALSE;
	if (!count || !prepare_draw(TRUE))
		return;
	trace_draw("immediate", type, count, device.immediate_vertices);
	offset = stream_upload(device.immediate_vertices, count * stride);
	glBindBuffer(GL_ARRAY_BUFFER, device.stream_buffer);
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, (GLsizei)stride,
			(const void *)(offset + index * 4 * sizeof(float)));
	}
	if (type == D3DPT_QUADLIST)
	{
		unsigned long index_count;
		WORD *indices = quad_indices(NULL, count, &index_count);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device.index_buffer);
		glDrawElements(GL_TRIANGLES, (GLsizei)index_count, GL_UNSIGNED_SHORT,
			(const void *)index_upload(indices, index_count * sizeof(WORD)));
		free(indices);
	}
	else
	{
		glDrawArrays(primitive_mode(type), 0, (GLsizei)count);
	}
	gl_check_errors("immediate draw");
}

static void set_attribute(INT reg, float a, float b, float c, float d)
{
	BOOL emit = FALSE;

	if (reg == D3DVSDE_VERTEX)
	{
		reg = 0;
		emit = TRUE;
	}
	if (reg < 0 || reg >= XGPU_VERTEX_ATTRIBUTE_COUNT)
		return;
	device.attributes[reg][0] = a;
	device.attributes[reg][1] = b;
	device.attributes[reg][2] = c;
	device.attributes[reg][3] = d;
	/* like the hardware, writing register 0 completes a vertex */
	if (device.immediate_active && (emit || reg == 0))
		immediate_emit();
}

void WINAPI D3DDevice_SetVertexData2f(INT reg, FLOAT a, FLOAT b)
{
	set_attribute(reg, a, b, 0.0f, 1.0f);
}

void WINAPI D3DDevice_SetVertexData4f(INT reg, FLOAT a, FLOAT b, FLOAT c, FLOAT d)
{
	set_attribute(reg, a, b, c, d);
}

void WINAPI D3DDevice_SetVertexData2s(INT reg, SHORT a, SHORT b)
{
	set_attribute(reg, (float)a, (float)b, 0.0f, 1.0f);
}

void WINAPI D3DDevice_SetVertexData4ub(INT reg, BYTE a, BYTE b, BYTE c, BYTE d)
{
	set_attribute(reg, a / 255.0f, b / 255.0f, c / 255.0f, d / 255.0f);
}

void WINAPI D3DDevice_SetVertexDataColor(INT reg, D3DCOLOR color)
{
	float value[4];

	color_to_vec4(color, value);
	set_attribute(reg, value[0], value[1], value[2], value[3]);
}

/* ---------- clearing */

void WINAPI D3DDevice_Clear(DWORD count, CONST D3DRECT *rectangles, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
	float rgba[4];
	GLbitfield mask = 0;
	BOOL has_depth = FALSE;
	DWORD index;

	if (!device.gl_ready || !bind_targets(&has_depth))
		return;
	if (trace_frame())
		platform_log("clear flags %lx color %08lx z %g count %lu target %08lx depth %08lx", (unsigned long)flags,
			(unsigned long)color, z, (unsigned long)count,
			device.render_target ? (unsigned long)device.render_target->Data : 0,
			device.depth_stencil ? (unsigned long)device.depth_stencil->Data : 0);
	stats.clears++;
	color_to_vec4(color, rgba);
	if (flags & D3DCLEAR_TARGET)
	{
		/* the Xbox clears the channels named (D3DCLEAR_TARGET_R, _G, _B, _A):
		the fog screen clears only alpha, leaving the picture under the fog */
		glColorMask((flags & D3DCLEAR_TARGET_R) != 0, (flags & D3DCLEAR_TARGET_G) != 0,
			(flags & D3DCLEAR_TARGET_B) != 0, (flags & D3DCLEAR_TARGET_A) != 0);
		glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
		mask |= GL_COLOR_BUFFER_BIT;
	}
	if (has_depth && (flags & D3DCLEAR_ZBUFFER))
	{
		glDepthMask(GL_TRUE);
		glClearDepth(z);
		mask |= GL_DEPTH_BUFFER_BIT;
	}
	if (has_depth && (flags & D3DCLEAR_STENCIL))
	{
		glStencilMask(0xff);
		glClearStencil((GLint)stencil);
		mask |= GL_STENCIL_BUFFER_BIT;
	}
	if (!mask)
		return;
	if (!count || !rectangles)
	{
		glDisable(GL_SCISSOR_TEST);
		glClear(mask);
		return;
	}
	glEnable(GL_SCISSOR_TEST);
	for (index = 0; index < count; index++)
	{
		glScissor(rectangles[index].x1 + UI_OFFSET, rectangles[index].y1,
			rectangles[index].x2 - rectangles[index].x1, rectangles[index].y2 - rectangles[index].y1);
		glClear(mask);
	}
	glDisable(GL_SCISSOR_TEST);
}

/* ---------- presentation */

static void write_screenshot(struct render_target_entry *target)
{
	const char *directory = getenv("HALO_SCREENSHOT_DIR");
	unsigned long width = target->target.width, height = target->target.height;
	unsigned char *pixels;
	char path[512];
	FILE *file;
	unsigned long row;
	unsigned char header[54] = { 'B', 'M' };
	unsigned long image_size = width * height * 4;

	if (!directory)
		return;
	pixels = malloc(image_size);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_get(target->target.texture, 0));
	glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	/* the display ignores destination alpha, which the game uses as scratch;
	image viewers would show it as transparency */
	for (row = 0; row < width * height; row++)
	{
#ifdef HALO_ANDROID
		unsigned char red = pixels[row * 4];

		pixels[row * 4] = pixels[row * 4 + 2];
		pixels[row * 4 + 2] = red;
#endif
		pixels[row * 4 + 3] = 0xff;
	}
	snprintf(path, sizeof(path), "%s/frame%05lu.bmp", directory, device.frame);
	file = fopen(path, "wb");
	if (file)
	{
		*(unsigned int *)(header + 2) = (unsigned int)(54 + image_size);
		*(unsigned int *)(header + 10) = 54;
		*(unsigned int *)(header + 14) = 40;
		*(int *)(header + 18) = (int)width;
		*(int *)(header + 22) = -(int)height; /* rows from the top, as read */
		*(unsigned short *)(header + 26) = 1;
		*(unsigned short *)(header + 28) = 32;
		*(unsigned int *)(header + 34) = (unsigned int)image_size;
		fwrite(header, 1, sizeof(header), file);
		for (row = 0; row < height; row++)
			fwrite(pixels + row * width * 4, 1, width * 4, file);
		fclose(file);
	}
	free(pixels);
}

void WINAPI D3DDevice_Present(CONST RECT *source_rectangle, CONST RECT *destination_rectangle,
	void *unused, void *unused2)
{
	static long screenshot_every = -1;

	(void)source_rectangle;
	(void)destination_rectangle;
	(void)unused;
	(void)unused2;
	if (screenshot_every < 0)
		screenshot_every = getenv("HALO_SCREENSHOT_EVERY") ? atol(getenv("HALO_SCREENSHOT_EVERY")) : 0;

	if (device.gl_ready)
	{
		struct render_target_entry *back_buffer = render_target_get(&device.back_buffer);
		int window_width, window_height, width, height, x, y;

		if (trace_frame())
			platform_log("present back buffer %08lx texture %u", (unsigned long)device.back_buffer.Data,
				back_buffer->target.texture);
		if (screenshot_every > 0 && device.frame % (unsigned long)screenshot_every == 0)
			write_screenshot(back_buffer);

		platform_video_drawable_size(&window_width, &window_height);
		/* letterbox to the back buffer's aspect ratio */
		width = window_width;
		height = (int)((long)window_width * back_buffer->target.height / back_buffer->target.width);
		if (height > window_height)
		{
			height = window_height;
			width = (int)((long)window_height * back_buffer->target.width / back_buffer->target.height);
		}
		x = (window_width - width) / 2;
		y = (window_height - height) / 2;
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDisable(GL_SCISSOR_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_get(back_buffer->target.texture, 0));
		/* row 0 of the render target is the top of the picture */
		glBlitFramebuffer(0, 0, (GLint)back_buffer->target.width, (GLint)back_buffer->target.height,
			x, y + height, x + width, y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		platform_video_swap();
		xgpu_texture_cache_begin_frame();
#ifdef HALO_ANDROID
		host_gl_fence_frame((unsigned int)device.buffer_ring);
		device.buffer_ring = (device.buffer_ring + 1) % STREAM_BUFFER_RING;
		host_gl_wait_frame((unsigned int)device.buffer_ring);
		device.stream_buffer = device.stream_buffers[device.buffer_ring];
		device.index_buffer = device.index_buffers[device.buffer_ring];
		device.stream_offset = 0;
		device.index_offset = 0;
#else
		device.stream_offset = STREAM_BUFFER_SIZE; /* orphan next frame */
		device.index_offset = INDEX_BUFFER_SIZE;
#endif
	}
	device.frame++;
	stats.presents++;
	if (getenv("HALO_GPU_STATS") && device.frame % 60 == 0)
	{
		platform_log("frame %lu: %lu draws, %lu immediate, %lu clears, %lu target changes; skipped %lu no program, %lu no target, %lu link",
			device.frame, stats.draws / stats.presents, stats.immediate_draws / stats.presents, stats.clears / stats.presents,
			stats.target_changes / stats.presents, stats.skipped_no_program, stats.skipped_no_target, stats.skipped_link);
		memset(&stats, 0, sizeof(stats));
	}
	platform_pump_events();

	pthread_mutex_lock(&vertical_blank_lock);
	/* the Xbox keeps at most two frames queued behind its 60 Hz display;
	with interpolation, frames come at the real display's rate instead,
	paced by vsync (platform_video_swap) */
	if (halo_interpolation_enabled())
	{
		flip_count++;
	}
	else
	{
		while (pending_flips >= 2)
			pthread_cond_wait(&vertical_blank_condition, &vertical_blank_lock);
		pending_flips++;
	}
	pthread_mutex_unlock(&vertical_blank_lock);
}

HRESULT WINAPI D3DDevice_PersistDisplay(void)
{
	return S_OK;
}
