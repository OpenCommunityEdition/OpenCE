/*
VM_TEXTURES.C

A guest program for tools/test_macos_textures.py: the macOS device's
textures and buffers (port/linux/src/xbox_textures.c and
port/macos/guest/d3d8_metal.c, built with HALO_METAL), run in the app on
texels the test also decodes itself. What each texture holds is drawn, a
level and a face or slice at a time, into a render target of RGBA8, read
back and printed; so are words of the contiguous window's mirror and of the
stream buffer. It then draws a cleared window (for the app's frame capture)
and prints "ok".

	vm_textures textures <case>...
		each case (hex, colon-separated) a format word, a size word, a
		seed for its texels, a seed for a palette (0 for none), a channel
		order (enum custom_edition_channel_order) and 1 to write the texels
		again with seed + 1 once read, and read them again
	vm_textures buffers
		the mirror's and the stream's words, and the mirror's refusals
	vm_textures targets
		render targets found again for a surface

Texels live in the contiguous window, mapped here as the game's platform
layer maps it (port/linux/src/platform.h); the memory watch is the stubs'
(vm_textures_stubs.c), whose generation the program moves on itself.
*/

#include "xgpu.h"
#include "opence_render.h"
#include "opence_render_writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define CHECK(condition) do { if (!(condition)) { printf("failed: %s (line %d)\n", #condition, __LINE__); exit(1); } } while (0)

/* the window's part this maps, and the room each case's texels get */
#define WINDOW_SIZE 0x1000000UL
#define CASE_SPACING 0x40000UL

/* the memory watch's state (vm_textures_stubs.c) */
extern unsigned long test_watch_generation;
extern unsigned long test_watch_serial;

