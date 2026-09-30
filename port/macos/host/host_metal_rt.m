/*
HOST_METAL_RT.M

World-space ray tracing for the macOS port's lighting
(port/linux/src/raytrace_gl.c), with Metal's ray tracing: in compute on
M1 and M2, in the GPU's ray tracing hardware on M3 and later.

- The level: port/linux/game/raytrace_world.c gives the active BSP's
  collision surfaces as triangles (host_rt_set_world), and they become a
  Metal acceleration structure, rebuilt when the BSP changes.
- The frame: the guest draws each pixel's linear depth and normal into a
  texture it shares with Metal (host_rt_texture: a Metal texture that
  ANGLE, which draws the game on the same Metal device, takes as a GL
  texture through EGL_ANGLE_metal_texture_client_buffer).
- The rays (host_rt_trace): from each pixel's point in the world, four rays
  over the hemisphere around its normal, which find the level's geometry
  near it wherever it is, on screen or not (ambient occlusion), and one
  along the view's reflection, whose hit is projected back onto the screen
  for its colour if the camera sees it there. The results go to a second
  shared texture, which the guest composites.

GL and Metal take turns on the GPU, through a Metal shared event
(EGL_ANGLE_metal_shared_event_sync): GL signals it when the depth and
normals are drawn, the rays wait for that and signal it when done, and GL
waits for that before it reads the results. The CPU waits for neither, so
it prepares the next frame while the GPU traces. Without the extension,
glFinish before the rays and the command buffer completed before GL
reads the results.
*/

#include "host.h"

#import <Metal/Metal.h>
#include <SDL3/SDL.h>
#include <dlfcn.h>
#include <math.h>
#include <string.h>

/* EGL and GL, as ANGLE has them (SDL loaded both) */
typedef void *EGLDisplay_;
typedef void *EGLImage_;
typedef intptr_t EGLAttrib_;
typedef int EGLint_;
#define EGL_NO_CONTEXT_ ((void *)0)
#define EGL_DEVICE_EXT_ 0x322C
#define EGL_METAL_DEVICE_ANGLE_ 0x34A6
#define EGL_METAL_TEXTURE_ANGLE_ 0x34A7
#define EGL_NONE_ 0x3038
#define EGL_EXTENSIONS_ 0x3055
#define EGL_SYNC_CONDITION_ 0x30F8
#define EGL_SYNC_METAL_SHARED_EVENT_ANGLE_ 0x34D8
#define EGL_SYNC_METAL_SHARED_EVENT_OBJECT_ANGLE_ 0x34D9
#define EGL_SYNC_METAL_SHARED_EVENT_SIGNAL_VALUE_LO_ANGLE_ 0x34DA
#define EGL_SYNC_METAL_SHARED_EVENT_SIGNAL_VALUE_HI_ANGLE_ 0x34DB
#define EGL_SYNC_METAL_SHARED_EVENT_SIGNALED_ANGLE_ 0x34DC
#define GL_TEXTURE_2D_ 0x0DE1
#define GL_TEXTURE_MIN_FILTER_ 0x2801
#define GL_TEXTURE_MAG_FILTER_ 0x2800
#define GL_NEAREST_ 0x2600

#define HOST_RT_MAXIMUM_OBJECTS 511

static struct
{
	int checked, available;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLComputePipelineState> pipeline;
	id<MTLAccelerationStructure> world;
	unsigned int world_generation;
	/* the objects (host_rt_set_objects): a unit sphere, stretched and placed
	by each instance's transform, and the scene of the level and them,
	rebuilt each frame (its instance lists in a ring: the GPU may still read
	the last ones) */
	id<MTLAccelerationStructure> blob, scene;
	id<MTLBuffer> instance_buffers[3], scene_scratch;
	int instance_ring;
	float object_transforms[HOST_RT_MAXIMUM_OBJECTS * 12];
	unsigned char object_masks[HOST_RT_MAXIMUM_OBJECTS];
	int object_count;
	id<MTLTexture> textures[2];
	unsigned int gl_textures[2];
	EGLImage_ images[2];
	int widths[2], heights[2];
	EGLDisplay_ display;
	EGLDisplay_ (*eglGetCurrentDisplay)(void);
	EGLImage_ (*eglCreateImageKHR)(EGLDisplay_, void *, unsigned int, void *, const EGLint_ *);
	unsigned int (*eglDestroyImageKHR)(EGLDisplay_, EGLImage_);
	unsigned int (*eglQueryDisplayAttribEXT)(EGLDisplay_, EGLint_, EGLAttrib_ *);
	unsigned int (*eglQueryDeviceAttribEXT)(void *, EGLint_, EGLAttrib_ *);
	void (*glGenTextures)(int, unsigned int *);
	void (*glDeleteTextures)(int, const unsigned int *);
	void (*glBindTexture)(unsigned int, unsigned int);
	void (*glTexParameteri)(unsigned int, unsigned int, int);
	void (*glEGLImageTargetTexture2DOES)(unsigned int, void *);
	void (*glFinish)(void);
	void (*glFlush)(void);
	/* the GPU-side turns: the event and its last value */
	id<MTLSharedEvent> event;
	uint64_t event_value;
	const char *(*eglQueryString)(EGLDisplay_, EGLint_);
	void *(*eglCreateSync)(EGLDisplay_, unsigned int, const EGLAttrib_ *);
	unsigned int (*eglDestroySync)(EGLDisplay_, void *);
	unsigned int (*eglWaitSync)(EGLDisplay_, void *, EGLint_);
} rt;

