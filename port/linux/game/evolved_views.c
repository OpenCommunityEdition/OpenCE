/* One local player can watch living teammates in an evolved team game.
The extra panes are render windows only. They are not local players, so
input, scoring and the single-player HUD stay with the one player. The
menu option display.teammate_views is off until chosen. Which pane is the
local player's is display.teammate_view_slot. A teammate pane borrows a
spare HUD slot for that one window, then gives it back, so their weapon,
shield and reticle draw without a second local player. */
#include "cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "game/players.h"
#include "camera/observer.h"
#include "interface/first_person_weapons.h"
#include "interface/hud_weapon.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "objects/objects.h"
#include "units/units.h"
#include "evolved_rules.h"
#include "evolved_views.h"
#include "port_config.h"
#include <string.h>

/* the drawn body is between ticks; the sight point is the tick's */
real_matrix4x3 *render_interpolation_object_node_matrices(long object_index);

#define EVOLVED_VIEW_MAXIMUM 4

/* slot 0 is the real local player */
#define EVOLVED_VIEWS_HUD_SLOT 1

static long view_unit[EVOLVED_VIEW_MAXIMUM];
static short view_count = 1;
static short local_slot = 0;
static short view_rendering = 0;
static boolean view_hiding = FALSE;
static boolean view_borrowed = FALSE;

/* 2 panes are left and right. 3 give the wide top to either top choice and
the two bottoms to the bottom choices. 4 are the four corners. */
static short evolved_views_slot(short count)
{
	char const *name = config_string("display.teammate_view_slot");
	boolean right = !strcmp(name, "top_right") || !strcmp(name, "bottom_right");
	boolean bottom = !strcmp(name, "bottom_left") || !strcmp(name, "bottom_right");
	short slot;

	if (count <= 2)
		slot = right ? 1 : 0;
	else if (count == 3)
		slot = bottom ? (right ? 2 : 1) : 0;
	else
		slot = (short)((bottom ? 2 : 0) + (right ? 1 : 0));
	if (slot >= count)
		slot = 0;
	return slot;
}

static void evolved_views_refresh(void)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *local;
	long local_index;
	long mates[EVOLVED_VIEW_MAXIMUM];
	short mate_count;
	short index;
	short placed;

	view_count = 1;
	local_slot = 0;
	view_unit[0] = NONE;
	if (!config_boolean("display.teammate_views") || !evolved_rules_active() ||
		!game_in_progress() || !game_engine_running() || !game_engine_has_teams() ||
		local_player_count() != 1 || !player_data || !player_data->valid)
		return;
	local_index = local_player_get_player_index(0);
	local = player_try_and_get(local_index);
	if (!local || local->team_index < 0)
		return;
	mate_count = 0;
	data_iterator_new(&iterator, player_data);
	while (mate_count < EVOLVED_VIEW_MAXIMUM - 1 &&
		(player = data_iterator_next(&iterator)) != NULL)
	{
		struct object_datum *body;

		if (iterator.datum_index == local_index || player->quit_out_of_game ||
			player->team_index != local->team_index || player->unit_index == NONE)
			continue;
		body = object_try_and_get(player->unit_index);
		if (!body || TEST_FLAG(body->object.damage_flags, _object_dead_bit))
			continue;
		mates[mate_count++] = player->unit_index;
	}
	if (!mate_count)
		return;
	view_count = (short)(mate_count + 1);
	local_slot = evolved_views_slot(view_count);
	for (index = 0; index < EVOLVED_VIEW_MAXIMUM; index++)
		view_unit[index] = NONE;
	placed = 0;
	for (index = 0; index < view_count; index++)
	{
		if (index == local_slot)
			continue;
		view_unit[index] = mates[placed++];
	}
}

short evolved_views_window_count(void)
{
	evolved_views_refresh();
	return view_count;
}

boolean evolved_views_is_teammate_window(short window_index)
{
	return window_index >= 0 && window_index < view_count &&
		view_unit[window_index] != NONE;
}

boolean evolved_views_is_local_window(short window_index)
{
	return view_count > 1 && window_index == local_slot;
}

short evolved_views_hud_slot(void)
{
	return EVOLVED_VIEWS_HUD_SLOT;
}

boolean evolved_views_camera(short window_index, struct observer_result *result)
{
	long unit_index;
	struct object_datum *object;
	real_vector3d forward;
	real_matrix4x3 *simulated;
	real_matrix4x3 *drawn;
	real length;

	if (!result || !evolved_views_is_teammate_window(window_index))
		return FALSE;
	unit_index = view_unit[window_index];
	object = object_try_and_get(unit_index);
	if (!object)
		return FALSE;
	csmemset(result, 0, sizeof(*result));
	unit_get_camera_position(unit_index, &result->position);
	unit_get_looking_vector(unit_index, &forward);
	length = forward.i * forward.i + forward.j * forward.j + forward.k * forward.k;
	if (!(length > 0.0001f))
		forward = object->object.forward;
	result->forward = forward;
	observer_up_from_forward(&result->forward, &result->up);
	/* the stock unzoomed view. display.fov still applies, and the local
	player's own zoom does not */
	result->field_of_view = DEGREES_TO_RADIANS(80.0f);
	simulated = object_get_node_matrices(unit_index);
	drawn = render_interpolation_object_node_matrices(unit_index);
	if (simulated && drawn)
	{
		result->position.x += drawn[0].position.x - simulated[0].position.x;
		result->position.y += drawn[0].position.y - simulated[0].position.y;
		result->position.z += drawn[0].position.z - simulated[0].position.z;
	}
	return TRUE;
}

void evolved_views_begin(short window_index)
{
	long player_index;

	view_hiding = TRUE;
	view_rendering = window_index;
	view_borrowed = FALSE;
	if (!evolved_views_is_teammate_window(window_index))
		return;
	player_index = player_index_from_unit_index(view_unit[window_index]);
	if (player_index == NONE)
		return;
	/* the HUD and the first-person weapon look up a local player. slot 1
	is free while one person plays, and it is put back when the pane ends.
	the pane's camera stays the unzoomed view, so this slot must not report
	the teammate's zoom */
	local_player_set_player_index(EVOLVED_VIEWS_HUD_SLOT, player_index);
	player_control_new_unit(EVOLVED_VIEWS_HUD_SLOT, NONE);
	first_person_weapon_observe_unit(EVOLVED_VIEWS_HUD_SLOT, view_unit[window_index]);
	hud_weapon_port_update_player(EVOLVED_VIEWS_HUD_SLOT);
	view_borrowed = TRUE;
}

void evolved_views_end(void)
{
	if (view_borrowed)
	{
		first_person_weapon_observe_clear(EVOLVED_VIEWS_HUD_SLOT);
		player_control_new_unit(EVOLVED_VIEWS_HUD_SLOT, NONE);
		local_player_set_player_index(EVOLVED_VIEWS_HUD_SLOT, NONE);
		view_borrowed = FALSE;
	}
	view_hiding = FALSE;
}

boolean evolved_views_hides_unit(long object_index)
{
	return view_hiding && view_rendering >= 0 && view_rendering < view_count &&
		view_unit[view_rendering] == object_index;
}
