/*
VR_RENDER.C

The VR mode's view (HALO_VR; port/linux/src/vr.h, port/linux/README.md
"VR"), for render.c and the rasterizer.

The game draws its 3D view once a frame, as on the flat screen, from the
centre of the head: the player's eye (where the game puts it, interpolated
between ticks) moved by the head's position, turned by the head's
orientation, its field of view the union of both eyes' (culling and LOD
cover both). The renderer draws each draw into both eyes at once
(GL_OVR_multiview2, port/linux/src/d3d8_gl.c), moving each vertex's clip
position from the centre view's to its eye's: the eyes look the way the
centre does, offset sideways, so the move is a fixed 4x4 per eye for the
projection the game has set, clip_eye = P_eye V_eye inverse(P_centre
V_centre) clip_centre, made here each time the game sets one
(rasterizer_set_frustum_z: the world's, the first-person weapon's and the
sky's differ in depth range only).

The HUD is drawn afterwards, from a camera looking where the head does with
the HUD layer's field of view, so that the reticle and waypoints fall where
they belong on the layer before the eyes.
*/

#ifdef HALO_VR

#include "cseries.h"
#include "math/real_math.h"
#include "render/render_cameras.h"
#include "render/render_cameras_internal.h"
#include "cutscene/cinematics.h"
#include "camera/director.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "tag_files/tag_files.h"
#include "interface/hud.h"
#include "physics/collision_usage.h"
#include "physics/collisions.h"
#include "render/render.h"

#include "../src/vr.h"
#include "vr_hands.h"

#include <math.h>
#include <string.h>

/* the union of the eyes' fields of view, a little wider (the eyes are
apart, and the culling's planes start at the centre) */
#define UNION_MARGIN 1.03f
/* the crosshairs in the eyes (vr_render_crosshairs): where the aim meets
the world, no farther than this (world units) nor nearer than this, the
distance's changes smoothed over this time constant (seconds) */
#define CROSSHAIR_FARTHEST 100.0f
#define CROSSHAIR_NEAREST 0.15f
#define CROSSHAIR_SMOOTHING 0.06f

static struct
{
	boolean active;
	struct halo_vr_view view;
	/* the centre camera's frustum bounds (render_camera_build_frustum's
	units) */
	real_rectangle2d bounds;
	/* each eye's frustum, of the eye's own camera */
	struct render_frustum eyes[2];
	/* the player's eye as the game has it, and the heading the head's and
	controllers' poses are turned by */
	real_point3d base_position;
	real base_yaw;
	/* the player's unit, seated in a vehicle and viewed from its head (not
	drawn: render_objects.c), else NONE */
	long seated_unit_index;
	/* each eye's frustum moved to the centre (the sky's: vr_render_sky) */
	struct render_frustum eyes_at_centre[2];
	boolean infinite;
	/* an eye's pixels for each of the centre view's pixels, at the middle
	of the view (vr_render_level_of_detail_scale) */
	real level_of_detail_scale;
	/* the projection set last (vr_render_projection_set): its clip
	transform, its depth row, and the eyes' transforms made from it */
	boolean projection_valid;
	real centre[4][4];
	real projection_depth[4];
	float transforms[2][16];
	/* the camera the first-person weapon was posed from this frame
	(vr_render_weapon_camera), and whether it is in the right controller */
	boolean weapon_camera_set;
	boolean weapon_camera_valid;
	struct render_camera weapon_camera;
	/* the eyes' middle, in the world */
	real_point3d head_position;
	/* where the gamepad aims (vr.aim = "gamepad"): the player's facing, and
	up from it */
	real_vector3d aim_forward;
	real_vector3d aim_up;
	/* the crosshairs were drawn in the eyes this frame (not in the HUD's
	layer), and the inverse of their distance, smoothed */
	boolean crosshairs_drawn;
	boolean crosshair_distance_valid;
	real crosshair_inverse_distance;
} vr_render;

/* where the crosshairs are drawn from the HUD's middle, in its 640x480
(hud_draw.c): where the right controller points */
short vr_render_crosshair_offset[2];

void vr_render_screen_point_end(void);

/* ---------- maths */

static void yaw_rotate(real yaw, const float local[3], real_vector3d *world)
{
	real c = (real)cos(yaw), s = (real)sin(yaw);

	world->i = local[0] * c - local[1] * s;
	world->j = local[0] * s + local[1] * c;
	world->k = local[2];
}

