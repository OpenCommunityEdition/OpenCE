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
#include "gpu_gl.h"
#include "sdl_platform.h"
#include "halo_ui_pointer.h"
#include "port_config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void d3d8_surface_initialize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);
void d3d8_surface_resize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);

/* what the GPU backend can do (gl_initialize) */
struct gpu_capabilities xgpu_gpu_capabilities;

/* ---------- the screen's width

The Xbox screen is 640x480. The native ports can draw a wider one: 480
lines, and as many columns as the display's shape gives. On Android that is
display.screen_width (port_config.c; 640 keeps 4:3); on the desktop, the
shape of the window (of the display while the game is fullscreen, or of
display.resolution), and 640 where display.resolution_scaling is "original".
The game's camera derives its horizontal field of view from the viewport, so
the 3D view simply widens. The menus and full-screen overlays are laid out
for 640 columns; while they draw (halo_screen_ui_offset), everything shifts
right to center them.

The desktop also draws at that resolution (platform_screen_mode): render
targets the size of the screen get that many pixels (screen_scale), and
viewports, clears and visibility counts are scaled to match, so the game
still works in its 480 lines; "original" draws 640x480, scaled up at
presentation. The width and the scale change only between frames, after one
is presented (halo_screen_commit). */

#define SCREEN_HEIGHT 480
#define SCREEN_MAXIMUM_WIDTH 1920

/* the width the game draws, 0 until first asked, and how many pixels a
render target the size of the screen has per unit of it */
static long screen_width;
static float screen_scale[2] = { 1.0f, 1.0f };
static long ui_offset;
#define UI_OFFSET ((GLint)ui_offset)

/* ---------- anti-aliasing

display.anti_aliasing, off unless it is set: the Xbox drew without any.
"fxaa" and "smaa" are passes over each window's 3D view before the HUD and
menus are drawn over it (halo_screen_anti_alias, xgpu_post.c); "ssaa2x"
draws the screen's targets at twice the resolution each way
(screen_mode_choose), which the display blit scales down; "msaa2x" to
"msaa8x" draw the back buffer and its depth buffer with that many samples a
pixel (bind_targets). The setting is read again between frames
(halo_screen_commit), so that a change applies from the next one. */

enum
{
	_anti_aliasing_off,
	_anti_aliasing_fxaa,
	_anti_aliasing_smaa,
	_anti_aliasing_ssaa,
	_anti_aliasing_msaa,
};

static const struct
{
	const char *name;
	int mode;
	int samples;
} anti_aliasing_values[] =
{
	{ "off", _anti_aliasing_off, 0 },
	{ "fxaa", _anti_aliasing_fxaa, 0 },
#ifdef HALO_ANDROID
	/* (SMAA's three passes and supersampling's four times the pixels are
	more than a phone's GPU has to spare: FXAA in SMAA's place, and none) */
	{ "smaa", _anti_aliasing_fxaa, 0 },
	{ "ssaa2x", _anti_aliasing_off, 0 },
#else
	{ "smaa", _anti_aliasing_smaa, 0 },
	{ "ssaa2x", _anti_aliasing_ssaa, 0 },
#endif
	{ "msaa2x", _anti_aliasing_msaa, 2 },
	{ "msaa4x", _anti_aliasing_msaa, 4 },
	{ "msaa8x", _anti_aliasing_msaa, 8 },
};

#define NUMBER_OF_ANTI_ALIASING_VALUES ((int)(sizeof(anti_aliasing_values) / sizeof(anti_aliasing_values[0])))

/* the value in effect, -1 until the setting is first read; and
multisampling's samples a pixel, at most the GPU's (anti_aliasing_prepare;
the GPU's most samples and largest target are 0 until the GL context
exists, xgpu_gpu_capabilities) */
static int anti_aliasing_value = -1;
static int anti_aliasing_samples;

static void anti_aliasing_prepare(void);

/* display.anti_aliasing's value (none of them: the first, off) */
static void anti_aliasing_read(void)
{
	const char *setting = config_string("display.anti_aliasing");
	int value;

	for (value = NUMBER_OF_ANTI_ALIASING_VALUES - 1;
		value > 0 && strcmp(setting, anti_aliasing_values[value].name);
		value--)
	{
	}
	if (value == anti_aliasing_value)
		return;
	anti_aliasing_value = value;
	if (strcmp(setting, anti_aliasing_values[value].name))
		platform_log("anti-aliasing: \"%s\" is unknown, so off", setting);
	else
		platform_log("anti-aliasing: %s", setting);
	anti_aliasing_prepare();
}

static int anti_aliasing(void)
{
	if (anti_aliasing_value < 0)
		anti_aliasing_read();
	return anti_aliasing_values[anti_aliasing_value].mode;
}

static void screen_mode_choose(long *width, float scale[2])
{
#ifdef HALO_ANDROID
	/* display.screen_width, or 0 for the display's shape, which the app
	passes (port/android/host/host_main.c) */
	const char *display = getenv("HALO_DISPLAY_WIDTH");

	*width = config_integer("display.screen_width");
	if (*width <= 0)
		*width = display ? atol(display) : 640;
	if (*width < 640)
		*width = 640;
	if (*width > 1600)
		*width = 1600;
	*width &= ~1L;
	scale[0] = scale[1] = 1.0f;
#else
	long display_width, display_height;

	*width = 640;
	scale[0] = scale[1] = 1.0f;
	if (platform_screen_mode(&display_width, &display_height) && display_width > 0 && display_height > 0)
	{
		long wanted = (SCREEN_HEIGHT * display_width + display_height / 2) / display_height;

		*width = wanted < 640 ? 640 : wanted > SCREEN_MAXIMUM_WIDTH ? SCREEN_MAXIMUM_WIDTH : wanted & ~1L;
		scale[0] = (float)display_width / (float)*width;
		scale[1] = (float)display_height / (float)SCREEN_HEIGHT;
		/* a display narrower or wider than the game can be: the picture
		keeps its shape and the display blit letterboxes it */
		if (*width != wanted && *width != (wanted & ~1L))
			scale[0] = scale[1] = scale[0] < scale[1] ? scale[0] : scale[1];
		/* supersampling: twice the pixels each way, or as many as the GPU's
		largest target has (once it is known), which the display blit
		scales down; not with "original" resolution scaling, which draws
		the Xbox's 640x480 */
		if (anti_aliasing() == _anti_aliasing_ssaa && xgpu_gpu_capabilities.max_target_size > 0)
		{
			float maximum_target_size = (float)xgpu_gpu_capabilities.max_target_size;
			float factor = 2.0f;

			if (*width * scale[0] * factor > maximum_target_size)
				factor = maximum_target_size / (*width * scale[0]);
			if (SCREEN_HEIGHT * scale[1] * factor > maximum_target_size)
				factor = maximum_target_size / (SCREEN_HEIGHT * scale[1]);
			if (factor > 1.0f)
			{
				scale[0] *= factor;
				scale[1] *= factor;
			}
		}
	}
#endif
}

long halo_screen_width(void)
{
	if (!screen_width)
	{
		screen_mode_choose(&screen_width, screen_scale);
		platform_log("screen: %ldx%d drawn at %.0fx%.0f", screen_width, SCREEN_HEIGHT,
			screen_width * screen_scale[0], SCREEN_HEIGHT * screen_scale[1]);
	}
	return screen_width;
}

/* the display's pixels for each of the 480 lines (text_hires.c) */
float halo_screen_pixel_scale(void)
{
	halo_screen_width();
	return screen_scale[1];
}

/* display.shadow_resolution: the size the shadow maps are drawn at.

Each object's shadow is drawn from above into a 128x128 map, blurred into
another and projected onto the level under it (rasterizer_xbox_shadows.c).
On a large screen, a shadow's 128 texels show as steps along its edge, which
crawl as the object moves. The two maps, the game's only R5G6B5 render
targets (rasterizer_xbox.c), can be drawn larger as the screen's targets are
(render_target_get): the game's viewports, clears and quads, in its 128 units,
scale up with them. The scale is a power of two up to 8 (1024x1024), which
the blur needs to cover the same part of the map as the Xbox's
(rasterizer_shadow_convolve); 1, the default, draws them as the Xbox did. It
changes only between frames (halo_screen_commit). */
#define SHADOW_MAP_SIZE 128
#define SHADOW_MAP_MAXIMUM_SCALE 8

/* 0 until first asked */
static long shadow_scale;
static unsigned long shadow_scale_read_at;

static long shadow_scale_choose(void)
{
	long resolution = config_integer("display.shadow_resolution");
	long scale = 1;

	while (scale < SHADOW_MAP_MAXIMUM_SCALE && SHADOW_MAP_SIZE * scale * 2 <= resolution)
		scale *= 2;
	return scale;
}

/* the shadow maps' pixels for each of their 128 texels each way */
long halo_shadow_map_scale(void)
{
	if (!shadow_scale)
	{
		shadow_scale_read_at = config_changes();
		shadow_scale = shadow_scale_choose();
		platform_log("shadow maps: %ldx%ld", SHADOW_MAP_SIZE * shadow_scale, SHADOW_MAP_SIZE * shadow_scale);
	}
	return shadow_scale;
}

void halo_screen_ui_offset(unsigned char centered)
{
	ui_offset = centered ? (halo_screen_width() - 640) / 2 : 0;
}

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
	gpu_shader shader[2];
	/* one of the game's model lighting programs (halo_vertex_shader_lighting),
	whose draws can be lit for each pixel (display.per_pixel_lighting): where
	its lighting's normal and position are (lighting.lights is 0 for the
	others), and its shaders that hand them on, as shader[] */
	struct nv2a_vertex_lighting lighting;
	gpu_shader lit_shader[2];
	/* a shader lit for each pixel failed to compile or link: lit as the
	vertex shader lights it from then on */
	BOOL lighting_failed;
};

/* ---------- pixel shaders */

struct fragment_entry
{
	struct fragment_entry *next;
	unsigned long hash;
	struct nv2a_pixel_shader_key key;
	gpu_shader shader;
};

#define FRAGMENT_BUCKETS 1024

static struct fragment_entry *fragment_buckets[FRAGMENT_BUCKETS];

/* ---------- render targets */

struct render_target_entry
{
	struct render_target_entry *next;
	/* the next with the same address bucket (render_target_bucket) */
	struct render_target_entry *next_in_bucket;
	struct xgpu_render_target target;
	unsigned long last_rendered;
	/* the back buffer or its depth buffer, which the 3D view is drawn into:
	multisampled with multisampling (bind_targets) */
	BOOL screen_buffer;
};

/* every draw looks up its targets and whether its textures are render
targets, of which there are dozens */
#define RENDER_TARGET_BUCKET_COUNT 256

static struct render_target_entry *render_target_buckets[RENDER_TARGET_BUCKET_COUNT];

static struct render_target_entry **render_target_bucket(unsigned long data)
{
	return &render_target_buckets[((data >> 12) ^ (data >> 20)) % RENDER_TARGET_BUCKET_COUNT];
}

static struct render_target_entry *render_targets;

/* ---------- the device */

