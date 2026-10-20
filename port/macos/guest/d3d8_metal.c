/*
D3D8_METAL.C

The Xbox Direct3D 8 device on Metal, for the macOS app (port/linux/src/xgpu.h,
"Metal"): what d3d8_gl.c does with GL calls is written as the host's render
commands (port/macos/include/opence_render.h), which the app's renderer
draws with Metal (MetalRenderer.swift).

This part holds the device's resources: the ring and the ids of what is
made on it, the staging memory uploads are copied into, the contiguous
window's mirror in Metal buffers, the stream buffer that dynamic vertices go
through, and the render targets of the game's surfaces. Textures are
xbox_textures.c's, which writes them here under HALO_METAL.
*/

#include "xgpu.h"
#include "opence_render.h"
#include "opence_render_writer.h"

#include <stdlib.h>
#include <string.h>

/* ---------- the ring

The commands are written to one batch buffer, which the writer gives to the
host whenever it is full and at each flush. The host copies a batch out and
runs it before host_render_submit returns, so what a command refers to in
guest memory need only stay as it is until the batch it is in is given. */

#define BATCH_BUFFER_SIZE 0x400000UL

static unsigned int batch_words[BATCH_BUFFER_SIZE / 4];
static struct opence_render_writer writer;
static BOOL writer_made;

static struct opence_render_writer *ring(void)
{
	if (!writer_made)
	{
		opence_render_writer_init(&writer, batch_words, sizeof(batch_words));
		writer_made = TRUE;
	}
	return &writer;
}

/* ---------- staging

Uploads are copied here as their commands are written, so that the game can
go on writing its memory before the batch goes. The copies stay until the
next flush: a batch the writer gives on its own, being full, may leave the
command that needs a copy for the next. */

#define STAGING_SIZE 0x800000UL

static unsigned char staging[STAGING_SIZE];
static unsigned long staging_used;

static void staging_reset(void)
{
	staging_used = 0;
}

unsigned int *xgpu_metal_command(unsigned int op, unsigned int words)
{
	return opence_render_command(ring(), op, words);
}

void xgpu_metal_flush(void)
{
	int refused = opence_render_submit(ring());

	if (refused)
		platform_log("metal: the host refused %d of a batch's commands", refused);
	staging_reset();
}

void *xgpu_metal_stage_space(unsigned long size)
{
	/* (16-byte aligned, for the texels written there) */
	unsigned long start = (staging_used + 15) & ~15UL;

	if (!size || size > STAGING_SIZE / 2)
		return NULL;
	if (start + size > STAGING_SIZE)
	{
		xgpu_metal_flush();
		start = 0;
	}
	staging_used = start + size;
	return staging + start;
}

const void *xgpu_metal_stage(const void *data, unsigned long size)
{
	void *copy = xgpu_metal_stage_space(size);

	if (copy)
		memcpy(copy, data, size);
	return copy;
}

/* (the device sets every state its draws need: none is kept yet) */
void xgpu_gl_state_invalidate(void)
{
}

/* UPLOAD_BUFFER, or WRITE_BUFFER (copied by the host when it reads it) */
static void copy_to_buffer(unsigned int op, unsigned int buffer, unsigned long offset, const void *data,
	unsigned long size)
{
	const unsigned char *bytes = data;

	while (size)
	{
		unsigned long piece = size < STAGING_SIZE / 2 ? size : STAGING_SIZE / 2;
		const void *copy = xgpu_metal_stage(bytes, piece);
		unsigned int *fields = copy ? xgpu_metal_command(op, OPENCE_RENDER_UPLOAD_BUFFER_WORDS) : NULL;

		if (!fields)
			return;
		fields[0] = buffer;
		fields[1] = (unsigned int)offset;
		fields[2] = (unsigned int)(unsigned long)copy;
		fields[3] = (unsigned int)piece;
		offset += piece;
		bytes += piece;
		size -= piece;
	}
}

void xgpu_metal_upload_buffer(unsigned int buffer, unsigned long offset, const void *data, unsigned long size)
{
	copy_to_buffer(OPENCE_RENDER_OP_UPLOAD_BUFFER, buffer, offset, data, size);
}

