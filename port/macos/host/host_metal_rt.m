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
- The rays (host_rt_trace): from each pixel's point in the world, eight rays
  over the hemisphere around its normal, which find the level's geometry
  near it wherever it is, on screen or not (ambient occlusion), and one
  along the view's reflection, whose hit is projected back onto the screen
  for its colour if the camera sees it there. The results go to a second
  shared texture, which the guest composites.

GL and Metal take turns: glFinish before the rays, and the command buffer
completed before GL reads the results.
*/

#include "host.h"

#import <Metal/Metal.h>
#include <SDL3/SDL.h>
#include <dlfcn.h>
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
#define GL_TEXTURE_2D_ 0x0DE1
#define GL_TEXTURE_MIN_FILTER_ 0x2801
#define GL_TEXTURE_MAG_FILTER_ 0x2800
#define GL_NEAREST_ 0x2600

static struct
{
	int checked, available;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLComputePipelineState> pipeline;
	id<MTLAccelerationStructure> world;
	unsigned int world_generation;
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
} rt;

static NSString *const kernel_source = @
	"#include <metal_stdlib>\n"
	"#include <metal_raytracing>\n"
	"using namespace metal;\n"
	"using namespace raytracing;\n"
	/* c: position 0-2, forward 3-5, up 6-8, right 9-11, near 12, far 13,
	   tan of half the vertical field of view 14, aspect 15, viewport 16-19,
	   frame 20, occlusion radius 21, reflection distance 22 */
	"kernel void trace(texture2d<float, access::read> gbuffer [[texture(0)]],\n"
	"	texture2d<float, access::write> result [[texture(1)]],\n"
	"	primitive_acceleration_structure world [[buffer(0)]],\n"
	"	constant float *c [[buffer(1)]],\n"
	"	uint2 id [[thread_position_in_grid]])\n"
	"{\n"
	"	float2 origin = float2(c[16], c[17]), size = float2(c[18], c[19]);\n"
	"	float2 p = float2(id) + 0.5;\n"
	"	if (any(p < origin) || any(p >= origin + size)) return;\n"
	"	float4 g = gbuffer.read(id);\n"
	"	float z = g.x;\n"
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
	"	intersector<triangle_data> any_hit;\n"
	"	any_hit.accept_any_intersection(true);\n"
	"	any_hit.assume_geometry_type(geometry_type::triangle);\n"
	"	any_hit.force_opacity(forced_opacity::opaque);\n"
	"	float radius = c[21];\n"
	"	float bias = 0.02 + z * 0.002;\n"
	"	float occlusion = 0.0;\n"
	"	for (uint i = 0; i < 8; i++)\n"
	"	{\n"
	/* stratified, in a pattern that repeats every 4x4 pixels: the guest's
	   4x4 blur takes in all 16 of its sets of directions, so the result is
	   smooth and holds still from frame to frame */
	"		uint k = (id.x & 3u) + 4u * (id.y & 3u);\n"
	"		float u = (float(i) + (float(k) + 0.5) / 16.0) / 8.0;\n"
	"		float v = fract(float(i) * 0.61803399 + float(k) / 16.0);\n"
	/* cosine-weighted over the hemisphere */
	"		float r = sqrt(u), angle = 6.2831853 * v;\n"
	"		float3 d = normalize(tangent * (r * cos(angle)) + bitangent * (r * sin(angle)) + N * sqrt(max(0.0, 1.0 - u)));\n"
	"		ray occlusion_ray(P + N * bias, d, 0.0, radius);\n"
	"		auto hit = any_hit.intersect(occlusion_ray, world);\n"
	"		if (hit.type != intersection_type::none)\n"
	"			occlusion += 1.0 - hit.distance / radius;\n"
	"	}\n"
	"	float visibility = 1.0 - occlusion / 8.0;\n"
	/* the reflection: its hit, where the camera sees it */
	"	float3 V = normalize(P - camera);\n"
	"	float3 R = reflect(V, N);\n"
	"	intersector<triangle_data> closest;\n"
	"	closest.assume_geometry_type(geometry_type::triangle);\n"
	"	closest.force_opacity(forced_opacity::opaque);\n"
	"	ray reflection_ray(P + N * bias, R, 0.0, c[22]);\n"
	"	auto hit = closest.intersect(reflection_ray, world);\n"
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
	"				float seen = gbuffer.read(uint2(screen)).x;\n"
	/* seen there, not hidden behind something nearer */
	"				if (seen > 0.0 && seen > hz * 0.9 - 0.1)\n"
	"				{\n"
	"					float2 edge = min(screen - origin, origin + size - screen) / (size * 0.08);\n"
	"					out.gb = screen / float2(gbuffer.get_width(), gbuffer.get_height());\n"
	/* how much it reflects: Fresnel's, more at glancing angles */
	"					float fresnel = pow(1.0 - clamp(dot(-V, N), 0.0, 1.0), 5.0);\n"
	"					out.a = clamp(min(edge.x, edge.y), 0.0, 1.0) * (1.0 - hit.distance / c[22]) *\n"
	"						mix(0.04, 1.0, fresnel);\n"
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
	rt.available = 1;
	host_logf(HOST_LOG_INFO, "ray tracing: Metal on %s (%s)", rt.device.name.UTF8String,
		[rt.device supportsFamily:MTLGPUFamilyApple9] ? "ray tracing hardware" : "in compute");
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

/* the rays, for the camera (the 24 values the kernel names); 1 if done */
int host_rt_trace(const float *camera, int width, int height)
{
	id<MTLCommandBuffer> commands;
	id<MTLComputeCommandEncoder> encoder;
	MTLSize group, groups;

	if (!rt.available || !rt.world || !rt.textures[0] || !rt.textures[1] ||
		rt.widths[0] != width || rt.widths[1] != width || rt.heights[0] != height || rt.heights[1] != height)
	{
		return 0;
	}
	rt.glFinish();
	commands = [rt.queue commandBuffer];
	encoder = [commands computeCommandEncoder];
	[encoder setComputePipelineState:rt.pipeline];
	[encoder setTexture:rt.textures[0] atIndex:0];
	[encoder setTexture:rt.textures[1] atIndex:1];
	[encoder setAccelerationStructure:rt.world atBufferIndex:0];
	[encoder setBytes:camera length:24 * sizeof(float) atIndex:1];
	group = MTLSizeMake(8, 8, 1);
	groups = MTLSizeMake(((NSUInteger)width + 7) / 8, ((NSUInteger)height + 7) / 8, 1);
	[encoder dispatchThreadgroups:groups threadsPerThreadgroup:group];
	[encoder endEncoding];
	[commands commit];
	[commands waitUntilCompleted];
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
