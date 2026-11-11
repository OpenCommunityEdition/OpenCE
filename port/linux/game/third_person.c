/* Optional shoulder view, applied to the director's eye after interpolation.
   The simulation's aim stays fixed; shoulder shots use world muzzles. */
#include "cseries.h"
#include "third_person.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "game/game.h"
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

struct shoulder_reticle_state
{
	boolean valid, prepared, view_valid;
	real_point2d shown, raw;
	real size;
	real_point3d eye;
	real_vector3d aim;
};
struct shoulder_aim_state
{
	boolean valid;
	long tick, unit_index, weapon_index;
	real_vector3d previous, latest;
};
static struct shoulder_reticle_state reticles[MAXIMUM_LOCAL_PLAYERS];
static struct shoulder_aim_state aim_samples[MAXIMUM_LOCAL_PLAYERS];
static boolean smooth_reticle;
static real reticle_size = 1.0f;

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

static void reset_look(struct shoulder_camera_state *state, short local_player_index)
{
	state->looking = FALSE;
	state->look_requires_release = (halo_keyboard_actions(local_player_index) & (1UL << HALO_KEYBOARD_FREE_LOOK)) != 0;
	state->return_time = 0.0f;
	state->look_yaw = state->look_pitch = 0.0f;
}

static boolean camera_orbiting(struct shoulder_camera_state const *state)
{
	return state->looking || state->look_yaw != 0.0f || state->look_pitch != 0.0f;
}

void third_person_reset(void)
{
	short index;
	for (index = 0; index < MAXIMUM_LOCAL_PLAYERS; index++)
	{
		cameras[index].initialized = FALSE;
		reticles[index].valid = FALSE;
		reticles[index].view_valid = FALSE;
		aim_samples[index].valid = FALSE;
		reset_look(&cameras[index], index);
		active[index] = FALSE;
	}
}

static real_point3d targets[MAXIMUM_LOCAL_PLAYERS];

boolean third_person_active(short local_player_index)
{
	return local_player_index >= 0 && local_player_index < MAXIMUM_LOCAL_PLAYERS &&
		active[local_player_index];
}

static void camera_settings_update(void)
{
	static unsigned long read_at = (unsigned long)-1;
	unsigned long changes = config_changes();
	if (read_at != changes)
	{
		read_at = changes;
		smooth_reticle = config_boolean("display.third_person_crosshair_smooth") != 0;
		reticle_size = camera_setting("display.third_person_crosshair_size", 0.75, 1.0, 1.0f);
		/* Settings changes must not reuse an earlier view or aim sample. */
		csmemset(reticles, 0, sizeof(reticles));
		csmemset(aim_samples, 0, sizeof(aim_samples));
		smooth_camera = config_boolean("display.third_person_smooth") != 0;
		requested_distance = camera_setting("display.third_person_distance", 0.3, 2.0, 0.85f);
		requested_height = camera_setting("display.third_person_height", -0.2, 0.6, 0.12f);
		requested_side = camera_setting("display.third_person_offset", 0.0, 0.6, 0.24f);
		if (config_boolean("display.third_person_left_shoulder"))
			requested_side = -requested_side;
	}
}

