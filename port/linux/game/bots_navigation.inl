/* Enhanced navigation adapted from BirchWoodGod/halo-ce-vita, via
thelinkin3000/halo-ce-universal 18a94774a73ca275e9c0ab4f281e492ad8bb656e.
Derived navigation retains GPL-3.0-only; see the provenance guide.

Included by bots.c to share its bounded path search and geometry readers.
Links belong to this map, reservations to a bot's life. Only ordinary player
controls traverse them; neither the level's graph nor teleport rules change. */
#define BOTS_NAV_LINKS 64
enum { _bots_nav_none, _bots_nav_exit, _bots_nav_entry, _bots_nav_going };
struct bots_nav_link
{
	real_point3d entrance, exit;
	long entrance_surface, exit_surface;
	boolean ladder;
	real climb_direction, top;
	long failures, successes, rest_until;
};
static struct
{
	long count;
	struct bots_nav_link links[BOTS_NAV_LINKS];
	byte *vehicle_walkable;
	struct structure_bsp vehicle_structure;
	long obstacle_tick;
	struct bots_nav_route
	{
		short phase, link;
		real_point3d goal;
		real walk, entrance_cost, exit_cost;
		long next_weigh, deadline, on_link, climb_tick, top_tick;
	} routes[BOTS_MAXIMUM];
} bots_navigation;

static void bots_navigation_reset(void)
{
	if (bots_navigation.vehicle_walkable) free(bots_navigation.vehicle_walkable - 1);
	csmemset(&bots_navigation, 0, sizeof(bots_navigation));
}

static void bots_navigation_close_surface(long surface)
{
	if (bots_navigation.vehicle_walkable && surface >= 0 &&
		surface < bots_navigation.vehicle_structure.pathfinding_surfaces.count)
		bots_navigation.vehicle_walkable[surface] &= (byte)~FLAG(BOTS_PATHFINDING_WALKABLE_BIT);
}

static struct structure_bsp *bots_navigation_vehicle_structure(void)
{
	long count;
	if (!bots_paths_copy()) return NULL;
	if (!bots_navigation.vehicle_walkable)
	{
		count = bots_paths.structure.pathfinding_surfaces.count;
		bots_navigation.vehicle_walkable = (byte *)malloc((size_t)count + 1);
		if (!bots_navigation.vehicle_walkable) return NULL;
		/* path.c reads surface NONE at open edges. Supply a closed sentinel. */
		*bots_navigation.vehicle_walkable++ = 0;
		csmemcpy(bots_navigation.vehicle_walkable, bots_paths.walkable, (size_t)count);
		bots_navigation.vehicle_structure = bots_paths.structure;
		bots_navigation.vehicle_structure.pathfinding_surfaces.address = bots_navigation.vehicle_walkable;
	}
	return &bots_navigation.vehicle_structure;
}

void bots_vehicle_obstacle(long vehicle_index, real_point3d const *goal)
{
	struct object_datum *vehicle = object_try_and_get_and_verify_type(vehicle_index, _object_mask_vehicle);
	struct collision_result hit;
	real_vector3d vector;
	real_point3d start;
	real reach;
	long now = game_time_get();
	unsigned long flags = FLAG(_collision_test_structure_bit) | FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_objects_bit) | FLAG(_collision_test_objects_scenery_bit) | FLAG(_collision_test_objects_machines_bit);
	if (!vehicle || !bots_enhanced_running || bots_navigation.obstacle_tick == now) return;
	bots_navigation.obstacle_tick = now;
	reach = MAX(2.5f, vehicle->object.bounding_sphere_radius + 0.6f);
	vector.i = vehicle->object.forward.i * reach;
	vector.j = vehicle->object.forward.j * reach;
	vector.k = vehicle->object.forward.k * reach;
	start = vehicle->object.position;
	start.z += 0.25f;
	if (!collision_test_vector(flags, &vehicle->object.bounding_sphere_center, &vector, vehicle_index, &hit) &&
		!collision_test_vector(flags, &start, &vector, vehicle_index, &hit)) return;
	if (hit.type == _collision_result_object)
		bots_paths_learn(&vehicle->object.position, goal);
	else if (hit.type == _collision_result_structure && bots_navigation_vehicle_structure())
	{
		struct collision_bsp const *bsp = TAG_BLOCK_GET_ELEMENT(&bots_paths.structure.collision_bsp, 0, struct collision_bsp);
		long surface, closed = 0;
		real about = MAX(1.5f, vehicle->object.bounding_sphere_radius);
		for (surface = 0; surface < bsp->surfaces.count; surface++)
		{
			real_point3d corners[BOTS_SURFACE_CORNERS], middle;
			real radius;
			long count;
			if (!bots_paths_walkable_surface(bsp, surface, corners, &count, &middle, &radius) ||
				!TEST_FLAG(bots_navigation.vehicle_walkable[surface], BOTS_PATHFINDING_WALKABLE_BIT)) continue;
			if (bots_distance2d(&middle, &hit.point) < about && fabs(middle.z - hit.point.z) < 2.5f &&
				bots_distance2d(&middle, &vehicle->object.position) > 1.2f && bots_distance2d(&middle, goal) > 3.0f)
			{
				bots_navigation_close_surface(surface);
				closed++;
			}
		}
		if (closed) platform_log("bots: learned %ld blocked surfaces for vehicles", closed);
	}
}

