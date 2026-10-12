/*
BLOOD_OBJECTS.C

Dynamic blood on objects (port/linux/game/blood.c): whether an object is a
local player's own body seen from inside it, and an impact's decals painted
onto an object's drawn surface (port/linux/game/object_mesh.c).
*/

#include "cseries.h"
#include "math/real_math.h"
#include "objects/objects.h"
#include "tag_files/tag_groups.h"

#include "blood.h"

#include <string.h>

#if BLOOD_EFFECTS

#include "game/players.h"
#include "camera/director.h"
#include "camera/camera_scripting.h"

boolean blood_object_seen_first_person(
	long object_index)
{
	short local_player;

	if (object_index == NONE)
		return FALSE;
	if (scripted_camera_object_is_first_person_camera(object_index))
		return TRUE;
	for (local_player = 0; local_player < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player++)
	{
		long player_index = local_player_get_player_index(local_player);

		if (player_index != NONE && player_get(player_index)->unit_index == object_index &&
			director_get_perspective(local_player) == _director_perspective_first_person)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/* ---------- an impact's decals onto an object (blood.c paints them) */

#include "effects/decal_definitions.h"
#include "effects/effect_definitions.h"
#include "effects/material_effects.h"
#include "effects/material_effect_definitions.h"
#include "game/game_globals.h"
#include "tag_files/tag_files.h"

enum { BLOOD_MATERIAL_EFFECT_IMPACT = 8 }; /* (particles.c's _material_effect_impact) */

/* (a tag index that is one: not NONE, nor 0, which some unset references
hold, and which the tag table refuses) */
#define BLOOD_TAG_OK(index) ((index) != NONE && (index) != 0)

short blood_effect_onto_object(
	long effect_index,
	long object_index,
	real_point3d const *point,
	real_vector3d const *normal,
	boolean without_blood)
{
	struct effect_definition *definition;
	long decals[8];
	real modifiers[8];
	short count = 0;
	short event_index;

	if (!BLOOD_TAG_OK(effect_index))
		return 0;
	definition = (struct effect_definition *)tag_get(EFFECT_DEFINITION_TAG, effect_index);
	if (!definition)
		return 0;
	/* (every event's decals: an impact's are in its first, but some wait) */
	for (event_index = 0; event_index < definition->events.count && count < 8; event_index++)
	{
		struct effect_event_definition *event = TAG_BLOCK_GET_ELEMENT(&definition->events, event_index,
			struct effect_event_definition);
		short part_index;

		for (part_index = 0; part_index < event->parts.count && count < 8; part_index++)
		{
			struct effect_part_definition *part = TAG_BLOCK_GET_ELEMENT(&event->parts, part_index,
				struct effect_part_definition);

			char const *name;

			if (part->runtime_base_class_tag != DECAL_GROUP_TAG || !BLOOD_TAG_OK(part->reference.index))
				continue;
			name = tag_get_name(part->reference.index);
			if (
				!(without_blood && name && strstr(name, "blood")))
			{
				decals[count] = part->reference.index;
				modifiers[count] = real_local_random_range(part->radius_modifier_lower_bound,
					MAX(part->radius_modifier_upper_bound, part->radius_modifier_lower_bound));
				if (!(modifiers[count] > 0.f))
					modifiers[count] = 1.f;
				count++;
			}
		}
	}
	return count > 0 ? blood_decals_onto_object(object_index, point, normal, decals, modifiers, count) : 0;
}

short blood_material_effect_onto_object(
	long material_effects_index,
	long object_index,
	real_point3d const *point,
	real_vector3d const *normal)
{
	/* (a hard surface's: the first of these the effects name) */
	static short const materials[] = { _material_stone, _material_thin_metal, _material_hollow_metal,
		_material_thick_metal, _material_wood, _material_plastic, _material_dirt, _material_sand,
		_material_glass, _material_rubber };
	struct material_effects_definition *definition;
	struct material_effect *effect;
	short index;

	if (!BLOOD_TAG_OK(material_effects_index))
		return 0;
	definition = material_effects_definition_get(material_effects_index);
	if (!definition || BLOOD_MATERIAL_EFFECT_IMPACT >= definition->effects.count)
		return 0;
	effect = TAG_BLOCK_GET_ELEMENT(&definition->effects, BLOOD_MATERIAL_EFFECT_IMPACT, struct material_effect);
	for (index = 0; index < (short)NUMBEROF(materials); index++)
	{
		if (materials[index] < effect->materials.count)
		{
			long effect_index = TAG_BLOCK_GET_ELEMENT(&effect->materials, materials[index],
				struct material_effect_material)->effect.index;

			if (BLOOD_TAG_OK(effect_index))
				return blood_effect_onto_object(effect_index, object_index, point, normal, FALSE);
		}
	}
	return 0;
}

#endif // BLOOD_EFFECTS
