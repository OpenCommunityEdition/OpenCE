/* ---------- headers */

#include "cseries/cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/game_globals.h"
#include "game/game_allegiance.h"
#include "scenario/scenario.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

char const *global_material_type_strings[NUMBER_OF_MATERIAL_TYPES] =
{
	"dirt",
	"sand",
	"stone",
	"snow",
	"wood",
	"metal (hollow)",
	"metal (thin)",
	"metal (thick)",
	"rubber",
	"glass",
	"force field",
	"grunt",
	"hunter armor",
	"hunter skin",
	"elite",
	"jackal",
	"jackal energy shield",
	"engineer skin",
	"engineer force field",
	"flood combat form",
	"flood carrier form",
	"cyborg armor",
	"cyborg energy shield",
	"human armor",
	"human skin",
	"sentinel",
	"monitor",
	"plastic",
	"water",
	"leaves",
	"elite energy shield",
	"ice",
	"hunter shield",
};

short const global_difficulty_friend_settings[NUMBER_OF_GAME_DIFFICULTY_VALUES] =
{
	_game_difficulty_value_friend_damage,
	_game_difficulty_value_friend_vitality,
	_game_difficulty_value_friend_shield,
	_game_difficulty_value_friend_recharge,
	NONE, NONE, NONE, NONE, NONE, NONE,
	NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
	NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
	NONE, NONE, NONE, NONE, NONE,
};

/* ---------- public code */

char const *material_get_name(
	short material_type)
{
	if (material_type != NONE)
	{
		match_assert("c:\\halo\\SOURCE\\game\\game_globals.c", 0x389,
			material_type>=0 && material_type<NUMBER_OF_MATERIAL_TYPES);
		return global_material_type_strings[material_type];
	}

	return "NONE";
}

/* ---------- private code */

static real game_difficulty_get_value_by_difficulty(
	short value_type,
	short difficulty)
{
	real result = 1.0f;
	struct game_globals *game_globals = scenario_get_game_globals();
	struct game_globals_difficulty_information *difficulty_information;
	short difficulty_level;

	match_assert("c:\\halo\\SOURCE\\game\\game_globals.c", 0x39a,
		(value_type >= 0) && (value_type < NUMBER_OF_GAME_DIFFICULTY_VALUES));
	if (game_globals && game_globals->difficulty_information.count)
	{
		difficulty_information = TAG_BLOCK_GET_ELEMENT(
			&game_globals->difficulty_information,
			0,
			struct game_globals_difficulty_information);
		if (difficulty_information)
		{
			if (difficulty < 0)
				difficulty_level = 0;
			else
				difficulty_level = MIN(difficulty, NUMBER_OF_GAME_DIFFICULTY_LEVELS - 1);

			result = difficulty_information->values[value_type][difficulty_level];
		}
	}

	return result;
}

real game_difficulty_get_value(
	short value_type)
{
	return game_difficulty_get_value_by_difficulty(value_type, game_difficulty_level_get());
}

real game_difficulty_get_team_value(
	short value_type,
	short team_index)
{
	short difficulty = game_difficulty_level_get();
	short friend_value_type;

	if (game_engine_running())
		difficulty = _game_difficulty_level_normal;
	else if (!game_team_is_enemy(_game_team_player, team_index))
	{
		match_assert("c:\\halo\\SOURCE\\game\\game_globals.c", 0x3bd,
			(value_type >= 0) && (value_type < NUMBER_OF_GAME_DIFFICULTY_VALUES));
		friend_value_type = global_difficulty_friend_settings[value_type];
		if (friend_value_type == NONE)
			return game_difficulty_get_value_by_difficulty(value_type, _game_difficulty_level_normal);

		return game_difficulty_get_value_by_difficulty(friend_value_type, difficulty);
	}

	return game_difficulty_get_value_by_difficulty(value_type, difficulty);
}
