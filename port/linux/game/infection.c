/*
INFECTION.C

Infection, a mode of team Slayer (game_modes.c). Humans are red and keep the
gametype's equipment; zombies are blue, carry the Oddball skull and nothing
else, and fight on foot, faster than humans, their skull's melee stronger.
The round waits for two living players, counts down, and then the host
chooses the first zombies (leaving at least one human). A human killed by a
zombie, by themselves or by the world becomes a zombie (one killed by a
teammate does not); a zombie respawns a zombie, and a player who joins late
joins the zombies. The zombies win when no human is left, the humans when the
round's time runs out. Each player scores a kill of the other side.

Only the host decides: it chooses the zombies, turns the dead, gives the
skulls and ends the round, and sends every client its state
(struct distributed_infection_state, with the game type's:
game_engine_write_network_state), which a client checks whole before taking.
Each machine then sets each player's team, speed and shields by their role.

Its options are the gametype's (game_engine.h's struct game_variant_options,
infection_*). The rules and much of the code follow the Infection of pull
request #420 by arisson2001rojas-design.
*/

#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_server_manager.h"
#include "objects/objects.h"
#include "objects/object_definitions.h"
#include "physics/collisions.h"
#include "text/unicode.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "game_modes.h"
#include "infection.h"
#include "network_infection.h"

void platform_log(char const *format, ...);

/* a score past this is not a client's to take */
#define INFECTION_MAXIMUM_SCORE 1000000

