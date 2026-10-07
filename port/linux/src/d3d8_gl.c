/*
D3D8_GL.C

The OpenGL 4.5 renderer of the Xbox Direct3D 8 device (xgpu_device.h; OpenGL
ES 3 on Android).

At each draw the device's state (d3d8_device.c) is read back and translated:
the vertex program into GLSL once per shader (nv2a_vsh.c), the pixel shader
- texture stages and register combiners, 57 render states - into GLSL once
per combination (nv2a_psh.c), and the rest into GL state. glClipControl
(GL_UPPER_LEFT, GL_ZERO_TO_ONE) makes GL's clip space agree with D3D's, so
viewports, scissors and texture rows line up with D3D's top-left origin; the
window blit at Present flips the image back for display.
*/

#include "xgpu_device.h"
#include "xgpu_gl.h"
#include "sdl_platform.h"
#include "port_config.h"
#include "screenshot.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83f1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83f2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83f3
#endif

/* the device's, as this file has always named them (xgpu_device.h) */
#define device xgpu_device
#define stats xgpu_statistics
#define debug_settings xgpu_debug_settings
#define render_target_get xgpu_render_target_get
#define trace_frame xgpu_trace_frame
#define trace_draw xgpu_trace_draw
#define index_extent xgpu_index_extent
#define quad_indices xgpu_quad_indices
#define anti_aliasing xgpu_anti_aliasing
#define anti_aliasing_samples xgpu_anti_aliasing_samples()
#define UI_OFFSET ((GLint)xgpu_ui_offset())
#define constant_serials xgpu_constant_serials.registers
#define constant_log xgpu_constant_serials.log
#define CONSTANT_LOG_SIZE XGPU_CONSTANT_LOG_SIZE
#define constants_checkpoint_serial xgpu_constant_serials.checkpoint_serial
#define constants_checkpoint_first xgpu_constant_serials.checkpoint_first
#define constants_checkpoint_last xgpu_constant_serials.checkpoint_last
#define draw_uniforms_serial xgpu_draw_uniforms_serial

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
	/* the lights of a draw lit for each pixel (XGPU_MODEL_LIGHT_COUNT), and
	the constants' serial at their last upload */
	GLint model_lights;
	unsigned long long model_lights_serial;

	/* the vertex constants c[0..constant_count) the program uses; with
	consecutive locations, a changed range is uploaded by itself */
	unsigned long constant_count;
	BOOL constants_consecutive;
	/* the constants' serial at the program's last constant upload (constants_store) */
	unsigned long long constants_serial;
	/* draw_uniforms_serial when the uniforms below were brought up to date */
	unsigned long uniforms_serial;
	/* what the program's other uniforms hold (all ones: unknown) */
	struct draw_uniforms uniforms;
};

#define FRAGMENT_BUCKETS 1024
#define PROGRAM_BUCKETS 1024

static struct fragment_entry *fragment_buckets[FRAGMENT_BUCKETS];
static struct program_entry *program_buckets[PROGRAM_BUCKETS];

/* ---------- render targets */

struct framebuffer_entry
{
	struct framebuffer_entry *next;
	GLuint color;
	GLuint depth;
	/* color and depth are multisampled renderbuffers, not textures */
	BOOL renderbuffers;
	GLuint framebuffer;
};

static struct framebuffer_entry *framebuffers;

/* ---------- the renderer */

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
#ifdef HALO_ANDROID
#define VISIBILITY_QUERY GL_ANY_SAMPLES_PASSED
#define VISIBILITY_ALL_SAMPLES 1000000
#else
#define VISIBILITY_QUERY GL_SAMPLES_PASSED
#endif

static struct
{
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
	/* the mirror's segments (xgpu_device.h), made when first needed */
	GLuint mirror_buffers[MIRROR_SEGMENT_COUNT];

	GLuint queries[VISIBILITY_TEST_SLOTS];
	BOOL query_pending[VISIBILITY_TEST_SLOTS];
	/* the pixels each of the game's pixels covered in the test's target
	(render_target_get), which its count is divided by */
	float query_area[VISIBILITY_TEST_SLOTS];
	GLuint active_query;
#ifdef HALO_ANDROID
	/* with atomic counters: one counter per test, used as a ring; the
	counter a test ended in, per result slot */
	GLuint visibility_counters;
	unsigned long counter_next;
	unsigned long counter_active;
	unsigned long counter_of_slot[VISIBILITY_TEST_SLOTS];
#else
	/* each test's latest result, which the GPU writes (as a query buffer)
	when the test's draws are done: the game waits for results at the start
	of the next frame, and a query would stop the CPU there until the GPU
	had caught up */
	GLuint visibility_results_buffer;
	volatile GLuint *visibility_results;
	/* a pipeline flush every flush_every draws (draw_flush), 0 never */
	unsigned long flush_every;
	unsigned long flush_draws;
#endif
} gl;

/* the GL target of a texture of type (xgpu.h) */
static GLenum gl_texture_target(int type)
{
	return type == _xgpu_texture_cube ? GL_TEXTURE_CUBE_MAP : type == _xgpu_texture_3d ? GL_TEXTURE_3D : GL_TEXTURE_2D;
}

/* ---------- GL state cache

Consecutive draws share most of their state, but each sets all of it: the
setters here skip the call when GL already holds the value. Code that
changes GL state behind the cache's back (clears, presentation, texture
uploads, render target and framebuffer creation) calls
xgpu_gl_state_invalidate, after which every value is set again. Unknown
values are all ones, which no real value matches (floats become NaN, which
compares unequal to everything). */

#ifdef HALO_ANDROID
struct attribute_pointer
{
	GLuint buffer;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLboolean integer;
	GLsizei stride;
	unsigned long offset;
};
#else
/* desktop GL (4.3) separates an attribute's format from the buffer it
reads: the attributes of a stream share one binding, so a draw that moves
the stream rebinds it once instead of pointing each attribute again */
struct attribute_format
{
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLboolean integer;
	GLuint relative_offset;
	GLuint binding;
};

struct vertex_binding
{
	GLuint buffer;
	unsigned long offset;
	GLsizei stride;
};

/* a binding per stream (setup_streams); GL has at least 16 */
#define VERTEX_BINDING_COUNT 16
#endif

