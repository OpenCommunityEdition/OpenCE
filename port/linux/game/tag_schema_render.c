/*
TAG_SCHEMA_RENDER.C

The schemas (tag_schema.h) of bitmaps, shaders and what is drawn (bitm, the
shader groups, sky, fog, lens, ligh, glw!, cont, part, pctl, deca, ant!,
flag, elec, rain, wind, dobc, colo).

The rasterizer reads a shader as the structure of its type (base.type), not
of its group: each shader group's check makes the type its group's.
*/

/* ---------- headers */

#include "cseries.h"
#include "tag_schema.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/color_table_group.h"
#include "effects/contrail_definitions.h"
#include "effects/decal_definitions.h"
#include "effects/particle_system_definitions.h"
#include "effects/particles.h"
#include "effects/weather_particle_definitions.h"
#include "math/periodic_functions.h"
#include "objects/widgets/antenna.h"
#include "objects/objects.h"
#include "scenario/wind_definitions.h"
#include "shaders/shader_definitions.h"

/* ---------- constants */

/* the game's, which their units keep to themselves */
enum
{
	/* periodic_functions.c */
	NUMBER_OF_PERIODIC_FUNCTIONS = 12,

	/* xbox_texture_cache.c, bitmap_group.c */
	_bitmap_type_2d = 0,
	_bitmap_type_3d,
	_bitmap_type_cube_map,
	NUMBER_OF_BITMAP_TYPES,

	_bitmap_format_a8 = 0,
	_bitmap_format_dxt1 = 14,
	_bitmap_format_dxt3,
	_bitmap_format_dxt5,
	_bitmap_format_p8_bump,
	NUMBER_OF_BITMAP_FORMATS,

	_bitmap_has_power_of_two_dimensions_bit = 0,
	_bitmap_compressed_bit,
	_bitmap_palettized_bit,
	_bitmap_swizzled_bit,
	_bitmap_linear_bit,
	_bitmap_v16u16_bit,
	_bitmap_allocated_bit,
	_bitmap_cached_bit,

	_bitmap_group_type_sprites = 3,
	NUMBER_OF_BITMAP_GROUP_TYPES = 5,

	/* the device's (d3d8_gl.c, D3DDevice_GetDeviceCaps; xbox_textures.c
	uploads nothing larger) */
	MAXIMUM_BITMAP_SIZE = 4096,
	MAXIMUM_VOLUME_BITMAP_SIZE = 512,
	/* bitmaps.c */
	MAXIMUM_BITMAP_DEPTH = 256,
	/* a linear texture's pitch (D3DSIZE_PITCH_MASK: 256 steps of 64 bytes) */
	BITMAP_PITCH_ALIGNMENT = 64,
	MAXIMUM_BITMAP_PITCH = 256 * BITMAP_PITCH_ALIGNMENT,
	/* the texture cache's memory (xbox_texture_cache.c), which no bitmap's
	pixels can be more than */
	MAXIMUM_BITMAP_PIXELS_SIZE = 0x1600000,

	/* shaders.c */
	_shader_type_screen = 0,
	_shader_type_effect,
	_shader_type_decal,
	_shader_type_environment,
	_shader_type_model,
	_shader_type_transparent_generic,
	_shader_type_transparent_chicago,
	_shader_type_transparent_water,
	_shader_type_transparent_glass,
	_shader_type_transparent_meter,
	_shader_type_transparent_plasma,
	NUMBER_OF_SHADER_TYPES,

	/* rasterizer_xbox.c, rasterizer_xbox_transparent_geometry.c */
	NUMBER_OF_FRAMEBUFFER_BLEND_FUNCTIONS = 8,
	NUMBER_OF_FRAMEBUFFER_FADE_MODES = 3,
	/* none, or one of an object's four outgoing functions (render_animation's
	values[source - 1]) */
	NUMBER_OF_SHADER_ANIMATION_SOURCES = NUMBER_OF_OBJECT_FUNCTION_REFERENCES,
	NUMBER_OF_SHADER_TRANSPARENT_TYPES = 4,
	/* the four texture stages a transparent shader's maps go to (and their
	transforms, vsh_constants__texanim[8]) */
	NUMBER_OF_SHADER_TRANSPARENT_MAPS = 4,
	/* the pixel shader's combiner stages, of which a generic shader's stages
	take all but the last, the fog's (pixel_shader_definition) */
	NUMBER_OF_PIXEL_SHADER_STAGES = 8,
	MAXIMUM_SHADER_TRANSPARENT_GENERIC_STAGES = NUMBER_OF_PIXEL_SHADER_STAGES - 1,
	_shader_transparent_flag_numeric_bit = 7,

	NUMBER_OF_SHADER_TRANSPARENT_GLASS_REFLECTION_TYPES = 3,
	/* rasterizer_xbox_water.c */
	NUMBER_OF_WATER_RIPPLES = 4,

	/* shader_transparent_generic_preprocessor.c */
	NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_INPUTS = 25,
	NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_INPUT_MAPPINGS = 8,
	NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUTS = 9,
	NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUT_FUNCTIONS = 2,
	NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUT_MAPPINGS = 6,
	/* shader_transparent_chicago_preprocessor.c */
	NUMBER_OF_SHADER_FUNCTIONS = 13,

	/* render_sky.c */
	MAXIMUM_SKY_SHADER_FUNCTIONS = 8,
	MAXIMUM_SKY_ANIMATIONS = 8,

	/* rasterizer_xbox_environment.c, rasterizer_xbox_models.c */
	NUMBER_OF_SHADER_ENVIRONMENT_TYPES = 3,
	NUMBER_OF_SHADER_ENVIRONMENT_REFLECTION_TYPES = 3,
	NUMBER_OF_SHADER_DETAIL_FUNCTIONS = 3,
	NUMBER_OF_SHADER_MODEL_DETAIL_MASKS = 9,
	NUMBER_OF_SHADER_EFFECT_SECONDARY_MAP_ANCHORS = 3,

	/* render_sprite.h */
	NUMBER_OF_BUILD_SPRITE_ORIENTATIONS = 3,
	/* weather_particle_systems.c */
	MAXIMUM_NUMBER_OF_WEATHER_PARTICLE_TYPES = 8,
	NUMBER_OF_WEATHER_PARTICLE_RENDER_DIRECTION_SOURCES = 2,
	/* particle_systems.h */
	MAXIMUM_PARTICLE_SYSTEM_TYPES_PER_SYSTEM = 4,
	/* contrail_definitions.h: vertical, horizontal, media, ground, viewer, of
	which render_contrails draws all but ground */
	NUMBER_OF_CONTRAIL_RENDER_TYPES = 5,
	/* decals.c */
	NUMBER_OF_DECAL_LAYERS = 5,
	/* rasterizer_xbox_detail_objects.c: a constant for each type on the
	stack (type_data; its sprites' it caps itself); two collection types
	(vertex shader permutations), or none */
	MAXIMUM_DETAIL_OBJECT_TYPES_PER_COLLECTION = 16,
	NUMBER_OF_DETAIL_OBJECT_COLLECTION_TYPES = 2,

	/* rasterizer_lights.c */
	NUMBER_OF_LENS_FLARE_OCCLUSION_OFFSET_DIRECTIONS = 3,
	NUMBER_OF_LENS_FLARE_CORONA_ROTATION_FUNCTIONS = 5,
	NUMBER_OF_LENS_FLARE_REFLECTION_SCALE_FUNCTIONS = 4,
	/* rasterizer_xbox_environment_fog.c */
	MAXIMUM_ENVIRONMENT_FOG_SCREEN_LAYERS = 4,
	/* glow.c */
	NUMBER_OF_GLOW_BOUNDARY_EFFECTS = 2,
	NUMBER_OF_GLOW_PARTICLE_DISTRIBUTIONS = 2,
	NUMBER_OF_GLOW_TRAILING_PARTICLE_DISTRIBUTIONS = 3,
	/* flags.c: a flag's vertices, cells and rows (attachment_force_points[MAXIMUM_FLAG_HEIGHT],
	written to its height) and its attachment points, on the stack */
	MAXIMUM_FLAG_WIDTH = 40,
	MAXIMUM_FLAG_HEIGHT = 40,
	MAXIMUM_FLAG_ATTACHMENT_POINTS = 5,
	/* lightning.c: a bolt's points, on the stack (points[MAXIMUM_LIGHTNING_POINTS]),
	2 to the octaves to each next marker of them */
	MAXIMUM_LIGHTNING_POINTS = 4097,
	MAXIMUM_LIGHTNING_OCTAVES = 12,

	/* the tool's limits, where the game has none of its own */
	MAXIMUM_BITMAP_SEQUENCES = 256,
	MAXIMUM_BITMAP_SPRITES_PER_SEQUENCE = 64,
	MAXIMUM_BITMAPS = 2048,
	MAXIMUM_COLOR_TABLE_COLORS = 512,
	MAXIMUM_SKY_LIGHTS = 8,
	MAXIMUM_LENS_FLARE_REFLECTIONS = 32,
	MAXIMUM_LIGHTNING_MARKERS = 16,
	MAXIMUM_LIGHTNING_SHADERS = 1,
	MAXIMUM_PARTICLE_SYSTEM_PHYSICS_CONSTANTS = 16,
	MAXIMUM_PARTICLE_SYSTEM_TYPE_STATES = 8,
	MAXIMUM_PARTICLE_SYSTEM_PARTICLE_STATES = 8,
	MAXIMUM_SHADER_TRANSPARENT_LAYERS = 4,
	MAXIMUM_SHADER_TRANSPARENT_CHICAGO_EXTENDED_MAPS = 2,

	/* how deep a transparent shader's extra layers may nest: each is drawn
	by a recursion of rasterizer_transparent_geometry_group_draw (retail
	nests one) */
	MAXIMUM_SHADER_LAYER_DEPTH = 4,
	/* how long a chain of decals may be (decal_new follows it: retail's are
	two long) */
	MAXIMUM_DECAL_CHAIN_LENGTH = 16,
};

/* the rates the game steps through in a loop, a frame or a point at a time
(particle_update, contrail_update, contrail_compute_new_point_count): a
negative one never ends it, one too fast to change the time left does not
either. Retail's are at most 30 frames and 1500 points a second */
#define MAXIMUM_ANIMATION_FRAMES_PER_SECOND 1000.0f
#define MAXIMUM_CONTRAIL_POINTS_PER_SECOND 10000.0f
/* the shortest a looping particle system's states may take in all, which the
game steps through state after state as time passes (particle_system_update):
retail's take at least a second */
#define MINIMUM_PARTICLE_SYSTEM_LOOP_TIME 0.01f

/* ---------- structures */

/* rasterizer_xbox_transparent_geometry.c, shader_transparent_*_preprocessor.c */

struct shader_transparent_generic_map
{
	word flags;
	word pad02;
	real map_u_scale;
	real map_v_scale;
	real map_u_offset;
	real map_v_offset;
	real map_rotation;
	real mipmap_bias;
	struct tag_reference map;
	struct shader_texture_animation map_animation;
};

struct shader_transparent_generic_stage
{
	word flags;
	short pad02;
	short constant_color0_animation_source;
	short constant_color0_animation_function;
	real constant_color0_animation_period;
	real_argb_color constant_color0_lower_bound;
	real_argb_color constant_color0_upper_bound;
	real_argb_color color1;
	short color_input_A;
	short color_input_A_mapping;
	short color_input_B;
	short color_input_B_mapping;
	short color_input_C;
	short color_input_C_mapping;
	short color_input_D;
	short color_input_D_mapping;
	short color_output_AB;
	short color_output_AB_function;
	short color_output_CD;
	short color_output_CD_function;
	short color_output_AB_CD_mux_sum;
	short color_output_mapping;
	short alpha_input_A;
	short alpha_input_A_mapping;
	short alpha_input_B;
	short alpha_input_B_mapping;
	short alpha_input_C;
	short alpha_input_C_mapping;
	short alpha_input_D;
	short alpha_input_D_mapping;
	short alpha_output_A;
	short alpha_output_B;
	short alpha_output_C;
	short alpha_output_mapping;
};

/* a transparent shader's extra layer: a shader drawn before it */
struct shader_layer
{
	struct tag_reference shader;
};

/* (the part of a generic and a chicago shader after their shader) */
struct shader_transparent
{
	byte numeric_counter_limit;
	byte flags;
	short first_map_type;
	short framebuffer_blend_function;
	short framebuffer_fade_mode;
	short framebuffer_fade_source;
	short pad32;
	real lens_flare_spacing;
	struct tag_reference lens_flare;
	struct tag_block extra_layers;
	struct tag_block maps;
};

struct shader_transparent_generic_definition
{
	struct shader shader;
	struct shader_transparent transparent;
	struct tag_block stages;
};

struct shader_transparent_chicago_map
{
	word flags;
	byte reserved02[42];
	short color_function;
	short alpha_function;
	byte reserved30[36];
	real map_u_scale;
	real map_v_scale;
	real map_u_offset;
	real map_v_offset;
	real map_rotation;
	real mipmap_bias;
	struct tag_reference map;
	byte reserved7C[40];
	struct shader_texture_animation map_animation;
};

struct shader_transparent_chicago_definition
{
	struct shader shader;
	struct shader_transparent transparent;
	unsigned long extra_flags;
	byte reserved64[8];
};

/* a chicago shader with two-stage maps (Custom Edition's scex): the game
has no such type, and reads it as a chicago shader, whose maps are its
four-stage maps */
struct shader_transparent_chicago_extended_definition
{
	struct shader shader;
	struct shader_transparent transparent;
	struct tag_block two_stage_maps;
	unsigned long extra_flags;
	byte reserved70[8];
};

typedef char verify_shader_size[sizeof(struct shader) == 0x28 ? 1 : -1];
typedef char verify_shader_transparent_generic_map_size[
	sizeof(struct shader_transparent_generic_map) == 0x64 ? 1 : -1];
typedef char verify_shader_transparent_generic_stage_size[
	sizeof(struct shader_transparent_generic_stage) == 0x70 ? 1 : -1];
typedef char verify_shader_transparent_generic_stage_inputs_offset[
	offsetof(struct shader_transparent_generic_stage, color_input_A) == 0x3C ? 1 : -1];
typedef char verify_shader_transparent_generic_maps_offset[
	offsetof(struct shader_transparent_generic_definition, transparent.maps) == 0x54 ? 1 : -1];
