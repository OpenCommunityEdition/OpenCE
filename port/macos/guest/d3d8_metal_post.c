/*
D3D8_METAL_POST.C

display.anti_aliasing's passes for the macOS app's device, as xgpu_post.c
draws them with OpenGL: the same FXAA, written again in the Metal Shading
Language, and port/third_party/smaa's SMAA at its HIGH preset, compiled as
MSL (SMAA_CUSTOM_SL) with its porting functions defined here. The device
(d3d8_metal_device.c) copies the target, draws the passes and writes the
colour back; this file makes their sources and SMAA's lookup textures.

Each library has a vertex_main drawing one triangle over the viewport from
its vertex id, and a fragment_main finding its texels from the pixel's
position: Metal's targets and textures both have their rows from the top,
so no coordinate is turned over. FXAA's metrics and bounds are uniforms;
SMAA's metrics are a literal of the target's size (SMAA_RT_METRICS), as its
functions use them where no uniform reaches, so its libraries are made for
each size of the screen.

SMAA.hlsl's out and inout parameters are rewritten as references (MSL is
C++), its arrays of offsets as pointers, the bool4 SMAAMovc as a select,
and the two corner patterns' calls given a variable in place of a swizzle,
which a reference cannot be bound to.
*/

#include "xgpu.h"
#include "zlib_prefixed.h"

#include <stdlib.h>
#include <string.h>

/* port/third_party/smaa, embedded by tools/embed_assets.py (xgpu_post.c) */
extern const unsigned int xgpu_smaa_shader[];
extern const unsigned long xgpu_smaa_shader_size;
extern const unsigned int xgpu_smaa_area_texture[];
extern const unsigned long xgpu_smaa_area_texture_size;
extern const unsigned int xgpu_smaa_search_texture[];
extern const unsigned long xgpu_smaa_search_texture_size;

/* ---------- FXAA (xgpu_post.c's fxaa_source) */

static const char post_vertex_source[] =
	"struct post_out { float4 position [[position]]; };\n"
	"\n"
	"/* one triangle over the whole viewport */\n"
	"vertex post_out vertex_main(uint id [[vertex_id]])\n"
	"{\n"
	"\tpost_out out;\n"
	"\tout.position = float4(float((id & 1) << 2) - 1.0, float((id & 2) << 1) - 1.0, 0.0, 1.0);\n"
	"\treturn out;\n"
	"}\n";

