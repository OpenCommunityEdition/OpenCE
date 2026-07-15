/*
PARTICLE_SYSTEMS.C

symbols in this file:
0008DCF0 0040:
	_particle_systems_initialize (0000)
0008DD30 0020:
	_particle_systems_initialize_for_new_map (0000)
0008DD50 0030:
	_particle_system_orphan (0000)
0008DD80 00b0:
	_code_0008dd80 (0000)
0008DE30 0010:
	_particle_systems_dispose (0000)
0008DE40 0010:
	_particle_systems_disconnect_from_structure_bsp (0000)
0008DE50 0140:
	_particle_systems_reconnect_to_structure_bsp (0000)
0008DF90 00b0:
	_code_0008df90 (0000)
0008E040 0090:
	_code_0008e040 (0000)
0008E0D0 0070:
	_code_0008e0d0 (0000)
0008E140 0040:
	_code_0008e140 (0000)
0008E180 0190:
	_code_0008e180 (0000)
0008E310 0030:
	_code_0008e310 (0000)
0008E340 0060:
	_particle_systems_dispose_from_old_map (0000)
0008E3A0 0350:
	_code_0008e3a0 (0000)
0008E6F0 0100:
	_code_0008e6f0 (0000)
0008E7F0 0680:
	_code_0008e7f0 (0000)
0008EE70 0550:
	_code_0008ee70 (0000)
0008F3C0 0110:
	_code_0008f3c0 (0000)
0008F4D0 0170:
	_code_0008f4d0 (0000)
0008F640 0120:
	_code_0008f640 (0000)
0008F760 0080:
	_particle_systems_update (0000)
0008F7E0 00a0:
	_particle_systems_render (0000)
0008F880 00d0:
	_particle_system_new_unattached (0000)
0008F950 01a0:
	_particle_system_new_attached (0000)
0025A6B8 0020:
	_rdata_0025a6b8 (0000)
	_ground_error (0018)
0025A6D8 001a:
	??_C@_0BK@BEJKBDOP@particle?5system?5particles?$AA@ (0000)
0025A6F4 0011:
	??_C@_0BB@DDOBODPO@particle?5systems?$AA@ (0000)
0025A708 0066:
	??_C@_0GG@NBDBKOPN@creation_function_index?$DO?$DN0?5?$CG?$CG?5cr@ (0000)
0025A770 0009:
	??_C@_08PKKGOGAD@particle?$AA@ (0000)
0025A77C 002a:
	??_C@_0CK@PCEEHMOG@c?3?2halo?2SOURCE?2effects?2particle_@ (0000)
0025A7A8 0092:
	??_C@_0JC@KDIJANOF@type_state_definition?9?$DOparticle_@ (0000)
0025A840 0081:
	??_C@_0IB@LHKMAGCA@system_definition?9?$DOsystem_update@ (0000)
0025A8C4 002c:
	??_C@_0CM@MKEIGADN@particle_systems?5?$CG?$CG?5particle_sys@ (0000)
*/

/* ---------- headers */

#include "effects/particle_systems.h"
#include "effects/particle_system_definitions.h"

#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "physics/point_physics.h"
#include "saved games/game_state.h"

/* ---------- constants */

enum
{
	MAXIMUM_PARTICLE_SYSTEMS = 64,
	PARTICLE_SYSTEM_DATUM_SIZE = 0x158,
	MAXIMUM_SYSTEM_PARTICLES = 512,
	SYSTEM_PARTICLE_DATUM_SIZE = 0x80,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

void code_0008dd80(
	long system_index);
void code_0008e0d0(
	struct particle_system_datum *system,
	real delta_time);

/* ---------- globals */

/* ---------- public code */

void particle_systems_initialize(
	void)
{
	particle_systems = game_state_data_new("particle systems", MAXIMUM_PARTICLE_SYSTEMS, PARTICLE_SYSTEM_DATUM_SIZE);
	system_particles = game_state_data_new("particle system particles", MAXIMUM_SYSTEM_PARTICLES, SYSTEM_PARTICLE_DATUM_SIZE);

	return;
}

void particle_systems_initialize_for_new_map(
	void)
{
	data_make_valid(particle_systems);
	data_make_valid(system_particles);

	return;
}

void particle_system_orphan(
	long system_index)
{
	struct particle_system_datum *system = particle_system_get(system_index);

	SET_FLAG(system->flags, _particle_system_attached_bit, FALSE);
	system->object_index = NONE;

	return;
}

void particle_systems_dispose_from_old_map(
	void)
{
	if (particle_systems && particle_systems->valid)
	{
		long system_index;

		for (system_index = data_next_index(particle_systems, NONE);
			system_index != NONE;
			system_index = data_next_index(particle_systems, system_index))
		{
			code_0008dd80(system_index);
		}

		data_make_invalid(particle_systems);
		data_make_invalid(system_particles);
	}

	return;
}

void particle_systems_dispose(
	void)
{
	return;
}

void particle_systems_disconnect_from_structure_bsp(
	void)
{
	return;
}

/* ---------- private code */

void code_0008e0d0(
	struct particle_system_datum *system,
	real delta_time)
{
	struct particle_system_definition *definition = particle_system_definition_get(system->definition_index);

	if (system->object_index == NONE && definition->point_physics_index != NONE)
	{
		point_physics_update(
			0,
			point_physics_definition_get(definition->point_physics_index),
			&system->location,
			NONE,
			&system->position,
			&system->velocity,
			NULL,
			NULL,
			NULL,
			1.0f,
			delta_time);
	}

	return;
}

void code_0008e140(
	struct particle_system_datum const *system,
	short type_index,
	struct system_particle_datum *particle,
	struct object_marker const *marker)
{
	particle->position = marker->matrix.position;
	particle->velocity = system->velocity;

	return;
}

void code_0008e310(
	struct particle_system_datum *system,
	real delta_time)
{
	particle_system_definition_get(system->definition_index);
	code_0008e0d0(system, delta_time);

	return;
}