#define VISIBILITY_TEST_SLOTS GPU_VISIBILITY_SLOTS
/* a count from a test that only says whether any sample passed */
#define VISIBILITY_ALL_SAMPLES 1000000

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

	BOOL query_pending[VISIBILITY_TEST_SLOTS];
	/* the pixels each of the game's pixels covered in the test's target
	(render_target_get), which an exact count is divided by */
	float query_area[VISIBILITY_TEST_SLOTS];
	BOOL visibility_test_active;
	/* each slot's latest count known (gpu_visibility_result) */
	uint32_t visibility_known[VISIBILITY_TEST_SLOTS];

	unsigned long frame;
	unsigned long next_vertex_shader_id;
	BOOL gl_ready;
	BOOL created;
};

static struct gl_device device;

/* debug.gpu_stats prints these once a second */
static struct
{
	unsigned long draws, immediate_draws, clears, presents;
	unsigned long skipped_no_program, skipped_no_target, skipped_link;
	unsigned long target_changes;
	/* vertex and index bytes drawn from the mirror, and streamed */
	unsigned long mirrored_bytes, streamed_bytes;
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

/* the GLSL the translators write for this context (gl_initialize) */
static struct nv2a_dialect shader_dialect;

/* ---------- debugging settings, read once (gl_initialize) */

static struct
{
	/* debug.gpu_skip_vertex_shaders "<id>,<id>..." drops draws by vertex
	shader, for finding which pass produces something (port_config.c) */
	const char *skip_vertex_shaders;
	const char *dump_shaders;
	BOOL statistics;
} debug_settings;

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

/* the shadow maps: the game's only R5G6B5 render targets, 128x128
(rasterizer_xbox.c) */
static BOOL surface_is_shadow_map(const D3DSurface *surface)
{
	struct xgpu_texture_description description;

	xgpu_texture_describe(surface->Format, surface->Size, &description);
	return description.format == D3DFMT_R5G6B5 && description.width == SHADOW_MAP_SIZE &&
		description.height == SHADOW_MAP_SIZE;
}

/* the targets render_target_get found last, by what it found them from:
each draw asks again for the same two (bind_targets) */
#define RECENT_RENDER_TARGET_COUNT 4

static struct
{
	DWORD data, format, size;
	long screen_width;
	float scale[2];
	struct render_target_entry *entry;
} recent_render_targets[RECENT_RENDER_TARGET_COUNT];
static unsigned long recent_render_target_next;

static struct render_target_entry *render_target_remember(const D3DSurface *surface, long screen,
	struct render_target_entry *entry)
{
	unsigned long slot = recent_render_target_next++ % RECENT_RENDER_TARGET_COUNT;

	recent_render_targets[slot].data = surface->Data;
	recent_render_targets[slot].format = surface->Format;
	recent_render_targets[slot].size = surface->Size;
	recent_render_targets[slot].screen_width = screen;
	recent_render_targets[slot].scale[0] = screen_scale[0];
	recent_render_targets[slot].scale[1] = screen_scale[1];
	recent_render_targets[slot].entry = entry;
	return entry;
}

static struct render_target_entry *render_target_get(const D3DSurface *surface)
{
	struct render_target_entry *entry;
	unsigned long width, height, slot;
	long screen;
	BOOL depth;

	if (!surface || !surface->Data)
		return NULL;
	float scale[2] = { 1.0f, 1.0f };

	/* (the entries are never freed) */
	screen = halo_screen_width();
	for (slot = 0; slot < RECENT_RENDER_TARGET_COUNT; slot++)
	{
		if (recent_render_targets[slot].entry && recent_render_targets[slot].data == surface->Data &&
			recent_render_targets[slot].format == surface->Format && recent_render_targets[slot].size == surface->Size &&
			recent_render_targets[slot].screen_width == screen && recent_render_targets[slot].scale[0] == screen_scale[0] &&
			recent_render_targets[slot].scale[1] == screen_scale[1])
		{
			return recent_render_targets[slot].entry;
		}
	}
	surface_dimensions(surface, &width, &height, &depth);
	/* the screen's targets are drawn at the screen's scale, and the shadow
	maps at display.shadow_resolution's (halo_shadow_map_scale) */
	if (width == (unsigned long)halo_screen_width() && height == SCREEN_HEIGHT)
	{
		scale[0] = screen_scale[0];
		scale[1] = screen_scale[1];
	}
	else if (!depth && surface_is_shadow_map(surface))
	{
		scale[0] = (float)halo_shadow_map_scale();
		scale[1] = scale[0];
	}
	for (entry = *render_target_bucket(surface->Data); entry; entry = entry->next_in_bucket)
	{
		if (entry->target.data == surface->Data && entry->target.width == width &&
			entry->target.height == height && entry->target.depth == depth &&
			entry->target.scale[0] == scale[0] && entry->target.scale[1] == scale[1])
		{
			return render_target_remember(surface, screen, entry);
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->target.data = surface->Data;
	entry->target.width = width;
	entry->target.height = height;
	entry->target.depth = depth;
	entry->target.scale[0] = scale[0];
	entry->target.scale[1] = scale[1];
	entry->target.gl_width = (unsigned long)(width * scale[0] + 0.5f);
	entry->target.gl_height = (unsigned long)(height * scale[1] + 0.5f);
	{
		struct gpu_texture_description description;

		memset(&description, 0, sizeof(description));
		description.type = GPU_TEXTURE_2D;
		description.format = depth ? GPU_FORMAT_DEPTH_STENCIL : GPU_FORMAT_BGRA8;
		description.usage = GPU_USAGE_RENDER_TARGET;
		description.width = (uint32_t)entry->target.gl_width;
		description.height = (uint32_t)entry->target.gl_height;
		description.depth = 1;
		description.levels = 1;
		entry->target.texture = gpu_texture_create(&description);
	}
	entry->screen_buffer = surface->Data == (depth ? device.depth_buffer.Data : device.back_buffer.Data);
	entry->next = render_targets;
	render_targets = entry;
	entry->next_in_bucket = *render_target_bucket(entry->target.data);
	*render_target_bucket(entry->target.data) = entry;
	return entry;
}

struct xgpu_render_target *xgpu_render_target_find(unsigned long data)
{
	struct render_target_entry *entry, *best = NULL;

	for (entry = *render_target_bucket(data); entry; entry = entry->next_in_bucket)
	{
		if (entry->target.data == data && !entry->target.depth && (!best || entry->last_rendered > best->last_rendered))
			best = entry;
	}
	return best ? &best->target : NULL;
}

/* ---------- multisampling

With display.anti_aliasing's multisampling, the back buffer and its depth
buffer are drawn into multisampled storage (bind_targets), and so is
any target drawn together with one of them (the mirror's view goes to the
secondary target with the back buffer's depth buffer): a framebuffer's
attachments are all multisampled or none is. Nothing samples a depth buffer
as a texture (the game's copy of the depth buffer's memory is a target of
its own), so a depth buffer is resolved only when it stops being
multisampled. */

/* the samples a pixel of the bound targets: their multisampled storage's
(or 1) */
static int target_samples = 1;

/* counts draws and clears into render targets (xgpu_render_target.written) */
static unsigned long render_target_write_serial;

/* the pixels per unit of the bound targets (render_target_get) */
static float target_scale[2] = { 1.0f, 1.0f };

/* the pixel edge of a coordinate in a target's units, at its scale:
floorf's, without its call (on 32-bit x86 it saves and restores the FPU's
rounding, several times a draw) */
static GLint scaled_pixel(float coordinate, float scale)
{
	float value = coordinate * scale + 0.5f;
	GLint pixel = (GLint)value;

	/* (the conversion is toward zero: a negative value with a fraction
	rounds down one more) */
	if ((float)pixel > value)
		pixel--;
	return pixel;
}

/* ... in the bound targets' units */
static GLint target_pixel(float coordinate, int axis)
{
	return scaled_pixel(coordinate, target_scale[axis]);
}

/* the targets draws and clears go to, and the samples a pixel of their
multisampled storage (0: their textures); FALSE if there is nothing to draw
into */
static BOOL bind_targets(gpu_texture *color_target, gpu_texture *depth_target, uint32_t *target_storage,
	BOOL *has_depth)
{
	struct render_target_entry *color = render_target_get(device.render_target);
	struct render_target_entry *depth = render_target_get(device.depth_stencil);
	int samples;

	if (depth && !depth->target.depth)
		depth = NULL;
	if (!color && !depth)
		return FALSE;
	if (color)
	{
		color->last_rendered = device.frame + 1;
		color->target.written = ++render_target_write_serial;
	}
	/* viewports and clears are in the targets' units (render_target_get) */
	target_scale[0] = color ? color->target.scale[0] : depth->target.scale[0];
	target_scale[1] = color ? color->target.scale[1] : depth->target.scale[1];
	/* with multisampling, multisampled where either is a screen buffer or
	is multisampled already */
	samples = anti_aliasing() == _anti_aliasing_msaa ? anti_aliasing_samples : 0;
	if (samples && !((color && (color->screen_buffer || color->target.samples)) ||
		(depth && (depth->screen_buffer || depth->target.samples))))
	{
		samples = 0;
	}
	*color_target = color ? color->target.texture : 0;
	*depth_target = depth ? depth->target.texture : 0;
	*target_storage = (uint32_t)samples;
	if (color)
		color->target.samples = samples;
	if (depth)
		depth->target.samples = samples;
	target_samples = samples ? samples : 1;
	*has_depth = depth != NULL;
	return TRUE;
}

/* ---------- device creation */

static void gl_initialize(void)
{
	struct gpu_capabilities *capabilities = &xgpu_gpu_capabilities;
	int index;

	gpu_initialize(capabilities);
	shader_dialect.version = capabilities->shading_language_es ?
		(capabilities->shading_language >= 310 ? "310 es" : "300 es") : "450 core";
	shader_dialect.es = capabilities->shading_language_es != 0;
	shader_dialect.lookup_lod_bias = !capabilities->sampler_lod_bias;
	shader_dialect.clip_control = capabilities->shader_clip_control != 0;
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
		device.attributes[index][3] = 1.0f;
	memory_watch_initialize();
	debug_settings.skip_vertex_shaders = config_string("debug.gpu_skip_vertex_shaders");
	debug_settings.dump_shaders = *config_string("debug.gpu_dump_shaders") ?
		config_string("debug.gpu_dump_shaders") : NULL;
	debug_settings.statistics = config_boolean("debug.gpu_stats");
	xgpu_gl_state_invalidate();
	device.gl_ready = TRUE;
	if (anti_aliasing_value < 0)
		anti_aliasing_read();
	else
		anti_aliasing_prepare();
}

/* what display.anti_aliasing's value needs of the GL context, once there is
one: multisampling's samples, at most the GPU's, and the passes' programs,
built as the value is chosen rather than in the middle of a frame (SMAA's
are large) */
static void anti_aliasing_prepare(void)
{
	int mode;

	if (!device.gl_ready || anti_aliasing_value < 0)
		return;
	mode = anti_aliasing_values[anti_aliasing_value].mode;
	anti_aliasing_samples = anti_aliasing_values[anti_aliasing_value].samples;
	if (anti_aliasing_samples > (int)xgpu_gpu_capabilities.max_samples)
	{
		int maximum_samples = (int)xgpu_gpu_capabilities.max_samples;

		platform_log("anti-aliasing: the GPU has at most %d samples a pixel", maximum_samples);
		anti_aliasing_samples = maximum_samples < 2 ? 0 : maximum_samples;
	}
	if ((mode == _anti_aliasing_fxaa || mode == _anti_aliasing_smaa) &&
		!gpu_anti_alias_prepare(mode == _anti_aliasing_smaa ? GPU_ANTI_ALIAS_SMAA : GPU_ANTI_ALIAS_FXAA))
	{
		platform_log("anti-aliasing: its programs do not build, so the 3D view is not antialiased");
	}
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

/* the vertex constant registers, and when each last changed (gpu.h's
struct gpu_constant_store) */
static struct gpu_constant_store vertex_constants = { .checkpoint_first = GPU_CONSTANT_COUNT };

static void constants_store(unsigned long first, const void *data, unsigned long count)
{
	struct gpu_constant_store *store = &vertex_constants;
	const float (*values)[4] = data;
	unsigned long index;

	for (index = 0; index < count; index++)
	{
		if (memcmp(store->c[first + index], values[index], sizeof(store->c[0])))
		{
			memcpy(store->c[first + index], values[index], sizeof(store->c[0]));
			store->serials[first + index] = ++store->serial;
			store->log[store->serial % GPU_CONSTANT_LOG_SIZE] = (uint8_t)(first + index);
			if (store->checkpoint_first > first + index)
				store->checkpoint_first = (uint32_t)(first + index);
			if (store->checkpoint_last < first + index)
				store->checkpoint_last = (uint32_t)(first + index);
		}
	}
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
		constants_store(XGPU_VERTEX_CONSTANT_BIAS - 38, device.viewport_scale, 1);
		constants_store(XGPU_VERTEX_CONSTANT_BIAS - 37, device.viewport_offset, 1);
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
#ifdef HALO_ANDROID
		d3d8_surface_initialize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, width, height);
		d3d8_surface_initialize(&device.depth_buffer, D3DFMT_LIN_D24S8, width, height);
#else
		/* room for the widest screen, which F11 can switch to (the screen's
		width, above) */
		d3d8_surface_initialize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, SCREEN_MAXIMUM_WIDTH, height);
		d3d8_surface_initialize(&device.depth_buffer, D3DFMT_LIN_D24S8, SCREEN_MAXIMUM_WIDTH, height);
		d3d8_surface_resize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, width, height);
		d3d8_surface_resize(&device.depth_buffer, D3DFMT_LIN_D24S8, width, height);
#endif
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

