#include "harness.h"
#include <stdarg.h>
typedef unsigned long DWORD;
typedef int BOOL;
static BOOL xgpu_reversed_depth;
#define XGPU_VERTEX_ATTRIBUTE_COUNT 16
#define XGPU_VERTEX_CONSTANT_COUNT 192
#define XGPU_VERTEX_CONSTANT_BIAS 96
struct xgpu_text;
void xgpu_text_append(struct xgpu_text *text, const char *format, ...);
#include "under_test.inc"
void xgpu_text_append(struct xgpu_text *text, const char *format, ...)
{
	char buffer[8192];
	va_list args;
	int length;
	va_start(args, format); length = vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);
	CHECK(length >= 0 && length < sizeof(buffer), "bounded shader append");
	text->buffer = realloc(text->buffer, text->length + length + 1);
	memcpy(text->buffer + text->length, buffer, length + 1);
	text->length += length;
}
int main(int argc, char **argv)
{
	unsigned int i, recognized = 0;
	int enabled, per_pixel, packed;
	struct xgpu_text pixel = {0};
	for (enabled = 0; enabled < 2; enabled++)
	for (per_pixel = 0; per_pixel < 2; per_pixel++)
	for (packed = 0; packed < 2; packed++)
	for (i = 0; i < sizeof(lengths)/sizeof(lengths[0]); i++)
	{
		struct nv2a_vertex_lighting lighting;
		int model = nv2a_vertex_shader_lighting(programs[i], lengths[i], &lighting);
		int point = model && lighting.lights == 2;
		char *shader = nv2a_vertex_shader_to_glsl(programs[i], lengths[i], packed ? 2 : 0,
			per_pixel && model ? &lighting : NULL, enabled);
		if (point && enabled)
		{
			recognized++;
#ifndef HALO_ANDROID
			CHECK(strstr(shader, "if (plasma_model_slots[0] > 0.5)"), "first point light color reached in program %u", i);
			CHECK(strstr(shader, "if (plasma_model_slots[1] > 0.5)"), "second point light color reached in program %u", i);
			CHECK(strstr(shader, "vec3 plasma_model_position") < strstr(shader, "B.xyz *="), "position captured before color use");
#endif
		}
		else CHECK(!strstr(shader, "plasma_") && !strstr(shader, "sampler3D"), "disabled and non-point programs are stock");
#ifdef HALO_ANDROID
		CHECK(!strstr(shader, "plasma_") && !strstr(shader, "sampler3D"), "Android vertex program has no optional lighting");
#endif
		free(shader);
	}
	CHECK(recognized >= 2, "real two-point-light programs tested");
	for (enabled = 0; enabled < 2; enabled++)
	{
		model_lighting(&pixel, 1, enabled);
#ifdef HALO_ANDROID
		CHECK(!strstr(pixel.buffer, "plasma_") && !strstr(pixel.buffer, "sampler3D"), "Android per-pixel lighting is stock");
#else
		if (enabled)
			CHECK(strstr(pixel.buffer, "plasma_model_falloff") && strstr(pixel.buffer, "plasma_model_slots[1]"), "supported per-pixel lighting retained");
		else
			CHECK(!strstr(pixel.buffer, "plasma_") && !strstr(pixel.buffer, "sampler3D"), "disabled per-pixel lighting has no feature code");
#endif
		if (!strstr(pixel.buffer, "plasma_model_falloff"))
		{
			CHECK(strstr(pixel.buffer, "light += max(1.0 - distance_squared * model_lights[1].w, 0.0)"), "first stock point light");
			CHECK(strstr(pixel.buffer, "light += max(1.0 - distance_squared * model_lights[4].w, 0.0)"), "second stock point light");
		}
		free(pixel.buffer);
		memset(&pixel, 0, sizeof(pixel));
		model_lighting(&pixel, 0, enabled);
		CHECK(!strstr(pixel.buffer, "plasma_") && !strstr(pixel.buffer, "xWorldPosition"), "ambient-only models unchanged");
		free(pixel.buffer);
		memset(&pixel, 0, sizeof(pixel));
	}
	return 0;
}
