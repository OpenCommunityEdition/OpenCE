/*
HOST_GL.C

OpenGL for the macOS guest, after the Android host's
(port/android/host/host_gl.c): Apple's OpenGL 4.1 core instead of OpenGL
ES. The guest's generated entry points (guest_gl.c) import
hostgl_<function>, resolved here to the framework's function; the
arguments already have host types by then. Only strings need copying back,
and the renderer's buffer and fence helpers below keep GL's pointers (a
mapping, a GLsync) on this side.
*/

#include "host.h"
#include "host_services.h"

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <dlfcn.h>
#include <string.h>

/* The loader fills the guest's imports with these as it loads the image,
before any context exists: the framework's functions are plain exports,
which need none (unlike a driver's extension functions). Functions newer
than 4.1 are not there, and their imports are the loader's fatal stub. */
void *host_gl_resolve(const char *name)
{
	static void *library;

	if (!library)
		library = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_NOW | RTLD_GLOBAL);
	return library ? dlsym(library, name) : NULL;
}

void host_gl_get_string(uint32_t name, int index, char *buffer, uint32_t size)
{
	const GLubyte *text = index >= 0 ? glGetStringi(name, (GLuint)index) : glGetString(name);

	if (!size)
		return;
	buffer[0] = 0;
	if (text)
	{
		strncpy(buffer, (const char *)text, size - 1);
		buffer[size - 1] = 0;
	}
}

int host_gl_has_extension(const char *name)
{
	GLint count = 0, index;

	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)index);

		if (extension && !strcmp(extension, name))
			return 1;
	}
	return 0;
}

/* copies size bytes of a buffer object (the snapshots of the visibility
tests' counters, d3d8_gl.c), which desktop GL reads back itself, waiting
for the GPU's writes to it. Binds GL_COPY_READ_BUFFER, which the renderer
uses only for its copies, and leaves it unbound. */
void host_gl_read_buffer(uint32_t buffer, uint32_t offset, uint32_t size, void *data)
{
	glBindBuffer(GL_COPY_READ_BUFFER, buffer);
	glGetBufferSubData(GL_COPY_READ_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
}

/* The renderer streams each frame's vertices and indices into the next of
a ring of buffers (d3d8_gl.c). A fence marks the end of each frame's work,
and a buffer is written again only once the GPU has passed the fence of the
frame that last used it: drivers queue several frames, and a draw still
waiting to run would otherwise read a later frame's vertices. */
#define FRAME_FENCE_SLOTS 8

static GLsync frame_fences[FRAME_FENCE_SLOTS];

void host_gl_fence_frame(uint32_t slot)
{
	if (slot >= FRAME_FENCE_SLOTS)
		return;
	if (frame_fences[slot])
		glDeleteSync(frame_fences[slot]);
	frame_fences[slot] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void host_gl_wait_frame(uint32_t slot)
{
	if (slot >= FRAME_FENCE_SLOTS || !frame_fences[slot])
		return;
	/* at most a second: a lost context must not hang the game */
	glClientWaitSync(frame_fences[slot], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
	glDeleteSync(frame_fences[slot]);
	frame_fences[slot] = NULL;
}

/* writes data into the buffer bound to target; the renderer streams a
range per draw, so a frame makes hundreds of these. Apple's driver waits
for the GPU on a glBufferSubData into a buffer queued draws use (about
50 us each, measured on an M2 Pro), where a map that promises no queued
draw reads the range costs well under a microsecond, with the range
invalidated or not (0.34 and 0.37 us). The promise holds: the renderer only
writes ranges no queued draw reads, because host_gl_wait_frame releases the
ring slot first. (Android's host leaves the invalidation out: Adreno pays
for it.) */
void host_gl_buffer_write(uint32_t target, uint32_t offset, uint32_t size, const void *data)
{
	void *mapping = glMapBufferRange(target, (GLintptr)offset, (GLsizeiptr)size,
		GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

	if (!mapping)
	{
		glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)size, data);
		return;
	}
	memcpy(mapping, data, size);
	glUnmapBuffer(target);
}
