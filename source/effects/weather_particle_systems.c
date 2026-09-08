/*
WEATHER_PARTICLE_SYSTEMS.C

symbols in this file:
000924D0 0040:
	_code_000924d0 (0000)
00092510 0050:
	_code_00092510 (0000)
00092560 0030:
	_weather_particle_systems_initialize (0000)
00092590 0070:
	_weather_particle_systems_initialize_for_new_map (0000)
00092600 0020:
	_weather_particle_systems_dispose_from_old_map (0000)
00092620 0020:
	_weather_particle_systems_dispose (0000)
00092640 0030:
	_code_00092640 (0000)
00092670 00a0:
	_code_00092670 (0000)
00092710 0160:
	_weather_particle_system_new (0000)
00092870 0110:
	_weather_particle_system_delete (0000)
00092980 02c0:
	_code_00092980 (0000)
00092C40 0040:
	_code_00092c40 (0000)
00092C80 02b0:
	_code_00092c80 (0000)
00092F30 0100:
	_code_00092f30 (0000)
00093030 0040:
	_code_00093030 (0000)
00093070 00b0:
	_code_00093070 (0000)
00093120 0130:
	_code_00093120 (0000)
00093250 0240:
	_code_00093250 (0000)
00093490 0690:
	_code_00093490 (0000)
00093B20 00e0:
	_weather_particle_systems_render (0000)
0025AAFC 0004:
	_one_over_char_max (0000)
0025AB00 0032:
	??_C@_0DC@LLJNAPDG@c?3?2halo?2SOURCE?2effects?2weather_p@ (0000)
0025AB34 003d:
	??_C@_0DN@CIHCKGAO@type_index?$DO?$DN0?5?$CG?$CG?5type_index?$DMdefi@ (0000)
0025AB74 0033:
	??_C@_0DD@FLIDKJLH@couldn?8t?5allocate?5weather?5partic@ (0000)
0025ABA8 0012:
	??_C@_0BC@EPMBJEML@weather?5particles?$AA@ (0000)
0025ABBC 001f:
	??_C@_0BP@EMHAEIEA@system?9?$DOdefinition_index?$DN?$DNNONE?$AA@ (0000)
0025ABDC 0024:
	??_C@_0CE@MLAKBDEP@too?5many?5weather?5polyhedra?5visib@ (0000)
0025AC00 003b:
	??_C@_0DL@JLJDEDI@box_count?$DMMAXIMUM_NUMBER_OF_VISI@ (0000)
002DDDAE 0001:
	_weather (0000)
0043D590 0274:
	_bss_0043d590 (0000)
*/

/* ---------- headers */

#include "effects/weather_particle_systems.h"

#include "cseries/errors.h"
#include "effects/weather_particle_definitions.h"
#include "networking/network_connection.h"
#include "objects/objects.h"

/* ---------- constants */

enum
{
	MAXIMUM_NUMBER_OF_WEATHER_PARTICLES = 512,
	MAXIMUM_NUMBER_OF_WEATHER_PARTICLE_TYPES = 8,
};

/* ---------- macros */

/* ---------- structures */

struct weather_particle_type
{
	real density;
	real box_width;
	short particle_count;
	word pad0A;
	long first_particle_index;
};

struct weather_particle_system
{
	long definition_index;
	real time;
	real time_delta_sec;
	real scale;
	struct location location;
	short weather_palette_index;
	boolean under_water;
	byte pad1B;
	struct weather_particle_type types[MAXIMUM_NUMBER_OF_WEATHER_PARTICLE_TYPES];
};

struct weather_particle
{
	short identifier;
	word pad02;
	real_point3d position;
	real_vector3d velocity;
	real_vector3d acceleration;
	short sequence_index;
	word pad2A;
	real sprite_index;
	real rotation;
	real_argb_color color;
	real radius;
	real rotation_rate;
	real animation_rate;
	long next_particle_index;
};

