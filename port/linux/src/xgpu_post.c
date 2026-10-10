/*
XGPU_POST.C

The post-process passes of display.anti_aliasing (xgpu.h): FXAA and SMAA,
run on a render target in place, over the rectangle of a window's 3D view.
Each pass draws one triangle covering the rectangle (its corners from
gl_VertexID, with no vertex data) and finds its texels from gl_FragCoord: a
target's rows are the picture's from the top, as the passes' own textures'
are, so no coordinate is turned over. The target's pixels are copied first,
so that no pass reads what it writes, and only colour is written back: the
game keeps values of its own in destination alpha. The programs are built
when the setting is chosen (xgpu_post_prepare), not in the middle of a frame.

FXAA is written here, after Timothy Lottes' description: the direction of an
edge from the luma around a pixel, the edge's ends found by stepping along
it, and the pixel blended across it by where it lies between them, or by how
much it stands out from its neighbours. SMAA is port/third_party/smaa's,
compiled as GLSL at its HIGH preset: edges from luma, blending weights from
its lookup textures, then the blend. Android has FXAA only (d3d8_gl.c's
anti_aliasing_values).

display.hdr's passes are here too: the tone mapping of a window's 3D view in
the GL_RGBA16F back buffer, in place as the anti-aliasing is (and before
it), and the presentation of that back buffer into the window, which a blit
from a floating-point target would not make linear for an HDR display (nor,
on OpenGL ES, convert at all). The game's values are gamma encoded (the Xbox
sent them to the television as they were): the tone mapping takes them to
linear light (a power of 2.2), scales them by the exposure, brings what is
past white into the display's range with its curve, and encodes them again.
*/

#include "xgpu.h"

#include <stdlib.h>

#ifndef HALO_GLES
#include "zlib_prefixed.h"

/* port/third_party/smaa, embedded by tools/embed_assets.py: the shader's
text (ending in a NUL), and the lookup textures' bytes compressed with
zlib; each size 0 where the build had no such file */
extern const unsigned int xgpu_smaa_shader[];
extern const unsigned long xgpu_smaa_shader_size;
extern const unsigned int xgpu_smaa_area_texture[];
extern const unsigned long xgpu_smaa_area_texture_size;
extern const unsigned int xgpu_smaa_search_texture[];
extern const unsigned long xgpu_smaa_search_texture_size;
#endif

enum
{
	_post_program_fxaa,
	_post_program_smaa_edges,
	_post_program_smaa_weights,
	_post_program_smaa_blend,
	_post_program_tone_map,
	_post_program_present,
	NUMBER_OF_POST_PROGRAMS
};

/* the sets of programs that build or fail together (post.failed) */
enum
{
	_post_set_fxaa,
	_post_set_smaa,
	_post_set_hdr,
	NUMBER_OF_POST_SETS
};

static struct
{
	GLuint vertex_shader;
	GLuint programs[NUMBER_OF_POST_PROGRAMS];
	/* each program's metrics uniform, FXAA's bounds, the tone mapping's
	settings, and the presentation's rectangle and scale */
	GLint metrics[NUMBER_OF_POST_PROGRAMS];
	GLint bounds;
	GLint tone, rectangle, scale;
	/* each set's programs or textures could not be made */
	BOOL failed[NUMBER_OF_POST_SETS];
	/* the size of the textures below, and color's internal format */
	unsigned long width, height;
	GLenum color_format;
	/* the target's pixels before the pass, and SMAA's edges and blending
	weights */
	GLuint color, edges, weights;
	GLuint color_framebuffer, edges_framebuffer, weights_framebuffer;
	/* SMAA's lookup textures */
	GLuint area, search;
	/* linear and clamped, as FXAA and SMAA sample */
	GLuint sampler;
	/* no attributes: the triangle comes from gl_VertexID */
	GLuint vertex_array;
} post;

/* ---------- programs */

static void shader_header(struct xgpu_text *text)
{
#ifdef HALO_GLES
	xgpu_text_append(text,
		"#version %s\n"
		"precision highp float;\n"
		"precision highp int;\n"
		"precision highp sampler2D;\n",
		xgpu_capabilities.shading_language);
#else
	xgpu_text_append(text, "#version 450 core\n");
#endif
}

