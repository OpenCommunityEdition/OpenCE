/*
GPU_GL.C

The OpenGL backend (gpu.h): OpenGL 4.5 on the desktop, OpenGL ES 3 on
Android. A gpu_texture is the GL texture's name.

Render targets are textures. With multisampling, draws go to a multisampled
renderbuffer of the target's instead, whose pixels are resolved into the
texture before anything reads it (gpu_gl_resolve).
*/

#include "gpu_gl.h"
#include "port_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83f1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83f2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83f3
#endif

#ifdef HALO_ANDROID
/* what the context supports (gpu_initialize) */
struct xgpu_capabilities xgpu_capabilities;
#endif

static void color_to_vec4(uint32_t color, float *out)
{
	out[0] = ((color >> 16) & 0xff) / 255.0f;
	out[1] = ((color >> 8) & 0xff) / 255.0f;
	out[2] = (color & 0xff) / 255.0f;
	out[3] = ((color >> 24) & 0xff) / 255.0f;
}

/* ---------- GL state cache

Consecutive draws share most of their state, but each sets all of it: the
setters here skip the call when GL already holds the value. Code that
changes GL state behind the cache's back (clears, presentation, texture
uploads, render target and framebuffer creation) calls
xgpu_gl_state_invalidate (gpu_gl.h), after which every value is set again.
Unknown values are all ones, which no real value matches (floats become
NaN, which compares unequal to everything). */

#ifdef HALO_ANDROID
struct attribute_pointer
{
	GLuint buffer;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLboolean integer;
	GLsizei stride;
	unsigned long offset;
};
#else
/* desktop GL (4.3) separates an attribute's format from the buffer it
reads: the attributes of a stream share one binding, so a draw that moves
the stream rebinds it once instead of pointing each attribute again */
struct attribute_format
{
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLboolean integer;
	GLuint relative_offset;
	GLuint binding;
};

struct vertex_binding
{
	GLuint buffer;
	unsigned long offset;
	GLsizei stride;
};

/* a binding per stream (setup_streams); GL has at least 16 */
#define VERTEX_BINDING_COUNT 16

/* a vertex array object for each vertex layout (the attributes a draw
enables, and each one's format and binding), made once: a draw binds its
layout's and points the bindings at its streams, rather than enabling,
formatting and binding each attribute again (setup_streams) */
struct vertex_layout
{
	unsigned long enabled;
	struct attribute_format formats[GPU_ATTRIBUTE_COUNT];
};

struct vertex_array_entry
{
	struct vertex_array_entry *next;
	unsigned long hash;
	struct vertex_layout layout;
	GLuint vertex_array;
	/* its bindings, as last pointed (nothing else changes them) */
	struct vertex_binding bindings[VERTEX_BINDING_COUNT];
};
#endif

static struct
{
	GLuint program;
	GLuint framebuffer;
	GLint viewport[4];
	GLint scissor[4];
	float depth_range[2];
	unsigned char depth_test, stencil_test, blend, cull_face, offset_fill, offset_line;
	unsigned char scissor_test;
	GLenum depth_function;
	unsigned char depth_mask;
	GLenum stencil_function;
	GLint stencil_reference;
	GLuint stencil_value_mask;
	GLenum stencil_operations[3];
	GLuint stencil_write_mask;
	GLenum blend_source, blend_destination, blend_equation;
	float blend_color[4];
	unsigned char color_mask;
	GLenum front_face, cull_mode, polygon_mode;
	float polygon_offset[2];
	GLenum active_texture;
	/* per unit: the GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP and GL_TEXTURE_3D
	bindings */
	GLuint textures[GPU_STAGE_COUNT][3];
	GLuint samplers[GPU_STAGE_COUNT];
	GLuint array_buffer;
	GLuint element_array_buffer;
	unsigned char attribute_enabled[GPU_ATTRIBUTE_COUNT];
#ifdef HALO_ANDROID
	struct attribute_pointer attribute_pointers[GPU_ATTRIBUTE_COUNT];
#else
	GLuint vertex_array;
#endif
	/* a disabled attribute's value; kind 1 is the integer zero */
	unsigned char attribute_value_kind[GPU_ATTRIBUTE_COUNT];
	float attribute_values[GPU_ATTRIBUTE_COUNT][4];
} gl_state;

#ifndef HALO_ANDROID
#define VERTEX_ARRAY_BUCKET_COUNT 64
static struct vertex_array_entry *vertex_array_buckets[VERTEX_ARRAY_BUCKET_COUNT];
static struct vertex_array_entry *current_vertex_array;
#endif

void xgpu_gl_state_invalidate(void)
{
	memset(&gl_state, 0xff, sizeof(gl_state));
}

static void state_enable(unsigned char *shadow, GLenum capability, BOOL enabled)
{
	unsigned char value = enabled ? 1 : 0;

	if (*shadow == value)
		return;
	*shadow = value;
	if (value)
		glEnable(capability);
	else
		glDisable(capability);
}

static void state_program(GLuint program)
{
	if (gl_state.program != program)
	{
		gl_state.program = program;
		glUseProgram(program);
	}
}

static void state_framebuffer(GLuint framebuffer)
{
	if (gl_state.framebuffer != framebuffer)
	{
		gl_state.framebuffer = framebuffer;
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	}
}

/* gl_state.textures' slot of a target */
static int texture_slot(GLenum target)
{
	return target == GL_TEXTURE_CUBE_MAP ? 1 : target == GL_TEXTURE_3D ? 2 : 0;
}

#ifdef HALO_ANDROID
static void state_texture(int unit, GLenum target, GLuint texture)
{
	int slot = texture_slot(target);

	if (gl_state.textures[unit][slot] == texture)
		return;
	if (gl_state.active_texture != GL_TEXTURE0 + (GLenum)unit)
	{
		gl_state.active_texture = GL_TEXTURE0 + (GLenum)unit;
		glActiveTexture(gl_state.active_texture);
	}
	gl_state.textures[unit][slot] = texture;
	glBindTexture(target, texture);
}
#endif

static void state_sampler(int unit, GLuint sampler)
{
	if (gl_state.samplers[unit] != sampler)
	{
		gl_state.samplers[unit] = sampler;
		glBindSampler((GLuint)unit, sampler);
	}
}

static void state_array_buffer(GLuint buffer)
{
	if (gl_state.array_buffer != buffer)
	{
		gl_state.array_buffer = buffer;
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
	}
}

static void state_element_array_buffer(GLuint buffer)
{
	if (gl_state.element_array_buffer != buffer)
	{
		gl_state.element_array_buffer = buffer;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	}
}

#ifdef HALO_ANDROID
/* enables the attribute, reading size elements of type from buffer: each
vertex is stride bytes on from the one before it, starting at
buffer_offset, with the attribute relative_offset bytes into it */
static void state_attribute_stream(GLuint index, GLuint binding, GLuint buffer, GLint size, GLenum type,
	GLboolean normalized, BOOL integer, GLsizei stride, unsigned long buffer_offset, unsigned long relative_offset)
{
	struct attribute_pointer *pointer = &gl_state.attribute_pointers[index];
	unsigned long offset = buffer_offset + relative_offset;

	(void)binding;
	if (gl_state.attribute_enabled[index] != 1)
	{
		gl_state.attribute_enabled[index] = 1;
		glEnableVertexAttribArray(index);
	}
	if (pointer->buffer == buffer && pointer->size == size && pointer->type == type &&
		pointer->normalized == normalized && pointer->integer == (integer ? GL_TRUE : GL_FALSE) &&
		pointer->stride == stride && pointer->offset == offset)
	{
		return;
	}
	state_array_buffer(buffer);
	if (integer)
		glVertexAttribIPointer(index, size, type, stride, (const void *)offset);
	else
		glVertexAttribPointer(index, size, type, normalized, stride, (const void *)offset);
	pointer->buffer = buffer;
	pointer->size = size;
	pointer->type = type;
	pointer->normalized = normalized;
	pointer->integer = integer ? GL_TRUE : GL_FALSE;
	pointer->stride = stride;
	pointer->offset = offset;
}
#else
/* the vertex array of a layout, made the first time (its attributes enabled,
formatted and bound to their bindings) */
static struct vertex_array_entry *vertex_array_get(const struct vertex_layout *layout)
{
	const unsigned char *bytes = (const unsigned char *)layout;
	unsigned long hash = 2166136261UL, index;
	struct vertex_array_entry **bucket, *entry;

