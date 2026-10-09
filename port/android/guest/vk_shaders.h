/*
VK_SHADERS.H

The Vulkan renderer's GLSL generators (nv2a_vsh_vk.c, nv2a_psh_vk.c; copies of
the OpenGL renderer's nv2a_vsh.c and nv2a_psh.c, port/linux/src): their
prototypes, the bindings and interface locations the generated shaders use,
and the uniform blocks' contents as C structures.

Everything is in set 0 and bindings are unique across the stages. Every block is
std140, every member a vec4 or an array of them, and every member has an
explicit layout(offset = N) written from the offsets below: the structures'
offsetof is checked against the same numbers at compile time, and glslang
rejects an offset std140 forbids, so a shader, this header and a structure
cannot disagree without a build failing. Fixed-width types and floats only, so
the host (LP64) can include this too.
*/

#ifndef __VK_SHADERS_H
#define __VK_SHADERS_H

#include <stddef.h>
#include <stdint.h>

/* the descriptor set every binding is in */
#define VK_SHADER_SET 0

/* bindings */
#define VK_BINDING_VERTEX_CONSTANTS 0
#define VK_BINDING_VERTEX_PARAMETERS 1
#define VK_BINDING_PIXEL_PARAMETERS 2
#define VK_BINDING_TEXTURE0 3
#define VK_BINDING_TEXTURE1 4
#define VK_BINDING_TEXTURE2 5
#define VK_BINDING_TEXTURE3 6

/* the locations the vertex and pixel stages meet at: every vertex shader writes all nine and every pixel shader
declares all nine, read or not, so that any vertex shader goes with any pixel shader */
#define VK_LOCATION_D0 0
#define VK_LOCATION_D1 1
#define VK_LOCATION_B0 2
#define VK_LOCATION_B1 3
#define VK_LOCATION_T0 4
#define VK_LOCATION_T1 5
#define VK_LOCATION_T2 6
#define VK_LOCATION_T3 7
#define VK_LOCATION_FOG 8
/* and, only between the shaders of a draw lit for each pixel (display.per_pixel_lighting), the normal and the world
position the model lighting program lights by */
#define VK_LOCATION_WORLD_NORMAL 9
#define VK_LOCATION_WORLD_POSITION 10

/* the pixel shader's output */
#define VK_LOCATION_FRAGMENT_COLOR 0

/* the generators' version: raised by any change to what nv2a_vsh_vk.c or nv2a_psh_vk.c write, so that the shader
cache makes new names for new GLSL */
#define VK_SHADER_GENERATOR_VERSION 1

/* ---------- vertex_constants (binding 0): Direct3D's constant registers c[-96..95] */

#define VK_VERTEX_CONSTANT_COUNT 192
#define VK_VERTEX_CONSTANTS_C 0
#define VK_VERTEX_CONSTANTS_SIZE 3072

struct vk_vertex_constants
{
	float c[VK_VERTEX_CONSTANT_COUNT][4];
};

/* ---------- vertex_parameters (binding 1) */

#define VK_VERTEX_PARAMETERS_VIEWPORT_SCALE 0
#define VK_VERTEX_PARAMETERS_VIEWPORT_OFFSET 16
#define VK_VERTEX_PARAMETERS_POINT_AND_SCREEN 32
#define VK_VERTEX_PARAMETERS_SIZE 48

struct vk_vertex_parameters
{
	float viewport_scale[4];
	float viewport_offset[4];
	/* x: the point size; y: the columns the menus shift by to centre on a wide screen (the GL renderer's
	screen_offset) */
	float point_and_screen[4];
};

/* ---------- pixel_parameters (binding 2) */

#define VK_PIXEL_PARAMETERS_PS_C0 0
#define VK_PIXEL_PARAMETERS_PS_C1 128
#define VK_PIXEL_PARAMETERS_PS_FINAL_C0 256
#define VK_PIXEL_PARAMETERS_PS_FINAL_C1 272
#define VK_PIXEL_PARAMETERS_FOG_COLOR 288
#define VK_PIXEL_PARAMETERS_FOG_PARAMETERS 304
#define VK_PIXEL_PARAMETERS_ALPHA_REFERENCE 320
#define VK_PIXEL_PARAMETERS_BUMP_MATRIX 336
#define VK_PIXEL_PARAMETERS_BUMP_LUMINANCE 400
#define VK_PIXEL_PARAMETERS_TEXTURE_SCALE 464
#define VK_PIXEL_PARAMETERS_MODEL_LIGHTS 528
#define VK_PIXEL_PARAMETERS_SIZE 720