		if (!config_boolean("debug.null_renderer") && platform_video_initialize(width, height))
			gl_initialize();
		else
			platform_log("Direct3D: running without a window (nothing is displayed)");
		device.created = TRUE;
	}
	*returned_device = device_pointer();
	return S_OK;
}

/* ---------- the menus' pointer */

#ifdef HALO_ANDROID
int halo_ui_pointer_update(int menus_active, struct halo_ui_pointer *pointer)
{
	(void)pointer;
	platform_menus_set_active(menus_active != 0);
	return 0;
}
#else
/* a point in the window, as SDL reports it, in the menus' coordinates: the
inverse of the letterboxed display blit at presentation, the screen's
width and the menus' centering (halo_screen_ui_offset) */
static void ui_point_from_window(float window_x, float window_y, short *x, short *y)
{
	struct render_target_entry *back_buffer = render_target_get(&device.back_buffer);
	int window_width, window_height, pixel_width, pixel_height, width, height, left, top;
	float screen_x, screen_y;

	*x = *y = -1;
	if (!back_buffer)
		return;
	platform_video_window_size(&window_width, &window_height);
	platform_video_drawable_size(&pixel_width, &pixel_height);
	if (window_width <= 0 || window_height <= 0)
		return;
	width = pixel_width;
	height = (int)((long)pixel_width * back_buffer->target.gl_height / back_buffer->target.gl_width);
	if (height > pixel_height)
	{
		height = pixel_height;
		width = (int)((long)pixel_height * back_buffer->target.gl_width / back_buffer->target.gl_height);
	}
	left = (pixel_width - width) / 2;
	top = (pixel_height - height) / 2;
	screen_x = (window_x * pixel_width / window_width - left) * (float)back_buffer->target.width / (float)width;
	screen_y = (window_y * pixel_height / window_height - top) * (float)back_buffer->target.height / (float)height;
	*x = (short)floorf(screen_x - (float)(halo_screen_width() - 640) / 2.0f);
	*y = (short)floorf(screen_y);
}

int halo_ui_pointer_update(int menus_active, struct halo_ui_pointer *pointer)
{
	struct platform_ui_pointer state;

	platform_menus_set_active(menus_active != 0);
	platform_ui_pointer_set_active(menus_active != 0);
	if (!menus_active || !device.gl_ready || !platform_ui_pointer_read(&state))
		return 0;
	memset(pointer, 0, sizeof(*pointer));
	ui_point_from_window(state.x, state.y, &pointer->x, &pointer->y);
	ui_point_from_window(state.click_x, state.click_y, &pointer->click_x, &pointer->click_y);
	pointer->moved = state.moved != FALSE;
	pointer->left_clicks = (unsigned char)(state.left_clicks < 255 ? state.left_clicks : 255);
	pointer->right_clicks = (unsigned char)(state.right_clicks < 255 ? state.right_clicks : 255);
	pointer->wheel_steps = (signed char)(state.wheel_steps < -8 ? -8 : state.wheel_steps > 8 ? 8 : state.wheel_steps);
	return 1;
}
#endif

/* takes up the display's shape and resolution, or the window's, if they
have changed; between frames, since the game's layout and the targets must
agree for a whole frame. Returns the width the game draws. */
long halo_screen_commit(void)
{
	long width;
	float scale[2];

	/* (display.anti_aliasing, as Settings or config.toml has it now:
	supersampling changes the scale below) */
	anti_aliasing_read();
	/* display.shadow_resolution, if it has changed: the maps' entries at
	the old scale stay (render_target_get), but are no longer found */
	if (shadow_scale && shadow_scale_read_at != config_changes())
	{
		long shadow = shadow_scale_choose();

		shadow_scale_read_at = config_changes();
		if (shadow != shadow_scale)
		{
			platform_log("shadow maps: %ldx%ld", SHADOW_MAP_SIZE * shadow, SHADOW_MAP_SIZE * shadow);
			shadow_scale = shadow;
			memset(recent_render_targets, 0, sizeof(recent_render_targets));
		}
	}
	if (!screen_width)
		return halo_screen_width();
	screen_mode_choose(&width, scale);
	if (width != screen_width || scale[0] != screen_scale[0] || scale[1] != screen_scale[1])
	{
		platform_log("screen: %ldx%d drawn at %.0fx%.0f", width, SCREEN_HEIGHT,
			width * scale[0], SCREEN_HEIGHT * scale[1]);
		screen_width = width;
		screen_scale[0] = scale[0];
		screen_scale[1] = scale[1];
#ifndef HALO_ANDROID
		if (device.created)
		{
			device.presentation.BackBufferWidth = (UINT)width;
			d3d8_surface_resize(&device.back_buffer, D3DFMT_LIN_A8R8G8B8, (unsigned long)width, SCREEN_HEIGHT);
			d3d8_surface_resize(&device.depth_buffer, D3DFMT_LIN_D24S8, (unsigned long)width, SCREEN_HEIGHT);
		}
#endif
	}
	return screen_width;
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
		gpu_flush();
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
	device.visibility_test_active = TRUE;
	gpu_visibility_begin();
}

HRESULT WINAPI D3DDevice_EndVisibilityTest(DWORD index)
{
	if (!device.gl_ready || !device.visibility_test_active)
		return S_OK;
	device.visibility_test_active = FALSE;
	index %= VISIBILITY_TEST_SLOTS;
	if (!index)
		index = 1;
	gpu_visibility_end((uint32_t)index);
	/* the target's samples to a game pixel (its pixels, by its samples a
	pixel with multisampling): an exact count is turned into one of the
	game's pixels (visibility_unscaled), which the game divides by its own
	test's area (lens flares, rasterizer_lights.c), a split-screen window's
	or the screen's alike */
	device.query_area[index] = target_scale[0] * target_scale[1] * (float)target_samples;
	device.query_pending[index] = TRUE;
	return S_OK;
}

/* a count of pixels in the game's pixels */
static uint32_t visibility_unscaled(uint32_t samples, DWORD index)
{
	float area = device.query_area[index];

	return area > 1.0f ? (uint32_t)(samples / area + 0.5f) : samples;
}

HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD index, UINT *result, ULONGLONG *time_stamp)
{
	uint32_t samples = 0;

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
	/* the latest count known: from this test, or while the GPU is still
	behind, from the slot's earlier ones */
	if (gpu_visibility_result((uint32_t)index, &samples))
	{
		switch (xgpu_gpu_capabilities.occlusion)
		{
		case GPU_OCCLUSION_EXACT:
			samples = visibility_unscaled(samples, index);
			break;
		case GPU_OCCLUSION_ANY_SAMPLE:
			/* ES's queries only say whether any sample passed. The game
			divides the count by the test's area (lens flare brightness,
			rasterizer_lights.c): report more than any test covers, well
			below what would overflow there. */
			if (samples)
				samples = VISIBILITY_ALL_SAMPLES;
			break;
		default:
			break;
		}
		device.visibility_known[index] = samples;
	}
	if (result)
		*result = device.visibility_known[index];
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

/* the game names its model lighting programs as it creates them
(rasterizer_xbox_vertex_shaders_initialize.c), so that their draws can be lit
for each pixel (display.per_pixel_lighting); one whose lighting is not as
the pixel shader computes it stays lit for each vertex */
void halo_vertex_shader_lighting(unsigned long handle)
{
	struct vertex_shader_object *object = vertex_shader_from_handle((DWORD)handle);

	if (!object || !object->instructions)
		return;
	if (!nv2a_vertex_shader_lighting(object->instructions, object->instruction_count, &object->lighting))
	{
		memset(&object->lighting, 0, sizeof(object->lighting));
		platform_log("GPU: vertex shader %lu is not lit as the pixel shader would light it: lit for each vertex",
			object->id);
	}
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
	constants_store((unsigned long)first, constant_data, constant_count);
}

/* the program that runs: the one loaded at the selected address, else the
current shader's own */
static struct vertex_shader_object *current_program(void)
{
	struct vertex_shader_object *program = device.program_slots[device.program_address];

	return program ? program : device.vertex_shader;
}

/* ---------- program cache */

/* size is a multiple of 4 */
static unsigned long hash_words(const void *data, unsigned long size)
{
	const DWORD *words = data;
	unsigned long hash = 2166136261UL;

	for (size /= 4; size; size--)
		hash = (hash ^ *words++) * 16777619UL;
	return hash;
}

/* lit: the shader that hands the lighting's normal and position on
(vertex_shader_object lit_shader) */
static gpu_shader vertex_shader_get(struct vertex_shader_object *program, BOOL immediate, BOOL lit)
{
	int variant = immediate ? 1 : 0;
	gpu_shader *shader = lit ? &program->lit_shader[variant] : &program->shader[variant];

	if (!*shader)
	{
		char *source = nv2a_vertex_shader_to_glsl(&shader_dialect, program->instructions, program->instruction_count,
			immediate ? 0 : device.vertex_shader->packed_mask, lit ? &program->lighting : NULL);

		*shader = gpu_shader_create(GPU_SHADER_VERTEX, source);
		if (debug_settings.dump_shaders)
		{
			char path[512];
			FILE *file;

			snprintf(path, sizeof(path), "%s/vs%03lu_%d%s.glsl", debug_settings.dump_shaders, program->id, variant,
				lit ? "_lit" : "");
			if ((file = fopen(path, "w")) != NULL)
			{
				fputs(source, file);
				fclose(file);
			}
		}
		free(source);
	}
	return *shader;
}

typedef char pixel_shader_key_size_assert[sizeof(struct nv2a_pixel_shader_key) % 4 == 0 ? 1 : -1];

static gpu_shader fragment_shader_get(const struct nv2a_pixel_shader_key *key)
{
	/* consecutive draws mostly use one of a few pixel shaders (an object's
	parts take turns) */
#define RECENT_FRAGMENT_COUNT 4
	static struct fragment_entry *recent[RECENT_FRAGMENT_COUNT];
	static unsigned long recent_next;
	unsigned long hash, index;
	struct fragment_entry **bucket;
	struct fragment_entry *entry;
	char *source;

	for (index = 0; index < RECENT_FRAGMENT_COUNT; index++)
	{
		if (recent[index] && !memcmp(&recent[index]->key, key, sizeof(*key)))
			return recent[index]->shader;
	}
	hash = hash_words(key, sizeof(*key));
	bucket = &fragment_buckets[hash % FRAGMENT_BUCKETS];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash && !memcmp(&entry->key, key, sizeof(*key)))
		{
			recent[recent_next++ % RECENT_FRAGMENT_COUNT] = entry;
			return entry->shader;
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->hash = hash;
	entry->key = *key;
	source = nv2a_pixel_shader_to_glsl(&shader_dialect, key);
	entry->shader = gpu_shader_create(GPU_SHADER_PIXEL, source);
	if (debug_settings.dump_shaders)
	{
		char path[512];
		FILE *file;

		snprintf(path, sizeof(path), "%s/ps_%08lx.glsl", debug_settings.dump_shaders, hash);
		if ((file = fopen(path, "w")) != NULL)
		{
			fputs(source, file);
			fclose(file);
		}
	}
	free(source);
	entry->next = *bucket;
	*bucket = entry;
	recent[recent_next++ % RECENT_FRAGMENT_COUNT] = entry;
	return entry->shader;
}

/* ---------- per-draw state */

static unsigned long stage_texture_mode(int stage)
{
	return (D3D__RenderState[D3DRS_PSTEXTUREMODES] >> (5 * stage)) & 0x1f;
}

/* a D3DTSS_*FILTER value as gpu.h's (any other is linear, as the Xbox's
filters but POINT are in GL) */
static unsigned char sampler_filter(DWORD filter)
{
	return filter <= D3DTEXF_GAUSSIANCUBIC ? (unsigned char)filter : GPU_FILTER_LINEAR;
}

static unsigned char sampler_address(DWORD mode)
{
	switch (mode)
	{
	case D3DTADDRESS_MIRROR: return GPU_ADDRESS_MIRROR;
	case D3DTADDRESS_CLAMP: return GPU_ADDRESS_CLAMP;
	case D3DTADDRESS_BORDER: return GPU_ADDRESS_BORDER;
	case D3DTADDRESS_CLAMPTOEDGE: return GPU_ADDRESS_CLAMP_TO_EDGE;
	default: return GPU_ADDRESS_WRAP;
	}
}

/* a stage's sampler state. hires: a high-res HUD texture (hud_hires.h),
drawn smaller than it is, so filtered and from its mip levels whatever the
game asks: the HUD's meters are point sampled for one player, to keep the
Xbox bitmaps' texels sharp */
static void sampler_state(int stage, BOOL mipmapped, BOOL hires, struct gpu_sampler_state *sampler)
{
	DWORD *state = D3D__TextureState[stage];

	memset(sampler, 0, sizeof(*sampler));
	sampler->min_filter = hires ? GPU_FILTER_LINEAR : sampler_filter(state[D3DTSS_MINFILTER]);
	sampler->mip_filter = hires ? GPU_FILTER_LINEAR : mipmapped ? sampler_filter(state[D3DTSS_MIPFILTER]) :
		GPU_FILTER_NONE;
	sampler->mag_filter = hires ? GPU_FILTER_LINEAR : sampler_filter(state[D3DTSS_MAGFILTER]);
	sampler->address_u = sampler_address(state[D3DTSS_ADDRESSU]);
	sampler->address_v = sampler_address(state[D3DTSS_ADDRESSV]);
	sampler->address_w = sampler_address(state[D3DTSS_ADDRESSW]);
	sampler->lod_bias = hires ? 0.0f : dword_to_float(state[D3DTSS_MIPMAPLODBIAS]);
	sampler->max_mip_level = hires ? 0 : (uint32_t)state[D3DTSS_MAXMIPLEVEL];
	sampler->max_anisotropy = (uint32_t)state[D3DTSS_MAXANISOTROPY];
	sampler->border_color = (uint32_t)state[D3DTSS_BORDERCOLOR];
}

/* ---------- render targets sampled with their mip chain

The game renders some textures one mip level at a time, each level being a
surface of its own (the water's ripple map). Sampling such a texture needs
every level in one GL texture, so the levels' render targets are copied into
a mipmapped composite. Each draw of the water binds it, some maps (a30) more
than once a frame, so the copy (and the mipmaps of the levels the game did not
render) is redone only once a level's target has been drawn into since the
last one. */

#define MIP_COMPOSITE_LEVELS 16

struct mip_composite
{
	struct mip_composite *next;
	unsigned long data, width, height, levels;
	gpu_texture texture;
	/* the levels last copied, and each one's target's texture and written
	serial then */
	unsigned long rendered_levels;
	gpu_texture level_sources[MIP_COMPOSITE_LEVELS];
	unsigned long level_written[MIP_COMPOSITE_LEVELS];
};

static struct mip_composite *mip_composites;

static gpu_texture mip_composite_get(const struct xgpu_texture_description *description, unsigned long data)
{
	struct mip_composite *composite;
	struct xgpu_render_target *targets[MIP_COMPOSITE_LEVELS];
	unsigned long level, rendered_levels = 0;
	BOOL changed;

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
		{
			struct gpu_texture_description levels;

			memset(&levels, 0, sizeof(levels));
			levels.type = GPU_TEXTURE_2D;
			levels.format = GPU_FORMAT_BGRA8;
			levels.usage = GPU_USAGE_RENDER_TARGET;
			levels.width = (uint32_t)description->width;
			levels.height = (uint32_t)description->height;
			levels.depth = 1;
			levels.levels = (uint32_t)description->levels;
			composite->texture = gpu_texture_create(&levels);
		}
		composite->rendered_levels = ~0UL;
		composite->next = mip_composites;
		mip_composites = composite;
	}
	for (level = 0; level < description->levels && level < MIP_COMPOSITE_LEVELS; level++)
	{
		unsigned long width = description->width >> level ? description->width >> level : 1;
		unsigned long height = description->height >> level ? description->height >> level : 1;
		struct xgpu_render_target *target =
			xgpu_render_target_find(data + xgpu_texture_level_offset(description, level));

		if (!target || target->width != width || target->height != height ||
			target->gl_width != width || target->gl_height != height)
			break;
		targets[level] = target;
		rendered_levels++;
	}
	changed = rendered_levels != composite->rendered_levels;
	for (level = 0; level < rendered_levels && !changed; level++)
	{
		changed = targets[level]->texture != composite->level_sources[level] ||
			targets[level]->written != composite->level_written[level];
	}
	if (!changed)
		return composite->texture;
	composite->rendered_levels = rendered_levels;
	for (level = 0; level < rendered_levels; level++)
	{
		struct xgpu_render_target *target = targets[level];

		composite->level_sources[level] = target->texture;
		composite->level_written[level] = target->written;
		gpu_texture_copy_level(target->texture, composite->texture, (uint32_t)level);
	}
	/* levels the game did not render come from the ones it did */
	if (rendered_levels < description->levels)
		gpu_texture_generate_mipmaps(composite->texture, rendered_levels ? (uint32_t)rendered_levels - 1 : 0);
	return composite->texture;
}

/* the draw's texture stages */
static void bind_textures(struct nv2a_pixel_shader_key *key, float texture_scale[4][4], struct gpu_stage *stages)
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = device.textures[stage];
		unsigned long mode = stage_texture_mode(stage);

		texture_scale[stage][0] = texture_scale[stage][1] = 1.0f;
		texture_scale[stage][2] = texture_scale[stage][3] = 1.0f;
		if (!texture || !texture->Data || mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x11)
		{
			key->sampler_type[stage] = mode == 0x11 ? _xgpu_sampler_2d : _xgpu_sampler_none;
			continue;
		}
		{
			struct xgpu_render_target *target = xgpu_render_target_find(texture->Data);
			struct xgpu_texture_description description;
			unsigned char type;
			gpu_texture handle;

			if (target)
			{
				xgpu_texture_describe(texture->Format, texture->Size, &description);
				handle = target->texture;
				type = GPU_TEXTURE_2D;
				if (description.linear)
				{
					texture_scale[stage][0] = 1.0f / (float)target->width;
					texture_scale[stage][1] = 1.0f / (float)target->height;
				}
				if (!description.linear && !description.cube_map && description.levels > 1 &&
					target->width == description.width && target->height == description.height)
					handle = mip_composite_get(&description, texture->Data);
				else
					description.levels = 1;
			}
			else
			{
				const D3DCOLOR *palette = device.palettes[stage] && device.palettes[stage]->Data ?
					(const D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(device.palettes[stage]->Data) : NULL;

				handle = xgpu_texture_get((const DWORD *)texture, palette, &type, &description);
				if (description.linear)
				{
					texture_scale[stage][0] = 1.0f / (float)description.width;
					texture_scale[stage][1] = 1.0f / (float)description.height;
				}
			}
			stages[stage].texture = handle;
			stages[stage].type = type;
			sampler_state(stage, description.levels > 1, description.hires, &stages[stage].sampler);
			if (stage == 0)
			{
				key->coverage_alpha = description.hires_coverage != FALSE;
				key->point_threshold = description.hires_point_threshold != FALSE;
			}
			key->sampler_type[stage] = type == GPU_TEXTURE_CUBE ? _xgpu_sampler_cube :
				type == GPU_TEXTURE_3D ? _xgpu_sampler_3d : _xgpu_sampler_2d;
		}
	}
}

static unsigned char compare_function(DWORD function)
{
	/* (0, unset: never) */
	return function >= D3DCMP_NEVER && function <= D3DCMP_ALWAYS ? (unsigned char)(function - D3DCMP_NEVER) :
		GPU_COMPARE_NEVER;
}

static unsigned char stencil_operation(DWORD operation)
{
	switch (operation)
	{
	case D3DSTENCILOP_ZERO: return GPU_STENCIL_ZERO;
	case D3DSTENCILOP_REPLACE: return GPU_STENCIL_REPLACE;
	case D3DSTENCILOP_INCRSAT: return GPU_STENCIL_INCREMENT_CLAMP;
	case D3DSTENCILOP_DECRSAT: return GPU_STENCIL_DECREMENT_CLAMP;
	case D3DSTENCILOP_INVERT: return GPU_STENCIL_INVERT;
	case D3DSTENCILOP_INCR: return GPU_STENCIL_INCREMENT_WRAP;
	case D3DSTENCILOP_DECR: return GPU_STENCIL_DECREMENT_WRAP;
	default: return GPU_STENCIL_KEEP;
	}
}

