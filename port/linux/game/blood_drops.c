/*
BLOOD_DROPS.C

Blood drips drawn moving (Video Setup > Death and Blood > Blood Drips:
animated): the drop at the head of each run of blood, swelling where it
gathers, and drops hanging from edges and undersides, then falling, as
blood.c runs them. Drawn as sprites with the map's own blood particle of the
same species as the run's splat (an Elite's, a Grunt's, a human's...),
tinted the colour of that splat, in the particles' pass (render_particles.c)
after the game's particles. Nothing here is simulated: only drawn.
*/

#include "cseries.h"
#include "math/real_math.h"
#include "effects/particles.h"
#include "effects/decals.h"
#include "render/render_sprite.h"
#include "tag_files/tag_files.h"
#include "cache/cache_files.h"
#include "game/game.h"

#include "blood.h"

#include <string.h>

enum
{
	BLOOD_DROPS_MAXIMUM = 96,
	BLOOD_DROPS_KINDS = 8,
};

/* each splat drawn so far this map: the particle its drops are drawn with,
and their tint */
static struct
{
	long decal;
	long particle;
	real_argb_color color;
} blood_drop_kinds[BLOOD_DROPS_KINDS];
static short blood_drop_kind_next = 0;
static long blood_drops_time = NONE;

/* (the species a blood tag's name says) */
static char const *blood_drops_species(char const *name)
{
	static char const *const species[] = { "elite", "grunt", "jackal", "hunter", "engineer", "flood" };
	short index;

	for (index = 0; name && index < (short)NUMBEROF(species); index++)
	{
		if (strstr(name, species[index]))
			return species[index];
	}
	return NULL;
}

/* the map's blood particle of a species (NULL: a human's), the most solid
of them: a burst's, else an impact's, else any */
static long blood_drops_particle(char const *species)
{
	struct tag_iterator iterator;
	long tag_index;
	long best = NONE;
	short best_score = -1;

	tag_iterator_new(&iterator, PARTICLE_TAG);
	while ((tag_index = tag_iterator_next(&iterator)) != NONE)
	{
		char const *name = tag_get_name(tag_index);
		struct particle_definition *definition;
		char const *its;
		short score;

		if (!name || !strstr(name, "blood") || strstr(name, "mist") || strstr(name, "glow"))
			continue;
		its = blood_drops_species(name);
		if (species ? (!its || strcmp(its, species)) : (its && !strstr(name, "blood h ")))
			continue;
		definition = particle_definition_get(tag_index);
		if (!definition || definition->bitmap.index == NONE || definition->bitmap.index == 0)
			continue;
		score = strstr(name, "burst") ? 3 : strstr(name, "impact") ? 2 : 1;
		if (!species && (strstr(name, "human") || strstr(name, "blood h ")))
			score += 2;
		if (score > best_score)
		{
			best = tag_index;
			best_score = score;
		}
	}
	return best;
}

static short blood_drops_kind(long decal)
{
	long now = game_time_get();
	short index;

	if (blood_drops_time == NONE || now < blood_drops_time)
	{
		for (index = 0; index < BLOOD_DROPS_KINDS; index++)
			blood_drop_kinds[index].decal = NONE;
	}
	blood_drops_time = now;
	for (index = 0; index < BLOOD_DROPS_KINDS; index++)
	{
		if (blood_drop_kinds[index].decal == decal)
			return index;
	}
	index = blood_drop_kind_next;
	blood_drop_kind_next = (short)((blood_drop_kind_next + 1) % BLOOD_DROPS_KINDS);
	{
		char const *species = blood_drops_species(tag_get_name(decal));
		real_rgb_color rgb;
		real brightest;

		blood_drop_kinds[index].decal = decal;
		blood_drop_kinds[index].particle = blood_drops_particle(species);
		if (blood_drop_kinds[index].particle == NONE && species)
			blood_drop_kinds[index].particle = blood_drops_particle(NULL);
		/* (the splat's colour: a tinted splat's tint; an untinted one's,
		white, leaves the particle's own colours; either darkened a little,
		wet blood, not lit spray) */
		decal_definition_color(decal, &rgb);
		brightest = MAX(rgb.red, MAX(rgb.green, rgb.blue));
		if (brightest <= 0.f)
			rgb.red = rgb.green = rgb.blue = brightest = 1.f;
		blood_drop_kinds[index].color.alpha = 1.f;
		/* (the particles' shaders add light: a drop in the air drawn dim,
		dark against what is behind it, not glowing) */
		blood_drop_kinds[index].color.red = rgb.red * 0.28f;
		blood_drop_kinds[index].color.green = rgb.green * 0.28f;
		blood_drop_kinds[index].color.blue = rgb.blue * 0.28f;
	}
	return index;
}

void blood_drops_render(
	void)
{
	static real_point3d positions[BLOOD_DROPS_MAXIMUM];
	static real_vector3d directions[BLOOD_DROPS_MAXIMUM];
	static real radii[BLOOD_DROPS_MAXIMUM];
	static long decals[BLOOD_DROPS_MAXIMUM];
	static short kinds[BLOOD_DROPS_MAXIMUM];
	short count;
	short index;
	short kind;

	count = blood_drops_for_drawing(positions, directions, radii, decals, BLOOD_DROPS_MAXIMUM);
	if (count <= 0)
		return;
	for (index = 0; index < count; index++)
		kinds[index] = blood_drops_kind(decals[index]);

	/* (each kind's drops together: one particle's picture and shader) */
	for (kind = 0; kind < BLOOD_DROPS_KINDS; kind++)
	{
		struct particle_definition *definition;
		struct build_sprite_data sprite_data;
		short kind_count = 0;
		real total_radius = 0.f;

		for (index = 0; index < count; index++)
		{
			if (kinds[index] == kind)
				kind_count++;
		}
		if (!kind_count || blood_drop_kinds[kind].particle == NONE)
			continue;
		definition = particle_definition_get(blood_drop_kinds[kind].particle);
		if (!definition || definition->bitmap.index == NONE || definition->bitmap.index == 0)
			continue;
		build_sprites_begin(&sprite_data, kind_count, definition->bitmap.index, &definition->shader, 0);
		for (index = 0; index < count; index++)
		{
			real speed_squared;
			boolean falling;

			if (kinds[index] != kind)
				continue;
			speed_squared = directions[index].i * directions[index].i + directions[index].j * directions[index].j +
				directions[index].k * directions[index].k;
			/* (falling fast: along its way; else round, facing you) */
			falling = directions[index].k < -0.004f && speed_squared > 1e-5f;
			build_sprite(&sprite_data, falling ? _build_sprite_parallel : _build_sprite_normal,
				definition->first_sequence_index > 0 ? definition->first_sequence_index : 0, 0,
				&positions[index], &directions[index], 0.f, radii[index] * 2.f * definition->runtime_oo_width,
				&blood_drop_kinds[kind].color, 1.f, 0);
			total_radius += radii[index];
		}
		((struct shader_effect_definition *)sprite_data.shader)->secondary_map_radius = total_radius / kind_count;
		build_sprites_end(&sprite_data);
	}
}