static struct
{
	GLuint program;
	GLuint framebuffer;
	GLint viewport[4];
	GLint scissor[4];
	float depth_range[2];
	unsigned char depth_test, stencil_test, blend, cull_face, offset_fill, offset_line;
	unsigned char scissor_test;
	GLenum depth_function;
	unsigned char depth_mask;
	GLenum stencil_function;
	GLint stencil_reference;
	GLuint stencil_value_mask;
	GLenum stencil_operations[3];
	GLuint stencil_write_mask;
	GLenum blend_source, blend_destination, blend_equation;
	float blend_color[4];
	unsigned char color_mask;
	GLenum front_face, cull_mode, polygon_mode;
	float polygon_offset[2];
	GLenum active_texture;
	/* per unit: the GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP and GL_TEXTURE_3D
	bindings */
	GLuint textures[D3DTSS_MAXSTAGES][3];
	GLuint samplers[D3DTSS_MAXSTAGES];
	GLuint array_buffer;
	GLuint element_array_buffer;
	unsigned char attribute_enabled[XGPU_VERTEX_ATTRIBUTE_COUNT];
#ifdef HALO_ANDROID
	struct attribute_pointer attribute_pointers[XGPU_VERTEX_ATTRIBUTE_COUNT];
#else
	struct attribute_format attribute_formats[XGPU_VERTEX_ATTRIBUTE_COUNT];
	struct vertex_binding vertex_bindings[VERTEX_BINDING_COUNT];
#endif
	/* a disabled attribute's value; kind 1 is the integer zero */
	unsigned char attribute_value_kind[XGPU_VERTEX_ATTRIBUTE_COUNT];
	float attribute_values[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
} gl_state;

void xgpu_gl_state_invalidate(void)
{
	memset(&gl_state, 0xff, sizeof(gl_state));
}

static void state_enable(unsigned char *shadow, GLenum capability, BOOL enabled)
{
	unsigned char value = enabled ? 1 : 0;

	if (*shadow == value)
		return;
	*shadow = value;
	if (value)
		glEnable(capability);
	else
		glDisable(capability);
}

static void state_program(GLuint program)
{
	if (gl_state.program != program)
	{
		gl_state.program = program;
		glUseProgram(program);
	}
}

static void state_framebuffer(GLuint framebuffer)
{
	if (gl_state.framebuffer != framebuffer)
	{
		gl_state.framebuffer = framebuffer;
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	}
}

static void state_texture(int unit, GLenum target, GLuint texture)
{
	int slot = target == GL_TEXTURE_CUBE_MAP ? 1 : target == GL_TEXTURE_3D ? 2 : 0;

	if (gl_state.textures[unit][slot] == texture)
		return;
	if (gl_state.active_texture != GL_TEXTURE0 + (GLenum)unit)
	{
		gl_state.active_texture = GL_TEXTURE0 + (GLenum)unit;
		glActiveTexture(gl_state.active_texture);
	}
	gl_state.textures[unit][slot] = texture;
	glBindTexture(target, texture);
}

static void state_sampler(int unit, GLuint sampler)
{
	if (gl_state.samplers[unit] != sampler)
	{
		gl_state.samplers[unit] = sampler;
		glBindSampler((GLuint)unit, sampler);
	}
}

static void state_array_buffer(GLuint buffer)
{
	if (gl_state.array_buffer != buffer)
	{
		gl_state.array_buffer = buffer;
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
	}
}

static void state_element_array_buffer(GLuint buffer)
{
	if (gl_state.element_array_buffer != buffer)
	{
		gl_state.element_array_buffer = buffer;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	}
}

/* enables the attribute, reading size elements of type from buffer: each
vertex is stride bytes on from the one before it, starting at
buffer_offset, with the attribute relative_offset bytes into it.
Attributes given the same binding share the buffer, its offset and its
stride (on desktop GL; ES points each attribute on its own). */
static void state_attribute_stream(GLuint index, GLuint binding, GLuint buffer, GLint size, GLenum type,
	GLboolean normalized, BOOL integer, GLsizei stride, unsigned long buffer_offset, unsigned long relative_offset)
{
#ifdef HALO_ANDROID
	struct attribute_pointer *pointer = &gl_state.attribute_pointers[index];
	unsigned long offset = buffer_offset + relative_offset;

	(void)binding;
	if (gl_state.attribute_enabled[index] != 1)
	{
		gl_state.attribute_enabled[index] = 1;
		glEnableVertexAttribArray(index);
	}
	if (pointer->buffer == buffer && pointer->size == size && pointer->type == type &&
		pointer->normalized == normalized && pointer->integer == (integer ? GL_TRUE : GL_FALSE) &&
		pointer->stride == stride && pointer->offset == offset)
	{
		return;
	}
	state_array_buffer(buffer);
	if (integer)
		glVertexAttribIPointer(index, size, type, stride, (const void *)offset);
	else
		glVertexAttribPointer(index, size, type, normalized, stride, (const void *)offset);
	pointer->buffer = buffer;
	pointer->size = size;
	pointer->type = type;
	pointer->normalized = normalized;
	pointer->integer = integer ? GL_TRUE : GL_FALSE;
	pointer->stride = stride;
	pointer->offset = offset;
#else
	struct attribute_format *format = &gl_state.attribute_formats[index];
	struct vertex_binding *vertex_binding = &gl_state.vertex_bindings[binding];

	if (gl_state.attribute_enabled[index] != 1)
	{
		gl_state.attribute_enabled[index] = 1;
		glEnableVertexAttribArray(index);
	}
	if (format->size != size || format->type != type || format->normalized != normalized ||
		format->integer != (integer ? GL_TRUE : GL_FALSE) || format->relative_offset != relative_offset)
	{
		if (integer)
			glVertexAttribIFormat(index, size, type, (GLuint)relative_offset);
		else
			glVertexAttribFormat(index, size, type, normalized, (GLuint)relative_offset);
		format->size = size;
		format->type = type;
		format->normalized = normalized;
		format->integer = integer ? GL_TRUE : GL_FALSE;
		format->relative_offset = (GLuint)relative_offset;
	}
	if (format->binding != binding)
	{
		glVertexAttribBinding(index, binding);
		format->binding = binding;
	}
	if (vertex_binding->buffer != buffer || vertex_binding->offset != buffer_offset || vertex_binding->stride != stride)
	{
		glBindVertexBuffer(binding, buffer, (GLintptr)buffer_offset, stride);
		vertex_binding->buffer = buffer;
		vertex_binding->offset = buffer_offset;
		vertex_binding->stride = stride;
	}
#endif
}

/* disables the attribute, which then reads value, or the integer zero */
static void state_attribute_value(GLuint index, const float *value)
{
	unsigned char kind = value ? 0 : 1;

	if (gl_state.attribute_enabled[index] != 0)
	{
		gl_state.attribute_enabled[index] = 0;
		glDisableVertexAttribArray(index);
	}
	if (gl_state.attribute_value_kind[index] == kind &&
		(!value || !memcmp(gl_state.attribute_values[index], value, sizeof(gl_state.attribute_values[index]))))
	{
		return;
	}
	gl_state.attribute_value_kind[index] = kind;
	if (value)
	{
		memcpy(gl_state.attribute_values[index], value, sizeof(gl_state.attribute_values[index]));
		glVertexAttrib4fv(index, value);
	}
	else
	{
		glVertexAttribI4ui(index, 0, 0, 0, 0);
	}
}

/* ---------- GL helpers */

GLuint xgpu_compile_shader(GLenum type, const char *source, const char *what)
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

GLuint xgpu_link_program(GLuint vertex_shader, GLuint fragment_shader, const char *what)
{
	GLuint program = glCreateProgram();
	GLint status = 0;

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		platform_log("cannot link the %s program: %s", what, log);
		return 0;
	}
	return program;
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

/* the framebuffer of these textures, or with renderbuffers, of these
multisampled renderbuffers (render_target_multisample) */
static GLuint framebuffer_find(GLuint color, GLuint depth, BOOL renderbuffers)
{
	struct framebuffer_entry *entry, **link;
	GLenum draw_buffer = color ? GL_COLOR_ATTACHMENT0 : GL_NONE;

	/* (found, it moves to the front: each draw asks again for the one
	the draw before it did) */
	for (link = &framebuffers; (entry = *link) != NULL; link = &entry->next)
	{
		if (entry->color == color && entry->depth == depth && entry->renderbuffers == renderbuffers)
		{
			*link = entry->next;
			entry->next = framebuffers;
			framebuffers = entry;
			return entry->framebuffer;
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->color = color;
	entry->depth = depth;
	entry->renderbuffers = renderbuffers;
	glGenFramebuffers(1, &entry->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, entry->framebuffer);
	if (renderbuffers)
	{
		if (color)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
		if (depth)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
	}
	else
	{
		if (color)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
		if (depth)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	}
	glDrawBuffers(1, &draw_buffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		platform_log("framebuffer %u/%u%s is incomplete", color, depth, renderbuffers ? " (multisampled)" : "");
	xgpu_gl_state_invalidate();
	entry->next = framebuffers;
	framebuffers = entry;
	return entry->framebuffer;
}

static GLuint framebuffer_get(GLuint color, GLuint depth)
{
	return framebuffer_find(color, depth, FALSE);
}

/* ---------- multisampling

With display.anti_aliasing's multisampling, the back buffer and its depth
buffer are drawn into multisampled renderbuffers, and so is any target drawn
together with one of them (the mirror's view goes to the secondary target
with the back buffer's depth buffer): a framebuffer's attachments are all
multisampled or none is, so that of a target's texture and its renderbuffer
only one has pixels drawn since the other had them. The renderbuffer's
pixels are resolved into the texture before anything reads the texture (a
draw's textures, the display blit), and the texture's are put into the
renderbuffer when it is made, or made again for other samples. Nothing
samples a depth buffer as a texture (the game's copy of the depth buffer's
memory is a target of its own), so a depth buffer is resolved only when it
stops being multisampled. */

/* the samples a pixel of the bound targets: their renderbuffers' (or 1) */
static int target_samples = 1;

/* the multisampled pixels of a target drawn into since into its texture */
static void render_target_resolve(struct xgpu_render_target *target)
{
	GLuint read, draw;

	if (!target->unresolved)
		return;
	target->unresolved = FALSE;
	/* (both found first: making a framebuffer binds it) */
	read = target->depth ? framebuffer_find(0, target->multisample, TRUE) :
		framebuffer_find(target->multisample, 0, TRUE);
	draw = target->depth ? framebuffer_get(0, target->texture) : framebuffer_get(target->texture, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(0, 0, (GLint)target->pixel_width, (GLint)target->pixel_height,
		0, 0, (GLint)target->pixel_width, (GLint)target->pixel_height,
		target->depth ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT, GL_NEAREST);
	xgpu_gl_state_invalidate();
}

/* a target's renderbuffer with these samples a pixel (0: none, its storage
a pixel, the renderbuffer kept for the framebuffers made of it): its pixels
resolved into its texture first, and the texture's put into the new
storage */
static void render_target_multisample(struct xgpu_render_target *target, int samples)
{
	GLuint draw;

	if (target->samples == samples)
		return;
	render_target_resolve(target);
	if (!target->multisample)
		glGenRenderbuffers(1, &target->multisample);
	glBindRenderbuffer(GL_RENDERBUFFER, target->multisample);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, target->depth ? GL_DEPTH24_STENCIL8 : GL_RGBA8,
		samples ? (GLsizei)target->pixel_width : 1, samples ? (GLsizei)target->pixel_height : 1);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	target->samples = samples;
	if (!samples)
		return;
	draw = target->depth ? framebuffer_find(0, target->multisample, TRUE) :
		framebuffer_find(target->multisample, 0, TRUE);
#ifdef HALO_ANDROID
	/* (ES blits into no multisampled framebuffer: it is cleared instead. A
	change of the setting is taken up between frames, and the frame clears
	the screen's targets before it draws) */
	glBindFramebuffer(GL_FRAMEBUFFER, draw);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(0xff);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClearStencil(0);
	glClear(target->depth ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT);
#else
	{
		GLuint read = target->depth ? framebuffer_get(0, target->texture) : framebuffer_get(target->texture, 0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
		glDisable(GL_SCISSOR_TEST);
		glBlitFramebuffer(0, 0, (GLint)target->pixel_width, (GLint)target->pixel_height,
			0, 0, (GLint)target->pixel_width, (GLint)target->pixel_height,
			target->depth ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
#endif
	xgpu_gl_state_invalidate();
}

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

/* binds the framebuffer for the current targets; returns FALSE if there is
nothing to draw into */
static BOOL bind_targets(BOOL *has_depth)
{
	struct render_target_entry *color = render_target_get(device.render_target);
	struct render_target_entry *depth = render_target_get(device.depth_stencil);
	int samples;

	if (depth && !depth->target.depth)
		depth = NULL;
	if (!color && !depth)
		return FALSE;
	if (color)
		color->last_rendered = device.frame + 1;
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
	if (color)
		render_target_multisample(&color->target, samples);
	if (depth)
		render_target_multisample(&depth->target, samples);
	if (samples)
	{
		state_framebuffer(framebuffer_find(color ? color->target.multisample : 0,
			depth ? depth->target.multisample : 0, TRUE));
		if (color)
			color->target.unresolved = TRUE;
		if (depth)
			depth->target.unresolved = TRUE;
		target_samples = samples;
	}
	else
	{
		state_framebuffer(framebuffer_get(color ? color->target.texture : 0, depth ? depth->target.texture : 0));
		target_samples = 1;
	}
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
		xgpu_capabilities.base_vertex = es32;
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
	if (config_boolean("debug.gl_debug"))
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(gl_debug_callback, NULL);
	}
	glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
	glEnable(GL_PROGRAM_POINT_SIZE);
#endif
	glGenVertexArrays(1, &gl.vertex_array);
	glBindVertexArray(gl.vertex_array);
#ifdef HALO_ANDROID
	{
		int ring;

		glGenBuffers(STREAM_BUFFER_RING, gl.stream_buffers);
		glGenBuffers(STREAM_BUFFER_RING, gl.index_buffers);
		for (ring = 0; ring < STREAM_BUFFER_RING; ring++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, gl.stream_buffers[ring]);
			glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.index_buffers[ring]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		}
		gl.stream_buffer = gl.stream_buffers[0];
		gl.index_buffer = gl.index_buffers[0];
	}
#endif
#ifndef HALO_ANDROID
	glGenBuffers(1, &gl.stream_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, gl.stream_buffer);
	glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
	glGenBuffers(1, &gl.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
#endif
	glGenSamplers(D3DTSS_MAXSTAGES, gl.samplers);
	glGenQueries(VISIBILITY_TEST_SLOTS, gl.queries);
#ifndef HALO_ANDROID
	glGenBuffers(1, &gl.visibility_results_buffer);
	glBindBuffer(GL_QUERY_BUFFER, gl.visibility_results_buffer);
	glBufferStorage(GL_QUERY_BUFFER, VISIBILITY_TEST_SLOTS * sizeof(GLuint), NULL,
		GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	gl.visibility_results = glMapBufferRange(GL_QUERY_BUFFER, 0, VISIBILITY_TEST_SLOTS * sizeof(GLuint),
		GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	if (!gl.visibility_results)
		platform_log("cannot map the visibility test results; tests wait for the GPU");
	{
		long every = config_integer("debug.gpu_flush_draws");
		const char *renderer = (const char *)glGetString(GL_RENDERER);

		if (every < 0)
			every = renderer && strstr(renderer, "Mesa Intel") ? 3 : 0;
		if (every > 0)
		{
			gl.flush_every = (unsigned long)every;
			platform_log("GPU: a pipeline flush every %ld draws", every);
		}
	}
#endif
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		glGenBuffers(1, &gl.visibility_counters);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl.visibility_counters);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, VISIBILITY_TEST_SLOTS * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}
#endif
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
		glVertexAttrib4fv(index, device.attributes[index]);
	{
		GLint samples = 0, texture_size = 0, renderbuffer_size = 0;

		glGetIntegerv(GL_MAX_SAMPLES, &samples);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size);
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &renderbuffer_size);
		device.maximum_samples = samples;
		device.maximum_target_size = renderbuffer_size < texture_size ? renderbuffer_size : texture_size;
	}
	xgpu_gl_state_invalidate();
}

static BOOL gl_backend_initialize(unsigned long width, unsigned long height)
{
	if (!platform_video_initialize(width, height))
		return FALSE;
	gl_initialize();
	return TRUE;
}

/* what display.anti_aliasing's mode needs of the GL context: the passes'
programs, built as the value is chosen rather than in the middle of a frame
(SMAA's are large) */
static void gl_anti_aliasing_prepare(int mode)
{
	if ((mode == _anti_aliasing_fxaa || mode == _anti_aliasing_smaa) && !xgpu_post_prepare(mode == _anti_aliasing_smaa))
		platform_log("anti-aliasing: its programs do not build, so the 3D view is not antialiased");
	xgpu_gl_state_invalidate();
}

/* ---------- visibility (occlusion) tests */

static void gl_visibility_begin(void)
{
	/* the query object is chosen when the test ends; use a scratch one */
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		const GLuint zero = 0;

		gl.counter_next = (gl.counter_next + 1) % VISIBILITY_TEST_SLOTS;
		gl.counter_active = gl.counter_next;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl.visibility_counters);
		host_gl_buffer_write(GL_ATOMIC_COUNTER_BUFFER, (unsigned int)(gl.counter_active * sizeof(GLuint)),
			sizeof(zero), &zero);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		return;
	}
#endif
	glBeginQuery(VISIBILITY_QUERY, gl.queries[0]);
}

static void gl_visibility_end(DWORD index)
{
	GLuint scratch;

#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		gl.counter_of_slot[index] = gl.counter_active;
		gl.query_pending[index] = TRUE;
		return;
	}
#endif
	glEndQuery(VISIBILITY_QUERY);
	/* the target's samples to a game pixel (its pixels, by its samples a
	pixel with multisampling): the result is a count of the game's pixels
	(visibility_unscaled), which the game divides by its own test's area
	(lens flares, rasterizer_lights.c), a split-screen window's or the
	screen's alike */
	gl.query_area[index] = target_scale[0] * target_scale[1] * (float)target_samples;
	/* swap the scratch query into the requested slot */
	scratch = gl.queries[0];
	gl.queries[0] = gl.queries[index];
	gl.queries[index] = scratch;
	gl.query_pending[index] = TRUE;
#ifndef HALO_ANDROID
	if (gl.visibility_results)
	{
		/* the GPU writes the count into the slot once it is known (given
		by name: Mesa's GL thread waits for everything before a
		glGetQueryObjectuiv, even one into a bound buffer) */
		glGetQueryBufferObjectuiv(gl.queries[index], gl.visibility_results_buffer, GL_QUERY_RESULT,
			(GLintptr)(index * sizeof(GLuint)));
	}
#endif
}

#ifndef HALO_ANDROID
/* a count of pixels in the game's pixels */
static GLuint visibility_unscaled(GLuint samples, DWORD index)
{
	float area = gl.query_area[index];

	return area > 1.0f ? (GLuint)(samples / area + 0.5f) : samples;
}

#endif
#ifndef HALO_ANDROID
/* debug.visibility_wait: the test's own count, waited for (the frames the
same whatever the renderer, as tools/render_test.py compares them; ES's
results are always waited for) */
static BOOL visibility_wait(void)
{
	static int wait = -1;

	if (wait < 0)
		wait = config_boolean("debug.visibility_wait") != 0;
	return wait;
}

#endif

static HRESULT gl_visibility_result(DWORD index, UINT *result)
{
	GLuint available = 0, samples = 0;

	if (!gl.query_pending[index])
	{
		if (result)
			*result = 0;
		return S_OK;
	}
#ifdef HALO_ANDROID
	if (xgpu_capabilities.atomic_counters)
	{
		/* reading the buffer waits for the draws that counted */
		samples = host_gl_read_buffer_word(gl.visibility_counters,
			(unsigned int)(gl.counter_of_slot[index] * sizeof(GLuint)));
		if (result)
			*result = samples;
		return S_OK;
	}
#endif
#ifndef HALO_ANDROID
	if (gl.visibility_results && visibility_wait())
	{
		/* the test's own count, waited for (a query buffer bound would take
		it in place of samples) */
		glBindBuffer(GL_QUERY_BUFFER, 0);
		glGetQueryObjectuiv(gl.queries[index], GL_QUERY_RESULT, &samples);
		glBindBuffer(GL_QUERY_BUFFER, gl.visibility_results_buffer);
		if (result)
			*result = visibility_unscaled(samples, index);
		return S_OK;
	}
	if (gl.visibility_results)
	{
		/* the latest count the GPU has written: from this test, or while
		the GPU is still behind, from the slot's earlier ones */
		if (result)
			*result = visibility_unscaled(gl.visibility_results[index], index);
		return S_OK;
	}
#endif
	glGetQueryObjectuiv(gl.queries[index], GL_QUERY_RESULT_AVAILABLE, &available);
	if (!available)
		return D3DERR_TESTINCOMPLETE;
	glGetQueryObjectuiv(gl.queries[index], GL_QUERY_RESULT, &samples);
#ifdef HALO_ANDROID
	/* ES only says whether any sample passed. The game divides the count by
	the test's area (lens flare brightness, rasterizer_lights.c): report
	more than any test covers, well below what would overflow there. */
	if (samples)
		samples = VISIBILITY_ALL_SAMPLES;
#else
	samples = visibility_unscaled(samples, index);
#endif
	if (result)
		*result = samples;
	return S_OK;
}

/* ---------- program cache */

/* lit: the shader that hands the lighting's normal and position on
(vertex_shader_object lit_shader) */
static GLuint vertex_shader_get(struct vertex_shader_object *program, BOOL immediate, BOOL lit)
{
	int variant = immediate ? 1 : 0;
	GLuint *shader = lit ? &program->lit_shader[variant] : &program->shader[variant];

	if (!*shader)
	{
		char *source = nv2a_vertex_shader_to_glsl(program->instructions, program->instruction_count,
			immediate ? 0 : device.vertex_shader->packed_mask, lit ? &program->lighting : NULL);

		*shader = xgpu_compile_shader(GL_VERTEX_SHADER, source, "vertex");
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
			/* and the same program in HLSL (the Direct3D 12 renderer's) */
			{
				struct nv2a_vertex_inputs inputs;
				char *hlsl;

				xgpu_vertex_inputs(immediate, &inputs);
				hlsl = nv2a_vertex_shader_to_hlsl(program->instructions, program->instruction_count, &inputs,
					lit ? &program->lighting : NULL);
				snprintf(path, sizeof(path), "%s/vs%03lu_%d%s.hlsl", debug_settings.dump_shaders, program->id, variant,
					lit ? "_lit" : "");
				if (hlsl && (file = fopen(path, "w")) != NULL)
				{
					fputs(hlsl, file);
					fclose(file);
				}
				free(hlsl);
			}
		}
		free(source);
	}
	return *shader;
}