	for (index = 0; index < sizeof(*layout); index++)
		hash = (hash ^ bytes[index]) * 16777619UL;
	bucket = &vertex_array_buckets[hash % VERTEX_ARRAY_BUCKET_COUNT];
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->hash == hash && !memcmp(&entry->layout, layout, sizeof(*layout)))
			return entry;
	}
	entry = calloc(1, sizeof(*entry));
	entry->hash = hash;
	entry->layout = *layout;
	memset(entry->bindings, 0xff, sizeof(entry->bindings));
	glGenVertexArrays(1, &entry->vertex_array);
	glBindVertexArray(entry->vertex_array);
	gl_state.vertex_array = entry->vertex_array;
	/* (the index buffer's binding is the vertex array's) */
	gl_state.element_array_buffer = (GLuint)-1;
	for (index = 0; index < GPU_ATTRIBUTE_COUNT; index++)
	{
		const struct attribute_format *format = &layout->formats[index];

		if (!(layout->enabled & (1UL << index)))
			continue;
		glEnableVertexAttribArray((GLuint)index);
		if (format->integer)
			glVertexAttribIFormat((GLuint)index, format->size, format->type, format->relative_offset);
		else
			glVertexAttribFormat((GLuint)index, format->size, format->type, format->normalized, format->relative_offset);
		glVertexAttribBinding((GLuint)index, format->binding);
	}
	entry->next = *bucket;
	*bucket = entry;
	return entry;
}

static void state_vertex_array(struct vertex_array_entry *entry)
{
	if (gl_state.vertex_array != entry->vertex_array)
	{
		gl_state.vertex_array = entry->vertex_array;
		glBindVertexArray(entry->vertex_array);
		/* (the index buffer's binding is the vertex array's) */
		gl_state.element_array_buffer = (GLuint)-1;
	}
	current_vertex_array = entry;
}

/* points the bound vertex array's binding at buffer: each vertex is stride
bytes on from the one before it, starting at offset */
static void state_vertex_buffer(GLuint binding, GLuint buffer, unsigned long offset, GLsizei stride)
{
	struct vertex_binding *vertex_binding = &current_vertex_array->bindings[binding];

	if (vertex_binding->buffer != buffer || vertex_binding->offset != offset || vertex_binding->stride != stride)
	{
		glBindVertexBuffer(binding, buffer, (GLintptr)offset, stride);
		vertex_binding->buffer = buffer;
		vertex_binding->offset = offset;
		vertex_binding->stride = stride;
	}
}

/* an attribute of a layout */
static void layout_attribute(struct vertex_layout *layout, unsigned long index, GLuint binding, GLint size,
	GLenum type, GLboolean normalized, BOOL integer, unsigned long relative_offset)
{
	struct attribute_format *format = &layout->formats[index];

	format->size = size;
	format->type = type;
	format->normalized = normalized;
	format->integer = integer ? GL_TRUE : GL_FALSE;
	format->relative_offset = (GLuint)relative_offset;
	format->binding = binding;
	layout->enabled |= 1UL << index;
}
#endif

/* disables the attribute, which then reads value, or the integer zero */
static void state_attribute_value(GLuint index, const float *value)
{
	unsigned char kind = value ? 0 : 1;

#ifdef HALO_ANDROID
	if (gl_state.attribute_enabled[index] != 0)
	{
		gl_state.attribute_enabled[index] = 0;
		glDisableVertexAttribArray(index);
	}
#endif
	if (gl_state.attribute_value_kind[index] == kind &&
		(!value || !memcmp(gl_state.attribute_values[index], value, sizeof(gl_state.attribute_values[index]))))
	{
		return;
	}
	gl_state.attribute_value_kind[index] = kind;
	if (value)
	{
		memcpy(gl_state.attribute_values[index], value, sizeof(gl_state.attribute_values[index]));
		glVertexAttrib4fv(index, value);
	}
	else
	{
		glVertexAttribI4ui(index, 0, 0, 0, 0);
	}
}

/* ---------- textures */

struct texture_record
{
	struct gpu_texture_description description;
	/* the channel of the texels each channel is sampled from
	(gpu_texture_channels) */
	uint8_t channels[4];
	/* a render target's multisampled renderbuffer, which draws go to with
	multisampling, its samples a pixel (0 when it has none), and whether it
	has been drawn into since the texture last had its pixels */
	GLuint multisample;
	int samples;
	BOOL unresolved;
};

/* by texture name (names are small and reused) */
static struct texture_record *texture_records;
static unsigned long texture_record_count;

/* the record of a texture made by gpu_texture_create; an empty one for any
other */
static struct texture_record *texture_record(gpu_texture texture)
{
	static struct texture_record none;

	if (texture >= texture_record_count)
	{
		memset(&none, 0, sizeof(none));
		return &none;
	}
	return &texture_records[texture];
}

static GLenum texture_target(unsigned char type)
{
	return type == GPU_TEXTURE_CUBE ? GL_TEXTURE_CUBE_MAP : type == GPU_TEXTURE_3D ? GL_TEXTURE_3D : GL_TEXTURE_2D;
}

static GLsizei level_dimension(uint32_t dimension, uint32_t level)
{
	return (GLsizei)(dimension >> level ? dimension >> level : 1);
}

gpu_texture gpu_texture_create(const struct gpu_texture_description *description)
{
	GLuint texture = 0;
	struct texture_record *record;
	uint32_t level;

	glGenTextures(1, &texture);
	if (texture >= texture_record_count)
	{
		unsigned long count = texture_record_count ? texture_record_count : 1024;
		struct texture_record *grown;

		while (count <= texture)
			count *= 2;
		grown = realloc(texture_records, count * sizeof(*grown));
		if (!grown)
		{
			platform_log("GPU: no memory to keep texture %u", texture);
			return texture;
		}
		memset(grown + texture_record_count, 0, (count - texture_record_count) * sizeof(*grown));
		texture_records = grown;
		texture_record_count = count;
	}
	record = &texture_records[texture];
	memset(record, 0, sizeof(*record));
	record->description = *description;
	for (level = 0; level < 4; level++)
		record->channels[level] = (uint8_t)level;
	if (description->usage != GPU_USAGE_RENDER_TARGET)
		return texture;
	/* a render target, or a texture whose levels are copied from render
	targets: every level now, its texels undefined */
	glBindTexture(GL_TEXTURE_2D, texture);
	if (description->levels > 1)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)description->levels - 1);
	for (level = 0; level < description->levels; level++)
	{
		GLsizei width = level_dimension(description->width, level);
		GLsizei height = level_dimension(description->height, level);

		if (description->format == GPU_FORMAT_DEPTH_STENCIL)
			glTexImage2D(GL_TEXTURE_2D, (GLint)level, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL,
				GL_UNSIGNED_INT_24_8, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, (GLint)level, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	}
	xgpu_gl_state_invalidate();
	return texture;
}

void gpu_texture_channels(gpu_texture texture, const uint8_t channels[4])
{
	memcpy(texture_record(texture)->channels, channels, 4);
}

static GLenum compressed_format(unsigned char format)
{
	switch (format)
	{
	case GPU_FORMAT_BC1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case GPU_FORMAT_BC2: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	default: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
}

void gpu_texture_upload(gpu_texture texture, uint32_t face, uint32_t level, const void *data, uint32_t size)
{
	const struct texture_record *record = texture_record(texture);
	const struct gpu_texture_description *description = &record->description;
	GLenum target = texture_target(description->type);
	GLenum image_target = description->type == GPU_TEXTURE_CUBE ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
	GLsizei width = level_dimension(description->width, level);
	GLsizei height = level_dimension(description->height, level);
	GLsizei depth = level_dimension(description->depth, level);

	glBindTexture(target, texture);
	xgpu_gl_state_invalidate();
	if (!face && !level)
	{
		if (description->format != GPU_FORMAT_RGBA8)
		{
			/* the channel each channel is sampled from, set at every upload:
			a texture can be given other texels */
			GLint stored[4] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };

#ifdef HALO_ANDROID
			/* BGRA8 texels are given to ES as RGBA, which it reads instead */
			if (description->format == GPU_FORMAT_BGRA8)
			{
				stored[0] = GL_BLUE;
				stored[2] = GL_RED;
			}
#endif
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, stored[record->channels[0] & 3]);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, stored[record->channels[1] & 3]);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, stored[record->channels[2] & 3]);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, stored[record->channels[3] & 3]);
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, (GLint)description->levels - 1);
	}
	switch (description->format)
	{
	case GPU_FORMAT_BC1:
	case GPU_FORMAT_BC2:
	case GPU_FORMAT_BC3:
		if (target == GL_TEXTURE_3D)
			glCompressedTexImage3D(image_target, (GLint)level, compressed_format(description->format), width, height,
				depth, 0, (GLsizei)size, data);
		else
			glCompressedTexImage2D(image_target, (GLint)level, compressed_format(description->format), width, height,
				0, (GLsizei)size, data);
		break;
	default:
	{
		GLenum format = description->format == GPU_FORMAT_RGBA8 ? GL_RGBA : GL_BGRA;

		if (target == GL_TEXTURE_3D)
			glTexImage3D(image_target, (GLint)level, GL_RGBA8, width, height, depth, 0, format, GL_UNSIGNED_BYTE, data);
		else
			glTexImage2D(image_target, (GLint)level, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		break;
	}
	}
}

void gpu_texture_upload_rows(gpu_texture texture, uint32_t first_row, uint32_t rows, const void *data)
{
	const struct gpu_texture_description *description = &texture_record(texture)->description;

	glBindTexture(GL_TEXTURE_2D, texture);
	xgpu_gl_state_invalidate();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLint)first_row, (GLsizei)description->width, (GLsizei)rows,
		description->format == GPU_FORMAT_RGBA8 ? GL_RGBA : GL_BGRA, GL_UNSIGNED_BYTE, data);
}

