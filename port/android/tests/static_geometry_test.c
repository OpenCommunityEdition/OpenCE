/* The static geometry cache of d3d8_gl.c (xgpu_static_buffer_add,
static_buffer_range and the rest), taken from the source by
tools/test_android_static_geometry.py, against a fake GL that keeps each
buffer's storage and can refuse it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int BOOL;
typedef unsigned long DWORD;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef long GLsizeiptr;
#define TRUE 1
#define FALSE 0
#define HALO_ANDROID 1
static int android_geometry_fixes = 1;
#define GL_NO_ERROR 0
#define GL_OUT_OF_MEMORY 0x0505
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

/* contiguous memory: physical addresses are offsets into it */
static unsigned char window[1 << 20];
#define PLATFORM_PHYSICAL_TO_VIRTUAL(p) ((void *)(window + (unsigned long)(p)))
#define PHYSICAL(p) ((DWORD)((unsigned char *)(p) - window))

#define BUFFERS 64
static struct
{
	BOOL live;
	unsigned char *storage;
	unsigned long size;
	GLenum target;
} buffers[BUFFERS];
static GLuint next_name = 1, array_bound, element_bound;
static GLenum error;
static int refuse_next, generated, uploads, deleted, invalidations, logs, queued_errors;

static void glGenBuffers(GLsizei count, GLuint *names)
{
	while (count--)
	{
		CHECK(next_name < BUFFERS);
		buffers[next_name].live = TRUE;
		*names++ = next_name++;
		generated++;
	}
}

static void glDeleteBuffers(GLsizei count, const GLuint *names)
{
	while (count--)
	{
		GLuint name = *names++;

		CHECK(name && name < BUFFERS && buffers[name].live);
		free(buffers[name].storage);
		memset(&buffers[name], 0, sizeof(buffers[name]));
		if (array_bound == name)
			array_bound = 0;
		if (element_bound == name)
			element_bound = 0;
		deleted++;
	}
}

static void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
	GLuint name = target == GL_ARRAY_BUFFER ? array_bound : element_bound;

	(void)usage;
	CHECK(name && buffers[name].live);
	if (refuse_next)
	{
		refuse_next = 0;
		error = GL_OUT_OF_MEMORY;
		return;
	}
	free(buffers[name].storage);
	buffers[name].storage = malloc((size_t)size);
	CHECK(buffers[name].storage);
	memcpy(buffers[name].storage, data, (size_t)size);
	buffers[name].size = (unsigned long)size;
	buffers[name].target = target;
	uploads++;
}

static GLenum glGetError(void)
{
	GLenum result;

	if (queued_errors)
	{
		queued_errors--;
		return 0x0500;
	}
	result = error;
	error = GL_NO_ERROR;
	return result;
}

static void state_array_buffer(GLuint buffer) { array_bound = buffer; }
static void state_element_array_buffer(GLuint buffer) { element_bound = buffer; }
static void xgpu_gl_state_invalidate(void) { invalidations++; }
static void platform_log(const char *format, ...) { (void)format; logs++; }

void xgpu_static_buffer_add(const void *resource, unsigned long size, int indices);
void xgpu_static_buffer_remove(const void *resource, int indices);
void xgpu_static_buffers_add(const void *buffers, long count, unsigned long stride, int indices, const void *end);
void xgpu_static_buffers_remove(const void *buffers, long count, unsigned long stride, int indices);
void xgpu_static_range_add(const void *start, unsigned long size, int indices);
void xgpu_static_range_remove(const void *start, int indices);

#include "static_geometry.inc"

/* a D3D resource: Common, Data, Lock */
struct resource
{
	DWORD common, data, lock;
};

static void fill(unsigned char *bytes, unsigned long size, unsigned char seed)
{
	unsigned long index;

	for (index = 0; index < size; index++)
		bytes[index] = (unsigned char)(seed + index * 7);
}

/* the bytes a draw would read from the buffer the cache gave */
static BOOL draw_reads(unsigned long address, unsigned long size, BOOL indices)
{
	GLuint buffer;
	unsigned long offset, serial;

	if (!static_buffer_range(address, size, indices, &buffer, &offset, &serial))
		return FALSE;
	CHECK(buffers[buffer].live && buffers[buffer].storage);
	CHECK(offset + size <= buffers[buffer].size);
	CHECK(!memcmp(buffers[buffer].storage + offset, (const void *)address, size));
	return TRUE;
}

