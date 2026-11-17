/*
PELICAN_LIGHTS.C

The Pelican's red running lights are drawn by two transparent chicago
shaders on small disks laid on its curved hull (vehicles\pelican\shaders\
lights and lights flashing, alike but for one flag). A model's transparent
shader with the decal flag is drawn right after the model, over its hull,
with the decals' depth bias (D3DRS_ZBIAS, 8: rasterizer_xbox_transparent_
geometry.c); one without is tested against the hull as it is. The tags give
lights the flag and lights flashing none, so its lights won or lost
against the hull by the last bits of depth. And where the hull curves away
from the disks, seen at an angle as a Pelican takes off or flies by (the
extractions at the end of a30 and b30), the hull rose over them by more
than the bias: the lights were cut through and sank in, on the Xbox too.

As an Xbox map's tags load (scenario_tags_load), lights flashing is given
the decal flag, and both shaders are noted: drawn with the model, they are
biased PELICAN_LIGHTS_ZBIAS (pelican_lights_zbias), four times the decals',
so that they lie whole over the hull at any angle (a30's take-off, traced,
was cut at 8 and 16 and whole at 32 and 64). Nothing else's bias changes.
a10 (PELICAN_LIGHTS_UNCHANGED_MAP) is left as the tags have it, so that the
Pelicans in its first cutscene's hangar, which sit still, are lit as on the
Xbox. A map whose shaders are named otherwise is left as it is too. Only
the drawing changes, nothing played or sent between machines.
*/

/* ---------- headers */

#include "cseries.h"
#include "memory/data.h"
#include "shaders/shader_definitions.h"
#include "tag_files/tag_groups.h"

#include "pelican_lights.h"

/* ---------- constants */

#define PELICAN_LIGHTS_TAG 'schi'
/* (shaders.c) */
#define PELICAN_LIGHTS_DECAL_BIT 1
/* their depth bias, in the Xbox's z bias units (the decals' is 8) */
#define PELICAN_LIGHTS_ZBIAS 32
/* the map (its header's name) whose Pelicans keep the tags' lights */
#define PELICAN_LIGHTS_UNCHANGED_MAP "a10"

/* ---------- structures */

/* a transparent chicago shader's head (shaders.c) */
struct pelican_lights_chicago
{
	struct shader shader;
	byte numeric_counter_limit;
	byte flags;
	short first_map_type;
};

/* ---------- globals */

static char const *const pelican_lights_shader_names[] =
{
	"vehicles\\pelican\\shaders\\lights",
	"vehicles\\pelican\\shaders\\lights flashing",
};

/* the loaded map's light shaders, or NULL */
static struct shader const *pelican_lights_shaders[NUMBEROF(pelican_lights_shader_names)];

/* ---------- public code */

short pelican_lights_tags_loaded(
	char const *map_name)
{
	short changed = 0;
	short index;

	for (index = 0; index < NUMBEROF(pelican_lights_shader_names); index++)
	{
		long shader_index = map_name && !csstrcasecmp(map_name, PELICAN_LIGHTS_UNCHANGED_MAP) ?
			NONE :
			tag_loaded(PELICAN_LIGHTS_TAG, pelican_lights_shader_names[index]);
		struct pelican_lights_chicago *chicago = shader_index != NONE ?
			(struct pelican_lights_chicago *)tag_get(PELICAN_LIGHTS_TAG, shader_index) :
			NULL;

		pelican_lights_shaders[index] = chicago ? &chicago->shader : NULL;
		if (chicago && !TEST_FLAG(chicago->flags, PELICAN_LIGHTS_DECAL_BIT))
		{
			SET_FLAG(chicago->flags, PELICAN_LIGHTS_DECAL_BIT, TRUE);
			changed++;
		}
	}

	return changed;
}

void pelican_lights_tags_unloaded(
	void)
{
	short index;

	for (index = 0; index < NUMBEROF(pelican_lights_shaders); index++)
	{
		pelican_lights_shaders[index] = NULL;
	}

	return;
}

long pelican_lights_zbias(
	struct shader const *shader,
	long zbias)
{
	short index;

	if (!shader)
	{
		return zbias;
	}
	for (index = 0; index < NUMBEROF(pelican_lights_shaders); index++)
	{
		if (shader == pelican_lights_shaders[index])
		{
			return PELICAN_LIGHTS_ZBIAS;
		}
	}

	return zbias;
}
