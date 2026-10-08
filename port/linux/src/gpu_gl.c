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

/* ---------- GL state cache (gpu_gl.h) */

#ifndef HALO_ANDROID
#define VERTEX_ARRAY_BUCKET_COUNT 64
static struct vertex_array_entry *vertex_array_buckets[VERTEX_ARRAY_BUCKET_COUNT];
static struct vertex_array_entry *current_vertex_array;
#endif

struct gl_state gl_state;

void xgpu_gl_state_invalidate(void)
{
	memset(&gl_state, 0xff, sizeof(gl_state));
}

void state_enable(unsigned char *shadow, GLenum capability, BOOL enabled)
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

void state_program(GLuint program)
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

void state_array_buffer(GLuint buffer)
{
	if (gl_state.array_buffer != buffer)
	{
		gl_state.array_buffer = buffer;
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
	}
}

void state_element_array_buffer(GLuint buffer)
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
void state_attribute_stream(GLuint index, GLuint binding, GLuint buffer, GLint size, GLenum type,
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
struct vertex_array_entry *vertex_array_get(const struct vertex_layout *layout)
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

void state_vertex_array(struct vertex_array_entry *entry)
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
void state_vertex_buffer(GLuint binding, GLuint buffer, unsigned long offset, GLsizei stride)
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
void layout_attribute(struct vertex_layout *layout, unsigned long index, GLuint binding, GLint size,
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
void state_attribute_value(GLuint index, const float *value)
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

void gpu_gl_bind_stages(const struct gpu_stage stages[GPU_STAGE_COUNT])
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
	glGenSamplers(GPU_STAGE_COUNT, stage_samplers);
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
