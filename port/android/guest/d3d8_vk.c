/*
D3D8_VK.C

The Xbox Direct3D 8 device for the Android build's Vulkan renderer.
Linked into halo_guest_vk.elf in place of
port/linux/src/d3d8_gl.c, which every other build and the Android GL ES image
keep unchanged.

It began as d3d8_gl.c with every OpenGL call and every host_gl_* import taken
out, keeping what d3d8_gl.c does that is not OpenGL: the screen's width and
scale, the state the XDK's inline functions keep, the vertical blank and its
thread and callbacks, the reserved viewport constants, render and texture
stage state, transforms, vertex shaders and their declarations, streams,
immediate mode, the surfaces and render targets as the game sees them. What
d3d8_gl.c draws with OpenGL is sent instead, a frame at a time, as records of
a command stream (vk_commands.h) to the host's Vulkan backend
(port/android/host/host_vk*.c). Changes to d3d8_gl.c are not followed
automatically.
*/

#include "xgpu.h"
#include "sdl_platform.h"
#include "halo_ui_pointer.h"
#include "port_config.h"
#include "vk_commands.h"
#include "vk_shaders.h"
#include "vk_device.h"
#include "hud_hires.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void d3d8_surface_initialize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);
void d3d8_surface_resize(D3DSurface *surface, D3DFORMAT format, unsigned long width, unsigned long height);

/* what the context supports: xbox_textures.c reads it (as d3d8_gl.c
defines it on Android); nothing here is OpenGL, so nothing is supported */
struct xgpu_capabilities xgpu_capabilities;

/* ---------- the screen's width

The Xbox screen is 640x480. The native ports can draw a wider one: 480
lines, and as many columns as the display's shape gives. On Android that is
display.screen_width (port_config.c; 640 keeps 4:3); on the desktop, the
display's shape while the game is fullscreen, and 640 in a window. The
game's camera derives its horizontal field of view from the viewport, so the
3D view simply widens. The menus and full-screen overlays are laid out for
640 columns; while they draw (halo_screen_ui_offset), everything shifts right
to center them.

Fullscreen on the desktop also draws at the display's resolution: render
targets the size of the screen get that many pixels (screen_scale), and
viewports, clears and visibility counts are scaled to match, so the game
still works in its 480 lines. The width and the scale change only between
frames, after one is presented (halo_screen_commit). */

#define SCREEN_HEIGHT 480
#define SCREEN_MAXIMUM_WIDTH 1920

/* the width the game draws, 0 until first asked, and how many pixels a
render target the size of the screen has per unit of it */
static long screen_width;
static float screen_scale[2] = { 1.0f, 1.0f };
static long ui_offset;
#define UI_OFFSET ((GLint)ui_offset)

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

void halo_screen_ui_offset(unsigned char centered)
{
	ui_offset = centered ? (halo_screen_width() - 640) / 2 : 0;
}

/* display.shadow_resolution: the size the shadow maps are drawn at, as in
d3d8_gl.c. Each object's shadow is drawn from above into a 128x128 map,
blurred into another and projected onto the level under it
(rasterizer_xbox_shadows.c). The two maps, the game's only R5G6B5 render
targets, are drawn larger as the screen's targets are (target_scale_of): the
game's viewports, clears and quads, in its 128 units, scale up with them. The
scale is a power of two up to 8 (1024x1024), which the blur needs to cover
the same part of the map as the Xbox's (rasterizer_shadow_convolve); 1, the
default, draws them as the Xbox did. It changes only between frames
(halo_screen_commit). */
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

/* display.shadow_resolution, if it has changed (halo_screen_commit, between
frames): the maps are then described to the host at the new size, which
makes them anew */
static void shadow_scale_commit(void)
{
	long scale;

	if (!shadow_scale || shadow_scale_read_at == config_changes())
		return;
	shadow_scale_read_at = config_changes();
	scale = shadow_scale_choose();
	if (scale != shadow_scale)
	{
		platform_log("shadow maps: %ldx%ld", SHADOW_MAP_SIZE * scale, SHADOW_MAP_SIZE * scale);
		shadow_scale = scale;
	}
}


/* ---------- anti-aliasing

display.anti_aliasing, off unless it is set, as d3d8_gl.c reads it on Android: "fxaa" (and "smaa", which Android draws
as FXAA) is a pass over each window's 3D view before the HUD and menus are drawn over it (halo_screen_anti_alias, the
host's host_vk_post.c); "msaa2x" to "msaa8x" draw the screen's targets, and any target drawn together with one of them,
with that many samples a pixel, or as many as the device has (targets_bind; the host's multisampled images, which it
resolves); "ssaa2x" is off. The setting is read again between frames (halo_screen_commit), so that a change applies from
the next one. */

enum
{
	_anti_aliasing_off,
	_anti_aliasing_fxaa,
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
	{ "smaa", _anti_aliasing_fxaa, 0 },
	{ "ssaa2x", _anti_aliasing_off, 0 },
	{ "msaa2x", _anti_aliasing_msaa, 2 },
	{ "msaa4x", _anti_aliasing_msaa, 4 },
	{ "msaa8x", _anti_aliasing_msaa, 8 },
};

#define NUMBER_OF_ANTI_ALIASING_VALUES ((int)(sizeof(anti_aliasing_values) / sizeof(anti_aliasing_values[0])))

uint32_t host_vk_samples_supported(uint32_t samples);

/* the value in effect, -1 until the setting is first read, and multisampling's samples a pixel, at most the device's (1
when it has no more) */
static int anti_aliasing_value = -1;
static unsigned long anti_aliasing_samples = 1;

static void multisampled_forget(void);

/* display.anti_aliasing's value (none of them: the first, off) */
static void anti_aliasing_read(void)
{
	const char *setting = config_string("display.anti_aliasing");
	int value;

	for (value = NUMBER_OF_ANTI_ALIASING_VALUES - 1; value > 0 && strcmp(setting, anti_aliasing_values[value].name); value--)
	{
	}
	if (value == anti_aliasing_value)
		return;
	anti_aliasing_value = value;
	anti_aliasing_samples = 1;
	multisampled_forget();
	if (anti_aliasing_values[value].mode == _anti_aliasing_msaa)
	{
		anti_aliasing_samples = host_vk_samples_supported((uint32_t)anti_aliasing_values[value].samples);
		if (anti_aliasing_samples < 2)
			platform_log("anti-aliasing: %s, which this device cannot draw: off", setting);
		else
			platform_log("anti-aliasing: %s, drawn with %lu samples a pixel", setting, anti_aliasing_samples);
	}
	else if (strcmp(setting, anti_aliasing_values[value].name))
	{
		platform_log("anti-aliasing: \"%s\" is unknown, so off", setting);
	}
	else
	{
		platform_log("anti-aliasing: %s%s", setting, anti_aliasing_values[value].mode == _anti_aliasing_fxaa &&
			strcmp(setting, "fxaa") ? " (FXAA here)" : anti_aliasing_values[value].mode == _anti_aliasing_off &&
			strcmp(setting, "off") ? " (off here)" : "");
	}
}

static int anti_aliasing(void)
{
	if (anti_aliasing_value < 0)
		anti_aliasing_read();
	return anti_aliasing_values[anti_aliasing_value].mode;
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
	/* FNV-1a 64 of the instruction count and words (vk_shaders.h): what the shader cache knows the program by */
	uint64_t program_hash;
	/* one of the game's model lighting programs (halo_vertex_shader_lighting), whose draws can be lit for each pixel
	(display.per_pixel_lighting): where its lighting's normal and position are (lighting.lights is 0 for the others) */
	struct nv2a_vertex_lighting lighting;
	/* a shader lit for each pixel failed to compile: lit as the vertex shader lights it from then on */
	BOOL lighting_failed;
};

/* ---------- the device */

struct vk_device
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
	/* SetIndices' base vertex (d3d8_gl.c) */
	UINT base_vertex_index;

	/* the current value of each input register (SetVertexData) */
	float attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	BOOL immediate_active;
	D3DPRIMITIVETYPE immediate_type;
	float *immediate_vertices;
	unsigned long immediate_count;
	unsigned long immediate_capacity;

	BOOL visibility_test_active;
	/* the platform layer made a window (not debug.null_renderer): there is something to draw into */
	BOOL video_ready;

	unsigned long frame;
	unsigned long next_vertex_shader_id;
	BOOL created;
};

static struct vk_device device;

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
/* ---------- render targets

The textures and render targets the OpenGL renderer keeps are the host
backend's to keep here. Until it does, there are none. */

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

/* ---------- the command stream (vk_commands.h)

Commands are written here over a frame and handed to the host half
(port/android/host/host_vk*.c) at its end, or sooner if the stream fills. The
host reads them during the call and nothing later. */

void host_vk_submit(unsigned int commands, unsigned int size);
/* the shader service (host_vk_shaders.c; the imports pass a 64-bit hash in one register) */
void host_vk_shader_compile(uint32_t stage, uint64_t hash, uint32_t glsl, uint32_t glsl_size);
uint32_t host_vk_shader_find(uint32_t stage, uint64_t hash, uint32_t status_out);

#define STREAM_SIZE (4 * 1024 * 1024)

static uint32_t stream[STREAM_SIZE / 4];
static unsigned long stream_used;

static void stream_flush(void)
{
	if (stream_used)
		host_vk_submit((unsigned int)(uintptr_t)stream, (unsigned int)stream_used);
	stream_used = 0;
}

/* room for a command of size bytes, its header filled in (vk_device.h) */
void *vk_stream_command(uint32_t type, unsigned long size)
{
	struct vk_command_header *header;

	size = (size + 3) & ~3UL;
	if (stream_used + size > STREAM_SIZE)
		stream_flush();
	header = (struct vk_command_header *)((unsigned char *)stream + stream_used);
	header->type = type;
	header->size = (uint32_t)size;
	stream_used += size;
	return header;
}

#define stream_command vk_stream_command

/* The data a draw reads is copied here, into the stream, when the draw is made, and
nowhere later: the game rewrites its buffers between draws of a frame. Returns the
frame's running number of the data (from 1; reset at Present), which commands name,
with an offset, to read it. Data that does not fit in what is left of the stream is
split into parts, across hand-overs: the host places them contiguously. */
static uint32_t data_id;

/* what is left in the stream for the payload of a data record, a multiple of 4 */
static unsigned long data_room(void)
{
	unsigned long left = STREAM_SIZE - stream_used;

	return left > sizeof(struct vk_command_data) ? (left - sizeof(struct vk_command_data)) & ~3UL : 0;
}

uint32_t vk_data_put(const void *bytes, unsigned long size)
{
	const unsigned char *at = bytes;
	unsigned long done = 0;
	uint32_t id = ++data_id;

	do
	{
		unsigned long part = size - done, room = data_room();
		struct vk_command_data *command;

		/* a part of a few bytes is not worth a record at the end of the stream */
		if (room < 64 && room < part)
		{
			stream_flush();
			room = data_room();
		}
		if (part > room)
			part = room;
		command = stream_command(VK_COMMAND_DATA, sizeof(*command) + part);
		command->id = id;
		command->part_offset = (uint32_t)done;
		command->total_size = (uint32_t)size;
		command->part_size = (uint32_t)part;
		if (part)
			memcpy(command->payload, at + done, part);
		if (part & 3)
			memset((unsigned char *)command->payload + part, 0, 4 - (part & 3));
		done += part;
	} while (done < size);
	return id;
}

/* the pixels per unit of the bound targets: what the screen's targets are drawn at,
as render_target_get in d3d8_gl.c works it out (a target the size of the screen gets
the screen's scale), and what clears are scaled by (target_pixel there) */
static float target_scale[2] = { 1.0f, 1.0f };