static void texture_resolve(gpu_texture texture);
static GLuint framebuffer_get(GLuint color, GLuint depth);

#ifdef HALO_ANDROID
/* glCopyImageSubData for ES 3.0/3.1 contexts without the extension */
static void copy_level_by_blit(GLuint source, GLuint destination, GLint level, GLsizei width, GLsizei height)
{
	static GLuint draw_framebuffer;

	if (!draw_framebuffer)
		glGenFramebuffers(1, &draw_framebuffer);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_get(source, 0));
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destination, level);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	/* the blit bypasses the cached state, so the next draw must re-apply it */
	xgpu_gl_state_invalidate();
}
#endif

void gpu_texture_copy_level(gpu_texture source, gpu_texture destination, uint32_t level)
{
	const struct gpu_texture_description *description = &texture_record(source)->description;
	GLsizei width = (GLsizei)description->width, height = (GLsizei)description->height;

	/* (multisampled: its pixels drawn since, resolved) */
	texture_resolve(source);
#ifdef HALO_ANDROID
	if (!xgpu_capabilities.copy_image)
	{
		copy_level_by_blit(source, destination, (GLint)level, width, height);
		return;
	}
#endif
	glCopyImageSubData(source, GL_TEXTURE_2D, 0, 0, 0, 0, destination, GL_TEXTURE_2D, (GLint)level, 0, 0, 0,
		width, height, 1);
}

void gpu_texture_generate_mipmaps(gpu_texture texture, uint32_t base_level)
{
	GLenum target = texture_target(texture_record(texture)->description.type);

	glBindTexture(target, texture);
	xgpu_gl_state_invalidate();
	if (base_level)
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, (GLint)base_level);
	glGenerateMipmap(target);
	if (base_level)
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
}

void gpu_texture_destroy(gpu_texture texture)
{
	/* (render targets, the only textures with renderbuffers, are never
	destroyed) */
	memset(texture_record(texture), 0, sizeof(struct texture_record));
	glDeleteTextures(1, &texture);
	xgpu_gl_state_invalidate();
}

uint32_t gpu_texture_read(gpu_texture texture, void *pixels, uint32_t size)
{
	const struct gpu_texture_description *description = &texture_record(texture)->description;
	uint32_t bytes = description->width * description->height * 4;

	if (description->type != GPU_TEXTURE_2D || description->format == GPU_FORMAT_DEPTH_STENCIL || size < bytes)
		return 0;
	if (description->usage == GPU_USAGE_RENDER_TARGET)
	{
		texture_resolve(texture);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_get(texture, 0));
		glReadPixels(0, 0, (GLsizei)description->width, (GLsizei)description->height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
#ifdef HALO_ANDROID
		{
			/* (read as RGBA) */
			unsigned char *bytes_read = pixels;
			uint32_t texel;

			for (texel = 0; texel < description->width * description->height; texel++)
			{
				unsigned char red = bytes_read[texel * 4];

				bytes_read[texel * 4] = bytes_read[texel * 4 + 2];
				bytes_read[texel * 4 + 2] = red;
			}
		}
#endif
		return bytes;
	}
#ifdef HALO_ANDROID
	/* ES cannot read textures back */
	return 0;
#else
	glBindTexture(GL_TEXTURE_2D, texture);
	xgpu_gl_state_invalidate();
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	return bytes;
#endif
}

/* ---------- render targets' framebuffers */

struct framebuffer_entry
{
	struct framebuffer_entry *next;
	GLuint color;
	GLuint depth;
	/* color and depth are multisampled renderbuffers, not textures */
	BOOL renderbuffers;
	GLuint framebuffer;
};

static struct framebuffer_entry *framebuffers;
/* the framebuffer of these textures, or with renderbuffers, of these
multisampled renderbuffers (texture_multisample) */
static GLuint framebuffer_find(GLuint color, GLuint depth, BOOL renderbuffers)
{
	struct framebuffer_entry *entry, **link;
	GLenum draw_buffer = color ? GL_COLOR_ATTACHMENT0 : GL_NONE;

	/* (found, it moves to the front: each draw asks again for the one
	the draw before it did) */
	for (link = &framebuffers; (entry = *link) != NULL; link = &entry->next)
	{
		if (entry->color == color && entry->depth == depth && entry->renderbuffers == renderbuffers)
		{
			*link = entry->next;
			entry->next = framebuffers;
			framebuffers = entry;
			return entry->framebuffer;
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->color = color;
	entry->depth = depth;
	entry->renderbuffers = renderbuffers;
	glGenFramebuffers(1, &entry->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, entry->framebuffer);
	if (renderbuffers)
	{
		if (color)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
		if (depth)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
	}
	else
	{
		if (color)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
		if (depth)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	}
	glDrawBuffers(1, &draw_buffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		platform_log("framebuffer %u/%u%s is incomplete", color, depth, renderbuffers ? " (multisampled)" : "");
	xgpu_gl_state_invalidate();
	entry->next = framebuffers;
	framebuffers = entry;
	return entry->framebuffer;
}

static GLuint framebuffer_get(GLuint color, GLuint depth)
{
	return framebuffer_find(color, depth, FALSE);
}

GLuint gpu_gl_framebuffer(gpu_texture color, gpu_texture depth)
{
	return framebuffer_get(color, depth);
}

/* ---------- multisampling

A framebuffer's attachments are all multisampled or none is (the device
chooses, gpu_gl_bind_targets), so that of a target's texture and its
renderbuffer only one has pixels drawn since the other had them. The
renderbuffer's pixels are resolved into the texture before anything reads
the texture, and the texture's are put into the renderbuffer when it is
made, or made again for other samples. */

/* the multisampled pixels of a target drawn into since into its texture */
static void texture_resolve(gpu_texture texture)
{
	struct texture_record *target = texture_record(texture);
	BOOL depth = target->description.format == GPU_FORMAT_DEPTH_STENCIL;
	GLint width = (GLint)target->description.width, height = (GLint)target->description.height;
	GLuint read, draw;

	if (!target->unresolved)
		return;
	target->unresolved = FALSE;
	/* (both found first: making a framebuffer binds it) */
	read = depth ? framebuffer_find(0, target->multisample, TRUE) : framebuffer_find(target->multisample, 0, TRUE);
	draw = depth ? framebuffer_get(0, texture) : framebuffer_get(texture, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
		depth ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT, GL_NEAREST);
	xgpu_gl_state_invalidate();
}

/* a target's renderbuffer with these samples a pixel (0: none, its storage
a pixel, the renderbuffer kept for the framebuffers made of it): its pixels
resolved into its texture first, and the texture's put into the new
storage */
static void texture_multisample(gpu_texture texture, int samples)
{
	struct texture_record *target = texture_record(texture);
	BOOL depth = target->description.format == GPU_FORMAT_DEPTH_STENCIL;
	GLint width = (GLint)target->description.width, height = (GLint)target->description.height;
	GLuint draw;

	if (target->samples == samples)
		return;
	texture_resolve(texture);
	if (!target->multisample)
		glGenRenderbuffers(1, &target->multisample);
	glBindRenderbuffer(GL_RENDERBUFFER, target->multisample);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, depth ? GL_DEPTH24_STENCIL8 : GL_RGBA8,
		samples ? width : 1, samples ? height : 1);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	target->samples = samples;
	if (!samples)
		return;
	draw = depth ? framebuffer_find(0, target->multisample, TRUE) : framebuffer_find(target->multisample, 0, TRUE);
#ifdef HALO_ANDROID
	/* (ES blits into no multisampled framebuffer: it is cleared instead. A
	change of the setting is taken up between frames, and the frame clears
	the screen's targets before it draws) */
	glBindFramebuffer(GL_FRAMEBUFFER, draw);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(0xff);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClearStencil(0);
	glClear(depth ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT);
#else
	{
		GLuint read = depth ? framebuffer_get(0, texture) : framebuffer_get(texture, 0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
		glDisable(GL_SCISSOR_TEST);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
			depth ? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
#endif
	xgpu_gl_state_invalidate();
}

void gpu_gl_resolve(gpu_texture target)
{
	texture_resolve(target);
}

void gpu_gl_multisample(gpu_texture target, uint32_t samples)
{
	texture_multisample(target, (int)samples);
}

void gpu_gl_bind_targets(gpu_texture color, gpu_texture depth, uint32_t samples)
{
	if (color)
		texture_multisample(color, (int)samples);
	if (depth)
		texture_multisample(depth, (int)samples);
	if (samples)
	{
		state_framebuffer(framebuffer_find(color ? texture_record(color)->multisample : 0,
			depth ? texture_record(depth)->multisample : 0, TRUE));
		if (color)
			texture_record(color)->unresolved = TRUE;
		if (depth)
			texture_record(depth)->unresolved = TRUE;
	}
	else
	{
		state_framebuffer(framebuffer_get(color, depth));
	}
}

/* ---------- sampler objects

A sampler object for each sampler state the game uses, made once: a draw
binds the one its state needs, rather than changing a sampler's parameters,
which costs a GL call each, and on Zink a new Vulkan sampler. The game uses
a few dozen states. */

#define SAMPLER_CACHE_SIZE 512

static struct
{
	struct gpu_sampler_state state;
	GLuint sampler;
} sampler_cache[SAMPLER_CACHE_SIZE];
static unsigned long sampler_cache_count;

/* each stage's own sampler, which takes the state when the cache is full */
static GLuint stage_samplers[GPU_STAGE_COUNT];

static GLenum address_mode(uint8_t mode)
{
	switch (mode)
	{
	case GPU_ADDRESS_MIRROR: return GL_MIRRORED_REPEAT;
	case GPU_ADDRESS_CLAMP: return GL_CLAMP_TO_EDGE;
#ifdef HALO_ANDROID
	case GPU_ADDRESS_BORDER: return xgpu_capabilities.border_clamp ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
#else
	case GPU_ADDRESS_BORDER: return GL_CLAMP_TO_BORDER;
#endif
	case GPU_ADDRESS_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
	default: return GL_REPEAT;
	}
}

/* a sampler's parameters from its state */
static void sampler_parameters(GLuint sampler, const struct gpu_sampler_state *state)
{
	GLenum minification;
	float border[4];

	if (state->min_filter == GPU_FILTER_POINT)
		minification = state->mip_filter == GPU_FILTER_NONE ? GL_NEAREST :
			state->mip_filter == GPU_FILTER_POINT ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
	else
		minification = state->mip_filter == GPU_FILTER_NONE ? GL_LINEAR :
			state->mip_filter == GPU_FILTER_POINT ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, (GLint)minification);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, state->mag_filter == GPU_FILTER_POINT ? GL_NEAREST : GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, (GLint)address_mode(state->address_u));
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, (GLint)address_mode(state->address_v));
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, (GLint)address_mode(state->address_w));
#ifdef HALO_ANDROID
	/* ES has no sampler LOD bias; the pixel shader applies it
	(texture_lod_bias) */
	glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, (float)state->max_mip_level);
	if (xgpu_capabilities.anisotropy)
		glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT,
			(state->min_filter == GPU_FILTER_ANISOTROPIC && state->max_anisotropy > 1) ? (float)state->max_anisotropy : 1.0f);
	if (xgpu_capabilities.border_clamp)
	{
		color_to_vec4(state->border_color, border);
		glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border);
	}