static const char fxaa_source[] =
	"/* 1 / width, 1 / height, width, height; and the rectangle's outermost\n"
	"texel centres, as texture coordinates */\n"
	"struct fxaa_uniforms { float4 metrics; float4 bounds; };\n"
	"\n"
	"constant float EDGE_THRESHOLD = 0.125;\n"
	"constant float EDGE_THRESHOLD_MINIMUM = 0.0312;\n"
	"constant float SUBPIXEL_QUALITY = 0.75;\n"
	"constant int STEPS = 12;\n"
	"constant float STEP_LENGTHS[12] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };\n"
	"constexpr sampler linear_clamp(filter::linear, address::clamp_to_edge);\n"
	"\n"
	"static float3 color_at(texture2d<float> color_texture, constant fxaa_uniforms &u, float2 position)\n"
	"{\n"
	"\treturn color_texture.sample(linear_clamp, clamp(position, u.bounds.xy, u.bounds.zw), level(0.0)).rgb;\n"
	"}\n"
	"\n"
	"static float luma_at(texture2d<float> color_texture, constant fxaa_uniforms &u, float2 position)\n"
	"{\n"
	"\treturn dot(color_at(color_texture, u, position), float3(0.299, 0.587, 0.114));\n"
	"}\n"
	"\n"
	"#define LUMA(position) luma_at(color_texture, u, position)\n"
	"\n"
	"fragment float4 fragment_main(post_out in [[stage_in]], texture2d<float> color_texture [[texture(0)]],\n"
	"\tconstant fxaa_uniforms &u [[buffer(0)]])\n"
	"{\n"
	"\tfloat2 metrics = u.metrics.xy;\n"
	"\tfloat2 position = in.position.xy * metrics;\n"
	"\tfloat luma = LUMA(position);\n"
	"\tfloat up = LUMA(position + float2(0.0, -metrics.y));\n"
	"\tfloat down = LUMA(position + float2(0.0, metrics.y));\n"
	"\tfloat left = LUMA(position + float2(-metrics.x, 0.0));\n"
	"\tfloat right = LUMA(position + float2(metrics.x, 0.0));\n"
	"\tfloat highest = max(luma, max(max(up, down), max(left, right)));\n"
	"\tfloat range = highest - min(luma, min(min(up, down), min(left, right)));\n"
	"\n"
	"\t/* no edge: the target has the pixel already */\n"
	"\tif (range < max(EDGE_THRESHOLD_MINIMUM, highest * EDGE_THRESHOLD))\n"
	"\t\tdiscard_fragment();\n"
	"\tfloat up_left = LUMA(position + float2(-metrics.x, -metrics.y));\n"
	"\tfloat up_right = LUMA(position + float2(metrics.x, -metrics.y));\n"
	"\tfloat down_left = LUMA(position + float2(-metrics.x, metrics.y));\n"
	"\tfloat down_right = LUMA(position + float2(metrics.x, metrics.y));\n"
	"\n"
	"\t/* how much the pixel stands out from its neighbourhood (a feature\n"
	"\tthinner than a pixel) */\n"
	"\tfloat average = (2.0 * (up + down + left + right) + up_left + up_right + down_left + down_right) / 12.0;\n"
	"\tfloat subpixel = smoothstep(0.0, 1.0, clamp(abs(average - luma) / range, 0.0, 1.0));\n"
	"\tsubpixel = subpixel * subpixel * SUBPIXEL_QUALITY;\n"
	"\n"
	"\t/* an edge along the rows where the luma changes more from row to row\n"
	"\tthan from column to column; across it, the side it changes more to */\n"
	"\tbool horizontal = abs(up_left + down_left - 2.0 * left) + 2.0 * abs(up + down - 2.0 * luma) +\n"
	"\t\tabs(up_right + down_right - 2.0 * right) >=\n"
	"\t\tabs(up_left + up_right - 2.0 * up) + 2.0 * abs(left + right - 2.0 * luma) +\n"
	"\t\tabs(down_left + down_right - 2.0 * down);\n"
	"\tfloat before = horizontal ? up : left;\n"
	"\tfloat after = horizontal ? down : right;\n"
	"\tbool before_steeper = abs(before - luma) >= abs(after - luma);\n"
	"\tfloat gradient = 0.25 * max(abs(before - luma), abs(after - luma));\n"
	"\tfloat edge_luma = 0.5 * ((before_steeper ? before : after) + luma);\n"
	"\tfloat2 across = (horizontal ? float2(0.0, metrics.y) : float2(metrics.x, 0.0)) * (before_steeper ? -1.0 : 1.0);\n"
	"\tfloat2 along = horizontal ? float2(metrics.x, 0.0) : float2(0.0, metrics.y);\n"
	"\n"
	"\t/* along the edge, half a pixel across, to where its luma ends either way */\n"
	"\tfloat2 end_before = position + 0.5 * across - along;\n"
	"\tfloat2 end_after = position + 0.5 * across + along;\n"
	"\tfloat luma_before = LUMA(end_before) - edge_luma;\n"
	"\tfloat luma_after = LUMA(end_after) - edge_luma;\n"
	"\tbool reached_before = abs(luma_before) >= gradient;\n"
	"\tbool reached_after = abs(luma_after) >= gradient;\n"
	"\tfor (int index = 1; index < STEPS && !(reached_before && reached_after); index++)\n"
	"\t{\n"
	"\t\tif (!reached_before)\n"
	"\t\t{\n"
	"\t\t\tend_before -= along * STEP_LENGTHS[index];\n"
	"\t\t\tluma_before = LUMA(end_before) - edge_luma;\n"
	"\t\t\treached_before = abs(luma_before) >= gradient;\n"
	"\t\t}\n"
	"\t\tif (!reached_after)\n"
	"\t\t{\n"
	"\t\t\tend_after += along * STEP_LENGTHS[index];\n"
	"\t\t\tluma_after = LUMA(end_after) - edge_luma;\n"
	"\t\t\treached_after = abs(luma_after) >= gradient;\n"
	"\t\t}\n"
	"\t}\n"
	"\n"
	"\t/* blended across the edge the more, the nearer the pixel is to an end;\n"
	"\tonly where the luma at that end changes the way the pixel's does */\n"
	"\tfloat distance_before = horizontal ? position.x - end_before.x : position.y - end_before.y;\n"
	"\tfloat distance_after = horizontal ? end_after.x - position.x : end_after.y - position.y;\n"
	"\tfloat pixel_offset = 0.5 - min(distance_before, distance_after) / (distance_before + distance_after);\n"
	"\tbool varies = ((distance_before < distance_after ? luma_before : luma_after) < 0.0) != (luma < edge_luma);\n"
	"\treturn float4(color_at(color_texture, u, position + max(varies ? pixel_offset : 0.0, subpixel) * across), 1.0);\n"
	"}\n";

