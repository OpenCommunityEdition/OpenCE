/*
XGPU_HLSL.C

What the shader translators (nv2a_vsh.c, nv2a_psh.c) need to write HLSL for
the Direct3D 12 renderer as well as GLSL: they write the same statements
for both (GLSL's syntax), and HLSL takes them with GLSL's names defined as
its own (xgpu_hlsl_prologue) and GLSL's one-argument vector constructors,
which HLSL has not, turned into casts (xgpu_hlsl_from_glsl). The constant
buffers and the values handed from the vertex shader to the pixel shader
are laid out here once, as the renderer fills and links them (xgpu.h).
*/

#include "xgpu.h"

#include <stdlib.h>
#include <string.h>

const char xgpu_hlsl_prologue[] =
	"#define vec2 float2\n"
	"#define vec3 float3\n"
	"#define vec4 float4\n"
	"#define mix lerp\n"
	"#define fract frac\n"
	"#define inversesqrt rsqrt\n"
	"#define lessThan(a, b) ((a) < (b))\n"
	"#define greaterThanEqual(a, b) ((a) >= (b))\n"
	"#define texture(t, c) t.Sample(s_##t, c)\n"
	/* the vertex shader's constants (struct xgpu_hlsl_vertex_constants) */
	"cbuffer vertex_constants : register(b0)\n"
	"{\n"
	"	float4 c[192];\n"
	"	float4 viewport_scale;\n"
	"	float4 viewport_offset;\n"
	"	float4 vertex_misc;\n"
	"	float4 attribute_values[16];\n"
	"};\n"
	"#define point_size (vertex_misc.x)\n"
	"#define screen_offset (vertex_misc.y)\n"
	/* the pixel shader's (struct xgpu_hlsl_pixel_constants) */
	"cbuffer pixel_constants : register(b1)\n"
	"{\n"
	"	float4 ps_c0[8];\n"
	"	float4 ps_c1[8];\n"
	"	float4 ps_final_c0;\n"
	"	float4 ps_final_c1;\n"
	"	float4 fog_color;\n"
	"	float4 fog_parameters;\n"
	"	float4 pixel_misc;\n"
	"	float4 bump_matrix[4];\n"
	"	float4 bump_luminance[4];\n"
	"	float4 texture_scale[4];\n"
	"	float4 model_lights[12];\n"
	"};\n"
	"#define alpha_reference (pixel_misc.x)\n"
	/* every vertex shader hands on all of them and every pixel shader takes
	all of them, so that any two link */
	"struct interpolants\n"
	"{\n"
	"	float4 position : SV_Position;\n"
	"	float4 xD0 : COLOR0;\n"
	"	float4 xD1 : COLOR1;\n"
	"	float4 xB0 : TEXCOORD4;\n"
	"	float4 xB1 : TEXCOORD5;\n"
	"	float4 xT0 : TEXCOORD0;\n"
	"	float4 xT1 : TEXCOORD1;\n"
	"	float4 xT2 : TEXCOORD2;\n"
	"	float4 xT3 : TEXCOORD3;\n"
	"	float xFog : TEXCOORD6;\n"
	"	float4 xWorldNormal : TEXCOORD7;\n"
	"	float3 xWorldPosition : TEXCOORD8;\n"
	"};\n"
	"static float4 xD0, xD1, xB0, xB1, xT0, xT1, xT2, xT3, xWorldNormal;\n"
	"static float xFog;\n"
	"static float3 xWorldPosition;\n";

/* the end of the parenthesized text that starts at text[0] == '(': the
index of its ')', and whether a comma is in it at its own level */
static size_t matching_parenthesis(const char *text, size_t length, int *comma)
{
	size_t index;
	int depth = 0;

	*comma = 0;
	for (index = 0; index < length; index++)
	{
		if (text[index] == '(' || text[index] == '[')
			depth++;
		else if (text[index] == ')' || text[index] == ']')
		{
			if (--depth == 0)
				return index;
		}
		else if (text[index] == ',' && depth == 1)
			*comma = 1;
	}
	return length;
}

static int identifier_character(char character)
{
	return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
		(character >= '0' && character <= '9') || character == '_';
}

/* text with GLSL's one-argument vector constructors, vecN(x), turned into
HLSL's casts, ((floatN)(x)), their arguments too */
static void convert(struct xgpu_text *out, const char *text, size_t length)
{
	size_t index = 0;

	while (index < length)
	{
		if (index + 5 <= length && text[index] == 'v' && !strncmp(text + index, "vec", 3) &&
			text[index + 3] >= '2' && text[index + 3] <= '4' && text[index + 4] == '(' &&
			(index == 0 || !identifier_character(text[index - 1])))
		{
			int comma;
			size_t close = index + 4 + matching_parenthesis(text + index + 4, length - index - 4, &comma);

			if (close < length && !comma)
			{
				xgpu_text_append(out, "((float%c)(", text[index + 3]);
				convert(out, text + index + 5, close - index - 5);
				xgpu_text_append(out, "))");
				index = close + 1;
				continue;
			}
		}
		xgpu_text_append(out, "%c", text[index]);
		index++;
	}
}

char *xgpu_hlsl_from_glsl(const char *text)
{
	struct xgpu_text out = { 0 };

	convert(&out, text, strlen(text));
	return out.buffer;
}
