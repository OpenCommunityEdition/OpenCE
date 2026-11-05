/* Cosmetic samples only: no equipment, vehicle controls or projectile state
   is written. Missing authored markers leave that source unlit. */
#include "cseries.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "tag_files/tag_files.h"
#include "ambient_emitter_lighting.h"
#include <string.h>

#include "lighting_config.h"
void render_interpolation_object_point(long object, real_point3d *point);

static const struct ambient_profile
{
	long group;
	const char *name;
	int kind;
	real radius;
	real_rgb_color color;
	const char *marker;
	int function, second_marker;
	real idle;
} ambient_profiles[] = {
	{'vehi', "vehicles\\c_dropship\\c_dropship", EMITTER_VEHICLE, 3.5f, {0.25f, 0.45f, 1.0f}, "hover thrusters", 2, 4, 0.35f},
	{'eqip', "powerups\\over shield", EMITTER_WORLD, 1.6f, {1.0f, 0.18f, 0.08f}, NULL, 0, 0, 0.0f},
	{'eqip', "powerups\\active camouflage", EMITTER_WORLD, 1.6f, {0.15f, 0.35f, 1.0f}, NULL, 0, 0, 0.0f},
	{'scen', "scenery\\teleporter_shield\\teleporter", EMITTER_WORLD, 2.4f, {0.2f, 1.0f, 0.35f}, NULL, 0, 0, 0.0f},
	{'vehi', "vehicles\\ghost\\ghost_mp", EMITTER_VEHICLE, 1.8f, {0.15f, 0.35f, 1.0f}, "running lights", 0, 1, 0.0f},
};
static long ambient_identities[NUMBEROF(ambient_profiles)], flame_definition;
static int ambient_ready;

/* One centroid per live firing owner, not a light per flame projectile.
   This bounds both allocation and occlusion work independently of fire rate. */
enum { FLAME_STREAMS = 8 };
static struct flame_stream
{
	long owner;
	int count;
	real_point3d sum;
} flames[FLAME_STREAMS];
static int flame_count;

int ambient_emitter_lighting_enabled(void)
{
	return lighting_config_boolean("display.vehicle_lighting") || lighting_config_boolean("display.world_lighting") ||
		(lighting_config_boolean("display.plasma_lighting") && (!ambient_ready || flame_definition != NONE));
}

void ambient_emitter_lighting_forget(void)
{
	ambient_ready = FALSE;
	flame_count = 0;
}

unsigned long ambient_emitter_lighting_begin(void)
{
	struct object_iterator iterator;
	struct object_datum *object;
	unsigned long mask = 0;
	int i;
	flame_count = 0;
	if (!ambient_emitter_lighting_enabled())
		return 0;
	if (!ambient_ready)
	{
		for (i = 0; i < NUMBEROF(ambient_profiles); i++)
			ambient_identities[i] = tag_loaded(ambient_profiles[i].group, ambient_profiles[i].name);
		flame_definition = tag_loaded('proj', "weapons\\flamethrower\\flame");
		ambient_ready = TRUE;
	}
	if (lighting_config_boolean("display.vehicle_lighting")) mask |= _object_mask_vehicle;
	if (lighting_config_boolean("display.world_lighting")) mask |= _object_mask_equipment | _object_mask_scenery;
	if (!lighting_config_boolean("display.plasma_lighting") || flame_definition == NONE)
		return mask;
	object_iterator_new(&iterator, _object_mask_projectile, 0);
	while ((object = object_iterator_next(&iterator)) != NULL)
	{
		real_point3d point;
		/* Model-less flame projectiles are marked invisible by the object
		   renderer; their stock particle stream is nevertheless visible. */
		if (object->definition_index != flame_definition ||
			(TEST_FLAG(object->object.flags, _object_invisible_bit) &&
			 object_definition_get(object->definition_index)->object.model.index != NONE) ||
			object->object.owner_object_index == NONE || !object_try_and_get(object->object.owner_object_index))
			continue;
		for (i = 0; i < flame_count; i++)
			if (flames[i].owner == object->object.owner_object_index)
				break;
		if (i == flame_count)
		{
			if (flame_count == FLAME_STREAMS)
				continue;
			memset(&flames[i], 0, sizeof(flames[i]));
			flames[i].owner = object->object.owner_object_index;
			flame_count++;
		}
		point = object->object.position;
		render_interpolation_object_point(iterator.index, &point);
		flames[i].sum.x += point.x;
		flames[i].sum.y += point.y;
		flames[i].sum.z += point.z;
		flames[i].count++;
	}
	if (flame_count) mask |= _object_mask_biped | _object_mask_vehicle | _object_mask_weapon;
	return mask;
}

int ambient_emitter_lighting_sample(long index, int component, struct emitter_sample *sample)
{
	struct object_datum *object = object_try_and_get(index);
	struct object_marker markers[8];
	int i, marker;
	if (!object || TEST_FLAG(object->object.flags, _object_invisible_bit))
		return FALSE;
	if (!component && lighting_config_boolean("display.plasma_lighting"))
		for (i = 0; i < flame_count; i++)
			if (flames[i].owner == index)
			{
				sample->kind = EMITTER_FLAME;
				sample->position.x = flames[i].sum.x / flames[i].count;
				sample->position.y = flames[i].sum.y / flames[i].count;
				sample->position.z = flames[i].sum.z / flames[i].count;
				sample->color.red = 1.0f;
				sample->color.green = 0.38f;
				sample->color.blue = 0.06f;
				sample->radius = 2.8f;
				sample->strength = MIN(1.0f, flames[i].count * 0.2f);
				return TRUE;
			}
	if (!ambient_ready || object->object.parent_object_index != NONE ||
		object->object.node_matrices.size < sizeof(real_matrix4x3))
		return FALSE;
	for (i = 0; i < NUMBEROF(ambient_profiles); i++)
	{
		const struct ambient_profile *profile = &ambient_profiles[i];
		if (ambient_identities[i] == NONE || object->definition_index != ambient_identities[i] ||
			!lighting_config_boolean(profile->kind == EMITTER_VEHICLE ? "display.vehicle_lighting" : "display.world_lighting"))
			continue;
		sample->kind = profile->kind;
		sample->radius = profile->radius;
		sample->color = profile->color;
		sample->strength = 0.8f;
		if (profile->kind == EMITTER_VEHICLE)
		{
			/* Two separated thruster sources, attached to the authored model.
			   The output follows the authored light/engine function. */
			marker = component ? profile->second_marker : 0;
			if (object_get_marker_by_name(index, profile->marker, markers, NUMBEROF(markers)) <= marker)
				return FALSE;
			sample->position = markers[marker].matrix.position;
			/* Dropship thruster surfaces stay luminous at idle; C-out
			   adds throttle intensity. Destroyed vehicles remain dark. */
			sample->strength *= profile->idle + (1.0f - profile->idle) *
				PIN(object->object.outgoing_function_values[profile->function], 0.0f, 1.0f);
			if (object->object.maximum_body_vitality > 0.0f && object->object.body_vitality <= 0.0f)
				sample->strength = 0.0f;
		}
		else
		{
			if (component)
				return FALSE;
			object_get_origin(index, &sample->position);
			render_interpolation_object_point(index, &sample->position);
			/* The luminous portal surface is separate from its unlit base.
			   Do not infer portals from netgame flags or illuminate every pad. */
			if (profile->group == 'scen')
				sample->position.z += 0.35f;
		}
		return TRUE;
	}
	return FALSE;
}