boolean third_person_available(short local_player_index)
{
	long unit_index;
	struct unit_datum *unit;

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
		reset_look(state, local_player_index);
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

/* The renderer supplies its unshaken, interpolated/instant view before the
   shoulder displacement. Carry the simulated aim offset into that view's
   basis; do not project a tick-old world direction through a newer camera. */
void third_person_reticle_view(short player, struct observer_result const *view)
{
	struct shoulder_reticle_state *reticle;
	struct shoulder_aim_state *sample;
	struct unit_datum *unit;
	real_vector3d requested, up, right, relative, blended, eye_delta;
	long unit_index, weapon_index, tick;
	real fraction;
	if (player < 0 || player >= MAXIMUM_LOCAL_PLAYERS)
		return;
	camera_settings_update();
	reticle = &reticles[player];
	sample = &aim_samples[player];
	reticle->view_valid = FALSE;
	if (!view || !smooth_reticle || !third_person_wanted(player))
	{
		sample->valid = FALSE;
		return;
	}
	unit_index = player_control_get_unit_index(player);
	unit = unit_get(unit_index);
	weapon_index = unit_inventory_get_weapon(unit_index, unit->unit.current_weapon_index);
	tick = game_time_get();
	requested = unit->unit.desired_aiming_vector;
	if (normalize3d(&requested) < 0.0001f)
	{
		sample->valid = FALSE;
		return;
	}
	observer_up_from_forward(&requested, &up);
	cross_product3d(&requested, &up, &right);
	relative.i = dot_product3d(&unit->unit.aiming_vector, &requested);
	relative.j = dot_product3d(&unit->unit.aiming_vector, &right);
	relative.k = dot_product3d(&unit->unit.aiming_vector, &up);
	vector_from_points3d(&reticle->eye, &view->position, &eye_delta);
	if (!sample->valid || magnitude3d(&eye_delta) > 2.0f || sample->unit_index != unit_index || sample->weapon_index != weapon_index ||
		tick < sample->tick || tick > sample->tick + 1 ||
		dot_product3d(&relative, &sample->latest) < 0.5f)
	{
		sample->previous = sample->latest = relative;
	}
	else if (tick != sample->tick)
	{
		sample->previous = sample->latest;
		sample->latest = relative;
	}
	sample->unit_index = unit_index;
	sample->weapon_index = weapon_index;
	sample->tick = tick;
	sample->valid = TRUE;
	fraction = render_interpolation_fraction();
	blended.i = sample->previous.i + (sample->latest.i - sample->previous.i) * fraction;
	blended.j = sample->previous.j + (sample->latest.j - sample->previous.j) * fraction;
	blended.k = sample->previous.k + (sample->latest.k - sample->previous.k) * fraction;
	cross_product3d(&view->forward, &view->up, &right);
	reticle->aim.i = view->forward.i * blended.i + right.i * blended.j + view->up.i * blended.k;
	reticle->aim.j = view->forward.j * blended.i + right.j * blended.j + view->up.j * blended.k;
	reticle->aim.k = view->forward.k * blended.i + right.k * blended.j + view->up.k * blended.k;
	reticle->eye = view->position;
	reticle->view_valid = normalize3d(&reticle->aim) > 0.0001f;
}

static void camera_target_update(short local_player_index, long unit_index)
{
	struct unit_datum *unit;
	real_point3d origin;
	real_vector3d ray;
	struct collision_result collision;

	/* Orbit and its return hide the reticle, so neither target ray is used. */
	if (camera_orbiting(&cameras[local_player_index]) ||
		third_person_muzzle_target(local_player_index, &targets[local_player_index],
			reticles[local_player_index].view_valid ? &reticles[local_player_index].eye : NULL,
			reticles[local_player_index].view_valid ? &reticles[local_player_index].aim : NULL))
		return;
	unit = unit_get(unit_index);
	/* Project the stock firing line, not a ray from the displaced camera.
	   This indicates the nominal aim point; spread and projectile gravity
	   remain the weapon's own, just as in first person. */
	if (reticles[local_player_index].view_valid)
	{
		origin = reticles[local_player_index].eye;
		scale_vector3d(&reticles[local_player_index].aim, 1000.0f, &ray);
	}
	else
	{
		unit_get_camera_position(unit_index, &origin);
		scale_vector3d(&unit->unit.aiming_vector, 1000.0f, &ray);
	}
	point_from_line3d(&origin, &ray, 1.0f, &targets[local_player_index]);
	if (collision_test_vector(_collision_test_for_projectiles_flags, &origin, &ray,
		unit_index, &collision))
		targets[local_player_index] = collision.point;
}

/* Sweep the whole boom and resolve recovery before moving the render eye.
   FALSE leaves position unchanged so the caller can restore first-person aim. */
static boolean camera_collision(struct shoulder_camera_state *state, long unit_index,
	struct render_camera *camera, real dt, boolean was_active)
{
	real_vector3d right, offset;
	struct collision_result collision;
	real fraction, length, hard_fraction, blend, wanted_boom;
	unsigned long flags;
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
		return FALSE;
	}
	state->boom = MAX(state->boom, MIN(0.14f, hard_fraction * length));
	point_from_line3d(&camera->position, &offset, state->boom / length, &camera->position);
	return TRUE;
}