/* a vector of the head (game axes from the recentred head) in the world:
pitched up about the body's left, then turned by the yaw (the facing's,
with vr.gamepad_view = "camera": the head looks about from the aim) */
static void view_rotate(real yaw, real pitch, const float local[3], real_vector3d *world)
{
	real c = (real)cos(pitch), s = (real)sin(pitch);
	float pitched[3];

	pitched[0] = (float)(local[0] * c - local[2] * s);
	pitched[1] = local[1];
	pitched[2] = (float)(local[0] * s + local[2] * c);
	yaw_rotate(yaw, pitched, world);
}

/* the view's pitch before the head's: the facing's while the gamepad aims
with vr.gamepad_view = "camera", else none (level with the horizon) */
static real view_pitch(void)
{
	if (!vr_render.view.gamepad_pitch || !halo_vr_aiming() || cinematic_in_progress() ||
		local_player_get_player_index(0) == NONE)
	{
		return 0.0f;
	}
	return player_control_get_facing_angles(0)->pitch;
}

/* the clip transform the game uploads (rasterizer_set_frustum_z): clip[i]
= sum over j of m[i][j] * (x, y, z, 1)[j] (in double: see eye_transforms_set) */
static void frustum_clip_transform(const struct render_frustum *frustum, double m[4][4])
{
	short column, row;

	for (column = 0; column < 4; column++)
	{
		for (row = 0; row < 4; row++)
		{
			const real *n = frustum->world_to_view.n[row];

			m[column][row] = (double)n[0] * frustum->projection_matrix[0][column] +
				(double)n[1] * frustum->projection_matrix[1][column] +
				(double)n[2] * frustum->projection_matrix[2][column];
		}
		m[column][3] += frustum->projection_matrix[3][column];
	}
}

static boolean matrix4_inverse(real m[4][4], double out[4][4])
{
	double a[4][8];
	short i, j, k;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			a[i][j] = m[i][j];
			a[i][j + 4] = i == j ? 1.0 : 0.0;
		}
	}
	for (i = 0; i < 4; i++)
	{
		short pivot = i;
		double scale;

		for (k = i + 1; k < 4; k++)
		{
			if (fabs(a[k][i]) > fabs(a[pivot][i]))
				pivot = k;
		}
		if (fabs(a[pivot][i]) < 1e-12)
			return FALSE;
		if (pivot != i)
		{
			for (j = 0; j < 8; j++)
			{
				double t = a[i][j];

				a[i][j] = a[pivot][j];
				a[pivot][j] = t;
			}
		}
		scale = 1.0 / a[i][i];
		for (j = 0; j < 8; j++)
			a[i][j] *= scale;
		for (k = 0; k < 4; k++)
		{
			if (k != i && a[k][i] != 0.0)
			{
				double factor = a[k][i];

				for (j = 0; j < 8; j++)
					a[k][j] -= factor * a[i][j];
			}
		}
	}
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
			out[i][j] = a[i][j + 4];
	}
	return TRUE;
}

/* ---------- the view */

/* the first player's unit, seated in a vehicle and not in a cinematic or
under a scripted camera, else NONE */
static long seated_unit(void)
{
	long player_index = local_player_get_player_index(0);
	long unit_index;
	struct object_datum *object;

	if (player_index == NONE || cinematic_in_progress() ||
		director_get_perspective(0) == _director_perspective_scripted)
	{
		return NONE;
	}
	unit_index = player_get(player_index)->unit_index;
	if (unit_index == NONE)
		return NONE;
	object = object_get(unit_index);
	if (object->object.type != _object_type_biped || object->object.parent_object_index == NONE ||
		!TEST_FLAG(_object_mask_unit, object_get(object->object.parent_object_index)->object.type))
	{
		return NONE;
	}
	return unit_index;
}