static int reuse(void)
{
	struct resource vertices = { 0, 0x1000, 0 }, indices = { 0, 0, 0 };
	static unsigned char index_bytes[256];

	fill(window + 0x1000, 0x800, 1);
	fill(index_bytes, sizeof(index_bytes), 2);
	indices.data = (DWORD)(unsigned long)index_bytes;
	xgpu_static_buffer_add(&vertices, 0x800, 0);
	xgpu_static_buffer_add(&indices, sizeof(index_bytes), 1);
	/* (filled once, at the first draw, and read from then on) */
	CHECK(uploads == 0);
	CHECK(draw_reads((unsigned long)window + 0x1000, 0x800, FALSE));
	CHECK(draw_reads((unsigned long)window + 0x1100, 0x40, FALSE));
	CHECK(draw_reads((unsigned long)index_bytes + 16, 32, TRUE));
	CHECK(draw_reads((unsigned long)index_bytes + 16, 32, TRUE));
	CHECK(uploads == 2 && generated == 2);
	/* (past the end, before the start, or the other kind: streamed) */
	CHECK(!draw_reads((unsigned long)window + 0x17f0, 0x20, FALSE));
	CHECK(!draw_reads((unsigned long)window + 0xff0, 0x20, FALSE));
	CHECK(!draw_reads((unsigned long)window + 0x1000, 0x20, TRUE));
	CHECK(uploads == 2);
	return 0;
}

static int reload(void)
{
	struct resource vertices = { 0, 0x2000, 0 };
	GLuint buffer;
	unsigned long offset, first_serial, second_serial;

	fill(window + 0x2000, 0x400, 3);
	xgpu_static_buffer_add(&vertices, 0x400, 0);
	CHECK(static_buffer_range((unsigned long)window + 0x2000, 0x400, FALSE, &buffer, &offset, &first_serial));
	/* (released: no draw finds it, its buffer goes at Present) */
	xgpu_static_buffer_remove(&vertices, 0);
	CHECK(!draw_reads((unsigned long)window + 0x2000, 0x10, FALSE));
	CHECK(deleted == 0 && static_buffers.doomed_count == 1);
	static_buffers_collect();
	CHECK(deleted == 1 && invalidations == 1 && !buffers[buffer].live);
	/* (made again with other bytes: uploaded again, under a new serial) */
	fill(window + 0x2000, 0x400, 9);
	xgpu_static_buffer_add(&vertices, 0x400, 0);
	CHECK(static_buffer_range((unsigned long)window + 0x2000, 0x400, FALSE, &buffer, &offset, &second_serial));
	CHECK(second_serial != first_serial);
	CHECK(draw_reads((unsigned long)window + 0x2000, 0x400, FALSE));
	CHECK(uploads == 2);
	return 0;
}

static int address_reuse(void)
{
	struct resource first = { 0, 0x3000, 0 }, second = { 0, 0x3000, 0 };

	/* (a smaller resource at the freed address: its own size and bytes) */
	fill(window + 0x3000, 0x1000, 4);
	xgpu_static_buffer_add(&first, 0x1000, 0);
	CHECK(draw_reads((unsigned long)window + 0x3800, 0x100, FALSE));
	xgpu_static_buffer_remove(&first, 0);
	fill(window + 0x3000, 0x200, 5);
	xgpu_static_buffer_add(&second, 0x200, 0);
	CHECK(draw_reads((unsigned long)window + 0x3000, 0x200, FALSE));
	CHECK(!draw_reads((unsigned long)window + 0x3800, 0x100, FALSE));
	/* (added twice without a release: one entry, uploaded again, as the
	bytes may be new) */
	xgpu_static_buffer_add(&second, 0x200, 0);
	CHECK(static_buffer_lists[0].count == 1);
	static_buffers_collect();
	CHECK(deleted == 2);
	CHECK(draw_reads((unsigned long)window + 0x3000, 0x200, FALSE) && uploads == 3);
	return 0;
}

