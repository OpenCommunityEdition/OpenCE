/*
GAME_SOUND.C

symbols in this file:
001B6B20 0030:
	_game_sound_initialize (0000)
001B6B50 0020:
	_game_sound_dispose (0000)
001B6B70 0030:
	_game_sound_initialize_for_new_map (0000)
001B6BA0 00b0:
	_game_sound_clear (0000)
001B6C50 00d0:
	_game_sound_restore (0000)
001B6D20 0100:
	_game_looping_sound_new (0000)
001B6E20 00a0:
	_game_looping_sound_delete (0000)
001B6EC0 00b0:
	_unattached_impulse_sound_new (0000)
001B6F70 0080:
	_unspatialized_impulse_sound_new (0000)
001B6FF0 0050:
	_scripted_sound_time (0000)
001B7040 0050:
	_scripted_sound_stop (0000)
001B7090 00b0:
	_scripted_foley_predict (0000)
001B7140 0070:
	_scripted_looping_sound_set_scale (0000)
001B71B0 0050:
	_scripted_looping_sound_set_alternate (0000)
001B7200 0060:
	_unattached_looping_sound_start (0000)
001B7260 0030:
	_unattached_looping_sound_stop (0000)
001B7290 0270:
	_code_001b7290 (0000)
001B7500 00f0:
	_track_object_impulse_sound (0000)
001B75F0 0040:
	_game_sound_set_mouth_aperture (0000)
001B7630 00f0:
	_code_001b7630 (0000)
001B7720 0070:
	_code_001b7720 (0000)
001B7790 0070:
	_code_001b7790 (0000)
001B7800 0060:
	_code_001b7800 (0000)
001B7860 00d0:
	_code_001b7860 (0000)
001B7930 0030:
	_game_sound_dispose_from_old_map (0000)
001B7960 0110:
	_object_impulse_sound_new (0000)
001B7A70 0160:
	_scripted_sound_new (0000)
001B7BD0 0060:
	_scripted_looping_sound_stop (0000)
001B7C30 01d0:
	_game_sound_update (0000)
001B7E00 0200:
	_compute_sound_obstruction (0000)
001B8000 0090:
	_scripted_looping_sound_start (0000)
002AA9B4 0013:
	??_C@_0BD@LHEMKJEF@game?5sound?5globals?$AA@ (0000)
002AA9C8 0016:
	??_C@_0BG@BOAENECE@object?5looping?5sounds?$AA@ (0000)
002AA9E0 003a:
	??_C@_0DK@LEFFPEOA@TEST_FLAG?$CIsound?9?$DOflags?0?5_game_lo@ (0000)
002AAA1C 0022:
	??_C@_0CC@BGEOADFC@c?3?2halo?2SOURCE?2sound?2game_sound?4@ (0000)
002AAA40 000c:
	??_C@_0M@GMDOBMHG@marker_name?$AA@ (0000)
002AAA50 0075:
	??_C@_0HF@HCOEOKBN@definition?9?$DOruntime_scripting_so@ (0000)
002AAAC8 003f:
	??_C@_0DP@OPHINILO@definition?9?$DOruntime_scripting_so@ (0000)
002AAB08 0019:
	??_C@_0BJ@CPMEKCKK@scale?$DO?$DN0?4f?5?$CG?$CG?5scale?$DM?$DN1?4f?$AA@ (0000)
002AAB28 0044:
	??_C@_0EE@BEMDGHLB@TEST_FLAG?$CIdefinition?9?$DOflags?0?5_lo@ (0000)
002AAB6C 0010:
	??_C@_0BA@EPHFINEK@attachment_data?$AA@ (0000)
002AAB80 0064:
	??_C@_0GE@GAKNCJMB@location?9?$DOcluster_index?$DO?$DNNONE?5?$CG?$CG@ (0000)
002AABE4 0014:
	??_C@_0BE@DCLHABOE@position?5?$CG?$CG?5forward?$AA@ (0000)
002AABF8 0041:
	??_C@_0EB@JEFKCLHK@source?9?$DOspatialization_mode?$DN?$DN_so@ (0000)
002AAC3C 0030:
	??_C@_0DA@LJCJMFEJ@definition?9?$DOruntime_scripting_so@ (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "game/game.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "saved games/game_state.h"
#include "sound/game_sound.h"
#include "sound/sound_definitions.h"
#include "sound/sound_manager.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	MAXIMUM_GAME_LOOPING_SOUNDS = 1024,
	_game_looping_sound_unattached_stop_bit = 1,
	_game_looping_sound_unattached_stop_fixed_fadeout_bit = 2,
	_game_looping_sound_alternate_bit = 3,
	_game_looping_sound_scripted_bit = 4,
};

/* ---------- macros */

/* ---------- structures */

struct game_looping_sound_datum
{
	struct datum_header header;
	short unknown2;
	unsigned long flags;
	real scale;
	long definition_index;
	long object_index;
	long sound_index;
	short function_index;
	short node_index;
	real_point3d position;
	real_vector3d forward;
};

struct game_sound_globals
{
	long update_index;
	long music_looping_sound_index;
};

typedef char game_looping_sound_datum_size_assert[
	sizeof(struct game_looping_sound_datum) == 0x34 ? 1 : -1];
typedef char game_sound_globals_size_assert[
	sizeof(struct game_sound_globals) == 0x8 ? 1 : -1];

/* ---------- prototypes */