unsigned int xgpu_metal_rgba_texture(const unsigned char *rgba, unsigned long width, unsigned long height,
	unsigned long levels)
{
	unsigned int id = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_TEXTURE);
	unsigned int *fields;

	if (!id)
		return 0;
	if (!(fields = xgpu_metal_command(OPENCE_RENDER_OP_CREATE_TEXTURE, OPENCE_RENDER_CREATE_TEXTURE_WORDS)))
	{
		xgpu_metal_destroy(OPENCE_RENDER_OBJECT_TEXTURE, id);
		return 0;
	}
	fields[0] = id;
	fields[1] = OPENCE_RENDER_TEXTURE_2D;
	fields[2] = OPENCE_RENDER_FORMAT_RGBA8;
	fields[3] = (unsigned int)width;
	fields[4] = (unsigned int)height;
	fields[5] = 1;
	fields[6] = (unsigned int)levels;
	fields[7] = OPENCE_RENDER_USAGE_SAMPLED;
	fields[8] = 1;
	/* (level 0 read from where the texels are, before this returns) */
	if ((fields = xgpu_metal_command(OPENCE_RENDER_OP_UPLOAD_TEXTURE, OPENCE_RENDER_UPLOAD_TEXTURE_WORDS)))
	{
		memset(fields, 0, (OPENCE_RENDER_UPLOAD_TEXTURE_WORDS - 1) * sizeof(*fields));
		fields[0] = id;
		fields[6] = (unsigned int)width;
		fields[7] = (unsigned int)height;
		fields[8] = 1;
		fields[9] = (unsigned int)(unsigned long)rgba;
		fields[10] = (unsigned int)(width * 4);
		fields[11] = (unsigned int)(width * height * 4);
		fields[12] = (unsigned int)(width * height * 4);
	}
	if (levels > 1)
	{
		xgpu_metal_pass_end();
		if ((fields = xgpu_metal_command(OPENCE_RENDER_OP_GENERATE_MIPMAPS, OPENCE_RENDER_GENERATE_MIPMAPS_WORDS)))
			fields[0] = id;
	}
	xgpu_metal_flush();
	return id;
}

/* ---------- ids

Each kind of object has its ids, from 1 to the ring's limit for the kind;
a destroyed object's id is given out again. */

#define ID_KINDS 6

static const unsigned int id_limits[ID_KINDS + 1] =
{
	0,
	OPENCE_RENDER_MAX_BUFFERS,
	OPENCE_RENDER_MAX_TEXTURES,
	OPENCE_RENDER_MAX_LIBRARIES,
	OPENCE_RENDER_MAX_PIPELINES,
	OPENCE_RENDER_MAX_DEPTH_STENCILS,
	OPENCE_RENDER_MAX_SAMPLERS,
};

static unsigned char ids_used[ID_KINDS + 1][OPENCE_RENDER_MAX_BUFFERS / 8 + 1];
/* where the next search starts */
static unsigned int id_next[ID_KINDS + 1];

/* what the shared code asks of the GPU: Apple's GPUs sample the
compressed formats (BC1 to BC3) as they are; the shaders are MSL, which
the GLSL version string does not name */
struct xgpu_capabilities xgpu_capabilities = { TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, "metal" };

unsigned int xgpu_metal_new_id(unsigned int kind)
{
	unsigned int tried, id;

	if (!kind || kind > ID_KINDS)
		return 0;
	id = id_next[kind];
	for (tried = 0; tried < id_limits[kind]; tried++)
	{
		id = id % id_limits[kind] + 1;
		if (!(ids_used[kind][id / 8] & (1 << (id % 8))))
		{
			ids_used[kind][id / 8] |= (unsigned char)(1 << (id % 8));
			id_next[kind] = id;
			return id;
		}
	}
	platform_log("metal: no ids are left for objects of kind %u", kind);
	return 0;
}

void xgpu_metal_destroy(unsigned int kind, unsigned int id)
{
	unsigned int *fields;

	if (!kind || kind > ID_KINDS || !id || id > id_limits[kind] || !(ids_used[kind][id / 8] & (1 << (id % 8))))
		return;
	fields = xgpu_metal_command(OPENCE_RENDER_OP_DESTROY, OPENCE_RENDER_DESTROY_WORDS);
	if (fields)
	{
		fields[0] = kind;
		fields[1] = id;
	}
	ids_used[kind][id / 8] &= (unsigned char)~(1 << (id % 8));
}