#else
	glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, state->lod_bias);
	glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, (float)state->max_mip_level);
	glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY,
		(state->min_filter == GPU_FILTER_ANISOTROPIC && state->max_anisotropy > 1) ? (float)state->max_anisotropy : 1.0f);
	color_to_vec4(state->border_color, border);
	glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border);
#endif
}

/* the sampler object of a sampler state, made the first time; with the
cache full (never seen), the stage's own sampler set to it */
static GLuint sampler_get(int stage, const struct gpu_sampler_state *state)
{
	const unsigned char *bytes = (const unsigned char *)state;
	unsigned long hash = 2166136261UL, index, probe;
	GLuint sampler;

	for (index = 0; index < sizeof(*state); index++)
		hash = (hash ^ bytes[index]) * 16777619UL;
	for (probe = 0; probe < SAMPLER_CACHE_SIZE; probe++)
	{
		index = (hash + probe) % SAMPLER_CACHE_SIZE;
		if (!sampler_cache[index].sampler)
			break;
		if (!memcmp(&sampler_cache[index].state, state, sizeof(*state)))
			return sampler_cache[index].sampler;
	}
	if (probe == SAMPLER_CACHE_SIZE || sampler_cache_count >= SAMPLER_CACHE_SIZE * 3 / 4)
	{
		sampler_parameters(stage_samplers[stage], state);
		return stage_samplers[stage];
	}
	glGenSamplers(1, &sampler);
	sampler_parameters(sampler, state);
	sampler_cache[index].state = *state;
	sampler_cache[index].sampler = sampler;
	sampler_cache_count++;
	return sampler;
}

static void configure_sampler(int stage, const struct gpu_sampler_state *state)
{
	/* the state each stage's sampler was last chosen by */
	static struct gpu_sampler_state configured[GPU_STAGE_COUNT];
	static GLuint configured_sampler[GPU_STAGE_COUNT];

	if (!configured_sampler[stage] || memcmp(&configured[stage], state, sizeof(*state)))
	{
		configured[stage] = *state;
		configured_sampler[stage] = sampler_get(stage, state);
	}
	state_sampler(stage, configured_sampler[stage]);
}

/* ---------- texture stages */

/* binds each stage's texture and sampler */
static void bind_stages(const struct gpu_stage stages[GPU_STAGE_COUNT])
{
	GLenum gl_targets[GPU_STAGE_COUNT];
	GLuint gl_textures[GPU_STAGE_COUNT];
	int stage;

	for (stage = 0; stage < GPU_STAGE_COUNT; stage++)
	{
		if (!stages[stage].type)
		{
			gl_targets[stage] = GL_TEXTURE_2D;
			gl_textures[stage] = 0;
			continue;
		}
		gl_targets[stage] = texture_target(stages[stage].type);
		gl_textures[stage] = stages[stage].texture;
		configure_sampler(stage, &stages[stage].sampler);
	}
#ifdef HALO_ANDROID
	for (stage = 0; stage < GPU_STAGE_COUNT; stage++)
		state_texture(stage, gl_targets[stage], gl_textures[stage]);
#else
	{
		/* the units whose texture changes, bound in one call (GL 4.4's
		multi-bind) rather than selecting and binding each unit; binding no
		texture unbinds all of the unit's targets */
		int first = -1, last = -1;

		for (stage = 0; stage < GPU_STAGE_COUNT; stage++)
		{
			if (gl_state.textures[stage][texture_slot(gl_targets[stage])] != gl_textures[stage])
			{
				if (first < 0)
					first = stage;
				last = stage;
			}
		}
		if (first >= 0)
		{
			glBindTextures((GLuint)first, (GLsizei)(last - first + 1), &gl_textures[first]);
			for (stage = first; stage <= last; stage++)
			{
				if (gl_textures[stage])
					gl_state.textures[stage][texture_slot(gl_targets[stage])] = gl_textures[stage];
				else
					memset(gl_state.textures[stage], 0, sizeof(gl_state.textures[stage]));
			}
		}
	}
#endif
}

/* ---------- GL helpers */

GLuint xgpu_compile_shader(GLenum type, const char *source, const char *what)
{
	GLuint shader = glCreateShader(type);
	GLint status = 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		platform_log("cannot compile the %s shader:\n%s\n%s", what, log, source);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint xgpu_link_program(GLuint vertex_shader, GLuint fragment_shader, const char *what)
{
	GLuint program = glCreateProgram();
	GLint status = 0;

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		platform_log("cannot link the %s program: %s", what, log);
		return 0;
	}
	return program;
}

gpu_shader gpu_shader_create(uint32_t stage, const char *source)
{
	return stage == GPU_SHADER_VERTEX ? xgpu_compile_shader(GL_VERTEX_SHADER, source, "vertex") :
		xgpu_compile_shader(GL_FRAGMENT_SHADER, source, "pixel");
}

/* ---------- programs */

struct program_entry
{
	struct program_entry *next;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLint constants;
	GLint viewport_scale;
	GLint viewport_offset;
	GLint point_size;
	GLint ps_c0, ps_c1, ps_final_c0, ps_final_c1;
	GLint fog_color, fog_parameters, alpha_reference;
	GLint bump_matrix, bump_luminance, texture_scale;
	GLint texture_lod_bias;
	GLint screen_offset;
	/* the lights of a draw lit for each pixel (GPU_MODEL_LIGHT_COUNT), and
	the constants' serial at their last upload */
	GLint model_lights;
	unsigned long long model_lights_serial;

	/* the vertex constants c[0..constant_count) the program uses; with
	consecutive locations, a changed range is uploaded by itself */
	unsigned long constant_count;
	BOOL constants_consecutive;
	/* the constants' serial at the program's last constant upload */
	unsigned long long constants_serial;
	/* the uniforms' serial when the uniforms below were brought up to date */
	unsigned long uniforms_serial;
	/* what the program's other uniforms hold (all ones: unknown) */
	struct gpu_uniforms uniforms;
};

#define PROGRAM_BUCKETS 1024

static struct program_entry *program_buckets[PROGRAM_BUCKETS];

static struct program_entry *program_get(GLuint vertex_shader, GLuint fragment_shader)
{
	static struct program_entry *last;
	unsigned long hash = (vertex_shader * 2654435761UL) ^ fragment_shader;
	struct program_entry **bucket = &program_buckets[hash % PROGRAM_BUCKETS];
	struct program_entry *entry;
	int stage;