static void bots_navigation_reset_bot(long index)
{
	csmemset(&bots_navigation.routes[index], 0, sizeof(bots_navigation.routes[index]));
}

static boolean bots_navigation_climbing(struct bots_bot *bot)
{
	struct bots_nav_route *route = &bots_navigation.routes[bot - bots_globals.bots];
	return route->phase == _bots_nav_going && route->climb_tick != 0;
}

static boolean bots_navigation_pays(real walk, real through)
{
	return through <= walk * 0.8f && walk - through >= 15.0f;
}

static real bots_navigation_cost(struct path_state *state, long surface, real_point3d const *point)
{
	short node = surface == NONE ? NONE : path_node_from_hash_table(state, surface);
	if (node < 0 || node >= state->node_count) return -1.0f;
	return state->node_list[node].path_distance_from_origin +
		bots_distance3d(&state->node_list[node].entry_point, point);
}

static real bots_navigation_remaining(struct path_state *state)
{
	long node = state->closest_node_index;
	if (node < 0 || node >= state->node_count) return -1.0f;
	return state->node_list[node].path_distance_from_origin + state->closest_distance;
}

static void bots_navigation_drop(struct bots_bot *bot, boolean failed)
{
	struct bots_nav_route *route = &bots_navigation.routes[bot - bots_globals.bots];
	if (route->phase == _bots_nav_none) return;
	if (failed)
	{
		struct bots_nav_link *link = &bots_navigation.links[route->link];
		link->failures++;
		link->rest_until = game_time_get() + 30 * TICKS_PER_SECOND;
		platform_log("bots: navigation link %d failed; resting for 30 seconds", route->link + 1);
	}
	route->phase = _bots_nav_none;
	route->next_weigh = game_time_get() + 5 * TICKS_PER_SECOND;
	bot->path.valid = FALSE;
	bot->path_tick = game_time_get() - TICKS_PER_SECOND;
}

static long bots_navigation_claimers(long link)
{
	long index, count = 0;
	for (index = 0; index < BOTS_MAXIMUM; index++)
		if (bots_navigation.routes[index].phase != _bots_nav_none && bots_navigation.routes[index].link == link)
			count++;
	return count;
}

/* Keep a goal's normal path while measuring the exit. Every measurement
uses the same per-tick budget as a normal search, including failed ones. */
static boolean bots_navigation_query(struct bots_bot *bot, real_point3d *start, long *start_surface,
	real_point3d *goal, long *goal_surface)
{
	struct bots_nav_route *route = &bots_navigation.routes[bot - bots_globals.bots];
	struct bots_nav_link *link;
	if (route->phase == _bots_nav_none) return FALSE;
	if (bots_distance3d(&route->goal, &bot->goal_point) > 8.0f)
	{
		bots_navigation_drop(bot, FALSE);
		return FALSE;
	}
	link = &bots_navigation.links[route->link];
	if (route->phase == _bots_nav_exit)
	{
		*start = link->exit;
		*start_surface = link->exit_surface;
	}
	else
	{
		*goal = link->entrance;
		*goal_surface = link->entrance_surface;
	}
	return TRUE;
}

