/*
RENDER_PARTICLES.C

symbols in this file:
0017BD20 00b0:
	_local_player_is_first_person (0000)
0017BDD0 0030:
	_code_0017bdd0 (0000)
0017BE00 0490:
	_render_particles (0000)
0030E180 05f8:
	_data_0030e180 (0000)
*/

/* ---------- headers */

#include "camera/director.h"
#include "cseries/cseries.h"
#include "cseries/profile.h"
#include "game/players.h"
#include "tag_files/tag_groups.h"
#include "units/unit_definitions.h"
#include "units/units.h"

/* ---------- constants */

enum
{
	_director_perspective_first_person = 0,
};

/* ---------- macros */

/* ---------- structures */

struct rendered_particle
{
	short particle_index;
	short definition_index;
	short cluster_index;
	boolean attached_to_first_person_weapon;
	byte pad;
};

typedef char rendered_particle_size_assert[
	sizeof(struct rendered_particle) == 8 ? 1 : -1];

/* ---------- prototypes */

/* ---------- globals */

struct profile_section render_particles_section =
{
	"render_particles",
	NONE,
	TRUE,
};

/* ---------- public code */

boolean local_player_is_first_person(
	short local_player_index)
{
	boolean result =
		director_get_perspective(local_player_index) ==
		_director_perspective_first_person;

	if (!result)
	{
		long player_index = local_player_get_player_index(local_player_index);
		struct player_datum *player = player_get(player_index);

		if (player->unit_index != NONE)
		{
			struct unit_datum *unit = unit_get(player->unit_index);

			if (unit->object.parent_object_index != NONE &&
				unit->unit.parent_seat_index != NONE)
			{
				struct unit_datum *parent_unit =
					unit_get(unit->object.parent_object_index);
				struct unit_seat *seat = TAG_BLOCK_GET_ELEMENT(
					&unit_definition_get(parent_unit->definition_index)->unit.seats,
					unit->unit.parent_seat_index,
					struct unit_seat);

				if (TEST_FLAG(seat->flags, _unit_seat_gunner_bit))
					result = TRUE;
			}
		}
	}

	return result;
}

int __cdecl compare_rendered_particles(
	void const *left_pointer,
	void const *right_pointer)
{
	struct rendered_particle const *left = left_pointer;
	struct rendered_particle const *right = right_pointer;
	int result = left->definition_index - right->definition_index;

	if (!result)
	{
		result = left->cluster_index - right->cluster_index;
		if (!result)
		{
			result =
				left->attached_to_first_person_weapon -
				right->attached_to_first_person_weapon;
		}
	}

	return result;
}

/* ---------- private code */
