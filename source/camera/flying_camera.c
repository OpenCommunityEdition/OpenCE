/*
FLYING_CAMERA.C

symbols in this file:
000779A0 0020:
	_flying_camera_new (0000)
000779C0 0050:
	_flying_camera_new_from_point_and_vector (0000)
00077A10 04b0:
	_flying_camera_update (0000)
00256E34 0026:
	??_C@_0CG@FECMEHCO@c?3?2halo?2SOURCE?2camera?2flying_cam@ (0000)
*/

/* ---------- headers */

#include "flying_camera.h"
#include "observer.h"
#include "static_camera.h"
#include "rasterizer/rasterizer_console_vars.h"
#include "clips.h"
#include "physics/collisions.h"
#include "main/main.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* the free camera stops on the level and keeps sliding along it */
static void clips_fly_stop_at_walls(real_point3d *position, real_point3d const *from);
/* the stick and the keys ease in and out */
static void clips_fly_smooth(real_vector3d *step);

/* ---------- globals */

/* the eased step from the last frame. cleared when a flight starts */
static real_vector3d clips_fly_step;

/* ---------- public code */

void flying_camera_new(
	struct flying_camera *camera)
{
	camera->position.y = 0.f;
	camera->position.x = 0.f;
	camera->facing.yaw = 0.f;
	camera->facing.pitch = 0.f;
	camera->roll = 0.f;
	camera->field_of_view = DEGREES_TO_RADIANS(70);
	clips_fly_step.i = 0.f;
	clips_fly_step.j = 0.f;
	clips_fly_step.k = 0.f;
	return;
}

void flying_camera_new_from_point_and_vector(
	struct flying_camera *camera,
	real_point3d const *position,
	real_vector3d const *forward)
{
	flying_camera_new(camera);
	camera->position = *position;
	euler_angles2d_from_vector3d(&camera->facing, forward);
	return;
}

/* an edge hit leaves the plane distance unset. a face hit has a real one */
static boolean clips_fly_face_plane(struct collision_result const *hit)
{
	return hit->plane.d < 1.0e20f && hit->plane.d > -1.0e20f;
}

/* how far the center sits inside the pill around this face, or 0 when clear */
static real clips_fly_overlap(
	real_point3d const *position,
	struct collision_result const *hit,
	real radius)
{
	real dist;

	if (!clips_fly_face_plane(hit))
		return 0.f;
	dist = plane3d_distance_to_point(&hit->plane, position);
	if (dist >= radius)
		return 0.f;
	return radius - dist;
}

/* a touch reports a hit at the start of the move, including a move away from
the wall or along it. that is not a wall in front of the camera */
static boolean clips_fly_into_wall(
	struct collision_result const *hit,
	real_vector3d const *move)
{
	if (hit->t >= 1.f)
		return FALSE;
	if (hit->t <= 0.001f && dot_product3d(&hit->plane.n, move) >= -0.0001f)
		return FALSE;
	return TRUE;
}

/* step back out of a face the pill already overlaps, so the next move can leave */
static void clips_fly_push_out(real_point3d *position, real radius, real skin)
{
	static real_vector3d const dirs[6] = {
		{ 1.f, 0.f, 0.f },
		{ -1.f, 0.f, 0.f },
		{ 0.f, 1.f, 0.f },
		{ 0.f, -1.f, 0.f },
		{ 0.f, 0.f, 1.f },
		{ 0.f, 0.f, -1.f }
	};
	int pass;

	for (pass = 0; pass < 4; pass++)
	{
		int dir;
		boolean pushed = FALSE;

		for (dir = 0; dir < 6; dir++)
		{
			real_vector3d probe;
			struct collision_result hit;
			real out;

			probe.i = dirs[dir].i * (radius * 4.f);
			probe.j = dirs[dir].j * (radius * 4.f);
			probe.k = dirs[dir].k * (radius * 4.f);
			if (!collision_test_pill(
					FLAG(_collision_test_structure_bit),
					position,
					&probe,
					radius,
					NONE,
					&hit))
				continue;
			out = clips_fly_overlap(position, &hit, radius);
			if (out <= 0.001f)
				continue;
			if (out > 0.4f)
				out = 0.4f;
			position->x += hit.plane.n.i * (out + skin);
			position->y += hit.plane.n.j * (out + skin);
			position->z += hit.plane.n.k * (out + skin);
			pushed = TRUE;
		}
		if (!pushed)
			break;
	}
}