static struct
{
	struct game_engine engine;
	struct game_engine slayer;
	struct distributed_infection_state state;
	/* the host's, by absolute player index: the player of the slot, the unit
	given its role's equipment, the unit counted dead, the skull given, and
	the unit whose shields a zombie's role took (and their maximum) */
	long participant[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long equipped_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long killed_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long skull[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long shield_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	real shield_maximum[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	/* a skull being given (the pickup checks allow it) */
	boolean equipping;
	/* the map has no skull, or one could not be given: the round ends */
	boolean missing_skull;
} infection;

/* ---- the players' roles */

boolean infection_enabled(
	void)
{
	return game_modes_mode() == _game_mode_infection;
}

/* a player's identity on every machine: their name and place in the game */
static unsigned long infection_player_identity(
	long player_index)
{
	struct network_player const *network_player = &player_get(player_index)->network_player_data;
	unsigned long hash = 2166136261UL;
	short index;

	for (index = 0; index < NUMBEROF(network_player->name); index++)
		hash = (hash ^ (word)network_player->name[index]) * 16777619UL;
	hash = (hash ^ (byte)network_player->machine_index) * 16777619UL;
	hash = (hash ^ (byte)network_player->controller_index) * 16777619UL;
	hash = (hash ^ (byte)network_player->player_list_index) * 16777619UL;
	return hash ? hash : 1;
}

/* a player's role, if the state has it for them (not one who left, whose
slot another took) */
static struct distributed_infection_player *infection_player(
	long player_index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);

	if (player_index == NONE || !VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS) ||
		!player_try_and_get(player_index) ||
		infection.state.players[slot].identity != infection_player_identity(player_index))
	{
		return NULL;
	}
	return &infection.state.players[slot];
}

static boolean infection_weapon_is_skull(
	long weapon_index)
{
	struct object_datum *object = object_try_and_get_and_verify_type(weapon_index, _object_mask_weapon);

	return object && object->definition_index == get_ball_definition_index();
}

boolean infection_can_delete_skull(
	long object_index)
{
	return infection_enabled() && infection_weapon_is_skull(object_index);
}

boolean infection_unit_is_zombie(
	long unit_index)
{
	struct unit_datum *unit;
	struct distributed_infection_player *role;

	if (unit_index == NONE || !infection_enabled() || (unit = unit_try_and_get(unit_index)) == NULL)
		return FALSE;
	role = infection_player(unit->unit.player_index);
	return role && role->role == _distributed_infection_role_zombie;
}

/* asked of a player's own drops only: a death, and the inventories the host
sends, still take a skull away */
boolean infection_allow_weapon_drop(
	long unit_index,
	long weapon_index)
{
	struct unit_datum *unit;
	struct player_datum *player;

	if (!infection_enabled() || infection.state.phase != _distributed_infection_active || infection.equipping ||
		!infection_unit_is_zombie(unit_index))
	{
		return TRUE;
	}
	unit = unit_get(unit_index);
	player = player_try_and_get(unit->unit.player_index);
	if (!player || player->quit_out_of_game || player->unit_index != unit_index ||
		TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
	{
		return TRUE;
	}
	return !infection_weapon_is_skull(weapon_index);
}

/* a zombie's skull in the hand */
static boolean infection_unit_holds_skull(
	long unit_index)
{
	struct unit_datum *unit = unit_get(unit_index);

	return VALID_INDEX(unit->unit.current_weapon_index, MAXIMUM_WEAPONS_PER_UNIT) &&
		infection_weapon_is_skull(unit->unit.weapon_object_indices[unit->unit.current_weapon_index]);
}

real infection_melee_multiplier(
	long unit_index)
{
	if (!infection_unit_is_zombie(unit_index) || infection.state.phase != _distributed_infection_active ||
		!infection_unit_holds_skull(unit_index))
	{
		return 1.0f;
	}
	return game_variant_options_get()->infection_skull_damage / 100.0f;
}

/* a living human in a seat of a vehicle that is not hidden, whom a zombie
holding its skull struck itself (not the vehicle: Slayer's melee hits the
vehicle a rider is in) */
boolean infection_melee_rider_target(
	long attacker_index,
	long target_index)
{
	struct unit_datum *target;
	struct unit_datum *vehicle;
	struct distributed_infection_player *role;
	struct unit_definition *definition;
	short seat;

	if (!infection_unit_is_zombie(attacker_index) || infection.state.phase != _distributed_infection_active ||
		!infection_unit_holds_skull(attacker_index))
	{
		return FALSE;
	}
	target = unit_try_and_get(target_index);
	if (!target || target->object.type != _object_type_biped || TEST_FLAG(target->object.damage_flags, _object_dead_bit))
		return FALSE;
	role = infection_player(target->unit.player_index);
	if (!role || role->role != _distributed_infection_role_human)
		return FALSE;
	vehicle = (struct unit_datum *)object_try_and_get_and_verify_type(target->object.parent_object_index,
		_object_mask_vehicle);
	if (!vehicle)
		return FALSE;
	definition = unit_definition_get(vehicle->definition_index);
	seat = target->unit.parent_seat_index;
	return VALID_INDEX(seat, definition->unit.seats.count) &&
		!TEST_FLAG(TAG_BLOCK_GET_ELEMENT(&definition->unit.seats, seat, struct unit_seat)->flags,
			_unit_seat_invisible_bit);
}

/* the host's check of a client's melee report on a rider: one of its own
lines through the melee's cone (0.8 world units from the attacker's head,
as the game's melee) reaches the rider, through the host's own world, not
from a place the client says */
boolean infection_melee_rider_visible(
	long attacker_index,
	long target_index,
	real_vector3d const *direction)
{
	struct object_marker head;
	real_vector3d facing = *direction;
	real_vector3d perpendicular;
	real_vector3d cross;
	real_vector3d ray;
	struct collision_result hit;
	long outer;
	long inner;

	if (!infection_melee_rider_target(attacker_index, target_index) || normalize3d(&facing) <= 0.0f)
		return FALSE;
	object_get_marker_by_name(attacker_index, "head", &head, 1);
	normalize3d(perpendicular3d(&facing, &perpendicular));
	cross_product3d(&facing, &perpendicular, &cross);
	for (outer = -2; outer <= 2; outer++)
	{
		for (inner = -2; inner <= 2; inner++)
		{
			ray.i = facing.i * 0.8f + (cross.i * inner + perpendicular.i * outer) * 0.1f;
			ray.j = facing.j * 0.8f + (cross.j * inner + perpendicular.j * outer) * 0.1f;
			ray.k = facing.k * 0.8f + (cross.k * inner + perpendicular.k * outer) * 0.1f;
			if (collision_test_vector(_collision_test_for_projectiles_flags, &head.matrix.position, &ray,
				attacker_index, &hit) && hit.type == _collision_result_object && hit.object_index == target_index)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* ---- the host's round */

static void infection_finish(
	long winner)
{
	if (infection.state.phase == _distributed_infection_finished)
		return;
	infection.state.winner = winner;
	infection.state.phase = _distributed_infection_finished;
	infection.state.revision++;
	platform_log("infection: finished, winner %ld", winner);
	game_engine_end_game();
}

/* a unit's armour in its team's colour (a human turned while alive), as a
spawn colours it; the colour channels a function drives (the shield's glow)
are left to it */
static void infection_color_unit(
	struct unit_datum *unit,
	long player_index)
{
	struct object_definition *definition = object_definition_get(unit->definition_index);
	real_rgb_color color;
	short channel;

	game_engine_player_get_change_color(&color, player_index);
	for (channel = 0; channel < NUMBER_OF_OBJECT_CHANGE_COLORS; channel++)
	{
		unit->object.base_change_colors[channel] = color;
		if (channel < definition->object.change_colors.count)
		{
			struct object_change_color_definition *change_color = TAG_BLOCK_GET_ELEMENT(
				&definition->object.change_colors, channel, struct object_change_color_definition);

			if (change_color->scaled_by || change_color->darken_by)
				continue;
		}
		unit->object.outgoing_change_colors[channel] = color;
	}
}

/* a zombie without shields (the gametype's infection_zombie_shields): a
shield capacity of 0, the game's own unshielded unit, which neither takes
shield damage nor recharges; a human again (or the option on) gets the
unit's capacity back, without healing its body */
static void infection_apply_shields(
	long slot,
	long unit_index,
	struct unit_datum *unit,
	boolean disabled)
{
	if (infection.shield_unit[slot] != unit_index)
		infection.shield_unit[slot] = NONE;
	if (disabled)
	{
		if (infection.shield_unit[slot] == NONE)
		{
			infection.shield_unit[slot] = unit_index;
			infection.shield_maximum[slot] = unit->object.maximum_shield_vitality;
		}
		unit->object.maximum_shield_vitality = 0.0f;
		unit->object.shield_vitality = 0.0f;
		SET_FLAG(unit->object.damage_flags, _object_shield_charging_bit, FALSE);
		SET_FLAG(unit->object.damage_flags, _object_shield_over_charging_bit, FALSE);
		SET_FLAG(unit->object.damage_flags, _object_shield_depleted_bit, TRUE);
	}
	else if (infection.shield_unit[slot] == unit_index)
	{
		unit->object.maximum_shield_vitality = infection.shield_maximum[slot];
		infection.shield_unit[slot] = NONE;
	}
}

void infection_reconcile_player(
	long player_index)
{
	struct game_variant_options const *options = game_variant_options_get();
	struct distributed_infection_player *role;
	struct player_datum *player;
	struct unit_datum *unit;
	long team;

	if (!infection_enabled() || (role = infection_player(player_index)) == NULL ||
		role->role == _distributed_infection_role_none)
	{
		return;
	}
	player = player_get(player_index);
	team = role->role == _distributed_infection_role_zombie ? 1 : 0;
	player->team_index = team;
	player->network_player_data.team_index = (char)team;
	player->speed_multiplier = team ? options->infection_zombie_speed / 100.0f : 1.0f;
	if (player->unit_index != NONE && (unit = unit_try_and_get(player->unit_index)) != NULL)
	{
		infection_apply_shields(DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index), player->unit_index, unit,
			team && !options->infection_zombie_shields);
		unit->object.owner_team_index = (short)team;
		infection_color_unit(unit, player_index);
		/* (a living zombie's grenades and powerups gone; a dead one's are its
		body's, which drops them) */
		if (team && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		{
			/* (a grenade being wound up when its thrower turned is not thrown:
			the throw would find no grenade left) */
			if (unit->unit.grenade_throw_state == _unit_grenade_throw_wind_up)
				unit->unit.grenade_throw_state = _unit_grenade_throw_ending;
			csmemset(unit->unit.grenade_counts, 0, sizeof(unit->unit.grenade_counts));
			csmemset(player->powerup_durations, 0, sizeof(player->powerup_durations));
		}
	}
	/* (and the host's game record, which the lobby and late joins read) */
	if (!network_game_distributed_client())
	{
		struct network_game_server *server = global_network_game_server_get();
		struct network_game *game = server ? network_game_server_get_game(server) : NULL;
		long list_index = player->network_player_data.player_list_index;

		if (game && VALID_INDEX(list_index, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
			game->players[list_index].team_index = (char)team;
	}
}

static void infection_delete_weapon(
	long weapon_index)
{
	long slot;

	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
	{
		if (infection.skull[slot] == weapon_index)
			infection.skull[slot] = NONE;
	}
	if (weapon_index != NONE && object_try_and_get(weapon_index))
		object_delete(weapon_index);
}

static void infection_delete_skull(
	long slot)
{
	infection_delete_weapon(infection.skull[slot]);
	infection.skull[slot] = NONE;
}

/* a unit's weapons gone but the one kept (skulls only: a human's skull
picked up), each deleted once */
static void infection_strip_weapons(
	long unit_index,
	long keep,
	boolean skulls_only)
{
	struct unit_datum *unit = unit_get(unit_index);
	long removed[MAXIMUM_WEAPONS_PER_UNIT];
	long count = 0;
	long slot;
	long previous;
	long kept = NONE;

	for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
	{
		long weapon_index = unit->unit.weapon_object_indices[slot];

		if (weapon_index == NONE)
			continue;
		if (weapon_index == keep && kept == NONE)
		{
			kept = slot;
			continue;
		}
		if (skulls_only && !infection_weapon_is_skull(weapon_index))
			continue;
		unit->unit.weapon_object_indices[slot] = NONE;
		if (unit->unit.current_weapon_index == slot)
			unit->unit.current_weapon_index = NONE;
		if (unit->unit.desired_weapon_index == slot)
			unit->unit.desired_weapon_index = NONE;
		if (weapon_index == keep)
			continue;
		for (previous = 0; previous < count && removed[previous] != weapon_index; previous++)
		{
		}
		if (previous == count)
			removed[count++] = weapon_index;
	}
	for (slot = 0; slot < count; slot++)
		infection_delete_weapon(removed[slot]);
	if (kept != NONE)
		unit->unit.desired_weapon_index = (short)kept;
	else if (unit->unit.desired_weapon_index == NONE)
		unit->unit.desired_weapon_index = unit_inventory_next_weapon(unit_index, NONE, 0);
}

/* the host gives a living player their role's equipment: a zombie its skull
alone (out of any vehicle, without grenades or powerups), a human what they
spawned with, without a skull */
static void infection_equip(
	long player_index)
{
	struct player_datum *player = player_get(player_index);
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	struct distributed_infection_player *role = infection_player(player_index);
	struct unit_datum *unit;
	long weapon_index;
	short inventory_slot;

	if (network_game_distributed_client() || !role || player->quit_out_of_game || player->unit_index == NONE ||
		infection.missing_skull)
	{
		return;
	}
	unit = unit_get(player->unit_index);
	if (TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
		return;
	if (infection.equipped_unit[slot] == player->unit_index)
	{
		if (role->role == _distributed_infection_role_human)
		{
			infection_strip_weapons(player->unit_index, NONE, TRUE);
			return;
		}
		/* (a zombie still holding its skull keeps it; any other weapon goes) */
		for (inventory_slot = 0; inventory_slot < MAXIMUM_WEAPONS_PER_UNIT; inventory_slot++)
		{
			weapon_index = unit->unit.weapon_object_indices[inventory_slot];
			if (infection_weapon_is_skull(weapon_index))
			{
				if (infection.skull[slot] != weapon_index)
					infection_delete_skull(slot);
				infection.skull[slot] = weapon_index;
				infection_strip_weapons(player->unit_index, weapon_index, FALSE);
				return;
			}
		}
	}
	infection.equipping = TRUE;
	infection_strip_weapons(player->unit_index, NONE, role->role == _distributed_infection_role_human);
	infection_delete_skull(slot);
	if (role->role == _distributed_infection_role_zombie)
	{
		if (unit->object.parent_object_index != NONE)
			unit_exit_seat_end(player->unit_index);
		csmemset(unit->unit.grenade_counts, 0, sizeof(unit->unit.grenade_counts));
		if (unit->unit.equipment_object_index != NONE)
		{
			object_delete(unit->unit.equipment_object_index);
			unit->unit.equipment_object_index = NONE;
		}
		weapon_index = NONE;
		if (get_ball_definition_index() != NONE)
		{
			struct object_placement_data placement;

			object_placement_data_new(&placement, get_ball_definition_index(), NONE);
			placement.owner_team_index = 1;
			weapon_index = object_new(&placement);
		}
		if (weapon_index == NONE || !unit_add_weapon_to_inventory(player->unit_index, weapon_index, 2))
		{
			if (weapon_index != NONE)
				object_delete(weapon_index);
			infection.missing_skull = TRUE;
		}
		else
		{
			infection.skull[slot] = weapon_index;
		}
	}
	infection.equipping = FALSE;
	infection.equipped_unit[slot] = player->unit_index;
	infection.killed_unit[slot] = NONE;
}

/* a player new to the round: a human before the zombies are chosen, a
zombie after */
static void infection_player_added(
	long player_index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	struct distributed_infection_player *role;

	if (infection.slayer.player_added)
		infection.slayer.player_added(player_index);
	if (!VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return;
	role = &infection.state.players[slot];
	if (!network_game_distributed_client())
	{
		infection_delete_skull(slot);
		csmemset(role, 0, sizeof(*role));
		role->identity = infection_player_identity(player_index);
		role->role = infection.state.phase >= _distributed_infection_active ? _distributed_infection_role_zombie :
			_distributed_infection_role_human;
		infection.participant[slot] = player_index;
		infection.equipped_unit[slot] = NONE;
		infection.killed_unit[slot] = NONE;
		infection.state.revision++;
	}
	infection_reconcile_player(player_index);
}

void infection_postspawn(
	long player_index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);

	if (!VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return;
	if (!network_game_distributed_client() && infection.participant[slot] != player_index)
		infection_player_added(player_index);
	infection_reconcile_player(player_index);
	infection.equipped_unit[slot] = NONE;
	infection_equip(player_index);
}

static void infection_player_killed_player(
	long killing_player_index,
	long killing_object_index,
	long dead_player_index,
	boolean friendly_fire)
{
	struct distributed_infection_player *dead = infection_player(dead_player_index);
	struct distributed_infection_player *killer = infection_player(killing_player_index);
	struct player_datum *player = player_get(dead_player_index);
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(dead_player_index);
	long unit_index = player->unit_index != NONE ? player->unit_index : player->dead_unit_index;

	(void)killing_object_index;
	if (network_game_distributed_client() || infection.state.phase != _distributed_infection_active || !dead ||
		player->quit_out_of_game || infection.killed_unit[slot] != NONE)
	{
		return;
	}
	infection.killed_unit[slot] = unit_index == NONE ? 0 : unit_index;
	if (killer && killer != dead && killer->role != dead->role && killer->score < INFECTION_MAXIMUM_SCORE)
		killer->score++;
	/* (a human killed by a zombie, themselves or the world turns; one killed
	by a human teammate does not) */
	if (dead->role == _distributed_infection_role_human &&
		!(friendly_fire && killer && killer != dead && killer->role == dead->role))
	{
		dead->role = _distributed_infection_role_zombie;
		platform_log("infection: player %ld infected", slot);
	}
	infection_reconcile_player(dead_player_index);
	/* (the body's skull is deleted on the player's next tick, once the
	unit's death has dropped what it held) */
	infection.equipped_unit[slot] = NONE;
	infection.state.revision++;
}

/* the round's result: 0 the humans won, 1 the zombies, 2 no one (everyone
left), NONE still on. Turning the last human wins over the time running
out on the same tick. */
static long infection_round_result(
	long humans,
	long zombies,
	long elapsed,
	long limit)
{
	if (!humans && !zombies)
		return 2;
	if (!humans)
		return 1;
	if (!zombies || (limit > 0 && elapsed >= limit))
		return 0;
	return NONE;
}

static long infection_time_limit_ticks(
	void)
{
	return game_variant_options_get()->time_limit * 60L * TICKS_PER_SECOND;
}

static void infection_update(
	void)
{
	struct game_variant_options const *options = game_variant_options_get();
	struct data_iterator iterator;
	struct player_datum *player;
	boolean present[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long candidates[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	long humans = 0;
	long zombies = 0;
	long count = 0;
	long index;
	long now = game_time_get();

	if (infection.slayer.update)
		infection.slayer.update();
	if (network_game_distributed_client() || infection.state.phase == _distributed_infection_finished)
		return;
	csmemset(present, 0, sizeof(present));
	data_iterator_new(&iterator, player_data);
	while ((player = (struct player_datum *)data_iterator_next(&iterator)) != NULL)
	{
		long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.datum_index);
		struct unit_datum *unit;

		if (player->quit_out_of_game || !VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
			continue;
		present[slot] = TRUE;
		if (infection.participant[slot] != iterator.datum_index)
			infection_player_added(iterator.datum_index);
		infection_reconcile_player(iterator.datum_index);
		if (infection.state.players[slot].role == _distributed_infection_role_human)
			humans++;
		else
			zombies++;
		/* (the first zombies are chosen of the living: a full map can leave
		players waiting to spawn) */
		unit = player->unit_index != NONE ? unit_try_and_get(player->unit_index) : NULL;
		if (unit && !TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
			candidates[count++] = iterator.datum_index;
	}
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		if (!present[index] && infection.participant[index] != NONE)
		{
			infection_delete_skull(index);
			infection.participant[index] = NONE;
			infection.equipped_unit[index] = NONE;
			infection.killed_unit[index] = NONE;
			csmemset(&infection.state.players[index], 0, sizeof(infection.state.players[index]));
			infection.state.revision++;
		}
	}
	if (infection.missing_skull)
	{
		platform_log("infection: no skull to give (the map has no Oddball ball), the round ends");
		infection_finish(NONE);
		return;
	}
	if (infection.state.phase == _distributed_infection_active)
	{
		long result = infection_round_result(humans, zombies, now - infection.state.phase_tick,
			infection_time_limit_ticks());

		if (result != NONE)
		{
			platform_log("infection: result %ld, %ld humans, %ld zombies", result, humans, zombies);
			infection_finish(result == 2 ? NONE : result);
		}
		return;
	}
	if (count < 2)
	{
		if (infection.state.phase != _distributed_infection_waiting)
			infection.state.revision++;
		infection.state.phase = _distributed_infection_waiting;
		infection.state.phase_tick = now;
		return;
	}
	if (infection.state.phase == _distributed_infection_waiting)
	{
		infection.state.phase = _distributed_infection_countdown;
		infection.state.phase_tick = now;
		infection.state.revision++;
		platform_log("infection: countdown, %ld players", count);
	}
	if (now - infection.state.phase_tick < options->infection_countdown * TICKS_PER_SECOND)
		return;
	infection.state.phase = _distributed_infection_active;
	infection.state.phase_tick = now;
	/* the first zombies, drawn without repeats, leaving a human */
	for (index = 0; index < MIN(count - 1, options->infection_initial_zombies); index++)
	{
		long pick = index + local_random_range(0, (short)(count - index));
		long chosen = candidates[pick];
		long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(chosen);

		candidates[pick] = candidates[index];
		candidates[index] = chosen;
		infection.state.players[slot].role = _distributed_infection_role_zombie;
		infection.equipped_unit[slot] = NONE;
		infection_reconcile_player(chosen);
		infection_equip(chosen);
	}
	infection.state.revision++;
	platform_log("infection: active, %ld players, %ld zombies", count,
		MIN(count - 1, (long)options->infection_initial_zombies));
}

long infection_remaining_ticks(
	void)
{
	long limit = infection_time_limit_ticks();

	return infection.state.phase == _distributed_infection_active ?
		MAX(0, limit - (game_time_get() - infection.state.phase_tick)) : limit;
}

static boolean infection_initialize_for_new_map(
	void)
{
	struct game_variant *variant = game_engine_get_variant();

	csmemset(&infection.state, 0, sizeof(infection.state));
	csmemset(infection.participant, 0xFF, sizeof(infection.participant));
	csmemset(infection.equipped_unit, 0xFF, sizeof(infection.equipped_unit));
	csmemset(infection.killed_unit, 0xFF, sizeof(infection.killed_unit));
	csmemset(infection.skull, 0xFF, sizeof(infection.skull));
	csmemset(infection.shield_unit, 0xFF, sizeof(infection.shield_unit));
	infection.state.winner = NONE;
	infection.state.game_number = network_game_get_game() ? network_game_get_number_of_games_played() : 0;
	infection.missing_skull = get_ball_definition_index() == NONE;
	infection.equipping = FALSE;
	/* (everyone respawns; the variant's traits give zombies no infinite
	grenades nor camouflage) */
	variant->universal_variant.lives = 0;
	variant->universal_variant.odd_man_out = FALSE;
	SET_FLAG(variant->universal_variant.flags, _game_variant_infinite_grenades_bit, FALSE);
	SET_FLAG(variant->universal_variant.flags, _game_variant_always_invisible_bit, FALSE);
	return infection.slayer.initialize_for_new_map ? infection.slayer.initialize_for_new_map() : TRUE;
}

/* a skull is picked up only as it is given; a zombie picks up nothing else
(and readying the skull it holds asks this too) */
static boolean infection_allow_pick_up(
	long unit_index,
	long weapon_index)
{
	short slot;

	if (infection.equipping)
		return TRUE;
	if (infection_weapon_is_skull(weapon_index))
	{
		if (infection_unit_is_zombie(unit_index))
		{
			for (slot = 0; slot < MAXIMUM_WEAPONS_PER_UNIT; slot++)
			{
				if (unit_get(unit_index)->unit.weapon_object_indices[slot] == weapon_index)
					return TRUE;
			}
		}
		return FALSE;
	}
	return !infection_unit_is_zombie(unit_index);
}

static boolean infection_picking_up(
	long weapon_index,
	long player_index)
{
	(void)weapon_index;
	(void)player_index;
	return infection.equipping;
}

/* a skull dropped (its zombie killed, or forced out of a hand): given again
on the next tick, if its zombie lives */
static void infection_weapon_dropped(
	long weapon_index)
{
	long slot;

	if (network_game_distributed_client() || infection.equipping)
		return;
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
	{
		if (infection.skull[slot] == weapon_index)
		{
			infection.equipped_unit[slot] = NONE;
			break;
		}
	}
}

static void infection_player_update_each_tick(
	long player_index)
{
	struct player_datum *player = player_get(player_index);

	infection_reconcile_player(player_index);
	if (network_game_distributed_client())
		return;
	/* (a dead zombie's skull deleted once its unit's death has dropped it) */
	if (player->quit_out_of_game || player->unit_index == NONE ||
		TEST_FLAG(unit_get(player->unit_index)->object.damage_flags, _object_dead_bit))
	{
		infection_delete_skull(DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index));
	}
	else
	{
		infection_equip(player_index);
	}
}

static long infection_get_player_score(
	long player_index,
	enum get_score_type score_type)
{
	struct distributed_infection_player *role = infection_player(player_index);
	long slot;
	long score = 0;

	if (!role)
		return 0;
	if (score_type != _get_score_team)
		return role->score;
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
	{
		if (infection.state.players[slot].role == role->role)
			score += infection.state.players[slot].score;
	}
	return score;
}

static wchar_t *infection_format_player_score(
	long player_index,
	wchar_t *text)
{
	usprintf(text, L"%ld", infection_get_player_score(player_index, _get_score_individual));
	return text;
}

/* (a team's score, whatever its name says) */
static wchar_t *infection_format_team_score(
	long team_index,
	wchar_t *text)
{
	usprintf(text, L"%ld", game_engine_get_team_score(team_index));
	return text;
}

static long infection_did_player_win(
	long player_index)
{
	struct distributed_infection_player *role = infection_player(player_index);

	if (infection.state.winner == NONE || !role)
		return NONE;
	return (role->role == _distributed_infection_role_zombie ? 1 : 0) == infection.state.winner;
}

struct game_engine *infection_engine_get(
	struct game_engine const *slayer)
{
	infection.slayer = *slayer;
	infection.engine = *slayer;
	infection.engine.initialize_for_new_map = infection_initialize_for_new_map;
	infection.engine.player_added = infection_player_added;
	infection.engine.player_update_each_tick = infection_player_update_each_tick;
	infection.engine.prespawn_player_update = infection_reconcile_player;
	infection.engine.update = infection_update;
	infection.engine.allow_pick_up = infection_allow_pick_up;
	infection.engine.picking_up = infection_picking_up;
	infection.engine.weapon_dropped = infection_weapon_dropped;
	infection.engine.player_killed_player = infection_player_killed_player;
	infection.engine.get_player_score = infection_get_player_score;
	infection.engine.format_player_score = infection_format_player_score;
	infection.engine.format_team_name = infection_format_team_score;
	infection.engine.format_message = NULL;
	infection.engine.did_player_win = infection_did_player_win;
	return &infection.engine;
}

/* ---- the state the host sends */

/* a state a client may take: of this game, no older than its own, every
value in range */
static boolean infection_state_valid(
	struct distributed_infection_state const *state,
	long game_number,
	long revision)
{
	short index;

	if (state->game_number != game_number || state->revision < revision || state->revision < 0 ||
		state->phase < _distributed_infection_waiting || state->phase > _distributed_infection_finished ||
		state->phase_tick < 0 || state->winner < NONE || state->winner > 1 ||
		(state->phase != _distributed_infection_finished && state->winner != NONE))
	{
		return FALSE;
	}
	for (index = 0; index < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; index++)
	{
		struct distributed_infection_player const *player = &state->players[index];

		if (player->role > _distributed_infection_role_zombie || player->score < 0 ||
			player->score > INFECTION_MAXIMUM_SCORE || (player->role && !player->identity) ||
			player->reserved[0] || player->reserved[1] || player->reserved[2])
		{
			return FALSE;
		}
	}
	return TRUE;
}

long infection_write_network_state(
	byte *buffer,
	long size)
{
	if (size < (long)sizeof(infection.state))
		return 0;
	csmemcpy(buffer, &infection.state, sizeof(infection.state));
	return sizeof(infection.state);
}

boolean infection_read_network_state(
	byte const *buffer,
	long size,
	boolean first)
{
	struct distributed_infection_state state;
	struct data_iterator iterator;

	if (!network_game_distributed_client() || size != (long)sizeof(state))
		return FALSE;
	csmemcpy(&state, buffer, sizeof(state));
	if (!infection_state_valid(&state, network_game_get_number_of_games_played(), first ? 0 : infection.state.revision))
		return FALSE;
	infection.state = state;
	data_iterator_new(&iterator, player_data);
	while (data_iterator_next(&iterator))
		infection_reconcile_player(iterator.datum_index);
	return TRUE;
}