/* the game's only R5G6B5 targets, 128x128: the shadow maps (halo_shadow_map_scale) */
static BOOL surface_is_shadow_map(const D3DSurface *surface)
{
	struct xgpu_texture_description description;

	xgpu_texture_describe(surface->Format, surface->Size, &description);
	return description.format == D3DFMT_R5G6B5 && description.width == SHADOW_MAP_SIZE &&
		description.height == SHADOW_MAP_SIZE;
}

static void target_scale_of(const D3DSurface *surface, unsigned long width, unsigned long height, BOOL depth,
	float scale[2])
{
	scale[0] = scale[1] = 1.0f;
	if (width == (unsigned long)halo_screen_width() && height == SCREEN_HEIGHT)
	{
		scale[0] = screen_scale[0];
		scale[1] = screen_scale[1];
	}
	else if (!depth && surface_is_shadow_map(surface))
	{
		scale[0] = scale[1] = (float)halo_shadow_map_scale();
	}
}

static long target_pixel(float coordinate, int axis)
{
	return (long)floorf(coordinate * target_scale[axis] + 0.5f);
}

/* a surface as the host knows it; NONE for none. A surface that is not a depth format
is not a depth buffer (d3d8_gl.c leaves it unbound) */
static void surface_describe(const D3DSurface *surface, BOOL depth_only, struct vk_surface *out, float scale_out[2])
{
	unsigned long width, height;
	float scale[2];
	BOOL depth;

	memset(out, 0, sizeof(*out));
	scale_out[0] = scale_out[1] = 1.0f;
	if (!surface || !surface->Data)
		return;
	surface_dimensions(surface, &width, &height, &depth);
	if (depth_only && !depth)
		return;
	target_scale_of(surface, width, height, depth, scale);
	out->data = surface->Data;
	out->width = (uint32_t)width;
	out->height = (uint32_t)height;
	out->pixel_width = (uint32_t)(width * scale[0] + 0.5f);
	out->pixel_height = (uint32_t)(height * scale[1] + 0.5f);
	out->kind = depth ? VK_SURFACE_DEPTH : VK_SURFACE_COLOR;
	scale_out[0] = scale[0];
	scale_out[1] = scale[1];
}

static void rendered_note(const D3DSurface *surface);

/* the targets the host has bound, as last told it; cleared when a frame starts there anew */
static struct vk_command_targets targets_told;
static BOOL targets_known;

/* ---------- multisampling's targets

With display.anti_aliasing's multisampling, the screen's targets are drawn multisampled, and so is any target drawn
together with one of them (the mirror's view goes to the secondary target with the back buffer's depth buffer), from
then on: a rendering's attachments have one count of samples, and a target the host makes multisampled keeps its samples
until it is made single again, which would lose what it holds (bind_targets in d3d8_gl.c, which keeps a renderbuffer of
the same sticking). The targets made multisampled are remembered by address until the setting changes. */

#define MULTISAMPLED_MAXIMUM 64

static unsigned long multisampled[MULTISAMPLED_MAXIMUM];
static unsigned multisampled_count;

static void multisampled_forget(void)
{
	multisampled_count = 0;
}

static BOOL multisampled_is(unsigned long data)
{
	unsigned index;

	for (index = 0; data && index < multisampled_count; index++)
	{
		if (multisampled[index] == data)
			return TRUE;
	}
	return FALSE;
}

static void multisampled_note(unsigned long data)
{
	if (data && !multisampled_is(data) && multisampled_count < MULTISAMPLED_MAXIMUM)
		multisampled[multisampled_count++] = data;
}

/* a surface the size of the screen (the back buffer, its depth buffer, the secondary targets) */
static BOOL surface_is_screen(const struct vk_surface *surface)
{
	return surface->kind != VK_SURFACE_NONE && surface->width == (uint32_t)halo_screen_width() &&
		surface->height == SCREEN_HEIGHT;
}

/* the samples a pixel of the bound targets (targets_told.samples), 1 for one */
static unsigned long target_samples = 1;

/* tells the host the current targets, if they are not what it has, and sets the scale
viewports and clears are in; FALSE if there is nothing to draw into (bind_targets in
d3d8_gl.c) */
static BOOL targets_bind(BOOL *has_depth)
{
	struct vk_command_targets targets;
	float color_scale[2], depth_scale[2];

	memset(&targets, 0, sizeof(targets));
	surface_describe(device.render_target, FALSE, &targets.color, color_scale);
	surface_describe(device.depth_stencil, TRUE, &targets.depth, depth_scale);
	if (targets.color.kind == VK_SURFACE_NONE && targets.depth.kind == VK_SURFACE_NONE)
		return FALSE;
	/* with multisampling, multisampled where either is a screen buffer or is multisampled already */
	target_samples = 1;
	if (anti_aliasing() == _anti_aliasing_msaa && anti_aliasing_samples > 1 &&
		(surface_is_screen(&targets.color) || surface_is_screen(&targets.depth) ||
		multisampled_is(targets.color.data) || multisampled_is(targets.depth.data)))
	{
		target_samples = anti_aliasing_samples;
		multisampled_note(targets.color.data);
		multisampled_note(targets.depth.data);
		targets.samples = (uint32_t)target_samples;
	}
	if (targets.color.kind != VK_SURFACE_NONE)
	{
		target_scale[0] = color_scale[0];
		target_scale[1] = color_scale[1];
	}
	else
	{
		target_scale[0] = depth_scale[0];
		target_scale[1] = depth_scale[1];
	}
	if (!targets_known || memcmp(&targets.color, &targets_told.color, sizeof(targets.color)) ||
		memcmp(&targets.depth, &targets_told.depth, sizeof(targets.depth)) || targets.samples != targets_told.samples)
	{
		struct vk_command_targets *command = stream_command(VK_COMMAND_TARGETS, sizeof(*command));

		command->color = targets.color;
		command->depth = targets.depth;
		command->samples = targets.samples;
		targets_told = targets;
		targets_known = TRUE;
		if (targets.color.kind != VK_SURFACE_NONE)
			rendered_note(device.render_target);
	}
	*has_depth = targets.depth.kind != VK_SURFACE_NONE;
	return TRUE;
}

/* display.anti_aliasing's FXAA over a window's 3D view (render.c calls this before the HUD and menus are drawn over it):
the rectangle in the game's units, of the back buffer, which the host passes over in place (host_vk_post.c) */
void halo_screen_anti_alias(short x0, short y0, short x1, short y1)
{
	struct vk_command_anti_alias *command;
	struct vk_surface surface;
	float scale[2];
	long left, top, right, bottom;

	if (!device.video_ready || anti_aliasing() != _anti_aliasing_fxaa)
		return;
	surface_describe(&device.back_buffer, FALSE, &surface, scale);
	if (surface.kind != VK_SURFACE_COLOR)
		return;
	left = (long)floorf(x0 * scale[0] + 0.5f);
	top = (long)floorf(y0 * scale[1] + 0.5f);
	right = (long)floorf(x1 * scale[0] + 0.5f);
	bottom = (long)floorf(y1 * scale[1] + 0.5f);
	if (right <= left || bottom <= top)
		return;
	command = stream_command(VK_COMMAND_ANTI_ALIAS, sizeof(*command));
	command->target = surface;
	command->rectangle[0] = (int32_t)left;
	command->rectangle[1] = (int32_t)top;
	command->rectangle[2] = (int32_t)(right - left);
	command->rectangle[3] = (int32_t)(bottom - top);
}

/* the colour surfaces the game has bound as targets, by address, and when each was last bound: a texture whose data is one samples
the target, not the game's memory (where the GPU never writes). As d3d8_gl.c's xgpu_render_target_find: the one bound last. */
#define RENDERED_MAXIMUM 256

static struct
{
	unsigned long data, width, height, clock;
} rendered[RENDERED_MAXIMUM];
static unsigned rendered_count;
static unsigned long rendered_clock;

static void rendered_note(const D3DSurface *surface)
{
	unsigned long width, height, index;
	BOOL depth;

	if (!surface || !surface->Data)
		return;
	surface_dimensions(surface, &width, &height, &depth);
	if (depth)
		return;
	for (index = 0; index < rendered_count; index++)
	{
		if (rendered[index].data == surface->Data && rendered[index].width == width && rendered[index].height == height)
		{
			rendered[index].clock = ++rendered_clock;
			return;
		}
	}
	if (rendered_count == RENDERED_MAXIMUM)
		return;
	rendered[rendered_count].data = surface->Data;
	rendered[rendered_count].width = width;
	rendered[rendered_count].height = height;
	rendered[rendered_count++].clock = ++rendered_clock;
}

/* the target last bound at a texture's data, or NULL */
static const void *rendered_find(unsigned long data, unsigned long *width, unsigned long *height)
{
	unsigned long index, best = RENDERED_MAXIMUM;

	for (index = 0; index < rendered_count; index++)
	{
		if (rendered[index].data == data && (best == RENDERED_MAXIMUM || rendered[index].clock > rendered[best].clock))
			best = index;
	}
	if (best == RENDERED_MAXIMUM)
		return NULL;
	*width = rendered[best].width;
	*height = rendered[best].height;
	return &rendered[best];
}

/* xbox_textures.c, hud_hires.c and text_hires.c call this after their own GL
calls, which under Vulkan have no context and do nothing */
void xgpu_gl_state_invalidate(void)
{
}

/* ---------- debug.vk_self_test: the data a draw reads is copied at the draw

A buffer of three vertices in the game's memory, a red triangle that covers the target,
is put; the same buffer is rewritten in place (green) and put again; a third, larger
than the stream, blue, with its vertices across the boundary of its first part, is put
in parts. Only then does the host draw each: red, green and blue say that the copies
were made at the puts, and not at the hand-over, and that parts land contiguously.
The host says what it saw in its log. */

static void test_vertices(struct vk_test_vertex *vertices, float red, float green, float blue)
{
	static const float corners[3][2] = { { -1.0f, -1.0f }, { 3.0f, -1.0f }, { -1.0f, 3.0f } };
	int index;

	for (index = 0; index < 3; index++)
	{
		vertices[index].position[0] = corners[index][0];
		vertices[index].position[1] = corners[index][1];
		vertices[index].position[2] = 0.5f;
		vertices[index].color[0] = red;
		vertices[index].color[1] = green;
		vertices[index].color[2] = blue;
		vertices[index].color[3] = 1.0f;
	}
}

static void test_draw(uint32_t id, uint32_t offset, uint32_t red, uint32_t green, uint32_t blue, uint32_t last)
{
	struct vk_command_test_draw *command = stream_command(VK_COMMAND_TEST_DRAW, sizeof(*command));

	command->data.id = id;
	command->data.offset = offset;
	command->expected[0] = red;
	command->expected[1] = green;
	command->expected[2] = blue;
	command->last = last;
}

static void data_self_test(void)
{
	struct vk_test_vertex vertices[3];
	const unsigned long total = STREAM_SIZE + 4096;
	unsigned char *large = malloc(total);
	uint32_t red, green, blue, offset;

	if (!large)
		return;
	test_vertices(vertices, 1.0f, 0.0f, 0.0f);
	red = vk_data_put(vertices, sizeof(vertices));
	test_vertices(vertices, 0.0f, 1.0f, 0.0f);
	green = vk_data_put(vertices, sizeof(vertices));
	/* the first part ends where the stream does: the vertices start 40 bytes before that, and end in the second */
	if (data_room() < 64)
		stream_flush();
	offset = (uint32_t)data_room() - 40;
	memset(large, 0xa5, total);
	test_vertices((struct vk_test_vertex *)(large + offset), 0.0f, 0.0f, 1.0f);
	blue = vk_data_put(large, total);
	free(large);
	test_draw(red, 0, 255, 0, 0, 0);
	test_draw(green, 0, 0, 255, 0, 0);
	test_draw(blue, offset, 0, 0, 255, 1);
	stream_flush();
	data_id = 0;
}

