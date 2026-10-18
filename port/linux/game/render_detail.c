/*
RENDER_DETAIL.C

Detail a slower machine may lower, as the PC version's video settings let it
(Specular, Shadows, Decals, Particles), and scenery too small on screen to
matter. Every setting's default draws as the original.

The switches set here are the game's own (also its console variables): each
is written only when its setting changes, and never while the setting is at
its default, so scripts and the console keep them otherwise.
*/

#include "cseries.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "render/render.h"
#include "rasterizer/rasterizer_model_types.h"
#include "models/models.h"
#include "models/model_definitions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HALO_MAC
#include "mac_gpu_info.h" /* port: port/mac/mac_gpu_info.c */
#endif

/* port/linux/src/port_config.c */
double config_real(const char *name);
int config_boolean(const char *name);
int config_is_default(const char *name);
const char *config_string(const char *name);
unsigned long config_changes(void);

/* render/render.c, render/render_objects.c */
extern boolean render_particles_enabled;
extern boolean render_contrails_enabled;
extern boolean render_weather_particle_systems_enabled;
extern boolean render_shadows;

enum
{
	_render_detail_particles_high,
	_render_detail_particles_low,
	_render_detail_particles_off
};

static unsigned long render_detail_changes = (unsigned long)-1;
static real render_detail_scenery_pixels = 0.f;
/* the values last applied: stock until a setting says otherwise */
static boolean render_detail_specular = TRUE;
static boolean render_detail_shadows = TRUE;
static boolean render_detail_decals = TRUE;
static short render_detail_particles = _render_detail_particles_high;

static short render_detail_particles_setting(
	void)
{
	char const *value = config_string("display.particles");

#ifdef HALO_MAC
	/* the Mac's mac.detail preset, unless display.particles is set */
	{
		struct mac_detail_preset preset;

		if (config_is_default("display.particles") && mac_gpu_info_detail(&preset))
			value = preset.particles == 2 ? "off" : preset.particles == 1 ? "low" : "high";
	}
#endif

	if (value && !strcmp(value, "off"))
		return _render_detail_particles_off;
	if (value && !strcmp(value, "low"))
		return _render_detail_particles_low;
	return _render_detail_particles_high;
}

/* the particles' detail: at low, the effects' particles smaller on screen
than this many pixels are left out, and none is enlarged to its definition's
minimum size (render_particles.c); high draws them all, as the original */
static real render_detail_particle_cull = 0.f;
static boolean render_detail_particle_enlarge = TRUE;

real render_detail_particle_cull_pixels(
	void)
{
	return render_detail_particle_cull;
}

boolean render_detail_particle_minimum_pixels(
	void)
{
	return render_detail_particle_enlarge;
}

/* At low particles the effects' particles (dust, smoke from a blast) are drawn no wider on screen
than this many pixels (0: as the original), and the ones above it are made less of by the mode:
0 only smaller, 1 and also this much less opaque, 2 as much less opaque as their area is cut,
3 and a share of them left out (render_particles.c; the Mac's presets). One that covers the
screen is hundreds of times the pixels of a small one, which a small card cannot fill.
(Common symbols, as render_particles.c declares them too: it is linked without this file in
the Mac's render graph.) */
real render_detail_particle_maximum;
short render_detail_particle_limit_mode;

/* The detail of objects (the Mac's presets): the models are drawn at the level of detail of this fraction of their size on
screen (1 is the original), and objects other than scenery (which has its own minimum) smaller
on screen than this many pixels are left out. */
static real render_detail_lod = 1.f;
static real render_detail_object_pixels = 0.f;

real render_detail_lod_scale(
	void)
{
	return render_detail_lod;
}

real render_detail_object_minimum_pixels(
	void)
{
	return render_detail_object_pixels;
}