static int allocation_failure(void)
{
	struct resource refused = { 0, 0x4000, 0 }, kept = { 0, 0x5000, 0 };
	GLuint buffer;
	unsigned long offset;

	fill(window + 0x4000, 0x100, 6);
	fill(window + 0x5000, 0x100, 7);
	xgpu_static_buffer_add(&refused, 0x100, 0);
	xgpu_static_buffer_add(&kept, 0x100, 0);
	/* (an error queued before the upload is not taken for the upload's) */
	queued_errors = 2;
	CHECK(draw_reads((unsigned long)window + 0x5000, 0x100, FALSE));
	/* (no storage: the draw streams, the buffer goes, nothing is retried) */
	refuse_next = 1;
	CHECK(!static_buffer_range((unsigned long)window + 0x4000, 0x100, FALSE, &buffer, &offset, NULL));
	CHECK(deleted == 1 && invalidations == 1 && logs == 1);
	generated = 0;
	CHECK(!draw_reads((unsigned long)window + 0x4000, 0x100, FALSE));
	CHECK(!draw_reads((unsigned long)window + 0x4000, 0x100, FALSE));
	CHECK(generated == 0 && logs == 1);
	/* (the other entry is untouched) */
	CHECK(draw_reads((unsigned long)window + 0x5000, 0x100, FALSE));
	/* (made again after a release, it is tried again) */
	xgpu_static_buffer_remove(&refused, 0);
	xgpu_static_buffer_add(&refused, 0x100, 0);
	CHECK(draw_reads((unsigned long)window + 0x4000, 0x100, FALSE));
	return 0;
}

static int regions(void)
{
	/* an Xbox bsp at 0x10000..0x20000: two vertex buffers, and lightmap
	vertex buffers that start lower, all running to the bsp's end */
	struct resource vertices[2] = { { 0, 0x13000, 0 }, { 0, 0x12000, 0 } };
	struct resource lightmaps[1] = { { 0, 0x11000, 0 } };
	unsigned char *end = window + 0x20000;

	fill(window + 0x10000, 0x10000, 8);
	xgpu_static_buffers_add(vertices, 2, sizeof(vertices[0]), 0, end);
	CHECK(static_buffer_lists[0].count == 1 && static_buffer_lists[0].entries[0].start == (unsigned long)window + 0x12000);
	xgpu_static_buffers_add(lightmaps, 1, sizeof(lightmaps[0]), 0, end);
	CHECK(static_buffer_lists[0].count == 1 && static_buffer_lists[0].entries[0].start == (unsigned long)window + 0x11000);
	xgpu_static_buffers_add(vertices, 2, sizeof(vertices[0]), 0, end);
	CHECK(static_buffer_lists[0].count == 1);
	xgpu_static_range_add(window + 0x10000, 0x10000, 1);
	CHECK(draw_reads((unsigned long)window + 0x13010, 0x40, FALSE));
	CHECK(draw_reads((unsigned long)window + 0x10100, 0x40, TRUE));
	CHECK(!draw_reads((unsigned long)window + 0x10100, 0x40, FALSE));
	/* (unloaded) */
	xgpu_static_buffers_remove(lightmaps, 1, sizeof(lightmaps[0]), 0);
	xgpu_static_range_remove(window + 0x10000, 1);
	CHECK(static_buffer_lists[0].count == 0 && static_buffer_lists[1].count == 0);
	static_buffers_collect();
	CHECK(deleted == 2);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 2;
	if (!strcmp(argv[1], "disabled"))
	{
		android_geometry_fixes = 0;
		xgpu_static_range_add(window + 0x1000, 0x400, 0);
		xgpu_static_range_add(window + 0x2000, 0x400, 1);
		CHECK(!static_buffer_lists[0].count && !static_buffer_lists[1].count);
		CHECK(!static_buffer_lists[0].entries && !static_buffer_lists[1].entries);
		CHECK(!draw_reads((unsigned long)window + 0x1000, 0x40, FALSE));
		CHECK(!draw_reads((unsigned long)window + 0x2000, 0x40, TRUE));
		xgpu_static_range_remove(window + 0x1000, 0);
		xgpu_static_range_remove(window + 0x2000, 1);
		static_buffers_collect();
		CHECK(!generated && !uploads && !deleted);
		return 0;
	}
	if (!strcmp(argv[1], "reuse"))
		return reuse();
	if (!strcmp(argv[1], "reload"))
		return reload();
	if (!strcmp(argv[1], "address_reuse"))
		return address_reuse();
	if (!strcmp(argv[1], "allocation_failure"))
		return allocation_failure();
	if (!strcmp(argv[1], "regions"))
		return regions();
	return 2;
}
