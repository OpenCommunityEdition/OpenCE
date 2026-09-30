/*
RAYTRACE_GL.C

Ray-traced lighting for the native ports (display.ray_tracing): rays
through the screen's depth on every port, and on macOS also rays through
the level itself with Metal (port/macos/host/host_metal_rt.m).

Once the game has drawn a window's opaque world, before its transparent
geometry, fog, effects and HUD (source/render/render.c), rays are marched
through that window's depth buffer, the scene's geometry as the camera
sees it:

- ambient occlusion: from each pixel, rays across the hemisphere around the
  surface's normal (from the depth's neighbours); what they hit close by
  darkens the pixel, as creases, corners and the ground under objects are
  in the real world;
- one bounce of indirect light: the colour a ray hits lights the pixel a
  little, so a red wall tints the floor beside it;
- reflections: a ray along the view's mirror direction; where it hits, the
  surface reflects that colour, by the Fresnel term (glancing angles
  reflect most), so floors and wet ground pick up the scene.

Rays through the screen's depth find only what the camera sees: rays
leaving the screen find nothing, and are faded out. On macOS, where Metal
can trace rays (in compute on M1 and M2, in the ray tracing hardware of the
M3 and later), the occlusion and reflections are also traced through the
level's own geometry (its collision surfaces,
port/linux/game/raytrace_world.c), which finds what the camera does not
see; the screen's rays still find the objects, which the level does not
hold. The pass costs a few milliseconds at the display's resolution.

It runs as two draws: rays into an effect texture (occlusion, reflection),
then the composite back into the window, with a depth-aware blur of the
occlusion. F9 switches it on and off while playing.
*/

#include "platform.h"
#include "gl.h"
#include "port_config.h"
#include "raytrace_gl.h"
#ifdef HALO_MACOS
/* the host's Metal ray tracing (port/macos/host/host_metal_rt.m) */
#include "guest_host_desktop.h"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

/* port/linux/game/raytrace_world.c: the level's triangles */
unsigned long halo_ray_tracing_world(const float **vertices, long *vertex_count, const unsigned long **indices,
	long *triangle_count);
/* the direction towards the sky's sun; 0 if none */
unsigned char halo_ray_tracing_sun(float *direction);

/* d3d8_gl.c: the window's current targets and viewport, in GL pixels */
int xgpu_current_targets(GLuint *color, GLuint *depth, int *width, int *height, int viewport[4]);
void xgpu_gl_state_invalidate(void);
void xgpu_gl_bind_device_vertex_array(void);

enum
{
	_ray_tracing_off,
	_ray_tracing_on,
	_ray_tracing_debug_occlusion,
	_ray_tracing_debug_depth,
};

static struct
{
	int initialized;
	int failed;
	int mode;
	int enabled;
	float occlusion_strength, reflection_strength, bounce_strength, shadow_strength, radius;
	GLuint trace_program, composite_program;
	GLint trace_uniforms, composite_uniforms;
	GLint trace_scene, trace_depth, composite_scene, composite_depth, composite_effect;
	GLint composite_debug, composite_rt, composite_rt_enabled;
	/* the world-space rays (macOS: Metal) */
	int hardware;
	GLuint gbuffer_program, gbuffer_framebuffer;
	GLint gbuffer_uniforms, gbuffer_depth;
	unsigned long world_generation;
	GLuint vertex_array;
	GLuint scene_texture, effect_texture;
	GLuint scene_framebuffer, effect_framebuffer, output_framebuffer, source_framebuffer;
	GLuint output_texture;
	int width, height;
	unsigned long frame;
} ray;

#ifdef HALO_GLES
#define SHADER_HEADER "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n"
#else
#define SHADER_HEADER "#version 330 core\n"
#endif

static const char vertex_source[] =
	SHADER_HEADER
	"void main()\n"
	"{\n"
	"	vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
	"	gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);\n"
	"}\n";

/* the shared part: positions from depth. u[0] = (near, far, tan of half
the vertical field of view, aspect), u[1] = (viewport x, y, width, height),
u[2] = (radius, occlusion, reflection, bounce), u[3] = (frame, texture
width, texture height, 0) */
/* the rays are traced at half the resolution (every other pixel each
way), and the composite blends them back up across edges by depth */
#define TRACE_SCALE 2
#define TRACE_SCALE_TEXT "2"