static void map_window(void)
{
	void *window = mmap((void *)PLATFORM_CONTIGUOUS_BASE, WINDOW_SIZE, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

	CHECK(window == (void *)PLATFORM_CONTIGUOUS_BASE);
}

/* the texels' bytes: xorshift32's words, little-endian */
static void fill(unsigned char *bytes, unsigned long size, unsigned long seed)
{
	unsigned long state = seed ? seed : 1, index;

	for (index = 0; index < size; index += 4)
	{
		unsigned long word;
		int byte;

		state ^= (state << 13) & 0xffffffffUL;
		state ^= state >> 17;
		state ^= (state << 5) & 0xffffffffUL;
		word = state;
		for (byte = 0; byte < 4 && index + byte < size; byte++)
			bytes[index + byte] = (unsigned char)(word >> (8 * byte));
	}
}

/* ---------- reading back */

/* a full-screen triangle whose fragments read the texel under them: of a
2D texture, a cube's face or a volume's slice, at the level given */
#define READ_SOURCE(texture, read) \
	"#include <metal_stdlib>\n" \
	"using namespace metal;\n" \
	"struct Out { float4 position [[position]]; };\n" \
	"vertex Out vertex_main(uint id [[vertex_id]]) {\n" \
	"\tOut out;\n" \
	"\tout.position = float4(float2((id << 1) & 2, id & 2) * 2.0 - 1.0, 0, 1);\n" \
	"\treturn out;\n" \
	"}\n" \
	"fragment float4 fragment_main(Out in [[stage_in]], " texture " t [[texture(0)]],\n" \
	"\tconstant uint4 &at [[buffer(0)]]) {\n" \
	"\tuint2 xy = uint2(in.position.xy);\n" \
	"\treturn " read ";\n" \
	"}\n"

static const char read_2d[] = READ_SOURCE("texture2d<float>", "t.read(xy, at.x)");
static const char read_cube[] = READ_SOURCE("texturecube<float>", "t.read(xy, at.y, at.x)");
static const char read_3d[] = READ_SOURCE("texture3d<float>", "t.read(uint3(xy, at.y), at.x)");

/* words of a buffer drawn as points, one a pixel of a row, each word's
bytes the pixel's red, green, blue and alpha */
static const char read_words[] =
	"#include <metal_stdlib>\n"
	"using namespace metal;\n"
	"struct Out { float4 position [[position]]; float4 color; float size [[point_size]]; };\n"
	"vertex Out vertex_main(uint id [[vertex_id]], device const uint *words [[buffer(0)]],\n"
	"\tconstant uint4 &count [[buffer(1)]]) {\n"
	"\tOut out;\n"
	"\tout.position = float4((float(id) + 0.5) / float(count.x) * 2.0 - 1.0, 0, 0, 1);\n"
	"\tout.color = unpack_unorm4x8_to_float(words[id]);\n"
	"\tout.size = 1;\n"
	"\treturn out;\n"
	"}\n"
	"fragment float4 fragment_main(Out in [[stage_in]]) {\n"
	"\treturn in.color;\n"
	"}\n";

enum
{
	_reader_2d,
	_reader_cube,
	_reader_3d,
	_reader_words,
	NUMBER_OF_READERS
};

static unsigned int reader_pipelines[NUMBER_OF_READERS];

static void make_readers(void)
{
	static const char *const sources[NUMBER_OF_READERS] = { read_2d, read_cube, read_3d, read_words };
	int reader;

	for (reader = 0; reader < NUMBER_OF_READERS; reader++)
	{
		unsigned int library = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_LIBRARY);
		unsigned int pipeline = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_PIPELINE);
		unsigned int *fields;

		CHECK(library && pipeline);
		fields = xgpu_metal_command(OPENCE_RENDER_OP_CREATE_LIBRARY, OPENCE_RENDER_CREATE_LIBRARY_WORDS);
		CHECK(fields);
		fields[0] = library;
		fields[1] = (unsigned int)(unsigned long)sources[reader];
		fields[2] = (unsigned int)strlen(sources[reader]);
		fields = xgpu_metal_command(OPENCE_RENDER_OP_CREATE_PIPELINE, OPENCE_RENDER_CREATE_PIPELINE_WORDS);
		CHECK(fields);
		memset(fields, 0, (OPENCE_RENDER_CREATE_PIPELINE_WORDS - 1) * sizeof(*fields));
		fields[0] = pipeline;
		fields[1] = library;
		fields[2] = library;
		fields[3] = OPENCE_RENDER_FORMAT_RGBA8;
		fields[5] = 1;
		fields[7] = OPENCE_RENDER_BLEND_ONE;
		fields[8] = OPENCE_RENDER_BLEND_ZERO;
		fields[9] = OPENCE_RENDER_BLEND_ADD;
		fields[10] = OPENCE_RENDER_BLEND_ONE;
		fields[11] = OPENCE_RENDER_BLEND_ZERO;
		fields[12] = OPENCE_RENDER_BLEND_ADD;
		fields[13] = OPENCE_RENDER_WRITE_RED | OPENCE_RENDER_WRITE_GREEN | OPENCE_RENDER_WRITE_BLUE |
			OPENCE_RENDER_WRITE_ALPHA;
		reader_pipelines[reader] = pipeline;
	}
}

static void set_bytes(unsigned int stage, unsigned int slot, unsigned int x, unsigned int y)
{
	unsigned int *fields = xgpu_metal_command(OPENCE_RENDER_OP_SET_BYTES, OPENCE_RENDER_SET_BYTES_WORDS + 4);

	CHECK(fields);
	fields[0] = stage;
	fields[1] = slot;
	fields[2] = 16;
	fields[3] = x;
	fields[4] = y;
	fields[5] = 0;
	fields[6] = 0;
}

