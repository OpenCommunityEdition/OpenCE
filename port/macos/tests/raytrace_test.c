/*
RAYTRACE_TEST.C

Draws a scene with a known depth buffer (a floor, a wall and a box, with
the game's depth convention: 0 at the near plane, rows from the top) into
textures like the game's targets, runs port/linux/src/raytrace_gl.c on it
through ANGLE, as the macOS port does, and writes the picture before and
after, and the pass's occlusion and depth views, as PPM files.

Built and run by port/macos/tests/run_raytrace_test.sh.
*/

#include <SDL3/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gl.h"
#include "raytrace_gl.h"

#ifdef HALO_MACOS
/* the host's (port/macos/host/host_metal_rt.m, linked in) */
void host_logf(int priority, const char *format, ...)
{
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
	printf("\n");
}
#endif

#define WIDTH 854
#define HEIGHT 480
#define NEAR 0.0625f
#define FAR 1024.0f
#define FIELD_OF_VIEW 1.22f

static const char *ray_tracing_mode = "on";

/* the scene in the game's world: right-handed, z up; the camera at the
origin looks along +y (view x, y up, z forward -> world x, z, y) */
static float world_vertices[4096 * 3];
/* 32-bit, as the guest's unsigned long is (the host reads them so) */
static unsigned int world_indices[4096];

/* the sun: high, from the right and behind the camera */
unsigned char halo_ray_tracing_sun(float *direction)
{
	direction[0] = 0.5f;
	direction[1] = -0.4f;
	direction[2] = 0.77f;
	return 1;
}

unsigned long halo_ray_tracing_world(const float **vertices, long *vertex_count, const unsigned long **indices,
	long *triangle_count);

void platform_log(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
	printf("\n");
}

const char *config_string(const char *name)
{
	return !strcmp(name, "display.ray_tracing") ? ray_tracing_mode : "";
}

double config_real(const char *name)
{
	if (!strcmp(name, "display.ray_tracing_occlusion"))
		return 0.8;
	if (!strcmp(name, "display.ray_tracing_shadows"))
		return 1.0;
	if (!strcmp(name, "display.ray_tracing_reflections"))
		return 0.25;
	return 0.25;
}

static GLuint color_texture, depth_texture, framebuffer;

int xgpu_current_targets(GLuint *color, GLuint *depth, int *width, int *height, int viewport[4])
{
	*color = color_texture;
	*depth = depth_texture;
	*width = WIDTH;
	*height = HEIGHT;
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = WIDTH;
	viewport[3] = HEIGHT;
	return 1;
}

void xgpu_gl_state_invalidate(void)
{
}

void xgpu_gl_bind_device_vertex_array(void)
{
}