/* one triangle over the whole viewport */
static const char vertex_source[] =
	"void main()\n"
	"{\n"
	"\tgl_Position = vec4(float((gl_VertexID & 1) << 2) - 1.0, float((gl_VertexID & 2) << 1) - 1.0, 0.0, 1.0);\n"
	"}\n";

static const char fxaa_source[] =
	"uniform sampler2D color_texture;\n"
	"/* 1 / width, 1 / height, width, height */\n"
	"uniform vec4 metrics;\n"
	"/* the rectangle's outermost texel centres, as texture coordinates */\n"
	"uniform vec4 bounds;\n"
	"out vec4 result;\n"
	"\n"
	"const float EDGE_THRESHOLD = 0.125;\n"
	"const float EDGE_THRESHOLD_MINIMUM = 0.0312;\n"
	"const float SUBPIXEL_QUALITY = 0.75;\n"
	"const int STEPS = 12;\n"
	"const float STEP_LENGTHS[12] = float[12](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);\n"
	"\n"
	"vec3 color_at(vec2 position)\n"
	"{\n"
	"\treturn textureLod(color_texture, clamp(position, bounds.xy, bounds.zw), 0.0).rgb;\n"
	"}\n"
	"\n"
	"float luma_at(vec2 position)\n"
	"{\n"
	"\treturn dot(color_at(position), vec3(0.299, 0.587, 0.114));\n"
	"}\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec2 position = gl_FragCoord.xy * metrics.xy;\n"
	"\tfloat luma = luma_at(position);\n"
	"\tfloat up = luma_at(position + vec2(0.0, -metrics.y));\n"
	"\tfloat down = luma_at(position + vec2(0.0, metrics.y));\n"
	"\tfloat left = luma_at(position + vec2(-metrics.x, 0.0));\n"
	"\tfloat right = luma_at(position + vec2(metrics.x, 0.0));\n"
	"\tfloat highest = max(luma, max(max(up, down), max(left, right)));\n"
	"\tfloat range = highest - min(luma, min(min(up, down), min(left, right)));\n"
	"\n"
	"\t/* no edge: the target has the pixel already */\n"
	"\tif (range < max(EDGE_THRESHOLD_MINIMUM, highest * EDGE_THRESHOLD))\n"
	"\t\tdiscard;\n"
	"\tfloat up_left = luma_at(position + vec2(-metrics.x, -metrics.y));\n"
	"\tfloat up_right = luma_at(position + vec2(metrics.x, -metrics.y));\n"
	"\tfloat down_left = luma_at(position + vec2(-metrics.x, metrics.y));\n"
	"\tfloat down_right = luma_at(position + vec2(metrics.x, metrics.y));\n"
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
	"\tvec2 across = (horizontal ? vec2(0.0, metrics.y) : vec2(metrics.x, 0.0)) * (before_steeper ? -1.0 : 1.0);\n"
	"\tvec2 along = horizontal ? vec2(metrics.x, 0.0) : vec2(0.0, metrics.y);\n"
	"\n"
	"\t/* along the edge, half a pixel across, to where its luma ends either way */\n"
	"\tvec2 end_before = position + 0.5 * across - along;\n"
	"\tvec2 end_after = position + 0.5 * across + along;\n"
	"\tfloat luma_before = luma_at(end_before) - edge_luma;\n"
	"\tfloat luma_after = luma_at(end_after) - edge_luma;\n"
	"\tbool reached_before = abs(luma_before) >= gradient;\n"
	"\tbool reached_after = abs(luma_after) >= gradient;\n"
	"\tfor (int index = 1; index < STEPS && !(reached_before && reached_after); index++)\n"
	"\t{\n"
	"\t\tif (!reached_before)\n"
	"\t\t{\n"
	"\t\t\tend_before -= along * STEP_LENGTHS[index];\n"
	"\t\t\tluma_before = luma_at(end_before) - edge_luma;\n"
	"\t\t\treached_before = abs(luma_before) >= gradient;\n"
	"\t\t}\n"
	"\t\tif (!reached_after)\n"
	"\t\t{\n"
	"\t\t\tend_after += along * STEP_LENGTHS[index];\n"
	"\t\t\tluma_after = luma_at(end_after) - edge_luma;\n"
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
	"\tresult = vec4(color_at(position + max(varies ? pixel_offset : 0.0, subpixel) * across), 1.0);\n"
	"}\n";