typedef char pixel_shader_key_size_assert[sizeof(struct nv2a_pixel_shader_key) % 4 == 0 ? 1 : -1];

static GLuint fragment_shader_get(const struct nv2a_pixel_shader_key *key)
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
	source = nv2a_pixel_shader_to_glsl(key);
	entry->shader = xgpu_compile_shader(GL_FRAGMENT_SHADER, source, "pixel");
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
		/* and the same shader in HLSL (the Direct3D 12 renderer's) */
		{
			char *hlsl = nv2a_pixel_shader_to_hlsl(key);

			snprintf(path, sizeof(path), "%s/ps_%08lx.hlsl", debug_settings.dump_shaders, hash);
			if (hlsl && (file = fopen(path, "w")) != NULL)
			{
				fputs(hlsl, file);
				fclose(file);
			}
			free(hlsl);
		}
	}
	free(source);
	entry->next = *bucket;
	*bucket = entry;
	recent[recent_next++ % RECENT_FRAGMENT_COUNT] = entry;
	return entry->shader;
}

static struct program_entry *program_get(GLuint vertex_shader, GLuint fragment_shader)
{
	static struct program_entry *last;
	unsigned long hash = (vertex_shader * 2654435761UL) ^ fragment_shader;
	struct program_entry **bucket = &program_buckets[hash % PROGRAM_BUCKETS];
	struct program_entry *entry;
	int stage;

