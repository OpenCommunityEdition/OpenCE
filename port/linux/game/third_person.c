/* Optional shoulder view, applied to the director's eye after interpolation.
   The simulation's aim stays fixed; shoulder shots use world muzzles. */
#include "cseries.h"
#include "third_person.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "units/units.h"
#include "physics/collisions.h"
#include "render/render.h"
#include "render/render_cameras_internal.h"
#include "main/main.h"
#include "camera/observer.h"
#include "halo_keyboard.h"
#include <math.h>

int config_boolean(char const *name);
unsigned long config_changes(void);
double config_real(char const *name);

struct shoulder_camera_state
{
	boolean initialized;
	long unit_index;
	real distance, height, side, boom;
	real_point3d eye;
	boolean looking;
	real look_yaw, look_pitch;
	boolean look_requires_release, collision_blocked;
	real clear_time, recovery_delay, vertical_lag;
	real return_time, return_yaw, return_pitch;
};
static struct shoulder_camera_state cameras[MAXIMUM_LOCAL_PLAYERS];
static boolean active[MAXIMUM_LOCAL_PLAYERS];
static boolean smooth_camera = TRUE;
static real requested_distance = 0.85f, requested_height = 0.12f, requested_side = 0.24f;

/* Exponential response gives the same settling time at different frame rates. */
static real camera_blend(real dt, real seconds)
{
	return smooth_camera ? (real)(1.0 - exp(-dt / seconds)) : 1.0f;
}

static void release_look(struct shoulder_camera_state *state)
{
	if (state->looking)
	{
		state->return_time = 0.25f;
		state->return_yaw = state->look_yaw;
		state->return_pitch = state->look_pitch;
		state->looking = FALSE;
	}
}

/* Reject non-finite values as well as out-of-range settings. */
static real camera_setting(char const *name, double low, double high, real fallback)
{
	double value = config_real(name);
	return value >= low && value <= high ? (real)value : fallback;
}

void third_person_reset(void)
{
	short index;
	for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
	{
		cameras[index].initialized = FALSE;
		cameras[index].looking = FALSE;
		cameras[index].look_requires_release = (halo_keyboard_actions(index) & (1UL << HALO_KEYBOARD_FREE_LOOK)) != 0;
		cameras[index].return_time = 0.0f;
		cameras[index].look_yaw = cameras[index].look_pitch = 0.0f;
		active[index] = FALSE;
	}
}

static real_point3d targets[MAXIMUM_LOCAL_PLAYERS];

boolean third_person_active(short local_player_index)
{
	return local_player_index >= 0 && local_player_index < MAXIMUM_LOCAL_PLAYERS &&
		active[local_player_index];
}

boolean third_person_available(short local_player_index)
{
	static unsigned long read_at = (unsigned long)-1;
	long unit_index;
	struct unit_datum *unit;

	if (read_at != config_changes())
	{
		read_at = config_changes();
		smooth_camera = config_boolean("display.third_person_smooth") != 0;
		requested_distance = camera_setting("display.third_person_distance", 0.3, 2.0, 0.85f);
		requested_height = camera_setting("display.third_person_height", -0.2, 0.6, 0.12f);
		requested_side = camera_setting("display.third_person_offset", 0.0, 0.6, 0.24f);
		if (config_boolean("display.third_person_left_shoulder"))
			requested_side = -requested_side;
	}
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS ||
		!third_person_enabled() || cinematic_in_progress() || *director_camera_scripted ||
		director_inhibited_facing(local_player_index))
		return FALSE;
	unit_index = player_control_get_unit_index(local_player_index);
	unit = unit_index != NONE ? unit_try_and_get(unit_index) : NULL;
	if (player_control_get_zoom_level(local_player_index) != NONE ||
		(unit && (unit->unit.current_zoom_level != NONE || unit->unit.desired_zoom_level != NONE)))
	{
		return FALSE;
	}
	return unit && unit->object.parent_object_index == NONE &&
		!TEST_FLAG(unit->object.damage_flags, _object_dead_bit);
}

