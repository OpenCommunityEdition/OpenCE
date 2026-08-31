/*
PARTICLES.C

symbols in this file:
0008FAF0 0030:
	_particles_initialize (0000)
0008FB20 0010:
	_particles_initialize_for_new_map (0000)
0008FB30 0010:
	_particles_dispose_from_old_map (0000)
0008FB40 0020:
	_particles_dispose (0000)
0008FB60 0020:
	_particle_delete (0000)
0008FB80 0070:
	_particles_stop_on_first_person_weapon (0000)
0008FBF0 0010:
	_particles_disconnect_from_structure_bsp (0000)
0008FC00 00d0:
	_particles_reconnect_to_structure_bsp (0000)
0008FCD0 0010:
	_code_0008fcd0 (0000)
0008FCE0 0040:
	_particle_get_radius (0000)
0008FD20 0060:
	_valid_real_point3d (0000)
0008FD80 0060:
	_valid_real_argb_color (0000)
0008FDE0 0150:
	_code_0008fde0 (0000)
0008FF30 0050:
	_code_0008ff30 (0000)
0008FF80 0180:
	_code_0008ff80 (0000)
00090100 00d0:
	_code_00090100 (0000)
000901D0 00d0:
	_code_000901d0 (0000)
000902A0 03a0:
	_code_000902a0 (0000)
00090640 05a0:
	_particle_new (0000)
00090BE0 0120:
	_particles_update (0000)
0025A8F0 0008:
	??_C@_07GFBFDLBM@gravity?$AA@ (0000)
0025A8F8 0008:
	_rdata_0025a8f8 (0000)
0025A900 0011:
	??_C@_0BB@HHDONMHD@particles_update?$AA@ (0000)
0025A914 0023:
	??_C@_0CD@JPDMOBBK@couldn?8t?5allocate?5particle?5globa@ (0000)
0025A938 0023:
	??_C@_0CD@IKBENHKF@c?3?2halo?2SOURCE?2effects?2particles@ (0000)
0025A95C 0009:
	??_C@_08HIJBMAOA@?$CGdiffuse?$AA@ (0000)
0025A968 0007:
	??_C@_06IOIMBPOK@?$CGlight?$AA@ (0000)
0025A970 0031:
	??_C@_0DB@EEHLCDHM@?$CFs?3?5assert_valid_real_argb_color@ (0000)
0025A9A4 000d:
	??_C@_0N@MNPLGION@?$CGdata?9?$DOcolor?$AA@ (0000)
0025A9B4 002a:
	??_C@_0CK@EAOMHCPC@?$CFs?3?5assert_valid_real_point3d?$CI?$CFf@ (0000)
0025A9E0 0010:
	??_C@_0BA@IFGIHPJG@?$CGdata?9?$DOposition?$AA@ (0000)
0025A9F0 002b:
	??_C@_0CL@HMPNAHGK@?$CFs?3?5assert_valid_real_vector2d?$CI?$CF@ (0000)
0025AA1C 0010:
	??_C@_0BA@OHENJGHL@?$CGdata?9?$DOvelocity?$AA@ (0000)
002DD7A0 0600:
	_data_002dd7a0 (0000)
*/

/* ---------- headers */

#define valid_real_point3d valid_real_point3d_inline
#include "effects/particles.h"
#undef valid_real_point3d

#include "bitmaps/bitmaps.h"
#include "cseries/errors.h"
#include "interface/first_person_weapons.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "saved games/game_state.h"
#include "scenario/scenario.h"

/* ---------- constants */