/* ---------- SMAA */

/* its porting functions in MSL, and the preset */
static const char smaa_porting[] =
	"constexpr sampler LinearSampler(filter::linear, address::clamp_to_edge);\n"
	"constexpr sampler PointSampler(filter::nearest, address::clamp_to_edge);\n"
	"#define SMAA_CUSTOM_SL 1\n"
	"#define SMAATexture2D(tex) texture2d<float> tex\n"
	"#define SMAATexturePass2D(tex) tex\n"
	"#define SMAASampleLevelZero(tex, coord) tex.sample(LinearSampler, coord, level(0.0))\n"
	"#define SMAASampleLevelZeroPoint(tex, coord) tex.sample(PointSampler, coord, level(0.0))\n"
	"#define SMAASampleLevelZeroOffset(tex, coord, offset) tex.sample(LinearSampler, coord, level(0.0), offset)\n"
	"#define SMAASample(tex, coord) tex.sample(LinearSampler, coord, level(0.0))\n"
	"#define SMAASamplePoint(tex, coord) tex.sample(PointSampler, coord, level(0.0))\n"
	"#define SMAASampleOffset(tex, coord, offset) tex.sample(LinearSampler, coord, level(0.0), offset)\n"
	"#define discard discard_fragment()\n"
	"#define SMAA_FLATTEN\n"
	"#define SMAA_BRANCH\n"
	"#define lerp(a, b, t) mix(a, b, t)\n"
	"#define mad(a, b, c) ((a) * (b) + (c))\n"
	"#define SMAA_PRESET_HIGH 1\n";

/* the main functions of its passes, as xgpu_post.c's smaa_mains */
static const char *const smaa_mains[] =
{
	/* edges */
	"fragment float4 fragment_main(post_out in [[stage_in]], texture2d<float> color_texture [[texture(0)]])\n"
	"{\n"
	"\tfloat2 texcoord = in.position.xy * SMAA_RT_METRICS.xy;\n"
	"\tfloat4 offset[3];\n"
	"\tSMAAEdgeDetectionVS(texcoord, offset);\n"
	"\treturn float4(SMAALumaEdgeDetectionPS(texcoord, offset, color_texture), 0.0, 0.0);\n"
	"}\n",
	/* weights */
	"fragment float4 fragment_main(post_out in [[stage_in]], texture2d<float> edges_texture [[texture(0)]],\n"
	"\ttexture2d<float> area_texture [[texture(1)]], texture2d<float> search_texture [[texture(2)]])\n"
	"{\n"
	"\tfloat2 texcoord = in.position.xy * SMAA_RT_METRICS.xy;\n"
	"\tfloat2 pixcoord;\n"
	"\tfloat4 offset[3];\n"
	"\tSMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);\n"
	"\treturn SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edges_texture, area_texture,\n"
	"\t\tsearch_texture, float4(0.0));\n"
	"}\n",
	/* blend */
	"fragment float4 fragment_main(post_out in [[stage_in]], texture2d<float> color_texture [[texture(0)]],\n"
	"\ttexture2d<float> weights_texture [[texture(1)]])\n"
	"{\n"
	"\tfloat2 texcoord = in.position.xy * SMAA_RT_METRICS.xy;\n"
	"\tfloat4 offset;\n"
	"\tSMAANeighborhoodBlendingVS(texcoord, offset);\n"
	"\treturn SMAANeighborhoodBlendingPS(texcoord, offset, color_texture, weights_texture);\n"
	"}\n",
};

/* SMAA.hlsl's text made MSL: each is found at least once */
static const char *const smaa_rewrites[][2] =
{
	{ "out float4 offset[3]", "thread float4 *offset" },
	{ "float4 offset[3]", "thread float4 *offset" },
	{ "out float2 pixcoord", "thread float2 &pixcoord" },
	{ "out float4 offset)", "thread float4 &offset)" },
	{ "out float2 e)", "thread float2 &e)" },
	{ "inout float2 weights", "thread float2 &weights" },
	{ "inout float2 variable", "thread float2 &variable" },
	{ "inout float4 variable", "thread float4 &variable" },
	{ "SMAAMovc(cond.xy, variable.xy, value.xy);", "variable = select(variable, value, cond);" },
	{ "SMAAMovc(cond.zw, variable.zw, value.zw);", "" },
	{ "SMAADetectHorizontalCornerPattern(SMAATexturePass2D(edgesTex), weights.rg, coords.xyzy, d);",
		"{ float2 corner = weights.rg; "
		"SMAADetectHorizontalCornerPattern(SMAATexturePass2D(edgesTex), corner, coords.xyzy, d); "
		"weights.rg = corner; }" },
	{ "SMAADetectVerticalCornerPattern(SMAATexturePass2D(edgesTex), weights.ba, coords.xyxz, d);",
		"{ float2 corner = weights.ba; "
		"SMAADetectVerticalCornerPattern(SMAATexturePass2D(edgesTex), corner, coords.xyxz, d); "
		"weights.ba = corner; }" },
};

