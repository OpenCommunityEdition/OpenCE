/*
GPU_GL.H

The OpenGL backend's own declarations (gpu_gl.c), for the files that still
make GL calls besides it: the device (d3d8_gl.c), whose draws, clears and
presentation do not go through gpu.h yet, and the anti-aliasing passes
(xgpu_post.c).
*/

#ifndef __HALO_LINUX_GPU_GL_H
#define __HALO_LINUX_GPU_GL_H

#include "platform.h"
#include "gl.h"
#include "gpu.h"

#ifdef HALO_ANDROID
/* OpenGL ES 3 (port/android/README.md): the desktop formats, enumerants
and entry points used that ES lacks */
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

/* OpenGL ES features that are optional (gpu_initialize) */
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
void host_gl_read_buffer(unsigned int buffer, unsigned int offset, unsigned int size, void *data);
void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data);
void host_gl_fence_frame(unsigned int slot);
void host_gl_wait_frame(unsigned int slot);
#endif

/* ---------- GL state cache

Consecutive draws share most of their state, but each sets all of it: the
setters here skip the call when GL already holds the value. Code that
changes GL state behind the cache's back (clears, presentation, texture
uploads, render target and framebuffer creation) calls
xgpu_gl_state_invalidate, after which every value is set again.
Unknown values are all ones, which no real value matches (floats become
NaN, which compares unequal to everything). */

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

/* a vertex array object for each vertex layout (the attributes a draw
enables, and each one's format and binding), made once: a draw binds its
layout's and points the bindings at its streams, rather than enabling,
formatting and binding each attribute again (setup_streams) */
struct vertex_layout
{
	unsigned long enabled;
	struct attribute_format formats[GPU_ATTRIBUTE_COUNT];
};

struct vertex_array_entry
{
	struct vertex_array_entry *next;
	unsigned long hash;
	struct vertex_layout layout;
	GLuint vertex_array;
	/* its bindings, as last pointed (nothing else changes them) */
	struct vertex_binding bindings[VERTEX_BINDING_COUNT];
};
#endif

struct gl_state
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
	GLuint textures[GPU_STAGE_COUNT][3];
	GLuint samplers[GPU_STAGE_COUNT];
	GLuint array_buffer;
	GLuint element_array_buffer;
	unsigned char attribute_enabled[GPU_ATTRIBUTE_COUNT];
#ifdef HALO_ANDROID
	struct attribute_pointer attribute_pointers[GPU_ATTRIBUTE_COUNT];
#else
	GLuint vertex_array;
#endif
	/* a disabled attribute's value; kind 1 is the integer zero */
	unsigned char attribute_value_kind[GPU_ATTRIBUTE_COUNT];
	float attribute_values[GPU_ATTRIBUTE_COUNT][4];
};

extern struct gl_state gl_state;

void xgpu_gl_state_invalidate(void);

void state_enable(unsigned char *shadow, GLenum capability, BOOL enabled);
void state_program(GLuint program);
void state_array_buffer(GLuint buffer);
void state_element_array_buffer(GLuint buffer);
#ifdef HALO_ANDROID
void state_attribute_stream(GLuint index, GLuint binding, GLuint buffer, GLint size, GLenum type,
	GLboolean normalized, BOOL integer, GLsizei stride, unsigned long buffer_offset, unsigned long relative_offset);
#else
struct vertex_array_entry *vertex_array_get(const struct vertex_layout *layout);
void state_vertex_array(struct vertex_array_entry *entry);
void state_vertex_buffer(GLuint binding, GLuint buffer, unsigned long offset, GLsizei stride);
void layout_attribute(struct vertex_layout *layout, unsigned long index, GLuint binding, GLint size,
	GLenum type, GLboolean normalized, BOOL integer, unsigned long relative_offset);
#endif
void state_attribute_value(GLuint index, const float *value);

/* ---------- render targets */

/* the framebuffer of these render targets' textures (either may be 0) */
GLuint gpu_gl_framebuffer(gpu_texture color, gpu_texture depth);
/* draws go to these render targets (either may be 0), into their
multisampled storage with samples a pixel, or with 0 into their textures */
void gpu_gl_bind_targets(gpu_texture color, gpu_texture depth, uint32_t samples);
/* a render target's texture gets the pixels drawn into its multisampled
storage since it last had them */
void gpu_gl_resolve(gpu_texture target);
/* a render target's multisampled storage, with samples a pixel (0: none):
its pixels resolved into its texture first, and the texture's put into the
new storage */
void gpu_gl_multisample(gpu_texture target, uint32_t samples);

/* ---------- texture stages */

/* binds each stage's texture and sampler */
void gpu_gl_bind_stages(const struct gpu_stage stages[GPU_STAGE_COUNT]);

#endif
