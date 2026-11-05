/* Reuse authored attachment timing, colours, cones and render poses. Never
   add a second light beside an existing source or change its simulation. */
#include "cseries.h"
#include "objects/objects.h"
#include "tag_files/tag_files.h"
#include "authored_emitter_lighting.h"
#include "camera/observer.h"
#include "game/players.h"

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
	/* Prong flares have a short, broad forward spill (category 4). The
	   authored pistol radius is 1.5, the rifle's 0.1: both reach 0.3 here. */
	{'weap', "weapons\\plasma pistol\\plasma pistol", "weapons\\plasma pistol\\plasma pistol flare", 0.2f, 4},
	{'weap', "weapons\\plasma rifle\\plasma rifle", "weapons\\plasma rifle\\plasma rifle flare", 3.0f, 4},
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

/* Tiny prong spill is a close-range detail. Fade its radius continuously
   from three to six world units, considering every local player's view.
   The authored luminous surface/flare is not changed at any distance. */
static float prong_reach(struct object_datum const *object)
{
	int i, count = local_player_count();
	float nearest = 36.0f;
	if (!count) return 1.0f;
	for (i = 0; i < count; i++)
	{
		struct observer_result const *camera = observer_get_camera((short)i);
		float x = camera->position.x - object->object.bounding_sphere_center.x;
		float y = camera->position.y - object->object.bounding_sphere_center.y;
		float z = camera->position.z - object->object.bounding_sphere_center.z;
		float distance = x*x + y*y + z*z;
		nearest = MIN(nearest, distance);
	}
	nearest = PIN((36.0f - nearest) / 27.0f, 0.0f, 1.0f);
	return nearest * nearest * (3.0f - 2.0f * nearest);
}

int authored_emitter_lighting_category_enabled(int category)
{
	return lighting_config_boolean(category == 4 ? "display.plasma_prong_lighting" : category == 1 ? "display.weapon_glow" :
		category == 2 ? "display.vehicle_lighting" : "display.plasma_lighting");
}

int authored_emitter_lighting_enabled(void)
{
	return lighting_config_boolean("display.weapon_glow") || lighting_config_boolean("display.plasma_prong_lighting") || lighting_config_boolean("display.vehicle_lighting");
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
			if (profiles[i].category == 4)
			{
				*scale *= prong_reach(object);
				if (*scale <= 0.0001f) return 0;
			}
			return profiles[i].category;
		}
	return 0;
}
