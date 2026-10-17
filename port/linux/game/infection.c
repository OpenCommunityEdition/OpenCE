#include "cseries/cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_engine_slayer.h"
#include "game/players.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "physics/collisions.h"
#include "items/weapons.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_server_manager.h"
#include "text/unicode.h"
#include "tag_files/tag_files.h"
#include "math/real_math_declarations.h"
#include "infection.h"
#include "network_infection.h"

struct distributed_infection_state infection_state;
static struct game_engine infection_engine;
static long participant[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static long equipped_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static long killed_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static long skull[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static long shield_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static real shield_maximum[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static boolean equipping;
static boolean missing_skull;
void platform_log(char const *format, ...);
long network_test_infection_initial_player(void);

boolean infection_enabled(void)
{
	struct game_variant const *variant = game_engine_get_variant();
	return game_engine_running() && variant && variant->game_engine_index == game_engine_slayer &&
		variant->universal_variant.teams && game_variant_options_get()->infection_enabled;
}

static struct distributed_infection_player *infection_player(long index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	if (index == NONE || !VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS) || !player_try_and_get(index))
		return NULL;
	if (infection_state.players[slot].identity != infection_player_identity(index))
		return NULL;
	return &infection_state.players[slot];
}

boolean infection_can_delete_skull(long index)
{
	return infection_enabled() && object_get(index)->definition_index == get_ball_definition_index();
}

boolean infection_unit_is_zombie(long index)
{
	struct unit_datum *unit;
	struct distributed_infection_player *role;
	if (!infection_enabled() || index == NONE || !(unit = unit_try_and_get(index)))
		return FALSE;
	role = infection_player(unit->unit.player_index);
	return role && role->role == 2;
}

/* Only voluntary player actions call this. Death/destruction and received
   authoritative inventories must still be able to remove an objective. */
boolean infection_allow_weapon_drop(long index, long weapon)
{
	struct unit_datum *unit;
	struct player_datum *player;
	struct object_datum *object;
	if (!infection_enabled() || infection_state.phase != _distributed_infection_active || equipping ||
		!infection_unit_is_zombie(index)) return TRUE;
	unit = unit_get(index);
	player = player_try_and_get(unit->unit.player_index);
	if (!player || player->quit_out_of_game || player->unit_index != index ||
		TEST_FLAG(unit->object.damage_flags, _object_dead_bit)) return TRUE;
	object = object_try_and_get_and_verify_type(weapon, _object_mask_weapon);
	return !object || object->definition_index != get_ball_definition_index();
}

real infection_melee_multiplier(long index)
{
	struct unit_datum *unit;
	long weapon;
	if (!infection_unit_is_zombie(index) || infection_state.phase != _distributed_infection_active)
		return 1.0f;
	unit = unit_get(index);
	if (!VALID_INDEX(unit->unit.current_weapon_index, MAXIMUM_WEAPONS_PER_UNIT))
		return 1.0f;
	weapon = unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
	return weapon != NONE && object_get(weapon)->definition_index == get_ball_definition_index() ?
		game_variant_options_get()->infection_damage_percent / 100.0f : 1.0f;
}

/* Preserve a directly struck exposed rider, never redirect a hull hit to
   occupants. Normal Slayer keeps the original parent-object selection. */
boolean infection_melee_rider_target(long attacker_index, long target_index)
{
	struct unit_datum *attacker, *target, *vehicle;
	struct distributed_infection_player *role;
	struct object_datum *weapon;
	struct unit_definition *definition;
	short slot, seat;
	if (!infection_unit_is_zombie(attacker_index) || infection_state.phase != _distributed_infection_active)
		return FALSE;
	attacker = unit_get(attacker_index);
	slot = attacker->unit.current_weapon_index;
	if (!VALID_INDEX(slot, MAXIMUM_WEAPONS_PER_UNIT)) return FALSE;
	weapon = object_try_and_get_and_verify_type(attacker->unit.weapon_object_indices[slot], _object_mask_weapon);
	if (!weapon || weapon->definition_index != get_ball_definition_index()) return FALSE;
	target = unit_try_and_get(target_index);
	if (!target || target->object.type != _object_type_biped || TEST_FLAG(target->object.damage_flags, _object_dead_bit))
		return FALSE;
	role = infection_player(target->unit.player_index);
	if (!role || role->role != 1) return FALSE;
	vehicle = (struct unit_datum *)object_try_and_get_and_verify_type(target->object.parent_object_index, _object_mask_vehicle);
	if (!vehicle) return FALSE;
	definition = unit_definition_get(vehicle->definition_index);
	seat = target->unit.parent_seat_index;
	return VALID_INDEX(seat, definition->unit.seats.count) &&
		!TEST_FLAG(TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, seat, struct unit_seat)->flags, _unit_seat_invisible_bit);
}

/* A new remote rider hit must also pass the native 0.8-unit melee cone on
   the host. Use the host's actual head and current geometry, not a reported
   origin that could be placed through the hull. Moving targets may reject a
   late report conservatively; there is no geometry/seat protection bypass. */
boolean infection_melee_rider_visible(long attacker_index, long target_index, real_vector3d const *direction)
{
	struct object_marker head;
	real_vector3d facing = *direction, perpendicular, cross, ray;
	struct collision_result hit;
	long outer, inner;
	if (!infection_melee_rider_target(attacker_index, target_index) || normalize3d(&facing) <= 0.0f) return FALSE;
	object_get_marker_by_name(attacker_index, "head", &head, 1);
	normalize3d(perpendicular3d(&facing, &perpendicular));
	cross_product3d(&facing, &perpendicular, &cross);
	for (outer = -2; outer <= 2; outer++)
		for (inner = -2; inner <= 2; inner++)
		{
			ray.i = facing.i * .8f + (cross.i * inner + perpendicular.i * outer) * .1f;
			ray.j = facing.j * .8f + (cross.j * inner + perpendicular.j * outer) * .1f;
			ray.k = facing.k * .8f + (cross.k * inner + perpendicular.k * outer) * .1f;
			if (collision_test_vector(_collision_test_for_projectiles_flags, &head.matrix.position, &ray, attacker_index, &hit) &&
				hit.type == _collision_result_object && hit.object_index == target_index) return TRUE;
		}
	return FALSE;
}

static void infection_finish(long winner)
{
	if (infection_state.phase == _distributed_infection_finished)
		return;
	infection_state.winner = winner;
	infection_state.phase = _distributed_infection_finished;
	infection_state.revision++;
	platform_log("infection: finished winner=%ld", winner);
	game_engine_end_game();
}

/* Match the four placement colors used by player spawn. Spartan armour is
   not necessarily channel A; channels driven by functions belong to effects
   such as shield glow, whose computed output must remain untouched. */
static void infection_color_unit(struct unit_datum *unit, long index)
{
	struct object_definition *definition = object_definition_get(unit->definition_index);
	real_rgb_color color;
	short slot;
	game_engine_player_get_change_color(&color, index);
	for (slot = 0; slot < NUMBER_OF_OBJECT_CHANGE_COLORS; slot++)
	{
		unit->object.base_change_colors[slot] = color;
		if (slot < definition->object.change_colors.count)
		{
			struct object_change_color_definition *channel = TAG_BLOCK_GET_ELEMENT(
				&definition->object.change_colors, slot, struct object_change_color_definition);
			if (channel->scaled_by || channel->darken_by)
				continue;
		}
		unit->object.outgoing_change_colors[slot] = color;
	}
}

/* A unit's zero shield capacity is the engine's existing no-shields path:
   both damage and recharge honor it. Keep its own original capacity so a
   role/option restoration never resets body health or instantly heals it. */
static void infection_apply_shields(long slot, long unit_index, struct unit_datum *unit, boolean disabled)
{
	if (shield_unit[slot] != unit_index)
		shield_unit[slot] = NONE;
	if (disabled)
	{
		if (shield_unit[slot] == NONE)
		{
			shield_unit[slot] = unit_index;
			shield_maximum[slot] = unit->object.maximum_shield_vitality;
		}
		unit->object.maximum_shield_vitality = 0.0f;
		unit->object.shield_vitality = 0.0f;
		SET_FLAG(unit->object.damage_flags, _object_shield_charging_bit, FALSE);
		SET_FLAG(unit->object.damage_flags, _object_shield_over_charging_bit, FALSE);
		SET_FLAG(unit->object.damage_flags, _object_shield_depleted_bit, TRUE);
	}
	else if (shield_unit[slot] == unit_index)
	{
		unit->object.maximum_shield_vitality = shield_maximum[slot];
		shield_unit[slot] = NONE;
	}
}

void infection_reconcile_player(long index)
{
	struct distributed_infection_player *role;
	struct player_datum *player;
	struct unit_datum *unit;
	long team;
	if (!infection_enabled() || !(role = infection_player(index)) || !role->role)
		return;
	player = player_get(index);
	team = role->role - 1;
	player->team_index = team;
	player->network_player_data.team_index = (char)team;
	player->speed_multiplier = team ? game_variant_options_get()->infection_speed_percent / 100.0f : 1.0f;
	if (player->unit_index != NONE && (unit = unit_try_and_get(player->unit_index)))
	{
		infection_apply_shields(DATUM_INDEX_TO_ABSOLUTE_INDEX(index), player->unit_index, unit,
			team && !game_variant_options_get()->infection_zombie_shields);
		unit->object.owner_team_index = (short)team;
		/* The alpha changes sides alive; update its armour as well as its team. */
		infection_color_unit(unit, index);
		/* port: lethal damage changes role before unit_died drops the corpse's
		   resources. Only a living zombie's inventory may be cleared here. */
		if (team && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		{
			/* An alpha can change sides after a throw starts but before its
			   grenade reaches the hand. Cancel that wind-up before removing
			   ammo; otherwise unit_throw_grenade_move_to_hand asserts on zero.
			   Already-spent grenades still use the normal release/cleanup. */
			if (unit->unit.grenade_throw_state == _unit_grenade_throw_wind_up)
				unit->unit.grenade_throw_state = _unit_grenade_throw_ending;
			csmemset(unit->unit.grenade_counts, 0, sizeof(unit->unit.grenade_counts));
			csmemset(player->powerup_durations, 0, sizeof(player->powerup_durations));
		}
	}
	if (!network_game_distributed_client())
	{
		struct network_game_server *server = global_network_game_server_get();
		struct network_game *game = server ? network_game_server_get_game(server) : NULL;
		long list = player->network_player_data.player_list_index;
		if (game && VALID_INDEX(list, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
			game->players[list].team_index = (char)team;
	}
}

static void infection_delete_weapon(long weapon)
{
	long slot;
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
		if (skull[slot] == weapon) skull[slot] = NONE;
	if (weapon != NONE && object_try_and_get(weapon)) object_delete(weapon);
}

static void infection_delete_skull(long slot)
{
	infection_delete_weapon(skull[slot]);
	skull[slot] = NONE;
}

/* Remove incompatible inventory entries once, even if damaged state aliases
   an object in two slots. Retain the issued skull rather than respawning it. */
static void infection_strip_weapons(long index, long keep, boolean skulls_only)
{
	struct unit_datum *unit = unit_get(index);
	long removed[MAXIMUM_WEAPONS_PER_UNIT], count = 0, slot, previous, kept = NONE;
	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		long weapon = unit->unit.weapon_object_indices[slot];
		struct object_datum *object = object_try_and_get_and_verify_type(weapon, _object_mask_weapon);
		if (weapon == NONE) continue;
		if (weapon == keep && kept == NONE) { kept = slot; continue; }
		if (skulls_only && (!object || object->definition_index != get_ball_definition_index())) continue;
		unit->unit.weapon_object_indices[slot] = NONE;
		if (unit->unit.current_weapon_index == slot) unit->unit.current_weapon_index = NONE;
		if (unit->unit.desired_weapon_index == slot) unit->unit.desired_weapon_index = NONE;
		if (weapon == keep) continue;
		for (previous = 0; previous < count; previous++) if (removed[previous] == weapon) break;
		if (previous == count) removed[count++] = weapon;
	}
	for (slot = 0; slot < count; slot++) infection_delete_weapon(removed[slot]);
	if (kept != NONE) unit->unit.desired_weapon_index = (short)kept;
	else if (unit->unit.desired_weapon_index == NONE)
		unit->unit.desired_weapon_index = unit_inventory_next_weapon(index, NONE, 0);
}

static void infection_equip(long index)
{
	struct player_datum *player = player_get(index);
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	struct distributed_infection_player *role = infection_player(index);
	struct unit_datum *unit;
	struct object_placement_data placement;
	long weapon, inventory_slot;
	if (network_game_distributed_client() || !role || player->quit_out_of_game || player->unit_index == NONE || missing_skull)
		return;
	unit = unit_get(player->unit_index);
	if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;
	if (equipped_unit[slot] == player->unit_index)
	{
		if (role->role == 1)
		{
			infection_strip_weapons(player->unit_index, NONE, TRUE);
			return;
		}
		for (inventory_slot = 0; inventory_slot < MAXIMUM_WEAPONS_PER_UNIT; inventory_slot++)
		{
			struct object_datum *object;
			weapon = unit->unit.weapon_object_indices[inventory_slot];
			object = object_try_and_get_and_verify_type(weapon, _object_mask_weapon);
			if (object && object->definition_index == get_ball_definition_index())
			{
				if (skull[slot] != weapon) infection_delete_skull(slot);
				skull[slot] = weapon;
				infection_strip_weapons(player->unit_index, weapon, FALSE);
				return;
			}
		}
	}
	equipping = TRUE;
	/* Humans keep player_spawn's native equipment. The normal postspawn
	   callback replaces it only when the map/variant specifies a loadout;
	   it is not a generic weapon refill. Only zombies lose every weapon. */
	infection_strip_weapons(player->unit_index, NONE, role->role == 1);
	infection_delete_skull(slot);
	if (role->role == 2)
	{
		if (unit->object.parent_object_index != NONE)
			unit_exit_seat_end(player->unit_index);
		csmemset(unit->unit.grenade_counts, 0, sizeof(unit->unit.grenade_counts));
		if (unit->unit.equipment_object_index != NONE)
		{
			object_delete(unit->unit.equipment_object_index);
			unit->unit.equipment_object_index = NONE;
		}
		if (get_ball_definition_index() == NONE)
			weapon = NONE;
		else
		{
			object_placement_data_new(&placement, get_ball_definition_index(), NONE);
			placement.owner_team_index = 1;
			weapon = object_new(&placement);
		}
		if (weapon == NONE || !unit_add_weapon_to_inventory(player->unit_index, weapon, 2))
		{
			if (weapon != NONE)
				object_delete(weapon);
			missing_skull = TRUE;
		}
		else
			skull[slot] = weapon;
	}
	else
	{
		game_engine_default_postspawn_player_update(index);
	}
	equipping = FALSE;
	equipped_unit[slot] = player->unit_index;
	killed_unit[slot] = NONE;
}

static void infection_added(long index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	struct distributed_infection_player *role = &infection_state.players[slot];
	if (!network_game_distributed_client())
	{
		infection_delete_skull(slot);
		csmemset(role, 0, sizeof(*role));
		role->identity = infection_player_identity(index);
		role->role = infection_state.phase >= _distributed_infection_active ? 2 : 1;
		platform_log("infection: joined slot=%ld role=%d phase=%ld", slot, role->role, infection_state.phase);
		participant[slot] = index;
		equipped_unit[slot] = killed_unit[slot] = NONE;
		infection_state.revision++;
	}
	infection_reconcile_player(index);
}

static void infection_spawn(long index)
{
	if (!network_game_distributed_client() && participant[DATUM_INDEX_TO_ABSOLUTE_INDEX(index)] != index)
		infection_added(index);
	infection_reconcile_player(index);
	infection_equip(index);
}

static void infection_killed(long killer, long object, long victim, boolean friendly)
{
	struct distributed_infection_player *dead = infection_player(victim);
	struct distributed_infection_player *attacker = infection_player(killer);
	struct player_datum *player = player_get(victim);
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(victim);
	long unit = player->unit_index != NONE ? player->unit_index : player->dead_unit_index;
	if (network_game_distributed_client() || infection_state.phase != _distributed_infection_active ||
		!dead || player->quit_out_of_game || killed_unit[slot] != NONE)
		return;
	killed_unit[slot] = unit == NONE ? 0 : unit;
	if (attacker && attacker != dead && attacker->role != dead->role && attacker->score < 1000000)
		attacker->score++;
	/* A teammate's betrayal is not an infection; suicides and world deaths
	   still convert, as do enemy kills. The host's death attribution owns this. */
	if (dead->role == 1 && !(friendly && attacker && attacker != dead && attacker->role == dead->role))
	{
		dead->role = 2;
		platform_log("infection: infected slot=%ld", slot);
	}
	infection_reconcile_player(victim);
	/* The player tick cleans the corpse's skull after unit death completes. */
	equipped_unit[slot] = NONE;
	infection_state.revision++;
}

/* Pure decision used by the host and the asset-free tests. Infection has
   priority when the last death and the clock expiration share a tick. */
long infection_round_result(long humans, long zombies, long elapsed, long limit)
{
	if (!humans && !zombies) return 2;
	if (!humans) return 1;
	if (!zombies || (limit > 0 && elapsed >= limit)) return 0;
	return NONE;
}

static void infection_update(void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	boolean present[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long humans = 0, zombies = 0, index, result, candidates[HALO_PORT_MAXIMUM_NETWORK_PLAYERS], count = 0;
	long now = game_time_get();
	if (network_game_distributed_client() || infection_state.phase == _distributed_infection_finished)
		return;
	csmemset(present, 0, sizeof(present));
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		struct unit_datum *unit;
		if (player->quit_out_of_game) continue;
		present[slot] = TRUE;
		if (participant[slot] != iterator.datum_index) infection_added(iterator.datum_index);
		infection_reconcile_player(iterator.datum_index);
		if (infection_state.players[slot].role == 1) humans++; else zombies++;
		/* A full map can leave connected players waiting for a spawn. The
		   initial zombie must have a living unit, with a live human left.
		   Active round totals above still include everyone connected. */
		unit = player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
		if (unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			candidates[count++] = iterator.datum_index;
	}
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
		if (!present[index] && participant[index] != NONE)
		{
			infection_delete_skull(index);
			participant[index] = equipped_unit[index] = killed_unit[index] = NONE;
			csmemset(&infection_state.players[index], 0, sizeof(infection_state.players[index]));
			infection_state.revision++;
		}
	if (missing_skull)
	{
		platform_log("infection: cancelled, skull unavailable or inventory allocation failed");
		infection_finish(NONE);
		return;
	}
	if (infection_state.phase == _distributed_infection_active)
	{
		result = infection_round_result(humans, zombies, now - infection_state.phase_tick,
			game_variant_options_get()->time_limit * 60L * TICKS_PER_SECOND);
		if (result != NONE)
		{
			platform_log("infection: result=%ld humans=%ld zombies=%ld elapsed=%ld limit=%ld", result, humans, zombies,
				now - infection_state.phase_tick, game_variant_options_get()->time_limit * 60L * TICKS_PER_SECOND);
			infection_finish(result == 2 ? NONE : result);
		}
		return;
	}
	if (count < 2)
	{
		if (infection_state.phase != _distributed_infection_waiting) infection_state.revision++;
		infection_state.phase = _distributed_infection_waiting;
		infection_state.phase_tick = now;
		return;
	}
	if (infection_state.phase == _distributed_infection_waiting)
	{
		infection_state.phase = _distributed_infection_countdown;
		infection_state.phase_tick = now;
		infection_state.revision++;
		platform_log("infection: countdown players=%ld", count);
	}
	if (now - infection_state.phase_tick < game_variant_options_get()->infection_countdown_seconds * TICKS_PER_SECOND)
		return;
	infection_state.phase = _distributed_infection_active;
	infection_state.phase_tick = now;
	/* Shuffle without replacement, leaving at least one human. */
	for (index = 0; index < MIN(count - 1, game_variant_options_get()->infection_initial_zombies); index++)
	{
		long pick = index + (long)(real_random_range(0.0f, 0.99999f) * (count - index));
		long fixture = network_test_infection_initial_player(), candidate;
		long chosen, slot;
		if (index == 0 && fixture != NONE)
			for (candidate = index; candidate < count; candidate++) if (candidates[candidate] == fixture) { pick = candidate; break; }
		chosen = candidates[pick];
		slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(chosen);
		candidates[pick] = candidates[index];
		candidates[index] = chosen;
		infection_state.players[slot].role = 2;
		equipped_unit[slot] = NONE;
		infection_reconcile_player(chosen);
		infection_equip(chosen);
	}
	infection_state.revision++;
	platform_log("infection: active players=%ld zombies=%ld spawned=%ld", humans + zombies,
		MIN(count - 1, game_variant_options_get()->infection_initial_zombies), count);
}

long infection_remaining_ticks(void)
{
	long limit = game_variant_options_get()->time_limit * 60L * TICKS_PER_SECOND;
	return infection_state.phase == _distributed_infection_active ?
		MAX(0, limit - (game_time_get() - infection_state.phase_tick)) : limit;
}

void infection_log_state(void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	long humans = 0, zombies = 0, armed = 0;
	if (!infection_enabled()) return;
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)))
	{
		struct distributed_infection_player *role = infection_player(iterator.datum_index);
		if (!role || player->quit_out_of_game) continue;
		if (role->role == 1) humans++;
		if (role->role == 2)
		{
			zombies++;
			if (player->unit_index != NONE && unit_has_weapon_definition_index(player->unit_index, get_ball_definition_index())) armed++;
		}
	}
	platform_log("infection state: phase=%ld humans=%ld zombies=%ld skulls=%ld winner=%ld revision=%ld",
		infection_state.phase, humans, zombies, armed, infection_state.winner, infection_state.revision);
}

static boolean infection_initialize(void)
{
	csmemset(&infection_state, 0, sizeof(infection_state));
	csmemset(participant, 0xFF, sizeof(participant));
	csmemset(equipped_unit, 0xFF, sizeof(equipped_unit));
	csmemset(killed_unit, 0xFF, sizeof(killed_unit));
	csmemset(skull, 0xFF, sizeof(skull));
	csmemset(shield_unit, 0xFF, sizeof(shield_unit));
	infection_state.winner = NONE;
	infection_state.game_number = network_game_get_game() ? network_game_get_number_of_games_played() : 0;
	missing_skull = get_ball_definition_index() == NONE;
	equipping = FALSE;
	platform_log("infection options: initial=%d speed=%d damage=%d respawn=%d countdown=%d minutes=%d shields=%d",
		game_variant_options_get()->infection_initial_zombies, game_variant_options_get()->infection_speed_percent,
		game_variant_options_get()->infection_damage_percent, game_variant_options_get()->infection_respawn_seconds,
		game_variant_options_get()->infection_countdown_seconds, game_variant_options_get()->time_limit,
		game_variant_options_get()->infection_zombie_shields);
	return TRUE;
}

static boolean infection_pickup(long unit, long weapon)
{
	short slot;
	if (equipping) return TRUE;
	if (weapon != NONE && object_get(weapon)->definition_index == get_ball_definition_index())
	{
		/* unit_can_use_weapon also asks this while readying a held weapon.
		   Permit our issued skull, but never another skull on the ground. */
		if (infection_unit_is_zombie(unit))
			for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
				if (unit_get(unit)->unit.weapon_object_indices[slot] == weapon) return TRUE;
		return FALSE;
	}
	return !infection_unit_is_zombie(unit);
}

static boolean infection_objective_pickup(long weapon, long player)
{
	return equipping;
}

static void infection_drop(long weapon)
{
	long slot;
	if (network_game_distributed_client() || equipping) return;
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
		if (skull[slot] == weapon)
		{
			/* Repair on the next player tick; ground skulls cannot be picked up. */
			equipped_unit[slot] = NONE;
			break;
		}
}

static void infection_tick_player(long index)
{
	struct player_datum *player = player_get(index);
	infection_reconcile_player(index);
	if (!network_game_distributed_client())
	{
		/* Deferred past the death callback, so unit death can finish detaching
		   its inventory. Do not leave a corpse's skull until the next spawn. */
		if (player->quit_out_of_game || player->unit_index == NONE ||
			TEST_FLAG(unit_get(player->unit_index)->object.damage_flags, _object_dead_bit))
			infection_delete_skull(DATUM_INDEX_TO_ABSOLUTE_INDEX(index));
		else infection_equip(index);
	}
}

static long infection_score(long index, enum get_score_type type)
{
	struct distributed_infection_player *role = infection_player(index);
	long slot, score = 0;
	if (!role) return 0;
	if (type != _get_score_team) return role->score;
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
		if (infection_state.players[slot].role == role->role) score += infection_state.players[slot].score;
	return score;
}

static wchar_t *infection_score_string(long index, wchar_t *text)
{
	usprintf(text, L"%ld", infection_score(index, _get_score_individual));
	return text;
}

static wchar_t *infection_team_string(long team, wchar_t *text)
{
	/* This callback formats the team's score, despite its historical name. */
	usprintf(text, L"%ld", game_engine_get_team_score(team));
	return text;
}

static long infection_won(long index)
{
	struct distributed_infection_player *role = infection_player(index);
	return infection_state.winner == NONE || !role ? NONE : role->role - 1 == infection_state.winner;
}

struct game_engine *infection_engine_get(void)
{
	/* Optional Team Slayer rules: retain its identity and native presentation.
	   Numeric score adapters read the host's existing Infection counters. */
	infection_engine = slayer_engine;
	infection_engine.initialize_for_new_map = infection_initialize;
	infection_engine.player_added = infection_added;
	infection_engine.player_update_each_tick = infection_tick_player;
	infection_engine.prespawn_player_update = infection_reconcile_player;
	infection_engine.player_update = infection_spawn;
	infection_engine.update = infection_update;
	infection_engine.allow_pick_up = infection_pickup;
	infection_engine.picking_up = infection_objective_pickup;
	infection_engine.weapon_dropped = infection_drop;
	infection_engine.player_killed_player = infection_killed;
	infection_engine.get_player_score = infection_score;
	infection_engine.format_player_score = infection_score_string;
	infection_engine.format_team_name = infection_team_string;
	infection_engine.format_message = NULL;
	infection_engine.did_player_win = infection_won;
	return &infection_engine;
}