/* turns the window's camera into the VR frame's centre camera (both its
render and rasterizer cameras); FALSE without a VR frame to draw */
boolean vr_render_camera(struct render_camera *camera, struct render_camera *rasterizer_camera)
{
	struct halo_vr_view *view = &vr_render.view;
	real base_yaw;
	real pitch;
	real_vector3d offset;
	real left = 0.0f, right = 0.0f, up = 0.0f, down = 0.0f, tangent;
	real aspect;
	short eye;

	vr_render.active = FALSE;
	vr_render.seated_unit_index = NONE;
	vr_render.weapon_camera_set = FALSE;
	vr_render.weapon_camera_valid = FALSE;
	if (!halo_vr_view(view))
		return FALSE;
	/* the turn of the stick (and of the aim) while the aim follows the head;
	else (a cinematic, the main menu's scene) the camera's own heading */
	base_yaw = cinematic_in_progress() || !halo_vr_aiming() ?
		(real)atan2(camera->forward.j, camera->forward.i) : view->body_yaw;
	if (view->vehicle_first_person && halo_vr_aiming() &&
		(vr_render.seated_unit_index = seated_unit()) != NONE)
	{
		/* in a vehicle's seat (vr.vehicle_view): the eye is the seated
		player's head, as the frame poses it between ticks, and the view is
		level and turned as on foot; not the game's camera following the
		vehicle, which swings about it with the aim */
		unit_get_head_position(vr_render.seated_unit_index, &camera->position);
	}
	vr_render.base_position = camera->position;
	vr_render.base_yaw = base_yaw;
	vr_render.aim_forward = camera->forward;
	vr_render.aim_up = camera->up;
	if (view->gamepad_aim && halo_vr_aiming() && local_player_get_player_index(0) != NONE)
	{
		/* the player's facing, not the camera: the camera shakes (firing,
		explosions: player_effect_get_camera_effect_matrix), which the
		weapon and the reticle, before a still head, need not */
		real along;

		player_control_get_facing_direction(0, &vr_render.aim_forward);
		vr_render.aim_up = *global_up3d;
		along = dot_product3d(&vr_render.aim_up, &vr_render.aim_forward);
		vr_render.aim_up.i -= vr_render.aim_forward.i * along;
		vr_render.aim_up.j -= vr_render.aim_forward.j * along;
		vr_render.aim_up.k -= vr_render.aim_forward.k * along;
		if (normalize3d(&vr_render.aim_up) <= 0.0f)
			vr_render.aim_up = camera->up;
	}
	pitch = view_pitch();
	view_rotate(base_yaw, pitch, view->head_forward, &camera->forward);
	view_rotate(base_yaw, pitch, view->head_up, &camera->up);
	view_rotate(base_yaw, pitch, view->head_position, &offset);
	camera->position.x += offset.i * view->world_units_per_metre;
	camera->position.y += offset.j * view->world_units_per_metre;
	camera->position.z += offset.k * view->world_units_per_metre;
	vr_render.head_position = camera->position;

	for (eye = 0; eye < 2; eye++)
	{
		left = eye ? MIN(left, view->fov[eye][0]) : view->fov[eye][0];
		right = eye ? MAX(right, view->fov[eye][1]) : view->fov[eye][1];
		up = eye ? MAX(up, view->fov[eye][2]) : view->fov[eye][2];
		down = eye ? MIN(down, view->fov[eye][3]) : view->fov[eye][3];
	}
	left *= UNION_MARGIN;
	right *= UNION_MARGIN;
	up *= UNION_MARGIN;
	down *= UNION_MARGIN;
	tangent = MAX(up, -down);
	camera->vertical_field_of_view = 2.0f * (real)atan(tangent);
	aspect = (real)(camera->viewport_bounds.x1 - camera->viewport_bounds.x0) /
		(real)(camera->viewport_bounds.y1 - camera->viewport_bounds.y0);
	vr_render.bounds.x0 = left / (aspect * tangent);
	vr_render.bounds.x1 = right / (aspect * tangent);
	vr_render.bounds.y0 = down / tangent;
	vr_render.bounds.y1 = up / tangent;
	camera->mirrored = FALSE;
	rasterizer_camera->position = camera->position;
	rasterizer_camera->forward = camera->forward;
	rasterizer_camera->up = camera->up;
	rasterizer_camera->vertical_field_of_view = camera->vertical_field_of_view;
	rasterizer_camera->mirrored = FALSE;

	/* each eye's camera: the centre's, moved along the head's axes across
	the view (not along it: an eye's depth is then the centre's, which
	eye_transforms_set relies on; the eyes are side by side, a millimetre or
	so at most from the plane through the centre across the view) */
	for (eye = 0; eye < 2; eye++)
	{
		struct render_camera eye_camera = *camera;
		real_rectangle2d eye_bounds;
		real_vector3d left_axis;
		real units = view->world_units_per_metre;
		const float *o = view->eye_offset[eye];

		cross_product3d(&camera->up, &camera->forward, &left_axis);
		normalize3d(&left_axis);
		eye_camera.position.x += (left_axis.i * o[1] + camera->up.i * o[2]) * units;
		eye_camera.position.y += (left_axis.j * o[1] + camera->up.j * o[2]) * units;
		eye_camera.position.z += (left_axis.k * o[1] + camera->up.k * o[2]) * units;
		eye_bounds.x0 = view->fov[eye][0] / (aspect * tangent);
		eye_bounds.x1 = view->fov[eye][1] / (aspect * tangent);
		eye_bounds.y0 = view->fov[eye][3] / tangent;
		eye_bounds.y1 = view->fov[eye][2] / tangent;
		render_camera_build_frustum(&eye_camera, &eye_bounds, &vr_render.eyes[eye], TRUE);
		render_camera_build_frustum(camera, &eye_bounds, &vr_render.eyes_at_centre[eye], TRUE);
	}
	{
		/* the centre view has the game's viewport (480 lines) over the
		union's height (up - down, in tangents); an eye has its image's
		height over its own */
		int eye_width = 0, eye_height = 0;
		real eye_span = MAX(view->fov[0][2] - view->fov[0][3], view->fov[1][2] - view->fov[1][3]);
		real viewport_height = (real)(camera->viewport_bounds.y1 - camera->viewport_bounds.y0);

		halo_vr_eye_size(&eye_width, &eye_height);
		vr_render.level_of_detail_scale = eye_height > 0 && eye_span > 0.0f && viewport_height > 0.0f ?
			((real)eye_height / eye_span) / (viewport_height / (up - down)) : 1.0f;
	}
	vr_render.infinite = FALSE;
	vr_render.projection_valid = FALSE;
	vr_render.active = TRUE;
	return TRUE;
}