	if (last && last->vertex_shader == vertex_shader && last->fragment_shader == fragment_shader)
		return last;
	for (entry = *bucket; entry; entry = entry->next)
	{
		if (entry->vertex_shader == vertex_shader && entry->fragment_shader == fragment_shader)
		{
			if (!entry->program)
				return NULL;
			last = entry;
			return entry;
		}
	}
	entry = calloc(1, sizeof(*entry));
	entry->vertex_shader = vertex_shader;
	entry->fragment_shader = fragment_shader;
	memset(&entry->uniforms, 0xff, sizeof(entry->uniforms));
	entry->next = *bucket;
	*bucket = entry;
	if (!vertex_shader || !fragment_shader)
		return NULL;

	entry->program = xgpu_link_program(vertex_shader, fragment_shader, "shader");
	if (!entry->program)
		return NULL;
	state_program(entry->program);
	entry->constants = glGetUniformLocation(entry->program, "c");
	entry->constant_count = GPU_CONSTANT_COUNT;
	if (entry->constants >= 0)
	{
		unsigned long index;

		/* c[i] is usually at c's location plus i, and the compiler may
		drop registers past the last one the program reads */
		entry->constants_consecutive = TRUE;
		for (index = 1; index < GPU_CONSTANT_COUNT; index++)
		{
			char name[16];
			GLint location;

			snprintf(name, sizeof(name), "c[%lu]", index);
			location = glGetUniformLocation(entry->program, name);
			if (location < 0)
			{
				entry->constant_count = index;
				break;
			}
			if (location != entry->constants + (GLint)index)
			{
				entry->constants_consecutive = FALSE;
				entry->constant_count = GPU_CONSTANT_COUNT;
				break;
			}
		}
	}
	entry->viewport_scale = glGetUniformLocation(entry->program, "viewport_scale");
	entry->viewport_offset = glGetUniformLocation(entry->program, "viewport_offset");
	entry->point_size = glGetUniformLocation(entry->program, "point_size");
	entry->ps_c0 = glGetUniformLocation(entry->program, "ps_c0");
	entry->ps_c1 = glGetUniformLocation(entry->program, "ps_c1");
	entry->ps_final_c0 = glGetUniformLocation(entry->program, "ps_final_c0");
	entry->ps_final_c1 = glGetUniformLocation(entry->program, "ps_final_c1");
	entry->fog_color = glGetUniformLocation(entry->program, "fog_color");
	entry->fog_parameters = glGetUniformLocation(entry->program, "fog_parameters");
	entry->alpha_reference = glGetUniformLocation(entry->program, "alpha_reference");
	entry->bump_matrix = glGetUniformLocation(entry->program, "bump_matrix");
	entry->bump_luminance = glGetUniformLocation(entry->program, "bump_luminance");
	entry->texture_scale = glGetUniformLocation(entry->program, "texture_scale");
	entry->texture_lod_bias = glGetUniformLocation(entry->program, "texture_lod_bias");
	entry->screen_offset = glGetUniformLocation(entry->program, "screen_offset");
	entry->model_lights = glGetUniformLocation(entry->program, "model_lights");
	for (stage = 0; stage < GPU_STAGE_COUNT; stage++)
	{
		char name[8];

		snprintf(name, sizeof(name), "tex%d", stage);
		glUniform1i(glGetUniformLocation(entry->program, name), stage);
	}
	last = entry;
	return entry;
}

/* sets a program's uniform unless it already holds value */
static void uniform_vec4(GLint location, float *shadow, const float *value, int count)
{
	if (location < 0 || !memcmp(shadow, value, (size_t)count * 4 * sizeof(float)))
		return;
	memcpy(shadow, value, (size_t)count * 4 * sizeof(float));
	glUniform4fv(location, count, value);
}

static void uniform_float(GLint location, float *shadow, float value)
{
	if (location < 0 || !memcmp(shadow, &value, sizeof(value)))
		return;
	*shadow = value;
	glUniform1f(location, value);
}

/* ---------- vertex and index data */

#ifdef HALO_ANDROID
/* Mobile drivers (Mali) keep every orphaned copy of a buffer until the GPU
is done with it, so a large buffer orphaned each frame costs its size per
frame in flight and more. Instead each frame streams into the next of a few
smaller buffers, reusing one only once the GPU has finished the frame that
last used it (host_gl_wait_frame). A busy frame streams about 5 MB of
vertices. */
#define STREAM_BUFFER_SIZE (16 * 1024 * 1024)
#define INDEX_BUFFER_SIZE (2 * 1024 * 1024)
#else
#define STREAM_BUFFER_SIZE (32 * 1024 * 1024)
#define INDEX_BUFFER_SIZE (8 * 1024 * 1024)
#endif

static struct
{
	GLuint stream_buffer;
#ifdef HALO_ANDROID
	GLuint stream_buffers[GPU_GL_FRAME_RING];
	GLuint index_buffers[GPU_GL_FRAME_RING];
	unsigned long buffer_ring;
#endif
	unsigned long stream_offset;
	GLuint index_buffer;
	unsigned long index_offset;
#ifndef HALO_ANDROID
	/* a pipeline flush every flush_every draws (draw_flush), 0 never */
	unsigned long flush_every;
	unsigned long flush_draws;
#endif
	/* frames presented (gl_check_errors) */
	unsigned long frame;
} streams;

/* the vertex array the ES attributes are on, and the desktop's before its
first draw */
GLuint gpu_gl_default_vertex_array;

/* Mesa's GL thread queues a glBufferSubData of up to 8 KB; a larger one
first waits for everything queued before it to have run. The same bytes in
pieces are queued (all but the largest, as a map loads, which would be
thousands; Android's GL has no such thread). */
#define BUFFER_UPLOAD_PIECE 4096
#define BUFFER_UPLOAD_PIECES_MAXIMUM 16

static void buffer_upload(GLenum target, unsigned long offset, unsigned long size, const void *data)
{
#ifndef HALO_ANDROID
	if (size <= BUFFER_UPLOAD_PIECE * BUFFER_UPLOAD_PIECES_MAXIMUM)
	{
		const unsigned char *bytes = data;

		while (size)
		{
			unsigned long piece = size < BUFFER_UPLOAD_PIECE ? size : BUFFER_UPLOAD_PIECE;

			glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)piece, bytes);
			offset += piece;
			bytes += piece;
			size -= piece;
		}
		return;
	}
#endif
	glBufferSubData(target, (GLintptr)offset, (GLsizeiptr)size, data);
}

gpu_buffer gpu_buffer_create(uint32_t size)
{
	GLuint buffer = 0;

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
	glBufferData(GL_COPY_WRITE_BUFFER, (GLsizeiptr)size, NULL, GL_DYNAMIC_DRAW);
	return buffer;
}

void gpu_buffer_write(gpu_buffer buffer, uint32_t offset, uint32_t size, const void *data, uint32_t flags)
{
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
#ifdef HALO_ANDROID
	/* Mali copies the whole buffer for a glBufferSubData that queued draws
	might read (see GPU_GL_FRAME_RING); unused ranges can be written without
	waiting for them */
	if (flags & GPU_WRITE_UNUSED)
	{
		host_gl_buffer_write(GL_COPY_WRITE_BUFFER, offset, size, data);
		return;
	}
#else
	(void)flags;
#endif
	buffer_upload(GL_COPY_WRITE_BUFFER, offset, size, data);
}

/* makes room for size bytes of uploads, orphaning the stream buffer if it
is full. A draw reserves room for all of its streams at once: orphaning
between two of them would leave the attributes already pointed at the
buffer reading its new, empty storage. */
void gpu_stream_reserve(uint32_t size)
{
	if (streams.stream_offset + size > STREAM_BUFFER_SIZE)
	{
		/* orphan the buffer and start again */
		state_array_buffer(streams.stream_buffer);
		glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		streams.stream_offset = 0;
	}
}

uint32_t gpu_stream(uint32_t kind, const void *data, uint32_t size, gpu_buffer *buffer)
{
	unsigned long offset;

	size = (size + 15) & ~15U;
	if (kind == GPU_STREAM_INDEX)
	{
		state_element_array_buffer(streams.index_buffer);
		if (streams.index_offset + size > INDEX_BUFFER_SIZE)
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
			streams.index_offset = 0;
		}
		offset = streams.index_offset;
#ifdef HALO_ANDROID
		host_gl_buffer_write(GL_ELEMENT_ARRAY_BUFFER, (unsigned int)offset, (unsigned int)size, data);
#else
		buffer_upload(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
#endif
		streams.index_offset += size;
		*buffer = streams.index_buffer;
		return (uint32_t)offset;
	}
	gpu_stream_reserve(size);
	offset = streams.stream_offset;
	state_array_buffer(streams.stream_buffer);
#ifdef HALO_ANDROID
	host_gl_buffer_write(GL_ARRAY_BUFFER, (unsigned int)offset, (unsigned int)size, data);
#else
	buffer_upload(GL_ARRAY_BUFFER, offset, size, data);
#endif
	streams.stream_offset += size;
	*buffer = streams.stream_buffer;
	return (uint32_t)offset;
}