static unsigned int create_buffer(unsigned long length)
{
	unsigned int id = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_BUFFER);
	unsigned int *fields = id ? xgpu_metal_command(OPENCE_RENDER_OP_CREATE_BUFFER,
		OPENCE_RENDER_CREATE_BUFFER_WORDS) : NULL;

	if (!fields)
		return 0;
	fields[0] = id;
	fields[1] = (unsigned int)length;
	return id;
}

/* ---------- frames */

static unsigned long frame;
/* the bytes of the stream buffer's ring this frame has used */
static unsigned long stream_frame_used;

void xgpu_metal_begin_frame(void)
{
	frame++;
	stream_frame_used = 0;
}

/* ---------- the contiguous window in Metal buffers

As d3d8_gl.c's mirror: a copy of the contiguous window in buffers, a
segment each, made when first needed, to which a page is uploaded when it is
first drawn from or after the game has written it; pages are write-protected
once uploaded, as cached textures are (memory_watch.c), and pages rewritten
frame after frame count as volatile for a while, drawn through the stream
buffer instead. An upload the ring orders after the draws written before
it, as glBufferSubData is: they read what the page held. */

#define MIRROR_SEGMENT_SIZE 0x400000UL
#define MIRROR_SEGMENT_COUNT (PLATFORM_CONTIGUOUS_SIZE / MIRROR_SEGMENT_SIZE)
#define MIRROR_PAGE_SIZE 0x1000UL
#define MIRROR_PAGE_COUNT (PLATFORM_CONTIGUOUS_SIZE / MIRROR_PAGE_SIZE)
/* rewrites no more than this many frames apart ... */
#define MIRROR_REWRITE_FRAMES 2
/* ... this many times in a row make a page volatile ... */
#define MIRROR_VOLATILE_REWRITES 4
/* ... for this many frames */
#define MIRROR_VOLATILE_FRAMES 600

enum
{
	_mirror_page_absent,
	_mirror_page_present,
	_mirror_page_volatile
};

static struct
{
	unsigned int buffers[MIRROR_SEGMENT_COUNT];
	unsigned char state[MIRROR_PAGE_COUNT];
	unsigned char rewrites[MIRROR_PAGE_COUNT];
	/* the page's memory_watch generation when it was uploaded */
	unsigned long generation[MIRROR_PAGE_COUNT];
	unsigned long rewritten_frame[MIRROR_PAGE_COUNT];
} mirror;

/* uploads the pages of [first, last) that are absent or stale; FALSE if one
of them turns out to be volatile */
static BOOL mirror_refresh(unsigned long first, unsigned long last)
{
	unsigned long page, run;
	BOOL volatile_page = FALSE;
	unsigned char stale[256];
	unsigned long count = last - first;

	if (count > sizeof(stale))
	{
		/* a range this long is refreshed in pieces */
		for (page = first; page < last; page += sizeof(stale))
		{
			if (!mirror_refresh(page, page + sizeof(stale) < last ? page + sizeof(stale) : last))
				return FALSE;
		}
		return TRUE;
	}
	for (page = first; page < last; page++)
	{
		BOOL written = mirror.state[page] == _mirror_page_present &&
			memory_watch_generation(PLATFORM_CONTIGUOUS_BASE + page * MIRROR_PAGE_SIZE, MIRROR_PAGE_SIZE) >
			mirror.generation[page];

		stale[page - first] = mirror.state[page] != _mirror_page_present || written;
		if (written)
		{
			if (frame - mirror.rewritten_frame[page] <= MIRROR_REWRITE_FRAMES)
				mirror.rewrites[page]++;
			else
				mirror.rewrites[page] = 1;
			mirror.rewritten_frame[page] = frame;
			if (mirror.rewrites[page] >= MIRROR_VOLATILE_REWRITES)
			{
				mirror.state[page] = _mirror_page_volatile;
				volatile_page = TRUE;
			}
		}
	}
	if (volatile_page)
		return FALSE;
	for (page = first; page < last; page = run)
	{
		unsigned long segment = page * MIRROR_PAGE_SIZE / MIRROR_SEGMENT_SIZE;
		unsigned long address, size;

		if (!stale[page - first])
		{
			run = page + 1;
			continue;
		}
		for (run = page; run < last && stale[run - first]; run++)
		{
			if (mirror.state[run] != _mirror_page_present)
			{
				mirror.rewrites[run] = 0;
				mirror.rewritten_frame[run] = frame;
			}
		}
		if (!mirror.buffers[segment])
		{
			mirror.buffers[segment] = create_buffer(MIRROR_SEGMENT_SIZE);
			if (!mirror.buffers[segment])
				return FALSE;
		}
		address = PLATFORM_CONTIGUOUS_BASE + page * MIRROR_PAGE_SIZE;
		size = (run - page) * MIRROR_PAGE_SIZE;
		/* protect first, so a write racing with the upload is noticed */
		memory_watch_protect(address, size);
		for (; page < run; page++)
		{
			mirror.generation[page] = memory_watch_generation(PLATFORM_CONTIGUOUS_BASE + page * MIRROR_PAGE_SIZE,
				MIRROR_PAGE_SIZE);
			mirror.state[page] = _mirror_page_present;
		}
		xgpu_metal_upload_buffer(mirror.buffers[segment], address - PLATFORM_CONTIGUOUS_BASE -
			segment * MIRROR_SEGMENT_SIZE, (const void *)address, size);
	}
	return TRUE;
}