void render_detail_update(
	void)
{
	unsigned long changes = config_changes();
#ifdef HALO_MAC
	/* (also when the machine or the map is known) */
	changes += 65536UL * mac_gpu_info_serial();
#endif

	if (changes != render_detail_changes)
	{
		double value = config_real("display.scenery_minimum_pixels");
		boolean specular = config_boolean("display.specular") != 0;
		boolean shadows = config_boolean("display.shadows") != 0;
		boolean decals = config_boolean("display.decals") != 0;
		short particles = render_detail_particles_setting();
		real lod = 1.f, object_pixels = 0.f, particle_maximum = 0.f;
		short particle_mode = 0;
		boolean lens_flares = TRUE;

#ifdef HALO_MAC
		/* mac.detail's preset takes the detail settings left at their defaults */
		{
			struct mac_detail_preset preset;

			if (mac_gpu_info_detail(&preset))
			{
				if (config_is_default("display.specular"))
					specular = preset.specular != 0;
				if (config_is_default("display.shadows"))
					shadows = preset.shadows != 0;
				if (config_is_default("display.decals"))
					decals = preset.decals != 0;
				if (config_is_default("display.scenery_minimum_pixels"))
					value = preset.scenery_pixels;
				lod = (real)preset.lod_scale;
				object_pixels = (real)preset.object_pixels;
				lens_flares = preset.lens_flares != 0;
				particle_maximum = (real)preset.particle_pixels;
				particle_mode = (short)preset.particle_mode;
				fprintf(stderr, "mac settings: detail: specular %d, shadows %d, decals %d, particles %s, scenery minimum %g pixels, object minimum %g pixels, detail %g, lens flares %d, particle maximum %g pixels (mode %d)\n",
					specular, shadows, decals, render_detail_particles_setting() == _render_detail_particles_off ? "off" :
					render_detail_particles_setting() == _render_detail_particles_low ? "low" : "high", value,
					(double)object_pixels, (double)lod, lens_flares, (double)particle_maximum, (int)particle_mode);
			}
		}
#endif

		render_detail_scenery_pixels = value > 0.0 && value < 4096.0 ? (real)value : 0.f;
		(void)lens_flares;
		render_detail_particle_maximum = particles == _render_detail_particles_low && particle_maximum > 0.f ?
			particle_maximum : 0.f;
		render_detail_particle_limit_mode = particle_mode;
		render_detail_lod = lod > 0.f && lod <= 1.f ? lod : 1.f;
		render_detail_object_pixels = object_pixels > 0.f && object_pixels < 4096.f ? object_pixels : 0.f;
#ifdef HALO_MAC
		rasterizer_debug_options.draw_lens_flares = lens_flares;
#endif
		if (specular != render_detail_specular)
		{
			/* the PC version's Specular: the environment's specular and
			reflection passes */
			rasterizer_debug_options.draw_environment_specular_lights = specular;
			rasterizer_debug_options.draw_environment_specular_lightmaps = specular;
			rasterizer_debug_options.draw_environment_reflection_lightmap_masks = specular;
			rasterizer_debug_options.draw_environment_reflections = specular;
			render_detail_specular = specular;
		}
		if (shadows != render_detail_shadows)
		{
			render_shadows = shadows;
			render_detail_shadows = shadows;
		}
		if (decals != render_detail_decals)
		{
			rasterizer_debug_options.draw_environment_decals = decals;
			render_detail_decals = decals;
		}
		if (particles != render_detail_particles)
		{
			/* low keeps the effects' own particles and contrails, without
			the particle systems (smoke, dust) and the weather */
			render_particle_systems_enabled = particles == _render_detail_particles_high;
			render_weather_particle_systems_enabled = particles == _render_detail_particles_high;
			render_particles_enabled = particles != _render_detail_particles_off;
			render_contrails_enabled = particles != _render_detail_particles_off;
			render_detail_particles = particles;
		}
		render_detail_changes = changes;
	}
}

real render_detail_scenery_minimum_pixels(
	void)
{
	render_detail_update();
	return render_detail_scenery_pixels;
}

/* Scenery batching (display.batch_scenery): the opaque models of scenery
(trees and rocks, in the hundreds on some levels) are drawn after the others
are found, a definition at a time and a shader at a time within it, so that
the draws which share a shader and its bitmaps follow one another and the
graphics card is told of fewer changes. Opaque draws are depth tested, so
the picture is the same; the objects with widgets, children or any effect
are drawn at once, as before. */

/* models/models.c */
extern short render_model_batch_filter;
long render_model_batch_first_shader(struct model const *model);
#define RENDER_MODEL_BATCH_REST (-2)

#define RENDER_BATCH_MAXIMUM_ENTRIES 384

struct render_batch_entry
{
	long model_index;
	real level_of_detail_pixels;
	real_matrix4x3 const *node_matrices;
	char const *region_permutation_indices;
	real_rgb_color const *change_colors;
	real const *function_values;
	struct render_lighting lighting;
	real_point3d centroid;
	real radius;
	struct render_model_effect model_effect;
	long object_index;
	short forced_shader_permutation_index;
	unsigned long flags;
};

struct render_batch_order
{
	long first_shader;
	long model_index;
	/* what picks an object's vertex program besides its model: the planar
	fog flag and whether point lights reach it */
	unsigned long program_class;
	long sequence;
};