unsigned long gpu_gl_frame_ring(void)
{
#ifdef HALO_ANDROID
	return streams.buffer_ring;
#else
	return 0;
#endif
}

void gpu_gl_frame_advance(void)
{
	streams.frame++;
#ifdef HALO_ANDROID
	host_gl_fence_frame((unsigned int)streams.buffer_ring);
	streams.buffer_ring = (streams.buffer_ring + 1) % GPU_GL_FRAME_RING;
	host_gl_wait_frame((unsigned int)streams.buffer_ring);
	streams.stream_buffer = streams.stream_buffers[streams.buffer_ring];
	streams.index_buffer = streams.index_buffers[streams.buffer_ring];
	streams.stream_offset = 0;
	streams.index_offset = 0;
#else
	streams.stream_offset = STREAM_BUFFER_SIZE; /* orphan next frame */
	streams.index_offset = INDEX_BUFFER_SIZE;
#endif
}

/* ---------- draws */

#ifdef HALO_ANDROID
/* ES has no debug callback in 3.0; debug.gl_debug polls glGetError around
each draw instead, reporting each distinct error a few times */
static void gl_check_errors(const char *where)
{
	static int enabled = -1;
	static unsigned long reports;
	GLenum error;

	if (enabled < 0)
		enabled = config_boolean("debug.gl_debug");
	if (!enabled)
		return;
	while ((error = glGetError()) != GL_NO_ERROR)
	{
		if (reports++ < 200)
			platform_log("GL error %04x at %s (frame %lu)", (unsigned)error, where, streams.frame);
	}
}
#else
#define gl_check_errors(where) ((void)0)
#endif

/* Intel's graphics with Mesa's driver can hang the GPU in a long run of
draws with no pipeline flush between them, which the game's effects make
(hundreds of small draws in a row): the command streamer stops at a draw,
and the reset that follows takes the desktop's other programs with it.
Intel's workaround for a hang of this kind on their DG2 graphics
(Wa_16014538804) is a flush at least every 3 draws, which Mesa does not
apply to the others. A memory barrier is one (and only that: nothing
here writes images). */
static void draw_flush(void)
{
#ifndef HALO_ANDROID
	if (streams.flush_every && ++streams.flush_draws >= streams.flush_every)
	{
		streams.flush_draws = 0;
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
#endif
}

static const GLenum gl_compare_functions[] =
{
	GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS,
};

static const GLenum gl_stencil_operations[] =
{
	GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_DECR, GL_INVERT, GL_INCR_WRAP, GL_DECR_WRAP,
};

static const GLenum gl_blend_factors[] =
{
	GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA_SATURATE,
	GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA,
};

static const GLenum gl_blend_equations[] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX };

#define TABLE_ENTRY(table, index) ((table)[(index) < sizeof(table) / sizeof((table)[0]) ? (index) : 0])

static void apply_raster_state(const struct gpu_draw *draw)
{
	const struct gpu_depth_stencil_state *depth_stencil = &draw->depth_stencil;
	const struct gpu_blend_state *blend = &draw->blend;
	const struct gpu_raster_state *raster = &draw->raster;
	GLint viewport[4];
	GLint scissor[4];
	float depth_range[2];

	viewport[0] = draw->viewport.rect.x;
	viewport[1] = draw->viewport.rect.y;
	viewport[2] = draw->viewport.rect.width;
	viewport[3] = draw->viewport.rect.height;
	if (memcmp(gl_state.viewport, viewport, sizeof(viewport)))
	{
		memcpy(gl_state.viewport, viewport, sizeof(viewport));
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}
	scissor[0] = draw->scissor.x;
	scissor[1] = draw->scissor.y;
	scissor[2] = draw->scissor.width;
	scissor[3] = draw->scissor.height;
	if (memcmp(gl_state.scissor, scissor, sizeof(scissor)))
	{
		memcpy(gl_state.scissor, scissor, sizeof(scissor));
		glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
	}
	state_enable(&gl_state.scissor_test, GL_SCISSOR_TEST, scissor[2] > 0 && scissor[3] > 0);
	depth_range[0] = draw->viewport.min_z;
	depth_range[1] = draw->viewport.max_z;
	if (memcmp(gl_state.depth_range, depth_range, sizeof(depth_range)))
	{
		memcpy(gl_state.depth_range, depth_range, sizeof(depth_range));
		glDepthRange(depth_range[0], depth_range[1]);
	}

	state_enable(&gl_state.depth_test, GL_DEPTH_TEST, depth_stencil->depth_test);
	if (depth_stencil->depth_test)
	{
		GLenum function = TABLE_ENTRY(gl_compare_functions, depth_stencil->depth_function);

		if (gl_state.depth_function != function)
		{
			gl_state.depth_function = function;
			glDepthFunc(function);
		}
	}
	{
		unsigned char mask = depth_stencil->depth_test && depth_stencil->depth_write ? 1 : 0;

		if (gl_state.depth_mask != mask)
		{
			gl_state.depth_mask = mask;
			glDepthMask(mask ? GL_TRUE : GL_FALSE);
		}
	}

	state_enable(&gl_state.stencil_test, GL_STENCIL_TEST, depth_stencil->stencil_test);
	if (depth_stencil->stencil_test)
	{
		GLenum function = TABLE_ENTRY(gl_compare_functions, depth_stencil->stencil_function);
		GLenum operations[3];

		if (gl_state.stencil_function != function || gl_state.stencil_reference != (GLint)depth_stencil->stencil_reference ||
			gl_state.stencil_value_mask != depth_stencil->stencil_read_mask)
		{
			gl_state.stencil_function = function;
			gl_state.stencil_reference = (GLint)depth_stencil->stencil_reference;
			gl_state.stencil_value_mask = depth_stencil->stencil_read_mask;
			glStencilFunc(function, (GLint)depth_stencil->stencil_reference, depth_stencil->stencil_read_mask);
		}
		operations[0] = TABLE_ENTRY(gl_stencil_operations, depth_stencil->stencil_fail);
		operations[1] = TABLE_ENTRY(gl_stencil_operations, depth_stencil->stencil_depth_fail);
		operations[2] = TABLE_ENTRY(gl_stencil_operations, depth_stencil->stencil_pass);
		if (memcmp(gl_state.stencil_operations, operations, sizeof(operations)))
		{
			memcpy(gl_state.stencil_operations, operations, sizeof(operations));
			glStencilOp(operations[0], operations[1], operations[2]);
		}
		if (gl_state.stencil_write_mask != depth_stencil->stencil_write_mask)
		{
			gl_state.stencil_write_mask = depth_stencil->stencil_write_mask;
			glStencilMask(depth_stencil->stencil_write_mask);
		}
	}

	state_enable(&gl_state.blend, GL_BLEND, blend->enable);
	if (blend->enable)
	{
		GLenum source = TABLE_ENTRY(gl_blend_factors, blend->source);
		GLenum destination = TABLE_ENTRY(gl_blend_factors, blend->destination);
		GLenum equation = TABLE_ENTRY(gl_blend_equations, blend->operation);
		float blend_color[4];

		if (gl_state.blend_source != source || gl_state.blend_destination != destination)
		{
			gl_state.blend_source = source;
			gl_state.blend_destination = destination;
			glBlendFunc(source, destination);
		}
		if (gl_state.blend_equation != equation)
		{
			gl_state.blend_equation = equation;
			glBlendEquation(equation);
		}
		color_to_vec4(blend->color, blend_color);
		if (memcmp(gl_state.blend_color, blend_color, sizeof(blend_color)))
		{
			memcpy(gl_state.blend_color, blend_color, sizeof(blend_color));
			glBlendColor(blend_color[0], blend_color[1], blend_color[2], blend_color[3]);
		}
	}
	if (gl_state.color_mask != blend->color_write_mask)
	{
		unsigned char color_mask = blend->color_write_mask;

		gl_state.color_mask = color_mask;
		glColorMask((color_mask & 1) != 0, (color_mask & 2) != 0, (color_mask & 4) != 0, (color_mask & 8) != 0);
	}

	state_enable(&gl_state.cull_face, GL_CULL_FACE, raster->cull_mode != GPU_CULL_NONE);
	if (raster->cull_mode != GPU_CULL_NONE)
	{
#ifdef HALO_ANDROID
		/* the vertex shader flips y in clip space, which (unlike desktop
		GL's upper-left clip origin) also flips the winding */
		GLenum front_face = raster->front_face == GPU_FRONT_COUNTER_CLOCKWISE ? GL_CW : GL_CCW;
#else
		GLenum front_face = raster->front_face == GPU_FRONT_COUNTER_CLOCKWISE ? GL_CCW : GL_CW;
#endif
		GLenum cull_mode = raster->cull_mode == GPU_CULL_FRONT ? GL_FRONT : GL_BACK;

		if (gl_state.front_face != front_face)
		{
			gl_state.front_face = front_face;
			glFrontFace(front_face);
		}
		if (gl_state.cull_mode != cull_mode)
		{
			gl_state.cull_mode = cull_mode;
			glCullFace(cull_mode);
		}
	}
#ifndef HALO_ANDROID
	/* ES draws filled polygons only (wireframe is a debug mode) */
	{
		GLenum polygon_mode = raster->fill_mode == GPU_FILL_LINE ? GL_LINE :
			raster->fill_mode == GPU_FILL_POINT ? GL_POINT : GL_FILL;

		if (gl_state.polygon_mode != polygon_mode)
		{
			gl_state.polygon_mode = polygon_mode;
			glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
		}
	}
#endif

	state_enable(&gl_state.offset_fill, GL_POLYGON_OFFSET_FILL, raster->depth_bias);
#ifndef HALO_ANDROID
	state_enable(&gl_state.offset_line, GL_POLYGON_OFFSET_LINE, raster->depth_bias);
#endif
	if (raster->depth_bias)
	{
		float offset[2];

		offset[0] = raster->depth_bias_slope;
		offset[1] = raster->depth_bias_constant;
		if (memcmp(gl_state.polygon_offset, offset, sizeof(offset)))
		{
			memcpy(gl_state.polygon_offset, offset, sizeof(offset));
			glPolygonOffset(offset[0], offset[1]);
		}
	}
}

