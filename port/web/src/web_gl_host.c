/*
WEB_GL_HOST.C

The services the Android host gives the OpenGL ES renderer
(port/linux/src/xgpu.h; port/android/host/host_gl.c), for WebGL 2.
*/

#include <GLES3/gl3.h>
#include <string.h>

/* (Emscripten's, for WebGL 2's getBufferSubData) */
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data);

int host_gl_has_extension(const char *name)
{
	/* WebGL's names for the extensions the renderer asks for (Emscripten
	gives WebGL's names with GL_ in front) */
	static const struct
	{
		const char *gl;
		const char *webgl;
	} aliases[] =
	{
		{ "GL_EXT_texture_compression_s3tc", "GL_WEBGL_compressed_texture_s3tc" },
		{ "GL_EXT_texture_compression_dxt1", "GL_WEBGL_compressed_texture_s3tc" },
		{ "GL_ANGLE_texture_compression_dxt3", "GL_WEBGL_compressed_texture_s3tc" },
		{ "GL_ANGLE_texture_compression_dxt5", "GL_WEBGL_compressed_texture_s3tc" },
	};
	const char *wanted = name;
	GLint count = 0, index;
	size_t alias;

	if (!name)
		return 0;
	for (alias = 0; alias < sizeof(aliases) / sizeof(aliases[0]); alias++)
	{
		if (!strcmp(aliases[alias].gl, name))
			wanted = aliases[alias].webgl;
	}
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (index = 0; index < count; index++)
	{
		const char *extension = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)index);

		if (extension && !strcmp(extension, wanted))
			return 1;
	}
	return 0;
}

/* (the visibility tests' atomic counters, which WebGL 2 does not have; for
completeness) */
void host_gl_read_buffer(unsigned int buffer, unsigned int offset, unsigned int size, void *data)
{
	glBindBuffer(GL_COPY_READ_BUFFER, buffer);
	glGetBufferSubData(GL_COPY_READ_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
}

/* A WebGL buffer write is a copy the browser orders with the draws around
it: a draw already made reads what it was given, so the renderer's ring of
streaming buffers never waits for the GPU here. */
void host_gl_buffer_write(unsigned int target, unsigned int offset, unsigned int size, const void *data)
{
	glBufferSubData((GLenum)target, (GLintptr)offset, (GLsizeiptr)size, data);
}

void host_gl_fence_frame(unsigned int slot)
{
	(void)slot;
}

void host_gl_wait_frame(unsigned int slot)
{
	(void)slot;
}
