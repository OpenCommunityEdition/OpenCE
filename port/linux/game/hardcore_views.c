/* One local player can watch teammates in a hardcore team game for the
whole match. A teammate who dies stays in that pane on the death camera,
with the respawn line, instead of the pane clearing. The extra panes are
render windows only. They are not local players, so input, scoring and
the single-player HUD stay with the one player. The menu option
display.teammate_views is off until chosen. Which pane is the local
player's is this machine's display.teammate_view_slot. Each teammate pane
keeps a spare HUD slot, so the weapon pose continues between frames. The
slot is a local player only while that pane is drawn. */
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
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "scenario/scenario.h"
#include "units/units.h"
#include "hardcore_rules.h"
#include "hardcore_views.h"
#include "../src/port_config.h"
#include <string.h>

#define HARDCORE_VIEW_MAXIMUM 4

/* the sight and the aim, kept from the last tick so the pane can draw
between them. the camera is asked before the frame's object blend. */
struct hardcore_view_camera
{
	boolean valid;
	boolean has_previous;
	long tick;
	struct observer_result previous;
	struct observer_result latest;
};

static long view_unit[HARDCORE_VIEW_MAXIMUM];
static long view_player[HARDCORE_VIEW_MAXIMUM];
static short view_count = 1;
static short local_slot = 0;
static short view_rendering = 0;
static short view_hud_slot[HARDCORE_VIEW_MAXIMUM];
static boolean view_slots_ready = FALSE;
static struct hardcore_view_camera view_camera[HARDCORE_VIEW_MAXIMUM];
static boolean view_hiding = FALSE;
static boolean view_borrowed = FALSE;
/* dead_camera's elevated look, held until that player is alive again.
the switch matches its multiplayer delay, then the next living teammate. */
static boolean death_on[HARDCORE_VIEW_MAXIMUM];
static long death_body[HARDCORE_VIEW_MAXIMUM];
static long death_since[HARDCORE_VIEW_MAXIMUM];
static real death_yaw[HARDCORE_VIEW_MAXIMUM];
static real_point3d death_focus[HARDCORE_VIEW_MAXIMUM];
/* The teammates in the order the split first drew them. A later arrival
sorts into the middle of the player list and would move every pane. */
static long roster_player[HARDCORE_VIEW_MAXIMUM - 1];
static short roster_count = 0;
static long roster_team = NONE;
static boolean roster_locked = FALSE;

/* A pane's HUD needs a local player slot. Slot 1 is the second person
when two play on this machine, and clearing it drops their listener.
Each teammate pane keeps the slot it took, so its weapon pose stays. */
static void hardcore_views_slots_init(void)
{
	short index;

	if (view_slots_ready)
		return;
	for (index = 0; index < HARDCORE_VIEW_MAXIMUM; index++)
		view_hud_slot[index] = NONE;
	view_slots_ready = TRUE;
}

static void hardcore_views_slots_release(void)
{
	short index;

	hardcore_views_slots_init();
	for (index = 0; index < HARDCORE_VIEW_MAXIMUM; index++)
	{
		if (view_hud_slot[index] != NONE &&
			local_player_get_player_index(view_hud_slot[index]) == NONE)
			first_person_weapon_observe_clear(view_hud_slot[index]);
		view_hud_slot[index] = NONE;
		view_camera[index].valid = FALSE;
	}
}

static void hardcore_views_slots_assign(void)
{
	boolean used[MAXIMUM_LOCAL_PLAYERS];
	short slot;
	short window;

	hardcore_views_slots_init();
	for (slot = 0; slot < MAXIMUM_LOCAL_PLAYERS; slot++)
		used[slot] = local_player_get_player_index(slot) != NONE;
	for (window = 0; window < HARDCORE_VIEW_MAXIMUM; window++)
	{
		if (window >= view_count || window == local_slot)
		{
			if (view_hud_slot[window] != NONE &&
				local_player_get_player_index(view_hud_slot[window]) == NONE)
				first_person_weapon_observe_clear(view_hud_slot[window]);
			view_hud_slot[window] = NONE;
			continue;
		}
		slot = view_hud_slot[window];
		if (slot >= 0 && slot < MAXIMUM_LOCAL_PLAYERS && !used[slot])
		{
			used[slot] = TRUE;
			continue;
		}
		if (slot != NONE && local_player_get_player_index(slot) == NONE)
			first_person_weapon_observe_clear(slot);
		view_hud_slot[window] = NONE;
		for (slot = 0; slot < MAXIMUM_LOCAL_PLAYERS; slot++)
		{
			if (!used[slot])
			{
				used[slot] = TRUE;
				view_hud_slot[window] = slot;
				break;
			}
		}
	}
}

