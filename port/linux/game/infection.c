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
infection_*). Floodfection (the options' mode_variant) is Infection with the
Flood's look, which each machine draws for itself: its zombies green instead
of blue and drawn as Flood combat forms, a human's or an elite's, but in
their own player's first person (floodfection_forms.c), without shields and
half as fast and as healthy again as a human (150% speed and health)
whatever the options say, a headshot not killing them outright, bleeding the
Flood's green and throwing its bits as they die; its humans each in their
own colour (as free for all's) instead of red; every map under a green murk
that hides the sky and all past some 55 world units (16 under a roof); and a
Flood carrier's burst where a human turns or a zombie dies (its tags from
opence_floodfection.pack: level_tag_import.c). A build without it plays the
same round, its zombies blue and its maps clear. The rules and much of the
code follow the Infection of pull request #420 by
arisson2001rojas-design.
*/

#include "cseries.h"
#include "effects/effect_definitions.h"
#include "effects/effects.h"
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
#include "render/render_cameras.h"
#include "saved games/player_profile.h"
#include "tag_files/tag_groups.h"
#include "text/unicode.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "floodfection_forms.h"
#include "game_modes.h"
#include "infection.h"
#include "network_infection.h"
#include "tag_import.h"

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
	/* the unit whose body a Floodfection zombie's role strengthened, and
	its maximum before (by absolute player index; every machine's) */
	long health_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
	real health_maximum[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
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

/* ---- Floodfection's look */

/* the Flood's fog: from a few world units off, opaque at its end, which the
window clears to (the sky not drawn). Outdoors as far as a field needs;
under a roof (a cluster with no sky: Prisoner's rooms, a base's halls) much
closer, as rooms a few units across would hardly be tinted by the
outdoors' */
#define FLOODFECTION_FOG_START_DISTANCE 3.0f
#define FLOODFECTION_FOG_OPAQUE_DISTANCE 55.0f
#define FLOODFECTION_INDOOR_FOG_START_DISTANCE 0.5f
#define FLOODFECTION_INDOOR_FOG_OPAQUE_DISTANCE 16.0f
/* the share of the way from one to the other each frame drawn, walking in
or out a door */
#define FLOODFECTION_FOG_BLEND 0.08f

static real_rgb_color const floodfection_zombie_color = { { 0.30f, 0.48f, 0.12f } };
static real_rgb_color const floodfection_fog_color = { { 0.16f, 0.20f, 0.09f } };

/* each local player's fog distances as last drawn, 0 before the first */
static real floodfection_fog_start[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
static real floodfection_fog_opaque[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];

static boolean infection_floodfection(
	void)
{
	return infection_enabled() && game_modes_mode_variant() == _game_mode_variant_floodfection;
}

boolean infection_floodfection_enabled(
	void)
{
	return infection_floodfection();
}

boolean infection_floodfection_zombie(
	long object_index)
{
	return object_index != NONE && infection_floodfection() &&
		object_try_and_get_and_verify_type(object_index, _object_mask_unit) &&
		infection_unit_is_zombie(object_index);
}

/* a zombie green; anyone else the colour of their profile, as free for
all's (the humans are one team, but not one colour) */
void infection_change_color(
	long player_index,
	real_rgb_color *color)
{
	struct distributed_infection_player *role;

	if (!infection_floodfection())
		return;
	role = infection_player(player_index);
	if (role && role->role == _distributed_infection_role_zombie)
		*color = floodfection_zombie_color;
	else
		*color = player_profile_get_rgb_color(player_get(player_index)->network_player_data.primary_color_index);
}

boolean infection_fog(
	short local_player_index,
	boolean indoors,
	struct render_fog *fog)
{
	real start = indoors ? FLOODFECTION_INDOOR_FOG_START_DISTANCE : FLOODFECTION_FOG_START_DISTANCE;
	real opaque = indoors ? FLOODFECTION_INDOOR_FOG_OPAQUE_DISTANCE : FLOODFECTION_FOG_OPAQUE_DISTANCE;

	if (!infection_floodfection())
		return FALSE;
	if (VALID_INDEX(local_player_index, MAXIMUM_NUMBER_OF_LOCAL_PLAYERS))
	{
		real *last_start = &floodfection_fog_start[local_player_index];
		real *last_opaque = &floodfection_fog_opaque[local_player_index];

		if (*last_opaque > 0.0f)
		{
			start = *last_start + (start - *last_start) * FLOODFECTION_FOG_BLEND;
			opaque = *last_opaque + (opaque - *last_opaque) * FLOODFECTION_FOG_BLEND;
		}
		*last_start = start;
		*last_opaque = opaque;
	}
	fog->atmospheric_color = floodfection_fog_color;
	fog->atmospheric_maximum_density = 1.0f;
	fog->atmospheric_minimum_distance = start;
	fog->atmospheric_maximum_distance = MAX(opaque, start + 0.0001f);
	return TRUE;
}

/* the map Floodfection's tags are copied from (Two Betrayals), and its
carrier's burst */
#define FLOODFECTION_DONOR "c40"
#define FLOODFECTION_BURST_TAG "characters\\floodcarrier\\effects\\body destroyed"

boolean infection_effect_local = FALSE;

/* how high above a dead zombie's feet its bits are thrown from (world units) */
#define FLOODFECTION_BITS_HEIGHT 0.5f

/* the Flood's burst (Two Betrayals' carrier's, copied into the level from
opence_floodfection.pack: infection_tag_import) where a player turned or a
zombie died, and where a zombie died the Flood's bits too (its limbs, head, skin and
innards thrown). Each machine plays its own, as it sees the turn (the
host's state) or the death (the unit's): no message, and nothing of the
game's but particles, decals, the sound and the bits, garbage, which each
machine keeps for itself (network_objects.c sends none): no damage, and no
objects a client would not have */
static long floodfection_burst_definition = NONE;
static char const *const floodfection_bits_tags[] =
{
	"effects\\flood bit limb explode",
	"effects\\flood bit h-head explode",
	"effects\\flood bit skin explode",
};
static long floodfection_bits_definitions[NUMBEROF(floodfection_bits_tags)];
/* by absolute player index: the role last seen, and the unit last burst */
static short floodfection_seen_role[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];
static long floodfection_burst_unit[HALO_PORT_MAXIMUM_NETWORK_PLAYERS];

/* Floodfection's tags of Two Betrayals, copied into a multiplayer level as
it loads (level_tag_import.c), from its pack (its manifest:
port/assets/tag_packs/opence_floodfection.toml). The game's mode,
as every machine of the game has it, says whether, so each copies alike */
boolean infection_tag_import_wanted(
	void)
{
	struct network_game *game = network_game_get_game();

	return game && game_modes_variant_mode_variant(&game->variant, &game->variant_options) ==
		_game_mode_variant_floodfection;
}

void infection_tag_import(
	void)
{
	static struct tag_import_root const roots[] =
	{
		{ 'effe', FLOODFECTION_BURST_TAG },
		/* (its bits where a zombie dies) */
		{ 'effe', "effects\\flood bit limb explode" },
		{ 'effe', "effects\\flood bit h-head explode" },
		{ 'effe', "effects\\flood bit skin explode" },
		/* (its zombies' forms: floodfection_forms.c) */
		{ 'mode', "characters\\floodcombat_human\\floodcombat_human" },
		{ 'antr', "characters\\floodcombat_human\\floodcombat_human" },
		{ 'mode', "characters\\floodcombat elite\\floodcombat elite" },
		{ 'antr', "characters\\floodcombat elite\\floodcombat elite" },
	};

	tag_import_roots(FLOODFECTION_PACK, FLOODFECTION_DONOR, roots, (int)NUMBEROF(roots));
}

static void floodfection_burst_reset(
	void)
{
	long slot;

	floodfection_burst_definition = tag_loaded(EFFECT_DEFINITION_TAG, FLOODFECTION_BURST_TAG);
	for (slot = 0; slot < (long)NUMBEROF(floodfection_bits_tags); slot++)
		floodfection_bits_definitions[slot] = tag_loaded(EFFECT_DEFINITION_TAG, floodfection_bits_tags[slot]);
	for (slot = 0; slot < HALO_PORT_MAXIMUM_NETWORK_PLAYERS; slot++)
	{
		floodfection_seen_role[slot] = _distributed_infection_role_none;
		floodfection_burst_unit[slot] = NONE;
	}
	csmemset(floodfection_fog_start, 0, sizeof(floodfection_fog_start));
	csmemset(floodfection_fog_opaque, 0, sizeof(floodfection_fog_opaque));
	floodfection_forms_reset();
}

static void floodfection_effect_at(
	long definition_index,
	real_point3d const *point)
{
	char const *marker_names[1];
	real_point3d marker_points[1];
	real_vector3d marker_forwards[1];

	if (definition_index == NONE)
		return;
	marker_names[0] = "";
	marker_points[0] = *point;
	marker_forwards[0] = *global_up3d;
	infection_effect_local = TRUE;
	effect_new_unattached_from_markers(definition_index, NONE, NULL, 1, marker_names, marker_points,
		marker_forwards, 0.0f, 0.0f, NULL, NULL, TRUE);
	infection_effect_local = FALSE;
}

/* the burst at the unit, and a dead zombie's bits (from its middle, about
where its form's chest was) */
static void floodfection_burst_at(
	long unit_index,
	boolean bits)
{
	real_point3d point;
	short index;

	object_get_origin(unit_index, &point);
	floodfection_effect_at(floodfection_burst_definition, &point);
	if (!bits)
		return;
	point.z += FLOODFECTION_BITS_HEIGHT;
	for (index = 0; index < (short)NUMBEROF(floodfection_bits_definitions); index++)
		floodfection_effect_at(floodfection_bits_definitions[index], &point);
}

/* a human turned (killed or chosen: the body, or the living unit), or a
zombie's unit dead: a burst there, once for each unit */
static void floodfection_bursts(
	long player_index)
{
	long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	struct distributed_infection_player *role = infection_player(player_index);
	struct player_datum *player = player_get(player_index);
	long unit_index = player->unit_index != NONE ? player->unit_index : player->dead_unit_index;
	struct unit_datum *unit = unit_index != NONE ? unit_try_and_get(unit_index) : NULL;
	short seen;

	if (!infection_floodfection() || !VALID_INDEX(slot, HALO_PORT_MAXIMUM_NETWORK_PLAYERS))
		return;
	seen = floodfection_seen_role[slot];
	floodfection_seen_role[slot] = role ? role->role : _distributed_infection_role_none;
	if (floodfection_burst_definition == NONE || !role || role->role != _distributed_infection_role_zombie ||
		!unit || player->quit_out_of_game || floodfection_burst_unit[slot] == unit_index)
	{
		return;
	}
	if (seen == _distributed_infection_role_human || TEST_FLAG(unit->object.damage_flags, _object_dead_bit))
	{
		floodfection_burst_unit[slot] = unit_index;
		floodfection_burst_at(unit_index, seen != _distributed_infection_role_human);
	}
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

/* a Floodfection zombie's body: half as much again as its unit's (the
maximum raised, its share of it kept, so turning heals no one); a human
again gets the unit's back */
#define FLOODFECTION_ZOMBIE_HEALTH_SCALE 1.5f
/* and its speed, whatever the gametype says (percent) */
#define FLOODFECTION_ZOMBIE_SPEED 150

static void infection_apply_health(
	long slot,
	long unit_index,
	struct unit_datum *unit,
	boolean stronger)
{
	if (infection.health_unit[slot] != unit_index)
		infection.health_unit[slot] = NONE;
	if (stronger)
	{
		if (infection.health_unit[slot] == NONE && unit->object.maximum_body_vitality > 0.0f)
		{
			infection.health_unit[slot] = unit_index;
			infection.health_maximum[slot] = unit->object.maximum_body_vitality;
			unit->object.maximum_body_vitality *= FLOODFECTION_ZOMBIE_HEALTH_SCALE;
		}
	}
	else if (infection.health_unit[slot] == unit_index)
	{
		unit->object.maximum_body_vitality = infection.health_maximum[slot];
		infection.health_unit[slot] = NONE;
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
	player->speed_multiplier = !team ? 1.0f :
		(infection_floodfection() ? FLOODFECTION_ZOMBIE_SPEED : options->infection_zombie_speed) / 100.0f;
	if (player->unit_index != NONE && (unit = unit_try_and_get(player->unit_index)) != NULL)
	{
		/* (Floodfection's zombies have none, whatever the option) */
		infection_apply_shields(DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index), player->unit_index, unit,
			team && (!options->infection_zombie_shields || infection_floodfection()));
		infection_apply_health(DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index), player->unit_index, unit,
			team && infection_floodfection());
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

/* how many players the round starts as zombies, of the players in the game
and the living drawn from: the gametype's count, or in Floodfection a tenth
of the players if more (rounded up: 11 players, 2), leaving a human */
#define FLOODFECTION_INITIAL_ZOMBIES_PERCENT 10

static long infection_initial_zombies(
	long players,
	long living)
{
	long zombies = game_variant_options_get()->infection_initial_zombies;

	if (infection_floodfection())
		zombies = MAX(zombies, (players * FLOODFECTION_INITIAL_ZOMBIES_PERCENT + 99) / 100);
	return MAX(0, MIN(living - 1, zombies));
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
	long initial_zombies;
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
	initial_zombies = infection_initial_zombies(humans + zombies, count);
	/* the first zombies, drawn without repeats, leaving a human */
	for (index = 0; index < initial_zombies; index++)
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
	platform_log("infection: active, %ld players, %ld zombies", count, initial_zombies);
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
	csmemset(infection.health_unit, 0xFF, sizeof(infection.health_unit));
	infection.state.winner = NONE;
	infection.state.game_number = network_game_get_game() ? network_game_get_number_of_games_played() : 0;
	infection.missing_skull = get_ball_definition_index() == NONE;
	infection.equipping = FALSE;
	floodfection_burst_reset();
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
	floodfection_bursts(player_index);
	if (infection_floodfection())
	{
		struct distributed_infection_player *role = infection_player(player_index);

		floodfection_forms_update(player_index, role && role->role == _distributed_infection_role_zombie);
	}
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
