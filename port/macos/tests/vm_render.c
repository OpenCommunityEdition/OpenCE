/*
VM_RENDER.C

A guest program for tools/test_macos_render.py: render batches written by
the guest's writer (port/macos/guest/opence_render_writer.c) and given to
the host with host_render_submit. It runs the case named by argv[1], prints
what the test compares (each batch's FNV-1a and the host's answers), then
"ok", and returns 0; else it prints what failed and returns 1.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_host.h"
#include "opence_render.h"
#include "opence_render_writer.h"

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

static unsigned int buffer[16384];
static unsigned char data[256];

static unsigned int fnv(const void *bytes, unsigned int size)
{
	const unsigned char *at = bytes;
	unsigned int hash = 0x811c9dc5u;
	unsigned int index;

	for (index = 0; index < size; index++)
		hash = (hash ^ at[index]) * 0x01000193u;
	return hash;
}

/* submits the writer's batch, printing its hash first */
static int submit(struct opence_render_writer *writer)
{
	unsigned int size = opence_render_finish(writer);

	printf("batch %u fnv 0x%08x\n", writer->submitted + 1, fnv(writer->words, size));
	return opence_render_submit(writer);
}

static void create_buffer(struct opence_render_writer *writer, unsigned int id, unsigned int length)
{
	unsigned int *fields = opence_render_command(writer, OPENCE_RENDER_OP_CREATE_BUFFER,
		OPENCE_RENDER_CREATE_BUFFER_WORDS);

	CHECK(fields);
	fields[0] = id;
	fields[1] = length;
}

static void upload_buffer(struct opence_render_writer *writer, unsigned int id, const void *from, unsigned int length)
{
	unsigned int *fields = opence_render_command(writer, OPENCE_RENDER_OP_UPLOAD_BUFFER,
		OPENCE_RENDER_UPLOAD_BUFFER_WORDS);

	CHECK(fields);
	fields[0] = id;
	fields[1] = 0;
	fields[2] = (unsigned int)(unsigned long)from;
	fields[3] = length;
}

static void destroy_buffer(struct opence_render_writer *writer, unsigned int id)
{
	unsigned int *fields = opence_render_command(writer, OPENCE_RENDER_OP_DESTROY, OPENCE_RENDER_DESTROY_WORDS);

	CHECK(fields);
	fields[0] = OPENCE_RENDER_OBJECT_BUFFER;
	fields[1] = id;
}

/* ---------- batches the host decodes as written, and refuses as it should */

static void batches(void)
{
	struct opence_render_writer writer;
	unsigned int bogus[4] = {0, OPENCE_RENDER_VERSION, 16, 0};

	memset(data, 0x5a, sizeof(data));
	opence_render_writer_init(&writer, buffer, sizeof(buffer));
	CHECK(opence_render_submit(&writer) == 0 && writer.submitted == 0);

	create_buffer(&writer, 1, sizeof(data));
	upload_buffer(&writer, 1, data, sizeof(data));
	create_buffer(&writer, 2, 64);
	CHECK(writer.count == 3);
	CHECK(submit(&writer) == 0);

	/* the objects last from batch to batch: buffer 1 is still there, and 2
	once destroyed is refused */
	upload_buffer(&writer, 1, data, 128);
	destroy_buffer(&writer, 2);
	upload_buffer(&writer, 2, data, 64);
	CHECK(submit(&writer) == 1);

	/* a buffer the guest has not mapped */
	upload_buffer(&writer, 1, (const void *)0xfffff000ul, 64);
	CHECK(submit(&writer) == 1);
	CHECK(writer.submitted == 3 && writer.refused == 2 && writer.failed == 0);

	/* what is not a batch is refused whole */
	CHECK(host_render_submit((unsigned int)(unsigned long)bogus, sizeof(bogus)) == -1);
	CHECK(host_render_submit((unsigned int)(unsigned long)bogus, 6) == -1);
	CHECK(host_render_submit(0xfffff000u, 16) == -1);
}

/* ---------- a writer whose buffer fills submits on its own */

static void flush(void)
{
	struct opence_render_writer writer;
	unsigned int small[64];
	unsigned int id;

	opence_render_writer_init(&writer, small, sizeof(small));
	/* (16 bytes of header and 20 commands of 12 bytes each in 256) */
	for (id = 1; id <= 100; id++)
		create_buffer(&writer, id, 16);
	CHECK(writer.submitted == 4 && writer.count == 20);
	CHECK(opence_render_submit(&writer) == 0);
	CHECK(writer.submitted == 5 && writer.refused == 0);

	/* a command that no batch of the buffer holds is refused here */
	CHECK(opence_render_command(&writer, OPENCE_RENDER_OP_SET_BYTES, 61) == NULL);
	CHECK(opence_render_command(&writer, OPENCE_RENDER_OP_SET_BYTES, 0) == NULL);
	CHECK(writer.count == 0 && writer.submitted == 5);
	printf("batches %u\n", writer.submitted);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("usage: vm_render case\n");
		return 1;
	}
	if (!strcmp(argv[1], "batches"))
		batches();
	else if (!strcmp(argv[1], "flush"))
		flush();
	else
	{
		printf("no case %s\n", argv[1]);
		return 1;
	}
	printf("ok\n");
	return 0;
}