/* render_objects.c: what the size of an object in the centre view's
pixels (the game's viewport, 480 lines over the union of the eyes' fields
of view) is in an eye's (its image's lines over its field of view), for the
models' detail levels, the lighting's refresh and the shadows: the game
judged them by the centre view, about 3.6 times as small as the headset
shows them (1728 lines an eye). 1 outside the eye pass. */
real vr_render_level_of_detail_scale(void)
{
	return vr_render.active ? vr_render.level_of_detail_scale : 1.0f;
}

/* the centre camera's frustum bounds (render_player_frame) */
void vr_render_frustum_bounds(real_rectangle2d *bounds)
{
	if (vr_render.active)
		*bounds = vr_render.bounds;
}

/* a hand's place (game axes from the recentred head, metres) in the world */
static void hand_point(const float place[3], real_point3d *point)
{
	real_vector3d offset;
	real units = vr_render.view.world_units_per_metre;

	yaw_rotate(vr_render.base_yaw, place, &offset);
	point->x = vr_render.base_position.x + offset.i * units;
	point->y = vr_render.base_position.y + offset.j * units;
	point->z = vr_render.base_position.z + offset.k * units;
}

/* the camera the first-person weapon is posed from (render.c): with the
controller aiming, one placed so that the weapon is in the right controller,
pointing where it points: with the hands floating (vr.hands), the right
hand's middle (as the weapon's idle pose has it) at the controller's grip,
else the weapon's grip vr.weapon_offset from the camera; FALSE leaves the
head's */
boolean vr_render_weapon_camera(struct render_camera *camera)
{
	struct halo_vr_view *view = &vr_render.view;
	real_vector3d forward, up, right, left;
	real_point3d hand, centre;
	real units = view->world_units_per_metre;
	const float *o = view->weapon_offset;

	if (!vr_render.active)
		return FALSE;
	/* (the head's, kept for the hands when the controller is not tracked) */
	vr_render.weapon_camera = *camera;
	vr_render.weapon_camera_set = halo_vr_aiming();
	if (view->gamepad_aim && halo_vr_aiming())
	{
		/* the gamepad aims (vr.aim = "gamepad"): the weapon is posed as on
		the flat screen, from the player's eye looking where the player
		faces, there whichever way the head looks or leans */
		camera->position = vr_render.base_position;
		camera->forward = vr_render.aim_forward;
		camera->up = vr_render.aim_up;
		vr_render.weapon_camera = *camera;
		vr_render.weapon_camera_valid = TRUE;
		return TRUE;
	}
	if (!view->controller_aim || !halo_vr_aiming())
		return FALSE;
	yaw_rotate(vr_render.base_yaw, view->hand_forward, &forward);
	yaw_rotate(vr_render.base_yaw, view->hand_up, &up);
	cross_product3d(&forward, &up, &right);
	normalize3d(&right);
	camera->forward = forward;
	camera->up = up;
	if (view->floating_hands && vr_hands_right_centre(&centre))
	{
		hand_point(view->right_grip_position, &hand);
		left.i = -right.i;
		left.j = -right.j;
		left.k = -right.k;
		camera->position.x = hand.x - (forward.i * centre.x + left.i * centre.y + up.i * centre.z);
		camera->position.y = hand.y - (forward.j * centre.x + left.j * centre.y + up.j * centre.z);
		camera->position.z = hand.z - (forward.k * centre.x + left.k * centre.y + up.k * centre.z);
	}
	else
	{
		hand_point(view->hand_position, &hand);
		camera->position.x = hand.x - (right.i * o[0] + up.i * o[1] + forward.i * o[2]) * units;
		camera->position.y = hand.y - (right.j * o[0] + up.j * o[1] + forward.j * o[2]) * units;
		camera->position.z = hand.z - (right.k * o[0] + up.k * o[1] + forward.k * o[2]) * units;
	}
	vr_render.weapon_camera = *camera;
	vr_render.weapon_camera_valid = TRUE;
	return TRUE;
}

