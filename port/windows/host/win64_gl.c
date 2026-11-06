/*
WIN64_GL.C

OpenGL for the guest on the Windows x64 host: the Linux host's host_gl.c
(its helpers, the same imports), with the entry points from
SDL_GL_GetProcAddress, which on Windows answers only once a context is
current (wglGetProcAddress). So the guest's OpenGL imports are resolved
when the guest's first context is created or made current
(host_gl_resolve_entry_points, from the wrappers of
host_sdl_gl_create_context and host_sdl_gl_make_current below, which the
import table names in place of host_sdl.c's), and this file's own
functions are looked up then too.
*/

#include "win64_host.h"

#include <SDL3/SDL.h>
#include <string.h>

/* the few OpenGL types and values this file uses */
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef unsigned char GLubyte;
typedef char GLchar;
typedef intptr_t GLintptr;
typedef intptr_t GLsizeiptr;
typedef uint64_t GLuint64;
typedef struct __GLsync *GLsync;
typedef void (APIENTRY *GLDEBUGPROC)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *);

#define GL_EXTENSIONS 0x1F03
#define GL_NUM_EXTENSIONS 0x821D
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#define GL_MAP_PERSISTENT_BIT 0x0040
#define GL_MAP_COHERENT_BIT 0x0080
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B

static struct
{
	const GLubyte *(APIENTRY *GetString)(GLenum);
	const GLubyte *(APIENTRY *GetStringi)(GLenum, GLuint);
	void (APIENTRY *GetIntegerv)(GLenum, GLint *);
	void (APIENTRY *BindBuffer)(GLenum, GLuint);
	void *(APIENTRY *MapBufferRange)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
	unsigned char (APIENTRY *UnmapBuffer)(GLenum);
	void (APIENTRY *BufferSubData)(GLenum, GLintptr, GLsizeiptr, const void *);
	GLsync (APIENTRY *FenceSync)(GLenum, GLbitfield);
	void (APIENTRY *DeleteSync)(GLsync);
	GLenum (APIENTRY *ClientWaitSync)(GLsync, GLbitfield, GLuint64);
	void (APIENTRY *DebugMessageCallback)(GLDEBUGPROC, const void *);
} gl;

static int resolved;

void *host_gl_resolve(const char *name)
{
	return (void *)SDL_GL_GetProcAddress(name);
}

void host_gl_resolve_entry_points(void)
{
	int missing;

	if (resolved || !SDL_GL_GetCurrentContext())
		return;
	resolved = 1;
	gl.GetString = (void *)SDL_GL_GetProcAddress("glGetString");
	gl.GetStringi = (void *)SDL_GL_GetProcAddress("glGetStringi");
	gl.GetIntegerv = (void *)SDL_GL_GetProcAddress("glGetIntegerv");
	gl.BindBuffer = (void *)SDL_GL_GetProcAddress("glBindBuffer");
	gl.MapBufferRange = (void *)SDL_GL_GetProcAddress("glMapBufferRange");
	gl.UnmapBuffer = (void *)SDL_GL_GetProcAddress("glUnmapBuffer");
	gl.BufferSubData = (void *)SDL_GL_GetProcAddress("glBufferSubData");
	gl.FenceSync = (void *)SDL_GL_GetProcAddress("glFenceSync");
	gl.DeleteSync = (void *)SDL_GL_GetProcAddress("glDeleteSync");
	gl.ClientWaitSync = (void *)SDL_GL_GetProcAddress("glClientWaitSync");
	gl.DebugMessageCallback = (void *)SDL_GL_GetProcAddress("glDebugMessageCallback");
	missing = host_thunks_resolve_gl(host_gl_resolve);
	host_logf(missing ? HOST_LOG_WARN : HOST_LOG_INFO, "OpenGL entry points resolved (%d unavailable)", missing);
}

/* host_sdl.c's, which the import table calls through these */
uint32_t host_sdl_gl_create_context(uint32_t window);
int host_sdl_gl_make_current(uint32_t window, uint32_t context);

uint32_t win64_sdl_gl_create_context(uint32_t window)
{
	uint32_t context = host_sdl_gl_create_context(window);

	if (context)
		host_gl_resolve_entry_points();
	return context;
}

int win64_sdl_gl_make_current(uint32_t window, uint32_t context)
{
	int result = host_sdl_gl_make_current(window, context);

	if (result && context)
		host_gl_resolve_entry_points();
	return result;
}