/* drop the part of the move that enters the face */
static void clips_fly_clip_move(real_vector3d *move, real_vector3d const *normal)
{
	real into = dot_product3d(normal, move);

	if (into < 0.f)
	{
		move->i -= normal->i * into;
		move->j -= normal->j * into;
		move->k -= normal->k * into;
	}
}

/* short steps, so a face we are already on cannot hide the wall ahead of it */
static void clips_fly_sweep(
	real_point3d *position,
	real_vector3d *move,
	real radius,
	real skin)
{
	real const piece = 0.05f;
	real_vector3d planes[2];
	int plane_count = 0;
	int guard;

	for (guard = 0; guard < 12; guard++)
	{
		real len;
		real_vector3d step;
		struct collision_result hit;
		boolean touched;
		real traveled;
		real t;
		real_vector3d wish;
		int plane;

		len = magnitude3d(move);
		if (len < 0.0001f)
			return;
		if (len > piece)
		{
			real scale = piece / len;

			step.i = move->i * scale;
			step.j = move->j * scale;
			step.k = move->k * scale;
		}
		else
			step = *move;
		touched = collision_test_pill(
			FLAG(_collision_test_structure_bit),
			position,
			&step,
			radius,
			NONE,
			&hit);
		/* sliding along a face we already touch. a short step cannot bury the camera */
		if (!touched || !clips_fly_into_wall(&hit, &step))
		{
			position->x += step.i;
			position->y += step.j;
			position->z += step.k;
			move->i -= step.i;
			move->j -= step.j;
			move->k -= step.k;
			if (touched && hit.t <= 0.001f)
				clips_fly_push_out(position, radius, skin);
			continue;
		}
		t = hit.t;
		if (t < 0.f)
			t = 0.f;
		if (t > 0.f)
		{
			real step_len = len > piece ? piece : len;
			real backoff = step_len > 0.0001f ? skin / step_len : 0.f;

			if (t > backoff)
				t -= backoff;
			else
				t = 0.f;
		}
		position->x += step.i * t;
		position->y += step.j * t;
		position->z += step.k * t;
		traveled = hit.t;
		if (traveled < 0.f)
			traveled = 0.f;
		if (traveled > 1.f)
			traveled = 1.f;
		move->i -= step.i * traveled;
		move->j -= step.j * traveled;
		move->k -= step.k * traveled;
		if (plane_count < 2)
		{
			planes[plane_count] = hit.plane.n;
			plane_count++;
		}
		else
		{
			planes[0] = planes[1];
			planes[1] = hit.plane.n;
		}
		wish = *move;
		for (plane = 0; plane < plane_count; plane++)
			clips_fly_clip_move(move, &planes[plane]);
		/* two walls meet. what is left of the move follows their edge */
		if (plane_count >= 2 && magnitude_squared3d(move) < 1.0e-8f)
		{
			real_vector3d crease;
			real along;

			cross_product3d(&planes[0], &planes[1], &crease);
			if (normalize3d(&crease) > 0.f)
			{
				along = dot_product3d(&crease, &wish);
				move->i = crease.i * along;
				move->j = crease.j * along;
				move->k = crease.k * along;
			}
		}
		clips_fly_push_out(position, radius, skin);
	}
}

static void clips_fly_stop_at_walls(real_point3d *position, real_point3d const *from)
{
	/* wide enough that the picture does not clip through, narrow enough for a doorway */
	real const radius = 0.15f;
	/* stay just off the face, so the next frame is not already inside it */
	real const skin = 0.01f;
	real_vector3d move;
	real len;

	vector_from_points3d(from, position, &move);
	*position = *from;
	clips_fly_push_out(position, radius, skin);
	if (magnitude_squared3d(&move) < 1.0e-8f)
		return;
	/* a long frame must not jump through a wall and come out inside it */
	len = magnitude3d(&move);
	if (len > 0.5f)
	{
		real scale = 0.5f / len;

		move.i *= scale;
		move.j *= scale;
		move.k *= scale;
	}
	clips_fly_sweep(position, &move, radius, skin);
}

static void clips_fly_smooth(real_vector3d *step)
{
	/* about a tenth of a second to catch up, so a start and a stop ease off */
	real const ease = 0.10f;
	real dt;
	real blend;

	dt = main_get_seconds_elapsed();
	if (dt < 0.0001f)
		dt = 0.0001f;
	if (dt > 0.05f)
		dt = 0.05f;
	blend = dt / (dt + ease);
	clips_fly_step.i += (step->i - clips_fly_step.i) * blend;
	clips_fly_step.j += (step->j - clips_fly_step.j) * blend;
	clips_fly_step.k += (step->k - clips_fly_step.k) * blend;
	*step = clips_fly_step;
}