/* vr_hands.c: the frame the hands are posed in (vr.hands), when the weapon
was posed for the eyes (vr_render_weapon_camera); FALSE when the hands are
the game's */
boolean vr_render_hands(struct vr_hands_frame *frame)
{
	struct halo_vr_view *view = &vr_render.view;
	struct render_camera const *camera = &vr_render.weapon_camera;

	if (!vr_render.active || !vr_render.weapon_camera_set || !view->floating_hands)
		return FALSE;
	frame->arms = view->arms;
	matrix4x3_from_point_and_vectors(&frame->camera, &camera->position, &camera->forward, &camera->up);
	frame->right_valid = vr_render.weapon_camera_valid;
	if (frame->right_valid)
	{
		real_point3d position;

		hand_point(view->right_grip_position, &position);
		matrix4x3_from_point_and_vectors(&frame->right, &position, &camera->forward, &camera->up);
	}
	frame->left_valid = view->left_hand_valid;
	if (frame->left_valid)
	{
		real_vector3d forward, up;
		real_point3d position;

		yaw_rotate(vr_render.base_yaw, view->left_hand_forward, &forward);
		yaw_rotate(vr_render.base_yaw, view->left_hand_up, &up);
		normalize3d(&forward);
		normalize3d(&up);
		hand_point(view->left_grip_position, &position);
		matrix4x3_from_point_and_vectors(&frame->left, &position, &forward, &up);
	}
	frame->two_handed = view->two_handed;
	frame->right_trigger = view->right_trigger;
	frame->left_grip = view->left_grip;
	frame->head = vr_render.head_position;
	frame->base_yaw = vr_render.base_yaw;
	frame->head_yaw = (real)atan2(view->head_forward[1], view->head_forward[0]);
	frame->shoulder[0] = view->shoulder_offset[0];
	frame->shoulder[1] = view->shoulder_offset[1];
	frame->shoulder[2] = view->shoulder_offset[2];
	frame->units = view->world_units_per_metre;
	frame->seconds = view->seconds;
	return TRUE;
}

/* player_control.c: the weapon is a long gun, which the left hand may hold
by its foregrip too (vr.two_handed): any but the pistols, the needler, the
flag and the ball */
boolean vr_render_long_gun(long weapon_index)
{
	static char const *const one_handed[] = { "pistol", "needler", "flag", "ball" };
	char const *name;
	short index;

	if (weapon_index == NONE)
		return FALSE;
	name = tag_get_name(object_get(weapon_index)->definition_index);
	if (!name)
		return FALSE;
	for (index = 0; index < (short)NUMBEROF(one_handed); index++)
	{
		if (strstr(name, one_handed[index]))
			return FALSE;
	}
	return TRUE;
}

/* the eye pass is over: the projections set from now on are no eye's */
void vr_render_end(void)
{
	vr_render.active = FALSE;
	vr_render.seated_unit_index = NONE;
	vr_render.infinite = FALSE;
	vr_render_screen_point_end();
}

/* the object is the player's unit, which the eye pass views its vehicle
from the head of: not drawn, as a first-person player's is not
(render_objects.c) */
boolean vr_render_hides_object(long object_index)
{
	return vr_render.active && object_index == vr_render.seated_unit_index;
}

