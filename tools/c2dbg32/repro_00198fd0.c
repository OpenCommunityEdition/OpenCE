/* Single-function isolation of code_00198fd0 for c2 debugging.
   Same include block as units.c so all types/offsets/enums are identical.
   Made public (non-static) so VC7 does not elide it. */

#include "cseries.h"
#include "units.h"

#include "bipeds.h"
#include "unit_definitions.h"
#include "biped_definitions.h"

#include "ai/actors.h"
#include "ai/ai_debug.h"
#include "bitmaps/bitmaps.h"
#include "cseries/errors.h"
#include "cseries/profile.h"
#include "effects/effects.h"
#include "game/cheats.h"
#include "game/game_globals.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "hs/object_lists.h"
#include "items/projectiles.h"
#include "items/weapon_definitions.h"
#include "items/weapons.h"
#include "main/console.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "objects/object_lights.h"
#include "physics/collision_bsp.h"
#include "physics/collision_models.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "physics/physics_definitions.h"
#include "scenario/scenario.h"
#include "saved games/game_state.h"
#include "sound/game_sound.h"

boolean repro_00198fd0(
	long unit_index,
	short animation_impulse)
{
	struct unit_datum *unit = unit_get(unit_index);
	boolean result;

	result = FALSE;

	switch (unit->unit.animation.state)
	{
	case _unit_state_hard_ping:
	case _unit_state_dying_airborne:
	case _unit_state_dying:
	case _unit_state_entering_seat:
	case _unit_state_exiting_seat:
	case _unit_state_ai_impulse:
	case _unit_state_melee_attack:
	case _unit_state_melee_airborne:
	case _unit_state_melee_continuous:
	case _unit_state_throw_grenade:
	case _unit_state_resurrect_front:
	case _unit_state_resurrect_back:
	case _unit_state_leap_start:
	case _unit_state_leap_melee:
		return FALSE;
	default:
		break;
	}

	if (unit->object.parent_object_index != NONE)
	{
		if (unit->unit.parent_seat_index != NONE)
		{
			struct unit_datum *parent = unit_try_and_get(unit->object.parent_object_index);

			if (parent)
			{
				struct unit_definition *parent_definition = unit_definition_get(parent->definition_index);
				struct unit_seat *unit_seat = TAG_BLOCK_GET_ELEMENT(
					&parent_definition->unit.seats,
					unit->unit.parent_seat_index,
					struct unit_seat);

				{
					long impulse = animation_impulse;

					if (impulse >= _unit_animation_impulse_celebrate &&
						impulse <= _unit_animation_impulse_panic)
					{
						result = (boolean)(unit_seat->flags >> _unit_seat_allows_vehicle_communication_animations_bit) & 1;
					}
				}
			}
		}
	}
	else
	{
		{
			long impulse = animation_impulse;

			if (impulse >= _unit_animation_impulse_celebrate &&
				impulse <= _unit_animation_impulse_panic)
			{
				return FALSE;
			}
		}

		return TRUE;
	}

	return result;
}
