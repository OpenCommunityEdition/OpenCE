/* Reuse authored attachment timing, colours, cones and render poses. Never
   add a second light beside an existing source or change its simulation. */
#include "cseries.h"
#include "objects/objects.h"
#include "tag_files/tag_files.h"
#include "authored_emitter_lighting.h"

#include "lighting_config.h"

static const struct authored_emitter_profile
{
	long group;
	const char *object, *light;
	float scale;
	int category;
} profiles[] = {
	{'weap', "weapons\\energy sword\\energy sword", "weapons\\energy sword\\energy sword dynamic", 2.5f, 1},
	{'proj', "weapons\\plasma grenade\\plasma grenade", "weapons\\plasma grenade\\plasma grenade", 1.5f, 3},
	{'weap', "weapons\\plasma pistol\\plasma pistol", "weapons\\plasma rifle\\overcharge", 1.5f, 1},
	{'weap', "weapons\\plasma pistol\\plasma pistol", "weapons\\plasma pistol\\plasma pistol flare", 1.0f, 1},
	{'weap', "weapons\\plasma rifle\\plasma rifle", "weapons\\plasma rifle\\plasma rifle flare", 10.0f, 1},
	{'vehi', "vehicles\\ghost\\ghost", "vehicles\\ghost\\ghost running", 2.2f, 2},
	{'vehi', "vehicles\\banshee\\banshee", "vehicles\\banshee\\banshee wing", 2.2f, 2},
	{'vehi', "vehicles\\banshee\\banshee_mp", "vehicles\\banshee\\banshee wing", 2.2f, 2},
	{'vehi', "vehicles\\warthog\\mp_warthog", "vehicles\\warthog\\headlights light", 1.0f, 2},
	{'vehi', "vehicles\\rwarthog\\rwarthog", "vehicles\\warthog\\headlights light", 1.0f, 2},
	{'vehi', "vehicles\\scorpion\\scorpion_mp", "vehicles\\scorpion\\headlight scorpion", 1.0f, 2},
	{'vehi', "vehicles\\warthog\\warthog", "vehicles\\warthog\\headlights light", 1.0f, 2},
	{'vehi', "vehicles\\scorpion\\scorpion", "vehicles\\scorpion\\headlight scorpion", 1.0f, 2},
};
static struct { long object, light; } identities[NUMBEROF(profiles)];
static int identities_ready;

int authored_emitter_lighting_category_enabled(int category)
{
	return lighting_config_boolean(category == 1 ? "display.weapon_glow" :
		category == 2 ? "display.vehicle_lighting" : "display.plasma_lighting");
}

int authored_emitter_lighting_enabled(void)
{
	return lighting_config_boolean("display.weapon_glow") || lighting_config_boolean("display.vehicle_lighting");
}

void authored_emitter_lighting_forget(void)
{
	identities_ready = FALSE;
}

int authored_emitter_lighting_profile(long object_index, long definition, float *scale)
{
	struct object_datum *object;
	int i;
	if (!authored_emitter_lighting_enabled() && !lighting_config_boolean("display.plasma_lighting"))
		return 0;
	object = object_try_and_get(object_index);
	if (!object || definition == NONE)
		return 0;
	if (!identities_ready)
	{
		for (i = 0; i < NUMBEROF(profiles); i++)
		{
			identities[i].object = tag_loaded(profiles[i].group, profiles[i].object);
			identities[i].light = tag_loaded('ligh', profiles[i].light);
		}
		identities_ready = TRUE;
	}
	for (i = 0; i < NUMBEROF(profiles); i++)
		if (identities[i].object != NONE && object->definition_index == identities[i].object &&
			definition == identities[i].light && authored_emitter_lighting_category_enabled(profiles[i].category))
		{
			*scale = profiles[i].scale;
			return profiles[i].category;
		}
	return 0;
}
