/*
WEB_HOST.C

The services the Android host supplies to the shared OpenGL ES renderer
(port/linux/src/xgpu.h), for WebGL 2.

WebGL copies buffer data when it is given, so the renderer's ring of
stream buffers never has to wait for the GPU, and it has no fences a thread
that never returns to its event loop could wait on.
*/

#include <GLES3/gl3.h>
#include <string.h>

#include "platform.h"

int host_gl_has_extension(const char *name)
{
	/* WebGL's names for the extensions the renderer asks for */
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
		{ "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_filter_anisotropic" },
	};
	const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
	const char *wanted = name;
	size_t index, length;
	const char *found;

	if (!extensions || !name)
		return 0;
	for (index = 0; index < sizeof(aliases) / sizeof(aliases[0]); index++)
	{
		if (!strcmp(aliases[index].gl, name))
		{
			wanted = aliases[index].webgl;
			break;
		}
	}
	length = strlen(wanted);
	for (found = strstr(extensions, wanted); found; found = strstr(found + 1, wanted))
	{
		if ((found == extensions || found[-1] == ' ') && (found[length] == ' ' || found[length] == '\0'))
			return 1;
	}
	return 0;
}

unsigned int host_gl_read_buffer_word(unsigned int buffer, unsigned int offset)
{
	/* only for atomic counters, which WebGL 2 does not have */
	(void)buffer;
	(void)offset;
	return 0;
}

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