struct vk_pixel_parameters
{
	float ps_c0[8][4];
	float ps_c1[8][4];
	float ps_final_c0[4];
	float ps_final_c1[4];
	float fog_color[4];
	float fog_parameters[4];
	/* the alpha test's reference value, in x */
	float alpha_reference[4];
	float bump_matrix[4][4];
	float bump_luminance[4][4];
	float texture_scale[4][4];
	/* a draw lit for each pixel: the vertex constants its lighting reads (XGPU_MODEL_LIGHT_COUNT, xgpu.h); zero otherwise.
	Declared only by the pixel shaders that read them */
	float model_lights[12][4];
};

/* ---------- compile-time checks: the structures are the blocks, byte for byte */

#define VK_SHADERS_CHECK(name, condition) typedef char vk_shaders_check_##name[(condition) ? 1 : -1]

VK_SHADERS_CHECK(vertex_constants_size, sizeof(struct vk_vertex_constants) == VK_VERTEX_CONSTANTS_SIZE);
VK_SHADERS_CHECK(vertex_constants_c, offsetof(struct vk_vertex_constants, c) == VK_VERTEX_CONSTANTS_C);
VK_SHADERS_CHECK(vertex_parameters_size, sizeof(struct vk_vertex_parameters) == VK_VERTEX_PARAMETERS_SIZE);
VK_SHADERS_CHECK(vertex_parameters_scale,
	offsetof(struct vk_vertex_parameters, viewport_scale) == VK_VERTEX_PARAMETERS_VIEWPORT_SCALE);
VK_SHADERS_CHECK(vertex_parameters_offset,
	offsetof(struct vk_vertex_parameters, viewport_offset) == VK_VERTEX_PARAMETERS_VIEWPORT_OFFSET);
VK_SHADERS_CHECK(vertex_parameters_point,
	offsetof(struct vk_vertex_parameters, point_and_screen) == VK_VERTEX_PARAMETERS_POINT_AND_SCREEN);
VK_SHADERS_CHECK(pixel_parameters_size, sizeof(struct vk_pixel_parameters) == VK_PIXEL_PARAMETERS_SIZE);
VK_SHADERS_CHECK(pixel_parameters_c0, offsetof(struct vk_pixel_parameters, ps_c0) == VK_PIXEL_PARAMETERS_PS_C0);
VK_SHADERS_CHECK(pixel_parameters_c1, offsetof(struct vk_pixel_parameters, ps_c1) == VK_PIXEL_PARAMETERS_PS_C1);
VK_SHADERS_CHECK(pixel_parameters_final_c0,
	offsetof(struct vk_pixel_parameters, ps_final_c0) == VK_PIXEL_PARAMETERS_PS_FINAL_C0);
VK_SHADERS_CHECK(pixel_parameters_final_c1,
	offsetof(struct vk_pixel_parameters, ps_final_c1) == VK_PIXEL_PARAMETERS_PS_FINAL_C1);
VK_SHADERS_CHECK(pixel_parameters_fog_color,
	offsetof(struct vk_pixel_parameters, fog_color) == VK_PIXEL_PARAMETERS_FOG_COLOR);
VK_SHADERS_CHECK(pixel_parameters_fog_parameters,
	offsetof(struct vk_pixel_parameters, fog_parameters) == VK_PIXEL_PARAMETERS_FOG_PARAMETERS);
VK_SHADERS_CHECK(pixel_parameters_alpha,
	offsetof(struct vk_pixel_parameters, alpha_reference) == VK_PIXEL_PARAMETERS_ALPHA_REFERENCE);
VK_SHADERS_CHECK(pixel_parameters_bump_matrix,
	offsetof(struct vk_pixel_parameters, bump_matrix) == VK_PIXEL_PARAMETERS_BUMP_MATRIX);
VK_SHADERS_CHECK(pixel_parameters_bump_luminance,
	offsetof(struct vk_pixel_parameters, bump_luminance) == VK_PIXEL_PARAMETERS_BUMP_LUMINANCE);
VK_SHADERS_CHECK(pixel_parameters_texture_scale,
	offsetof(struct vk_pixel_parameters, texture_scale) == VK_PIXEL_PARAMETERS_TEXTURE_SCALE);
VK_SHADERS_CHECK(pixel_parameters_model_lights,
	offsetof(struct vk_pixel_parameters, model_lights) == VK_PIXEL_PARAMETERS_MODEL_LIGHTS);