/* Two panes are top and bottom. Three are a wide top and two bottoms.
Four are the corners. The name is this machine's setting, so a bottom
choice stays on a bottom pane instead of being moved to the top. */
/* A hardcore team game has one team on this display. The other team's
local player would be looking at this team, and this team at them. */
static boolean hardcore_views_team_screen(void)
{
	return hardcore_rules_active() && game_in_progress() && game_engine_running() &&
		game_engine_has_teams();
}

static long hardcore_views_screen_team(void)
{
	short slot = local_player_get_next(NONE);
	struct player_datum *local;

	if (slot == NONE)
		return NONE;
	local = player_try_and_get(local_player_get_player_index(slot));
	if (!local || local->team_index < 0)
		return NONE;
	return local->team_index;
}

static boolean hardcore_views_local_on_screen(short local_player_index)
{
	struct player_datum *player;
	long team;

	if (!hardcore_views_team_screen())
		return TRUE;
	team = hardcore_views_screen_team();
	if (team == NONE)
		return TRUE;
	player = player_try_and_get(local_player_get_player_index(local_player_index));
	if (!player || player->team_index < 0)
		return FALSE;
	return player->team_index == team;
}

short hardcore_views_local_count(void)
{
	short count = 0;
	short index = NONE;

	if (!hardcore_views_team_screen() || hardcore_views_screen_team() == NONE)
		return local_player_count();
	while ((index = local_player_get_next(index)) != NONE)
	{
		if (hardcore_views_local_on_screen(index))
			count++;
	}
	if (count < 1)
		count = 1;
	return count;
}

short hardcore_views_next_local(short previous)
{
	short index = previous;

	if (!hardcore_views_team_screen() || hardcore_views_screen_team() == NONE)
		return local_player_get_next(previous);
	while ((index = local_player_get_next(index)) != NONE)
	{
		if (hardcore_views_local_on_screen(index))
			return index;
	}
	return NONE;
}

static short hardcore_views_slot(short count)
{
	char const *name = config_string("display.teammate_view_slot");
	boolean bottom;
	short slot;

	if (!name)
		name = "";
	bottom = !strcmp(name, "bottom") || !strcmp(name, "bottom_left") ||
		!strcmp(name, "bottom_right");
	if (count <= 2)
		slot = bottom ? 1 : 0;
	else if (count == 3)
	{
		if (!strcmp(name, "bottom_left"))
			slot = 1;
		else if (bottom)
			slot = 2;
		else
			slot = 0;
	}
	else if (!strcmp(name, "top_right"))
		slot = 1;
	else if (!strcmp(name, "bottom_left"))
		slot = 2;
	else if (bottom)
		slot = 3;
	else
		slot = 0;
	if (slot >= count)
		slot = bottom ? (short)(count - 1) : 0;
	return slot;
}

static void hardcore_views_roster_clear(void)
{
	short index;

	roster_locked = FALSE;
	roster_count = 0;
	roster_team = NONE;
	for (index = 0; index < HARDCORE_VIEW_MAXIMUM - 1; index++)
		roster_player[index] = NONE;
	for (index = 0; index < HARDCORE_VIEW_MAXIMUM; index++)
		death_on[index] = FALSE;
}