/* the shader service: a trivial pair of shaders that compile, and one that does not (the host says so once and keeps its GLSL
in vk_cache/failed/): each is sent, then asked for until it is made or has failed */
static void shader_self_test_one(const char *name, uint32_t stage, uint64_t hash, const char *glsl, BOOL expect_made)
{
	uint32_t status = VK_SHADER_STATUS_UNKNOWN, handle = 0;
	int attempt;

	host_vk_shader_compile(stage, hash, (uint32_t)(uintptr_t)glsl, (uint32_t)strlen(glsl));
	for (attempt = 0; attempt < 500; attempt++)
	{
		handle = host_vk_shader_find(stage, hash, (uint32_t)(uintptr_t)&status);
		if (handle || status == VK_SHADER_STATUS_FAILED)
			break;
		usleep(10000);
	}
	platform_log("shader service self-test: %s %s (handle %u, status %u, after %d tries)",
		name, (handle != 0) == expect_made && (handle != 0 || status == VK_SHADER_STATUS_FAILED) ? "ok" : "FAILED",
		(unsigned)handle, (unsigned)status, attempt + 1);
}

static void shader_self_test(void)
{
	shader_self_test_one("a vertex shader that compiles", VK_SHADER_STAGE_VERTEX, 0x5e1f7e57deadbeefull,
		"#version 450\nvoid main()\n{\n\tgl_Position = vec4(0.0);\n}\n", TRUE);
	shader_self_test_one("a pixel shader that compiles", VK_SHADER_STAGE_PIXEL, 0x5e1f7e57deadbef0ull,
		"#version 450\nlayout(location = 0) out vec4 c;\nvoid main()\n{\n\tc = vec4(1.0);\n}\n", TRUE);
	shader_self_test_one("a pixel shader that does not", VK_SHADER_STAGE_PIXEL, 0x5e1f7e57deadbef1ull,
		"#version 450\nvoid main()\n{\n\tthis is not glsl;\n}\n", FALSE);
}

/* ---------- device creation */

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

/* each vertex constant register's serial is the value constants_serial took
when the register last changed; a program's registers are current up to
the serial it recorded when it last uploaded them */
static unsigned long constant_serials[XGPU_VERTEX_CONSTANT_COUNT];
static unsigned long constants_serial;
/* the register each of the latest serials changed, so a program that is
only a little behind finds its changed registers without a full scan */
#define CONSTANT_LOG_SIZE 1024
static unsigned char constant_log[CONSTANT_LOG_SIZE];