static void scripted_looping_sound_stop_internal(
	long sound_index,
	boolean fixed_fadeout);

/* ---------- globals */

struct game_sound_globals *game_sound_globals;
struct data_array *game_looping_sound_data;

/* ---------- public code */

void game_sound_initialize(
	void)
{
	game_looping_sound_data = game_state_data_new(
		"object looping sounds",
		MAXIMUM_GAME_LOOPING_SOUNDS,
		sizeof(struct game_looping_sound_datum));
	game_sound_globals = (struct game_sound_globals *)game_state_malloc(
		"game sound globals",
		NULL,
		sizeof(*game_sound_globals));

	return;
}

void game_sound_dispose(
	void)
{
	if (game_looping_sound_data)
	{
		game_looping_sound_data = NULL;
	}

	return;
}

void game_sound_initialize_for_new_map(
	void)
{
	if (game_looping_sound_data)
	{
		data_make_valid(game_looping_sound_data);
		game_sound_globals->music_looping_sound_index = NONE;
		game_sound_globals->update_index = 0;
	}

	return;
}

void game_sound_dispose_from_old_map(
	void)
{
	if (game_looping_sound_data && game_looping_sound_data->valid)
	{
		game_sound_clear();
		data_make_invalid(game_looping_sound_data);
	}

	return;
}

long scripted_sound_time(
	long sound_index)
{
	struct sound_definition *definition;
	long time = 0;

	if (sound_index != NONE)
	{
		definition = sound_definition_get(sound_index);
		if (definition->scripting_time != NONE)
		{
			long remaining = definition->scripting_time - game_time_get();

			time = MAX(remaining, 0);
		}
	}

	return time;
}

void scripted_sound_stop(
	long sound_index)
{
	struct sound_definition *definition;

	if (sound_index != NONE)
	{
		definition = sound_definition_get(sound_index);
		if (definition->scripting_sound_index != NONE)
		{
			sound_stop_impulse(definition->scripting_sound_index);
			definition->scripting_sound_index = NONE;
			definition->scripting_time = NONE;
		}
	}

	return;
}

void scripted_looping_sound_set_scale(
	long sound_index,
	real scale)
{
	struct looping_sound_definition *definition;
	struct game_looping_sound_datum *looping_sound;

	if (sound_index != NONE)
	{
		definition = looping_sound_definition_get(sound_index);
		if (definition->scripting_sound_index != NONE)
		{
			looping_sound = datum_get(
				game_looping_sound_data,
				definition->scripting_sound_index);
			looping_sound->scale = PIN(scale, 0.0f, 1.0f);
		}
	}

	return;
}

void scripted_looping_sound_set_alternate(
	long sound_index,
	boolean alternate)
{
	struct looping_sound_definition *definition;
	struct game_looping_sound_datum *looping_sound;

	if (sound_index != NONE)
	{
		definition = looping_sound_definition_get(sound_index);
		if (definition->scripting_sound_index != NONE)
		{
			looping_sound = datum_get(
				game_looping_sound_data,
				definition->scripting_sound_index);
			SET_FLAG(looping_sound->flags, _game_looping_sound_alternate_bit, alternate);
		}
	}

	return;
}

void unattached_looping_sound_stop(
	long looping_sound_index)
{
	struct game_looping_sound_datum *looping_sound = datum_get(
		game_looping_sound_data,
		looping_sound_index);

	SET_FLAG(looping_sound->flags, _game_looping_sound_unattached_stop_bit, TRUE);

	return;
}

static void scripted_looping_sound_stop_internal(
	long sound_index,
	boolean fixed_fadeout)
{
	struct looping_sound_definition *definition;
	struct game_looping_sound_datum *looping_sound;

	if (sound_index != NONE)
	{
		definition = looping_sound_definition_get(sound_index);
		if (definition->scripting_sound_index != NONE)
		{
			looping_sound = datum_get(
				game_looping_sound_data,
				definition->scripting_sound_index);
			SET_FLAG(looping_sound->flags, _game_looping_sound_scripted_bit, FALSE);
			unattached_looping_sound_stop(definition->scripting_sound_index);
			definition->scripting_sound_index = NONE;
			if (fixed_fadeout)
			{
				SET_FLAG(
					looping_sound->flags,
					_game_looping_sound_unattached_stop_fixed_fadeout_bit,
					TRUE);
			}
		}
	}

	return;
}

void scripted_looping_sound_stop(
	long sound_index)
{
	scripted_looping_sound_stop_internal(sound_index, FALSE);

	return;
}

long unattached_looping_sound_start(
	long definition_index,
	long period,
	real scale)
{
	long looping_sound_index;
	struct game_looping_sound_datum *looping_sound;

	looping_sound_definition_get(definition_index);
	looping_sound_index = game_looping_sound_new(
		period,
		definition_index,
		"",
		NONE);
	if (looping_sound_index != NONE)
	{
		looping_sound = datum_get(
			game_looping_sound_data,
			looping_sound_index);
		SET_FLAG(looping_sound->flags, 0, TRUE);
		looping_sound->scale = scale;
	}

	return looping_sound_index;
}

void game_sound_set_mouth_aperture(
	long object_index,
	real mouth_aperture)
{
	if (game_looping_sound_data->valid)
	{
		if (object_try_and_get_and_verify_type(object_index, _object_mask_unit))
		{
			unit_set_mouth_aperture(object_index, mouth_aperture);
		}
	}

	return;
}

/* ---------- private code */
