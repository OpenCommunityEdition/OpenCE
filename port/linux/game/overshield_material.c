/*
OVERSHIELD_MATERIAL.C

A shot at an overshield lying in the world played what a projectile plays on
dirt (its dust and its sound), where a shot at an active camouflage plays what
it plays on an engineer's force field (flashes and energy sparks). Each
powerup's collision model has one material, named "force field", on all its
surfaces: the active camouflage's is an engineer's force field, but the
overshield's type was left dirt (in the Xbox maps the overshield's collision
model is powerups\active camoflage\active camoflage, and the active
camouflage's is powerups\over shield\over shield: each powerup's model and
collision model are named for the other). So needles also stuck in an
overshield, where they bounce off an active camouflage.

When a map's tags load (scenario_tags_load), an overshield's collision material
named "force field" whose type is dirt is made an engineer's force field, as the
active camouflage's is. A shot at it then plays what it plays at an active
camouflage and responds as it does there. The machines of a network game must
agree on how a projectile responds, so this is in a network version of its own.
*/

#include "cseries.h"
#include "cache/cache_files.h"
#include "game/game_globals.h"
#include "items/equipment_definitions.h"
#include "objects/damage_resistances.h"
#include "physics/collision_model_definitions.h"
#include "tag_files/tag_groups.h"

/* ---------- constants */

#define OVERSHIELD_MATERIAL_NAME "force field"

/* ---------- prototypes */

boolean overshield_material_fixed(struct damage_resistance_material *material);
void overshield_material_tags_loaded(void);

/* ---------- public code */

/* a powerup's force field left dirt made an engineer's force field; FALSE if
the material is not that */
boolean overshield_material_fixed(
	struct damage_resistance_material *material)
{
	if (material->material_type != _material_dirt ||
		strncmp(material->name, OVERSHIELD_MATERIAL_NAME, sizeof(material->name)) != 0)
	{
		return FALSE;
	}

	material->material_type = _material_engineer_force_field;

	return TRUE;
}

void overshield_material_tags_loaded(
	void)
{
	struct tag_iterator iterator;
	long equipment_index;

	tag_iterator_new(&iterator, EQUIPMENT_DEFINITION_TAG);
	while ((equipment_index = tag_iterator_next(&iterator)) != NONE)
	{
		struct equipment_definition const *equipment = equipment_definition_get(equipment_index);
		struct collision_model *collision_model;
		long material_index;

		if (equipment->equipment.powerup_type != _equipment_powerup_overshield ||
			equipment->object.collision_model.index == NONE)
		{
			continue;
		}

		collision_model = collision_model_definition_get(equipment->object.collision_model.index);
		if (!collision_model->resistance.materials.address)
		{
			continue;
		}
		for (material_index = 0; material_index < collision_model->resistance.materials.count; material_index++)
		{
			overshield_material_fixed(TAG_BLOCK_GET_ELEMENT(&collision_model->resistance.materials, material_index,
				struct damage_resistance_material));
		}
	}

	return;
}