static void constants_store(unsigned long first, const void *data, unsigned long count)
{
	const float (*values)[4] = data;
	unsigned long index;

	for (index = 0; index < count; index++)
	{
		if (memcmp(device.constants[first + index], values[index], sizeof(device.constants[0])))
		{
			memcpy(device.constants[first + index], values[index], sizeof(device.constants[0]));
			constant_serials[first + index] = ++constants_serial;
			constant_log[constants_serial % CONSTANT_LOG_SIZE] = (unsigned char)(first + index);
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

		/* The window is real but has no GL context (the host's stand-in,
		port/android/host/host.h): the platform layer's event loop runs only
		while it has one, and its swap holds frames to the display's rate */
		if (!config_boolean("debug.null_renderer") && platform_video_initialize(width, height))
		{
			device.video_ready = TRUE;
			/* the texture cache tells a texture was rewritten by the memory watch's generations (d3d8_gl.c starts it in
			gl_initialize): without it a texture the game rewrites would be drawn stale */
			memory_watch_initialize();
			platform_log("Direct3D: the Vulkan renderer");
		}
		else
			platform_log("Direct3D: running without a window (nothing is displayed)");
		device.created = TRUE;
		if (device.video_ready && config_boolean("debug.vk_self_test"))
		{
			data_self_test();
			shader_self_test();
		}
	}
	*returned_device = device_pointer();
	return S_OK;
}

/* ---------- the menus' pointer */

int halo_ui_pointer_update(int menus_active, struct halo_ui_pointer *pointer)
{
	(void)pointer;
	platform_menus_set_active(menus_active != 0);
	return 0;
}

/* the scoreboard's: none, as on Android's OpenGL ES (d3d8_gl.c); a vote to
kick starts from the console (votekick) */
int halo_scoreboard_pointer_update(int offered, struct halo_ui_pointer *pointer)
{
	(void)offered;
	(void)pointer;
	return -1;
}

/* takes up the display's shape and resolution, or the window's, if they
have changed; between frames, since the game's layout and the targets must
agree for a whole frame. Returns the width the game draws. */
long halo_screen_commit(void)
{
	long width;
	float scale[2];

	shadow_scale_commit();
	/* (display.anti_aliasing, as Settings or config.toml has it now) */
	anti_aliasing_read();
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

void WINAPI D3DDevice_SetRenderTarget(D3DSurface *render_target, D3DSurface *depth_stencil)
{
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
/* ---------- GPU synchronisation */

BOOL WINAPI D3DDevice_IsBusy(void)
{
	return FALSE;
}

void WINAPI D3DDevice_KickPushBuffer(void)
{
}

void WINAPI D3DDevice_InsertCallback(D3DCALLBACKTYPE type, D3DCALLBACK callback, DWORD context)
{
	(void)type;
	/* the "GPU" reaches the callback immediately */
	if (callback)
		callback(context);
}

/* ---------- the main menu's line under the version number (menu_functions.c): this renderer, and the driver archive the host
loaded, if it is not the phone's own driver (on a second line) */

uint32_t host_vk_driver_name(uint32_t out, uint32_t size);

char const *d3d8_renderer_description(void)
{
	static char description[64];

	if (!description[0])
	{
		char driver[56];

		driver[0] = 0;
		host_vk_driver_name((uint32_t)(uintptr_t)driver, sizeof(driver));
		if (driver[0])
			snprintf(description, sizeof(description), "Vulkan\r%s", driver);
		else
			snprintf(description, sizeof(description), "Vulkan");
	}
	return description;
}

/* ---------- visibility (occlusion) tests

The draws between Begin and End are the test's: the host counts the samples they pass (an occlusion query, host_vk_visibility.c)
and keeps the count, in the game's pixels, for the slot End names. The result is asked for through an import that never waits (the
latest count of the slot: from this test, or from an earlier one while the GPU is behind), as d3d8_gl.c's query buffer path gives. */

#define VISIBILITY_TEST_SLOTS 4096

uint32_t host_vk_visibility(uint32_t index);

static BOOL visibility_pending[VISIBILITY_TEST_SLOTS];

void WINAPI D3DDevice_BeginVisibilityTest(void)
{
	if (!device.video_ready || device.visibility_test_active)
		return;
	device.visibility_test_active = TRUE;
	stream_command(VK_COMMAND_VISIBILITY_BEGIN, sizeof(struct vk_command_header));
}

HRESULT WINAPI D3DDevice_EndVisibilityTest(DWORD index)
{
	struct vk_command_visibility_end *command;

	if (!device.video_ready || !device.visibility_test_active)
		return S_OK;
	device.visibility_test_active = FALSE;
	index %= VISIBILITY_TEST_SLOTS;
	if (!index)
		index = 1;
	command = stream_command(VK_COMMAND_VISIBILITY_END, sizeof(*command));
	command->index = (uint32_t)index;
	/* the target's pixels to a game pixel: the count is of the game's pixels (visibility_unscaled in d3d8_gl.c), which the game
	divides by its own test's area (lens flares, rasterizer_lights.c), a split-screen window's or the screen's alike */
	/* (multisampled, the host counts samples) */
	command->area = target_scale[0] * target_scale[1] * (float)target_samples;
	visibility_pending[index] = TRUE;
	return S_OK;
}

HRESULT WINAPI D3DDevice_GetVisibilityTestResult(DWORD index, UINT *result, ULONGLONG *time_stamp)
{
	if (time_stamp)
		*time_stamp = 0;
	index %= VISIBILITY_TEST_SLOTS;
	if (!index)
		index = 1;
	if (!device.video_ready || !visibility_pending[index])
	{
		if (result)
			*result = 0;
		return S_OK;
	}
	if (result)
		*result = host_vk_visibility((uint32_t)index);
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
/* the vertex shaders by id, so that the dump can name them */
static struct vertex_shader_object **vertex_shaders_by_id;
static unsigned long vertex_shaders_by_id_count;

static void vertex_shader_note(struct vertex_shader_object *object)
{
	if (object->id >= vertex_shaders_by_id_count)
	{
		unsigned long count = object->id + 64;
		struct vertex_shader_object **grown = realloc(vertex_shaders_by_id, count * sizeof(*grown));

		if (!grown)
			return;
		memset(grown + vertex_shaders_by_id_count, 0, (count - vertex_shaders_by_id_count) * sizeof(*grown));
		vertex_shaders_by_id = grown;
		vertex_shaders_by_id_count = count;
	}
	vertex_shaders_by_id[object->id] = object;
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
	{
		uint32_t count = (uint32_t)object->instruction_count;

		object->program_hash = vk_hash_mix(vk_hash_init(), &count, sizeof(count));
		if (object->instructions)
			object->program_hash = vk_hash_mix(object->program_hash, object->instructions, count * 4 * sizeof(DWORD));
	}
	vertex_shader_note(object);
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

/* the game names its model lighting programs as it creates them (rasterizer_xbox_vertex_shaders_initialize.c), so that
their draws can be lit for each pixel (display.per_pixel_lighting); one whose lighting is not as the pixel shader computes
it stays lit for each vertex (d3d8_gl.c) */
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

/* display.per_pixel_lighting: the model lighting programs' draws are lit for each pixel (nv2a_psh_vk.c model_lighting),
from shaders of their own; read again when a setting changes */
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

/* the vertex constant register of each of the per-pixel lighting's (XGPU_MODEL_LIGHT_COUNT): c[-82], then c[-79] to
c[-69] */
static unsigned long model_light_register(int light)
{
	return (unsigned long)(XGPU_VERTEX_CONSTANT_BIAS + (light ? -80 + light : -82));
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
/* ---------- the shaders a draw would use

Nothing is drawn yet, but at each draw the device makes the pixel shader key as d3d8_gl.c's prepare_draw
does, without binding anything, and notes it and the vertex shader for the dump (debug.gpu_dump_shaders), which
a check of the generators would take as its corpus. */

/* size is a multiple of 4 (d3d8_gl.c's hash_words) */
static unsigned long hash_words(const void *data, unsigned long size)
{
	const DWORD *words = data;
	unsigned long hash = 2166136261UL;

	for (size /= 4; size; size--)
		hash = (hash ^ *words++) * 16777619UL;
	return hash;
}

typedef char pixel_shader_key_size_assert[sizeof(struct nv2a_pixel_shader_key) % 4 == 0 ? 1 : -1];

static unsigned long stage_texture_mode(int stage)
{
	return (D3D__RenderState[D3DRS_PSTEXTUREMODES] >> (5 * stage)) & 0x1f;
}

/* what bind_textures in d3d8_gl.c decides for the key: a stage with no texture, or in modes 0, 0x04 and 0x05, has no
sampler (mode 0x11 a 2D one); otherwise the sampler is the texture's own type, and stage 0's coverage_alpha is whether
its bitmap has a high-res meter (hud_hires.h) whose green is its coverage. The cache there finds the override when it
uploads the pixels; here it is asked the same way, without the GL texture (the texture's own decoding failing, which
d3d8_gl.c would see, is not seen) */
static void key_textures(struct nv2a_pixel_shader_key *key)
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = device.textures[stage];
		unsigned long mode = stage_texture_mode(stage);
		struct xgpu_texture_description description;
		BOOL palettized;

		if (!texture || !texture->Data || mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x11)
		{
			key->sampler_type[stage] = mode == 0x11 ? _xgpu_sampler_2d : _xgpu_sampler_none;
			continue;
		}
		/* a render target sampled as a texture is a 2D image whatever the texture says (bind_textures in d3d8_gl.c) */
		{
			unsigned long target_width, target_height;

			if (rendered_find(texture->Data, &target_width, &target_height))
			{
				key->sampler_type[stage] = _xgpu_sampler_2d;
				continue;
			}
		}
		xgpu_texture_describe(texture->Format, texture->Size, &description);
		if (stage == 0)
		{
			palettized = ((texture->Format & D3DFORMAT_FORMAT_MASK) >> D3DFORMAT_FORMAT_SHIFT) == 0x0b;
			if (!palettized && !description.cube_map && description.depth == 1)
			{
				long override = hud_hires_override_find((unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data),
					description.width, description.height, description.levels > 1 ?
					xgpu_texture_level_offset(&description, 1) : xgpu_texture_face_size(&description));

				key->coverage_alpha = override >= 0 && hud_hires_override_coverage(override) != 0;
			}
		}
		key->sampler_type[stage] = description.cube_map ? _xgpu_sampler_cube :
			description.depth > 1 ? _xgpu_sampler_3d : _xgpu_sampler_2d;
	}
}

/* the key as prepare_draw makes it (count_samples stays 0: the visibility tests are occlusion queries) */
static void pixel_key_make(struct nv2a_pixel_shader_key *key)
{
	int stage;

	memset(key, 0, sizeof(*key));
	memcpy(key->combiner_state, D3D__RenderState, sizeof(key->combiner_state));
	/* constants are uniforms, not part of the program */
	memset(&key->combiner_state[D3DRS_PSCONSTANT0_0], 0, 16 * sizeof(DWORD));
	key->combiner_state[D3DRS_PSFINALCOMBINERCONSTANT0] = 0;
	key->combiner_state[D3DRS_PSFINALCOMBINERCONSTANT1] = 0;
	key->texture_modes = D3D__RenderState[D3DRS_PSTEXTUREMODES];
	key_textures(key);
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		key->alpha_kill[stage] = D3D__TextureState[stage][D3DTSS_ALPHAKILL] == D3DTALPHAKILL_ENABLE;
		key->color_sign[stage] = (unsigned char)((D3D__TextureState[stage][D3DTSS_COLORSIGN] >> 28) & 0xf);
	}
	/* (only with the meter's blend: hud_hires.h, nv2a_pixel_shader_key) */
	key->coverage_alpha = key->coverage_alpha && D3D__RenderState[D3DRS_ALPHABLENDENABLE] &&
		D3D__RenderState[D3DRS_SRCBLEND] == D3DBLEND_CONSTANTCOLOR &&
		D3D__RenderState[D3DRS_DESTBLEND] == D3DBLEND_SRCALPHA;
	key->alpha_test_function = D3D__RenderState[D3DRS_ALPHATESTENABLE] ? D3D__RenderState[D3DRS_ALPHAFUNC] : 0;
	/* multisampled (the targets are bound first: draw_make), an alpha-tested draw that is not blended covers samples in
	proportion to how far alpha is past the reference, so that cut-out edges (foliage, grates) are smoothed too */
	if (target_samples > 1 && key->alpha_test_function && !D3D__RenderState[D3DRS_ALPHABLENDENABLE])
		key->alpha_test_samples = (unsigned char)target_samples;
	key->fog_enable = D3D__RenderState[D3DRS_FOGENABLE] != 0;
	key->fog_table_mode = (unsigned char)D3D__RenderState[D3DRS_FOGTABLEMODE];
}

/* ---------- the dump */

struct dump_pixel
{
	struct nv2a_pixel_shader_key key;
	unsigned long hash;
	struct dump_pixel *next;
};

struct dump_vertex
{
	unsigned long id, mask;
	struct dump_vertex *next;
};

#define DUMP_BUCKETS 1024

static struct dump_pixel *dump_pixels[DUMP_BUCKETS];
static struct dump_vertex *dump_vertices[DUMP_BUCKETS];
static unsigned long dump_pixel_count, dump_vertex_count, dump_collisions;
static FILE *dump_manifest;
static BOOL dump_present_done;

static const char *dump_folder(void)
{
	const char *folder = config_string("debug.gpu_dump_shaders");

	return folder && *folder ? folder : NULL;
}

static void dump_file(const char *folder, const char *name, const void *data, unsigned long size, const char *what)
{
	char path[512];
	FILE *file;

	snprintf(path, sizeof(path), "%s/%s", folder, name);
	if ((file = fopen(path, "wb")) == NULL)
	{
		platform_log("shader dump: cannot write %s", path);
		return;
	}
	fwrite(data, 1, size, file);
	fclose(file);
	if (!dump_manifest)
	{
		snprintf(path, sizeof(path), "%s/manifest.txt", folder);
		dump_manifest = fopen(path, "a");
	}
	if (dump_manifest)
	{
		fprintf(dump_manifest, "%s %s\n", name, what);
		fflush(dump_manifest);
	}
}

static void dump_text(const char *folder, const char *name, const char *text, const char *what)
{
	dump_file(folder, name, text, strlen(text), what);
}

/* the GL ES generators' text, for the comparison: they write the context's #version, which this image has none of */
static char *gl_vertex_shader(const struct vertex_shader_object *program, unsigned long mask)
{
	const char *saved = xgpu_capabilities.shading_language;
	char *text;

	xgpu_capabilities.shading_language = "310 es";
	text = nv2a_vertex_shader_to_glsl(program->instructions, program->instruction_count, mask, NULL);
	xgpu_capabilities.shading_language = saved;
	return text;
}

static char *gl_pixel_shader(const struct nv2a_pixel_shader_key *key)
{
	const char *saved = xgpu_capabilities.shading_language;
	char *text;

	xgpu_capabilities.shading_language = "310 es";
	text = nv2a_pixel_shader_to_glsl(key);
	xgpu_capabilities.shading_language = saved;
	return text;
}

/* the first time a vertex shader is met with a packed-attribute mask: its Vulkan GLSL and the GL ES one */
static void dump_vertex_shader(const char *folder, const struct vertex_shader_object *program, unsigned long mask)
{
	struct dump_vertex **bucket = &dump_vertices[(program->id * 31 + mask) % DUMP_BUCKETS];
	struct dump_vertex *entry;
	char name[64], what[96];
	char *text;

	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->id == program->id && entry->mask == mask)
			return;
	}
	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return;
	entry->id = program->id;
	entry->mask = mask;
	entry->next = *bucket;
	*bucket = entry;
	dump_vertex_count++;
	snprintf(what, sizeof(what), "vertex shader %lu, %lu instructions, packed mask %lx", program->id,
		program->instruction_count, mask);
	text = nv2a_vk_vertex_shader_to_glsl((const uint32_t *)program->instructions, program->instruction_count, mask, NULL);
	snprintf(name, sizeof(name), "vs_%lu_%lx.vert", program->id, mask);
	dump_text(folder, name, text, what);
	free(text);
	text = gl_vertex_shader(program, mask);
	snprintf(name, sizeof(name), "vs_%lu_%lx.gl.vert", program->id, mask);
	dump_text(folder, name, text, what);
	free(text);
}

static void dump_pixel_shader(const char *folder, const struct nv2a_pixel_shader_key *key)
{
	unsigned long hash = hash_words(key, sizeof(*key)), same = 0;
	struct dump_pixel **bucket = &dump_pixels[hash % DUMP_BUCKETS];
	struct dump_pixel *entry;
	char name[64], what[96];
	char *text;

	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash != hash)
			continue;
		if (!memcmp(&entry->key, key, sizeof(*key)))
			return;
		same++;
	}
	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return;
	entry->key = *key;
	entry->hash = hash;
	entry->next = *bucket;
	*bucket = entry;
	dump_pixel_count++;
	/* two keys with one 32-bit hash get different names (d3d8_gl.c would have shared one program) */
	if (same)
		dump_collisions++;
	snprintf(what, sizeof(what), "pixel shader, key hash %08lx%s", hash, same ? " (a collision)" : "");
	if (same)
		snprintf(name, sizeof(name), "ps_%08lx_%lu", hash, same);
	else
		snprintf(name, sizeof(name), "ps_%08lx", hash);
	{
		char file[80];

		snprintf(file, sizeof(file), "%s.key", name);
		dump_file(folder, file, key, sizeof(*key), what);
		text = nv2a_vk_pixel_shader_to_glsl(key);
		snprintf(file, sizeof(file), "%s.frag", name);
		dump_text(folder, file, text, what);
		free(text);
		text = gl_pixel_shader(key);
		snprintf(file, sizeof(file), "%s.gl.frag", name);
		dump_text(folder, file, text, what);
		free(text);
	}
}

/* ---------- the shader service's handles

The host never sees a key: a shader is asked for by its hash (vk_shaders.h's identity), and for one the host does not
have, the GLSL is generated here and sent, to be compiled on the host's thread and kept on the device. A draw's shader
is the handle the host gave, or 0 while it is queued or compiling (the draw would be skipped). */

struct shader_entry
{
	uint64_t hash;
	uint32_t handle;
	unsigned char sent, failed;
	struct shader_entry *next;
};

#define SHADER_BUCKETS 1024

static struct shader_entry *shader_entries[2][SHADER_BUCKETS];
static BOOL shader_import_logged;

/* the handle of a shader, asking the host (and sending its GLSL when the host does not know it); 0 if it is not ready,
and *failed set if it never will be. lit: a vertex shader that hands its lighting on (nv2a_vk_vertex_shader_to_glsl) */
static uint32_t shader_handle(uint32_t stage, uint64_t hash, const struct vertex_shader_object *program,
	unsigned long mask, BOOL lit, const struct nv2a_pixel_shader_key *key, BOOL *failed)
{
	struct shader_entry **bucket = &shader_entries[stage][hash % SHADER_BUCKETS];
	struct shader_entry *entry;
	uint32_t status = VK_SHADER_STATUS_UNKNOWN, handle;

	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash)
			break;
	}
	if (!entry)
	{
		entry = calloc(1, sizeof(*entry));
		if (!entry)
			return 0;
		entry->hash = hash;
		entry->next = *bucket;
		*bucket = entry;
	}
	if (entry->handle)
		return entry->handle;
	if (entry->failed)
	{
		*failed = TRUE;
		return 0;
	}
	if (!shader_import_logged)
	{
		shader_import_logged = TRUE;
		platform_log("vk shaders: the first host_vk_shader_find: stage %u, hash %08x%08x (the host logs its own)",
			(unsigned)stage, (unsigned)(hash >> 32), (unsigned)hash);
	}
	handle = host_vk_shader_find(stage, hash, (uint32_t)(uintptr_t)&status);
	if (handle)
	{
		entry->handle = handle;
		return handle;
	}
	if (status == VK_SHADER_STATUS_FAILED)
	{
		entry->failed = 1;
		*failed = TRUE;
		return 0;
	}
	if (status == VK_SHADER_STATUS_UNKNOWN && entry->sent < 3)
	{
		char *glsl = stage == VK_SHADER_STAGE_VERTEX ?
			nv2a_vk_vertex_shader_to_glsl((const uint32_t *)program->instructions, program->instruction_count, mask,
				lit ? &program->lighting : NULL) :
			nv2a_vk_pixel_shader_to_glsl(key);

		if (glsl)
		{
			host_vk_shader_compile(stage, hash, (uint32_t)(uintptr_t)glsl, (uint32_t)strlen(glsl));
			free(glsl);
			entry->sent++;
		}
	}
	return 0;
}