typedef char verify_shader_transparent_generic_size[
	sizeof(struct shader_transparent_generic_definition) == 0x6C ? 1 : -1];
typedef char verify_shader_transparent_chicago_map_size[
	sizeof(struct shader_transparent_chicago_map) == 0xDC ? 1 : -1];
typedef char verify_shader_transparent_chicago_map_map_offset[
	offsetof(struct shader_transparent_chicago_map, map) == 0x6C ? 1 : -1];
typedef char verify_shader_transparent_chicago_extra_flags_offset[
	offsetof(struct shader_transparent_chicago_definition, extra_flags) == 0x60 ? 1 : -1];
typedef char verify_shader_transparent_chicago_size[
	sizeof(struct shader_transparent_chicago_definition) == 0x6C ? 1 : -1];
typedef char verify_shader_transparent_chicago_extended_size[
	sizeof(struct shader_transparent_chicago_extended_definition) == 0x78 ? 1 : -1];

/* rasterizer_xbox_transparent_geometry.c */

struct shader_transparent_glass_definition
{
	struct shader shader;
	word flags;
	short pad2A;
	byte reserved2C[40];
	real_rgb_color tint_color;
	real tint_map_scale;
	struct tag_reference tint_map;
	byte reserved74[20];
	word reflection_flags;
	short reflection_type;
	real_argb_color reflection_view_perpendicular_color;
	real_argb_color reflection_view_parallel_color;
	struct tag_reference reflection_map;
	real reflection_bump_map_scale;
	struct tag_reference reflection_bump_map;
	byte reservedD0[128];
	word diffuse_flags;
	word pad152;
	real diffuse_map_scale;
	struct tag_reference diffuse_map;
	real diffuse_detail_map_scale;
	struct tag_reference diffuse_detail_map;
	byte reserved17C[100];
};

struct shader_transparent_meter_definition
{
	struct shader shader;
	word flags;
	short pad2A;
	byte reserved2C[32];
	struct tag_reference map;
	byte reserved5C[0xA8];
};

/* rasterizer_xbox_water.c */

struct water_ripple
{
	byte reserved00[4];
	real contribution_factor;
	byte reserved08[0x20];
	real animation_angle;
	real animation_velocity;
	real_point2d map_offset;
	short map_repeats;
	short map_index;
	byte reserved3C[0x10];
};

struct shader_transparent_water_definition
{
	struct shader shader;
	unsigned short flags;
	short type;
	byte reserved2C[0x20];
	struct tag_reference base_map;
	byte reserved5C[0x10];
	real_argb_color view_perpendicular_tint_color;
	real_argb_color view_parallel_tint_color;
	byte reserved8C[0x10];
	struct tag_reference reflection_map;
	byte reservedAC[0x10];
	real ripple_animation_angle;
	real ripple_animation_velocity;
	real ripple_scale;
	struct tag_reference ripple_maps;
	short ripple_mipmap_levels;
	short pad0DA;
	real ripple_mipmap_fade_factor;
	real ripple_mipmap_lod_bias;
	byte reserved0E4[0x40];
	struct tag_block ripples;
	byte reserved130[0x10];
};

/* rasterizer_xbox_plasma_energy.c (whose offsets are from the end of the
shader), rasterizer_xbox_models.c */

struct shader_transparent_plasma_definition
{
	struct shader shader;
	byte reserved28[4];
	short intensity_exponent_source;
	short pad2E;
	real intensity_exponent;
	short offset_exponent_source;
	short pad36;
	real offset_amount;
	real offset_exponent;
	byte reserved40[0x20];
	real perpendicular_alpha;
	real_rgb_color perpendicular_color;
	real parallel_alpha;
	real_rgb_color parallel_color;
	short color_source;
	byte reserved82[0x3E];
	real primary_noise_map_animation_period;
	real_vector3d primary_noise_map_animation_direction;
	real primary_noise_map_scale;
	struct tag_reference primary_noise_map;
	byte reservedE4[0x24];
	real secondary_noise_map_animation_period;
	real_vector3d secondary_noise_map_animation_direction;
	real secondary_noise_map_scale;
	struct tag_reference secondary_noise_map;
	byte reserved12C[0x20];
};

typedef char verify_shader_transparent_glass_reflection_type_offset[
	offsetof(struct shader_transparent_glass_definition, reflection_type) == 0x8A ? 1 : -1];
typedef char verify_shader_transparent_glass_diffuse_detail_map_offset[
	offsetof(struct shader_transparent_glass_definition, diffuse_detail_map) == 0x16C ? 1 : -1];
typedef char verify_shader_transparent_glass_size[
	sizeof(struct shader_transparent_glass_definition) == 0x1E0 ? 1 : -1];
typedef char verify_shader_transparent_meter_map_offset[
	offsetof(struct shader_transparent_meter_definition, map) == 0x4C ? 1 : -1];
typedef char verify_shader_transparent_meter_size[
	sizeof(struct shader_transparent_meter_definition) == 0x104 ? 1 : -1];
typedef char verify_water_ripple_size[sizeof(struct water_ripple) == 0x4C ? 1 : -1];
typedef char verify_water_ripple_map_index_offset[offsetof(struct water_ripple, map_index) == 0x3A ? 1 : -1];
typedef char verify_shader_transparent_water_ripple_maps_offset[
	offsetof(struct shader_transparent_water_definition, ripple_maps) == 0xC8 ? 1 : -1];
typedef char verify_shader_transparent_water_ripples_offset[
	offsetof(struct shader_transparent_water_definition, ripples) == 0x124 ? 1 : -1];
typedef char verify_shader_transparent_water_size[
	sizeof(struct shader_transparent_water_definition) == 0x140 ? 1 : -1];
typedef char verify_shader_transparent_plasma_intensity_exponent_source_offset[
	offsetof(struct shader_transparent_plasma_definition, intensity_exponent_source) == 0x2C ? 1 : -1];
typedef char verify_shader_transparent_plasma_primary_noise_map_offset[
	offsetof(struct shader_transparent_plasma_definition, primary_noise_map) == 0xD4 ? 1 : -1];
typedef char verify_shader_transparent_plasma_secondary_noise_map_offset[
	offsetof(struct shader_transparent_plasma_definition, secondary_noise_map) == 0x11C ? 1 : -1];
typedef char verify_shader_transparent_plasma_size[
	sizeof(struct shader_transparent_plasma_definition) == 0x14C ? 1 : -1];

/* rasterizer_xbox_environment.c, rasterizer_xbox_models.c, shaders.c,
object_lights.c (the fields the game reads) */

struct shader_environment_definition
{
	struct shader shader;
	word flags;
	short type;
	real lens_flare_spacing;
	struct tag_reference lens_flare;
	byte reserved40[0x2C];
	/* diffuse */
	word diffuse_flags;
	short pad6E;
	byte reserved70[0x18];
	struct tag_reference base_map;
	byte reserved98[0x18];
	short detail_map_function;
	short padB2;
	real primary_detail_map_scale;
	struct tag_reference primary_detail_map;
	real secondary_detail_map_scale;
	struct tag_reference secondary_detail_map;
	byte reservedDC[0x18];
	short micro_detail_map_function;
	short padF6;
	real micro_detail_map_scale;
	struct tag_reference micro_detail_map;
	real_rgb_color material_color;
	byte reserved118[0xC];
	real bump_map_scale;
	struct tag_reference bump_map;
	real_vector2d runtime_bump_map_scale;
	byte reserved140[0x10];
	short u_animation_function;
	short pad152;
	real u_animation_period;
	real u_animation_scale;
	short v_animation_function;
	short pad15E;
	real v_animation_period;
	real v_animation_scale;
	byte reserved168[0x18];
	/* self-illumination */
	word self_illumination_flags;
	short pad182;
	byte reserved184[0x18];
	real_rgb_color primary_on_color;
	real_rgb_color primary_off_color;
	short primary_animation_function;
	short pad1B6;
	real primary_animation_period;
	real primary_animation_phase;
	byte reserved1C0[0x18];
	real_rgb_color secondary_on_color;
	real_rgb_color secondary_off_color;
	short secondary_animation_function;
	short pad1F2;
	real secondary_animation_period;
	real secondary_animation_phase;
	byte reserved1FC[0x18];
	real_rgb_color plasma_on_color;
	real_rgb_color plasma_off_color;
	short plasma_animation_function;
	short pad22E;
	real plasma_animation_period;
	real plasma_animation_phase;
	byte reserved238[0x18];
	real self_illumination_map_scale;
	struct tag_reference self_illumination_map;
	byte reserved264[0x18];
	/* specular */
	word specular_flags;
	short specular_type;
	byte reserved280[0x50];
	/* reflection */
	word reflection_flags;
	short reflection_type;
	byte reserved2D4[0x50];
	struct tag_reference reflection_cube_map;
	byte reserved334[0x10];
};

struct shader_model_definition
{
	struct shader shader;
	word flags;
	short type;
	byte reserved2C[0xC];
	real translucency;
	byte reserved3C[0x10];
	short change_color_source;
	byte reserved4E[0x1E];
	word self_illumination_flags;
	short pad6E;
	short self_illumination_color_source;
	short self_illumination_animation_function;
	real self_illumination_animation_period;
	real_rgb_color self_illumination_color_lower_bound;
	real_rgb_color self_illumination_color_upper_bound;
	byte reserved90[0xC];
	real map_u_scale;
	real map_v_scale;
	struct tag_reference base_map;
	byte reservedB4[0x8];
	struct tag_reference multipurpose_map;
	byte reservedCC[0x8];
	short detail_function;
	short detail_mask;
	real detail_map_scale;
	struct tag_reference detail_map;
	real detail_map_v_scale;
	byte reservedF0[0xC];
	struct shader_texture_animation texture_animation;
	byte reserved134[0x8];
	real reflection_falloff_distance;
	real reflection_cutoff_distance;
	real perpendicular_brightness;
	real_rgb_color perpendicular_tint_color;
	real parallel_brightness;
	real_rgb_color parallel_tint_color;
	struct tag_reference reflection_cube_map;
	byte reserved174[0x44];
};

typedef char verify_shader_environment_base_map_offset[
	offsetof(struct shader_environment_definition, base_map) == 0x88 ? 1 : -1];
typedef char verify_shader_environment_detail_map_function_offset[
	offsetof(struct shader_environment_definition, detail_map_function) == 0xB0 ? 1 : -1];
typedef char verify_shader_environment_micro_detail_map_offset[
	offsetof(struct shader_environment_definition, micro_detail_map) == 0xFC ? 1 : -1];
typedef char verify_shader_environment_bump_map_offset[
	offsetof(struct shader_environment_definition, bump_map) == 0x128 ? 1 : -1];
typedef char verify_shader_environment_u_animation_function_offset[
	offsetof(struct shader_environment_definition, u_animation_function) == 0x150 ? 1 : -1];
typedef char verify_shader_environment_v_animation_period_offset[
	offsetof(struct shader_environment_definition, v_animation_period) == 0x160 ? 1 : -1];
typedef char verify_shader_environment_primary_animation_function_offset[
	offsetof(struct shader_environment_definition, primary_animation_function) == 0x1B4 ? 1 : -1];
typedef char verify_shader_environment_secondary_animation_function_offset[
	offsetof(struct shader_environment_definition, secondary_animation_function) == 0x1F0 ? 1 : -1];
typedef char verify_shader_environment_plasma_animation_function_offset[
	offsetof(struct shader_environment_definition, plasma_animation_function) == 0x22C ? 1 : -1];
typedef char verify_shader_environment_self_illumination_map_offset[
	offsetof(struct shader_environment_definition, self_illumination_map) == 0x254 ? 1 : -1];
typedef char verify_shader_environment_specular_flags_offset[
	offsetof(struct shader_environment_definition, specular_flags) == 0x27C ? 1 : -1];
typedef char verify_shader_environment_reflection_flags_offset[
	offsetof(struct shader_environment_definition, reflection_flags) == 0x2D0 ? 1 : -1];
typedef char verify_shader_environment_reflection_cube_map_offset[
	offsetof(struct shader_environment_definition, reflection_cube_map) == 0x324 ? 1 : -1];
typedef char verify_shader_environment_size[sizeof(struct shader_environment_definition) == 0x344 ? 1 : -1];
typedef char verify_shader_model_translucency_offset[
	offsetof(struct shader_model_definition, translucency) == 0x38 ? 1 : -1];
typedef char verify_shader_model_change_color_source_offset[
	offsetof(struct shader_model_definition, change_color_source) == 0x4C ? 1 : -1];
typedef char verify_shader_model_self_illumination_animation_function_offset[
	offsetof(struct shader_model_definition, self_illumination_animation_function) == 0x72 ? 1 : -1];
typedef char verify_shader_model_base_map_offset[offsetof(struct shader_model_definition, base_map) == 0xA4 ? 1 : -1];
typedef char verify_shader_model_multipurpose_map_offset[
	offsetof(struct shader_model_definition, multipurpose_map) == 0xBC ? 1 : -1];
typedef char verify_shader_model_detail_function_offset[
	offsetof(struct shader_model_definition, detail_function) == 0xD4 ? 1 : -1];
typedef char verify_shader_model_detail_map_offset[offsetof(struct shader_model_definition, detail_map) == 0xDC ? 1 : -1];
typedef char verify_shader_model_texture_animation_offset[
	offsetof(struct shader_model_definition, texture_animation) == 0xFC ? 1 : -1];
typedef char verify_shader_model_reflection_cube_map_offset[
	offsetof(struct shader_model_definition, reflection_cube_map) == 0x164 ? 1 : -1];
typedef char verify_shader_model_size[sizeof(struct shader_model_definition) == 0x1B8 ? 1 : -1];

/* weather_particle_systems.c */

struct weather_particle_type_definition
{
	char name[32];
	unsigned long flags;
	real distance_fade[4];
	real height_fade[4];
	long unused44[24];
	real particle_count_lower_bound;
	real particle_count_upper_bound;
	struct tag_reference physics;
	long unusedBC[4];
	real acceleration[4];
	long unusedDC[8];
	real radius_lower_bound;
	real radius_upper_bound;
	real animation_rate_lower_bound;
	real animation_rate_upper_bound;
	real rotation_rate_lower_bound;
	real rotation_rate_upper_bound;
	long unused114[8];
	real_argb_color color_lower_bound;
	real_argb_color color_upper_bound;
	real runtime_one_over_sprite_width;
	long unused158[15];
	struct tag_reference bitmap;
	short render_mode;
	short render_direction_source;
	struct shader_effect_definition shader;
};