enum particle_private_abi_offsets
{
	_particle_datum_definition_index_offset = offsetof(struct particle_datum, definition_index),
	_particle_definition_effect_group_tag_offset = offsetof(struct particle_definition, effect) + offsetof(struct tag_reference, group_tag),
	_particle_definition_effect_index_offset = offsetof(struct particle_definition, effect) + offsetof(struct tag_reference, index)
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

// particle is passed in eax, group tag in ecx, and effect index in esi
void code_0008fde0(
	long parameter);

/* ---------- globals */

/* ---------- public code */

void particles_initialize(
	void)
{
	particle_data = game_state_data_new("particle", 1024, 0x70);
	if (!particle_data)
		error(_error_immediate, "couldn't allocate particle globals");

	return;
}

void particles_initialize_for_new_map(
	void)
{
	data_make_valid(particle_data);

	return;
}

void particles_dispose_from_old_map(
	void)
{
	data_make_invalid(particle_data);

	return;
}

void particles_dispose(
	void)
{
	if (particle_data)
		particle_data = NULL;

	return;
}

void particle_delete(
	long particle_index)
{
	datum_delete(particle_data, particle_index);

	return;
}

void particles_stop_on_first_person_weapon(
	short local_player_index)
{
	long particle_index;

	for (particle_index = data_next_index(particle_data, NONE);
		particle_index != NONE;
		particle_index = data_next_index(particle_data, particle_index))
	{
		struct particle_datum *particle = particle_get(particle_index);

		if (particle->local_player_index == local_player_index &&
			TEST_FLAG(particle->flags, _particle_datum_attached_to_local_player_bit) &&
			particle->object_index != NONE)
		{
			datum_delete(particle_data, particle_index);
		}
	}

	return;
}

void particles_disconnect_from_structure_bsp(
	void)
{
	return;
}

void particles_reconnect_to_structure_bsp(
	void)
{
	long particle_index;

	for (particle_index = data_next_index(particle_data, NONE);
		particle_index != NONE;
		particle_index = data_next_index(particle_data, particle_index))
	{
		struct particle_datum *particle = particle_get(particle_index);
		real_point3d const *position;

		if (particle->object_index == NONE)
		{
			position = &particle->position;
		}
		else if (TEST_FLAG(particle->flags, _particle_datum_attached_to_local_player_bit))
		{
			position = &first_person_weapon_get_node_matrix(
				particle->local_player_index,
				particle->node_index)->position;
		}
		else if (object_try_and_get(particle->object_index))
		{
			position = &object_get_node_matrix(
				particle->object_index,
				particle->node_index)->position;
		}
		else
		{
			position = NULL;
		}

		if (!position)
		{
			particle_delete(particle_index);
		}
		else
		{
			scenario_location_from_point(&particle->location, position);
			if (particle->location.cluster_index == NONE)
			{
				particle_delete(particle_index);
			}
		}
	}

	return;
}

// struct location const *location is passed in eax
__declspec(naked) boolean code_0008fcd0(
	void)
{
	__asm
	{
		push eax
		call scenario_location_potentially_visible_local
		add esp, 4
		test al, al
		setne al
		ret
	}
}

real particle_get_radius(
	long particle_index)
{
	struct particle_datum *particle = particle_get(particle_index);
	struct particle_definition *definition = particle_definition_get(particle->definition_index);

	return ((definition->radius_upper_bound - definition->radius_lower_bound) *
			(particle->age / particle->lifespan) + definition->radius_lower_bound) *
		particle->radius;
}

boolean valid_real_point3d(
	real_point3d const *point)
{
	return valid_real(point->x) && valid_real(point->y) && valid_real(point->z);
}

boolean valid_real_argb_color(
	real_argb_color const *color)
{
	return valid_real(color->alpha) &&
		color->alpha >= 0.0f &&
		color->alpha <= 1.0f &&
		valid_real_rgb_color(&color->rgb);
}

// long particle_index is passed in ebx
__declspec(naked) void code_0008ff30(
	void)
{
	__asm
	{
		mov eax, particle_data
		push esi
		push edi
		push ebx
		push eax
		call datum_get
		mov edi, eax
		mov ecx, [edi + _particle_datum_definition_index_offset]
		push ecx
		push PARTICLE_TAG
		call tag_get
		mov esi, [eax + _particle_definition_effect_index_offset]
		add esp, 16
		cmp esi, NONE
		je delete_particle
		mov ecx, [eax + _particle_definition_effect_group_tag_offset]
		push FALSE
		mov eax, edi
		call code_0008fde0
		add esp, 4
	delete_particle:
		mov edx, particle_data
		push ebx
		push edx
		call datum_delete
		add esp, 8
		pop edi
		pop esi
		ret
	}
}

/* ---------- private code */
