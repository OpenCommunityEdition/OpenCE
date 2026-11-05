/* Additional model samplers use units 6/7; the game's four texture stages
   and their cached bindings are not changed. */
#include "plasma_occlusion.h"
#include "xgpu.h"
#include <stdio.h>
#include <string.h>

static int model_bindings_known;

void plasma_occlusion_model_initialize(unsigned long program, struct plasma_occlusion_uniforms *uniforms)
{
	int slot;
	char name[40];
	uniforms->known = 0;
	uniforms->enabled = glGetUniformLocation(program, "plasma_occ_enabled");
	if (uniforms->enabled < 0) return;
	for (slot = 0; slot < 2; slot++)
	{
		snprintf(name, sizeof(name), "plasma_occ_transform%d", slot);
		uniforms->transform[slot] = glGetUniformLocation(program, name);
		snprintf(name, sizeof(name), "plasma_occ_texture%d", slot);
		glUniform1i(glGetUniformLocation(program, name), 6 + slot);
	}
}

/* Each model mask is immutable for its view. Resolve/decode/upload a resource
   once, rather than asking the texture cache again for every material draw.
   Discard the table at every view boundary, including map reset. */
static GLuint model_texture_get(DWORD const *resource)
{
	static struct { DWORD const *resource; GLuint texture; } resolved[16];
	static unsigned long view;
	static int count;
	unsigned long current = plasma_occlusion_view_serial();
	GLuint texture;
	GLint active;
	GLenum target;
	struct xgpu_texture_description description;
	int i;
	if (view != current) { view = current; count = 0; model_bindings_known = 0; }
	for (i = 0; i < count; i++)
		if (resolved[i].resource == resource) return resolved[i].texture;
	/* A cache miss can upload and bind a texture. Confine that side effect
	   to our units; ordinary material draws need no active-unit query. */
	glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
	glActiveTexture(GL_TEXTURE0 + 6);
	texture = xgpu_texture_get(resource, NULL, &target, &description);
	glActiveTexture(active);
	model_bindings_known = 0;
	if (texture && count < 16)
	{
		resolved[count].resource = resource;
		resolved[count++].texture = texture;
	}
	return texture;
}

static void model_bind_textures(const GLuint textures[2], GLuint sampler)
{
	static GLuint bound[2], bound_sampler;
	static unsigned long view;
	unsigned long current = plasma_occlusion_view_serial();
	GLint active;
	int slot;
	if (view != current) { view = current; model_bindings_known = 0; }
	if (sampler && model_bindings_known && bound_sampler == sampler &&
		!memcmp(bound, textures, sizeof(bound))) return;
	/* Only this module owns units 6/7. Reset at every view (including map
	   cleanup), and after any resource upload that may change their bindings.
	   Desktop multi-bind leaves the active unit and stock stages untouched. */
#ifndef HALO_GLES
	if (sampler)
	{
		glBindTextures(6, 2, textures);
		if (!model_bindings_known || bound_sampler != sampler)
			for (slot = 0; slot < 2; slot++) glBindSampler(6 + slot, sampler);
	}
	else
#endif
	{
		glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
		for (slot = 0; slot < 2; slot++)
		{
			glActiveTexture(GL_TEXTURE0 + 6 + slot);
			glBindTexture(GL_TEXTURE_3D, textures[slot]);
			glBindSampler(6 + slot, sampler);
			if (!sampler)
			{
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			}
		}
		glActiveTexture(active);
	}
	memcpy(bound, textures, sizeof(bound));
	bound_sampler = sampler;
	model_bindings_known = 1;
}

void plasma_occlusion_bind_model(struct plasma_occlusion_uniforms *uniforms)
{
	GLint active;
	GLuint texture, textures[2];
	DWORD const *resource;
	int slot;
	float transform[4], enabled[2] = {0, 0};
	/* Like the fallback texture, this immutable sampler belongs to the GL
	   context, not a map or light. It never touches the game's four stages. */
	static GLuint white, sampler;
	if (uniforms->enabled < 0)
		return;
	if (!sampler)
	{
		glGenSamplers(1, &sampler);
		if (sampler)
		{
			glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}
	}
	for (slot = 0; slot < 2; slot++)
	{
		resource = plasma_occlusion_model_texture(slot, transform);
		texture = 0;
		if (resource)
			texture = model_texture_get(resource);
		if (texture)
		{
			enabled[slot] = 1;
			if (!(uniforms->known & (1 << slot)) || memcmp(uniforms->last_transform[slot], transform, sizeof(transform)))
			{
				glUniform4fv(uniforms->transform[slot], 1, transform);
				memcpy(uniforms->last_transform[slot], transform, sizeof(transform));
				uniforms->known |= 1 << slot;
			}
		}
		else
		{
			if (!white)
			{
				unsigned char pixel[4] = {255, 255, 255, 255};
				glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
				glActiveTexture(GL_TEXTURE0 + 6);
				glGenTextures(1, &white);
				glBindTexture(GL_TEXTURE_3D, white);
				glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
				glActiveTexture(active);
				model_bindings_known = 0;
			}
			texture = white;
		}
		textures[slot] = texture;
	}
	model_bind_textures(textures, sampler);
	if (!(uniforms->known & 4) || memcmp(uniforms->last_enabled, enabled, sizeof(enabled)))
	{
		glUniform2f(uniforms->enabled, enabled[0], enabled[1]);
		memcpy(uniforms->last_enabled, enabled, sizeof(enabled));
		uniforms->known |= 4;
	}
}