	if (last && last->vertex_shader == vertex_shader && last->fragment_shader == fragment_shader)
		return last;
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->vertex_shader == vertex_shader && entry->fragment_shader == fragment_shader)
		{
			if (!entry->program)
				return NULL;
			last = entry;
			return entry;
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->vertex_shader = vertex_shader;
	entry->fragment_shader = fragment_shader;
	memset(&entry->uniforms, 0xff, sizeof(entry->uniforms));
	entry->next = *bucket;
	*bucket = entry;
	if (!vertex_shader || !fragment_shader)
		return NULL;

	entry->program = xgpu_link_program(vertex_shader, fragment_shader, "shader");
	if (!entry->program)
		return NULL;
	state_program(entry->program);
	entry->constants = glGetUniformLocation(entry->program, "c");
	entry->constant_count = XGPU_VERTEX_CONSTANT_COUNT;
	if (entry->constants >= 0)
	{
		unsigned long index;

		/* c[i] is usually at c's location plus i, and the compiler may
		drop registers past the last one the program reads */
		entry->constants_consecutive = TRUE;
		for (index = 1; index < XGPU_VERTEX_CONSTANT_COUNT; index++)
		{
			char name[16];
			GLint location;

			snprintf(name, sizeof(name), "c[%lu]", index);
			location = glGetUniformLocation(entry->program, name);
			if (location < 0)
			{
				entry->constant_count = index;
				break;
			}
			if (location != entry->constants + (GLint)index)
			{
				entry->constants_consecutive = FALSE;
				entry->constant_count = XGPU_VERTEX_CONSTANT_COUNT;
				break;
			}
		}
	}
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
	entry->model_lights = glGetUniformLocation(entry->program, "model_lights");
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		char name[8];

		snprintf(name, sizeof(name), "tex%d", stage);
		glUniform1i(glGetUniformLocation(entry->program, name), stage);
	}
	last = entry;
	return entry;
}

/* ---------- per-draw state */

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

/* hires: a high-res HUD texture (hud_hires.h), drawn smaller than it is, so
filtered and from its mip levels whatever the game asks: the HUD's meters are
point sampled for one player, to keep the Xbox bitmaps' texels sharp */
static void configure_sampler(int stage, BOOL mipmapped, BOOL hires)
{
	/* the texture stage state each sampler was last configured from */
	static DWORD configured[D3DTSS_MAXSTAGES][11];
	static BOOL configured_valid[D3DTSS_MAXSTAGES];
	GLuint sampler = gl.samplers[stage];
	DWORD *state = D3D__TextureState[stage];
	DWORD min_filter = hires ? D3DTEXF_LINEAR : state[D3DTSS_MINFILTER];
	DWORD mip_filter = hires ? D3DTEXF_LINEAR : mipmapped ? state[D3DTSS_MIPFILTER] : D3DTEXF_NONE;
	DWORD mag_filter = hires ? D3DTEXF_LINEAR : state[D3DTSS_MAGFILTER];
	DWORD maximum_mip_level = hires ? 0 : state[D3DTSS_MAXMIPLEVEL];
	DWORD lod_bias = hires ? 0 : state[D3DTSS_MIPMAPLODBIAS];
	GLenum minification;
	float border[4];
	DWORD inputs[11];

	inputs[0] = min_filter;
	inputs[1] = mip_filter;
	inputs[2] = mag_filter;
	inputs[3] = state[D3DTSS_ADDRESSU];
	inputs[4] = state[D3DTSS_ADDRESSV];
	inputs[5] = state[D3DTSS_ADDRESSW];
	inputs[6] = lod_bias;
	inputs[7] = maximum_mip_level;
	inputs[8] = state[D3DTSS_MAXANISOTROPY];
	inputs[9] = state[D3DTSS_BORDERCOLOR];
	inputs[10] = hires;
	if (configured_valid[stage] && !memcmp(configured[stage], inputs, sizeof(inputs)))
		return;
	memcpy(configured[stage], inputs, sizeof(inputs));
	configured_valid[stage] = TRUE;

	if (min_filter == D3DTEXF_POINT)
		minification = mip_filter == D3DTEXF_NONE ? GL_NEAREST :
			mip_filter == D3DTEXF_POINT ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
	else
		minification = mip_filter == D3DTEXF_NONE ? GL_LINEAR :
			mip_filter == D3DTEXF_POINT ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, (GLint)minification);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, mag_filter == D3DTEXF_POINT ? GL_NEAREST : GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, (GLint)address_mode(state[D3DTSS_ADDRESSU]));
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, (GLint)address_mode(state[D3DTSS_ADDRESSV]));
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, (GLint)address_mode(state[D3DTSS_ADDRESSW]));
#ifdef HALO_ANDROID
	/* ES has no sampler LOD bias; the pixel shader applies it
	(texture_lod_bias) */
	glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, (float)maximum_mip_level);
	if (xgpu_capabilities.anisotropy)
		glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			(min_filter == D3DTEXF_ANISOTROPIC && state[D3DTSS_MAXANISOTROPY] > 1) ? (float)state[D3DTSS_MAXANISOTROPY] : 1.0f);
	if (xgpu_capabilities.border_clamp)
	{
		color_to_vec4(state[D3DTSS_BORDERCOLOR], border);
		glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border);
	}
#else
	glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, dword_to_float(lod_bias));
	glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, (float)maximum_mip_level);
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
	/* the blit bypasses the cached state, so the next draw must re-apply it */
	xgpu_gl_state_invalidate();
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

		if (!target || target->width != width || target->height != height ||
			target->pixel_width != width || target->pixel_height != height)
			break;
		render_target_resolve(target);
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
	xgpu_gl_state_invalidate();
	return composite->texture;
}