/* The players already in the match, in list order. This runs until the
clock has started, then the list is kept as it was. */
static void hardcore_views_roster_take(long local_index, long team)
{
	struct data_iterator iterator;
	struct player_datum *player;

	roster_count = 0;
	roster_team = team;
	data_iterator_new(&iterator, player_data);
	while (roster_count < HARDCORE_VIEW_MAXIMUM - 1 &&
		(player = data_iterator_next(&iterator)) != NULL)
	{
		if (iterator.datum_index == local_index || player->quit_out_of_game ||
			player->team_index != team || player->local_player_index != NONE)
			continue;
		roster_player[roster_count++] = iterator.datum_index;
	}
}

static void hardcore_views_refresh(void)
{
	struct player_datum *player;
	struct player_datum *local;
	long local_index;
	long mates[HARDCORE_VIEW_MAXIMUM - 1];
	short index;
	short placed;

	/* the pane has the slot until it ends. a refresh here would forget
	which one and clear a different player. */
	if (view_borrowed)
		return;
	view_count = 1;
	local_slot = 0;
	view_unit[0] = NONE;
	if (!hardcore_views_team_screen())
	{
		hardcore_views_roster_clear();
		hardcore_views_slots_release();
		return;
	}
	if (!config_boolean("display.teammate_views") ||
		hardcore_views_local_count() != 1 || !player_data || !player_data->valid)
	{
		hardcore_views_slots_release();
		return;
	}
	local_index = local_player_get_player_index(local_player_get_next(NONE));
	local = player_try_and_get(local_index);
	if (!local || local->team_index < 0)
	{
		hardcore_views_slots_release();
		return;
	}
	if (roster_locked && roster_team != local->team_index)
		hardcore_views_roster_clear();
	if (!roster_locked)
	{
		hardcore_views_roster_take(local_index, local->team_index);
		if (game_time_get() > 0)
			roster_locked = TRUE;
	}
	if (!roster_count)
	{
		hardcore_views_slots_release();
		return;
	}
	/* A player who leaves keeps the pane, empty, so the others stay
	where the split put them. Their body is looked up from the same
	handle, including after they respawn. */
	for (index = 0; index < roster_count; index++)
	{
		player = player_try_and_get(roster_player[index]);
		if (!player || player->quit_out_of_game || player->team_index != roster_team)
			mates[index] = NONE;
		else
			mates[index] = player->unit_index;
	}
	view_count = (short)(roster_count + 1);
	local_slot = hardcore_views_slot(view_count);
	for (index = 0; index < HARDCORE_VIEW_MAXIMUM; index++)
	{
		view_unit[index] = NONE;
		view_player[index] = NONE;
	}
	placed = 0;
	for (index = 0; index < view_count; index++)
	{
		if (index == local_slot)
			continue;
		view_player[index] = roster_player[placed];
		view_unit[index] = mates[placed++];
	}
	hardcore_views_slots_assign();
}

short hardcore_views_window_count(void)
{
	hardcore_views_refresh();
	return view_count;
}

boolean hardcore_views_is_teammate_window(short window_index)
{
	/* a classic split and the campaign still have the last match's pane
	   count until something asks again. those windows are their players. */
	if (!hardcore_views_team_screen())
		return FALSE;
	return view_count > 1 && window_index >= 0 && window_index < view_count &&
		window_index != local_slot;
}

boolean hardcore_views_is_local_window(short window_index)
{
	if (!hardcore_views_team_screen())
		return FALSE;
	return view_count > 1 && window_index == local_slot;
}

boolean hardcore_views_split(void)
{
	return hardcore_views_team_screen() && view_count > 1;
}

short hardcore_views_hud_slot(short window_index)
{
	if (window_index < 0 || window_index >= HARDCORE_VIEW_MAXIMUM)
		return NONE;
	return view_hud_slot[window_index];
}

boolean hardcore_views_listener(short slot, struct observer_result *camera)
{
	short window;

	if (!camera || slot < 0 || slot >= MAXIMUM_LOCAL_PLAYERS ||
		local_player_get_player_index(slot) != NONE)
		return FALSE;
	hardcore_views_refresh();
	if (!hardcore_views_team_screen())
		return FALSE;
	for (window = 0; window < view_count && window < HARDCORE_VIEW_MAXIMUM; window++)
	{
		if (window == local_slot || view_hud_slot[window] != slot)
			continue;
		return hardcore_views_camera(window, camera);
	}
	return FALSE;
}

