/*
VM_RENDER.C

A guest program for tools/test_macos_render.py: render batches written by
the guest's writer (port/macos/guest/opence_render_writer.c) and given to
the host with host_render_submit. It runs the case named by argv[1], prints
what the test compares (each batch's FNV-1a and the host's answers), then
"ok", and returns 0; else it prints what failed and returns 1.

The case "scene" draws one frame in the window, for the app's frame capture
(tools/test_macos_frames.py).
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

/* ---------- a frame for the frame checks: a triangle of three colours, and
a quad of half-transparent white blended over a part of it */

static const char scene_source[] =
	"#include <metal_stdlib>\n"
	"using namespace metal;\n"
	"struct In { float2 position [[attribute(0)]]; float4 color [[attribute(1)]]; };\n"
	"struct Out { float4 position [[position]]; float4 color; };\n"
	"vertex Out vertex_main(In in [[stage_in]]) {\n"
	"\tOut out;\n"
	"\tout.position = float4(in.position, 0, 1);\n"
	"\tout.color = in.color;\n"
	"\treturn out;\n"
	"}\n"
	"fragment float4 fragment_main(Out in [[stage_in]], constant float4 &tint [[buffer(0)]]) {\n"
	"\treturn in.color * tint;\n"
	"}\n";

/* a position (x, y) and a colour (RGBA8): 12 bytes a vertex */
struct scene_vertex
{
	float x, y;
	unsigned char color[4];
};

static const struct scene_vertex scene_vertices[] =
{
	/* the triangle */
	{-0.8f, -0.8f, {255, 0, 0, 255}},
	{0.8f, -0.8f, {0, 255, 0, 255}},
	{0.0f, 0.8f, {0, 0, 255, 255}},
	/* the quad, a strip */
	{-0.2f, -0.5f, {255, 255, 255, 128}},
	{0.9f, -0.5f, {255, 255, 255, 128}},
	{-0.2f, 0.4f, {255, 255, 255, 128}},
	{0.9f, 0.4f, {255, 255, 255, 128}},
};

static void scene_pipeline(struct opence_render_writer *writer, unsigned int id, int blended)
{
	unsigned int *fields = opence_render_command(writer, OPENCE_RENDER_OP_CREATE_PIPELINE,
		OPENCE_RENDER_CREATE_PIPELINE_WORDS + 2 * OPENCE_RENDER_PIPELINE_ATTRIBUTE_WORDS +
		OPENCE_RENDER_PIPELINE_LAYOUT_WORDS);

	CHECK(fields);
	fields[0] = id;
	fields[1] = 1;
	fields[2] = 1;
	fields[3] = OPENCE_RENDER_FORMAT_BGRA8;
	fields[4] = 0;
	fields[5] = 1;
	fields[6] = blended;
	fields[7] = blended ? OPENCE_RENDER_BLEND_SOURCE_ALPHA : OPENCE_RENDER_BLEND_ONE;
	fields[8] = blended ? OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_ALPHA : OPENCE_RENDER_BLEND_ZERO;
	fields[9] = OPENCE_RENDER_BLEND_ADD;
	fields[10] = OPENCE_RENDER_BLEND_ONE;
	fields[11] = blended ? OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_ALPHA : OPENCE_RENDER_BLEND_ZERO;
	fields[12] = OPENCE_RENDER_BLEND_ADD;
	fields[13] = OPENCE_RENDER_WRITE_RED | OPENCE_RENDER_WRITE_GREEN | OPENCE_RENDER_WRITE_BLUE |
		OPENCE_RENDER_WRITE_ALPHA;
	fields[14] = 2;
	fields[15] = 1;
	/* the attributes, from buffer slot 1 */
	fields[16] = 0;
	fields[17] = OPENCE_RENDER_VERTEX_FLOAT2;
	fields[18] = 0;
	fields[19] = 1;
	fields[20] = 1;
	fields[21] = OPENCE_RENDER_VERTEX_UCHAR4_NORMALIZED;
	fields[22] = 8;
	fields[23] = 1;
	/* and its layout */
	fields[24] = 1;
	fields[25] = sizeof(struct scene_vertex);
	fields[26] = OPENCE_RENDER_STEP_VERTEX;
	fields[27] = 1;
}