#define COMMON_SOURCE \
	"#define TRACE_SCALE " TRACE_SCALE_TEXT "\n" \
	"uniform vec4 u[4];\n" \
	"uniform sampler2D depth_texture;\n" \
	"float linear_depth(float d)\n" \
	"{\n" \
	"	float n = u[0].x, f = u[0].y;\n" \
	"	return n * f / (f - d * (f - n));\n" \
	"}\n" \
	"float depth_at(ivec2 p)\n" \
	"{\n" \
	"	p = clamp(p, ivec2(u[1].xy), ivec2(u[1].xy + u[1].zw) - 1);\n" \
	"	return texelFetch(depth_texture, p, 0).r;\n" \
	"}\n" \
	"vec3 view_position(vec2 pixel, float z)\n" \
	"{\n" \
	"	vec2 ndc = (pixel - u[1].xy) / u[1].zw * 2.0 - 1.0;\n" \
	"	return vec3(ndc.x * u[0].z * u[0].w * z, ndc.y * u[0].z * z, z);\n" \
	"}\n" \
	"vec3 position_at(ivec2 p)\n" \
	"{\n" \
	"	return view_position(vec2(p) + 0.5, linear_depth(depth_at(p)));\n" \
	"}\n" \
	"vec2 project(vec3 v)\n" \
	"{\n" \
	"	vec2 ndc = vec2(v.x / (v.z * u[0].z * u[0].w), v.y / (v.z * u[0].z));\n" \
	"	return u[1].xy + (ndc * 0.5 + 0.5) * u[1].zw;\n" \
	"}\n"