static boolean third_person_wanted(short local_player_index)
{
	return third_person_available(local_player_index) &&
		third_person_director_active(local_player_index);
}

boolean third_person_free_look_input(short local_player_index, real yaw, real pitch)
{
	struct shoulder_camera_state *state;
	boolean held;
	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS)
		return FALSE;
	state = &cameras[local_player_index];
	held = (halo_keyboard_actions(local_player_index) & (1UL << HALO_KEYBOARD_FREE_LOOK)) != 0;
	if (!held)
		state->look_requires_release = FALSE;
	if (!third_person_wanted(local_player_index) || !state->initialized ||
		state->unit_index != player_control_get_unit_index(local_player_index))
	{
		state->looking = FALSE;
		state->look_requires_release = held;
		state->return_time = 0.0f;
		state->look_yaw = state->look_pitch = 0.0f;
		return FALSE;
	}
	if (!held)
		release_look(state);
	else if (!state->look_requires_release && third_person_active(local_player_index))
		state->looking = TRUE;
	if (!state->looking)
		return FALSE;
	state->return_time = 0.0f;
	state->look_yaw += yaw;
	while (state->look_yaw > _pi) state->look_yaw -= 2.0f * _pi;
	while (state->look_yaw < -_pi) state->look_yaw += 2.0f * _pi;
	state->look_pitch = PIN(state->look_pitch + pitch, -_pi, _pi);
	return TRUE;
}

