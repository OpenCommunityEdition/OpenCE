/*
SHADER_PRECOMPILE.C

Every GPU program a map can draw with, made before it is drawn with
(display.shader_precompile).

The Xbox's GPU took its pixel shaders as register states; the port turns
each combination the game sets into GLSL and links it with the vertex shader
in the frame that first draws with it, which stops that frame on some
drivers for tens or hundreds of milliseconds. So once a map has loaded, when
its first view is drawn, every renderer of the game is run over every tag
of the map that it can draw (every model with each of its permutations and
levels of detail, lit as the game lights models, first person, camouflaged,
fogged; the skies; one surface of each of the structure's materials through
each of its passes, under each light, shadow and fog screen; the decals,
effects, widgets, HUD meters and screen effects) with the GL layer's draws
turned off (d3d_gl_shader_precompile): each draw still makes its program,
and draws nothing. The list of programs comes from the map's tags, not from
what one game happened to draw. The draws get the textures' headers without
their pixels (texture_cache_set_no_pixels), as a program depends only on the
kind of texture each stage samples; the HUD meters get theirs with them (a
high-res meter's texture brings its coverage into the program). With
display.shader_cache on, each program is
kept and loaded from there the next time.

The draws are the game's own, so they set the states the game sets: a pass
that reads a state an earlier pass left (the decals' shader keeps part of
the blend function before it; the fog screen's surfaces draw with the
texture modes of the models before them) is run after each of those.
*/

#include "cseries.h"
#include "bitmaps/bitmap_group.h"
#include "bitmaps/bitmap_group_lookup.h"
#include "cache/cache_files.h"
#include "cache/texture_cache.h"
#include "effects/contrail_definitions.h"
#include "effects/particle_system_definitions.h"
#include "effects/particles.h"
#include "effects/weather_particle_definitions.h"
#include "game/game_globals.h"
#include "interface/hud_draw.h"
#include "interface/interface.h"
#include "interface/unit_hud_interface_definition.h"
#include "interface/weapon_hud_interface_definition.h"
#include "math/real_math.h"
#include "models/model_definitions.h"
#include "models/models.h"
#include "objects/light_definitions.h"
#include "objects/widgets/antenna.h"
#include "rasterizer/common/rasterizer_common.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/rasterizer_geometry.h"
#include "rasterizer/rasterizer_lights.h"
#include "rasterizer/rasterizer_model_types.h"
#include "rasterizer/rasterizer_transparent_geometry.h"
#include "render/render.h"
#include "scenario/sky_definitions.h"
#include "shaders/shader_definitions.h"
#include "shaders/shaders.h"
#include "tag_files/tag_groups.h"

#include "shader_precompile.h"

#include <string.h>

/* port/linux/src/port_config.c */
int config_boolean(const char *name);
/* port/linux/src/d3d8_gl.c: while on, draws make their programs and draw
nothing; FALSE when there is nothing to make them with */
int d3d_gl_shader_precompile(int on);
void platform_log(const char *format, ...);
unsigned long system_milliseconds(void);

/* the rasterizer's own (rasterizer_xbox_*.c) */
void _rasterizer_screen_flash(void);
void _rasterizer_hud_motion_sensor_blip_begin(void);
void _rasterizer_hud_motion_sensor_blip_draw(real_point2d const *center, real scale, real size,
	real_rgb_color const *color, boolean large);
void _rasterizer_hud_motion_sensor_blip_end(real_point2d const *center, real scale);
void rasterizer_environment_fog_screen_window_begin(void);
real rasterizer_environment_fog_screen_far_distance(void);
void _rasterizer_models_begin(boolean sky);
/* structure_render.c */
void structure_render_fog_screen(void);
void _rasterizer_psuedo_dynamic_screen_quad_draw(struct rasterizer_dynamic_screen_geometry_parameters const *parameters,
	struct dynamic_screen_vertex *vertices);

/* ---------- constants */