static const char trace_source[] =
	SHADER_HEADER
	COMMON_SOURCE
	"uniform sampler2D scene_texture;\n"
	"out vec4 result;\n"
	"float noise(vec2 p)\n"
	"{\n"
	"	return fract(52.9829189 * fract(dot(p + u[3].x * vec2(5.588238, 7.231), vec2(0.06711056, 0.00583715))));\n"
	"}\n"
	"void main()\n"
	"{\n"
	"	ivec2 p = ivec2(gl_FragCoord.xy) * TRACE_SCALE;\n"
	"	float d = depth_at(p);\n"
	"	if (d >= 0.99999) { result = vec4(0.0, 0.0, 0.0, 1.0); return; }\n"
	"	vec3 P = position_at(p);\n"
	/* the normal: the flatter of the two differences on each axis, so
	   edges do not bend it */
	"	vec3 l = position_at(p - ivec2(1, 0)), r = position_at(p + ivec2(1, 0));\n"
	"	vec3 b = position_at(p - ivec2(0, 1)), t = position_at(p + ivec2(0, 1));\n"
	"	vec3 dx = abs(r.z - P.z) < abs(P.z - l.z) ? r - P : P - l;\n"
	"	vec3 dy = abs(t.z - P.z) < abs(P.z - b.z) ? t - P : P - b;\n"
	"	vec3 N = normalize(cross(dy, dx));\n"
	"	if (dot(N, P) > 0.0) N = -N;\n"
	"	float jitter = noise(gl_FragCoord.xy);\n"
	/* occlusion and bounce: 8 directions around the normal, 4 steps each,
	   within the radius (screen-space size from the depth) */
	"	float radius = u[2].x;\n"
	"	float pixels = radius / (P.z * u[0].z) * u[1].w * 0.5;\n"
	"	pixels = clamp(pixels, 2.0, 96.0);\n"
	"	float occlusion = 0.0;\n"
	"	vec3 bounce = vec3(0.0);\n"
	"	for (int i = 0; i < 8; i++)\n"
	"	{\n"
	"		float angle = (float(i) + jitter) * 0.78539816;\n"
	"		vec2 direction = vec2(cos(angle), sin(angle));\n"
	"		float horizon = 0.0;\n"
	"		for (int j = 1; j <= 4; j++)\n"
	"		{\n"
	"			float step_length = pixels * (float(j) - 0.5 + 0.5 * jitter) / 4.0;\n"
	"			ivec2 q = p + ivec2(direction * step_length);\n"
	"			vec3 S = position_at(q);\n"
	"			vec3 v = S - P;\n"
	"			float distance_squared = dot(v, v);\n"
	"			float cosine = dot(N, v) * inversesqrt(distance_squared + 1e-6);\n"
	"			float falloff = clamp(1.0 - distance_squared / (radius * radius), 0.0, 1.0);\n"
	"			float h = max(cosine - 0.1, 0.0) * falloff;\n"
	"			if (h > horizon)\n"
	"			{\n"
	"				bounce += texelFetch(scene_texture, clamp(q, ivec2(u[1].xy), ivec2(u[1].xy + u[1].zw) - 1), 0).rgb"
	" * (h - horizon);\n"
	"				horizon = h;\n"
	"			}\n"
	"		}\n"
	"		occlusion += horizon;\n"
	"	}\n"
	"	occlusion = clamp(occlusion / 8.0 * 1.6, 0.0, 1.0);\n"
	"	bounce /= 8.0;\n"
	/* reflection: march the mirror ray in view space */
	"	vec3 V = normalize(P);\n"
	"	vec3 R = reflect(V, N);\n"
	"	float fresnel = pow(1.0 - clamp(dot(-V, N), 0.0, 1.0), 5.0);\n"
	"	vec3 reflection = vec3(0.0);\n"
	"	float reflected = 0.0;\n"
	"	if (u[2].z > 0.0 && R.z > -0.5)\n"
	"	{\n"
	"		float travel = 0.02 * P.z * (1.0 + jitter);\n"
	"		vec3 ray = P + N * 0.01 * P.z;\n"
	"		for (int k = 0; k < 24; k++)\n"
	"		{\n"
	"			ray += R * travel;\n"
	"			travel *= 1.25;\n"
	"			if (ray.z <= u[0].x) break;\n"
	"			vec2 screen = project(ray);\n"
	"			if (any(lessThan(screen, u[1].xy)) || any(greaterThanEqual(screen, u[1].xy + u[1].zw))) break;\n"
	"			float scene_z = linear_depth(depth_at(ivec2(screen)));\n"
	"			float behind = ray.z - scene_z;\n"
	"			if (behind > 0.0 && behind < travel * 2.0 + 0.05 * scene_z)\n"
	"			{\n"
	"				vec2 edge = min(screen - u[1].xy, u[1].xy + u[1].zw - screen) / (u[1].zw * 0.1);\n"
	"				float fade = clamp(min(edge.x, edge.y), 0.0, 1.0) * (1.0 - float(k) / 24.0);\n"
	"				reflection = texelFetch(scene_texture, ivec2(screen), 0).rgb;\n"
	"				reflected = fade;\n"
	"				break;\n"
	"			}\n"
	"		}\n"
	"	}\n"
	"	float weight = reflected * mix(0.04, 1.0, fresnel) * u[2].z;\n"
	"	vec3 light = reflection * weight + bounce * u[2].w;\n"
	"	result = vec4(light, 1.0 - occlusion * u[2].y);\n"
	"}\n";

/* each pixel's linear depth and view-space normal, for the world-space
rays: x, y, z of the normal with x right, y down, z into the screen */
static const char gbuffer_source[] =
	SHADER_HEADER
	COMMON_SOURCE
	"out vec4 result;\n"
	"void main()\n"
	"{\n"
	"	ivec2 p = ivec2(gl_FragCoord.xy) * TRACE_SCALE;\n"
	"	float d = depth_at(p);\n"
	"	if (d >= 0.99999) { result = vec4(0.0); return; }\n"
	"	vec3 P = position_at(p);\n"
	"	vec3 l = position_at(p - ivec2(1, 0)), r = position_at(p + ivec2(1, 0));\n"
	"	vec3 b = position_at(p - ivec2(0, 1)), t = position_at(p + ivec2(0, 1));\n"
	"	vec3 dx = abs(r.z - P.z) < abs(P.z - l.z) ? r - P : P - l;\n"
	"	vec3 dy = abs(t.z - P.z) < abs(P.z - b.z) ? t - P : P - b;\n"
	"	vec3 N = normalize(cross(dy, dx));\n"
	"	if (dot(N, P) > 0.0) N = -N;\n"
	"	result = vec4(P.z, N);\n"
	"}\n";

