/*
GEOMETRY_BYTES.C (test)

The real geometry_bytes_add (custom_edition_geometry.c), which counts the room
a Custom Edition map's model parts and structure BSP materials take before it
is allocated. Parts may all name the same vertices, so a hostile map can name
far more than its file holds: the count must stop at MAXIMUM_GEOMETRY_BYTES
rather than wrap. test_geometry_bytes.py takes the limit (config.inc) and the
code (under_test.inc).
*/

#include "harness.h"
#include "config.inc"

typedef unsigned short word;

#include "under_test.inc"

/* the compressed model vertex (rasterizer_geometry_get_vertex_size) */
enum { MODEL_VERTEX_BYTES = 32 };

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	unsigned long bytes = 0;
	long part;

	/* a real map's parts: counted exactly */
	CASE("counts-real-parts")
	{
		for (part = 0; part < 1000; part++)
			CHECK(geometry_bytes_add(&bytes, 300, MODEL_VERTEX_BYTES, 600), "part %ld of a real map refused", part);
		CHECK(bytes == 1000UL * (300 * MODEL_VERTEX_BYTES + 600 * sizeof(word)), "counted 0x%lX bytes", bytes);
		return 0;
	}
	/* parts of 65535 vertices each, all naming one block: refused before the
	count passes the limit, never wrapping to a small size */
	CASE("refuses-before-wrapping")
	{
		for (part = 0; part < 4096 && geometry_bytes_add(&bytes, 65535, MODEL_VERTEX_BYTES, 65537); part++)
			;
		CHECK(part < 4096, "4096 parts of 65535 vertices were all counted (0x%lX bytes)", bytes);
		CHECK(bytes <= MAXIMUM_GEOMETRY_BYTES, "counted 0x%lX bytes, past the most 0x%X", bytes, MAXIMUM_GEOMETRY_BYTES);
		CHECK(bytes > MAXIMUM_GEOMETRY_BYTES - 65535UL * MODEL_VERTEX_BYTES - 65537UL * sizeof(word),
			"stopped at 0x%lX bytes, well short of the most", bytes);
		CHECK(!geometry_bytes_add(&bytes, 1, MODEL_VERTEX_BYTES, 0) || bytes <= MAXIMUM_GEOMETRY_BYTES,
			"counted past the most");
		return 0;
	}
	/* counts that alone would wrap the multiplication are refused */
	CASE("refuses-huge-counts")
	{
		CHECK(!geometry_bytes_add(&bytes, 0x08000001UL, MODEL_VERTEX_BYTES, 0), "a vertex count that wraps was counted");
		CHECK(!geometry_bytes_add(&bytes, 0, MODEL_VERTEX_BYTES, 0x80000001UL), "an index count that wraps was counted");
		CHECK(bytes == 0, "a refused part changed the count to 0x%lX", bytes);
		return 0;
	}
	/* exactly the most fits */
	CASE("takes-exactly-the-most")
	{
		CHECK(geometry_bytes_add(&bytes, MAXIMUM_GEOMETRY_BYTES / MODEL_VERTEX_BYTES, MODEL_VERTEX_BYTES, 0),
			"exactly the most was refused");
		CHECK(!geometry_bytes_add(&bytes, 0, MODEL_VERTEX_BYTES, 1), "one index past the most was counted");
		return 0;
	}
	fprintf(stderr, "unknown case '%s'\n", case_name);
	return 2;
}