/* ---------- identity

A shader is known by a 64-bit FNV-1a hash that the guest makes; the host never sees keys, only hashes and GLSL. A
vertex shader's is of VK_SHADER_GENERATOR_VERSION, its program's hash and its packed mask (0 for immediate mode); a
pixel shader's of the version and the key's bytes (count_samples 0). */

static inline uint64_t vk_hash_init(void)
{
	return 14695981039346656037ull;
}

static inline uint64_t vk_hash_mix(uint64_t hash, const void *data, unsigned long size)
{
	const unsigned char *bytes = data;

	while (size--)
		hash = (hash ^ *bytes++) * 1099511628211ull;
	return hash;
}

/* the stages, as the host's imports take them */
#define VK_SHADER_STAGE_VERTEX 0
#define VK_SHADER_STAGE_PIXEL 1

/* what host_vk_shader_find says when it returns 0 */
#define VK_SHADER_STATUS_UNKNOWN 0 /* neither made nor queued: send its GLSL */
#define VK_SHADER_STATUS_QUEUED 1
#define VK_SHADER_STATUS_COMPILING 2
#define VK_SHADER_STATUS_FAILED 3 /* for the rest of the run */
#define VK_SHADER_STATUS_NOT_READY 4 /* the backend's device is not made yet: ask again */

/* ---------- the pipeline's state

Everything in a pipeline that is not one of Vulkan 1.0's core dynamic states (viewport, scissor, depth bias, blend
constants, stencil masks and reference), so that a draw's record names its pipeline whole. Fixed-width fields; every enumeration is
the Vulkan enumerant's number (VkPrimitiveTopology, VkCompareOp and so on), which the guest writes without Vulkan's
headers. Unused fields are zero, so that equal states hash equal. The attachments' formats are 0 (none) or 1 (the
backend's colour format, or its depth-stencil format, which only the host knows). */

#define VK_PIPELINE_VERTEX_ATTRIBUTES 16
#define VK_PIPELINE_VERTEX_BINDINGS 16

struct vk_stencil_face_state
{
	uint32_t fail_op, pass_op, depth_fail_op, compare_op;
};

struct vk_pipeline_state
{
	uint32_t topology, polygon_mode, cull_mode, front_face;
	uint32_t depth_test, depth_write, depth_compare_op, depth_bias;
	uint32_t stencil_test;
	struct vk_stencil_face_state stencil_front, stencil_back;
	uint32_t blend_enable, source_color_factor, destination_color_factor, color_op;
	uint32_t source_alpha_factor, destination_alpha_factor, alpha_op, color_write_mask;
	uint32_t color_format, depth_format;
	/* the samples a pixel of the targets (vk_command_targets): 0 for one */
	uint32_t samples;
	uint32_t binding_count;
	struct { uint32_t stride, rate; } bindings[VK_PIPELINE_VERTEX_BINDINGS];
	struct { uint32_t format, binding, offset; } attributes[VK_PIPELINE_VERTEX_ATTRIBUTES];
};

/* the placeholder state (a draw fills it from the game's): the numbers are Vulkan's */
#define VK_PLACEHOLDER_TOPOLOGY_TRIANGLE_LIST 3
#define VK_PLACEHOLDER_FORMAT_R32G32B32A32_SFLOAT 109
#define VK_PLACEHOLDER_FORMAT_R32_UINT 98

/* ---------- the generators */

/* GLSL 450 for glslang for an NV2A vertex program (the instruction words after the program header). Attributes whose
bit is set in packed_attribute_mask are fed as NORMPACKED3 32-bit integers and unpacked in the shader. With lighting
(else NULL), the normal and world position go to the pixel shader too (VK_LOCATION_WORLD_*), which lights the diffuse
color for each pixel (the key's per_pixel_lighting). Returns a malloc'd string. */
struct nv2a_vertex_lighting;
char *nv2a_vk_vertex_shader_to_glsl(const uint32_t *instructions, unsigned long instruction_count,
	unsigned long packed_attribute_mask, const struct nv2a_vertex_lighting *lighting);

struct nv2a_pixel_shader_key;

/* GLSL 450 for glslang for an Xbox pixel shader (the key is xgpu.h's); the key's count_samples is ignored (the
visibility tests are occlusion queries). With per_pixel_lighting, the diffuse color is lit for each pixel from
model_lights; with alpha_test_samples, the alpha test covers samples. Returns a malloc'd string. */
char *nv2a_vk_pixel_shader_to_glsl(const struct nv2a_pixel_shader_key *key);

#endif
