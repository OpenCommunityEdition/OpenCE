/*
HOST_GL.C

OpenGL for the guest. Its generated entry points (guest_gl.c, for the
OpenGL ES 3 subset the renderer uses, port/linux/src/gl.h) import
hostgl_<function>, resolved here to the generated wrappers
(tools/macos_host_wrappers.py), which add the guest's base to its pointers
and call the system's OpenGL 4.1. Strings and buffer writes have functions
of their own.
*/

#include "host.h"

#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#include <string.h>

/* host_gl_wrappers.c (generated) */
void *host_gl_wrapper(const char *name);

void *host_gl_resolve(const char *name)
{
	return host_gl_wrapper(name);
}

void host_gl_get_string(uint32_t name, int index, uint32_t buffer, uint32_t size)
{
	const GLubyte *text = index >= 0 ? glGetStringi(name, (GLuint)index) : glGetString(name);
	char *result = G2H(buffer);

	if (!result || !size)
		return;
	result[0] = 0;
	if (text)
	{
		strncpy(result, (const char *)text, size - 1);
		result[size - 1] = 0;
	}
}

int host_gl_has_extension(uint32_t name)
{
	const char *wanted = G2H(name);
	GLint count = 0, index;

	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)index);

		if (extension && !strcmp(extension, wanted))
			return 1;
	}
	return 0;
}

/* one 32-bit word of a buffer object (the renderer's visibility counters,
which it has only with atomic counters: never on OpenGL 4.1) */
uint32_t host_gl_read_buffer_word(uint32_t buffer, uint32_t offset)
{
	uint32_t value = 0;
	GLint previous = 0;

	glGetIntegerv(GL_COPY_READ_BUFFER, &previous);
	glBindBuffer(GL_COPY_READ_BUFFER, buffer);
	glGetBufferSubData(GL_COPY_READ_BUFFER, offset, sizeof(value), &value);
	glBindBuffer(GL_COPY_READ_BUFFER, (GLuint)previous);
	return value;
}

/* The renderer streams each frame's vertices and indices into the next of
a ring of buffers (d3d8_gl.c). A fence marks the end of each frame's work,
and a buffer is written again only once the GPU has passed the fence of the
frame that last used it (port/android/host/host_gl.c has the story). */
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

/* writes data into the buffer bound to target, without waiting: the ring
(above) keeps the GPU off the range */
void host_gl_buffer_write(uint32_t target, uint32_t offset, uint32_t size, uint32_t data)
{
	void *mapping = glMapBufferRange(target, offset, size, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	if (!mapping)
	{
		glBufferSubData(target, offset, size, G2H(data));
		return;
	}
	memcpy(mapping, G2H(data), size);
	glUnmapBuffer(target);
}