/* display.hdr's tone mapping. "neutral" leaves the picture as it is drawn
up to KNEE of the peak and rolls off only what is brighter, by the largest
channel (so that a colour keeps its hue and saturation), along an extended
Reinhard curve that meets the line below it at the same slope and reaches
the peak at WHITE times it. "aces" is Krzysztof Narkowicz's fit of the ACES
filmic curve (more contrast; white itself comes out at 0.8), "reinhard" the
extended Reinhard curve on luminance (softer, and darker throughout). */
static const char tone_map_source[] =
	"uniform sampler2D color_texture;\n"
	"/* the curve (xgpu.h), the exposure and the peak */\n"
	"uniform vec4 tone;\n"
	"out vec4 result;\n"
	"\n"
	"const float GAMMA = 2.2;\n"
	"const float KNEE = 0.8;\n"
	"const float WHITE = 8.0;\n"
	"\n"
	"float extended_reinhard(float value, float white)\n"
	"{\n"
	"\treturn value * (1.0 + value / (white * white)) / (1.0 + value);\n"
	"}\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec3 color = pow(max(texelFetch(color_texture, ivec2(gl_FragCoord.xy), 0).rgb, vec3(0.0)), vec3(GAMMA)) * tone.y;\n"
	"\tfloat peak = tone.z;\n"
	"\n"
	"\tif (tone.x < 0.5)\n"
	"\t{\n"
	"\t\tfloat knee = KNEE * peak;\n"
	"\t\tfloat largest = max(color.r, max(color.g, color.b));\n"
	"\n"
	"\t\tif (largest > knee)\n"
	"\t\t{\n"
	"\t\t\tfloat range = peak - knee;\n"
	"\t\t\tfloat rolled = knee + range * extended_reinhard((largest - knee) / range, (WHITE * peak - knee) / range);\n"
	"\n"
	"\t\t\tcolor *= rolled / largest;\n"
	"\t\t}\n"
	"\t}\n"
	"\telse if (tone.x < 1.5)\n"
	"\t{\n"
	"\t\tvec3 x = color / peak;\n"
	"\n"
	"\t\tcolor = peak * clamp(x * (2.51 * x + 0.03) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);\n"
	"\t}\n"
	"\telse\n"
	"\t{\n"
	"\t\tfloat luminance = dot(color, vec3(0.2126, 0.7152, 0.0722)) / peak;\n"
	"\n"
	"\t\tif (luminance > 0.0)\n"
	"\t\t\tcolor *= extended_reinhard(luminance, WHITE) / luminance;\n"
	"\t}\n"
	"\tresult = vec4(pow(min(color, vec3(peak)), vec3(1.0 / GAMMA)), 1.0);\n"
	"}\n";

/* the back buffer into the window: the window's rows are from the bottom */
static const char present_source[] =
	"uniform sampler2D color_texture;\n"
	"/* the window's rectangle: x, y, 1 / width, 1 / height */\n"
	"uniform vec4 rectangle;\n"
	"/* more than 0: linear, times this (scRGB) */\n"
	"uniform float scale;\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec2 position = (gl_FragCoord.xy - rectangle.xy) * rectangle.zw;\n"
	"\tvec3 color = max(textureLod(color_texture, vec2(position.x, 1.0 - position.y), 0.0).rgb, vec3(0.0));\n"
	"\n"
	"\tresult = vec4(scale > 0.0 ? pow(color, vec3(2.2)) * scale : min(color, vec3(1.0)), 1.0);\n"
	"}\n";

#ifndef HALO_GLES
/* the main functions of SMAA's passes, after SMAA.hlsl; its vertex
functions' offsets are found for each pixel */
static const char *const smaa_mains[] =
{
	/* edges */
	"uniform sampler2D color_texture;\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec2 texcoord = gl_FragCoord.xy * metrics.xy;\n"
	"\tvec4 offset[3];\n"
	"\tSMAAEdgeDetectionVS(texcoord, offset);\n"
	"\tresult = vec4(SMAALumaEdgeDetectionPS(texcoord, offset, color_texture), 0.0, 0.0);\n"
	"}\n",
	/* weights */
	"uniform sampler2D edges_texture;\n"
	"uniform sampler2D area_texture;\n"
	"uniform sampler2D search_texture;\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec2 texcoord = gl_FragCoord.xy * metrics.xy;\n"
	"\tvec2 pixcoord;\n"
	"\tvec4 offset[3];\n"
	"\tSMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);\n"
	"\tresult = SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edges_texture, area_texture,\n"
	"\t\tsearch_texture, vec4(0.0));\n"
	"}\n",
	/* blend */
	"uniform sampler2D color_texture;\n"
	"uniform sampler2D weights_texture;\n"
	"out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tvec2 texcoord = gl_FragCoord.xy * metrics.xy;\n"
	"\tvec4 offset;\n"
	"\tSMAANeighborhoodBlendingVS(texcoord, offset);\n"
	"\tresult = SMAANeighborhoodBlendingPS(texcoord, offset, color_texture, weights_texture);\n"
	"}\n",
};
#endif