static const char scene_vertex[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"in vec3 position;\n"
	"in vec3 color;\n"
	"out vec3 shade;\n"
	"uniform vec4 camera;\n" /* near, far, tan(fov/2), aspect */
	"void main()\n"
	"{\n"
	"	float z = position.z;\n"
	"	vec2 ndc = vec2(position.x / (z * camera.z * camera.w), position.y / (z * camera.z));\n"
	/* the game's depth: 0 at the near plane, 1 at the far one */
	"	float depth = camera.y / (camera.y - camera.x) * (1.0 - camera.x / z);\n"
	/* rows from the top, as the game's targets hold them */
	"	gl_Position = vec4(ndc.x * z, -ndc.y * z, (depth * 2.0 - 1.0) * z, z);\n"
	"	shade = color;\n"
	"}\n";

static const char scene_pixel[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"in vec3 shade;\n"
	"out vec4 result;\n"
	"void main() { result = vec4(shade, 1.0); }\n";

struct vertex { float x, y, z, r, g, b; };

static int vertex_count;
static struct vertex vertices[4096];
static void build_scene(void);
static int vertex_count_scene(void);
static const struct vertex *vertices_scene(void);

static void quad(const float *a, const float *b, const float *c, const float *d, float r, float g, float bl)
{
	const float *corners[6] = { a, b, c, a, c, d };
	int index;

	for (index = 0; index < 6; index++)
	{
		struct vertex *v = &vertices[vertex_count++];

		v->x = corners[index][0];
		v->y = corners[index][1];
		v->z = corners[index][2];
		v->r = r;
		v->g = g;
		v->b = bl;
	}
}

/* view space: x right, y up, z into the screen */
static void build_scene(void)
{
	vertex_count = 0;
	float f0[3] = { -8, -1, 1 }, f1[3] = { 8, -1, 1 }, f2[3] = { 8, -1, 20 }, f3[3] = { -8, -1, 20 };
	float w0[3] = { -8, -1, 12 }, w1[3] = { 8, -1, 12 }, w2[3] = { 8, 5, 12 }, w3[3] = { -8, 5, 12 };
	float s0[3] = { -3, -1, 3 }, s1[3] = { -3, -1, 12 }, s2[3] = { -3, 5, 12 }, s3[3] = { -3, 5, 3 };
	/* a box on the floor */
	float bx0 = 0.5f, bx1 = 2.0f, by0 = -1, by1 = 0.5f, bz0 = 5, bz1 = 6.5f;
	float b000[3] = { bx0, by0, bz0 }, b100[3] = { bx1, by0, bz0 }, b110[3] = { bx1, by1, bz0 }, b010[3] = { bx0, by1, bz0 };
	float b001[3] = { bx0, by0, bz1 }, b101[3] = { bx1, by0, bz1 }, b111[3] = { bx1, by1, bz1 }, b011[3] = { bx0, by1, bz1 };

	quad(f0, f1, f2, f3, 0.55f, 0.55f, 0.5f);
	quad(w0, w1, w2, w3, 0.35f, 0.45f, 0.7f);
	quad(s0, s1, s2, s3, 0.8f, 0.2f, 0.15f);
	quad(b000, b100, b110, b010, 0.9f, 0.8f, 0.3f);
	quad(b010, b110, b111, b011, 0.95f, 0.9f, 0.4f);
	quad(b000, b010, b011, b001, 0.8f, 0.7f, 0.25f);
	quad(b100, b101, b111, b110, 0.8f, 0.7f, 0.25f);
}

static int vertex_count_scene(void)
{
	if (!vertex_count)
		build_scene();
	return vertex_count;
}

static const struct vertex *vertices_scene(void)
{
	return vertices;
}

static GLuint program_from(const char *vertex_source, const char *pixel_source)
{
	GLuint vertex = glCreateShader(GL_VERTEX_SHADER), pixel = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint program = glCreateProgram();

	glShaderSource(vertex, 1, &vertex_source, NULL);
	glCompileShader(vertex);
	glShaderSource(pixel, 1, &pixel_source, NULL);
	glCompileShader(pixel);
	glAttachShader(program, vertex);
	glAttachShader(program, pixel);
	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "color");
	glLinkProgram(program);
	return program;
}

unsigned long halo_ray_tracing_world(const float **vertices, long *vertex_count, const unsigned long **indices,
	long *triangle_count)
{
	int index;

	for (index = 0; index < vertex_count_scene(); index++)
	{
		world_vertices[index * 3 + 0] = vertices_scene()[index].x;
		world_vertices[index * 3 + 1] = vertices_scene()[index].z;
		world_vertices[index * 3 + 2] = vertices_scene()[index].y;
		world_indices[index] = (unsigned int)index;
	}
	/* each triangle facing the camera, counterclockwise from it, as the
	level's face outwards (raytrace_world.c) */
	for (index = 0; index + 2 < vertex_count_scene(); index += 3)
	{
		float *a = &world_vertices[index * 3], *b = a + 3, *c = a + 6;
		float u[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, v[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
		float n[3] = { u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0] };

		if (n[0] * -a[0] + n[1] * -a[1] + n[2] * -a[2] < 0.0f)
		{
			unsigned int swap = world_indices[index + 1];

			world_indices[index + 1] = world_indices[index + 2];
			world_indices[index + 2] = swap;
		}
	}
	*vertices = world_vertices;
	*vertex_count = vertex_count_scene();
	*indices = (const unsigned long *)world_indices;
	*triangle_count = vertex_count_scene() / 3;
	return 1;
}

static void draw_scene(void)
{
	static GLuint program, buffer, array;
	float camera[4] = { NEAR, FAR, tanf(FIELD_OF_VIEW * 0.5f), (float)WIDTH / HEIGHT };

	if (!program)
	{
		program = program_from(scene_vertex, scene_pixel);
		vertex_count_scene();
		glGenVertexArrays(1, &array);
		glBindVertexArray(array);
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(struct vertex), vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void *)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (void *)12);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, WIDTH, HEIGHT);
	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
	glClearDepthf(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);
	glUniform4fv(glGetUniformLocation(program, "camera"), 1, camera);
	glBindVertexArray(array);
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}

