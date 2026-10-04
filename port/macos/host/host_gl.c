/*
HOST_GL.C

OpenGL on behalf of the guest's renderer (port/linux/src/d3d8_gl.c, which
speaks GLES3 through the guest_gl.c wrappers). The host resolves each
entry point at run time through SDL's GL loader, against the context the
guest made (host_sdl.c). With SDL_HINT_OPENGL_ES_DRIVER set (host_sdl.c),
SDL hands out ANGLE's entries when ANGLE is around, else the system
GLES-over-OpenGL translation; the guest's code is the same either way.

The helpers are the Android ones (port/android/host/host_gl.c) with the
loader swapped: strings copied back, one-word buffer reads through the
mapping (Apple's GL has glGetBufferSubData), the frame fences, and the
buffer writes through an unsynchronized mapping.
*/

#include "host.h"

#include <SDL3/SDL.h>
#include <string.h>

#include "host_gl_types.h"

void *host_gl_resolve(const char *name)
{
	void *function = SDL_GL_GetProcAddress(name);

	if (!function)
		host_logf(HOST_LOG_WARN, "GL entry point %s is not available", name);
	return function;
}

void host_gl_get_string(uint32_t name, int index, char *buffer, uint32_t size)
{
	typedef const GLubyte *(*glGetStringi_t)(unsigned, unsigned);
	typedef const GLubyte *(*glGetString_t)(unsigned);
	static glGetStringi_t get_stringi;
	static glGetString_t get_string;
	const GLubyte *text;

	if (!size)
		return;
	buffer[0] = 0;
	if (!get_stringi)
	{
		get_stringi = (glGetStringi_t)SDL_GL_GetProcAddress("glGetStringi");
		get_string = (glGetString_t)SDL_GL_GetProcAddress("glGetString");
	}
	if (!get_stringi || !get_string)
		return;
	text = index >= 0 ? get_stringi(name, (unsigned)index) : get_string(name);
	if (text)
	{
		strncpy(buffer, (const char *)text, size - 1);
		buffer[size - 1] = 0;
	}
}

int host_gl_has_extension(const char *name)
{
	typedef void (*glGetIntegerv_t)(unsigned, int *);
	typedef const GLubyte *(*glGetStringi_t)(unsigned, unsigned);
	static glGetIntegerv_t get_integer;
	static glGetStringi_t get_stringi;
	GLint count = 0, index;

	if (!get_integer)
	{
		get_integer = (glGetIntegerv_t)SDL_GL_GetProcAddress("glGetIntegerv");
		get_stringi = (glGetStringi_t)SDL_GL_GetProcAddress("glGetStringi");
	}
	if (!get_integer || !get_stringi)
		return 0;
	get_integer(0x8616 /* GL_NUM_EXTENSIONS */, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)get_stringi(0x1F03 /* GL_EXTENSIONS */, (unsigned)index);

		if (extension && !strcmp(extension, name))
			return 1;
	}
	return 0;
}

/* one 32-bit word of a buffer object (the visibility test counters of
d3d8_gl.c); the guest's GLES has no glGetBufferSubData, but this host's
desktop GL (or ANGLE's) does */
uint32_t host_gl_read_buffer_word(uint32_t buffer, uint32_t offset)
{
	typedef void (*glGetBufferSubData_t)(unsigned, long long, long long, void *);
	typedef void (*glBindBuffer_t)(unsigned, unsigned);
	static glGetBufferSubData_t get_buffer_sub_data;
	static glBindBuffer_t bind_buffer;
	uint32_t value = 0;

	if (!get_buffer_sub_data)
	{
		get_buffer_sub_data = (glGetBufferSubData_t)SDL_GL_GetProcAddress("glGetBufferSubData");
		bind_buffer = (glBindBuffer_t)SDL_GL_GetProcAddress("glBindBuffer");
	}
	if (!get_buffer_sub_data || !bind_buffer)
		return 0;
	bind_buffer(0x2400 /* GL_ATOMIC_COUNTER_BUFFER */, buffer);
	get_buffer_sub_data(0x2400, (long long)offset, (long long)sizeof(value), &value);
	bind_buffer(0x2400, 0);
	return value;
}