/* a program, its samplers given their texture units; 0 if it does not
build */
static GLuint program_build(int which)
{
	static const char *const names[NUMBER_OF_POST_PROGRAMS] =
	{
		"FXAA", "SMAA edge detection", "SMAA blending weight", "SMAA neighborhood blending",
		"HDR tone mapping", "HDR presentation"
	};
	static const struct
	{
		const char *name;
		GLint unit;
	} units[] =
	{
		{ "color_texture", 0 }, { "edges_texture", 0 }, { "area_texture", 1 },
		{ "search_texture", 2 }, { "weights_texture", 1 },
	};
	struct xgpu_text text = { 0 };
	GLuint fragment_shader, program;
	unsigned long index;

	if (!post.vertex_shader)
	{
		shader_header(&text);
		xgpu_text_append(&text, "%s", vertex_source);
		post.vertex_shader = xgpu_compile_shader(GL_VERTEX_SHADER, text.buffer, "post-process");
		text.length = 0;
	}
	shader_header(&text);
	if (which == _post_program_fxaa)
	{
		xgpu_text_append(&text, "%s", fxaa_source);
	}
	else if (which == _post_program_tone_map)
	{
		xgpu_text_append(&text, "%s", tone_map_source);
	}
	else if (which == _post_program_present)
	{
		xgpu_text_append(&text, "%s", present_source);
	}
#ifndef HALO_GLES
	else if (xgpu_smaa_shader_size)
	{
		xgpu_text_append(&text,
			"#define SMAA_GLSL_4 1\n"
			"uniform vec4 metrics;\n"
			"#define SMAA_RT_METRICS metrics\n"
			"#define SMAA_PRESET_HIGH 1\n"
			"%s\n%s",
			(const char *)xgpu_smaa_shader, smaa_mains[which - _post_program_smaa_edges]);
	}
#endif
	else
	{
		platform_log("anti-aliasing: the build has no SMAA");
		free(text.buffer);
		return 0;
	}
	fragment_shader = post.vertex_shader ?
		xgpu_compile_shader(GL_FRAGMENT_SHADER, text.buffer, names[which]) :
		0;
	free(text.buffer);
	if (!fragment_shader)
		return 0;
	program = xgpu_link_program(post.vertex_shader, fragment_shader, names[which]);
	glDeleteShader(fragment_shader);
	if (!program)
		return 0;
	glUseProgram(program);
	/* (a name the program does not have is location -1, which is ignored) */
	for (index = 0; index < sizeof(units) / sizeof(units[0]); index++)
		glUniform1i(glGetUniformLocation(program, units[index].name), units[index].unit);
	post.metrics[which] = glGetUniformLocation(program, "metrics");
	if (which == _post_program_fxaa)
		post.bounds = glGetUniformLocation(program, "bounds");
	else if (which == _post_program_tone_map)
		post.tone = glGetUniformLocation(program, "tone");
	else if (which == _post_program_present)
	{
		post.rectangle = glGetUniformLocation(program, "rectangle");
		post.scale = glGetUniformLocation(program, "scale");
	}
	return program;
}

/* ---------- textures */

static GLuint texture_new(GLenum format, unsigned long width, unsigned long height, GLenum data_format,
	const void *data)
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	/* (OpenGL ES takes a half float format's texels only as half floats) */
	glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, (GLsizei)width, (GLsizei)height, 0, data_format,
		format == GL_RGBA16F ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE, data);
	return texture;
}

static GLuint framebuffer_new(GLuint texture)
{
	GLuint framebuffer;
	GLenum draw_buffer = GL_COLOR_ATTACHMENT0;

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glDrawBuffers(1, &draw_buffer);
	return framebuffer;
}