void third_person_camera(short local_player_index, struct render_camera *camera)
{
	long unit_index;
	real_vector3d offset, saved_forward, saved_up;
	real_euler_angles2d look_angles;
	real blend, dt, vertical_step, remaining;
	boolean was_active;
	struct shoulder_camera_state *state;

	if (local_player_index < 0 || local_player_index >= MAXIMUM_LOCAL_PLAYERS)
		return;
	reticles[local_player_index].prepared = FALSE;
	was_active = active[local_player_index];
	active[local_player_index] = FALSE;
	state = &cameras[local_player_index];
	if (!camera || !third_person_wanted(local_player_index))
	{
		state->initialized = FALSE;
		reticles[local_player_index].valid = FALSE;
		reset_look(state, local_player_index);
		return;
	}
	unit_index = player_control_get_unit_index(local_player_index);
	camera_settings_update();
	dt = PIN(main_get_seconds_elapsed(), 0.0f, 0.1f);

	vector_from_points3d(&state->eye, &camera->position, &offset);
	if (!state->initialized || state->unit_index != unit_index || magnitude3d(&offset) > 2.0f)
	{
		/* A respawn or teleport must not restart a held orbit key. */
		reset_look(state, local_player_index);
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
		reticles[local_player_index].valid = FALSE;
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
	if (camera_orbiting(state))
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
	if (!camera_collision(state, unit_index, camera, dt, was_active))
	{
		reticles[local_player_index].valid = FALSE;
		/* Collision fallback must retain the player's first-person aim. */
		camera->forward = saved_forward;
		camera->up = saved_up;
		return;
	}
	active[local_player_index] = TRUE;
	/* Use the current unscoped target FOV, not the observer's remaining
	   zoom interpolation. Collision fallback retains the stock camera. */
	camera->vertical_field_of_view = 2.0f * arctangent(
		0.75f * render_camera_get_adjusted_field_of_view_tangent(
			player_control_get_field_of_view(local_player_index)), 1.0f);

	camera_target_update(local_player_index, unit_index);
}

/* Every bitmap piece uses the current projected target without a catch-up
   filter. Fractional placement is applied after the integer HUD layout. */
static void reticle_prepare(real_point2d const *offset, real scale)
{
	struct shoulder_reticle_state *state = &reticles[render.local_player_index];
	state->shown = *offset;
	state->raw.x = (short)(offset->x / scale) * scale;
	state->raw.y = (short)(offset->y / scale) * scale;
	state->size = reticle_size;
	state->prepared = state->valid = TRUE;
}

boolean third_person_reticle(point2d *point, real reticle_scale)
{
	real_point2d screen, offset;
	real center_x, center_y;
	short player = render.local_player_index;

	if (!third_person_active(player))
		return TRUE;
	if (camera_orbiting(&cameras[player]) ||
		!render_camera_world_to_screen(&render.camera, &render.frustum, &targets[player], &screen))
	{
		reticles[player].valid = FALSE;
		return FALSE;
	}
	center_x = (render.camera.window_bounds.x0 + render.camera.window_bounds.x1) * 0.5f;
	center_y = (render.camera.window_bounds.y0 + render.camera.window_bounds.y1) * 0.5f;
	offset.x = screen.x - center_x;
	offset.y = screen.y - center_y;
	if (smooth_reticle || reticle_size != 1.0f)
		reticle_prepare(&offset, reticle_scale);
	/* Preserve the established draw exactly with both options at default. */
	point->x += (short)(offset.x / reticle_scale);
	point->y += (short)(offset.y / reticle_scale);
	return TRUE;
}

/* Called only for centre-anchored weapon reticles, after ordinary HUD/FOV
   scaling. Move fractional coordinates here, after the integer HUD layout;
   scale all artwork and authored offsets about the same projected aim point. */
void third_person_reticle_vertex(real *x, real *y)
{
	short player = render.local_player_index;
	struct shoulder_reticle_state const *state;
	real center_x, center_y;
	if (!third_person_active(player) || (!smooth_reticle && reticle_size == 1.0f))
		return;
	state = &reticles[player];
	if (!state->prepared || !state->valid)
		return;
	center_x = (render.camera.window_bounds.x0 + render.camera.window_bounds.x1) * 0.5f - render.camera.viewport_bounds.x0;
	center_y = (render.camera.window_bounds.y0 + render.camera.window_bounds.y1) * 0.5f - render.camera.viewport_bounds.y0;
	*x = center_x + state->shown.x + (*x - center_x - state->raw.x) * state->size;
	*y = center_y + state->shown.y + (*y - center_y - state->raw.y) * state->size;
}