/* draws with a reader into a new width x height target, and prints what
it holds, a line headed by name */
static void read_back(const char *name, int reader, unsigned int texture, unsigned int at_x, unsigned int at_y,
	unsigned int buffer, unsigned int buffer_offset, unsigned int width, unsigned int height)
{
	unsigned int target = xgpu_metal_new_id(OPENCE_RENDER_OBJECT_TEXTURE);
	unsigned int length = width * height * 4, index;
	unsigned char *pixels = malloc(length);
	unsigned int *fields;

	CHECK(target && pixels);
	memset(pixels, 0xcd, length);
	fields = xgpu_metal_command(OPENCE_RENDER_OP_CREATE_TEXTURE, OPENCE_RENDER_CREATE_TEXTURE_WORDS);
	CHECK(fields);
	fields[0] = target;
	fields[1] = OPENCE_RENDER_TEXTURE_2D;
	fields[2] = OPENCE_RENDER_FORMAT_RGBA8;
	fields[3] = width;
	fields[4] = height;
	fields[5] = 1;
	fields[6] = 1;
	fields[7] = OPENCE_RENDER_USAGE_SAMPLED | OPENCE_RENDER_USAGE_RENDER_TARGET;
	fields[8] = 1;

	fields = xgpu_metal_command(OPENCE_RENDER_OP_BEGIN_PASS, OPENCE_RENDER_BEGIN_PASS_WORDS);
	CHECK(fields);
	memset(fields, 0, (OPENCE_RENDER_BEGIN_PASS_WORDS - 1) * sizeof(*fields));
	fields[0] = target;
	fields[2] = OPENCE_RENDER_LOAD_CLEAR;
	fields[3] = OPENCE_RENDER_LOAD_DONT_CARE;
	fields[4] = OPENCE_RENDER_LOAD_DONT_CARE;
	fields[9] = opence_render_float(1.0f);
	fields = xgpu_metal_command(OPENCE_RENDER_OP_SET_PIPELINE, OPENCE_RENDER_SET_PIPELINE_WORDS);
	CHECK(fields);
	fields[0] = reader_pipelines[reader];
	if (reader == _reader_words)
	{
		fields = xgpu_metal_command(OPENCE_RENDER_OP_SET_VERTEX_BUFFER, OPENCE_RENDER_SET_VERTEX_BUFFER_WORDS);
		CHECK(fields);
		fields[0] = 0;
		fields[1] = buffer;
		fields[2] = buffer_offset;
		set_bytes(OPENCE_RENDER_STAGE_VERTEX, 1, width, 0);
	}
	else
	{
		fields = xgpu_metal_command(OPENCE_RENDER_OP_SET_TEXTURE, OPENCE_RENDER_SET_TEXTURE_WORDS);
		CHECK(fields);
		fields[0] = OPENCE_RENDER_STAGE_FRAGMENT;
		fields[1] = 0;
		fields[2] = texture;
		set_bytes(OPENCE_RENDER_STAGE_FRAGMENT, 0, at_x, at_y);
	}
	fields = xgpu_metal_command(OPENCE_RENDER_OP_DRAW, OPENCE_RENDER_DRAW_WORDS);
	CHECK(fields);
	fields[0] = reader == _reader_words ? OPENCE_RENDER_PRIMITIVE_POINT : OPENCE_RENDER_PRIMITIVE_TRIANGLE;
	fields[1] = 0;
	fields[2] = reader == _reader_words ? width : 3;
	fields[3] = 1;
	CHECK(xgpu_metal_command(OPENCE_RENDER_OP_END_PASS, OPENCE_RENDER_END_PASS_WORDS));

	fields = xgpu_metal_command(OPENCE_RENDER_OP_READ_TEXTURE, OPENCE_RENDER_READ_TEXTURE_WORDS);
	CHECK(fields);
	fields[0] = target;
	fields[1] = 0;
	fields[2] = 0;
	fields[3] = 0;
	fields[4] = 0;
	fields[5] = width;
	fields[6] = height;
	fields[7] = (unsigned int)(unsigned long)pixels;
	fields[8] = width * 4;
	fields[9] = length;
	xgpu_metal_destroy(OPENCE_RENDER_OBJECT_TEXTURE, target);
	xgpu_metal_flush();

	printf("%s %ux%u ", name, width, height);
	for (index = 0; index < length; index++)
		printf("%02x", pixels[index]);
	printf("\n");
	free(pixels);
}

/* ---------- textures */

static unsigned long level_side(unsigned long side, unsigned long level)
{
	return side >> level ? side >> level : 1;
}

/* every face, level and slice of the texture of this resource */
static void read_texture(int number, const char *pass, const DWORD *resource, const D3DCOLOR *palette)
{
	struct xgpu_texture_description description;
	GLenum target;
	unsigned int texture = xgpu_texture_get(resource, palette, &target, &description);
	unsigned long faces = description.cube_map ? 6 : 1, face, level, slice;
	int reader = description.cube_map ? _reader_cube : description.depth > 1 ? _reader_3d : _reader_2d;

	CHECK(texture);
	CHECK(target == (GLenum)(description.cube_map ? GL_TEXTURE_CUBE_MAP :
		description.depth > 1 ? GL_TEXTURE_3D : GL_TEXTURE_2D));
	for (face = 0; face < faces; face++)
	{
		for (level = 0; level < description.levels; level++)
		{
			for (slice = 0; slice < level_side(description.depth, level); slice++)
			{
				char name[64];

				sprintf(name, "texture %d %s face %lu level %lu slice %lu", number, pass, face, level, slice);
				read_back(name, reader, texture, (unsigned int)level, (unsigned int)(face + slice), 0, 0,
					(unsigned int)level_side(description.width, level),
					(unsigned int)level_side(description.height, level));
			}
		}
	}
}