/* (as the files that use them declare them) */
enum
{
	/* models.c */
	_render_model_shadow_bit = 1,
	_render_model_first_person_bit = 3,
	/* models.c, render_sprite.c */
	_rasterizer_geometry_viewspace_bit = 5,
	_rasterizer_geometry_first_person_bit = 7,
	/* player_effects.c */
	_render_screen_flash_type_lighten = 1,
	_render_screen_flash_type_tint = 6,
	/* tag_schema_effects.c */
	NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS = 8,
	/* hud_weapon.c */
	WEAPON_HUD_INTERFACE_TAG = 'wphi',
	/* rasterizer_transparent_geometry.c */
	_shader_type_transparent_water = 7
};

#define SHADER_PRECOMPILE_DETAIL_LEVELS 5
/* the models' levels of detail are chosen by their size on the screen: a
size within each level's band (the last: the highest) */
#define SHADER_PRECOMPILE_LARGEST_PIXELS 100000.0f

/* ---------- structures */

/* (the parts used here of the tags as the files that read them declare
them) */

/* models.c */
struct shader_precompile_model_shader
{
	struct tag_reference shader;
	short permutation_index;
	word pad;
	long unused[3];
};

typedef char shader_precompile_model_shader_size_assert[
	sizeof(struct shader_precompile_model_shader) == 0x20 ? 1 : -1];

/* render_sky.c */
struct shader_precompile_sky
{
	struct tag_reference model;
};

/* hud_weapon.c */
struct shader_precompile_weapon_hud_meter
{
	byte header[0x24];
	struct meter_hud_element_definition meter;
	long unused[10];
};

/* weather_particle_systems.c */
struct shader_precompile_weather_particle_type
{
	byte unused[0x194];
	struct tag_reference bitmap;
	short render_mode;
	short render_direction_source;
	struct shader_effect_definition shader;
};

typedef char shader_precompile_weather_particle_type_size_assert[
	sizeof(struct shader_precompile_weather_particle_type) == 0x25C ? 1 : -1];

/* ---------- globals */

static boolean shader_precompile_pending = FALSE;
/* the rasterizer memory pool's use when the precompile began: given back to
it after each batch of draws, as the pool holds a frame's draws only */
static unsigned long shader_precompile_pool_used;

static real_matrix4x3 shader_precompile_node_matrices[MAXIMUM_NODES_PER_MODEL];
static real_rgb_color shader_precompile_change_colors[MAXIMUM_CHANGE_COLORS_PER_MODEL];
static real shader_precompile_function_values[MAXIMUM_FUNCTION_VALUES_PER_MODEL];
static char shader_precompile_permutations[MAXIMUM_REGIONS_PER_MODEL];

/* ---------- private code */

static void shader_precompile_transparent_flush(void)
{
	rasterizer_transparent_geometry_draw(TRUE);
	rasterizer_transparent_geometry_draw(FALSE);
	rasterizer_transparent_geometry_stop();
	rasterizer_transparent_geometry_begin();
	/* (and the model parts the draws gave the fog screen, whose matrices
	were in the pool: it keeps 128) */
	rasterizer_environment_fog_screen_window_begin();
	rasterizer_memory_pool_release(shader_precompile_pool_used);
}

static void shader_precompile_model_state_initialize(void)
{
	short i;

	for (i = 0; i < MAXIMUM_NODES_PER_MODEL; i++)
		matrix4x3_identity(&shader_precompile_node_matrices[i]);
	for (i = 0; i < MAXIMUM_CHANGE_COLORS_PER_MODEL; i++)
	{
		shader_precompile_change_colors[i].red = 1.0f;
		shader_precompile_change_colors[i].green = 1.0f;
		shader_precompile_change_colors[i].blue = 1.0f;
	}
	for (i = 0; i < MAXIMUM_FUNCTION_VALUES_PER_MODEL; i++)
		shader_precompile_function_values[i] = 1.0f;
}