typedef char verify_weather_particle_type_definition_bitmap_offset[
	offsetof(struct weather_particle_type_definition, bitmap) == 0x194 ? 1 : -1];
typedef char verify_weather_particle_type_definition_size[
	sizeof(struct weather_particle_type_definition) == 0x25C ? 1 : -1];

/* decals.c */

struct decal_shader_definition
{
	struct shader shader;
	word flags;
	short type;
	short framebuffer_blend_function;
	word pad2E;
	long unused30[5];
	struct tag_reference map;
	long unused54[5];
};

struct decal_definition
{
	word flags;
	short type;
	short layer;
	word pad006;
	struct tag_reference next_decal_in_chain;
	real radius_lower_bound;
	real radius_upper_bound;
	long unused020[3];
	real intensity_lower_bound;
	real intensity_upper_bound;
	real_rgb_color color_lower_bound;
	real_rgb_color color_upper_bound;
	long unused04C[3];
	short animation_loop_frame_index;
	short animation_speed;
	long unused05C[7];
	real lifetime_lower_bound;
	real lifetime_upper_bound;
	real decay_time_lower_bound;
	real decay_time_upper_bound;
	long unused088[3];
	struct decal_shader_definition shader;
	real runtime_maximum_sprite_extent;
	word runtime_incremental_counter;
	word pad102;
	long unused104[2];
};

typedef char verify_decal_definition_map_index_offset[
	offsetof(struct decal_definition, shader.map.index) == 0xE4 ? 1 : -1];
typedef char verify_decal_definition_size[sizeof(struct decal_definition) == 0x10C ? 1 : -1];

/* rasterizer_xbox_detail_objects.c */

struct detail_object_type_definition
{
	char name[32];
	byte sequence_index;
	byte flags;
	byte first_sprite_index;
	byte sprite_count;
	real color_override_factor;
	long unused28[2];
	real near_fade_distance;
	real far_fade_distance;
	real size_min;
	real size_max;
	byte reserved40[0x20];
};

struct detail_object_collection_definition
{
	short collection_type;
	word pad02;
	real global_z_offset;
	long unused08[11];
	struct tag_reference map;
	struct tag_block type_definitions;
	long unused50[12];
};

typedef char verify_detail_object_type_definition_size[sizeof(struct detail_object_type_definition) == 0x60 ? 1 : -1];
typedef char verify_detail_object_collection_definition_size[
	sizeof(struct detail_object_collection_definition) == 0x80 ? 1 : -1];

/* rasterizer_lights.c */

struct lens_flare_reflection
{
	word flags;
	short type;
	short bitmap_index;
	word pad06;
	byte reserved08[0x14];
	real offset;
	real rotation_offset;
	byte reserved24[0x4];
	real radius_lower_bound;
	real radius_upper_bound;
	short radius_scale_function;
	word pad32;
	real brightness_lower_bound;
	real brightness_upper_bound;
	short brightness_scale_function;
	word pad3E;
	real_argb_color tint_color;
	real_argb_color animation_color_lower_bound;
	real_argb_color animation_color_upper_bound;
	word animation_flags;
	short animation_function;
	real animation_period;
	real animation_phase;
	byte reserved7C[0x4];
};

struct lens_flare_definition
{
	real falloff_angle;
	real cutoff_angle;
	real runtime_cosine_falloff_angle;
	real runtime_cosine_cutoff_angle;
	real occlusion_radius;
	short occlusion_offset_direction;
	word pad16;
	real near_fade_distance;
	real far_fade_distance;
	struct tag_reference primary_map;
	word flags;
	word pad32;
	byte reserved34[0x4C];
	short corona_rotation_function;
	word pad82;
	real corona_rotation_function_scale;
	byte reserved88[0x18];
	real_vector2d corona_radius_scale;
	byte reservedA8[0x1C];
	struct tag_block reflections;
	byte reservedD0[0x20];
};

typedef char verify_lens_flare_reflection_size[sizeof(struct lens_flare_reflection) == 0x80 ? 1 : -1];
typedef char verify_lens_flare_reflections_offset[offsetof(struct lens_flare_definition, reflections) == 0xC4 ? 1 : -1];
typedef char verify_lens_flare_size[sizeof(struct lens_flare_definition) == 0xF0 ? 1 : -1];

/* object_lights.c, rasterizer_xbox_environment.c */

struct light_definition
{
	unsigned long flags;
	real radius;
	real radius_modifier_lower_bound;
	real radius_modifier_upper_bound;
	real falloff_angle;
	real cutoff_angle;
	real lens_flare_radius;
	real runtime_cosine_falloff_angle;
	real runtime_cosine_cutoff_angle;
	real specular_radius_multiplier;
	real runtime_sine_cutoff_angle;
	byte reserved2C[0x8];
	unsigned long color_interpolation_flags;
	real_argb_color color_lower_bound;
	real_argb_color color_upper_bound;
	byte reserved58[0xC];
	/* gel */
	struct tag_reference gel_map;
	word pad74;
	short texture_animation_function;
	real texture_animation_rate;
	struct tag_reference gel_secondary_map;
	word pad8C;
	short yaw_function;
	real yaw_period;
	word pad94;
	short roll_function;
	real roll_period;
	word pad9C;
	short pitch_function;
	real pitch_period;
	byte reservedA4[0x8];
	struct tag_reference lens_flare;
	byte reservedBC[0x38];
	real transition_duration;
	word padF8;
	short falloff_function;
	byte reservedFC[0x64];
};

typedef char verify_light_gel_map_offset[offsetof(struct light_definition, gel_map) == 0x64 ? 1 : -1];
typedef char verify_light_pitch_function_offset[offsetof(struct light_definition, pitch_function) == 0x9E ? 1 : -1];
typedef char verify_light_lens_flare_offset[offsetof(struct light_definition, lens_flare) == 0xAC ? 1 : -1];
typedef char verify_light_falloff_function_offset[offsetof(struct light_definition, falloff_function) == 0xFA ? 1 : -1];
typedef char verify_light_size[sizeof(struct light_definition) == 0x160 ? 1 : -1];

/* fog_definitions.h, structures.c, rasterizer_xbox_environment_fog.c */

struct fog_screen_definition
{
	word flags;
	short layer_count;
	real near_distance;
	real far_distance;
	real near_density;
	real far_density;
	real start_distance_from_fog_plane;
	byte reserved18[4];
	pixel32 color;
	real rotation_multiplier;
	real strafing_multiplier;
	real zoom_multiplier;
	byte reserved2C[8];
	real map_scale;
	struct tag_reference map;
	real animation_period;
	real animation_unused;
	real wind_velocity[2];
	real wind_period[2];
	real wind_acceleration_weight;
	real wind_perpendicular_weight;
};

struct fog_definition_full
{
	word flags;
	word pad02;
	real animation_distance;
	byte unused08[0x50];
	real maximum_density;
	byte unused5C[4];
	real maximum_distance;
	byte unused64[4];
	real maximum_depth;
	byte unused6C[8];
	real plane_distance;
	real_rgb_color color;
	struct fog_screen_definition screen;
	byte reservedEC[8];
	struct tag_reference background_sound;
	struct tag_reference sound_environment;
	byte reserved114[0x78];
};

typedef char verify_fog_screen_offset[offsetof(struct fog_definition_full, screen) == 0x84 ? 1 : -1];
typedef char verify_fog_screen_map_offset[offsetof(struct fog_definition_full, screen.map) == 0xBC ? 1 : -1];
typedef char verify_fog_plane_distance_offset[offsetof(struct fog_definition_full, plane_distance) == 0x74 ? 1 : -1];
typedef char verify_fog_background_sound_offset[
	offsetof(struct fog_definition_full, background_sound.index) == 0x100 ? 1 : -1];
typedef char verify_fog_sound_environment_offset[offsetof(struct fog_definition_full, sound_environment) == 0x104 ? 1 : -1];
typedef char verify_fog_size[sizeof(struct fog_definition_full) == 0x18C ? 1 : -1];

/* glow.c */

struct glow_definition
{
	char attachment_marker[32];
	short number_of_particles;
	short boundary_effect;
	short particle_distribution;
	short trailing_particle_distribution;
	unsigned long flags;
	long unused02C[7];
	short render_mode;
	short render_orientation;
	long render_flags;
	short particle_rotational_velocity_attachment_index;
	short pad052;
	real particle_rotational_velocity;
	real particle_rotational_velocity_scale_lower_bound;
	real particle_rotational_velocity_scale_upper_bound;
	short effect_rotational_velocity_attachment_index;
	short pad062;
	real effect_rotational_velocity;
	real effect_rotational_velocity_scale_lower_bound;
	real effect_rotational_velocity_scale_upper_bound;
	short effect_translational_velocity_attachment_index;
	short pad072;
	real effect_translational_velocity;
	real effect_translational_velocity_scale_lower_bound;
	real effect_translational_velocity_scale_upper_bound;
	short distance_to_object_attachment_index;
	short pad082;
	real minimum_distance_to_object;
	real maximum_distance_to_object;
	real distance_to_object_scale_lower_bound;
	real distance_to_object_scale_upper_bound;
	long unused094[2];
	short particle_size_attachment_index;
	short pad09E;
	real particle_size_lower_bound;
	real particle_size_upper_bound;
	real particle_size_scale_lower_bound;
	real particle_size_scale_upper_bound;
	short color_attachment_index;
	short pad0B2;
	real_argb_color color_lower_bound;
	real_argb_color color_upper_bound;
	real_argb_color scale_color_lower_bound;
	real_argb_color scale_color_upper_bound;
	real color_rate_of_change;
	real percentage_edge_fade;
	real trailing_particle_generation_frequency;
	real trailing_particle_lifetime;
	real trailing_particle_velocity;
	real trailing_particle_minimum_t;
	real trailing_particle_maximum_t;
	long unused110[13];
	struct tag_reference texture;
};

typedef char verify_glow_flags_offset[offsetof(struct glow_definition, flags) == 0x28 ? 1 : -1];
typedef char verify_glow_color_lower_bound_offset[offsetof(struct glow_definition, color_lower_bound) == 0xB4 ? 1 : -1];
typedef char verify_glow_size[sizeof(struct glow_definition) == 0x154 ? 1 : -1];

/* flags.c */

struct flag_attachment_point
{
	short height_to_next_attachment;
	short pad2;
	long unused[4];
	char marker_name[32];
};

struct flag_definition
{
	unsigned long flags;
	short trailing_edge_shape;
	short trailing_edge_offset;
	short attached_edge_shape;
	short padA;
	short width;
	short height;
	real cell_width_scale;
	real cell_height_scale;
	struct tag_reference shader_red;
	struct tag_reference physics;
	real wind_noise;
	long unused3C[2];
	struct tag_reference shader_blue;
	struct tag_block attachment_points;
};

typedef char verify_flag_attachment_point_size[sizeof(struct flag_attachment_point) == 0x34 ? 1 : -1];
typedef char verify_flag_size[sizeof(struct flag_definition) == 0x60 ? 1 : -1];

/* lightning.c */

struct lightning_marker_definition
{
	char attachment_marker[32];
	word flags;
	short type;
	short octaves_to_next_marker;
	word pad26;
	byte reserved28[0x4C];
	real_vector3d random_position_bounds;
	real random_jitter_offset;
	real thickness;
	real_argb_color tint;
	byte reserved98[0x4C];
};

struct lightning_shader
{
	struct shader_effect_definition shader;
};

struct lightning_definition
{
	word flags;
	short count;
	byte reserved04[0x10];
	real near_fade_distance;
	real far_fade_distance;
	byte reserved1C[0x10];
	short jitter_scale_source;
	short thickness_scale_source;
	short tint_modulation_source;
	short brightness_scale_source;
	struct tag_reference map;
	byte reserved44[0x54];
	struct tag_block markers;
	struct tag_block shaders;
	byte reservedB0[0x58];
};

typedef char verify_lightning_marker_size[sizeof(struct lightning_marker_definition) == 0xE4 ? 1 : -1];
typedef char verify_lightning_shader_size[sizeof(struct lightning_shader) == 0xB4 ? 1 : -1];
typedef char verify_lightning_markers_offset[offsetof(struct lightning_definition, markers) == 0x98 ? 1 : -1];
typedef char verify_lightning_size[sizeof(struct lightning_definition) == 0x108 ? 1 : -1];

enum
{
	_lightning_marker_not_connected_to_next_marker_bit = 0,
};

/* render_sky.c, sky_definitions.h */

struct sky_shader_function
{
	long unused;
	char global_function_name[TAG_STRING_LENGTH + 1];
};

struct sky_animation
{
	short animation_index;
	word pad02;
	real period;
	byte unused08[0x1C];
};

struct sky_light
{
	struct tag_reference lens_flare;
	char marker_name[TAG_STRING_LENGTH + 1];
	byte unused30[0x44];
};

struct sky_definition
{
	struct tag_reference model;
	struct tag_reference animation_graph;
	byte unused20[0x78];
	struct tag_reference indoor_fog_screen;
	long unusedA8;
	struct tag_block shader_functions;
	struct tag_block animations;
	struct tag_block lights;
};

typedef char verify_sky_shader_function_size[sizeof(struct sky_shader_function) == 0x24 ? 1 : -1];
typedef char verify_sky_animation_size[sizeof(struct sky_animation) == 0x24 ? 1 : -1];
typedef char verify_sky_light_size[sizeof(struct sky_light) == 0x74 ? 1 : -1];
typedef char verify_sky_indoor_fog_screen_offset[offsetof(struct sky_definition, indoor_fog_screen) == 0x98 ? 1 : -1];
typedef char verify_sky_animations_offset[offsetof(struct sky_definition, animations) == 0xB8 ? 1 : -1];
typedef char verify_sky_size[sizeof(struct sky_definition) == 0xD0 ? 1 : -1];

/* ---------- private code */

static short integer_floor_log2(
	unsigned long value)
{
	short result = 0;

	while (value > 1)
	{
		value >>= 1;
		result++;
	}

	return result;
}

/* bitmaps */