/* the camera the HUD is drawn from (in place of the window's): where the
head looks, with the HUD layer's field of view; FALSE without a VR frame */
boolean vr_render_hud_camera(struct render_camera *camera)
{
	struct halo_vr_view *view = &vr_render.view;
	real base_yaw;

	if (!halo_vr_frame_active())
		return FALSE;
	base_yaw = cinematic_in_progress() || !halo_vr_aiming() ?
		(real)atan2(camera->forward.j, camera->forward.i) : view->body_yaw;
	vr_render_crosshair_offset[0] = vr_render_crosshair_offset[1] = 0;
	if (vr_render.crosshairs_drawn)
	{
		/* (drawn in the eyes: vr_render_crosshairs) */
		vr_render_crosshair_offset[0] = 10000;
		vr_render.crosshairs_drawn = FALSE;
	}
	if (view->hud_tangent > 0.0f)
	{
		real pitch = view_pitch();

		view_rotate(base_yaw, pitch, view->head_forward, &camera->forward);
		view_rotate(base_yaw, pitch, view->head_up, &camera->up);
		camera->vertical_field_of_view = 2.0f * (real)atan(view->hud_tangent);
		if ((view->controller_aim || view->gamepad_aim) && halo_vr_aiming() && !vr_render_crosshair_offset[0])
		{
			/* a point far along the controller (or the player's facing, where
			the gamepad aims), where the HUD shows it */
			struct render_frustum frustum;
			real_vector3d forward;
			real_point3d point;
			real_point2d screen;

			render_camera_build_frustum(camera, NULL, &frustum, TRUE);
			if (view->gamepad_aim)
				forward = vr_render.aim_forward;
			else
				yaw_rotate(base_yaw, view->hand_forward, &forward);
			point.x = camera->position.x + forward.i * 100.0f;
			point.y = camera->position.y + forward.j * 100.0f;
			point.z = camera->position.z + forward.k * 100.0f;
			if (render_camera_world_to_screen(camera, &frustum, &point, &screen))
			{
				vr_render_crosshair_offset[0] = (short)(screen.x - (camera->viewport_bounds.x0 + camera->viewport_bounds.x1) * 0.5f);
				vr_render_crosshair_offset[1] = (short)(screen.y - (camera->viewport_bounds.y0 + camera->viewport_bounds.y1) * 0.5f);
			}
			else
			{
				/* (off the HUD: not drawn) */
				vr_render_crosshair_offset[0] = 10000;
			}
		}
	}
	return TRUE;
}

/* render.c, at the end of the eye pass: the crosshairs, drawn in each eye
where the aim meets the world (vr.aim = "gamepad": the facing from the
player's eye; "controller": the right controller's line), as the
first-person weapon is, and as large as in the HUD's layer. In that layer,
in front of the head, they were placed for the head's pose as the frame was
drawn, which the compositor moved them with to the pose shown while it
moved the eyes' images to it, so they swam against the world as the head
moved, and at the layer's distance the eyes saw them doubled against what
they aimed at. Drawn in the screen, about the middle as the game draws
them, then moved to the point in each eye by a scale and offset (no
rounding to the HUD's pixels). */
void vr_render_crosshairs(void)
{
	struct halo_vr_view *view = &vr_render.view;
	real_point3d origin, point;
	real_vector3d direction, vector;
	struct collision_result collision;
	long player_index = local_player_get_player_index(0);
	long ignore_index = NONE;
	real distance = CROSSHAIR_FARTHEST, width, height, aspect, centre_x, centre_y;
	float corrections[2][4];
	short eye, i;

	if (!vr_render.active || !halo_vr_aiming() || cinematic_in_progress() || view->hud_tangent <= 0.0f ||
		!(view->gamepad_aim || view->controller_aim) || player_index == NONE)
	{
		vr_render.crosshair_distance_valid = FALSE;
		return;
	}
	if (view->gamepad_aim)
	{
		origin = vr_render.base_position;
		direction = vr_render.aim_forward;
	}
	else
	{
		hand_point(view->hand_position, &origin);
		yaw_rotate(vr_render.base_yaw, view->hand_forward, &direction);
	}
	if (normalize3d(&direction) <= 0.0f)
		return;
	if (player_get(player_index)->unit_index != NONE)
		ignore_index = object_get_ultimate_parent(player_get(player_index)->unit_index);
	vector.i = direction.i * CROSSHAIR_FARTHEST;
	vector.j = direction.j * CROSSHAIR_FARTHEST;
	vector.k = direction.k * CROSSHAIR_FARTHEST;
	if (global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH)
	{
		global_current_collision_users[global_current_collision_user_depth++] = _collision_user_ui;
		if (collision_test_vector(_collision_test_for_line_of_sight_flags | FLAG(_collision_test_objects_bipeds_bit),
			&origin, &vector, ignore_index, &collision))
		{
			distance = (real)sqrt(distance_squared3d(&origin, &collision.point));
		}
		--global_current_collision_user_depth;
	}
	if (distance < CROSSHAIR_NEAREST)
		distance = CROSSHAIR_NEAREST;
	if (!vr_render.crosshair_distance_valid)
	{
		vr_render.crosshair_inverse_distance = 1.0f / distance;
		vr_render.crosshair_distance_valid = TRUE;
	}
	else
	{
		real t = view->seconds > 0.0f ? 1.0f - (real)exp(-view->seconds / CROSSHAIR_SMOOTHING) : 1.0f;

		vr_render.crosshair_inverse_distance += (1.0f / distance - vr_render.crosshair_inverse_distance) * t;
	}
	distance = 1.0f / vr_render.crosshair_inverse_distance;
	point.x = origin.x + direction.i * distance;
	point.y = origin.y + direction.j * distance;
	point.z = origin.z + direction.k * distance;

	/* the game's screen (the viewport, its pixels: x = ((ndc + 1) width -
	1) / 2, y from the top), and its middle, where the game draws them */
	width = (real)(render.camera.viewport_bounds.x1 - render.camera.viewport_bounds.x0);
	height = (real)(render.camera.viewport_bounds.y1 - render.camera.viewport_bounds.y0);
	if (width <= 0.0f || height <= 0.0f)
		return;
	aspect = width / height;
	centre_x = (real)(short)((render.camera.window_bounds.x1 + render.camera.window_bounds.x0) / 2) -
		render.camera.viewport_bounds.x0;
	centre_y = (real)(short)((render.camera.window_bounds.y1 + render.camera.window_bounds.y0) / 2) -
		render.camera.viewport_bounds.y0;
	for (eye = 0; eye < 2; eye++)
	{
		double m[4][4];
		real clip[4], screen_x, screen_y, scale_x, scale_y;
		const float *fov = view->fov[eye];

		frustum_clip_transform(&vr_render.eyes[eye], m);
		for (i = 0; i < 4; i++)
			clip[i] = (real)(m[i][0] * point.x + m[i][1] * point.y + m[i][2] * point.z + m[i][3]);
		if (clip[3] <= 0.0f || fov[1] <= fov[0] || fov[2] <= fov[3])
			return;
		screen_x = ((clip[0] / clip[3] + 1.0f) * width - 1.0f) * 0.5f;
		screen_y = ((1.0f - clip[1] / clip[3]) * height - 1.0f) * 0.5f;
		/* the HUD's layer has height / (2 hud_tangent) pixels a unit of
		tangent, an eye width / (right - left) across and height / (up -
		down) up */
		scale_x = 2.0f * view->hud_tangent * aspect / (fov[1] - fov[0]);
		scale_y = 2.0f * view->hud_tangent / (fov[2] - fov[3]);
		corrections[eye][0] = (float)scale_x;
		corrections[eye][1] = (float)scale_y;
		corrections[eye][2] = (float)(screen_x - scale_x * centre_x);
		corrections[eye][3] = (float)(screen_y - scale_y * centre_y);
	}
	vr_render_crosshair_offset[0] = vr_render_crosshair_offset[1] = 0;
	halo_vr_set_screen_corrections(corrections);
	/* (each eye in a draw of its own: halo_vr_draw_eye) */
	for (eye = 0; eye < 2; eye++)
	{
		halo_vr_draw_eye(eye);
		hud_draw_vr_crosshairs();
	}
	halo_vr_draw_eye(-1);
	vr_render_screen_point_end();
	vr_render.crosshairs_drawn = TRUE;
}