/* the passes' textures, of the target's size (SMAA's only for SMAA); color
is the target's format, so that the copy is exact */
static void textures_fit(BOOL smaa, GLenum format, unsigned long width, unsigned long height)
{
	if (post.width != width || post.height != height || post.color_format != format)
	{
		GLuint textures[3] = { post.color, post.edges, post.weights };
		GLuint framebuffers[3] = { post.color_framebuffer, post.edges_framebuffer, post.weights_framebuffer };

		glDeleteFramebuffers(3, framebuffers);
		glDeleteTextures(3, textures);
		post.color = post.edges = post.weights = 0;
		post.width = width;
		post.height = height;
		post.color_format = format;
		post.color = texture_new(format, width, height, GL_RGBA, NULL);
		post.color_framebuffer = framebuffer_new(post.color);
	}
	if (smaa && !post.edges)
	{
		post.edges = texture_new(GL_RG8, width, height, GL_RG, NULL);
		post.weights = texture_new(GL_RGBA8, width, height, GL_RGBA, NULL);
		post.edges_framebuffer = framebuffer_new(post.edges);
		post.weights_framebuffer = framebuffer_new(post.weights);
	}
}

#ifndef HALO_GLES
/* a lookup texture of SMAA's from its zlib stream; 0 if it does not
inflate */
static GLuint lookup_texture(const unsigned int *stream, unsigned long stream_size, GLenum format,
	GLenum data_format, unsigned long width, unsigned long height, unsigned long texel_size)
{
	uLongf size = (uLongf)(width * height * texel_size);
	unsigned char *texels = malloc(size);
	GLuint texture = 0;
	int result;

	if (!texels)
		return 0;
	result = uncompress(texels, &size, (const Bytef *)stream, (uLong)stream_size);
	/* (the size says whether it is whole, as with the game's zlib 1.1,
	which could stop short of saying the stream had ended) */
	if ((result == Z_OK || result == Z_BUF_ERROR) && size == (uLongf)(width * height * texel_size))
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		texture = texture_new(format, width, height, data_format, texels);
	}
	free(texels);
	return texture;
}
#endif

/* ---------- the passes */

static void texture_bind(GLuint unit, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindSampler(unit, post.sampler);
}

/* a program's triangle drawn into a framebuffer over the rectangle;
color_only keeps the framebuffer's alpha */
static void pass_draw(int which, GLuint framebuffer, const GLint corners[4], BOOL color_only)
{
	float metrics[4];

	metrics[0] = 1.0f / (float)post.width;
	metrics[1] = 1.0f / (float)post.height;
	metrics[2] = (float)post.width;
	metrics[3] = (float)post.height;
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(corners[0], corners[1], corners[2] - corners[0], corners[3] - corners[1]);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, color_only ? GL_FALSE : GL_TRUE);
	glUseProgram(post.programs[which]);
	glUniform4fv(post.metrics[which], 1, metrics);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