static unsigned char blend_factor(DWORD factor)
{
	switch (factor)
	{
	case D3DBLEND_ZERO: return GPU_BLEND_ZERO;
	case D3DBLEND_SRCCOLOR: return GPU_BLEND_SOURCE_COLOR;
	case D3DBLEND_INVSRCCOLOR: return GPU_BLEND_ONE_MINUS_SOURCE_COLOR;
	case D3DBLEND_SRCALPHA: return GPU_BLEND_SOURCE_ALPHA;
	case D3DBLEND_INVSRCALPHA: return GPU_BLEND_ONE_MINUS_SOURCE_ALPHA;
	case D3DBLEND_DESTALPHA: return GPU_BLEND_DESTINATION_ALPHA;
	case D3DBLEND_INVDESTALPHA: return GPU_BLEND_ONE_MINUS_DESTINATION_ALPHA;
	case D3DBLEND_DESTCOLOR: return GPU_BLEND_DESTINATION_COLOR;
	case D3DBLEND_INVDESTCOLOR: return GPU_BLEND_ONE_MINUS_DESTINATION_COLOR;
	case D3DBLEND_SRCALPHASAT: return GPU_BLEND_SOURCE_ALPHA_SATURATE;
	case D3DBLEND_CONSTANTCOLOR: return GPU_BLEND_CONSTANT_COLOR;
	case D3DBLEND_INVCONSTANTCOLOR: return GPU_BLEND_ONE_MINUS_CONSTANT_COLOR;
	case D3DBLEND_CONSTANTALPHA: return GPU_BLEND_CONSTANT_ALPHA;
	case D3DBLEND_INVCONSTANTALPHA: return GPU_BLEND_ONE_MINUS_CONSTANT_ALPHA;
	default: return GPU_BLEND_ONE;
	}
}

static unsigned char blend_operation(DWORD operation)
{
	switch (operation)
	{
	case D3DBLENDOP_SUBTRACT: return GPU_BLEND_OP_SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT:
	case D3DBLENDOP_REVSUBTRACTSIGNED: return GPU_BLEND_OP_REVERSE_SUBTRACT;
	case D3DBLENDOP_MIN: return GPU_BLEND_OP_MIN;
	case D3DBLENDOP_MAX: return GPU_BLEND_OP_MAX;
	default: return GPU_BLEND_OP_ADD;
	}
}

/* the draw's viewport, depth, stencil, blending and rasterization */
static void raster_state(struct gpu_draw *draw, BOOL has_depth)
{
	DWORD *rs = D3D__RenderState;
	DWORD write = rs[D3DRS_COLORWRITEENABLE];
	struct gpu_depth_stencil_state *depth_stencil = &draw->depth_stencil;
	struct gpu_blend_state *blend = &draw->blend;
	struct gpu_raster_state *raster = &draw->raster;
	struct gpu_rect *viewport = &draw->viewport.rect;

	viewport->x = target_pixel((float)device.viewport.X, 0);
	viewport->y = target_pixel((float)device.viewport.Y, 1);
	viewport->width = target_pixel((float)(device.viewport.X + device.viewport.Width), 0) - viewport->x;
	viewport->height = target_pixel((float)(device.viewport.Y + device.viewport.Height), 1) - viewport->y;
	/* the game never issues a scissor rectangle, and the NV2A scissor register
	defaults to the viewport, so fragment clipping follows the viewport: this is
	what keeps a split-screen window's geometry from bleeding across the divider */
	draw->scissor = *viewport;
	draw->viewport.min_z = device.viewport.MinZ;
	draw->viewport.max_z = device.viewport.MaxZ;

	depth_stencil->depth_test = has_depth && rs[D3DRS_ZENABLE];
	depth_stencil->depth_function = compare_function(rs[D3DRS_ZFUNC]);
	depth_stencil->depth_write = rs[D3DRS_ZWRITEENABLE] != 0;
	depth_stencil->stencil_test = has_depth && rs[D3DRS_STENCILENABLE];
	depth_stencil->stencil_function = compare_function(rs[D3DRS_STENCILFUNC]);
	depth_stencil->stencil_reference = (uint32_t)rs[D3DRS_STENCILREF];
	depth_stencil->stencil_read_mask = (uint32_t)rs[D3DRS_STENCILMASK];
	depth_stencil->stencil_write_mask = (uint32_t)rs[D3DRS_STENCILWRITEMASK];
	depth_stencil->stencil_fail = stencil_operation(rs[D3DRS_STENCILFAIL]);
	depth_stencil->stencil_depth_fail = stencil_operation(rs[D3DRS_STENCILZFAIL]);
	depth_stencil->stencil_pass = stencil_operation(rs[D3DRS_STENCILPASS]);

	blend->enable = rs[D3DRS_ALPHABLENDENABLE] != 0;
	blend->source = blend_factor(rs[D3DRS_SRCBLEND]);
	blend->destination = blend_factor(rs[D3DRS_DESTBLEND]);
	blend->operation = blend_operation(rs[D3DRS_BLENDOP]);
	blend->color = (uint32_t)rs[D3DRS_BLENDCOLOR];
	blend->color_write_mask = (uint8_t)(((write & D3DCOLORWRITEENABLE_RED) ? 1 : 0) |
		((write & D3DCOLORWRITEENABLE_GREEN) ? 2 : 0) | ((write & D3DCOLORWRITEENABLE_BLUE) ? 4 : 0) |
		((write & D3DCOLORWRITEENABLE_ALPHA) ? 8 : 0));

	/* the cull mode names the winding to discard; FRONTFACE names the
	front winding */
	raster->cull_mode = rs[D3DRS_CULLMODE] == D3DCULL_NONE ? GPU_CULL_NONE :
		rs[D3DRS_CULLMODE] == rs[D3DRS_FRONTFACE] ? GPU_CULL_FRONT : GPU_CULL_BACK;
	raster->front_face = rs[D3DRS_FRONTFACE] == D3DFRONT_CCW ? GPU_FRONT_COUNTER_CLOCKWISE : GPU_FRONT_CLOCKWISE;
	raster->fill_mode = rs[D3DRS_FILLMODE] == D3DFILL_WIREFRAME ? GPU_FILL_LINE :
		rs[D3DRS_FILLMODE] == D3DFILL_POINT ? GPU_FILL_POINT : GPU_FILL_SOLID;
	/* D3DRS_ZBIAS is expressed in these states (D3DDevice_SetRenderState_ZBias) */
	raster->depth_bias = rs[D3DRS_SOLIDOFFSETENABLE] != 0;
	raster->depth_bias_slope = dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]);
	raster->depth_bias_constant = dword_to_float(rs[D3DRS_POLYGONOFFSETZOFFSET]);
}

/* the uniforms of the latest draws, converted from these inputs; the serial
counts the conversions */
#define DRAW_UNIFORM_INPUT_COUNT (4 + 4 + 16 + 1 + 16 + 2 + 4 + 1 + 1 + 7 * D3DTSS_MAXSTAGES)

static DWORD draw_uniform_inputs[DRAW_UNIFORM_INPUT_COUNT];
static struct gpu_uniforms draw_uniforms;

/* display.per_pixel_lighting: the model lighting programs' draws are lit for
each pixel (nv2a_psh.c model_lighting), from shaders of their own; read
again when a setting changes */
static BOOL per_pixel_lighting(void)
{
	static unsigned long read_at = (unsigned long)-1;
	static BOOL enabled;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		enabled = config_boolean("display.per_pixel_lighting") != 0;
	}
	return enabled;
}

/* the draw prepare_draw made, for submit_draw: its program, whether it is
lit for each pixel, and its pixel shader's key */
static struct
{
	struct vertex_shader_object *program;
	BOOL immediate;
	BOOL lit;
	struct nv2a_pixel_shader_key key;
} draw_shaders;

