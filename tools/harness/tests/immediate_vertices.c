/* The real immediate mode's vertices (d3d8_gl.c): D3DDevice_Begin, the
register writes and the first vertex's registers at End, against a vertex of
all sixteen registers as they stood when register 0 was written. */

#include "harness.h"

#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define D3DVSDE_VERTEX (-1)
#define WINAPI
typedef int BOOL;
typedef int INT;
typedef float FLOAT;
typedef int D3DPRIMITIVETYPE;

static struct
{
	float attributes[XGPU_VERTEX_ATTRIBUTE_COUNT][4];
	BOOL immediate_active;
	D3DPRIMITIVETYPE immediate_type;
	unsigned long immediate_written;
	unsigned char immediate_registers[XGPU_VERTEX_ATTRIBUTE_COUNT];
	unsigned long immediate_register_count;
	float *immediate_vertices;
	unsigned long immediate_count;
	unsigned long immediate_capacity;
} device;

#include "under_test.inc"

#define FLOATS (XGPU_VERTEX_ATTRIBUTE_COUNT * 4)
#define MAXIMUM_VERTICES 64
#define SENTINEL 12345.0f

/* every register of every vertex as it stood when its register 0 was written */
static float expected[MAXIMUM_VERTICES][FLOATS];
static unsigned long expected_count;
static unsigned long expected_written;

static void begin(void)
{
	unsigned long index;

	/* (the vertices' room in place, holding what no register holds) */
	if (!device.immediate_vertices)
	{
		device.immediate_vertices = malloc(MAXIMUM_VERTICES * FLOATS * sizeof(float));
		device.immediate_capacity = MAXIMUM_VERTICES;
	}
	for (index = 0; index < MAXIMUM_VERTICES * FLOATS; index++)
		device.immediate_vertices[index] = SENTINEL;
	D3DDevice_Begin(1);
	expected_count = 0;
	expected_written = 0;
}

static void write(INT reg, float a, float b, float c, float d)
{
	D3DDevice_SetVertexData4f(reg, a, b, c, d);
	if (reg == D3DVSDE_VERTEX)
		reg = 0;
	expected_written |= 1UL << reg;
	if (!reg)
		memcpy(expected[expected_count++], device.attributes, sizeof(expected[0]));
}

/* the vertices' written registers as expected, and the first vertex's others */
static void end(void)
{
	unsigned long vertex, reg, written = 0;

	device.immediate_active = FALSE;
	immediate_end_unwritten();
	CHECK(device.immediate_count == expected_count, "%lu vertices, not %lu", device.immediate_count, expected_count);
	CHECK(device.immediate_written == expected_written, "registers %lx written, not %lx", device.immediate_written,
		expected_written);
	for (reg = 0; reg < device.immediate_register_count && reg < XGPU_VERTEX_ATTRIBUTE_COUNT; reg++)
		written |= 1UL << device.immediate_registers[reg];
	CHECK(device.immediate_register_count <= XGPU_VERTEX_ATTRIBUTE_COUNT && written == expected_written,
		"%lu registers listed (%lx), not those written (%lx)", device.immediate_register_count, written,
		expected_written);
	for (vertex = 0; vertex < expected_count; vertex++)
	{
		for (reg = 0; reg < XGPU_VERTEX_ATTRIBUTE_COUNT; reg++)
		{
			const float *given = device.immediate_vertices + vertex * FLOATS + reg * 4;

			if (vertex && !(expected_written & (1UL << reg)))
				continue;
			CHECK(!memcmp(given, expected[vertex] + reg * 4, 4 * sizeof(float)),
				"vertex %lu register %lu: %g %g %g %g, not %g %g %g %g", vertex, reg, given[0], given[1], given[2],
				given[3], expected[vertex][reg * 4], expected[vertex][reg * 4 + 1], expected[vertex][reg * 4 + 2],
				expected[vertex][reg * 4 + 3]);
		}
	}
}

static float value(void)
{
	return (float)(rand() % 1000) / 8.0f;
}

int main(int argc, char **argv)
{
	char const *case_name = argc == 2 ? argv[1] : "";
	unsigned long reg, draw, vertex;

	srand(1);
	for (reg = 0; reg < XGPU_VERTEX_ATTRIBUTE_COUNT; reg++)
	{
		device.attributes[reg][0] = (float)reg;
		device.attributes[reg][3] = 1.0f;
	}
	/* (a decal's: a color and a texture coordinate, then the position) */
	CASE("decal")
	{
		begin();
		for (vertex = 0; vertex < 12; vertex++)
		{
			write(3, value(), value(), value(), 1.0f);
			write(9, value(), value(), 0.0f, 1.0f);
			write(D3DVSDE_VERTEX, value(), value(), value(), 1.0f);
		}
		end();
	}
	/* registers first written after vertices that took their values from before */
	else CASE("late-register")
	{
		write(5, 7.0f, 8.0f, 9.0f, 1.0f);
		begin();
		for (vertex = 0; vertex < 10; vertex++)
		{
			if (vertex >= 4)
				write(5, value(), value(), value(), value());
			if (vertex >= 7)
				write(11, value(), value(), value(), value());
			write(0, value(), value(), value(), 1.0f);
		}
		end();
	}
	/* a register written only once, before the first vertex */
	else CASE("written-once")
	{
		begin();
		write(2, 1.0f, 2.0f, 3.0f, 4.0f);
		for (vertex = 0; vertex < 6; vertex++)
			write(0, value(), value(), value(), 1.0f);
		end();
	}
	/* the first vertex's registers not written, for the trace */
	else CASE("unwritten")
	{
		write(13, 4.0f, 3.0f, 2.0f, 1.0f);
		begin();
		write(0, 1.0f, 1.0f, 1.0f, 1.0f);
		write(0, 2.0f, 2.0f, 2.0f, 1.0f);
		end();
	}
	/* many draws, each of its own registers */
	else CASE("draws")
	{
		for (draw = 0; draw < 200; draw++)
		{
			unsigned long count = 1 + rand() % 40;

			begin();
			for (vertex = 0; vertex < count; vertex++)
			{
				unsigned long writes = rand() % 4;

				while (writes--)
					write((INT)(1 + rand() % 15), value(), value(), value(), value());
				write(rand() % 2 ? 0 : D3DVSDE_VERTEX, value(), value(), value(), 1.0f);
			}
			end();
		}
	}
	/* all sixteen registers: End uploads the vertices as they are */
	else CASE("all-registers")
	{
		begin();
		for (vertex = 0; vertex < 8; vertex++)
		{
			for (reg = XGPU_VERTEX_ATTRIBUTE_COUNT - 1; reg > 0; reg--)
			{
				if (vertex >= reg % 5)
					write((INT)reg, value(), value(), value(), value());
			}
			write(0, value(), value(), value(), 1.0f);
		}
		end();
		for (vertex = 0; vertex < expected_count; vertex++)
		{
			CHECK(!memcmp(device.immediate_vertices + vertex * FLOATS, expected[vertex], sizeof(expected[0])),
				"vertex %lu not all its registers", vertex);
		}
	}
	else
		return 2;
	return 0;
}
