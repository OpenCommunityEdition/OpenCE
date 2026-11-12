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
#include "units/unit_definitions.h"
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

	if (engine_team == _game_team_player)
	{
		extern boolean game_team_is_enemy(short team1_index, short team2_index);
		extern void game_allegiance_create(
			short team1_index, boolean team1_suspicious,
			short team2_index, boolean team2_suspicious,
			short incident_threshold, short suspicious_threshold,
			boolean locked);
		if (game_team_is_enemy(_game_team_player, _game_team_human))
			game_allegiance_create(_game_team_player, FALSE, _game_team_human, FALSE, NONE, NONE, FALSE);
		if (game_team_is_enemy(_game_team_player, _game_team_default))
			game_allegiance_create(_game_team_player, FALSE, _game_team_default, FALSE, NONE, NONE, FALSE);
	}

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

void forge_ai_update_unit_team(
	long actor_index,
	long unit_index,
	int16_t team_designator)
{
	short engine_team = map_forge_team_to_engine_team(team_designator);

	if (engine_team == _game_team_player)
	{
		extern boolean game_team_is_enemy(short team1_index, short team2_index);
		extern void game_allegiance_create(
			short team1_index, boolean team1_suspicious,
			short team2_index, boolean team2_suspicious,
			short incident_threshold, short suspicious_threshold,
			boolean locked);
		if (game_team_is_enemy(_game_team_player, _game_team_human))
			game_allegiance_create(_game_team_player, FALSE, _game_team_human, FALSE, NONE, NONE, FALSE);
		if (game_team_is_enemy(_game_team_player, _game_team_default))
			game_allegiance_create(_game_team_player, FALSE, _game_team_default, FALSE, NONE, NONE, FALSE);
	}

	if (unit_index != NONE)
	{
		struct unit_datum *u = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
		if (u)
		{
			u->object.owner_team_index = engine_team;
			if (team_designator == _forge_ai_team_friendly)
			{
				u->object.base_change_colors[0].red = 0.20f;
				u->object.base_change_colors[0].green = 0.55f;
				u->object.base_change_colors[0].blue = 0.95f;
			}
			else
			{
				u->object.base_change_colors[0].red = 0.90f;
				u->object.base_change_colors[0].green = 0.22f;
				u->object.base_change_colors[0].blue = 0.18f;
			}
			u->object.outgoing_change_colors[0] = u->object.base_change_colors[0];
		}
	}

	if (actor_index != NONE && actor_data && datum_try_and_get(actor_data, actor_index) != NULL)
	{
		actor_set_team(actor_index, engine_team);
	}
}