static void bind_textures(struct nv2a_pixel_shader_key *key, float texture_scale[4][4])
{
	/* Bind only after resolving every stage, since texture uploads can
	overwrite the active unit's binding. */
	GLenum gl_targets[D3DTSS_MAXSTAGES];
	GLuint gl_textures[D3DTSS_MAXSTAGES];
	int stage;

	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
	{
		D3DBaseTexture *texture = device.textures[stage];
		unsigned long mode = stage_texture_mode(stage);

		texture_scale[stage][0] = texture_scale[stage][1] = 1.0f;
		texture_scale[stage][2] = texture_scale[stage][3] = 1.0f;
		if (!texture || !texture->Data || mode == 0 || mode == 0x04 || mode == 0x05 || mode == 0x11)
		{
			gl_targets[stage] = GL_TEXTURE_2D;
			gl_textures[stage] = 0;
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
				/* (multisampled: its pixels drawn since, resolved) */
				render_target_resolve(target);
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
				int type;

				gl_texture = xgpu_texture_get((const DWORD *)texture, palette, &type, &description);
				gl_target = gl_texture_target(type);
				if (description.linear)
				{
					texture_scale[stage][0] = 1.0f / (float)description.width;
					texture_scale[stage][1] = 1.0f / (float)description.height;
				}
			}
			gl_targets[stage] = gl_target;
			gl_textures[stage] = gl_texture;
			state_sampler(stage, gl.samplers[stage]);
			configure_sampler(stage, description.levels > 1, description.hires);
			if (stage == 0)
				key->coverage_alpha = description.hires_coverage != FALSE;
			key->sampler_type[stage] = gl_target == GL_TEXTURE_CUBE_MAP ? _xgpu_sampler_cube :
				gl_target == GL_TEXTURE_3D ? _xgpu_sampler_3d : _xgpu_sampler_2d;
		}
	}
	for (stage = 0; stage < D3DTSS_MAXSTAGES; stage++)
		state_texture(stage, gl_targets[stage], gl_textures[stage]);
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
	GLint viewport[4];
	GLint scissor[4];
	float depth_range[2];
	unsigned char color_mask;
	BOOL depth_test = has_depth && rs[D3DRS_ZENABLE];

	viewport[0] = target_pixel((float)device.viewport.X, 0);
	viewport[1] = target_pixel((float)device.viewport.Y, 1);
	viewport[2] = target_pixel((float)(device.viewport.X + device.viewport.Width), 0) - viewport[0];
	viewport[3] = target_pixel((float)(device.viewport.Y + device.viewport.Height), 1) - viewport[1];
	if (memcmp(gl_state.viewport, viewport, sizeof(viewport)))
	{
		memcpy(gl_state.viewport, viewport, sizeof(viewport));
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}
	/* the game never issues a scissor rectangle, and the NV2A scissor register
	defaults to the viewport, so fragment clipping follows the viewport: this is
	what keeps a split-screen window's geometry from bleeding across the divider */
	/* (glScissor takes the corner and the size, as glViewport does) */
	memcpy(scissor, viewport, sizeof(scissor));
	if (memcmp(gl_state.scissor, scissor, sizeof(scissor)))
	{
		memcpy(gl_state.scissor, scissor, sizeof(scissor));
		glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
	}
	state_enable(&gl_state.scissor_test, GL_SCISSOR_TEST, scissor[2] > 0 && scissor[3] > 0);
	depth_range[0] = device.viewport.MinZ;
	depth_range[1] = device.viewport.MaxZ;
	if (memcmp(gl_state.depth_range, depth_range, sizeof(depth_range)))
	{
		memcpy(gl_state.depth_range, depth_range, sizeof(depth_range));
		glDepthRange(depth_range[0], depth_range[1]);
	}

	state_enable(&gl_state.depth_test, GL_DEPTH_TEST, depth_test);
	if (depth_test)
	{
		GLenum function = rs[D3DRS_ZFUNC] ? (GLenum)rs[D3DRS_ZFUNC] : GL_NEVER;

		if (gl_state.depth_function != function)
		{
			gl_state.depth_function = function;
			glDepthFunc(function);
		}
	}
	{
		unsigned char mask = depth_test && rs[D3DRS_ZWRITEENABLE] ? 1 : 0;

		if (gl_state.depth_mask != mask)
		{
			gl_state.depth_mask = mask;
			glDepthMask(mask ? GL_TRUE : GL_FALSE);
		}
	}

	state_enable(&gl_state.stencil_test, GL_STENCIL_TEST, has_depth && rs[D3DRS_STENCILENABLE]);
	if (has_depth && rs[D3DRS_STENCILENABLE])
	{
		GLenum function = rs[D3DRS_STENCILFUNC] ? (GLenum)rs[D3DRS_STENCILFUNC] : GL_NEVER;
		GLenum operations[3];

		if (gl_state.stencil_function != function || gl_state.stencil_reference != (GLint)rs[D3DRS_STENCILREF] ||
			gl_state.stencil_value_mask != rs[D3DRS_STENCILMASK])
		{
			gl_state.stencil_function = function;
			gl_state.stencil_reference = (GLint)rs[D3DRS_STENCILREF];
			gl_state.stencil_value_mask = rs[D3DRS_STENCILMASK];
			glStencilFunc(function, (GLint)rs[D3DRS_STENCILREF], rs[D3DRS_STENCILMASK]);
		}
		operations[0] = stencil_operation(rs[D3DRS_STENCILFAIL]);
		operations[1] = stencil_operation(rs[D3DRS_STENCILZFAIL]);
		operations[2] = stencil_operation(rs[D3DRS_STENCILPASS]);
		if (memcmp(gl_state.stencil_operations, operations, sizeof(operations)))
		{
			memcpy(gl_state.stencil_operations, operations, sizeof(operations));
			glStencilOp(operations[0], operations[1], operations[2]);
		}
		if (gl_state.stencil_write_mask != rs[D3DRS_STENCILWRITEMASK])
		{
			gl_state.stencil_write_mask = rs[D3DRS_STENCILWRITEMASK];
			glStencilMask(rs[D3DRS_STENCILWRITEMASK]);
		}
	}

	state_enable(&gl_state.blend, GL_BLEND, rs[D3DRS_ALPHABLENDENABLE] != 0);
	if (rs[D3DRS_ALPHABLENDENABLE])
	{
		GLenum equation = blend_equation(rs[D3DRS_BLENDOP]);
		float blend_color[4];

		if (gl_state.blend_source != (GLenum)rs[D3DRS_SRCBLEND] ||
			gl_state.blend_destination != (GLenum)rs[D3DRS_DESTBLEND])
		{
			gl_state.blend_source = (GLenum)rs[D3DRS_SRCBLEND];
			gl_state.blend_destination = (GLenum)rs[D3DRS_DESTBLEND];
			glBlendFunc(gl_state.blend_source, gl_state.blend_destination);
		}
		if (gl_state.blend_equation != equation)
		{
			gl_state.blend_equation = equation;
			glBlendEquation(equation);
		}
		color_to_vec4(rs[D3DRS_BLENDCOLOR], blend_color);
		if (memcmp(gl_state.blend_color, blend_color, sizeof(blend_color)))
		{
			memcpy(gl_state.blend_color, blend_color, sizeof(blend_color));
			glBlendColor(blend_color[0], blend_color[1], blend_color[2], blend_color[3]);
		}
	}
	color_mask = (unsigned char)(((write & D3DCOLORWRITEENABLE_RED) ? 1 : 0) | ((write & D3DCOLORWRITEENABLE_GREEN) ? 2 : 0) |
		((write & D3DCOLORWRITEENABLE_BLUE) ? 4 : 0) | ((write & D3DCOLORWRITEENABLE_ALPHA) ? 8 : 0));
	if (gl_state.color_mask != color_mask)
	{
		gl_state.color_mask = color_mask;
		glColorMask((color_mask & 1) != 0, (color_mask & 2) != 0, (color_mask & 4) != 0, (color_mask & 8) != 0);
	}

	/* the cull mode names the winding to discard; FRONTFACE names the
	front winding */
	state_enable(&gl_state.cull_face, GL_CULL_FACE, rs[D3DRS_CULLMODE] != D3DCULL_NONE);
	if (rs[D3DRS_CULLMODE] != D3DCULL_NONE)
	{
#ifdef HALO_ANDROID
		/* the vertex shader flips y in clip space, which (unlike desktop
		GL's upper-left clip origin) also flips the winding */
		GLenum front_face = rs[D3DRS_FRONTFACE] == D3DFRONT_CCW ? GL_CW : GL_CCW;
#else
		GLenum front_face = rs[D3DRS_FRONTFACE] == D3DFRONT_CCW ? GL_CCW : GL_CW;
#endif
		GLenum cull_mode = rs[D3DRS_CULLMODE] == rs[D3DRS_FRONTFACE] ? GL_FRONT : GL_BACK;

		if (gl_state.front_face != front_face)
		{
			gl_state.front_face = front_face;
			glFrontFace(front_face);
		}
		if (gl_state.cull_mode != cull_mode)
		{
			gl_state.cull_mode = cull_mode;
			glCullFace(cull_mode);
		}
	}
#ifndef HALO_ANDROID
	/* ES draws filled polygons only (wireframe is a debug mode) */
	{
		GLenum polygon_mode = rs[D3DRS_FILLMODE] == D3DFILL_WIREFRAME ? GL_LINE :
			rs[D3DRS_FILLMODE] == D3DFILL_POINT ? GL_POINT : GL_FILL;

		if (gl_state.polygon_mode != polygon_mode)
		{
			gl_state.polygon_mode = polygon_mode;
			glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
		}
	}
#endif

	/* D3DRS_ZBIAS is expressed in these states (D3DDevice_SetRenderState_ZBias) */
	state_enable(&gl_state.offset_fill, GL_POLYGON_OFFSET_FILL, rs[D3DRS_SOLIDOFFSETENABLE] != 0);
#ifndef HALO_ANDROID
	state_enable(&gl_state.offset_line, GL_POLYGON_OFFSET_LINE, rs[D3DRS_SOLIDOFFSETENABLE] != 0);
#endif
	if (rs[D3DRS_SOLIDOFFSETENABLE])
	{
		float offset[2];

		offset[0] = dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE]);
		offset[1] = dword_to_float(rs[D3DRS_POLYGONOFFSETZOFFSET]);
		if (memcmp(gl_state.polygon_offset, offset, sizeof(offset)))
		{
			memcpy(gl_state.polygon_offset, offset, sizeof(offset));
			glPolygonOffset(offset[0], offset[1]);
		}
	}
}