/* the eyes' transforms for the projection set last (vr_render.centre):
each eye's own (or, for the sky, the eye where the centre is, its view
turned the same way: the sky is at infinity).

An eye's depth (its clip z and w) is the centre's: the eyes are beside the
centre and look the same way, with the same depth range. So those rows are
exactly the identity's. Made from the matrices they were not: the inverse
of the centre's clip transform loses the precision that depth needs (the
near plane 0.0625, the far 1024), and its rounding, new each frame, moved
the depth of all that was far: some frames a few parts in ten thousand,
enough to put all beyond about 100 world units past the far plane (where
the horizon's water flickered, and, while the whole eye pass was
depth-clamped, all of it was at one depth and the last drawn won: the
Pillar of Autumn's hull in a10's opening went dark in single frames, its
far side drawn over its near). The eyes' x and y are made in double
precision. */
static void eye_transforms_set(void)
{
	float transforms[2][16];
	double inverse[4][4];
	short eye, i, j, k;

	memset(transforms, 0, sizeof(transforms));
	if (!vr_render.active || !vr_render.projection_valid || !matrix4_inverse(vr_render.centre, inverse))
	{
		for (eye = 0; eye < 2; eye++)
		{
			for (i = 0; i < 4; i++)
				transforms[eye][i * 5] = 1.0f;
		}
	}
	else
	{
		for (eye = 0; eye < 2; eye++)
		{
			struct render_frustum eye_frustum = vr_render.infinite ? vr_render.eyes_at_centre[eye] : vr_render.eyes[eye];
			double m[4][4];

			/* the depth range is the centre's as set (the first-person
			weapon's, the sky's): the eyes look the same way, so their depth is
			the same */
			for (k = 0; k < 4; k++)
				eye_frustum.projection_matrix[k][2] = vr_render.projection_depth[k];
			frustum_clip_transform(&eye_frustum, m);
			for (i = 0; i < 2; i++)
			{
				for (j = 0; j < 4; j++)
				{
					double sum = 0.0;

					for (k = 0; k < 4; k++)
						sum += m[i][k] * inverse[k][j];
					transforms[eye][i * 4 + j] = (float)sum;
				}
			}
			/* (z and w: the centre's) */
			transforms[eye][2 * 4 + 2] = 1.0f;
			transforms[eye][3 * 4 + 3] = 1.0f;
		}
	}
	memcpy(vr_render.transforms, transforms, sizeof(transforms));
	halo_vr_set_eye_transforms(transforms);
}