/* the vertex constant register of each of the per-pixel lighting's lights */
static unsigned long model_light_register(int light)
{
	return (unsigned long)(96 + (light ? -80 + light : -82));
}

/* the constants and the other uniforms a program has not had yet */
static void upload_uniforms(struct program_entry *entry, struct gpu_constant_store *store,
	const struct gpu_uniforms *uniforms)
{
	if (entry->constants >= 0 && entry->constants_serial != store->serial)
	{
		unsigned long first = entry->constant_count, last = 0, index;

		if (entry->constants_serial == store->checkpoint_serial &&
			store->checkpoint_last < entry->constant_count)
		{
			if (store->checkpoint_first <= store->checkpoint_last)
			{
				first = store->checkpoint_first;
				last = store->checkpoint_last;
			}
		}
		else if (store->serial - entry->constants_serial <= GPU_CONSTANT_COUNT)
		{
			unsigned long long serial;

			for (serial = entry->constants_serial + 1; serial <= store->serial; serial++)
			{
				index = store->log[serial % GPU_CONSTANT_LOG_SIZE];
				if (index >= entry->constant_count)
					continue;
				if (first > index)
					first = index;
				if (last < index)
					last = index;
			}
		}
		else
		{
			for (index = 0; index < entry->constant_count; index++)
			{
				if (store->serials[index] > entry->constants_serial)
				{
					if (first > index)
						first = index;
					last = index;
				}
			}
		}
		if (first < entry->constant_count)
		{
			if (entry->constants_consecutive)
				glUniform4fv(entry->constants + (GLint)first, (GLsizei)(last - first + 1), store->c[first]);
			else
				glUniform4fv(entry->constants, GPU_CONSTANT_COUNT, &store->c[0][0]);
		}
		entry->constants_serial = store->serial;
		store->checkpoint_serial = store->serial;
		store->checkpoint_first = GPU_CONSTANT_COUNT;
		store->checkpoint_last = 0;
	}
	/* the lights of a draw lit for each pixel, from the same registers, when
	any of them changed since the program last had them */
	if (entry->model_lights >= 0 && entry->model_lights_serial != store->serial)
	{
		int light;

		for (light = 0; light < GPU_MODEL_LIGHT_COUNT; light++)
		{
			if (store->serials[model_light_register(light)] > entry->model_lights_serial)
				break;
		}
		if (light < GPU_MODEL_LIGHT_COUNT)
		{
			float lights[GPU_MODEL_LIGHT_COUNT][4];

			for (light = 0; light < GPU_MODEL_LIGHT_COUNT; light++)
				memcpy(lights[light], store->c[model_light_register(light)], sizeof(lights[0]));
			glUniform4fv(entry->model_lights, GPU_MODEL_LIGHT_COUNT, lights[0]);
		}
		entry->model_lights_serial = store->serial;
	}
	/* a program that has had the other uniforms since they last changed
	needs none of them */
	if (entry->uniforms_serial == uniforms->serial)
		return;
	entry->uniforms_serial = uniforms->serial;
	uniform_vec4(entry->viewport_scale, entry->uniforms.viewport_scale, uniforms->viewport_scale, 1);
	uniform_vec4(entry->viewport_offset, entry->uniforms.viewport_offset, uniforms->viewport_offset, 1);
	uniform_float(entry->point_size, &entry->uniforms.point_size, uniforms->point_size);
	uniform_vec4(entry->ps_c0, entry->uniforms.ps_c0[0], uniforms->ps_c0[0], 8);
	uniform_vec4(entry->ps_c1, entry->uniforms.ps_c1[0], uniforms->ps_c1[0], 8);
	uniform_vec4(entry->ps_final_c0, entry->uniforms.ps_final_c0, uniforms->ps_final_c0, 1);
	uniform_vec4(entry->ps_final_c1, entry->uniforms.ps_final_c1, uniforms->ps_final_c1, 1);
	uniform_vec4(entry->fog_color, entry->uniforms.fog_color, uniforms->fog_color, 1);
	uniform_vec4(entry->fog_parameters, entry->uniforms.fog_parameters, uniforms->fog_parameters, 1);
	uniform_float(entry->alpha_reference, &entry->uniforms.alpha_reference, uniforms->alpha_reference);
	uniform_vec4(entry->bump_matrix, entry->uniforms.bump_matrix[0], uniforms->bump_matrix[0], 4);
	uniform_vec4(entry->bump_luminance, entry->uniforms.bump_luminance[0], uniforms->bump_luminance[0], 4);
	uniform_vec4(entry->texture_scale, entry->uniforms.texture_scale[0], uniforms->texture_scale[0], 4);
	uniform_float(entry->screen_offset, &entry->uniforms.screen_offset, uniforms->screen_offset);
	uniform_vec4(entry->texture_lod_bias, entry->uniforms.texture_lod_bias, uniforms->texture_lod_bias, 1);
}

static void attribute_format(unsigned char format, GLint *size, GLenum *type, GLboolean *normalized)
{
	*normalized = GL_FALSE;
	switch (format)
	{
	case GPU_ATTRIBUTE_FLOAT1: *size = 1; *type = GL_FLOAT; break;
	case GPU_ATTRIBUTE_FLOAT2: *size = 2; *type = GL_FLOAT; break;
	case GPU_ATTRIBUTE_FLOAT3: *size = 3; *type = GL_FLOAT; break;
	case GPU_ATTRIBUTE_BGRA8: *size = GL_BGRA; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_RGBA8: *size = 4; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_SHORT1: *size = 1; *type = GL_SHORT; break;
	case GPU_ATTRIBUTE_SHORT2: *size = 2; *type = GL_SHORT; break;
	case GPU_ATTRIBUTE_SHORT3: *size = 3; *type = GL_SHORT; break;
	case GPU_ATTRIBUTE_SHORT4: *size = 4; *type = GL_SHORT; break;
	case GPU_ATTRIBUTE_NORMSHORT1: *size = 1; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_NORMSHORT2: *size = 2; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_NORMSHORT3: *size = 3; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_NORMSHORT4: *size = 4; *type = GL_SHORT; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_UBYTE1: *size = 1; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_UBYTE2: *size = 2; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_UBYTE3: *size = 3; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	case GPU_ATTRIBUTE_UBYTE4: *size = 4; *type = GL_UNSIGNED_BYTE; *normalized = GL_TRUE; break;
	default: *size = 4; *type = GL_FLOAT; break;
	}
}