static char const bitmap_format_bits_per_pixel[NUMBER_OF_BITMAP_FORMATS] =
{
	8, 8, 8, 16, 0, 0, 16, 0, 16, 16, 32, 32, 0, 0, 4, 8, 8, 8
};

/* the bytes of pixels the game reads for a bitmap (bitmap_get_pixel_data_size:
every mipmap's width by height by depth, compressed sides rounded to 4, a
cube map's six faces, at its format's bits a pixel), or NONE when they are
more than the texture cache holds or a long counts. The bitmap's type,
format and dimensions have been checked */
static long bitmap_read_pixel_data_size(
	struct bitmap_data const *bitmap)
{
	boolean compressed = TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) != 0;
	long bits_per_pixel = bitmap_format_bits_per_pixel[bitmap->format];
	unsigned long pixel_count = 0;
	short mipmap_index;

	for (mipmap_index = 0; mipmap_index <= bitmap->mipmap_count; mipmap_index++)
	{
		unsigned long width = MAX(bitmap->width >> mipmap_index, 1);
		unsigned long height = MAX(bitmap->height >> mipmap_index, 1);
		unsigned long depth = MAX(bitmap->depth >> mipmap_index, 1);

		if (compressed)
		{
			width += (-width) & 3;
			height += (-height) & 3;
		}
		pixel_count += width * height * depth * (bitmap->type == _bitmap_type_cube_map ? 6 : 1);
	}
	/* (at most 4096x4096x6 and a third again for mipmaps, so no overflow
	yet; the bytes could overflow a long) */
	if (!bits_per_pixel || pixel_count > (unsigned long)MAXIMUM_BITMAP_PIXELS_SIZE * 8 / bits_per_pixel)
		return NONE;

	return (long)(pixel_count * bits_per_pixel / 8);
}

/* a bitmap of no pixels the game can read: one a8 pixel at the start of the
file (there is always a file) */
static void bitmap_make_empty(
	struct bitmap_data *bitmap)
{
	bitmap->type = _bitmap_type_2d;
	bitmap->format = _bitmap_format_a8;
	bitmap->width = 1;
	bitmap->height = 1;
	bitmap->depth = 1;
	bitmap->mipmap_count = 0;
	SET_FLAG(bitmap->flags, _bitmap_compressed_bit, FALSE);
	SET_FLAG(bitmap->flags, _bitmap_linear_bit, FALSE);
	bitmap->pixels_offset = 0;
	bitmap->pixels_size = 1;

	return;
}

/* what the texture cache makes of a bitmap (texture_cache_initialize_hardware_format,
rasterizer_xbox_bitmap_get_pixel_data_size) must be a texture the device has:
a format with a hardware format, dimensions it takes, mipmaps there are, a
linear pitch it can say. The pixels are read from the map file into a cache
block of at least pixels_size bytes */
static boolean bitmap_data_check(
	struct tag_validation *validation,
	void *base)
{
	struct bitmap_data *bitmap = base;
	boolean volume = bitmap->type == _bitmap_type_3d;
	boolean compressed;
	short maximum_size = volume ? MAXIMUM_VOLUME_BITMAP_SIZE : MAXIMUM_BITMAP_SIZE;
	short maximum_depth = volume ? MAXIMUM_BITMAP_DEPTH : 1;
	short maximum_mipmap_count;

	if (!bitmap_format_bits_per_pixel[bitmap->format])
	{
		tag_validate_correct(validation, "has the unused format %d: a8", bitmap->format);
		bitmap->format = _bitmap_format_a8;
	}
	compressed = bitmap->format >= _bitmap_format_dxt1 && bitmap->format <= _bitmap_format_dxt5;
	if (TEST_FLAG(bitmap->flags, _bitmap_compressed_bit) != compressed)
	{
		tag_validate_correct(validation, "says it is%s compressed, but its format %d is%s",
			compressed ? " not" : "", bitmap->format, compressed ? "" : " not");
		SET_FLAG(bitmap->flags, _bitmap_compressed_bit, compressed);
	}
	if (bitmap->width < 1 || bitmap->width > maximum_size ||
		bitmap->height < 1 || bitmap->height > maximum_size ||
		bitmap->depth < 1 || bitmap->depth > maximum_depth)
	{
		tag_validate_correct(validation, "is %dx%dx%d, past the device's %dx%dx%d: cut",
			bitmap->width, bitmap->height, bitmap->depth, maximum_size, maximum_size, maximum_depth);
		bitmap->width = (short)PIN(bitmap->width, 1, maximum_size);
		bitmap->height = (short)PIN(bitmap->height, 1, maximum_size);
		bitmap->depth = (short)PIN(bitmap->depth, 1, maximum_depth);
	}
	maximum_mipmap_count = integer_floor_log2(MAX(bitmap->width, MAX(bitmap->height, bitmap->depth)));
	if (bitmap->mipmap_count < 0 || bitmap->mipmap_count > maximum_mipmap_count)
	{
		tag_validate_correct(validation, "has %d mipmaps, not 0 to %d: cut", bitmap->mipmap_count,
			maximum_mipmap_count);
		bitmap->mipmap_count = (short)PIN(bitmap->mipmap_count, 0, maximum_mipmap_count);
	}
	/* (a linear texture has a format with a linear form, and a pitch of
	64-byte steps: a Custom Edition map's rows are padded to them as its
	pixels load, custom_edition_bitmaps.c) */
	if (TEST_FLAG(bitmap->flags, _bitmap_linear_bit))
	{
		long pitch = (long)bitmap->width * bitmap_format_bits_per_pixel[bitmap->format] / 8;
		long padded_pitch = (pitch + BITMAP_PITCH_ALIGNMENT - 1) / BITMAP_PITCH_ALIGNMENT * BITMAP_PITCH_ALIGNMENT;

		if (compressed || bitmap->format == _bitmap_format_p8_bump || bitmap->type != _bitmap_type_2d ||
			(pitch != padded_pitch && !tag_validate_custom_edition(validation)) || padded_pitch > MAXIMUM_BITMAP_PITCH)
		{
			tag_validate_correct(validation, "is linear, but of format %d, type %d, pitch %ld: not linear",
				bitmap->format, bitmap->type, pitch);
			SET_FLAG(bitmap->flags, _bitmap_linear_bit, FALSE);
		}
	}
	/* (the pixels the game reads are as many as its dimensions, format and
	mipmaps make, whatever the map says: texture_cache_bitmap_new) */
	if (bitmap->pixels_size < 0 || bitmap->pixels_size > MAXIMUM_BITMAP_PIXELS_SIZE ||
		!tag_validate_file_contains(validation, bitmap->pixels_offset, bitmap->pixels_size) ||
		!tag_validate_file_contains(validation, bitmap->pixels_offset, bitmap_read_pixel_data_size(bitmap)))
	{
		tag_validate_correct(validation, "has %ld (reads %ld) bytes of pixels at %08lx, outside the map's files: none",
			bitmap->pixels_size, bitmap_read_pixel_data_size(bitmap), (unsigned long)bitmap->pixels_offset);
		bitmap_make_empty(bitmap);
	}

	return TRUE;
}

/* a sequence's bitmaps are in the group's */
static boolean bitmap_group_check(
	struct tag_validation *validation,
	void *base)
{
	struct bitmap_group *group = base;
	long sequence_index;

	/* (the texture cache adds the pixel data's offset in the file to each
	bitmap's own: the sum must be in the map's files, as bitmap_data_check
	has the bitmap's alone) */
	{
		long bitmap_index;

		if (!tag_validate_file_contains(validation, group->pixel_data.file_offset, 0))
		{
			tag_validate_correct(validation, "has its pixel data at %08lx, outside the map's files: 0",
				(unsigned long)group->pixel_data.file_offset);
			group->pixel_data.file_offset = 0;
		}
		for (bitmap_index = 0; bitmap_index < group->bitmaps.count; bitmap_index++)
		{
			struct bitmap_data *bitmap = (struct bitmap_data *)group->bitmaps.address + bitmap_index;
			unsigned long offset = (unsigned long)group->pixel_data.file_offset + (unsigned long)bitmap->pixels_offset;

			if (offset > (unsigned long)LONG_MAX ||
				!tag_validate_file_contains(validation, (long)offset, bitmap->pixels_size) ||
				!tag_validate_file_contains(validation, (long)offset, bitmap_read_pixel_data_size(bitmap)))
			{
				tag_validate_correct(validation,
					"has bitmap %ld with %ld (reads %ld) bytes of pixels at %08lx past its data at %08lx, outside the map's files: none",
					bitmap_index, bitmap->pixels_size, bitmap_read_pixel_data_size(bitmap),
					(unsigned long)bitmap->pixels_offset, (unsigned long)group->pixel_data.file_offset);
				bitmap_make_empty(bitmap);
				/* (the group's offset is added by the game: the start of the
				file is where it stands) */
				bitmap->pixels_offset = -group->pixel_data.file_offset;
			}
		}
	}
	for (sequence_index = 0; sequence_index < group->sequences.count; sequence_index++)
	{
		struct bitmap_group_sequence *sequence = (struct bitmap_group_sequence *)group->sequences.address +
			sequence_index;

		if (sequence->first_bitmap_index < 0 || sequence->bitmap_count < 0 ||
			sequence->first_bitmap_index + sequence->bitmap_count > group->bitmaps.count ||
			(sequence->bitmap_count && sequence->first_bitmap_index >= group->bitmaps.count))
		{
			tag_validate_correct(validation, "has sequence %ld of bitmaps %d to %d of its %ld: none",
				sequence_index, sequence->first_bitmap_index,
				sequence->first_bitmap_index + sequence->bitmap_count, group->bitmaps.count);
			sequence->first_bitmap_index = 0;
			sequence->bitmap_count = 0;
		}
	}

	return TRUE;
}

/* shaders */

static boolean shader_type_check(
	struct tag_validation *validation,
	void *base,
	short type)
{
	struct shader *shader = base;

	if (shader->base.type != type)
	{
		tag_validate_correct(validation, "is a shader of type %d in a group of type %d: %d",
			shader->base.type, type, type);
		shader->base.type = type;
	}

	return TRUE;
}

/* the extra layers of a generic or chicago shader (NULL for any other) */
static struct tag_block *shader_extra_layers(
	struct tag_validation *validation,
	long shader_index)
{
	struct shader_transparent_generic_definition *generic =
		tag_validate_tag_get(validation, shader_index, 'sotr');
	struct shader_transparent_chicago_definition *chicago =
		tag_validate_tag_get(validation, shader_index, 'schi');
	struct shader_transparent_chicago_extended_definition *chicago_extended =
		tag_validate_tag_get(validation, shader_index, 'scex');

	if (generic)
		return &generic->transparent.extra_layers;
	if (chicago)
		return &chicago->transparent.extra_layers;
	if (chicago_extended)
		return &chicago_extended->transparent.extra_layers;

	return NULL;
}