/* a vertex shader's identity (vk_shaders.h): the generator's version, the program's hash and its packed mask, and for one
that hands its lighting on (lit), a word saying so (the unlit ones' are as they always were) */
static uint64_t vertex_shader_hash(const struct vertex_shader_object *program, unsigned long mask, BOOL lit)
{
	uint32_t generator = VK_SHADER_GENERATOR_VERSION, mask32 = (uint32_t)mask;
	uint64_t hash;

	hash = vk_hash_mix(vk_hash_init(), &generator, sizeof(generator));
	hash = vk_hash_mix(hash, &program->program_hash, sizeof(program->program_hash));
	hash = vk_hash_mix(hash, &mask32, sizeof(mask32));
	if (lit)
	{
		static const char marker[] = "lit";

		hash = vk_hash_mix(hash, marker, sizeof(marker));
	}
	return hash;
}

/* a pixel shader's: the generator's version and the key's bytes */
static uint64_t pixel_shader_hash(const struct nv2a_pixel_shader_key *key)
{
	uint32_t generator = VK_SHADER_GENERATOR_VERSION;

	return vk_hash_mix(vk_hash_mix(vk_hash_init(), &generator, sizeof(generator)), key, sizeof(*key));
}

/* the shaders a draw needs, as d3d8_gl.c's prepare_draw chooses them, asked for from the host (and sent when the host lacks
them), and noted for the dump (debug.gpu_dump_shaders). FALSE if there is no program to draw with. The handles are 0 while a
shader is queued or compiling (the draw is skipped). The key is made without binding anything. */
static BOOL draw_shaders(BOOL immediate, struct nv2a_pixel_shader_key *key, unsigned long *mask_out, uint32_t *vertex,
	uint32_t *pixel)
{
	struct vertex_shader_object *program = current_program();
	const char *folder = dump_folder();
	unsigned long mask;

	*vertex = *pixel = 0;
	if (!program || !device.vertex_shader || !program->instructions)
		return FALSE;
	pixel_key_make(key);
	mask = immediate ? 0 : device.vertex_shader->packed_mask;
	*mask_out = mask;
	if (folder)
	{
		dump_vertex_shader(folder, program, mask);
		dump_pixel_shader(folder, key);
	}
	if (!device.video_ready)
		return TRUE;
	/* lit for each pixel where the setting is on and the program is one the pixel shader can light: a pair of shaders of
	their own, and the draw as the vertex shader lights it if either fails */
	if (program->lighting.lights && !program->lighting_failed && per_pixel_lighting())
	{
		BOOL failed = FALSE;

		key->per_pixel_lighting = (unsigned char)program->lighting.lights;
		*vertex = shader_handle(VK_SHADER_STAGE_VERTEX, vertex_shader_hash(program, mask, TRUE), program, mask, TRUE, NULL,
			&failed);
		*pixel = shader_handle(VK_SHADER_STAGE_PIXEL, pixel_shader_hash(key), NULL, 0, FALSE, key, &failed);
		if (!failed)
			return TRUE;
		platform_log("GPU: vertex shader %lu cannot be lit for each pixel here (refer to the host's shader log): "
			"lit for each vertex", program->id);
		program->lighting_failed = TRUE;
		key->per_pixel_lighting = 0;
	}
	{
		BOOL failed = FALSE;

		*vertex = shader_handle(VK_SHADER_STAGE_VERTEX, vertex_shader_hash(program, mask, FALSE), program, mask, FALSE, NULL,
			&failed);
		*pixel = shader_handle(VK_SHADER_STAGE_PIXEL, pixel_shader_hash(key), NULL, 0, FALSE, key, &failed);
	}
	return TRUE;
}

/* at each Present: the first one writes every vertex shader with mask 0 as well, so that each has its immediate-mode
form; the log says how many were written, again when more have come */
static void dump_present(void)
{
	static unsigned long logged_vertices, logged_pixels;
	const char *folder = dump_folder();
	unsigned long id;

	if (!folder)
		return;
	if (!dump_present_done && device.frame >= 60)
	{
		dump_present_done = TRUE;
		for (id = 0; id < vertex_shaders_by_id_count; id++)
		{
			if (vertex_shaders_by_id[id] && vertex_shaders_by_id[id]->instructions)
				dump_vertex_shader(folder, vertex_shaders_by_id[id], 0);
		}
	}
	if (dump_present_done && device.frame % 600 == 0 && (dump_vertex_count != logged_vertices || dump_pixel_count != logged_pixels))
	{
		logged_vertices = dump_vertex_count;
		logged_pixels = dump_pixel_count;
		platform_log("shader dump: %lu vertex shaders and %lu pixel shaders written to %s (%lu key hash collisions)",
			dump_vertex_count, dump_pixel_count, folder, dump_collisions);
	}
}

/* ---------- vertex data and drawing */

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
	/* an index buffer's Data is a window address, not an offset within one:
	from a map file it is the address the window was linked at, and from
	CreateIndexBuffer ordinary memory, which the move leaves alone */
	D3D__IndexData = index_data ? (WORD *)index_data->Data : NULL;
}

/* debug.gpu_stats: the draws of the last 60 frames, and what skipped the rest */
static struct
{
	unsigned long draws, immediate_draws, presents;
	unsigned long skipped_no_program, skipped_no_target, skipped_shader, skipped_texture, skipped_other;
} stats;
static BOOL statistics_on, statistics_read;

/* The Xbox's enumerants are OpenGL's (D3DBLEND_*, D3DCMP_*, the stencil operations, D3DBLENDOP_*): Vulkan's numbers for them */

static uint32_t compare_op(DWORD value)
{
	/* GL_NEVER to GL_ALWAYS are 0x200 to 0x207, in Vulkan's order; 0 is never (d3d8_gl.c) */
	return value >= 0x200 && value <= 0x207 ? value - 0x200 : 0;
}

static uint32_t stencil_op(DWORD value)
{
	switch (value)
	{
	case 0: return 1; /* ZERO */
	case D3DSTENCILOP_REPLACE: return 2;
	case D3DSTENCILOP_INCRSAT: return 3;
	case D3DSTENCILOP_DECRSAT: return 4;
	case D3DSTENCILOP_INVERT: return 5;
	case D3DSTENCILOP_INCR: return 6;
	case D3DSTENCILOP_DECR: return 7;
	default: return 0; /* KEEP */
	}
}

static uint32_t blend_factor(DWORD value)
{
	switch (value)
	{
	case D3DBLEND_ZERO: return 0;
	case D3DBLEND_ONE: return 1;
	case D3DBLEND_SRCCOLOR: return 2;
	case D3DBLEND_INVSRCCOLOR: return 3;
	case D3DBLEND_DESTCOLOR: return 4;
	case D3DBLEND_INVDESTCOLOR: return 5;
	case D3DBLEND_SRCALPHA: return 6;
	case D3DBLEND_INVSRCALPHA: return 7;
	case D3DBLEND_DESTALPHA: return 8;
	case D3DBLEND_INVDESTALPHA: return 9;
	case D3DBLEND_CONSTANTCOLOR: return 10;
	case D3DBLEND_INVCONSTANTCOLOR: return 11;
	case D3DBLEND_CONSTANTALPHA: return 12;
	case D3DBLEND_INVCONSTANTALPHA: return 13;
	case D3DBLEND_SRCALPHASAT: return 14;
	default: return 1;
	}
}

static uint32_t blend_operation(DWORD value)
{
	switch (value)
	{
	case D3DBLENDOP_SUBTRACT: return 1;
	case D3DBLENDOP_REVSUBTRACT:
	case D3DBLENDOP_REVSUBTRACTSIGNED: return 2;
	case D3DBLENDOP_MIN: return 3;
	case D3DBLENDOP_MAX: return 4;
	default: return 0;
	}
}

/* primitive_mode in d3d8_gl.c, as Vulkan's topology: quads and line loops are drawn as indices (draw_make) */
static uint32_t topology_of(D3DPRIMITIVETYPE type)
{
	switch (type)
	{
	case D3DPT_POINTLIST: return 0;
	case D3DPT_LINELIST: return 1;
	case D3DPT_LINELOOP:
	case D3DPT_LINESTRIP: return 2;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_QUADSTRIP: return 4;
	case D3DPT_TRIANGLEFAN:
	case D3DPT_POLYGON: return 5;
	default: return 3;
	}
}

/* apply_raster_state in d3d8_gl.c, as the pipeline's state and the draw's dynamic state */
static void raster_state_make(struct vk_command_draw *draw, BOOL has_color, BOOL has_depth)
{
	struct vk_pipeline_state *state = &draw->state;
	DWORD *rs = D3D__RenderState;
	DWORD write = rs[D3DRS_COLORWRITEENABLE];
	BOOL depth_test = has_depth && rs[D3DRS_ZENABLE];
	long x0 = target_pixel((float)device.viewport.X, 0), y0 = target_pixel((float)device.viewport.Y, 1);
	long x1 = target_pixel((float)(device.viewport.X + device.viewport.Width), 0);
	long y1 = target_pixel((float)(device.viewport.Y + device.viewport.Height), 1);

	draw->viewport[0] = (float)x0;
	draw->viewport[1] = (float)y0;
	draw->viewport[2] = (float)(x1 - x0);
	draw->viewport[3] = (float)(y1 - y0);
	draw->viewport[4] = device.viewport.MinZ;
	draw->viewport[5] = device.viewport.MaxZ;
	/* the game never issues a scissor rectangle, and the NV2A scissor register defaults to the viewport, so fragment
	clipping follows the viewport: this is what keeps a split-screen window's geometry from bleeding across the divider */
	draw->scissor[0] = (int32_t)x0;
	draw->scissor[1] = (int32_t)y0;
	draw->scissor[2] = (int32_t)(x1 - x0);
	draw->scissor[3] = (int32_t)(y1 - y0);

	state->color_format = has_color ? 1 : 0;
	state->depth_format = has_depth ? 1 : 0;
	state->samples = target_samples > 1 ? (uint32_t)target_samples : 0;
	state->color_write_mask = ((write & D3DCOLORWRITEENABLE_RED) ? 1 : 0) | ((write & D3DCOLORWRITEENABLE_GREEN) ? 2 : 0) |
		((write & D3DCOLORWRITEENABLE_BLUE) ? 4 : 0) | ((write & D3DCOLORWRITEENABLE_ALPHA) ? 8 : 0);
	state->polygon_mode = rs[D3DRS_FILLMODE] == D3DFILL_WIREFRAME ? 1 : rs[D3DRS_FILLMODE] == D3DFILL_POINT ? 2 : 0;
	state->depth_test = depth_test;
	if (depth_test)
	{
		state->depth_write = rs[D3DRS_ZWRITEENABLE] != 0;
		state->depth_compare_op = compare_op(rs[D3DRS_ZFUNC]);
	}
	if (has_depth && rs[D3DRS_STENCILENABLE])
	{
		struct vk_stencil_face_state face;

		face.compare_op = compare_op(rs[D3DRS_STENCILFUNC]);
		face.fail_op = stencil_op(rs[D3DRS_STENCILFAIL]);
		face.depth_fail_op = stencil_op(rs[D3DRS_STENCILZFAIL]);
		face.pass_op = stencil_op(rs[D3DRS_STENCILPASS]);
		state->stencil_test = 1;
		state->stencil_front = face;
		state->stencil_back = face;
		draw->stencil_compare_mask = rs[D3DRS_STENCILMASK] & 0xff;
		draw->stencil_write_mask = rs[D3DRS_STENCILWRITEMASK] & 0xff;
		draw->stencil_reference = rs[D3DRS_STENCILREF] & 0xff;
	}
	if (rs[D3DRS_ALPHABLENDENABLE])
	{
		state->blend_enable = 1;
		state->source_color_factor = state->source_alpha_factor = blend_factor(rs[D3DRS_SRCBLEND]);
		state->destination_color_factor = state->destination_alpha_factor = blend_factor(rs[D3DRS_DESTBLEND]);
		state->color_op = state->alpha_op = blend_operation(rs[D3DRS_BLENDOP]);
		color_to_vec4(rs[D3DRS_BLENDCOLOR], draw->blend_constants);
	}
	/* the cull mode names the winding to discard, FRONTFACE the front winding: d3d8_gl.c's desktop branch (the Android one
	answers its y flip in the shaders, which these do not make: the flip is the viewport's) */
	if (rs[D3DRS_CULLMODE] != D3DCULL_NONE)
	{
		state->cull_mode = rs[D3DRS_CULLMODE] == rs[D3DRS_FRONTFACE] ? 1 : 2; /* front : back */
		state->front_face = rs[D3DRS_FRONTFACE] == D3DFRONT_CCW ? 0 : 1; /* counter-clockwise : clockwise */
	}
	/* D3DRS_ZBIAS is expressed in these states (D3DDevice_SetRenderState_ZBias): a polygon offset's units and slope factor */
	if (rs[D3DRS_SOLIDOFFSETENABLE])
	{
		state->depth_bias = 1;
		draw->depth_bias_slope = dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]);
		draw->depth_bias_constant = dword_to_float(rs[D3DRS_POLYGONOFFSETZOFFSET]);
	}
}

