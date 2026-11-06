/*
GLES_HOST.C

The services the Android host gives the OpenGL ES renderer
(port/linux/src/xgpu.h; port/android/host/host_gl.c), for the armv7 build,
where the renderer and the context are in one process. Its entry points are
resolved here, with SDL, so that gl.h's list (which the Android guest and
WebGL share) is left as it is.
*/

#ifdef HALO_LINUX_ARMV7

#include <GLES3/gl32.h>
#include <SDL3/SDL.h>
#include <string.h>

#define GLES_HOST_FUNCTIONS(X) \
	X(glGetIntegerv) \
	X(glGetStringi) \
	X(glBindBuffer) \
	X(glBufferSubData) \
	X(glMapBufferRange) \
	X(glUnmapBuffer) \
	X(glFenceSync) \
	X(glClientWaitSync) \
	X(glDeleteSync)

#define GLES_HOST_DECLARE(name) static __typeof__(&name) gles_host_##name;
GLES_HOST_FUNCTIONS(GLES_HOST_DECLARE)

static int gles_host_ready;

/* (when first used, with the context current) */
static void gles_host_load(void)
{
#define GLES_HOST_LOAD(name) gles_host_##name = (__typeof__(&name))(void *)SDL_GL_GetProcAddress(#name);
	GLES_HOST_FUNCTIONS(GLES_HOST_LOAD)
	gles_host_ready = 1;
}

/* The renderer streams each frame's vertices and indices into the next of a
ring of buffers (d3d8_gl.c). A fence marks the end of each frame's work, and a
buffer is written again only once the GPU has passed the fence of the frame
that last used it. */
#define FRAME_FENCE_SLOTS 8

static GLsync frame_fences[FRAME_FENCE_SLOTS];

int host_gl_has_extension(const char *name)
{
	GLint count = 0, index;

	if (!gles_host_ready)
		gles_host_load();
	if (!name || !gles_host_glGetIntegerv || !gles_host_glGetStringi)
		return 0;
	gles_host_glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)gles_host_glGetStringi(GL_EXTENSIONS, (GLuint)index);

		if (extension && !strcmp(extension, name))
			return 1;
	}
	return 0;
}

/* (ES has no glGetBufferSubData: the mapping it offers instead is a host pointer) */
void host_gl_read_buffer(unsigned int buffer, unsigned int offset, unsigned int size, void *data)
{
	const void *mapping = NULL;

	if (!gles_host_ready)
		gles_host_load();
	gles_host_glBindBuffer(GL_COPY_READ_BUFFER, buffer);
	if (gles_host_glMapBufferRange)
		mapping = gles_host_glMapBufferRange(GL_COPY_READ_BUFFER, offset, size, GL_MAP_READ_BIT);
	if (mapping)
	{
		memcpy(data, mapping, size);
		gles_host_glUnmapBuffer(GL_COPY_READ_BUFFER);
	}
	else
	{
		memset(data, 0, size);
	}
	gles_host_glBindBuffer(GL_COPY_READ_BUFFER, 0);
}

void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data)
{
	if (!gles_host_ready)
		gles_host_load();
	gles_host_glBufferSubData((GLenum)target, (GLintptr)offset, (GLsizeiptr)size, data);
}

void host_gl_fence_frame(unsigned int slot)
{
	if (!gles_host_ready)
		gles_host_load();
	if (slot >= FRAME_FENCE_SLOTS)
		return;
	if (frame_fences[slot])
		gles_host_glDeleteSync(frame_fences[slot]);
	frame_fences[slot] = gles_host_glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void host_gl_wait_frame(unsigned int slot)
{
	if (!gles_host_ready)
		gles_host_load();
	if (slot >= FRAME_FENCE_SLOTS || !frame_fences[slot])
		return;
	/* at most a second: a lost context must not hang the game */
	gles_host_glClientWaitSync(frame_fences[slot], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
	gles_host_glDeleteSync(frame_fences[slot]);
	frame_fences[slot] = NULL;
}

#endif