/* the draw's targets, state, shaders, texture stages and uniforms: all of it
but its vertices; FALSE if it is not drawn */
static BOOL prepare_draw(struct gpu_draw *draw, BOOL immediate)
{
	struct vertex_shader_object *program = current_program();
	struct nv2a_pixel_shader_key *key = &draw_shaders.key;
	float texture_scale[4][4];
	BOOL has_depth = FALSE;
	int stage;

	if (!device.gl_ready || !program || !device.vertex_shader || !program->instructions)
	{
		stats.skipped_no_program++;
		return FALSE;
	}
	{
		const char *skip = debug_settings.skip_vertex_shaders;

		while (skip && *skip)
		{
			if ((unsigned long)atol(skip) == program->id)
				return FALSE;
			skip = strchr(skip, ',');
			if (skip)
				skip++;
		}
	}
	memset(draw, 0, sizeof(*draw));
	memset(key, 0, sizeof(*key));
	memcpy(key->combiner_state, D3D__RenderState, sizeof(key->combiner_state));
	/* constants are uniforms, not part of the program */
	memset(&key->combiner_state[D3DRS_PSCONSTANT0_0], 0, 16 * sizeof(DWORD));
	key->combiner_state[D3DRS_PSFINALCOMBINERCONSTANT0] = 0;
	key->combiner_state[D3DRS_PSFINALCOMBINERCONSTANT1] = 0;
	key->texture_modes = D3D__RenderState[D3DRS_PSTEXTUREMODES];
	bind_textures(key, texture_scale, draw->stages);
	if (!bind_targets(&draw->color_target, &draw->depth_target, &draw->samples, &has_depth))
	{
		stats.skipped_no_target++;
		return FALSE;
	}
	raster_state(draw, has_depth);
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		key->alpha_kill[stage] = D3D__TextureState[stage][D3DTSS_ALPHAKILL] == D3DTALPHAKILL_ENABLE;
		key->color_sign[stage] = (unsigned char)((D3D__TextureState[stage][D3DTSS_COLORSIGN] >> 28) & 0xf);
	}
	/* (only with the meter's blend: hud_hires.h, nv2a_pixel_shader_key) */
	key->coverage_alpha = key->coverage_alpha && D3D__RenderState[D3DRS_ALPHABLENDENABLE] &&
		D3D__RenderState[D3DRS_SRCBLEND] == D3DBLEND_CONSTANTCOLOR &&
		D3D__RenderState[D3DRS_DESTBLEND] == D3DBLEND_SRCALPHA;
	key->point_threshold = key->point_threshold && key->coverage_alpha;
	key->alpha_test_function = D3D__RenderState[D3DRS_ALPHATESTENABLE] ? D3D__RenderState[D3DRS_ALPHAFUNC] : 0;
	/* (gl_SampleMask: ES has it only from 3.2) */
	if (xgpu_gpu_capabilities.sample_mask && target_samples > 1 && key->alpha_test_function &&
		!D3D__RenderState[D3DRS_ALPHABLENDENABLE])
	{
		key->alpha_test_samples = (unsigned char)target_samples;
	}
	key->fog_enable = D3D__RenderState[D3DRS_FOGENABLE] != 0;
	key->fog_table_mode = (unsigned char)D3D__RenderState[D3DRS_FOGTABLEMODE];
	key->count_samples = device.visibility_test_active &&
		xgpu_gpu_capabilities.occlusion == GPU_OCCLUSION_SHADER_COUNTER;

	draw_shaders.program = program;
	draw_shaders.immediate = immediate;
	draw_shaders.lit = program->lighting.lights && !program->lighting_failed && per_pixel_lighting();
	if (draw_shaders.lit)
		key->per_pixel_lighting = (unsigned char)program->lighting.lights;
	draw->vertex_shader = vertex_shader_get(program, immediate, draw_shaders.lit);
	draw->pixel_shader = fragment_shader_get(key);

	/* the state the other uniforms come from: most draws share it with the
	draw before them, and so share its uniforms */
	{
		DWORD inputs[DRAW_UNIFORM_INPUT_COUNT];
		unsigned long count = 0;

		memcpy(&inputs[count], device.viewport_scale, sizeof(device.viewport_scale));
		count += 4;
		memcpy(&inputs[count], device.viewport_offset, sizeof(device.viewport_offset));
		count += 4;
		memcpy(&inputs[count], texture_scale, sizeof(texture_scale));
		count += 16;
		inputs[count++] = D3D__RenderState[D3DRS_POINTSIZE];
		for (stage = 0; stage < 8; stage++)
		{
			inputs[count++] = D3D__RenderState[D3DRS_PSCONSTANT0_0 + stage];
			inputs[count++] = D3D__RenderState[D3DRS_PSCONSTANT1_0 + stage];
		}
		inputs[count++] = D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0];
		inputs[count++] = D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1];
		inputs[count++] = D3D__RenderState[D3DRS_FOGCOLOR];
		inputs[count++] = D3D__RenderState[D3DRS_FOGSTART];
		inputs[count++] = D3D__RenderState[D3DRS_FOGEND];
		inputs[count++] = D3D__RenderState[D3DRS_FOGDENSITY];
		inputs[count++] = D3D__RenderState[D3DRS_ALPHAREF];
		inputs[count++] = (DWORD)UI_OFFSET;
		for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
		{
			DWORD *state = D3D__TextureState[stage];

			inputs[count++] = state[D3DTSS_BUMPENVMAT00];
			inputs[count++] = state[D3DTSS_BUMPENVMAT01];
			inputs[count++] = state[D3DTSS_BUMPENVMAT10];
			inputs[count++] = state[D3DTSS_BUMPENVMAT11];
			inputs[count++] = state[D3DTSS_BUMPENVLSCALE];
			inputs[count++] = state[D3DTSS_BUMPENVLOFFSET];
			inputs[count++] = state[D3DTSS_MIPMAPLODBIAS];
		}
		if (!draw_uniforms.serial || memcmp(inputs, draw_uniform_inputs, sizeof(inputs)))
		{
			struct gpu_uniforms *converted = &draw_uniforms;

			memcpy(draw_uniform_inputs, inputs, sizeof(inputs));
			converted->serial++;
			memcpy(converted->viewport_scale, device.viewport_scale, sizeof(converted->viewport_scale));
			memcpy(converted->viewport_offset, device.viewport_offset, sizeof(converted->viewport_offset));
			memcpy(converted->texture_scale, texture_scale, sizeof(converted->texture_scale));
			converted->point_size = D3D__RenderState[D3DRS_POINTSIZE] ?
				dword_to_float(D3D__RenderState[D3DRS_POINTSIZE]) : 1.0f;
			for (stage = 0; stage < 8; stage++)
			{
				color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0 + stage], converted->ps_c0[stage]);
				color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0 + stage], converted->ps_c1[stage]);
			}
			color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0], converted->ps_final_c0);
			color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1], converted->ps_final_c1);
			color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR], converted->fog_color);
			converted->fog_parameters[0] = dword_to_float(D3D__RenderState[D3DRS_FOGSTART]);
			converted->fog_parameters[1] = dword_to_float(D3D__RenderState[D3DRS_FOGEND]);
			converted->fog_parameters[2] = dword_to_float(D3D__RenderState[D3DRS_FOGDENSITY]);
			converted->fog_parameters[3] = 0.0f;
			converted->alpha_reference = (float)(D3D__RenderState[D3DRS_ALPHAREF] & 0xff);
			for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
			{
				DWORD *state = D3D__TextureState[stage];

				converted->bump_matrix[stage][0] = dword_to_float(state[D3DTSS_BUMPENVMAT00]);
				converted->bump_matrix[stage][1] = dword_to_float(state[D3DTSS_BUMPENVMAT01]);
				converted->bump_matrix[stage][2] = dword_to_float(state[D3DTSS_BUMPENVMAT10]);
				converted->bump_matrix[stage][3] = dword_to_float(state[D3DTSS_BUMPENVMAT11]);
				converted->bump_luminance[stage][0] = dword_to_float(state[D3DTSS_BUMPENVLSCALE]);
				converted->bump_luminance[stage][1] = dword_to_float(state[D3DTSS_BUMPENVLOFFSET]);
				converted->bump_luminance[stage][2] = converted->bump_luminance[stage][3] = 0.0f;
				converted->texture_lod_bias[stage] = dword_to_float(state[D3DTSS_MIPMAPLODBIAS]);
			}
			converted->screen_offset = (float)UI_OFFSET;
		}
	}
	return TRUE;
}

/* draws what prepare_draw and the vertex setup made; a draw whose shaders
lit for each pixel do not link is drawn lit as the vertex shader lights it
(and not at all, were that to fail too) */
static void submit_draw(struct gpu_draw *draw)
{
	BOOL drawn;

	drawn = gpu_draw(draw, &vertex_constants, &draw_uniforms) != 0;
	if (!drawn && draw_shaders.lit)
	{
		platform_log("GPU: vertex shader %lu cannot be lit for each pixel here (refer to the shader log above): "
			"lit for each vertex", draw_shaders.program->id);
		draw_shaders.program->lighting_failed = TRUE;
		draw_shaders.key.per_pixel_lighting = 0;
		draw->vertex_shader = vertex_shader_get(draw_shaders.program, draw_shaders.immediate, FALSE);
		draw->pixel_shader = fragment_shader_get(&draw_shaders.key);
		drawn = gpu_draw(draw, &vertex_constants, &draw_uniforms) != 0;
	}
	if (!drawn)
		stats.skipped_link++;
	else if (draw_shaders.immediate)
		stats.immediate_draws++;
	else
		stats.draws++;
}

/* ---------- tracing (debug.gpu_trace_frame) */