/* the next living teammate after the body this pane is on, or the first
one. dead_camera does this once its switch time runs out. */
static long hardcore_views_next_body(long dead_player, long current_body)
{
	struct data_iterator iterator;
	struct player_datum *player;
	struct player_datum *dead;
	long team;
	long first_unit;
	long next_unit;
	long current_index;
	boolean passed;

	dead = player_try_and_get(dead_player);
	if (!dead)
		return NONE;
	team = dead->team_index;
	first_unit = NONE;
	next_unit = NONE;
	current_index = NONE;
	data_iterator_new(&iterator, player_data);
	while ((player = data_iterator_next(&iterator)) != NULL)
	{
		if (current_body != NONE && player->unit_index == current_body)
			current_index = iterator.datum_index;
	}
	passed = current_index == NONE;
	data_iterator_new(&iterator, player_data);
	while ((player = data_iterator_next(&iterator)) != NULL)
	{
		struct object_datum *body;

		if (iterator.datum_index == dead_player || player->unit_index == NONE ||
			player->team_index != team)
			continue;
		body = object_try_and_get(player->unit_index);
		if (!body || TEST_FLAG(body->object.damage_flags, _object_dead_bit))
			continue;
		if (first_unit == NONE)
			first_unit = player->unit_index;
		if (passed && next_unit == NONE)
			next_unit = player->unit_index;
		if (iterator.datum_index == current_index)
			passed = TRUE;
	}
	if (next_unit != NONE)
		return next_unit;
	return first_unit;
}

/* the camera sits back from the body. a wall in between pulls it in,
which is what keeps the death view from being inside the map. */
static void hardcore_views_death_place(
	real_point3d const *focus,
	real_vector3d const *forward,
	real distance,
	real_point3d *position)
{
	struct collision_result hit;
	unsigned long flags;
	real_point3d desired;
	real scale;

	desired.x = focus->x - forward->i * distance;
	desired.y = focus->y - forward->j * distance;
	desired.z = focus->z - forward->k * distance;
	scale = 1.0f;
	flags = FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit);
	if (global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH)
	{
		global_current_collision_users[global_current_collision_user_depth++] =
			_collision_user_observer;
		if (collision_test_line(flags, focus, &desired, NONE, &hit) &&
			hit.t > 0.05f && hit.t < 1.0f)
			scale = hit.t * 0.85f;
		global_current_collision_user_depth--;
	}
	position->x = focus->x + (desired.x - focus->x) * scale;
	position->y = focus->y + (desired.y - focus->y) * scale;
	position->z = focus->z + (desired.z - focus->z) * scale;
}

static void hardcore_views_smooth_drop(short window_index)
{
	if (window_index >= 0 && window_index < HARDCORE_VIEW_MAXIMUM)
		view_camera[window_index].valid = FALSE;
}

