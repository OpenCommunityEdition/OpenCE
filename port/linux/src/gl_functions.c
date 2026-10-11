/*
GL_FUNCTIONS.C

Run-time resolution of the OpenGL entry points listed in gl.h.
*/

#include "platform.h"
#define GL_FUNCTIONS_DEFINE
#include "gl.h"

#include <SDL3/SDL.h>

#define GL_DEFINE_FUNCTION(name) __typeof__(&name) halo_##name;
GL_FUNCTIONS(GL_DEFINE_FUNCTION)
#ifdef HALO_GLES
GL_OPTIONAL_FUNCTIONS(GL_DEFINE_FUNCTION)
#endif

int gl_functions_load(void)
{
	int success = TRUE;

#ifdef HALO_WEB
/* WebGL 2 is OpenGL ES 3.0: the 3.1 and 3.2 functions it lacks
(glCopyImageSubData, glDrawElementsBaseVertex, glMemoryBarrier) stay NULL,
and the renderer calls them only where its capabilities say the context
has them (xgpu_capabilities, d3d8_gl.c) */
#define GL_MISSING_FAILS 0
#else
#define GL_MISSING_FAILS 1
#endif
#define GL_LOAD_FUNCTION(name) \
	halo_##name = (__typeof__(halo_##name))SDL_GL_GetProcAddress(#name); \
	if (!halo_##name) \
	{ \
		if (GL_MISSING_FAILS) \
			platform_log("OpenGL function %s is unavailable", #name); \
		success = success && !GL_MISSING_FAILS; \
	}
	GL_FUNCTIONS(GL_LOAD_FUNCTION)
#undef GL_LOAD_FUNCTION
#if defined(HALO_GLES) && !defined(HALO_WEB)
	/* a missing one is no failure: the renderer uses each only where the
	context lists its extension (gl_initialize, d3d8_gl.c). WebGL has none of
	these, and its table is not asked */
#define GL_LOAD_OPTIONAL_FUNCTION(name) \
	halo_##name = (__typeof__(halo_##name))SDL_GL_GetProcAddress(#name);
	GL_OPTIONAL_FUNCTIONS(GL_LOAD_OPTIONAL_FUNCTION)
#undef GL_LOAD_OPTIONAL_FUNCTION
#endif
	return success;
}