/* an intermediate emptied, all of it, so that no edge of another window's
pass is found: SMAA's passes write only where there is an edge */
static void intermediate_clear(GLuint framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

/* the sampler and the vertex array every pass uses */
static void common_prepare(void)
{
	if (!post.sampler)
	{
		glGenSamplers(1, &post.sampler);
		glSamplerParameteri(post.sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(post.sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameteri(post.sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(post.sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glGenVertexArrays(1, &post.vertex_array);
	}
}

/* a set's programs, built if they are not; FALSE (and the set failed) if one
does not build */
static BOOL programs_prepare(int set, int first, int last)
{
	int which;

	if (post.failed[set])
		return FALSE;
	for (which = first; which <= last; which++)
	{
		if (!post.programs[which] && !(post.programs[which] = program_build(which)))
		{
			post.failed[set] = TRUE;
			return FALSE;
		}
	}
	common_prepare();
	return TRUE;
}

/* the state a pass draws in, whatever the game's was */
static void pass_state(void)
{
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
#ifndef HALO_GLES
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
	glBindVertexArray(post.vertex_array);
}

/* the target's pixels over the rectangle into color, read while the target
is drawn into */
static void color_copy(GLuint framebuffer, const GLint corners[4])
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, post.color_framebuffer);
	glBlitFramebuffer(corners[0], corners[1], corners[2], corners[3], corners[0], corners[1], corners[2], corners[3],
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

BOOL xgpu_post_prepare(BOOL smaa)
{
	if (!(smaa ?
		programs_prepare(_post_set_smaa, _post_program_smaa_edges, _post_program_smaa_blend) :
		programs_prepare(_post_set_fxaa, _post_program_fxaa, _post_program_fxaa)))
	{
		return FALSE;
	}
#ifndef HALO_GLES
	if (smaa && !post.area)
	{
		post.area = lookup_texture(xgpu_smaa_area_texture, xgpu_smaa_area_texture_size, GL_RG8, GL_RG,
			160, 560, 2);
		post.search = lookup_texture(xgpu_smaa_search_texture, xgpu_smaa_search_texture_size, GL_R8, GL_RED,
			64, 16, 1);
		if (!post.area || !post.search)
		{
			platform_log("anti-aliasing: cannot inflate SMAA's lookup textures");
			post.failed[_post_set_smaa] = TRUE;
			return FALSE;
		}
	}
#endif
	return TRUE;
}

BOOL xgpu_post_prepare_hdr(void)
{
	return programs_prepare(_post_set_hdr, _post_program_tone_map, _post_program_present);
}

BOOL xgpu_post_anti_alias(BOOL smaa, GLuint framebuffer, GLenum format, unsigned long width,
	unsigned long height, const GLint corners[4])
{
	/* (made already, as the setting was chosen) */
	if (!xgpu_post_prepare(smaa))
		return FALSE;
	textures_fit(smaa, format, width, height);
	pass_state();
	color_copy(framebuffer, corners);

	if (!smaa)
	{
		float bounds[4];

		/* (split screen: no texel of the next window is read) */
		bounds[0] = ((float)corners[0] + 0.5f) / (float)width;
		bounds[1] = ((float)corners[1] + 0.5f) / (float)height;
		bounds[2] = ((float)corners[2] - 0.5f) / (float)width;
		bounds[3] = ((float)corners[3] - 0.5f) / (float)height;
		texture_bind(0, post.color);
		glUseProgram(post.programs[_post_program_fxaa]);
		glUniform4fv(post.bounds, 1, bounds);
		pass_draw(_post_program_fxaa, framebuffer, corners, TRUE);
	}
	else
	{
		intermediate_clear(post.edges_framebuffer);
		texture_bind(0, post.color);
		pass_draw(_post_program_smaa_edges, post.edges_framebuffer, corners, FALSE);

		intermediate_clear(post.weights_framebuffer);
		texture_bind(0, post.edges);
		texture_bind(1, post.area);
		texture_bind(2, post.search);
		pass_draw(_post_program_smaa_weights, post.weights_framebuffer, corners, FALSE);

		texture_bind(0, post.color);
		texture_bind(1, post.weights);
		pass_draw(_post_program_smaa_blend, framebuffer, corners, TRUE);
	}
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	return TRUE;
}

BOOL xgpu_post_tone_map(GLuint source, GLuint destination, unsigned long width, unsigned long height,
	const GLint corners[4], const struct xgpu_tone_mapping *tone_mapping)
{
	float tone[4];

	if (!xgpu_post_prepare_hdr())
		return FALSE;
	textures_fit(FALSE, GL_RGBA16F, width, height);
	pass_state();
	color_copy(source, corners);

	tone[0] = (float)tone_mapping->curve;
	tone[1] = tone_mapping->exposure;
	tone[2] = tone_mapping->peak;
	tone[3] = 0.0f;
	texture_bind(0, post.color);
	glUseProgram(post.programs[_post_program_tone_map]);
	glUniform4fv(post.tone, 1, tone);
	pass_draw(_post_program_tone_map, destination, corners, TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	return TRUE;
}

BOOL xgpu_post_present(GLuint texture, int x, int y, int width, int height, float scale)
{
	float rectangle[4];

	if (!xgpu_post_prepare_hdr() || width <= 0 || height <= 0)
		return FALSE;
	pass_state();
	rectangle[0] = (float)x;
	rectangle[1] = (float)y;
	rectangle[2] = 1.0f / (float)width;
	rectangle[3] = 1.0f / (float)height;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(x, y, width, height);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	texture_bind(0, texture);
	glUseProgram(post.programs[_post_program_present]);
	glUniform4fv(post.rectangle, 1, rectangle);
	glUniform1f(post.scale, scale);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	return TRUE;
}