/* The renderer streams each frame's vertices and indices into the next of
a ring of buffers (d3d8_gl.c). A fence marks the end of each frame's work,
and a buffer is written again only once the GPU has passed the fence of the
frame that last used it. */
#define FRAME_FENCE_SLOTS 8

static void *frame_fences[FRAME_FENCE_SLOTS];

void host_gl_fence_frame(uint32_t slot)
{
	typedef void *(*glFenceSync_t)(unsigned, int);
	typedef void (*glDeleteSync_t)(void *);
	static glFenceSync_t fence_sync;
	static glDeleteSync_t delete_sync;

	if (slot >= FRAME_FENCE_SLOTS)
		return;
	if (!fence_sync)
	{
		fence_sync = (glFenceSync_t)SDL_GL_GetProcAddress("glFenceSync");
		delete_sync = (glDeleteSync_t)SDL_GL_GetProcAddress("glDeleteSync");
	}
	if (!fence_sync)
		return;
	if (frame_fences[slot] && delete_sync)
		delete_sync(frame_fences[slot]);
	frame_fences[slot] = fence_sync(0x9117 /* GL_SYNC_GPU_COMMANDS_COMPLETE */, 0);
}

void host_gl_wait_frame(uint32_t slot)
{
	typedef unsigned long long (*glClientWaitSync_t)(void *, unsigned, unsigned long long);
	typedef void (*glDeleteSync_t)(void *);
	static glClientWaitSync_t client_wait;
	static glDeleteSync_t delete_sync;

	if (slot >= FRAME_FENCE_SLOTS || !frame_fences[slot])
		return;
	if (!client_wait)
	{
		client_wait = (glClientWaitSync_t)SDL_GL_GetProcAddress("glClientWaitSync");
		delete_sync = (glDeleteSync_t)SDL_GL_GetProcAddress("glDeleteSync");
	}
	if (!client_wait)
		return;
	/* at most a second: a lost context must not hang the game */
	client_wait(frame_fences[slot], 1 /* GL_SYNC_FLUSH_COMMANDS_BIT */, 1000000000ull);
	if (delete_sync)
		delete_sync(frame_fences[slot]);
	frame_fences[slot] = NULL;
}

/* writes data into the buffer bound to target. The renderer streams a
range per draw, so a frame makes hundreds of these, and the cost per call
rather than per byte is what a frame is made of. As on Android
(host_gl.c's note), the mapping is unsynchronized: the renderer only
writes ranges that no queued draw reads, because host_gl_wait_frame
releases the ring slot first. */
void host_gl_buffer_write(uint32_t target, uint32_t offset, uint32_t size, const void *data)
{
	typedef void *(*glMapBufferRange_t)(unsigned, long long, long long, unsigned);
	typedef void (*glUnmapBuffer_t)(unsigned);
	typedef void (*glBufferSubData_t)(unsigned, long long, long long, const void *);
	static glMapBufferRange_t map_range;
	static glUnmapBuffer_t unmap_buffer;
	static glBufferSubData_t sub_data;
	void *mapping;

	if (!map_range)
	{
		map_range = (glMapBufferRange_t)SDL_GL_GetProcAddress("glMapBufferRange");
		unmap_buffer = (glUnmapBuffer_t)SDL_GL_GetProcAddress("glUnmapBuffer");
		sub_data = (glBufferSubData_t)SDL_GL_GetProcAddress("glBufferSubData");
	}
	if (!map_range || !unmap_buffer)
	{
		if (sub_data)
			sub_data(target, (long long)offset, (long long)size, data);
		return;
	}
	mapping = map_range(target, (long long)offset, (long long)size,
		0x1 /* GL_MAP_WRITE_BIT */ | 0x10 /* GL_MAP_UNSYNCHRONIZED_BIT */);
	if (!mapping)
	{
		if (sub_data)
			sub_data(target, (long long)offset, (long long)size, data);
		return;
	}
	memcpy(mapping, data, size);
	unmap_buffer(target);
}