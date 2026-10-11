#ifndef HALO_WEB_SHADER_CACHE_H
#define HALO_WEB_SHADER_CACHE_H

#include "xgpu.h"

/* Generated game shaders only: their handles live for the GL context. */
GLuint web_shader_cache_get(GLenum type, const char *source, const char *what);
void web_shader_cache_linked(GLuint vertex_shader, GLuint fragment_shader);
void web_shader_cache_initialize(void (*prepare_program)(GLuint, GLuint));

#endif