/* the most permutations any region of the model has */
static short shader_precompile_model_permutation_count(struct model *model)
{
	short permutation_count = 1;
	short i;

	for (i = 0; i < model->regions.count && i < MAXIMUM_REGIONS_PER_MODEL; i++)
	{
		struct model_region *region = TAG_BLOCK_GET_ELEMENT(&model->regions, i, struct model_region);

		if (region->permutations.count > permutation_count)
			permutation_count = (short)region->permutations.count;
	}
	return permutation_count;
}

/* each region at this permutation (its last, if it has fewer) */
static void shader_precompile_model_permutation_set(struct model *model, short permutation)
{
	short i;

	for (i = 0; i < model->regions.count && i < MAXIMUM_REGIONS_PER_MODEL; i++)
	{
		struct model_region *region = TAG_BLOCK_GET_ELEMENT(&model->regions, i, struct model_region);

		shader_precompile_permutations[i] = (char)(permutation < region->permutations.count ?
			permutation : region->permutations.count - 1);
	}
}

/* the size on the screen (render_model's pixels) at which the model draws
at this level of detail */
static real shader_precompile_model_pixels(struct model *model, short detail)
{
	real pixels = detail == SHADER_PRECOMPILE_DETAIL_LEVELS - 1 ?
		SHADER_PRECOMPILE_LARGEST_PIXELS : model->detail_cutoff_pixels[detail] + 0.001f;

	return pixels / halo_model_detail_scale(FALSE);
}

/* whether the model has a shader of the water pass (water, or drawn before
it): such a model is never held in first person, and the water pass takes
no first-person draw */
static boolean shader_precompile_model_has_water(struct model *model)
{
	long i;

	for (i = 0; i < model->shaders.count; i++)
	{
		struct shader_precompile_model_shader *reference =
			TAG_BLOCK_GET_ELEMENT(&model->shaders, i, struct shader_precompile_model_shader);
		struct shader *shader;

		if (reference->shader.index == NONE)
			continue;
		shader = shader_definition_get(reference->shader.index);
		if (shader && (shader->base.type == _shader_type_transparent_water || shader_is_water_decal(shader)))
			return TRUE;
	}
	return FALSE;
}