/* an OpenGL import the guest called before a context existed */
void host_thunk_unresolved(void)
{
	host_fatal("the game called an OpenGL function before it had a context, or one the driver lacks");
}

void host_gl_get_string(uint32_t name, int index, char *buffer, uint32_t size)
{
	const GLubyte *text = NULL;

	if (!size)
		return;
	buffer[0] = 0;
	if (index >= 0 && gl.GetStringi)
		text = gl.GetStringi(name, (GLuint)index);
	else if (index < 0 && gl.GetString)
		text = gl.GetString(name);
	if (text)
	{
		strncpy(buffer, (const char *)text, size - 1);
		buffer[size - 1] = 0;
	}
}

int host_gl_has_extension(const char *name)
{
	GLint count = 0, index;

	if (!gl.GetIntegerv || !gl.GetStringi)
		return 0;
	gl.GetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)gl.GetStringi(GL_EXTENSIONS, (GLuint)index);

		if (extension && !strcmp(extension, name))
			return 1;
	}
	return 0;
}

void host_gl_read_buffer(uint32_t buffer, uint32_t offset, uint32_t size, void *data)
{
	const void *mapping = NULL;

	if (gl.BindBuffer && gl.MapBufferRange && gl.UnmapBuffer)
	{
		gl.BindBuffer(GL_COPY_READ_BUFFER, buffer);
		mapping = gl.MapBufferRange(GL_COPY_READ_BUFFER, offset, size, GL_MAP_READ_BIT);
	}
	if (mapping)
	{
		memcpy(data, mapping, size);
		gl.UnmapBuffer(GL_COPY_READ_BUFFER);
	}
	else
	{
		memset(data, 0, size);
	}
	if (gl.BindBuffer)
		gl.BindBuffer(GL_COPY_READ_BUFFER, 0);
}

/* (host_gl.c explains the ring of frame buffers and their fences) */
#define FRAME_FENCE_SLOTS 8

static GLsync frame_fences[FRAME_FENCE_SLOTS];

void host_gl_fence_frame(uint32_t slot)
{
	if (slot >= FRAME_FENCE_SLOTS || !gl.FenceSync || !gl.DeleteSync)
		return;
	if (frame_fences[slot])
		gl.DeleteSync(frame_fences[slot]);
	frame_fences[slot] = gl.FenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void host_gl_wait_frame(uint32_t slot)
{
	if (slot >= FRAME_FENCE_SLOTS || !frame_fences[slot] || !gl.ClientWaitSync)
		return;
	gl.ClientWaitSync(frame_fences[slot], GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ull);
	gl.DeleteSync(frame_fences[slot]);
	frame_fences[slot] = NULL;
}

void host_gl_buffer_write(uint32_t target, uint32_t offset, uint32_t size, const void *data)
{
	void *mapping = gl.MapBufferRange ? gl.MapBufferRange(target, offset, size,
		GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT) : NULL;

	if (!mapping)
	{
		if (gl.BufferSubData)
			gl.BufferSubData(target, offset, size, data);
		return;
	}
	memcpy(mapping, data, size);
	gl.UnmapBuffer(target);
}

static void APIENTRY host_gl_debug_message(GLenum origin, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar *message, const void *user)
{
	(void)origin; (void)id; (void)length; (void)user;
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		host_logf(HOST_LOG_WARN, "GL %s: %s", type == GL_DEBUG_TYPE_ERROR ? "error" : "debug", message);
}

void host_gl_debug_output(void)
{
	if (gl.DebugMessageCallback)
		gl.DebugMessageCallback(host_gl_debug_message, NULL);
}

static const volatile void *results_mapping;

int host_gl_map_results(uint32_t buffer, uint32_t size)
{
	if (!gl.BindBuffer || !gl.MapBufferRange)
		return 0;
	gl.BindBuffer(GL_COPY_READ_BUFFER, buffer);
	results_mapping = gl.MapBufferRange(GL_COPY_READ_BUFFER, 0, size,
		GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	gl.BindBuffer(GL_COPY_READ_BUFFER, 0);
	return results_mapping != NULL;
}

void host_gl_read_results(void *data, uint32_t size)
{
	if (results_mapping)
		memcpy(data, (const void *)results_mapping, size);
}