static void save(const char *path)
{
	static unsigned char pixels[WIDTH * HEIGHT * 4];
	FILE *file = fopen(path, "wb");
	int y, x;

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	fprintf(file, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
	/* the targets' rows run from the top */
	for (y = 0; y < HEIGHT; y++)
		for (x = 0; x < WIDTH; x++)
			fwrite(&pixels[(y * WIDTH + x) * 4], 1, 3, file);
	fclose(file);
	printf("wrote %s\n", path);
}

static const float camera_position[3] = { 0, 0, 0 }, camera_forward[3] = { 0, 1, 0 }, camera_up[3] = { 0, 0, 1 };

int main(int argc, char **argv)
{
	SDL_Window *window;
	SDL_GLContext context;
	char path[1024];

	if (argc < 2)
	{
		fprintf(stderr, "usage: raytrace_test <angle folder>\n");
		return 2;
	}
	snprintf(path, sizeof(path), "%s/libGLESv2.dylib", argv[1]);
	SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, path);
	snprintf(path, sizeof(path), "%s/libEGL.dylib", argv[1]);
	SDL_SetHint(SDL_HINT_EGL_LIBRARY, path);
	setenv("ANGLE_DEFAULT_PLATFORM", "metal", 0);
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	window = SDL_CreateWindow("raytrace test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
	context = window ? SDL_GL_CreateContext(window) : NULL;
	if (!context || !gl_functions_load())
	{
		fprintf(stderr, "no OpenGL ES context: %s\n", SDL_GetError());
		return 1;
	}
	printf("%s\n", (const char *)glGetString(GL_RENDERER));
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_2D, color_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenTextures(1, &depth_texture);
	glBindTexture(GL_TEXTURE_2D, depth_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

	draw_scene();
	save("raytrace_before.ppm");
	halo_ray_traced_lighting(NEAR, FAR, FIELD_OF_VIEW, camera_position, camera_forward, camera_up);
	save("raytrace_after.ppm");
	/* the pass's views: occlusion, then depth (F9 turns it off and on, and
	the mode is read again) */
	draw_scene();
	halo_ray_tracing_debug_mode(2);
	halo_ray_traced_lighting(NEAR, FAR, FIELD_OF_VIEW, camera_position, camera_forward, camera_up);
	save("raytrace_occlusion.ppm");
	draw_scene();
	halo_ray_tracing_debug_mode(3);
	halo_ray_traced_lighting(NEAR, FAR, FIELD_OF_VIEW, camera_position, camera_forward, camera_up);
	save("raytrace_depth.ppm");
#ifdef HALO_MACOS
	{
		/* the depth and normals the rays start from, and the rays'
		results, across row 150 */
		extern int host_rt_debug_read(int which, int x, int y, int count, float *values);
		static float row[WIDTH * 4], results[WIDTH * 4];
		int x;

		host_rt_debug_read(0, 0, 166, WIDTH / 2, row);
		host_rt_debug_read(1, 0, 166, WIDTH / 2, results);
		for (x = 60; x < 130; x += 8)
			printf("half row 166 x %d: z %.4f normal %.3f %.3f %.3f  visibility %.3f\n", x, row[x * 4], row[x * 4 + 1],
				row[x * 4 + 2], row[x * 4 + 3], results[x * 4]);
	}
#endif
	printf("GL error %#x\n", glGetError());
	SDL_Quit();
	return 0;
}
