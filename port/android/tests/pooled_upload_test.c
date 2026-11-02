/* Android's streamed uploads into pooled buffers (web_chunk_get,
stream_reserve, stream_upload, index_upload and web_chunks_begin_frame of
d3d8_gl.c), taken from the source by tools/test_android_static_geometry.py,
against a fake GL that records which buffer ranges each queued draw reads
and fails if an upload writes one while it is queued. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int BOOL;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef int GLsizei;
typedef long GLsizeiptr;
typedef long GLintptr;
#define TRUE 1
#define FALSE 0
#define HALO_GLES 1
#ifndef HALO_WEB
#define HALO_ANDROID 1
static int android_geometry_fixes = 1;
#endif
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define STREAM_BUFFER_RING 3
#define STREAM_BUFFER_SIZE (16 * 1024 * 1024)
#define INDEX_BUFFER_SIZE (2 * 1024 * 1024)
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static struct
{
	GLuint stream_buffer;
	GLuint stream_buffers[STREAM_BUFFER_RING];
	GLuint index_buffers[STREAM_BUFFER_RING];
	unsigned long buffer_ring;
	unsigned long stream_offset;
	GLuint index_buffer;
	unsigned long index_offset;
	unsigned long stream_capacity;
	unsigned long index_capacity;
	unsigned long draw_serial;
	unsigned long stream_serial;
	unsigned long index_serial;
} device;

#define BUFFERS 4096
/* the frame that last drew from each buffer, and since when it is safe */
static long last_read_frame[BUFFERS];
static GLuint next_name = 100, array_bound, element_bound;
static long frame;
static int subdata, unsynchronized, created, writes_into_read;
/* what the current draw reads, recorded as it uploads */
static GLuint draw_buffers[8];
static int draw_buffer_count;

static void glGenBuffers(GLsizei count, GLuint *names)
{
	while (count--)
	{
		CHECK(next_name < BUFFERS);
		last_read_frame[next_name] = -100;
		*names++ = next_name++;
		created++;
	}
}

static void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
	(void)target;
	(void)size;
	(void)data;
	(void)usage;
}

/* a write into a buffer a draw of this frame, or of the two before it
(the GPU may not have run them: the ring's fences), reads */
static void written(GLuint buffer)
{
	int index;

	if (frame - last_read_frame[buffer] < STREAM_BUFFER_RING)
	{
		/* (unless the reader is the draw being made: its own streams) */
		for (index = 0; index < draw_buffer_count; index++)
		{
			if (draw_buffers[index] == buffer)
				return;
		}
		writes_into_read++;
	}
}

static void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
{
	(void)offset;
	(void)size;
	(void)data;
	written(target == GL_ARRAY_BUFFER ? array_bound : element_bound);
	subdata++;
}

static void host_gl_buffer_write(GLenum target, unsigned int offset, unsigned int size, const void *data)
{
	(void)target;
	(void)offset;
	(void)size;
	(void)data;
	unsynchronized++;
}

static void state_array_buffer(GLuint buffer) { array_bound = buffer; }
static void state_element_array_buffer(GLuint buffer) { element_bound = buffer; }
static void buffer_upload(GLenum target, unsigned long offset, unsigned long size, const void *data)
{
	(void)target;
	(void)offset;
	(void)size;
	(void)data;
}

#include "pooled_upload.inc"

static unsigned char bytes[1 << 20];

/* a draw: its streams reserved together, uploaded, then its indices */
static void draw(int streams, unsigned long vertex_bytes, unsigned long index_bytes)
{
	unsigned long total = 0;
	int stream, index;

	device.draw_serial++;
	draw_buffer_count = 0;
	for (stream = 0; stream < streams; stream++)
		total += (vertex_bytes + 15) & ~15UL;
	stream_reserve(total);
	for (stream = 0; stream < streams; stream++)
	{
		stream_upload(bytes, vertex_bytes);
		CHECK(draw_buffer_count < 8);
		draw_buffers[draw_buffer_count++] = device.stream_buffer;
	}
	if (index_bytes)
	{
		index_upload(bytes, index_bytes);
		draw_buffers[draw_buffer_count++] = device.index_buffer;
	}
	/* (queued: read when the GPU gets to it) */
	for (index = 0; index < draw_buffer_count; index++)
		last_read_frame[draw_buffers[index]] = frame;
}

/* Present: the ring's next slot, whose frame the fence says is done */
static void present(void)
{
	frame++;
	device.buffer_ring = (device.buffer_ring + 1) % STREAM_BUFFER_RING;
	device.stream_buffer = device.stream_buffers[device.buffer_ring];
	device.index_buffer = device.index_buffers[device.buffer_ring];
	device.stream_offset = 0;
	device.index_offset = 0;
	web_chunks_begin_frame();
}

int main(int argc, char **argv)
{
	int round, count;

	if (argc != 2)
		return 2;
	for (count = 0; count < STREAM_BUFFER_RING; count++)
	{
		device.stream_buffers[count] = (GLuint)(1 + count);
		device.index_buffers[count] = (GLuint)(10 + count);
	}
	device.stream_buffer = device.stream_buffers[0];
	device.index_buffer = device.index_buffers[0];
#ifdef HALO_ANDROID
	if (!strcmp(argv[1], "disabled"))
	{
		android_geometry_fixes = 0;
		CHECK(!stream_uses_chunks());
		stream_reserve(64);
		CHECK(stream_upload(bytes, 32) == 0);
		device.draw_serial++;
		CHECK(stream_upload(bytes, 32) == 32);
		CHECK(index_upload(bytes, 32) == 0);
		device.draw_serial++;
		CHECK(index_upload(bytes, 32) == 32);
		device.stream_offset = STREAM_BUFFER_SIZE - 16;
		stream_reserve(64);
		CHECK(stream_upload(bytes, 64) == 0);
		device.index_offset = INDEX_BUFFER_SIZE - 16;
		CHECK(index_upload(bytes, 64) == 0);
		CHECK(device.stream_buffer == 1 && device.index_buffer == 10);
		CHECK(!created && !subdata && unsynchronized == 6);
		present();
		CHECK(!created && device.stream_buffer == 2 && device.index_buffer == 11);
		CHECK(!device.stream_offset && !device.index_offset);
		return 0;
	}
#endif
	if (!strcmp(argv[1], "frames"))
	{
		/* frames of different sizes and draws: the pools grow, then are reused */
		for (round = 0; round < 60; round++)
		{
			for (count = 0; count < 40 + (round * 37) % 90; count++)
				draw(1 + count % 3, 64 + (unsigned long)(count * 97 % 5000), (count % 4) ? 200 + count * 13 % 3000 : 0);
			present();
		}
		CHECK(writes_into_read == 0);
		#ifdef HALO_WEB
		CHECK(subdata == 0 && unsynchronized > 0);
#else
		CHECK(unsynchronized == 0 && subdata > 0);
#endif
		/* (reused: far fewer buffers than draws) */
		CHECK(created < 1200);
		fprintf(stderr, "subdata=%d created=%d\n", subdata, created);
		return 0;
	}
	if (!strcmp(argv[1], "large"))
	{
		/* uploads past a pool class and past the ring buffer's size */
		draw(2, 600000, 300000);
		draw(1, 5000000, 3000000);
		present();
		draw(1, 64, 64);
		CHECK(writes_into_read == 0);
#ifndef HALO_WEB
		CHECK(unsynchronized == 0);
#endif
		return 0;
	}
	return 2;
}