/* between the last two ticks, the same fraction the world is drawn at.
a jump in place or aim (a respawn, the death camera) is left as a cut */
static void hardcore_views_smooth(short window_index, struct observer_result *result)
{
	struct hardcore_view_camera *camera;
	long tick;
	real t;
	real dx;
	real dy;
	real dz;
	real dot;
	real length;
	struct observer_result blended;

	if (!result || window_index < 0 || window_index >= HARDCORE_VIEW_MAXIMUM)
		return;
	camera = &view_camera[window_index];
	tick = game_time_get();
	if (!camera->valid || camera->tick != tick)
	{
		camera->has_previous = camera->valid && camera->tick + 1 == tick;
		if (camera->has_previous)
		{
			dx = result->position.x - camera->latest.position.x;
			dy = result->position.y - camera->latest.position.y;
			dz = result->position.z - camera->latest.position.z;
			dot = result->forward.i * camera->latest.forward.i +
				result->forward.j * camera->latest.forward.j +
				result->forward.k * camera->latest.forward.k;
			if (!(dx * dx + dy * dy + dz * dz <= 9.0f) || !(dot >= 0.5f))
				camera->has_previous = FALSE;
		}
		if (camera->has_previous)
			camera->previous = camera->latest;
		camera->latest = *result;
		camera->tick = tick;
		camera->valid = TRUE;
	}
	if (!halo_interpolation_enabled() || !camera->has_previous)
		return;
	t = game_time_get_tick_fraction();
	blended = camera->latest;
	blended.position.x = camera->previous.position.x +
		(camera->latest.position.x - camera->previous.position.x) * t;
	blended.position.y = camera->previous.position.y +
		(camera->latest.position.y - camera->previous.position.y) * t;
	blended.position.z = camera->previous.position.z +
		(camera->latest.position.z - camera->previous.position.z) * t;
	blended.forward.i = camera->previous.forward.i +
		(camera->latest.forward.i - camera->previous.forward.i) * t;
	blended.forward.j = camera->previous.forward.j +
		(camera->latest.forward.j - camera->previous.forward.j) * t;
	blended.forward.k = camera->previous.forward.k +
		(camera->latest.forward.k - camera->previous.forward.k) * t;
	length = blended.forward.i * blended.forward.i +
		blended.forward.j * blended.forward.j +
		blended.forward.k * blended.forward.k;
	if (length > 0.0001f)
	{
		length = 1.0f / sqrt(length);
		blended.forward.i *= length;
		blended.forward.j *= length;
		blended.forward.k *= length;
		observer_up_from_forward(&blended.forward, &blended.up);
	}
	blended.field_of_view = camera->previous.field_of_view +
		(camera->latest.field_of_view - camera->previous.field_of_view) * t;
	*result = blended;
}

static boolean hardcore_views_death_camera(
	short window_index,
	long player_index,
	struct player_datum *player,
	struct observer_result *result)
{
	struct object_datum *body;
	real_euler_angles2d facing;
	long body_index;
	long next;

	if (!death_on[window_index])
	{
		body_index = player->unit_index != NONE
			? player->unit_index
			: player->dead_unit_index;
		death_on[window_index] = TRUE;
		death_since[window_index] = game_time_get();
		death_body[window_index] = body_index;
		body = object_try_and_get(body_index);
		if (body &&
			body->object.forward.i * body->object.forward.i +
				body->object.forward.j * body->object.forward.j > 0.0001f)
			death_yaw[window_index] = arctangent(
				body->object.forward.j, body->object.forward.i);
		else
			death_yaw[window_index] = 0.0f;
		if (body)
			death_focus[window_index] = body->object.bounding_sphere_center;
		else
			death_focus[window_index] = *global_origin3d;
	}
	else if (game_time_get() - death_since[window_index] >= 15 * TICKS_PER_SECOND)
	{
		/* dead_camera's multiplayer_switch_timer is 15 seconds */
		next = hardcore_views_next_body(player_index, death_body[window_index]);
		death_since[window_index] = game_time_get();
		if (next != NONE && next != death_body[window_index])
			death_body[window_index] = next;
	}
	body = object_try_and_get(death_body[window_index]);
	if (body)
		death_focus[window_index] = body->object.bounding_sphere_center;
	facing.yaw = death_yaw[window_index];
	facing.pitch = -0.6f;
	csmemset(result, 0, sizeof(*result));
	vector3d_from_euler_angles2d(&result->forward, &facing);
	observer_up_from_forward(&result->forward, &result->up);
	hardcore_views_death_place(
		&death_focus[window_index],
		&result->forward,
		4.0f,
		&result->position);
	/* dead_camera's field of view */
	result->field_of_view = DEGREES_TO_RADIANS(70.0f);
	hardcore_views_smooth(window_index, result);
	return TRUE;
}

