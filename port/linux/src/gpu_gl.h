/*
GPU_GL.H

The OpenGL backend's own declarations (gpu_gl.c), shared with its
anti-aliasing passes (xgpu_post.c).
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

/* ---------- shaders */

/* a compiled shader, or a linked program of two, or 0 with the log
written */
GLuint xgpu_compile_shader(GLenum type, const char *code, const char *what);
GLuint xgpu_link_program(GLuint vertex_shader, GLuint fragment_shader, const char *what);

/* ---------- anti-aliasing

display.anti_aliasing's passes (xgpu_post.c): FXAA or SMAA antialias each
window's 3D view in place before the HUD and menus are drawn over it, so
that their text stays sharp (gpu_anti_alias). */

/* the programs and textures of FXAA, or of SMAA, made now; FALSE if they
cannot be (once FALSE, it stays so) */
BOOL xgpu_post_prepare(BOOL smaa);

/* FXAA, or SMAA, on the corners x0, y0 to x1, y1 (from row 0) of a render
target's framebuffer, width by height, GL_RGBA8; FALSE if its programs do
not build */
BOOL xgpu_post_anti_alias(BOOL smaa, GLuint framebuffer, unsigned long width, unsigned long height,
	const GLint corners[4]);

#endif