/* points the attributes at the draw's streams, and gives the others their
values */
static void setup_streams(const struct gpu_draw *draw)
{
	unsigned long index;
#ifndef HALO_ANDROID
	/* the vertex array of the draw before, and the layout it was for: the
	same declaration draws again and again */
	static struct vertex_array_entry *last_array;
	static struct vertex_layout last_layout;
	struct vertex_layout layout;
	unsigned long streams_used = 0;

	/* (zeroed: layouts are compared and hashed whole) */
	memset(&layout, 0, sizeof(layout));
#endif
	for (index = 0; index < GPU_ATTRIBUTE_COUNT; index++)
	{
		const struct gpu_vertex_attribute *attribute = &draw->attributes[index];
		GLint size;
		GLenum type;
		GLboolean normalized;

		if (attribute->stream >= GPU_STREAM_COUNT)
			continue;
#ifdef HALO_ANDROID
		{
			const struct gpu_vertex_stream *stream = &draw->streams[attribute->stream];

			if (attribute->format == GPU_ATTRIBUTE_NORMPACKED3)
			{
				state_attribute_stream((GLuint)index, attribute->stream, stream->buffer, 1, GL_UNSIGNED_INT, GL_FALSE,
					TRUE, (GLsizei)stream->stride, stream->offset, attribute->offset);
			}
			else
			{
				attribute_format(attribute->format, &size, &type, &normalized);
				state_attribute_stream((GLuint)index, attribute->stream, stream->buffer, size, type, normalized,
					FALSE, (GLsizei)stream->stride, stream->offset, attribute->offset);
			}
		}
#else
		if (attribute->format == GPU_ATTRIBUTE_NORMPACKED3)
		{
			layout_attribute(&layout, index, attribute->stream, 1, GL_UNSIGNED_INT, GL_FALSE, TRUE, attribute->offset);
		}
		else
		{
			attribute_format(attribute->format, &size, &type, &normalized);
			layout_attribute(&layout, index, attribute->stream, size, type, normalized, FALSE, attribute->offset);
		}
		streams_used |= 1UL << attribute->stream;
#endif
	}
#ifndef HALO_ANDROID
	/* (the layout follows from the declaration and the streams it has, so
	a draw mostly has the layout of the draw before it: no need to look it
	up) */
	if (!last_array || memcmp(&last_layout, &layout, sizeof(layout)))
	{
		last_array = vertex_array_get(&layout);
		last_layout = layout;
	}
	state_vertex_array(last_array);
	for (index = 0; index < GPU_STREAM_COUNT; index++)
	{
		if (streams_used & (1UL << index))
			state_vertex_buffer((GLuint)index, draw->streams[index].buffer, draw->streams[index].offset,
				(GLsizei)draw->streams[index].stride);
	}
#endif
	for (index = 0; index < GPU_ATTRIBUTE_COUNT; index++)
	{
		const struct gpu_vertex_attribute *attribute = &draw->attributes[index];

		if (attribute->stream == GPU_STREAM_CONSTANT)
			state_attribute_value((GLuint)index, draw->constant_values[index]);
		else if (attribute->stream >= GPU_STREAM_COUNT)
			state_attribute_value((GLuint)index, NULL);
	}
}

static const GLenum gl_primitives[] =
{
	GL_POINTS, GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN,
};

uint32_t gpu_draw(const struct gpu_draw *draw, struct gpu_constant_store *constants,
	const struct gpu_uniforms *uniforms)
{
	struct program_entry *entry;
	GLenum mode = TABLE_ENTRY(gl_primitives, draw->primitive);
	int stage;

	/* the textures before the targets: a render target the draw samples
	has its multisampled pixels resolved by a blit, which binds framebuffers
	of its own, and the back buffer can be both sampled and drawn into */
	for (stage = 0; stage < GPU_STAGE_COUNT; stage++)
	{
		if (draw->stages[stage].type)
			texture_resolve(draw->stages[stage].texture);
	}
	gpu_gl_bind_targets(draw->color_target, draw->depth_target, draw->samples);
	apply_raster_state(draw);
	entry = program_get(draw->vertex_shader, draw->pixel_shader);
	if (!entry)
	{
		gl_check_errors("program");
		return 0;
	}
	gl_check_errors("state");
	draw_flush();
	state_program(entry->program);
	upload_uniforms(entry, constants, uniforms);
	bind_stages(draw->stages);
	setup_streams(draw);
	if (!draw->index_buffer)
	{
		glDrawArrays(mode, 0, (GLsizei)draw->count);
	}
	else
	{
		state_element_array_buffer(draw->index_buffer);
		if (draw->base_vertex)
			glDrawElementsBaseVertex(mode, (GLsizei)draw->count, GL_UNSIGNED_SHORT,
				(const void *)(uintptr_t)draw->index_offset, draw->base_vertex);
		else
			glDrawElements(mode, (GLsizei)draw->count, GL_UNSIGNED_SHORT, (const void *)(uintptr_t)draw->index_offset);
	}
	gl_check_errors("draw");
	return 1;
}

/* ---------- the backend */

#ifndef HALO_ANDROID
static void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar *message, const void *user)
{
	(void)source; (void)id; (void)length; (void)user;
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		platform_log("GL %s: %s", type == GL_DEBUG_TYPE_ERROR ? "error" : "debug", message);
}
#endif

void gpu_initialize(struct gpu_capabilities *capabilities)
{
	GLint major = 0, minor = 0;

	memset(capabilities, 0, sizeof(*capabilities));
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
#ifdef HALO_ANDROID
	{
		BOOL es32 = major > 3 || (major == 3 && minor >= 2);
		BOOL es31 = major > 3 || (major == 3 && minor >= 1);

		/* clip control is emulated in the vertex shader (nv2a_vsh.c) */
		xgpu_capabilities.copy_image = es32 || host_gl_has_extension("GL_EXT_copy_image") ||
			host_gl_has_extension("GL_OES_copy_image");
		xgpu_capabilities.border_clamp = es32 || host_gl_has_extension("GL_EXT_texture_border_clamp") ||
			host_gl_has_extension("GL_OES_texture_border_clamp");
		xgpu_capabilities.anisotropy = host_gl_has_extension("GL_EXT_texture_filter_anisotropic");
		xgpu_capabilities.base_vertex = es32;
		xgpu_capabilities.shading_language = es31 ? "310 es" : "300 es";
		if (es31)
		{
			GLint counters = 0;

			glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &counters);
			xgpu_capabilities.atomic_counters = counters > 0;
		}
		xgpu_capabilities.s3tc = host_gl_has_extension("GL_EXT_texture_compression_s3tc") ||
			(host_gl_has_extension("GL_EXT_texture_compression_dxt1") &&
			host_gl_has_extension("GL_ANGLE_texture_compression_dxt3") &&
			host_gl_has_extension("GL_ANGLE_texture_compression_dxt5"));
		platform_log("OpenGL ES %d.%d: copy image %d, border clamp %d, anisotropy %d, S3TC %d, sample counting %d",
			(int)major, (int)minor, xgpu_capabilities.copy_image, xgpu_capabilities.border_clamp,
			xgpu_capabilities.anisotropy, xgpu_capabilities.s3tc, xgpu_capabilities.atomic_counters);
		capabilities->shading_language = es31 ? 310 : 300;
		capabilities->shading_language_es = 1;
		capabilities->sampler_lod_bias = 0;
		capabilities->shader_clip_control = 1;
		capabilities->s3tc = xgpu_capabilities.s3tc != FALSE;
		/* (ES has no BGRA attributes) */
		capabilities->vertex_bgra = 0;
		capabilities->base_vertex = xgpu_capabilities.base_vertex != FALSE;
		capabilities->occlusion = xgpu_capabilities.atomic_counters ? GPU_OCCLUSION_SHADER_COUNTER :
			GPU_OCCLUSION_ANY_SAMPLE;
		/* (gl_SampleMask: ES has it only from 3.2) */
		capabilities->sample_mask = 0;
	}
#else
	if (config_boolean("debug.gl_debug"))
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(gl_debug_callback, NULL);
	}
	glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
	glEnable(GL_PROGRAM_POINT_SIZE);
	capabilities->shading_language = 450;
	capabilities->sampler_lod_bias = 1;
	capabilities->s3tc = 1;
	capabilities->vertex_bgra = 1;
	capabilities->base_vertex = 1;
	capabilities->occlusion = GPU_OCCLUSION_EXACT;
	capabilities->sample_mask = 1;
#endif
	glGenVertexArrays(1, &gpu_gl_default_vertex_array);
	glBindVertexArray(gpu_gl_default_vertex_array);
#ifdef HALO_ANDROID
	{
		int ring;

		glGenBuffers(GPU_GL_FRAME_RING, streams.stream_buffers);
		glGenBuffers(GPU_GL_FRAME_RING, streams.index_buffers);
		for (ring = 0; ring < GPU_GL_FRAME_RING; ring++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, streams.stream_buffers[ring]);
			glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, streams.index_buffers[ring]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
		}
		streams.stream_buffer = streams.stream_buffers[0];
		streams.index_buffer = streams.index_buffers[0];
	}
#else
	glGenBuffers(1, &streams.stream_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, streams.stream_buffer);
	glBufferData(GL_ARRAY_BUFFER, STREAM_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
	glGenBuffers(1, &streams.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, streams.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_BUFFER_SIZE, NULL, GL_STREAM_DRAW);
	{
		long every = config_integer("debug.gpu_flush_draws");
		const char *renderer = (const char *)glGetString(GL_RENDERER);

		if (every < 0)
			every = renderer && strstr(renderer, "Mesa Intel") ? 3 : 0;
		if (every > 0)
		{
			streams.flush_every = (unsigned long)every;
			platform_log("GPU: a pipeline flush every %ld draws", every);
		}
	}
#endif
	glGenSamplers(GPU_STAGE_COUNT, stage_samplers);
	{
		/* an attribute a draw does not stream reads (0, 0, 0, 1) until
		given a value, as the Xbox's registers start */
		static const float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		GLuint index;

		for (index = 0; index < GPU_ATTRIBUTE_COUNT; index++)
			glVertexAttrib4fv(index, origin);
	}
	{
		GLint samples = 0, texture_size = 0, renderbuffer_size = 0;

		glGetIntegerv(GL_MAX_SAMPLES, &samples);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size);
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &renderbuffer_size);
		capabilities->max_samples = (uint32_t)samples;
		capabilities->max_target_size = (uint32_t)(renderbuffer_size < texture_size ? renderbuffer_size : texture_size);
	}
	xgpu_gl_state_invalidate();
}