static NSString *const kernel_source = @
	"#include <metal_stdlib>\n"
	"#include <metal_raytracing>\n"
	"using namespace metal;\n"
	"using namespace raytracing;\n"
	/* c: position 0-2, forward 3-5, up 6-8, right 9-11, near 12, far 13,
	   tan of half the vertical field of view 14, aspect 15, viewport 16-19,
	   frame 20, occlusion radius 21, reflection distance 22, whether the
	   objects' pixels are known 23 (then their depth is negative), the
	   direction to the sun 24-26 and whether there is one 27 */
	"kernel void trace(texture2d<float, access::read> gbuffer [[texture(0)]],\n"
	"	texture2d<float, access::write> result [[texture(1)]],\n"
	"	instance_acceleration_structure world [[buffer(0)]],\n"
	"	constant float *c [[buffer(1)]],\n"
	"	uint2 id [[thread_position_in_grid]])\n"
	"{\n"
	"	float2 origin = float2(c[16], c[17]), size = float2(c[18], c[19]);\n"
	"	float2 p = float2(id) + 0.5;\n"
	"	if (any(p < origin) || any(p >= origin + size)) return;\n"
	"	float4 g = gbuffer.read(id);\n"
	"	float z = abs(g.x);\n"
	"	if (z <= c[12] || z >= c[13] * 0.999) { result.write(float4(1.0, 0.0, 0.0, 0.0), id); return; }\n"
	"	float3 camera = float3(c[0], c[1], c[2]), forward = float3(c[3], c[4], c[5]);\n"
	"	float3 up = float3(c[6], c[7], c[8]), right = float3(c[9], c[10], c[11]);\n"
	"	float t = c[14], aspect = c[15];\n"
	"	float2 ndc = (p - origin) / size * 2.0 - 1.0;\n"
	/* rows run from the top: +y on screen is down */
	"	float3 P = camera + forward * z + right * (ndc.x * t * aspect * z) - up * (ndc.y * t * z);\n"
	"	float3 N = normalize(right * g.y - up * g.z + forward * g.w);\n"
	"	float3 tangent = normalize(abs(N.z) < 0.9 ? cross(N, float3(0, 0, 1)) : cross(N, float3(1, 0, 0)));\n"
	"	float3 bitangent = cross(N, tangent);\n"
	/* the instances' masks: 1 the level, 2 the objects, 4 the player's body
	   (which the game does not draw in the first person) */
	"	bool object = c[23] > 0.5 && g.x < 0.0;\n"
	"	intersector<triangle_data, instancing> any_hit;\n"
	"	any_hit.accept_any_intersection(true);\n"
	"	any_hit.assume_geometry_type(geometry_type::triangle);\n"
	"	any_hit.force_opacity(forced_opacity::opaque);\n"
	/* the level's triangles face outwards, wound counterclockwise around
	   their normal (raytrace_world.c), which Metal's rays see from the front
	   as clockwise (tested: port/macos/tests/run_raytrace_test.sh): a ray
	   leaving a surface from behind it passes */
	"	any_hit.set_triangle_front_facing_winding(winding::clockwise);\n"
	"	any_hit.set_triangle_cull_mode(triangle_cull_mode::back);\n"
	"	float radius = c[21];\n"
	"	float bias = 0.02 + z * 0.002;\n"
	"	float occlusion = 0.0;\n"
	"	for (uint i = 0; i < 4; i++)\n"
	"	{\n"
	/* stratified, in a pattern that repeats every 4x4 pixels: the guest's
	   4x4 blur takes in all 16 of its sets of directions (64 in all), so the
	   result is smooth and holds still from frame to frame */
	"		uint k = (id.x & 3u) + 4u * (id.y & 3u);\n"
	"		float u = (float(i) + (float(k) + 0.5) / 16.0) / 4.0;\n"
	"		float v = fract(float(i) * 0.61803399 + float(k) / 16.0);\n"
	/* cosine-weighted over the hemisphere */
	"		float r = sqrt(u), angle = 6.2831853 * v;\n"
	"		float3 d = normalize(tangent * (r * cos(angle)) + bitangent * (r * sin(angle)) + N * sqrt(max(0.0, 1.0 - u)));\n"
	"		ray occlusion_ray(P + N * bias, d, 0.0, radius);\n"
	/* (an object's occlusion: the level's and the other objects'; the
	   level's: everything's, the player's body too) */
	"		auto hit = any_hit.intersect(occlusion_ray, world, object ? 3u : 7u);\n"
	"		if (hit.type != intersection_type::none)\n"
	"			occlusion += 1.0 - hit.distance / radius;\n"
	"	}\n"
	"	float visibility = 1.0 - occlusion / 4.0;\n"
	/* the sun's shadow, on what the level's lightmaps do not shade: an
	   object's pixel (the guest marks them; without the marks, a pixel where
	   a short ray into it finds no level surface) facing the sun, whose ray
	   to it the level blocks. The sun is a small disc: the rays spread a
	   little. */
	"	if (c[27] > 0.0)\n"
	"	{\n"
	"		float3 sun = float3(c[24], c[25], c[26]);\n"
	"		bool on_level;\n"
	"		if (c[23] > 0.5) on_level = g.x > 0.0;\n"
	"		else\n"
	"		{\n"
	"			intersector<triangle_data, instancing> surface_probe;\n"
	"			surface_probe.accept_any_intersection(true);\n"
	"			surface_probe.force_opacity(forced_opacity::opaque);\n"
	"			ray probe(P + N * bias, -N, 0.0, bias * 3.0);\n"
	"			on_level = surface_probe.intersect(probe, world, 1u).type != intersection_type::none;\n"
	"		}\n"
	/* an object's pixel: the level's and the other objects' shadows; the
	   level's: the player's body's only (the lightmaps have the level's,
	   and the game draws the other objects') */
	"		if (dot(N, sun) > 0.0)\n"
	"		{\n"
	"			uint k = (id.x & 3u) + 4u * (id.y & 3u);\n"
	"			float3 spread = (tangent * (float(k & 3u) - 1.5) + bitangent * (float(k >> 2) - 1.5)) * 0.006;\n"
	"			ray shadow_ray(P + N * bias, normalize(sun + spread), 0.0, 2000.0);\n"
	"			if (any_hit.intersect(shadow_ray, world, on_level ? 4u : 3u).type != intersection_type::none)\n"
	"				visibility *= 1.0 - 0.55 * c[27];\n"
	"		}\n"
	"	}\n"
	/* the reflection: its hit, where the camera sees it. How much the
	   surface reflects is Fresnel's, more at glancing angles; facing the
	   camera, it reflects so little (4%, times the guest's strength) that
	   the ray is left out, faded in from a twentieth of the full reflection. */
	"	float3 V = normalize(P - camera);\n"
	"	float3 R = reflect(V, N);\n"
	"	float fresnel = mix(0.04, 1.0, pow(1.0 - clamp(dot(-V, N), 0.0, 1.0), 5.0));\n"
	"	float fade = smoothstep(0.05, 0.1, fresnel);\n"
	"	if (fade <= 0.0) { result.write(float4(visibility, 0.0, 0.0, 0.0), id); return; }\n"
	"	intersector<triangle_data, instancing> closest;\n"
	"	closest.assume_geometry_type(geometry_type::triangle);\n"
	"	closest.force_opacity(forced_opacity::opaque);\n"
	"	closest.set_triangle_front_facing_winding(winding::clockwise);\n"
	"	closest.set_triangle_cull_mode(triangle_cull_mode::back);\n"
	"	ray reflection_ray(P + N * bias, R, 0.0, c[22]);\n"
	"	auto hit = closest.intersect(reflection_ray, world, 1u);\n"
	"	float4 out = float4(visibility, 0.0, 0.0, 0.0);\n"
	"	if (hit.type != intersection_type::none)\n"
	"	{\n"
	"		float3 H = P + N * bias + R * hit.distance;\n"
	"		float3 relative = H - camera;\n"
	"		float hz = dot(relative, forward);\n"
	"		if (hz > c[12])\n"
	"		{\n"
	"			float2 hit_ndc = float2(dot(relative, right) / (hz * t * aspect), -dot(relative, up) / (hz * t));\n"
	"			float2 screen = origin + (hit_ndc * 0.5 + 0.5) * size;\n"
	"			if (all(screen >= origin) && all(screen < origin + size))\n"
	"			{\n"
	"				float seen = abs(gbuffer.read(uint2(screen)).x);\n"
	/* seen there, not hidden behind something nearer */
	"				if (seen > 0.0 && seen > hz * 0.9 - 0.1)\n"
	"				{\n"
	"					float2 edge = min(screen - origin, origin + size - screen) / (size * 0.08);\n"
	"					out.gb = screen / float2(gbuffer.get_width(), gbuffer.get_height());\n"
	"					out.a = clamp(min(edge.x, edge.y), 0.0, 1.0) * (1.0 - hit.distance / c[22]) * fresnel * fade;\n"
	"				}\n"
	"			}\n"
	"		}\n"
	"	}\n"
	"	result.write(out, id);\n"
	"}\n";