void forge_ai_apply_order(
	long actor_index,
	long unit_index,
	uint8_t ai_order,
	float home_x,
	float home_y,
	float home_z,
	long cur_tick)
{
	struct actor_datum *actor = NULL;
	struct unit_datum *unit = NULL;

	if (unit_index == NONE)
		return;
	unit = (struct unit_datum *)object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
	if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;

	if (actor_index != NONE && actor_data)
		actor = (struct actor_datum *)datum_try_and_get(actor_data, actor_index);
	if (!actor)
		return;

	if (ai_order == _forge_ai_order_default)
	{
		actor->emotions.currently_defending = FALSE;
		actor->emotions.forced_to_charge = FALSE;
		return;
	}

	if (ai_order == _forge_ai_order_guard)
	{
		float dx = unit->object.position.x - home_x;
		float dy = unit->object.position.y - home_y;
		float dz = unit->object.position.z - home_z;
		float dist_sq = dx * dx + dy * dy + dz * dz;

		actor->emotions.currently_defending = TRUE;
		actor->emotions.forced_to_charge = FALSE;

		if (unit->object.parent_object_index == NONE && dist_sq > (4.0f * 4.0f))
		{
			real_point3d home_pt;
			home_pt.x = home_x;
			home_pt.y = home_y;
			home_pt.z = home_z + 0.05f;
			actor_move_to_point(actor_index, &home_pt, NONE, NONE);
		}
		return;
	}

	if (ai_order == _forge_ai_order_patrol)
	{
		actor->emotions.currently_defending = FALSE;
		actor->emotions.forced_to_charge = FALSE;

		if (unit->object.parent_object_index == NONE &&
			!actor->control.current_fire_target_visible &&
			!actor_path_has_path(actor_index))
		{
			int wp = (int)(((unsigned long)(cur_tick / 90L) + (unsigned long)(actor_index & 0xFFFF)) & 3u);
			float angle = (float)wp * 1.5707963f;
			real_point3d pt;

			pt.x = home_x + cosine(angle) * 3.5f;
			pt.y = home_y + sine(angle) * 3.5f;
			pt.z = home_z + 0.05f;
			actor_move_to_point(actor_index, &pt, NONE, NONE);
		}
		return;
	}

	if (ai_order == _forge_ai_order_charge)
	{
		actor->emotions.currently_defending = FALSE;
		actor->emotions.forced_to_charge = TRUE;

		if (unit->object.parent_object_index == NONE &&
			!actor->control.current_fire_target_visible &&
			!actor_path_has_path(actor_index))
		{
			struct data_iterator iter;
			struct object_header_datum *hdr;
			long best_enemy_idx = NONE;
			float best_dist_sq = 60.0f * 60.0f;

			if (object_header_data)
			{
				data_iterator_new(&iter, object_header_data);
				while ((hdr = (struct object_header_datum *)data_iterator_next(&iter)) != NULL)
				{
					struct unit_datum *cand;
					float dx, dy, dz, d2;

					if (hdr->datum == NULL || iter.datum_index == unit_index)
						continue;
					cand = (struct unit_datum *)object_try_and_get_and_verify_type(iter.datum_index, _object_mask_unit);
					if (!cand || TEST_FLAG(cand->object.damage_flags, _object_dead_bit))
						continue;
					if (forge_is_unit_in_monitor_mode(iter.datum_index))
						continue;
					if (!game_team_is_enemy(unit->object.owner_team_index, cand->object.owner_team_index))
						continue;

					dx = cand->object.position.x - unit->object.position.x;
					dy = cand->object.position.y - unit->object.position.y;
					dz = cand->object.position.z - unit->object.position.z;
					d2 = dx * dx + dy * dy + dz * dz;
					if (d2 < best_dist_sq)
					{
						best_dist_sq = d2;
						best_enemy_idx = iter.datum_index;
					}
				}
			}

			if (best_enemy_idx != NONE)
			{
				struct object_datum *enemy_obj = object_get(best_enemy_idx);
				real_point3d target_pt = enemy_obj->object.position;
				actor_move_to_point(actor_index, &target_pt, NONE, best_enemy_idx);
			}
		}
		return;
	}

	if (ai_order == _forge_ai_order_vehicle)
	{
		struct actor_definition *adef = NULL;

		actor->emotions.currently_defending = FALSE;
		actor->emotions.forced_to_charge = FALSE;

		if (actor->meta.definition_index != NONE)
		{
			adef = (struct actor_definition *)tag_get('actr', actor->meta.definition_index);
			if (adef)
			{
				adef->flags &= ~(1u << 3);
				if (unit->object.owner_team_index == _game_team_player ||
					unit->object.owner_team_index == _game_team_human)
				{
					adef->flags |= (1u << _actor_definition_helps_players_in_vehicles_bit);
				}
			}
		}

		if (unit->object.parent_object_index == NONE && object_header_data)
		{
			struct data_iterator iter;
			struct object_header_datum *hdr;
			long best_veh_idx = NONE;
			short best_seat_idx = NONE;
			int best_seat_rank = -1;
			float best_dist_sq = 14.0f * 14.0f;

			data_iterator_new(&iter, object_header_data);
			while ((hdr = (struct object_header_datum *)data_iterator_next(&iter)) != NULL)
			{
				struct unit_datum *veh;
				struct unit_definition *vdef;
				float dx, dy, dz, d2;
				short s;

				if (hdr->datum == NULL || iter.datum_index == unit_index)
					continue;
				veh = (struct unit_datum *)object_try_and_get_and_verify_type(iter.datum_index, _object_mask_vehicle);
				if (!veh || TEST_FLAG(veh->object.damage_flags, _object_dead_bit))
					continue;

				dx = veh->object.position.x - unit->object.position.x;
				dy = veh->object.position.y - unit->object.position.y;
				dz = veh->object.position.z - unit->object.position.z;
				d2 = dx * dx + dy * dy + dz * dz;
				if (d2 > best_dist_sq)
					continue;

				vdef = (struct unit_definition *)tag_get('unit', veh->definition_index);
				if (!vdef)
					vdef = (struct unit_definition *)tag_get('vehi', veh->definition_index);
				if (!vdef || vdef->unit.seats.count <= 0)
					continue;

				for (s = 0; s < vdef->unit.seats.count; s++)
				{
					struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(&vdef->unit.seats, s, struct unit_seat);
					int rank = 1;

					if (!seat)
						continue;
					if (!unit_can_enter_seat(unit_index, iter.datum_index, s, NULL))
						continue;
					if (!unit_has_animation_to_enter_seat(unit_index, iter.datum_index, s))
						continue;

					if (TEST_FLAG(seat->flags, _unit_seat_driver_bit))
						rank = 3;
					else if (TEST_FLAG(seat->flags, _unit_seat_gunner_bit))
						rank = 2;

					if (best_veh_idx == NONE || d2 + 0.5f < best_dist_sq ||
						(d2 <= best_dist_sq + 1.0f && rank > best_seat_rank))
					{
						best_veh_idx = iter.datum_index;
						best_seat_idx = s;
						best_seat_rank = rank;
						best_dist_sq = d2;
					}
				}
			}

			if (best_veh_idx != NONE && best_seat_idx != NONE)
			{
				unit_enter_seat(unit_index, best_veh_idx, best_seat_idx);
			}
		}
	}
}