/* ---------- uniforms: the three blocks, put when they differ from what this frame last put */

static struct
{
	BOOL constants_valid, parameters_valid;
	unsigned long constants_serial;
	struct vk_data_ref constants_ref, parameters_ref;
	struct vk_vertex_parameters parameters;
	/* the pixel parameters (a few distinct ones a frame at most) */
	BOOL pixel_valid;
	struct vk_pixel_parameters pixel;
	struct vk_data_ref pixel_ref;
	/* the attributes' current values (SetVertexData), as the unfed binding holds them */
	BOOL attributes_valid;
	float attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	struct vk_data_ref attributes_ref;
} puts_this_frame;

/* the data ids are good for a frame (Present resets them): so is what was put */
static void puts_reset(void)
{
	memset(&puts_this_frame, 0, sizeof(puts_this_frame));
}

static struct vk_data_ref data_ref(uint32_t id)
{
	struct vk_data_ref ref;

	ref.id = id;
	ref.offset = 0;
	return ref;
}

static void pixel_parameters_make(struct vk_pixel_parameters *p, const float texture_scale[4][4], BOOL lit)
{
	int stage;

	memset(p, 0, sizeof(*p));
	for (stage = 0; stage < 8; stage++)
	{
		color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT0_0 + stage], p->ps_c0[stage]);
		color_to_vec4(D3D__RenderState[D3DRS_PSCONSTANT1_0 + stage], p->ps_c1[stage]);
	}
	color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT0], p->ps_final_c0);
	color_to_vec4(D3D__RenderState[D3DRS_PSFINALCOMBINERCONSTANT1], p->ps_final_c1);
	color_to_vec4(D3D__RenderState[D3DRS_FOGCOLOR], p->fog_color);
	p->fog_parameters[0] = dword_to_float(D3D__RenderState[D3DRS_FOGSTART]);
	p->fog_parameters[1] = dword_to_float(D3D__RenderState[D3DRS_FOGEND]);
	p->fog_parameters[2] = dword_to_float(D3D__RenderState[D3DRS_FOGDENSITY]);
	p->alpha_reference[0] = (float)(D3D__RenderState[D3DRS_ALPHAREF] & 0xff);
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		DWORD *state = D3D__TextureState[stage];

		p->bump_matrix[stage][0] = dword_to_float(state[D3DTSS_BUMPENVMAT00]);
		p->bump_matrix[stage][1] = dword_to_float(state[D3DTSS_BUMPENVMAT01]);
		p->bump_matrix[stage][2] = dword_to_float(state[D3DTSS_BUMPENVMAT10]);
		p->bump_matrix[stage][3] = dword_to_float(state[D3DTSS_BUMPENVMAT11]);
		p->bump_luminance[stage][0] = dword_to_float(state[D3DTSS_BUMPENVLSCALE]);
		p->bump_luminance[stage][1] = dword_to_float(state[D3DTSS_BUMPENVLOFFSET]);
	}
	memcpy(p->texture_scale, texture_scale, sizeof(p->texture_scale));
	/* (only for a draw lit for each pixel, so that the others' parameters stay equal from draw to draw) */
	for (stage = 0; lit && stage < XGPU_MODEL_LIGHT_COUNT; stage++)
		memcpy(p->model_lights[stage], device.constants[model_light_register(stage)], sizeof(p->model_lights[stage]));
}

static void uniforms_make(struct vk_command_draw *draw, const float texture_scale[4][4], BOOL lit)
{
	struct vk_vertex_parameters parameters;
	struct vk_pixel_parameters pixel;

	if (!puts_this_frame.constants_valid || puts_this_frame.constants_serial != constants_serial)
	{
		puts_this_frame.constants_ref = data_ref(vk_data_put(device.constants, sizeof(device.constants)));
		puts_this_frame.constants_serial = constants_serial;
		puts_this_frame.constants_valid = TRUE;
	}
	draw->vertex_constants = puts_this_frame.constants_ref;

	memcpy(parameters.viewport_scale, device.viewport_scale, sizeof(parameters.viewport_scale));
	memcpy(parameters.viewport_offset, device.viewport_offset, sizeof(parameters.viewport_offset));
	parameters.point_and_screen[0] = D3D__RenderState[D3DRS_POINTSIZE] ? dword_to_float(D3D__RenderState[D3DRS_POINTSIZE]) : 1.0f;
	parameters.point_and_screen[1] = (float)UI_OFFSET;
	parameters.point_and_screen[2] = parameters.point_and_screen[3] = 0.0f;
	if (!puts_this_frame.parameters_valid || memcmp(&puts_this_frame.parameters, &parameters, sizeof(parameters)))
	{
		puts_this_frame.parameters = parameters;
		puts_this_frame.parameters_ref = data_ref(vk_data_put(&parameters, sizeof(parameters)));
		puts_this_frame.parameters_valid = TRUE;
	}
	draw->vertex_parameters = puts_this_frame.parameters_ref;

	pixel_parameters_make(&pixel, texture_scale, lit);
	if (!puts_this_frame.pixel_valid || memcmp(&puts_this_frame.pixel, &pixel, sizeof(pixel)))
	{
		puts_this_frame.pixel = pixel;
		puts_this_frame.pixel_ref = data_ref(vk_data_put(&pixel, sizeof(pixel)));
		puts_this_frame.pixel_valid = TRUE;
	}
	draw->pixel_parameters = puts_this_frame.pixel_ref;
}

/* ---------- vertex input */

/* Vulkan's format for a vertex element, as attribute_format in d3d8_gl.c maps it to GL (its desktop branch: D3DCOLOR is
B8G8R8A8_UNORM, which reads the Xbox's byte order as it is) */
static uint32_t element_format(unsigned long type)
{
	switch (type)
	{
	case D3DVSDT_FLOAT1: return VK_VERTEX_FORMAT_R32_SFLOAT;
	case D3DVSDT_FLOAT2: return VK_VERTEX_FORMAT_R32G32_SFLOAT;
	case D3DVSDT_FLOAT3:
	case D3DVSDT_FLOAT2H: return VK_VERTEX_FORMAT_R32G32B32_SFLOAT;
	case D3DVSDT_FLOAT4: return VK_VERTEX_FORMAT_R32G32B32A32_SFLOAT;
	case D3DVSDT_D3DCOLOR: return VK_VERTEX_FORMAT_B8G8R8A8_UNORM;
	case D3DVSDT_SHORT1: return VK_VERTEX_FORMAT_R16_SSCALED;
	case D3DVSDT_SHORT2: return VK_VERTEX_FORMAT_R16G16_SSCALED;
	case D3DVSDT_SHORT3: return VK_VERTEX_FORMAT_R16G16B16_SSCALED;
	case D3DVSDT_SHORT4: return VK_VERTEX_FORMAT_R16G16B16A16_SSCALED;
	case D3DVSDT_NORMSHORT1: return VK_VERTEX_FORMAT_R16_SNORM;
	case D3DVSDT_NORMSHORT2: return VK_VERTEX_FORMAT_R16G16_SNORM;
	case D3DVSDT_NORMSHORT3: return VK_VERTEX_FORMAT_R16G16B16_SNORM;
	case D3DVSDT_NORMSHORT4: return VK_VERTEX_FORMAT_R16G16B16A16_SNORM;
	case D3DVSDT_NORMPACKED3: return VK_VERTEX_FORMAT_R32_UINT;
	case D3DVSDT_PBYTE1: return VK_VERTEX_FORMAT_R8_UNORM;
	case D3DVSDT_PBYTE2: return VK_VERTEX_FORMAT_R8G8_UNORM;
	case D3DVSDT_PBYTE3: return VK_VERTEX_FORMAT_R8G8B8_UNORM;
	case D3DVSDT_PBYTE4: return VK_VERTEX_FORMAT_R8G8B8A8_UNORM;
	default: return VK_VERTEX_FORMAT_R32G32B32A32_SFLOAT;
	}
}

uint32_t host_vk_format_supported(uint32_t format);

/* Vulkan requires few formats as vertex attributes (the 32-bit ones; the rest are optional): the host says, once for each
format, and the guest expands what is missing into four floats. Said once in the log. */
static BOOL format_supported(uint32_t format)
{
	static unsigned char known[100]; /* 0 not asked, 1 yes, 2 no */

	if (format >= 98)
		return TRUE;
	if (!known[format])
	{
		known[format] = host_vk_format_supported(format) ? 1 : 2;
		if (known[format] == 2)
			platform_log("vk: vertex format %u is not readable by the device as a vertex attribute; those attributes are "
				"expanded to four floats", (unsigned)format);
	}
	return known[format] == 1;
}

/* an element's value as up to four floats (the rest 0, 0, 0, 1 as a vertex input fills them) */
static void element_decode(unsigned long type, const unsigned char *source, float out[4])
{
	const float *f = (const float *)source;
	const short *s = (const short *)source;
	unsigned long index, count = 0;

	out[0] = out[1] = out[2] = 0.0f;
	out[3] = 1.0f;
	switch (type)
	{
	case D3DVSDT_FLOAT1: count = 1; break;
	case D3DVSDT_FLOAT2: count = 2; break;
	case D3DVSDT_FLOAT3:
	case D3DVSDT_FLOAT2H: count = 3; break;
	case D3DVSDT_FLOAT4: count = 4; break;
	default: break;
	}
	if (count)
	{
		for (index = 0; index < count; index++)
			out[index] = f[index];
		return;
	}
	switch (type)
	{
	case D3DVSDT_D3DCOLOR:
		out[0] = source[2] / 255.0f;
		out[1] = source[1] / 255.0f;
		out[2] = source[0] / 255.0f;
		out[3] = source[3] / 255.0f;
		return;
	case D3DVSDT_SHORT1: case D3DVSDT_SHORT2: case D3DVSDT_SHORT3: case D3DVSDT_SHORT4:
		count = type >> 4; /* the type's high nibble is its component count */
		for (index = 0; index < count; index++)
			out[index] = (float)s[index];
		return;
	case D3DVSDT_NORMSHORT1: case D3DVSDT_NORMSHORT2: case D3DVSDT_NORMSHORT3: case D3DVSDT_NORMSHORT4:
		count = type >> 4;
		for (index = 0; index < count; index++)
		{
			float value = s[index] / 32767.0f;

			out[index] = value < -1.0f ? -1.0f : value;
		}
		return;
	case D3DVSDT_PBYTE1: case D3DVSDT_PBYTE2: case D3DVSDT_PBYTE3: case D3DVSDT_PBYTE4:
		count = type >> 4;
		for (index = 0; index < count; index++)
			out[index] = source[index] / 255.0f;
		return;
	default:
		return;
	}
}