static BOOL trace_frame(void)
{
	static long frame = -2;

	if (frame == -2)
		frame = config_integer("debug.gpu_trace_frame");
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
	if (config_boolean("debug.gpu_trace_constants"))
	{
		int constant;

		for (constant = 0; constant < XGPU_VERTEX_CONSTANT_COUNT; constant++)
		{
			const float *value = vertex_constants.c[constant];

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


/* ---------- the contiguous window in GPU buffers

Vertex and index buffers live in the Xbox's contiguous memory, where most
never change once loaded. The mirror keeps a copy of that memory in GL
buffers (one per segment, created when first needed) and uploads a page
only when it is first drawn from or after the game has written it: pages
are write-protected once uploaded, as cached textures are (memory_watch.c).
Pages the game rewrites frame after frame (dynamic vertices) would fault on
every write; after a few such rewrites a page counts as volatile for a
while, and draws that use it stream their data as before. */

#define MIRROR_SEGMENT_SIZE 0x400000UL
#define MIRROR_SEGMENT_COUNT (PLATFORM_CONTIGUOUS_SIZE / MIRROR_SEGMENT_SIZE)
#define MIRROR_PAGE_SIZE 0x1000UL
#define MIRROR_PAGE_COUNT (PLATFORM_CONTIGUOUS_SIZE / MIRROR_PAGE_SIZE)
/* rewrites no more than this many frames apart ... */
#define MIRROR_REWRITE_FRAMES 2
/* ... this many times in a row make a page volatile ... */
#define MIRROR_VOLATILE_REWRITES 4
/* ... for this many frames */
#define MIRROR_VOLATILE_FRAMES 600

enum
{
	_mirror_page_absent,
	_mirror_page_present,
	_mirror_page_volatile
};

static struct
{
	gpu_buffer buffers[MIRROR_SEGMENT_COUNT];
	unsigned char state[MIRROR_PAGE_COUNT];
	unsigned char rewrites[MIRROR_PAGE_COUNT];
	/* the page's memory_watch generation when it was uploaded */
	unsigned long generation[MIRROR_PAGE_COUNT];
	unsigned long rewritten_frame[MIRROR_PAGE_COUNT];
} mirror;

/* uploads the pages of [first, last) that are absent or stale; FALSE if one
of them turns out to be volatile */
static BOOL mirror_refresh(unsigned long first, unsigned long last)
{
	unsigned long page, run;
	BOOL volatile_page = FALSE;
	unsigned char stale[256];
	unsigned long count = last - first;

	if (count > sizeof(stale))
	{
		/* a range this long is refreshed in pieces */
		for (page = first; page < last; page += sizeof(stale))
		{
			if (!mirror_refresh(page, page + sizeof(stale) < last ? page + sizeof(stale) : last))
				return FALSE;
		}
		return TRUE;
	}
	for (page = first; page < last; page++)
	{
		BOOL written = mirror.state[page] == _mirror_page_present &&
			memory_watch_generation(PLATFORM_CONTIGUOUS_BASE + page * MIRROR_PAGE_SIZE, MIRROR_PAGE_SIZE) >
			mirror.generation[page];

		stale[page - first] = mirror.state[page] != _mirror_page_present || written;
		if (written)
		{
			if (device.frame - mirror.rewritten_frame[page] <= MIRROR_REWRITE_FRAMES)
				mirror.rewrites[page]++;
			else
				mirror.rewrites[page] = 1;
			mirror.rewritten_frame[page] = device.frame;
			if (mirror.rewrites[page] >= MIRROR_VOLATILE_REWRITES)
			{
				mirror.state[page] = _mirror_page_volatile;
				volatile_page = TRUE;
			}
		}
	}
	if (volatile_page)
		return FALSE;
	for (page = first; page < last; page = run)
	{
		unsigned long segment = page * MIRROR_PAGE_SIZE / MIRROR_SEGMENT_SIZE;
		unsigned long address, size;
		/* no queued draw can read pages uploaded for the first time */
		BOOL unused = TRUE;

		if (!stale[page - first])
		{
			run = page + 1;
			continue;
		}
		for (run = page; run < last && stale[run - first]; run++)
		{
			if (mirror.state[run] != _mirror_page_present)
			{
				mirror.rewrites[run] = 0;
				mirror.rewritten_frame[run] = device.frame;
			}
			else
			{
				unused = FALSE;
			}
		}
		address = PLATFORM_CONTIGUOUS_BASE + page * MIRROR_PAGE_SIZE;
		size = (run - page) * MIRROR_PAGE_SIZE;
		/* protect first, so a write racing with the upload is noticed */
		memory_watch_protect(address, size);
		for (; page < run; page++)
		{
			mirror.generation[page] = memory_watch_generation(PLATFORM_CONTIGUOUS_BASE + page * MIRROR_PAGE_SIZE,
				MIRROR_PAGE_SIZE);
			mirror.state[page] = _mirror_page_present;
		}
		if (!mirror.buffers[segment])
			mirror.buffers[segment] = gpu_buffer_create(MIRROR_SEGMENT_SIZE);
		gpu_buffer_write(mirror.buffers[segment],
			(uint32_t)(address - PLATFORM_CONTIGUOUS_BASE - segment * MIRROR_SEGMENT_SIZE), (uint32_t)size,
			(const void *)address, unused ? GPU_WRITE_UNUSED : 0);
	}
	return TRUE;
}

/* makes [address, address + size) current in the mirror, giving the buffer
that holds it, the range's offset in that buffer and the newest upload
generation of its pages (which changes whenever its contents do); FALSE if
the range is outside the window, spans two segments or is volatile */
static BOOL mirror_range(unsigned long address, unsigned long size, gpu_buffer *buffer, unsigned long *offset,
	unsigned long *generation)
{
	unsigned long start = address - PLATFORM_CONTIGUOUS_BASE;
	unsigned long segment, first, last, page, oldest = ~0UL, newest = 0;
	BOOL present = TRUE;

	if (!size || address < PLATFORM_CONTIGUOUS_BASE || start + size > PLATFORM_CONTIGUOUS_SIZE)
		return FALSE;
	segment = start / MIRROR_SEGMENT_SIZE;
	if ((start + size - 1) / MIRROR_SEGMENT_SIZE != segment)
		return FALSE;
	first = start / MIRROR_PAGE_SIZE;
	last = (start + size - 1) / MIRROR_PAGE_SIZE + 1;
	for (page = first; page < last; page++)
	{
		if (mirror.state[page] == _mirror_page_volatile)
		{
			if (device.frame - mirror.rewritten_frame[page] < MIRROR_VOLATILE_FRAMES)
				return FALSE;
			mirror.state[page] = _mirror_page_absent;
		}
		if (mirror.state[page] != _mirror_page_present)
		{
			present = FALSE;
		}
		else
		{
			if (mirror.generation[page] < oldest)
				oldest = mirror.generation[page];
			if (mirror.generation[page] > newest)
				newest = mirror.generation[page];
		}
	}
	if (!present || memory_watch_generation(address, size) > oldest)
	{
		if (!mirror_refresh(first, last))
			return FALSE;
		for (newest = 0, page = first; page < last; page++)
		{
			if (mirror.generation[page] > newest)
				newest = mirror.generation[page];
		}
	}
	*buffer = mirror.buffers[segment];
	*offset = start - segment * MIRROR_SEGMENT_SIZE;
	if (generation)
		*generation = newest;
	stats.mirrored_bytes += size;
	return TRUE;
}

/* the smallest and largest index of an index range the mirror holds: the
same ranges are drawn frame after frame */
#define INDEX_RANGE_SLOTS 4096

static struct
{
	unsigned long address;
	unsigned long count;
	unsigned long generation;
	WORD minimum;
	WORD maximum;
} index_ranges[INDEX_RANGE_SLOTS];

static void index_extent(const WORD *indices, unsigned long count, unsigned long generation, BOOL cached,
	unsigned long *minimum, unsigned long *maximum)
{
	unsigned long slot = (((unsigned long)indices >> 1) ^ (count * 2654435761UL)) % INDEX_RANGE_SLOTS;
	unsigned long index, low = 0xffff, high = 0;

	if (cached && index_ranges[slot].address == (unsigned long)indices && index_ranges[slot].count == count &&
		index_ranges[slot].generation == generation)
	{
		*minimum = index_ranges[slot].minimum;
		*maximum = index_ranges[slot].maximum;
		return;
	}
	for (index = 0; index < count; index++)
	{
		if (indices[index] < low)
			low = indices[index];
		if (indices[index] > high)
			high = indices[index];
	}
	if (cached)
	{
		index_ranges[slot].address = (unsigned long)indices;
		index_ranges[slot].count = count;
		index_ranges[slot].generation = generation;
		index_ranges[slot].minimum = (WORD)low;
		index_ranges[slot].maximum = (WORD)high;
	}
	*minimum = low;
	*maximum = high;
}

/* ---------- vertex data */

/* streams the vertices, with the D3DCOLOR elements of the stream turned from
BGRA into the RGBA byte order a GPU without vertex_bgra reads (OpenGL ES) */
static uint32_t stream_upload(const struct vertex_shader_object *declaration, unsigned long stream,
	const unsigned char *data, unsigned long size, unsigned long stride, gpu_buffer *buffer)
{
	static unsigned char *scratch;
	static unsigned long scratch_size;
	unsigned long offsets[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long count = 0, index, vertex;

	if (!xgpu_gpu_capabilities.vertex_bgra)
	{
		for (index = 0; index < declaration->element_count; index++)
		{
			const struct vertex_element *element = &declaration->elements[index];

			if (element->stream == stream && element->type == D3DVSDT_D3DCOLOR)
				offsets[count++] = element->offset;
		}
	}
	if (!count || !stride)
		return gpu_stream(GPU_STREAM_VERTEX, data, (uint32_t)size, buffer);
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
	return gpu_stream(GPU_STREAM_VERTEX, scratch, (uint32_t)size, buffer);
}

static unsigned char attribute_format(const struct vertex_element *element)
{
	switch (element->type)
	{
	case D3DVSDT_FLOAT1: return GPU_ATTRIBUTE_FLOAT1;
	case D3DVSDT_FLOAT2: return GPU_ATTRIBUTE_FLOAT2;
	case D3DVSDT_FLOAT3: case D3DVSDT_FLOAT2H: return GPU_ATTRIBUTE_FLOAT3;
	case D3DVSDT_FLOAT4: return GPU_ATTRIBUTE_FLOAT4;
	/* (without BGRA attributes, stream_upload swaps the bytes) */
	case D3DVSDT_D3DCOLOR: return xgpu_gpu_capabilities.vertex_bgra ? GPU_ATTRIBUTE_BGRA8 : GPU_ATTRIBUTE_RGBA8;
	case D3DVSDT_SHORT1: return GPU_ATTRIBUTE_SHORT1;
	case D3DVSDT_SHORT2: return GPU_ATTRIBUTE_SHORT2;
	case D3DVSDT_SHORT3: return GPU_ATTRIBUTE_SHORT3;
	case D3DVSDT_SHORT4: return GPU_ATTRIBUTE_SHORT4;
	case D3DVSDT_NORMSHORT1: return GPU_ATTRIBUTE_NORMSHORT1;
	case D3DVSDT_NORMSHORT2: return GPU_ATTRIBUTE_NORMSHORT2;
	case D3DVSDT_NORMSHORT3: return GPU_ATTRIBUTE_NORMSHORT3;
	case D3DVSDT_NORMSHORT4: return GPU_ATTRIBUTE_NORMSHORT4;
	case D3DVSDT_NORMPACKED3: return GPU_ATTRIBUTE_NORMPACKED3;
	case D3DVSDT_PBYTE1: return GPU_ATTRIBUTE_UBYTE1;
	case D3DVSDT_PBYTE2: return GPU_ATTRIBUTE_UBYTE2;
	case D3DVSDT_PBYTE3: return GPU_ATTRIBUTE_UBYTE3;
	case D3DVSDT_PBYTE4: return GPU_ATTRIBUTE_UBYTE4;
	default: return GPU_ATTRIBUTE_FLOAT4;
	}
}

/* a stream with colors is uploaded with its bytes swapped where the GPU
reads no BGRA attributes (stream_upload), and cannot come from the mirror */
static BOOL stream_has_colors(const struct vertex_shader_object *declaration, unsigned long stream)
{
	unsigned long index;

	if (xgpu_gpu_capabilities.vertex_bgra)
		return FALSE;
	for (index = 0; index < declaration->element_count; index++)
	{
		if (declaration->elements[index].stream == stream && declaration->elements[index].type == D3DVSDT_D3DCOLOR)
			return TRUE;
	}
	return FALSE;
}

/* the draw's vertices [first, first + count) of every stream the
declaration uses, the attributes pointed at them (attribute data then
starts at vertex 0 of the range), and the value of each attribute that
reads none */
static void setup_streams(struct gpu_draw *draw, unsigned long first, unsigned long count)
{
	struct vertex_shader_object *declaration = device.vertex_shader;
	BOOL placed[16] = { FALSE };
	unsigned long index, total = 0;

	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
		draw->attributes[index].stream = GPU_STREAM_ZERO;
	/* the mirror first; then one reservation for everything streamed */
	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];
		unsigned long stream = element->stream;
		unsigned long stride = device.streams[stream].stride;
		unsigned long bytes = stride ? stride * count : 64;
		unsigned long base, offset;

		if (!device.streams[stream].data || element->type == D3DVSDT_NONE || placed[stream])
			continue;
		placed[stream] = TRUE;
		draw->streams[stream].buffer = 0;
		draw->streams[stream].stride = (uint32_t)stride;
		base = (unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data) + first * stride;
		if (!stream_has_colors(declaration, stream) &&
			mirror_range(base, bytes, &draw->streams[stream].buffer, &offset, NULL))
		{
			draw->streams[stream].offset = (uint32_t)offset;
			continue;
		}
		draw->streams[stream].buffer = 0;
		total += (bytes + 15) & ~15UL;
	}
	gpu_stream_reserve((uint32_t)total);
	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];
		unsigned long stream = element->stream;
		unsigned long stride = device.streams[stream].stride;
		struct gpu_vertex_attribute *attribute = &draw->attributes[element->reg];

		if (!device.streams[stream].data || element->type == D3DVSDT_NONE)
			continue;
		if (!draw->streams[stream].buffer)
		{
			const unsigned char *base = PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data);
			unsigned long bytes = stride ? stride * count : 64;

			draw->streams[stream].offset = stream_upload(declaration, stream, base + first * stride, bytes, stride,
				&draw->streams[stream].buffer);
			stats.streamed_bytes += bytes;
		}
		attribute->format = attribute_format(element);
		attribute->stream = (uint8_t)stream;
		attribute->offset = element->offset;
	}
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		if (draw->attributes[index].stream != GPU_STREAM_ZERO || (declaration->packed_mask & (1UL << index)))
			continue;
		draw->attributes[index].stream = GPU_STREAM_CONSTANT;
		memcpy(draw->constant_values[index], device.attributes[index], sizeof(draw->constant_values[index]));
	}
}

static uint32_t primitive_mode(D3DPRIMITIVETYPE type)
{
	switch (type)
	{
	case D3DPT_POINTLIST: return GPU_PRIMITIVE_POINTS;
	case D3DPT_LINELIST: return GPU_PRIMITIVE_LINES;
	case D3DPT_LINELOOP: return GPU_PRIMITIVE_LINE_LOOP;
	case D3DPT_LINESTRIP: return GPU_PRIMITIVE_LINE_STRIP;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_QUADSTRIP: return GPU_PRIMITIVE_TRIANGLE_STRIP;
	case D3DPT_TRIANGLEFAN:
	case D3DPT_POLYGON: return GPU_PRIMITIVE_TRIANGLE_FAN;
	default: return GPU_PRIMITIVE_TRIANGLES;
	}
}

/* the draw's vertices by count indices, streamed */
static void draw_indices(struct gpu_draw *draw, const WORD *indices, unsigned long count)
{
	draw->index_offset = gpu_stream(GPU_STREAM_INDEX, indices, (uint32_t)(count * sizeof(WORD)), &draw->index_buffer);
	draw->count = (uint32_t)count;
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
	device.base_vertex_index = base_vertex_index;
	D3D__IndexData = index_data ? (WORD *)index_data->Data : NULL;
}