static void *egl_symbol(const char *name)
{
	return (void *)SDL_EGL_GetProcAddress(name);
}

static void *gl_symbol(const char *name)
{
	return (void *)SDL_GL_GetProcAddress(name);
}

int host_rt_available(void)
{
	NSError *error = nil;
	EGLAttrib_ device = 0, metal_device = 0;
	id<MTLLibrary> library;
	id<MTLFunction> function;

	if (rt.checked)
		return rt.available;
	rt.checked = 1;
	rt.eglGetCurrentDisplay = egl_symbol("eglGetCurrentDisplay");
	rt.eglCreateImageKHR = egl_symbol("eglCreateImageKHR");
	rt.eglDestroyImageKHR = egl_symbol("eglDestroyImageKHR");
	rt.eglQueryDisplayAttribEXT = egl_symbol("eglQueryDisplayAttribEXT");
	rt.eglQueryDeviceAttribEXT = egl_symbol("eglQueryDeviceAttribEXT");
	rt.glGenTextures = gl_symbol("glGenTextures");
	rt.glDeleteTextures = gl_symbol("glDeleteTextures");
	rt.glBindTexture = gl_symbol("glBindTexture");
	rt.glTexParameteri = gl_symbol("glTexParameteri");
	rt.glEGLImageTargetTexture2DOES = gl_symbol("glEGLImageTargetTexture2DOES");
	rt.glFinish = gl_symbol("glFinish");
	rt.glFlush = gl_symbol("glFlush");
	rt.eglQueryString = egl_symbol("eglQueryString");
	rt.eglCreateSync = egl_symbol("eglCreateSync");
	rt.eglDestroySync = egl_symbol("eglDestroySync");
	rt.eglWaitSync = egl_symbol("eglWaitSync");
	if (!rt.eglGetCurrentDisplay || !rt.eglCreateImageKHR || !rt.eglQueryDisplayAttribEXT ||
		!rt.eglQueryDeviceAttribEXT || !rt.glEGLImageTargetTexture2DOES || !rt.glFinish || !rt.glGenTextures)
	{
		host_logf(HOST_LOG_INFO, "ray tracing: ANGLE lacks the EGL functions to share Metal textures");
		return 0;
	}
	rt.display = rt.eglGetCurrentDisplay();
	if (!rt.display || !rt.eglQueryDisplayAttribEXT(rt.display, EGL_DEVICE_EXT_, &device) || !device ||
		!rt.eglQueryDeviceAttribEXT((void *)device, EGL_METAL_DEVICE_ANGLE_, &metal_device) || !metal_device)
	{
		host_logf(HOST_LOG_INFO, "ray tracing: ANGLE's Metal device is not available");
		return 0;
	}
	rt.device = (__bridge id<MTLDevice>)(void *)metal_device;
	if (!rt.device.supportsRaytracing)
	{
		host_logf(HOST_LOG_INFO, "ray tracing: %s has no Metal ray tracing", rt.device.name.UTF8String);
		return 0;
	}
	library = [rt.device newLibraryWithSource:kernel_source options:nil error:&error];
	function = library ? [library newFunctionWithName:@"trace"] : nil;
	rt.pipeline = function ? [rt.device newComputePipelineStateWithFunction:function error:&error] : nil;
	if (!rt.pipeline)
	{
		host_logf(HOST_LOG_ERROR, "ray tracing: the Metal kernel does not build: %s",
			error ? error.localizedDescription.UTF8String : "?");
		return 0;
	}
	rt.queue = [rt.device newCommandQueue];
	{
		const char *extensions = rt.eglQueryString ? rt.eglQueryString(rt.display, EGL_EXTENSIONS_) : NULL;

		if (extensions && strstr(extensions, "EGL_ANGLE_metal_shared_event_sync") && rt.eglCreateSync &&
			rt.eglDestroySync && rt.eglWaitSync && rt.glFlush && !getenv("HALO_RT_CPU_SYNC"))
		{
			rt.event = [rt.device newSharedEvent];
		}
	}
	rt.available = 1;
	host_logf(HOST_LOG_INFO, "ray tracing: Metal on %s (%s; %s)", rt.device.name.UTF8String,
		[rt.device supportsFamily:MTLGPUFamilyApple9] ? "ray tracing hardware" : "in compute",
		rt.event ? "GL and Metal in turn on the GPU" : "the CPU waits for GL and Metal");
	return 1;
}

