/*
MATERIAL_EFFECTS.C

symbols in this file:
0008DA20 0080:
	_material_effect_visible (0000)
0008DAA0 0140:
	_material_effect_new (0000)
0008DBE0 0110:
	_material_effect_new_from_point (0000)
0043D589 0001:
	_debug_material_effects (0000)
*/

/* ---------- headers */

#include "effects/material_effects.h"

#include "camera/observer.h"
#include "game/players.h"
#include "networking/network_connection.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

/* ---------- public code */

boolean material_effect_visible(
	real_point3d const *position)
{
	boolean visible = FALSE;
	short local_player_index;

	if (local_player_count() > 2)
	{
		visible = TRUE;
	}
	else
	{
		for (local_player_index = 0; local_player_index < MAXIMUM_NUMBER_OF_LOCAL_PLAYERS; local_player_index++)
		{
			if (local_player_get_player_index(local_player_index) != NONE)
			{
				struct observer_result const *camera = observer_get_camera(local_player_index);
				if (distance_squared3d(&camera->position, position) < 100.0f)
				{
					visible = TRUE;
					break;
				}
			}
		}
	}

	return visible;
}

/* ---------- private code */
