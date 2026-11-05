/* Additional Covenant projectile policy and bounded missing-light fallbacks.
   Authored attachments take precedence; effects and simulation stay stock. */
#include "cseries.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "objects/object_lights.h"
#include "tag_files/tag_files.h"
#include "camera/observer.h"
#include "game/players.h"
#include "covenant_lighting.h"
#include "plasma_occlusion.h"
#include <string.h>

#include "lighting_config.h"
long light_port_glow_definition_get(void);
boolean light_port_glow_set(long light_index, long object_index, real_rgb_color const *color, real radius);

enum { COVENANT_LIGHTS = 8 };
static struct covenant_light
{
	int active, connected, profile;
	long object_index, light_index, rasterizer_index;
} covenant_lights[COVENANT_LIGHTS];
static const struct covenant_profile
{
	const char *tag;
	real scale, radius; /* radius zero: use authored lights only */
	real_rgb_color color;
} covenant_profiles[] = {
	{"weapons\\needler\\needle", 1.5f, 1.0f, {0.85f, 0.2f, 1.0f}},
	{"weapons\\needler\\mp_needle", 1.5f, 1.0f, {0.85f, 0.2f, 1.0f}},
	{"vehicles\\ghost\\ghost bolt", 1.75f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"vehicles\\banshee\\banshee bolt", 1.75f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"vehicles\\banshee\\banshee fuel rod", 2.75f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"vehicles\\banshee\\mp_banshee fuel rod", 2.75f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"vehicles\\c gun turret\\c gun turret", 2.5f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"vehicles\\c gun turret\\mp gun turret", 2.0f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"weapons\\fuel rod gun\\fuel rod", 2.75f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"weapons\\plasma_cannon\\plasma_cannon", 2.75f, 0.0f, {0.0f, 0.0f, 0.0f}},
	{"vehicles\\wraith\\wraith mortar", 1.75f, 3.5f, {0.25f, 0.35f, 1.0f}},
};

static int covenant_profile_index(const char *name)
{
	int i;
	if (name)
		for (i = 0; i < NUMBEROF(covenant_profiles); i++)
			if (!strcmp(name, covenant_profiles[i].tag))
				return i;
	return NONE;
}

float covenant_lighting_scale(const char *name)
{
	int profile = covenant_profile_index(name);
	return profile == NONE ? 0.0f : covenant_profiles[profile].scale;
}

static const real_rgb_color covenant_black = {0.0f, 0.0f, 0.0f};
static long covenant_fuel_exhaust = NONE - 1;

/* Tint only the submitted copy of the shared Grunt/Hunter flight light.
   Keep its green channel and authored modulation; reducing red and blue
   removes the pale cast without brightening or recolouring stock effects. */
void covenant_lighting_color(long object_index, long definition_index, real_rgb_color *color)
{
	struct object_datum *object;
	const char *name;
	if (!lighting_config_boolean("display.plasma_lighting"))
		return;
	object = object_try_and_get(object_index);
	if (!object || object->object.type != _object_type_projectile)
		return;
	name = tag_get_name(object->definition_index);
	if (!name || strcmp(name, "weapons\\fuel rod gun\\fuel rod"))
		return;
	if (covenant_fuel_exhaust == NONE - 1)
		covenant_fuel_exhaust = tag_loaded('ligh', "weapons\\fuel rod gun\\fuel rod exhaust");
	if (covenant_fuel_exhaust == NONE || definition_index != covenant_fuel_exhaust)
		return;
	color->red *= 0.30f;
	color->blue *= 0.20f;
}

/* Return the validated profile so admission need not resolve the tag again. */
static int covenant_visible_profile(long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);
	struct object_definition *definition;
	const char *name;
	short player;
	int i, profile;
	if (!object || object->object.type != _object_type_projectile ||
		object->object.node_matrices.size < sizeof(real_matrix4x3))
		return NONE;
	name = tag_get_name(object->definition_index);
	profile = covenant_profile_index(name);
	if (profile == NONE || covenant_profiles[profile].radius <= 0.0f)
		return NONE;
	definition = object_definition_get(object->definition_index);
	/* Model-less projectiles (the stock Wraith mortar) remain invisible in
	   object_set_visibility even while their particle attachments are visible. */
	if (TEST_FLAG(object->object.flags, _object_invisible_bit) && definition->object.model.index != NONE)
		return NONE;
	/* A custom map's authored attachment takes precedence over a new glow. */
	for (i = 0; i < definition->object.attachments.count && i < MAXIMUM_NUMBER_OF_ATTACHMENTS_PER_OBJECT; i++)
	{
		struct object_attachment_definition *attachment =
			TAG_BLOCK_GET_ELEMENT(&definition->object.attachments, i, struct object_attachment_definition);
		if (attachment->type.group_tag == 'ligh' && attachment->type.index != NONE)
			return NONE;
	}
	for (player = local_player_get_next(NONE); player != NONE; player = local_player_get_next(player))
	{
		const struct observer_result *camera = observer_get_camera(player);
		if (camera && distance3d(&object->object.bounding_sphere_center, &camera->position) <= 25.0f)
			return profile;
	}
	return NONE;
}

