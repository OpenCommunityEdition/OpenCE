/*
METAL_STATE.C (test)

The Metal device's real translations of Direct3D 8 state into the render
ring's values (port/macos/guest/d3d8_metal_device.c): compare functions,
blending, stencil operations, sampler addressing and filters, border
colours, culling, write masks, and the primitives Metal has no equal of
drawn through indices. They are the pipeline's and the depth-stencil
state's keys, so a wrong one draws the wrong thing everywhere.
test_metal_state.py takes the enums (xdk_pdb.h), the ring's values
(opence_render.h) and the code (under_test.inc).
*/

#include "harness.h"
#include "opence_render.h"
#include "config.inc"

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef DWORD D3DCOLOR;
typedef int BOOL;
typedef enum _D3DPRIMITIVETYPE D3DPRIMITIVETYPE;
#define TRUE 1
#define FALSE 0

#include "under_test.inc"

static int indices_are(const WORD *got, unsigned long count, const WORD *expected, unsigned long expected_count)
{
	unsigned long index;

	if (count != expected_count)
		return 0;
	for (index = 0; index < count; index++)
		if (got[index] != expected[index])
			return 0;
	return 1;
}

int main(int argc, char **argv)
{
	const char *case_name = argc > 1 ? argv[1] : "";
	unsigned int cull, winding;
	BOOL flag;

	/* each compare function in order; 0 (D3DCMP_NEVER's low bits) is never,
	and a value outside them always */
	CASE("compare")
	{
		static const DWORD functions[] = { D3DCMP_NEVER, D3DCMP_LESS, D3DCMP_EQUAL, D3DCMP_LESSEQUAL,
			D3DCMP_GREATER, D3DCMP_NOTEQUAL, D3DCMP_GREATEREQUAL, D3DCMP_ALWAYS };
		static const unsigned int expected[] = { OPENCE_RENDER_COMPARE_NEVER, OPENCE_RENDER_COMPARE_LESS,
			OPENCE_RENDER_COMPARE_EQUAL, OPENCE_RENDER_COMPARE_LESS_EQUAL, OPENCE_RENDER_COMPARE_GREATER,
			OPENCE_RENDER_COMPARE_NOT_EQUAL, OPENCE_RENDER_COMPARE_GREATER_EQUAL, OPENCE_RENDER_COMPARE_ALWAYS };
		unsigned long index;

		for (index = 0; index < sizeof(functions) / sizeof(*functions); index++)
			CHECK(ring_compare(functions[index]) == expected[index], "compare 0x%lX is %u", functions[index],
				ring_compare(functions[index]));
		CHECK(ring_compare(0) == OPENCE_RENDER_COMPARE_NEVER, "0 is %u", ring_compare(0));
		CHECK(ring_compare(0x1234) == OPENCE_RENDER_COMPARE_ALWAYS, "an unknown compare is %u", ring_compare(0x1234));
		return 0;
	}
	CASE("blend")
	{
		static const DWORD factors[] = { D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCCOLOR, D3DBLEND_INVSRCCOLOR,
			D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA, D3DBLEND_DESTCOLOR,
			D3DBLEND_INVDESTCOLOR, D3DBLEND_SRCALPHASAT, D3DBLEND_CONSTANTCOLOR, D3DBLEND_INVCONSTANTCOLOR,
			D3DBLEND_CONSTANTALPHA, D3DBLEND_INVCONSTANTALPHA, 0x4321 };
		static const unsigned int expected[] = { OPENCE_RENDER_BLEND_ZERO, OPENCE_RENDER_BLEND_ONE,
			OPENCE_RENDER_BLEND_SOURCE_COLOR, OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_COLOR,
			OPENCE_RENDER_BLEND_SOURCE_ALPHA, OPENCE_RENDER_BLEND_ONE_MINUS_SOURCE_ALPHA,
			OPENCE_RENDER_BLEND_DESTINATION_ALPHA, OPENCE_RENDER_BLEND_ONE_MINUS_DESTINATION_ALPHA,
			OPENCE_RENDER_BLEND_DESTINATION_COLOR, OPENCE_RENDER_BLEND_ONE_MINUS_DESTINATION_COLOR,
			OPENCE_RENDER_BLEND_SOURCE_ALPHA_SATURATED, OPENCE_RENDER_BLEND_BLEND_COLOR,
			OPENCE_RENDER_BLEND_ONE_MINUS_BLEND_COLOR, OPENCE_RENDER_BLEND_BLEND_ALPHA,
			OPENCE_RENDER_BLEND_ONE_MINUS_BLEND_ALPHA, OPENCE_RENDER_BLEND_ONE };
		unsigned long index;

		for (index = 0; index < sizeof(factors) / sizeof(*factors); index++)
			CHECK(ring_blend_factor(factors[index]) == expected[index], "factor 0x%lX is %u", factors[index],
				ring_blend_factor(factors[index]));
		/* the signed operations are drawn as their unsigned ones */
		CHECK(ring_blend_operation(D3DBLENDOP_ADD) == OPENCE_RENDER_BLEND_ADD, "add");
		CHECK(ring_blend_operation(D3DBLENDOP_ADDSIGNED) == OPENCE_RENDER_BLEND_ADD, "signed add");
		CHECK(ring_blend_operation(D3DBLENDOP_SUBTRACT) == OPENCE_RENDER_BLEND_SUBTRACT, "subtract");
		CHECK(ring_blend_operation(D3DBLENDOP_REVSUBTRACT) == OPENCE_RENDER_BLEND_REVERSE_SUBTRACT, "reverse");
		CHECK(ring_blend_operation(D3DBLENDOP_REVSUBTRACTSIGNED) == OPENCE_RENDER_BLEND_REVERSE_SUBTRACT,
			"signed reverse");
		CHECK(ring_blend_operation(D3DBLENDOP_MIN) == OPENCE_RENDER_BLEND_MIN, "min");
		CHECK(ring_blend_operation(D3DBLENDOP_MAX) == OPENCE_RENDER_BLEND_MAX, "max");
		return 0;
	}
	CASE("stencil")
	{
		CHECK(ring_stencil_operation(D3DSTENCILOP_KEEP) == OPENCE_RENDER_STENCIL_KEEP, "keep");
		CHECK(ring_stencil_operation(D3DSTENCILOP_ZERO) == OPENCE_RENDER_STENCIL_ZERO, "zero");
		CHECK(ring_stencil_operation(D3DSTENCILOP_REPLACE) == OPENCE_RENDER_STENCIL_REPLACE, "replace");
		CHECK(ring_stencil_operation(D3DSTENCILOP_INCRSAT) == OPENCE_RENDER_STENCIL_INCREMENT_CLAMP, "incrsat");
		CHECK(ring_stencil_operation(D3DSTENCILOP_DECRSAT) == OPENCE_RENDER_STENCIL_DECREMENT_CLAMP, "decrsat");
		CHECK(ring_stencil_operation(D3DSTENCILOP_INVERT) == OPENCE_RENDER_STENCIL_INVERT, "invert");
		CHECK(ring_stencil_operation(D3DSTENCILOP_INCR) == OPENCE_RENDER_STENCIL_INCREMENT_WRAP, "incr");
		CHECK(ring_stencil_operation(D3DSTENCILOP_DECR) == OPENCE_RENDER_STENCIL_DECREMENT_WRAP, "decr");
		return 0;
	}
	/* addressing, filters and border colours: the sampler's key */
	CASE("sampler")
	{
		CHECK(ring_address(D3DTADDRESS_WRAP) == OPENCE_RENDER_ADDRESS_REPEAT, "wrap");
		CHECK(ring_address(D3DTADDRESS_MIRROR) == OPENCE_RENDER_ADDRESS_MIRROR, "mirror");
		CHECK(ring_address(D3DTADDRESS_CLAMP) == OPENCE_RENDER_ADDRESS_CLAMP_TO_EDGE, "clamp");
		CHECK(ring_address(D3DTADDRESS_CLAMPTOEDGE) == OPENCE_RENDER_ADDRESS_CLAMP_TO_EDGE, "clamp to edge");
		CHECK(ring_address(D3DTADDRESS_BORDER) == OPENCE_RENDER_ADDRESS_CLAMP_TO_BORDER, "border");
		CHECK(ring_filter(D3DTEXF_NONE, TRUE) == OPENCE_RENDER_FILTER_NONE, "no mip filter reads no mipmaps");
		CHECK(ring_filter(D3DTEXF_NONE, FALSE) == OPENCE_RENDER_FILTER_NEAREST, "no filter is nearest");
		CHECK(ring_filter(D3DTEXF_POINT, TRUE) == OPENCE_RENDER_FILTER_NEAREST, "point");
		CHECK(ring_filter(D3DTEXF_LINEAR, FALSE) == OPENCE_RENDER_FILTER_LINEAR, "linear");
		CHECK(ring_filter(D3DTEXF_ANISOTROPIC, FALSE) == OPENCE_RENDER_FILTER_LINEAR, "anisotropic");
		CHECK(ring_border(0x00ffffff) == OPENCE_RENDER_BORDER_TRANSPARENT_BLACK, "transparent");
		CHECK(ring_border(0xffffffff) == OPENCE_RENDER_BORDER_OPAQUE_WHITE, "white");
		CHECK(ring_border(0xff808080) == OPENCE_RENDER_BORDER_OPAQUE_BLACK, "grey is the nearest: black");
		return 0;
	}
	/* the faces culled are named by the front face's winding */
	CASE("cull")
	{
		ring_cull(D3DCULL_NONE, D3DFRONT_CW, &cull, &winding);
		CHECK(cull == OPENCE_RENDER_CULL_NONE && winding == OPENCE_RENDER_WINDING_CLOCKWISE, "none: %u %u", cull,
			winding);
		ring_cull(D3DCULL_CCW, D3DFRONT_CW, &cull, &winding);
		CHECK(cull == OPENCE_RENDER_CULL_BACK, "counter-clockwise culled, clockwise front: %u", cull);
		ring_cull(D3DCULL_CW, D3DFRONT_CW, &cull, &winding);
		CHECK(cull == OPENCE_RENDER_CULL_FRONT, "clockwise culled, clockwise front: %u", cull);
		ring_cull(D3DCULL_CW, D3DFRONT_CCW, &cull, &winding);
		CHECK(cull == OPENCE_RENDER_CULL_BACK && winding == OPENCE_RENDER_WINDING_COUNTER_CLOCKWISE,
			"clockwise culled, counter-clockwise front: %u %u", cull, winding);
		return 0;
	}
	CASE("write-mask")
	{
		CHECK(write_mask(D3DCOLORWRITEENABLE_ALL) == (OPENCE_RENDER_WRITE_RED | OPENCE_RENDER_WRITE_GREEN |
			OPENCE_RENDER_WRITE_BLUE | OPENCE_RENDER_WRITE_ALPHA), "all: %u", write_mask(D3DCOLORWRITEENABLE_ALL));
		CHECK(write_mask(D3DCOLORWRITEENABLE_RED) == OPENCE_RENDER_WRITE_RED, "red");
		CHECK(write_mask(D3DCOLORWRITEENABLE_GREEN) == OPENCE_RENDER_WRITE_GREEN, "green");
		CHECK(write_mask(D3DCOLORWRITEENABLE_BLUE) == OPENCE_RENDER_WRITE_BLUE, "blue");
		CHECK(write_mask(D3DCOLORWRITEENABLE_ALPHA) == OPENCE_RENDER_WRITE_ALPHA, "alpha");
		CHECK(write_mask(0) == 0, "none");
		return 0;
	}
	CASE("primitives")
	{
		CHECK(ring_primitive(D3DPT_POINTLIST, &flag) == OPENCE_RENDER_PRIMITIVE_POINT && !flag, "points");
		CHECK(ring_primitive(D3DPT_LINELIST, &flag) == OPENCE_RENDER_PRIMITIVE_LINE && !flag, "lines");
		CHECK(ring_primitive(D3DPT_LINESTRIP, &flag) == OPENCE_RENDER_PRIMITIVE_LINE_STRIP && !flag, "strip");
		CHECK(ring_primitive(D3DPT_LINELOOP, &flag) == OPENCE_RENDER_PRIMITIVE_LINE_STRIP && flag, "loop");
		CHECK(ring_primitive(D3DPT_TRIANGLELIST, &flag) == OPENCE_RENDER_PRIMITIVE_TRIANGLE && !flag, "triangles");
		CHECK(ring_primitive(D3DPT_TRIANGLESTRIP, &flag) == OPENCE_RENDER_PRIMITIVE_TRIANGLE_STRIP && !flag,
			"triangle strip");
		CHECK(ring_primitive(D3DPT_QUADSTRIP, &flag) == OPENCE_RENDER_PRIMITIVE_TRIANGLE_STRIP && !flag,
			"quad strip");
		CHECK(ring_primitive(D3DPT_TRIANGLEFAN, &flag) == OPENCE_RENDER_PRIMITIVE_TRIANGLE && flag, "fan");
		CHECK(ring_primitive(D3DPT_POLYGON, &flag) == OPENCE_RENDER_PRIMITIVE_TRIANGLE && flag, "polygon");
		CHECK(ring_primitive(D3DPT_QUADLIST, &flag) == OPENCE_RENDER_PRIMITIVE_TRIANGLE && flag, "quads");
		return 0;
	}
	/* fans, polygons, quads and loops as the indices of what Metal draws */
	CASE("primitive-indices")
	{
		static const WORD given[] = { 10, 11, 12, 13, 14, 15, 16, 17 };
		static const WORD fan[] = { 0, 1, 2, 0, 2, 3, 0, 3, 4 };
		static const WORD quads[] = { 10, 11, 12, 10, 12, 13, 14, 15, 16, 14, 16, 17 };
		static const WORD loop[] = { 10, 11, 12, 13, 10 };
		WORD out[8 * 3 + 3];
		unsigned long count;

		count = primitive_indices(D3DPT_TRIANGLEFAN, NULL, 5, out);
		CHECK(indices_are(out, count, fan, 9), "a fan of 5 is %lu indices", count);
		count = primitive_indices(D3DPT_POLYGON, NULL, 5, out);
		CHECK(indices_are(out, count, fan, 9), "a polygon of 5 is %lu indices", count);
		count = primitive_indices(D3DPT_QUADLIST, given, 8, out);
		CHECK(indices_are(out, count, quads, 12), "two quads are %lu indices", count);
		count = primitive_indices(D3DPT_QUADLIST, given, 7, out);
		CHECK(count == 6, "a quad and a part are %lu indices", count);
		count = primitive_indices(D3DPT_LINELOOP, given, 4, out);
		CHECK(indices_are(out, count, loop, 5), "a loop of 4 is %lu indices", count);
		CHECK(primitive_indices(D3DPT_TRIANGLEFAN, NULL, 2, out) == 0, "a fan of 2 draws nothing");
		CHECK(primitive_indices(D3DPT_LINELOOP, NULL, 0, out) == 0, "an empty loop draws nothing");
		return 0;
	}
	return 2;
}