/* whether a shader's layers go deeper than depth more (a ring of layers
does, as each is drawn by a recursion) */
static boolean shader_layers_too_deep(
	struct tag_validation *validation,
	long shader_index,
	short depth)
{
	struct tag_block *layers = shader_extra_layers(validation, shader_index);
	long layer_index;

	if (!layers || !layers->count)
		return FALSE;
	if (!depth)
		return TRUE;
	for (layer_index = 0; layer_index < layers->count; layer_index++)
	{
		if (shader_layers_too_deep(validation, ((struct tag_reference *)layers->address)[layer_index].index,
			(short)(depth - 1)))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/* a transparent shader: its layers nest no deeper than the game draws, and a
numeric one's first map has the digits it divides by */
static void shader_transparent_check(
	struct tag_validation *validation,
	struct shader_transparent *transparent,
	struct tag_reference const *first_map)
{
	long layer_index;

	for (layer_index = 0; layer_index < transparent->extra_layers.count; layer_index++)
	{
		struct tag_reference *layer = (struct tag_reference *)transparent->extra_layers.address + layer_index;

		if (shader_layers_too_deep(validation, layer->index, MAXIMUM_SHADER_LAYER_DEPTH - 1))
		{
			tag_validate_correct(validation, "has layer %ld, whose layers nest deeper than %d: none",
				layer_index, MAXIMUM_SHADER_LAYER_DEPTH);
			layer->index = NONE;
		}
	}
	if (TEST_FLAG(transparent->flags, _shader_transparent_flag_numeric_bit) && first_map)
	{
		struct bitmap_group *digits = tag_validate_tag_get(validation, first_map->index, 'bitm');

		if (!digits || digits->bitmaps.count <= 0 || digits->bitmaps.count > SHORT_MAX)
		{
			tag_validate_correct(validation, "is numeric, but its first map has no digits: not numeric");
			SET_FLAG(transparent->flags, _shader_transparent_flag_numeric_bit, FALSE);
		}
	}

	return;
}

static boolean shader_transparent_generic_check(
	struct tag_validation *validation,
	void *base)
{
	struct shader_transparent_generic_definition *generic = base;

	shader_type_check(validation, base, _shader_type_transparent_generic);
	shader_transparent_check(validation, &generic->transparent, generic->transparent.maps.count ?
		&((struct shader_transparent_generic_map *)generic->transparent.maps.address)->map : NULL);

	return TRUE;
}

static boolean shader_transparent_chicago_check(
	struct tag_validation *validation,
	void *base)
{
	struct shader_transparent_chicago_definition *chicago = base;

	shader_type_check(validation, base, _shader_type_transparent_chicago);
	shader_transparent_check(validation, &chicago->transparent, chicago->transparent.maps.count ?
		&((struct shader_transparent_chicago_map *)chicago->transparent.maps.address)->map : NULL);

	return TRUE;
}

static boolean shader_transparent_chicago_extended_check(
	struct tag_validation *validation,
	void *base)
{
	struct shader_transparent_chicago_extended_definition *chicago = base;

	shader_type_check(validation, base, _shader_type_transparent_chicago);
	/* (it is drawn as a chicago shader, which reads its extra flags where
	this one's two stage maps' count is: the game never reads the maps) */
	if (chicago->two_stage_maps.count)
	{
		tag_validate_correct(validation, "has %ld two stage maps, which the game reads as its flags: none",
			chicago->two_stage_maps.count);
		chicago->two_stage_maps.count = 0;
		chicago->two_stage_maps.address = NULL;
	}
	shader_transparent_check(validation, &chicago->transparent, chicago->transparent.maps.count ?
		&((struct shader_transparent_chicago_map *)chicago->transparent.maps.address)->map : NULL);

	return TRUE;
}

static boolean shader_transparent_glass_check(
	struct tag_validation *validation,
	void *base)
{
	return shader_type_check(validation, base, _shader_type_transparent_glass);
}

static boolean shader_transparent_meter_check(
	struct tag_validation *validation,
	void *base)
{
	return shader_type_check(validation, base, _shader_type_transparent_meter);
}

static boolean shader_transparent_water_check(
	struct tag_validation *validation,
	void *base)
{
	return shader_type_check(validation, base, _shader_type_transparent_water);
}

static boolean shader_transparent_plasma_check(
	struct tag_validation *validation,
	void *base)
{
	return shader_type_check(validation, base, _shader_type_transparent_plasma);
}

static boolean shader_environment_check(
	struct tag_validation *validation,
	void *base)
{
	struct shader_environment_definition *environment = base;
	struct bitmap_group *base_map = tag_validate_tag_get(validation, environment->base_map.index, 'bitm');

	shader_type_check(validation, base, _shader_type_environment);
	/* (lights take a lightmap's permutation modulo its base map's bitmaps,
	object_lights.c) */
	if (base_map && base_map->bitmaps.count <= 0)
	{
		tag_validate_correct(validation, "has a base map with no bitmaps: none");
		environment->base_map.index = NONE;
	}

	return TRUE;
}

static boolean shader_model_check(
	struct tag_validation *validation,
	void *base)
{
	return shader_type_check(validation, base, _shader_type_model);
}

/* a shader inside a particle, contrail or weather particle: drawn as an
effect shader */
static boolean shader_effect_check(
	struct tag_validation *validation,
	void *base)
{
	return shader_type_check(validation, base, _shader_type_effect);
}

/* a rate the game steps through a step at a time: none to maximum */
static void rate_check(
	struct tag_validation *validation,
	char const *name,
	real *rate,
	real maximum)
{
	if (!(*rate >= 0.0f && *rate <= maximum))
	{
		real corrected = *rate > maximum ? maximum : 0.0f;

		tag_validate_correct(validation, "has a %s of %f, not 0 to %f: %f", name, *rate, maximum, corrected);
		*rate = corrected;
	}

	return;
}

/* a time: none or more */
static void time_check(
	struct tag_validation *validation,
	char const *name,
	real *time)
{
	if (!(*time >= 0.0f))
	{
		tag_validate_correct(validation, "has a %s of %f: 0", name, *time);
		*time = 0.0f;
	}

	return;
}

/* particles */

static boolean particle_check(
	struct tag_validation *validation,
	void *base)
{
	struct particle_definition *particle = base;

	rate_check(validation, "frame rate", &particle->frames_per_second_lower_bound, MAXIMUM_ANIMATION_FRAMES_PER_SECOND);
	rate_check(validation, "frame rate", &particle->frames_per_second_upper_bound, MAXIMUM_ANIMATION_FRAMES_PER_SECOND);
	/* (seconds added to a frame's on contact) */
	time_check(validation, "contact deterioration", &particle->frames_per_second_contact_deterioration);

	return TRUE;
}

/* particle systems */

static boolean particle_system_type_state_check(
	struct tag_validation *validation,
	void *base)
{
	struct particle_system_type_state *state = base;

	time_check(validation, "duration", &state->duration_lower_bound);
	time_check(validation, "duration", &state->duration_upper_bound);
	time_check(validation, "transition time", &state->transition_time_lower_bound);
	time_check(validation, "transition time", &state->transition_time_upper_bound);

	return TRUE;
}

static boolean particle_system_particle_state_check(
	struct tag_validation *validation,
	void *base)
{
	struct particle_system_type_particle_state *state = base;

	time_check(validation, "duration", &state->duration_lower_bound);
	time_check(validation, "duration", &state->duration_upper_bound);
	time_check(validation, "transition time", &state->transition_time_lower_bound);
	time_check(validation, "transition time", &state->transition_time_upper_bound);

	return TRUE;
}

/* a type whose states loop takes time going round them; a particle state's
sequence (the next one, for rotational sprites) has sprites, which a
particle's sprite is taken modulo (particle_systems_render): a type with one
that does not is disabled */
static boolean particle_system_type_check(
	struct tag_validation *validation,
	void *base)
{
	struct particle_system_type *type = base;
	real loop_time;
	long state_index;

	/* (the states' checks have run: their times are none or more) */
	loop_time = 0.0f;
	for (state_index = 0; state_index < type->type_states.count; state_index++)
	{
		struct particle_system_type_state *state = (struct particle_system_type_state *)type->type_states.address +
			state_index;

		loop_time += state->duration_lower_bound + state->transition_time_lower_bound;
	}
	if (TEST_FLAG(type->flags, _particle_system_type_type_states_loop_bit) && loop_time < MINIMUM_PARTICLE_SYSTEM_LOOP_TIME)
	{
		tag_validate_correct(validation, "loops through type states of %f seconds: no loop", loop_time);
		SET_FLAG(type->flags, _particle_system_type_type_states_loop_bit, FALSE);
	}
	loop_time = 0.0f;
	for (state_index = 0; state_index < type->particle_states.count; state_index++)
	{
		struct particle_system_type_particle_state *state =
			(struct particle_system_type_particle_state *)type->particle_states.address + state_index;
		struct bitmap_group *bitmap = tag_validate_tag_get(validation, state->bitmaps.index, 'bitm');
		long sequence_index = state->sequence_index +
			(type->complex_sprite_render_mode == _particle_system_type_complex_sprite_render_mode_rotational);

		loop_time += state->duration_lower_bound + state->transition_time_lower_bound;
		if (!TEST_FLAG(type->flags, _particle_system_type_disabled_bit) &&
			(!bitmap || state->sequence_index < 0 || sequence_index >= bitmap->sequences.count ||
				((struct bitmap_group_sequence *)bitmap->sequences.address)[sequence_index].sprites.count <= 0))
		{
			tag_validate_correct(validation, "has particle state %ld of sequence %ld, which has no sprites: disabled",
				state_index, sequence_index);
			SET_FLAG(type->flags, _particle_system_type_disabled_bit, TRUE);
		}
	}
	if (TEST_FLAG(type->flags, _particle_system_type_particle_states_loop_bit) &&
		loop_time < MINIMUM_PARTICLE_SYSTEM_LOOP_TIME)
	{
		tag_validate_correct(validation, "loops through particle states of %f seconds: no loop", loop_time);
		SET_FLAG(type->flags, _particle_system_type_particle_states_loop_bit, FALSE);
	}

	return TRUE;
}

/* contrails */

static boolean contrail_check(
	struct tag_validation *validation,
	void *base)
{
	struct contrail_definition *contrail = base;

	rate_check(validation, "point generation rate", &contrail->point_generation_rate, MAXIMUM_CONTRAIL_POINTS_PER_SECOND);
	rate_check(validation, "frame rate", &contrail->frames_per_second, MAXIMUM_ANIMATION_FRAMES_PER_SECOND);
	/* (a ground contrail is not drawn: render_contrails returns with its
	buffers locked) */
	if (contrail->render_type == _contrail_render_type_ground)
	{
		tag_validate_correct(validation, "has the ground render type, which is not drawn: vertical");
		contrail->render_type = _contrail_render_type_vertical;
	}
	/* (sequences from the first to the count after, each checked against the
	bitmap's as it is used) */
	if (contrail->first_sequence_index < 0 || contrail->sequence_count < 0)
	{
		tag_validate_correct(validation, "has sequences %d and %d more: 0 and none", contrail->first_sequence_index,
			contrail->sequence_count);
		contrail->first_sequence_index = 0;
		contrail->sequence_count = 0;
	}

	return TRUE;
}

/* decals */

/* a decal's chain ends (decal_new makes each decal in it) */
static boolean decal_check(
	struct tag_validation *validation,
	void *base)
{
	struct decal_definition *decal = base;
	struct decal_definition *next = decal;
	short length = 0;

	while (next && next->next_decal_in_chain.index != NONE && length <= MAXIMUM_DECAL_CHAIN_LENGTH)
	{
		next = tag_validate_tag_get(validation, next->next_decal_in_chain.index, 'deca');
		length++;
	}
	if (length > MAXIMUM_DECAL_CHAIN_LENGTH)
	{
		tag_validate_correct(validation, "has a chain of more than %d decals: none", MAXIMUM_DECAL_CHAIN_LENGTH);
		decal->next_decal_in_chain.index = NONE;
	}

	return TRUE;
}

/* detail objects */

/* (detail_object_build_vertices takes an object's sprite modulo its type's
sprite count) */
static boolean detail_object_type_check(
	struct tag_validation *validation,
	void *base)
{
	struct detail_object_type_definition *type = base;

	if (!type->sprite_count)
	{
		tag_validate_correct(validation, "has no sprites: 1");
		type->sprite_count = 1;
	}

	return TRUE;
}

/* fog */

/* (a fog screen's layers are the rasterizer's layers[MAXIMUM_ENVIRONMENT_FOG_SCREEN_LAYERS];
its bitmap is a layer's taken modulo its bitmaps) */
static boolean fog_check(
	struct tag_validation *validation,
	void *base)
{
	struct fog_definition_full *fog = base;
	struct bitmap_group *map = tag_validate_tag_get(validation, fog->screen.map.index, 'bitm');

	if (fog->screen.layer_count > MAXIMUM_ENVIRONMENT_FOG_SCREEN_LAYERS)
	{
		tag_validate_correct(validation, "has %d screen layers, more than the game's %d: cut",
			fog->screen.layer_count, MAXIMUM_ENVIRONMENT_FOG_SCREEN_LAYERS);
		fog->screen.layer_count = MAXIMUM_ENVIRONMENT_FOG_SCREEN_LAYERS;
	}
	if (map && map->bitmaps.count <= 0)
	{
		tag_validate_correct(validation, "has a screen map with no bitmaps: none");
		fog->screen.map.index = NONE;
	}

	return TRUE;
}

/* glows */

/* the bitmap bitmap_group_get_bitmap_from_sequence finds, or NULL */
static struct bitmap_data *bitmap_from_sequence(
	struct bitmap_group *group,
	short sequence_index,
	short frame_index)
{
	short bitmap_index = NONE;

	if (group->sequences.count > 0 && sequence_index >= 0)
	{
		struct bitmap_group_sequence *sequence = (struct bitmap_group_sequence *)group->sequences.address +
			sequence_index % group->sequences.count;

		if (sequence->bitmap_count > 0)
		{
			bitmap_index = (short)(frame_index % sequence->bitmap_count + sequence->first_bitmap_index);
		}
		else if (sequence->sprites.count && frame_index >= 0)
		{
			bitmap_index = frame_index < sequence->sprites.count ?
				((struct bitmap_group_sprite *)sequence->sprites.address)[frame_index].bitmap_index : 0;
		}
	}
	if (bitmap_index == NONE)
		bitmap_index = frame_index;

	return bitmap_index >= 0 && bitmap_index < group->bitmaps.count ?
		(struct bitmap_data *)group->bitmaps.address + bitmap_index : NULL;
}

/* a glow's sprite is its texture's first sequence's first sprite, whose
bitmap glow_new reads; its particles move along it at a bounded speed */
static boolean glow_check(
	struct tag_validation *validation,
	void *base)
{
	struct glow_definition *glow = base;
	struct bitmap_group *texture = tag_validate_tag_get(validation, glow->texture.index, 'bitm');

	if (texture && texture->type == _bitmap_group_type_sprites)
	{
		struct bitmap_group_sprite *sprite = NULL;

		if (texture->sequences.count > 0 && ((struct bitmap_group_sequence *)texture->sequences.address)->sprites.count > 0)
			sprite = (struct bitmap_group_sprite *)((struct bitmap_group_sequence *)texture->sequences.address)->sprites.address;
		if (!bitmap_from_sequence(texture, 0, sprite ? sprite->bitmap_index : 0))
		{
			tag_validate_correct(validation, "has a texture whose first sprite has no bitmap: none");
			glow->texture.index = NONE;
		}
	}
	/* (its particles' times move along it at any speed: glow.c keeps them
	within its length) */

	return TRUE;
}

/* flags */

static boolean flag_check(
	struct tag_validation *validation,
	void *base)
{
	struct flag_definition *flag = base;

	/* (a flag of 1 row or column has none before its first, flag_render;
	one wider than MAXIMUM_FLAG_WIDTH is not drawn) */
	if (flag->width < 2 || flag->height < 2 || flag->height >= MAXIMUM_FLAG_HEIGHT)
	{
		tag_validate_correct(validation, "is %dx%d, not 2x2 to %dx%d: cut", flag->width, flag->height,
			MAXIMUM_FLAG_WIDTH - 1, MAXIMUM_FLAG_HEIGHT - 1);
		flag->width = (short)PIN(flag->width, 2, MAXIMUM_FLAG_WIDTH);
		flag->height = (short)PIN(flag->height, 2, MAXIMUM_FLAG_HEIGHT - 1);
	}

	return TRUE;
}

/* lightning */

/* (each marker connected to the next takes 2 to its octaves of the bolt's
points, as lightning_render counts them: no more than it has) */
static boolean lightning_check(
	struct tag_validation *validation,
	void *base)
{
	struct lightning_definition *lightning = base;
	long point_count = 0;
	long marker_index;

	for (marker_index = 0; marker_index < lightning->markers.count; marker_index++)
	{
		struct lightning_marker_definition *marker =
			(struct lightning_marker_definition *)lightning->markers.address + marker_index;

		if (marker->octaves_to_next_marker < 0 || marker->octaves_to_next_marker > MAXIMUM_LIGHTNING_OCTAVES)
		{
			tag_validate_correct(validation, "has marker %ld of %d octaves, not 0 to %d: cut", marker_index,
				marker->octaves_to_next_marker, MAXIMUM_LIGHTNING_OCTAVES);
			marker->octaves_to_next_marker = (short)PIN(marker->octaves_to_next_marker, 0, MAXIMUM_LIGHTNING_OCTAVES);
		}
		if (marker_index == lightning->markers.count - 1 ||
			TEST_FLAG(marker->flags, _lightning_marker_not_connected_to_next_marker_bit))
		{
			continue;
		}
		if (point_count + (1L << marker->octaves_to_next_marker) > MAXIMUM_LIGHTNING_POINTS - 1)
		{
			tag_validate_correct(validation, "has marker %ld of %d octaves, more points than the game's %d: not connected",
				marker_index, marker->octaves_to_next_marker, MAXIMUM_LIGHTNING_POINTS);
			SET_FLAG(marker->flags, _lightning_marker_not_connected_to_next_marker_bit, TRUE);
			continue;
		}
		point_count += 1L << marker->octaves_to_next_marker;
	}

	return TRUE;
}

/* ---------- globals */

/* bitmaps */

static struct tag_schema_field const bitmap_sprite_fields[] =
{
	TAG_SCHEMA_BLOCK_INDEX(struct bitmap_group_sprite, bitmap_index, TAG_SCHEMA_ROOT,
		offsetof(struct bitmap_group, bitmaps), 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const bitmap_sprite_schema =
	TAG_SCHEMA_DEFINITION(bitmap_sprite, struct bitmap_group_sprite, bitmap_sprite_fields);

static struct tag_schema_field const bitmap_sequence_fields[] =
{
	TAG_SCHEMA_STRING(struct bitmap_group_sequence, name),
	TAG_SCHEMA_BLOCK(struct bitmap_group_sequence, sprites, bitmap_sprite_schema,
		MAXIMUM_BITMAP_SPRITES_PER_SEQUENCE),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const bitmap_sequence_schema =
	TAG_SCHEMA_DEFINITION(bitmap_sequence, struct bitmap_group_sequence, bitmap_sequence_fields);

/* (the texture cache sets a bitmap's cache block, base address and
hardware format as it loads its pixels, texture_cache_start_loading_bitmap;
a cached bitmap's hardware format is never read, an uncached one's is a
pointer the rasterizer made, which no map has. The map's base addresses are
the tool's) */
static struct tag_schema_field const bitmap_data_fields[] =
{
	TAG_SCHEMA_ENUM(struct bitmap_data, type, NUMBER_OF_BITMAP_TYPES, 0),
	TAG_SCHEMA_ENUM(struct bitmap_data, format, NUMBER_OF_BITMAP_FORMATS, 0),
	TAG_SCHEMA_TAG_INDEX(struct bitmap_data, tag_index, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_RESET(struct bitmap_data, cache_block_index, NONE),
	TAG_SCHEMA_RESET(struct bitmap_data, hardware_format, 0),
	TAG_SCHEMA_RESET(struct bitmap_data, base_address, 0),
	TAG_SCHEMA_CHECK(bitmap_data_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const bitmap_data_schema =
	TAG_SCHEMA_DEFINITION(bitmap_data, struct bitmap_data, bitmap_data_fields);

/* (the pixels are each bitmap's, in the file: the group's pixel data is
empty in a map) */
static struct tag_schema_field const bitmap_group_fields[] =
{
	TAG_SCHEMA_ENUM(struct bitmap_group, type, NUMBER_OF_BITMAP_GROUP_TYPES, 0),
	TAG_SCHEMA_BLOCK(struct bitmap_group, sequences, bitmap_sequence_schema, MAXIMUM_BITMAP_SEQUENCES),
	TAG_SCHEMA_BLOCK(struct bitmap_group, bitmaps, bitmap_data_schema, MAXIMUM_BITMAPS),
	TAG_SCHEMA_CHECK(bitmap_group_check),
	TAG_SCHEMA_END
};

/* shaders */

static struct tag_schema_field const shader_texture_animation_fields[] =
{
	TAG_SCHEMA_ENUM(struct shader_texture_animation, u_source, NUMBER_OF_SHADER_ANIMATION_SOURCES, 0),
	TAG_SCHEMA_ENUM(struct shader_texture_animation, u_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_texture_animation, v_source, NUMBER_OF_SHADER_ANIMATION_SOURCES, 0),
	TAG_SCHEMA_ENUM(struct shader_texture_animation, v_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_texture_animation, r_source, NUMBER_OF_SHADER_ANIMATION_SOURCES, 0),
	TAG_SCHEMA_ENUM(struct shader_texture_animation, r_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_texture_animation_schema =
	TAG_SCHEMA_DEFINITION(shader_texture_animation, struct shader_texture_animation,
		shader_texture_animation_fields);

static struct tag_schema_field const shader_fields[] =
{
	TAG_SCHEMA_ENUM(struct shader, base.type, NUMBER_OF_SHADER_TYPES, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_schema =
	TAG_SCHEMA_DEFINITION(shader, struct shader, shader_fields);

/* (a tag of the shader group itself is no shader the game can draw: it
draws a shader as its type says, from the bytes past a shader's own, which
such a tag doesn't have. The tools make none) */
static boolean shader_group_check(
	struct tag_validation *validation,
	void *base)
{
	(void)base;
	tag_validate_refuse(validation, "is a shader of no kind");

	return FALSE;
}

static struct tag_schema_field const shader_definition_fields[] =
{
	TAG_SCHEMA_CHECK(shader_group_check),
	TAG_SCHEMA_END
};

static struct tag_schema_field const shader_layer_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct shader_layer, shader, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_layer_schema =
	TAG_SCHEMA_DEFINITION(shader_layer, struct shader_layer, shader_layer_fields);

static struct tag_schema_field const shader_transparent_fields[] =
{
	TAG_SCHEMA_ENUM(struct shader_transparent, first_map_type, NUMBER_OF_SHADER_TRANSPARENT_TYPES, 0),
	TAG_SCHEMA_ENUM(struct shader_transparent, framebuffer_blend_function, NUMBER_OF_FRAMEBUFFER_BLEND_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_transparent, framebuffer_fade_mode, NUMBER_OF_FRAMEBUFFER_FADE_MODES, 0),
	TAG_SCHEMA_ENUM(struct shader_transparent, framebuffer_fade_source, NUMBER_OF_SHADER_ANIMATION_SOURCES, 0),
	TAG_SCHEMA_REFERENCE(struct shader_transparent, lens_flare, TAG_SCHEMA_GROUPS('lens')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_transparent_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent, struct shader_transparent, shader_transparent_fields);

/* generic */

static struct tag_schema_field const shader_transparent_generic_map_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct shader_transparent_generic_map, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_STRUCT(struct shader_transparent_generic_map, map_animation, shader_texture_animation_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_transparent_generic_map_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_generic_map, struct shader_transparent_generic_map,
		shader_transparent_generic_map_fields);

#define STAGE_INPUT(field) \
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, field, NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_INPUTS, 0), \
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, field##_mapping, \
		NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_INPUT_MAPPINGS, 0)
#define STAGE_OUTPUT(field) \
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, field, NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUTS, 0)

/* (the preprocessor's tables, shader_transparent_generic_preprocessor.c) */
static struct tag_schema_field const shader_transparent_generic_stage_fields[] =
{
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, constant_color0_animation_source,
		NUMBER_OF_SHADER_ANIMATION_SOURCES, 0),
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, constant_color0_animation_function,
		NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	STAGE_INPUT(color_input_A),
	STAGE_INPUT(color_input_B),
	STAGE_INPUT(color_input_C),
	STAGE_INPUT(color_input_D),
	STAGE_OUTPUT(color_output_AB),
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, color_output_AB_function,
		NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUT_FUNCTIONS, 0),
	STAGE_OUTPUT(color_output_CD),
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, color_output_CD_function,
		NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUT_FUNCTIONS, 0),
	STAGE_OUTPUT(color_output_AB_CD_mux_sum),
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, color_output_mapping,
		NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUT_MAPPINGS, 0),
	STAGE_INPUT(alpha_input_A),
	STAGE_INPUT(alpha_input_B),
	STAGE_INPUT(alpha_input_C),
	STAGE_INPUT(alpha_input_D),
	STAGE_OUTPUT(alpha_output_A),
	STAGE_OUTPUT(alpha_output_B),
	STAGE_OUTPUT(alpha_output_C),
	TAG_SCHEMA_ENUM(struct shader_transparent_generic_stage, alpha_output_mapping,
		NUMBER_OF_SHADER_TRANSPARENT_GENERIC_STAGE_OUTPUT_MAPPINGS, 0),
	TAG_SCHEMA_END
};

#undef STAGE_INPUT
#undef STAGE_OUTPUT

static struct tag_schema_definition const shader_transparent_generic_stage_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_generic_stage, struct shader_transparent_generic_stage,
		shader_transparent_generic_stage_fields);

/* (maps: the four texture stages; stages: the combiners but the fog's) */
static struct tag_schema_field const shader_transparent_generic_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_generic_definition, shader, shader_schema),
	TAG_SCHEMA_STRUCT(struct shader_transparent_generic_definition, transparent, shader_transparent_schema),
	TAG_SCHEMA_BLOCK(struct shader_transparent_generic_definition, transparent.extra_layers, shader_layer_schema,
		MAXIMUM_SHADER_TRANSPARENT_LAYERS),
	TAG_SCHEMA_BLOCK(struct shader_transparent_generic_definition, transparent.maps,
		shader_transparent_generic_map_schema, NUMBER_OF_SHADER_TRANSPARENT_MAPS),
	TAG_SCHEMA_BLOCK(struct shader_transparent_generic_definition, stages, shader_transparent_generic_stage_schema,
		MAXIMUM_SHADER_TRANSPARENT_GENERIC_STAGES),
	TAG_SCHEMA_CHECK(shader_transparent_generic_check),
	TAG_SCHEMA_END
};

/* chicago */

static struct tag_schema_field const shader_transparent_chicago_map_fields[] =
{
	TAG_SCHEMA_ENUM(struct shader_transparent_chicago_map, color_function, NUMBER_OF_SHADER_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_transparent_chicago_map, alpha_function, NUMBER_OF_SHADER_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_chicago_map, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_STRUCT(struct shader_transparent_chicago_map, map_animation, shader_texture_animation_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_transparent_chicago_map_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_chicago_map, struct shader_transparent_chicago_map,
		shader_transparent_chicago_map_fields);

static struct tag_schema_field const shader_transparent_chicago_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_chicago_definition, shader, shader_schema),
	TAG_SCHEMA_STRUCT(struct shader_transparent_chicago_definition, transparent, shader_transparent_schema),
	TAG_SCHEMA_BLOCK(struct shader_transparent_chicago_definition, transparent.extra_layers, shader_layer_schema,
		MAXIMUM_SHADER_TRANSPARENT_LAYERS),
	TAG_SCHEMA_BLOCK(struct shader_transparent_chicago_definition, transparent.maps,
		shader_transparent_chicago_map_schema, NUMBER_OF_SHADER_TRANSPARENT_MAPS),
	TAG_SCHEMA_CHECK(shader_transparent_chicago_check),
	TAG_SCHEMA_END
};

/* (its two-stage maps are never read: checked as its maps are) */
static struct tag_schema_field const shader_transparent_chicago_extended_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_chicago_extended_definition, shader, shader_schema),
	TAG_SCHEMA_STRUCT(struct shader_transparent_chicago_extended_definition, transparent, shader_transparent_schema),
	TAG_SCHEMA_BLOCK(struct shader_transparent_chicago_extended_definition, transparent.extra_layers,
		shader_layer_schema, MAXIMUM_SHADER_TRANSPARENT_LAYERS),
	TAG_SCHEMA_BLOCK(struct shader_transparent_chicago_extended_definition, transparent.maps,
		shader_transparent_chicago_map_schema, NUMBER_OF_SHADER_TRANSPARENT_MAPS),
	TAG_SCHEMA_BLOCK(struct shader_transparent_chicago_extended_definition, two_stage_maps,
		shader_transparent_chicago_map_schema, MAXIMUM_SHADER_TRANSPARENT_CHICAGO_EXTENDED_MAPS),
	TAG_SCHEMA_CHECK(shader_transparent_chicago_extended_check),
	TAG_SCHEMA_END
};

/* environment */

static struct tag_schema_field const shader_environment_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_environment_definition, shader, shader_schema),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, type, NUMBER_OF_SHADER_ENVIRONMENT_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, lens_flare, TAG_SCHEMA_GROUPS('lens')),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, base_map, TAG_SCHEMA_GROUPS('bitm')),
	/* (an index into set_environment_shader_pixel_shader's tables of 3, on a
	model, rasterizer_xbox_models.c) */
	TAG_SCHEMA_ENUM(struct shader_environment_definition, detail_map_function, NUMBER_OF_SHADER_DETAIL_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, primary_detail_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, secondary_detail_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, micro_detail_map_function,
		NUMBER_OF_SHADER_DETAIL_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, micro_detail_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, bump_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, u_animation_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, v_animation_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, primary_animation_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, secondary_animation_function,
		NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_environment_definition, plasma_animation_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, self_illumination_map, TAG_SCHEMA_GROUPS('bitm')),
	/* (the reflection vertex shader's permutation, of which there are 3) */
	TAG_SCHEMA_ENUM(struct shader_environment_definition, reflection_type,
		NUMBER_OF_SHADER_ENVIRONMENT_REFLECTION_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct shader_environment_definition, reflection_cube_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(shader_environment_check),
	TAG_SCHEMA_END
};