void third_person_camera(short local_player_index, struct render_camera *camera)
{
	long unit_index;
	struct unit_datum *unit;
	real_vector3d right, offset, ray, saved_forward, saved_up;
	real_euler_angles2d look_angles;
	real_point3d origin;
	struct collision_result collision;
	real fraction, length;
	real hard_fraction, blend, dt, wanted_boom, vertical_step, remaining;
	boolean was_active;
	struct shoulder_camera_state *state;
	unsigned long flags;

	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS)
		return;
	was_active = active[local_player_index];
	active[local_player_index] = FALSE;
	state = &cameras[local_player_index];
	if (!camera || !third_person_wanted(local_player_index))
	{
		state->initialized = FALSE;
		state->look_requires_release = (halo_keyboard_actions(local_player_index) & (1UL << HALO_KEYBOARD_FREE_LOOK)) != 0;
		state->return_time = 0.0f;
		state->looking = FALSE;
		state->look_yaw = state->look_pitch = 0.0f;
		return;
	}
	unit_index = player_control_get_unit_index(local_player_index);
	unit = unit_get(unit_index);
	dt = PIN(main_get_seconds_elapsed(), 0.0f, 0.1f);

	vector_from_points3d(&state->eye, &camera->position, &offset);
	if (!state->initialized || state->unit_index != unit_index || magnitude3d(&offset) > 2.0f)
	{
		state->looking = FALSE;
		/* A respawn or teleport must not restart a held orbit key. */
		state->look_requires_release = (halo_keyboard_actions(local_player_index) & (1UL << HALO_KEYBOARD_FREE_LOOK)) != 0;
		state->look_yaw = state->look_pitch = 0.0f;
		state->distance = requested_distance;
		state->height = requested_height;
		state->side = requested_side;
		state->boom = 0.0f;
		state->vertical_lag = 0.0f;
		state->eye = camera->position;
		state->collision_blocked = FALSE;
		state->clear_time = state->recovery_delay = state->return_time = 0.0f;
		state->unit_index = unit_index;
		state->initialized = TRUE;
		was_active = FALSE;
	}
	vertical_step = camera->position.z - state->eye.z;
	/* Dampen small stair/slope changes only. Keep the real eye as the
	   collision pivot; the bounded lag is part of the swept camera offset. */
	if (smooth_camera && vertical_step > -0.2f && vertical_step < 0.2f)
		state->vertical_lag = PIN((state->vertical_lag - vertical_step) *
			(1.0f - camera_blend(dt, 0.08f)), -0.06f, 0.06f);
	else
		state->vertical_lag = 0.0f;
	state->eye = camera->position;
	/* Input releases immediately even if gameplay is paused by a menu. */
	if (!(halo_keyboard_actions(local_player_index) & (1UL << HALO_KEYBOARD_FREE_LOOK)))
	{
		state->look_requires_release = FALSE;
		release_look(state);
	}
	if (!state->looking)
	{
		state->return_time = smooth_camera ? MAX(0.0f, state->return_time - dt) : 0.0f;
		if (state->return_time < 0.000001f)
			state->return_time = 0.0f;
		remaining = state->return_time / 0.25f;
		blend = remaining * remaining * (3.0f - 2.0f * remaining);
		state->look_yaw = state->return_yaw * blend;
		state->look_pitch = state->return_pitch * blend;
	}

	saved_forward = camera->forward;
	saved_up = camera->up;
	if (state->looking || state->look_yaw != 0.0f || state->look_pitch != 0.0f)
	{
		euler_angles2d_from_vector3d(&look_angles, &camera->forward);
		/* Retain steep gameplay aim on release; an 80-degree orbit limit
		   must not leave a permanent pitch offset when aiming higher. */
		if (state->looking)
			state->look_pitch = PIN(state->look_pitch,
				MIN(-DEGREES_TO_RADIANS(80.0f), look_angles.pitch) - look_angles.pitch,
				MAX(DEGREES_TO_RADIANS(80.0f), look_angles.pitch) - look_angles.pitch);
		look_angles.yaw += state->look_yaw;
		look_angles.pitch = PIN(look_angles.pitch + state->look_pitch,
			MIN(-DEGREES_TO_RADIANS(80.0f), look_angles.pitch),
			MAX(DEGREES_TO_RADIANS(80.0f), look_angles.pitch));
		vector3d_from_euler_angles2d(&camera->forward, &look_angles);
		observer_up_from_forward(&camera->forward, &camera->up);
	}
	blend = camera_blend(dt, 0.12f);
	state->distance += (requested_distance - state->distance) * blend;
	state->height += (requested_height - state->height) * blend;
	state->side += (requested_side - state->side) * blend;
	/* Halo world units are about three metres. Sweep the whole diagonal
	   from the eye so the shoulder cannot pass through a corner. */
	cross_product3d(&camera->forward, &camera->up, &right);
	normalize3d(&right);
	offset.i = -state->distance * camera->forward.i + state->side * right.i + state->height * camera->up.i;
	offset.j = -state->distance * camera->forward.j + state->side * right.j + state->height * camera->up.j;
	offset.k = -state->distance * camera->forward.k + state->side * right.k + state->height * camera->up.k + state->vertical_lag;
	length = magnitude3d(&offset);
	fraction = 1.0f;
	flags = FLAG(_collision_test_front_facing_surfaces_bit) |
		FLAG(_collision_test_back_facing_surfaces_bit) |
		FLAG(_collision_test_structure_bit) | FLAG(_collision_test_objects_bit) |
		_collision_test_objects_sight_blocking_flags;
	/* The sphere sweep covers the BSP; the ray also tests scenery,
	   doors and vehicles, which the engine's pill test does not visit. */
	if (collision_test_pill_new(flags, &camera->position, &offset, 0.06f, unit_index, &collision))
		fraction = PIN(collision.t - 0.02f / length, 0.0f, 1.0f);
	if (collision_test_vector(flags, &camera->position, &offset, unit_index, &collision))
		fraction = MIN(fraction, MAX(0.0f, collision.t - 0.06f / length));
	hard_fraction = fraction;
	/* Start drawing inward before the hard collision limit. Safety always
	   wins over smoothing; recovery is slower than pull-in. */
	if (collision_test_pill_new(flags, &camera->position, &offset, 0.10f, unit_index, &collision))
		fraction = MIN(fraction, MAX(0.0f, collision.t - 0.02f / length));
	/* A soft sweep can start inside a narrow doorway even when the hard
	   sweep fits. It must not itself force a change to first person. */
	if (hard_fraction * length >= 0.16f)
		fraction = MAX(fraction, 0.16f / length);
	wanted_boom = fraction * length;
	/* Briefly hold after an inward correction so door edges and steps
	   cannot pump the camera outward on every other frame. */
	if (wanted_boom < state->boom - 0.01f)
		state->recovery_delay = 0.08f;
	else
		state->recovery_delay = MAX(0.0f, state->recovery_delay - dt);
	blend = camera_blend(dt, wanted_boom < state->boom ? 0.06f : 0.25f);
	if (smooth_camera && wanted_boom > state->boom && state->recovery_delay > 0.0f)
		blend = 0.0f;
	state->boom += (wanted_boom - state->boom) * blend;
	state->boom = MIN(state->boom, hard_fraction * length);
	/* Separate entry and exit thresholds prevent rapid perspective changes
	   when a wall leaves just enough room for the player's body. */
	if (hard_fraction * length < (was_active ? 0.10f : 0.14f))
	{
		state->collision_blocked = TRUE;
		state->clear_time = 0.0f;
	}
	if (state->collision_blocked)
	{
		if (hard_fraction * length >= 0.14f)
			state->clear_time += dt;
		if (state->clear_time >= 0.10f)
			state->collision_blocked = FALSE;
	}
	if (state->collision_blocked)
	{
		state->boom = 0.0f;
		/* Collision fallback must retain the player's first-person aim. */
		camera->forward = saved_forward;
		camera->up = saved_up;
		return;
	}
	state->boom = MAX(state->boom, MIN(0.14f, hard_fraction * length));
	point_from_line3d(&camera->position, &offset, state->boom / length, &camera->position);
	active[local_player_index] = TRUE;
	/* Use the current unscoped target FOV, not the observer's remaining
	   zoom interpolation. Collision fallback retains the stock camera. */
	camera->vertical_field_of_view = 2.0f * arctangent(
		0.75f * render_camera_get_adjusted_field_of_view_tangent(
			player_control_get_field_of_view(local_player_index)), 1.0f);

	/* Project the stock firing line, not a ray from the displaced camera.
	   This indicates the nominal aim point; spread and projectile gravity
	   remain the weapon's own, just as in first person. */
	unit_get_camera_position(unit_index, &origin);
	scale_vector3d(&unit->unit.aiming_vector, 1000.0f, &ray);
	point_from_line3d(&origin, &ray, 1.0f, &targets[local_player_index]);
	if (collision_test_vector(_collision_test_for_projectiles_flags, &origin, &ray,
		unit_index, &collision))
		targets[local_player_index] = collision.point;
	third_person_muzzle_target(local_player_index, &targets[local_player_index]);
}

boolean third_person_reticle(point2d *point, real reticle_scale)
{
	real_point2d screen;
	real center_x, center_y;

	if (!third_person_active(render.local_player_index))
		return TRUE;
	if (cameras[render.local_player_index].looking ||
		cameras[render.local_player_index].look_yaw != 0.0f ||
		cameras[render.local_player_index].look_pitch != 0.0f)
		return FALSE;
	if (!render_camera_world_to_screen(&render.camera, &render.frustum,
		&targets[render.local_player_index], &screen))
		return FALSE;
	center_x = (render.camera.window_bounds.x0 + render.camera.window_bounds.x1) * 0.5f;
	center_y = (render.camera.window_bounds.y0 + render.camera.window_bounds.y1) * 0.5f;
	/* hud_draw_bitmap_internal scales about the original screen centre. */
	point->x += (short)((screen.x - center_x) / reticle_scale);
	point->y += (short)((screen.y - center_y) / reticle_scale);
	return TRUE;
}