static void textures(int count, char **cases)
{
	int number;

	for (number = 0; number < count; number++)
	{
		unsigned long format_word, size_word, seed, palette_seed, order, rewrite;
		struct xgpu_texture_description description;
		unsigned long data = CASE_SPACING * (unsigned long)(number + 1);
		unsigned char *texels = PLATFORM_PHYSICAL_TO_VIRTUAL(data);
		D3DCOLOR palette[256];
		DWORD resource[5];
		unsigned long size;

		CHECK(sscanf(cases[number], "%lx:%lx:%lx:%lx:%lx:%lx", &format_word, &size_word, &seed, &palette_seed,
			&order, &rewrite) == 6);
		xgpu_texture_describe(format_word, size_word, &description);
		size = xgpu_texture_face_size(&description) * (description.cube_map ? 6 : 1);
		CHECK(size <= CASE_SPACING);
		fill(texels, size, seed);
		fill((unsigned char *)palette, sizeof(palette), palette_seed);
		if (order)
			halo_custom_edition_texels_channels(texels, (unsigned char)order);
		memset(resource, 0, sizeof(resource));
		resource[1] = data;
		resource[3] = format_word;
		resource[4] = size_word;
		/* (the texture is new: its pages have not been written) */
		test_watch_serial++;
		read_texture(number, "first", resource, palette_seed ? palette : NULL);
		/* found again unchanged, it is not uploaded again */
		read_texture(number, "again", resource, palette_seed ? palette : NULL);
		if (rewrite)
		{
			fill(texels, size, seed + 1);
			test_watch_generation++;
			test_watch_serial++;
			read_texture(number, "rewritten", resource, palette_seed ? palette : NULL);
		}
	}
}

/* ---------- the window's mirror and the stream buffer */

#define WORDS 16

static void buffers(void)
{
	unsigned long *words = PLATFORM_PHYSICAL_TO_VIRTUAL(0x123000UL + 0x40);
	unsigned long stream_words[WORDS];
	unsigned int buffer, other;
	unsigned long offset, generation, index;
	int frame;

	/* what is not in the window, or is not in one segment of it */
	CHECK(!xgpu_metal_mirror_range(PLATFORM_CONTIGUOUS_BASE - 0x1000, 0x100, &buffer, &offset, NULL));
	CHECK(!xgpu_metal_mirror_range(PLATFORM_CONTIGUOUS_BASE + 0x100, 0, &buffer, &offset, NULL));
	CHECK(!xgpu_metal_mirror_range(PLATFORM_CONTIGUOUS_BASE + PLATFORM_CONTIGUOUS_SIZE - 0x10, 0x20, &buffer,
		&offset, NULL));
	CHECK(!xgpu_metal_mirror_range(PLATFORM_CONTIGUOUS_BASE + 0x3ffff0, 0x20, &buffer, &offset, NULL));

	for (index = 0; index < WORDS; index++)
	{
		words[index] = 0x01020304UL * (index + 1) ^ 0x80706050UL;
		stream_words[index] = 0x11223344UL + index * 0x01010101UL;
	}
	test_watch_generation = 7;
	CHECK(xgpu_metal_mirror_range((unsigned long)words, sizeof(*words) * WORDS, &buffer, &offset, &generation));
	printf("mirror offset 0x%lx generation %lu\n", offset, generation);
	CHECK(offset == 0x123040UL && generation == 7);
	read_back("mirror", _reader_words, 0, 0, 0, buffer, (unsigned int)offset, WORDS, 1);

	/* the next segment's own buffer */
	CHECK(xgpu_metal_mirror_range(PLATFORM_CONTIGUOUS_BASE + 0x400000UL, 16, &other, &offset, NULL));
	CHECK(other != buffer && offset == 0);

	/* written in consecutive frames: uploaded again three times, then
	refused (volatile) */
	for (frame = 1; frame <= 5; frame++)
	{
		BOOL mirrored;

		xgpu_metal_begin_frame();
		words[0] = 0xa0b0c0d0UL + (unsigned long)frame;
		test_watch_generation++;
		mirrored = xgpu_metal_mirror_range((unsigned long)words, sizeof(*words) * WORDS, &buffer, &offset, &generation);
		printf("rewrite %d %s\n", frame, mirrored ? "mirrored" : "refused");
		if (frame == 1)
			read_back("mirror rewritten", _reader_words, 0, 0, 0, buffer, (unsigned int)offset, WORDS, 1);
	}

	CHECK(xgpu_metal_stream(stream_words, sizeof(stream_words), 256, &buffer, &offset));
	CHECK(offset % 256 == 0);
	read_back("stream", _reader_words, 0, 0, 0, buffer, (unsigned int)offset, WORDS, 1);
	CHECK(xgpu_metal_stream(stream_words, 4, 256, &buffer, &offset));
	CHECK(offset == 256);
	CHECK(!xgpu_metal_stream(stream_words, 0, 4, &buffer, &offset));
}