/* TRUE consumes a measuring answer without replacing the bot's current
path. An accepted entrance answer becomes its path through the usual code. */
static boolean bots_navigation_answer(struct bots_bot *bot, struct path_state *state,
	real_point3d const *start, boolean reached, boolean waypoint, real radius)
{
	long bot_index = (long)(bot - bots_globals.bots), now = game_time_get();
	struct bots_nav_route *route = &bots_navigation.routes[bot_index];
	struct bots_tactics_bot const *tactics = &bots_tactics_bots[bot_index];
	long index, best = NONE;
	real walk, best_cost = REAL_MAX, best_entrance = 0.0f;
	boolean exhausted = state->node_count < PATH_NODE_LIST_SIZE && state->heap_count <= 1;
	if (route->phase == _bots_nav_exit)
	{
		real beyond = bots_navigation_cost(state, bot->goal_surface_index, &bot->goal_point);
		if (beyond < 0.0f && !exhausted) beyond = bots_navigation_remaining(state);
		if (beyond < 0.0f || !bots_navigation_pays(route->walk, route->entrance_cost + beyond))
			bots_navigation_drop(bot, FALSE);
		else
		{
			route->exit_cost = beyond;
			route->phase = _bots_nav_entry;
		}
		bot->path_tick = now - 4 * TICKS_PER_SECOND - 1;
		return TRUE;
	}
	if (route->phase == _bots_nav_entry || route->phase == _bots_nav_going)
	{
		struct bots_nav_link *link = &bots_navigation.links[route->link];
		real entrance = bots_navigation_cost(state, link->entrance_surface, &link->entrance);
		if ((!reached && exhausted) || (reached && route->walk >= 0.0f &&
			!bots_navigation_pays(route->walk, entrance + route->exit_cost)))
		{
			bots_navigation_drop(bot, !reached);
			return TRUE;
		}
		if (route->phase == _bots_nav_entry)
		{
			route->phase = _bots_nav_going;
			route->deadline = now + (long)((8.0f + MAX(entrance, route->entrance_cost) / 1.5f) * TICKS_PER_SECOND);
			platform_log("bots: bot %ld taking %s %d", bot_index + 1, link->ladder ? "ladder" : "teleporter", route->link + 1);
		}
		return FALSE;
	}
	if (waypoint || radius > BOTS_PATH_RADIUS || now < route->next_weigh ||
		bot->goal_kind == _bots_goal_item || bot->goal_kind == _bots_goal_vehicle) return FALSE;
	route->next_weigh = now + 5 * TICKS_PER_SECOND;
	walk = bots_navigation_cost(state, bot->goal_surface_index, &bot->goal_point);
	if (walk < 0.0f && !exhausted) walk = bots_navigation_remaining(state);
	for (index = 0; index < bots_navigation.count; index++)
	{
		struct bots_nav_link *link = &bots_navigation.links[index];
		real entrance, beyond, through;
		if (now < link->rest_until || (link->failures >= 3 && !link->successes) ||
			bots_navigation_claimers(index) >= 2 || bots_distance3d(start, &link->exit) < 2.0f) continue;
		entrance = bots_navigation_cost(state, link->entrance_surface, &link->entrance);
		beyond = bots_distance3d(&link->exit, &bot->goal_point);
		if (walk < 0.0f)
		{
			/* A disconnected floor: take a reachable link that makes progress,
			then search again on the other side (including chains of links). */
			if (entrance < 0.0f || beyond >= state->closest_distance - 2.0f) continue;
		}
		else if (link->ladder || tactics->role == _bots_role_defender ||
			(tactics->lane && !tactics->lane_given_up)) continue;
		if (entrance < 0.0f) entrance = bots_distance3d(start, &link->entrance);
		through = entrance + beyond;
		if (walk >= 0.0f && !bots_navigation_pays(walk, through)) continue;
		if (through < best_cost) { best = index; best_cost = through; best_entrance = entrance; }
	}
	if (best != NONE)
	{
		route->link = (short)best;
		route->goal = bot->goal_point;
		route->walk = walk;
		route->entrance_cost = best_entrance;
		route->phase = walk >= 0.0f ? _bots_nav_exit : _bots_nav_entry;
		route->deadline = now + 10 * TICKS_PER_SECOND;
		route->on_link = route->climb_tick = route->top_tick = 0;
	}
	return FALSE;
}

