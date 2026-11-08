/*
 * OpenCE : Forge
 * Interactive In-Engine AI Placement System (source/ai/ integration).
 * Spawns native Halo: Combat Evolved NPC actors & bipeds (Marines, Elites,
 * Grunts, Jackals, Hunters, Flood, Sentinels) assigned to Friendly or Enemy
 * combat teams.
 */

#ifndef HALO_RELEASE
#define HALO_RELEASE
#endif
#include "cseries.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math/real_math.h"
#include "game/game.h"
#include "game/game_allegiance.h"
#include "tag_files/tag_groups.h"
#include "tag_files/tag_files.h"
#include "objects/objects.h"
#include "units/units.h"
#include "ai/ai.h"
#include "ai/actors.h"
#include "ai/actor_definitions.h"
#include "ai/actions.h"

#include "forge_engine.h"

static short map_forge_team_to_engine_team(int16_t forge_team_designator)
{
	switch (forge_team_designator)
	{
	case _forge_ai_team_friendly:
		return _game_team_player; /* Friendly to Spartan players (Human/Player/Blue) */
	case _forge_ai_team_enemy_flood:
		return _game_team_flood;
	case _forge_ai_team_enemy_sentinel:
		return _game_team_sentinel;
	case _forge_ai_team_enemy_covenant:
	default:
		return _game_team_covenant; /* Hostile Enemy Covenant / Red */
	}
}

int32_t forge_ai_spawn_unit_live(
	char const *actor_variant_path,
	float x, float y, float z, float yaw,
	int16_t team_designator,
	int32_t *out_unit_index)
{
	long actv_idx = NONE;
	long unit_def_idx = NONE;
	long unit_index = NONE;
	long actor_index = NONE;
	short engine_team = map_forge_team_to_engine_team(team_designator);
	struct object_placement_data placement;

	if (out_unit_index)
		*out_unit_index = NONE;
	if (!actor_variant_path || !actor_variant_path[0])
		return NONE;

	/* Look up the actor_variant ('actv') tag in the active map */
	actv_idx = forge_resolve_engine_tag('actv', actor_variant_path, NULL);
	if (actv_idx != NONE)
	{
		struct actor_variant_definition *actv_def =
			(struct actor_variant_definition *)tag_get('actv', actv_idx);
		if (actv_def && actv_def->unit_reference.index != NONE)
		{
			unit_def_idx = actv_def->unit_reference.index;
		}
	}

	/* Fallback to resolving a biped ('bipd') directly if the current map has not yet baked this 'actv' */
	if (unit_def_idx == NONE)
	{
		unit_def_idx = forge_resolve_engine_tag('bipd', actor_variant_path, NULL);
	}
	if (unit_def_idx == NONE)
		return NONE;

	object_placement_data_new(&placement, unit_def_idx, NONE);
	placement.position.x = x;
	placement.position.y = y;
	placement.position.z = z;
	placement.forward.i = (real)cos((double)yaw);
	placement.forward.j = (real)sin((double)yaw);
	placement.forward.k = 0.0f;
	placement.up.i = 0.0f;
	placement.up.j = 0.0f;
	placement.up.k = 1.0f;
	placement.owner_team_index = engine_team;

	/* Tint friendly vs enemy change-colors so team allegiance is visually clear */
	if (team_designator == _forge_ai_team_friendly)
	{
		placement.change_colors[0].red = 0.20f;
		placement.change_colors[0].green = 0.55f;
		placement.change_colors[0].blue = 0.95f;
	}
	else
	{
		placement.change_colors[0].red = 0.90f;
		placement.change_colors[0].green = 0.22f;
		placement.change_colors[0].blue = 0.18f;
	}

	unit_index = object_new(&placement);
	if (unit_index == NONE)
		return NONE;

	if (out_unit_index)
		*out_unit_index = (int32_t)unit_index;

	if (actv_idx != NONE)
	{
		/* Equip the unit with the actor variant's weapon, grenades, shields & vitality */
		actor_customize_unit(actv_idx, unit_index);

		/* Create and attach the live Halo CE AI actor brain */
		actor_index = actor_create_for_unit(
			FALSE,
			unit_index,
			actv_idx,
			NONE,
			NONE,
			FALSE,
			NONE,
			FALSE,
			_actor_action_alert,
			_actor_action_alert,
			NONE,
			NONE);

		if (actor_index != NONE)
		{
			actor_set_team(actor_index, engine_team);
			actor_set_dormant(actor_index, FALSE);
			actor_braindead(actor_index, FALSE);
		}
	}

	return (int32_t)actor_index;
}