/* the level's triangles (vertices: x, y, z; indices: three a triangle) */
int host_rt_set_world(uint32_t generation, const float *vertices, int vertex_count, const uint32_t *indices,
	int triangle_count)
{
	MTLAccelerationStructureTriangleGeometryDescriptor *geometry;
	MTLPrimitiveAccelerationStructureDescriptor *descriptor;
	MTLAccelerationStructureSizes sizes;
	id<MTLBuffer> vertex_buffer, index_buffer, scratch;
	id<MTLCommandBuffer> commands;
	id<MTLAccelerationStructureCommandEncoder> encoder;

	if (!rt.available || generation == rt.world_generation)
		return rt.world != nil;
	rt.world = nil;
	rt.world_generation = generation;
	if (vertex_count <= 0 || triangle_count <= 0)
		return 0;
	vertex_buffer = [rt.device newBufferWithBytes:vertices length:(NSUInteger)vertex_count * 12
		options:MTLResourceStorageModeShared];
	index_buffer = [rt.device newBufferWithBytes:indices length:(NSUInteger)triangle_count * 12
		options:MTLResourceStorageModeShared];
	geometry = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
	geometry.vertexBuffer = vertex_buffer;
	geometry.vertexStride = 12;
	geometry.indexBuffer = index_buffer;
	geometry.indexType = MTLIndexTypeUInt32;
	geometry.triangleCount = (NSUInteger)triangle_count;
	geometry.opaque = YES;
	descriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
	descriptor.geometryDescriptors = @[ geometry ];
	sizes = [rt.device accelerationStructureSizesWithDescriptor:descriptor];
	rt.world = [rt.device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
	scratch = [rt.device newBufferWithLength:sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
	commands = [rt.queue commandBuffer];
	encoder = [commands accelerationStructureCommandEncoder];
	[encoder buildAccelerationStructure:rt.world descriptor:descriptor scratchBuffer:scratch scratchBufferOffset:0];
	[encoder endEncoding];
	[commands commit];
	[commands waitUntilCompleted];
	host_logf(HOST_LOG_INFO, "ray tracing: the level, %d triangles", triangle_count);
	return 1;
}

/* a unit sphere (an icosahedron split once: 80 triangles), wound as the
level's are, counterclockwise seen from outside */
static id<MTLAccelerationStructure> build_blob(void)
{
	static const float t = 1.6180340f;
	float corners[12][3] = { { -1, t, 0 }, { 1, t, 0 }, { -1, -t, 0 }, { 1, -t, 0 }, { 0, -1, t }, { 0, 1, t },
		{ 0, -1, -t }, { 0, 1, -t }, { t, 0, -1 }, { t, 0, 1 }, { -t, 0, -1 }, { -t, 0, 1 } };
	static const int faces[20][3] = { { 0, 11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7, 10 }, { 0, 10, 11 }, { 1, 5, 9 },
		{ 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 }, { 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 },
		{ 3, 8, 9 }, { 4, 9, 5 }, { 2, 4, 11 }, { 6, 2, 10 }, { 8, 6, 7 }, { 9, 8, 1 } };
	float vertices[80 * 3 * 3];
	int face, corner, count = 0;
	MTLAccelerationStructureTriangleGeometryDescriptor *geometry;
	MTLPrimitiveAccelerationStructureDescriptor *descriptor;
	MTLAccelerationStructureSizes sizes;
	id<MTLAccelerationStructure> blob;
	id<MTLBuffer> scratch;
	id<MTLCommandBuffer> commands;
	id<MTLAccelerationStructureCommandEncoder> encoder;

	for (corner = 0; corner < 12; corner++)
	{
		float length = sqrtf(corners[corner][0] * corners[corner][0] + corners[corner][1] * corners[corner][1] +
			corners[corner][2] * corners[corner][2]);

		corners[corner][0] /= length;
		corners[corner][1] /= length;
		corners[corner][2] /= length;
	}
	for (face = 0; face < 20; face++)
	{
		const float *a = corners[faces[face][0]], *b = corners[faces[face][1]], *c = corners[faces[face][2]];
		float ab[3], bc[3], ca[3];
		const float *triangles[4][3] = { { a, ab, ca }, { ab, b, bc }, { ca, bc, c }, { ab, bc, ca } };
		int i, k;

		for (i = 0; i < 3; i++)
		{
			ab[i] = a[i] + b[i];
			bc[i] = b[i] + c[i];
			ca[i] = c[i] + a[i];
		}
		for (i = 0; i < 3; i++)
		{
			float *m = i == 0 ? ab : i == 1 ? bc : ca;
			float length = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);

			m[0] /= length;
			m[1] /= length;
			m[2] /= length;
		}
		for (i = 0; i < 4; i++)
		{
			const float *p = triangles[i][0], *q = triangles[i][1], *r = triangles[i][2];
			float u[3] = { q[0] - p[0], q[1] - p[1], q[2] - p[2] }, v[3] = { r[0] - p[0], r[1] - p[1], r[2] - p[2] };
			float n[3] = { u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0] };
			int flip = n[0] * p[0] + n[1] * p[1] + n[2] * p[2] < 0.0f;

			for (k = 0; k < 3; k++)
			{
				const float *vertex = triangles[i][flip && k ? 3 - k : k];

				vertices[count * 3 + 0] = vertex[0];
				vertices[count * 3 + 1] = vertex[1];
				vertices[count * 3 + 2] = vertex[2];
				count++;
			}
		}
	}
	geometry = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
	geometry.vertexBuffer = [rt.device newBufferWithBytes:vertices length:sizeof(vertices)
		options:MTLResourceStorageModeShared];
	geometry.vertexStride = 12;
	geometry.triangleCount = 80;
	geometry.opaque = YES;
	descriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
	descriptor.geometryDescriptors = @[ geometry ];
	sizes = [rt.device accelerationStructureSizesWithDescriptor:descriptor];
	blob = [rt.device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
	scratch = [rt.device newBufferWithLength:sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
	commands = [rt.queue commandBuffer];
	encoder = [commands accelerationStructureCommandEncoder];
	[encoder buildAccelerationStructure:blob descriptor:descriptor scratchBuffer:scratch scratchBufferOffset:0];
	[encoder endEncoding];
	[commands commit];
	[commands waitUntilCompleted];
	return blob;
}

/* this frame's objects: each a unit sphere under a 3x4 transform (rows, 12
floats: the ellipsoid's axes as columns, then its center), with its mask (2
an object, 4 the player's body) */
void host_rt_set_objects(const float *transforms, const unsigned char *masks, int count)
{
	if (count < 0)
		count = 0;
	if (count > HOST_RT_MAXIMUM_OBJECTS)
		count = HOST_RT_MAXIMUM_OBJECTS;
	memcpy(rt.object_transforms, transforms, (size_t)count * 12 * sizeof(float));
	memcpy(rt.object_masks, masks, (size_t)count);
	rt.object_count = count;
}

/* the scene for this frame's rays: the level, and the objects, built in the
command buffer before the rays */
static int encode_scene(id<MTLCommandBuffer> commands)
{
	MTLInstanceAccelerationStructureDescriptor *descriptor;
	MTLAccelerationStructureInstanceDescriptor *instances;
	id<MTLBuffer> buffer;
	id<MTLAccelerationStructureCommandEncoder> encoder;
	int index, count = 1 + rt.object_count;

	if (!rt.blob)
		rt.blob = build_blob();
	if (!rt.blob)
		return 0;
	if (!rt.instance_buffers[0])
	{
		for (index = 0; index < 3; index++)
			rt.instance_buffers[index] = [rt.device newBufferWithLength:(HOST_RT_MAXIMUM_OBJECTS + 1) *
				sizeof(MTLAccelerationStructureInstanceDescriptor) options:MTLResourceStorageModeShared];
	}
	buffer = rt.instance_buffers[rt.instance_ring];
	rt.instance_ring = (rt.instance_ring + 1) % 3;
	instances = (MTLAccelerationStructureInstanceDescriptor *)buffer.contents;
	memset(instances, 0, (size_t)count * sizeof(*instances));
	for (index = 0; index < count; index++)
	{
		MTLAccelerationStructureInstanceDescriptor *instance = &instances[index];
		int column;

		instance->options = MTLAccelerationStructureInstanceOptionOpaque;
		if (!index)
		{
			instance->transformationMatrix.columns[0] = (MTLPackedFloat3){ { 1, 0, 0 } };
			instance->transformationMatrix.columns[1] = (MTLPackedFloat3){ { 0, 1, 0 } };
			instance->transformationMatrix.columns[2] = (MTLPackedFloat3){ { 0, 0, 1 } };
			instance->transformationMatrix.columns[3] = (MTLPackedFloat3){ { 0, 0, 0 } };
			instance->mask = 1;
			instance->accelerationStructureIndex = 0;
			continue;
		}
		for (column = 0; column < 4; column++)
		{
			const float *m = &rt.object_transforms[(index - 1) * 12];

			instance->transformationMatrix.columns[column] =
				(MTLPackedFloat3){ { m[column], m[4 + column], m[8 + column] } };
		}
		instance->mask = rt.object_masks[index - 1];
		instance->accelerationStructureIndex = 1;
	}
	descriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
	descriptor.instancedAccelerationStructures = @[ rt.world, rt.blob ];
	descriptor.instanceCount = (NSUInteger)count;
	descriptor.instanceDescriptorBuffer = buffer;
	descriptor.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeDefault;
	if (!rt.scene)
	{
		/* sized for the most objects, once */
		MTLAccelerationStructureSizes sizes;

		descriptor.instanceCount = HOST_RT_MAXIMUM_OBJECTS + 1;
		sizes = [rt.device accelerationStructureSizesWithDescriptor:descriptor];
		descriptor.instanceCount = (NSUInteger)count;
		rt.scene = [rt.device newAccelerationStructureWithSize:sizes.accelerationStructureSize];
		rt.scene_scratch = [rt.device newBufferWithLength:sizes.buildScratchBufferSize
			options:MTLResourceStorageModePrivate];
		if (!rt.scene || !rt.scene_scratch)
			return 0;
	}
	encoder = [commands accelerationStructureCommandEncoder];
	[encoder buildAccelerationStructure:rt.scene descriptor:descriptor scratchBuffer:rt.scene_scratch
		scratchBufferOffset:0];
	[encoder endEncoding];
	return 1;
}

/* the GL texture sharing Metal texture `which` (0: the depth and normals,
32-bit floats; 1: the results, 16-bit floats), at this size */
uint32_t host_rt_texture(int which, int width, int height)
{
	MTLTextureDescriptor *descriptor;
	const EGLint_ attributes[] = { EGL_NONE_ };

	if (!rt.available || which < 0 || which > 1 || width <= 0 || height <= 0)
		return 0;
	if (rt.textures[which] && rt.widths[which] == width && rt.heights[which] == height)
		return rt.gl_textures[which];
	if (rt.gl_textures[which])
	{
		rt.glDeleteTextures(1, &rt.gl_textures[which]);
		rt.gl_textures[which] = 0;
	}
	if (rt.images[which] && rt.eglDestroyImageKHR)
		rt.eglDestroyImageKHR(rt.display, rt.images[which]);
	rt.images[which] = NULL;
	descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:which == 0 ? MTLPixelFormatRGBA32Float :
		MTLPixelFormatRGBA16Float width:(NSUInteger)width height:(NSUInteger)height mipmapped:NO];
	descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite | MTLTextureUsageRenderTarget;
	descriptor.storageMode = MTLStorageModePrivate;
	rt.textures[which] = [rt.device newTextureWithDescriptor:descriptor];
	rt.images[which] = rt.textures[which] ? rt.eglCreateImageKHR(rt.display, EGL_NO_CONTEXT_, EGL_METAL_TEXTURE_ANGLE_,
		(__bridge void *)rt.textures[which], attributes) : NULL;
	if (!rt.images[which])
	{
		host_logf(HOST_LOG_ERROR, "ray tracing: ANGLE does not take the Metal texture");
		rt.textures[which] = nil;
		rt.available = 0;
		return 0;
	}
	rt.glGenTextures(1, &rt.gl_textures[which]);
	rt.glBindTexture(GL_TEXTURE_2D_, rt.gl_textures[which]);
	rt.glEGLImageTargetTexture2DOES(GL_TEXTURE_2D_, rt.images[which]);
	rt.glTexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_MIN_FILTER_, GL_NEAREST_);
	rt.glTexParameteri(GL_TEXTURE_2D_, GL_TEXTURE_MAG_FILTER_, GL_NEAREST_);
	rt.widths[which] = width;
	rt.heights[which] = height;
	return rt.gl_textures[which];
}