static void bots_navigation_update(struct bots_bot *bot, real_point3d const *position, boolean riding)
{
	struct bots_nav_route *route = &bots_navigation.routes[bot - bots_globals.bots];
	struct bots_nav_link *link;
	long now = game_time_get();
	if (route->phase == _bots_nav_none) return;
	link = &bots_navigation.links[route->link];
	/* Fighting can hold a bot off its route. Charge only travel time to
	the approach deadline; climbing itself still has a bounded timeout. */
	if (!route->climb_tick && bot->target_player_index != NONE &&
		now - bot->target_seen_tick <= TICKS_PER_SECOND / 2)
	{
		route->deadline++;
		if (route->on_link) route->on_link++;
	}
	if (!link->ladder && bots_distance3d(position, &bot->last_position) > 3.0f &&
		bots_distance3d(position, &link->exit) < 4.0f)
	{
		link->successes++;
		platform_log("bots: bot %ld completed teleporter %d", (long)(bot - bots_globals.bots) + 1, route->link + 1);
		bots_navigation_drop(bot, FALSE);
		return;
	}
	if (riding || bot->goal_kind == _bots_goal_none || bot->goal_kind == _bots_goal_item ||
		bot->goal_kind == _bots_goal_vehicle || bots_distance3d(&route->goal, &bot->goal_point) > 8.0f)
		bots_navigation_drop(bot, FALSE);
	else if (now > route->deadline)
		bots_navigation_drop(bot, TRUE);
}

/* At a link, finish the approach without asking the floor graph to cross
the gap. A ladder uses the biped's ordinary facing-and-forward climbing. */
static boolean bots_navigation_follow(struct bots_bot *bot, real_point3d const *position, real *direction)
{
	struct bots_nav_route *route = &bots_navigation.routes[bot - bots_globals.bots];
	struct bots_nav_link *link;
	long now = game_time_get();
	real_point3d const *target;
	if (route->phase != _bots_nav_going) return FALSE;
	link = &bots_navigation.links[route->link];
	if (bots_distance3d(position, &link->entrance) > 1.0f && !route->climb_tick) return FALSE;
	bot->path.valid = FALSE;
	bot->climb_until_tick = 0;
	if (!route->on_link) route->on_link = now;
	target = &link->entrance;
	if (link->ladder)
	{
		/* Stock bipeds climbed about 0.48 units/second in Boarding Action.
		Allow three seconds per unit, with bounded approach and landing slack. */
		long climb_ticks = (long)(MIN(60.0f, MAX(8.0f, 3.0f * (link->top - link->entrance.z))) * TICKS_PER_SECOND);
		if (!route->climb_tick && bots_distance2d(position, target) < 0.35f)
		{
			route->climb_tick = now;
			route->deadline = now + climb_ticks + 2 * TICKS_PER_SECOND;
		}
		if (route->climb_tick)
		{
			if (position->z < link->top - 0.25f && !route->top_tick)
			{
				if (now - route->climb_tick > climb_ticks) bots_navigation_drop(bot, TRUE);
				*direction = link->climb_direction;
				return route->phase != _bots_nav_none;
			}
			if (!route->top_tick) route->top_tick = now;
			target = &link->exit;
			if (bots_distance2d(position, target) < 0.4f)
			{
				link->successes++;
				platform_log("bots: bot %ld completed ladder %d", (long)(bot - bots_globals.bots) + 1, route->link + 1);
				bots_navigation_drop(bot, FALSE);
				return FALSE;
			}
			if (now - route->top_tick > 2 * TICKS_PER_SECOND)
			{
				bots_navigation_drop(bot, TRUE);
				return FALSE;
			}
		}
	}
	else if (now - route->on_link > 2 * TICKS_PER_SECOND)
	{
		bots_navigation_drop(bot, TRUE);
		return FALSE;
	}
	*direction = (real)atan2(target->y - position->y, target->x - position->x);
	return TRUE;
}

