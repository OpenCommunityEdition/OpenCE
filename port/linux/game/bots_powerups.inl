/* Powerup decisions adapted from BirchWoodGod/halo-ce-vita via
thelinkin3000/halo-ce-universal 18a94774 (GPL-3.0-only). Included by bots.c
to reuse its item ownership, unreachable-item memory and path lifecycle. */
static struct
{
	long next_scan, count, items[32];
} bots_powerups;

static void bots_powerups_reset(void)
{
	csmemset(&bots_powerups, 0, sizeof(bots_powerups));
}

static boolean bots_powerup_claimed(struct bots_bot *bot, long item_index, real distance)
{
	long other, index = (long)(bot - bots_globals.bots);
	for (other = 0; other < BOTS_MAXIMUM; other++)
	{
		struct bots_bot *claim = &bots_globals.bots[other];
		if (other != index && claim->unit_index != NONE && claim->goal_kind == _bots_goal_item &&
			claim->powerup_index == item_index && (claim->powerup_distance < distance ||
			(claim->powerup_distance == distance && other < index))) return TRUE;
	}
	return FALSE;
}

static boolean bots_powerup_goal(struct bots_bot *bot, struct unit_datum *unit,
	real_point3d const *position, boolean fighting)
{
	long now = game_time_get(), index, best = NONE;
	real best_score = REAL_MAX;
	long bot_index = (long)(bot - bots_globals.bots);
	if (infection_enabled() && infection_unit_is_zombie(bot->unit_index)) return FALSE;
	if (bot->powerup_index != NONE)
	{
		struct object_datum *item = object_try_and_get_and_verify_type(bot->powerup_index, _object_mask_equipment);
		boolean still_goal = bot->goal_kind == _bots_goal_item && bot->item_index == bot->powerup_index;
		real distance = item ? bots_distance3d(&item->object.position, position) : REAL_MAX;
		boolean healthy = item && equipment_definition_get(item->definition_index)->equipment.powerup_type ==
			_equipment_powerup_health && unit->object.body_vitality >= 1.0f;
		if (still_goal && bots_powerup_claimed(bot, bot->powerup_index, distance))
		{
			bot->powerup_index = NONE;
			bot->goal_kind = _bots_goal_none;
			bot->powerup_check = now + TICKS_PER_SECOND;
			return FALSE;
		}
		if (still_goal && item && item->object.parent_object_index == NONE && !healthy)
		{
			bot->powerup_distance = distance;
			if (distance < 0.8f && !bot->powerup_arrived) bot->powerup_arrived = now;
			if (now < bot->powerup_deadline && (!bot->powerup_arrived || now - bot->powerup_arrived < 2 * TICKS_PER_SECOND))
				return TRUE;
		}
		if (item && item->object.parent_object_index == NONE && !healthy &&
			(distance > 1.5f || still_goal)) bots_item_set_unreachable(bot, bot->powerup_index);
		bot->powerup_index = NONE;
		if (still_goal) bot->goal_kind = _bots_goal_none;
	}
	if (now < bot->powerup_check) return FALSE;
	bot->powerup_check = now + TICKS_PER_SECOND;
	if (now >= bots_powerups.next_scan)
	{
		struct object_iterator iterator;
		struct object_datum *item;
		bots_powerups.next_scan = now + TICKS_PER_SECOND;
		bots_powerups.count = 0;
		object_iterator_new(&iterator, _object_mask_equipment, 0);
		while ((item = object_iterator_next(&iterator)) != NULL && bots_powerups.count < (long)NUMBEROF(bots_powerups.items))
			if (item->object.parent_object_index == NONE && TEST_FLAG(item->object.flags, _object_connected_to_map_bit))
				bots_powerups.items[bots_powerups.count++] = iterator.index;
	}
	for (index = 0; index < bots_powerups.count; index++)
	{
		long item_index = bots_powerups.items[index];
		struct object_datum *item = object_try_and_get_and_verify_type(item_index, _object_mask_equipment);
		real distance, score;
		if (!item || item->object.parent_object_index != NONE || bots_item_unreachable(bot, item_index)) continue;
		distance = bots_distance3d(position, &item->object.position);
		score = enhanced_bots_powerup_score(unit, item->definition_index, distance, bots_globals.skill,
			fighting, bots_tactics_bots[bot_index].hold_tick != 0);
		if (score >= best_score) continue;
		if (bots_powerup_claimed(bot, item_index, distance)) continue;
		best = item_index;
		best_score = score;
	}
	if (best != NONE)
	{
		real_point3d ground;
		long surface = bots_surface_below(&object_get(best)->object.position, &ground);
		if (surface == NONE) { bots_item_set_unreachable(bot, best); return FALSE; }
		bots_set_goal(bot, _bots_goal_item, &ground, surface);
		bot->item_index = bot->powerup_index = best;
		bot->powerup_distance = bots_distance3d(position, &ground);
		bot->powerup_arrived = 0;
		bot->powerup_deadline = now + 10 * TICKS_PER_SECOND + (long)(bot->powerup_distance * 0.75f * TICKS_PER_SECOND);
		return TRUE;
	}
	return FALSE;
}