/* the stream's elements, with the formats the device cannot read as attributes put expanded to four floats: a buffer of its
own, made at the draw from the game's bytes, every element of the stream in it (a NORMPACKED3 one keeps its 32 bits) */
static uint32_t stream_expanded_put(const struct vertex_shader_object *declaration, unsigned long stream,
	const unsigned char *base, unsigned long stride, unsigned long vertices, struct vk_pipeline_state *state,
	unsigned long binding)
{
	unsigned long indexes[XGPU_VERTEX_ATTRIBUTE_COUNT], elements = 0, index, vertex;
	unsigned long out_stride, rows = stride ? vertices : 1;
	unsigned char *buffer;
	uint32_t id;

	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];

		if (element->stream == stream && element->type != D3DVSDT_NONE)
			indexes[elements++] = index;
	}
	out_stride = elements * 16;
	buffer = calloc(rows, out_stride);
	if (!buffer)
		return 0;
	for (vertex = 0; vertex < rows; vertex++)
	{
		for (index = 0; index < elements; index++)
		{
			const struct vertex_element *element = &declaration->elements[indexes[index]];
			unsigned char *at = buffer + vertex * out_stride + index * 16;
			const unsigned char *source = base + vertex * stride + element->offset;

			if (element->type == D3DVSDT_NORMPACKED3)
				memcpy(at, source, 4);
			else
				element_decode(element->type, source, (float *)at);
		}
	}
	for (index = 0; index < elements; index++)
	{
		const struct vertex_element *element = &declaration->elements[indexes[index]];

		state->attributes[element->reg].format = element->type == D3DVSDT_NORMPACKED3 ? VK_VERTEX_FORMAT_R32_UINT :
			VK_VERTEX_FORMAT_R32G32B32A32_SFLOAT;
		state->attributes[element->reg].binding = (uint32_t)binding;
		state->attributes[element->reg].offset = (uint32_t)(index * 16);
	}
	state->bindings[binding].stride = stride ? (uint32_t)out_stride : 0;
	id = vk_data_put(buffer, rows * out_stride);
	free(buffer);
	return id;
}

/* the vertices [first, first + count) of every stream the declaration reads, put, and the pipeline's bindings and attributes
for them (setup_streams in d3d8_gl.c); the attributes the declaration does not feed read the current values SetVertexData
set, from one more binding of stride 0, put again whenever they change. FALSE if the draw cannot be made. */
static BOOL vertex_input_make(struct vk_command_draw *draw, unsigned long first, unsigned long count)
{
	const struct vertex_shader_object *declaration = device.vertex_shader;
	struct vk_pipeline_state *state = &draw->state;
	int binding_of[16];
	BOOL fed[XGPU_VERTEX_ATTRIBUTE_COUNT] = { FALSE };
	unsigned long index, bindings = 0, stream;

	for (index = 0; index < 16; index++)
		binding_of[index] = -1;
	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];

		if (!device.streams[element->stream].data || element->type == D3DVSDT_NONE || binding_of[element->stream] >= 0)
			continue;
		if (bindings >= VK_PIPELINE_VERTEX_BINDINGS - 1)
			return FALSE;
		binding_of[element->stream] = (int)bindings++;
	}
	for (stream = 0; stream < 16; stream++)
	{
		unsigned long stride = device.streams[stream].stride, bytes;
		const unsigned char *base;
		BOOL expand = FALSE;
		unsigned long binding;

		if (binding_of[stream] < 0)
			continue;
		binding = (unsigned long)binding_of[stream];
		bytes = stride ? stride * count : 64;
		base = (const unsigned char *)PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data) + first * stride;
		for (index = 0; index < declaration->element_count; index++)
		{
			const struct vertex_element *element = &declaration->elements[index];

			if (element->stream == stream && element->type != D3DVSDT_NONE &&
				!format_supported(element_format(element->type)))
				expand = TRUE;
		}
		if (expand)
		{
			uint32_t id = stream_expanded_put(declaration, stream, base, stride, count, state, binding);

			if (!id)
				return FALSE;
			draw->vertex_buffers[binding] = data_ref(id);
		}
		else
		{
			draw->vertex_buffers[binding] = data_ref(vk_data_put(base, bytes));
			state->bindings[binding].stride = (uint32_t)stride;
			for (index = 0; index < declaration->element_count; index++)
			{
				const struct vertex_element *element = &declaration->elements[index];

				if (element->stream != stream || element->type == D3DVSDT_NONE)
					continue;
				state->attributes[element->reg].format = element_format(element->type);
				state->attributes[element->reg].binding = (uint32_t)binding;
				state->attributes[element->reg].offset = element->offset;
			}
		}
		for (index = 0; index < declaration->element_count; index++)
		{
			const struct vertex_element *element = &declaration->elements[index];

			if (element->stream == stream && element->type != D3DVSDT_NONE)
				fed[element->reg] = TRUE;
		}
	}
	/* the unfed attributes' current values: all sixteen in one binding of stride 0 (a packed attribute reads zero) */
	{
		float values[XGPU_VERTEX_ATTRIBUTE_COUNT][4];

		memcpy(values, device.attributes, sizeof(values));
		for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
		{
			if (declaration->packed_mask & (1UL << index))
				memset(values[index], 0, sizeof(values[index]));
		}
		if (!puts_this_frame.attributes_valid || memcmp(puts_this_frame.attributes, values, sizeof(values)))
		{
			memcpy(puts_this_frame.attributes, values, sizeof(values));
			puts_this_frame.attributes_ref = data_ref(vk_data_put(values, sizeof(values)));
			puts_this_frame.attributes_valid = TRUE;
		}
		draw->vertex_buffers[bindings] = puts_this_frame.attributes_ref;
		state->bindings[bindings].stride = 0;
		for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
		{
			if (fed[index])
				continue;
			state->attributes[index].format = declaration->packed_mask & (1UL << index) ? VK_VERTEX_FORMAT_R32_UINT :
				VK_VERTEX_FORMAT_R32G32B32A32_SFLOAT;
			state->attributes[index].binding = (uint32_t)bindings;
			state->attributes[index].offset = (uint32_t)(index * 16);
		}
		bindings++;
	}
	state->binding_count = (uint32_t)bindings;
	return TRUE;
}

/* the largest and smallest index of an index list */
static void index_extent(const WORD *indices, unsigned long count, unsigned long *minimum, unsigned long *maximum)
{
	unsigned long index, low = 0xffff, high = 0;

	for (index = 0; index < count; index++)
	{
		if (indices[index] < low)
			low = indices[index];
		if (indices[index] > high)
			high = indices[index];
	}
	*minimum = low;
	*maximum = high;
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

/* a line loop is a line strip with its first vertex again at its end: Vulkan has no loop */
static WORD *loop_indices(const WORD *indices, unsigned long count, unsigned long *out_count)
{
	WORD *result = malloc((count + 1) * sizeof(WORD) + 2);
	unsigned long index;

	for (index = 0; index < count; index++)
		result[index] = indices ? indices[index] : (WORD)index;
	result[count] = result[0];
	*out_count = count + 1;
	return result;
}

/* ---------- textures: bind_textures in d3d8_gl.c */

static unsigned long stage_texture_mode_of(int stage)
{
	return stage_texture_mode(stage);
}

/* the Vulkan address mode for a Direct3D one (address_mode in d3d8_gl.c) */
static uint32_t address_mode(DWORD mode)
{
	switch (mode)
	{
	case D3DTADDRESS_MIRROR: return 1; /* mirrored repeat */
	case D3DTADDRESS_CLAMP:
	case D3DTADDRESS_CLAMPTOEDGE: return 2; /* clamp to edge */
	case D3DTADDRESS_BORDER: return 3; /* clamp to border */
	default: return 0; /* repeat */
	}
}

/* configure_sampler in d3d8_gl.c, as a state the host keeps a VkSampler for. hires: a high-res HUD texture (hud_hires.h),
drawn smaller than it is, so filtered and from its mip levels whatever the game asks */
static void sampler_state_make(int stage, BOOL mipmapped, BOOL hires, struct vk_sampler_state *out)
{
	DWORD *state = D3D__TextureState[stage];
	DWORD min_filter = hires ? D3DTEXF_LINEAR : state[D3DTSS_MINFILTER];
	DWORD mip_filter = hires ? D3DTEXF_LINEAR : mipmapped ? state[D3DTSS_MIPFILTER] : D3DTEXF_NONE;
	DWORD mag_filter = hires ? D3DTEXF_LINEAR : state[D3DTSS_MAGFILTER];
	DWORD maximum_mip_level = hires ? 0 : state[D3DTSS_MAXMIPLEVEL];
	DWORD lod_bias = hires ? 0 : state[D3DTSS_MIPMAPLODBIAS];

	memset(out, 0, sizeof(*out));
	out->min_filter = min_filter == D3DTEXF_POINT ? 0 : 1;
	out->mag_filter = mag_filter == D3DTEXF_POINT ? 0 : 1;
	out->mip_mode = mip_filter == D3DTEXF_NONE ? 0 : mip_filter == D3DTEXF_POINT ? 1 : 2;
	out->address[0] = address_mode(state[D3DTSS_ADDRESSU]);
	out->address[1] = address_mode(state[D3DTSS_ADDRESSV]);
	out->address[2] = address_mode(state[D3DTSS_ADDRESSW]);
	out->border_color = state[D3DTSS_BORDERCOLOR];
	/* Vulkan's sampler has the LOD bias (the shaders do not) and the lowest level used */
	out->lod_bias = dword_to_float(lod_bias);
	out->min_lod = (float)maximum_mip_level;
	out->anisotropy = (min_filter == D3DTEXF_ANISOTROPIC && state[D3DTSS_MAXANISOTROPY] > 1) ?
		(float)state[D3DTSS_MAXANISOTROPY] : 1.0f;
}

/* the mip composite of a texture the game renders a level at a time: sent to the host once for each (data, size, levels) */
#define COMPOSITES_MAXIMUM 256

static struct
{
	unsigned long data, width, height, levels;
} composites_sent[COMPOSITES_MAXIMUM];
static unsigned composites_sent_count;

static void composite_send(unsigned long data, const struct xgpu_texture_description *description)
{
	struct vk_command_composite *command;
	unsigned long index;

	for (index = 0; index < composites_sent_count; index++)
	{
		if (composites_sent[index].data == data && composites_sent[index].width == description->width &&
			composites_sent[index].height == description->height && composites_sent[index].levels == description->levels)
			return;
	}
	if (composites_sent_count == COMPOSITES_MAXIMUM || description->levels > VK_COMPOSITE_LEVELS)
		return;
	composites_sent[composites_sent_count].data = data;
	composites_sent[composites_sent_count].width = description->width;
	composites_sent[composites_sent_count].height = description->height;
	composites_sent[composites_sent_count++].levels = description->levels;
	command = stream_command(VK_COMMAND_COMPOSITE, sizeof(*command));
	command->data = (uint32_t)data;
	command->width = (uint32_t)description->width;
	command->height = (uint32_t)description->height;
	command->levels = (uint32_t)description->levels;
	for (index = 0; index < description->levels; index++)
		command->level_data[index] = (uint32_t)(data + xgpu_texture_level_offset(description, index));
}

/* what each stage samples, and the scale a linear texture's coordinates get */
static void textures_make(struct vk_command_draw *draw, const struct nv2a_pixel_shader_key *key, float texture_scale[4][4])
{
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = device.textures[stage];
		unsigned long mode = stage_texture_mode_of(stage);
		struct vk_draw_texture *out = &draw->textures[stage];

		texture_scale[stage][0] = texture_scale[stage][1] = texture_scale[stage][2] = texture_scale[stage][3] = 1.0f;
		out->sampler_type = key->sampler_type[stage];
		if (!texture || !texture->Data || mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x11)
			continue;
		{
			struct xgpu_texture_description description;
			const D3DCOLOR *palette = device.palettes[stage] && device.palettes[stage]->Data ?
				(const D3DCOLOR *)PLATFORM_PHYSICAL_TO_VIRTUAL(device.palettes[stage]->Data) : NULL;
			unsigned long target_width, target_height;
			int kind;
			uint32_t id;

			/* a texture whose data is a render target's samples the target (render-to-texture); linear ones have their
			coordinates scaled by the target's size */
			if (rendered_find(texture->Data, &target_width, &target_height))
			{
				unsigned long levels = 1;

				xgpu_texture_describe(texture->Format, texture->Size, &description);
				out->kind = VK_TEXTURE_TARGET;
				out->id = texture->Data;
				if (description.linear)
				{
					texture_scale[stage][0] = 1.0f / (float)target_width;
					texture_scale[stage][1] = 1.0f / (float)target_height;
				}
				/* the water renders a texture a mip level at a time, each a surface of its own: the levels are put together
				in a composite (mip_composite_get) */
				if (!description.linear && !description.cube_map && description.levels > 1 &&
					target_width == description.width && target_height == description.height)
				{
					levels = description.levels;
					composite_send(texture->Data, &description);
				}
				out->levels = (uint32_t)levels;
				out->width = (uint32_t)description.width;
				out->height = (uint32_t)description.height;
				sampler_state_make(stage, levels > 1, FALSE, &out->sampler);
				continue;
			}
			id = vk_texture_get((const DWORD *)texture, palette, &kind, &description);
			if (!id)
				continue;
			if (description.linear)
			{
				texture_scale[stage][0] = 1.0f / (float)description.width;
				texture_scale[stage][1] = 1.0f / (float)description.height;
			}
			out->kind = VK_TEXTURE_IMAGE;
			out->id = id;
			sampler_state_make(stage, description.levels > 1, description.hires, &out->sampler);
		}
	}
}