static void bots_navigation_action(struct bots_bot *bot, struct player_action *action)
{
	struct bots_nav_route *route = &bots_navigation.routes[bot - bots_globals.bots];
	if (route->phase == _bots_nav_going && route->climb_tick && !route->top_tick)
	{
		action->desired_facing.yaw = bots_navigation.links[route->link].climb_direction;
		action->desired_facing.pitch = 0.0f;
		action->throttle.i = 1.0f;
		action->throttle.j = 0.0f;
		action->primary_trigger = 0.0f;
		action->control_flags &= ~FLAG(_unit_control_weapon_primary_trigger_bit);
		if ((game_time_get() - route->climb_tick) % 15 == 0)
			action->control_flags |= FLAG(_unit_control_jump_bit);
	}
}

static void bots_navigation_ladders(struct collision_bsp const *bsp);

static void bots_navigation_prepare(void)
{
	struct scenario *scenario = global_scenario_get();
	struct structure_bsp *structure = global_structure_bsp_get();
	long source_index;
	bots_navigation_reset();
	if (!scenario || !structure) return;
	for (source_index = 0; source_index < scenario->netgame_flags.count && bots_navigation.count < BOTS_NAV_LINKS; source_index++)
	{
		struct scenario_netgame_flag *source = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, source_index, struct scenario_netgame_flag);
		long target_index;
		if (source->type != _netgame_flag_teleporter_source) continue;
		for (target_index = 0; target_index < scenario->netgame_flags.count; target_index++)
		{
			struct scenario_netgame_flag *target = TAG_BLOCK_GET_ELEMENT(&scenario->netgame_flags, target_index, struct scenario_netgame_flag);
			struct bots_nav_link *link = &bots_navigation.links[bots_navigation.count];
			if (target->type != _netgame_flag_teleporter_target || target->team_index != source->team_index) continue;
			link->entrance_surface = bots_surface_below(&source->position, &link->entrance);
			link->exit_surface = bots_surface_below(&target->position, &link->exit);
			if (link->entrance_surface != NONE && link->exit_surface != NONE) bots_navigation.count++;
			/* Match game_engine_update_teleporter: the first exit on a channel wins. */
			break;
		}
	}
	if (structure->collision_bsp.count > 0)
		bots_navigation_ladders(TAG_BLOCK_GET_ELEMENT(&structure->collision_bsp, 0, struct collision_bsp));
	platform_log("bots: enhanced navigation prepared %ld teleporter/ladder links", bots_navigation.count);
}