/* the rays, for the camera (the 28 values the kernel names); 1 if done */
/* the time waited on GL (glFinish) and on the rays, for the frame
statistics (host_sdl.c) */
uint64_t host_rt_finish_ns, host_rt_trace_ns, host_rt_traces;

int host_rt_trace(const float *camera, int width, int height)
{
	uint64_t start, finished;
	id<MTLCommandBuffer> commands;
	id<MTLComputeCommandEncoder> encoder;
	MTLSize group, groups;

	if (!rt.available || !rt.world || !rt.textures[0] || !rt.textures[1] ||
		rt.widths[0] != width || rt.widths[1] != width || rt.heights[0] != height || rt.heights[1] != height)
	{
		return 0;
	}
	start = SDL_GetTicksNS();
	if (rt.event)
	{
		/* GL signals the event when it has drawn the depth and normals */
		uint64_t drawn = ++rt.event_value;
		const EGLAttrib_ attributes[] = { EGL_SYNC_METAL_SHARED_EVENT_OBJECT_ANGLE_, (EGLAttrib_)(__bridge void *)rt.event,
			EGL_SYNC_METAL_SHARED_EVENT_SIGNAL_VALUE_LO_ANGLE_, (EGLAttrib_)(drawn & 0xFFFFFFFFu),
			EGL_SYNC_METAL_SHARED_EVENT_SIGNAL_VALUE_HI_ANGLE_, (EGLAttrib_)(drawn >> 32), EGL_NONE_ };
		void *sync = rt.eglCreateSync(rt.display, EGL_SYNC_METAL_SHARED_EVENT_ANGLE_, attributes);

		if (!sync)
		{
			host_logf(HOST_LOG_ERROR, "ray tracing: GL does not signal the Metal event; the CPU waits instead");
			rt.event = nil;
			rt.glFinish();
		}
		else
		{
			rt.eglDestroySync(rt.display, sync);
			rt.glFlush();
		}
	}
	else
	{
		rt.glFinish();
	}
	finished = SDL_GetTicksNS();
	commands = [rt.queue commandBuffer];
	if (rt.event)
		[commands encodeWaitForEvent:rt.event value:rt.event_value];
	if (!encode_scene(commands))
	{
		[commands commit];
		return 0;
	}
	encoder = [commands computeCommandEncoder];
	[encoder setComputePipelineState:rt.pipeline];
	[encoder setTexture:rt.textures[0] atIndex:0];
	[encoder setTexture:rt.textures[1] atIndex:1];
	[encoder setAccelerationStructure:rt.scene atBufferIndex:0];
	[encoder useResource:rt.world usage:MTLResourceUsageRead];
	[encoder useResource:rt.blob usage:MTLResourceUsageRead];
	[encoder setBytes:camera length:28 * sizeof(float) atIndex:1];
	group = MTLSizeMake(8, 8, 1);
	groups = MTLSizeMake(((NSUInteger)width + 7) / 8, ((NSUInteger)height + 7) / 8, 1);
	[encoder dispatchThreadgroups:groups threadsPerThreadgroup:group];
	[encoder endEncoding];
	if (rt.event)
	{
		/* GL waits on the GPU for the rays to finish */
		uint64_t traced = ++rt.event_value;
		const EGLAttrib_ attributes[] = { EGL_SYNC_CONDITION_, EGL_SYNC_METAL_SHARED_EVENT_SIGNALED_ANGLE_,
			EGL_SYNC_METAL_SHARED_EVENT_OBJECT_ANGLE_, (EGLAttrib_)(__bridge void *)rt.event,
			EGL_SYNC_METAL_SHARED_EVENT_SIGNAL_VALUE_LO_ANGLE_, (EGLAttrib_)(traced & 0xFFFFFFFFu),
			EGL_SYNC_METAL_SHARED_EVENT_SIGNAL_VALUE_HI_ANGLE_, (EGLAttrib_)(traced >> 32), EGL_NONE_ };
		void *sync;

		[commands encodeSignalEvent:rt.event value:traced];
		[commands commit];
		sync = rt.eglCreateSync(rt.display, EGL_SYNC_METAL_SHARED_EVENT_ANGLE_, attributes);
		if (sync && rt.eglWaitSync(rt.display, sync, 0))
		{
			rt.eglDestroySync(rt.display, sync);
			host_rt_finish_ns += finished - start;
			host_rt_trace_ns += SDL_GetTicksNS() - finished;
			host_rt_traces++;
			return 1;
		}
		if (sync)
			rt.eglDestroySync(rt.display, sync);
		host_logf(HOST_LOG_ERROR, "ray tracing: GL does not wait for the Metal event; the CPU waits instead");
		rt.event = nil;
		[commands waitUntilCompleted];
		return commands.status == MTLCommandBufferStatusCompleted;
	}
	[commands commit];
	[commands waitUntilCompleted];
	host_rt_finish_ns += finished - start;
	host_rt_trace_ns += SDL_GetTicksNS() - finished;
	host_rt_traces++;
	return commands.status == MTLCommandBufferStatusCompleted;
}