/* model (its color sources are checked as they are read) */

static struct tag_schema_field const shader_model_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_model_definition, shader, shader_schema),
	TAG_SCHEMA_ENUM(struct shader_model_definition, self_illumination_animation_function,
		NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct shader_model_definition, base_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_model_definition, multipurpose_map, TAG_SCHEMA_GROUPS('bitm')),
	/* (indices into set_environment_shader_pixel_shader's tables of 3 and 9) */
	TAG_SCHEMA_ENUM(struct shader_model_definition, detail_function, NUMBER_OF_SHADER_DETAIL_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_model_definition, detail_mask, NUMBER_OF_SHADER_MODEL_DETAIL_MASKS, 0),
	TAG_SCHEMA_REFERENCE(struct shader_model_definition, detail_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_STRUCT(struct shader_model_definition, texture_animation, shader_texture_animation_schema),
	TAG_SCHEMA_REFERENCE(struct shader_model_definition, reflection_cube_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(shader_model_check),
	TAG_SCHEMA_END
};

/* the effect shader inside particles, contrails and weather particles (its
secondary map's radius is the game's, for particles, set as they are drawn,
and the map's otherwise) */

static struct tag_schema_field const shader_effect_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_effect_definition, shader, shader_schema),
	TAG_SCHEMA_ENUM(struct shader_effect_definition, framebuffer_blend_function,
		NUMBER_OF_FRAMEBUFFER_BLEND_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct shader_effect_definition, framebuffer_fade_mode, NUMBER_OF_FRAMEBUFFER_FADE_MODES, 0),
	TAG_SCHEMA_REFERENCE(struct shader_effect_definition, secondary_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct shader_effect_definition, secondary_map_anchor,
		NUMBER_OF_SHADER_EFFECT_SECONDARY_MAP_ANCHORS, 0),
	TAG_SCHEMA_STRUCT(struct shader_effect_definition, secondary_map_animation, shader_texture_animation_schema),
	TAG_SCHEMA_CHECK(shader_effect_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const shader_effect_schema =
	TAG_SCHEMA_DEFINITION(shader_effect, struct shader_effect_definition, shader_effect_fields);

/* particles (a sequence index is taken to the bitmap's, particle_new) */

static struct tag_schema_field const particle_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct particle_definition, bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct particle_definition, physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_REFERENCE(struct particle_definition, collision_material_effects, TAG_SCHEMA_GROUPS('foot')),
	TAG_SCHEMA_REFERENCE(struct particle_definition, collision_effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_REFERENCE(struct particle_definition, effect, TAG_SCHEMA_GROUPS('effe', 'snd!')),
	TAG_SCHEMA_ENUM(struct particle_definition, sprite_orientation, NUMBER_OF_BUILD_SPRITE_ORIENTATIONS, 0),
	TAG_SCHEMA_STRUCT(struct particle_definition, shader, shader_effect_schema),
	TAG_SCHEMA_CHECK(particle_check),
	TAG_SCHEMA_END
};

/* particle systems */

static struct tag_schema_field const particle_system_physics_constant_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const particle_system_physics_constant_schema =
	TAG_SCHEMA_DEFINITION(particle_system_physics_constant, struct particle_system_physics_constant,
		particle_system_physics_constant_fields);

/* (creation and update physics: indices into particle_systems.c's tables of
functions) */
static struct tag_schema_field const particle_system_type_state_fields[] =
{
	TAG_SCHEMA_STRING(struct particle_system_type_state, name),
	TAG_SCHEMA_ENUM(struct particle_system_type_state, particle_creation_physics,
		NUMBER_OF_PARTICLE_SYSTEM_TYPE_CREATION_PHYSICS, 0),
	TAG_SCHEMA_ENUM(struct particle_system_type_state, particle_update_physics,
		NUMBER_OF_PARTICLE_SYSTEM_TYPE_UPDATE_PHYSICS, 0),
	TAG_SCHEMA_BLOCK(struct particle_system_type_state, physics_constants, particle_system_physics_constant_schema,
		MAXIMUM_PARTICLE_SYSTEM_PHYSICS_CONSTANTS),
	TAG_SCHEMA_CHECK(particle_system_type_state_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const particle_system_type_state_schema =
	TAG_SCHEMA_DEFINITION(particle_system_type_state, struct particle_system_type_state,
		particle_system_type_state_fields);

static struct tag_schema_field const particle_system_particle_state_fields[] =
{
	TAG_SCHEMA_STRING(struct particle_system_type_particle_state, name),
	TAG_SCHEMA_REFERENCE(struct particle_system_type_particle_state, bitmaps, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct particle_system_type_particle_state, point_physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_STRUCT(struct particle_system_type_particle_state, shader, shader_effect_schema),
	TAG_SCHEMA_CHECK(particle_system_particle_state_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const particle_system_particle_state_schema =
	TAG_SCHEMA_DEFINITION(particle_system_particle_state, struct particle_system_type_particle_state,
		particle_system_particle_state_fields);

/* (a type's physics constants are read by index: none past the block's are
the empty element's) */
static struct tag_schema_field const particle_system_type_fields[] =
{
	TAG_SCHEMA_STRING(struct particle_system_type, name),
	TAG_SCHEMA_ENUM(struct particle_system_type, complex_sprite_render_mode,
		NUMBER_OF_PARTICLE_SYSTEM_TYPE_COMPLEX_SPRITE_RENDER_MODES, 0),
	TAG_SCHEMA_ENUM(struct particle_system_type, sprite_render_mode, NUMBER_OF_BUILD_SPRITE_ORIENTATIONS, 0),
	TAG_SCHEMA_ENUM(struct particle_system_type, initial_particle_creation_physics,
		NUMBER_OF_PARTICLE_SYSTEM_TYPE_CREATION_PHYSICS, 0),
	TAG_SCHEMA_BLOCK(struct particle_system_type, physics_constants, particle_system_physics_constant_schema,
		MAXIMUM_PARTICLE_SYSTEM_PHYSICS_CONSTANTS),
	TAG_SCHEMA_BLOCK(struct particle_system_type, type_states, particle_system_type_state_schema,
		MAXIMUM_PARTICLE_SYSTEM_TYPE_STATES),
	TAG_SCHEMA_BLOCK(struct particle_system_type, particle_states, particle_system_particle_state_schema,
		MAXIMUM_PARTICLE_SYSTEM_PARTICLE_STATES),
	TAG_SCHEMA_CHECK(particle_system_type_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const particle_system_type_schema =
	TAG_SCHEMA_DEFINITION(particle_system_type, struct particle_system_type, particle_system_type_fields);

/* (types: particle_system_datum's types[MAXIMUM_PARTICLE_SYSTEM_TYPES_PER_SYSTEM];
the system's physics constants are never read) */
static struct tag_schema_field const particle_system_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct particle_system_definition, system_update_point_physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_ENUM(struct particle_system_definition, system_update_physics,
		NUMBER_OF_PARTICLE_SYSTEM_UPDATE_PHYSICS, 0),
	TAG_SCHEMA_BLOCK(struct particle_system_definition, types, particle_system_type_schema,
		MAXIMUM_PARTICLE_SYSTEM_TYPES_PER_SYSTEM),
	TAG_SCHEMA_END
};

/* contrails */

static struct tag_schema_field const contrail_point_state_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct contrail_point_state, physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const contrail_point_state_schema =
	TAG_SCHEMA_DEFINITION(contrail_point_state, struct contrail_point_state, contrail_point_state_fields);

/* (states: a point's state index is a signed byte) */
static struct tag_schema_field const contrail_fields[] =
{
	TAG_SCHEMA_ENUM(struct contrail_definition, render_type, NUMBER_OF_CONTRAIL_RENDER_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct contrail_definition, bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_STRUCT(struct contrail_definition, shader, shader_effect_schema),
	TAG_SCHEMA_BLOCK(struct contrail_definition, states, contrail_point_state_schema,
		MAXIMUM_CONTRAIL_POINT_STATES_PER_CONTRAIL),
	TAG_SCHEMA_CHECK(contrail_check),
	TAG_SCHEMA_END
};

/* weather particle systems */

static struct tag_schema_field const weather_particle_type_fields[] =
{
	TAG_SCHEMA_STRING(struct weather_particle_type_definition, name),
	TAG_SCHEMA_REFERENCE(struct weather_particle_type_definition, physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_REFERENCE(struct weather_particle_type_definition, bitmap, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct weather_particle_type_definition, render_mode, NUMBER_OF_BUILD_SPRITE_ORIENTATIONS, 0),
	TAG_SCHEMA_ENUM(struct weather_particle_type_definition, render_direction_source,
		NUMBER_OF_WEATHER_PARTICLE_RENDER_DIRECTION_SOURCES, 0),
	TAG_SCHEMA_STRUCT(struct weather_particle_type_definition, shader, shader_effect_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const weather_particle_type_schema =
	TAG_SCHEMA_DEFINITION(weather_particle_type, struct weather_particle_type_definition, weather_particle_type_fields);

/* (types: weather_particle_system's types[MAXIMUM_NUMBER_OF_WEATHER_PARTICLE_TYPES]) */
static struct tag_schema_field const weather_particle_system_fields[] =
{
	TAG_SCHEMA_BLOCK(struct weather_particle_system_definition, particle_types, weather_particle_type_schema,
		MAXIMUM_NUMBER_OF_WEATHER_PARTICLE_TYPES),
	TAG_SCHEMA_END
};

/* decals (type: decal_wrap_parameters[NUMBER_OF_DECAL_TYPES]; layer:
decal_globals' first_decal_indices[NUMBER_OF_DECAL_LAYERS]; the shader's
type is not read) */

static struct tag_schema_field const decal_fields[] =
{
	TAG_SCHEMA_ENUM(struct decal_definition, type, NUMBER_OF_DECAL_TYPES, 0),
	TAG_SCHEMA_ENUM(struct decal_definition, layer, NUMBER_OF_DECAL_LAYERS, 0),
	TAG_SCHEMA_REFERENCE(struct decal_definition, next_decal_in_chain, TAG_SCHEMA_GROUPS('deca')),
	TAG_SCHEMA_ENUM(struct decal_definition, shader.framebuffer_blend_function, NUMBER_OF_FRAMEBUFFER_BLEND_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct decal_definition, shader.map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(decal_check),
	TAG_SCHEMA_END
};

/* detail objects (a type's sequence and sprites are indices into the
collection's bitmap's, looked up as they are used) */

static struct tag_schema_field const detail_object_type_fields[] =
{
	TAG_SCHEMA_STRING(struct detail_object_type_definition, name),
	TAG_SCHEMA_CHECK(detail_object_type_check),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const detail_object_type_schema =
	TAG_SCHEMA_DEFINITION(detail_object_type, struct detail_object_type_definition, detail_object_type_fields);

static struct tag_schema_field const detail_object_collection_fields[] =
{
	TAG_SCHEMA_ENUM(struct detail_object_collection_definition, collection_type,
		NUMBER_OF_DETAIL_OBJECT_COLLECTION_TYPES, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_REFERENCE(struct detail_object_collection_definition, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct detail_object_collection_definition, type_definitions, detail_object_type_schema,
		MAXIMUM_DETAIL_OBJECT_TYPES_PER_COLLECTION),
	TAG_SCHEMA_END
};

/* lens flares */

/* (a reflection's bitmap is the primary map's, modulo their count: a
negative one is no bitmap, which rasterizer_set_texture_non_blocking reads) */
static struct tag_schema_field const lens_flare_reflection_fields[] =
{
	TAG_SCHEMA_ENUM(struct lens_flare_reflection, bitmap_index, SHORT_MAX + 1, 0),
	TAG_SCHEMA_ENUM(struct lens_flare_reflection, brightness_scale_function,
		NUMBER_OF_LENS_FLARE_REFLECTION_SCALE_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct lens_flare_reflection, animation_function, NUMBER_OF_PERIODIC_FUNCTIONS,
		FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const lens_flare_reflection_schema =
	TAG_SCHEMA_DEFINITION(lens_flare_reflection, struct lens_flare_reflection, lens_flare_reflection_fields);

static struct tag_schema_field const lens_flare_fields[] =
{
	TAG_SCHEMA_ENUM(struct lens_flare_definition, occlusion_offset_direction,
		NUMBER_OF_LENS_FLARE_OCCLUSION_OFFSET_DIRECTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct lens_flare_definition, primary_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct lens_flare_definition, corona_rotation_function,
		NUMBER_OF_LENS_FLARE_CORONA_ROTATION_FUNCTIONS, 0),
	TAG_SCHEMA_BLOCK(struct lens_flare_definition, reflections, lens_flare_reflection_schema,
		MAXIMUM_LENS_FLARE_REFLECTIONS),
	TAG_SCHEMA_END
};

/* lights (the gel's functions are evaluated for each light the environment
is drawn with) */

static struct tag_schema_field const light_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct light_definition, gel_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct light_definition, gel_secondary_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct light_definition, yaw_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct light_definition, roll_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_ENUM(struct light_definition, pitch_function, NUMBER_OF_PERIODIC_FUNCTIONS, 0),
	TAG_SCHEMA_REFERENCE(struct light_definition, lens_flare, TAG_SCHEMA_GROUPS('lens')),
	TAG_SCHEMA_ENUM(struct light_definition, falloff_function, NUMBER_OF_TRANSITION_FUNCTIONS, 0),
	TAG_SCHEMA_END
};

/* fog */

static struct tag_schema_field const fog_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct fog_definition_full, screen.map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct fog_definition_full, background_sound, TAG_SCHEMA_GROUPS('lsnd')),
	TAG_SCHEMA_REFERENCE(struct fog_definition_full, sound_environment, TAG_SCHEMA_GROUPS('snde')),
	TAG_SCHEMA_CHECK(fog_check),
	TAG_SCHEMA_END
};

/* antennas (a vertex's sequence is checked against the texture's as it is
drawn; vertices: the antenna's vertices[MAXIMUM_ANTENNA_VERTICES + 1], the
last its end) */

static struct tag_schema_field const antenna_vertex_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const antenna_vertex_schema =
	TAG_SCHEMA_DEFINITION(antenna_vertex, struct antenna_vertex_definition, antenna_vertex_fields);

static struct tag_schema_field const antenna_fields[] =
{
	TAG_SCHEMA_STRING(struct antenna_definition, attachment_marker),
	TAG_SCHEMA_REFERENCE(struct antenna_definition, texture, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct antenna_definition, physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_BLOCK(struct antenna_definition, vertices, antenna_vertex_schema, MAXIMUM_ANTENNA_VERTICES),
	TAG_SCHEMA_END
};

/* glows (attachment indices: NONE or an object's outgoing function,
object_get_function_value; the particle rotation's and size's are not read) */

static struct tag_schema_field const glow_fields[] =
{
	TAG_SCHEMA_STRING(struct glow_definition, attachment_marker),
	TAG_SCHEMA_ENUM(struct glow_definition, boundary_effect, NUMBER_OF_GLOW_BOUNDARY_EFFECTS, 0),
	TAG_SCHEMA_ENUM(struct glow_definition, particle_distribution, NUMBER_OF_GLOW_PARTICLE_DISTRIBUTIONS, 0),
	TAG_SCHEMA_ENUM(struct glow_definition, trailing_particle_distribution,
		NUMBER_OF_GLOW_TRAILING_PARTICLE_DISTRIBUTIONS, 0),
	TAG_SCHEMA_ENUM(struct glow_definition, effect_rotational_velocity_attachment_index,
		NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct glow_definition, effect_translational_velocity_attachment_index,
		NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct glow_definition, distance_to_object_attachment_index,
		NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_ENUM(struct glow_definition, color_attachment_index,
		NUMBER_OF_OUTGOING_OBJECT_FUNCTIONS, FLAG(_tag_schema_none_bit)),
	TAG_SCHEMA_REFERENCE(struct glow_definition, texture, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(glow_check),
	TAG_SCHEMA_END
};

/* flags (their edge shapes are only compared) */

static struct tag_schema_field const flag_attachment_point_fields[] =
{
	TAG_SCHEMA_STRING(struct flag_attachment_point, marker_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const flag_attachment_point_schema =
	TAG_SCHEMA_DEFINITION(flag_attachment_point, struct flag_attachment_point, flag_attachment_point_fields);

static struct tag_schema_field const flag_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct flag_definition, shader_red, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_REFERENCE(struct flag_definition, physics, TAG_SCHEMA_GROUPS('pphy')),
	TAG_SCHEMA_REFERENCE(struct flag_definition, shader_blue, TAG_SCHEMA_GROUPS('shdr')),
	TAG_SCHEMA_BLOCK(struct flag_definition, attachment_points, flag_attachment_point_schema,
		MAXIMUM_FLAG_ATTACHMENT_POINTS),
	TAG_SCHEMA_CHECK(flag_check),
	TAG_SCHEMA_END
};

/* lightning (its sources are checked as they are read; only its first shader
is drawn) */

static struct tag_schema_field const lightning_marker_fields[] =
{
	TAG_SCHEMA_STRING(struct lightning_marker_definition, attachment_marker),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const lightning_marker_schema =
	TAG_SCHEMA_DEFINITION(lightning_marker, struct lightning_marker_definition, lightning_marker_fields);

static struct tag_schema_field const lightning_shader_fields[] =
{
	TAG_SCHEMA_STRUCT(struct lightning_shader, shader, shader_effect_schema),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const lightning_shader_schema =
	TAG_SCHEMA_DEFINITION(lightning_shader, struct lightning_shader, lightning_shader_fields);

static struct tag_schema_field const lightning_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct lightning_definition, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_BLOCK(struct lightning_definition, markers, lightning_marker_schema, MAXIMUM_LIGHTNING_MARKERS),
	TAG_SCHEMA_BLOCK(struct lightning_definition, shaders, lightning_shader_schema, MAXIMUM_LIGHTNING_SHADERS),
	TAG_SCHEMA_CHECK(lightning_check),
	TAG_SCHEMA_END
};

/* glass (its specular maps are never read) */

static struct tag_schema_field const shader_transparent_glass_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_glass_definition, shader, shader_schema),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_glass_definition, tint_map, TAG_SCHEMA_GROUPS('bitm')),
	/* (the reflection vertex shader's permutation: rasterizer_xbox_vertex_shaders_runtime.c's
	translation table has 3 for it) */
	TAG_SCHEMA_ENUM(struct shader_transparent_glass_definition, reflection_type,
		NUMBER_OF_SHADER_TRANSPARENT_GLASS_REFLECTION_TYPES, 0),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_glass_definition, reflection_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_glass_definition, reflection_bump_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_glass_definition, diffuse_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_glass_definition, diffuse_detail_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(shader_transparent_glass_check),
	TAG_SCHEMA_END
};

/* meter (its sources are checked as they are read) */

static struct tag_schema_field const shader_transparent_meter_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_meter_definition, shader, shader_schema),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_meter_definition, map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(shader_transparent_meter_check),
	TAG_SCHEMA_END
};

/* water */

/* (a ripple's map index is a sequence index into the ripple maps, which a
negative one is not: rasterizer_set_texture takes it modulo their count and
finds no bitmap) */
static struct tag_schema_field const water_ripple_fields[] =
{
	TAG_SCHEMA_ENUM(struct water_ripple, map_index, SHORT_MAX + 1, 0),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const water_ripple_schema =
	TAG_SCHEMA_DEFINITION(water_ripple, struct water_ripple, water_ripple_fields);

/* (ripples: copied into rasterizer_water_update's ripples[NUMBER_OF_WATER_RIPPLES];
the mipmap levels are taken to at most NUMBER_OF_WATER_RIPPLES, and a negative
count is shifted into the water texture's format) */
static struct tag_schema_field const shader_transparent_water_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_water_definition, shader, shader_schema),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_water_definition, base_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_water_definition, reflection_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_water_definition, ripple_maps, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_ENUM(struct shader_transparent_water_definition, ripple_mipmap_levels, SHORT_MAX + 1, 0),
	TAG_SCHEMA_BLOCK(struct shader_transparent_water_definition, ripples, water_ripple_schema,
		NUMBER_OF_WATER_RIPPLES),
	TAG_SCHEMA_CHECK(shader_transparent_water_check),
	TAG_SCHEMA_END
};

/* plasma (its sources are checked as they are read) */

static struct tag_schema_field const shader_transparent_plasma_fields[] =
{
	TAG_SCHEMA_STRUCT(struct shader_transparent_plasma_definition, shader, shader_schema),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_plasma_definition, primary_noise_map, TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_REFERENCE(struct shader_transparent_plasma_definition, secondary_noise_map,
		TAG_SCHEMA_GROUPS('bitm')),
	TAG_SCHEMA_CHECK(shader_transparent_plasma_check),
	TAG_SCHEMA_END
};

/* color tables */

static struct tag_schema_field const color_table_color_fields[] =
{
	TAG_SCHEMA_STRING(struct color_table_color, name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const color_table_color_schema =
	TAG_SCHEMA_DEFINITION(color_table_color, struct color_table_color, color_table_color_fields);

/* (an interface color is any of them, its index modulo their count:
interface_get_real_argb_color) */
static struct tag_schema_field const color_table_fields[] =
{
	TAG_SCHEMA_BLOCK(struct color_table_definition, colors, color_table_color_schema, MAXIMUM_COLOR_TABLE_COLORS),
	TAG_SCHEMA_END
};

/* skies */

static struct tag_schema_field const sky_shader_function_fields[] =
{
	TAG_SCHEMA_STRING(struct sky_shader_function, global_function_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sky_shader_function_schema =
	TAG_SCHEMA_DEFINITION(sky_shader_function, struct sky_shader_function, sky_shader_function_fields);

/* (an animation's index is checked against the animation graph's as it
plays, render_sky, which then plays the graph's animation of the sky
animation's own index, the empty one past them) */
static struct tag_schema_field const sky_animation_fields[] =
{
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sky_animation_schema =
	TAG_SCHEMA_DEFINITION(sky_animation, struct sky_animation, sky_animation_fields);

static struct tag_schema_field const sky_light_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct sky_light, lens_flare, TAG_SCHEMA_GROUPS('lens')),
	TAG_SCHEMA_STRING(struct sky_light, marker_name),
	TAG_SCHEMA_END
};

static struct tag_schema_definition const sky_light_schema =
	TAG_SCHEMA_DEFINITION(sky_light, struct sky_light, sky_light_fields);

/* (render_sky: a region scale for each shader function, region_scales[8];
a phase for each animation, render_sky_globals[MAXIMUM_SKIES_PER_SCENARIO]) */
static struct tag_schema_field const sky_fields[] =
{
	TAG_SCHEMA_REFERENCE(struct sky_definition, model, TAG_SCHEMA_GROUPS('mode')),
	TAG_SCHEMA_REFERENCE(struct sky_definition, animation_graph, TAG_SCHEMA_GROUPS('antr')),
	TAG_SCHEMA_REFERENCE(struct sky_definition, indoor_fog_screen, TAG_SCHEMA_GROUPS('fog ')),
	TAG_SCHEMA_BLOCK(struct sky_definition, shader_functions, sky_shader_function_schema,
		MAXIMUM_SKY_SHADER_FUNCTIONS),
	TAG_SCHEMA_BLOCK(struct sky_definition, animations, sky_animation_schema, MAXIMUM_SKY_ANIMATIONS),
	TAG_SCHEMA_BLOCK(struct sky_definition, lights, sky_light_schema, MAXIMUM_SKY_LIGHTS),
	TAG_SCHEMA_END
};

/* wind (wind.c reads its numbers only) */

static struct tag_schema_field const wind_fields[] =
{
	TAG_SCHEMA_END
};

/* the groups' roots */

static struct tag_schema_definition const shader_environment_schema =
	TAG_SCHEMA_DEFINITION(shader_environment, struct shader_environment_definition, shader_environment_fields);
static struct tag_schema_definition const shader_model_schema =
	TAG_SCHEMA_DEFINITION(shader_model, struct shader_model_definition, shader_model_fields);
static struct tag_schema_definition const particle_schema =
	TAG_SCHEMA_DEFINITION(particle, struct particle_definition, particle_fields);
static struct tag_schema_definition const particle_system_schema =
	TAG_SCHEMA_DEFINITION(particle_system, struct particle_system_definition, particle_system_fields);
static struct tag_schema_definition const contrail_schema =
	TAG_SCHEMA_DEFINITION(contrail, struct contrail_definition, contrail_fields);
static struct tag_schema_definition const weather_particle_system_schema =
	TAG_SCHEMA_DEFINITION(weather_particle_system, struct weather_particle_system_definition,
		weather_particle_system_fields);
static struct tag_schema_definition const decal_schema =
	TAG_SCHEMA_DEFINITION(decal, struct decal_definition, decal_fields);
static struct tag_schema_definition const detail_object_collection_schema =
	TAG_SCHEMA_DEFINITION(detail_object_collection, struct detail_object_collection_definition,
		detail_object_collection_fields);
static struct tag_schema_definition const lens_flare_schema =
	TAG_SCHEMA_DEFINITION(lens_flare, struct lens_flare_definition, lens_flare_fields);
static struct tag_schema_definition const light_schema =
	TAG_SCHEMA_DEFINITION(light, struct light_definition, light_fields);
static struct tag_schema_definition const fog_schema =
	TAG_SCHEMA_DEFINITION(fog, struct fog_definition_full, fog_fields);
static struct tag_schema_definition const antenna_schema =
	TAG_SCHEMA_DEFINITION(antenna, struct antenna_definition, antenna_fields);
static struct tag_schema_definition const glow_schema =
	TAG_SCHEMA_DEFINITION(glow, struct glow_definition, glow_fields);
static struct tag_schema_definition const flag_schema =
	TAG_SCHEMA_DEFINITION(flag, struct flag_definition, flag_fields);
static struct tag_schema_definition const lightning_schema =
	TAG_SCHEMA_DEFINITION(lightning, struct lightning_definition, lightning_fields);
static struct tag_schema_definition const shader_transparent_glass_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_glass, struct shader_transparent_glass_definition,
		shader_transparent_glass_fields);
static struct tag_schema_definition const shader_transparent_meter_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_meter, struct shader_transparent_meter_definition,
		shader_transparent_meter_fields);
static struct tag_schema_definition const shader_transparent_water_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_water, struct shader_transparent_water_definition,
		shader_transparent_water_fields);
static struct tag_schema_definition const shader_transparent_plasma_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_plasma, struct shader_transparent_plasma_definition,
		shader_transparent_plasma_fields);
static struct tag_schema_definition const color_table_schema =
	TAG_SCHEMA_DEFINITION(color_table, struct color_table_definition, color_table_fields);
static struct tag_schema_definition const sky_schema =
	TAG_SCHEMA_DEFINITION(sky, struct sky_definition, sky_fields);
static struct tag_schema_definition const wind_schema =
	TAG_SCHEMA_DEFINITION(wind, struct wind_definition, wind_fields);
static struct tag_schema_definition const bitmap_group_schema =
	TAG_SCHEMA_DEFINITION(bitmap_group, struct bitmap_group, bitmap_group_fields);
static struct tag_schema_definition const shader_definition_schema =
	TAG_SCHEMA_DEFINITION(shader_definition, struct shader, shader_definition_fields);
static struct tag_schema_definition const shader_transparent_generic_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_generic, struct shader_transparent_generic_definition,
		shader_transparent_generic_fields);
static struct tag_schema_definition const shader_transparent_chicago_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_chicago, struct shader_transparent_chicago_definition,
		shader_transparent_chicago_fields);
static struct tag_schema_definition const shader_transparent_chicago_extended_schema =
	TAG_SCHEMA_DEFINITION(shader_transparent_chicago_extended, struct shader_transparent_chicago_extended_definition,
		shader_transparent_chicago_extended_fields);

struct tag_schema_group const tag_schema_render_groups[] =
{
	{ 'bitm', { NONE, NONE }, &bitmap_group_schema },
	{ 'shdr', { NONE, NONE }, &shader_definition_schema },
	{ 'schi', { 'shdr', NONE }, &shader_transparent_chicago_schema },
	{ 'scex', { 'shdr', NONE }, &shader_transparent_chicago_extended_schema },
	{ 'senv', { 'shdr', NONE }, &shader_environment_schema },
	{ 'sgla', { 'shdr', NONE }, &shader_transparent_glass_schema },
	{ 'smet', { 'shdr', NONE }, &shader_transparent_meter_schema },
	{ 'soso', { 'shdr', NONE }, &shader_model_schema },
	{ 'sotr', { 'shdr', NONE }, &shader_transparent_generic_schema },
	{ 'spla', { 'shdr', NONE }, &shader_transparent_plasma_schema },
	{ 'swat', { 'shdr', NONE }, &shader_transparent_water_schema },
	{ 'sky ', { NONE, NONE }, &sky_schema },
	{ 'fog ', { NONE, NONE }, &fog_schema },
	{ 'lens', { NONE, NONE }, &lens_flare_schema },
	{ 'ligh', { NONE, NONE }, &light_schema },
	{ 'glw!', { NONE, NONE }, &glow_schema },
	{ 'cont', { NONE, NONE }, &contrail_schema },
	{ 'part', { NONE, NONE }, &particle_schema },
	{ 'pctl', { NONE, NONE }, &particle_system_schema },
	{ 'deca', { NONE, NONE }, &decal_schema },
	{ 'ant!', { NONE, NONE }, &antenna_schema },
	{ 'flag', { NONE, NONE }, &flag_schema },
	{ 'elec', { NONE, NONE }, &lightning_schema },
	{ 'rain', { NONE, NONE }, &weather_particle_system_schema },
	{ 'wind', { NONE, NONE }, &wind_schema },
	{ 'dobc', { NONE, NONE }, &detail_object_collection_schema },
	{ 'colo', { NONE, NONE }, &color_table_schema },
	{ 0 }
};