boolean hardcore_views_camera(short window_index, struct observer_result *result)
{
	long player_index;
	struct player_datum *player;
	long team;
	long unit_index;
	struct object_datum *object;
	real_vector3d forward;
	real length;

	if (!result || !hardcore_views_is_teammate_window(window_index))
		return FALSE;
	player_index = view_player[window_index];
	player = player_try_and_get(player_index);
	team = hardcore_views_screen_team();
	/* another local player has their own pane, or none when they are
	on the other team. a player who left keeps the pane empty. */
	if (!player || player->quit_out_of_game || player->local_player_index != NONE ||
		(team != NONE && player->team_index != team))
	{
		death_on[window_index] = FALSE;
		hardcore_views_smooth_drop(window_index);
		return FALSE;
	}
	unit_index = player->unit_index;
	object = object_try_and_get(unit_index);
	if (object && !TEST_FLAG(object->object.damage_flags, _object_dead_bit))
	{
		death_on[window_index] = FALSE;
		view_unit[window_index] = unit_index;
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
		hardcore_views_smooth(window_index, result);
		/* the listener and the wall test need the cluster this view is in.
		the pose above leaves that at the zero cluster */
		scenario_location_from_point(&result->location, &result->position);
		return TRUE;
	}
	/* not spawned yet: the pane stays empty until they are alive.
	a death, the body or none after it, uses the death camera */
	if (player->statistics.deaths <= 0 &&
		!(object && TEST_FLAG(object->object.damage_flags, _object_dead_bit)))
	{
		death_on[window_index] = FALSE;
		hardcore_views_smooth_drop(window_index);
		return FALSE;
	}
	if (!hardcore_views_death_camera(window_index, player_index, player, result))
		return FALSE;
	scenario_location_from_point(&result->location, &result->position);
	return TRUE;
}

void hardcore_views_begin(short window_index)
{
	long player_index;
	long unit_index;
	short slot;

	view_rendering = window_index;
	view_borrowed = FALSE;
	view_hiding = FALSE;
	if (!hardcore_views_is_teammate_window(window_index))
		return;
	player_index = view_player[window_index];
	slot = hardcore_views_hud_slot(window_index);
	if (player_index == NONE || slot == NONE ||
		local_player_get_player_index(slot) != NONE)
		return;
	/* the HUD and the first-person weapon look up a local player. the
	slot is one only while this pane draws, and it is not a listener.
	the weapon pose stays on the slot, so the next frame continues it.
	the pane's camera stays the unzoomed view, so this slot must not
	report the teammate's zoom. a dead body stays visible: the death
	camera is looking at it, and it has no first-person weapon */
	local_player_set_player_index(slot, player_index);
	player_control_new_unit(slot, NONE);
	unit_index = view_unit[window_index];
	if (death_on[window_index] || unit_index == NONE)
		first_person_weapon_observe_clear(slot);
	else
	{
		first_person_weapon_observe_unit(slot, unit_index);
		hud_weapon_port_update_player(slot);
		view_hiding = TRUE;
	}
	view_borrowed = TRUE;
}

void hardcore_views_end(void)
{
	short slot;

	if (view_borrowed)
	{
		slot = hardcore_views_hud_slot(view_rendering);
		if (slot != NONE)
		{
			player_control_new_unit(slot, NONE);
			local_player_set_player_index(slot, NONE);
		}
		view_borrowed = FALSE;
	}
	view_hiding = FALSE;
}

void hardcore_views_weapons_tick(void)
{
	short window;

	hardcore_views_refresh();
	for (window = 0; window < view_count && window < HARDCORE_VIEW_MAXIMUM; window++)
	{
		struct object_datum *object;
		short slot;

		if (window == local_slot)
			continue;
		slot = view_hud_slot[window];
		if (slot == NONE || local_player_get_player_index(slot) != NONE)
			continue;
		object = object_try_and_get(view_unit[window]);
		if (death_on[window] || !object ||
			TEST_FLAG(object->object.damage_flags, _object_dead_bit))
			first_person_weapon_observe_clear(slot);
		else
			first_person_weapon_observe_tick(slot, view_unit[window]);
	}
}

boolean hardcore_views_hides_unit(long object_index)
{
	return view_hiding && view_rendering >= 0 && view_rendering < view_count &&
		view_unit[view_rendering] == object_index;
}