/* tests: count pixels of row y of shared texture `which` from x, as floats
(RGBA; the results texture's halves widened), into values */
int host_rt_debug_read(int which, int x, int y, int count, float *values)
{
	id<MTLBuffer> buffer;
	id<MTLCommandBuffer> commands;
	id<MTLBlitCommandEncoder> blit;
	int bytes = which == 0 ? 16 : 8, index;

	if (!rt.available || which < 0 || which > 1 || !rt.textures[which])
		return 0;
	rt.glFinish();
	buffer = [rt.device newBufferWithLength:(NSUInteger)(count * bytes) options:MTLResourceStorageModeShared];
	commands = [rt.queue commandBuffer];
	blit = [commands blitCommandEncoder];
	[blit copyFromTexture:rt.textures[which] sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake((NSUInteger)x,
		(NSUInteger)y, 0) sourceSize:MTLSizeMake((NSUInteger)count, 1, 1) toBuffer:buffer destinationOffset:0
		destinationBytesPerRow:(NSUInteger)(count * bytes) destinationBytesPerImage:(NSUInteger)(count * bytes)];
	[blit endEncoding];
	[commands commit];
	[commands waitUntilCompleted];
	for (index = 0; index < count * 4; index++)
		values[index] = which == 0 ? ((const float *)buffer.contents)[index] :
			(float)((const __fp16 *)buffer.contents)[index];
	return 1;
}