/* a draw: prepare_draw and the draw calls of d3d8_gl.c, as one VK_COMMAND_DRAW. index_data is the game's indices (NULL for
a draw of consecutive vertices from start); immediate is End's draw of the vertices of Begin. */
static void draw_make(D3DPRIMITIVETYPE type, unsigned long count, BOOL immediate, const WORD *index_data, unsigned long start)
{
	struct vk_command_draw record;
	struct nv2a_pixel_shader_key key;
	float texture_scale[4][4];
	unsigned long mask, minimum = 0, maximum = 0, first, vertices, index_count = 0;
	uint32_t vertex, pixel;
	BOOL has_depth = FALSE;
	WORD *made = NULL;
	const WORD *indices = NULL;
	int stage;

	if (!device.video_ready)
		return;
	/* the targets first: the pixel shader's key depends on their samples a pixel */
	if (!targets_bind(&has_depth))
	{
		stats.skipped_no_target++;
		return;
	}
	if (!draw_shaders(immediate, &key, &mask, &vertex, &pixel))
	{
		stats.skipped_no_program++;
		return;
	}
	if (!vertex || !pixel)
	{
		stats.skipped_shader++;
		return;
	}
	memset(&record, 0, sizeof(record));
	textures_make(&record, &key, texture_scale);
	record.vertex_shader = vertex;
	record.pixel_shader = pixel;
	raster_state_make(&record, targets_told.color.kind != VK_SURFACE_NONE, has_depth);
	record.state.topology = topology_of(type);

	/* the indices: the game's (an indexed draw), or made (a quad list, a line loop) */
	if (index_data)
	{
		index_extent(index_data, count, &minimum, &maximum);
		first = device.base_vertex_index + minimum;
		vertices = maximum - minimum + 1;
		indices = index_data;
		index_count = count;
	}
	else
	{
		first = start;
		vertices = count;
	}
	if (type == D3DPT_QUADLIST)
	{
		made = quad_indices(index_data, count, &index_count);
		indices = made;
	}
	else if (type == D3DPT_LINELOOP)
	{
		made = loop_indices(index_data, count, &index_count);
		indices = made;
	}
	if (immediate)
	{
		record.vertex_buffers[0] = data_ref(vk_data_put(device.immediate_vertices,
			count * XGPU_VERTEX_ATTRIBUTE_COUNT * 4 * sizeof(float)));
		record.state.binding_count = 1;
		record.state.bindings[0].stride = XGPU_VERTEX_ATTRIBUTE_COUNT * 4 * sizeof(float);
		for (stage = 0; stage < XGPU_VERTEX_ATTRIBUTE_COUNT; stage++)
		{
			record.state.attributes[stage].format = VK_VERTEX_FORMAT_R32G32B32A32_SFLOAT;
			record.state.attributes[stage].binding = 0;
			record.state.attributes[stage].offset = (uint32_t)stage * 16;
		}
	}
	else if (!vertex_input_make(&record, first, vertices))
	{
		stats.skipped_other++;
		free(made);
		return;
	}
	if (indices)
	{
		if (!index_count)
		{
			free(made);
			return;
		}
		record.indexed = 1;
		record.index_data = data_ref(vk_data_put(indices, index_count * sizeof(WORD)));
		record.vertex_offset = index_data ? -(int32_t)minimum : 0;
		record.count = (uint32_t)index_count;
	}
	else
	{
		record.count = (uint32_t)count;
	}
	free(made);
	uniforms_make(&record, texture_scale, key.per_pixel_lighting != 0);
	{
		struct vk_command_draw *command = stream_command(VK_COMMAND_DRAW, sizeof(*command));

		record.header = command->header;
		*command = record;
	}
	if (immediate)
		stats.immediate_draws++;
	else
		stats.draws++;
}

void WINAPI D3DDevice_DrawVertices(D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT vertex_count)
{
	if (vertex_count)
		draw_make(primitive_type, vertex_count, FALSE, NULL, start_vertex);
}

void WINAPI D3DDevice_DrawIndexedVertices(D3DPRIMITIVETYPE primitive_type, UINT vertex_count, CONST WORD *index_data)
{
	if (vertex_count && index_data)
		draw_make(primitive_type, vertex_count, FALSE, index_data, 0);
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
	unsigned long count = device.immediate_count;

	device.immediate_active = FALSE;
	if (count)
		draw_make(device.immediate_type, count, TRUE, NULL, 0);
	device.immediate_count = 0;
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
	struct vk_command_clear *command;
	uint32_t clear_flags = 0;
	BOOL has_depth = FALSE;
	DWORD index, kept = 0;

	if (!device.video_ready || !targets_bind(&has_depth))
		return;
	if (flags & D3DCLEAR_TARGET)
	{
		/* the Xbox clears the channels named (D3DCLEAR_TARGET_R, _G, _B, _A):
		the fog screen clears only alpha, leaving the picture under the fog */
		clear_flags |= ((flags & D3DCLEAR_TARGET_R) ? VK_CLEAR_RED : 0) | ((flags & D3DCLEAR_TARGET_G) ? VK_CLEAR_GREEN : 0) |
			((flags & D3DCLEAR_TARGET_B) ? VK_CLEAR_BLUE : 0) | ((flags & D3DCLEAR_TARGET_A) ? VK_CLEAR_ALPHA : 0);
	}
	if (has_depth && (flags & D3DCLEAR_ZBUFFER))
		clear_flags |= VK_CLEAR_DEPTH;
	if (has_depth && (flags & D3DCLEAR_STENCIL))
		clear_flags |= VK_CLEAR_STENCIL;
	if (!clear_flags)
		return;
	command = stream_command(VK_COMMAND_CLEAR, sizeof(*command) + (count && rectangles ? count : 1) * 4 * sizeof(uint32_t));
	command->flags = clear_flags;
	color_to_vec4(color, command->color);
	command->depth = z;
	command->stencil = stencil;
	if (!count || !rectangles)
	{
		/* the NV2A clips a viewport-less clear to the viewport, which is what
		keeps a split-screen window's clear from wiping the other window */
		long x0 = target_pixel((float)device.viewport.X, 0);
		long y0 = target_pixel((float)device.viewport.Y, 1);

		command->rectangles[0][0] = (uint32_t)x0;
		command->rectangles[0][1] = (uint32_t)y0;
		command->rectangles[0][2] = (uint32_t)(target_pixel((float)(device.viewport.X + device.viewport.Width), 0) - x0);
		command->rectangles[0][3] = (uint32_t)(target_pixel((float)(device.viewport.Y + device.viewport.Height), 1) - y0);
		command->rectangle_count = 1;
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
		long x0, y0;

		if (left >= right || top >= bottom)
			continue;
		x0 = target_pixel((float)(left + UI_OFFSET), 0);
		y0 = target_pixel((float)top, 1);
		command->rectangles[kept][0] = (uint32_t)x0;
		command->rectangles[kept][1] = (uint32_t)y0;
		command->rectangles[kept][2] = (uint32_t)(target_pixel((float)(right + UI_OFFSET), 0) - x0);
		command->rectangles[kept][3] = (uint32_t)(target_pixel((float)bottom, 1) - y0);
		kept++;
	}
	command->rectangle_count = kept;
}

/* ---------- presentation */

void WINAPI D3DDevice_Present(CONST RECT *source_rectangle, CONST RECT *destination_rectangle,
	void *unused, void *unused2)
{
	(void)source_rectangle;
	(void)destination_rectangle;
	(void)unused;
	(void)unused2;
	if (device.video_ready)
	{
		struct vk_command_present *command = stream_command(VK_COMMAND_PRESENT, sizeof(*command));
		float scale[2];

		surface_describe(&device.back_buffer, FALSE, &command->back_buffer, scale);
		stream_flush();
		/* the host starts the next frame with nothing bound, and with no data */
		targets_known = FALSE;
		data_id = 0;
		puts_reset();
		vk_texture_cache_begin_frame();
		stats.presents++;
		if (!statistics_read)
		{
			statistics_read = TRUE;
			statistics_on = config_boolean("debug.gpu_stats");
		}
		if (statistics_on && stats.presents >= 60)
		{
			platform_log("frame %lu: %lu draws, %lu immediate; skipped %lu no program, %lu no target, %lu shader not ready, "
				"%lu textured, %lu other", device.frame, stats.draws / stats.presents, stats.immediate_draws / stats.presents,
				stats.skipped_no_program, stats.skipped_no_target, stats.skipped_shader, stats.skipped_texture,
				stats.skipped_other);
			memset(&stats, 0, sizeof(stats));
		}
	}
	dump_present();
	/* the stand-in window's swap: holds the frame to the display's rate until the
	host's swapchain presents (host_vk_presenting), after which it does */
	platform_video_swap();
	device.frame++;
	platform_pump_events();

	pthread_mutex_lock(&vertical_blank_lock);
	/* the Xbox keeps at most two frames queued behind its 60 Hz display;
	with interpolation, frames come at the real display's rate instead */
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