static const char composite_source[] =
	SHADER_HEADER
	COMMON_SOURCE
	"uniform sampler2D scene_texture;\n"
	"uniform sampler2D effect_texture;\n"
	"uniform sampler2D rt_texture;\n"
	"uniform int rt_enabled;\n"
	"uniform int debug_mode;\n"
	"out vec4 result;\n"
	"void main()\n"
	"{\n"
	"	ivec2 p = ivec2(gl_FragCoord.xy);\n"
	"	vec4 scene = texelFetch(scene_texture, p, 0);\n"
	"	float d = depth_at(p);\n"
	"	if (debug_mode == 3) { float z = linear_depth(d); result = vec4(vec3(fract(z / 10.0)), 1.0); return; }\n"
	"	if (d >= 0.99999) { result = scene; return; }\n"
	/* the occlusion blurred over 4x4 pixels of similar depth */
	"	float z = linear_depth(d);\n"
	"	float total = 0.0, visibility = 0.0;\n"
	"	vec3 light = vec3(0.0);\n"
	"	for (int y = -2; y < 2; y++)\n"
	"		for (int x = -2; x < 2; x++)\n"
	"		{\n"
	"			ivec2 q = clamp(p + ivec2(x, y), ivec2(u[1].xy), ivec2(u[1].xy + u[1].zw) - 1);\n"
	"			float w = 1.0 / (1e-3 + abs(linear_depth(texelFetch(depth_texture, q, 0).r) - z) / z * 40.0);\n"
	"			vec4 e = texelFetch(effect_texture, q / TRACE_SCALE, 0);\n"
	"			float v = e.a;\n"
	/* the world's occlusion (Metal's rays) with the screen's (which also
	   finds the objects) */
	"			if (rt_enabled != 0) v *= mix(1.0, texelFetch(rt_texture, q / TRACE_SCALE, 0).r, u[2].y);\n"
	"			visibility += v * w;\n"
	"			light += e.rgb * w;\n"
	"			total += w;\n"
	"		}\n"
	"	visibility /= total;\n"
	"	light /= total;\n"
	/* a traced reflection, where the camera sees what it hit, before the
	   screen's */
	"	if (rt_enabled != 0)\n"
	"	{\n"
	"		vec4 hit = texelFetch(rt_texture, p / TRACE_SCALE, 0);\n"
	/* (a: how much, with the Fresnel term; the screen's rays traced no
	   reflection: u[2].z was 0 for them) */
	"		if (hit.a > 0.0)\n"
	"			light += texelFetch(scene_texture, ivec2(hit.gb * u[3].yz), 0).rgb * hit.a * u[3].w;\n"
	"	}\n"
	"	if (debug_mode == 2) { result = vec4(vec3(visibility), 1.0); return; }\n"
	"	result = vec4(scene.rgb * visibility + light * (1.0 - scene.rgb * 0.5), scene.a);\n"
	"}\n";

static GLuint compile(GLenum type, const char *source, const char *what)
{
	GLuint shader = glCreateShader(type);
	GLint status = 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		platform_log("ray tracing: cannot compile the %s shader:\n%s", what, log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint link(const char *fragment, const char *what)
{
	GLuint vertex = compile(GL_VERTEX_SHADER, vertex_source, "full-screen vertex");
	GLuint pixel = compile(GL_FRAGMENT_SHADER, fragment, what);
	GLuint program;
	GLint status = 0;

	if (!vertex || !pixel)
		return 0;
	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, pixel);
	glLinkProgram(program);
	glDeleteShader(vertex);
	glDeleteShader(pixel);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status)
	{
		char log[4096];

		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		platform_log("ray tracing: cannot link the %s program:\n%s", what, log);
		return 0;
	}
	return program;
}

static int mode_from_setting(const char *text)
{
	if (!strcmp(text, "off") || !strcmp(text, "false"))
		return _ray_tracing_off;
	if (!strcmp(text, "occlusion"))
		return _ray_tracing_debug_occlusion;
	if (!strcmp(text, "depth"))
		return _ray_tracing_debug_depth;
	if (!strcmp(text, "screen"))
		return _ray_tracing_on;
	return _ray_tracing_on;
}