/* rasterizer_set_frustum_z: the game has set the projection with this clip
transform (centre); the renderer gets each eye's from it */
void vr_render_projection_set(const struct render_frustum *frustum, real centre[4][4])
{
	short k;

	vr_render.projection_valid = vr_render.active;
	if (vr_render.active)
	{
		/* the depth range the eyes' depth is submitted with (the world's:
		the camera's, which the first-person weapon's and the sky's change
		only in the matrix) */
		halo_vr_depth_range(frustum->z_near / vr_render.view.world_units_per_metre,
			frustum->z_far / vr_render.view.world_units_per_metre);
		memcpy(vr_render.centre, centre, sizeof(vr_render.centre));
		for (k = 0; k < 4; k++)
			vr_render.projection_depth[k] = frustum->projection_matrix[k][2];
	}
	eye_transforms_set();
}

/* render_sky.c: the sky is drawn about the camera, a thousandth of its size
(as near as a few metres): while it is, the eyes see it from the centre of
the head, as they would at infinity */
void vr_render_sky(boolean sky)
{
	if (vr_render.infinite != sky)
	{
		vr_render.infinite = sky;
		if (vr_render.active)
		{
			eye_transforms_set();
			halo_vr_sky(sky);
		}
	}
}

/* rasterizer_xbox_widgets.c: a point the game has projected to the screen
itself (a lens flare's, a sprite's), at this clip position of the centre
view (x, y, z, w), in a viewport this wide and high: what the game draws
about it in the screen moves to each eye as the point does, and scales as
the eye's view does there */
void vr_render_screen_point(const real clip[4], real width, real height)
{
	float corrections[2][4];
	short eye, axis, i;

	if (!vr_render.active || clip[3] <= 0.0f)
		return;
	for (eye = 0; eye < 2; eye++)
	{
		const float *t = vr_render.transforms[eye];
		real moved[4], scale[2], centre_screen[2], eye_screen[2];

		for (i = 0; i < 4; i++)
			moved[i] = t[i * 4] * clip[0] + t[i * 4 + 1] * clip[1] + t[i * 4 + 2] * clip[2] + t[i * 4 + 3] * clip[3];
		if (moved[3] <= 0.0f)
			return;
		for (axis = 0; axis < 2; axis++)
		{
			/* a step across the centre's screen, at the point's depth */
			const real step = 0.01f;
			real nudged = (moved[axis] + t[axis * 4 + axis] * step * clip[3]) / (moved[3] + t[12 + axis] * step * clip[3]);
			real centre_ndc = clip[axis] / clip[3], eye_ndc = moved[axis] / moved[3];

			scale[axis] = (nudged - eye_ndc) / step;
			/* (the game's screen: x = ((ndc + 1) width - 1) / 2, y from the
			top) */
			if (axis == 0)
			{
				centre_screen[0] = ((centre_ndc + 1.0f) * width - 1.0f) * 0.5f;
				eye_screen[0] = ((eye_ndc + 1.0f) * width - 1.0f) * 0.5f;
			}
			else
			{
				centre_screen[1] = ((1.0f - centre_ndc) * height - 1.0f) * 0.5f;
				eye_screen[1] = ((1.0f - eye_ndc) * height - 1.0f) * 0.5f;
			}
		}
		corrections[eye][0] = (float)scale[0];
		corrections[eye][1] = (float)scale[1];
		corrections[eye][2] = (float)(eye_screen[0] - scale[0] * centre_screen[0]);
		corrections[eye][3] = (float)(eye_screen[1] - scale[1] * centre_screen[1]);
	}
	halo_vr_set_screen_corrections(corrections);
}

/* the draws in the screen are the same in both eyes again */
void vr_render_screen_point_end(void)
{
	static const float none[2][4] = { { 1.0f, 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f, 0.0f } };

	halo_vr_set_screen_corrections(none);
}

#endif
