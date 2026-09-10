/* January frame-statistics layout, linkage, and Models arithmetic contract. */
#include "rasterizer/rasterizer_frame_statistics.h"

#define FRAME_STATISTICS_FIELD(member) \
	offsetof(struct rasterizer_frame_statistics_globals, member), \
	sizeof(((struct rasterizer_frame_statistics_globals *)0)->member)

unsigned long const rasterizer_frame_statistics_layout[] =
{
	sizeof(struct rasterizer_frame_statistics_globals),
	FRAME_STATISTICS_FIELD(reserved000),
	FRAME_STATISTICS_FIELD(shadow_count),
	FRAME_STATISTICS_FIELD(shadow_vertex_count),
	FRAME_STATISTICS_FIELD(shadow_triangle_count),
	FRAME_STATISTICS_FIELD(shadow_draw_count),
	FRAME_STATISTICS_FIELD(environment_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(environment_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(environment_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(decal_vertex_count),
	FRAME_STATISTICS_FIELD(decal_triangle_count),
	FRAME_STATISTICS_FIELD(decal_draw_count),
	FRAME_STATISTICS_FIELD(decal_shader_change_count),
	FRAME_STATISTICS_FIELD(decal_texture_change_count),
	FRAME_STATISTICS_FIELD(diffuse_texture_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(diffuse_texture_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(diffuse_texture_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(specular_light_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(specular_light_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(specular_light_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(specular_lightmap_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(specular_lightmap_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(specular_lightmap_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(reflection_mask_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(reflection_mask_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(reflection_mask_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(reflection_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(reflection_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(reflection_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(transparent_geometry_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(transparent_geometry_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(transparent_geometry_largest_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(transparent_geometry_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(environment_fog_dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(environment_fog_dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(environment_fog_dynamic_draw_count),
	FRAME_STATISTICS_FIELD(reserved0B8),
	FRAME_STATISTICS_FIELD(model_count),
	FRAME_STATISTICS_FIELD(reserved0D8),
	FRAME_STATISTICS_FIELD(model_shadow_count),
	FRAME_STATISTICS_FIELD(model_shadow_vertex_count),
	FRAME_STATISTICS_FIELD(model_shadow_triangle_count),
	FRAME_STATISTICS_FIELD(model_shadow_draw_count),
	FRAME_STATISTICS_FIELD(dynamic_unlit_draw_count),
	FRAME_STATISTICS_FIELD(dynamic_unlit_triangle_count),
	FRAME_STATISTICS_FIELD(largest_dynamic_unlit_triangle_count),
	FRAME_STATISTICS_FIELD(dynamic_unlit_vertex_count),
	FRAME_STATISTICS_FIELD(reserved114),
	FRAME_STATISTICS_FIELD(dynamic_vertex_count),
	FRAME_STATISTICS_FIELD(dynamic_vertex_buffer_count),
	FRAME_STATISTICS_FIELD(dynamic_triangle_count),
	FRAME_STATISTICS_FIELD(dynamic_triangle_buffer_count),
	FRAME_STATISTICS_FIELD(debug_primitive_count),
	FRAME_STATISTICS_FIELD(reserved144),
	FRAME_STATISTICS_FIELD(dynamic_light_count),
	FRAME_STATISTICS_FIELD(lens_flare_count),
	FRAME_STATISTICS_FIELD(vertex_shader_skinning_constant_bytes),
	FRAME_STATISTICS_FIELD(vertex_shader_lighting_constant_bytes),
	FRAME_STATISTICS_FIELD(vertex_shader_instruction_count),
	FRAME_STATISTICS_FIELD(pixel_shader_pushbuffer_bytes),
	FRAME_STATISTICS_FIELD(model_skinning_constant_bytes),
	FRAME_STATISTICS_FIELD(model_lighting_constant_bytes),
	FRAME_STATISTICS_FIELD(reserved168),
};

static void frame_statistics_expect_signed(
	long *value)
{
	(void)value;
	return;
}

static void frame_statistics_expect_unsigned(
	unsigned long *value)
{
	(void)value;
	return;
}

#define EXPECT_SIGNED(member) frame_statistics_expect_signed(&statistics->member)
#define EXPECT_UNSIGNED(member) frame_statistics_expect_unsigned(&statistics->member)

#pragma warning(push)
#pragma warning(error: 4057)
void rasterizer_frame_statistics_type_contract(
	struct rasterizer_frame_statistics_globals *statistics)
{
	EXPECT_UNSIGNED(shadow_count);
	EXPECT_UNSIGNED(shadow_vertex_count);
	EXPECT_UNSIGNED(shadow_triangle_count);
	EXPECT_UNSIGNED(shadow_draw_count);
	EXPECT_UNSIGNED(environment_dynamic_vertex_count);
	EXPECT_UNSIGNED(environment_dynamic_triangle_count);
	EXPECT_UNSIGNED(environment_dynamic_draw_count);
	EXPECT_UNSIGNED(decal_vertex_count);
	EXPECT_UNSIGNED(decal_triangle_count);
	EXPECT_UNSIGNED(decal_draw_count);
	EXPECT_UNSIGNED(decal_shader_change_count);
	EXPECT_UNSIGNED(decal_texture_change_count);
	EXPECT_UNSIGNED(diffuse_texture_dynamic_vertex_count);
	EXPECT_UNSIGNED(diffuse_texture_dynamic_triangle_count);
	EXPECT_UNSIGNED(diffuse_texture_dynamic_draw_count);
	EXPECT_UNSIGNED(specular_light_dynamic_vertex_count);
	EXPECT_UNSIGNED(specular_light_dynamic_triangle_count);
	EXPECT_UNSIGNED(specular_light_dynamic_draw_count);
	EXPECT_UNSIGNED(specular_lightmap_dynamic_vertex_count);
	EXPECT_UNSIGNED(specular_lightmap_dynamic_triangle_count);
	EXPECT_UNSIGNED(specular_lightmap_dynamic_draw_count);
	EXPECT_UNSIGNED(reflection_mask_dynamic_vertex_count);
	EXPECT_UNSIGNED(reflection_mask_dynamic_triangle_count);
	EXPECT_UNSIGNED(reflection_mask_dynamic_draw_count);
	EXPECT_UNSIGNED(reflection_dynamic_vertex_count);
	EXPECT_UNSIGNED(reflection_dynamic_triangle_count);
	EXPECT_UNSIGNED(reflection_dynamic_draw_count);
	EXPECT_UNSIGNED(transparent_geometry_dynamic_vertex_count);
	EXPECT_UNSIGNED(transparent_geometry_dynamic_triangle_count);
	EXPECT_SIGNED(transparent_geometry_largest_dynamic_triangle_count);
	EXPECT_UNSIGNED(transparent_geometry_dynamic_draw_count);
	EXPECT_UNSIGNED(environment_fog_dynamic_vertex_count);
	EXPECT_UNSIGNED(environment_fog_dynamic_triangle_count);
	EXPECT_UNSIGNED(environment_fog_dynamic_draw_count);
	EXPECT_UNSIGNED(model_count);
	EXPECT_UNSIGNED(model_shadow_count);
	EXPECT_UNSIGNED(model_shadow_vertex_count);
	EXPECT_UNSIGNED(model_shadow_triangle_count);
	EXPECT_UNSIGNED(model_shadow_draw_count);
	EXPECT_UNSIGNED(dynamic_unlit_draw_count);
	EXPECT_UNSIGNED(dynamic_unlit_triangle_count);
	EXPECT_SIGNED(largest_dynamic_unlit_triangle_count);
	EXPECT_UNSIGNED(dynamic_unlit_vertex_count);
	EXPECT_SIGNED(dynamic_vertex_count);
	EXPECT_SIGNED(dynamic_vertex_buffer_count);
	EXPECT_SIGNED(dynamic_triangle_count);
	EXPECT_SIGNED(dynamic_triangle_buffer_count);
	EXPECT_SIGNED(debug_primitive_count);
	EXPECT_SIGNED(dynamic_light_count);
	EXPECT_SIGNED(lens_flare_count);
	EXPECT_SIGNED(vertex_shader_skinning_constant_bytes);
	EXPECT_SIGNED(vertex_shader_lighting_constant_bytes);
	EXPECT_SIGNED(vertex_shader_instruction_count);
	EXPECT_SIGNED(pixel_shader_pushbuffer_bytes);
	EXPECT_UNSIGNED(model_skinning_constant_bytes);
	EXPECT_UNSIGNED(model_lighting_constant_bytes);
	return;
}
#pragma warning(pop)

struct rasterizer_frame_statistics_globals *rasterizer_frame_statistics_owner_address(
	void)
{
	return &rasterizer_frame_statistics;
}

unsigned long rasterizer_frame_statistics_model_delta(
	long provider_after,
	unsigned long provider_before)
{
	return provider_after - provider_before;
}

unsigned long rasterizer_frame_statistics_model_accumulate(
	unsigned long accumulator,
	long provider_after,
	unsigned long provider_before)
{
	return accumulator + (provider_after - provider_before);
}
