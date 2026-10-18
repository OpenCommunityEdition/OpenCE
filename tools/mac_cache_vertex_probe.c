#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine_physical_memory.h"
#include "engine_device_state.h"
#include "engine_cache_resources.h"
#include "cache/cache_files.h"
#include "xtl.h"

#undef strcmp

/* Probe logging only; production resolves this to the native platform log. */
void platform_log(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
}

void display_assert(char *information, char *file, long line, boolean fatal)
{
	fprintf(stderr, "probe assertion %s:%ld: %s (%d)\n", file, line,
		information ? information : "", fatal);
	abort();
}

void system_exit(long code)
{
	exit((int)code);
}

struct cache_file_tag_header
{
	void *tag_instances; long scenario_tag_index; unsigned long checksum;
	long tag_count; long vertex_buffer_count; D3DVertexBuffer *vertex_buffers;
	long index_buffer_count; D3DIndexBuffer *index_buffers; unsigned long signature;
};

static unsigned int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); } } while (0)

void tags_header_register_vertex_and_index_buffers(struct cache_file_tag_header *);
void tags_header_deregister_vertex_and_index_buffers(struct cache_file_tag_header *);

int main(int argc, char **argv)
{
	unsigned char *memory;
	struct cache_file_tag_header *header;
	D3DVertexBuffer *vertex;
	D3DIndexBuffer *index;
	if (argc > 1 && strcmp(argv[1], "outside") == 0)
	{
		mac_engine_physical_memory_start();
		memory = (unsigned char *)XPhysicalAlloc(4096, 0xffffffffUL, 0, PAGE_READWRITE);
		if (!memory) return 2;
		vertex = (D3DVertexBuffer *)(memory + 16);
		vertex->Common = D3DCOMMON_TYPE_VERTEXBUFFER | 1;
		vertex->Data = (unsigned long)(memory + 512);
		mac_engine_cache_resource_validate((D3DResource *)vertex, FALSE);
		return 3;
	}
	if (argc > 1 && strcmp(argv[1], "outside-device-busy") == 0)
	{
		mac_device_state_start();
		D3DDevice_IsBusy();
		return 3;
	}
	if (argc > 1 && strcmp(argv[1], "outside-device-kick") == 0)
	{
		mac_device_state_start();
		D3DDevice_KickPushBuffer();
		return 3;
	}
	CHECK(mac_engine_physical_memory_start());
	CHECK(mac_device_headless_start(32, 32));
	CHECK(D3DDevice_IsBusy() == FALSE);
	D3DDevice_KickPushBuffer();
	memory = (unsigned char *)XPhysicalAlloc(4096, 0xffffffffUL, 0, PAGE_READWRITE);
	CHECK(memory != NULL);
	if (!memory) return 1;
	header = (struct cache_file_tag_header *)memory;
	vertex = (D3DVertexBuffer *)(memory + 64);
	index = (D3DIndexBuffer *)(memory + 80);
	header->vertex_buffer_count = 1;
	header->vertex_buffers = vertex;
	header->index_buffer_count = 1;
	header->index_buffers = index;
	vertex->Data = (unsigned long)(memory + 512);
	index->Data = (unsigned long)(memory + 768);
	tags_header_register_vertex_and_index_buffers(header);
	CHECK(vertex->Common == (D3DCOMMON_TYPE_VERTEXBUFFER | 1));
	CHECK(index->Common == (D3DCOMMON_TYPE_INDEXBUFFER | 1));
	CHECK(vertex->Data == ((unsigned long)(memory + 512) & ~0x80000000UL));
	CHECK(D3DResource_IsBusy((D3DResource *)vertex) == FALSE);
	CHECK(D3DResource_IsBusy((D3DResource *)index) == FALSE);
	tags_header_deregister_vertex_and_index_buffers(header);
	CHECK(mac_engine_physical_memory_contains(header, 96));
	XPhysicalFree(memory);
	CHECK(!mac_engine_physical_memory_contains(header, 1));
	mac_device_state_stop();
	mac_engine_physical_memory_stop();
	fprintf(stderr, "Mac cache vertex lifecycle: %u checks, %u failures\n", checks, failures);
	return failures ? 1 : 0;
}