/* every model in each permutation and level of detail, with 0-2 distant
lights or 1-2 point lights (those from light_index), first person or not,
camouflaged or not */
static void shader_precompile_models(long light_index, short point_light_count)
{
	struct render_lighting lighting;
	struct render_model_effect effect;
	struct tag_iterator iterator;
	real_point3d centroid = global_window_parameters.camera.position;
	real fog_screen_distance = rasterizer_environment_fog_screen_far_distance();
	long model_index;
	short i;

	/* (past the window's fog screen, which takes the models nearer: those
	are drawn each at once, and the far ones with display.batch_models
	batched; shader_precompile_fog_screen_models draws the near ones) */
	if (fog_screen_distance > 0.0f)
	{
		point_from_line3d(&global_window_parameters.camera.position, &global_window_parameters.camera.forward,
			fog_screen_distance + 1.0f, &centroid);
	}
	tag_iterator_new(&iterator, MODELS_GROUP_TAG);
	while ((model_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct model *model = model_definition_get(model_index);
		short permutation_count = shader_precompile_model_permutation_count(model);
		boolean water = shader_precompile_model_has_water(model);
		short permutation, detail, lights, variant;

		for (permutation = 0; permutation < permutation_count; permutation++)
		{
			shader_precompile_model_permutation_set(model, permutation);
			for (detail = 0; detail < SHADER_PRECOMPILE_DETAIL_LEVELS; detail++)
			{
				real pixels = shader_precompile_model_pixels(model, detail);

				for (lights = 0; lights <= 2 + point_light_count; lights++)
				{
					for (variant = 0; variant < 3; variant++)
					{
						if (variant == 1 && water)
							continue;
						memset(&lighting, 0, sizeof(lighting));
						lighting.ambient_color.red = 0.3f;
						lighting.ambient_color.green = 0.3f;
						lighting.ambient_color.blue = 0.3f;
						lighting.distant_light_count = lights <= 2 ? lights : 0;
						lighting.point_light_count = lights <= 2 ? 0 : lights - 2;
						for (i = 0; i < lighting.point_light_count; i++)
							lighting.point_light_indices[i] = light_index + i;
						for (i = 0; i < lighting.distant_light_count; i++)
						{
							lighting.distant_lights[i].color.red = 1.0f;
							lighting.distant_lights[i].direction.k = -1.0f;
						}
						lighting.reflection_tint_color.alpha = 1.0f;
						lighting.reflection_tint_color.red = 1.0f;
						lighting.reflection_tint_color.green = 1.0f;
						lighting.reflection_tint_color.blue = 1.0f;
						lighting.shadow_vector.k = -1.0f;
						memset(&effect, 0, sizeof(effect));
						effect.source_object_index = NONE;
						if (variant == 2)
						{
							/* (active camouflage) */
							effect.type = 1;
							effect.intensity = 1.0f;
						}
						/* (as objects are: not as the sky, and with
						display.batch_models, batched) */
						rasterizer_models_begin(FALSE);
						render_model(model_index, pixels, shader_precompile_node_matrices, shader_precompile_permutations,
							shader_precompile_change_colors, shader_precompile_function_values, &lighting, &centroid, 1.0f,
							&effect, NONE, 0, variant == 1 ? FLAG(_render_model_first_person_bit) : 0);
						rasterizer_models_end();
						shader_precompile_transparent_flush();
					}
				}
			}
		}
	}
}

/* each sky's model, as render_sky draws it */
static void shader_precompile_skies(void)
{
	struct render_lighting lighting;
	struct tag_iterator iterator;
	long sky_index;

	tag_iterator_new(&iterator, SKY_DEFINITION_TAG);
	while ((sky_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct shader_precompile_sky *sky = tag_get(SKY_DEFINITION_TAG, sky_index);

		if (!sky || sky->model.index == NONE)
			continue;
		rasterizer_models_begin(TRUE);
		memset(&lighting, 0, sizeof(lighting));
		lighting.ambient_color = *global_real_rgb_white;
		render_model(sky->model.index, 0.0f, shader_precompile_node_matrices, NULL, NULL, NULL, &lighting,
			&render.camera.position, 0.0f, NULL, 0, 0, TRUE);
		rasterizer_models_end();
	}
	shader_precompile_transparent_flush();
}

/* the screen flash of each type */
static void shader_precompile_screen_flashes(void)
{
	struct render_screen_flash saved = global_window_parameters.screen_flash;
	short type;

	for (type = _render_screen_flash_type_lighten; type <= _render_screen_flash_type_tint; type++)
	{
		global_window_parameters.screen_flash.type = type;
		global_window_parameters.screen_flash.intensity = 1.0f;
		global_window_parameters.screen_flash.color.alpha = 1.0f;
		global_window_parameters.screen_flash.color.red = 0.5f;
		global_window_parameters.screen_flash.color.green = 0.5f;
		global_window_parameters.screen_flash.color.blue = 0.5f;
		_rasterizer_screen_flash();
	}
	global_window_parameters.screen_flash = saved;
}

/* the screen quad's shaders over what its callers pass (the HUD, the menus,
text, cinematics): one to three maps with each blend function between them,
alpha weighted or not, the plasma effect (the meters: with their own
textures, shader_precompile_hud_meter) */
static void shader_precompile_screen_quads(void)
{
	struct rasterizer_dynamic_screen_geometry_parameters parameters;
	struct dynamic_screen_vertex vertices[NUMBER_OF_VERTICES_PER_QUADRILATERAL];
	struct bitmap_group *group = bitmap_group_get(global_rasterizer_data->glow.index);
	struct bitmap_data *bitmap;
	short maps, blend01, blend12, plasma, weighted;

	if (!group || group->bitmaps.count < 1)
		return;
	bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data);
	if (!_texture_cache_bitmap_get_hardware_format(bitmap, TRUE, TRUE))
		return;
	memset(vertices, 0, sizeof(vertices));
	for (maps = 1; maps <= 3; maps++)
	{
		for (blend01 = 0; blend01 < (maps > 1 ? NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS : 1); blend01++)
		{
			for (blend12 = 0; blend12 < (maps > 2 ? NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS : 1); blend12++)
			{
				for (plasma = 0; plasma < (maps > 2 ? 2 : 1); plasma++)
				{
					for (weighted = 0; weighted < 2; weighted++)
					{
						memset(&parameters, 0, sizeof(parameters));
						parameters.map[0] = bitmap;
						parameters.map[1] = maps > 1 ? bitmap : NULL;
						parameters.map[2] = maps > 2 ? bitmap : NULL;
						parameters.map0_to_1_blend_function = blend01;
						parameters.map1_to_2_blend_function = blend12;
						parameters.doing_plasma_effect = (boolean)plasma;
						parameters.alpha_weighted = (boolean)weighted;
						_rasterizer_psuedo_dynamic_screen_quad_draw(&parameters, vertices);
					}
				}
			}
		}
	}
}

/* a HUD meter, its bitmap loaded, as the HUD draws it (a high-res meter
texture brings its own coverage into the shader) */
static void shader_precompile_hud_meter(struct hud_absolute_placement_definition const *placement,
	struct meter_hud_element_definition const *meter)
{
	struct bitmap_data *bitmap = bitmap_group_get_bitmap_from_sequence(meter->meter_bitmap.index, meter->sequence_index, 0);

	if (bitmap && _texture_cache_bitmap_get_hardware_format(bitmap, TRUE, TRUE))
		hud_draw_meter(0, placement, meter, UNSIGNED_CHAR_MAX, UNSIGNED_CHAR_MAX, 0, -1.0f, 1.0f);
}

/* each unit HUD's and weapon HUD's meters */
static void shader_precompile_hud_meters(void)
{
	struct tag_iterator iterator;
	long tag_index;

	tag_iterator_new(&iterator, UNIT_HUD_INTERFACE_DEFINITION_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct unit_hud_interface_definition *definition = tag_get(UNIT_HUD_INTERFACE_DEFINITION_TAG, tag_index);
		long meter_index;

		shader_precompile_hud_meter(&definition->absolute_placement, &definition->shield_meter.meter);
		shader_precompile_hud_meter(&definition->absolute_placement, &definition->health_meter.meter);
		for (meter_index = 0; meter_index < definition->auxilary_meters.count; meter_index++)
		{
			struct auxilary_meter_definition *meter = TAG_BLOCK_GET_ELEMENT(
				&definition->auxilary_meters, meter_index, struct auxilary_meter_definition);

			shader_precompile_hud_meter(&definition->absolute_placement, &meter->panel.meter);
		}
	}
	tag_iterator_new(&iterator, WEAPON_HUD_INTERFACE_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct weapon_hud_interface_definition *definition = tag_get(WEAPON_HUD_INTERFACE_TAG, tag_index);
		long element_index;

		for (element_index = 0; element_index < definition->meters.count; element_index++)
		{
			struct shader_precompile_weapon_hud_meter *element = TAG_BLOCK_GET_ELEMENT(
				&definition->meters, element_index, struct shader_precompile_weapon_hud_meter);

			shader_precompile_hud_meter(&definition->absolute_placement, &element->meter);
		}
	}
}

/* the motion sensor's blips (small and large) and its sweep, their bitmaps
loaded */
static void shader_precompile_motion_sensor(void)
{
	short bitmaps[4] = { _interface_bitmap_motion_blip, _interface_bitmap_iface_map1,
		_interface_bitmap_motion_sweep, _interface_bitmap_motion_sweep_mask };
	real_point2d center = { 0.0f, 0.0f };
	real_rgb_color color = { 1.0f, 1.0f, 1.0f };
	short i;

	for (i = 0; i < 4; i++)
	{
		struct bitmap_data *bitmap = bitmap_group_try_and_get_bitmap(interface_get_tag_index(bitmaps[i]), 0);

		if (!bitmap || !_texture_cache_bitmap_get_hardware_format(bitmap, TRUE, TRUE))
			return;
	}
	_rasterizer_hud_motion_sensor_blip_begin();
	_rasterizer_hud_motion_sensor_blip_draw(&center, 1.0f, 1.0f, &color, FALSE);
	_rasterizer_hud_motion_sensor_blip_draw(&center, 1.0f, 1.0f, &color, TRUE);
	_rasterizer_hud_motion_sensor_blip_end(&center, 1.0f);
}

/* each particle's, particle system's, weather's, contrail's and antenna's
shader (lightning and glows: lightning.c, glow.c) */
static void shader_precompile_effects(void)
{
	struct tag_iterator iterator;
	long index;

	tag_iterator_new(&iterator, PARTICLE_TAG);
	while ((index = tag_iterator_next(&iterator)) != NONE)
	{
		struct particle_definition *definition = particle_definition_get(index);

		shader_precompile_effect((struct shader const *)&definition->shader, definition->bitmap.index);
	}
	tag_iterator_new(&iterator, PARTICLE_SYSTEM_DEFINITION_TAG);
	while ((index = tag_iterator_next(&iterator)) != NONE)
	{
		struct particle_system_definition *definition = particle_system_definition_get(index);
		long type_index, state_index;

		for (type_index = 0; type_index < definition->types.count; type_index++)
		{
			struct particle_system_type *type = TAG_BLOCK_GET_ELEMENT(&definition->types, type_index,
				struct particle_system_type);

			for (state_index = 0; state_index < type->particle_states.count; state_index++)
			{
				struct particle_system_type_particle_state *state = TAG_BLOCK_GET_ELEMENT(
					&type->particle_states, state_index, struct particle_system_type_particle_state);

				shader_precompile_effect((struct shader const *)&state->shader, state->bitmaps.index);
			}
		}
	}
	tag_iterator_new(&iterator, WEATHER_PARTICLE_SYSTEM_DEFINITION_TAG);
	while ((index = tag_iterator_next(&iterator)) != NONE)
	{
		struct weather_particle_system_definition *definition = weather_particle_system_definition_get(index);
		long type_index;

		for (type_index = 0; type_index < definition->particle_types.count; type_index++)
		{
			struct shader_precompile_weather_particle_type *type = TAG_BLOCK_GET_ELEMENT(
				&definition->particle_types, type_index, struct shader_precompile_weather_particle_type);

			shader_precompile_effect((struct shader const *)&type->shader, type->bitmap.index);
		}
	}
	tag_iterator_new(&iterator, CONTRAIL_DEFINITION_TAG);
	while ((index = tag_iterator_next(&iterator)) != NONE)
	{
		struct contrail_definition *definition = contrail_definition_get(index);

		shader_precompile_effect((struct shader const *)&definition->shader, definition->bitmap.index);
	}
	lightning_precompile_shaders();
	glows_precompile_shaders();
	tag_iterator_new(&iterator, ANTENNA_TAG);
	while ((index = tag_iterator_next(&iterator)) != NONE)
	{
		shader_precompile_effect((struct shader const *)&global_shader_effect_alpha_blended,
			antenna_definition_get(index)->texture.index);
	}
}

static void shader_precompile_map(void)
{
	struct rasterizer_light_submit_parameters saved_lights[2];
	struct tag_iterator iterator;
	long light_count = rasterizer_lights.light_count;
	long light_tag;
	short point_light_count = 0;
	short planar_mode = global_window_parameters.fog.planar_mode;
	short fog, i;

	shader_precompile_model_state_initialize();
	/* (two point lights of the map's first light tag at the camera, for a
	while: the light passes of the models, and of the structure with each
	light tag) */
	tag_iterator_new(&iterator, LIGHT_DEFINITION_TAG);
	light_tag = tag_iterator_next(&iterator);
	if (light_tag != NONE && light_count + 2 <= MAXIMUM_LIGHTS_PER_WINDOW)
	{
		for (i = 0; i < 2; i++)
		{
			struct rasterizer_light_submit_parameters *light = &rasterizer_lights.lights[light_count + i];

			saved_lights[i] = *light;
			memset(light, 0, sizeof(*light));
			light->definition = tag_get(LIGHT_DEFINITION_TAG, light_tag);
			light->position = global_window_parameters.camera.position;
			light->forward.k = 1.0f;
			light->up.i = 1.0f;
			light->color.red = 1.0f;
			light->radius = 10.0f;
		}
		rasterizer_lights.light_count = light_count + 2;
		point_light_count = 2;
	}

	/* (the models without planar fog and with it) */
	for (fog = 0; fog < 2; fog++)
	{
		global_window_parameters.fog.planar_mode = fog;
		shader_precompile_models(light_count, point_light_count);
	}
	global_window_parameters.fog.planar_mode = planar_mode;
	shader_precompile_skies();
	structure_render_precompile_shaders(point_light_count ? light_count : NONE);
	rasterizer_memory_pool_release(shader_precompile_pool_used);
	rasterizer_decals_precompile_shaders();
	rasterizer_widgets_precompile_shaders();
	shader_precompile_screen_flashes();
	shader_precompile_screen_quads();
	/* (the HUD's meters and motion sensor with their pixels: a high-res
	meter's texture brings its coverage and thresholds into the program) */
	texture_cache_set_no_pixels(FALSE);
	shader_precompile_hud_meters();
	shader_precompile_motion_sensor();
	texture_cache_set_no_pixels(TRUE);
	rasterizer_sun_glows_precompile_shaders();
	rasterizer_detail_objects_precompile_shaders();
	shader_precompile_effects();
	rasterizer_memory_pool_release(shader_precompile_pool_used);

	rasterizer_lights.light_count = light_count;
	for (i = 0; i < point_light_count; i++)
		rasterizer_lights.lights[light_count + i] = saved_lights[i];
}

/* ---------- public code */

void shader_precompile_new_map(
	void)
{
	shader_precompile_pending = config_boolean("display.shader_precompile") != 0;
}

void shader_precompile_window(
	void)
{
	unsigned long start;

	if (!shader_precompile_pending)
		return;
	shader_precompile_pending = FALSE;
	if (!d3d_gl_shader_precompile(TRUE))
		return;
	start = system_milliseconds();
	shader_precompile_pool_used = rasterizer_memory_pool_used();
	texture_cache_set_no_pixels(TRUE);
	shader_precompile_map();
	texture_cache_set_no_pixels(FALSE);
	d3d_gl_shader_precompile(FALSE);
	platform_log("shader precompile: %lu ms", system_milliseconds() - start);
}

void shader_precompile_effect(
	struct shader const *shader,
	long bitmap_group_index)
{
	struct bitmap_group *group;
	struct bitmap_data *bitmap;
	short variant;

	if (!shader || bitmap_group_index == NONE)
		return;
	group = bitmap_group_get(bitmap_group_index);
	if (!group || group->bitmaps.count < 1)
		return;
	bitmap = TAG_BLOCK_GET_ELEMENT(&group->bitmaps, 0, struct bitmap_data);
	/* (its header, without its pixels: a draw without a texture is one
	without a sampler) */
	if (!_texture_cache_bitmap_get_hardware_format(bitmap, TRUE, TRUE))
		return;
	/* (as contrails and lightning draw, in world space; as sprites, in
	view space, first person or not; on a quad of the unlit vertices, each
	24 bytes, as lightning's) */
	for (variant = 0; variant < 3; variant++)
	{
		unsigned long geometry_flags = variant == 0 ? 0 : FLAG(_rasterizer_geometry_viewspace_bit);
		long vertex_buffer_index;

		if (variant == 2)
			SET_FLAG(geometry_flags, _rasterizer_geometry_first_person_bit, TRUE);
		rasterizer_globals.current_lock_operation = _rasterizer_lock_sprite;
		vertex_buffer_index = rasterizer_dynamic_vertices_new(_rasterizer_vertex_type_dynamic_unlit, 4);
		if (vertex_buffer_index != NONE)
		{
			void *vertices = rasterizer_dynamic_vertices_lock(vertex_buffer_index);

			if (vertices)
				memset(vertices, 0, 4 * 24);
			rasterizer_dynamic_vertices_unlock(vertex_buffer_index);
			rasterizer_dynamic_unlit_geometry_draw(shader, bitmap, NULL, -4, vertex_buffer_index, 2,
				global_origin3d, geometry_flags);
			rasterizer_dynamic_vertices_delete(vertex_buffer_index);
		}
		rasterizer_globals.current_lock_operation = _rasterizer_lock_none;
	}
	shader_precompile_transparent_flush();
}

void shader_precompile_model_shadows(
	void)
{
	struct render_lighting lighting;
	struct tag_iterator iterator;
	real_point3d centroid = global_window_parameters.camera.position;
	long model_index;

	memset(&lighting, 0, sizeof(lighting));
	lighting.shadow_vector.k = -1.0f;
	tag_iterator_new(&iterator, MODELS_GROUP_TAG);
	while ((model_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct model *model = model_definition_get(model_index);
		short permutation_count = shader_precompile_model_permutation_count(model);
		short permutation, detail;

		for (permutation = 0; permutation < permutation_count; permutation++)
		{
			shader_precompile_model_permutation_set(model, permutation);
			for (detail = 0; detail < SHADER_PRECOMPILE_DETAIL_LEVELS; detail++)
			{
				render_model(model_index, shader_precompile_model_pixels(model, detail), shader_precompile_node_matrices,
					shader_precompile_permutations, shader_precompile_change_colors, shader_precompile_function_values,
					&lighting, &centroid, 1.0f, NULL, NONE, 0, FLAG(_render_model_shadow_bit));
			}
		}
	}
}

void shader_precompile_fog_screen_models(
	void)
{
	struct render_lighting lighting;
	struct render_model_effect effect;
	struct tag_iterator iterator;
	real_point3d centroid = global_window_parameters.camera.position;
	long model_index;

	memset(&lighting, 0, sizeof(lighting));
	lighting.ambient_color.red = 0.3f;
	lighting.ambient_color.green = 0.3f;
	lighting.ambient_color.blue = 0.3f;
	lighting.shadow_vector.k = -1.0f;
	memset(&effect, 0, sizeof(effect));
	effect.source_object_index = NONE;
	tag_iterator_new(&iterator, MODELS_GROUP_TAG);
	while ((model_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct model *model = model_definition_get(model_index);
		short permutation_count = shader_precompile_model_permutation_count(model);
		short permutation, detail;

		for (permutation = 0; permutation < permutation_count; permutation++)
		{
			shader_precompile_model_permutation_set(model, permutation);
			for (detail = 0; detail < SHADER_PRECOMPILE_DETAIL_LEVELS; detail++)
			{
				/* (the screen keeps at most 128 model parts: one level of
				detail of a model at a time) */
				rasterizer_environment_fog_screen_window_begin();
				/* (not as the sky, whose models the fog screen does not
				fog) */
				_rasterizer_models_begin(FALSE);
				render_model(model_index, shader_precompile_model_pixels(model, detail), shader_precompile_node_matrices,
					shader_precompile_permutations, shader_precompile_change_colors, shader_precompile_function_values,
					&lighting, &centroid, 1.0f, &effect, NONE, 0, 0);
				rasterizer_models_end();
				/* (with the surfaces after them, which draw with the
				texture modes the models leave) */
				structure_render_fog_screen();
				shader_precompile_transparent_flush();
			}
		}
	}
	/* (none left for the window's own) */
	rasterizer_environment_fog_screen_window_begin();
}