#ifdef HALO_ANDROID
/* ES has no debug callback in 3.0; debug.gl_debug polls glGetError around
each draw instead, reporting each distinct error a few times */
static void gl_check_errors(const char *where)
{
	static int enabled = -1;
	static unsigned long reports;
	GLenum error;

	if (enabled < 0)
		enabled = config_boolean("debug.gl_debug");
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

/* sets a program's uniform unless it already holds value */
static void uniform_vec4(GLint location, float *shadow, const float *value, int count)
{
	if (location < 0 || !memcmp(shadow, value, (size_t)count * 4 * sizeof(float)))
		return;
	memcpy(shadow, value, (size_t)count * 4 * sizeof(float));
	glUniform4fv(location, count, value);
}

static void uniform_float(GLint location, float *shadow, float value)
{
	if (location < 0 || !memcmp(shadow, &value, sizeof(value)))
		return;
	*shadow = value;
	glUniform1f(location, value);
}

/* Intel's graphics with Mesa's driver can hang the GPU in a long run of
draws with no pipeline flush between them, which the game's effects make
(hundreds of small draws in a row): the command streamer stops at a draw,
and the reset that follows takes the desktop's other programs with it.
Intel's workaround for a hang of this kind on their DG2 graphics
(Wa_16014538804) is a flush at least every 3 draws, which Mesa does not
apply to the others. A memory barrier is one (and only that: nothing
here writes images). */
static void draw_flush(void)
{
#ifndef HALO_ANDROID
	if (gl.flush_every && ++gl.flush_draws >= gl.flush_every)
	{
		gl.flush_draws = 0;
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
#endif
}

static struct program_entry *prepare_draw(BOOL immediate)
{
	struct vertex_shader_object *program = current_program();
	struct nv2a_pixel_shader_key key;
	struct program_entry *entry;
	struct draw_uniforms uniforms;
	BOOL has_depth = FALSE;

	if (!device.ready || !program || !device.vertex_shader || !program->instructions)
	{
		stats.skipped_no_program++;
		return NULL;
	}
	if (xgpu_skip_program(program))
		return NULL;
	xgpu_pixel_shader_key_begin(&key);
	/* the textures before the targets: a render target the draw samples
	has its multisampled pixels resolved by a blit (render_target_resolve),
	which binds framebuffers of its own, and the back buffer can be both
	sampled and drawn into */
	bind_textures(&key, uniforms.texture_scale);
	if (!bind_targets(&has_depth))
	{
		stats.skipped_no_target++;
		return NULL;
	}
	apply_raster_state(has_depth);
	xgpu_pixel_shader_key_finish(&key, target_samples);
#ifdef HALO_ANDROID
	key.count_samples = device.visibility_test_active && xgpu_capabilities.atomic_counters;
#endif

	entry = NULL;
	if (program->lighting.lights && !program->lighting_failed && xgpu_per_pixel_lighting())
	{
		key.per_pixel_lighting = (unsigned char)program->lighting.lights;
		entry = program_get(vertex_shader_get(program, immediate, TRUE), fragment_shader_get(&key));
		if (!entry)
		{
			/* drawn as the vertex shader lights it instead (not at all,
			were it to fail too) */
			platform_log("GPU: vertex shader %lu cannot be lit for each pixel here (refer to the shader log above): "
				"lit for each vertex", program->id);
			program->lighting_failed = TRUE;
			key.per_pixel_lighting = 0;
		}
	}
	if (!entry)
		entry = program_get(vertex_shader_get(program, immediate, FALSE), fragment_shader_get(&key));
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
	draw_flush();
	state_program(entry->program);
#ifdef HALO_ANDROID
	if (key.count_samples)
		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, gl.visibility_counters,
			(GLintptr)(gl.counter_active * sizeof(GLuint)), sizeof(GLuint));
#endif

	if (entry->constants >= 0 && entry->constants_serial != xgpu_constant_serials.serial)
	{
		unsigned long first = entry->constant_count, last = 0, index;

		if (entry->constants_serial == constants_checkpoint_serial &&
			constants_checkpoint_last < entry->constant_count)
		{
			if (constants_checkpoint_first <= constants_checkpoint_last)
			{
				first = constants_checkpoint_first;
				last = constants_checkpoint_last;
			}
		}
		else if (xgpu_constant_serials.serial - entry->constants_serial <= XGPU_VERTEX_CONSTANT_COUNT)
		{
			unsigned long long serial;

			for (serial = entry->constants_serial + 1; serial <= xgpu_constant_serials.serial; serial++)
			{
				index = constant_log[serial % CONSTANT_LOG_SIZE];
				if (index >= entry->constant_count)
					continue;
				if (first > index)
					first = index;
				if (last < index)
					last = index;
			}
		}
		else
		{
			for (index = 0; index < entry->constant_count; index++)
			{
				if (constant_serials[index] > entry->constants_serial)
				{
					if (first > index)
						first = index;
					last = index;
				}
			}
		}
		if (first < entry->constant_count)
		{
			if (entry->constants_consecutive)
				glUniform4fv(entry->constants + (GLint)first, (GLsizei)(last - first + 1), device.constants[first]);
			else
				glUniform4fv(entry->constants, XGPU_VERTEX_CONSTANT_COUNT, &device.constants[0][0]);
		}
		entry->constants_serial = xgpu_constant_serials.serial;
		constants_checkpoint_serial = xgpu_constant_serials.serial;
		constants_checkpoint_first = XGPU_VERTEX_CONSTANT_COUNT;
		constants_checkpoint_last = 0;
	}
	/* the lights of a draw lit for each pixel, from the same registers, when
	any of them changed since the program last had them */
	if (entry->model_lights >= 0 && entry->model_lights_serial != xgpu_constant_serials.serial)
	{
		int light;

		for (light = 0; light < XGPU_MODEL_LIGHT_COUNT; light++)
		{
			if (constant_serials[model_light_register(light)] > entry->model_lights_serial)
				break;
		}
		if (light < XGPU_MODEL_LIGHT_COUNT)
		{
			float lights[XGPU_MODEL_LIGHT_COUNT][4];

			for (light = 0; light < XGPU_MODEL_LIGHT_COUNT; light++)
				memcpy(lights[light], device.constants[model_light_register(light)], sizeof(lights[0]));
			glUniform4fv(entry->model_lights, XGPU_MODEL_LIGHT_COUNT, lights[0]);
		}
		entry->model_lights_serial = xgpu_constant_serials.serial;
	}

	/* the state the other uniforms come from (most draws share it with the
	draw before them, and so share its uniforms) */
	xgpu_draw_uniforms_update(uniforms.texture_scale);
	/* and a program that has had them since needs none of them */
	if (entry->uniforms_serial == draw_uniforms_serial)
		return entry;
	entry->uniforms_serial = draw_uniforms_serial;
	uniform_vec4(entry->viewport_scale, entry->uniforms.viewport_scale, xgpu_draw_uniforms.viewport_scale, 1);
	uniform_vec4(entry->viewport_offset, entry->uniforms.viewport_offset, xgpu_draw_uniforms.viewport_offset, 1);
	uniform_float(entry->point_size, &entry->uniforms.point_size, xgpu_draw_uniforms.point_size);
	uniform_vec4(entry->ps_c0, entry->uniforms.ps_c0[0], xgpu_draw_uniforms.ps_c0[0], 8);
	uniform_vec4(entry->ps_c1, entry->uniforms.ps_c1[0], xgpu_draw_uniforms.ps_c1[0], 8);
	uniform_vec4(entry->ps_final_c0, entry->uniforms.ps_final_c0, xgpu_draw_uniforms.ps_final_c0, 1);
	uniform_vec4(entry->ps_final_c1, entry->uniforms.ps_final_c1, xgpu_draw_uniforms.ps_final_c1, 1);
	uniform_vec4(entry->fog_color, entry->uniforms.fog_color, xgpu_draw_uniforms.fog_color, 1);
	uniform_vec4(entry->fog_parameters, entry->uniforms.fog_parameters, xgpu_draw_uniforms.fog_parameters, 1);
	uniform_float(entry->alpha_reference, &entry->uniforms.alpha_reference, xgpu_draw_uniforms.alpha_reference);
	uniform_vec4(entry->bump_matrix, entry->uniforms.bump_matrix[0], xgpu_draw_uniforms.bump_matrix[0], 4);
	uniform_vec4(entry->bump_luminance, entry->uniforms.bump_luminance[0], xgpu_draw_uniforms.bump_luminance[0], 4);
	uniform_vec4(entry->texture_scale, entry->uniforms.texture_scale[0], xgpu_draw_uniforms.texture_scale[0], 4);
	uniform_float(entry->screen_offset, &entry->uniforms.screen_offset, xgpu_draw_uniforms.screen_offset);
	uniform_vec4(entry->texture_lod_bias, entry->uniforms.texture_lod_bias, xgpu_draw_uniforms.texture_lod_bias, 1);
	return entry;
}

/* Mesa's GL thread queues a glBufferSubData of up to 8 KB; a larger one
first waits for everything queued before it to have run. The same bytes in
pieces are queued (all but the largest, as a map loads, which would be
thousands; Android's GL has no such thread). */
#define BUFFER_UPLOAD_PIECE 4096
#define BUFFER_UPLOAD_PIECES_MAXIMUM 16

static void buffer_upload(GLenum target, unsigned long offset, unsigned long size, const void *data)
{
#ifndef HALO_ANDROID
	if (size <= BUFFER_UPLOAD_PIECE * BUFFER_UPLOAD_PIECES_MAXIMUM)
	{
		const unsigned char *bytes = data;

		while (size)
		{
			unsigned long piece = size < BUFFER_UPLOAD_PIECE ? size : BUFFER_UPLOAD_PIECE;

			glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)piece, bytes);
			offset += piece;
			bytes += piece;
			size -= piece;
		}
		return;
	}
#endif
	glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)size, data);
}

/* ---------- the mirror's buffers (xgpu_device.h) */

