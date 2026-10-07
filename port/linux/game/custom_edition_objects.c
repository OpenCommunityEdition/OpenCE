/*
CUSTOM_EDITION_OBJECTS.C

The multiplayer vehicles of Halo Custom Edition maps (custom_edition_cache.h).

This build chooses a multiplayer game's vehicles by type: only the first
three of the globals' multiplayer vehicles are ever created, and of those
the ones the game variant's vehicle set names (game_engine_remap_vehicle).
Retail Halo, whose rules Custom Edition maps are made for, chooses them by
placement: a vehicle placement's multiplayer spawn flags name the game
types it is placed in by default, and a script may create any vehicle.
Custom Edition maps depend on that. hugeass.map places one jet by default
only in oddball, and its scripts recreate that jet while it exists and turn
the map to night when a biped under it dies; placed in a slayer game, the
jet is recreated on the biped and the map turns to night at once. So for a
Custom Edition map the native builds place the vehicles whose spawn flags
name the game type by default, and let a script create any vehicle, unless
the variant has no vehicles. Race has no spawn flag, and keeps this build's
rule.
*/

/* ---------- headers */

#include "cseries.h"
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
#include "game/game_engine.h"
#include "scenario/scenario_definitions.h"
#include "cache_file_formats.h"
#include "custom_edition_cache.h"

/* ---------- constants */

/* the game types, as game_engine_list.c orders their engines */
enum
{
	_game_engine_ctf = 1,
	_game_engine_slayer,
	_game_engine_oddball,
	_game_engine_king,
	_game_engine_race,
};

/* a variant's vehicle set that places no vehicle (game_engine_remap_vehicle) */
#define VEHICLE_SET_NONE 1

/* the game types a vehicle placement is placed in by default */
enum
{
	_multiplayer_spawn_slayer_default_bit = 0,
	_multiplayer_spawn_ctf_default_bit,
	_multiplayer_spawn_king_default_bit,
	_multiplayer_spawn_oddball_default_bit,
};

/* ---------- structures */

/* A vehicle placement: a scenario object, a unit's fields, and then the
vehicle's own, of which the multiplayer team and spawn flags come first.
OpenSauce (scenario_object_definitions_structures.hpp, s_scenario_vehicle)
leaves those 0x20 bytes unnamed; in hugeass.map the byte at 0x58 is 1 on
exactly the blue team's vehicles, and the word at 0x5A is 0x0303, 0x0404,
0x0808, 0x0F0F or 0 by vehicle, the game types' default and allowed bits
(docs/custom_edition_caches.md). */
struct custom_edition_vehicle_placement
{
	struct scenario_object_datum object;
	byte unit[0x30];
	char multiplayer_team_index;
	byte pad59;
	word multiplayer_spawn_flags;
	byte unused[0x1C];
};

typedef char verify_custom_edition_vehicle_placement_spawn_flags_offset[
	offsetof(struct custom_edition_vehicle_placement, multiplayer_spawn_flags) == 0x5A ? 1 : -1];
typedef char verify_custom_edition_vehicle_placement_size[
	sizeof(struct custom_edition_vehicle_placement) == 0x78 ? 1 : -1];

/* ---------- private code */

/* the spawn flag that places a vehicle by default in the running game type,
or NONE */
static short default_spawn_flag_bit(
	void)
{
	switch (game_engine_get_variant()->game_engine_index)
	{
	case _game_engine_slayer:
		return _multiplayer_spawn_slayer_default_bit;
	case _game_engine_ctf:
		return _multiplayer_spawn_ctf_default_bit;
	case _game_engine_king:
		return _multiplayer_spawn_king_default_bit;
	case _game_engine_oddball:
		return _multiplayer_spawn_oddball_default_bit;
	default:
		return NONE;
	}
}

/* ---------- public code */

boolean custom_edition_vehicles_by_placement(
	void)
{
	return custom_edition_cache_tags_loaded() &&
		game_engine_running() &&
		game_engine_get_variant()->universal_variant.vehicle_set != VEHICLE_SET_NONE &&
		default_spawn_flag_bit() != NONE;
}

boolean custom_edition_vehicle_placement_allowed(
	struct scenario_object_datum const *placement)
{
	struct custom_edition_vehicle_placement const *vehicle = (struct custom_edition_vehicle_placement const *)placement;

	return !custom_edition_vehicles_by_placement() ||
		TEST_FLAG(vehicle->multiplayer_spawn_flags, default_spawn_flag_bit());
}