/* ---------- render targets */

static void targets(void)
{
	/* a linear A8R8G8B8 640x480 surface and its depth (LIN_D24S8) */
	D3DSurface color = { 0, 0x200000, 0, 0x12 << 8, 639 | (479UL << 12) | ((640UL * 4 / 64 - 1) << 24), NULL };
	D3DSurface depth = { 0, 0x200000, 0, 0x2e << 8, 639 | (479UL << 12) | ((640UL * 4 / 64 - 1) << 24), NULL };
	D3DSurface smaller = color;
	struct xgpu_render_target *target, *again, *depth_target, *smaller_target;

	smaller.Size = 319 | (239UL << 12) | ((320UL * 4 / 64 - 1) << 24);
	target = xgpu_metal_render_target_get(&color);
	again = xgpu_metal_render_target_get(&color);
	depth_target = xgpu_metal_render_target_get(&depth);
	smaller_target = xgpu_metal_render_target_get(&smaller);
	CHECK(target && target == again && target->texture);
	CHECK(target->width == 640 && target->height == 480 && !target->depth);
	CHECK(target->scale[0] == 1.0f && target->gl_width == 640 && target->gl_height == 480);
	CHECK(depth_target && depth_target != target && depth_target->depth && depth_target->texture != target->texture);
	CHECK(smaller_target && smaller_target != target && smaller_target->width == 320);
	CHECK(!xgpu_metal_render_target_get(NULL));
	printf("targets %u %u %u\n", target->texture, depth_target->texture, smaller_target->texture);
}

/* ---------- the window, for the app's capture */

static void present(void)
{
	unsigned int *fields = xgpu_metal_command(OPENCE_RENDER_OP_BEGIN_PASS, OPENCE_RENDER_BEGIN_PASS_WORDS);

	CHECK(fields);
	memset(fields, 0, (OPENCE_RENDER_BEGIN_PASS_WORDS - 1) * sizeof(*fields));
	fields[2] = OPENCE_RENDER_LOAD_CLEAR;
	fields[3] = OPENCE_RENDER_LOAD_DONT_CARE;
	fields[4] = OPENCE_RENDER_LOAD_DONT_CARE;
	fields[8] = opence_render_float(1.0f);
	fields[9] = opence_render_float(1.0f);
	CHECK(xgpu_metal_command(OPENCE_RENDER_OP_END_PASS, OPENCE_RENDER_END_PASS_WORDS));
	CHECK(xgpu_metal_command(OPENCE_RENDER_OP_PRESENT, OPENCE_RENDER_PRESENT_WORDS));
	xgpu_metal_flush();
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("usage: vm_textures textures|buffers|targets ...\n");
		return 1;
	}
	map_window();
	make_readers();
	if (!strcmp(argv[1], "textures"))
		textures(argc - 2, argv + 2);
	else if (!strcmp(argv[1], "buffers"))
		buffers();
	else if (!strcmp(argv[1], "targets"))
		targets();
	else
	{
		printf("no case %s\n", argv[1]);
		return 1;
	}
	present();
	printf("ok\n");
	return 0;
}