struct weather_particle_system_globals
{
	short active_system_count;
	word pad02;
	struct weather_particle_system systems[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];
};

typedef char weather_particle_type_size_assert[
	sizeof(struct weather_particle_type) == 0x10 ? 1 : -1];
typedef char weather_particle_system_size_assert[
	sizeof(struct weather_particle_system) == 0x9C ? 1 : -1];
typedef char weather_particle_size_assert[
	sizeof(struct weather_particle) == 0x54 ? 1 : -1];
typedef char weather_particle_system_globals_size_assert[
	sizeof(struct weather_particle_system_globals) == 0x274 ? 1 : -1];

/* ---------- prototypes */

static struct weather_particle_system *weather_particle_system_get(
	short local_player_index);
static struct weather_particle_type *weather_particle_system_get_type(
	struct weather_particle_system *system,
	short type_index);
static void weather_particle_system_type_delete_particle(
	struct weather_particle_type *type);

/* ---------- globals */

static struct weather_particle_system_globals weather_particle_system_globals;

/* ---------- public code */

void weather_particle_systems_initialize(
	void)
{
	weather_particle_data = data_new("weather particles", MAXIMUM_NUMBER_OF_WEATHER_PARTICLES, sizeof(struct weather_particle));
	if (!weather_particle_data)
		error(_error_immediate, "couldn't allocate weather particle system globals.");

	return;
}

void weather_particle_systems_initialize_for_new_map(
	void)
{
	short local_player_index;

	for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
	{
		match_assert("c:\\halo\\SOURCE\\effects\\weather_particle_systems.c", 91, local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);
		weather_particle_system_globals.systems[local_player_index].definition_index = NONE;
	}

	weather_particle_system_globals.active_system_count = 0;
	data_make_valid(weather_particle_data);

	return;
}

void weather_particle_systems_dispose_from_old_map(
	void)
{
	if (weather_particle_data->valid)
		data_make_invalid(weather_particle_data);

	return;
}

void weather_particle_systems_dispose(
	void)
{
	if (weather_particle_data)
	{
		data_dispose(weather_particle_data);
		weather_particle_data = NULL;
	}

	return;
}

void weather_particle_system_delete(
	short local_player_index)
{
	struct weather_particle_system *system = weather_particle_system_get(local_player_index);
	struct weather_particle_system_definition *definition = weather_particle_system_definition_get(system->definition_index);
	short type_index;

	for (type_index = 0; type_index<definition->particle_types.count; type_index++)
	{
		struct weather_particle_type *type = weather_particle_system_get_type(system, type_index);

		while (type->first_particle_index!=NONE)
		{
			weather_particle_system_type_delete_particle(type);
		}
	}

	weather_particle_system_globals.active_system_count-= 1;
	system->definition_index = NONE;

	return;
}

/* ---------- private code */

static struct weather_particle_system *weather_particle_system_get(
	short local_player_index)
{
	match_assert("c:\\halo\\SOURCE\\effects\\weather_particle_systems.c", 91, local_player_index>=0 && local_player_index<MAXIMUM_NUMBER_OF_LOCAL_PLAYERS);

	return &weather_particle_system_globals.systems[local_player_index];
}

static struct weather_particle_type *weather_particle_system_get_type(
	struct weather_particle_system *system,
	short type_index)
{
	struct weather_particle_system_definition *definition = weather_particle_system_definition_get(system->definition_index);

	match_assert("c:\\halo\\SOURCE\\effects\\weather_particle_systems.c", 102, type_index>=0 && type_index<definition->particle_types.count);

	return &system->types[type_index];
}

static void weather_particle_system_type_delete_particle(
	struct weather_particle_type *type)
{
	struct weather_particle *particle = datum_get(weather_particle_data, type->first_particle_index);
	long next_particle_index = particle->next_particle_index;

	datum_delete(weather_particle_data, type->first_particle_index);
	type->particle_count-= 1;
	type->first_particle_index = next_particle_index;

	return;
}