static void scene_draw(struct opence_render_writer *writer, unsigned int pipeline, unsigned int primitive,
	unsigned int first, unsigned int vertices)
{
	unsigned int *fields = opence_render_command(writer, OPENCE_RENDER_OP_SET_PIPELINE,
		OPENCE_RENDER_SET_PIPELINE_WORDS);

	CHECK(fields);
	fields[0] = pipeline;
	fields = opence_render_command(writer, OPENCE_RENDER_OP_DRAW, OPENCE_RENDER_DRAW_WORDS);
	CHECK(fields);
	fields[0] = primitive;
	fields[1] = first;
	fields[2] = vertices;
	fields[3] = 1;
}

static void scene(void)
{
	struct opence_render_writer writer;
	unsigned int *fields;

	opence_render_writer_init(&writer, buffer, sizeof(buffer));
	fields = opence_render_command(&writer, OPENCE_RENDER_OP_CREATE_LIBRARY, OPENCE_RENDER_CREATE_LIBRARY_WORDS);
	CHECK(fields);
	fields[0] = 1;
	fields[1] = (unsigned int)(unsigned long)scene_source;
	fields[2] = sizeof(scene_source) - 1;
	scene_pipeline(&writer, 1, 0);
	scene_pipeline(&writer, 2, 1);
	create_buffer(&writer, 1, sizeof(scene_vertices));
	upload_buffer(&writer, 1, scene_vertices, sizeof(scene_vertices));

	/* the window's pass, cleared to a dark blue */
	fields = opence_render_command(&writer, OPENCE_RENDER_OP_BEGIN_PASS, OPENCE_RENDER_BEGIN_PASS_WORDS);
	CHECK(fields);
	memset(fields, 0, (OPENCE_RENDER_BEGIN_PASS_WORDS - 1) * sizeof(*fields));
	fields[2] = OPENCE_RENDER_LOAD_CLEAR;
	fields[3] = OPENCE_RENDER_LOAD_DONT_CARE;
	fields[4] = OPENCE_RENDER_LOAD_DONT_CARE;
	fields[5] = opence_render_float(0.1f);
	fields[6] = opence_render_float(0.1f);
	fields[7] = opence_render_float(0.2f);
	fields[8] = opence_render_float(1.0f);
	fields[9] = opence_render_float(1.0f);
	fields = opence_render_command(&writer, OPENCE_RENDER_OP_SET_VERTEX_BUFFER,
		OPENCE_RENDER_SET_VERTEX_BUFFER_WORDS);
	CHECK(fields);
	fields[0] = 1;
	fields[1] = 1;
	fields[2] = 0;
	/* (the tint: white, so the vertices' own colours) */
	fields = opence_render_command(&writer, OPENCE_RENDER_OP_SET_BYTES, OPENCE_RENDER_SET_BYTES_WORDS + 4);
	CHECK(fields);
	fields[0] = OPENCE_RENDER_STAGE_FRAGMENT;
	fields[1] = 0;
	fields[2] = 16;
	fields[3] = fields[4] = fields[5] = fields[6] = opence_render_float(1.0f);
	scene_draw(&writer, 1, OPENCE_RENDER_PRIMITIVE_TRIANGLE, 0, 3);
	scene_draw(&writer, 2, OPENCE_RENDER_PRIMITIVE_TRIANGLE_STRIP, 3, 4);
	CHECK(opence_render_command(&writer, OPENCE_RENDER_OP_END_PASS, OPENCE_RENDER_END_PASS_WORDS));
	CHECK(opence_render_command(&writer, OPENCE_RENDER_OP_PRESENT, OPENCE_RENDER_PRESENT_WORDS));
	CHECK(submit(&writer) == 0);
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
	else if (!strcmp(argv[1], "scene"))
		scene();
	else
	{
		printf("no case %s\n", argv[1]);
		return 1;
	}
	printf("ok\n");
	return 0;
}