BOOL xgpu_metal_mirror_range(unsigned long address, unsigned long size, unsigned int *buffer,
	unsigned long *offset, unsigned long *generation)
{
	unsigned long start = address - PLATFORM_CONTIGUOUS_BASE;
	unsigned long segment, first, last, page, oldest = ~0UL, newest = 0;
	BOOL present = TRUE;

	if (!size || address < PLATFORM_CONTIGUOUS_BASE || start + size > PLATFORM_CONTIGUOUS_SIZE ||
		start + size < start)
		return FALSE;
	segment = start / MIRROR_SEGMENT_SIZE;
	if ((start + size - 1) / MIRROR_SEGMENT_SIZE != segment)
		return FALSE;
	first = start / MIRROR_PAGE_SIZE;
	last = (start + size - 1) / MIRROR_PAGE_SIZE + 1;
	for (page = first; page < last; page++)
	{
		if (mirror.state[page] == _mirror_page_volatile)
		{
			if (frame - mirror.rewritten_frame[page] < MIRROR_VOLATILE_FRAMES)
				return FALSE;
			mirror.state[page] = _mirror_page_absent;
		}
		if (mirror.state[page] != _mirror_page_present)
		{
			present = FALSE;
		}
		else
		{
			if (mirror.generation[page] < oldest)
				oldest = mirror.generation[page];
			if (mirror.generation[page] > newest)
				newest = mirror.generation[page];
		}
	}
	if (!present || memory_watch_generation(address, size) > oldest)
	{
		if (!mirror_refresh(first, last))
			return FALSE;
		for (newest = 0, page = first; page < last; page++)
		{
			if (mirror.generation[page] > newest)
				newest = mirror.generation[page];
		}
	}
	*buffer = mirror.buffers[segment];
	*offset = start - segment * MIRROR_SEGMENT_SIZE;
	if (generation)
		*generation = newest;
	return TRUE;
}

/* ---------- the stream buffer

Vertices and indices that are not in the mirror (the game's own arrays,
volatile pages) are copied to one buffer, used as a ring. The host copies
them when it reads the command (WRITE_BUFFER), so that a pass's encoder is
not ended for a blit, and waits for earlier frames that read the range;
nothing this frame has read is written over, unless the frame goes round
the whole ring, when the copies are uploads ordered after the draws. */

#define STREAM_SIZE 0x2000000UL

static unsigned int stream_buffer;
static unsigned long stream_head;

BOOL xgpu_metal_stream(const void *data, unsigned long size, unsigned long alignment, unsigned int *buffer,
	unsigned long *offset)
{
	unsigned long start;

	if (!size || size > STREAM_SIZE || size > STAGING_SIZE / 2)
		return FALSE;
	if (!stream_buffer)
	{
		stream_buffer = create_buffer(STREAM_SIZE);
		if (!stream_buffer)
			return FALSE;
	}
	if (!alignment)
		alignment = 1;
	start = (stream_head + alignment - 1) & ~(alignment - 1);
	if (start + size > STREAM_SIZE)
	{
		stream_frame_used += STREAM_SIZE - stream_head;
		start = 0;
	}
	else
		stream_frame_used += start - stream_head;
	stream_frame_used += size;
	copy_to_buffer(stream_frame_used <= STREAM_SIZE ? OPENCE_RENDER_OP_WRITE_BUFFER : OPENCE_RENDER_OP_UPLOAD_BUFFER,
		stream_buffer, start, data, size);
	stream_head = start + size;
	*buffer = stream_buffer;
	*offset = start;
	return TRUE;
}