static void initialize(void)
{
	ray.initialized = 1;
	ray.mode = mode_from_setting(config_string("display.ray_tracing"));
	ray.enabled = ray.mode != _ray_tracing_off;
	ray.occlusion_strength = (float)config_real("display.ray_tracing_occlusion");
	ray.reflection_strength = (float)config_real("display.ray_tracing_reflections");
	ray.bounce_strength = (float)config_real("display.ray_tracing_bounce");
	ray.shadow_strength = (float)config_real("display.ray_tracing_shadows");
	/* world units (a world unit is about 3 m) */
	ray.radius = 0.35f;
	ray.trace_program = link(trace_source, "ray tracing");
	ray.composite_program = link(composite_source, "ray tracing composite");
	if (!ray.trace_program || !ray.composite_program)
	{
		ray.failed = 1;
		platform_log("ray tracing: unavailable");
		return;
	}
	ray.trace_uniforms = glGetUniformLocation(ray.trace_program, "u");
	ray.trace_scene = glGetUniformLocation(ray.trace_program, "scene_texture");
	ray.trace_depth = glGetUniformLocation(ray.trace_program, "depth_texture");
	ray.composite_uniforms = glGetUniformLocation(ray.composite_program, "u");
	ray.composite_scene = glGetUniformLocation(ray.composite_program, "scene_texture");
	ray.composite_depth = glGetUniformLocation(ray.composite_program, "depth_texture");
	ray.composite_effect = glGetUniformLocation(ray.composite_program, "effect_texture");
	ray.composite_debug = glGetUniformLocation(ray.composite_program, "debug_mode");
	ray.composite_rt = glGetUniformLocation(ray.composite_program, "rt_texture");
	ray.composite_rt_enabled = glGetUniformLocation(ray.composite_program, "rt_enabled");
#ifdef HALO_MACOS
	/* "screen" keeps to the screen's rays */
	if (strcmp(config_string("display.ray_tracing"), "screen") && host_rt_available())
	{
		ray.gbuffer_program = link(gbuffer_source, "ray tracing depth and normals");
		if (ray.gbuffer_program)
		{
			ray.gbuffer_uniforms = glGetUniformLocation(ray.gbuffer_program, "u");
			ray.gbuffer_depth = glGetUniformLocation(ray.gbuffer_program, "depth_texture");
			glGenFramebuffers(1, &ray.gbuffer_framebuffer);
			ray.hardware = 1;
		}
	}
#endif
	glGenVertexArrays(1, &ray.vertex_array);
	glGenFramebuffers(1, &ray.scene_framebuffer);
	glGenFramebuffers(1, &ray.effect_framebuffer);
	glGenFramebuffers(1, &ray.output_framebuffer);
	glGenFramebuffers(1, &ray.source_framebuffer);
	platform_log("ray tracing: %s, %s (F9 switches it; occlusion %.2f, reflections %.2f, bounce %.2f)",
		ray.enabled ? "on" : "off", ray.hardware ? "world-space rays (Metal) with the screen's" : "the screen's rays",
		ray.occlusion_strength, ray.reflection_strength, ray.bounce_strength);
}

static GLuint make_texture(int width, int height)
{
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	return texture;
}

/* the scene copy and the effect texture, at the targets' size */
static void size_textures(int width, int height)
{
	if (ray.width == width && ray.height == height)
		return;
	if (ray.scene_texture)
	{
		glDeleteTextures(1, &ray.scene_texture);
		glDeleteTextures(1, &ray.effect_texture);
	}
	ray.scene_texture = make_texture(width, height);
	ray.effect_texture = make_texture((width + TRACE_SCALE - 1) / TRACE_SCALE, (height + TRACE_SCALE - 1) / TRACE_SCALE);
	ray.width = width;
	ray.height = height;
	glBindFramebuffer(GL_FRAMEBUFFER, ray.scene_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ray.scene_texture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, ray.effect_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ray.effect_texture, 0);
}