#define NUMBER_OF_SMAA_REWRITES ((int)(sizeof(smaa_rewrites) / sizeof(smaa_rewrites[0])))

/* text with every `from` replaced by `to` (malloc'd; text is freed), NULL
if there is none or no memory */
static char *text_replace(char *text, const char *from, const char *to)
{
	unsigned long from_length = strlen(from), to_length = strlen(to), count = 0;
	const char *place;
	char *result, *out;

	for (place = strstr(text, from); place; place = strstr(place + from_length, from))
		count++;
	result = count ? malloc(strlen(text) - count * from_length + count * to_length + 1) : NULL;
	if (result)
	{
		const char *in = text;

		out = result;
		for (place = strstr(in, from); place; place = strstr(in, from))
		{
			memcpy(out, in, (unsigned long)(place - in));
			out += place - in;
			memcpy(out, to, to_length);
			out += to_length;
			in = place + from_length;
		}
		strcpy(out, in);
	}
	free(text);
	return result;
}

/* SMAA.hlsl's text as MSL, NULL if the build has none or it is not the
text expected */
static char *smaa_text(void)
{
	const char *shader = (const char *)xgpu_smaa_shader;
	unsigned long length = xgpu_smaa_shader_size ? strlen(shader) : 0;
	char *text = length ? malloc(length + 1) : NULL;
	unsigned long index;
	int rewrite;

	if (!text)
		return NULL;
	/* (its comments' Latin-1, which is not UTF-8, left out) */
	for (index = 0; index < length; index++)
		text[index] = (unsigned char)shader[index] < 0x80 ? shader[index] : '?';
	text[length] = '\0';
	for (rewrite = 0; text && rewrite < NUMBER_OF_SMAA_REWRITES; rewrite++)
		text = text_replace(text, smaa_rewrites[rewrite][0], smaa_rewrites[rewrite][1]);
	return text;
}

char *xgpu_metal_post_source(int which, unsigned long width, unsigned long height)
{
	struct xgpu_text text = { 0 };
	char *smaa;

	if (which < 0 || which >= NUMBER_OF_METAL_POST_PASSES || !width || !height)
		return NULL;
	xgpu_text_append(&text, "#include <metal_stdlib>\nusing namespace metal;\n%s", post_vertex_source);
	if (which == _metal_post_fxaa)
	{
		xgpu_text_append(&text, "%s", fxaa_source);
		return text.buffer;
	}
	if (!(smaa = smaa_text()))
	{
		free(text.buffer);
		return NULL;
	}
	xgpu_text_append(&text, "#define SMAA_RT_METRICS float4(1.0 / %lu.0, 1.0 / %lu.0, %lu.0, %lu.0)\n%s%s\n%s",
		width, height, width, height, smaa_porting, smaa, smaa_mains[which - _metal_post_smaa_edges]);
	free(smaa);
	return text.buffer;
}

unsigned char *xgpu_metal_post_lookup(BOOL search, unsigned long *width, unsigned long *height,
	unsigned long *texel_size)
{
	const unsigned int *stream = search ? xgpu_smaa_search_texture : xgpu_smaa_area_texture;
	unsigned long stream_size = search ? xgpu_smaa_search_texture_size : xgpu_smaa_area_texture_size;
	uLongf size;
	unsigned char *texels;
	int result;

	*width = search ? 64 : 160;
	*height = search ? 16 : 560;
	*texel_size = search ? 1 : 2;
	size = (uLongf)(*width * *height * *texel_size);
	if (!stream_size || !(texels = malloc(size)))
		return NULL;
	result = uncompress(texels, &size, (const Bytef *)stream, (uLong)stream_size);
	/* (the size says whether it is whole, as xgpu_post.c's lookup_texture
	finds) */
	if ((result != Z_OK && result != Z_BUF_ERROR) || size != (uLongf)(*width * *height * *texel_size))
	{
		free(texels);
		return NULL;
	}
	return texels;
}
