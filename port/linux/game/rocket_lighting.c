/* Stock rockets have no flight light. These bounded, render-owned glows
   reuse the engine's glow interface without editing projectile/effect tags. */
#include "cseries.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "objects/object_lights.h"
#include "tag_files/tag_files.h"
#include "camera/observer.h"
#include "game/players.h"
#include "rocket_lighting.h"
#include "plasma_occlusion.h"
#include <string.h>

#include "lighting_config.h"
long light_port_glow_definition_get(void);
boolean light_port_glow_set(long light_index, long object_index, real_rgb_color const *color, real radius);

enum { ROCKET_LIGHTS = 8 };
static struct rocket_light
{
	int active, connected;
	long object_index, light_index, rasterizer_index;
} rocket_lights[ROCKET_LIGHTS];
static const real_rgb_color rocket_color = {1.0f, 0.45f, 0.12f};
static const real_rgb_color rocket_black = {0.0f, 0.0f, 0.0f};

static int rocket_visible(long object_index)
{
	struct object_datum *object = object_try_and_get(object_index);
	struct object_definition *definition;
	const char *name;
	short player;
	int i;
	if (!object || object->object.type != _object_type_projectile ||
		TEST_FLAG(object->object.flags, _object_invisible_bit) ||
		object->object.node_matrices.size < sizeof(real_matrix4x3))
		return FALSE;
	name = tag_get_name(object->definition_index);
	if (!name || strcmp(name, "weapons\\rocket launcher\\rocket"))
		return FALSE;
	definition = object_definition_get(object->definition_index);
	/* A custom map's authored attachment takes precedence over a new glow. */
	for (i = 0; i < definition->object.attachments.count && i < MAXIMUM_NUMBER_OF_ATTACHMENTS_PER_OBJECT; i++)
	{
		struct object_attachment_definition *attachment =
			TAG_BLOCK_GET_ELEMENT(&definition->object.attachments, i, struct object_attachment_definition);
		if (attachment->type.group_tag == 'ligh' && attachment->type.index != NONE)
			return FALSE;
	}
	for (player = local_player_get_next(NONE); player != NONE; player = local_player_get_next(player))
	{
		const struct observer_result *camera = observer_get_camera(player);
		if (camera && distance3d(&object->object.bounding_sphere_center, &camera->position) <= 25.0f)
			return TRUE;
	}
	return FALSE;
}

static void rocket_release(struct rocket_light *light)
{
	/* The salted handle, owner and glow definition must still match. */
	if (light->active && light_port_glow_set(light->light_index, light->object_index, &rocket_black, 0.0f))
		light_delete(light->light_index);
	memset(light, 0, sizeof(*light));
}

void rocket_lighting_update(void)
{
	struct object_iterator iterator;
	int i, slot;
	int enabled = lighting_config_boolean("display.plasma_lighting");
	for (i = 0; i < ROCKET_LIGHTS; i++)
		if (rocket_lights[i].active && (!enabled || !rocket_visible(rocket_lights[i].object_index)))
			rocket_release(&rocket_lights[i]);
	if (!enabled)
		return;
	object_iterator_new(&iterator, _object_mask_projectile, 0);
	while (object_iterator_next(&iterator))
	{
		struct rocket_light *light;
		long definition;
		if (!rocket_visible(iterator.index))
			continue;
		slot = NONE;
		for (i = 0; i < ROCKET_LIGHTS; i++)
		{
			if (rocket_lights[i].active && rocket_lights[i].object_index == iterator.index)
			{
				slot = i;
				break;
			}
			if (!rocket_lights[i].active && slot == NONE)
				slot = i;
		}
		if (slot == NONE)
			continue;
		light = &rocket_lights[slot];
		if (light->active && light_port_glow_set(light->light_index, iterator.index, &rocket_color, 1.75f))
		{
			if (!light->connected)
				light_reconnect_to_map(light->light_index);
			light->connected = TRUE;
			continue;
		}
		memset(light, 0, sizeof(*light));
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
		light_port_glow_set(light->light_index, iterator.index, &rocket_color, 1.75f);
	}
}

void rocket_lighting_frame_end(void)
{
	int i;
	for (i = 0; i < ROCKET_LIGHTS; i++)
	{
		struct rocket_light *light = &rocket_lights[i];
		if (!light->active)
			continue;
		if (!lighting_config_boolean("display.plasma_lighting"))
			rocket_release(light);
		else if (light_port_glow_set(light->light_index, light->object_index, &rocket_black, 0.0f) && light->connected)
		{
			light_disconnect_from_map(light->light_index);
			light->connected = FALSE;
		}
		light->rasterizer_index = NONE;
	}
}

void rocket_lighting_forget(void)
{
	/* Called only as the engine discards its entire light array/partition. */
	memset(rocket_lights, 0, sizeof(rocket_lights));
}

void rocket_lighting_window_begin(void)
{
	int i;
	for (i = 0; i < ROCKET_LIGHTS; i++)
		rocket_lights[i].rasterizer_index = NONE;
}

void rocket_lighting_submitted(long light_index, long rasterizer_index)
{
	int i;
	for (i = 0; i < ROCKET_LIGHTS; i++)
		if (rocket_lights[i].active && rocket_lights[i].light_index == light_index)
		{
			rocket_lights[i].rasterizer_index = rasterizer_index;
			plasma_occlusion_submit(light_index, rasterizer_index);
			return;
		}
}

int rocket_lighting_selected(long rasterizer_index)
{
	int i;
	if (rasterizer_index == NONE || !lighting_config_boolean("display.plasma_lighting"))
		return FALSE;
	for (i = 0; i < ROCKET_LIGHTS; i++)
		if (rocket_lights[i].active && rocket_lights[i].rasterizer_index == rasterizer_index)
			return TRUE;
	return FALSE;
}

/* Visibility bounds must cover the owned flight glow, even when the map's
   borrowed template describes a smaller muzzle flash. */
float rocket_lighting_radius(long light_index, long object_index, float radius)
{
	int i;
	if (!lighting_config_boolean("display.plasma_lighting"))
		return radius;
	for (i = 0; i < ROCKET_LIGHTS; i++)
		if (rocket_lights[i].active && rocket_lights[i].light_index == light_index &&
			rocket_lights[i].object_index == object_index)
			return MAX(radius, 1.75f);
	return radius;
}