static void covenant_release(struct covenant_light *light)
{
	/* The salted handle, owner and glow definition must still match. */
	if (light->active && light_port_glow_set(light->light_index, light->object_index, &covenant_black, 0.0f))
		light_delete(light->light_index);
	memset(light, 0, sizeof(*light));
}

void covenant_lighting_update(void)
{
	struct object_iterator iterator;
	int i, slot, profile;
	int enabled = lighting_config_boolean("display.plasma_lighting");
	for (i = 0; i < COVENANT_LIGHTS; i++)
		if (covenant_lights[i].active && (!enabled || covenant_visible_profile(covenant_lights[i].object_index) == NONE))
			covenant_release(&covenant_lights[i]);
	if (!enabled)
		return;
	object_iterator_new(&iterator, _object_mask_projectile, 0);
	while (object_iterator_next(&iterator))
	{
		struct covenant_light *light;
		long definition;
		profile = covenant_visible_profile(iterator.index);
		if (profile == NONE)
			continue;
		slot = NONE;
		for (i = 0; i < COVENANT_LIGHTS; i++)
		{
			if (covenant_lights[i].active && covenant_lights[i].object_index == iterator.index)
			{
				slot = i;
				break;
			}
			if (!covenant_lights[i].active && slot == NONE)
				slot = i;
		}
		if (slot == NONE)
			continue;
		light = &covenant_lights[slot];
		light->profile = profile;
		if (light->active && light_port_glow_set(light->light_index, iterator.index, &covenant_profiles[light->profile].color, covenant_profiles[light->profile].radius))
		{
			if (!light->connected)
				light_reconnect_to_map(light->light_index);
			light->connected = TRUE;
			continue;
		}
		memset(light, 0, sizeof(*light));
		light->profile = profile;
		definition = light_port_glow_definition_get();
		if (definition == NONE)
			continue;
		light->light_index = light_new_unattached(definition, iterator.index, 0,
			(real_point3d const *)global_zero_vector3d, global_forward3d, 0.0f);
		if (light->light_index == NONE)
			continue;
		light->object_index = iterator.index;
		light->rasterizer_index = NONE;
		light->active = light->connected = TRUE;
		light_port_glow_set(light->light_index, iterator.index, &covenant_profiles[light->profile].color, covenant_profiles[light->profile].radius);
	}
}

void covenant_lighting_frame_end(void)
{
	int i;
	for (i = 0; i < COVENANT_LIGHTS; i++)
	{
		struct covenant_light *light = &covenant_lights[i];
		if (!light->active)
			continue;
		if (!lighting_config_boolean("display.plasma_lighting"))
			covenant_release(light);
		else if (light_port_glow_set(light->light_index, light->object_index, &covenant_black, 0.0f) && light->connected)
		{
			light_disconnect_from_map(light->light_index);
			light->connected = FALSE;
		}
		light->rasterizer_index = NONE;
	}
}

void covenant_lighting_forget(void)
{
	/* Called only as the engine discards its entire light array/partition. */
	memset(covenant_lights, 0, sizeof(covenant_lights));
	covenant_fuel_exhaust = NONE - 1;
}

void covenant_lighting_window_begin(void)
{
	int i;
	for (i = 0; i < COVENANT_LIGHTS; i++)
		covenant_lights[i].rasterizer_index = NONE;
}

void covenant_lighting_submitted(long light_index, long rasterizer_index)
{
	int i;
	for (i = 0; i < COVENANT_LIGHTS; i++)
		if (covenant_lights[i].active && covenant_lights[i].light_index == light_index)
		{
			covenant_lights[i].rasterizer_index = rasterizer_index;
			plasma_occlusion_submit(light_index, rasterizer_index);
			return;
		}
}

int covenant_lighting_selected(long rasterizer_index)
{
	int i;
	if (rasterizer_index == NONE || !lighting_config_boolean("display.plasma_lighting"))
		return FALSE;
	for (i = 0; i < COVENANT_LIGHTS; i++)
		if (covenant_lights[i].active && covenant_lights[i].rasterizer_index == rasterizer_index)
			return TRUE;
	return FALSE;
}

/* Visibility bounds must cover the owned flight glow, even when the map's
   borrowed template describes a smaller muzzle flash. */
float covenant_lighting_radius(long light_index, long object_index, float radius)
{
	int i;
	if (!lighting_config_boolean("display.plasma_lighting"))
		return radius;
	for (i = 0; i < COVENANT_LIGHTS; i++)
		if (covenant_lights[i].active && covenant_lights[i].light_index == light_index &&
			covenant_lights[i].object_index == object_index)
			return MAX(radius, covenant_profiles[covenant_lights[i].profile].radius);
	return radius;
}

int covenant_lighting_owns(long light_index, long object_index)
{
	int i;
	for (i = 0; i < COVENANT_LIGHTS; i++)
		if (covenant_lights[i].active && covenant_lights[i].light_index == light_index &&
			covenant_lights[i].object_index == object_index)
			return TRUE;
	return FALSE;
}