void WINAPI D3DDevice_DrawVertices(D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT vertex_count)
{
	struct gpu_draw draw;

	if (!vertex_count || !prepare_draw(&draw, FALSE))
		return;
	trace_draw("draw", primitive_type, vertex_count, NULL);
	setup_streams(&draw, start_vertex, vertex_count);
	if (primitive_type == D3DPT_QUADLIST)
	{
		unsigned long count;
		WORD *indices = quad_indices(NULL, vertex_count, &count);

		draw.primitive = GPU_PRIMITIVE_TRIANGLES;
		draw_indices(&draw, indices, count);
		free(indices);
	}
	else
	{
		draw.primitive = primitive_mode(primitive_type);
		draw.count = vertex_count;
	}
	submit_draw(&draw);
}

void WINAPI D3DDevice_DrawIndexedVertices(D3DPRIMITIVETYPE primitive_type, UINT vertex_count, CONST WORD *index_data)
{
	unsigned long minimum, maximum, index, count, generation = 0, index_offset = 0;
	WORD *indices = NULL;
	const WORD *source = index_data;
	gpu_buffer index_buffer = 0;
	struct gpu_draw draw;
	BOOL mirrored;

	if (!vertex_count || !index_data || !prepare_draw(&draw, FALSE))
		return;
	/* quads are drawn as triangles, from indices made for the draw */
	mirrored = primitive_type != D3DPT_QUADLIST && xgpu_gpu_capabilities.base_vertex &&
		mirror_range((unsigned long)index_data, vertex_count * sizeof(WORD), &index_buffer, &index_offset, &generation);
	index_extent(index_data, vertex_count, generation, mirrored, &minimum, &maximum);
	trace_draw("indexed", primitive_type, vertex_count, NULL);
	/* (the streams from the base vertex on: index i is vertex base + i) */
	setup_streams(&draw, device.base_vertex_index + minimum, maximum - minimum + 1);
	draw.primitive = primitive_mode(primitive_type);
	if (mirrored)
	{
		/* the attributes start at vertex minimum */
		draw.index_buffer = index_buffer;
		draw.index_offset = (uint32_t)index_offset;
		draw.count = vertex_count;
		draw.base_vertex = -(int32_t)minimum;
		submit_draw(&draw);
		return;
	}
	stats.streamed_bytes += vertex_count * sizeof(WORD);
	count = vertex_count;
	if (primitive_type == D3DPT_QUADLIST)
	{
		indices = quad_indices(index_data, vertex_count, &count);
		source = indices;
		draw.primitive = GPU_PRIMITIVE_TRIANGLES;
	}
	if (!xgpu_gpu_capabilities.base_vertex)
	{
		/* the indices are copied anyway: rebase them */
		WORD *rebased = malloc(count * sizeof(WORD) + 2);

		for (index = 0; index < count; index++)
			rebased[index] = (WORD)(source[index] - minimum);
		draw_indices(&draw, rebased, count);
		free(rebased);
	}
	else
	{
		draw_indices(&draw, source, count);
		draw.base_vertex = -(int32_t)minimum;
	}
	free(indices);
	submit_draw(&draw);
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
	unsigned long index, count = device.immediate_count;
	D3DPRIMITIVETYPE type = device.immediate_type;
	struct gpu_draw draw;

	device.immediate_active = FALSE;
	if (!count || !prepare_draw(&draw, TRUE))
		return;
	trace_draw("immediate", type, count, device.immediate_vertices);
	/* every attribute four floats, one after another */
	draw.streams[0].offset = gpu_stream(GPU_STREAM_VERTEX, device.immediate_vertices, (uint32_t)(count * stride),
		&draw.streams[0].buffer);
	draw.streams[0].stride = (uint32_t)stride;
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		draw.attributes[index].format = GPU_ATTRIBUTE_FLOAT4;
		draw.attributes[index].stream = 0;
		draw.attributes[index].offset = (uint16_t)(index * 4 * sizeof(float));
	}
	if (type == D3DPT_QUADLIST)
	{
		unsigned long index_count;
		WORD *indices = quad_indices(NULL, count, &index_count);

		draw.primitive = GPU_PRIMITIVE_TRIANGLES;
		draw_indices(&draw, indices, index_count);
		free(indices);
	}
	else
	{
		draw.primitive = primitive_mode(type);
		draw.count = (uint32_t)count;
	}
	submit_draw(&draw);
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
	static struct gpu_rect *pixels;
	static unsigned long pixels_capacity;
	struct gpu_clear clear;
	unsigned long pixel_count = 0;
	BOOL has_depth = FALSE;
	DWORD index;

	memset(&clear, 0, sizeof(clear));
	if (!device.gl_ready || !bind_targets(&clear.color_target, &clear.depth_target, &clear.samples, &has_depth))
		return;
	if (trace_frame())
		platform_log("clear flags %lx color %08lx z %g count %lu target %08lx depth %08lx", (unsigned long)flags,
			(unsigned long)color, z, (unsigned long)count,
			device.render_target ? (unsigned long)device.render_target->Data : 0,
			device.depth_stencil ? (unsigned long)device.depth_stencil->Data : 0);
	stats.clears++;
	if (flags & D3DCLEAR_TARGET)
	{
		/* the Xbox clears the channels named (D3DCLEAR_TARGET_R, _G, _B, _A):
		the fog screen clears only alpha, leaving the picture under the fog */
		clear.flags |= GPU_CLEAR_COLOR;
		clear.channel_mask = (uint8_t)(((flags & D3DCLEAR_TARGET_R) ? 1 : 0) | ((flags & D3DCLEAR_TARGET_G) ? 2 : 0) |
			((flags & D3DCLEAR_TARGET_B) ? 4 : 0) | ((flags & D3DCLEAR_TARGET_A) ? 8 : 0));
		clear.color = (uint32_t)color;
	}
	if (has_depth && (flags & D3DCLEAR_ZBUFFER))
	{
		clear.flags |= GPU_CLEAR_DEPTH;
		clear.depth = z;
	}
	if (has_depth && (flags & D3DCLEAR_STENCIL))
	{
		clear.flags |= GPU_CLEAR_STENCIL;
		clear.stencil = (uint32_t)stencil;
	}
	if (!clear.flags)
	{
		/* (the targets are bound all the same) */
		gpu_clear(&clear, NULL, 0);
		return;
	}
	if (pixels_capacity < (count ? count : 1))
	{
		free(pixels);
		pixels_capacity = count > 16 ? count : 16;
		pixels = malloc(pixels_capacity * sizeof(*pixels));
		if (!pixels)
		{
			pixels_capacity = 0;
			return;
		}
	}
	if (!count || !rectangles)
	{
		/* the NV2A clips a viewport-less clear to the viewport, which is what
		keeps a split-screen window's clear from wiping the other window */
		pixels[0].x = target_pixel((float)device.viewport.X, 0);
		pixels[0].y = target_pixel((float)device.viewport.Y, 1);
		pixels[0].width = target_pixel((float)(device.viewport.X + device.viewport.Width), 0) - pixels[0].x;
		pixels[0].height = target_pixel((float)(device.viewport.Y + device.viewport.Height), 1) - pixels[0].y;
		gpu_clear(&clear, pixels, 1);
		return;
	}
	for (index = 0; index < count; index++)
	{
		INT left = rectangles[index].x1 > device.viewport.X ? rectangles[index].x1 : device.viewport.X;
		INT top = rectangles[index].y1 > device.viewport.Y ? rectangles[index].y1 : device.viewport.Y;
		INT right = rectangles[index].x2 < device.viewport.X + device.viewport.Width ?
			rectangles[index].x2 : device.viewport.X + device.viewport.Width;
		INT bottom = rectangles[index].y2 < device.viewport.Y + device.viewport.Height ?
			rectangles[index].y2 : device.viewport.Y + device.viewport.Height;
		struct gpu_rect *pixel = &pixels[pixel_count];

		if (left >= right || top >= bottom)
			continue;
		pixel->x = target_pixel((float)(left + UI_OFFSET), 0);
		pixel->y = target_pixel((float)top, 1);
		pixel->width = target_pixel((float)(right + UI_OFFSET), 0) - pixel->x;
		pixel->height = target_pixel((float)bottom, 1) - pixel->y;
		pixel_count++;
	}
	gpu_clear(&clear, pixels, (uint32_t)pixel_count);
}

/* ---------- the anti-aliasing passes */

/* display.anti_aliasing's pass over a window's 3D view, before the HUD and
menus are drawn over it (source/render/render.c); the window's bounds in the
game's units of the screen */
void halo_screen_anti_alias(short x0, short y0, short x1, short y1)
{
	struct render_target_entry *target;
	int32_t corners[4];
	int mode = anti_aliasing();

	if (!device.gl_ready || (mode != _anti_aliasing_fxaa && mode != _anti_aliasing_smaa))
		return;
	/* (the primary target's view: the back buffer's) */
	target = render_target_get(&device.back_buffer);
	if (!target)
		return;
	/* (no longer multisampled, if it was before the setting changed) */
	target->target.samples = 0;
	corners[0] = scaled_pixel(x0, target->target.scale[0]);
	corners[1] = scaled_pixel(y0, target->target.scale[1]);
	corners[2] = scaled_pixel(x1, target->target.scale[0]);
	corners[3] = scaled_pixel(y1, target->target.scale[1]);
	gpu_anti_alias(mode == _anti_aliasing_smaa ? GPU_ANTI_ALIAS_SMAA : GPU_ANTI_ALIAS_FXAA, target->target.texture,
		corners);
}

/* ---------- presentation */

static void write_screenshot(struct render_target_entry *target)
{
	const char *directory = *config_string("debug.screenshot_directory") ?
		config_string("debug.screenshot_directory") : NULL;
	unsigned long width = target->target.gl_width, height = target->target.gl_height;
	unsigned char *pixels;
	char path[512];
	FILE *file;
	unsigned long row;
	unsigned char header[54] = { 'B', 'M' };
	unsigned long image_size = width * height * 4;

	if (!directory)
		return;
	pixels = malloc(image_size);
	if (!pixels || !gpu_texture_read(target->target.texture, pixels, (uint32_t)image_size))
	{
		free(pixels);
		return;
	}
	/* the display ignores destination alpha, which the game uses as scratch;
	image viewers would show it as transparency */
	for (row = 0; row < width * height; row++)
		pixels[row * 4 + 3] = 0xff;
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
		screenshot_every = config_integer("debug.screenshot_every");

	if (device.gl_ready)
	{
		struct render_target_entry *back_buffer = render_target_get(&device.back_buffer);

		if (trace_frame())
			platform_log("present back buffer %08lx texture %u", (unsigned long)device.back_buffer.Data,
				back_buffer->target.texture);
		if (screenshot_every > 0 && device.frame % (unsigned long)screenshot_every == 0)
			write_screenshot(back_buffer);
		gpu_present(back_buffer->target.texture);
		xgpu_texture_cache_begin_frame();
	}
	device.frame++;
	stats.presents++;
	if (debug_settings.statistics && device.frame % 60 == 0)
	{
		platform_log("frame %lu: %lu draws, %lu immediate, %lu clears, %lu target changes; skipped %lu no program, %lu no target, %lu link; "
			"%lu KB mirrored, %lu KB streamed",
			device.frame, stats.draws / stats.presents, stats.immediate_draws / stats.presents, stats.clears / stats.presents,
			stats.target_changes / stats.presents, stats.skipped_no_program, stats.skipped_no_target, stats.skipped_link,
			stats.mirrored_bytes / stats.presents / 1024, stats.streamed_bytes / stats.presents / 1024);
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