/* ---------- render targets

A surface the game draws into is a texture of the ring, found by the
surface's memory and size (d3d8_gl.c, render_target_get). Until the device
draws at the display's resolution every target is at the game's own, a
pixel a unit. */

#define RENDER_TARGET_BUCKET_COUNT 256

struct render_target_entry
{
	struct render_target_entry *next_in_bucket;
	struct xgpu_render_target target;
};

static struct render_target_entry *render_target_buckets[RENDER_TARGET_BUCKET_COUNT];

static struct render_target_entry **render_target_bucket(unsigned long data)
{
	return &render_target_buckets[((data >> 12) ^ (data >> 20)) % RENDER_TARGET_BUCKET_COUNT];
}

static BOOL format_is_depth(DWORD format)
{
	return format == D3DFMT_D24S8 || format == D3DFMT_F24S8 || format == D3DFMT_D16 || format == D3DFMT_F16 ||
		format == D3DFMT_LIN_D24S8 || format == D3DFMT_LIN_F24S8 || format == D3DFMT_LIN_D16 ||
		format == D3DFMT_LIN_F16;
}

struct xgpu_render_target *xgpu_metal_render_target_get(const D3DSurface *surface)
{
	struct xgpu_texture_description description;
	struct render_target_entry *entry;
	unsigned int *fields;
	BOOL depth;

	if (!surface || !surface->Data)
		return NULL;
	xgpu_texture_describe(surface->Format, surface->Size, &description);
	depth = format_is_depth(description.format);
	for (entry = *render_target_bucket(surface->Data); entry; entry = entry->next_in_bucket)
	{
		if (entry->target.data == surface->Data && entry->target.width == description.width &&
			entry->target.height == description.height && entry->target.depth == depth)
			return &entry->target;
	}
	if (description.width > OPENCE_RENDER_MAX_TEXTURE_SIZE || description.height > OPENCE_RENDER_MAX_TEXTURE_SIZE)
		return NULL;
	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return NULL;
	entry->target.data = surface->Data;
	entry->target.width = description.width;
	entry->target.height = description.height;
	entry->target.depth = depth;
	entry->target.scale[0] = 1.0f;
	entry->target.scale[1] = 1.0f;
	entry->target.gl_width = description.width;
	entry->target.gl_height = description.height;
	entry->target.texture = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_TEXTURE);
	fields = entry->target.texture ? xgpu_metal_command(OPENCE_RENDER_OP_CREATE_TEXTURE,
		OPENCE_RENDER_CREATE_TEXTURE_WORDS) : NULL;
	if (!fields)
	{
		free(entry);
		return NULL;
	}
	fields[0] = entry->target.texture;
	fields[1] = OPENCE_RENDER_TEXTURE_2D;
	fields[2] = depth ? OPENCE_RENDER_FORMAT_DEPTH32F_STENCIL8 : OPENCE_RENDER_FORMAT_BGRA8;
	fields[3] = (unsigned int)description.width;
	fields[4] = (unsigned int)description.height;
	fields[5] = 1;
	fields[6] = 1;
	fields[7] = OPENCE_RENDER_USAGE_SAMPLED | OPENCE_RENDER_USAGE_RENDER_TARGET;
	fields[8] = 1;
	entry->next_in_bucket = *render_target_bucket(surface->Data);
	*render_target_bucket(surface->Data) = entry;
	return &entry->target;
}

/* the colour target whose memory starts at data, if it has been drawn to */
struct xgpu_render_target *xgpu_render_target_find(unsigned long data)
{
	struct render_target_entry *entry;

	for (entry = *render_target_bucket(data); entry; entry = entry->next_in_bucket)
	{
		if (entry->target.data == data && !entry->target.depth)
			return &entry->target;
	}
	return NULL;
}

/* the menus' art is the GL renderer's; nothing of it is held here */
void xgpu_gl_texture_deleted(unsigned int texture)
{
	(void)texture;
}