static void gl_mirror_upload(unsigned long segment, unsigned long offset, unsigned long size, const void *data,
	BOOL unused)
{
	if (!gl.mirror_buffers[segment])
	{
		glGenBuffers(1, &gl.mirror_buffers[segment]);
		glBindBuffer(GL_COPY_WRITE_BUFFER, gl.mirror_buffers[segment]);
		glBufferData(GL_COPY_WRITE_BUFFER, MIRROR_SEGMENT_SIZE, NULL, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, gl.mirror_buffers[segment]);
#ifdef HALO_ANDROID
	/* Mali copies the whole buffer for a glBufferSubData that queued
	draws might read (see STREAM_BUFFER_RING); unused pages can be
	written without waiting for them */
	if (unused)
	{
		host_gl_buffer_write(GL_COPY_WRITE_BUFFER, (unsigned int)offset, (unsigned int)size, data);
		return;
	}
#else
	(void)unused;
#endif
	buffer_upload(GL_COPY_WRITE_BUFFER, offset, size, data);
}

/* xgpu_mirror_range, giving the GL buffer of its segment */
static BOOL mirror_range(unsigned long address, unsigned long size, GLuint *buffer, unsigned long *offset,
	unsigned long *generation)
{
	unsigned long segment;

	if (!xgpu_mirror_range(address, size, &segment, offset, generation))
		return FALSE;
	*buffer = gl.mirror_buffers[segment];
	return TRUE;
}

/* ---------- vertex data */

/* makes room for size bytes of uploads, orphaning the stream buffer if it
is full. A draw reserves room for all of its streams at once: orphaning
between two of them would leave the attributes already pointed at the
buffer reading its new, empty storage. */
static void stream_reserve(unsigned long size)
{
	if (gl.stream_offset + size > STREAM_BUFFER_SIZE)
	{
		/* orphan the buffer and start again */
		state_array_buffer(gl.stream_buffer);
		glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		gl.stream_offset = 0;
	}
}

static unsigned long stream_upload(const void *data, unsigned long size)
{
	unsigned long offset;

	size = (size + 15) & ~15UL;
	stream_reserve(size);
	offset = gl.stream_offset;
	state_array_buffer(gl.stream_buffer);
#ifdef HALO_ANDROID
	host_gl_buffer_write(GL_ARRAY_BUFFER, (unsigned int)offset, (unsigned int)size, data);
#else
	buffer_upload(GL_ARRAY_BUFFER, offset, size, data);
#endif
	gl.stream_offset += size;
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
	state_element_array_buffer(gl.index_buffer);
	if (gl.index_offset + size > INDEX_BUFFER_SIZE)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		gl.index_offset = 0;
	}
	offset = gl.index_offset;
#ifdef HALO_ANDROID
	host_gl_buffer_write(GL_ELEMENT_ARRAY_BUFFER, (unsigned int)offset, (unsigned int)size, data);
#else
	buffer_upload(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
#endif
	gl.index_offset += size;
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
#ifdef HALO_ANDROID
/* ES has no BGRA attributes, so a stream with colours is swizzled as it is
uploaded (stream_upload_swizzled) and cannot come from the mirror */
static BOOL stream_has_colors(const struct vertex_shader_object *declaration, unsigned long stream)
{
	unsigned long index;

	for (index = 0; index < declaration->element_count; index++)
	{
		if (declaration->elements[index].stream == stream && declaration->elements[index].type == D3DVSDT_D3DCOLOR)
			return TRUE;
	}
	return FALSE;
}
#endif

static void setup_streams(unsigned long first, unsigned long count)
{
	struct vertex_shader_object *declaration = device.vertex_shader;
	GLuint stream_buffers[16];
	unsigned long stream_offsets[16];
	BOOL placed[16] = { FALSE };
	BOOL enabled[XGPU_VERTEX_ATTRIBUTE_COUNT] = { FALSE };
	unsigned long index, total = 0;

	/* the mirror first; then one reservation for everything streamed */
	for (index = 0; index < declaration->element_count; index++)
	{
		const struct vertex_element *element = &declaration->elements[index];
		unsigned long stream = element->stream;
		unsigned long stride = device.streams[stream].stride;
		unsigned long bytes = stride ? stride * count : 64;
		unsigned long base;

		if (!device.streams[stream].data || element->type == D3DVSDT_NONE || placed[stream])
			continue;
		placed[stream] = TRUE;
		stream_buffers[stream] = 0;
		base = (unsigned long)PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data) + first * stride;
#ifdef HALO_ANDROID
		if (!stream_has_colors(declaration, stream))
#endif
		if (mirror_range(base, bytes, &stream_buffers[stream], &stream_offsets[stream], NULL))
			continue;
		stream_buffers[stream] = 0;
		total += (bytes + 15) & ~15UL;
	}
	stream_reserve(total);
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
		if (!stream_buffers[stream])
		{
			const unsigned char *base = PLATFORM_PHYSICAL_TO_VIRTUAL(device.streams[stream].data);
			unsigned long bytes = stride ? stride * count : 64;

#ifdef HALO_ANDROID
			stream_offsets[stream] = stream_upload_swizzled(declaration, stream, base + first * stride, bytes, stride);
#else
			stream_offsets[stream] = stream_upload(base + first * stride, bytes);
#endif
			stream_buffers[stream] = gl.stream_buffer;
			stats.streamed_bytes += bytes;
		}
		if (element->type == D3DVSDT_NORMPACKED3)
		{
			state_attribute_stream(element->reg, (GLuint)stream, stream_buffers[stream], 1, GL_UNSIGNED_INT, GL_FALSE,
				TRUE, (GLsizei)stride, stream_offsets[stream], element->offset);
		}
		else
		{
			attribute_format(element, &size, &type, &normalized);
			state_attribute_stream(element->reg, (GLuint)stream, stream_buffers[stream], size, type, normalized,
				FALSE, (GLsizei)stride, stream_offsets[stream], element->offset);
		}
		enabled[element->reg] = TRUE;
	}
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		if (!enabled[index])
			state_attribute_value(index, declaration->packed_mask & (1UL << index) ? NULL : device.attributes[index]);
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

static void gl_draw_vertices(D3DPRIMITIVETYPE primitive_type, UINT start_vertex, UINT vertex_count)
{
	if (!vertex_count || !prepare_draw(FALSE))
		return;
	trace_draw("draw", primitive_type, vertex_count, NULL);
	setup_streams(start_vertex, vertex_count);
	if (primitive_type == D3DPT_QUADLIST)
	{
		unsigned long count;
		WORD *indices = quad_indices(NULL, vertex_count, &count);

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

static void gl_draw_indexed_vertices(D3DPRIMITIVETYPE primitive_type, UINT vertex_count, const WORD *index_data)
{
	unsigned long minimum, maximum, index, count, generation = 0, index_offset = 0;
	WORD *indices = NULL;
	const WORD *source = index_data;
	GLuint index_buffer = 0;
	BOOL mirrored;

	if (!vertex_count || !index_data || !prepare_draw(FALSE))
		return;
	/* quads are drawn as triangles, from indices made for the draw */
	mirrored = primitive_type != D3DPT_QUADLIST &&
#ifdef HALO_ANDROID
		xgpu_capabilities.base_vertex &&
#endif
		mirror_range((unsigned long)index_data, vertex_count * sizeof(WORD), &index_buffer, &index_offset, &generation);
	index_extent(index_data, vertex_count, generation, mirrored, &minimum, &maximum);
	trace_draw("indexed", primitive_type, vertex_count, NULL);
	/* (the streams from the base vertex on: index i is vertex base + i) */
	setup_streams(device.base_vertex_index + minimum, maximum - minimum + 1);
	if (mirrored)
	{
		/* the attributes start at vertex minimum */
		state_element_array_buffer(index_buffer);
		glDrawElementsBaseVertex(primitive_mode(primitive_type), (GLsizei)vertex_count, GL_UNSIGNED_SHORT,
			(const void *)index_offset, -(GLint)minimum);
		return;
	}
	stats.streamed_bytes += vertex_count * sizeof(WORD);
	count = vertex_count;
	if (primitive_type == D3DPT_QUADLIST)
	{
		indices = quad_indices(index_data, vertex_count, &count);
		source = indices;
	}
#ifdef HALO_ANDROID
	if (!xgpu_capabilities.base_vertex)
	{
		/* the indices are copied anyway: rebase them */
		WORD *rebased = malloc(count * sizeof(WORD) + 2);

		for (index = 0; index < count; index++)
			rebased[index] = (WORD)(source[index] - minimum);
		glDrawElements(primitive_mode(primitive_type), (GLsizei)count, GL_UNSIGNED_SHORT,
			(const void *)index_upload(rebased, count * sizeof(WORD)));
		free(rebased);
		free(indices);
		return;
	}
#endif
	(void)index;
	glDrawElementsBaseVertex(primitive_mode(primitive_type), (GLsizei)count, GL_UNSIGNED_SHORT,
		(const void *)index_upload(source, count * sizeof(WORD)), -(GLint)minimum);
	free(indices);
}

static void gl_draw_immediate(D3DPRIMITIVETYPE type, const float *vertices, unsigned long count)
{
	unsigned long stride = XGPU_VERTEX_ATTRIBUTE_COUNT * 4 * sizeof(float);
	unsigned long offset, index;

	if (!count || !prepare_draw(TRUE))
		return;
	trace_draw("immediate", type, count, vertices);
	offset = stream_upload(vertices, count * stride);
	for (index = 0; index < XGPU_VERTEX_ATTRIBUTE_COUNT; index++)
	{
		state_attribute_stream(index, 0, gl.stream_buffer, 4, GL_FLOAT, GL_FALSE, FALSE, (GLsizei)stride,
			offset, index * 4 * sizeof(float));
	}
	if (type == D3DPT_QUADLIST)
	{
		unsigned long index_count;
		WORD *indices = quad_indices(NULL, count, &index_count);

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

/* ---------- clearing */

static void gl_clear(DWORD count, const D3DRECT *rectangles, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
	float rgba[4];
	GLbitfield mask = 0;
	BOOL has_depth = FALSE;
	DWORD index;

	if (!bind_targets(&has_depth))
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
		/* the NV2A clips a viewport-less clear to the viewport, which is what
		keeps a split-screen window's clear from wiping the other window */
		GLint x0 = target_pixel((float)device.viewport.X, 0);
		GLint y0 = target_pixel((float)device.viewport.Y, 1);

		glEnable(GL_SCISSOR_TEST);
		glScissor(x0, y0, target_pixel((float)(device.viewport.X + device.viewport.Width), 0) - x0,
			target_pixel((float)(device.viewport.Y + device.viewport.Height), 1) - y0);
		glClear(mask);
		glDisable(GL_SCISSOR_TEST);
		xgpu_gl_state_invalidate();
		return;
	}
	glEnable(GL_SCISSOR_TEST);
	for (index = 0; index < count; index++)
	{
		INT left = rectangles[index].x1 > device.viewport.X ? rectangles[index].x1 : device.viewport.X;
		INT top = rectangles[index].y1 > device.viewport.Y ? rectangles[index].y1 : device.viewport.Y;
		INT right = rectangles[index].x2 < device.viewport.X + device.viewport.Width ?
			rectangles[index].x2 : device.viewport.X + device.viewport.Width;
		INT bottom = rectangles[index].y2 < device.viewport.Y + device.viewport.Height ?
			rectangles[index].y2 : device.viewport.Y + device.viewport.Height;
		GLint x0, y0;

		if (left >= right || top >= bottom)
			continue;
		x0 = target_pixel((float)(left + UI_OFFSET), 0);
		y0 = target_pixel((float)top, 1);
		glScissor(x0, y0, target_pixel((float)(right + UI_OFFSET), 0) - x0,
			target_pixel((float)bottom, 1) - y0);
		glClear(mask);
	}
	glDisable(GL_SCISSOR_TEST);
	xgpu_gl_state_invalidate();
}

/* ---------- the anti-aliasing passes */

static void gl_anti_alias(short x0, short y0, short x1, short y1)
{
	struct render_target_entry *target;
	GLint corners[4];

	/* (the primary target's view: the back buffer's) */
	target = render_target_get(&device.back_buffer);
	if (!target)
		return;
	/* (no longer multisampled, if it was before the setting changed) */
	render_target_multisample(&target->target, 0);
	corners[0] = scaled_pixel(x0, target->target.scale[0]);
	corners[1] = scaled_pixel(y0, target->target.scale[1]);
	corners[2] = scaled_pixel(x1, target->target.scale[0]);
	corners[3] = scaled_pixel(y1, target->target.scale[1]);
	xgpu_post_anti_alias(anti_aliasing() == _anti_aliasing_smaa, framebuffer_get(target->target.texture, 0),
		target->target.pixel_width, target->target.pixel_height, corners);
	glBindVertexArray(gl.vertex_array);
	xgpu_gl_state_invalidate();
}

/* ---------- presentation */

/* the back buffer's pixels, saved to path as a BMP file (screenshot.h); 1 on
success */
static int gl_save_screenshot(struct render_target_entry *target, const char *path)
{
	unsigned long width = target->target.pixel_width, height = target->target.pixel_height;
	unsigned char *pixels;
	int saved;

	render_target_resolve(&target->target);
	pixels = malloc(width * height * 4);
	if (!pixels)
		return 0;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_get(target->target.texture, 0));
	/* rows from the top, as screenshot_write_bmp takes them */
	glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
#ifdef HALO_ANDROID
	{
		unsigned long pixel;

		for (pixel = 0; pixel < width * height; pixel++)
		{
			unsigned char red = pixels[pixel * 4];

			pixels[pixel * 4] = pixels[pixel * 4 + 2];
			pixels[pixel * 4 + 2] = red;
		}
	}
#endif
	saved = screenshot_write_bmp(path, pixels, width, height);
	free(pixels);
	return saved;
}

static void gl_present(struct render_target_entry *back_buffer)
{
	int window_width, window_height, width, height, x, y;

	render_target_resolve(&back_buffer->target);
	platform_video_drawable_size(&window_width, &window_height);
	/* letterbox to the back buffer's aspect ratio */
	width = window_width;
	height = (int)((long)window_width * back_buffer->target.pixel_height / back_buffer->target.pixel_width);
	if (height > window_height)
	{
		height = window_height;
		width = (int)((long)window_height * back_buffer->target.pixel_width / back_buffer->target.pixel_height);
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
	glBlitFramebuffer(0, 0, (GLint)back_buffer->target.pixel_width, (GLint)back_buffer->target.pixel_height,
		x, y + height, x + width, y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	platform_video_swap();
	xgpu_gl_state_invalidate();
#ifdef HALO_ANDROID
	host_gl_fence_frame((unsigned int)gl.buffer_ring);
	gl.buffer_ring = (gl.buffer_ring + 1) % STREAM_BUFFER_RING;
	host_gl_wait_frame((unsigned int)gl.buffer_ring);
	gl.stream_buffer = gl.stream_buffers[gl.buffer_ring];
	gl.index_buffer = gl.index_buffers[gl.buffer_ring];
	gl.stream_offset = 0;
	gl.index_offset = 0;
#else
	gl.stream_offset = STREAM_BUFFER_SIZE; /* orphan next frame */
	gl.index_offset = INDEX_BUFFER_SIZE;
#endif
}

static void gl_flush(void)
{
	glFlush();
}

/* ---------- render targets */

static void gl_render_target_create(struct xgpu_render_target *target)
{
	glGenTextures(1, &target->texture);
	glBindTexture(GL_TEXTURE_2D, target->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	if (target->depth)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (GLsizei)target->pixel_width,
			(GLsizei)target->pixel_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)target->pixel_width, (GLsizei)target->pixel_height,
			0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	xgpu_gl_state_invalidate();
}

/* ---------- textures (xgpu.h)

Each texture's type, format and size, kept by its GL name, so that its
levels can be written without asking GL. */

#define TEXTURE_RECORD_BUCKETS 4096

struct texture_record
{
	struct texture_record *next;
	GLuint name;
	int type, format;
	unsigned long width, height, depth, levels;
	/* level 0 has storage (written whole, or rows into storage made first) */
	BOOL allocated;
};

static struct texture_record *texture_records[TEXTURE_RECORD_BUCKETS];

static struct texture_record *texture_record_find(GLuint name)
{
	struct texture_record *record;

	for (record = texture_records[name % TEXTURE_RECORD_BUCKETS]; record; record = record->next)
	{
		if (record->name == name)
			return record;
	}
	return NULL;
}

static BOOL format_compressed(int format)
{
	return format == _xgpu_format_dxt1 || format == _xgpu_format_dxt3 || format == _xgpu_format_dxt5;
}

static GLenum compressed_format(int format)
{
	switch (format)
	{
	case _xgpu_format_dxt1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case _xgpu_format_dxt3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	default: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
}

/* the order of the bytes of an uncompressed format's texels */
static GLenum pixel_format(int format)
{
	return format == _xgpu_format_bgra8 ? GL_BGRA : GL_RGBA;
}

static unsigned long level_size(unsigned long base, unsigned long level)
{
	return base >> level ? base >> level : 1;
}

static unsigned int gl_texture_new(int type, int format, unsigned long width, unsigned long height,
	unsigned long depth, unsigned long levels)
{
	GLenum target = gl_texture_target(type);
	struct texture_record *record = calloc(1, sizeof(*record));
	GLuint texture;

	if (!record)
		return 0;
	glGenTextures(1, &texture);
	glBindTexture(target, texture);
#ifdef HALO_ANDROID
	/* converted texels are BGRA in memory (32-bit ARGB words); ES takes
	RGBA */
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, format == _xgpu_format_bgra8 ? GL_BLUE : GL_RED);
	glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, format == _xgpu_format_bgra8 ? GL_RED : GL_BLUE);
#endif
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, (GLint)levels - 1);
	xgpu_gl_state_invalidate();
	record->name = texture;
	record->type = type;
	record->format = format;
	record->width = width;
	record->height = height;
	record->depth = depth;
	record->levels = levels;
	record->next = texture_records[texture % TEXTURE_RECORD_BUCKETS];
	texture_records[texture % TEXTURE_RECORD_BUCKETS] = record;
	return texture;
}

static void gl_texture_write(unsigned int texture, unsigned long face, unsigned long level, const void *data)
{
	struct texture_record *record = texture_record_find(texture);
	GLenum target, image_target;
	GLsizei width, height, depth;

	if (!record)
		return;
	target = gl_texture_target(record->type);
	image_target = record->type == _xgpu_texture_cube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum)face : target;
	width = (GLsizei)level_size(record->width, level);
	height = (GLsizei)level_size(record->height, level);
	depth = (GLsizei)level_size(record->depth, level);
	glBindTexture(target, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (format_compressed(record->format))
	{
		GLsizei size = (GLsizei)(((width + 3) / 4) * ((height + 3) / 4) * (record->format == _xgpu_format_dxt1 ? 8 : 16) *
			depth);

		if (target == GL_TEXTURE_3D)
			glCompressedTexImage3D(image_target, (GLint)level, compressed_format(record->format), width, height, depth, 0,
				size, data);
		else
			glCompressedTexImage2D(image_target, (GLint)level, compressed_format(record->format), width, height, 0,
				size, data);
	}
	else if (target == GL_TEXTURE_3D)
	{
		glTexImage3D(image_target, (GLint)level, GL_RGBA8, width, height, depth, 0, pixel_format(record->format),
			GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexImage2D(image_target, (GLint)level, GL_RGBA8, width, height, 0, pixel_format(record->format),
			GL_UNSIGNED_BYTE, data);
	}
	if (level == 0)
		record->allocated = TRUE;
	xgpu_gl_state_invalidate();
}

static void gl_texture_write_rows(unsigned int texture, unsigned long first_row, unsigned long rows, const void *data)
{
	struct texture_record *record = texture_record_find(texture);

	if (!record || record->type != _xgpu_texture_2d || format_compressed(record->format))
		return;
	glBindTexture(GL_TEXTURE_2D, texture);
	if (!record->allocated)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)record->width, (GLsizei)record->height, 0,
			pixel_format(record->format), GL_UNSIGNED_BYTE, NULL);
		record->allocated = TRUE;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)first_row, (GLsizei)record->width, (GLsizei)rows,
		pixel_format(record->format), GL_UNSIGNED_BYTE, data);
	xgpu_gl_state_invalidate();
}