static struct render_batch_entry render_batch_entries[RENDER_BATCH_MAXIMUM_ENTRIES];
static struct render_batch_order render_batch_order[RENDER_BATCH_MAXIMUM_ENTRIES];
static long render_batch_count = 0;

boolean render_detail_batch_scenery(
	void)
{
	return config_boolean("display.batch_scenery") != 0;
}

static int render_batch_compare(
	void const *a,
	void const *b)
{
	struct render_batch_order const *x = (struct render_batch_order const *)a;
	struct render_batch_order const *y = (struct render_batch_order const *)b;

	if (x->first_shader != y->first_shader)
		return x->first_shader < y->first_shader ? -1 : 1;
	if (x->model_index != y->model_index)
		return x->model_index < y->model_index ? -1 : 1;
	if (x->program_class != y->program_class)
		return x->program_class < y->program_class ? -1 : 1;
	return x->sequence < y->sequence ? -1 : x->sequence > y->sequence;
}

void render_detail_batch_scenery_flush(
	void)
{
	long i, n, first, last, shader_index;

	if (render_batch_count <= 0)
	{
		return;
	}

	for (i = 0; i < render_batch_count; i++)
	{
		struct model *model = model_definition_get(render_batch_entries[i].model_index);

		render_batch_order[i].first_shader = render_model_batch_first_shader(model);
		render_batch_order[i].model_index = render_batch_entries[i].model_index;
		render_batch_order[i].program_class = render_batch_entries[i].flags * 2 +
			(render_batch_entries[i].lighting.point_light_count > 0 ? 1 : 0);
		render_batch_order[i].sequence = i;
	}
	qsort(render_batch_order, (size_t)render_batch_count, sizeof(render_batch_order[0]), render_batch_compare);

	for (first = 0; first < render_batch_count; first = last)
	{
		struct model *model;
		long model_index = render_batch_order[first].model_index;

		for (last = first + 1; last < render_batch_count && render_batch_order[last].model_index == model_index; last++)
			;
		model = model_definition_get(model_index);

		/* a shader at a time, then the decals and transparent parts of each */
		for (shader_index = 0; shader_index <= model->shaders.count; shader_index++)
		{
			render_model_batch_filter = shader_index < model->shaders.count ? (short)shader_index : RENDER_MODEL_BATCH_REST;
			/* (and the objects in the other direction each time, so that the
			program class that ended a shader's draws starts the next's) */
			for (n = 0; n < last - first; n++)
			{
				struct render_batch_entry *entry = &render_batch_entries[render_batch_order[
					(shader_index & 1) ? last - 1 - n : first + n].sequence];

				render_model(
					entry->model_index,
					entry->level_of_detail_pixels,
					entry->node_matrices,
					entry->region_permutation_indices,
					entry->change_colors,
					entry->function_values,
					&entry->lighting,
					&entry->centroid,
					entry->radius,
					&entry->model_effect,
					entry->object_index,
					entry->forced_shader_permutation_index,
					entry->flags);
			}
		}
		render_model_batch_filter = NONE;
	}

	render_batch_count = 0;
}

/* TRUE if the model was kept for the flush (render_model's arguments) */
boolean render_detail_batch_scenery_model(
	long model_index,
	real level_of_detail_pixels,
	real_matrix4x3 const *node_matrices,
	char const *region_permutation_indices,
	real_rgb_color const *change_colors,
	real const *function_values,
	struct render_lighting const *lighting,
	real_point3d const *centroid,
	real radius,
	struct render_model_effect const *model_effect,
	long object_index,
	short forced_shader_permutation_index,
	unsigned long flags)
{
	struct render_batch_entry *entry;

	if (!lighting || !centroid || !node_matrices || !model_effect || model_index == NONE ||
		model_effect->type != 0 /* _render_model_effect_type_none */ || model_effect->modifier_shader ||
		!render_detail_batch_scenery())
	{
		return FALSE;
	}
	if (render_batch_count >= RENDER_BATCH_MAXIMUM_ENTRIES)
	{
		render_detail_batch_scenery_flush();
	}

	entry = &render_batch_entries[render_batch_count++];
	entry->model_index = model_index;
	entry->level_of_detail_pixels = level_of_detail_pixels;
	entry->node_matrices = node_matrices;
	entry->region_permutation_indices = region_permutation_indices;
	entry->change_colors = change_colors;
	entry->function_values = function_values;
	entry->lighting = *lighting;
	entry->centroid = *centroid;
	entry->radius = radius;
	entry->model_effect = *model_effect;
	entry->object_index = object_index;
	entry->forced_shader_permutation_index = forced_shader_permutation_index;
	entry->flags = flags;
	return TRUE;
}