void flying_camera_update(
	struct flying_camera *camera,
	struct flying_camera_action const *controls,
	struct camera_command *result)
{
	match_assert("c:\\halo\\SOURCE\\camera\\flying_camera.c", 41, camera);
	match_assert("c:\\halo\\SOURCE\\camera\\flying_camera.c", 42, controls);
	match_assert("c:\\halo\\SOURCE\\camera\\flying_camera.c", 43, result);

	if (controls->active)
	{
		camera->facing.yaw += controls->facing_delta.yaw;
		camera->facing.pitch = PIN(
			camera->facing.pitch + controls->facing_delta.pitch,
			-1.56765485f,
			1.56765485f);
		camera->roll += controls->facing_delta.roll;
	}

	if (rasterizer_debug_options.freeze_flying_camera > 0)
	{
		camera->facing.yaw = 0.f;
		camera->facing.pitch = 0.f;
		camera->roll = 0.f;
		rasterizer_debug_options.freeze_flying_camera--;
	}

	result->timer = clips_flying() ? 0.f : 0.3f;
	vector3d_from_euler_angles2d(&result->forward, &camera->facing);
	observer_up_from_forward(&result->forward, &result->up);
	rotate_vector_about_axis(
		&result->up,
		&result->forward,
		sine(camera->roll),
		cosine(camera->roll));

	if (controls->active)
	{
		real cosine_yaw = cosine(camera->facing.yaw);
		real sine_yaw = sine(camera->facing.yaw);
		real_vector3d translation;
		real_point3d position;

		set_real_vector3d(
			&translation,
			cosine_yaw*controls->translation.i - sine_yaw*controls->translation.j,
			cosine_yaw*controls->translation.j + sine_yaw*controls->translation.i,
			controls->translation.k);
		if (clips_flying())
			clips_fly_smooth(&translation);
		position.x = camera->position.x + translation.i;
		position.y = camera->position.y + translation.j;
		position.z = camera->position.z + translation.k;
		if (clips_flying())
			clips_fly_stop_at_walls(&position, &camera->position);
		camera->position = position;
	}

	result->position = camera->position;
	result->offset = *global_zero_vector3d;
	result->depth = 0.f;
	result->field_of_view = camera->field_of_view;
	result->flags = FLAG(_observer_command_valid_bit);

	match_vassert(
		"c:\\halo\\SOURCE\\camera\\flying_camera.c",
		149,
		!TEST_FLAG(result->flags, _observer_command_valid_bit) ||
		(valid_real_vector3d_axes2(&result->forward, &result->up) &&
			valid_real(result->position.x) && result->position.x>=-5000.f && result->position.x<=5000.f &&
			valid_real(result->position.y) && result->position.y>=-5000.f && result->position.y<=5000.f &&
			valid_real(result->position.z) && result->position.z>=-5000.f && result->position.z<=5000.f &&
			valid_real(result->offset.i) && result->offset.i>=-5000.f && result->offset.i<=5000.f &&
			valid_real(result->offset.j) && result->offset.j>=-5000.f && result->offset.j<=5000.f &&
			valid_real(result->offset.k) && result->offset.k>=-5000.f && result->offset.k<=5000.f &&
			valid_real_vector3d(&result->velocity) &&
			valid_real(result->depth) && result->depth>=0.f && result->depth<=5000.f &&
			valid_real(result->field_of_view) && result->field_of_view>=0.001f && result->field_of_view<=_pi / 2.f &&
			valid_real(result->timer) && result->timer>=0.f && result->timer<=3600.f),
		csprintf(
			temporary,
			"Invalid camera command.\nF: (%f, %f, %f) U: (%f, %f, %f)\nP: (%f, %f, %f) O: (%f, %f, %f)\nD: %f V: (%f, %f, %f), FOV: %f, T: %f, FL: %ld",
			result->forward.i,
			result->forward.j,
			result->forward.k,
			result->up.i,
			result->up.j,
			result->up.k,
			result->position.x,
			result->position.y,
			result->position.z,
			result->offset.i,
			result->offset.j,
			result->offset.k,
			result->depth,
			result->velocity.i,
			result->velocity.j,
			result->velocity.k,
			result->field_of_view,
			result->timer,
			result->flags));

	return;
}

/* ---------- private code */