static void gl_texture_mipmaps(unsigned int texture, const void *level0)
{
	struct texture_record *record = texture_record_find(texture);

	(void)level0;
	if (!record)
		return;
	glBindTexture(gl_texture_target(record->type), texture);
	glGenerateMipmap(gl_texture_target(record->type));
	xgpu_gl_state_invalidate();
}

static void gl_texture_delete(unsigned int texture)
{
	struct texture_record **link;
	GLuint name = texture;

	for (link = &texture_records[name % TEXTURE_RECORD_BUCKETS]; *link; link = &(*link)->next)
	{
		if ((*link)->name == name)
		{
			struct texture_record *record = *link;

			*link = record->next;
			free(record);
			break;
		}
	}
	glDeleteTextures(1, &name);
	xgpu_gl_state_invalidate();
}

static BOOL gl_texture_compressed_supported(unsigned long width, unsigned long height)
{
	(void)width;
	(void)height;
#ifdef HALO_ANDROID
	return xgpu_capabilities.s3tc;
#else
	return TRUE;
#endif
}

/* debug.texture_dump_directory writes level 0 of every upload as a TGA, read
back from GL */
static void gl_texture_dump(unsigned int texture, const struct xgpu_texture_description *description)
{
#ifdef HALO_ANDROID
	/* ES cannot read textures back */
	(void)texture;
	(void)description;
#else
	static unsigned long dump_index = 0;
	const char *directory = *config_string("debug.texture_dump_directory") ?
		config_string("debug.texture_dump_directory") : NULL;
	struct texture_record *record = texture_record_find(texture);
	unsigned long width = description->width, height = description->height;
	unsigned char header[18];
	unsigned char *pixels;
	char path[512];
	FILE *file;

	if (!directory || !record || record->type != _xgpu_texture_2d)
		return;
	pixels = malloc(width * height * 4);
	if (!pixels)
		return;
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	xgpu_gl_state_invalidate();
	snprintf(path, sizeof(path), "%s/tex%05lu_fmt%02x_%lux%lu.tga", directory, dump_index++,
		(unsigned)description->format, width, height);
	file = fopen(path, "wb");
	if (file)
	{
		memset(header, 0, sizeof(header));
		header[2] = 2;
		header[12] = (unsigned char)width; header[13] = (unsigned char)(width >> 8);
		header[14] = (unsigned char)height; header[15] = (unsigned char)(height >> 8);
		header[16] = 32; header[17] = 0x28;
		fwrite(header, 1, sizeof(header), file);
		fwrite(pixels, 4, width * height, file);
		fclose(file);
	}
	free(pixels);
#endif
}

/* ---------- the renderer */

const struct xgpu_backend xgpu_backend_gl =
{
	.name = "OpenGL",
	.initialize = gl_backend_initialize,
	.anti_aliasing_prepare = gl_anti_aliasing_prepare,
	.render_target_create = gl_render_target_create,
	.draw_vertices = gl_draw_vertices,
	.draw_indexed_vertices = gl_draw_indexed_vertices,
	.draw_immediate = gl_draw_immediate,
	.clear = gl_clear,
	.visibility_begin = gl_visibility_begin,
	.visibility_end = gl_visibility_end,
	.visibility_result = gl_visibility_result,
	.anti_alias = gl_anti_alias,
	.save_screenshot = gl_save_screenshot,
	.present = gl_present,
	.flush = gl_flush,
	.mirror_upload = gl_mirror_upload,
	.texture_new = gl_texture_new,
	.texture_write = gl_texture_write,
	.texture_write_rows = gl_texture_write_rows,
	.texture_mipmaps = gl_texture_mipmaps,
	.texture_delete = gl_texture_delete,
	.texture_compressed_supported = gl_texture_compressed_supported,
	.texture_dump = gl_texture_dump,
};