#define BOTS_MAXIMUM_LADDER_PIECES 128
static void bots_navigation_ladders(struct collision_bsp const *bsp)
{
	struct
	{
		real_point3d middle;
		real_vector3d normal;
		real low, high;
	} pieces[BOTS_MAXIMUM_LADDER_PIECES];
	long piece_count = 0, surface_index, index;

	for (surface_index = 0; surface_index < bsp->surfaces.count; surface_index++)
	{
		real_point3d corners[BOTS_SURFACE_CORNERS], middle = { 0.0f, 0.0f, 0.0f };
		real_vector3d a, b, normal;
		real low = REAL_MAX, high = -REAL_MAX, length;
		long corner, count;

		if (!TEST_FLAG(TAG_BLOCK_GET_ELEMENT(&bsp->surfaces, surface_index, struct collision_surface)->flags,
			2))
		{
			continue;
		}
		count = bots_surface_corners(bsp, surface_index, corners);
		if (count < 3)
			continue;
		for (corner = 0; corner < count; corner++)
		{
			middle.x += corners[corner].x / (real)count;
			middle.y += corners[corner].y / (real)count;
			middle.z += corners[corner].z / (real)count;
			low = MIN(low, corners[corner].z);
			high = MAX(high, corners[corner].z);
		}
		vector_from_points3d(&corners[0], &corners[1], &a);
		vector_from_points3d(&corners[0], &corners[2], &b);
		cross_product3d(&a, &b, &normal);
		normal.k = 0.0f;
		length = magnitude3d(&normal);
		/* (upright, and a metre high at least) */
		if (length < 0.0001f || high - low < 0.3f)
			continue;
		normal.i /= length;
		normal.j /= length;
		/* (a piece over or under one, or its other face: the one ladder) */
		for (index = 0; index < piece_count; index++)
		{
			if (bots_distance2d(&pieces[index].middle, &middle) < 0.5f &&
				fabs(pieces[index].normal.i * normal.i + pieces[index].normal.j * normal.j) > 0.9f &&
				low <= pieces[index].high + 0.1f && high >= pieces[index].low - 0.1f)
			{
				pieces[index].low = MIN(pieces[index].low, low);
				pieces[index].high = MAX(pieces[index].high, high);
				break;
			}
		}
		if (index < piece_count || piece_count >= BOTS_MAXIMUM_LADDER_PIECES)
			continue;
		pieces[piece_count].middle = middle;
		pieces[piece_count].normal = normal;
		pieces[piece_count].low = low;
		pieces[piece_count].high = high;
		piece_count++;
	}
	for (index = 0; index < piece_count && bots_navigation.count < BOTS_NAV_LINKS; index++)
	{
		long teleporter = bots_navigation.count;
		real_point3d foot = { 0.0f, 0.0f, 0.0f }, landing = { 0.0f, 0.0f, 0.0f }, ground;
		long foot_surface = NONE, landing_surface = NONE;
		real side, foot_side = 0.0f;

		for (side = -1.0f; side <= 1.0f; side += 2.0f)
		{
			real_point3d point;
			long surface;

			point.x = pieces[index].middle.x + pieces[index].normal.i * side * 0.4f;
			point.y = pieces[index].middle.y + pieces[index].normal.j * side * 0.4f;
			point.z = pieces[index].low + 0.3f;
			surface = bots_surface_below(&point, &ground);
			if (foot_surface == NONE && surface != NONE && fabs(ground.z - pieces[index].low) < 0.35f)
			{
				foot_surface = surface;
				foot = ground;
				foot_side = side;
			}
		}
		/* (the landing beyond the top first, as a ladder up a wall's face
		goes over it; else on the same side, a ladder up to a ledge) */
		for (side = -1.0f; side <= 1.0f && foot_surface != NONE; side += 2.0f)
		{
			real_point3d point;
			long surface;
			real way = side * foot_side;

			point.x = pieces[index].middle.x + pieces[index].normal.i * way * 0.6f;
			point.y = pieces[index].middle.y + pieces[index].normal.j * way * 0.6f;
			point.z = pieces[index].high + 0.5f;
			surface = bots_surface_below(&point, &ground);
			if (landing_surface == NONE && surface != NONE && fabs(ground.z - pieces[index].high) < 0.5f)
			{
				landing_surface = surface;
				landing = ground;
			}
		}
		if (bots_trace_enabled())
		{
			platform_log("bots: trace ladder at (%.1f %.1f) z %.1f to %.1f normal (%.2f %.2f): foot %s (%.1f %.1f %.1f), "
				"landing %s (%.1f %.1f %.1f)", pieces[index].middle.x, pieces[index].middle.y, pieces[index].low,
				pieces[index].high, pieces[index].normal.i, pieces[index].normal.j, foot_surface != NONE ? "yes" : "no",
				foot.x, foot.y, foot.z, landing_surface != NONE ? "yes" : "no", landing.x, landing.y, landing.z);
		}
		if (foot_surface == NONE || landing_surface == NONE)
			continue;
		/* Ladders bridge disconnected floors; they are not fast-travel shortcuts. */
		csmemset(&bots_navigation.links[teleporter], 0, sizeof(bots_navigation.links[teleporter]));
		bots_navigation.links[teleporter].exit_surface = landing_surface;
		bots_navigation.links[teleporter].entrance = foot;
		bots_navigation.links[teleporter].entrance_surface = foot_surface;
		bots_navigation.links[teleporter].exit = landing;
		bots_navigation.links[teleporter].ladder = TRUE;
		/* (facing it from its foot) */
		bots_navigation.links[teleporter].climb_direction =
			(real)atan2(-pieces[index].normal.j * foot_side, -pieces[index].normal.i * foot_side);
		bots_navigation.links[teleporter].top = pieces[index].high;
		bots_navigation.links[teleporter].successes = 0;
		bots_navigation.links[teleporter].failures = 0;
		bots_navigation.count++;
	}
}