const char *halo_ray_tracing_toggle(void)
{
	if (!ray.initialized)
		initialize();
	ray.enabled = !ray.enabled;
	if (ray.enabled && ray.mode == _ray_tracing_off)
		ray.mode = _ray_tracing_on;
	platform_log("ray tracing: %s", ray.enabled ? "on" : "off");
	if (!ray.enabled)
		return "off";
	if (ray.failed)
		return "unavailable";
	return ray.hardware ? "on (Metal: level rays, sun shadows, screen rays)" : "on (screen rays)";
}

/* what it shows: 1 the lighting, 2 the occlusion, 3 the depth (tests) */
void halo_ray_tracing_debug_mode(int mode)
{
	if (!ray.initialized)
		initialize();
	ray.mode = mode;
	ray.enabled = mode != _ray_tracing_off;
}

#ifdef HALO_MACOS
/* the level's rays with Metal: the depth and normals into the shared
texture, the rays, and the results' texture; 0 if not */
static GLuint world_rays(const float *uniforms, const float *position, const float *forward, const float *up,
	int width, int height, GLuint depth)
{
	const GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
	const float *vertices;
	const unsigned long *indices;
	long vertex_count, triangle_count;
	unsigned long generation;
	GLuint input, output;
	float camera[28], right[3], length;

	generation = halo_ray_tracing_world(&vertices, &vertex_count, &indices, &triangle_count);
	if (!generation)
		return 0;
	if (generation != ray.world_generation)
	{
		ray.world_generation = generation;
		host_rt_set_world(generation, vertices, (int)vertex_count, (const unsigned int *)indices, (int)triangle_count);
	}
	width = (width + TRACE_SCALE - 1) / TRACE_SCALE;
	height = (height + TRACE_SCALE - 1) / TRACE_SCALE;
	input = host_rt_texture(0, width, height);
	output = host_rt_texture(1, width, height);
	/* (the host's texture creation binds on the active unit) */
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depth);
	if (!input || !output)
		return 0;
	glBindFramebuffer(GL_FRAMEBUFFER, ray.gbuffer_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, input, 0);
	glDrawBuffers(1, &draw_buffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		platform_log("ray tracing: the shared depth texture cannot be drawn to; world-space rays off");
		ray.hardware = 0;
		return 0;
	}
	glUseProgram(ray.gbuffer_program);
	glUniform4fv(ray.gbuffer_uniforms, 4, uniforms);
	glUniform1i(ray.gbuffer_depth, 1);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	/* the camera: its right is forward x up (the game's world is
	right-handed, z up) */
	right[0] = forward[1] * up[2] - forward[2] * up[1];
	right[1] = forward[2] * up[0] - forward[0] * up[2];
	right[2] = forward[0] * up[1] - forward[1] * up[0];
	length = sqrtf(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
	if (length <= 0.0f)
		return 0;
	memcpy(camera, position, 3 * sizeof(float));
	memcpy(camera + 3, forward, 3 * sizeof(float));
	memcpy(camera + 6, up, 3 * sizeof(float));
	camera[9] = right[0] / length;
	camera[10] = right[1] / length;
	camera[11] = right[2] / length;
	camera[12] = uniforms[0];
	camera[13] = uniforms[1];
	camera[14] = uniforms[2];
	camera[15] = uniforms[3];
	/* the viewport on the half-resolution grid */
	camera[16] = uniforms[4] / TRACE_SCALE;
	camera[17] = uniforms[5] / TRACE_SCALE;
	camera[18] = uniforms[6] / TRACE_SCALE;
	camera[19] = uniforms[7] / TRACE_SCALE;
	camera[20] = (float)(ray.frame & 1023);
	/* world units: about 3 m each */
	camera[21] = 0.6f;
	camera[22] = 40.0f;
	camera[23] = 0.0f;
	/* the sun, for shadows on the objects */
	camera[27] = halo_ray_tracing_sun(camera + 24) ? ray.shadow_strength : 0.0f;
	if (!host_rt_trace(camera, width, height))
		return 0;
	return output;
}
#endif

