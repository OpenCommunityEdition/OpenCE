/* Full-width stock loading artwork and a smooth, screen-resolution sweep.
 * The stock progress renderer still owns progress and the Xbox chant.
 * No tag, cache, or map-family state is accessed here. */
#include "loading_widescreen.h"
#include "port_config.h"
#include "xgpu.h"
#include <string.h>

#define ART_WIDTH 320
#define ART_HEIGHT 240

static struct
{
	GLuint texture, program, vertex_array;
	GLint metrics, sweep;
	int failed;
} loading;

#ifdef HALO_GLES
#define SHADER_HEADER "#version 300 es\nprecision highp float;\nprecision highp int;\n"
#else
#define SHADER_HEADER "#version 450 core\n"
#endif

static const char vertex_source[] = SHADER_HEADER
	"void main() {\n"
	" gl_Position = vec4(float((gl_VertexID & 1) << 2) - 1.0,\n"
	"                    float((gl_VertexID & 2) << 1) - 1.0, 0.0, 1.0);\n"
	"}\n";

static const char fragment_source[] = SHADER_HEADER
	"uniform sampler2D artwork;\n"
	"uniform vec2 metrics;\n"
	"uniform float progress;\n"
	"out vec4 result;\n"
	/* Catmull-Rom reconstruction retains detail when enlarged. At small
	 * resolutions ordinary linear mip filtering avoids shimmering. */
	"vec4 weights(float t) {\n"
	" return vec4(-0.5*t+t*t-0.5*t*t*t, 1.0-2.5*t*t+1.5*t*t*t,\n"
	"             0.5*t+2.0*t*t-1.5*t*t*t, -0.5*t*t+0.5*t*t*t);\n"
	"}\n"
	"vec3 picture(vec2 uv, float scale) {\n"
	" if (scale <= 1.0) return texture(artwork, uv).rgb;\n"
	" vec2 p = uv * vec2(320.0,240.0) - 0.5;\n"
	" vec2 base = floor(p);\n"
	" vec4 wx = weights(fract(p.x)), wy = weights(fract(p.y));\n"
	" vec3 color = vec3(0.0);\n"
	" for (int y=0; y<4; ++y) for (int x=0; x<4; ++x)\n"
	"  color += texelFetch(artwork, clamp(ivec2(base)+ivec2(x-1,y-1),\n"
	"           ivec2(0),ivec2(319,239)),0).rgb * wx[x] * wy[y];\n"
	" return clamp(color,0.0,1.0);\n"
	"}\n"
	"void main() {\n"
	/* The engine's offscreen targets have row zero at the top. Cover
	 * crops the excess axis rather than stretching the picture or tiling. */
	" vec2 position = gl_FragCoord.xy / metrics;\n"
	" float scale = max(metrics.x/320.0,metrics.y/240.0);\n"
	" vec2 uv = (gl_FragCoord.xy-metrics*0.5)/(vec2(320.0,240.0)*scale)+0.5;\n"
	" vec3 color = picture(uv,scale);\n"
	/* A continuous mask, independent of the old 128x16 mask and central
	 * 640-column feedback texture. Its short trailing glow needs no frame
	 * history, so resize, failure and map transitions cannot retain a frame. */
	" float band = progress*1.4-0.2;\n"
	" float distance = position.x-band;\n"
	" float light = exp(-distance*distance/0.004);\n"
	" float trail = exp(min(distance,0.0)*10.0)*(1.0-smoothstep(0.0,0.05,distance));\n"
	" float vertical = pow(max(sin(position.y*3.14159265),0.0),0.75);\n"
	" float fade = 1.0-smoothstep(0.9,1.0,progress);\n"
	" result = vec4(color*(0.22+(light*1.4+trail*0.65)*vertical)*fade,1.0);\n"
	"}\n";

void loading_widescreen_end(void)
{
	if (xgpu_gl_ready())
	{
		if (loading.texture)
		{
			xgpu_gl_texture_deleted(loading.texture);
			glDeleteTextures(1, &loading.texture);
		}
		if (loading.program) glDeleteProgram(loading.program);
		if (loading.vertex_array) glDeleteVertexArrays(1, &loading.vertex_array);
		if (loading.texture || loading.program || loading.vertex_array)
			xgpu_gl_state_invalidate();
	}
	memset(&loading, 0, sizeof(loading));
}

static int prepare(IDirect3DTexture8 *stock_texture)
{
	D3DSURFACE_DESC description;
	D3DLOCKED_RECT locked;
	GLuint vertex, fragment;

	if (loading.program) return 1;
	if (loading.failed) return 0;
	if (!stock_texture) goto failed;
	IDirect3DTexture8_GetLevelDesc(stock_texture, 0, &description);
	if (description.Width != ART_WIDTH || description.Height != ART_HEIGHT ||
		description.Format != D3DFMT_LIN_A8B8G8R8) goto failed;
	memset(&locked, 0, sizeof(locked));
	IDirect3DTexture8_LockRect(stock_texture, 0, &locked, NULL, D3DLOCK_READONLY);
	if (!locked.pBits || locked.Pitch < ART_WIDTH * 4 || locked.Pitch % 4)
	{
		IDirect3DTexture8_UnlockRect(stock_texture, 0);
		goto failed;
	}
	glGenTextures(1, &loading.texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, loading.texture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, locked.Pitch / 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ART_WIDTH, ART_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, locked.pBits);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	IDirect3DTexture8_UnlockRect(stock_texture, 0);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (!loading.texture || glGetError() != GL_NO_ERROR) goto failed;
	vertex = xgpu_compile_shader(GL_VERTEX_SHADER, vertex_source, "widescreen loading");
	fragment = xgpu_compile_shader(GL_FRAGMENT_SHADER, fragment_source, "widescreen loading");
	if (vertex && fragment)
		loading.program = xgpu_link_program(vertex, fragment, "widescreen loading");
	if (vertex) glDeleteShader(vertex);
	if (fragment) glDeleteShader(fragment);
	if (!loading.program) goto failed;
	glGenVertexArrays(1, &loading.vertex_array);
	if (!loading.vertex_array || glGetError() != GL_NO_ERROR) goto failed;
	loading.metrics = glGetUniformLocation(loading.program, "metrics");
	loading.sweep = glGetUniformLocation(loading.program, "progress");
	glUseProgram(loading.program);
	glUniform1i(glGetUniformLocation(loading.program, "artwork"), 0);
	xgpu_gl_state_invalidate();
	return 1;

failed:
	loading_widescreen_end();
	loading.failed = 1;
	platform_log("widescreen loading: cannot prepare picture; using the stock loading screen");
	return 0;
}

int loading_widescreen_draw(float progress, void *stock_texture)
{
	unsigned long width, height;

	if (!config_boolean("display.widescreen_loading"))
	{
		loading_widescreen_end();
		return 0;
	}
	if (!xgpu_gl_ready() || !prepare(stock_texture)) return 0;
	if (!xgpu_presentation_target(&width, &height)) return 0;
	glViewport(0, 0, width, height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
#ifndef HALO_GLES
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glBindVertexArray(loading.vertex_array);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, loading.texture);
	glBindSampler(0, 0);
	glUseProgram(loading.program);
	glUniform2f(loading.metrics, (float)width, (float)height);
	/* Also handles NaN from a one-frame load whose initial progress is 1. */
	progress = progress < 0.f ? 0.f : !(progress <= 1.f) ? 1.f : progress;
	glUniform1f(loading.sweep, progress);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
	xgpu_gl_state_invalidate();
	return 1;
}
