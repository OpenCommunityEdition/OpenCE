/* Infection goals from thelinkin3000/halo-ce-universal 18a94774,
using its inherited BirchWoodGod bot tactics (GPL-3.0-only). Included by
enhanced_bots.c. The role snapshot is shared, bounded and refreshed once
a second, rather than rescanning every player for every candidate place. */
static struct
{
	long next_scan, count;
	struct { long player; boolean zombie; real_point3d point; } players[128];
} bots_infection;

static boolean bots_infection_goal(struct bots_bot *bot, struct player_datum *player, real_point3d const *position)
{
	long now = game_time_get(), index, humans = 0;
	boolean zombie = infection_unit_is_zombie(player->unit_index);
	real_point3d center = { 0.0f, 0.0f, 0.0f }, target;
	real nearest = REAL_MAX, best_score = REAL_MAX;
	boolean found = FALSE;
	if (now >= bots_infection.next_scan)
	{
		struct data_iterator iterator;
		struct player_datum *other;
		bots_infection.count = 0;
		bots_infection.next_scan = now + TICKS_PER_SECOND;
		data_iterator_new(&iterator, player_data);
		while ((other = (struct player_datum *)data_iterator_next(&iterator)) != NULL &&
			bots_infection.count < (long)NUMBEROF(bots_infection.players))
		{
			struct object_datum *unit = object_try_and_get(other->unit_index);
			if (!unit || TEST_FLAG(unit->object.damage_flags, _object_dead_bit)) continue;
			index = bots_infection.count++;
			bots_infection.players[index].player = iterator.datum_index;
			bots_infection.players[index].zombie = infection_unit_is_zombie(other->unit_index);
			bots_infection.players[index].point = unit->object.position;
		}
	}
	for (index = 0; index < bots_infection.count; index++)
	{
		real distance;
		real_point3d const *point = &bots_infection.players[index].point;
		if (bots_infection.players[index].player == bot->player_index || bots_infection.players[index].zombie) continue;
		center.x += point->x; center.y += point->y; center.z += point->z;
		humans++;
		distance = bots_distance3d(position, point);
		if (distance < nearest) { nearest = distance; target = *point; found = TRUE; }
	}
	if (zombie) return found && bots_ctf_set_goal(bot, &target, 3.0f);
	if (bot->goal_kind == _bots_goal_objective && now < bot->guard_until_tick) return TRUE;
	found = FALSE;
	if (humans) { center.x /= humans; center.y /= humans; center.z /= humans; }
	for (index = 0; index < bots_globals.roam_point_count; index++)
	{
		real_point3d const *point = &bots_globals.roam_points[index];
		real score;
		if (humans) score = bots_distance3d(point, &center);
		else
		{
			long other;
			real danger = REAL_MAX;
			for (other = 0; other < bots_infection.count; other++)
				if (bots_infection.players[other].zombie)
					danger = MIN(danger, bots_distance3d(point, &bots_infection.players[other].point));
			score = -danger;
		}
		score += bots_random_real(bot) * 4.0f;
		if (score < best_score) { best_score = score; target = *point; found = TRUE; }
	}
	if (!found || !bots_ctf_set_goal(bot, &target, 1.0f)) return FALSE;
	bot->guard_until_tick = now + TICKS_PER_SECOND * (8 + bots_random(bot) % 6);
	return TRUE;
}