void halo_ray_traced_lighting(float z_near, float z_far, float vertical_field_of_view, const float *position,
	const float *forward, const float *up)
{
	GLuint color, depth, world_results = 0;
	int width, height, viewport[4];
	float uniforms[16];
	const GLenum draw_buffer = GL_COLOR_ATTACHMENT0;

	if (!ray.initialized)
		initialize();
	if (!ray.enabled || ray.failed || !(z_near > 0.0f) || !(z_far > z_near) || !(vertical_field_of_view > 0.0f))
		return;
	if (!xgpu_current_targets(&color, &depth, &width, &height, viewport) || !color || !depth ||
		viewport[2] < 16 || viewport[3] < 16)
	{
		return;
	}
	ray.frame++;
	size_textures(width, height);

	/* the window's colour, copied: it is read and written */
	glBindFramebuffer(GL_READ_FRAMEBUFFER, ray.source_framebuffer);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ray.scene_framebuffer);
	glBlitFramebuffer(viewport[0], viewport[1], viewport[0] + viewport[2], viewport[1] + viewport[3],
		viewport[0], viewport[1], viewport[0] + viewport[2], viewport[1] + viewport[3],
		GL_COLOR_BUFFER_BIT, GL_NEAREST);

	uniforms[0] = z_near;
	uniforms[1] = z_far;
	uniforms[2] = tanf(vertical_field_of_view * 0.5f);
	uniforms[3] = (float)viewport[2] / (float)viewport[3];
	uniforms[4] = (float)viewport[0];
	uniforms[5] = (float)viewport[1];
	uniforms[6] = (float)viewport[2];
	uniforms[7] = (float)viewport[3];
	uniforms[8] = ray.radius;
	uniforms[9] = ray.occlusion_strength;
	uniforms[10] = ray.reflection_strength;
	uniforms[11] = ray.bounce_strength;
	uniforms[12] = (float)(ray.frame & 63);
	uniforms[13] = (float)width;
	uniforms[14] = (float)height;
	/* the reflections' strength for the composite; the screen's rays
	trace none when Metal's do */
	uniforms[15] = ray.reflection_strength;
	if (ray.hardware)
		uniforms[10] = 0.0f;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glEnable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	/* the rays on the half-resolution grid */
	glViewport(0, 0, (width + TRACE_SCALE - 1) / TRACE_SCALE, (height + TRACE_SCALE - 1) / TRACE_SCALE);
	glScissor(viewport[0] / TRACE_SCALE, viewport[1] / TRACE_SCALE, (viewport[2] + TRACE_SCALE - 1) / TRACE_SCALE,
		(viewport[3] + TRACE_SCALE - 1) / TRACE_SCALE);
	glBindVertexArray(ray.vertex_array);

	/* the rays */
	glBindFramebuffer(GL_FRAMEBUFFER, ray.effect_framebuffer);
	glDrawBuffers(1, &draw_buffer);
	glUseProgram(ray.trace_program);
	glUniform4fv(ray.trace_uniforms, 4, uniforms);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ray.scene_texture);
	glBindSampler(0, 0);
	glUniform1i(ray.trace_scene, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depth);
	glBindSampler(1, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glUniform1i(ray.trace_depth, 1);
	glDrawArrays(GL_TRIANGLES, 0, 3);

#ifdef HALO_MACOS
	if (ray.hardware && position && forward && up)
		world_results = world_rays(uniforms, position, forward, up, width, height, depth);
#else
	(void)position;
	(void)forward;
	(void)up;
#endif

	/* the composite, into the window's colour at its full resolution (its
	depth not attached, being read) */
	glViewport(0, 0, width, height);
	glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, ray.output_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
	glDrawBuffers(1, &draw_buffer);
	glUseProgram(ray.composite_program);
	glUniform4fv(ray.composite_uniforms, 4, uniforms);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ray.scene_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depth);
	glUniform1i(ray.composite_scene, 0);
	glUniform1i(ray.composite_depth, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ray.effect_texture);
	glBindSampler(2, 0);
	glUniform1i(ray.composite_effect, 2);
	glUniform1i(ray.composite_debug, ray.mode);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, world_results);
	glBindSampler(3, 0);
	glUniform1i(ray.composite_rt, 3);
	glUniform1i(ray.composite_rt_enabled, world_results != 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	/* the renderer's state is its own again */
	glActiveTexture(GL_TEXTURE0);
	xgpu_gl_bind_device_vertex_array();
	xgpu_gl_state_invalidate();
}
