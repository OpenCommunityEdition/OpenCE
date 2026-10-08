/*
GPU_GL.H

The OpenGL backend's own declarations (gpu_gl.c), for the files that still
make GL calls besides it: the device (d3d8_gl.c), whose clears, visibility
tests and presentation do not go through gpu.h yet, and the anti-aliasing
passes (xgpu_post.c).
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

/* the frames whose stream buffers (and visibility counts) take turns: one
is reused only once the GPU has finished the frame that last used it */
#define GPU_GL_FRAME_RING 3

/* port/android/guest/runtime/guest_host.h */
int host_gl_has_extension(const char *name);
void host_gl_read_buffer(unsigned int buffer, unsigned int offset, unsigned int size, void *data);
void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data);
void host_gl_fence_frame(unsigned int slot);
void host_gl_wait_frame(unsigned int slot);
#endif

/* ---------- GL state

The backend caches the GL state it sets for draws; code that changes GL
state behind it (binding a texture to upload it, a blit) must call this
afterwards. */

void xgpu_gl_state_invalidate(void);

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

/* ---------- shaders */

/* a compiled shader, or a linked program of two, or 0 with the log
written */
GLuint xgpu_compile_shader(GLenum type, const char *code, const char *what);
GLuint xgpu_link_program(GLuint vertex_shader, GLuint fragment_shader, const char *what);

/* ---------- frames */

/* the vertex array the ES attributes are on, and the desktop's before its
first draw */
extern GLuint gpu_gl_default_vertex_array;

/* the frame's slot in the ring of stream buffers (GPU_GL_FRAME_RING; 0 on
the desktop) */
unsigned long gpu_gl_frame_ring(void);
/* after a frame is presented: the stream buffers of the next (with OpenGL ES,
the next slot's, once the GPU has finished with them) */
void gpu_gl_frame_advance(void);

#endif
